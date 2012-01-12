#include "enews.h"

void
rss_item_free(rss_item_t *p)
{
    free(p->description);
    if (p->item)
        azy_rss_item_free(p->item);
    free(p);
}

/* enews_src_t {{{ */

enews_src_t *
enews_src_init_from_conf(enews_src_t *src)
{
    src->items = eina_hash_string_djb2_new((Eina_Free_Cb)rss_item_free);

    if (src->port == 0) {
        char *pos = strchr(src->host, ':');

        src->port = 80;

        if (pos) {
            char *new_host;
            size_t len = pos - src->host;

            new_host = malloc(sizeof(char) * (len + 1));
            memcpy(new_host, src->host, len);
            new_host[len] = '\0';
            src->port = atoi(pos+1);
            src->host = new_host;
        }
    }

    return src;
}

enews_src_t *
enews_src_init(enews_src_t *src)
{
    memset(src, 0, sizeof(*src));
    src->items = eina_hash_string_djb2_new((Eina_Free_Cb)rss_item_free);

    return src;
}

enews_src_t *
enews_src_new(void)
{
    return enews_src_init(malloc(sizeof(enews_src_t)));
}

enews_src_t *
enews_src_wipe(enews_src_t *src)
{
    if (src->cli)
        azy_client_free(src->cli);
    return src;
}

void
enews_src_del(enews_src_t **p)
{
    assert(p);
    if (*p) {
        enews_src_wipe(*p);
        free(*p);
        *p = NULL;
    }
}

const char *
enews_src_title_get(const enews_src_t *src)
{
    assert(src);

    if (src->title)
        return src->title;
    if (src->host)
        return src->host;

    return NULL;
}

/* }}} */

/* webkit {{{ */

static void
_web_widget_hide(Evas_Object *web)
{
    evas_object_del(web);

    enews_g.current_widget_hide = NULL;
    enews_g.cb_data = NULL;
    enews_g.current_widget = NONE;
}

static void rss_item_show_web(const rss_item_t *item)
{
    Evas_Object *web;
    Elm_Object_Item *tb_item;

    if (enews_g.current_widget_hide)
        enews_g.current_widget_hide(enews_g.cb_data);

    tb_item = elm_toolbar_selected_item_get(enews_g.tb);
    if (tb_item)
        elm_toolbar_item_selected_set(tb_item, false);

    web = elm_web_add(enews_g.win);
    DBG("displaying '%s'", item->url);
    elm_web_uri_set(web, item->url);
    EXPAND(web);
    FILL(web);
    evas_object_show(web);

    elm_box_pack_end(enews_g.bx, web);

    enews_g.current_widget_hide = (enews_hide_f)_web_widget_hide;
    enews_g.cb_data = web;
    enews_g.current_widget = WEB;
}

/* }}} */

void rss_item_show(const rss_item_t *item)
{
    if (enews_g.has_elm_web) { // TODO: and conf to use webkit
        rss_item_show_web(item);
    } else {
        /* TODO: use xdg */
    }
}
