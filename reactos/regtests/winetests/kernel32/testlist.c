/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include "wine/test.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include <windows.h>

extern void func_alloc(void);
extern void func_atom(void);
extern void func_change(void);
extern void func_codepage(void);
extern void func_comm(void);
extern void func_console(void);
extern void func_directory(void);
extern void func_drive(void);
extern void func_environ(void);
extern void func_file(void);
extern void func_format_msg(void);
extern void func_heap(void);
extern void func_interlck(void);
extern void func_locale(void);
extern void func_module(void);
extern void func_mailslot(void);
extern void func_path(void);
extern void func_pipe(void);
extern void func_process(void);
extern void func_profile(void);
extern void func_sync(void);
extern void func_thread(void);
extern void func_time(void);
extern void func_timer(void);
extern void func_virtual(void);

const struct test winetest_testlist[] =
{
    { "alloc", func_alloc },
    { "atom", func_atom },
    { "change", func_change },
    { "codepage", func_codepage },
    { "comm", func_comm },
    { "console", func_console },
    { "directory", func_directory },
    { "drive", func_drive },
    { "environ", func_environ },
    { "file", func_file },
    { "format_msg", func_format_msg },
    { "heap", func_heap },
    { "interlck", func_interlck },
    { "locale", func_locale },
    { "module", func_module },
    { "mailslot", func_mailslot },
    { "path", func_path },
    { "pipe", func_pipe },
    { "process", func_process },
    { "profile", func_profile },
    { "sync", func_sync },
    { "thread", func_thread },
    { "time", func_time },
    { "timer", func_timer },
    { "virtual", func_virtual },
    { 0, 0 }
};

