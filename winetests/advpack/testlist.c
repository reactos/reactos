/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_advpack(void);
extern void func_files(void);
extern void func_install(void);

const struct test winetest_testlist[] =
{
    { "advpack", func_advpack },
    { "files", func_files },
    { "install", func_install },
    { 0, 0 }
};
