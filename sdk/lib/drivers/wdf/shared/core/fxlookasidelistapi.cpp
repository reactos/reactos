/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxLookasideListApi.cpp

Abstract:

    This modules implements the C API's for the FxLookasideList.

Author:


Environment:

    kernel mode only

Revision History:

--*/

#include "coreprivshared.hpp"
#include "fxnpagedlookasidelist.hpp"
#include "fxpagedlookasidelist.hpp"

extern "C" {
// #include "FxLookasideListAPI.tmh"
}

extern "C" {

_Must_inspect_result_
__drv_when(PoolType == 1 || PoolType == 257, __drv_maxIRQL(APC_LEVEL))
__drv_when(PoolType == 0 || PoolType == 256, __drv_maxIRQL(DISPATCH_LEVEL))
NTSTATUS
WDFAPI
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxLookasideList *pLookaside;
    WDFLOOKASIDE hLookaside;
    NTSTATUS status;
    FxObject* pParent;

    pParent = NULL;
    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    //
    // Get the parent's globals if it is present
    //
    if (NT_SUCCESS(FxValidateObjectAttributesForParentHandle(
                        pFxDriverGlobals,
                        LookasideAttributes))) {
        FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                       LookasideAttributes->ParentObject,
                                       FX_TYPE_OBJECT,
                                       (PVOID*)&pParent,
                                       &pFxDriverGlobals);
    }
    else if (NT_SUCCESS(FxValidateObjectAttributesForParentHandle(
                            pFxDriverGlobals,
                            MemoryAttributes))) {
        FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                       MemoryAttributes->ParentObject,
                                       FX_TYPE_OBJECT,
                                       (PVOID*)&pParent,
                                       &pFxDriverGlobals);
    }

    FxPointerNotNull(pFxDriverGlobals, PLookaside);

    hLookaside = NULL;
    *PLookaside = NULL;

    if (BufferSize == 0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Zero BufferSize not allowed, %!STATUS!", status);
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, LookasideAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, MemoryAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (PoolTag == 0) {
        PoolTag = pFxDriverGlobals->Tag;
    }

    FxVerifierCheckNxPoolType(pFxDriverGlobals, PoolType, PoolTag);

    //
    // Create the appropriate object
    //
    if (FxIsPagedPoolType(PoolType) == FALSE) {
        if (BufferSize < PAGE_SIZE) {
            pLookaside = new(pFxDriverGlobals, LookasideAttributes)
                FxNPagedLookasideList(pFxDriverGlobals, PoolTag);
        }
        else {
            pLookaside = new(pFxDriverGlobals, LookasideAttributes)
                FxNPagedLookasideListFromPool(pFxDriverGlobals, PoolTag);
        }
    }
    else {
        FxDeviceBase* pLookasideDB, *pMemoryDB;

        status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        pLookasideDB  = FxDeviceBase::_SearchForDevice(pFxDriverGlobals,
                                                       LookasideAttributes);

        pMemoryDB = FxDeviceBase::_SearchForDevice(pFxDriverGlobals,
                                                   MemoryAttributes);

        if (pLookasideDB != NULL && pMemoryDB != NULL &&
            pLookasideDB != pMemoryDB) {
            status = STATUS_INVALID_PARAMETER;

            //
            // No need to check if LookasideAttributes or MemoryAttributes are
            // equal to NULL b/c we could not get a valid pLookasideDB or
            // pMemoryDB if they were NULL.
            //
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Lookaside Attributes ancestor WDFDEVICE %p (from ParentObject %p) "
                " is not the same as Memory Attributes ancestor WDFDEVICE %p "
                "(from ParentObject %p), %!STATUS!",
                pLookasideDB->GetHandle(), LookasideAttributes->ParentObject,
                pMemoryDB->GetHandle(), MemoryAttributes->ParentObject,
                status);

            return status;
        }

        //
        // For paged allocations we always split the WDFMEMORY from its buffer
        // pointer because the memory behind the WDFMEMORY must be non pageable
        // while its buffer is pageable.
        //
        pLookaside = new(pFxDriverGlobals, LookasideAttributes)
            FxPagedLookasideListFromPool(pFxDriverGlobals,
                                         PoolTag,
                                         pLookasideDB,
                                         pMemoryDB);
    }

    if (pLookaside == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = pLookaside->Initialize(BufferSize, MemoryAttributes);

    if (NT_SUCCESS(status)) {
        //
        // Follow the global driver policy and either return a PVOID cookie or
        // an index into a handle table.
        //
        status = pLookaside->Commit(LookasideAttributes, (WDFOBJECT*)&hLookaside);
    }

    if (NT_SUCCESS(status)) {
        *PLookaside = hLookaside;
    }
    else {
        pLookaside->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
STDCALL
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
    FxLookasideList* pLookaside;
    FxMemoryObject *pMemory;
    WDFMEMORY hMemory;
    NTSTATUS status;

    pLookaside = NULL;
    pMemory = NULL;

    //
    // Make sure the caller passed in a valid handle
    //
    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Lookaside,
                         FX_TYPE_LOOKASIDE,
                         (PVOID*)&pLookaside);

    FxPointerNotNull(pLookaside->GetDriverGlobals(), Memory);

    *Memory = NULL;

    status = pLookaside->Allocate(&pMemory);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = pMemory->Commit(&pLookaside->m_MemoryAttributes,
                             (WDFOBJECT*) &hMemory);

    if (NT_SUCCESS(status)) {
        *Memory = hMemory;
    }
    else {
        pMemory->DeleteFromFailedCreate();
    }

    return status;
}

} // extern "C"
