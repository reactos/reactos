/* Automatically generated file; DO NOT EDIT!! */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"

extern void func_class(void);
extern void func_dde(void);
extern void func_dialog(void);
//extern void func_input(void);
extern void func_listbox(void);
extern void func_msg(void);
extern void func_resource(void);
extern void func_sysparams(void);
extern void func_win(void);
extern void func_wsprintf(void);

struct test
{
    const char *name;
    void (*func)(void);
};

static const struct test winetest_testlist[] =
{
    { "class", func_class },
    { "dde", func_dde },
    { "dialog", func_dialog },
    //{ "input", func_input },
    { "listbox", func_listbox },
    { "msg", func_msg },
    { "resource", func_resource },
    { "sysparams", func_sysparams },
    { "win", func_win },
    { "wsprintf", func_wsprintf },
    { 0, 0 }
};

#define WINETEST_WANT_MAIN
#include "wine/test.h"
