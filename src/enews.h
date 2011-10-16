#ifndef E_NEWS_H
#define E_NEWS_H

#include <Azy.h>
#include <Elementary.h>

#include <stdbool.h>

#include "config.h"

#define ERR(...) EINA_LOG_DOM_ERR(enews_g.log_domain, __VA_ARGS__)
#define DBG(...) EINA_LOG_DOM_DBG(enews_g.log_domain, __VA_ARGS__)


typedef struct _Rss_Item {
    const char *image;
    const char *title;
    const char *description;

    int pending_img_dl;
} Rss_Item;

typedef struct _Rss_Ressource {
    const char *host;
    const char *uri;
} Rss_Ressource;

struct enews_g {
    int log_domain;
    Evas_Object *win,
                *bx,
                *tb;
    Rss_Ressource rss_ressources[];
};
extern struct enews_g enews_g;

/* Dashboard */
void dashboard_initialize(void);
void dashboard_item_add(Rss_Item *item);

/* Utils */
char *extract_text_from_html(const char *src);
#endif
