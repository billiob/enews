/*
 * Copyright 2011 Nicolas Aguirre <aguirre.nicolas@gmail.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, version 2.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Public Licence with this file;
 * if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "enews.h"

struct enews_g enews_g = {
    .log_domain = -1,
    .rss_ressources = {
        //{"http://www.linuxdevices.com", "/backend/headlines.rss"},
        {"http://www.engadget.com", "/rss.xml"},
        //{"http://feeds.arstechnica.com", "/arstechnica/index/"},
        //{"http://feeds2.feedburner.com", "/LeJournalduGeek"},
        //{"http://www.lemonde.fr", "/rss/une.xml"},
        //{"http://rss.feedsportal.com", "/c/499/f/413823/index.rss"},
        {NULL, NULL}
    }
};

static Eina_Error
on_client_return(void *data , int type , Azy_Content *content)
{
    Azy_Rss *rss;
    Azy_Rss_Item *it;
    Eina_List *l;
    int i = 0;

    if (azy_content_error_is_set(content)) {
        ERR("Error encountered: %s", azy_content_error_message_get(content));
        return azy_content_error_code_get(content);
    }

    DBG("type=%d", type);
    DBG("content='%s'", azy_content_dump_string(content, 2));

    rss = azy_content_return_get(content);
    if (!rss) {
        DBG("no content");
        return AZY_ERROR_NONE;
    }

    DBG("rss=%p", rss);

    DBG("title='%s'", azy_rss_title_get(rss));

    EINA_LIST_FOREACH(azy_rss_items_get(rss), l, it) {
        Rss_Item *rss_item;

        rss_item = calloc(1, sizeof(Rss_Item));

        rss_item->description = extract_text_from_html(azy_rss_item_desc_get(it));
        rss_item->title = azy_rss_item_title_get(it);
        /* TODO: image from <media:content> ? */

        i++;

        dashboard_item_add(rss_item);
        /* TODO: free rss_item and item->description, and it?? */
    }

    return AZY_ERROR_NONE;
}

static Eina_Bool
on_disconnection(void *data , int type , Azy_Client *cli)
{
    DBG("cli=%p", cli);
    return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
on_connection(void *data , int type , Azy_Client *cli)
{
    Azy_Client_Call_Id id;

    DBG("cli=%p", cli);

    if (!azy_client_current(cli)) {
        id = azy_client_blank(cli, AZY_NET_TYPE_GET, NULL, NULL, NULL);
        EINA_SAFETY_ON_TRUE_RETURN_VAL(!id, ECORE_CALLBACK_CANCEL);
        //azy_client_callback_free_set(cli, id, (Ecore_Cb)azy_rss_free);
    } else {
        ERR("not current cli?!");
    }

    return ECORE_CALLBACK_RENEW;
}


#if 0
static Evas_Object *
gl_icon_get(void *data , Evas_Object *obj, const char *part)
{
    Evas_Object *ic;
    Azy_Rss *rss = data;
    Eina_List *l;
    Azy_Rss_Item *it;
    int i = 0;

    if (strcmp("elm.swallow.icon", part))
        return NULL;

    ic = elm_gengrid_add(obj);
    elm_gengrid_item_size_set(ic, 160, 160);
    elm_gengrid_multi_select_set(ic, EINA_FALSE);
    elm_gengrid_bounce_set(ic, EINA_TRUE, EINA_FALSE);
    evas_object_size_hint_weight_set(ic, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(ic, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_gengrid_horizontal_set(ic, EINA_TRUE);

    EINA_LIST_FOREACH(azy_rss_items_get(rss), l, it) {
        const char *http_image;
        Rss_Item *rss_item;
        char *tmp;

        rss_item = calloc(1, sizeof(Rss_Item));

        rss_item->git = elm_gengrid_item_append(ic, &grid_itc, rss_item, NULL, NULL);
        tmp = strdup(azy_rss_item_desc_get(it));
        http_image = _find_http_image(tmp);
        rss_item->description = eina_stringshare_add(tmp);
        rss_item->title = azy_rss_item_title_get(it);
        free(tmp);

        if (http_image) {
            char dir[4096];

            /*TODO: cleanup path */
            snprintf(dir, sizeof(dir), "%s/enews/%s/",
                     efreet_cache_home_get(), azy_rss_title_get(rss));
            if (!ecore_file_mkpath(dir)) {
                ERR("can not create dir '%s': %m", dir);
            }

            rss_item->image = eina_stringshare_printf("%s%d.jpg", dir, i);
            ecore_file_unlink(rss_item->image);
            ecore_file_download(http_image, rss_item->image, _http_img_dl_cb,
                                NULL, rss_item->git, NULL);
        }

        i++;
    }

    return ic;
}
#endif

int
main(int argc, char **argv)
{
    Evas_Object *bg;
    Azy_Client *cli = NULL;

    eina_init();
    ecore_init();
    azy_init();

    enews_g.log_domain = eina_log_domain_register("enews", NULL);
    if (enews_g.log_domain < 0) {
        EINA_LOG_CRIT("could not register log domain 'enews'");
    }

    elm_init(argc, argv);


    enews_g.win = elm_win_add(NULL, "Enews RSS Reader", ELM_WIN_BASIC);
    elm_win_title_set(enews_g.win, "Enews");
    elm_win_autodel_set(enews_g.win, EINA_TRUE);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

    bg = elm_bg_add(enews_g.win);
    elm_win_resize_object_add(enews_g.win, bg);
    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(bg);

    enews_g.bx = elm_box_add(enews_g.win);
    elm_box_horizontal_set(enews_g.bx, EINA_FALSE);
    elm_box_homogeneous_set(enews_g.bx, EINA_FALSE);
    evas_object_size_hint_weight_set(enews_g.bx,
                                     EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_fill_set(enews_g.bx, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
    elm_win_resize_object_add(enews_g.win, enews_g.bx);
    evas_object_show(enews_g.bx);

    dashboard_initialize();

    for (int i = 0; enews_g.rss_ressources[i].host; i++) {
        cli = azy_client_new();
        DBG("add cli=%p", cli);
        azy_client_host_set(cli,  enews_g.rss_ressources[i].host, 80);
        azy_client_connect(cli, EINA_FALSE);
        azy_net_uri_set(azy_client_net_get(cli),
                        enews_g.rss_ressources[i].uri);
        azy_net_version_set(azy_client_net_get(cli), 0);
    }

    ecore_event_handler_add(AZY_CLIENT_CONNECTED,
                            (Ecore_Event_Handler_Cb)on_connection,
                            NULL);
    ecore_event_handler_add(AZY_CLIENT_RETURN,
                            (Ecore_Event_Handler_Cb)on_client_return,
                            NULL);
    ecore_event_handler_add(AZY_CLIENT_DISCONNECTED,
                            (Ecore_Event_Handler_Cb)on_disconnection,
                            NULL);

    evas_object_resize(enews_g.win, 480, 800);
    evas_object_show(enews_g.win);

    elm_run();

    if (enews_g.log_domain >= 0) {
        eina_log_domain_unregister(enews_g.log_domain);
    }

    /* TODO: free all azy_clients */
    azy_client_free(cli);

    elm_shutdown();
    azy_shutdown();
    ecore_shutdown();
    eina_shutdown();

    return 0;
}

