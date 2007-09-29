/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#include "str_util.h"

#ifndef DEBUG
#define DEBUG 1
#endif

#define LAST_TXT "last"
void str_util_test(void)
{
    char    buf[256];
    char *  tmp;

    assert(!str_endswith(NULL, NULL));
    assert(!str_endswith(NULL, "foo"));
    assert(!str_endswith("bar", NULL));
    assert(!str_endswith("bar", "baru"));
    assert(str_endswith("whammy", "whammy"));
    assert(str_endswith("whammy", "hammy"));
    assert(str_endswith("whammy", "y"));
    assert(str_endswith("whmmy", ""));
    str_copy(buf, sizeof(buf), LAST_TXT);
    str_strip_left(buf, "zot");
    assert(str_eq(buf, LAST_TXT));
    str_strip_right(buf, "zpo");
    assert(str_eq(buf, LAST_TXT));
    str_copy(buf, sizeof(buf), " \n last ");
    str_strip_left(buf, " \n");
    assert(str_eq(buf, "last "));
    str_strip_right(buf, " \n");
    assert(str_eq(buf, LAST_TXT));
    str_copy(buf, sizeof(buf), LAST_TXT);
    str_strip_left(buf, LAST_TXT);
    assert(0 == buf[0]);
    str_copy(buf, sizeof(buf), LAST_TXT);
    str_strip_right(buf, LAST_TXT);
    assert(0 == buf[0]);
    str_copy(buf, sizeof(buf), "\x0d\x0a");
    tmp = str_normalize_newline(buf, UNIX_NEWLINE);
    assert(str_eq(tmp, UNIX_NEWLINE));
    free((void*)tmp);
    tmp = NULL;
}
