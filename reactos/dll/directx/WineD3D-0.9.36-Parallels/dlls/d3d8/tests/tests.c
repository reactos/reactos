#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_d3d8_main(void);
extern void func_device(void);
extern void func_surface(void);
extern void func_texture(void);
extern void func_visual(void);
extern void func_volume(void);

const struct test winetest_testlist[] =
{
	{ "d3d8_main", func_d3d8_main },
	{ "device", func_device },
	{ "surface", func_surface },
	{ "texture", func_texture },
	{ "visual", func_visual },
	{ "volume", func_volume },
	{ 0, 0 }
};
