/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_clist(void);
extern void func_clsid(void);
#ifdef RUN_COMPILE_TIME_ONLY_GENERATED
extern void func_generated(void);
#endif
extern void func_istream(void);
extern void func_ordinal(void);
extern void func_path(void);
extern void func_shreg(void);
extern void func_string(void);
extern void func_thread(void);
extern void func_url(void);

const struct test winetest_testlist[] =
{
    { "clist", func_clist },
    { "clsid", func_clsid },
#ifdef RUN_COMPILE_TIME_ONLY_GENERATED
    { "generated", func_generated },
#endif
    { "istream", func_istream },
    { "ordinal", func_ordinal },
    { "path", func_path },
    { "shreg", func_shreg },
    { "string", func_string },
    { "thread", func_thread },
    { "url", func_url },
    { 0, 0 }
};
