/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPool.h

Abstract:

    This module contains private Pool package definitions

Author:




Environment:

    Both kernel and user mode

Revision History:


        Made it mode agnostic

        New failure paths:
            Initialization failures of paged/non-paged lock -

            Upon failure we disable pool tracking, rest of the behavior
            remains unchanged, and FxPoolInitialize not being bubbled up
            doesn't become an issue.

--*/

#ifndef _FXPOOL_H_
#define _FXPOOL_H_

//
// Common pool header for small allocations (less than PAGE_SIZE)
//
struct FX_POOL_HEADER {

    PVOID                 Base;

    PFX_DRIVER_GLOBALS    FxDriverGlobals;

    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) ULONG AllocationStart[1];
};

typedef FX_POOL_HEADER* PFX_POOL_HEADER;

#define FX_POOL_HEADER_SIZE      FIELD_OFFSET(FX_POOL_HEADER, AllocationStart)


//
// This structure described an indivdually tracked pool.
//
// The frameworks tracks pool on behalf of the frameworks (global),
// and per driver.
//

struct FX_POOL {
    MxLockNoDynam       NonPagedLock;
    LIST_ENTRY          NonPagedHead;

    MxPagedLockNoDynam  PagedLock;
    LIST_ENTRY          PagedHead;

    // Current Pool Usage Information
    SIZE_T      NonPagedBytes;
    SIZE_T      PagedBytes;

    ULONG       NonPagedAllocations;
    ULONG       PagedAllocations;

    // Peak Pool Usage Information
    SIZE_T      PeakNonPagedBytes;
    SIZE_T      PeakPagedBytes;

    ULONG       PeakNonPagedAllocations;
    ULONG       PeakPagedAllocations;
};

typedef FX_POOL *PFX_POOL;

//
// This structure is allocated along with the pool item and
// is used to track it.
//
// Note: We would be messing up cache aligned if its greater
//       than 16.
//
//       Our struct is 7 DWORD's on an x86, and 11 DWORDS on 64 bit
//       machines.
//
//       This rounds up to 8 or 12 DWORDS.
//
//
struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) FX_POOL_TRACKER {
    LIST_ENTRY Link;
    PFX_POOL   Pool;
    ULONG      Tag;
    SIZE_T     Size;
    POOL_TYPE  PoolType;
    PVOID      CallersAddress;
};

typedef FX_POOL_TRACKER *PFX_POOL_TRACKER;

/*++

Routine Description:

    Initialize the FX_POOL tracking object

Arguments:

    Pool    - FX_POOL object for tracking allocations

Returns:

    status

--*/
_Must_inspect_result_
NTSTATUS
FxPoolInitialize(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PFX_POOL Pool
    );

/*++

Routine Description:

    Destory the FX_POOL tracking object

Arguments:

    Pool    - FX_POOL object for tracking allocations

--*/
VOID
FxPoolDestroy(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PFX_POOL  Pool
    );


extern "C"
_Must_inspect_result_
PWDF_DRIVER_GLOBALS
FxAllocateDriverGlobals(
    VOID
    );

extern "C"
VOID
FxFreeDriverGlobals(
    __in PWDF_DRIVER_GLOBALS DriverGlobals
    );

BOOLEAN
FxIsPagedPoolType(
    __in POOL_TYPE Type
    );

/*++

Routine Description:

    Allocates system pool tracked in a FX_POOL tracking object.

Arguments:

    Pool    - FX_POOL object for tracking allocations

    Type    - POOL_TYPE from ntddk.h

    Size    - Size in bytes of the allocation

    Tag     - Caller specified additional tag value for debugging/tracing

    PVOID   - Caller's address, usefull for finding who allocated the memory

Returns:

    NULL - Could not allocate pool
    !NULL - Pointer to pool of minimum Size bytes

--*/
PVOID
FxPoolAllocator(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PFX_POOL  Pool,
    __in POOL_TYPE Type,
    __in SIZE_T    Size,
    __in ULONG     Tag,
    __in PVOID     CallersAddress
    );

/*++

Routine Description:

    Release tracked pool

Arguments:

    Pool - FX_POOL object allocation is tracked in

    ptr - Pointer to pool to release

Returns:

--*/
void
FxPoolFree(
    __in_xcount(ptr is at an offset from AllocationStart)  PVOID ptr
    );

/*++

Routine Description:

    Dump the FX_POOL tracking object

Arguments:

    Pool    - FX_POOL object for tracking allocations

Returns:

    STATUS_SUCCESS

--*/
NTSTATUS
FxPoolDump(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PFX_POOL  Pool
    );

/*++

Routine Description:

    Initialize the pool support package at startup time.

    This must be called before the first allocation.

Arguments:

Returns:

    STATUS_SUCCESS

--*/
_Must_inspect_result_
NTSTATUS
FxPoolPackageInitialize(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );
/*++

Routine Description:

    Destroy the pool support package at unload time

    This must be after the last free

Arguments:

Returns:

    status

--*/
VOID
FxPoolPackageDestroy(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );



#endif // _FXPOOL_H_
