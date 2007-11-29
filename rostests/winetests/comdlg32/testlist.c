/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_filedlg(void);
extern void func_printdlg(void);

const struct test winetest_testlist[] =
{
    { "filedlg", func_filedlg },
    { "printdlg", func_printdlg },
    { 0, 0 }
};
