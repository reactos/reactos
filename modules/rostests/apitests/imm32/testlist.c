
#define STANDALONE
#include <wine/test.h>

extern void func_clientimc(void);
extern void func_imcc(void);
extern void func_ImmIsUIMessage(void);

const struct test winetest_testlist[] =
{
    { "clientimc", func_clientimc },
    { "imcc", func_imcc },
    { "ImmIsUIMessage", func_ImmIsUIMessage },
    { 0, 0 }
};
