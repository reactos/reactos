/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_avisplitter(void);
extern void func_videorenderer(void);
extern void func_referenceclock(void);
extern void func_misc(void);
extern void func_memallocator(void);
extern void func_filtermapper(void);
extern void func_filtergraph(void);

const struct test winetest_testlist[] =
{
    { "avisplitter", func_avisplitter },
	{ "videorenderer", func_videorenderer },
	{ "referenceclock", func_referenceclock },
	{ "misc", func_misc },
	{ "memallocator", func_memallocator },
	{ "filtermapper", func_filtermapper },
	{ "filtergraph", func_filtergraph },
    { 0, 0 }
};
