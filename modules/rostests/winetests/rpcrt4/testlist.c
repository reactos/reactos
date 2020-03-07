/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_cstub(void);
#ifdef RUN_COMPILE_TIME_ONLY_GENERATED
extern void func_generated(void);
#endif
extern void func_ndr_marshall(void);
extern void func_rpc(void);
extern void func_rpc_async(void);
extern void func_server(void);

const struct test winetest_testlist[] =
{
    { "cstub", func_cstub },
#ifdef RUN_COMPILE_TIME_ONLY_GENERATED
    { "generated", func_generated },
#endif
    { "ndr_marshall", func_ndr_marshall },
    { "rpc", func_rpc },
    { "rpc_async", func_rpc_async },
    { "server", func_server },
    { 0, 0 }
};
