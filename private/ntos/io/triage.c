
/*++

Copyright(c) 1999 Microsoft Corporation

Module Name:

    triage.c

Abstract:

    Triage dump support.

Author:

    Matthew D. Hendel (math) 20-Jan-1999

Comments:

    Do not merge this file with some other file. By leaving it in it's own
    compiland, we avoid having to link with all the other random variables
    in crashlib.


--*/

#include "iop.h"
#include <nt.h>
#include <ntrtl.h>
#include <windef.h>
#include <stdio.h>
#include <malloc.h>
#include <ntiodump.h>
#include <triage.h>
#include <ntverp.h>


#ifndef NtBuildNumber
#  if DBG
#    define NtBuildNumber   (VER_PRODUCTBUILD | 0xC0000000)
#  else
#    define NtBuildNumber (VER_PRODUCTBUILD | 0xF0000000)
# endif
#endif


//
// NOTE: Pages sizes copied from ntos\inc. These must be kept in sync with
// global header files.
//

#define PAGE_SIZE_I386      0x1000
#define PAGE_SIZE_ALPHA     0x2000
#define PAGE_SIZE_IA64      0x2000


ULONG TriageImagePageSize = -1;

BOOLEAN
TriagepVerifyDump(
    IN LPVOID TriageDumpBlock
    );

ULONG
TriagepGetPageSize(
    ULONG Architecture
    );

PTRIAGE_DUMP_HEADER
TriagepGetTriagePointer(
    IN PVOID TriageDumpBlock
    );


#ifdef ALLOC_PRAGMA

#pragma alloc_text (INIT, TriagepVerifyDump)
#pragma alloc_text (INIT, TriagepGetPageSize)
#pragma alloc_text (INIT, TriagepGetTriagePointer)

#pragma alloc_text (INIT, TriageGetVersion)
#pragma alloc_text (INIT, TriageGetDriverCount)
#pragma alloc_text (INIT, TriageGetContext)
#pragma alloc_text (INIT, TriageGetExceptionRecord)
#pragma alloc_text (INIT, TriageGetBugcheckData)
#pragma alloc_text (INIT, TriageGetDriverEntry)

#endif


//++
//
// PULONG
// IndexByUlong(
//     PVOID Pointer,
//     ULONG Index
//     )
//
// Routine Description:
//
//     Return the address Index ULONGs into Pointer. That is,
//     Index * sizeof (ULONG) bytes into Pointer.
//
// Arguments:
//
//     Pointer - Start of region.
//
//     Index - Number of ULONGs to index into.
//
// Return Value:
//
//     PULONG representing the pointer described above.
//
//--

#define IndexByUlong(Pointer,Index) (&(((ULONG*) (Pointer)) [Index]))


//++
//
// PBYTE
// IndexByByte(
//     PVOID Pointer,
//     ULONG Index
//     )
//
// Routine Description:
//
//     Return the address Index BYTEs into Pointer. That is,
//     Index * sizeof (BYTE) bytes into Pointer.
//
// Arguments:
//
//     Pointer - Start of region.
//
//     Index - Number of BYTEs to index into.
//
// Return Value:
//
//     PBYTE representing the pointer described above.
//
//--

#define IndexByByte(Pointer, Index) (&(((BYTE*) (Pointer)) [Index]))


ULONG
TriagepGetPageSize(
    ULONG Architecture
    )
{
    switch (Architecture) {

        case IMAGE_FILE_MACHINE_I386:
            return PAGE_SIZE_I386;

        case IMAGE_FILE_MACHINE_ALPHA:
            return PAGE_SIZE_ALPHA;

        case IMAGE_FILE_MACHINE_IA64:
            return PAGE_SIZE_IA64;

        default:
            return -1;
    }
}



BOOLEAN
TriagepVerifyDump(
    IN LPVOID TriageDumpBlock
    )
{
    BOOLEAN Succ = FALSE;
    PDUMP_HEADER DumpHeader = NULL;
    PTRIAGE_DUMP_HEADER TriageHeader = NULL;

    if (!TriageDumpBlock) {

        return FALSE;
    }
    
    DumpHeader = (PDUMP_HEADER) TriageDumpBlock;

    try {
    
        if (DumpHeader->ValidDump != 'PMUD' ||
            DumpHeader->Signature != 'EGAP' ||
            TriagepGetPageSize (DumpHeader->MachineImageType) == -1) {

            Succ = FALSE;
            leave;
        }

        TriageImagePageSize = TriagepGetPageSize (DumpHeader->MachineImageType);
        
        TriageHeader = (PTRIAGE_DUMP_HEADER)
            IndexByByte ( TriageDumpBlock, TriageImagePageSize );

        if ( *(ULONG*)IndexByUlong (DumpHeader, DH_DUMP_TYPE) != DUMP_TYPE_TRIAGE ||
             *(ULONG*)IndexByByte (DumpHeader, TriageHeader->SizeOfDump - sizeof (DWORD)) != TRIAGE_DUMP_VALID ) {

            Succ = FALSE;
            leave;
        }

        // else

        Succ = TRUE;
    }

    except (EXCEPTION_EXECUTE_HANDLER) {

        Succ = FALSE;
    }

    return Succ;
}


PTRIAGE_DUMP_HEADER
TriagepGetTriagePointer(
    IN PVOID TriageDumpBlock
    )
{
    ASSERT (TriageImagePageSize != -1);
    ASSERT (TriagepVerifyDump (TriageDumpBlock));

    return (PTRIAGE_DUMP_HEADER) IndexByByte (TriageDumpBlock, TriageImagePageSize);
}



NTSTATUS
TriageGetVersion(
    IN LPVOID TriageDumpBlock,
    OUT ULONG * MajorVersion,
    OUT ULONG * MinorVersion,
    OUT ULONG * ServicePackBuild
    )
{
    PTRIAGE_DUMP_HEADER TriageDump;
    PDUMP_HEADER DumpHeader;
    
    if (!TriagepVerifyDump (TriageDumpBlock)) {
        return STATUS_INVALID_PARAMETER;
    }

    TriageDump = TriagepGetTriagePointer (TriageDumpBlock);

    if (!TriageDump) {
        return STATUS_INVALID_PARAMETER;
    }

    DumpHeader = (PDUMP_HEADER) TriageDumpBlock;

    if (MajorVersion) {
        *MajorVersion = DumpHeader->MajorVersion;
    }

    if (MinorVersion) {
        *MinorVersion = DumpHeader->MinorVersion;
    }

    if (ServicePackBuild) {
        *ServicePackBuild = TriageDump->ServicePackBuild;
    }

    return STATUS_SUCCESS;
}



NTSTATUS
TriageGetDriverCount(
    IN LPVOID TriageDumpBlock,
    OUT ULONG * DriverCount
    )
{
    PTRIAGE_DUMP_HEADER TriageDump;
    
    if (!TriagepVerifyDump (TriageDumpBlock)) {
        return STATUS_INVALID_PARAMETER;
    }

    TriageDump = TriagepGetTriagePointer (TriageDumpBlock);

    if (!TriageDump) {
        return STATUS_INVALID_PARAMETER;
    }

    *DriverCount = TriageDump->DriverCount;

    return STATUS_SUCCESS;
}

    

#if 0

NTSTATUS
TriageGetContext(
    IN LPVOID TriageDumpBlock,
    OUT LPVOID Context,
    IN ULONG SizeInBytes
    )
{
    PTRIAGE_DUMP_HEADER TriageDump;
    
    if (!TriagepVerifyDump (TriageDumpBlock)) {
        return STATUS_INVALID_PARAMETER;
    }

    TriageDump = TriagepGetTriagePointer (TriageDumpBlock);

    if (!TriageDump) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Copy the CONTEXT record.
    //

    if (SizeInBytes == -1) {
        SizeInBytes = sizeof (CONTEXT);
    }

    RtlCopyMemory (Context,
                   IndexByUlong (TriageDumpBlock, TriageDump->ContextOffset),
                   SizeInBytes
                   );

    return STATUS_SUCCESS;
}


NTSTATUS
TriageGetExceptionRecord(
    IN LPVOID TriageDumpBlock,
    OUT EXCEPTION_RECORD * ExceptionRecord
    )
{
    PTRIAGE_DUMP_HEADER TriageDump;
    
    if (!TriagepVerifyDump (TriageDumpBlock)) {
        return STATUS_INVALID_PARAMETER;
    }

    TriageDump = TriagepGetTriagePointer (TriageDumpBlock);

    if (!TriageDump) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlCopyMemory (ExceptionRecord,
                   IndexByUlong (TriageDumpBlock, TriageDump->ExceptionOffset),
                   sizeof (*ExceptionRecord)
                   );

    return STATUS_SUCCESS;
}
#endif


LOGICAL
TriageActUpon(
    IN PVOID TriageDumpBlock
    )
{
    PTRIAGE_DUMP_HEADER TriageDump;
    
    if (!TriagepVerifyDump (TriageDumpBlock)) {
        return FALSE;
    }

    TriageDump = TriagepGetTriagePointer (TriageDumpBlock);

    if (!TriageDump) {
        return FALSE;
    }
    
    if ((TriageDump->TriageOptions & DCB_TRIAGE_DUMP_ACT_UPON_ENABLED) == 0) {
        return FALSE;
    }

    return TRUE;
}


NTSTATUS
TriageGetBugcheckData(
    IN LPVOID TriageDumpBlock,
    OUT ULONG * BugCheckCode,
    OUT UINT_PTR * BugCheckParam1,
    OUT UINT_PTR * BugCheckParam2,
    OUT UINT_PTR * BugCheckParam3,
    OUT UINT_PTR * BugCheckParam4
    )
{
    PDUMP_HEADER DumpHeader;
    
    if (!TriagepVerifyDump (TriageDumpBlock)) {
        return STATUS_INVALID_PARAMETER;
    }

    DumpHeader = (PDUMP_HEADER) TriageDumpBlock;
    
    *BugCheckCode = DumpHeader->BugCheckCode;
    *BugCheckParam1 = DumpHeader->BugCheckParameter1;
    *BugCheckParam2 = DumpHeader->BugCheckParameter2;
    *BugCheckParam3 = DumpHeader->BugCheckParameter3;
    *BugCheckParam4 = DumpHeader->BugCheckParameter4;

    return STATUS_SUCCESS;
}



PLDR_DATA_TABLE_ENTRY
TriageGetLoaderEntry(
    IN PVOID TriageDumpBlock,
    IN ULONG ModuleIndex
    )

/*++

Routine Description:

    This function retrieves a loaded module list entry.

Arguments:

    TriageDumpBlock - Supplies the triage dump to reference.

    ModuleIndex - Supplies the driver index number to locate.

Return Value:

    A pointer to a loader data table entry if one is available, NULL if not.

Environment:

    Kernel mode, APC_LEVEL or below.  Phase 0 only.

    N.B. This function is for use by memory management ONLY.

--*/

{
    ULONG i;
    PDUMP_STRING DriverName;
    PDUMP_DRIVER_ENTRY DriverList;
    PTRIAGE_DUMP_HEADER TriageDump;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;

    if (!TriagepVerifyDump (TriageDumpBlock)) {
        return NULL;
    }

    TriageDump = TriagepGetTriagePointer (TriageDumpBlock);

    if (ModuleIndex >= TriageDump->DriverCount) {
        return NULL;
    }
    
    DriverList = (PDUMP_DRIVER_ENTRY)
            IndexByByte (TriageDumpBlock, TriageDump->DriverListOffset);

        
    DataTableEntry = &DriverList [ ModuleIndex ].LdrEntry;

    //
    // Repoint the module driver name into the triage buffer.
    //

    DriverName = (PDUMP_STRING)
            IndexByByte (TriageDumpBlock,
                         DriverList [ ModuleIndex ].DriverNameOffset);

    DataTableEntry->BaseDllName.Length = (USHORT) (DriverName->Length * sizeof (WCHAR));
    DataTableEntry->BaseDllName.MaximumLength = DataTableEntry->BaseDllName.Length;
    DataTableEntry->BaseDllName.Buffer = DriverName->Buffer;

    return DataTableEntry;
}


PVOID
TriageGetMmInformation(
    IN PVOID TriageDumpBlock
    )

/*++

Routine Description:

    This function retrieves a loaded module list entry.

Arguments:

    TriageDumpBlock - Supplies the triage dump to reference.

Return Value:

    A pointer to an opaque Mm information structure.

Environment:

    Kernel mode, APC_LEVEL or below.  Phase 0 only.

    N.B. This function is for use by memory management ONLY.

--*/

{
    PTRIAGE_DUMP_HEADER TriageDump;
    
    if (!TriagepVerifyDump (TriageDumpBlock)) {
        return NULL;
    }

    TriageDump = TriagepGetTriagePointer (TriageDumpBlock);

    if (!TriageDump) {
        return NULL;
    }

    return (PVOID)IndexByByte (TriageDumpBlock, TriageDump->MmOffset);
}
