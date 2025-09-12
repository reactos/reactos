#define __ROS_LONG64__

#define STANDALONE
#include <wine/test.h>

extern void func_ntoskrnl_SectionFlags(void);

const struct test winetest_testlist[] =
{
    { "ntoskrnl_SectionFlags",                  func_ntoskrnl_SectionFlags },

    { 0, 0 }
};

