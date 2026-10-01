/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_asm(void);
extern void func_blob(void);
extern void func_hlsl_d3d11(void);
extern void func_hlsl_d3d9(void);
extern void func_reflection(void);

const struct test winetest_testlist[] =
{
    { "asm", func_asm },
    { "blob", func_blob },
    { "hlsl_d3d11", func_hlsl_d3d11 },
    { "hlsl_d3d9", func_hlsl_d3d9 },
    { "reflection", func_reflection },
    { 0, 0 }
};
