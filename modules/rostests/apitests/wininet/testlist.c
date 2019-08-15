#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_InternetOpen(void);
extern void func_Redirection(void);

const struct test winetest_testlist[] =
{
    { "InternetOpen", func_InternetOpen },
    { "Redirection", func_Redirection },

    { 0, 0 }
};
