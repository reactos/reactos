#define __ROS_LONG64__

#define STANDALONE
#include <wine/test.h>

extern void func_roscompat(void);

const struct test winetest_testlist[] =
{
    { "roscompat",                    func_roscompat },

    { 0, 0 }
};

