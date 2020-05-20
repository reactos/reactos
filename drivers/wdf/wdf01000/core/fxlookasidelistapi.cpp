/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Lookaside api functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "wdf.h"



extern "C" {

_Must_inspect_result_
__drv_when(PoolType == 1 || PoolType == 257, __drv_maxIRQL(APC_LEVEL))
__drv_when(PoolType == 0 || PoolType == 256, __drv_maxIRQL(DISPATCH_LEVEL))
NTSTATUS
WDFAPI
WDFEXPORT(WdfLookasideListCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES LookasideAttributes,
    __in
    __drv_when(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    __in
    __drv_strictTypeMatch(__drv_typeExpr)
    POOL_TYPE PoolType,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES MemoryAttributes,
    __in_opt
    ULONG PoolTag,
    __out
    WDFLOOKASIDE* PLookaside
    )
/*++

Routine Description:
    Creates a WDFLOOKASIDE list handle.  The returned handle can then create
    WDFMEMORY handles on behalf of the client driver.  The underlying
    WDFLOOKASIDE is a true NTOS lookaside list (of the appropriate paged or
    npaged variety).

Arguments:
    LookasideAttributes - Object attributes for the lookaside handle being
        created

    BufferSize - Specifies how big each buffer created by the lookaside is

    PoolType - Indicates whether the lookaside list is to create paged or
        nonpaged WDFMEMORY handles.

    MemoryAttributes - Attributes to be associated with each memory handle created
        using the created lookaside list handle

    PoolTag - Pool tag to use for each allocation.  If 0, the frameworks tag
        will be used

    PLookaside - Pointer to store the created handle

Return Value:
    STATUS_INVALID_PARAMETER if any of the required parameters are incorrect

    STATUS_INSUFFICIENT_RESOURCES if no memory is available to create the list

    STATUS SUCCESS if succesful

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfMemoryCreateFromLookaside)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFLOOKASIDE Lookaside,
    __out
    WDFMEMORY* Memory
    )
/*++

Routine Description:
    Allocates a WDFMEMORY handle from a lookaside HANDLE that the caller
    previously created with WdfLookasideListCreate.

Arguments:
    Lookaside - Handle to a lookaside list previously created by the caller

    Memory - Handle to be returned to the caller

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

} // extern "C"
