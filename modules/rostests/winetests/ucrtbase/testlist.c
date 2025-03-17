/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_cpp(void);
extern void func_environ(void);
extern void func_file(void);
extern void func_misc(void);
extern void func_printf(void);
extern void func_scanf(void);
extern void func_string(void);
extern void func_thread(void);


const struct test winetest_testlist[] =
{
    { "cpp", func_cpp },
    { "environ", func_environ },
    { "file", func_file },
    { "misc", func_misc },
    { "printf", func_printf },
    { "scanf", func_scanf },
    { "string", func_string },
    { "thread", func_thread },

    { 0, 0 }
};
