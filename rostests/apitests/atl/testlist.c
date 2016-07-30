#define STANDALONE
#include <apitest.h>

extern void func_CComBSTR(void);
extern void func_CComHeapPtr(void);
extern void func_CString(void);

const struct test winetest_testlist[] =
{
    { "CComBSTR", func_CComBSTR },
    { "CComHeapPtr", func_CComHeapPtr },
    { "CString", func_CString },
    { 0, 0 }
};
