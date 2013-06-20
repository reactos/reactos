/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_generated(void);
extern void func_misc(void);
extern void func_protocol(void);
extern void func_sec_mgr(void);
extern void func_stream(void);
extern void func_uri(void);
extern void func_url(void);

const struct test winetest_testlist[] =
{
    { "generated", func_generated },
    { "misc", func_misc },
    { "protocol", func_protocol },
    { "sec_mgr", func_sec_mgr },
    { "stream", func_stream },
    { "uri", func_uri },
    { "url", func_url },
    { 0, 0 }
};
