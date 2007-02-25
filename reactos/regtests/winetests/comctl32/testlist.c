#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_comboex(void);
extern void func_dpa(void);
extern void func_header(void);
extern void func_imagelist(void);
extern void func_listview(void);
extern void func_monthcal(void);
extern void func_mru(void);
extern void func_progress(void);
extern void func_propsheet(void);
extern void func_subclass(void);
extern void func_tab(void);
extern void func_toolbar(void);
extern void func_tooltips(void);
extern void func_treeview(void);
extern void func_updown(void);

const struct test winetest_testlist[] =
{
    { "comboex", func_comboex },
    { "dpa", func_dpa },
    { "header", func_header },
    { "imagelist", func_imagelist },
    { "listview", func_listview },
    { "monthcal", func_monthcal },
    { "mru", func_mru },
    { "progress", func_progress },
    { "propsheet", func_propsheet },
    { "subclass", func_subclass },
    { "tab", func_tab },
    { "toolbar", func_toolbar },
    { "tooltips", func_tooltips },
    { "treeview", func_treeview },
    { "updown", func_updown },
    { 0, 0 }
};
