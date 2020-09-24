/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRegistryAPI.cpp

Abstract:

    This module implements registry access in the framework

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "FxSupportPch.hpp"

extern "C" {
#include "FxRegistryAPI.tmh"
}

extern "C" {
//
// Not in a public header that we can reach, but is documented
//
NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteValueKey(
    __in IN HANDLE Key,
    __in IN PUNICODE_STRING ValueName
    );
}


//
// Extern "C" the entire file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryOpenKey)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    WDFKEY ParentKey,
    __in
    PCUNICODE_STRING KeyName,
    __in
    ACCESS_MASK DesiredAccess,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    __out
    WDFKEY* Key
    )
{
    DDI_ENTRY();

    FxRegKey* pKey;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    WDFKEY keyHandle;
    HANDLE parentHandle;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);
    keyHandle = NULL;

    if (ParentKey != NULL) {
        FxRegKey* pParent;

        FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                       ParentKey,
                                       FX_TYPE_REG_KEY,
                                       (PVOID*) &pParent,
                                       &pFxDriverGlobals);

        parentHandle = pParent->GetHandle();
    }
    else {
        parentHandle = NULL;

        //
        // Get the parent's globals if it is present
        //
        if (NT_SUCCESS(FxValidateObjectAttributesForParentHandle(pFxDriverGlobals,
                                                                 KeyAttributes))) {
            FxObject* pParent;

            FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                           KeyAttributes->ParentObject,
                                           FX_TYPE_OBJECT,
                                           (PVOID*)&pParent,
                                           &pFxDriverGlobals);
        }
    }

    FxPointerNotNull(pFxDriverGlobals, KeyName);
    FxPointerNotNull(pFxDriverGlobals, Key);

    *Key = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, KeyAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, KeyName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pKey = new (pFxDriverGlobals, KeyAttributes) FxRegKey(pFxDriverGlobals);
    if (pKey == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "Could not allocate memory for a WDFKEY, %!STATUS!", status);

        return status;
    }

    status = pKey->Commit(KeyAttributes, (WDFOBJECT*)&keyHandle);

    if (NT_SUCCESS(status)) {
        status = pKey->Open(parentHandle, KeyName, DesiredAccess);

        if (NT_SUCCESS(status)) {
            *Key = keyHandle;
        }
        else {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                "new WDFKEY object open failed, %!STATUS!", status);
        }
    }

    if (!NT_SUCCESS(status)) {
        pKey->DeleteFromFailedCreate();
        pKey = NULL;
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryCreateKey)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    WDFKEY ParentKey,
    __in
    PCUNICODE_STRING KeyName,
    __in
    ACCESS_MASK DesiredAccess,
    __in
    ULONG CreateOptions,
    __out_opt
    PULONG CreateDisposition,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    __out
    WDFKEY* Key
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRegKey* pKey;
    NTSTATUS status;
    WDFKEY keyHandle;
    HANDLE parentHandle;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    if (ParentKey != NULL) {
        FxRegKey* pParent;

        FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                       ParentKey,
                                       FX_TYPE_REG_KEY,
                                       (PVOID*) &pParent,
                                       &pFxDriverGlobals);

        parentHandle = pParent->GetHandle();
    }
    else {
        parentHandle = NULL;

        //
        // Get the parent's globals if it is present
        //
        if (NT_SUCCESS(FxValidateObjectAttributesForParentHandle(pFxDriverGlobals,
                                                                 KeyAttributes))) {
            FxObject* pParent;

            FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                           KeyAttributes->ParentObject,
                                           FX_TYPE_OBJECT,
                                           (PVOID*)&pParent,
                                           &pFxDriverGlobals);
        }
    }

    FxPointerNotNull(pFxDriverGlobals, KeyName);
    FxPointerNotNull(pFxDriverGlobals, Key);

    *Key = NULL;
    keyHandle = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, KeyAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, KeyName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pKey = new (pFxDriverGlobals, KeyAttributes) FxRegKey(pFxDriverGlobals);

    if (pKey == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "Could not allocate memory for WDFKEY, %!STATUS!", status);

        return status;
    }

    status = pKey->Commit(KeyAttributes, (WDFOBJECT*)&keyHandle);

    if (NT_SUCCESS(status)) {
        status = pKey->Create(parentHandle,
                              KeyName,
                              DesiredAccess,
                              CreateOptions,
                              CreateDisposition);

        if (NT_SUCCESS(status)) {
            *Key = keyHandle;
        }
        else {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                "Registry key creation failed, %!STATUS!", status);
        }
    }

    if (!NT_SUCCESS(status)) {
        pKey->DeleteFromFailedCreate();
        pKey = NULL;
    }

    return status;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfRegistryClose)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRegKey* pKey;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    pKey->Close();

    pKey->DeleteObject();
}

__drv_maxIRQL(PASSIVE_LEVEL)
HANDLE
WDFEXPORT(WdfRegistryWdmGetHandle)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key
    )
{
    DDI_ENTRY();

    FxRegKey* pKey;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Key,
                         FX_TYPE_REG_KEY,
                         (PVOID*) &pKey);

    return pKey->GetHandle();
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryRemoveKey)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRegKey* pKey;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = Mx::MxDeleteKey(pKey->GetHandle());

    if (NT_SUCCESS(status)) {
        //
        // pKey->GetHandle() is now useless, delete the Fx object
        //
        pKey->DeleteObject();
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryRemoveValue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRegKey* pKey;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, ValueName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxPointerNotNull(pFxDriverGlobals, ValueName);

    status = ZwDeleteValueKey(pKey->GetHandle(), (PUNICODE_STRING) ValueName);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryQueryValue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    ULONG ValueLength,
    __out_bcount_opt( ValueLength)
    PVOID Value,
     __out_opt
    PULONG ValueLengthQueried,
     __out_opt
    PULONG ValueType
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRegKey* pKey;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ValueName);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, ValueName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxRegKey::_QueryValue(pFxDriverGlobals,
                                   pKey->GetHandle(),
                                   ValueName,
                                   ValueLength,
                                   Value,
                                   ValueLengthQueried,
                                   ValueType);
    if (!NT_SUCCESS(status)) {
        UCHAR traceLevel = TRACE_LEVEL_ERROR;

        //
        // Label message as Verbose if this is the known pattern of
        // passing a 0-length NULL buffer to query the required buffer size.
        //
        if (status == STATUS_BUFFER_OVERFLOW && Value == NULL && ValueLength == 0) {
            traceLevel = TRACE_LEVEL_VERBOSE;
        }

        DoTraceLevelMessage(pFxDriverGlobals, traceLevel, TRACINGERROR,
                            "WDFKEY %p QueryValue failed, %!STATUS!",
                            Key, status);
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryQueryMemory)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    __drv_strictTypeMatch( 1)
    POOL_TYPE PoolType,
     __in_opt
    PWDF_OBJECT_ATTRIBUTES MemoryAttributes,
    __out
    WDFMEMORY* Memory,
    __out_opt
    PULONG ValueType
    )
{
    DDI_ENTRY();

    FxRegKey* pKey;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    ULONG dataLength;
    PVOID dataBuffer;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ValueName);
    FxPointerNotNull(pFxDriverGlobals, Memory);

    *Memory = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxVerifierCheckNxPoolType(pFxDriverGlobals, PoolType, pFxDriverGlobals->Tag);

    status = FxValidateObjectAttributes(pFxDriverGlobals, MemoryAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, ValueName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Query the buffer length required.
    //
    status = pKey->QueryValue(ValueName, 0, NULL, &dataLength, NULL);
    if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW) {
        return status;
    }

    dataBuffer = FxPoolAllocate(pFxDriverGlobals, PagedPool, dataLength);
    if (dataBuffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "WDFKEY %p KEY_VALUE_PARTIAL_INFORMATION allocation failed, %!STATUS!",
            Key, status);

        return status;
    }

    status = pKey->QueryValue(ValueName, dataLength, dataBuffer, &dataLength, ValueType);
    if (NT_SUCCESS(status)) {
        FxMemoryObject* pObject;

        status = FxMemoryObject::_Create(pFxDriverGlobals,
                                         MemoryAttributes,
                                         PoolType,
                                         pFxDriverGlobals->Tag,
                                         dataLength,
                                         &pObject);

        if (NT_SUCCESS(status)) {
            status  = pObject->Commit(MemoryAttributes, (WDFOBJECT*) Memory);

            if (NT_SUCCESS(status)) {
                RtlCopyMemory(pObject->GetBuffer(),
                              dataBuffer,
                              dataLength);
            }
            else {
                pObject->DeleteFromFailedCreate();
            }
        }
        else {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                "WDFKEY %p WDFMEMORY object create failed, %!STATUS!",
                Key, status);
        }
    }
    else {
        DoTraceLevelMessage( pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                             "WDFKEY %p QueryPartial failed, %!STATUS!",
                             Key, status);
    }

    FxPoolFree(dataBuffer);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryQueryMultiString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES StringsAttributes,
    __in
    WDFCOLLECTION Collection
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDeviceBase* pDeviceBase;
    FxCollection* pCollection;
    FxRegKey* pKey;
    NTSTATUS status;
    ULONG dataLength, type;
    PVOID dataBuffer;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    pDeviceBase = NULL;

    FxPointerNotNull(pFxDriverGlobals, ValueName);
    FxPointerNotNull(pFxDriverGlobals, Collection);

    status = FxValidateObjectAttributes(pFxDriverGlobals, StringsAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, ValueName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Collection,
                         FX_TYPE_COLLECTION,
                         (PVOID*) &pCollection);

    pDeviceBase = FxDeviceBase::_SearchForDevice(pFxDriverGlobals,
                                                 StringsAttributes);

    status = pKey->QueryValue(ValueName, 0, NULL, &dataLength, &type);
    if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "WDFKEY %p QueryPartial failed: %!STATUS!", Key, status);

        return status;
    }

    if (type != REG_MULTI_SZ) {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    dataBuffer = FxPoolAllocate(pFxDriverGlobals, PagedPool, dataLength);
    if (dataBuffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "WDFKEY %p KEY_VALUE_PARTIAL_INFORMATION allocation failed, %!STATUS!",
            Key, status);

        return status;
    }

    status = pKey->QueryValue(ValueName, dataLength, dataBuffer, &dataLength, &type);
    if (NT_SUCCESS(status)) {
        //
        // Verify that the data from the registry is a valid multi-sz string.
        //
        status = FxRegKey::_VerifyMultiSzString(pFxDriverGlobals,
                                                ValueName,
                                                (PWCHAR) dataBuffer,
                                                dataLength);
    }

    if (NT_SUCCESS(status)) {
        ULONG initialCount;
        PWCHAR pCur;

        initialCount = pCollection->Count();
        pCur = (PWCHAR) dataBuffer;

        while (*pCur != UNICODE_NULL) {
            FxString* pString;

            pString = new (pFxDriverGlobals, StringsAttributes)
                FxString(pFxDriverGlobals);

            if (pString != NULL) {
                if (pDeviceBase != NULL) {
                    pString->SetDeviceBase(pDeviceBase);
                }

                status = pString->Assign(pCur);

                if (NT_SUCCESS(status)) {
                    WDFOBJECT dummy;

                    status = pString->Commit(StringsAttributes, &dummy);
                }

                if (NT_SUCCESS(status)) {
                    if (pCollection->Add(pString) == FALSE) {
                        status = STATUS_INSUFFICIENT_RESOURCES;

                        DoTraceLevelMessage(
                            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                            "WDFKEY %p, WDFCOLLECTION %p, collection add failed "
                            "%!STATUS!", Key, Collection, status);
                    }
                }

                if (!NT_SUCCESS(status)) {
                    //
                    // Delete the string we just created
                    //
                    pString->DeleteFromFailedCreate();
                }
                else {
                    //
                    // NT_SUCCES(status)
                    //
                    // Either the caller is responsible for freeing the
                    // WDFSTRING or it has been parented to another object.
                    //
                    DO_NOTHING();
                }
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                    "WDFKEY %p: WDFSTRING creation failed: %!STATUS!",
                    Key, status);
                break;
            }

            //
            // Increment to the next string in the multi sz (length of string +
            // 1 for the NULL)
            //
            pCur += wcslen(pCur) + 1;
        }

        if (!NT_SUCCESS(status)) {
            //
            // Clear out all the items we added to the collection
            //
            while (pCollection->Count() > initialCount) {
                pCollection->Remove(initialCount);
            }
        }
    }

    FxPoolFree(dataBuffer);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryQueryUnicodeString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __out_opt
    PUSHORT ValueByteLength,
    __inout_opt
    PUNICODE_STRING Value
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRegKey* pKey;
    NTSTATUS status;
    ULONG dataLength, type;
    PVOID dataBuffer;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ValueName);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, ValueName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Value != NULL) {
        status = FxValidateUnicodeString(pFxDriverGlobals, Value);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    if (Value == NULL) {
        //
        // Caller wants to know just the length
        //
        dataLength = 0;
        dataBuffer = NULL;
    }
    else {
        dataLength = Value->MaximumLength;
        dataBuffer = FxPoolAllocate(pFxDriverGlobals, PagedPool, dataLength);

        if (dataBuffer == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                "WDFKEY %p KEY_VALUE_PARTIAL_INFORMATION allocation failed, "
                "%!STATUS!", Key, status);

            return status;
        }
    }

    status = pKey->QueryValue(ValueName, dataLength, dataBuffer, &dataLength, &type);
    if (NT_SUCCESS(status) &&
        FxRegKey::_IsValidSzType(type) == FALSE) {
        status = STATUS_OBJECT_TYPE_MISMATCH;
    }

    //
    // Set ValueByteLength before doing the copy
    //
    if (NT_SUCCESS(status) || status == STATUS_BUFFER_OVERFLOW) {
        //
        // pPartial->DataLength is in bytes, convert to number of
        // WCHARs
        //
        if (ValueByteLength != NULL) {
            *ValueByteLength = (USHORT)dataLength ;
        }
    }

    if (NT_SUCCESS(status) && Value != NULL) {




        ASSERT(ValueByteLength == NULL ||
               *ValueByteLength  >= dataLength);

        //
        // pPartial->DataLength cannot be greater than Value->MaximumLength
        // based on the call to _ComputePartialSize above. So it is safe to
        // copy the pPartial data buffer to the Value buffer.
        //
        __analysis_assume(dataLength <= Value->MaximumLength);
        RtlCopyMemory(Value->Buffer, dataBuffer, dataLength);

        //terminating null shouldn't be included in the Length
        Value->Length = (USHORT)dataLength;

        if (Value->Buffer[Value->Length/sizeof(WCHAR)-1] == UNICODE_NULL) {
                Value->Length -= sizeof(WCHAR);
        }
    }

    if (dataBuffer != NULL) {
        FxPoolFree(dataBuffer);
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryQueryString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    WDFSTRING String
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxString* pString;
    FxRegKey* pKey;
    NTSTATUS status;
    ULONG dataLength, type;
    PVOID dataBuffer;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ValueName);
    FxPointerNotNull(pFxDriverGlobals, String);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, ValueName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         String,
                         FX_TYPE_STRING,
                         (PVOID*) &pString);

    status = pKey->QueryValue(ValueName, 0, NULL, &dataLength, &type);
    if (NT_SUCCESS(status) &&
        FxRegKey::_IsValidSzType(type) == FALSE) {
        status = STATUS_OBJECT_TYPE_MISMATCH;
    }

    if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                            "WDFKEY %p, QueryPartial failed, %!STATUS!",
                            Key, status);
        return status;
    }

    dataBuffer = FxPoolAllocate(pFxDriverGlobals, PagedPool, dataLength);
    if (dataBuffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "WDFKEY %p KEY_VALUE_PARTIAL_INFORMATION allocation failed, "
            "%!STATUS!", Key, status);

        return status;
    }

    status = pKey->QueryValue(ValueName, dataLength, dataBuffer, &dataLength, &type);
    if (NT_SUCCESS(status) &&
        FxRegKey::_IsValidSzType(type) == FALSE) {
        status = STATUS_OBJECT_TYPE_MISMATCH;
    }

    if (NT_SUCCESS(status)) {
        if (dataLength <= USHORT_MAX) {
            UNICODE_STRING tmp;

            if (dataLength == 0x0) {
                //
                // Empty string
                //
                tmp.Buffer = L"";
                tmp.Length = 0;
                tmp.MaximumLength = 0;
            }
            else {

                //
                // The string we read may not be NULL terminated, so put it into a
                // UNICODE_STRING.  If the final character is NULL, shorten the
                // length of the string so that it does not include for the NULL.
                //
                // If there are embedded NULLs in the string previous to the final
                // character, we leave them in place.
                //
                tmp.Buffer = (PWCHAR) dataBuffer;
                tmp.Length = (USHORT) dataLength;
                tmp.MaximumLength = tmp.Length;

                if (tmp.Buffer[(tmp.Length/sizeof(WCHAR))-1] == UNICODE_NULL) {
                    //
                    // Do not include the UNICODE_NULL in the length
                    //
                    tmp.Length -= sizeof(WCHAR);
                }
            }

            status = pString->Assign(&tmp);
        }
        else {
            status = STATUS_INVALID_BUFFER_SIZE;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                "WDFKEY %p QueryPartial failed, Length %d > max %d, %!STATUS!",
                Key, dataLength, USHORT_MAX, status);
        }
    }
    else {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "WDFKEY %p QueryPartial failed, Length %d, %!STATUS!",
            Key, dataLength, status);
    }

    FxPoolFree(dataBuffer);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryQueryULong)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __out
    PULONG Value
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRegKey* pKey;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ValueName);
    FxPointerNotNull(pFxDriverGlobals, Value);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, ValueName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxRegKey::_QueryULong(pKey->GetHandle(), ValueName, Value);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "WDFKEY %p, QueryULong, %!STATUS!", Key, status);
    }

    return status;
}


_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryAssignValue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    ULONG ValueType,
    __in
    ULONG ValueLength,
    __in_ecount( ValueLength)
    PVOID Value
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRegKey* pKey;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ValueName);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, ValueName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = pKey->SetValue(ValueName, ValueType, Value, ValueLength);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                            "WDFKEY %p SetValue, %!STATUS!", Key, status);
    }

    return status;
}


_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryAssignMemory)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    ULONG ValueType,
    __in
    WDFMEMORY Memory,
    __in_opt
    PWDFMEMORY_OFFSET MemoryOffsets
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    IFxMemory* pMemory;
    FxRegKey* pKey;
    PVOID pBuffer;
    ULONG length;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ValueName);
    FxPointerNotNull(pFxDriverGlobals, Memory);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, ValueName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Memory,
                         IFX_TYPE_MEMORY,
                         (PVOID*) &pMemory);

    pBuffer = pMemory->GetBuffer();
    length = (ULONG) pMemory->GetBufferSize();

    if (MemoryOffsets != NULL) {
        status = pMemory->ValidateMemoryOffsets(MemoryOffsets);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                "WDFKEY %p, WDFMEMORY %p Offsets overflowed, %!STATUS!",
                Key, Memory, status);

            return status;
        }

        if (MemoryOffsets->BufferLength > 0) {
            status = RtlSizeTToULong(MemoryOffsets->BufferLength, &length);

            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                    "WDFKEY %p, WDFMEMORY %p BufferLength in Offsets truncated, "
                    "%!STATUS!", Key, Memory, status);

                return status;
            }
        }

        pBuffer = WDF_PTR_ADD_OFFSET(pBuffer, MemoryOffsets->BufferOffset);
    }

    status = pKey->SetValue(ValueName, ValueType, pBuffer, length);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                            "WDFKEY handle %p SetValue, %!STATUS!", Key, status);
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryAssignULong)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    ULONG Value
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRegKey* pKey;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ValueName);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, ValueName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = pKey->SetValue(ValueName, REG_DWORD, &Value, sizeof(Value));

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                            "WDFKEY %p SetValue, %!STATUS!",
                            Key, status);
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryAssignUnicodeString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    PCUNICODE_STRING Value
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRegKey* pKey;
    NTSTATUS status;
    PWCHAR tempValueBuf;
    ULONG length;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ValueName);
    FxPointerNotNull(pFxDriverGlobals, Value);

    tempValueBuf = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, ValueName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, Value);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    length = Value->Length + sizeof(UNICODE_NULL);

    //
    // Buffer must be NULL terminated and Length of the buffer must also include the NULL
    // Allocate a temporary buffer and NULL terminate it.
    //
    tempValueBuf = (PWCHAR) FxPoolAllocate(pFxDriverGlobals, PagedPool, length);

    if (tempValueBuf == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                            "WDFKEY %p allocate temporary buffer failed, "
                            "%!STATUS!", Key, status);

        return status;
    }

    //
    // Copy over the string from the callers buffer and make sure it is
    // NULL terminated.
    //
    RtlCopyMemory(tempValueBuf, Value->Buffer, Value->Length);
    tempValueBuf[Value->Length/sizeof(WCHAR)] = UNICODE_NULL;

    status = pKey->SetValue(ValueName, REG_SZ, tempValueBuf, length);

    FxPoolFree(tempValueBuf);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage( pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                             "WDFKEY %p set value failed, %!STATUS!",
                             Key, status);
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryAssignString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    WDFSTRING String
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxString* pString;
    FxRegKey* pKey;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ValueName);
    FxPointerNotNull(pFxDriverGlobals, String);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, ValueName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         String,
                         FX_TYPE_STRING,
                         (PVOID*) &pString);

    status = pKey->SetValue(ValueName,
                            REG_SZ,
                            pString->Buffer(),
                            pString->ByteLength(TRUE));

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                            "WDFKEY handle %p SetValue, %!STATUS!",
                            Key, status);
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryAssignMultiString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    WDFCOLLECTION StringsCollection
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxCollection* pCollection;
    FxRegKey* pKey;
    PWCHAR pValue;
    NTSTATUS status;
    ULONG length;
    BOOLEAN valid;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Key,
                                   FX_TYPE_REG_KEY,
                                   (PVOID*) &pKey,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ValueName);
    FxPointerNotNull(pFxDriverGlobals, StringsCollection);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, ValueName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         StringsCollection,
                         FX_TYPE_COLLECTION,
                         (PVOID *) &pCollection);

    valid = FALSE;

    status = RtlSizeTToULong(
        FxCalculateTotalStringSize(pCollection, TRUE, &valid),
        &length);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                            "WDFCOLLECTION %p, collection too large to fit into "
                            "a ULONG, %!STATUS!", StringsCollection, status);
        return status;
    }

    if (valid == FALSE) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                            "WDFKEY %p,  WDFCOLLECTION %p contains "
                            "non string objects, %!STATUS!",
                            Key, StringsCollection, status);
        return status;
    }

    pValue = (PWCHAR) FxPoolAllocate(pFxDriverGlobals, PagedPool, length);

    if (pValue == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                            "WDFKEY %p allocate for query buffer failed, "
                            "%!STATUS!", Key, status);

        return status;
    }

    FxCopyMultiSz(pValue, pCollection);

    status = pKey->SetValue(ValueName, REG_MULTI_SZ, pValue, length);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage( pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                             "WDFKEY %p SetValue, %!STATUS!",
                             Key, status);
    }

    FxPoolFree(pValue);

    return status;
}

} // extern "C"
