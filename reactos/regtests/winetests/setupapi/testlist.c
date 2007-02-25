/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_devclass(void);
extern void func_install(void);
extern void func_parser(void);
extern void func_query(void);
extern void func_stringtable(void);

const struct test winetest_testlist[] =
{
    { "devclass", func_devclass },
    { "install", func_install },
    { "parser", func_parser },
    { "query", func_query },
    { "stringtable", func_stringtable },
    { 0, 0 }
};
