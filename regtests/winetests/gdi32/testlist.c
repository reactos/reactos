/* stdarg.h is needed for Winelib */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"

extern void func_bitmap(void);
extern void func_brush(void);
extern void func_gdiobj(void);
extern void func_metafile(void);

struct test
{
    const char *name;
    void (*func)(void);
};

static const struct test winetest_testlist[] =
{
    { "bitmap", func_bitmap },
    { "brush", func_brush },
    { "gdiobj", func_gdiobj },
    { "metafile", func_metafile },
    { 0, 0 }
};

#define WINETEST_WANT_MAIN
#include "wine/test.h"
