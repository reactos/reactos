/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_enum_files(void);
extern void func_enum_jobs(void);
extern void func_file(void);
extern void func_job(void);
extern void func_qmgr(void);

const struct test winetest_testlist[] =
{
    { "enum_files", func_enum_files },
    { "enum_jobs", func_enum_jobs },
    { "file", func_file },
    { "job", func_job },
    { "qmgr", func_qmgr },
    { 0, 0 }
};
