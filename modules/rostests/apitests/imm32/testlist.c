
#define STANDALONE
#include <wine/test.h>

extern void func_clientimc(void);
extern void func_himc(void);
extern void func_imcc(void);
extern void func_ImmEnumInputContext(void);
extern void func_ImmGetImeInfoEx(void);
extern void func_ImmIsUIMessage(void);
extern void func_KLID(void);
extern void func_JapanImeConvTestA(void);
extern void func_JapanImeConvTestW(void);

const struct test winetest_testlist[] =
{
    { "clientimc", func_clientimc },
    { "himc", func_himc },
    { "imcc", func_imcc },
    { "ImmEnumInputContext", func_ImmEnumInputContext },
    { "ImmGetImeInfoEx", func_ImmGetImeInfoEx },
    { "ImmIsUIMessage", func_ImmIsUIMessage },
    { "KLID", func_KLID },
    { "JapanImeConvTestA", func_JapanImeConvTestA },
    { "JapanImeConvTestW", func_JapanImeConvTestW },
    { 0, 0 }
};
