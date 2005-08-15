#ifndef __INCLUDE_INTERNAL_LDR_H
#define __INCLUDE_INTERNAL_LDR_H

#define  KERNEL_MODULE_NAME  L"ntoskrnl.exe"
#define  HAL_MODULE_NAME  L"hal.dll"
#define  DRIVER_ROOT_NAME  L"\\Driver\\"
#define  FILESYSTEM_ROOT_NAME  L"\\FileSystem\\"

extern ULONG_PTR LdrHalBase;

NTSTATUS
LdrLoadInitialProcess(
    PHANDLE ProcessHandle,
    PHANDLE ThreadHandle
);

VOID
LdrLoadAutoConfigDrivers (VOID);

VOID
LdrInitModuleManagement (VOID);

NTSTATUS
LdrpMapImage(
    HANDLE	ProcessHandle,
    HANDLE	SectionHandle,
    PVOID	* ImageBase
);

NTSTATUS
LdrpLoadImage(
    PUNICODE_STRING DriverName,
    PVOID *ModuleBase,
    PVOID *SectionPointer,
    PVOID *EntryPoint,
    PVOID *ExportDirectory
);

NTSTATUS
LdrpUnloadImage(PVOID ModuleBase);

NTSTATUS
LdrpLoadAndCallImage(PUNICODE_STRING DriverName);

NTSTATUS
LdrpQueryModuleInformation(
    PVOID Buffer,
    ULONG Size,
    PULONG ReqSize
);

VOID
LdrInit1(VOID);

VOID
LdrInitDebug(
    PLOADER_MODULE Module, 
    PWCH Name
);

PVOID 
LdrSafePEProcessModule(
    PVOID ModuleLoadBase,
    PVOID DriverBase,
    PVOID ImportModuleBase,
    PULONG DriverSize
);

NTSTATUS
LdrLoadModule(
    PUNICODE_STRING Filename,
    PLDR_DATA_TABLE_ENTRY *ModuleObject
);

NTSTATUS
LdrUnloadModule(PLDR_DATA_TABLE_ENTRY ModuleObject);

PLDR_DATA_TABLE_ENTRY
LdrGetModuleObject(PUNICODE_STRING ModuleName);

#endif /* __INCLUDE_INTERNAL_LDR_H */
