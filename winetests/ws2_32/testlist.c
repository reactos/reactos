/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_protocol(void);
extern void func_sock(void);

const struct test winetest_testlist[] =
{
    { "protocol", func_protocol },
    { "sock", func_sock },
    { 0, 0 }
};
