/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_enum_files(void);
extern void func_enum_jobs(void);
extern void func_file(void);
extern void func_job(void);
extern void func_qmgr(void);

#ifdef __REACTOS__
void func_skipped(void) { skip("qmgr tests skipped due to CORE-6536\n"); }
#endif

const struct test winetest_testlist[] =
{
/* Skipped because of testbot timeouts. See CORE-6536. */
#ifdef __REACTOS__
    { "skipped", func_skipped },
#else
    { "enum_files", func_enum_files },
    { "enum_jobs", func_enum_jobs },
    { "file", func_file },
    { "job", func_job },
    { "qmgr", func_qmgr },
#endif
    { 0, 0 }
};
