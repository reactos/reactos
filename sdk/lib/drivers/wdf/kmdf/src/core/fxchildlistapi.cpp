/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    FxChildListApi.cpp

Abstract:

    This module exposes the "C" interface to the FxChildList object.

Author:



Environment:

    Kernel mode only

Revision History:

--*/

#include "fxcorepch.hpp"

extern "C" {
// #include "FxChildListAPI.tmh"
}

//
// extern "C" the entire file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfChildListCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_CHILD_LIST_CONFIG Config,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES DeviceListAttributes,
    __out
    WDFCHILDLIST* DeviceList
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice* pDevice;
    FxChildList* pList;
    size_t totalDescriptionSize = 0;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID*)&pDevice,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Enter, WDFDEVICE %p", Device);

    FxPointerNotNull(pFxDriverGlobals, Config);
    FxPointerNotNull(pFxDriverGlobals, DeviceList);

    *DeviceList = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxChildList::_ValidateConfig(pFxDriverGlobals,
                                          Config,
                                          &totalDescriptionSize);
    if (!NT_SUCCESS(status)) {
         DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "WDFDEVICE 0x%p Config is invalid", Device);
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals,
                                        DeviceListAttributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = pDevice->AllocateEnumInfo();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxChildList::_CreateAndInit(&pList,
                                         pFxDriverGlobals,
                                         DeviceListAttributes,
                                         totalDescriptionSize,
                                         pDevice,
                                         Config);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = pList->Commit(DeviceListAttributes,
                           (WDFOBJECT*)DeviceList,
                           pDevice);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Could not convert object to handle, %!STATUS!",
                            status);
        pList->DeleteFromFailedCreate();
    }

    if (NT_SUCCESS(status)) {
        pDevice->SetDeviceTelemetryInfoFlags(DeviceInfoHasDynamicChildren);
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
STDCALL
WDFEXPORT(WdfChildListGetDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxChildList* pList;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DeviceList,
                                   FX_TYPE_CHILD_LIST,
                                   (PVOID*)&pList,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Enter: WDFCHILDLIST %p", DeviceList);

    return pList->GetDevice();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfChildListRetrieveAddressDescription)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __in
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    __inout
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxChildList* pList;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DeviceList,
                                   FX_TYPE_CHILD_LIST,
                                   (PVOID*)&pList,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Enter: WDFCHILDLIST %p", DeviceList);

    FxPointerNotNull(pFxDriverGlobals, IdentificationDescription);
    FxPointerNotNull(pFxDriverGlobals, AddressDescription);

    if (pList->GetIdentificationDescriptionSize() !=
                    IdentificationDescription->IdentificationDescriptionSize) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "identification description size %d incorrect, expected %d, %!STATUS!",
            IdentificationDescription->IdentificationDescriptionSize,
            pList->GetIdentificationDescriptionSize(), status);

        return status;
    }

    if (pList->HasAddressDescriptions() == FALSE) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "cannot retrieve an address description from a list"
                            " which was not initialized to use them, %!STATUS!",
                            status);

        return status;
    }

    if (pList->GetAddressDescriptionSize() !=
                                AddressDescription->AddressDescriptionSize) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "description size %d incorrect, expected %d, %!STATUS!",
                            AddressDescription->AddressDescriptionSize,
                            pList->GetAddressDescriptionSize(), status);

        return status;
    }

    status = pList->GetAddressDescription(IdentificationDescription,
                                          AddressDescription);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exit: WDFCHILDLIST %p, %!STATUS!",
                        DeviceList, status);

    return status;

}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfChildListBeginScan)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxChildList* pList;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DeviceList,
                                   FX_TYPE_CHILD_LIST,
                                   (PVOID*)&pList,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Enter: WDFCHILDLIST %p", DeviceList);

    pList->BeginScan();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfChildListEndScan)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxChildList* pList;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DeviceList,
                                   FX_TYPE_CHILD_LIST,
                                   (PVOID*)&pList,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Enter: WDFCHILDLIST %p", DeviceList);

    pList->EndScan();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfChildListBeginIteration)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __in
    PWDF_CHILD_LIST_ITERATOR Iterator
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxChildList* pList;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DeviceList,
                                   FX_TYPE_CHILD_LIST,
                                   (PVOID*)&pList,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Enter: WDFCHILDLIST %p", DeviceList);

    FxPointerNotNull(pFxDriverGlobals, Iterator);

    if (Iterator->Size != sizeof(WDF_CHILD_LIST_ITERATOR)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Iterator Size %d not correct, expected %d, %!STATUS!",
                            Iterator->Size, sizeof(WDF_CHILD_LIST_ITERATOR),
                            STATUS_INFO_LENGTH_MISMATCH);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    if ((Iterator->Flags & ~WdfRetrieveAllChildren) != 0) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Iterator Flags 0x%x not correct, valid mask 0x%x, %!STATUS!",
            Iterator->Flags, WdfRetrieveAllChildren, STATUS_INVALID_PARAMETER);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    RtlZeroMemory(&Iterator->Reserved[0], sizeof(Iterator->Reserved));

    pList->BeginIteration(Iterator);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfChildListRetrieveNextDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __in
    PWDF_CHILD_LIST_ITERATOR Iterator,
    __out
    WDFDEVICE* Device,
    __inout_opt
    PWDF_CHILD_RETRIEVE_INFO Info
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxChildList* pList;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DeviceList,
                                   FX_TYPE_CHILD_LIST,
                                   (PVOID*)&pList,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Iterator);
    FxPointerNotNull(pFxDriverGlobals, Device);

    *Device = NULL;

    if (Iterator->Size != sizeof(WDF_CHILD_LIST_ITERATOR)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Iterator Size %d not correct, expected %d, %!STATUS!",
                            Iterator->Size, sizeof(WDF_CHILD_LIST_ITERATOR), status);
        return status;
    }

    if ((Iterator->Flags & ~WdfRetrieveAllChildren) != 0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Iterator Flags 0x%x not correct, valid mask 0x%x, %!STATUS!",
            Iterator->Flags, WdfRetrieveAllChildren, status);
        return status;
    }

    if (Info != NULL) {
        if (Info->Size != sizeof(WDF_CHILD_RETRIEVE_INFO)) {
            status = STATUS_INFO_LENGTH_MISMATCH;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Invalid RetrieveInfo Size %d, expected %d, %!STATUS!",
                Info->Size, sizeof(WDF_CHILD_RETRIEVE_INFO), status);
            return status;
        }
        if (Info->IdentificationDescription != NULL
            &&
            pList->GetIdentificationDescriptionSize() !=
                Info->IdentificationDescription->IdentificationDescriptionSize) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "identification description size %d incorrect, expected %d"
                "%!STATUS!",
                Info->IdentificationDescription->IdentificationDescriptionSize,
                pList->GetIdentificationDescriptionSize(), status);

            return status;
        }

        if (Info->AddressDescription != NULL) {
            if (pList->HasAddressDescriptions() == FALSE) {
                status = STATUS_INVALID_DEVICE_REQUEST;

                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                    "cannot retrieve an address description from a list"
                    " which was not initialized to use them, %!STATUS!",
                    status);

                return status;
            }
            else if (pList->GetAddressDescriptionSize() !=
                            Info->AddressDescription->AddressDescriptionSize) {
                status = STATUS_INVALID_PARAMETER;

                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                    "address description size %d incorrect, expected %d, %!STATUS!",
                    Info->AddressDescription->AddressDescriptionSize,
                    pList->GetAddressDescriptionSize(), status);

                return status;
            }
        }
    }

    return pList->GetNextDevice(Device, Iterator, Info);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfChildListEndIteration)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __in
    PWDF_CHILD_LIST_ITERATOR Iterator
    )
{
    FxChildList* pList;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DeviceList,
                                   FX_TYPE_CHILD_LIST,
                                   (PVOID*)&pList,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Enter: WDFCHILDLIST %p", DeviceList);

    FxPointerNotNull(pFxDriverGlobals, Iterator);

    if (Iterator->Size != sizeof(WDF_CHILD_LIST_ITERATOR)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Iterator Size %d not correct, expected %d, %!STATUS!",
                            Iterator->Size, sizeof(WDF_CHILD_LIST_ITERATOR),
                            STATUS_INFO_LENGTH_MISMATCH);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    if ((Iterator->Flags & ~WdfRetrieveAllChildren) != 0) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Iterator Flags 0x%x not correct, valid mask 0x%x, %!STATUS!",
            Iterator->Flags, WdfRetrieveAllChildren, STATUS_INVALID_PARAMETER);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    pList->EndIteration(Iterator);
}


_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfChildListAddOrUpdateChildDescriptionAsPresent)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __in
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    __in_opt
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxChildList* pList;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DeviceList,
                                   FX_TYPE_CHILD_LIST,
                                   (PVOID*)&pList,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Enter: WDFCHILDLIST %p", DeviceList);

    FxPointerNotNull(pFxDriverGlobals, IdentificationDescription);

    if (AddressDescription != NULL) {
        if (pList->HasAddressDescriptions() == FALSE) {
            status =  STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "cannot retrieve an address description from a list"
                " which was not initialized to use them, %!STATUS!", status);

            return status;
        }
        if (pList->GetAddressDescriptionSize() !=
                                AddressDescription->AddressDescriptionSize) {
            status = STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "address description size %d incorrect, expected %d, %!STATUS!",
                AddressDescription->AddressDescriptionSize,
                pList->GetAddressDescriptionSize(), status);

            return status;
        }
    }
    else {
        if (pList->HasAddressDescriptions()) {
            status =  STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Must provide a valid AddressDescription because the"
                " WDFCHILDLIST 0x%p is configured with AddressDescriptionSize,"
                " %!STATUS!", DeviceList, status);

            return status;
        }
    }

    if (pList->GetIdentificationDescriptionSize() !=
                    IdentificationDescription->IdentificationDescriptionSize) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "identification description size %d incorrect, expected %d, %!STATUS!",
            IdentificationDescription->IdentificationDescriptionSize,
            pList->GetIdentificationDescriptionSize(), status);

        return status;
    }

    status = pList->Add(IdentificationDescription, AddressDescription);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exit: WDFCHILDLIST %p, %!STATUS!", DeviceList, status);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfChildListUpdateChildDescriptionAsMissing)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __in
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxChildList* pList;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DeviceList,
                                   FX_TYPE_CHILD_LIST,
                                   (PVOID*)&pList,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, IdentificationDescription);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Enter: WDFCHILDLIST %p", DeviceList);

    if (pList->GetIdentificationDescriptionSize() !=
                    IdentificationDescription->IdentificationDescriptionSize) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "identification description size %d incorrect, expected %d, %!STATUS!",
            IdentificationDescription->IdentificationDescriptionSize,
            pList->GetIdentificationDescriptionSize(), status);

        return status;
    }

    status = pList->UpdateAsMissing(IdentificationDescription);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exit: WDFCHILDLIST %p, %!STATUS!",
                        DeviceList, status);

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfChildListUpdateAllChildDescriptionsAsPresent)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxChildList* pList;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DeviceList,
                                   FX_TYPE_CHILD_LIST,
                                   (PVOID*)&pList,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Enter: WDFCHILDLIST %p", DeviceList);

    pList->UpdateAllAsPresent();

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exit: WDFCHILDLIST %p", DeviceList);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
STDCALL
WDFEXPORT(WdfChildListRetrievePdo)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __inout
    PWDF_CHILD_RETRIEVE_INFO RetrieveInfo
    )
{
    FxChildList* pList;
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER pId;
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER pAddr;
    FxDevice* device;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DeviceList,
                                   FX_TYPE_CHILD_LIST,
                                   (PVOID*)&pList,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Enter: WDFCHILDLIST %p", DeviceList);

    FxPointerNotNull(pFxDriverGlobals, RetrieveInfo);

    if (RetrieveInfo->Size != sizeof(WDF_CHILD_RETRIEVE_INFO)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Invalid RetrieveInfo Size %x, expected %d, %!STATUS!",
            RetrieveInfo->Size, sizeof(WDF_CHILD_RETRIEVE_INFO),
            STATUS_INFO_LENGTH_MISMATCH);
        return NULL;
    }

    pId = RetrieveInfo->IdentificationDescription;
    if (pId == NULL) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Invalid ID Description, %!STATUS!",
                            STATUS_INVALID_PARAMETER);
        return NULL;
    }

    if (pList->GetIdentificationDescriptionSize() !=
                                        pId->IdentificationDescriptionSize) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "identification description size %d incorrect, expected %d",
            pId->IdentificationDescriptionSize,
            pList->GetIdentificationDescriptionSize());

        return NULL;
    }

    pAddr = RetrieveInfo->AddressDescription;
    if (pAddr != NULL) {
        if (pList->HasAddressDescriptions() == FALSE) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "cannot retrieve an address description from a list"
                " which was not initialized to use them, %!STATUS!",
                STATUS_INVALID_DEVICE_REQUEST);
            return NULL;
        }
        else if (pList->GetAddressDescriptionSize() != pAddr->AddressDescriptionSize) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "address description size %d incorrect, expected %d",
                pAddr->AddressDescriptionSize, pList->GetAddressDescriptionSize());

            return NULL;
        }
    }

    RetrieveInfo->Status = WdfChildListRetrieveDeviceUndefined;

    device = pList->GetDeviceFromId(RetrieveInfo);

    WDFDEVICE handle;

    if (device != NULL) {
        handle = device->GetHandle();
    }
    else {
        handle = NULL;
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exit: WDFCHILDLIST %p, WDFDEVICE Pdo %p, "
                        "%!WDF_CHILD_LIST_RETRIEVE_DEVICE_STATUS!",
                        DeviceList, handle, RetrieveInfo->Status);

    return handle;
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
STDCALL
WDFEXPORT(WdfChildListRequestChildEject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __in
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    )
{
    FxChildList* pList;
    FxDevice* device;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DeviceList,
                                   FX_TYPE_CHILD_LIST,
                                   (PVOID*)&pList,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Enter: WDFCHILDLIST %p", DeviceList);

    FxPointerNotNull(pFxDriverGlobals, IdentificationDescription);

    if (pList->GetIdentificationDescriptionSize() !=
                    IdentificationDescription->IdentificationDescriptionSize) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "identification description size 0x%x incorrect, expected 0x%x",
            IdentificationDescription->IdentificationDescriptionSize,
            pList->GetIdentificationDescriptionSize());

        return FALSE;
    }

    WDF_CHILD_RETRIEVE_INFO info;

    WDF_CHILD_RETRIEVE_INFO_INIT(&info, IdentificationDescription);

    device = pList->GetDeviceFromId(&info);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "found: WDFCHILDLIST %p, WDFDEVICE PDO %p",
                        DeviceList, device == NULL ? NULL : device->GetHandle());

    if (device != NULL) {
        PDEVICE_OBJECT pdo;

        //
        // Make sure we have a valid PDO that can be ejected
        //
        pdo = device->GetSafePhysicalDevice();

        if (pdo != NULL) {
            IoRequestDeviceEject(pdo);
            return TRUE;
        }
        else {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "PDO WDFDEVICE %p not reported yet to pnp, cannot eject!",
                                device->GetHandle());
        }
    }

    return FALSE;
}

} // extern "C" of entire file
