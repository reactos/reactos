/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_generated(void);
extern void func_misc(void);
extern void func_protocol(void);
extern void func_stream(void);
extern void func_url(void);

const struct test winetest_testlist[] =
{
	{ "generated", func_generated },
	{ "misc", func_misc },
	{ "protocol", func_protocol },
	{ "stream", func_stream },
	{ "url", func_url },
    { 0, 0 }
};
