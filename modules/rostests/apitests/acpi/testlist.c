#define STANDALONE
#include <apitest.h>

extern void func_Bus_PDO_EvalMethod(void);
extern void func_Bus_PDO_QueryResourceRequirements(void);

const struct test winetest_testlist[] =
{
    { "Bus_PDO_EvalMethod", func_Bus_PDO_EvalMethod },
    { "Bus_PDO_QueryResourceRequirements", func_Bus_PDO_QueryResourceRequirements },
    { 0, 0 }
};
