#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_RtlCaptureContext(void);

const struct test winetest_testlist[] =
{
#ifdef _M_AMD64
    { "RtlCaptureContext",        func_RtlCaptureContext },
#endif

    { 0, 0 }
};
