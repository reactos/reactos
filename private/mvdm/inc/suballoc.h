/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    suballoc.h

Abstract:

    This is the public include file for the suballocation
    package.

Author:

    Dave Hastings (daveh) creation-date 25-Jan-1994

Revision History:


--*/

//
// Constants
//

//
// Minimum granularity for the commit routine
//   this is done as a constant rather than a parameter
//   to make defining data structures easier
//
#ifdef i386
#define COMMIT_GRANULARITY 4096
#else
#define COMMIT_GRANULARITY 65536
#endif

//
// Types
//
//
// Routine for committing a specific region of of the address
// space.  Although the return type is NTSTATUS, the only value
// that is checked is 0 (for STATUS_SUCCESS).  If STATUS_SUCCESS
// is returned, it is assumed that the function worked. If not,
// it is assumed that it failed.  No special meaning is attached to
// particular non-zero values.
//
typedef
NTSTATUS 
(*PSACOMMITROUTINE)(
    ULONG BaseAddress,
    ULONG Size
    );
    
//
// Routine for moving memory around in the address space.
// Note:  This routine MUST correctly handle overlapping
//        source and destination
//
typedef 
VOID
(*PSAMEMORYMOVEROUTINE)(
    ULONG Destination,
    ULONG Source,
    ULONG Size
    );

//
// Public prototypes
//
PVOID
SAInitialize(
    ULONG BaseAddress,
    ULONG Size,
    PSACOMMITROUTINE CommitRoutine,
    PSACOMMITROUTINE DecommitRoutine,
    PSAMEMORYMOVEROUTINE MemoryMoveRoutine
    );
    
BOOL 
SAQueryFree(
    PVOID SubAllocation,
    PULONG FreeBytes,
    PULONG LargestFreeBlock
    );
       
BOOL
SAAllocate(
    PVOID SubAllocation,
    ULONG Size,
    PULONG Address
    );
    
BOOL
SAFree(
    PVOID SubAllocation,
    ULONG Size,
    ULONG Address
    );
    
BOOL
SAReallocate(
    PVOID SubAllocation,
    ULONG OriginalSize,
    ULONG OriginalAddress,
    ULONG NewSize,
    PULONG NewAddress
    );

    
