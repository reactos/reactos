/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_actctx(void);
extern void func_atom(void);
extern void func_change(void);
extern void func_codepage(void);
extern void func_comm(void);
extern void func_console(void);
extern void func_cpu(void);
extern void func_debugger(void);
extern void func_directory(void);
extern void func_drive(void);
extern void func_environ(void);
extern void func_fiber(void);
extern void func_file(void);
extern void func_format_msg(void);
extern void func_generated(void);
extern void func_heap(void);
extern void func_loader(void);
extern void func_locale(void);
extern void func_mailslot(void);
extern void func_module(void);
extern void func_path(void);
extern void func_pipe(void);
extern void func_process(void);
extern void func_profile(void);
extern void func_resource(void);
extern void func_sync(void);
extern void func_thread(void);
extern void func_time(void);
extern void func_timer(void);
extern void func_toolhelp(void);
extern void func_version(void);
extern void func_virtual(void);
extern void func_volume(void);
/* ReactOS */
extern void func_dosdev(void);
extern void func_interlck(void);

const struct test winetest_testlist[] =
{
    { "actctx", func_actctx },
    { "atom", func_atom },
    { "change", func_change },
    { "codepage", func_codepage },
    { "comm", func_comm },
    { "console", func_console },
    /* ReactOS */
    //{ "debugger", func_debugger },
    { "directory", func_directory },
    { "drive", func_drive },
    { "environ", func_environ },
    { "fiber", func_fiber },
    { "file", func_file },
    { "format_msg", func_format_msg },
    /* ReactOS */
    //{ "generated", func_generated },
    { "heap", func_heap },
    { "loader", func_loader },
    { "locale", func_locale },
    { "mailslot", func_mailslot },
    { "module", func_module },
    { "path", func_path },
    { "pipe", func_pipe },
    { "process", func_process },
    { "profile", func_profile },
    { "resource", func_resource },
    { "sync", func_sync },
    { "thread", func_thread },
    { "time", func_time },
    { "timer", func_timer },
    { "toolhelp", func_toolhelp },
    { "version", func_version },
    { "virtual", func_virtual },
    { "volume", func_volume },
    { 0, 0 }
};
