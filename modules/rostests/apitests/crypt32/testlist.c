#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_CertEnumSystemStoreLocation(void);

const struct test winetest_testlist[] =
{
    { "CertEnumSystemStoreLocation", func_CertEnumSystemStoreLocation },

    { 0, 0 }
};
