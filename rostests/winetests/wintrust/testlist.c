/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_asn(void);
extern void func_crypt(void);
extern void func_register(void);
extern void func_softpub(void);

const struct test winetest_testlist[] =
{
    { "asn", func_asn },
    { "crypt", func_crypt },
    { "register", func_register },
    { "softpub", func_softpub },
    { 0, 0 }
};
