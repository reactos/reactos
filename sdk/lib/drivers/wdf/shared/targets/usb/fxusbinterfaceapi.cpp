/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxUsbInterfaceAPI.cpp

Abstract:


Author:

Environment:

    Both kernel and user mode

Revision History:

--*/
#include "fxusbpch.hpp"

extern "C" {
#include "FxUsbInterfaceAPI.tmh"
}

//
// Extern "C" all APIs
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbInterfaceSelectSetting)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES PipesAttributes,
    __in
    PWDF_USB_INTERFACE_SELECT_SETTING_PARAMS Params
    )
/*++

Routine Description:
    Selects an alternate setting on a given interface number

Arguments:
    UsbInterface - the interface whose setting will be selected

    PipesAttributes - Attributes to apply to each of the new pipes on the setting

    Params - Strucutre indicating how to select a new setting

Return Value:
    NTSTATUS

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbInterface* pUsbInterface;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbInterface,
                                   FX_TYPE_USB_INTERFACE,
                                   (PVOID*) &pUsbInterface,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Params);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Params->Size != sizeof(WDF_USB_INTERFACE_SELECT_SETTING_PARAMS)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Params size %d, expected %d %!STATUS!", Params->Size,
            sizeof(WDF_USB_INTERFACE_SELECT_SETTING_PARAMS), status);
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals,
                                        PipesAttributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED);
    if (!NT_SUCCESS(status)) {
        return status;
    }

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    if (Params->Type != WdfUsbInterfaceSelectSettingTypeSetting) {
        FX_VERIFY_WITH_NAME(INTERNAL, TRAPMSG("UMDF may only select settings by index."),
            DriverGlobals->DriverName);
    }
#endif

    switch (Params->Type) {
    case WdfUsbInterfaceSelectSettingTypeDescriptor:
        if (Params->Types.Descriptor.InterfaceDescriptor == NULL) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "InterfaceDescriptor passed in is NULL %!STATUS!", status);

        }
        else {
            status = pUsbInterface->SelectSettingByDescriptor(
                PipesAttributes,
                Params->Types.Descriptor.InterfaceDescriptor
                );
        }
        break;

    case WdfUsbInterfaceSelectSettingTypeSetting:
        status = pUsbInterface->SelectSettingByIndex(
            PipesAttributes,
            Params->Types.Interface.SettingIndex
            );
        break;

    case WdfUsbInterfaceSelectSettingTypeUrb:
        if (Params->Types.Urb.Urb == NULL ||
            Params->Types.Urb.Urb->UrbHeader.Function != URB_FUNCTION_SELECT_INTERFACE ||
            (Params->Types.Urb.Urb->UrbHeader.Length  <
            sizeof(struct _URB_SELECT_INTERFACE) - sizeof(USBD_PIPE_INFORMATION)) ) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "URB or URB fields passed in are invalid Urb 0x%p  %!STATUS!",
                Params->Types.Urb.Urb, status);

        }
        else {
            status = pUsbInterface->SelectSetting(PipesAttributes,
                                                 Params->Types.Urb.Urb);
        }
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    return status;
}


__drv_maxIRQL(DISPATCH_LEVEL)
BYTE
WDFAPI
WDFEXPORT(WdfUsbInterfaceGetInterfaceNumber)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface
    )
/*++

Routine Description:
    Returns the interface number of the given interface.  The interface number
    is not necessarily the same as the index of the interface.  Once is specified
    by the device, the other is where it is located within an internal array.

Arguments:
    UsbInterface - Interace whose number will be returned

Return Value:
    interface number

  --*/
{
    DDI_ENTRY();

    FxUsbInterface * pUsbInterface;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         UsbInterface,
                         FX_TYPE_USB_INTERFACE,
                         (PVOID*) &pUsbInterface);

    return pUsbInterface->GetInterfaceNumber();
}

__drv_maxIRQL(DISPATCH_LEVEL)
BYTE
WDFAPI
WDFEXPORT(WdfUsbInterfaceGetNumEndpoints)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface,
    __in
    UCHAR SettingIndex
    )
/*++

Routine Description:
    Returns the number of endpoints (not configured) for a given setting on the
    given interface.

Arguments:
    UsbInterface - interface whose number of endpoints will be returned

    SettingsIndex - the index into the alternate setting array

Return Value:
    Number of endpoints

  --*/
{
    DDI_ENTRY();

    FxUsbInterface * pUsbInterface;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         UsbInterface,
                         FX_TYPE_USB_INTERFACE  ,
                         (PVOID*) &pUsbInterface);

    return pUsbInterface->GetNumEndpoints(SettingIndex);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfUsbInterfaceGetEndpointInformation)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface,
    __in
    UCHAR SettingIndex,
    __in
    UCHAR EndpointIndex,
    __out
    PWDF_USB_PIPE_INFORMATION EndpointInfo
    )
/*++

Routine Description:
    Returns information on a given endpoint (unconfigured) for an interface +
    alternate setting index.

Arguments:
    UsbInterface - interface which contains  the endpoint

    SettingIndex - alternate setting index

    EndpointIndex - index into the endpoint array contained by interface+setting

    EndpointInfo - structure to be filled in with the info of the endpoint

Return Value:
    None

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbInterface * pUsbInterface;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbInterface,
                                   FX_TYPE_USB_INTERFACE,
                                   (PVOID*) &pUsbInterface,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, EndpointInfo);

    if (EndpointInfo->Size != sizeof(WDF_USB_PIPE_INFORMATION)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "EndpointInfo Size %d incorrect, expected %d, %!STATUS!",
            EndpointInfo->Size, sizeof(WDF_USB_PIPE_INFORMATION),
            STATUS_INFO_LENGTH_MISMATCH);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    pUsbInterface->GetEndpointInformation(SettingIndex,
                                          EndpointIndex,
                                          EndpointInfo);
}

__drv_maxIRQL(DISPATCH_LEVEL)
BYTE
WDFEXPORT(WdfUsbInterfaceGetNumSettings)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface
    )
/*++

Routine Description:
    Returns the number of settings available on an interface.

Arguments:
    UsbInterface - the usb interface being queried

Return Value:
    Number of settings  as described by the config descriptor

  --*/
{
    DDI_ENTRY();

    FxUsbInterface * pUsbInterface;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         UsbInterface,
                         FX_TYPE_USB_INTERFACE,
                         (PVOID*) &pUsbInterface);

    return pUsbInterface->GetNumSettings();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfUsbInterfaceGetDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface,
    __in
    UCHAR SettingIndex,
    __out
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor
    )
/*++

Routine Description:
    Returns the underlying USB interface descriptor for the given WDF handle

Arguments:
    UsbInterface - interface whose descriptor will be returned

    SettingIndex - alternatve setting whose interface should be returned

    InterfaceDescriptor - Pointer which will receive the descriptor

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbInterface * pUsbInterface;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbInterface,
                                   FX_TYPE_USB_INTERFACE,
                                   (PVOID*) &pUsbInterface,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, InterfaceDescriptor);

    pUsbInterface->GetDescriptor(InterfaceDescriptor, SettingIndex);
}

__drv_maxIRQL(DISPATCH_LEVEL)
BYTE
WDFAPI
WDFEXPORT(WdfUsbInterfaceGetConfiguredSettingIndex)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface
    )
/*++

Routine Description:
    Returns the alternate setting index which is currently configured

Arguments:
    UsbInterface - interfase whose configured setting is being queried

Return Value:
    The current setting or 0 on failure

  --*/
{
    DDI_ENTRY();

    FxUsbInterface * pUsbInterface;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         UsbInterface,
                         FX_TYPE_USB_INTERFACE  ,
                         (PVOID*) &pUsbInterface);

    return pUsbInterface->GetConfiguredSettingIndex();
}

__drv_maxIRQL(DISPATCH_LEVEL)
BYTE
WDFAPI
WDFEXPORT(WdfUsbInterfaceGetNumConfiguredPipes)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE  UsbInterface
    )
/*++

Routine Description:
    Returns the number of configured pipes on a given interface.

Arguments:
    UsbInterface - the configured interface

Return Value:
    Number of configured pipes or 0 on error

  --*/
{
    DDI_ENTRY();

    FxUsbInterface * pUsbInterface;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         UsbInterface,
                         FX_TYPE_USB_INTERFACE  ,
                         (PVOID*) &pUsbInterface);

    return pUsbInterface->GetNumConfiguredPipes();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFUSBPIPE
WDFAPI
WDFEXPORT(WdfUsbInterfaceGetConfiguredPipe)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBINTERFACE UsbInterface,
    __in
    UCHAR PipeIndex,
    __out_opt
    PWDF_USB_PIPE_INFORMATION PipeInfo
    )
/*++

Routine Description:
    Returns the WDFUSBPIPE handle for the configured interface + pipe index.
    Optionally, information about the pipe is also returned.

Arguments:
    UsbInterface  - configured interface

    PipeIndex - index into the number of pipes on the interface

    PipeInfo - information to be returned on the pipe

Return Value:
    valid WDFUSBPIPE or NULL on error

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbInterface * pUsbInterface;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbInterface,
                                   FX_TYPE_USB_INTERFACE,
                                   (PVOID*) &pUsbInterface,
                                   &pFxDriverGlobals);

    if (PipeInfo != NULL && PipeInfo->Size != sizeof(WDF_USB_PIPE_INFORMATION)) {
        status = STATUS_INFO_LENGTH_MISMATCH;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "PipeInfo Size %d incorrect, expected %d, %!STATUS!",
            PipeInfo->Size, sizeof(WDF_USB_PIPE_INFORMATION), status);

        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return NULL;
    }

    return pUsbInterface->GetConfiguredPipe(PipeIndex, PipeInfo);
}

} // extern "C"
