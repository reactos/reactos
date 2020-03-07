/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_ftp(void);
#ifdef RUN_COMPILE_TIME_ONLY_GENERATED
extern void func_generated(void);
#endif
extern void func_http(void);
extern void func_internet(void);
extern void func_url(void);
extern void func_urlcache(void);

const struct test winetest_testlist[] =
{
    { "ftp", func_ftp },
#ifdef RUN_COMPILE_TIME_ONLY_GENERATED
    { "generated", func_generated },
#endif
    { "http", func_http },
    { "internet", func_internet },
    { "url", func_url },
    { "urlcache", func_urlcache },
    { 0, 0 }
};
