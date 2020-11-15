#define STANDALONE
#include <apitest.h>

extern void func_CachedGetUserFromSid(void);

const struct test winetest_testlist[] =
{
    { "CachedGetUserFromSid", func_CachedGetUserFromSid },
    { 0, 0 }
};
