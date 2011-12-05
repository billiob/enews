
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

void
dashboard_item_add(const rss_item_t *item)
{
    Evas_Object *ly;

    ly = elm_layout_add(enews_g.win);
    elm_layout_file_set(ly, DATADIR"/enews/enews.edj",
                        "enews/dashboard/item");

    ALIGN(ly, EVAS_HINT_FILL, 0);

    elm_object_part_text_set(ly, "title", item->title);
    elm_object_part_text_set(ly, "content", item->description);

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
