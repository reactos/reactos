/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_clist(void);
extern void func_clsid(void);
extern void func_generated(void);
extern void func_ordinal(void);
extern void func_path(void);
extern void func_shreg(void);
extern void func_string(void);

const struct test winetest_testlist[] =
{
    { "clist", func_clist },
    { "clsid", func_clsid },
    { "generated", func_generated },
    { "ordinal", func_ordinal },
    { "path", func_path },
    { "shreg", func_shreg },
    { "string", func_string },
    { 0, 0 }
};
