#ifndef E_NEWS_H
#define E_NEWS_H

#include <Azy.h>
#include <Elementary.h>

#include <stdbool.h>

#include "config.h"

#define ERR(...)  EINA_LOG_DOM_ERR(enews_g.log_domain, __VA_ARGS__)
#define DBG(...)  EINA_LOG_DOM_DBG(enews_g.log_domain, __VA_ARGS__)
#define INFO(...) EINA_LOG_DOM_INFO(enews_g.log_domain, __VA_ARGS__)
#define WARN(...) EINA_LOG_DOM_WARN(enews_g.log_domain, __VA_ARGS__)
#define CRIT(...) EINA_LOG_DOM_CRIT(enews_g.log_domain, __VA_ARGS__)

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

typedef enum enews_widget_t{
    NONE,
    DASHBOARD,
    ADD_RSS,

    ENEWS_SCREENS_COUNT
} enews_widget_t;

typedef void (*enews_hide_f)(void *data);

struct enews_g {
    int log_domain;
    Evas_Object *win,
                *bx,
                *tb,
                *dashboard;

    enews_widget_t current_widget;
    enews_hide_f current_widget_hide;
    void *cb_data;

    Rss_Ressource rss_ressources[];
};
extern struct enews_g enews_g;

/* Dashboard */
void dashboard_initialize(void);
void dashboard_item_add(const Rss_Item *item);
void dashboard_show(void);

/* Utils */
char *extract_text_from_html(const char *src);
#endif
