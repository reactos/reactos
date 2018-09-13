/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tsecomm.c

Abstract:

    Common security definitions and routines.

    This module defines macros that provide a mode-independent
    interface for security test procedures.

    The mode must be specified by defining one, but not both,
    of:
         _TST_USER_    (for user mode tests)
         _TST_KERNEL_  (for kernel mode tests)

Author:

    Jim Kelly       (JimK)     23-Mar-1990

Environment:

    Test of security.

Revision History:

--*/

#ifndef _TSECOMM_
#define _TSECOMM_


////////////////////////////////////////////////////////////////
//                                                            //
// Common Definitions                                         //
//                                                            //
////////////////////////////////////////////////////////////////

#define SEASSERT_SUCCESS(s) {                                                 \
            if (!NT_SUCCESS((s))) {                                              \
                DbgPrint("** ! Failed ! **\n");                               \
                DbgPrint("Status is: 0x%lx \n", (s));                         \
            }                                                                 \
            ASSERT(NT_SUCCESS(s)); }



////////////////////////////////////////////////////////////////
//                                                            //
// Kernel Mode Definitions                                    //
//                                                            //
////////////////////////////////////////////////////////////////

#ifdef _TST_KERNEL_

#define TstAllocatePool(PoolType,NumberOfBytes)  \
    (ExAllocatePool( (PoolType), (NumberOfBytes) ))

#define TstDeallocatePool(Pointer, NumberOfBytes) \
    (ExFreePool( (Pointer) ))

#endif // _TST_KERNEL_


////////////////////////////////////////////////////////////////
//                                                            //
// User Mode Definitions                                      //
//                                                            //
////////////////////////////////////////////////////////////////


#ifdef _TST_USER_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


#include "sep.h"

#define TstAllocatePool(IgnoredPoolType,NumberOfBytes)    \
    (ITstAllocatePool( (NumberOfBytes) ))

#define TstDeallocatePool(Pointer, NumberOfBytes) \
    (ITstDeallocatePool((Pointer),(NumberOfBytes) ))

PVOID
ITstAllocatePool(
    IN ULONG NumberOfBytes
    )
{
    NTSTATUS Status;
    PVOID PoolAddress = NULL;
    ULONG RegionSize;

    RegionSize = NumberOfBytes;

    Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                      &PoolAddress,
                                      0,
                                      &RegionSize,
                                      MEM_COMMIT,
                                      PAGE_READWRITE
                                    );

    return PoolAddress;
}

VOID
ITstDeallocatePool(
    IN PVOID Pointer,
    IN ULONG NumberOfBytes
    )
{
    NTSTATUS Status;
    PVOID PoolAddress;
    ULONG RegionSize;

    RegionSize = NumberOfBytes;
    PoolAddress = Pointer;

    Status = NtFreeVirtualMemory( NtCurrentProcess(),
                                  &PoolAddress,
                                  &RegionSize,
                                  MEM_DECOMMIT
                                  );

    return;
}
#endif // _TST_USER_

#endif //_TSECOMM_
