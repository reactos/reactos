/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_bitmap(void);
extern void func_brush(void);
extern void func_clipping(void);
extern void func_dc(void);
extern void func_font(void);
extern void func_gdiobj(void);
extern void func_generated(void);
extern void func_mapping(void);
extern void func_metafile(void);
extern void func_palette(void);
extern void func_pen(void);

const struct test winetest_testlist[] =
{
    { "bitmap", func_bitmap },
    { "brush", func_brush },
    { "clipping", func_clipping },
    { "dc", func_dc },
    { "font", func_font },
    { "gdiobj", func_gdiobj },
//    { "generated", func_generated },
    { "mapping", func_mapping },
    { "metafile", func_metafile },
    { "palette", func_palette },
//    { "pen", func_pen },
    { 0, 0 }
};
