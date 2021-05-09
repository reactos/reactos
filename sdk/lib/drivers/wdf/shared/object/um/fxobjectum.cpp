/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxObjectUm.cpp

Abstract:

    User mode implementations of FxObject APIs

Author:


Environment:

    user mode only

Revision History:

--*/

#include "fxobjectpch.hpp"

extern "C" {

#if defined(EVENT_TRACING)
#include "FxObjectUm.tmh"
#endif

}

extern "C" {

#define INITGUID
#include <guiddef.h>

#include <WdfFileObject_private.h>

//
// Function declarations for the WdfObjectQuery DDIs
//
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfFileObjectIncrementProcessKeepAliveCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfFileObjectDecrementProcessKeepAliveCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    );

}


extern "C" {

_Must_inspect_result_
NTSTATUS
FxObject::_ObjectQuery(
    _In_    FxObject* Object,
    _In_    CONST GUID* Guid,
    _In_    ULONG QueryBufferLength,
    _Out_writes_bytes_(QueryBufferLength)
            PVOID QueryBuffer
    )

/*++

Routine Description:

    Query the object handle for specific information

    This allows dynamic extensions to DDI's.

    Currently, it is used to allow test hooks for verification
    which are not available in a production release.

Arguments:

    Object - Object to query

    Guid - GUID to represent the information/DDI to query for

    QueryBufferLength - Length of QueryBuffer to return data in

    QueryBuffer - Pointer to QueryBuffer

Returns:

    NTSTATUS

--*/

{
    PFX_DRIVER_GLOBALS pFxDriverGlobals = Object->GetDriverGlobals();

    //
    // Design Note: This interface does not look strongly typed
    // but it is. The GUID defines a specific strongly typed
    // contract for QueryBuffer and QueryBufferLength.
    //

#if DBG

    //
    // These operations are only available on checked builds for deep unit
    // testing, code coverage analysis, and model verification.













    //

    // Add code based on the GUID

    // IsEqualGUID(guid1, guid2), DEFINE_GUID, INITGUID, inc\wnet\guiddef.h
#endif

    if (IsEqualGUID(*Guid, GUID_WDFP_FILEOBJECT_INTERFACE)) {

        //
        // Check the query buffer size before performing the cast
        //
        const ULONG RequiredBufferLength = sizeof(WDFP_FILEOBJECT_INTERFACE);

        if (QueryBufferLength < RequiredBufferLength) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Insufficient query buffer size for file object query "
                "Required size %d, %!STATUS!",
                RequiredBufferLength,
                STATUS_BUFFER_TOO_SMALL);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return STATUS_BUFFER_TOO_SMALL;
        }

        if (nullptr == QueryBuffer) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "NULL query buffer for file object query, %!STATUS!",
                STATUS_BUFFER_TOO_SMALL);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return STATUS_INVALID_PARAMETER;
        }

        PWDFP_FILEOBJECT_INTERFACE FileObjectInterface =
            reinterpret_cast<PWDFP_FILEOBJECT_INTERFACE>(QueryBuffer);

        //
        // Check the struct version (require an exact match for a private DDI)
        //
        if (FileObjectInterface->Size != RequiredBufferLength) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Wrong struct version provided for file object query, "
                "%!STATUS!",
                STATUS_INVALID_PARAMETER);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return STATUS_INVALID_PARAMETER;
        }

        FileObjectInterface->WdfpFileObjectIncrementProcessKeepAliveCount =
            WDFEXPORT(WdfFileObjectIncrementProcessKeepAliveCount);
        FileObjectInterface->WdfpFileObjectDecrementProcessKeepAliveCount =
            WDFEXPORT(WdfFileObjectDecrementProcessKeepAliveCount);

        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}

} // extern "C"
