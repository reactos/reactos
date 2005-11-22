/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    ldrfuncs.h

Abstract:

    Functions definitions for the Loader.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _LDRFUNCS_H
#define _LDRFUNCS_H

//
// Dependencies
//
#include "ldrtypes.h"
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

#endif
