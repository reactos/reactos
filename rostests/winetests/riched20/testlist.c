/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_editor(void);
extern void func_txtsrv(void);

const struct test winetest_testlist[] =
{
	{ "editor", func_editor },
	{ "txtsrv", func_txtsrv },
    { 0, 0 }
};
