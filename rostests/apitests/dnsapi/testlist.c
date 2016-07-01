#define STANDALONE
#include <apitest.h>

extern void func_DnsQuery(void);

const struct test winetest_testlist[] =
{
    { "DnsQuery", func_DnsQuery },
    { 0, 0 }
};
