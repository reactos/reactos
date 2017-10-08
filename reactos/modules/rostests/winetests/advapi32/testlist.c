/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_cred(void);
extern void func_crypt(void);
extern void func_crypt_lmhash(void);
extern void func_crypt_md4(void);
extern void func_crypt_md5(void);
extern void func_crypt_sha(void);
extern void func_eventlog(void);
extern void func_lsa(void);
extern void func_registry(void);
extern void func_security(void);
extern void func_service(void);

const struct test winetest_testlist[] =
{
    { "cred", func_cred },
    { "crypt", func_crypt },
    { "crypt_lmhash", func_crypt_lmhash },
    { "crypt_md4", func_crypt_md4 },
    { "crypt_md5", func_crypt_md5 },
    { "crypt_sha", func_crypt_sha },
    { "eventlog", func_eventlog },
    { "lsa", func_lsa },
    { "registry", func_registry },
    { "security", func_security },
    { "service", func_service },
    { 0, 0 }
};
