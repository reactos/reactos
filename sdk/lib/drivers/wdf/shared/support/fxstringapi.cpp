/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxStringApi.cpp

Abstract:

    This module implements the "C" interface to the collection object.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "FxSupportPch.hpp"

extern "C"  {
#include "FxStringAPI.tmh"
}

extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfStringCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PCUNICODE_STRING UnicodeString,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES StringAttributes,
    __out
    WDFSTRING* String
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxString* pString;
    NTSTATUS status;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    //
    // Get the parent's globals if it is present
    //
    if (NT_SUCCESS(FxValidateObjectAttributesForParentHandle(pFxDriverGlobals,
                                                             StringAttributes))) {
        FxObject* pParent;

        FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                       StringAttributes->ParentObject,
                                       FX_TYPE_OBJECT,
                                       (PVOID*)&pParent,
                                       &pFxDriverGlobals);
    }

    FxPointerNotNull(pFxDriverGlobals, String);

    *String = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, StringAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (UnicodeString != NULL) {
        status = FxValidateUnicodeString(pFxDriverGlobals, UnicodeString);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    pString = new (pFxDriverGlobals, StringAttributes) FxString(pFxDriverGlobals);

    if (pString != NULL) {
        if (UnicodeString != NULL) {
            status = pString->Assign(UnicodeString);
        }

        if (NT_SUCCESS(status)) {
            status = pString->Commit(StringAttributes, (WDFOBJECT*)String);
        }

        if (!NT_SUCCESS(status)) {
            pString->DeleteFromFailedCreate();
            pString = NULL;
        }
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "Could not allocate WDFSTRING handle, %!STATUS!", status);
    }

    return status;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfStringGetUnicodeString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFSTRING String,
    __out
    PUNICODE_STRING UnicodeString
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxString* pString;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   String,
                                   FX_TYPE_STRING,
                                   (PVOID*) &pString,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, UnicodeString);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return;
    }

    RtlCopyMemory(UnicodeString,
                  pString->GetUnicodeString(),
                  sizeof(UNICODE_STRING));
}

} // extern "C"
