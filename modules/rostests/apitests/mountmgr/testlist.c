#define STANDALONE
#include <apitest.h>

extern void func_QueryDosVolumePaths(void);
extern void func_QueryPoints(void);

const struct test winetest_testlist[] =
{
    { "QueryDosVolumePaths", func_QueryDosVolumePaths },
    { "QueryPoints", func_QueryPoints },
    { 0, 0 }
};

