/* Automatically generated file; DO NOT EDIT!! */

/* stdarg.h is needed for Winelib */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"

extern void func_shelllink(void);
extern void func_shellpath(void);
extern void func_shlexec(void);
extern void func_shlfileop(void);
extern void func_shlfolder(void);
extern void func_string(void);

struct test
{
    const char *name;
    void (*func)(void);
};


const struct test winetest_testlist[] =
{
    { "shelllink", func_shelllink },
    { "shellpath", func_shellpath },
    { "shlexec", func_shlexec },
    { "shlfileop", func_shlfileop },
    { "shlfolder", func_shlfolder },
    { "string", func_string },
    { 0, 0 }
};

#define WINETEST_WANT_MAIN
#include "wine/test.h"
