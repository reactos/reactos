/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_devinst(void);
extern void func_diskspace(void);
extern void func_install(void);
extern void func_misc(void);
extern void func_parser(void);
extern void func_query(void);
extern void func_setupcab(void);
extern void func_stringtable(void);

const struct test winetest_testlist[] =
{
    { "devinst", func_devinst },
    { "diskspace", func_diskspace },
    { "install", func_install },
    { "misc", func_misc },
    { "parser", func_parser },
    { "query", func_query },
    { "setupcab", func_setupcab },
    { "stringtable", func_stringtable },
    { 0, 0 }
};
