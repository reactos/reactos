/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_domdoc(void);
extern void func_saxreader(void);
extern void func_schema(void);
extern void func_xmldoc(void);
extern void func_xmlparser(void);
extern void func_xmlview(void);

const struct test winetest_testlist[] =
{
    { "domdoc", func_domdoc },
    { "saxreader", func_saxreader },
    { "schema", func_schema },
    { "xmldoc", func_xmldoc },
    { "xmlparser", func_xmlparser },
    { "xmlview", func_xmlview },
    { 0, 0 }
};
