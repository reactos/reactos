/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_avisplitter(void);
extern void func_dsoundrender(void);
extern void func_filtergraph(void);
extern void func_filtermapper(void);
extern void func_memallocator(void);
extern void func_misc(void);
extern void func_referenceclock(void);
extern void func_videorenderer(void);

const struct test winetest_testlist[] =
{
    { "avisplitter", func_avisplitter },
    { "dsoundrender", func_dsoundrender },
    { "filtergraph", func_filtergraph },
    { "filtermapper", func_filtermapper },
    { "memallocator", func_memallocator },
    { "misc", func_misc },
    { "videorenderer", func_videorenderer },
    { "referenceclock", func_referenceclock },
    { 0, 0 }
};
