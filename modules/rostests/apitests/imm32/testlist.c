
#define STANDALONE
#include <wine/test.h>

extern void func_clientimc(void);
extern void func_imcc(void);

const struct test winetest_testlist[] =
{
    { "clientimc", func_clientimc },
    { "imcc", func_imcc },
    { 0, 0 }
};
