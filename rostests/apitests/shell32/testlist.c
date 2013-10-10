#define __ROS_LONG64__

#define STANDALONE
#include <wine/test.h>

extern void func_menu(void);

const struct test winetest_testlist[] =
{
    { "menu", func_menu },
    
    { 0, 0 }
};
