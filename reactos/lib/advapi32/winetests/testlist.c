/* Automatically generated file; DO NOT EDIT!! */

/* stdarg.h is needed for Winelib */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"

extern void func_crypt(void);
extern void func_crypt_lmhash(void);
extern void func_crypt_md4(void);
extern void func_crypt_md5(void);
extern void func_crypt_sha(void);
extern void func_registry(void);
extern void func_security(void);

struct test
{
    const char *name;
    void (*func)(void);
};

static const struct test winetest_testlist[] =
{
    { "crypt", func_crypt },
    { "crypt_lmhash", func_crypt_lmhash },
    { "crypt_md4", func_crypt_md4 },
    { "crypt_md5", func_crypt_md5 },
    { "crypt_sha", func_crypt_sha },
    { "registry", func_registry },
    { "security", func_security },
    { 0, 0 }
};

#define WINETEST_WANT_MAIN
#include "wine/test.h"
