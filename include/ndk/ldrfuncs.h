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

//
// Resource Functions
//
NTSTATUS
NTAPI
LdrAccessResource(
    IN PVOID BaseAddress,
    IN PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
    OUT PVOID *Resource OPTIONAL,
    OUT PULONG Size OPTIONAL
);

NTSTATUS
NTAPI
LdrFindResource_U(
    IN PVOID BaseAddress,
    IN PLDR_RESOURCE_INFO ResourceInfo,
    IN ULONG Level,
    OUT PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry
);

NTSTATUS
NTAPI
LdrFindResourceDirectory_U(
    IN PVOID BaseAddress,
    IN PLDR_RESOURCE_INFO ResourceInfo,
    IN ULONG Level,
    OUT PIMAGE_RESOURCE_DIRECTORY *ResourceDirectory
);

BOOLEAN
LdrUnloadAlternateResourceModule(
    IN PVOID BaseAddress
);

//
// Misc. Functions
//
NTSTATUS
NTAPI
LdrGetProcedureAddress(
    IN PVOID BaseAddress,
    IN PANSI_STRING Name,
    IN ULONG Ordinal,
    OUT PVOID *ProcedureAddress
);

ULONG
NTAPI
LdrRelocateImage(
    IN PVOID NewBase,
    IN PUCHAR LoaderName,
    IN ULONG Success,
    IN ULONG Conflict,
    IN ULONG Invalid
);

NTSTATUS
LdrLockLoaderLock(
    IN ULONG Flags,
    OUT PULONG Disposition OPTIONAL,
    OUT PULONG Cookie OPTIONAL
);

NTSTATUS
NTAPI
LdrUnlockLoaderLock(
    IN ULONG Flags,
    IN ULONG Cookie OPTIONAL
);

BOOLEAN
NTAPI
LdrVerifyMappedImageMatchesChecksum(
    IN PVOID BaseAddress,
    IN ULONG NumberOfBytes,
    IN ULONG FileLength
);

#endif
