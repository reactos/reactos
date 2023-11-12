#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_RtlCaptureContext(void);
extern void func_RtlIntSafe(void);

const struct test winetest_testlist[] =
{
    { "RtlIntSafe",               func_RtlIntSafe },

#ifdef _M_AMD64
    { "RtlCaptureContext",        func_RtlCaptureContext },
#endif

    { 0, 0 }
};
