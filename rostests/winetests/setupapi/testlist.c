/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_devclass(void);
extern void func_devinst(void);
extern void func_install(void);
extern void func_misc(void);
extern void func_parser(void);
extern void func_query(void);
extern void func_stringtable(void);

const struct test winetest_testlist[] =
{
    { "devclass", func_devclass },
    { "devinst", func_devinst },
    { "install", func_install },
    { "misc", func_misc },
    { "parser", func_parser },
    { "query", func_query },
    { "stringtable", func_stringtable },
    { 0, 0 }
};
