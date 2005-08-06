/* Automatically generated file; DO NOT EDIT!! */

/* stdarg.h is needed for Winelib */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"

extern void func_clist(void);
extern void func_ordinal(void);
extern void func_path(void);
extern void func_shreg(void);
extern void func_string(void);

struct test
{
    const char *name;
    void (*func)(void);
};

const struct test winetest_testlist[] =
{
    { "clist", func_clist },
    { "ordinal", func_ordinal },
    { "shreg", func_shreg },
    { "string", func_string },
    { 0, 0 }
};

#define WINETEST_WANT_MAIN
#include "wine/test.h"
