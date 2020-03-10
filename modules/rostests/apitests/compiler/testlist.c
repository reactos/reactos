#define STANDALONE
#include <apitest.h>

extern void func_pseh(void);
extern void func_pseh_cpp(void);

const struct test winetest_testlist[] =
{
    { "pseh", func_pseh },
    { "pseh_cpp", func_pseh_cpp },
    { 0, 0 }
};
