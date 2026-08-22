
#define STANDALONE
#include <apitest.h>

extern void func_devclass(void);
extern void func_SetupInstallServicesFromInfSectionEx(void);
extern void func_SetupDiInstallClassExA(void);
extern void func_SetupDiGetActualModelsSection(void);

const struct test winetest_testlist[] =
{
    { "devclass", func_devclass },
    { "SetupInstallServicesFromInfSectionEx", func_SetupInstallServicesFromInfSectionEx},
    { "SetupDiInstallClassExA", func_SetupDiInstallClassExA},
    { "SetupDiGetActualModelsSection", func_SetupDiGetActualModelsSection},
    { 0, 0 }
};
