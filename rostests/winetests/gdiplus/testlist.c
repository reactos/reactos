/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_brush(void);
extern void func_customlinecap(void);
extern void func_font(void);
extern void func_graphics(void);
extern void func_graphicspath(void);
extern void func_image(void);
extern void func_matrix(void);
extern void func_pathiterator(void);
extern void func_pen(void);
extern void func_region(void);
extern void func_stringformat(void);

const struct test winetest_testlist[] =
{
    { "brush", func_brush },
	{ "customlinecap", func_customlinecap },
    { "font", func_font },
    { "graphics", func_graphics },
    { "graphicspath", func_graphicspath },
    { "image", func_image },
    { "matrix", func_matrix },
    { "pathiterator", func_pathiterator },
    { "pen", func_pen },
    { "region", func_region },
    { "stringformat", func_stringformat },
    { 0, 0 }
};
