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
};
typedef struct {
    const char *host;
    const char *uri;
    const char *title;
    Azy_Client *cli;
} enews_src_t;

#define CURRENT_CONFIG_VERSION 0U
typedef struct {
   unsigned int version;
   Eina_List *sources;
} enews_config_t;

static struct {
    Eet_Data_Descriptor *conf_desc;
    Eet_Data_Descriptor *src_desc;
    enews_config_t *cfg;

    Evas_Object *hv;
} _G;

/* Config {{{ */

static void
_config_init(void)
{
    Eet_Data_Descriptor_Class eddc;
    char path[PATH_MAX];

    snprintf(path, sizeof(path), "%s/enews", efreet_config_home_get());
    if (!ecore_file_mkpath(path)) {
        ERR("unable to create path '%s': %m", path);
    }

    EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, enews_config_t);
    _G.conf_desc = eet_data_descriptor_stream_new(&eddc);

    EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, enews_src_t);
    _G.src_desc = eet_data_descriptor_stream_new(&eddc);

#define CFG_ADD_BASIC(member, eet_type)\
    EET_DATA_DESCRIPTOR_ADD_BASIC\
    (_G.conf_desc, enews_config_t, #member, member, eet_type)

    CFG_ADD_BASIC(version, EET_T_UINT);
    EET_DATA_DESCRIPTOR_ADD_LIST(_G.conf_desc, enews_config_t,
                                 "sources", sources, _G.src_desc);
#undef CFG_ADD_BASIC

#define SRC_ADD_BASIC(member, eet_type)\
    EET_DATA_DESCRIPTOR_ADD_BASIC\
    (_G.src_desc, enews_src_t, #member, member, eet_type)

    SRC_ADD_BASIC(host,  EET_T_STRING);
    SRC_ADD_BASIC(uri,   EET_T_STRING);
    SRC_ADD_BASIC(title, EET_T_STRING);
#undef SRC_ADD_BASIC
}

static void
_config_shutdown(void)
{
    eet_data_descriptor_free(_G.src_desc);
    eet_data_descriptor_free(_G.conf_desc);
}

static void
_config_load(void)
{
    char path[PATH_MAX];
    Eet_File *ef;

    snprintf(path, sizeof(path), "%s/enews/config.eet",
             efreet_config_home_get());
    if (access(path, R_OK)) {
        INFO("no configuration file found");
        _G.cfg = calloc(1, sizeof(enews_config_t));
        return;
    }

    ef = eet_open(path, EET_FILE_MODE_READ);
    if (!ef) {
        ERR("unable to open configuration file '%s': %m", path);
        return;
    }

    _G.cfg = eet_data_read(ef, _G.conf_desc, "config");
    if (!_G.cfg) {
        ERR("unable to read configuration file '%s': %m", path);
        goto end;
    }

    if (_G.cfg->version > CURRENT_CONFIG_VERSION) {
        ERR("unable to read configuration file: wrong version");
        goto end;
    }

end:
    eet_close(ef);
}

static int
_config_save(void)
{
    char path[PATH_MAX];
    char tmp[PATH_MAX];
    Eet_File *ef;
    int len;
    int i;

    /* XXX: can't use a tmp file, we want it on the same partition */

    len = snprintf(path, sizeof(path), "%s/enews/config.eet",
                   efreet_config_home_get());
    memcpy(tmp, path, len);
    if (len == sizeof(path)) {
        tmp[sizeof(tmp) - 1] = '\0';
    } else {
        tmp[len] = '\0';
    }

    i = 0;
    do {
        snprintf(tmp + len, sizeof(tmp) - len, ".%u", i);
        i++;
    } while (access(tmp, F_OK) == 0 || errno != ENOENT);

    ef = eet_open(tmp, EET_FILE_MODE_WRITE);
    if (!ef) {
        ERR("unable to open '%s' for writing: %m", tmp);
        return -1;
    }

    if (!eet_data_write(ef, _G.conf_desc, "config", _G.cfg, true)) {
        ERR("unable to write configuration file to '%s': %m", tmp);
        eet_close(ef);
        return -1;
    }

    eet_close(ef);

    if (rename(tmp, path)) {
        ERR("unable to rename '%s' to '%s': %m", tmp, path);
        return -1;
    }

    return 0;
}

/* }}} */
/* Network {{{ */

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
        rss_item_t *rss_item;

        rss_item = calloc(1, sizeof(rss_item_t));

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

static void
_enews_src_connect(enews_src_t *src)
{
    Azy_Net *net;
    assert(src);
    assert(!src->cli);

    DBG("src->host='%s', src->uri='%s'", src->host, src->uri);
    src->cli = azy_client_new();
    azy_client_host_set(src->cli, src->host, 80);
    azy_client_connect(src->cli, false);
    net = azy_client_net_get(src->cli);
    azy_net_uri_set(net, src->uri);
    azy_net_version_set(net, 0);
}


/* }}} */
/* Add RSS {{{ */

static void
_add_rss_widget_hide(Evas_Object *bx)
{
    if (enews_g.current_widget != ADD_RSS)
        return;
    evas_object_del(bx);

    enews_g.current_widget_hide = NULL;
    enews_g.cb_data = NULL;
    enews_g.current_widget = NONE;
}

static void
_bt_add_rss_cb(Evas_Object *entry,
               Evas_Object *obj __UNUSED__,
               void *event_info __UNUSED__)
{
    const char *addr;
    char *uri;
    enews_src_t *src;
    int len;
    /* How do i handle failures? hover-popup? */

    addr = elm_object_text_get(entry);
    if (!addr)
        return;
    if (!strncmp(addr, "http://", strlen("http://"))) {
        addr += strlen("http://");
    }

    uri = strchr(addr, '/');
    if (!uri) {
        ERR("invalid rss address '%s'", addr);
        return;
    }

    len = uri - addr;

    /* check not to add twice */
    for (Eina_List *l = _G.cfg->sources; l; l = l->next) {
        src = l->data;

        if (!strncmp(src->host, addr, len)
        &&  !strncmp(src->uri, uri, strlen(uri))) {
            return;
        }
    }

    src = calloc(1, sizeof(enews_src_t));
    src->host = malloc((len + 1) * sizeof(char));
    memcpy((char*)src->host, addr, len);
    ((char*)src->host)[len] = '\0';
    src->uri = strdup(uri);

    EINA_LIST_APPEND(_G.cfg->sources, src);

    _enews_src_connect(src);

    _config_save();

    dashboard_show();
}

static void
_tb_add_rss_cb(void *data __UNUSED__,
               Evas_Object *obj __UNUSED__,
               void *event_info __UNUSED__)
{
    Evas_Object *bx, *label, *bt, *entry, *f;

    if (enews_g.current_widget_hide)
        enews_g.current_widget_hide(enews_g.cb_data);

    bx = elm_box_add(enews_g.win);
    elm_box_homogeneous_set(bx, false);
    elm_box_horizontal_set(bx, false);
    evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_fill_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(bx);

    elm_box_pack_end(enews_g.bx, bx);

    label = elm_label_add(enews_g.win);
    elm_object_text_set(label, "Enter the URL of an RSS stream to add:");
    elm_box_pack_end(bx, label);
    evas_object_show(label);

    entry = elm_entry_add(enews_g.win);
    elm_entry_editable_set(entry, true);
    elm_entry_single_line_set(entry, true);
    evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_fill_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
    f = elm_frame_add(enews_g.win);
    elm_frame_content_set(f, entry);
    evas_object_size_hint_weight_set(f, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_fill_set(f, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(entry);
    elm_box_pack_end(bx, f);
    evas_object_show(f);

    bt = elm_button_add(enews_g.win);
    elm_object_text_set(bt, "Add RSS");
    elm_box_pack_end(bx, bt);
    evas_object_smart_callback_add(bt, "clicked",
                                   (Evas_Smart_Cb)_bt_add_rss_cb, entry);
    evas_object_show(bt);

    enews_g.current_widget_hide = (enews_hide_f)_add_rss_widget_hide;
    enews_g.cb_data = bx;
    enews_g.current_widget = ADD_RSS;
}

/* }}} */
/* Streams List {{{ */

static struct {
    Evas_Object *li;
    Evas_Object *idx;
} _streams_list_widgets;

static void
_streams_list_widget_hide(void *_ __UNUSED__)
{
    if (enews_g.current_widget != STREAMS_LIST)
        return;

    if (_streams_list_widgets.li) {
        evas_object_del(_streams_list_widgets.li);
        _streams_list_widgets.li = NULL;
    }
    if (_streams_list_widgets.idx) {
        evas_object_del(_streams_list_widgets.idx);
        _streams_list_widgets.idx = NULL;
    }

    enews_g.current_widget_hide = NULL;
    enews_g.cb_data = NULL;
    enews_g.current_widget = NONE;
}

static void
_index_changed(void *data __UNUSED__,
               Evas_Object *obj __UNUSED__,
               Elm_List_Item *item)
{
    elm_list_item_show(item);
}

static void
_tb_streams_list_cb(void *data __UNUSED__,
                    Evas_Object *obj __UNUSED__,
                    void *event_info __UNUSED__)
{
    Evas_Object *li,
                *idx;

    if (enews_g.current_widget_hide)
        enews_g.current_widget_hide(enews_g.cb_data);

    li = elm_list_add(enews_g.win);
    elm_list_always_select_mode_set(li, 1);
    evas_object_size_hint_weight_set(li, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_fill_set(li, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_box_pack_end(enews_g.bx, li);
    evas_object_show(li);

    idx = elm_index_add(enews_g.win);
    evas_object_smart_callback_add(idx, "delay,changed",
                                   (Evas_Smart_Cb)_index_changed, NULL);
    evas_object_size_hint_weight_set(idx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(enews_g.win, idx);
    evas_object_show(idx);

    for (Eina_List *l = _G.cfg->sources; l; l = l->next) {
        enews_src_t *src = l->data;
        Elm_List_Item *it;
        char letter[2] = {'\0', '\0'};
        const char *label = src->title;

        if (!src->title)
            label = src->host;
        if (!label)
            continue;

        it = elm_list_item_append(li, label, NULL, NULL, NULL, NULL);
        letter[0] = label[0];
        elm_index_item_append(idx, letter, it);
     }
    elm_index_item_go(idx, 0);
    elm_list_go(li);

    _streams_list_widgets.li = li;
    _streams_list_widgets.idx = idx;

    enews_g.current_widget_hide = (enews_hide_f)_streams_list_widget_hide;
    enews_g.cb_data = NULL;
    enews_g.current_widget = STREAMS_LIST;
}

/* }}} */
/* Toolbar {{{ */

static void
_tb_dashboard_cb(void *data __UNUSED__,
                 Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
    if (enews_g.current_widget == DASHBOARD)
        return;

    if (enews_g.current_widget_hide)
        enews_g.current_widget_hide(enews_g.cb_data);

    dashboard_show();
}

static void
_toolbar_setup(void)
{
    Elm_Toolbar_Item *item;

    enews_g.tb = elm_toolbar_add(enews_g.win);
    elm_toolbar_homogeneous_set(enews_g.tb, false);
    elm_toolbar_mode_shrink_set(enews_g.tb, ELM_TOOLBAR_SHRINK_MENU);
    evas_object_size_hint_weight_set(enews_g.tb, 0.0, 0.0);
    evas_object_size_hint_align_set(enews_g.tb, EVAS_HINT_FILL, 0.0);
    elm_box_pack_start(enews_g.bx, enews_g.tb);
    evas_object_show(enews_g.tb);

    item = elm_toolbar_item_append(enews_g.tb, "home", "Dashboard",
                                   _tb_dashboard_cb, NULL);
    item = elm_toolbar_item_append(enews_g.tb, "add", "Add RSS",
                                   _tb_add_rss_cb, NULL);
    item = elm_toolbar_item_append(enews_g.tb, "apps", "Streams",
                                   _tb_streams_list_cb, NULL);
}

/* }}} */
/* Main {{{ */

int
main(int argc, char **argv)
{
    Evas_Object *bg;

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
    elm_win_autodel_set(enews_g.win, true);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

    bg = elm_bg_add(enews_g.win);
    elm_win_resize_object_add(enews_g.win, bg);
    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(bg);

    enews_g.bx = elm_box_add(enews_g.win);
    elm_box_horizontal_set(enews_g.bx, false);
    elm_box_homogeneous_set(enews_g.bx, false);
    evas_object_size_hint_weight_set(enews_g.bx,
                                     EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_fill_set(enews_g.bx, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
    elm_win_resize_object_add(enews_g.win, enews_g.bx);
    evas_object_show(enews_g.bx);

    _toolbar_setup();

    dashboard_initialize();

    _config_init();
    _config_load();

    for (Eina_List *l = _G.cfg->sources; l; l = l->next) {
        enews_src_t *src = l->data;

        _enews_src_connect(src);
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

    _config_save();
    _config_shutdown();

    if (enews_g.log_domain >= 0) {
        eina_log_domain_unregister(enews_g.log_domain);
    }

    for (Eina_List *l = _G.cfg->sources; l; l = l->next) {
        enews_src_t *src = l->data;

        if (src->cli)
            azy_client_free(src->cli);
    }

    elm_shutdown();
    azy_shutdown();
    ecore_shutdown();
    eina_shutdown();

    return 0;
}
/* }}} */
