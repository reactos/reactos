/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/ldrfuncs.h
 * PURPOSE:         Defintions for Loader Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _LDRFUNCS_H
#define _LDRFUNCS_H

/* DEPENDENCIES **************************************************************/
#include "ldrtypes.h"

/* FUNCTION TYPES ************************************************************/

/* PROTOTYPES ****************************************************************/

NTSTATUS
STDCALL
LdrAccessResource(
    IN  PVOID BaseAddress,
    IN  PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
    OUT PVOID *Resource OPTIONAL,
    OUT PULONG Size OPTIONAL
);

NTSTATUS
STDCALL
LdrFindResource_U(
    IN  PVOID BaseAddress,
    IN  PLDR_RESOURCE_INFO ResourceInfo,
    IN  ULONG Level,
    OUT PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry
);

NTSTATUS
STDCALL 
LdrFindResourceDirectory_U(
    IN PVOID BaseAddress,
    IN PLDR_RESOURCE_INFO ResourceInfo,
    IN ULONG Level,
    OUT PIMAGE_RESOURCE_DIRECTORY *ResourceDirectory
);

NTSTATUS
STDCALL
LdrGetProcedureAddress(
    IN PVOID BaseAddress,
    IN PANSI_STRING Name,
    IN ULONG Ordinal,
    OUT PVOID *ProcedureAddress
);

#endif
