/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_imalloc(void);
extern void func_prop(void);
extern void func_util(void);

const struct test winetest_testlist[] =
{
    { "imalloc", func_imalloc },
    { "prop", func_prop },
    { "util", func_util },
    { 0, 0 }
};
