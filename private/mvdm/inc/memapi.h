/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    memapi.h

Abstract:

    This header defines the function prototypes for the memory
    manipulation routines in the 486 emulator.

Author:

    Neil Sandlin (neilsa)

Notes:

    
Revision History:


--*/

NTSTATUS
VdmAllocateVirtualMemory(
    PULONG Address,
    ULONG Size,
    BOOLEAN Commit
    );

NTSTATUS 
VdmFreeVirtualMemory(
    ULONG Address
    );

NTSTATUS
VdmCommitVirtualMemory(
    ULONG Address,
    ULONG Size
    );

NTSTATUS
VdmDeCommitVirtualMemory(
    ULONG Address,
    ULONG Size
    );

NTSTATUS
VdmQueryFreeVirtualMemory(
    PULONG FreeBytes,
    PULONG LargestFreeBlock
    );

NTSTATUS
VdmReallocateVirtualMemory(
    ULONG OldAddress,
    PULONG NewAddress,
    ULONG NewSize
    );

NTSTATUS
VdmAddVirtualMemory(
    ULONG HostAddress,
    ULONG Size,
    PULONG IntelAddress
    );

NTSTATUS
VdmRemoveVirtualMemory(
    ULONG IntelAddress
    );


BOOL
VdmAddDescriptorMapping(
    USHORT SelectorStart,
    USHORT SelectorCount,
    ULONG LdtBase,
    ULONG Flat
    );
