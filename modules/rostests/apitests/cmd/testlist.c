#define STANDALONE
#include <apitest.h>

extern void func_attrib(void);
extern void func_cd(void);
extern void func_echo(void);
extern void func_exit(void);
extern void func_pushd(void);

const struct test winetest_testlist[] =
{
    { "attrib", func_attrib },
    { "cd", func_cd },
    { "echo", func_echo },
    { "exit", func_exit },
    { "pushd", func_pushd },
    { 0, 0 }
};
