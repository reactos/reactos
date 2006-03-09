/* Automatically generated file; DO NOT EDIT!! */

/* stdarg.h is needed for Winelib */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"

extern void func_cpp(void);
extern void func_environ(void);
extern void func_file(void);
extern void func_heap(void);
extern void func_printf(void);
extern void func_scanf(void);
extern void func_string(void);
extern void func_time(void);

struct test
{
    const char *name;
    void (*func)(void);
};

static const struct test winetest_testlist[] =
{
    { "cpp", func_cpp },
    { "environ", func_environ },
    { "file", func_file },
    { "heap", func_heap },
    { "printf", func_printf },
    { "scanf", func_scanf },
    { "string", func_string },
    { "time", func_time },
    { 0, 0 }
};

#define WINETEST_WANT_MAIN
#include "wine/test.h"
