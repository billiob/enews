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
