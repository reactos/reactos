/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCX controller object interface
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include "ucxusbdevice.h"

WDF_EXTERN_C_START

typedef enum _UCX_CONTROLLER_PARENT_BUS_TYPE
{
    UcxControllerParentBusTypeCustom = 0,
    UcxControllerParentBusTypePci = 1,
    UcxControllerParentBusTypeAcpi = 2,
    UcxControllerParentBusTypeMaUsb = 3
} UCX_CONTROLLER_PARENT_BUS_TYPE;

typedef enum _UCX_CONTROLLER_STATE
{
    UcxControllerStateLost = 0,
    UcxControllerStatePreserved = 1
} UCX_CONTROLLER_STATE;

/* A controller answering yes wants Clear TT Buffer sent when an async
 * transfer to a low or full speed device behind a TT is canceled */
DEFINE_GUID(GUID_USB_CAPABILITY_CLEAR_TT_BUFFER_ON_ASYNC_TRANSFER_CANCEL,
            0x09b76d52, 0x0a6d, 0x4e4f, 0xa9, 0x11, 0xd0, 0x36, 0xd1, 0x92, 0x94, 0x97);

#define MAX_VENDOR_ID_STRING_LENGTH 5
#define MAX_DEVICE_ID_STRING_LENGTH 5
#define MAX_REVISION_ID_STRING_LENGTH 5
#define MAX_GENERIC_USB_CONTROLLER_NAME_SIZE 40

typedef struct _UCX_CONTROLLER_PCI_INFORMATION
{
    ULONG VendorId;
    ULONG DeviceId;
    USHORT RevisionId;
    ULONG BusNumber;
    ULONG DeviceNumber;
    ULONG FunctionNumber;
} UCX_CONTROLLER_PCI_INFORMATION, *PUCX_CONTROLLER_PCI_INFORMATION;

typedef struct _UCX_CONTROLLER_ACPI_INFORMATION
{
    CHAR VendorId[MAX_VENDOR_ID_STRING_LENGTH];
    CHAR DeviceId[MAX_DEVICE_ID_STRING_LENGTH];
    CHAR RevisionId[MAX_REVISION_ID_STRING_LENGTH];
} UCX_CONTROLLER_ACPI_INFORMATION, *PUCX_CONTROLLER_ACPI_INFORMATION;

typedef union _UCX_CONTROLLER_TRANSPORT_CHARACTERISTICS_CHANGE_FLAGS
{
    ULONG AsUlong32;
    struct
    {
        ULONG CurrentRoundtripLatencyChanged:1;
        ULONG CurrentTotalBandwidthChanged:1;
    } UCX_ANONYMOUS_FLAGS;
} UCX_CONTROLLER_TRANSPORT_CHARACTERISTICS_CHANGE_FLAGS;

typedef struct _UCX_CONTROLLER_TRANSPORT_CHARACTERISTICS
{
    ULONG TransportCharacteristicsFlags;
    ULONG64 CurrentRoundtripLatencyInMilliSeconds;
    ULONG64 MaxPotentialBandwidth;
} UCX_CONTROLLER_TRANSPORT_CHARACTERISTICS, *PUCX_CONTROLLER_TRANSPORT_CHARACTERISTICS;

typedef
_Function_class_(EVT_UCX_CONTROLLER_QUERY_USB_CAPABILITY)
_IRQL_requires_same_
_Must_inspect_result_
NTSTATUS
NTAPI
EVT_UCX_CONTROLLER_QUERY_USB_CAPABILITY(
    _In_ UCXCONTROLLER UcxController,
    _In_ PGUID CapabilityType,
    _In_ ULONG OutputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _Out_ PULONG ResultLength);
typedef EVT_UCX_CONTROLLER_QUERY_USB_CAPABILITY *PFN_UCX_CONTROLLER_QUERY_USB_CAPABILITY;

typedef
_Function_class_(EVT_UCX_CONTROLLER_GET_CURRENT_FRAMENUMBER)
_IRQL_requires_same_
_Must_inspect_result_
NTSTATUS
NTAPI
EVT_UCX_CONTROLLER_GET_CURRENT_FRAMENUMBER(
    _In_ UCXCONTROLLER UcxController,
    _Out_ PULONG FrameNumber);
typedef EVT_UCX_CONTROLLER_GET_CURRENT_FRAMENUMBER *PFN_UCX_CONTROLLER_GET_CURRENT_FRAMENUMBER;

typedef
_Function_class_(EVT_UCX_CONTROLLER_USBDEVICE_ADD)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
EVT_UCX_CONTROLLER_USBDEVICE_ADD(
    _In_ UCXCONTROLLER UcxController,
    _In_ PUCXUSBDEVICE_INFO UcxUsbDeviceInfo,
    _In_ PUCXUSBDEVICE_INIT UsbDeviceInit);
typedef EVT_UCX_CONTROLLER_USBDEVICE_ADD *PFN_UCX_CONTROLLER_USBDEVICE_ADD;

typedef
_Function_class_(EVT_UCX_CONTROLLER_RESET)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
EVT_UCX_CONTROLLER_RESET(
    _In_ UCXCONTROLLER UcxController);
typedef EVT_UCX_CONTROLLER_RESET *PFN_UCX_CONTROLLER_RESET;

typedef
_Function_class_(EVT_UCX_CONTROLLER_GET_TRANSPORT_CHARACTERISTICS)
_IRQL_requires_same_
_Must_inspect_result_
NTSTATUS
NTAPI
EVT_UCX_CONTROLLER_GET_TRANSPORT_CHARACTERISTICS(
    _In_ UCXCONTROLLER UcxController,
    _Out_ PUCX_CONTROLLER_TRANSPORT_CHARACTERISTICS UcxControllerTransportCharacteristics);
typedef EVT_UCX_CONTROLLER_GET_TRANSPORT_CHARACTERISTICS *PFN_UCX_CONTROLLER_GET_TRANSPORT_CHARACTERISTICS;

typedef
_Function_class_(EVT_UCX_CONTROLLER_SET_TRANSPORT_CHARACTERISTICS_CHANGE_NOTIFICATION)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_CONTROLLER_SET_TRANSPORT_CHARACTERISTICS_CHANGE_NOTIFICATION(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCX_CONTROLLER_TRANSPORT_CHARACTERISTICS_CHANGE_FLAGS ChangeNotificationFlags);
typedef EVT_UCX_CONTROLLER_SET_TRANSPORT_CHARACTERISTICS_CHANGE_NOTIFICATION
    *PFN_UCX_CONTROLLER_SET_TRANSPORT_CHARACTERISTICS_CHANGE_NOTIFICATION;

typedef
_Function_class_(EVT_UCX_CONTROLLER_START_TRACKING_FOR_TIME_SYNC)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NTAPI
EVT_UCX_CONTROLLER_START_TRACKING_FOR_TIME_SYNC(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST WdfRequest,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength);
typedef EVT_UCX_CONTROLLER_START_TRACKING_FOR_TIME_SYNC *PFN_UCX_CONTROLLER_START_TRACKING_FOR_TIME_SYNC;

typedef
_Function_class_(EVT_UCX_CONTROLLER_STOP_TRACKING_FOR_TIME_SYNC)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NTAPI
EVT_UCX_CONTROLLER_STOP_TRACKING_FOR_TIME_SYNC(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST WdfRequest,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength);
typedef EVT_UCX_CONTROLLER_STOP_TRACKING_FOR_TIME_SYNC *PFN_UCX_CONTROLLER_STOP_TRACKING_FOR_TIME_SYNC;

typedef
_Function_class_(EVT_UCX_CONTROLLER_GET_FRAME_NUMBER_AND_QPC_FOR_TIME_SYNC)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_CONTROLLER_GET_FRAME_NUMBER_AND_QPC_FOR_TIME_SYNC(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST WdfRequest,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength);
typedef EVT_UCX_CONTROLLER_GET_FRAME_NUMBER_AND_QPC_FOR_TIME_SYNC
    *PFN_UCX_CONTROLLER_GET_FRAME_NUMBER_AND_QPC_FOR_TIME_SYNC;

typedef struct _UCX_CONTROLLER_CONFIG
{
    ULONG Size;
    ULONG NumberOfPresentedDeviceMgmtEvtCallbacks;
    PFN_UCX_CONTROLLER_QUERY_USB_CAPABILITY EvtControllerQueryUsbCapability;
    HANDLE Reserved1;
    PFN_UCX_CONTROLLER_GET_CURRENT_FRAMENUMBER EvtControllerGetCurrentFrameNumber;
    PFN_UCX_CONTROLLER_USBDEVICE_ADD EvtControllerUsbDeviceAdd;
    PFN_UCX_CONTROLLER_RESET EvtControllerReset;
    HANDLE Reserved2;
    HANDLE Reserved3;
    HANDLE Reserved4;
    UCX_CONTROLLER_PARENT_BUS_TYPE ParentBusType;
    UCX_CONTROLLER_PCI_INFORMATION PciDeviceInfo;
    UCX_CONTROLLER_ACPI_INFORMATION AcpiDeviceInfo;

    /* Reported as is through GUID_USB_WMI_NODE_INFO */
    UCHAR DeviceDescription[MAX_GENERIC_USB_CONTROLLER_NAME_SIZE];

    UNICODE_STRING ManufacturerNameString;
    UNICODE_STRING ModelNameString;
    UNICODE_STRING ModelNumberString;
    PFN_UCX_CONTROLLER_GET_TRANSPORT_CHARACTERISTICS EvtControllerGetTransportCharacteristics;
    PFN_UCX_CONTROLLER_SET_TRANSPORT_CHARACTERISTICS_CHANGE_NOTIFICATION
        EvtControllerSetTransportCharacteristicsChangeNotification;
    HANDLE Reserved5;
    HANDLE Reserved6;
    HANDLE Reserved7;
} UCX_CONTROLLER_CONFIG, *PUCX_CONTROLLER_CONFIG;

FORCEINLINE
VOID
NTAPI
UcxpCopyAnsiString(
    _Out_writes_z_(DestinationSize) PCHAR Destination,
    _In_ SIZE_T DestinationSize,
    _In_z_ PCSTR Source)
{
    SIZE_T Index = 0;

    if (DestinationSize == 0)
        return;

    while (Index + 1 < DestinationSize && Source[Index] != ANSI_NULL)
    {
        Destination[Index] = Source[Index];
        Index++;
    }

    Destination[Index] = ANSI_NULL;
}

/** Prepares a config with no parent bus information; MAXLONG marks the PCI IDs unset. */
FORCEINLINE
VOID
NTAPI
UCX_CONTROLLER_CONFIG_INIT(
    _Out_ PUCX_CONTROLLER_CONFIG Config,
    _In_ LPCSTR DeviceDescription)
{
    RtlZeroMemory(Config, sizeof(*Config));
    Config->Size = sizeof(*Config);

    Config->NumberOfPresentedDeviceMgmtEvtCallbacks = MAXULONG;

    Config->ParentBusType = UcxControllerParentBusTypeCustom;
    Config->PciDeviceInfo.VendorId = MAXLONG;
    Config->PciDeviceInfo.DeviceId = MAXLONG;

    UcxpCopyAnsiString((PCHAR)Config->DeviceDescription,
                       sizeof(Config->DeviceDescription),
                       DeviceDescription);
}

FORCEINLINE
VOID
NTAPI
UCX_CONTROLLER_CONFIG_SET_PCI_INFO(
    _Inout_ PUCX_CONTROLLER_CONFIG Config,
    _In_ ULONG VendorId,
    _In_ ULONG DeviceId,
    _In_ USHORT RevisionId,
    _In_ ULONG BusNumber,
    _In_ ULONG DeviceNumber,
    _In_ ULONG FunctionNumber)
{
    PUCX_CONTROLLER_PCI_INFORMATION Pci = &Config->PciDeviceInfo;

    Pci->VendorId = VendorId;
    Pci->DeviceId = DeviceId;
    Pci->RevisionId = RevisionId;
    Pci->BusNumber = BusNumber;
    Pci->DeviceNumber = DeviceNumber;
    Pci->FunctionNumber = FunctionNumber;

    Config->ParentBusType = UcxControllerParentBusTypePci;
}

FORCEINLINE
VOID
NTAPI
UCX_CONTROLLER_CONFIG_SET_ACPI_INFO(
    _Inout_ PUCX_CONTROLLER_CONFIG Config,
    _In_reads_(MAX_VENDOR_ID_STRING_LENGTH) PSTR VendorId,
    _In_reads_(MAX_DEVICE_ID_STRING_LENGTH) PSTR DeviceId,
    _In_reads_(MAX_REVISION_ID_STRING_LENGTH) PSTR RevisionId)
{
    PUCX_CONTROLLER_ACPI_INFORMATION Acpi = &Config->AcpiDeviceInfo;

    UcxpCopyAnsiString(Acpi->VendorId, sizeof(Acpi->VendorId), VendorId);
    UcxpCopyAnsiString(Acpi->DeviceId, sizeof(Acpi->DeviceId), DeviceId);
    UcxpCopyAnsiString(Acpi->RevisionId, sizeof(Acpi->RevisionId), RevisionId);

    Config->ParentBusType = UcxControllerParentBusTypeAcpi;
}

typedef struct _UCX_CONTROLLER_RESET_COMPLETE_INFO
{
    ULONG Size;
    UCX_CONTROLLER_STATE UcxControllerState;
    BOOLEAN UcxCoordinated;
} UCX_CONTROLLER_RESET_COMPLETE_INFO, *PUCX_CONTROLLER_RESET_COMPLETE_INFO;

FORCEINLINE
VOID
NTAPI
UCX_CONTROLLER_RESET_COMPLETE_INFO_INIT(
    _Out_ PUCX_CONTROLLER_RESET_COMPLETE_INFO UcxControllerResetCompleteInfo,
    _In_ UCX_CONTROLLER_STATE UcxControllerState,
    _In_ BOOLEAN UcxCoordinated)
{
    RtlZeroMemory(UcxControllerResetCompleteInfo, sizeof(*UcxControllerResetCompleteInfo));
    UcxControllerResetCompleteInfo->Size = sizeof(*UcxControllerResetCompleteInfo);
    UcxControllerResetCompleteInfo->UcxControllerState = UcxControllerState;
    UcxControllerResetCompleteInfo->UcxCoordinated = UcxCoordinated;
}

/* PEVT_ spellings from older headers */
typedef PFN_UCX_CONTROLLER_QUERY_USB_CAPABILITY PEVT_UCX_CONTROLLER_QUERY_USB_CAPABILITY;
typedef PFN_UCX_CONTROLLER_GET_CURRENT_FRAMENUMBER PEVT_UCX_CONTROLLER_GET_CURRENT_FRAMENUMBER;
typedef PFN_UCX_CONTROLLER_USBDEVICE_ADD PEVT_UCX_CONTROLLER_USBDEVICE_ADD;
typedef PFN_UCX_CONTROLLER_RESET PEVT_UCX_CONTROLLER_RESET;

/* Class entry points */

typedef
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
(NTAPI *PFN_UCXIODEVICECONTROL)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode);

typedef
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
(NTAPI *PFN_UCXCONTROLLERCREATE)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ WDFDEVICE Device,
    _In_ PUCX_CONTROLLER_CONFIG Config,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXCONTROLLER *Controller);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXCONTROLLERNEEDSRESET)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXCONTROLLERRESETCOMPLETE)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUCX_CONTROLLER_RESET_COMPLETE_INFO UcxControllerResetCompleteInfo);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXCONTROLLERSETFAILED)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
(NTAPI *PFN_UCXCONTROLLERSETIDSTRINGS)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUNICODE_STRING ManufacturerNameString,
    _In_ PUNICODE_STRING ModelNameString,
    _In_ PUNICODE_STRING ModelNumberString);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXCONTROLLERNOTIFYTRANSPORTCHARACTERISTICSCHANGE)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUCX_CONTROLLER_TRANSPORT_CHARACTERISTICS UcxControllerTransportCharacteristics);

/** Offers an IOCTL to UCX, returning TRUE once UCX owns the request. */
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
BOOLEAN
NTAPI
UcxIoDeviceControl(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    PFN_UCXIODEVICECONTROL DeviceControl;

    DeviceControl = UCX_BOUND_FUNCTION(PFN_UCXIODEVICECONTROL, UcxIoDeviceControlTableIndex);
    return DeviceControl(UcxDriverGlobals,
                         Device,
                         Request,
                         OutputBufferLength,
                         InputBufferLength,
                         IoControlCode);
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
NTSTATUS
NTAPI
UcxControllerCreate(
    _In_ WDFDEVICE Device,
    _In_ PUCX_CONTROLLER_CONFIG Config,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXCONTROLLER *Controller)
{
    PFN_UCXCONTROLLERCREATE Create;

    Create = UCX_BOUND_FUNCTION(PFN_UCXCONTROLLERCREATE, UcxControllerCreateTableIndex);
    return Create(UcxDriverGlobals, Device, Config, Attributes, Controller);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
VOID
NTAPI
UcxControllerNeedsReset(
    _In_ UCXCONTROLLER Controller)
{
    PFN_UCXCONTROLLERNEEDSRESET NeedsReset;

    NeedsReset = UCX_BOUND_FUNCTION(PFN_UCXCONTROLLERNEEDSRESET,
                                    UcxControllerNeedsResetTableIndex);
    NeedsReset(UcxDriverGlobals, Controller);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
VOID
NTAPI
UcxControllerResetComplete(
    _In_ UCXCONTROLLER Controller,
    _In_ PUCX_CONTROLLER_RESET_COMPLETE_INFO UcxControllerResetCompleteInfo)
{
    PFN_UCXCONTROLLERRESETCOMPLETE ResetComplete;

    ResetComplete = UCX_BOUND_FUNCTION(PFN_UCXCONTROLLERRESETCOMPLETE,
                                       UcxControllerResetCompleteTableIndex);
    ResetComplete(UcxDriverGlobals, Controller, UcxControllerResetCompleteInfo);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
VOID
NTAPI
UcxControllerSetFailed(
    _In_ UCXCONTROLLER Controller)
{
    PFN_UCXCONTROLLERSETFAILED SetFailed;

    SetFailed = UCX_BOUND_FUNCTION(PFN_UCXCONTROLLERSETFAILED, UcxControllerSetFailedTableIndex);
    SetFailed(UcxDriverGlobals, Controller);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
NTSTATUS
NTAPI
UcxControllerSetIdStrings(
    _In_ UCXCONTROLLER Controller,
    _In_ PUNICODE_STRING ManufacturerNameString,
    _In_ PUNICODE_STRING ModelNameString,
    _In_ PUNICODE_STRING ModelNumberString)
{
    PFN_UCXCONTROLLERSETIDSTRINGS SetIdStrings;

    SetIdStrings = UCX_BOUND_FUNCTION(PFN_UCXCONTROLLERSETIDSTRINGS,
                                      UcxControllerSetIdStringsTableIndex);
    return SetIdStrings(UcxDriverGlobals,
                        Controller,
                        ManufacturerNameString,
                        ModelNameString,
                        ModelNumberString);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
VOID
NTAPI
UcxControllerNotifyTransportCharacteristicsChange(
    _In_ UCXCONTROLLER Controller,
    _In_ PUCX_CONTROLLER_TRANSPORT_CHARACTERISTICS UcxControllerTransportCharacteristics)
{
    PFN_UCXCONTROLLERNOTIFYTRANSPORTCHARACTERISTICSCHANGE Notify;

    Notify = UCX_BOUND_FUNCTION(PFN_UCXCONTROLLERNOTIFYTRANSPORTCHARACTERISTICSCHANGE,
                                UcxControllerNotifyTransportCharacteristicsChangeTableIndex);
    Notify(UcxDriverGlobals, Controller, UcxControllerTransportCharacteristics);
}

WDF_EXTERN_C_END
