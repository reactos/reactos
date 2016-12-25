/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_activex(void);
extern void func_dom(void);
extern void func_events(void);
extern void func_htmldoc(void);
extern void func_htmllocation(void);
extern void func_misc(void);
extern void func_protocol(void);
extern void func_script(void);
extern void func_style(void);
extern void func_xmlhttprequest(void);

const struct test winetest_testlist[] =
{
    { "activex", func_activex },
    { "dom", func_dom },
    { "events", func_events },
    { "htmldoc", func_htmldoc },
    { "htmllocation", func_htmllocation },
    { "misc", func_misc },
    { "protocol", func_protocol },
    { "script", func_script },
    { "style", func_style },
    { "xmlhttprequest", func_xmlhttprequest },
    { 0, 0 }
};
