/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_capture(void);
extern void func_ds3d8(void);
extern void func_ds3d(void);
extern void func_dsound8(void);
extern void func_dsound(void);
extern void func_duplex(void);
extern void func_propset(void);

const struct test winetest_testlist[] =
{
    { "capture", func_capture },
    { "ds3d8", func_ds3d8 },
    { "ds3d", func_ds3d },
    { "dsound8", func_dsound8 },
    { "dsound", func_dsound },
    { "duplex", func_duplex },
    { "propset", func_propset },
    { 0, 0 }
};
