#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_CreateService(void);
extern void func_DuplicateTokenEx(void);
extern void func_eventlog(void);
extern void func_HKEY_CLASSES_ROOT(void);
extern void func_IsTextUnicode(void);
extern void func_LockServiceDatabase(void);
extern void func_QueryServiceConfig2(void);
extern void func_RegEnumKey(void);
extern void func_RegEnumValueW(void);
extern void func_RegOpenKeyExW(void);
extern void func_RegQueryInfoKey(void);
extern void func_RegQueryValueExW(void);
extern void func_RtlEncryptMemory(void);
extern void func_SaferIdentifyLevel(void);
extern void func_ServiceArgs(void);
extern void func_ServiceEnv(void);
extern void func_ServiceNetwork(void);

const struct test winetest_testlist[] =
{
    { "CreateService", func_CreateService },
    { "DuplicateTokenEx", func_DuplicateTokenEx },
    { "eventlog_supp", func_eventlog },
    { "HKEY_CLASSES_ROOT", func_HKEY_CLASSES_ROOT },
    { "IsTextUnicode" , func_IsTextUnicode },
    { "LockServiceDatabase" , func_LockServiceDatabase },
    { "QueryServiceConfig2", func_QueryServiceConfig2 },
    { "RegEnumKey", func_RegEnumKey },
    { "RegEnumValueW", func_RegEnumValueW },
    { "RegQueryInfoKey", func_RegQueryInfoKey },
    { "RegOpenKeyExW", func_RegOpenKeyExW },
    { "RegQueryValueExW", func_RegQueryValueExW },
    { "RtlEncryptMemory", func_RtlEncryptMemory },
    { "SaferIdentifyLevel", func_SaferIdentifyLevel },
    { "ServiceArgs", func_ServiceArgs },
    { "ServiceEnv", func_ServiceEnv },
    { "ServiceNetwork", func_ServiceNetwork },
    { 0, 0 }
};

