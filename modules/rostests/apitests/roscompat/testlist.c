
#define STANDALONE
#include <wine/test.h>

extern void func_export_rewriting(void);

const struct test winetest_testlist[] =
{
    { "export_rewriting",           func_export_rewriting },
    { 0, 0 }
};

