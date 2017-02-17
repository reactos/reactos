
#define STANDALONE
#include <wine/test.h>

extern void func_pdb(void);
extern void func_rsym(void);

const struct test winetest_testlist[] =
{
    { "pdb", func_pdb },
    { "rsym", func_rsym },
    { 0, 0 }
};
