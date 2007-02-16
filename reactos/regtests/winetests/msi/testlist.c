#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_db(void);
extern void func_format(void);
extern void func_install(void);
extern void func_msi(void);
extern void func_package(void);
extern void func_record(void);
extern void func_suminfo(void);

const struct test winetest_testlist[] =
{
    { "db", func_db },
    { "format", func_format },
    { "install", func_install },
    { "msi", func_msi },
    { "package", func_package },
    { "record", func_record },
    { "suminfo", func_suminfo },
    { 0, 0 }
};
