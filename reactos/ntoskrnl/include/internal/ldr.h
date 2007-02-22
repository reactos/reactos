#ifndef __INCLUDE_INTERNAL_LDR_H
#define __INCLUDE_INTERNAL_LDR_H

#define KERNEL_MODULE_NAME      L"ntoskrnl.exe"
#define HAL_MODULE_NAME         L"hal.dll"
#define DRIVER_ROOT_NAME        L"\\Driver\\"
#define FILESYSTEM_ROOT_NAME    L"\\FileSystem\\"

extern ULONG_PTR LdrHalBase;

NTSTATUS
NTAPI
LdrLoadInitialProcess(
    PHANDLE ProcessHandle,
    PHANDLE ThreadHandle
);

VOID
NTAPI
LdrLoadAutoConfigDrivers(VOID);

NTSTATUS
NTAPI
LdrpMapImage(
    HANDLE ProcessHandle,
    HANDLE SectionHandle,
    PVOID *ImageBase
);

NTSTATUS
NTAPI
LdrpLoadImage(
    PUNICODE_STRING DriverName,
    PVOID *ModuleBase,
    PVOID *SectionPointer,
    PVOID *EntryPoint,
    PVOID *ExportDirectory
);

NTSTATUS
NTAPI
LdrpUnloadImage(PVOID ModuleBase);

NTSTATUS
NTAPI
LdrpLoadAndCallImage(PUNICODE_STRING DriverName);

NTSTATUS
NTAPI
LdrpQueryModuleInformation(
    PVOID Buffer,
    ULONG Size,
    PULONG ReqSize
);

VOID
NTAPI
LdrInit1(VOID);

VOID
NTAPI
LdrInitDebug(
    PLOADER_MODULE Module,
    PWCH Name
);

NTSTATUS
NTAPI
MmLoadSystemImage(IN PUNICODE_STRING FileName,
                  IN PUNICODE_STRING NamePrefix OPTIONAL,
                  IN PUNICODE_STRING LoadedName OPTIONAL,
                  IN ULONG Flags,
                  OUT PVOID *ModuleObject,
                  OUT PVOID *ImageBaseAddress);

NTSTATUS
NTAPI
LdrUnloadModule(PLDR_DATA_TABLE_ENTRY ModuleObject);

PLDR_DATA_TABLE_ENTRY
NTAPI
LdrGetModuleObject(PUNICODE_STRING ModuleName);

#endif /* __INCLUDE_INTERNAL_LDR_H */
