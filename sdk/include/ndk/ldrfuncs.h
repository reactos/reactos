/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    ldrfuncs.h

Abstract:

    Functions definitions for the Loader.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _LDRFUNCS_H
#define _LDRFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <ldrtypes.h>
#if defined(_MSC_VER) && !defined(NTOS_MODE_USER)
#include <ntimage.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Resource Functions
//
NTSTATUS
NTAPI
LdrAccessResource(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
    _Out_opt_ PVOID *Resource,
    _Out_opt_ PULONG Size
);

NTSTATUS
NTAPI
LdrFindResource_U(
    _In_ PVOID BaseAddress,
    _In_ PLDR_RESOURCE_INFO ResourceInfo,
    _In_ ULONG Level,
    _Out_ PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry
);

NTSTATUS
NTAPI
LdrEnumResources(
    _In_ PVOID BaseAddress,
    _In_ PLDR_RESOURCE_INFO ResourceInfo,
    _In_ ULONG Level,
    _Inout_ ULONG *ResourceCount,
    _Out_writes_to_(*ResourceCount,*ResourceCount) LDR_ENUM_RESOURCE_INFO *Resources
);

NTSTATUS
NTAPI
LdrFindResourceDirectory_U(
    _In_ PVOID BaseAddress,
    _In_ PLDR_RESOURCE_INFO ResourceInfo,
    _In_ ULONG Level,
    _Out_ PIMAGE_RESOURCE_DIRECTORY *ResourceDirectory
);

NTSTATUS
NTAPI
LdrLoadAlternateResourceModule(
    _In_ PVOID Module,
    _In_ PWSTR Buffer
);

BOOLEAN
NTAPI
LdrUnloadAlternateResourceModule(
    _In_ PVOID BaseAddress
);

//
// Misc. Functions
//
NTSTATUS
NTAPI
LdrGetProcedureAddress(
    _In_ PVOID BaseAddress,
    _In_opt_ _When_(Ordinal == 0, _Notnull_) PANSI_STRING Name,
    _In_opt_ _When_(Name == NULL, _In_range_(>, 0)) ULONG Ordinal,
    _Out_ PVOID *ProcedureAddress
);

ULONG
NTAPI
LdrRelocateImage(
    _In_ PVOID BaseAddress,
    _In_opt_ PCSTR LoaderName,
    _In_ ULONG Success,
    _In_ ULONG Conflict,
    _In_ ULONG Invalid
);

NTSTATUS
NTAPI
LdrLockLoaderLock(
    _In_ ULONG Flags,
    _Out_opt_ PULONG Disposition,
    _Out_opt_ PULONG_PTR Cookie
);

NTSTATUS
NTAPI
LdrUnlockLoaderLock(
    _In_ ULONG Flags,
    _In_opt_ ULONG_PTR Cookie
);

BOOLEAN
NTAPI
LdrVerifyMappedImageMatchesChecksum(
    _In_ PVOID BaseAddress,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG FileLength
);

PIMAGE_BASE_RELOCATION
NTAPI
LdrProcessRelocationBlockLongLong(
    _In_ ULONG_PTR Address,
    _In_ ULONG Count,
    _In_ PUSHORT TypeOffset,
    _In_ LONGLONG Delta
);

NTSTATUS
NTAPI
LdrEnumerateLoadedModules(
    _Reserved_ ULONG ReservedFlag,
    _In_ PLDR_ENUM_CALLBACK EnumProc,
    _In_opt_ PVOID Context
);

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA) || (DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA)

NTSTATUS
NTAPI
LdrRegisterDllNotification(
    _In_ ULONG Flags,
    _In_ PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction,
    _In_opt_ PVOID Context,
    _Out_ PVOID* Cookie);

NTSTATUS
NTAPI
LdrUnregisterDllNotification(
    _In_ PVOID Cookie);

#endif /* (_WIN32_WINNT >= _WIN32_WINNT_VISTA) || (DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA) */

#ifdef NTOS_MODE_USER
NTSYSAPI
BOOLEAN
NTAPI
RtlDllShutdownInProgress(
    VOID
);
#endif

#ifdef __cplusplus
}
#endif

#endif
