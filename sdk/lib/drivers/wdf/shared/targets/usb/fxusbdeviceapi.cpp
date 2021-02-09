/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxUsbDeviceAPI.cpp

Abstract:


Author:

Environment:

    Both kernel and user mode

Revision History:

--*/

#include "fxusbpch.hpp"

extern "C" {
#include "FxUsbDeviceAPI.tmh"
}

//
// Extern "C" all APIs
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
FxUsbTargetDeviceCreate(
    __in
    PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in
    FxDeviceBase* Device,
    __in
    ULONG USBDClientContractVersion,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFUSBDEVICE* UsbDevice
    )
/*++

Routine Description:
    Creates a WDFUSBDEVICE handle for the client.

Arguments:

    Device - FxDeviceBase object

    USBDClientContractVersion - The USBD Client Contract version of the client driver

    UsbDevice - Pointer which will receive the created handle

Return Value:
    STATUS_SUCCESS - success
    STATUS_INSUFFICIENT_RESOURCES - no memory available
    ...

  --*/
{
    FxUsbDevice* pUsbDevice;
    NTSTATUS status;

    //
    // Basic parameter validation
    //

    FxPointerNotNull(FxDriverGlobals, UsbDevice);
    *UsbDevice = NULL;

    status = FxVerifierCheckIrqlLevel(FxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateObjectAttributes(FxDriverGlobals,
                                        Attributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pUsbDevice = new(FxDriverGlobals, Attributes) FxUsbDevice(FxDriverGlobals);
    if (pUsbDevice == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Perform all init and handle creation functions.  Check for error at the
    // end and clean up there.
    //
    status = pUsbDevice->Init(Device);

    if (NT_SUCCESS(status)) {
        WDFOBJECT device;

        device = NULL;

        status = pUsbDevice->InitDevice(USBDClientContractVersion);

        if (NT_SUCCESS(status)) {
            status = pUsbDevice->CreateInterfaces();
        }

        if (NT_SUCCESS(status)) {
            status = Device->AddIoTarget(pUsbDevice);
        }

        if (NT_SUCCESS(status)) {
            status = pUsbDevice->Commit(Attributes, &device, Device);
        }

        if (NT_SUCCESS(status)) {
            *UsbDevice = (WDFUSBDEVICE) device;
        }
    }

    if (!NT_SUCCESS(status)) {
        //
        // And now free it
        //
        pUsbDevice->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFUSBDEVICE* UsbDevice
    )
/*++

Routine Description:
    Creates a WDFUSBDEVICE handle for the client.

Arguments:
    Device - WDFDEVICE handle to which we are attaching the WDFUSBDEVICE handle
        to
    PUsbDevice - Pointer which will receive the created handle

Return Value:
    STATUS_SUCCESS - success
    STATUS_INSUFFICIENT_RESOURCES - no memory available
    ...

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDeviceBase* pDevice;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE_BASE,
                                   (PVOID*)&pDevice,
                                   &pFxDriverGlobals);

    return FxUsbTargetDeviceCreate(pFxDriverGlobals,
                                   pDevice,
                                   USBD_CLIENT_CONTRACT_VERSION_INVALID,
                                   Attributes,
                                   UsbDevice);
}

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceCreateWithParameters)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_USB_DEVICE_CREATE_CONFIG Config,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFUSBDEVICE* UsbDevice
    )
/*++

Routine Description:
    Creates a WDFUSBDEVICE handle for the client.

Arguments:
    Device - WDFDEVICE handle to which we are attaching the WDFUSBDEVICE handle
        to
    PUsbDevice - Pointer which will receive the created handle

Return Value:
    STATUS_SUCCESS - success
    STATUS_INSUFFICIENT_RESOURCES - no memory available
    ...

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDeviceBase* pDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE_BASE,
                                   (PVOID*)&pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Config);

    if (Config->Size != sizeof(WDF_USB_DEVICE_CREATE_CONFIG)) {
        status = STATUS_INFO_LENGTH_MISMATCH;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDF_USB_DEVICE_CREATE_CONFIG Size 0x%x, expected 0x%x, %!STATUS!",
            Config->Size, sizeof(WDF_USB_DEVICE_CREATE_CONFIG), status);

        return status;
    }

    return FxUsbTargetDeviceCreate(pFxDriverGlobals,
                                   pDevice,
                                   Config->USBDClientContractVersion,
                                   Attributes,
                                   UsbDevice);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceRetrieveInformation)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __out
    PWDF_USB_DEVICE_INFORMATION Information
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Information);

    if (Information->Size != sizeof(WDF_USB_DEVICE_INFORMATION)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Information size %d, expected %d %!STATUS!",
                            Information->Size, sizeof(WDF_USB_DEVICE_INFORMATION),
                            status);
        return status;
    }

    pUsbDevice->GetInformation(Information);

    return STATUS_SUCCESS;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceGetDeviceDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __out
    PUSB_DEVICE_DESCRIPTOR UsbDeviceDescriptor
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, UsbDeviceDescriptor);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return;
    }

    pUsbDevice->CopyDeviceDescriptor(UsbDeviceDescriptor);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceRetrieveConfigDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __out_bcount_part_opt(*ConfigDescriptorLength, *ConfigDescriptorLength)
    PVOID ConfigDescriptor,
    __inout
    PUSHORT ConfigDescriptorLength
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ConfigDescriptorLength);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return pUsbDevice->GetConfigDescriptor(ConfigDescriptor,
                                           ConfigDescriptorLength);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceQueryString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_ecount_opt(*NumCharacters)
    PUSHORT String,
    __inout
    PUSHORT NumCharacters,
    __in
    UCHAR StringIndex,
    __in_opt
    USHORT LangID
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, NumCharacters);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateRequestOptions(pFxDriverGlobals, RequestOptions);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = pUsbDevice->GetString(String,
                                   NumCharacters,
                                   StringIndex,
                                   LangID,
                                   Request,
                                   RequestOptions);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceAllocAndQueryString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES StringMemoryAttributes,
    __out
    WDFMEMORY*   StringMemory,
    __out_opt
    PUSHORT NumCharacters,
    __in
    UCHAR StringIndex,
    __in_opt
    USHORT LangID
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WDFMEMORY hMemory;
    FxUsbDevice* pUsbDevice;
    NTSTATUS status;
    USHORT numChars = 0;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, StringMemory);

    *StringMemory = NULL;
    if (NumCharacters != NULL) {
        *NumCharacters = 0;
    }

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, StringMemoryAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = pUsbDevice->GetString(NULL, &numChars, StringIndex, LangID);

    if (NT_SUCCESS(status) && numChars > 0) {
        FxMemoryObject* pBuffer;

        status = FxMemoryObject::_Create(pFxDriverGlobals,
                                         StringMemoryAttributes,
                                         NonPagedPool,
                                         pFxDriverGlobals->Tag,
                                         numChars * sizeof(WCHAR),
                                         &pBuffer);

        if (!NT_SUCCESS(status)) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = pBuffer->Commit(StringMemoryAttributes,
                                 (WDFOBJECT*)&hMemory);

        if (NT_SUCCESS(status)) {
            status = pUsbDevice->GetString((PUSHORT) pBuffer->GetBuffer(),
                                           &numChars,
                                           StringIndex,
                                           LangID);

            if (NT_SUCCESS(status)) {
                if (NumCharacters != NULL) {
                    *NumCharacters  = numChars;
                }
                *StringMemory = hMemory;
            }
        }

        if (!NT_SUCCESS(status)) {
            //
            // There can only be one context on this object right now,
            // so just clear out the one.
            //
            pBuffer->DeleteFromFailedCreate();
        }
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceFormatRequestForString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    WDFREQUEST Request,
    __in
    WDFMEMORY Memory,
    __in_opt
    PWDFMEMORY_OFFSET Offset,
    __in
    UCHAR StringIndex,
    __in_opt
    USHORT LangID
    )
/*++

Routine Description:
    Formats a request so that it can be used to query for a string  from the
    device.

Arguments:
    UsbDevice - device to be queried

    Request - request to format

    Memory - memory to write the string into


Return Value:


  --*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    IFxMemory* pMemory;
    FxUsbDevice* pUsbDevice;
    FxRequestBuffer buf;
    FxRequest* pRequest;
    NTSTATUS status;
    size_t bufferSize;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(
        pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "WDFUSBDEVICE %p, WDFREQUEST %p, WDFMEMORY %p, StringIndex %d, LandID 0x%x",
        UsbDevice, Request, Memory, StringIndex, LangID);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Memory,
                         IFX_TYPE_MEMORY,
                         (PVOID*) &pMemory);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*) &pRequest);

    status = pMemory->ValidateMemoryOffsets(Offset);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    buf.SetMemory(pMemory, Offset);

    bufferSize = buf.GetBufferLength();

    //
    // the string descriptor is array of WCHARs so the buffer being used must be
    // of an integral number of them.
    //
    if ((bufferSize % sizeof(WCHAR)) != 0) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "WDFMEMORY %p length must be even number of WCHARs, but is %I64d in "
            "length, %!STATUS!",
            Memory, bufferSize, status);

        return status;
    }

    //
    // While the length in the descriptor (bLength) is a byte, so the requested
    // buffer cannot be bigger then that, do not check the bufferSize < 0xFF.
    // We don't check because the driver can be using the WDFMEMORY as a part
    // of a larger structure.
    //

    //
    // Format the request
    //
    status = pUsbDevice->FormatStringRequest(pRequest, &buf, StringIndex, LangID);

    if (NT_SUCCESS(status)) {
        FxUsbDeviceStringContext* pContext;

        pContext = (FxUsbDeviceStringContext*) pRequest->GetContext();
        pContext->m_UsbParameters.Parameters.DeviceString.Buffer = Memory;
        pContext->m_UsbParameters.Parameters.DeviceString.StringIndex = StringIndex;
        pContext->m_UsbParameters.Parameters.DeviceString.LangID = LangID;
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBDEVICE %p, WDFREQUEST %p, WDFMEMORY %p, %!STATUS!",
                        UsbDevice, Request, Memory, status);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceSelectConfig)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES PipesAttributes,
    __inout
    PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Params);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Params->Size != sizeof(WDF_USB_DEVICE_SELECT_CONFIG_PARAMS)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Params size %d, expected %d %!STATUS!",
                            Params->Size, sizeof(WDF_USB_DEVICE_SELECT_CONFIG_PARAMS),
                            status);
        return status;
    }

    //
    // Sanity check the Type value as well to be in range.
    //
    if (Params->Type < WdfUsbTargetDeviceSelectConfigTypeDeconfig
        ||
        Params->Type > WdfUsbTargetDeviceSelectConfigTypeUrb) {

        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Params Type %d not a valid value, %!STATUS!",
                            Params->Size, status);

        return status;
    }

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    if ((Params->Type == WdfUsbTargetDeviceSelectConfigTypeDeconfig) ||
        (Params->Type == WdfUsbTargetDeviceSelectConfigTypeUrb) ||
        (Params->Type == WdfUsbTargetDeviceSelectConfigTypeInterfacesDescriptor)) {
        status = STATUS_NOT_SUPPORTED;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Params Type %d not supported for UMDF, %!STATUS!",
                            Params->Type, status);

        return status;

    }
#endif

    status = FxValidateObjectAttributes(pFxDriverGlobals,
                                        PipesAttributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (pUsbDevice->HasMismatchedInterfacesInConfigDescriptor()) {
        //
        // Config descriptor reported zero interfaces, but we found an
        // interface descriptor in it.
        //
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFUSBDEVICE %p number of interfaces found in the config descriptor "
            "does not match bNumInterfaces in config descriptor, failing config "
            "operation %!WdfUsbTargetDeviceSelectConfigType!, %!STATUS!",
            UsbDevice, Params->Type, status);

        return status;
    }
    else if (pUsbDevice->GetNumInterfaces() == 0) {
        //
        // Special case the zero interface case and exit early
        //
        status = STATUS_SUCCESS;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFUSBDEVICE %p succeeding config operation "
            "%!WdfUsbTargetDeviceSelectConfigType! on zero interfaces "
            "immediately, %!STATUS!", UsbDevice, Params->Type, status);

        return status;
    }

    switch (Params->Type) {

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)

    case WdfUsbTargetDeviceSelectConfigTypeDeconfig:
        status = pUsbDevice->Deconfig();
        break;

    case WdfUsbTargetDeviceSelectConfigTypeInterfacesDescriptor:
        if (Params->Types.Descriptor.InterfaceDescriptors == NULL ||
            Params->Types.Descriptor.NumInterfaceDescriptors == 0) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Either  InterfaceDescriptor is NULL or  NumInterfaceDescriptors is zero "
                "WDFUSBDEVICE %p InterfaceDescriptor %p NumInterfaceDescriptors 0x%x"
                "%!WdfUsbTargetDeviceSelectConfigType! %!STATUS!", UsbDevice,
                Params->Types.Descriptor.InterfaceDescriptors,
                Params->Types.Descriptor.NumInterfaceDescriptors,
                Params->Type,
                status);

        }
        else {
            status = pUsbDevice->SelectConfigDescriptor(
                PipesAttributes,
                Params);
        }
        break;

    case WdfUsbTargetDeviceSelectConfigTypeUrb:
        //
        // Since the USBD macro's dont include the  USBD_PIPE_INFORMATION for 0 EP's,
        // make the length check to use
        // sizeof(struct _URB_SELECT_CONFIGURATION) - sizeof(USBD_PIPE_INFORMATION)
        //
        if (Params->Types.Urb.Urb == NULL ||
            Params->Types.Urb.Urb->UrbHeader.Function != URB_FUNCTION_SELECT_CONFIGURATION ||
            Params->Types.Urb.Urb->UrbHeader.Length <
               (sizeof(struct _URB_SELECT_CONFIGURATION) - sizeof(USBD_PIPE_INFORMATION))) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Either URB passed in was NULL or the URB Function or Length was invalid "
                " WDFUSBDEVICE %p Urb 0x%p "
                "%!WdfUsbTargetDeviceSelectConfigType!"
                " %!STATUS!", UsbDevice,
                Params->Types.Urb.Urb,
                Params->Type,
                status);

        }
        else {
            status = pUsbDevice->SelectConfig(
                                              PipesAttributes,
                                              Params->Types.Urb.Urb,
                                              FxUrbTypeLegacy,
                                              NULL);
        }
        break;

#endif





    case WdfUsbTargetDeviceSelectConfigTypeInterfacesPairs:
        if (Params->Types.MultiInterface.Pairs == NULL) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "WDFUSBDEVICE %p SettingPairs Array passed is NULL, %!STATUS!",
                pUsbDevice->GetHandle(),status);

            break;
        }
        else if (Params->Types.MultiInterface.NumberInterfaces !=
                                        pUsbDevice->GetNumInterfaces()) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "WDFUSBDEVICE %p MultiInterface.NumberInterfaces %d != %d "
                "(reported num interfaces), %!STATUS!",
                pUsbDevice->GetHandle(),
                Params->Types.MultiInterface.NumberInterfaces,
                pUsbDevice->GetNumInterfaces(), status);

            break;
        }

        //    ||      ||   Fall through     ||        ||
        //    \/      \/                    \/        \/
    case WdfUsbTargetDeviceSelectConfigTypeMultiInterface:

        //
        // Validate SettingIndexes passed-in
        //
        for (ULONG i = 0;
             i < Params->Types.MultiInterface.NumberInterfaces;
             i++) {

            PWDF_USB_INTERFACE_SETTING_PAIR  pair;
            FxUsbInterface * pUsbInterface;
            UCHAR numSettings;

            pair = &(Params->Types.MultiInterface.Pairs[i]);

            FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                                 pair->UsbInterface,
                                 FX_TYPE_USB_INTERFACE,
                                 (PVOID*) &pUsbInterface);

            numSettings = pUsbInterface->GetNumSettings();

            if (pair->SettingIndex >= numSettings) {
                status = STATUS_INVALID_PARAMETER;

                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "WDFUSBDEVICE %p SettingPairs contains invalid SettingIndex"
                    " for WDFUSBINTERFACE %p. Setting index passed in: %d, "
                    "max index: %d, returning %!STATUS!",
                    pUsbDevice->GetHandle(),
                    pair->UsbInterface,
                    pair->SettingIndex,
                    pUsbInterface->GetNumSettings() - 1,
                    status);

                return status;
            }
        }

        status = pUsbDevice->SelectConfigMulti(
            PipesAttributes,
            Params);
        break;

    case WdfUsbTargetDeviceSelectConfigTypeSingleInterface:
        status = pUsbDevice->SelectConfigSingle(   //vm changed name from  SelectConfigAuto
            PipesAttributes,
            Params);
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}


__drv_maxIRQL(DISPATCH_LEVEL)
UCHAR
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceGetNumInterfaces)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice
    )
{
    DDI_ENTRY();

    FxUsbDevice* pUsbDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         UsbDevice,
                         FX_TYPE_IO_TARGET_USB_DEVICE,
                         (PVOID*) &pUsbDevice);

    return pUsbDevice->GetNumInterfaces();
}


__drv_maxIRQL(DISPATCH_LEVEL)
USBD_CONFIGURATION_HANDLE
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceWdmGetConfigurationHandle)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice
    )
{
    DDI_ENTRY();

    FxUsbDevice* pUsbDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         UsbDevice,
                         FX_TYPE_IO_TARGET_USB_DEVICE,
                         (PVOID*) &pUsbDevice);

    return pUsbDevice->GetConfigHandle();
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceSendControlTransferSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in
    PWDF_USB_CONTROL_SETUP_PACKET SetupPacket,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    __out_opt
    PULONG BytesTransferred
    )
/*++

Routine Description:
    Synchronously sends a control transfer to the default control pipe on the
    device.

Arguments:
    UsbDevice - the target representing the device

    Request - Request whose PIRP to use

    RequestOptions - options to use when sending the request

    SetupPacket - control setup packet to be used in the transfer

    MemoryDescriptor - memory to use in the transfer after the setup packet

    BytesTransferred - number of bytes sent to or by the device

Return Value:
    NTSTATUS

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;
    NTSTATUS status;
    FxRequestBuffer buf;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    FxUsbDeviceControlContext context(FxUrbTypeLegacy);

    FxSyncRequest request(pFxDriverGlobals, &context, Request);

    //
    // FxSyncRequest always succeesds for KM but can fail for UM.
    //
    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        return status;
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBDEVICE %p control transfer sync", UsbDevice);

    FxPointerNotNull(pFxDriverGlobals, SetupPacket);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateRequestOptions(pFxDriverGlobals, RequestOptions);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = buf.ValidateMemoryDescriptor(
        pFxDriverGlobals,
        MemoryDescriptor,
        MemoryDescriptorNullAllowed | MemoryDescriptorNoBufferAllowed
        );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = pUsbDevice->FormatControlRequest(request.m_TrueRequest,
                                              SetupPacket,
                                              &buf);

    if (NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "WDFUSBDEVICE %p, WDFREQUEST %p being submitted",
            UsbDevice, request.m_TrueRequest->GetTraceObjectHandle());

        status = pUsbDevice->SubmitSync(request.m_TrueRequest, RequestOptions);

        if (BytesTransferred != NULL) {
            if (NT_SUCCESS(status)) {
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
                *BytesTransferred = context.m_Urb->TransferBufferLength;
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
                *BytesTransferred = context.m_UmUrb.UmUrbControlTransfer.TransferBufferLength;
#endif
            }
            else {
                *BytesTransferred = 0;
            }
        }
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBDEVICE %p, %!STATUS!", UsbDevice, status);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceFormatRequestForControlTransfer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    WDFREQUEST Request,
    __in
    PWDF_USB_CONTROL_SETUP_PACKET SetupPacket,
    __in_opt
    WDFMEMORY TransferMemory,
    __in_opt
    PWDFMEMORY_OFFSET TransferOffset
    )
/*++

Routine Description:
    Formats a request so that a control transfer can be sent to the device
    after this call returns successfully.

Arguments:
    UsbDevice - the target representing the device

    Request - Request to format

    SetupPacket - control setup packet to be used in the transfer

    TransferMemory - memory to use in the transfer after the setup packet

    TransferOffset - offset into TransferMemory and size override for transfer
                     length

Return Value:
    NTSTATUS

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    IFxMemory* pMemory;
    FxUsbDevice* pUsbDevice;
    FxRequestBuffer buf;
    FxRequest* pRequest;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBDEVICE %p, WDFREQUEST %p, WDFMEMORY %p",
                        UsbDevice, Request, TransferMemory);

    FxPointerNotNull(pFxDriverGlobals, SetupPacket);

    if (TransferMemory != NULL) {
        FxObjectHandleGetPtr(pFxDriverGlobals,
                             TransferMemory,
                             IFX_TYPE_MEMORY,
                             (PVOID*) &pMemory);

        status = pMemory->ValidateMemoryOffsets(TransferOffset);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        buf.SetMemory(pMemory, TransferOffset);
    }
    else {
        pMemory = NULL;
    }

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*) &pRequest);

    status = pUsbDevice->FormatControlRequest(pRequest, SetupPacket, &buf);

    if (NT_SUCCESS(status)) {
        FxUsbDeviceControlContext* pContext;

        pContext = (FxUsbDeviceControlContext*) pRequest->GetContext();

        RtlCopyMemory(
            &pContext->m_UsbParameters.Parameters.DeviceControlTransfer.SetupPacket,
            SetupPacket,
            sizeof(*SetupPacket)
            );

        if (pMemory != NULL) {
            pContext->m_UsbParameters.Parameters.DeviceControlTransfer.Buffer =
                TransferMemory;
        }
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "format control request WDFUSBDEVICE %p, WDFREQWUEST %p, WDFMEMORY %p, %!STATUS!",
                        UsbDevice, Request, TransferMemory, status);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceResetPortSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = pUsbDevice->Reset();

    return status;
}

#pragma warning(disable:28285)
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceCreateIsochUrb)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in
    ULONG NumberOfIsochPackets,
    __out
    WDFMEMORY* UrbMemory,
    __deref_opt_out_bcount(GET_ISOCH_URB_SIZE(NumberOfIsochPackets))
    PURB* Urb
    )
/*++

Routine Description:
    Creates a WDFUSBDEVICE handle for the client.

Arguments:
    Attributes - Attributes associated with this object

    NumberOfIsochPacket - Maximum number of Isoch packets that will be programmed into this Urb

    UrbMemory - The returned handle to the caller for the allocated Urb

    Urb - (opt) Pointer to the associated urb buffer.

Return Value:
    STATUS_INVALID_PARAMETER - any required parameters are not present/invalid

    STATUS_INVALID_DEVICE_STATE - If the client did not specify a client contract verion while
        creating the WDFUSBDEVICE

    STATUS_INSUFFICIENT_RESOURCES - could not allocated the object that backs
        the handle

    STATUS_SUCCESS - success

    ...

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    //
    // Basic parameter validation
    //
    FxPointerNotNull(pFxDriverGlobals, UrbMemory);

    if (pUsbDevice->GetUSBDHandle() == NULL) {
        status = STATUS_INVALID_DEVICE_STATE;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "USBDEVICE Must have been created with Client Contract Verion Info, %!STATUS!",
            status);

        return status;
    }

    status = pUsbDevice->CreateIsochUrb(Attributes,
                                        NumberOfIsochPackets,
                                        UrbMemory,
                                        Urb);

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFUSBINTERFACE
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceGetInterface)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    UCHAR InterfaceIndex
    )
/*++

Routine Description:


Arguments:

Return Value:


  --*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;
    FxUsbInterface *pUsbInterface;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    pUsbInterface = pUsbDevice->GetInterfaceFromIndex(InterfaceIndex);

    if (pUsbInterface != NULL) {
        return pUsbInterface->GetHandle();
    }
    else {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFUSBDEVICE %p has %d interfaces, index %d requested, returning "
            "NULL handle",
            UsbDevice, pUsbDevice->GetNumInterfaces(), InterfaceIndex);

        return NULL;
    }
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceQueryUsbCapability)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    CONST GUID* CapabilityType,
    __in
    ULONG CapabilityBufferLength,
    __drv_when(CapabilityBufferLength == 0, __out_opt)
    __drv_when(CapabilityBufferLength != 0 && ResultLength == NULL, __out_bcount(CapabilityBufferLength))
    __drv_when(CapabilityBufferLength != 0 && ResultLength != NULL, __out_bcount_part_opt(CapabilityBufferLength, *ResultLength))
    PVOID CapabilityBuffer,
    __out_opt
    __drv_when(ResultLength != NULL,__deref_out_range(<=,CapabilityBufferLength))
    PULONG ResultLength
    )
/*++

Routine Description:
    Queries USB capability for the device. Such capabilities are available
    only with USB 3.0 stack. On earlier stacks this API will fail with
    STATUS_NOT_IMPLEMENTED

Arguments:
    UsbDevice - Device whose capability is to be queried.

    CapabilityType - Type of capability as defined by
                     IOCTL_INTERNAL_USB_GET_USB_CAPABILITY

    CapabilityBufferLength - Length of Capability buffer

    CapabilityBuffer - Buffer for capability. Can be NULL if
                       CapabilitiyBufferLength is 0

    ResultLength - Actual length of the capability. This parameter is optional.

Return Value:
    STATUS_SUCCESS - success
    STATUS_NOT_IMPLEMENTED - Capabilties are not supported by USB stack
    STATUS_INSUFFICIENT_RESOURCES - no memory available
    ...

  --*/
{
    DDI_ENTRY();

    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    if (CapabilityBufferLength > 0) {
        FxPointerNotNull(pFxDriverGlobals, CapabilityBuffer);
    }

    status = pUsbDevice->QueryUsbCapability(CapabilityType,
                                            CapabilityBufferLength,
                                            CapabilityBuffer,
                                            ResultLength);
    return status;
}

} // extern "C"
