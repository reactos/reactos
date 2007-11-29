/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_clipboard(void);
extern void func_compobj(void);
extern void func_dragdrop(void);
extern void func_errorinfo(void);
extern void func_hglobalstream(void);
extern void func_marshal(void);
extern void func_moniker(void);
extern void func_ole2(void);
extern void func_propvariant(void);
extern void func_stg_prop(void);
extern void func_storage32(void);
extern void func_usrmarshal(void);

const struct test winetest_testlist[] =
{
    { "clipboard", func_clipboard },
    { "compobj", func_compobj },
    { "dragdrop", func_dragdrop },
    { "errorinfo", func_errorinfo },
    { "hglobalstream", func_hglobalstream },
    { "marshal", func_marshal },
    { "moniker", func_moniker },
    { "ole2", func_ole2 },
    { "propvariant", func_propvariant },
    { "stg_prop", func_stg_prop },
    { "storage32", func_storage32 },
    { "usrmarshal", func_usrmarshal },
    { 0, 0 }
};
