#ifndef __INCLUDE_INTERNAL_LDR_H
#define __INCLUDE_INTERNAL_LDR_H

#define KERNEL_MODULE_NAME      L"ntoskrnl.exe"
#define HAL_MODULE_NAME         L"hal.dll"
#define DRIVER_ROOT_NAME        L"\\Driver\\"
#define FILESYSTEM_ROOT_NAME    L"\\FileSystem\\"

PLDR_DATA_TABLE_ENTRY
NTAPI
LdrGetModuleObject(PUNICODE_STRING ModuleName);

#endif /* __INCLUDE_INTERNAL_LDR_H */
