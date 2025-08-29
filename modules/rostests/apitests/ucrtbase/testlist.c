/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func__finite(void);
extern void func__isnan(void);
extern void func__wsystem(void);
extern void func_acos(void);
extern void func_asin(void);
extern void func_atan(void);
extern void func_ceil(void);
extern void func_cos(void);
extern void func_exp(void);
extern void func_log(void);
extern void func_log10(void);
extern void func_round(void);
extern void func_setjmp(void);
extern void func_sin(void);
extern void func_sqrt(void);
extern void func_system(void);
extern void func_tan(void);
extern void func_wctomb(void);


const struct test winetest_testlist[] =
{
    { "_finite", func__finite },
    { "_isnan", func__isnan },
    { "_wsystem", func__wsystem },
    { "acos", func_acos },
    { "asin", func_asin },
    { "atan", func_atan },
    { "ceil", func_ceil },
    { "cos", func_cos },
    { "exp", func_exp },
    { "log", func_log },
    { "log10", func_log10 },
    { "round", func_round },
    { "setjmp", func_setjmp },
    { "sin", func_sin },
    { "sqrt", func_sqrt },
    { "system", func_system },
    { "tan", func_tan },
    { "wctomb", func_wctomb },

    { 0, 0 }
};
