/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_olefont(void);
extern void func_olepicture(void);
extern void func_safearray(void);
extern void func_tmarshal(void);
extern void func_typelib(void);
extern void func_usrmarshal(void);
extern void func_varformat(void);
extern void func_vartest(void);
extern void func_vartype(void);

const struct test winetest_testlist[] =
{
    { "olefont", func_olefont },
    { "olepicture", func_olepicture },
    { "safearray", func_safearray },
    { "tmarshal", func_tmarshal },
    { "typelib", func_typelib },
    { "usrmarshal", func_usrmarshal },
    { "varformat", func_varformat },
    { "vartest", func_vartest },
    { "vartype", func_vartype },
    { 0, 0 }
};
