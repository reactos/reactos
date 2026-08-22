
#define STANDALONE
#include <apitest.h>

extern void func_devclass(void);
extern void func_SetupDiGetActualModelsSection(void);
extern void func_SetupDiInstallClassExA(void);
extern void func_SetupInstallServicesFromInfSectionEx(void);

const struct test winetest_testlist[] =
{
    { "devclass", func_devclass },
    { "SetupDiGetActualModelsSection", func_SetupDiGetActualModelsSection },
    { "SetupDiInstallClassExA", func_SetupDiInstallClassExA },
    { "SetupInstallServicesFromInfSectionEx", func_SetupInstallServicesFromInfSectionEx },
    { 0, 0 }
};
