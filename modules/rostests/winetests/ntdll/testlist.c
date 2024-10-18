/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_atom(void);
extern void func_change(void);
extern void func_directory(void);
extern void func_env(void);
extern void func_error(void);
extern void func_exception(void);
extern void func_file(void);
extern void func_generated(void);
extern void func_info(void);
extern void func_large_int(void);
extern void func_om(void);
extern void func_path(void);
extern void func_pipe(void);
extern void func_port(void);
extern void func_process(void);
extern void func_reg(void);
extern void func_rtl(void);
extern void func_rtlbitmap(void);
extern void func_rtlstr(void);
extern void func_string(void);
extern void func_threadpool(void);
extern void func_time(void);

const struct test winetest_testlist[] =
{
    { "atom", func_atom },
    { "change", func_change },
    { "directory", func_directory },
    { "env", func_env },
    { "error", func_error },
    { "exception", func_exception },
    { "file", func_file },
#ifdef _M_IX86
    { "generated", func_generated },
#endif
    { "info", func_info },
    { "large_int", func_large_int },
    { "om", func_om },
    { "path", func_path },
    { "pipe", func_pipe },
    { "port", func_port },
    { "process", func_process },
    { "reg", func_reg },
    { "rtl", func_rtl },
    { "rtlbitmap", func_rtlbitmap },
    { "rtlstr", func_rtlstr },
    { "string", func_string },
    { "threadpool", func_threadpool},
    { "time", func_time },
    { 0, 0 }
};
