#ifndef E_NEWS_H
#define E_NEWS_H

#include "config.h"

struct enews_g {
    int log_domain;
};
extern struct enews_g enews_g;

#define ERR(...) EINA_LOG_DOM_ERR(enews_g.log_domain, __VA_ARGS__)
#define DBG(...) EINA_LOG_DOM_DBG(enews_g.log_domain, __VA_ARGS__)

#endif
