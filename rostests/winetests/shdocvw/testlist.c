/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_intshcut(void);
extern void func_shdocvw(void);
extern void func_shortcut(void);
extern void func_protocol(void);
extern void func_webbrowser(void);

const struct test winetest_testlist[] =
{
    { "intshcut", func_intshcut },
    { "shdocvw", func_shdocvw },
    { "shortcut", func_shortcut },
    { "webbrowser", func_webbrowser },
    { 0, 0 }
};
