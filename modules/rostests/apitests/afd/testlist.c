#define STANDALONE
#include <apitest.h>

extern void func_send(void);

const struct test winetest_testlist[] =
{
    { "send", func_send },
    { 0, 0 }
};
