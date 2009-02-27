/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_qmgr(void);
extern void func_job(void);
extern void func_file(void);
extern void func_enum_jobs(void);
extern void func_enum_files(void);

const struct test winetest_testlist[] =
{
    { "qmgr", func_qmgr },
	{ "job", func_job },
	{ "file", func_file },
	{ "enum_jobs", func_enum_jobs },
	{ "enum_files", func_enum_files },
    { 0, 0 }
};
