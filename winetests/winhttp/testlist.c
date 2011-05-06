/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_winhttp(void);
extern void func_notification(void);
extern void func_url(void);

const struct test winetest_testlist[] =
{
	 { "winhttp", func_winhttp },
	 { "notification", func_notification },
	 { "url", func_url },
	 { 0, 0 }
};
