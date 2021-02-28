#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_ACListISF(void);
extern void func_IACLCustomMRU(void);
extern void func_IAutoComplete(void);
extern void func_SHEnumClassesOfCategories(void);
extern void func_SHExplorerParseCmdLine(void);

const struct test winetest_testlist[] =
{
    { "ACListISF", func_ACListISF },
    { "IACLCustomMRU", func_IACLCustomMRU },
    { "IAutoComplete", func_IAutoComplete },
    { "SHEnumClassesOfCategories", func_SHEnumClassesOfCategories },
    { "SHExplorerParseCmdLine", func_SHExplorerParseCmdLine },
    { 0, 0 }
};
