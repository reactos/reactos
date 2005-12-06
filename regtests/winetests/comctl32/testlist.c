/* Automatically generated file; DO NOT EDIT!! */

/* stdarg.h is needed for Winelib */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"

extern void func_dpa(void);
extern void func_mru(void);
extern void func_subclass(void);
extern void func_tab(void);

struct test
{
    const char *name;
    void (*func)(void);
};

static const struct test winetest_testlist[] =
{
    { "dpa", func_dpa },
    { "mru", func_mru },
    { "subclass", func_subclass },
    { "tab", func_tab },
    { 0, 0 }
};

#define WINETEST_WANT_MAIN
#include "wine/test.h"
