
#include "enews.h"

static struct dashboard_g {
    Evas_Object *bx;
    Evas_Object *sc;
} dashboard_g;
#define _G dashboard_g

void dashboard_initialize(void)
{
    _G.sc = elm_scroller_add(enews_g.win);
    elm_scroller_policy_set(_G.sc,
                            ELM_SCROLLER_POLICY_OFF,
                            ELM_SCROLLER_POLICY_AUTO);
    evas_object_show(_G.sc);
    evas_object_size_hint_weight_set(_G.sc,
                                     EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_fill_set(_G.sc, EVAS_HINT_FILL, EVAS_HINT_FILL);

    _G.bx = elm_box_add(enews_g.win);
    elm_box_homogeneous_set(_G.bx, EINA_FALSE);
    elm_box_horizontal_set(_G.bx, EINA_FALSE);
    elm_scroller_content_set(_G.sc, _G.bx);
    evas_object_show(_G.bx);

    evas_object_size_hint_weight_set(_G.bx,
                                     EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_fill_set(_G.bx, EVAS_HINT_FILL, EVAS_HINT_FILL);

    elm_box_pack_end(enews_g.bx, _G.sc);
}

void dashboard_item_add(Rss_Item *item)
{
    Evas_Object *edj;
    Evas_Coord w, h;

    DBG("");

    edj = edje_object_add(evas_object_evas_get(enews_g.win));

    edje_object_file_set(edj, DATADIR"/enews/enews.edj",
                         "enews/dashboard/item");

    edje_object_size_min_get(edj, &w, &h);
    evas_object_size_hint_min_set(edj, w, h);
    edje_object_size_max_get(edj, &w, &h);
    evas_object_size_hint_max_set(edj, w, h);

    evas_object_size_hint_weight_set(edj,
                                     0,
                                     0);
    evas_object_size_hint_align_set(edj, EVAS_HINT_FILL, 0);

    /*
    elm_object_text_part_set(ly, "title", item->title);
    elm_object_text_part_set(ly, "content", item->description);
    DBG("item->title='%s'", item->title);
    DBG("item->description='%s'", item->description);
    */

    elm_box_pack_end(_G.bx, edj);
    evas_object_show(edj);
}
