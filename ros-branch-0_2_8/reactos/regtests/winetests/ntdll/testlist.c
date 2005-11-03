/* Automatically generated file; DO NOT EDIT!! */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include <windows.h>

extern void func_atom(void);
extern void func_env(void);
extern void func_error(void);
extern void func_info(void);
extern void func_large_int(void);
extern void func_path(void);
extern void func_reg(void);
extern void func_rtl(void);
extern void func_rtlbitmap(void);
extern void func_rtlstr(void);
extern void func_string(void);
extern void func_time(void);

struct test
{
    const char *name;
    void (*func)(void);
};


const struct test winetest_testlist[] =
{
    { "atom", func_atom },
    { "env", func_env },
    { "error", func_error },
    { "info", func_info },
    { "large_int", func_large_int },
    { "path", func_path },
    { "reg", func_reg },
    { "rtl", func_rtl },
    { "rtlbitmap", func_rtlbitmap },
    { "rtlstr", func_rtlstr },
    { "string", func_string },
    { "time", func_time },
    { 0, 0 }
};

#define WINETEST_WANT_MAIN
#include "wine/test.h"

