
#include "enews.h"

static struct dashboard_g {
    Evas_Object *bx;
    Evas_Object *sc;
} dashboard_g;
#define _G dashboard_g

void dashboard_initialize(void)
{
    _G.sc = elm_scroller_add(enews_g.win);
    evas_object_size_hint_weight_set(_G.sc,
                                     EVAS_HINT_EXPAND,
                                     0);
    evas_object_size_hint_fill_set(_G.sc, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_scroller_policy_set(_G.sc,
                            ELM_SCROLLER_POLICY_OFF,
                            ELM_SCROLLER_POLICY_AUTO);

    _G.bx = elm_box_add(enews_g.win);
    elm_box_homogeneous_set(_G.bx, EINA_FALSE);
    elm_box_horizontal_set(_G.bx, EINA_FALSE);

    evas_object_size_hint_weight_set(_G.bx,
                                     EVAS_HINT_EXPAND,
                                     0);
    //evas_object_size_hint_fill_set(_G.bx, EVAS_HINT_FILL, EVAS_HINT_FILL);

    //elm_box_pack_end(enews_g.bx, _G.sc);
    elm_win_resize_object_add(enews_g.win, _G.sc);
    elm_scroller_content_set(_G.sc, _G.bx);
    evas_object_show(_G.bx);
    evas_object_show(_G.sc);
}

void dashboard_item_add(Rss_Item *item)
{
    Evas_Object *ly;

    DBG("");

    ly = elm_layout_add(enews_g.win);

    elm_layout_file_set(ly, DATADIR"/enews/enews.edj",
                        "enews/dashboard/item");

    evas_object_size_hint_weight_set(ly,
                                     EVAS_HINT_EXPAND,
                                     0);
    evas_object_size_hint_fill_set(ly, EVAS_HINT_FILL, 0);
    evas_object_size_hint_min_set(ly, 100, 300);
    evas_object_size_hint_max_set(ly, -1, 300);

    elm_object_text_part_set(ly, "title", item->title);
    elm_object_text_part_set(ly, "content", item->description);
    DBG("item->title='%s'", item->title);
    DBG("item->description='%s'", item->description);

    elm_box_pack_end(_G.bx, ly);
    evas_object_show(ly);
}
