
#include "enews.h"

static struct dashboard_g {
    Evas_Object *bx;
} _G;

static void
_dashboard_hide(void *data __UNUSED__)
{
    if (enews_g.current_widget != DASHBOARD)
        return;
    evas_object_hide(enews_g.dashboard);
    elm_box_unpack(enews_g.bx, enews_g.dashboard);

    enews_g.current_widget_hide = NULL;
    enews_g.cb_data = NULL;
    enews_g.current_widget = NONE;
}

void
dashboard_initialize(void)
{
    enews_g.dashboard = elm_scroller_add(enews_g.win);
    elm_scroller_policy_set(enews_g.dashboard,
                            ELM_SCROLLER_POLICY_OFF,
                            ELM_SCROLLER_POLICY_AUTO);
    evas_object_show(enews_g.dashboard);
    EXPAND(enews_g.dashboard);
    FILL(enews_g.dashboard);
    _G.bx = elm_box_add(enews_g.win);
    elm_box_homogeneous_set(_G.bx, false);
    elm_box_horizontal_set(_G.bx, false);
    elm_object_content_set(enews_g.dashboard, _G.bx);
    evas_object_show(_G.bx);

    EXPAND(_G.bx);
    FILL(_G.bx);
    elm_box_pack_end(enews_g.bx, enews_g.dashboard);

    enews_g.current_widget_hide = _dashboard_hide;
    enews_g.cb_data = NULL;
    enews_g.current_widget = DASHBOARD;
}

static void
_closed_cb(const rss_item_t *item,
           Evas_Object *obj,
           void *event_info __UNUSED__)
{
    Evas_Object *ly = elm_object_parent_widget_get(obj);
    evas_object_del(ly);

    /* TODO: mark rss_item as read or whatever needed */
}

static void
_read_cb(const rss_item_t *item,
         Evas_Object *ly,
         const char  *emission,
         const char  *source)
{
    /* TODO: mark rss_item as read or whatever needed */
    DBG("title: %s", item->title);
    evas_object_del(ly);
    rss_item_show(item);
}

static void
_host_cb(const rss_item_t *item,
         Evas_Object *ly,
         const char  *emission,
         const char  *source)
{
    /* TODO: show a dashboard with only items from that stream */
}

void
dashboard_item_add(const rss_item_t *item)
{
    Evas_Object *ly,
                *ic,
                *edj;

    ly = elm_layout_add(enews_g.win);
    elm_layout_file_set(ly, DATADIR"/enews/enews.edj",
                        "enews/dashboard/item");

    ALIGN(ly, EVAS_HINT_FILL, 0);

    elm_object_part_text_set(ly, "host", enews_src_title_get(item->src));
    elm_object_part_text_set(ly, "title", item->title);
    elm_object_part_text_set(ly, "content", item->description);

    ic = elm_icon_add(enews_g.win);
    elm_icon_standard_set(ic, "close");
    elm_object_part_content_set(ly, "hide", ic);
    evas_object_show(ic);
    evas_object_smart_callback_add(ic, "clicked",
                                   (Evas_Smart_Cb)_closed_cb, item);

    edj = elm_layout_edje_get(ly);
    edje_object_signal_callback_add(edj, "mouse,up,1", "title",
                                   (Edje_Signal_Cb)_read_cb, (void *)item);
    edje_object_signal_callback_add(edj, "mouse,up,1", "content",
                                   (Edje_Signal_Cb)_read_cb, (void *)item);
    edje_object_signal_callback_add(edj, "mouse,up,1", "host",
                                   (Edje_Signal_Cb)_host_cb, (void *)item);

    elm_box_pack_end(_G.bx, ly);
    evas_object_show(ly);
}

void
dashboard_show(void)
{
    if (enews_g.current_widget == DASHBOARD)
        return;

    if (enews_g.current_widget_hide)
        enews_g.current_widget_hide(enews_g.cb_data);

    elm_box_pack_end(enews_g.bx, enews_g.dashboard);
    evas_object_show(enews_g.dashboard);

    enews_g.current_widget_hide = _dashboard_hide;
    enews_g.cb_data = NULL;
    enews_g.current_widget = DASHBOARD;
}
