
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
    evas_object_size_hint_weight_set(enews_g.dashboard,
                                     EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_fill_set(enews_g.dashboard,
                                   EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);

    _G.bx = elm_box_add(enews_g.win);
    elm_box_homogeneous_set(_G.bx, false);
    elm_box_horizontal_set(_G.bx, false);
    elm_object_content_set(enews_g.dashboard, _G.bx);
    evas_object_show(_G.bx);

    evas_object_size_hint_weight_set(_G.bx,
                                     EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_fill_set(_G.bx, EVAS_HINT_FILL, EVAS_HINT_FILL);

    elm_box_pack_end(enews_g.bx, enews_g.dashboard);

    enews_g.current_widget_hide = _dashboard_hide;
    enews_g.cb_data = NULL;
    enews_g.current_widget = DASHBOARD;
}

void
dashboard_item_add(const rss_item_t *item)
{
    Evas_Object *edj;
    Evas_Coord w, h;

    edj = edje_object_add(evas_object_evas_get(enews_g.win));

    if (!edje_object_file_set(edj, DATADIR "/enews/enews.edj",
                         "enews/dashboard/item"))
      {
         if (!edje_object_file_set(edj, "data/theme/enews.edj",
                         "enews/dashboard/item"))
           {
              if (!edje_object_file_set(edj, "enews.edj",
                         "enews/dashboard/item"))
                CRIT("Could not load theme!");
           }
      }

    edje_object_size_min_get(edj, &w, &h);
    evas_object_size_hint_min_set(edj, w, h);
    edje_object_size_max_get(edj, &w, &h);
    evas_object_size_hint_max_set(edj, w, h);

    evas_object_size_hint_weight_set(edj, 0, 0);
    evas_object_size_hint_align_set(edj, EVAS_HINT_FILL, 0);

    edje_object_part_text_set(edj, "title", item->title);
    edje_object_part_text_set(edj, "content", item->description);

    elm_box_pack_end(_G.bx, edj);
    evas_object_show(edj);
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
