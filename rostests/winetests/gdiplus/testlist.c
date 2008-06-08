/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_brush(void);
extern void func_graphics(void);
extern void func_graphicspath(void);
extern void func_image(void);
extern void func_matrix(void);
extern void func_pen(void);
extern void func_stringformat(void);
extern void func_font(void);

const struct test winetest_testlist[] =
{
    { "brush", func_brush },
    { "graphics", func_graphics },
    { "graphicspath", func_graphicspath },
    { "font", func_font },
    { "image", func_image },
    { "matrix", func_matrix },
    { "stringformat", func_stringformat },
    { "pen", func_pen },
    { 0, 0 }
};
