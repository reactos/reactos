#define __ROS_LONG64__
#define wine_dbgstr_wn wine_dbgstr_wn_
#define STANDALONE
#include <apitest.h>

extern void func_RtlCaptureContext(void);
extern void func_RtlIntSafe(void);
extern void func_RtlUnwind(void);

const struct test winetest_testlist[] =
{
    { "RtlIntSafe",               func_RtlIntSafe },

#ifdef _M_IX86
    { "RtlUnwind",                func_RtlUnwind },
#endif
#ifdef _M_AMD64
    { "RtlCaptureContext",        func_RtlCaptureContext },
#endif

    { 0, 0 }
};
