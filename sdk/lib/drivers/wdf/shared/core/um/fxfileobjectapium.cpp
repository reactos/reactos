/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxFileObjectApiUm.cpp

Abstract:

    This modules implements the C API's for the FxFileObject.

Author:



Environment:

    User mode only

Revision History:


--*/

#include "coreprivshared.hpp"
#include "FxFileObject.hpp"

extern "C" {
#include "FxFileObjectApiUm.tmh"
}

//
// Extern "C" the entire file
//
extern "C" {

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfFileObjectClose)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    )
{
    DDI_ENTRY();

    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(FileObject);

    FX_VERIFY_WITH_NAME(INTERNAL, TRAPMSG("Not implemented"), DriverGlobals->DriverName);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
ULONG
WDFEXPORT(WdfFileObjectGetInitiatorProcessId)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    )
{
    DDI_ENTRY();

    FxFileObject* pFO;

    //
    // Validate the FileObject object handle, and get its FxFileObject*
    //
    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         FileObject,
                         FX_TYPE_FILEOBJECT,
                         (PVOID*)&pFO);

    if (pFO->GetWdmFileObject() != NULL) {
        return pFO->GetWdmFileObject()->GetInitiatorProcessId();
    }
    else {
        FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), TRAPMSG("Cannot get initiator "
            "process ID from a file object that doesn't have a WDM file object"),
            DriverGlobals->DriverName);
        return 0;
    }
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
WDFFILEOBJECT
WDFEXPORT(WdfFileObjectGetRelatedFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    )
{
    DDI_ENTRY();

    FxFileObject* pFO;
    FxFileObject* pFoRelated;

    //
    // Validate the FileObject object handle, and get its FxFileObject*
    //
    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         FileObject,
                         FX_TYPE_FILEOBJECT,
                         (PVOID*)&pFO);

    pFoRelated = pFO->GetRelatedFileObject();

    if (pFoRelated != NULL) {
        return pFoRelated->GetHandle();
    }
    else {
        return NULL;
    }
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfFileObjectIncrementProcessKeepAliveCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    )
{
    //
    // This DDI is expected to be called per WDFREQUEST
    // and the caller may be impersonated at that time.
    //
    DDI_ENTRY_IMPERSONATION_OK();

    FxFileObject* pFO;

    //
    // Validate the FileObject object handle, and get its FxFileObject*
    //
    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         FileObject,
                         FX_TYPE_FILEOBJECT,
                         (PVOID*)&pFO);

    return pFO->UpdateProcessKeepAliveCount(TRUE /*Increment*/);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfFileObjectDecrementProcessKeepAliveCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    )
{
    //
    // This DDI is expected to be called per WDFREQUEST
    // and the caller may be impersonated at that time.
    //
    DDI_ENTRY_IMPERSONATION_OK();

    FxFileObject* pFO;

    //
    // Validate the FileObject object handle, and get its FxFileObject*
    //
    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         FileObject,
                         FX_TYPE_FILEOBJECT,
                         (PVOID*)&pFO);

    return pFO->UpdateProcessKeepAliveCount(FALSE /*Increment*/);
}

} // extern "C"
