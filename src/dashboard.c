
#include "enews.h"

static struct dashboard_g {
    Evas_Object *bx;
} dashboard_g;
#define _G dashboard_g

void dashboard_initialize(void)
{
    _G.bx = elm_box_add(enews_g.win);
    elm_box_horizontal_set(_G.bx, EINA_FALSE);

    evas_object_size_hint_weight_set(_G.bx,
                                     EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(_G.bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_box_pack_end(enews_g.bx, _G.bx);
    evas_object_show(_G.bx);
}

void dashboard_item_add(Rss_Item *item)
{
    Evas_Object *ly;

    DBG("");

    ly = elm_layout_add(enews_g.win);

    elm_layout_file_set(ly, "data/enews.edj", "enews/dashboard/item");
    evas_object_size_hint_weight_set(ly,
                                     EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(ly, 0, 0);

    /*
    elm_object_text_part_set(ly, "title", item->title);
    elm_object_text_part_set(ly, "content", item->description);
    */
    elm_object_text_part_set(ly, "title", "item->title");
    elm_object_text_part_set(ly, "content", "item->description");

    elm_box_pack_end(enews_g.bx, ly);
    evas_object_show(ly);
}
