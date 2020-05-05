#define STANDALONE
#include <apitest.h>

extern void func_EnumObjects(void);
extern void func_EnumParentDir(void);
extern void func_IDataObject(void);

const struct test winetest_testlist[] =
{
    { "EnumObjects", func_EnumObjects },
    { "EnumParentDir", func_EnumParentDir },
    { "IDataObject", func_IDataObject },
    { 0, 0 }
};
