/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_dom(void);
extern void func_htmldoc(void);
extern void func_misc(void);
extern void func_protocol(void);
extern void func_script(void);

const struct test winetest_testlist[] =
{
    { "dom", func_dom },
    { "htmldoc", func_htmldoc },
    { "misc", func_misc },
    { "protocol", func_protocol },
    { "script", func_script },
    { 0, 0 }
};
