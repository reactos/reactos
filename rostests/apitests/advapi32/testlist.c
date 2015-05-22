#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_CreateService(void);
extern void func_DuplicateTokenEx(void);
extern void func_HKEY_CLASSES_ROOT(void);
extern void func_LockDatabase(void);
extern void func_QueryServiceConfig2(void);
extern void func_RegEnumValueW(void);
extern void func_RegQueryInfoKey(void);
extern void func_RtlEncryptMemory(void);
extern void func_SaferIdentifyLevel(void);

const struct test winetest_testlist[] =
{
    { "CreateService", func_CreateService },
    { "DuplicateTokenEx", func_DuplicateTokenEx },
    { "HKEY_CLASSES_ROOT", func_HKEY_CLASSES_ROOT },
    { "LockDatabase" , func_LockDatabase },
    { "QueryServiceConfig2", func_QueryServiceConfig2 },
    { "RegEnumValueW", func_RegEnumValueW },
    { "RegQueryInfoKey", func_RegQueryInfoKey },
    { "RtlEncryptMemory", func_RtlEncryptMemory },
    { "SaferIdentifyLevel", func_SaferIdentifyLevel },

    { 0, 0 }
};

