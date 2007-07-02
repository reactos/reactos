#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_d3d(void);
extern void func_ddrawmodes(void);
extern void func_dsurface(void);
extern void func_refcount(void);
extern void func_visual(void);

const struct test winetest_testlist[] =
{
	{ "d3d", func_d3d },
	{ "ddrawmodes", func_ddrawmodes },
	{ "dsurface", func_dsurface },
	{ "refcount", func_refcount },
	{ "visual", func_visual },
	{ 0, 0 }
};
