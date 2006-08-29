/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_class(void);
extern void func_clipboard(void);
extern void func_dce(void);
extern void func_dde(void);
extern void func_dialog(void);
extern void func_edit(void);
extern void func_generated(void);
extern void func_input(void);
extern void func_listbox(void);
extern void func_menu(void);
extern void func_monitor(void);
extern void func_msg(void);
extern void func_resource(void);
extern void func_sysparams(void);
extern void func_text(void);
extern void func_win(void);
extern void func_winstation(void);
extern void func_wsprintf(void);

const struct test winetest_testlist[] =
{
    { "class", func_class },
    { "clipboard", func_clipboard },
    { "dce", func_dce },
    { "dde", func_dde },
    { "dialog", func_dialog },
    { "edit", func_edit },
//    { "generated", func_generated },
    { "input", func_input },
    { "listbox", func_listbox },
    { "menu", func_menu },
    { "monitor", func_monitor },
    { "msg", func_msg },
    { "resource", func_resource },
    { "sysparams", func_sysparams },
    { "text", func_text },
    { "win", func_win },
    { "winstation", func_winstation },
    { "wsprintf", func_wsprintf },
    { 0, 0 }
};
