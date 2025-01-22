/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void  func_d3d(void);
extern void  func_ddraw1(void);
extern void  func_ddraw2(void);
extern void  func_ddraw4(void);
extern void  func_ddraw7(void);
extern void  func_ddrawmodes(void);
extern void  func_dsurface(void);
extern void  func_refcount(void);
extern void  func_visual(void);

const struct test winetest_testlist[] =
{
    { "d3d", func_d3d },
    { "ddraw1", func_ddraw1 },
    { "ddraw2", func_ddraw2 },
    { "ddraw4", func_ddraw4 },
    { "ddraw7", func_ddraw7 },
    { "ddrawmodes", func_ddrawmodes },
    { "dsurface", func_dsurface },
    { "refcount", func_refcount },
    { "visual", func_visual },
    { 0, 0 }
};
