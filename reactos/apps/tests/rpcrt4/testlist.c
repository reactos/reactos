/* Automatically generated file; DO NOT EDIT!! */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"

extern void func_rpc(void);

struct test
{
    const char *name;
    void (*func)(void);
};

static const struct test winetest_testlist[] =
{
    { "rpc", func_rpc },
    { 0, 0 }
};

#define WINETEST_WANT_MAIN
#include "wine/test.h"
