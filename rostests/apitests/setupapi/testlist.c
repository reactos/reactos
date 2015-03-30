#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_devclass(void);
extern void func_SetupInstallServicesFromInfSectionEx(void);
extern void func_SetupDiInstallClassExA(void);

const struct test winetest_testlist[] =
{
    { "devclass", func_devclass },
    { "SetupInstallServicesFromInfSectionEx", func_SetupInstallServicesFromInfSectionEx},
    { "SetupDiInstallClassExA", func_SetupDiInstallClassExA},
    { 0, 0 }
};
