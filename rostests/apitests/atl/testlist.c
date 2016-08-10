#define STANDALONE
#include <apitest.h>

extern void func_atltypes(void);
extern void func_CComBSTR(void);
extern void func_CComHeapPtr(void);
extern void func_CRegKey(void);
extern void func_CString(void);

const struct test winetest_testlist[] =
{
    { "atltypes", func_atltypes },
    { "CComBSTR", func_CComBSTR },
    { "CComHeapPtr", func_CComHeapPtr },
    { "CRegKey", func_CRegKey },
    { "CString", func_CString },
    { 0, 0 }
};
