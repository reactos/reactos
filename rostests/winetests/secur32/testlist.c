/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_secur32(void);
extern void func_schannel(void);
extern void func_ntlm(void);
extern void func_main(void);

const struct test winetest_testlist[] =
{
    { "secur32", func_secur32 },
	{ "schannel", func_schannel },
	{ "ntlm", func_ntlm },
	{ "main", func_main },
    { 0, 0 }
};
