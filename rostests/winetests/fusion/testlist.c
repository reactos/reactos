/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_fusion(void);
extern void func_asmname(void);
extern void func_asmenum(void);
extern void func_asmcache(void);

const struct test winetest_testlist[] =
{
    { "fusion", func_fusion },
	{ "asmname", func_asmname },
	{ "asmenum", func_asmenum },
	{ "asmcache", func_asmcache },
    { 0, 0 }
};
