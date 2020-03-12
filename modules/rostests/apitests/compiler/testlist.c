#define STANDALONE
#include <apitest.h>

extern void func_ms_seh(void);
extern void func_pseh(void);
extern void func_pseh_cpp(void);

const struct test winetest_testlist[] =
{
    { "ms-seh", func_ms_seh },
    { "pseh", func_pseh },
    { "pseh_cpp", func_pseh_cpp },
    { 0, 0 }
};
