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

#define CURRENT_CONFIG_VERSION 0U
typedef struct {
   unsigned int version;
   Eina_List *sources; // enews_src_t
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

    for (Eina_List *l = _G.cfg->sources; l; l = l->next) {
        enews_src_init_from_conf(l->data);
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

static const char *
azy_rss_item_key_get(Azy_Rss_Item *item)
{
    const char *key;

    assert(item);

    key = azy_rss_item_guid_get(item);
    if (key)
        return key;

    key = azy_rss_item_link_get(item);
    if (key)
        return key;

    return azy_rss_item_date_get(item);
}

static Eina_Error
on_client_return(Azy_Client *cli, Azy_Content *content, void *ret)
{
    Azy_Rss *rss;
    Azy_Rss_Item *item;
    Eina_List *l;
    enews_src_t *src;

    if (azy_content_error_is_set(content)) {
        ERR("Error encountered: %s", azy_content_error_message_get(content));
        return azy_content_error_code_get(content);
    }

    DBG("cli=%p", cli);
    src = azy_client_data_get(cli);

    DBG("content='%s'", azy_content_dump_string(content, 2));

    rss = azy_content_return_get(content);
    if (!rss) {
        DBG("no content");
        return AZY_ERROR_NONE;
    }

    DBG("rss=%p", rss);

    DBG("title='%s'", azy_rss_title_get(rss));

    if (!src->title) {
        const char *title = azy_rss_title_get(rss);

        if (title && *title) {
            src->title = strdup(title);
            _config_save();
        }
    }

    EINA_LIST_FOREACH(azy_rss_items_steal(rss), l, item) {
        rss_item_t *rss_item;
        const char *description;
        const char *key;

        key = azy_rss_item_key_get(item);
        if (!key || eina_hash_find(src->items, key)) {
            azy_rss_item_free(item);
            continue;
        }

        rss_item = calloc(1, sizeof(rss_item_t));
        rss_item->src = src;

        description = azy_rss_item_desc_get(item);
        rss_item->title = azy_rss_item_title_get(item);
        rss_item->description = extract_text_from_html(description);
        /* TODO: image from <media:content> ? */

        dashboard_item_add(rss_item);

        eina_hash_add(src->items, key, rss_item);
    }

    azy_rss_free(rss);
    azy_content_free(content);

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

        azy_client_callback_set(cli, id, on_client_return);
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

    DBG("src->host='%s', src->uri='%s'", src->host, src->uri);
    if (!src->cli)
        src->cli = azy_client_new();
    azy_client_host_set(src->cli, src->host, 80);
    azy_client_connect(src->cli, false);
    net = azy_client_net_get(src->cli);
    azy_net_uri_set(net, src->uri);
    azy_net_version_set(net, 0);

    azy_client_data_set(src->cli, src);
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

    src = enews_src_new();
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
    Evas_Object *bx,
                *label,
                *bt,
                *ic,
                *entry;

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
    elm_entry_scrollable_set(entry, true);
    elm_entry_editable_set(entry, true);
    elm_entry_single_line_set(entry, true);
    evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, 0.);
    evas_object_size_hint_fill_set(entry, EVAS_HINT_FILL, 0.5);
    elm_box_pack_end(bx, entry);
    evas_object_show(entry);

    ic = elm_icon_add(enews_g.win);
    elm_icon_standard_set(ic, "add");
    evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
    bt = elm_button_add(enews_g.win);
    elm_object_content_set(bt, ic);
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

static Evas_Object *_bx_info = NULL;
static Evas_Object *_bx_streams_list = NULL;
static Evas_Object *_idx_streams = NULL;
static Evas_Object *_hv_streams = NULL;
static enews_src_t *_current_src = NULL;

static void
_streams_list_widget_hide(Evas_Object *bx)
{
    if (enews_g.current_widget != STREAMS_LIST)
        return;

    evas_object_del(bx);
    evas_object_del(_idx_streams);
    evas_object_del(_hv_streams);

    enews_g.current_widget_hide = NULL;
    enews_g.cb_data = NULL;
    enews_g.current_widget = NONE;
    _bx_info = NULL;
    _bx_streams_list = NULL;
    _idx_streams = NULL;
    _current_src = NULL;
}

static void
_bt_remove_rss_hover_cb(Evas_Object *hv,
                        Evas_Object *obj __UNUSED__,
                        void *event_info __UNUSED__)
{
    evas_object_show(hv);
}

static void
_bt_remove_rss_hover_no_cb(Evas_Object *hv,
                           Evas_Object *obj __UNUSED__,
                           void *event_info __UNUSED__)
{
    evas_object_hide(hv);
}

static void
_bt_remove_rss_hover_yes_cb(Evas_Object *hv,
                            Evas_Object *obj __UNUSED__,
                            void *event_info __UNUSED__)
{
    Evas_Object *li;
    Elm_List_Item *item;

    if (!_current_src)
        return;

    EINA_LIST_REMOVE(_G.cfg->sources, _current_src);

    li = elm_box_children_get(_bx_streams_list)->data;
    item = elm_list_selected_item_get(li);
    if (item) {
        elm_index_item_del(_idx_streams, item);
        elm_list_item_del(item);
    }


    _config_save();
    _current_src = NULL;
    evas_object_hide(hv);

    evas_object_del(hv);
    _hv_streams = NULL;
    evas_object_del(_bx_info);
    _bx_info = NULL;
}

static void
_streams_list_cb(enews_src_t *src,
                 Evas_Object *obj __UNUSED__,
                 Elm_List_Item *item __UNUSED__)
{
    if (!_bx_info) {
        Evas_Object *bt,
                    *hv,
                    *ic;

        hv = elm_hover_add(enews_g.win);

        _bx_info = elm_box_add(enews_g.win);
        elm_box_homogeneous_set(_bx_info, false);
        elm_box_horizontal_set(_bx_info, false);
        evas_object_size_hint_fill_set(_bx_info, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_pack_end(_bx_streams_list, _bx_info);
        evas_object_show(_bx_info);

        ic = elm_icon_add(enews_g.win);
        elm_icon_standard_set(ic, "delete");
        evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        bt = elm_button_add(enews_g.win);
        elm_object_content_set(bt, ic);
        elm_object_text_set(bt, "Remove RSS");
        evas_object_smart_callback_add(bt, "clicked",
                                       (Evas_Smart_Cb)_bt_remove_rss_hover_cb,
                                       hv);
        elm_hover_parent_set(hv, enews_g.win);
        elm_hover_target_set(hv, bt);
        elm_box_pack_end(_bx_info, bt);
        evas_object_show(bt);

        bt = elm_button_add(enews_g.win);
        elm_object_text_set(bt, "Yes");
        elm_hover_content_set(hv, "left", bt);
        evas_object_smart_callback_add(bt, "clicked",
                                       (Evas_Smart_Cb)_bt_remove_rss_hover_yes_cb,
                                       hv);
        evas_object_show(bt);

        bt = elm_button_add(enews_g.win);
        elm_object_text_set(bt, "No");
        elm_hover_content_set(hv, "right", bt);
        evas_object_smart_callback_add(bt, "clicked",
                                       (Evas_Smart_Cb)_bt_remove_rss_hover_no_cb,
                                       hv);
        evas_object_show(bt);

        _hv_streams = hv;
    }

    _current_src = src;
}

static void
_index_changed(Evas_Object *li __UNUSED__,
               Evas_Object *idx __UNUSED__,
               Elm_List_Item *item)
{
    elm_list_item_show(item);
    elm_list_item_selected_set(item, true);
}

static void
_list_index_move(Evas_Object *idx,
                 Evas *e __UNUSED__,
                 Evas_Object *li,
                 void *event_info __UNUSED__)
{
    Evas_Coord x, y;

    evas_object_geometry_get(li, &x, &y, NULL, NULL);
    evas_object_move(idx, x, y);
}

static void
_list_index_resize(Evas_Object *idx,
                   Evas *e __UNUSED__,
                   Evas_Object *li,
                   void *event_info __UNUSED__)
{
    Evas_Coord w, h;

    evas_object_geometry_get(li, NULL, NULL, &w, &h);
    evas_object_resize(idx, w, h);
}

static void
_tb_streams_list_cb(void *data __UNUSED__,
                    Evas_Object *obj __UNUSED__,
                    void *event_info __UNUSED__)
{
    Evas_Object *bx,
                *li,
                *idx;

    if (enews_g.current_widget_hide)
        enews_g.current_widget_hide(enews_g.cb_data);

    bx = elm_box_add(enews_g.win);
    elm_box_homogeneous_set(bx, false);
    elm_box_horizontal_set(bx, false);
    evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_fill_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(bx);

    elm_box_pack_end(enews_g.bx, bx);

    li = elm_list_add(enews_g.win);
    elm_list_always_select_mode_set(li, 1);
    evas_object_size_hint_weight_set(li, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_fill_set(li, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_box_pack_end(bx, li);
    evas_object_show(li);

    idx = elm_index_add(li);
    evas_object_smart_callback_add(idx, "delay,changed",
                                   (Evas_Smart_Cb)_index_changed, li);
    evas_object_size_hint_weight_set(idx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_event_callback_add(li, EVAS_CALLBACK_RESIZE,
                                   (Evas_Object_Event_Cb)_list_index_resize,
                                   idx);
    evas_object_event_callback_add(li, EVAS_CALLBACK_MOVE,
                                   (Evas_Object_Event_Cb)_list_index_move,
                                   idx);
    evas_object_show(idx);

    for (Eina_List *l = _G.cfg->sources; l; l = l->next) {
        enews_src_t *src = l->data;
        Elm_List_Item *it;
        char letter[2] = {'\0', '\0'};
        const char *label;

        label = enews_src_title_get(src);
        if (!label)
            continue;

        it = elm_list_item_append(li, label, NULL, NULL,
                                  (Evas_Smart_Cb)_streams_list_cb, src);
        letter[0] = label[0];
        elm_index_item_append(idx, letter, it);
    }
    elm_index_item_go(idx, 0);
    elm_list_go(li);

    enews_g.current_widget_hide = (enews_hide_f)_streams_list_widget_hide;
    enews_g.cb_data = bx;
    enews_g.current_widget = STREAMS_LIST;

    _idx_streams = idx;
    _bx_streams_list = bx;
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
