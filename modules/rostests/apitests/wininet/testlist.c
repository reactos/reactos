#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_Download(void);
extern void func_InternetOpen(void);

const struct test winetest_testlist[] =
{
    { "Download", func_Download },
    { "InternetOpen", func_InternetOpen },

    { 0, 0 }
};
