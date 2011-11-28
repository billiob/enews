
#include "enews.h"

static const char *
__attribute__((pure))
_skip_spaces(const char *str)
{
    for(; *str && isspace(*str); str++) {}

    return str;
}

static const char *
__attribute__((pure))
_skip_till_end_html_mark(const char *str)
{
    while (*str && *str != '>') {
        switch (*str) {
          case '\'':
            for(; *str && *str != '\''; str++) {}
            break;
          case '\"':
            for(; *str && *str != '\"'; str++) {}
            break;
          default:
            break;
        }
        str++;
    }
    if (*str)
        str++;

    return str;
}

static const char *
__attribute__((pure))
_skip_till_end_comment(const char *str)
{
    char *found = strstr(str, "-->");

    if (found)
        str = found + sizeof("-->") - 1;

    return str;
}

char *extract_text_from_html(const char *src)
{
    Eina_Strbuf *sb;
    const char *old_pos = src;
    char *res;

    if (!src)
        return NULL;

    sb = eina_strbuf_new();

    while (*src) {
        const char * const html_tags[] = { "a", "abbr", "acronym",
            "address", "area", "b", "base", "bdo", "big", "blockquote",
            "body", "br", "button", "caption", "cite", "code", "col",
            "colgroup", "dd", "del", "dfn", "div", "dl", "DOCTYPE", "dt",
            "em", "embed", "fieldset", "form", "h1", "h2", "h3", "h4",
            "h5", "h6", "head", "html", "hr", "i", "iframe", "img",
            "input", "ins", "kbd", "label", "legend", "li", "link", "map",
            "meta", "noscript", "object", "ol", "optgroup", "option", "p",
            "param", "pre", "q", "samp", "script", "select", "small",
            "span", "strong", "style", "sub", "sup", "table", "tbody",
            "td", "textarea", "tfoot", "th", "thead", "title", "tr", "tt",
            "ul", "var", "!--", NULL};

        if (*src != '<') {
            src++;
            continue;
        }

        eina_strbuf_append_length(sb, old_pos, src - old_pos);
        src = _skip_spaces(++src);
        if (*src == '/') {
            src++;
        }

        for (int i = 0; html_tags[i]; i++) {
            size_t len = strlen(html_tags[i]);
            if (!strncasecmp(html_tags[i], src, len) &&
                !isalpha(*(src + len))) {

                src += len;
                if (!strncmp(html_tags[i], "!--", sizeof("!--") - 1)) {
                    src = _skip_till_end_comment(src);
                    break;
                }
                src = _skip_till_end_html_mark(src);
                if (!strncmp(html_tags[i], "br", sizeof("br") - 1))
                    eina_strbuf_append_char(sb, '\n');
                if (!strncmp(html_tags[i], "p", sizeof("p") - 1))
                    eina_strbuf_append_char(sb, ' ');
                break;
            }
        }
        old_pos = src;
    }

    eina_strbuf_append_length(sb, old_pos, src - old_pos);

    eina_strbuf_trim(sb);

    res = eina_strbuf_string_steal(sb);

    eina_strbuf_free(sb);
    return res;
}
