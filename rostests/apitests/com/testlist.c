#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_browseui(void);
extern void func_ieframe(void);
extern void func_shdocvw(void);
extern void func_shell32(void);

const struct test winetest_testlist[] =
{
    { "browseui", func_browseui },
    { "ieframe", func_ieframe },
    { "shdocvw", func_shdocvw },
    { "shell32", func_shell32 },

    { 0, 0 }
};
