
#include "enews.h"


static const char *
_skip_spaces(const char *str)
{
    for(; *str && isspace(*str); str++) {}

    return str;
}

static const char *
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
_skip_till_end_comment(const char *str)
{
    char *found = strstr(str, "-->");

    if (found)
        str = found + strlen("-->");

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
        if (*src == '<') {
            const char * const html_tags[] = { "a", "abbr", "acronym",
                "address", "area", "b", "base", "bdo", "big", "blockquote",
                "body", "br", "button", "caption", "cite", "code", "col",
                "colgroup", "dd", "del", "dfn", "div", "dl", "DOCTYPE", "dt",
                "em", "fieldset", "form", "h1", "h2", "h3", "h4", "h5", "h6",
                "head", "html", "hr", "i", "img", "input", "ins", "kbd",
                "label", "legend", "li", "link", "map", "meta", "noscript",
                "object", "ol", "optgroup", "option", "p", "param", "pre",
                "q", "samp", "script", "select", "small", "span", "strong",
                "style", "sub", "sup", "table", "tbody", "td", "textarea",
                "tfoot", "th", "thead", "title", "tr", "tt", "ul", "var",
                "!--", NULL};

            eina_strbuf_append_length(sb, old_pos, src - old_pos);
            src = _skip_spaces(++src);
            if (*src == '/') {
                src++;
            }

            for (int i = 0; html_tags[i]; i++) {
                if (!strncasecmp(html_tags[i], src, strlen(html_tags[i])) &&
                    !isalpha(*(src + strlen(html_tags[i])))) {

                    src += strlen(html_tags[i]);
                    if (!strncmp(html_tags[i], "!--", strlen("!--"))) {
                        src = _skip_till_end_comment(src);
                        break;
                    }
                    src = _skip_till_end_html_mark(src);
                    if (!strncmp(html_tags[i], "br", strlen("br")))
                        eina_strbuf_append_char(sb, '\n');
                    if (!strncmp(html_tags[i], "p", strlen("p")))
                        eina_strbuf_append_char(sb, ' ');
                    break;
                }
            }
            old_pos = src;
        } else {
            src++;
        }
    }

    eina_strbuf_append_length(sb, old_pos, src - old_pos);

    res = eina_strbuf_string_steal(sb);

    eina_strbuf_free(sb);
    return res;
}
