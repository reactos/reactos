/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_access(void);
extern void func_apibuf(void);
extern void func_ds(void);
extern void func_wksta(void);

const struct test winetest_testlist[] =
{
    { "access", func_access },
    { "apibuf", func_apibuf },
    { "ds", func_ds },
    { "wksta", func_wksta },
    { 0, 0 }
};
