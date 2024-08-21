#define STANDALONE
#include <apitest.h>

extern void func_floatconv(void);
extern void func_ms_seh(void);
extern void func_pseh(void);
extern void func_pseh_cpp(void);

const struct test winetest_testlist[] =
{
    { "floatconv", func_floatconv },
#ifndef __VS_PROJECT__
#if !(defined(__GNUC__) && defined(_M_AMD64))
    { "ms-seh", func_ms_seh },
#endif
    { "pseh", func_pseh },
    { "pseh_cpp", func_pseh_cpp },
#endif /* __VS_PROJECT__ */
    { 0, 0 }
};
