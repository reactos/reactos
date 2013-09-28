/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_ftp(void);
extern void func_generated(void);
extern void func_http(void);
extern void func_internet(void);
extern void func_url(void);
extern void func_urlcache(void);

const struct test winetest_testlist[] =
{
    { "ftp", func_ftp },
    { "generated", func_generated },
    { "http", func_http },
    { "internet", func_internet },
    { "url", func_url },
    { "urlcache", func_urlcache },
    { 0, 0 }
};
