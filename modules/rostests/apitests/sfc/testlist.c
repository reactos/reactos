#define STANDALONE
#include <apitest.h>

extern void func_SfcGetFiles(void);
extern void func_SfcIsFileProtected(void);

const struct test winetest_testlist[] =
{
    { "SfcGetFiles", func_SfcGetFiles },
    { "SfcIsFileProtected", func_SfcIsFileProtected },
    { 0, 0 }
};
