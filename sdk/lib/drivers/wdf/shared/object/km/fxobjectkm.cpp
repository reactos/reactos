/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxObjectKm.cpp

Abstract:

    Kernel mode implementations of FxObject APIs

Author:


Environment:

    kernel mode only

Revision History:

--*/

#include "fxobjectpch.hpp"

extern "C" {

#if defined(EVENT_TRACING)
#include "FxObjectKm.tmh"
#endif

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
    // PFX_DRIVER_GLOBALS pFxDriverGlobals = Object->GetDriverGlobals();

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
    UNREFERENCED_PARAMETER(Object);
    UNREFERENCED_PARAMETER(Guid);
    UNREFERENCED_PARAMETER(QueryBufferLength);
    UNREFERENCED_PARAMETER(QueryBuffer);
#else
    UNREFERENCED_PARAMETER(Object);
    UNREFERENCED_PARAMETER(Guid);
    UNREFERENCED_PARAMETER(QueryBufferLength);
    UNREFERENCED_PARAMETER(QueryBuffer);
#endif

    return STATUS_NOT_FOUND;
}

} // extern "C"
