
#define STANDALONE
#include <wine/test.h>

extern void func_clientimc(void);
extern void func_himc(void);
extern void func_imcc(void);
extern void func_ImmGetImeInfoEx(void);
extern void func_ImmIsUIMessage(void);

const struct test winetest_testlist[] =
{
    { "clientimc", func_clientimc },
    { "himc", func_himc },
    { "imcc", func_imcc },
    { "ImmGetImeInfoEx", func_ImmGetImeInfoEx },
    { "ImmIsUIMessage", func_ImmIsUIMessage },
    { 0, 0 }
};
