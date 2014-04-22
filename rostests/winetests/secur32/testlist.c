/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_main(void);
extern void func_negotiate(void);
extern void func_ntlm(void);
extern void func_schannel(void);
extern void func_secur32(void);

const struct test winetest_testlist[] =
{
    { "main", func_main },
    { "negotiate", func_negotiate },
    { "ntlm", func_ntlm },
    { "schannel", func_schannel },
    { "secur32", func_secur32 },
    { 0, 0 }
};
