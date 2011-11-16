#ifndef E_NEWS_H
#define E_NEWS_H

#include <Azy.h>
#include <Elementary.h>

#include <stdbool.h>

#include <assert.h>

#include "config.h"

#define ERR(...)  EINA_LOG_DOM_ERR(enews_g.log_domain, __VA_ARGS__)
#define DBG(...)  EINA_LOG_DOM_DBG(enews_g.log_domain, __VA_ARGS__)
#define INFO(...) EINA_LOG_DOM_INFO(enews_g.log_domain, __VA_ARGS__)
#define WARN(...) EINA_LOG_DOM_WARN(enews_g.log_domain, __VA_ARGS__)
#define CRIT(...) EINA_LOG_DOM_CRIT(enews_g.log_domain, __VA_ARGS__)

typedef struct {
    const char *host;
    const char *uri;
    const char *title;
    Azy_Client *cli;
    Eina_Hash  *items; // rss_item_t
} enews_src_t;

enews_src_t *enews_src_init_from_conf(enews_src_t *src);
enews_src_t *enews_src_init(enews_src_t *src);
enews_src_t *enews_src_new(void);
enews_src_t *enews_src_wipe(enews_src_t *src);
void enews_src_del(enews_src_t **p);
const char* enews_src_title_get(const enews_src_t *src);

typedef struct rss_item_t {
    const char *image;
    const char *title;
    char *description;

    Azy_Rss_Item *item;

    enews_src_t *src;

    int pending_img_dl;
} rss_item_t;

void rss_item_free(rss_item_t *p);
typedef enum enews_widget_t {
    NONE,
    DASHBOARD,
    ADD_RSS,
    STREAMS_LIST,

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
};
extern struct enews_g enews_g;

/* Dashboard */
void dashboard_initialize(void);
void dashboard_item_add(const rss_item_t *item);
void dashboard_show(void);

/* Utils */
char *extract_text_from_html(const char *src);

#define EINA_LIST_IS_IN(_list, _el) \
    (eina_list_data_find(_list, _el) == _el)
#define EINA_LIST_APPEND(_list, _el) \
    _list = eina_list_append(_list, _el)
#define EINA_LIST_REMOVE(_list, _el) \
    _list = eina_list_remove(_list, _el)

#endif
