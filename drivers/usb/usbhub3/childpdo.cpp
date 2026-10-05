/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Child PDO: creation, PnP and power, query interfaces and the client IOCTL front end
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"
#include "devhcd.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* Reference tag on the device object held by its PDO; only the address matters */
static const char HubTagPdo[] = "hub child pdo";

/* Time the device gets to find a driver before the device machine suspends it */
#define HUB_PDO_DRIVER_WAIT_TIME        10000

/* Last valid URB function and the undocumented ones below it */
#define HUB_URB_FUNCTION_LAST           0x003C
#define HUB_URB_FUNCTION_FAST_PATH_1    0x0039
#define HUB_URB_FUNCTION_FAST_PATH_2    0x003A
#define HUB_URB_FUNCTION_SELECT_LIKE    0x003B

/* MS OS 2.0 descriptor set index of GET_MS_FEATURE_DESCRIPTOR */
#define HUB_MSOS20_SET_INDEX            7

/* Device IOCTLs passed to UCX with the device handle stamped in */
#define HUB_USB_BUFFERED_IOCTL(Function) \
    CTL_CODE(FILE_DEVICE_USB, Function, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HUB_IOCTL_GET_TRANSPORT_CHARACTERISTICS     HUB_USB_BUFFERED_IOCTL(USB_GET_TRANSPORT_CHARACTERISTICS)
#define HUB_IOCTL_REGISTER_TRANSPORT_CHANGE         HUB_USB_BUFFERED_IOCTL(USB_REGISTER_FOR_TRANSPORT_CHARACTERISTICS_CHANGE)
#define HUB_IOCTL_NOTIFY_TRANSPORT_CHANGE           HUB_USB_BUFFERED_IOCTL(USB_NOTIFY_ON_TRANSPORT_CHARACTERISTICS_CHANGE)
#define HUB_IOCTL_UNREGISTER_TRANSPORT_CHANGE       HUB_USB_BUFFERED_IOCTL(USB_UNREGISTER_FOR_TRANSPORT_CHARACTERISTICS_CHANGE)
#define HUB_IOCTL_START_TIME_SYNC                   HUB_USB_BUFFERED_IOCTL(USB_START_TRACKING_FOR_TIME_SYNC)
#define HUB_IOCTL_GET_TIME_SYNC                     HUB_USB_BUFFERED_IOCTL(USB_GET_FRAME_NUMBER_AND_QPC_FOR_TIME_SYNC)
#define HUB_IOCTL_STOP_TIME_SYNC                    HUB_USB_BUFFERED_IOCTL(USB_STOP_TRACKING_FOR_TIME_SYNC)
#define HUB_IOCTL_GET_DEVICE_CHARACTERISTICS        HUB_USB_BUFFERED_IOCTL(USB_GET_DEVICE_CHARACTERISTICS)

/* Payload sizes of the two stamped IOCTLs; both are pack 1 with the handle at offset 4 */
#define HUB_TRANSPORT_REGISTRATION_SIZE (sizeof(ULONG) + sizeof(PVOID) + 24)
#define HUB_DEVICE_CHARACTERISTICS_SIZE 0x18
#define HUB_STAMPED_HANDLE_OFFSET       4

/* User mode IOCTLs that reach the PDO queue */
#define HUB_IOCTL_MEDIA_SERIAL_NUMBER   CTL_CODE(FILE_DEVICE_MASS_STORAGE, 0x0304, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* RECORD_FAILURE copies at most this much client data */
#define HUB_START_FAIL_MAX              4096

/* Header of USB_START_FAILDATA, which usbioctl.h hides behind USB20_API */
struct HubStartFailHeader
{
    ULONG LengthInBytes;
    NTSTATUS NtStatus;
    USBD_STATUS UsbdStatus;
    ULONG ConnectStatus;
    UCHAR DriverData[4];
};

/* Billboard property payload: a 4 byte header, then one entry per alternate mode */
struct HubBillboardHeader
{
    UCHAR NumberOfAlternateModes;
    UCHAR PreferredModeEntry;
    UCHAR FailureInfo;
    UCHAR Reserved;
};

struct HubBillboardMode
{
    USHORT SVid;
    UCHAR Mode;
    UCHAR Reserved;
    ULONG Status;
};

C_ASSERT(sizeof(HubBillboardHeader) == 4);
C_ASSERT(sizeof(HubBillboardMode) == 8);

/* Context of the work item that completes a device power IRP */
struct HubPdoIrpWork
{
    PIRP Irp;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HubPdoIrpWork, HubGetPdoIrpWork);

/* {4D36E97E-E325-11CE-BFC1-08002BE10318} */
static const GUID HubClassUnknown =
    { 0x4d36e97e, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };

/* Device connection speed capabilities without a public name */
static const GUID HubCapFullSpeedCompatible =
    { 0xcde43191, 0x1fad, 0x4120, { 0xa7, 0xab, 0x74, 0x09, 0x56, 0xc6, 0x6f, 0x37 } };
static const GUID HubCapLowSpeedCompatible =
    { 0xad6f2a13, 0xc4d1, 0x4925, { 0x83, 0xcc, 0x96, 0x85, 0x7a, 0x24, 0x31, 0xd9 } };

/* Device properties the PDO publishes */
static const DEVPROPKEY HubKeyBillboardInfo =
    { { 0xb1049018, 0x6da0, 0x4490, { 0xb0, 0xc9, 0x1e, 0x91, 0xf1, 0xf5, 0x5b, 0xd7 } }, 2 };
static const DEVPROPKEY HubKeyDualRoleFeatures =
    { { 0x32d615be, 0x32f4, 0x4fdc, { 0xbc, 0x48, 0xfc, 0x54, 0x30, 0x1a, 0xb6, 0xc4 } }, 2 };
static const DEVPROPKEY HubKeyUsb4TunnelState =
    { { 0xb5a825b8, 0x3907, 0x40b5, { 0xab, 0xca, 0x84, 0x51, 0xe2, 0x45, 0x3d, 0x5f } }, 3 };

/* LPM power settings; slots 0 to 3 differ for hubs, slot 4 is shared */
static const GUID HubLpmDeviceSettings[HUB_PDO_LPM_SETTINGS] =
{
    { 0xfb093825, 0xbf59, 0x4fe6, { 0x8b, 0x6f, 0xaf, 0xd6, 0x71, 0x4c, 0x69, 0xe5 } },
    { 0x903281ea, 0xc859, 0x47a9, { 0x89, 0x01, 0x7a, 0x11, 0x8c, 0x7b, 0xf5, 0xa7 } },
    { 0xa7b5d35e, 0xc12c, 0x4f36, { 0xa4, 0x4f, 0x00, 0x62, 0x8f, 0x90, 0x03, 0x88 } },
    { 0x300c0440, 0xee77, 0x4c1b, { 0xaf, 0x99, 0x3a, 0x33, 0xf8, 0xa9, 0xca, 0x5d } },
    { 0xd4e98f31, 0x5ffe, 0x4ce1, { 0xbe, 0x31, 0x1b, 0x38, 0xb3, 0x84, 0xc0, 0x09 } }
};

static const GUID HubLpmHubSettings[HUB_PDO_LPM_SETTINGS] =
{
    { 0xc5f91e9d, 0x0ced, 0x43f2, { 0x90, 0x5e, 0x8f, 0x9d, 0x38, 0xe9, 0x33, 0x39 } },
    { 0x34751a37, 0xe607, 0x452a, { 0xb9, 0xf7, 0xd0, 0x20, 0xfc, 0x5c, 0xd0, 0xdc } },
    { 0x329423dd, 0x222f, 0x4b04, { 0x81, 0x5b, 0x08, 0xc1, 0xa6, 0xd8, 0x9c, 0x8e } },
    { 0xbbfbadaf, 0x7520, 0x491a, { 0x91, 0x0d, 0x4a, 0x2c, 0xee, 0x72, 0x20, 0x3f } },
    { 0xd4e98f31, 0x5ffe, 0x4ce1, { 0xbe, 0x31, 0x1b, 0x38, 0xb3, 0x84, 0xc0, 0x09 } }
};

enum HubLpmSlot
{
    HubLpmU1Enable,
    HubLpmU2Enable,
    HubLpmU1Timeout,
    HubLpmU2Timeout,
    HubLpmPolicy
};

#define HUB_LPM_POLICY_LINK_BITS (HUB_LPM_POLICY_U1_ENABLED | HUB_LPM_POLICY_U2_ENABLED | \
                                  HUB_LPM_POLICY_U1_ACCEPT | HUB_LPM_POLICY_U2_ACCEPT | \
                                  HUB_LPM_POLICY_U1_INITIATE | HUB_LPM_POLICY_U2_INITIATE)

/* PoSetSystemWakeDevice is not exported everywhere; resolved at the first PDO creation */
typedef
VOID
(NTAPI *PFN_HUB_SET_SYSTEM_WAKE_DEVICE)(
    _In_ PDEVICE_OBJECT DeviceObject);

static PFN_HUB_SET_SYSTEM_WAKE_DEVICE HubSetSystemWakeDevice;
static BOOLEAN HubSetSystemWakeDeviceResolved;

static EVT_WDF_DEVICE_D0_ENTRY HubPdoEvtD0Entry;
static EVT_WDF_DEVICE_D0_EXIT HubPdoEvtD0Exit;
static EVT_WDF_DEVICE_PREPARE_HARDWARE HubPdoEvtPrepareHardware;
static EVT_WDF_DEVICE_RELEASE_HARDWARE HubPdoEvtReleaseHardware;
static EVT_WDF_DEVICE_SURPRISE_REMOVAL HubPdoEvtSurpriseRemoval;
static EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT HubPdoEvtSelfManagedIoStart;
static EVT_WDF_DEVICE_SELF_MANAGED_IO_SUSPEND HubPdoEvtSelfManagedIoSuspend;
static EVT_WDF_DEVICE_QUERY_STOP HubPdoEvtQueryStop;
static EVT_WDF_DEVICE_QUERY_REMOVE HubPdoEvtQueryRemove;
static EVT_WDF_DEVICE_ENABLE_WAKE_AT_BUS HubPdoEvtEnableWakeAtBus;
static EVT_WDF_DEVICE_DISABLE_WAKE_AT_BUS HubPdoEvtDisableWakeAtBus;
static EVT_WDF_DEVICE_REPORTED_MISSING HubPdoEvtReportedMissing;
static EVT_WDF_DEVICE_RESOURCE_REQUIREMENTS_QUERY HubPdoEvtResourceRequirementsQuery;
static EVT_WDF_OBJECT_CONTEXT_CLEANUP HubPdoEvtCleanup;
static EVT_WDFDEVICE_WDM_IRP_PREPROCESS HubPdoPreprocessInternalIoctl;
static EVT_WDFDEVICE_WDM_IRP_PREPROCESS HubPdoPreprocessIoctl;
static EVT_WDFDEVICE_WDM_IRP_PREPROCESS HubPdoPreprocessPnp;
static EVT_WDFDEVICE_WDM_IRP_PREPROCESS HubPdoPreprocessSetPower;
static EVT_WDF_DEVICE_PROCESS_QUERY_INTERFACE_REQUEST HubPdoEvtParentInterfaceQuery;
static EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL HubPdoEvtIoInternalDeviceControl;
static EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL HubPdoEvtIoDeviceControl;
static EVT_WDF_WORKITEM HubPdoEvtCompletePowerIrp;
static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubPdoWakeNotificationComplete;
static POWER_SETTING_CALLBACK HubPdoLpmSettingChanged;

/* FUNCTIONS ******************************************************************/

HubPdo*
HubPdo::FromDevice(
    _In_ WDFDEVICE Device)
{
    return HubGetPdoContext(Device);
}

/* Status mappings */

USBD_STATUS
NTAPI
HubNtStatusToUsbd(
    _In_ NTSTATUS Status)
{
    switch (Status)
    {
        case STATUS_SUCCESS:
            return USBD_STATUS_SUCCESS;

        /* The first one is the hub's own addition to the shared table */
        case STATUS_DEVICE_NOT_CONNECTED:
        case STATUS_NO_SUCH_DEVICE:
            return USBD_STATUS_DEVICE_GONE;

        case STATUS_INSUFFICIENT_RESOURCES:
            return USBD_STATUS_INSUFFICIENT_RESOURCES;

        case STATUS_NOT_SUPPORTED:
            return USBD_STATUS_NOT_SUPPORTED;

        case STATUS_CANCELLED:
            return USBD_STATUS_CANCELED;

        default:
            return USBD_STATUS_INVALID_PARAMETER;
    }
}

static
NTSTATUS
NTAPI
HubUsbdStatusToNt(
    _In_ USBD_STATUS Status)
{
    switch (Status)
    {
        case USBD_STATUS_SUCCESS:
        case USBD_STATUS_PORT_OPERATION_PENDING:
            return STATUS_SUCCESS;

        case USBD_STATUS_INSUFFICIENT_RESOURCES:
            return STATUS_INSUFFICIENT_RESOURCES;

        case USBD_STATUS_INVALID_URB_FUNCTION:
        case USBD_STATUS_INVALID_PARAMETER:
        case USBD_STATUS_INVALID_PIPE_HANDLE:
        case USBD_STATUS_BAD_START_FRAME:
            return STATUS_INVALID_PARAMETER;

        case USBD_STATUS_NOT_SUPPORTED:
            return STATUS_NOT_SUPPORTED;

        case USBD_STATUS_DEVICE_GONE:
            return STATUS_NO_SUCH_DEVICE;

        case USBD_STATUS_CANCELED:
            return STATUS_CANCELLED;

        default:
            return STATUS_UNSUCCESSFUL;
    }
}

/* Small helpers */

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubPdoPostAndWait(
    _In_ HubChild* Child,
    _In_ DsmEvent Event,
    _In_ PCSTR What)
{
    PAGED_CODE();

    KeClearEvent(&Child->m_PnpEvent);
    Child->Post(Event);
    HubWaitForPnpEvent(&Child->m_PnpEvent, What, Child->m_Object);
    return Child->m_PnpStatus;
}

_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoPreStart(
    _In_ HubChild* Child)
{
    PAGED_CODE();

    KeClearEvent(&Child->m_PreStartEvent);
    Child->Post(DsmEvent::PdoPreStart);
    HubWaitForPnpEvent(&Child->m_PreStartEvent, "PdoPreStart", Child->m_Object);
}

static
NTSTATUS
NTAPI
HubPdoCompleteIrp(
    _Inout_ PIRP Irp,
    _In_ NTSTATUS Status)
{
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

/** Completes with whatever status the IRP already carries. */
static
NTSTATUS
NTAPI
HubPdoCompleteAsIs(
    _Inout_ PIRP Irp)
{
    NTSTATUS Status = Irp->IoStatus.Status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

static
NTSTATUS
NTAPI
HubPdoPassDown(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp)
{
    IoSkipCurrentIrpStackLocation(Irp);
    return WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);
}

/* A placeholder has no controller target; it only gets here through the gate exemptions */
static
NTSTATUS
NTAPI
HubPdoToController(
    _In_ HubPdo* Pdo,
    _Inout_ PIRP Irp)
{
    if (Pdo->m_ControllerTarget == NULL)
    {
        DPRINT1("Port %u has no controller target for IRP %p\n", Pdo->m_PortNumber, Irp);
        return HubPdoCompleteIrp(Irp, STATUS_NO_SUCH_DEVICE);
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(Pdo->m_ControllerTarget, Irp);
}

static
NTSTATUS
NTAPI
HubPdoSignalCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/** Lets KMDF handle the IRP, then hands it back here still uncompleted. */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoDispatchAndWait(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp)
{
    KEVENT Event;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, HubPdoSignalCompletion, &Event, TRUE, TRUE, TRUE);

    WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);
    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
}

/* Billboard and dual role properties */

_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoPublishBillboard(
    _In_ HubPdo* Pdo)
{
    PUSB_DEVICE_CAPABILITY_BILLBOARD_DESCRIPTOR Descriptor = Pdo->m_Child->m_Billboard->Descriptor;
    ULONG Count = Descriptor->bNumberOfAlternateModes;
    ULONG Size = sizeof(HubBillboardHeader) + Count * sizeof(HubBillboardMode);
    HubBillboardHeader* Header;
    HubBillboardMode* Modes;
    ULONG Index;
    ULONG Bit;
    NTSTATUS Status;

    PAGED_CODE();

    Header = (HubBillboardHeader*)ExAllocatePoolWithTag(PagedPool, Size, HUB_TAG_DEVICE);
    if (Header == NULL)
    {
        DPRINT1("No memory for the billboard property\n");
        return;
    }

    RtlZeroMemory(Header, Size);
    Header->NumberOfAlternateModes = (UCHAR)Count;
    Header->PreferredModeEntry = Descriptor->bPreferredAlternateMode;

    /* Two bmConfigured bits per mode, the even bit is the low one */
    Modes = (HubBillboardMode*)(Header + 1);
    for (Index = 0; Index < Count; Index++)
    {
        Bit = Index * 2;
        Modes[Index].SVid = Descriptor->AlternateMode[Index].wSVID;
        Modes[Index].Mode = Descriptor->AlternateMode[Index].bAlternateMode;
        Modes[Index].Status = (Descriptor->bmConfigured[Bit / 8] >> (Bit % 8)) & 3;
    }

    Status = IoSetDevicePropertyData(WdfDeviceWdmGetDeviceObject(Pdo->m_Device),
                                     &HubKeyBillboardInfo,
                                     LOCALE_NEUTRAL,
                                     0,
                                     DEVPROP_TYPE_BINARY,
                                     Size,
                                     Header);
    if (!NT_SUCCESS(Status))
        DPRINT1("Billboard property failed 0x%lx\n", Status);

    ExFreePoolWithTag(Header, HUB_TAG_DEVICE);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoReleaseBillboard(
    _In_ HubPdo* Pdo)
{
    HubChild* Child = Pdo->m_Child;

    PAGED_CODE();

    if (Child->m_Billboard == NULL)
        return;

    IoSetDevicePropertyData(WdfDeviceWdmGetDeviceObject(Pdo->m_Device),
                            &HubKeyBillboardInfo,
                            LOCALE_NEUTRAL,
                            0,
                            DEVPROP_TYPE_EMPTY,
                            0,
                            NULL);

    HubFreeBillboardInfo(Child);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoPublishDualRole(
    _In_ HubPdo* Pdo)
{
    ULONG Features = Pdo->m_Child->m_DualRolePartnerFeatures;
    NTSTATUS Status;

    PAGED_CODE();

    Status = IoSetDevicePropertyData(WdfDeviceWdmGetDeviceObject(Pdo->m_Device),
                                     &HubKeyDualRoleFeatures,
                                     LOCALE_NEUTRAL,
                                     0,
                                     DEVPROP_TYPE_UINT32,
                                     sizeof(Features),
                                     &Features);
    if (!NT_SUCCESS(Status))
        DPRINT1("Dual role property failed 0x%lx\n", Status);
}

/* A root port device publishes its own tunnel state, others the one their hub got. QUIRK: written once per enumeration */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoPublishTunnelState(
    _In_ HubPdo* Pdo)
{
    HubFdo* Hub = Pdo->m_Hub;
    ULONG State;
    NTSTATUS Status;

    PAGED_CODE();

    State = Hub->IsRootHub() ? Pdo->m_Child->m_TunnelState : Hub->m_Parent.TunnelState;
    if (State < HUB_TUNNEL_STATE_NOT_USB4 || State > HUB_TUNNEL_STATE_UNKNOWN)
    {
        DPRINT1("Port %u has no valid USB4 tunnel state (%lu)\n", Pdo->m_PortNumber, State);
        return;
    }

    Status = IoSetDevicePropertyData(WdfDeviceWdmGetDeviceObject(Pdo->m_Device),
                                     &HubKeyUsb4TunnelState,
                                     LOCALE_NEUTRAL,
                                     0,
                                     DEVPROP_TYPE_UINT32,
                                     sizeof(State),
                                     &State);
    if (!NT_SUCCESS(Status))
        DPRINT1("Port %u USB4 tunnel state property failed 0x%lx\n", Pdo->m_PortNumber, Status);
}

/* D3cold support interface handed to the function driver */

static
VOID
NTAPI
HubD3ColdReference(
    _In_ PVOID Context)
{
    HubPdo* Pdo = (HubPdo*)Context;

    if (Pdo->m_AcpiD3Cold.InterfaceReference != NULL)
        Pdo->m_AcpiD3Cold.InterfaceReference(Pdo->m_AcpiD3Cold.Context);
}

static
VOID
NTAPI
HubD3ColdDereference(
    _In_ PVOID Context)
{
    HubPdo* Pdo = (HubPdo*)Context;

    if (Pdo->m_AcpiD3Cold.InterfaceDereference != NULL)
        Pdo->m_AcpiD3Cold.InterfaceDereference(Pdo->m_AcpiD3Cold.Context);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubD3ColdSetSupport(
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context,
    _In_ BOOLEAN D3ColdSupport)
{
    HubPdo* Pdo = (HubPdo*)Context;

    if (Pdo->m_AcpiD3Cold.SetD3ColdSupport != NULL)
        Pdo->m_AcpiD3Cold.SetD3ColdSupport(Pdo->m_AcpiD3Cold.Context, D3ColdSupport);

    if (D3ColdSupport)
        Pdo->m_Child->SetState(ChildState::D3ColdEnabledByDriver);
    else
        Pdo->m_Child->ClearState(ChildState::D3ColdEnabledByDriver);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubD3ColdGetIdleWakeInfo(
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context,
    _In_ SYSTEM_POWER_STATE SystemPowerState,
    _Out_ PDEVICE_WAKE_DEPTH DeepestWakeableDstate)
{
    HubPdo* Pdo = (HubPdo*)Context;
    HubChild* Child = Pdo->m_Child;
    NTSTATUS Status;

    PAGED_CODE();

    if (Pdo->m_AcpiD3Cold.GetIdleWakeInfo != NULL)
    {
        Status = Pdo->m_AcpiD3Cold.GetIdleWakeInfo(Pdo->m_AcpiD3Cold.Context,
                                                   SystemPowerState,
                                                   DeepestWakeableDstate);
        if (NT_SUCCESS(Status))
            return Status;
    }

    if (Pdo->m_Hub->m_Capabilities.SystemWake >= SystemPowerState &&
        (Child->HasProperty(ChildProperty::RemoteWakeCapable) || Child->HasProperty(ChildProperty::IsHub)))
    {
        *DeepestWakeableDstate = DeviceWakeDepthD2;
    }
    else
    {
        *DeepestWakeableDstate = DeviceWakeDepthNotWakeable;
    }

    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubD3ColdGetCapability(
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context,
    _Out_ PBOOLEAN D3ColdSupported)
{
    HubPdo* Pdo = (HubPdo*)Context;
    NTSTATUS Status;

    PAGED_CODE();

    if (Pdo->m_AcpiD3Cold.GetD3ColdCapability == NULL)
    {
        *D3ColdSupported = FALSE;
        return STATUS_SUCCESS;
    }

    Status = Pdo->m_AcpiD3Cold.GetD3ColdCapability(Pdo->m_AcpiD3Cold.Context, D3ColdSupported);
    if (NT_SUCCESS(Status) && *D3ColdSupported &&
        Pdo->m_Child->m_Port->HasProperty(PortProperty::Removable))
    {
        DPRINT1("Platform claims D3cold for a removable port %u\n", Pdo->m_PortNumber);
    }

    return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubD3ColdGetBusDriverSupport(
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context,
    _Out_ PBOOLEAN D3ColdSupported)
{
    HubPdo* Pdo = (HubPdo*)Context;

    PAGED_CODE();

    *D3ColdSupported = !Pdo->m_Child->m_Port->HasProperty(PortProperty::Removable);
    return STATUS_SUCCESS;
}

/* A device that was reset on resume lost its state, which is what the client asks */
_IRQL_requires_max_(DISPATCH_LEVEL)
static
VOID
NTAPI
HubD3ColdGetLastTransition(
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context,
    _Out_ PD3COLD_LAST_TRANSITION_STATUS LastTransitionStatus)
{
    HubPdo* Pdo = (HubPdo*)Context;

    *LastTransitionStatus = LastDStateTransitionD3hot;

    if (Pdo->m_AcpiD3Cold.GetLastTransitionStatus != NULL)
    {
        Pdo->m_AcpiD3Cold.GetLastTransitionStatus(Pdo->m_AcpiD3Cold.Context, LastTransitionStatus);
        if (*LastTransitionStatus == LastDStateTransitionD3cold)
            return;
    }

    if (Pdo->HasFlag(PdoFlag::WasResetAtResume))
        *LastTransitionStatus = LastDStateTransitionD3cold;
}

/* Asks the PDO's own stack, so only an ACPI filter above it can answer */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoQueryAcpiD3Cold(
    _In_ HubPdo* Pdo)
{
    WDF_IO_TARGET_OPEN_PARAMS Open;
    D3COLD_SUPPORT_INTERFACE Interface;
    WDFIOTARGET Target;
    BOOLEAN Supported = FALSE;
    NTSTATUS Status;

    PAGED_CODE();

    Status = WdfIoTargetCreate(Pdo->m_Device, WDF_NO_OBJECT_ATTRIBUTES, &Target);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("D3cold query target on port %u not created 0x%lx\n", Pdo->m_PortNumber, Status);
        return;
    }

    WDF_IO_TARGET_OPEN_PARAMS_INIT_EXISTING_DEVICE(&Open, WdfDeviceWdmGetDeviceObject(Pdo->m_Device));

    Status = WdfIoTargetOpen(Target, &Open);
    if (!NT_SUCCESS(Status))
        DPRINT1("D3cold query target on port %u not opened 0x%lx\n", Pdo->m_PortNumber, Status);

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(&Interface, sizeof(Interface));
        Status = WdfIoTargetQueryForInterface(Target,
                                              &GUID_D3COLD_SUPPORT_INTERFACE,
                                              (PINTERFACE)&Interface,
                                              sizeof(Interface),
                                              1,
                                              Pdo);

        /* Expected without an ACPI filter */
        if (!NT_SUCCESS(Status))
            DPRINT("No ACPI D3cold interface on port %u, 0x%lx\n", Pdo->m_PortNumber, Status);

        if (NT_SUCCESS(Status))
        {
            if (Interface.GetD3ColdCapability != NULL &&
                NT_SUCCESS(Interface.GetD3ColdCapability(Interface.Context, &Supported)) &&
                Supported)
            {
                Pdo->m_Child->SetProperty(ChildProperty::AcpiAllowsD3Cold);
            }

            if (Interface.InterfaceDereference != NULL)
                Interface.InterfaceDereference(Interface.Context);
        }
    }

    WdfObjectDelete(Target);
}

/* Query interface */

static
NTSTATUS
NTAPI
HubPdoAnswerD3Cold(
    _In_ HubPdo* Pdo,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PD3COLD_SUPPORT_INTERFACE Interface = (PD3COLD_SUPPORT_INTERFACE)Stack->Parameters.QueryInterface.Interface;

    if (Stack->Parameters.QueryInterface.Size != sizeof(*Interface) ||
        Stack->Parameters.QueryInterface.Version != 1)
    {
        DPRINT("D3cold query on port %u with size %u version %u left alone\n",
               Pdo->m_PortNumber,
               Stack->Parameters.QueryInterface.Size,
               Stack->Parameters.QueryInterface.Version);
        return HubPdoCompleteAsIs(Irp);
    }

    /* An ACPI filter above already filled it on the way down; keep its copy */
    if (Irp->IoStatus.Status == STATUS_SUCCESS)
    {
        if (Interface->Size != sizeof(*Interface) || Interface->Version != 1)
        {
            DPRINT1("ACPI D3cold interface on port %u has size %u version %u\n",
                    Pdo->m_PortNumber,
                    Interface->Size,
                    Interface->Version);
            return HubPdoCompleteIrp(Irp, STATUS_NOT_SUPPORTED);
        }

        Pdo->m_AcpiD3Cold = *Interface;
    }
    else
    {
        Interface->Size = sizeof(*Interface);
        Interface->Version = 1;
    }

    Interface->Context = Pdo;
    Interface->InterfaceReference = HubD3ColdReference;
    Interface->InterfaceDereference = HubD3ColdDereference;
    Interface->SetD3ColdSupport = HubD3ColdSetSupport;
    Interface->GetIdleWakeInfo = HubD3ColdGetIdleWakeInfo;
    Interface->GetD3ColdCapability = HubD3ColdGetCapability;
    Interface->GetBusDriverD3ColdSupport = HubD3ColdGetBusDriverSupport;
    Interface->GetLastTransitionStatus = HubD3ColdGetLastTransition;

    return HubPdoCompleteIrp(Irp, STATUS_SUCCESS);
}

static
NTSTATUS
NTAPI
HubPdoQueryInterface(
    _In_ HubPdo* Pdo,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    const GUID* Type = Stack->Parameters.QueryInterface.InterfaceType;
    PUSBD_CLIENT_INTERFACE Client;
    HubChild* Child = Pdo->m_Child;

    /* Placeholders answer nothing; the PDO's own D3cold probe is left to ACPI */
    if (!Child->HasState(ChildState::WorkingDevice) ||
        Stack->Parameters.QueryInterface.InterfaceSpecificData == Pdo)
    {
        DPRINT("Interface query on port %u left alone\n", Pdo->m_PortNumber);
        return HubPdoCompleteAsIs(Irp);
    }

    if (RtlEqualMemory(Type, &USB_BUS_INTERFACE_USBDI_GUID, sizeof(GUID)))
    {
        Stack->Parameters.QueryInterface.InterfaceSpecificData = Child->m_UsbDevice;
        return HubPdoToController(Pdo, Irp);
    }

    /* The contract is recorded for every 602 or later client, not only exact matches */
    if (RtlEqualMemory(Type, &GUID_USBD_CLIENT_INTERFACE, sizeof(GUID)))
    {
        Stack->Parameters.QueryInterface.InterfaceSpecificData = Child->m_UsbDevice;
        Client = (PUSBD_CLIENT_INTERFACE)Stack->Parameters.QueryInterface.Interface;

        if (Stack->Parameters.QueryInterface.Size >= sizeof(*Client) ||
            Stack->Parameters.QueryInterface.Version >= USBD_CLIENT_INTERFACE_VERSION)
        {
            Pdo->m_ClientContractVersion = Client->ClientContractVersion;
        }

        return HubPdoToController(Pdo, Irp);
    }

    if (RtlEqualMemory(Type, &GUID_UCXHUB_STACK_INTERFACE, sizeof(GUID)))
        return HubPdoToController(Pdo, Irp);

    if (RtlEqualMemory(Type, &GUID_D3COLD_SUPPORT_INTERFACE, sizeof(GUID)))
        return HubPdoAnswerD3Cold(Pdo, Irp);

    return HubPdoPassDown(Pdo->m_Device, Irp);
}

/* Parent interface of a child hub */

_IRQL_requires_max_(DISPATCH_LEVEL)
static
BOOLEAN
NTAPI
HubPdoWasProgrammingLost(
    _In_ PVOID Context)
{
    return ((HubPdo*)Context)->TestAndClearFlag(PdoFlag::ProgrammingLostOnReset);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
static
BOOLEAN
NTAPI
HubPdoWasHubReset(
    _In_ PVOID Context)
{
    HubPdo* Pdo = (HubPdo*)Context;

    if (!Pdo->HasFlag(PdoFlag::WasResetAtResume))
        return FALSE;

    Pdo->ClearFlag(PdoFlag::WasResetAtResume);
    Pdo->ClearFlag(PdoFlag::ReportPortDisabled);
    return TRUE;
}

/* A view of the PDO's string, not a copy */
_IRQL_requires_max_(DISPATCH_LEVEL)
static
VOID
NTAPI
HubPdoGetSymbolicLink(
    _In_ PVOID Context,
    _In_ PUNICODE_STRING SymbolicLinkName)
{
    HubPdo* Pdo = (HubPdo*)Context;

    RtlInitEmptyUnicodeString(SymbolicLinkName, NULL, 0);

    if (Pdo->m_SymbolicLink != NULL)
        WdfStringGetUnicodeString(Pdo->m_SymbolicLink, SymbolicLinkName);
}

/* SuperSpeedPlus is the only enhanced SuperSpeed mode the hub tells apart */
static
BOOLEAN
NTAPI
HubPdoIsEnhancedSuperSpeed(
    _In_ HubChild* Child)
{
    return (Child->m_Kind & DSM_KIND_SUPER_SPEED) && Child->m_SublinkSpeedAttrCount != 0;
}

/* The requester's Size is only checked by KMDF's template test */
static
NTSTATUS
NTAPI
HubPdoEvtParentInterfaceQuery(
    _In_ WDFDEVICE Device,
    _In_ LPGUID InterfaceType,
    _Inout_ PINTERFACE ExposedInterface,
    _Inout_opt_ PVOID ExposedInterfaceSpecificData)
{
    HubPdo* Pdo = HubGetPdoContext(Device);
    HubChild* Child = Pdo->m_Child;
    HubFdo* Hub = Pdo->m_Hub;
    HubPort* Port = Child->m_Port;
    PUCXHUB_PARENT_INTERFACE Parent = (PUCXHUB_PARENT_INTERFACE)ExposedInterface;
    UCHAR Depth;

    UNREFERENCED_PARAMETER(InterfaceType);
    UNREFERENCED_PARAMETER(ExposedInterfaceSpecificData);

    if (!Child->HasProperty(ChildProperty::IsHub))
    {
        DPRINT1("Parent interface query on port %u, which is not a hub\n", Pdo->m_PortNumber);
        return STATUS_UNSUCCESSFUL;
    }

    Depth = Hub->m_Parent.HubDepth + 1;

    Parent->Header.Size = sizeof(*Parent);
    Parent->Header.Version = UCXHUB_PARENT_INTERFACE_VERSION;
    Parent->Header.Context = Pdo;
    Parent->Header.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
    Parent->Header.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

    Parent->HubDepth = Depth;
    Parent->ParentCanWake = Child->HasProperty(ChildProperty::RemoteWakeCapable);

    Parent->HubTopologyAddress = Hub->m_Parent.HubTopologyAddress;
    if (Depth == 1)
        Parent->HubTopologyAddress.RootHubPortNumber = Pdo->m_PortNumber;
    else if (Depth - 2 < (UCHAR)RTL_NUMBER_OF(Parent->HubTopologyAddress.HubPortNumber))
        Parent->HubTopologyAddress.HubPortNumber[Depth - 2] = Pdo->m_PortNumber;

    Parent->HubSpeed = Child->Speed();
    Parent->IsEnhancedSuperSpeed = HubPdoIsEnhancedSuperSpeed(Child);
    Parent->Hub = Child->m_UsbDevice;

    /* A hub on a root port hands down its own tunnel state; deeper hubs inherit it */
    Parent->TunnelState = (Depth == 1) ? Child->m_TunnelState : Hub->m_Parent.TunnelState;

    Parent->ParentLostStateDuringResume = HubPdoWasProgrammingLost;
    Parent->ParentResetDuringResume = HubPdoWasHubReset;
    Parent->ConnectorId = &Port->m_ConnectorId;
    Parent->QueryHubLinkName = HubPdoGetSymbolicLink;

    if (Port->HasProperty(PortProperty::IntegratedHub))
    {
        Parent->FirstCompanionPort = Port->m_FirstCompanionPort;
        Parent->LastCompanionPort = Port->m_LastCompanionPort;

        if (Hub->HasFlag(HubFlag::NoSelectiveSuspendIntegrated))
            Parent->ParentCanWake = FALSE;
    }

    Pdo->m_ChildHubFdo = Parent->ChildHubObject;
    DPRINT("Child hub %p on port %u at depth %u\n", Pdo->m_ChildHubFdo, Pdo->m_PortNumber, Depth);
    return STATUS_SUCCESS;
}

/* Location interface */

_IRQL_requires_max_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubPdoGetLocationString(
    _Inout_opt_ PVOID Context,
    _Outptr_ PZZWSTR* LocationStrings)
{
    HubChild* Child = (HubChild*)Context;
    const SIZE_T Count = 10;
    PWCHAR Buffer;
    NTSTATUS Status;

    PAGED_CODE();

    *LocationStrings = NULL;

    Buffer = (PWCHAR)ExAllocatePoolWithTag(PagedPool, Count * sizeof(WCHAR), HUB_TAG_DEVICE);
    if (Buffer == NULL)
    {
        DPRINT1("No memory for the location string of port %u\n", Child->m_Port->Number());
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Filling behind the NUL leaves the second NUL of the multi string */
    Status = RtlStringCchPrintfExW(Buffer, Count, NULL, NULL, STRSAFE_FILL_BEHIND_NULL,
                                   L"USB(%d)", Child->m_Port->Number());
    if (!NT_SUCCESS(Status))
        DPRINT1("Location string failed 0x%lx\n", Status);

    *LocationStrings = Buffer;
    return STATUS_SUCCESS;
}

/* PnP IRP preprocessing */

/* A device id query before start brings the device back for an AddDevice that sends I/O */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoPreStartOnQueryId(
    _In_ HubPdo* Pdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    WDFDEVICE HubDevice = Pdo->m_Hub->m_Device;
    NTSTATUS Status;

    if (Pdo->HasFlag(PdoFlag::Started) ||
        Stack->Parameters.QueryId.IdType != BusQueryDeviceID ||
        KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
        return;
    }

    Status = WdfDeviceStopIdle(HubDevice, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub did not power up for pre start 0x%lx\n", Status);
        return;
    }

    HubPdoPreStart(Pdo->m_Child);
    WdfDeviceResumeIdle(HubDevice);
}

static
NTSTATUS
NTAPI
HubPdoPreprocessPnp(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp)
{
    HubPdo* Pdo = HubGetPdoContext(Device);
    HubChild* Child = Pdo->m_Child;
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS EntryStatus = Irp->IoStatus.Status;

    switch (Stack->MinorFunction)
    {
        case IRP_MN_QUERY_INTERFACE:
            return HubPdoQueryInterface(Pdo, Irp);

        case IRP_MN_QUERY_DEVICE_TEXT:
            return HubIdQueryDeviceText(Child, Irp);

        case IRP_MN_QUERY_ID:
            HubPdoPreStartOnQueryId(Pdo, Irp);
            return HubPdoPassDown(Device, Irp);

        case IRP_MN_REMOVE_DEVICE:
            Pdo->m_ChildHubFdo = NULL;
            return HubPdoPassDown(Device, Irp);

        /* KMDF leaves STATUS_NOT_SUPPORTED here; the EHCI stack answered success */
        case IRP_MN_DEVICE_ENUMERATED:
            HubPdoDispatchAndWait(Device, Irp);

            HubPdoQueryAcpiD3Cold(Pdo);
            if (Child->m_Billboard != NULL)
                HubPdoPublishBillboard(Pdo);
            if (Child->HasProperty(ChildProperty::DualRole))
                HubPdoPublishDualRole(Pdo);
            HubPdoPublishTunnelState(Pdo);

            return HubPdoCompleteIrp(Irp, STATUS_SUCCESS);

        /* Kept synchronous; the EHCI stack never pended these */
        case IRP_MN_START_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
            HubPdoDispatchAndWait(Device, Irp);
            return HubPdoCompleteAsIs(Irp);

        /* KMDF writes success; the status from above is returned instead */
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            HubPdoDispatchAndWait(Device, Irp);
            return HubPdoCompleteIrp(Irp, EntryStatus);

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            HubPdoDispatchAndWait(Device, Irp);

            if (Irp->IoStatus.Information == 0)
                return HubPdoCompleteIrp(Irp, EntryStatus);

            if (Child->m_EnumMessageId != 0)
            {
                DPRINT1("Port %u reports enumeration problem %lu\n", Pdo->m_PortNumber, Child->m_EnumMessageId);
                HubWriteEnumerationFailureCode(Child);
                HubReportPnpProblem(Device, Child->m_EnumMessageId);
            }

            return HubPdoCompleteAsIs(Irp);

        default:
            return HubPdoPassDown(Device, Irp);
    }
}

/* Power IRP preprocessing */

static
VOID
NTAPI
HubPdoEvtCompletePowerIrp(
    _In_ WDFWORKITEM WorkItem)
{
    IoCompleteRequest(HubGetPdoIrpWork(WorkItem)->Irp, IO_NO_INCREMENT);
    WdfObjectDelete(WorkItem);
}

/*
 * KMDF may complete a D IRP inside its power machine; finishing it on a work
 * item keeps a driver above from sending the next IRP on this same thread.
 */
static
NTSTATUS
NTAPI
HubPdoPowerIrpComplete(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    HubPdo* Pdo = (HubPdo*)Context;
    WDF_WORKITEM_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFWORKITEM WorkItem;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned)
        IoMarkIrpPending(Irp);

    WDF_WORKITEM_CONFIG_INIT(&Config, HubPdoEvtCompletePowerIrp);
    Config.AutomaticSerialization = FALSE;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, HubPdoIrpWork);
    Attributes.ParentObject = Pdo->m_Device;

    Status = WdfWorkItemCreate(&Config, &Attributes, &WorkItem);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Port %u power IRP %p completes inline, work item failed 0x%lx\n",
                Pdo->m_PortNumber,
                Irp,
                Status);
        return STATUS_CONTINUE_COMPLETION;
    }

    HubGetPdoIrpWork(WorkItem)->Irp = Irp;
    WdfWorkItemEnqueue(WorkItem);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/* A client that went D2 then D3 on a system sleep gets its wait wake and idle IRPs back */
static
NTSTATUS
NTAPI
HubPdoPreprocessSetPower(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp)
{
    HubPdo* Pdo = HubGetPdoContext(Device);
    HubChild* Child = Pdo->m_Child;
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    DEVICE_POWER_STATE Target;
    NTSTATUS Status;

    if (Stack->Parameters.Power.Type != DevicePowerState)
        return HubPdoPassDown(Device, Irp);

    Target = Stack->Parameters.Power.State.DeviceState;
    DPRINT("Port %u device power IRP to D%d\n", Pdo->m_PortNumber, (int)Target - 1);

    if (Target == PowerDeviceD3 && Child->m_PowerState == PowerDeviceD2)
    {
        /* Fails when no wait wake IRP is pending, which is fine */
        Status = WdfDeviceIndicateWakeStatus(Device, STATUS_POWER_STATE_INVALID);
        if (!NT_SUCCESS(Status))
            DPRINT1("Port %u D2 to D3 wake status not taken 0x%lx\n", Pdo->m_PortNumber, Status);

        Pdo->m_Idle.Post(IdleEvent::PoweringDown, NULL);
    }

    Child->m_PowerState = Target;

    /* The completion may finish the IRP later on a work item, so it is always pending */
    IoMarkIrpPending(Irp);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, HubPdoPowerIrpComplete, Pdo, TRUE, TRUE, TRUE);
    WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);
    return STATUS_PENDING;
}

/* PnP and power callbacks */

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubPdoEvtPrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    DECLARE_CONST_UNICODE_STRING(SymbolicName, L"SymbolicName");
    HubPdo* Pdo = HubGetPdoContext(Device);
    HubChild* Child = Pdo->m_Child;
    HubFdo* Hub = Pdo->m_Hub;
    WDF_OBJECT_ATTRIBUTES Attributes;
    const GUID* InterfaceGuid;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PAGED_CODE();

    DPRINT("Child PDO %p on port %u prepare hardware\n", Device, Pdo->m_PortNumber);

    /* From here on a device id query no longer pre starts the device */
    Pdo->SetFlag(PdoFlag::Started);

    HubReadDeviceHardwareValues(Child);

    /* The state is the controller's answer from the device update, which ran before */
    if (HubUsb4NeedsHostPower(Child))
    {
        if (Hub->m_TunneledDevicesSkipU2)
        {
            DPRINT("Port %u keeps U2 off for its tunneled device\n", Pdo->m_PortNumber);
            InterlockedOr((volatile LONG*)&Child->m_Hacks, (LONG)ChildHack::DisableLpm);
        }

        HubUsb4AcquireHostPower(Child->m_Port);
    }

    HubCacheMsOs20SetInfo(Child);
    HubFlushSqmFlags(Child);
    HubWriteEnumerationData(Child);

    if (Child->HasProperty(ChildProperty::IsHub))
    {
        if (Hub->m_Parent.HubDepth >= Child->m_Port->m_Info.TotalHubDepth)
        {
            DPRINT1("Hub on port %u nested too deeply, depth %u\n", Pdo->m_PortNumber, Hub->m_Parent.HubDepth);
            Child->m_Port->m_ConnectionStatus = DeviceHubNestedTooDeeply;
            HubWmiNotifyHubNestedTooDeeply(Hub, Pdo->m_PortNumber);
            return STATUS_UNSUCCESSFUL;
        }

        InterfaceGuid = &GUID_DEVINTERFACE_USB_HUB;
    }
    else
    {
        InterfaceGuid = &GUID_DEVINTERFACE_USB_DEVICE;
    }

    Status = WdfDeviceCreateDeviceInterface(Device, InterfaceGuid, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Port %u device interface not created 0x%lx\n", Pdo->m_PortNumber, Status);
        return Status;
    }

    if (Pdo->m_SymbolicLink == NULL)
    {
        WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
        Attributes.ParentObject = Device;

        Status = WdfStringCreate(NULL, &Attributes, &Pdo->m_SymbolicLink);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Port %u symbolic link string not created 0x%lx\n", Pdo->m_PortNumber, Status);
            return Status;
        }
    }

    Status = WdfDeviceRetrieveDeviceInterfaceString(Device, InterfaceGuid, NULL, Pdo->m_SymbolicLink);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Port %u symbolic link not retrieved 0x%lx\n", Pdo->m_PortNumber, Status);
        return Status;
    }

    Status = HubWriteDeviceString(Child, &SymbolicName, Pdo->m_SymbolicLink);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Port %u symbolic link not written to the registry 0x%lx\n", Pdo->m_PortNumber, Status);
        return Status;
    }

    if (!Pdo->HasFlag(PdoFlag::DeviceGone) && Child->HasState(ChildState::WorkingDevice))
        Pdo->m_FailRequests = FALSE;

    if (!Child->HasProperty(ChildProperty::IsHub) && !Child->HasProperty(ChildProperty::IsComposite))
        HubWmiRegisterDevice(Device);

    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubPdoEvtReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    HubPdo* Pdo = HubGetPdoContext(Device);
    HubChild* Child = Pdo->m_Child;

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PAGED_CODE();

    DPRINT("Child PDO %p on port %u release hardware\n", Device, Pdo->m_PortNumber);

    Pdo->m_FailRequests = TRUE;
    Pdo->ClearFlag(PdoFlag::Started);

    /* A rebalance keeps the UXD and dual role state */
    if (Pdo->TestAndClearFlag(PdoFlag::QueryStopped))
        return STATUS_SUCCESS;

    if (Pdo->TestAndClearFlag(PdoFlag::QueryRemoved))
        HubPurgeUxdState(Child, HubUxdEvent::Disable);

    if (Child->HasProperty(ChildProperty::DualRole))
        Child->ClearProperty(ChildProperty::DualRole);

    /* QUIRK: a rebalance returned above, so its next prepare takes a second reference */
    if (HubUsb4NeedsHostPower(Child))
        HubUsb4ReleaseHostPower(Child->m_Port);

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
HubPdoEvtQueryStop(
    _In_ WDFDEVICE Device)
{
    HubGetPdoContext(Device)->SetFlag(PdoFlag::QueryStopped);
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
HubPdoEvtQueryRemove(
    _In_ WDFDEVICE Device)
{
    HubGetPdoContext(Device)->SetFlag(PdoFlag::QueryRemoved);
    return STATUS_SUCCESS;
}

/* KMDF may skip ReportedMissing when the whole hub is gone, so the child list is cleaned here */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoEvtSurpriseRemoval(
    _In_ WDFDEVICE Device)
{
    HubPdo* Pdo = HubGetPdoContext(Device);
    HubChild* Child = Pdo->m_Child;
    HubFdo* Hub = Pdo->m_Hub;

    PAGED_CODE();

    DPRINT("Child PDO %p on port %u surprise removed\n", Device, Pdo->m_PortNumber);

    if (Hub->m_Stack.IsDeviceDisconnected != NULL &&
        Hub->m_Stack.IsDeviceDisconnected(Hub->UsbDevice()) &&
        Child->m_SerialNumber != NULL)
    {
        USBD_RemoveDeviceFromGlobalList(Child);
    }

    Pdo->m_FailRequests = TRUE;
    HubPdoReleaseBillboard(Pdo);
}

static
VOID
NTAPI
HubPdoEvtReportedMissing(
    _In_ WDFDEVICE Device)
{
    HubPdo* Pdo = HubGetPdoContext(Device);

    DPRINT("Child PDO %p on port %u reported missing\n", Device, Pdo->m_PortNumber);
    Pdo->m_Child->Post(DsmEvent::PdoReportedMissing);
}

/* MS OS extended properties go in here, before the function driver's AddDevice */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubPdoEvtResourceRequirementsQuery(
    _In_ WDFDEVICE Device,
    _In_ WDFIORESREQLIST IoResourceRequirementsList)
{
    HubPdo* Pdo = HubGetPdoContext(Device);
    WDFDEVICE HubDevice = Pdo->m_Hub->m_Device;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(IoResourceRequirementsList);

    PAGED_CODE();

    Status = WdfDeviceStopIdle(HubDevice, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub did not power up for the resource query 0x%lx\n", Status);
        return STATUS_SUCCESS;
    }

    /* The driver wait timer may have disabled the device meanwhile */
    if (!Pdo->HasFlag(PdoFlag::Started))
        HubPdoPreStart(Pdo->m_Child);

    HubPdoPostAndWait(Pdo->m_Child, DsmEvent::PdoInstallMsOsExt, "PdoInstallMsOsExt");

    WdfDeviceResumeIdle(HubDevice);
    return STATUS_SUCCESS;
}

/* An unreported PDO skips the device machine */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoEvtCleanup(
    _In_ WDFOBJECT Object)
{
    HubPdo* Pdo = HubGetPdoContext(Object);
    HubChild* Child = Pdo->m_Child;
    HubFdo* Hub = Pdo->m_Hub;
    UCXHUB_STOP_IDLE_CONTEXT StopIdle;
    NTSTATUS Status;
    ULONG Index;

    PAGED_CODE();

    DPRINT("Child PDO %p on port %u cleanup, reported %u\n", Object, Pdo->m_PortNumber, Pdo->m_Reported);

    for (Index = 0; Index < HUB_PDO_LPM_SETTINGS; Index++)
    {
        if (Pdo->m_LpmSettings[Index] == NULL)
            continue;

        Status = HubDriver.UnregisterPowerSetting(Pdo->m_LpmSettings[Index]);
        if (!NT_SUCCESS(Status))
            DPRINT1("LPM setting %lu unregister failed 0x%lx\n", Index, Status);
        Pdo->m_LpmSettings[Index] = NULL;
    }

    if (Pdo->HasFlag(PdoFlag::IdleMachineStarted))
        Pdo->m_Idle.Post(IdleEvent::Cleanup, NULL);

    if (Pdo->m_Reported)
    {
        /* The device machine deletes the device from a controller that may be idle */
        RtlZeroMemory(&StopIdle, sizeof(StopIdle));
        Hub->m_Stack.BlockControllerIdle(Hub->UsbDevice(), &StopIdle);

        HubPdoPostAndWait(Child, DsmEvent::PdoCleanup, "PdoCleanup");
        HubPdoReleaseBillboard(Pdo);

        Hub->m_Stack.AllowControllerIdle(Hub->UsbDevice(), &StopIdle);
    }

    WdfObjectDereferenceWithTag(Child->m_Object, (PVOID)HubTagPdo);

    if (Pdo->m_StartFailData != NULL)
    {
        ExFreePoolWithTag(Pdo->m_StartFailData, HUB_TAG_DEVICE);
        Pdo->m_StartFailData = NULL;
    }
}

static
NTSTATUS
NTAPI
HubPdoEvtSelfManagedIoStart(
    _In_ WDFDEVICE Device)
{
    HubGetPdoContext(Device)->SetFlag(PdoFlag::InD0);
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
HubPdoEvtSelfManagedIoSuspend(
    _In_ WDFDEVICE Device)
{
    HubPdo* Pdo = HubGetPdoContext(Device);

    Pdo->ClearFlag(PdoFlag::InD0);
    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubPdoEvtD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
    HubPdo* Pdo = HubGetPdoContext(Device);
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(PreviousState);

    PAGED_CODE();

    DPRINT("Child PDO %p on port %u D0 entry from %d\n", Device, Pdo->m_PortNumber, (int)PreviousState);

    Pdo->m_WdfPowerState = WdfPowerDeviceD0;
    Pdo->m_Idle.Post(IdleEvent::PoweredUp, NULL);

    /* Activating the hub's PoFx component for the host router is not supported */
    if (HubUsb4NeedsHostPower(Pdo->m_Child))
        DPRINT("Port %u USB4 power component activation is not available\n", Pdo->m_PortNumber);

    Status = HubPdoPostAndWait(Pdo->m_Child, DsmEvent::PdoPowerUp, "PdoPowerUp");
    if (!NT_SUCCESS(Status))
        DPRINT1("Port %u D0 entry failed 0x%lx\n", Pdo->m_PortNumber, Status);

    return Status;
}

/* A D3Final with a system power action posts PdoPowerDown */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubPdoEvtD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState)
{
    HubPdo* Pdo = HubGetPdoContext(Device);
    HubChild* Child = Pdo->m_Child;
    POWER_ACTION Action;
    DsmEvent Event;
    NTSTATUS Status;

    PAGED_CODE();

    Pdo->m_WdfPowerState = TargetState;
    KeClearEvent(&Child->m_PnpEvent);
    Action = WdfDeviceGetSystemPowerAction(Device);

    DPRINT("Child PDO %p on port %u D0 exit to %d, action %d\n",
           Device,
           Pdo->m_PortNumber,
           (int)TargetState,
           (int)Action);

    if (TargetState != WdfPowerDeviceD3Final && Action == PowerActionNone)
        HubRecordSelectiveSuspend(Child);

    if (TargetState == WdfPowerDeviceD3Final && Action == PowerActionNone)
    {
        Event = DsmEvent::PdoPowerDownFinal;
    }
    else if (TargetState == WdfPowerDevicePrepareForHibernation)
    {
        Event = DsmEvent::PdoPrepareHibernate;
    }
    else
    {
        /* Wait wake and idle IRPs complete when the device goes to D3 */
        if (TargetState == WdfPowerDeviceD3)
        {
            if (Pdo->HasFlag(PdoFlag::WakeArmRequested))
            {
                Status = WdfDeviceIndicateWakeStatus(Device, STATUS_POWER_STATE_INVALID);
                if (!NT_SUCCESS(Status))
                    DPRINT1("Wake status not taken 0x%lx\n", Status);
                Pdo->ClearFlag(PdoFlag::WakeArmRequested);
            }

            if (!Child->HasProperty(ChildProperty::AllowIdleIrpInD3))
                Pdo->m_Idle.Post(IdleEvent::PoweringDown, NULL);
        }

        Event = DsmEvent::PdoPowerDown;
    }

    Child->Post(Event);
    HubWaitForPnpEvent(&Child->m_PnpEvent, "PdoPowerDown", Child->m_Object);
    Status = Child->m_PnpStatus;
    if (!NT_SUCCESS(Status))
        DPRINT1("Port %u D0 exit failed 0x%lx\n", Pdo->m_PortNumber, Status);

    if (WdfTimerStop(Child->m_BandwidthRetryTimer, TRUE))
    {
        DPRINT1("Port %u ran out of bandwidth\n", Pdo->m_PortNumber);
        HubWmiNotifyInsufficientBandwidth(Pdo->m_Hub, Pdo->m_PortNumber);
    }

    /* Idling the hub's PoFx component for the host router is not supported */
    if (HubUsb4NeedsHostPower(Child))
        DPRINT("Port %u USB4 power component idling is not available\n", Pdo->m_PortNumber);

    return Status;
}

/* Wake */

static
VOID
NTAPI
HubPdoIndicateWake(
    _In_ HubPdo* Pdo)
{
    NTSTATUS Status;
    KIRQL OldIrql;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    Status = WdfDeviceIndicateWakeStatus(Pdo->m_Device, STATUS_SUCCESS);
    KeLowerIrql(OldIrql);

    if (!NT_SUCCESS(Status))
        DPRINT1("Wake status not taken 0x%lx\n", Status);
}

/* A notification canceled by DisableWakeAtBus still reports success; KMDF ignores it then */
static
VOID
NTAPI
HubPdoWakeNotificationComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubPdo* Pdo = (HubPdo*)Context;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Params);

    DPRINT("Port %u wake notification done 0x%lx\n", Pdo->m_PortNumber, Params->IoStatus.Status);
    HubPdoIndicateWake(Pdo);
    KeSetEvent(&Pdo->m_WakeDone, IO_NO_INCREMENT, FALSE);
}

static
NTSTATUS
NTAPI
HubPdoSendWakeNotification(
    _In_ HubPdo* Pdo)
{
    WDF_REQUEST_REUSE_PARAMS Reuse;
    IO_STACK_LOCATION Stack;
    WDFREQUEST Request = Pdo->m_WakeRequest;
    NTSTATUS Status;

    WDF_REQUEST_REUSE_PARAMS_INIT(&Reuse, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
    Status = WdfRequestReuse(Request, &Reuse);
    if (!NT_SUCCESS(Status))
        DPRINT1("Wake request reuse failed 0x%lx\n", Status);

    RtlZeroMemory(&Pdo->m_WakeBlock, sizeof(Pdo->m_WakeBlock));
    Pdo->m_WakeBlock.Size = sizeof(Pdo->m_WakeBlock);
    KeClearEvent(&Pdo->m_WakeDone);

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    Stack.Parameters.Others.Argument1 = &Pdo->m_WakeBlock;
    Stack.Parameters.Others.Argument2 = Pdo->m_Child->m_UsbDevice;
    Stack.Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_REQUEST_REMOTE_WAKE_NOTIFICATION;

    WdfRequestWdmFormatUsingStackLocation(Request, &Stack);
    WdfRequestSetCompletionRoutine(Request, HubPdoWakeNotificationComplete, Pdo);

    if (!WdfRequestSend(Request, Pdo->m_Hub->m_RootHubTarget, WDF_NO_SEND_OPTIONS))
    {
        Status = WdfRequestGetStatus(Request);
        DPRINT1("Port %u wake notification not sent 0x%lx\n", Pdo->m_PortNumber, Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

/* WaitWakePending stays set on failure */
static
NTSTATUS
NTAPI
HubPdoEvtEnableWakeAtBus(
    _In_ WDFDEVICE Device,
    _In_ SYSTEM_POWER_STATE PowerState)
{
    HubPdo* Pdo = HubGetPdoContext(Device);
    HubChild* Child = Pdo->m_Child;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("Port %u enable wake for S%d\n", Pdo->m_PortNumber, (int)PowerState - 1);

    Pdo->SetFlag(PdoFlag::WaitWakePending);

    /* Hubs pass their children's wake on but are not armed for Sx here */
    if (Child->HasProperty(ChildProperty::IsHub) && PowerState != PowerSystemWorking)
        return STATUS_SUCCESS;

    if (!Child->HasProperty(ChildProperty::RemoteWakeCapable))
    {
        DPRINT1("Port %u device cannot wake\n", Pdo->m_PortNumber);
        return STATUS_NOT_SUPPORTED;
    }

    if (Pdo->HasFlag(PdoFlag::ClientDoesFunctionSuspend))
        return STATUS_SUCCESS;

    if (Child->m_Kind & DSM_KIND_SUPER_SPEED)
    {
        Status = HubPdoSendWakeNotification(Pdo);
        if (NT_SUCCESS(Status))
            Pdo->SetFlag(PdoFlag::WakeNotificationSent);
    }

    Pdo->SetFlag(PdoFlag::WakeArmRequested);
    return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoEvtDisableWakeAtBus(
    _In_ WDFDEVICE Device)
{
    HubPdo* Pdo = HubGetPdoContext(Device);

    PAGED_CODE();

    DPRINT("Port %u disable wake\n", Pdo->m_PortNumber);

    Pdo->ClearFlag(PdoFlag::WakeArmRequested);
    Pdo->ClearFlag(PdoFlag::WaitWakePending);

    if (Pdo->HasFlag(PdoFlag::ClientDoesFunctionSuspend))
        return;

    if (Pdo->TestAndClearFlag(PdoFlag::WakeNotificationSent))
    {
        WdfRequestCancelSentRequest(Pdo->m_WakeRequest);
        KeWaitForSingleObject(&Pdo->m_WakeDone, Executive, KernelMode, FALSE, NULL);
    }
}

/* LPM power settings; the link power management policy value length is not checked */
static
NTSTATUS
NTAPI
HubPdoLpmSettingChanged(
    _In_ LPCGUID SettingGuid,
    _In_reads_bytes_(ValueLength) PVOID Value,
    _In_ ULONG ValueLength,
    _Inout_opt_ PVOID Context)
{
    HubChild* Child = (HubChild*)Context;
    ULONG Policy = Child->m_LpmPolicy;
    ULONG Slot;
    ULONG Setting;
    ULONG Accept;
    ULONG Initiate;

    for (Slot = 0; Slot < HUB_PDO_LPM_SETTINGS; Slot++)
    {
        if (RtlEqualMemory(SettingGuid, &HubLpmDeviceSettings[Slot], sizeof(GUID)) ||
            RtlEqualMemory(SettingGuid, &HubLpmHubSettings[Slot], sizeof(GUID)))
        {
            break;
        }
    }

    if (Slot == HUB_PDO_LPM_SETTINGS)
        return STATUS_SUCCESS;

    if (Slot != HubLpmPolicy && ValueLength < sizeof(ULONG))
    {
        DPRINT1("LPM setting %lu of %p has only %lu bytes\n", Slot, Child, ValueLength);
        return STATUS_INVALID_PARAMETER;
    }

    Setting = *(PULONG)Value;

    switch (Slot)
    {
        case HubLpmPolicy:
            if (Setting > 3)
            {
                DPRINT1("LPM policy %lu of %p is out of range\n", Setting, Child);
                return STATUS_INVALID_PARAMETER;
            }

            if (Setting == 0)
            {
                Policy = 0;
                break;
            }

            Policy |= HUB_LPM_POLICY_LINK_BITS;
            Policy &= ~(HUB_LPM_POLICY_CONSERVATIVE | HUB_LPM_POLICY_AGGRESSIVE);
            if (Setting == 1)
                Policy |= HUB_LPM_POLICY_CONSERVATIVE;
            else if (Setting == 3)
                Policy |= HUB_LPM_POLICY_AGGRESSIVE;
            break;

        case HubLpmU1Enable:
            if (Setting != 0)
                Policy |= HUB_LPM_POLICY_U1_ENABLED;
            else
                Policy &= ~HUB_LPM_POLICY_U1_ENABLED;
            break;

        case HubLpmU2Enable:
            if (Setting != 0)
                Policy |= HUB_LPM_POLICY_U2_ENABLED;
            else
                Policy &= ~HUB_LPM_POLICY_U2_ENABLED;
            break;

        default:
            Accept = (Slot == HubLpmU1Timeout) ? HUB_LPM_POLICY_U1_ACCEPT : HUB_LPM_POLICY_U2_ACCEPT;
            Initiate = (Slot == HubLpmU1Timeout) ? HUB_LPM_POLICY_U1_INITIATE : HUB_LPM_POLICY_U2_INITIATE;

            if (Setting == 0)
                Policy |= Accept | Initiate;
            else if (Setting == 1)
                Policy = (Policy | Accept) & ~Initiate;
            else if (Setting == 2)
                Policy &= ~(Accept | Initiate);
            break;
    }

    if (Policy != Child->m_LpmPolicy)
    {
        DPRINT("LPM policy of %p now 0x%lx\n", Child, Policy);
        Child->m_LpmPolicy = Policy;
        Child->Post(DsmEvent::LpmSettingChanged);
    }

    return STATUS_SUCCESS;
}

/* The power manager calls back at once with the current values */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoRegisterLpmSettings(
    _In_ HubPdo* Pdo)
{
    HubChild* Child = Pdo->m_Child;
    const GUID* Settings;
    NTSTATUS Status;
    ULONG Index;

    PAGED_CODE();

    if (HubDriver.RegisterPowerSetting == NULL)
        return;

    Settings = Child->HasProperty(ChildProperty::IsHub) ? HubLpmHubSettings : HubLpmDeviceSettings;

    for (Index = 0; Index < HUB_PDO_LPM_SETTINGS; Index++)
    {
        Status = HubDriver.RegisterPowerSetting(WdfDeviceWdmGetDeviceObject(Pdo->m_Device),
                                                &Settings[Index],
                                                HubPdoLpmSettingChanged,
                                                Child,
                                                &Pdo->m_LpmSettings[Index]);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("LPM setting %lu register failed 0x%lx\n", Index, Status);
            Pdo->m_LpmSettings[Index] = NULL;
        }
    }
}

/* Client internal IOCTLs */

/* KMDF can fail a URB before the device machine sees it; give it a USBD status then */
static
NTSTATUS
NTAPI
HubPdoSyncComplete(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = Irp->IoStatus.Status;
    PURB Urb;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!NT_SUCCESS(Status) &&
        Stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB)
    {
        Urb = (PURB)Stack->Parameters.Others.Argument1;
        if (Urb->UrbHeader.Status == USBD_STATUS_SUCCESS)
            Urb->UrbHeader.Status = HubNtStatusToUsbd(Status);
    }

    KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/* Callers must be at PASSIVE_LEVEL */
static
NTSTATUS
NTAPI
HubPdoQueueAndWait(
    _In_ HubPdo* Pdo,
    _Inout_ PIRP Irp,
    _In_ WDFQUEUE Queue)
{
    KEVENT Event;
    NTSTATUS Status;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, HubPdoSyncComplete, &Event, TRUE, TRUE, TRUE);

    /* On failure KMDF completes the IRP, so the completion routine still runs */
    Status = WdfDeviceWdmDispatchIrpToIoQueue(Pdo->m_Device, Irp, Queue, WDF_DISPATCH_IRP_TO_IO_QUEUE_PREPROCESSED_IRP);
    if (!NT_SUCCESS(Status))
        DPRINT1("Dispatch to queue failed 0x%lx\n", Status);

    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

    Status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

static
NTSTATUS
NTAPI
HubPdoQueueAsync(
    _In_ HubPdo* Pdo,
    _Inout_ PIRP Irp)
{
    IoCopyCurrentIrpStackLocationToNext(Irp);
    return WdfDeviceWdmDispatchIrpToIoQueue(Pdo->m_Device, Irp, Pdo->m_Queue, WDF_DISPATCH_IRP_TO_IO_QUEUE_PREPROCESSED_IRP);
}

static
VOID
NTAPI
HubPdoStampUrb(
    _In_ HubPdo* Pdo,
    _Inout_ PURB Urb)
{
    Urb->UrbHeader.UsbdDeviceHandle = Pdo->m_Child->m_UsbDevice;
}

/* A pipe request length error is reported as an invalid function */
static
NTSTATUS
NTAPI
HubPdoValidateUrb(
    _Inout_ PURB Urb)
{
    USHORT Function = Urb->UrbHeader.Function;

    if (Urb->UrbHeader.Length < sizeof(struct _URB_HEADER))
        DPRINT1("URB %p header length %u too small\n", Urb, Urb->UrbHeader.Length);

    Urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
    Urb->UrbHeader.UsbdFlags = 0;

    if (Function > HUB_URB_FUNCTION_LAST)
    {
        DPRINT1("URB %p has invalid function 0x%x\n", Urb, Function);
        Urb->UrbHeader.Status = USBD_STATUS_INVALID_URB_FUNCTION;
        return STATUS_INVALID_PARAMETER;
    }

    if ((Function == URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL ||
         Function == URB_FUNCTION_SYNC_RESET_PIPE ||
         Function == URB_FUNCTION_SYNC_CLEAR_STALL) &&
        Urb->UrbHeader.Length != sizeof(struct _URB_PIPE_REQUEST))
    {
        DPRINT1("Pipe URB %p length %u, expected %lu\n",
                Urb,
                Urb->UrbHeader.Length,
                (ULONG)sizeof(struct _URB_PIPE_REQUEST));
        Urb->UrbHeader.Status = USBD_STATUS_INVALID_URB_FUNCTION;
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

static
PVOID
NTAPI
HubPdoUrbBuffer(
    _In_ PVOID Pointer,
    _In_opt_ PMDL Mdl)
{
    if (Mdl != NULL)
        return MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority);

    return Pointer;
}

/* Language and the device descriptor index are ignored */
static
NTSTATUS
NTAPI
HubPdoCachedDescriptor(
    _In_ HubPdo* Pdo,
    _Inout_ PIRP Irp,
    _Inout_ PURB Urb)
{
    struct _URB_CONTROL_DESCRIPTOR_REQUEST* Request = &Urb->UrbControlDescriptorRequest;
    HubChild* Child = Pdo->m_Child;
    PVOID Source;
    PVOID Buffer;
    ULONG Size;
    NTSTATUS Status;

    if (Child->m_DeviceDescriptor.bcdUSB <= 0x0200)
    {
        HubPdoStampUrb(Pdo, Urb);
        return HubPdoToController(Pdo, Irp);
    }

    Status = HubPdoValidateUrb(Urb);
    if (!NT_SUCCESS(Status))
        return HubPdoCompleteIrp(Irp, Status);

    Buffer = HubPdoUrbBuffer(Request->TransferBuffer, Request->TransferBufferMDL);
    if (Buffer == NULL)
    {
        DPRINT1("Port %u descriptor URB %p has no buffer\n", Pdo->m_PortNumber, Urb);
        Urb->UrbHeader.Status = USBD_STATUS_INVALID_PARAMETER;
        return HubPdoCompleteIrp(Irp, STATUS_INVALID_PARAMETER);
    }

    if (Request->DescriptorType == USB_DEVICE_DESCRIPTOR_TYPE)
    {
        Source = &Child->m_DeviceDescriptor;
        Size = min(sizeof(Child->m_DeviceDescriptor), Request->TransferBufferLength);
    }
    else if (Request->DescriptorType == USB_CONFIGURATION_DESCRIPTOR_TYPE &&
             Request->Index == 0 &&
             Child->m_ConfigDescriptor != NULL)
    {
        Source = Child->m_ConfigDescriptor;
        Size = min((ULONG)Child->m_ConfigDescriptor->wTotalLength, Request->TransferBufferLength);
    }
    else
    {
        HubPdoStampUrb(Pdo, Urb);
        return HubPdoToController(Pdo, Irp);
    }

    /* Compat: a default pipe request reads back as a control transfer */
    Urb->UrbControlTransfer.TransferFlags |= USBD_TRANSFER_DIRECTION_IN;
    if (Urb->UrbControlTransfer.TransferFlags & USBD_DEFAULT_PIPE_TRANSFER)
        Urb->UrbHeader.Function = URB_FUNCTION_CONTROL_TRANSFER;

    RtlCopyMemory(Buffer, Source, Size);
    Request->TransferBufferLength = Size;
    return HubPdoCompleteIrp(Irp, STATUS_SUCCESS);
}

/* NOT_SUPPORTED answers leave the URB status alone */
static
NTSTATUS
NTAPI
HubPdoMsFeatureDescriptor(
    _In_ HubPdo* Pdo,
    _Inout_ PIRP Irp,
    _Inout_ PURB Urb)
{
    struct _URB_OS_FEATURE_DESCRIPTOR_REQUEST* Request = &Urb->UrbOSFeatureDescriptorRequest;
    HubChild* Child = Pdo->m_Child;
    USHORT BcdUsb = Child->m_DeviceDescriptor.bcdUSB;
    const HubMsOs20SetHeader* Set;
    PVOID Buffer;
    ULONG Size;

    if (Request->MS_FeatureDescriptorIndex == HUB_MSOS20_SET_INDEX)
    {
        Set = (const HubMsOs20SetHeader*)Child->m_MsOs20Set;
        if (Set == NULL)
        {
            DPRINT1("Port %u has no MS OS 2.0 descriptor set\n", Pdo->m_PortNumber);
            return HubPdoCompleteIrp(Irp, STATUS_NOT_SUPPORTED);
        }

        Buffer = HubPdoUrbBuffer(Request->TransferBuffer, Request->TransferBufferMDL);
        if (Buffer == NULL)
        {
            DPRINT1("Port %u MS OS 2.0 URB %p has no buffer\n", Pdo->m_PortNumber, Urb);
            return HubPdoCompleteIrp(Irp, STATUS_NOT_SUPPORTED);
        }

        Size = min((ULONG)Set->wTotalLength, Request->TransferBufferLength);
        RtlCopyMemory(Buffer, Set, Size);
        Request->TransferBufferLength = Size;
        return HubPdoCompleteIrp(Irp, STATUS_SUCCESS);
    }

    if (((BcdUsb >= 0x0200 || BcdUsb < 0x0100) && !Child->HasProperty(ChildProperty::MsOsNotSupported)) ||
        Child->HasHack(ChildHack::AlwaysQueryMsOs))
    {
        /* The device's own vendor code goes into the request */
        Urb->UrbControlVendorClassRequest.Request = Child->m_MsOsVendorCode;
        HubPdoStampUrb(Pdo, Urb);
        return HubPdoToController(Pdo, Irp);
    }

    DPRINT1("Port %u device does not support MS OS descriptors\n", Pdo->m_PortNumber);
    return HubPdoCompleteIrp(Irp, STATUS_NOT_SUPPORTED);
}

static
NTSTATUS
NTAPI
HubPdoSelectLikeComplete(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned)
        IoMarkIrpPending(Irp);

    if (NT_SUCCESS(Irp->IoStatus.Status))
        ((HubChild*)Context)->SetState(ChildState::ConfigurationValid);

    return STATUS_CONTINUE_COMPLETION;
}

/* Function 0x3B: payload not decoded, so it is forwarded and only tracked like a select */
static
NTSTATUS
NTAPI
HubPdoForwardSelectLike(
    _In_ HubPdo* Pdo,
    _Inout_ PIRP Irp,
    _Inout_ PURB Urb)
{
    if (Pdo->m_ControllerTarget == NULL)
    {
        DPRINT1("Port %u has no controller target for URB %p\n", Pdo->m_PortNumber, Urb);
        return HubPdoCompleteIrp(Irp, STATUS_NO_SUCH_DEVICE);
    }

    HubPdoStampUrb(Pdo, Urb);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, HubPdoSelectLikeComplete, Pdo->m_Child, TRUE, TRUE, TRUE);
    return IoCallDriver(Pdo->m_ControllerTarget, Irp);
}

/* Hubs select without waiting */
static
NTSTATUS
NTAPI
HubPdoRouteUrb(
    _In_ HubPdo* Pdo,
    _Inout_ PIRP Irp,
    _Inout_ PURB Urb)
{
    NTSTATUS Status;

    switch (Urb->UrbHeader.Function)
    {
        case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
        case URB_FUNCTION_ISOCH_TRANSFER:
        case HUB_URB_FUNCTION_FAST_PATH_1:
        case HUB_URB_FUNCTION_FAST_PATH_2:
            HubPdoStampUrb(Pdo, Urb);
            return HubPdoToController(Pdo, Irp);

        case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
            return HubPdoCachedDescriptor(Pdo, Irp, Urb);

        case URB_FUNCTION_SELECT_CONFIGURATION:
        case URB_FUNCTION_SELECT_INTERFACE:
        case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
        case URB_FUNCTION_SYNC_RESET_PIPE:
        case URB_FUNCTION_SYNC_CLEAR_STALL:
            Status = HubPdoValidateUrb(Urb);
            if (!NT_SUCCESS(Status))
                return HubPdoCompleteIrp(Irp, Status);

            if (Pdo->m_Child->HasProperty(ChildProperty::IsHub))
                return HubPdoQueueAsync(Pdo, Irp);

            return HubPdoQueueAndWait(Pdo, Irp, Pdo->m_Queue);

        case HUB_URB_FUNCTION_SELECT_LIKE:
            return HubPdoForwardSelectLike(Pdo, Irp, Urb);

        case URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR:
            return HubPdoMsFeatureDescriptor(Pdo, Irp, Urb);

        case URB_FUNCTION_OPEN_STATIC_STREAMS:
        case URB_FUNCTION_CLOSE_STATIC_STREAMS:
            HubPdoStampUrb(Pdo, Urb);
            return HubPdoQueueAndWait(Pdo, Irp, Pdo->m_Queue);

        case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
            if (Pdo->HasFlag(PdoFlag::FailGetStatus))
            {
                DPRINT1("Port %u GET_STATUS failed, the client said it is unsupported\n", Pdo->m_PortNumber);
                return HubPdoCompleteIrp(Irp, STATUS_NOT_SUPPORTED);
            }

            HubPdoStampUrb(Pdo, Urb);
            return HubPdoToController(Pdo, Irp);

        /* UCX validates everything else */
        default:
            HubPdoStampUrb(Pdo, Urb);
            return HubPdoToController(Pdo, Irp);
    }
}

/* Information may exceed the output buffer when the name does not fit */
static
NTSTATUS
NTAPI
HubPdoGetHubName(
    _In_ HubPdo* Pdo,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PUSB_HUB_NAME HubName = (PUSB_HUB_NAME)Irp->AssociatedIrp.SystemBuffer;
    ULONG OutputLength = Stack->Parameters.DeviceIoControl.OutputBufferLength;
    UNICODE_STRING Link;
    PWCHAR Name;
    ULONG Chars;
    ULONG Index;
    ULONG NameBytes = 0;

    if (HubName == NULL)
    {
        DPRINT1("Port %u hub name request has no buffer\n", Pdo->m_PortNumber);
        return HubPdoCompleteIrp(Irp, STATUS_INVALID_PARAMETER);
    }

    if (OutputLength < sizeof(*HubName))
    {
        DPRINT1("Port %u hub name buffer of %lu bytes too small\n", Pdo->m_PortNumber, OutputLength);
        return HubPdoCompleteIrp(Irp, STATUS_BUFFER_TOO_SMALL);
    }

    if (!Pdo->m_Child->HasProperty(ChildProperty::IsHub))
    {
        HubName->ActualLength = sizeof(*HubName);
        HubName->HubName[0] = UNICODE_NULL;
        Irp->IoStatus.Information = sizeof(*HubName);
        return HubPdoCompleteIrp(Irp, STATUS_SUCCESS);
    }

    RtlZeroMemory(HubName, OutputLength);
    HubPdoGetSymbolicLink(Pdo, &Link);

    /* Drop the leading "\xxx\" of the symbolic link */
    Name = Link.Buffer;
    Chars = Link.Length / sizeof(WCHAR);
    if (Chars != 0 && Name[0] == L'\\')
    {
        for (Index = 1; Index < Chars && Name[Index] != L'\\'; Index++)
            ;

        if (Index < Chars)
        {
            Name += Index + 1;
            NameBytes = (Chars - Index - 1) * sizeof(WCHAR);
        }
    }

    if (NameBytes != 0 && OutputLength >= NameBytes + sizeof(*HubName))
        RtlCopyMemory(HubName->HubName, Name, NameBytes);

    HubName->ActualLength = NameBytes + sizeof(*HubName);
    Irp->IoStatus.Information = HubName->ActualLength;
    return HubPdoCompleteIrp(Irp, STATUS_SUCCESS);
}

/* The client's data is copied without probing */
static
NTSTATUS
NTAPI
HubPdoRecordFailure(
    _In_ HubPdo* Pdo,
    _In_opt_ HubStartFailHeader* Data)
{
    HubStartFailHeader* Copy;
    ULONG Length;

    if (Pdo->m_StartFailData != NULL || Data == NULL)
    {
        DPRINT1("Port %u start failure not recorded, data %p, already %p\n",
                Pdo->m_PortNumber,
                Data,
                Pdo->m_StartFailData);
        return STATUS_INVALID_PARAMETER;
    }

    if (Data->LengthInBytes > HUB_START_FAIL_MAX)
        DPRINT1("Port %u start failure data of %lu bytes cut down\n", Pdo->m_PortNumber, Data->LengthInBytes);

    Length = min(Data->LengthInBytes, (ULONG)HUB_START_FAIL_MAX);
    if (Length < sizeof(*Data))
    {
        DPRINT1("Port %u start failure data of %lu bytes too small\n", Pdo->m_PortNumber, Length);
        return STATUS_BUFFER_TOO_SMALL;
    }

    Copy = (HubStartFailHeader*)ExAllocatePoolWithTag(NonPagedPool, Length, HUB_TAG_DEVICE);
    if (Copy == NULL)
    {
        DPRINT1("No memory for %lu bytes of port %u start failure data\n", Length, Pdo->m_PortNumber);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(Copy, Data, Length);
    Pdo->m_StartFailData = Copy;
    Pdo->m_Child->m_Port->m_ConnectionStatus = (USB_CONNECTION_STATUS)Copy->ConnectStatus;
    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubPdoDeviceConfigInfo(
    _In_ HubPdo* Pdo,
    _Inout_opt_ PHUB_DEVICE_CONFIG_INFO Info)
{
    HubChild* Child = Pdo->m_Child;
    HubFdo* Hub = Pdo->m_Hub;
    USB_ID_STRING FriendlyName;
    NTSTATUS Status;

    if (Info == NULL || Info->Version != 1)
    {
        DPRINT1("Port %u config info %p has a bad version\n", Pdo->m_PortNumber, Info);
        return STATUS_INVALID_PARAMETER;
    }

    if (Info->Length != sizeof(*Info))
    {
        DPRINT1("Port %u config info length %lu, expected %lu\n",
                Pdo->m_PortNumber,
                Info->Length,
                (ULONG)sizeof(*Info));
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlZeroMemory(Info, sizeof(*Info));
    Info->Version = 1;
    Info->Length = sizeof(*Info);

    if ((Hub->m_ParentInfo.Flags & HUB_PARENT_HIGH_SPEED_CAPABLE) ||
        Hub->m_ParentInfo.DeviceDescriptor.bcdUSB >= 0x0200)
    {
        Info->HubFlags.HubIsHighSpeedCapable = 1;
    }

    if (Hub->HasFlag(HubFlag::MultiTtHub))
    {
        Info->HubFlags.HubIsMultiTt = 1;
        Info->HubFlags.HubIsMultiTtCapable = 1;
    }

    if (Hub->m_Parent.HubSpeed == UsbHighSpeed)
        Info->HubFlags.HubIsHighSpeed = 1;
    if (Hub->HasFlag(HubFlag::WakeOnConnect))
        Info->HubFlags.HubIsArmedWakeOnConnect = 1;
    if (Hub->IsRootHub())
        Info->HubFlags.HubIsRoot = 1;
    if (Hub->m_MaxPortPower == 100)
        Info->HubFlags.HubIsBusPowered = 1;

    /* Reserved[18] bit 0 says the UXD settings below are valid */
    if (Child->HasProperty(ChildProperty::UxdReserved))
    {
        Info->Reserved[18] = 1;
        RtlCopyMemory(&Info->UxdSettings, &Child->m_Uxd, sizeof(Info->UxdSettings));
    }

    Status = HubIdBuildCompatibleIds(Child, NULL, &Info->CompatibleIds);
    if (NT_SUCCESS(Status))
        Status = HubIdBuildHardwareIds(Child, NULL, &Info->HardwareIds);

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(&FriendlyName, sizeof(FriendlyName));
        FriendlyName.LengthInBytes = Child->m_FriendlyNameLength;
        FriendlyName.Buffer = Child->m_FriendlyName;
        Status = HubIdListCopy(&Info->DeviceDescription, &FriendlyName);
    }

    if (!NT_SUCCESS(Status))
    {
        HubIdListFree(&Info->CompatibleIds);
        HubIdListFree(&Info->HardwareIds);
        HubIdListFree(&Info->DeviceDescription);
    }

    return Status;
}

static
NTSTATUS
NTAPI
HubPdoTopologyAddress(
    _In_ HubPdo* Pdo,
    _Out_opt_ PUSB_TOPOLOGY_ADDRESS Address)
{
    HubFdo* Hub = Pdo->m_Hub;
    UCHAR Depth = Hub->m_Parent.HubDepth;

    if (Address == NULL)
    {
        DPRINT1("Port %u topology address request has no buffer\n", Pdo->m_PortNumber);
        return STATUS_INVALID_PARAMETER;
    }

    *Address = Hub->m_Parent.HubTopologyAddress;

    if (Depth == 0)
        Address->RootHubPortNumber = Pdo->m_PortNumber;
    else if (Depth - 1 < (UCHAR)RTL_NUMBER_OF(Address->HubPortNumber))
        Address->HubPortNumber[Depth - 1] = Pdo->m_PortNumber;

    return STATUS_SUCCESS;
}

/* Fields not written stay as the child hub set them; HubFlags is OR'ed into */
static
VOID
NTAPI
HubPdoFillHubInfo(
    _In_ HubPdo* Pdo,
    _Inout_ HubParentInfo* Info)
{
    HubChild* Child = Pdo->m_Child;
    HubFdo* Hub = Pdo->m_Hub;
    const HubParentInfo* Mine = &Hub->m_ParentInfo;

    Info->RootHubPdo = Mine->RootHubPdo;
    Info->DeviceDescriptor = Child->m_DeviceDescriptor;
    Info->U1ExitLatency = Child->m_U1ExitLatency;
    Info->U2ExitLatency = Child->m_U2ExitLatency;
    Info->SublinkSpeedAttr = Child->m_SublinkSpeedAttr;
    Info->SublinkSpeedAttrCount = Child->m_SublinkSpeedAttrCount;
    Info->TotalHubDepth = (UCHAR)Child->m_Port->m_Info.TotalHubDepth;
    Info->HostInitiatedU1ExitLatency = Child->m_HostU1ExitLatency;
    Info->HostInitiatedU2ExitLatency = Child->m_HostU2ExitLatency;
    Info->TotalTpPropagationDelay = Mine->TotalTpPropagationDelay +
                                    Hub->m_HubDescriptor.Usb30.wHubDelay +
                                    Child->m_TxTpDelay;

    if (Child->m_SlowestLinkU1)
    {
        Info->SlowestLinkU1ExitLatency = Child->m_Sel.U1Pel;
        Info->SlowestLinkU1Depth = Hub->m_Parent.HubDepth;
    }
    else
    {
        Info->SlowestLinkU1ExitLatency = Mine->SlowestLinkU1ExitLatency;
        Info->SlowestLinkU1Depth = Mine->SlowestLinkU1Depth;
    }

    if (Child->m_SlowestLinkU2)
    {
        Info->SlowestLinkU2ExitLatency = Child->m_Sel.U2Pel;
        Info->SlowestLinkU2Depth = Hub->m_Parent.HubDepth;
    }
    else
    {
        Info->SlowestLinkU2ExitLatency = Mine->SlowestLinkU2ExitLatency;
        Info->SlowestLinkU2Depth = Mine->SlowestLinkU2Depth;
    }

    if (Mine->Flags & HUB_PARENT_DISABLE_LPM)
        Info->Flags |= HUB_PARENT_DISABLE_LPM;
    if (Child->HasProperty(ChildProperty::HighSpeedCapable))
        Info->Flags |= HUB_PARENT_HIGH_SPEED_CAPABLE;
}

static
NTSTATUS
NTAPI
HubPdoQueryCapability(
    _In_ HubPdo* Pdo,
    _Inout_ PIRP Irp,
    _Inout_ PUCXHUB_QUERY_CAPABILITY Query)
{
    ULONG Kind = Pdo->m_Child->m_Kind;
    const GUID* Type = &Query->CapabilityType;
    BOOLEAN Supported;

    Query->ResultLength = 0;

    DPRINT("Port %u capability query {%08lx-...}\n", Pdo->m_PortNumber, Type->Data1);

    if (RtlEqualMemory(Type, &GUID_USB_CAPABILITY_FUNCTION_SUSPEND, sizeof(GUID)))
        return HubPdoCompleteIrp(Irp, (Kind & DSM_KIND_PORT20) ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS);

    if (RtlEqualMemory(Type, &GUID_USB_CAPABILITY_STATIC_STREAMS, sizeof(GUID)))
    {
        if (Kind & DSM_KIND_PORT20)
            return HubPdoCompleteIrp(Irp, STATUS_NOT_SUPPORTED);

        return HubPdoToController(Pdo, Irp);
    }

    /* Answered here, without output */
    if (RtlEqualMemory(Type, &GUID_USB_CAPABILITY_SSP_ISOCH_PIPE_FLAGS, sizeof(GUID)))
        return HubPdoCompleteIrp(Irp, STATUS_SUCCESS);

    if (RtlEqualMemory(Type, &GUID_USB_CAPABILITY_DEVICE_CONNECTION_SUPER_SPEED_COMPATIBLE, sizeof(GUID)))
        Supported = (Kind & DSM_KIND_SUPER_SPEED) != 0;
    else if (RtlEqualMemory(Type, &GUID_USB_CAPABILITY_DEVICE_CONNECTION_HIGH_SPEED_COMPATIBLE, sizeof(GUID)))
        Supported = (Kind & (DSM_KIND_SUPER_SPEED | DSM_KIND_HIGH_SPEED)) != 0;
    else if (RtlEqualMemory(Type, &HubCapFullSpeedCompatible, sizeof(GUID)))
        Supported = (Kind & (DSM_KIND_SUPER_SPEED | DSM_KIND_HIGH_SPEED | DSM_KIND_FULL_SPEED)) != 0;
    else if (RtlEqualMemory(Type, &HubCapLowSpeedCompatible, sizeof(GUID)))
        Supported = TRUE;
    else
        return HubPdoToController(Pdo, Irp);

    return HubPdoCompleteIrp(Irp, Supported ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED);
}

/* The one cycle a PDO gets is never given back */
static
NTSTATUS
NTAPI
HubPdoCyclePort(
    _In_ HubPdo* Pdo,
    _Inout_ PIRP Irp)
{
    if (KeGetCurrentIrql() > PASSIVE_LEVEL)
    {
        DPRINT1("Port %u cycle request above passive level\n", Pdo->m_PortNumber);
        return HubPdoCompleteIrp(Irp, STATUS_INVALID_PARAMETER);
    }

    if (InterlockedCompareExchange(&Pdo->m_CycleQueued, 1, 0) == 0)
    {
        DPRINT1("Client cycles port %u\n", Pdo->m_PortNumber);
        Pdo->m_Child->Post(DsmEvent::ClientCyclePort);
    }
    else
    {
        DPRINT1("Port %u cycle ignored, one was already requested\n", Pdo->m_PortNumber);
    }

    return HubPdoCompleteIrp(Irp, STATUS_SUCCESS);
}

/* Unknown codes keep the status the IRP carries (2.0 stack compat) */
static
NTSTATUS
NTAPI
HubPdoDispatchIoctl(
    _In_ HubPdo* Pdo,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PVOID Argument1 = Stack->Parameters.Others.Argument1;
    HubChild* Child = Pdo->m_Child;
    PREGISTER_COMPOSITE_DEVICE Composite;
    PUCXHUB_DUMP_DEVICE_INFO Dump;
    PULONG PortStatus;
    ULONG Code = Stack->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS Status;

    switch (Code)
    {
        case IOCTL_INTERNAL_USB_RESET_PORT:
            if (!Pdo->HasFlag(PdoFlag::InD0))
            {
                DPRINT1("Port %u reset refused, the device is not in D0\n", Pdo->m_PortNumber);
                return HubPdoCompleteIrp(Irp, STATUS_POWER_STATE_INVALID);
            }
            if (KeGetCurrentIrql() > PASSIVE_LEVEL)
            {
                DPRINT1("Port %u reset requested above passive level\n", Pdo->m_PortNumber);
                return HubPdoCompleteIrp(Irp, STATUS_INVALID_PARAMETER);
            }
            return HubPdoQueueAndWait(Pdo, Irp, Pdo->m_Queue);

        case IOCTL_UCXHUB_RESET_PORT_ASYNC:
            if (!Pdo->HasFlag(PdoFlag::InD0))
            {
                DPRINT1("Port %u async reset refused, the device is not in D0\n", Pdo->m_PortNumber);
                return HubPdoCompleteIrp(Irp, STATUS_POWER_STATE_INVALID);
            }
            if (KeGetCurrentIrql() > DISPATCH_LEVEL)
            {
                DPRINT1("Port %u async reset requested above dispatch level\n", Pdo->m_PortNumber);
                return HubPdoCompleteIrp(Irp, STATUS_INVALID_PARAMETER);
            }
            return HubPdoQueueAsync(Pdo, Irp);

        case IOCTL_INTERNAL_USB_GET_PORT_STATUS:
            PortStatus = (PULONG)Argument1;
            if (KeGetCurrentIrql() > PASSIVE_LEVEL || PortStatus == NULL)
            {
                DPRINT1("Port %u status request refused, buffer %p\n", Pdo->m_PortNumber, PortStatus);
                return HubPdoCompleteIrp(Irp, STATUS_INVALID_PARAMETER);
            }

            /* The hub machine reads the port and clears ReportPortDisabled through Argument2 */
            *PortStatus = 0;
            Stack->Parameters.Others.Argument2 = Pdo;
            return HubPdoQueueAndWait(Pdo, Irp, WdfDeviceGetDefaultQueue(Pdo->m_Hub->m_Device));

        case IOCTL_INTERNAL_USB_ENABLE_PORT:
        case IOCTL_INTERNAL_USB_GET_HUB_COUNT:
            return HubPdoCompleteIrp(Irp, STATUS_SUCCESS);

        case IOCTL_INTERNAL_USB_CYCLE_PORT:
            return HubPdoCyclePort(Pdo, Irp);

        case IOCTL_INTERNAL_USB_GET_HUB_NAME:
            return HubPdoGetHubName(Pdo, Irp);

        case IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION:
            return HubPdoSubmitIdleRequest(Pdo, Irp);

        case IOCTL_INTERNAL_USB_RECORD_FAILURE:
            return HubPdoCompleteIrp(Irp, HubPdoRecordFailure(Pdo, (HubStartFailHeader*)Argument1));

        case IOCTL_INTERNAL_USB_GET_BUS_INFO:
            return HubPdoCompleteIrp(Irp, HubQueryDeviceBusInfo(Child, (PUSB_BUS_NOTIFICATION)Argument1));

        case IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME:
            return HubPdoCompleteIrp(Irp, HubQueryControllerName(Pdo->m_Hub,
                                                                 (PUSB_HUB_NAME)Argument1,
                                                                 (ULONG)(ULONG_PTR)Stack->Parameters.Others.Argument2));

        case IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE:
            if (Argument1 == NULL)
            {
                DPRINT1("Port %u device handle request has no buffer\n", Pdo->m_PortNumber);
                return HubPdoCompleteIrp(Irp, STATUS_INVALID_PARAMETER);
            }
            return HubPdoCompleteIrp(Irp, STATUS_SUCCESS);

        case IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO:
        case IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO:
        case IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE_EX:
        case IOCTL_INTERNAL_USB_GET_TT_DEVICE_HANDLE:
        case IOCTL_INTERNAL_USB_NOTIFY_IDLE_READY:
            DPRINT1("Port %u internal IOCTL 0x%lx is not supported\n", Pdo->m_PortNumber, Code);
            return HubPdoCompleteIrp(Irp, STATUS_NOT_SUPPORTED);

        case IOCTL_INTERNAL_USB_GET_TOPOLOGY_ADDRESS:
            return HubPdoCompleteIrp(Irp, HubPdoTopologyAddress(Pdo, (PUSB_TOPOLOGY_ADDRESS)Argument1));

        case IOCTL_INTERNAL_USB_GET_DEVICE_CONFIG_INFO:
            return HubPdoCompleteIrp(Irp, HubPdoDeviceConfigInfo(Pdo, (PHUB_DEVICE_CONFIG_INFO)Argument1));

        case IOCTL_INTERNAL_USB_REGISTER_COMPOSITE_DEVICE:
            Composite = (PREGISTER_COMPOSITE_DEVICE)Argument1;
            Stack->Parameters.Others.Argument2 = Child->m_UsbDevice;
            if (Composite != NULL && Composite->CapabilityFlags.CapabilityFunctionSuspend)
                Pdo->SetFlag(PdoFlag::ClientDoesFunctionSuspend);
            return HubPdoToController(Pdo, Irp);

        case IOCTL_INTERNAL_USB_UNREGISTER_COMPOSITE_DEVICE:
        case IOCTL_INTERNAL_USB_REQUEST_REMOTE_WAKE_NOTIFICATION:
        case IOCTL_UCXHUB_SET_FUNCTION_HANDLE_DATA:
            Stack->Parameters.Others.Argument2 = Child->m_UsbDevice;
            return HubPdoToController(Pdo, Irp);

        case IOCTL_UCXHUB_GET_HUB_INFO:
            HubPdoFillHubInfo(Pdo, (HubParentInfo*)Argument1);
            return HubPdoCompleteIrp(Irp, STATUS_SUCCESS);

        case IOCTL_UCXHUB_QUERY_USB_CAPABILITY:
            return HubPdoQueryCapability(Pdo, Irp, (PUCXHUB_QUERY_CAPABILITY)Argument1);

        case IOCTL_UCXHUB_GET_DUMP_DATA:
            Dump = (PUCXHUB_DUMP_DEVICE_INFO)Irp->AssociatedIrp.SystemBuffer;
            Dump->Device = Child->m_UsbDevice;
            return HubPdoToController(Pdo, Irp);

        case IOCTL_UCXHUB_FREE_DUMP_DATA:
            return HubPdoToController(Pdo, Irp);

        case IOCTL_UCXHUB_NOTIFY_FORWARD_PROGRESS:
            ((PUCXHUB_FORWARD_PROGRESS_INFO)Argument1)->Device = Child->m_UsbDevice;
            return HubPdoToController(Pdo, Irp);

        default:
            Status = Irp->IoStatus.Status;
            if (!NT_SUCCESS(Status))
                DPRINT1("Port %u unknown internal IOCTL 0x%lx completed 0x%lx\n", Pdo->m_PortNumber, Code, Status);
            return HubPdoCompleteAsIs(Irp);
    }
}

/* Runs at the caller's IRQL, up to DISPATCH_LEVEL */
static
NTSTATUS
NTAPI
HubPdoPreprocessInternalIoctl(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp)
{
    HubPdo* Pdo = HubGetPdoContext(Device);
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG Code = Stack->Parameters.DeviceIoControl.IoControlCode;
    PURB Urb = NULL;
    BOOLEAN Exempt = FALSE;

    switch (Code)
    {
        case IOCTL_INTERNAL_USB_SUBMIT_URB:
            Urb = (PURB)Stack->Parameters.Others.Argument1;
            Exempt = Urb->UrbHeader.Function == URB_FUNCTION_OPEN_STATIC_STREAMS ||
                     Urb->UrbHeader.Function == URB_FUNCTION_CLOSE_STATIC_STREAMS;
            break;

        case IOCTL_INTERNAL_USB_UNREGISTER_COMPOSITE_DEVICE:
        case IOCTL_UCXHUB_QUERY_USB_CAPABILITY:
            Exempt = TRUE;
            break;

        case IOCTL_UCXHUB_RESET_PORT_ASYNC:
            Exempt = Pdo->HasFlag(PdoFlag::InBootPath);
            break;

        case IOCTL_INTERNAL_USB_FAIL_GET_STATUS_FROM_DEVICE:
            DPRINT("Port %u client says GET_STATUS is unsupported\n", Pdo->m_PortNumber);
            Pdo->SetFlag(PdoFlag::FailGetStatus);
            return HubPdoCompleteIrp(Irp, STATUS_SUCCESS);

        default:
            break;
    }

    /* XP compat: a failed URB carries an invalid parameter status */
    if (Pdo->m_FailRequests && !Exempt)
    {
        DPRINT1("Port %u IRP %p IOCTL 0x%lx failed, the device is not present\n", Pdo->m_PortNumber, Irp, Code);

        if (Urb != NULL)
            Urb->UrbHeader.Status = USBD_STATUS_INVALID_PARAMETER;

        return HubPdoCompleteIrp(Irp, STATUS_NO_SUCH_DEVICE);
    }

    if (Urb != NULL)
        return HubPdoRouteUrb(Pdo, Irp, Urb);

    return HubPdoDispatchIoctl(Pdo, Irp);
}

/* Device IOCTLs answered by UCX, which expects the device handle stamped in */
static
NTSTATUS
NTAPI
HubPdoStampAndForward(
    _In_ HubPdo* Pdo,
    _Inout_ PIRP Irp,
    _In_ ULONG Size)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PUCHAR Buffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
    UCXUSBDEVICE UsbDevice = Pdo->m_Child->m_UsbDevice;

    if (Stack->Parameters.DeviceIoControl.InputBufferLength != Size ||
        Stack->Parameters.DeviceIoControl.OutputBufferLength != Size ||
        Buffer == NULL)
    {
        DPRINT1("Port %u IOCTL 0x%lx buffers %lu/%lu, expected %lu\n",
                Pdo->m_PortNumber,
                Stack->Parameters.DeviceIoControl.IoControlCode,
                Stack->Parameters.DeviceIoControl.InputBufferLength,
                Stack->Parameters.DeviceIoControl.OutputBufferLength,
                Size);
        return HubPdoCompleteIrp(Irp, STATUS_INVALID_PARAMETER);
    }

    RtlCopyMemory(Buffer + HUB_STAMPED_HANDLE_OFFSET, &UsbDevice, sizeof(UsbDevice));
    return HubPdoToController(Pdo, Irp);
}

static
NTSTATUS
NTAPI
HubPdoPreprocessIoctl(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp)
{
    HubPdo* Pdo = HubGetPdoContext(Device);
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);

    switch (Stack->Parameters.DeviceIoControl.IoControlCode)
    {
        case HUB_IOCTL_GET_TRANSPORT_CHARACTERISTICS:
        case HUB_IOCTL_NOTIFY_TRANSPORT_CHANGE:
        case HUB_IOCTL_UNREGISTER_TRANSPORT_CHANGE:
        case HUB_IOCTL_START_TIME_SYNC:
        case HUB_IOCTL_GET_TIME_SYNC:
        case HUB_IOCTL_STOP_TIME_SYNC:
            return HubPdoToController(Pdo, Irp);

        case HUB_IOCTL_REGISTER_TRANSPORT_CHANGE:
            return HubPdoStampAndForward(Pdo, Irp, HUB_TRANSPORT_REGISTRATION_SIZE);

        case HUB_IOCTL_GET_DEVICE_CHARACTERISTICS:
            return HubPdoStampAndForward(Pdo, Irp, HUB_DEVICE_CHARACTERISTICS_SIZE);

        default:
            return HubPdoPassDown(Device, Irp);
    }
}

/* Default queue: the one request the device machine works on */

/* A request that maps to no event fails; only a filter sending straight to the queue gets it here */
static
VOID
NTAPI
HubPdoEvtIoInternalDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    HubPdo* Pdo = HubGetPdoContext(WdfIoQueueGetDevice(Queue));
    HubChild* Child = Pdo->m_Child;
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(WdfRequestWdmGetIrp(Request));
    PURB Urb;
    DsmEvent Event = DsmEvent::Count;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    switch (IoControlCode)
    {
        case IOCTL_INTERNAL_USB_SUBMIT_URB:
            Urb = (PURB)Stack->Parameters.Others.Argument1;

            switch (Urb->UrbHeader.Function)
            {
                /* The device handle goes in; an unconfigure drops the handle */
                case URB_FUNCTION_SELECT_CONFIGURATION:
                    Urb->UrbHeader.UsbdDeviceHandle = Child->m_UsbDevice;
                    if (Urb->UrbSelectConfiguration.ConfigurationDescriptor == NULL)
                    {
                        Urb->UrbSelectConfiguration.ConfigurationHandle = NULL;
                        Event = DsmEvent::ClientUnconfigure;
                    }
                    else
                    {
                        Event = DsmEvent::ClientSelectConfig;
                    }
                    break;

                case URB_FUNCTION_SELECT_INTERFACE:
                    Event = DsmEvent::ClientSetInterface;
                    break;

                case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
                    DPRINT1("Client resets pipe %p on port %u\n", Urb->UrbPipeRequest.PipeHandle, Pdo->m_PortNumber);
                    Event = DsmEvent::ClientResetPipe;
                    break;

                case URB_FUNCTION_SYNC_RESET_PIPE:
                    DPRINT1("Client sync resets pipe %p on port %u\n", Urb->UrbPipeRequest.PipeHandle, Pdo->m_PortNumber);
                    Event = DsmEvent::ClientSyncResetPipe;
                    break;

                case URB_FUNCTION_SYNC_CLEAR_STALL:
                    DPRINT1("Client clears stall on pipe %p on port %u\n", Urb->UrbPipeRequest.PipeHandle, Pdo->m_PortNumber);
                    Event = DsmEvent::ClientClearStall;
                    break;

                case URB_FUNCTION_OPEN_STATIC_STREAMS:
                case URB_FUNCTION_CLOSE_STATIC_STREAMS:
                    Event = DsmEvent::ClientStreams;
                    break;

                default:
                    break;
            }
            break;

        case IOCTL_INTERNAL_USB_RESET_PORT:
        case IOCTL_UCXHUB_RESET_PORT_ASYNC:
            DPRINT1("Client reset of port %u starts\n", Pdo->m_PortNumber);
            Child->m_Port->m_ConnectionStatus = DeviceReset;
            Event = DsmEvent::ClientResetDevice;
            break;

        default:
            break;
    }

    if (Event == DsmEvent::Count)
    {
        DPRINT1("Port %u queue got IOCTL 0x%lx it cannot handle\n", Pdo->m_PortNumber, IoControlCode);
        WdfRequestComplete(Request, STATUS_NOT_SUPPORTED);
        return;
    }

    DPRINT("Port %u client request %p, IOCTL 0x%lx\n", Pdo->m_PortNumber, Request, IoControlCode);
    Child->m_ClientRequest = Request;
    Child->Post(Event);
}

/* Everything else keeps the status the request carries (2.0 compat) */
static
VOID
NTAPI
HubPdoEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    PIRP Irp = WdfRequestWdmGetIrp(Request);

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    switch (IoControlCode)
    {
        case HUB_IOCTL_MEDIA_SERIAL_NUMBER:
            DPRINT("Media serial number request on a USB PDO is not supported\n");
            WdfRequestComplete(Request, STATUS_NOT_SUPPORTED);
            break;

        default:
            DPRINT("PDO IOCTL 0x%lx completed 0x%lx\n", IoControlCode, Irp->IoStatus.Status);
            WdfRequestComplete(Request, Irp->IoStatus.Status);
            break;
    }
}

/* Out of bandwidth notice, a second after UCX ran out */
VOID
NTAPI
HubEvtBandwidthRetryTimer(
    _In_ WDFTIMER Timer)
{
    HubPort* Port = HubPort::FromObject(WdfTimerGetParentObject(Timer));
    HubChild* Child = Port->m_Child;

    /* The timer outlives a device that left the port; only the current one reports */
    if (Child == NULL || Child->m_BandwidthRetryTimer != Timer)
        return;

    DPRINT1("Port %u ran out of bandwidth\n", Port->Number());
    HubWmiNotifyInsufficientBandwidth(Port->m_Hub, Port->Number());
}

/* UCX calls this only while a transfer of the device is pending, so the device is alive */
VOID
NTAPI
HubNoPingResponse(
    _In_ UCXHUB_HUB_CONTEXT HubContext,
    _In_ UCXHUB_DEVICE_CONTEXT DeviceContext)
{
    UNREFERENCED_PARAMETER(HubContext);

    DPRINT1("Device %p did not answer a ping\n", DeviceContext);
    ((HubChild*)DeviceContext)->Post(DsmEvent::NoPingResponse);
}

/* PDO creation */

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubPdoAssignPreprocess(
    _In_ PWDFDEVICE_INIT Init)
{
    static UCHAR PnpMinors[] =
    {
        IRP_MN_QUERY_INTERFACE,
        IRP_MN_QUERY_DEVICE_TEXT,
        IRP_MN_REMOVE_DEVICE,
        IRP_MN_DEVICE_ENUMERATED,
        IRP_MN_START_DEVICE,
        IRP_MN_SURPRISE_REMOVAL,
        IRP_MN_QUERY_RESOURCE_REQUIREMENTS,
        IRP_MN_QUERY_PNP_DEVICE_STATE,
        IRP_MN_QUERY_ID
    };
    static UCHAR PowerMinors[] = { IRP_MN_SET_POWER };
    NTSTATUS Status;

    PAGED_CODE();

    Status = WdfDeviceInitAssignWdmIrpPreprocessCallback(Init,
                                                         HubPdoPreprocessInternalIoctl,
                                                         IRP_MJ_INTERNAL_DEVICE_CONTROL,
                                                         NULL,
                                                         0);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = WdfDeviceInitAssignWdmIrpPreprocessCallback(Init,
                                                         HubPdoPreprocessPnp,
                                                         IRP_MJ_PNP,
                                                         PnpMinors,
                                                         RTL_NUMBER_OF(PnpMinors));
    if (!NT_SUCCESS(Status))
        return Status;

    Status = WdfDeviceInitAssignWdmIrpPreprocessCallback(Init,
                                                         HubPdoPreprocessSetPower,
                                                         IRP_MJ_POWER,
                                                         PowerMinors,
                                                         RTL_NUMBER_OF(PowerMinors));
    if (!NT_SUCCESS(Status))
        return Status;

    /* Device IOCTLs that go to UCX */
    return WdfDeviceInitAssignWdmIrpPreprocessCallback(Init,
                                                       HubPdoPreprocessIoctl,
                                                       IRP_MJ_DEVICE_CONTROL,
                                                       NULL,
                                                       0);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoSetCallbacks(
    _In_ PWDFDEVICE_INIT Init)
{
    WDF_PNPPOWER_EVENT_CALLBACKS PnpPower;
    WDF_PDO_EVENT_CALLBACKS PdoEvents;

    PAGED_CODE();

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPower);
    PnpPower.EvtDeviceD0Entry = HubPdoEvtD0Entry;
    PnpPower.EvtDeviceD0Exit = HubPdoEvtD0Exit;
    PnpPower.EvtDevicePrepareHardware = HubPdoEvtPrepareHardware;
    PnpPower.EvtDeviceReleaseHardware = HubPdoEvtReleaseHardware;
    PnpPower.EvtDeviceSurpriseRemoval = HubPdoEvtSurpriseRemoval;
    PnpPower.EvtDeviceSelfManagedIoInit = HubPdoEvtSelfManagedIoStart;
    PnpPower.EvtDeviceSelfManagedIoRestart = HubPdoEvtSelfManagedIoStart;
    PnpPower.EvtDeviceSelfManagedIoSuspend = HubPdoEvtSelfManagedIoSuspend;
    PnpPower.EvtDeviceUsageNotificationEx = HubPdoEvtUsageNotificationEx;
    PnpPower.EvtDeviceQueryStop = HubPdoEvtQueryStop;
    PnpPower.EvtDeviceQueryRemove = HubPdoEvtQueryRemove;
    WdfDeviceInitSetPnpPowerEventCallbacks(Init, &PnpPower);

    WDF_PDO_EVENT_CALLBACKS_INIT(&PdoEvents);
    PdoEvents.EvtDeviceEnableWakeAtBus = HubPdoEvtEnableWakeAtBus;
    PdoEvents.EvtDeviceDisableWakeAtBus = HubPdoEvtDisableWakeAtBus;
    PdoEvents.EvtDeviceReportedMissing = HubPdoEvtReportedMissing;
    PdoEvents.EvtDeviceResourceRequirementsQuery = HubPdoEvtResourceRequirementsQuery;
    WdfPdoInitSetEventCallbacks(Init, &PdoEvents);
}

/* Steps that hand the device a WDFDEVICE name; collisions just try the next number */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubPdoCreateNamed(
    _Inout_ PWDFDEVICE_INIT* Init,
    _Out_ WDFDEVICE* Device)
{
    DECLARE_CONST_UNICODE_STRING(Sddl, L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGW;;;WD)(A;;GR;;;RC)");
    WDF_OBJECT_ATTRIBUTES Attributes;
    WCHAR Buffer[32];
    UNICODE_STRING Name;
    NTSTATUS Status;
    ULONG Index;

    PAGED_CODE();

    Status = WdfDeviceInitAssignSDDLString(*Init, &Sddl);
    if (!NT_SUCCESS(Status))
        return Status;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, HubPdo);
    Attributes.EvtCleanupCallback = HubPdoEvtCleanup;

    for (Index = 0;; Index++)
    {
        Status = RtlStringCchPrintfW(Buffer, RTL_NUMBER_OF(Buffer), L"\\Device\\USBPDO-%d", Index);
        if (!NT_SUCCESS(Status))
            return Status;

        RtlInitUnicodeString(&Name, Buffer);
        Status = WdfDeviceInitAssignName(*Init, &Name);
        if (!NT_SUCCESS(Status))
            return Status;

        Status = WdfDeviceCreate(Init, &Attributes, Device);
        if (Status != STATUS_OBJECT_NAME_COLLISION)
            return Status;

        DPRINT("PDO name %wZ is taken, trying the next\n", &Name);
    }
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubPdoAddInterfaces(
    _In_ HubPdo* Pdo)
{
    UCXHUB_PARENT_INTERFACE Parent;
    PNP_LOCATION_INTERFACE Location;
    WDF_QUERY_INTERFACE_CONFIG Config;
    HubChild* Child = Pdo->m_Child;
    NTSTATUS Status;

    PAGED_CODE();

    if (Child->HasProperty(ChildProperty::IsHub))
    {
        RtlZeroMemory(&Parent, sizeof(Parent));
        Parent.Header.Size = sizeof(Parent);
        Parent.Header.Version = UCXHUB_PARENT_INTERFACE_VERSION;
        Parent.Header.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
        Parent.Header.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;
        Parent.HubDepth = Pdo->m_Hub->m_Parent.HubDepth + 1;
        Parent.HubSpeed = Child->Speed();
        Parent.IsEnhancedSuperSpeed = HubPdoIsEnhancedSuperSpeed(Child);
        Parent.Hub = Child->m_UsbDevice;

        WDF_QUERY_INTERFACE_CONFIG_INIT(&Config,
                                        (PINTERFACE)&Parent,
                                        &GUID_UCXHUB_PARENT_INTERFACE,
                                        HubPdoEvtParentInterfaceQuery);
        Config.ImportInterface = TRUE;

        Status = WdfDeviceAddQueryInterface(Pdo->m_Device, &Config);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    RtlZeroMemory(&Location, sizeof(Location));
    Location.Size = sizeof(Location);
    Location.Version = PNP_LOCATION_INTERFACE_VERSION;
    Location.Context = Child;
    Location.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
    Location.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;
    Location.GetLocationString = HubPdoGetLocationString;

    WDF_QUERY_INTERFACE_CONFIG_INIT(&Config, (PINTERFACE)&Location, &GUID_PNP_LOCATION_INTERFACE, NULL);
    return WdfDeviceAddQueryInterface(Pdo->m_Device, &Config);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubPdoCreateQueue(
    _In_ HubPdo* Pdo)
{
    WDF_IO_QUEUE_CONFIG Config;

    PAGED_CODE();

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&Config, WdfIoQueueDispatchSequential);
    Config.PowerManaged = WdfFalse;
    Config.EvtIoInternalDeviceControl = HubPdoEvtIoInternalDeviceControl;
    Config.EvtIoDeviceControl = HubPdoEvtIoDeviceControl;

    return WdfIoQueueCreate(Pdo->m_Device, &Config, WDF_NO_OBJECT_ATTRIBUTES, &Pdo->m_Queue);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoSetPnpCapabilities(
    _In_ HubPdo* Pdo)
{
    WDF_DEVICE_PNP_CAPABILITIES Caps;
    HubChild* Child = Pdo->m_Child;

    PAGED_CODE();

    WDF_DEVICE_PNP_CAPABILITIES_INIT(&Caps);
    Caps.Removable = Child->HasProperty(ChildProperty::NotRemovable) ? WdfFalse : WdfTrue;
    Caps.UniqueID = (Child->m_SerialNumber != NULL) ? WdfTrue : WdfFalse;
    Caps.SurpriseRemovalOK = WdfFalse;
    Caps.Address = Pdo->m_PortNumber;
    Caps.UINumber = 0xFFFFFFFF;
    WdfDeviceSetPnpCapabilities(Pdo->m_Device, &Caps);
}

/* Hubs always claim wake to pass on their children's wake; non wake devices get D2 up to SystemWake */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubPdoSetPowerCapabilities(
    _In_ HubPdo* Pdo)
{
    WDF_DEVICE_POWER_CAPABILITIES Caps;
    WDF_OBJECT_ATTRIBUTES Attributes;
    HubChild* Child = Pdo->m_Child;
    SYSTEM_POWER_STATE SystemWake = Pdo->m_Hub->m_Capabilities.SystemWake;
    BOOLEAN Wake;
    ULONG State;

    PAGED_CODE();

    Wake = Child->HasProperty(ChildProperty::RemoteWakeCapable) || Child->HasProperty(ChildProperty::IsHub);

    WDF_DEVICE_POWER_CAPABILITIES_INIT(&Caps);
    Caps.SystemWake = SystemWake;
    Caps.DeviceState[PowerSystemWorking] = PowerDeviceD0;
    for (State = PowerSystemSleeping1; State <= PowerSystemShutdown; State++)
        Caps.DeviceState[State] = (State <= (ULONG)SystemWake) ? PowerDeviceD2 : PowerDeviceD3;

    Caps.DeviceWake = Wake ? PowerDeviceD2 : PowerDeviceD0;
    Caps.WakeFromD0 = WdfTrue;
    Caps.WakeFromD1 = Wake ? WdfTrue : WdfFalse;
    Caps.WakeFromD2 = Wake ? WdfTrue : WdfFalse;
    Caps.WakeFromD3 = WdfFalse;
    Caps.DeviceD1 = Wake ? WdfTrue : WdfFalse;
    Caps.DeviceD2 = Wake ? WdfTrue : WdfFalse;
    Caps.D1Latency = 0;
    Caps.D2Latency = 0;
    Caps.D3Latency = 0;
    WdfDeviceSetPowerCapabilities(Pdo->m_Device, &Caps);

    if (!Wake || !(Child->m_Kind & DSM_KIND_SUPER_SPEED))
        return STATUS_SUCCESS;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Pdo->m_Device;

    return WdfRequestCreate(&Attributes, WdfDeviceGetIoTarget(Pdo->m_Hub->m_Device), &Pdo->m_WakeRequest);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPdoResolveSystemWake(VOID)
{
    UNICODE_STRING Name;

    PAGED_CODE();

    if (HubSetSystemWakeDeviceResolved)
        return;

    RtlInitUnicodeString(&Name, L"PoSetSystemWakeDevice");
    HubSetSystemWakeDevice = (PFN_HUB_SET_SYSTEM_WAKE_DEVICE)MmGetSystemRoutineAddress(&Name);
    HubSetSystemWakeDeviceResolved = TRUE;
}

/* On failure the PDO is deleted once and its cleanup skips the device machine */
_IRQL_requires_(PASSIVE_LEVEL)
static
BOOLEAN
NTAPI
HubPdoCreate(
    _In_ HubChild* Child,
    _In_ BOOLEAN Known)
{
    HubFdo* Hub = Child->m_Hub;
    HubPort* Port = Child->m_Port;
    WDF_REMOVE_LOCK_OPTIONS RemoveLock;
    PWDFDEVICE_INIT Init;
    WDFDEVICE Device = NULL;
    BOOLEAN Referenced = FALSE;
    HubPdo* Pdo;
    NTSTATUS Status;

    PAGED_CODE();

    HubPdoResolveSystemWake();

    if (Known)
    {
        Child->SetState(ChildState::WorkingDevice);
    }
    else
    {
        Child->ClearState(ChildState::WorkingDevice);
        DPRINT1("Port %u device failed enumeration, creating a placeholder PDO\n", Port->Number());

        /* The "device not recognized" popup, except for fixed ports ACPI describes */
        if (!(Hub->HasFlag(HubFlag::InAcpiNamespace) && Child->HasProperty(ChildProperty::NotRemovable)))
            HubWmiNotifyEnumerationFailure(Hub, Port->Number());
    }

    Init = WdfPdoInitAllocate(Hub->m_Device);
    if (Init == NULL)
    {
        DPRINT1("No PDO init for port %u\n", Port->Number());
        return FALSE;
    }

    HubPdoSetCallbacks(Init);
    WdfDeviceInitSetDeviceType(Init, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetExclusive(Init, FALSE);

    Status = HubPdoAssignPreprocess(Init);
    if (!NT_SUCCESS(Status))
        goto Failed;

    WdfPdoInitAllowForwardingRequestToParent(Init);

    /* Some clients keep the PDO referenced and send I/O after remove */
    WDF_REMOVE_LOCK_OPTIONS_INIT(&RemoveLock, WDF_REMOVE_LOCK_OPTION_ACQUIRE_FOR_IO);
    WdfDeviceInitSetRemoveLockOptions(Init, &RemoveLock);

    Status = HubIdAssignPdoIds(Child, Init);
    if (!NT_SUCCESS(Status))
        goto Failed;

    /* Lets the null driver start a failed device so it can be invalidated later */
    if (!Known)
    {
        Status = WdfPdoInitAssignRawDevice(Init, &HubClassUnknown);
        if (!NT_SUCCESS(Status))
            DPRINT1("Raw device assignment failed 0x%lx\n", Status);
    }

    WdfObjectReferenceWithTag(Child->m_Object, (PVOID)HubTagPdo);
    Referenced = TRUE;

    Status = HubPdoCreateNamed(&Init, &Device);
    if (!NT_SUCCESS(Status))
        goto Failed;

    /* The cleanup callback owns the device reference from here on */
    Referenced = FALSE;

    Pdo = new (HubGetPdoContext(Device)) HubPdo();
    Pdo->m_Device = Device;
    Pdo->m_Hub = Hub;
    Pdo->m_Child = Child;
    Pdo->m_PortNumber = Port->Number();
    Pdo->m_WdfPowerState = WdfPowerDeviceD3Final;
    Pdo->m_ClientContractVersion = HUB_PDO_CONTRACT_UNKNOWN;
    KeInitializeEvent(&Pdo->m_WakeDone, NotificationEvent, FALSE);

    /* Bus filters may still send I/O to a placeholder */
    if (Known)
        Pdo->m_ControllerTarget = Hub->m_ParentInfo.RootHubPdo;
    else
        Pdo->m_FailRequests = TRUE;

    Status = HubPdoAddInterfaces(Pdo);
    if (!NT_SUCCESS(Status))
        goto Failed;

    WdfDeviceSetSpecialFileSupport(Device, WdfSpecialFilePaging, TRUE);
    WdfDeviceSetSpecialFileSupport(Device, WdfSpecialFileHibernation, TRUE);
    WdfDeviceSetSpecialFileSupport(Device, WdfSpecialFileDump, TRUE);
    WdfDeviceSetSpecialFileSupport(Device, WdfSpecialFileBoot, TRUE);

    Child->m_Pdo = Pdo;

    Status = HubPdoCreateQueue(Pdo);
    if (!NT_SUCCESS(Status))
        goto Failed;

    HubPdoSetPnpCapabilities(Pdo);

    Status = HubPdoSetPowerCapabilities(Pdo);
    if (!NT_SUCCESS(Status))
        goto Failed;

    Pdo->m_Idle.Initialize(Pdo);
    Status = HubPdoCreateIdleWorkItem(Pdo);
    if (!NT_SUCCESS(Status))
        goto Failed;

    Pdo->m_Idle.Post(IdleEvent::Start, NULL);
    Pdo->SetFlag(PdoFlag::IdleMachineStarted);

    Status = WdfFdoAddStaticChild(Hub->m_Device, Device);
    if (!NT_SUCCESS(Status))
        goto Failed;

    Pdo->m_Reported = TRUE;

    Child->m_LpmPolicy |= HUB_LPM_POLICY_LINK_BITS;
    HubPdoRegisterLpmSettings(Pdo);

    DPRINT1("Child PDO %p for port %u, VID %04x PID %04x\n",
            Device,
            Pdo->m_PortNumber,
            Child->m_DeviceDescriptor.idVendor,
            Child->m_DeviceDescriptor.idProduct);
    return TRUE;

Failed:
    DPRINT1("Child PDO creation on port %u failed 0x%lx\n", Port->Number(), Status);

    if (Init != NULL)
        WdfDeviceInitFree(Init);

    if (Device != NULL)
    {
        Child->m_Pdo = NULL;
        WdfObjectDelete(Device);
    }

    if (Referenced)
        WdfObjectDereferenceWithTag(Child->m_Object, (PVOID)HubTagPdo);

    return FALSE;
}

/* Serialized client requests */

static
PURB
NTAPI
HubClientUrb(
    _In_ WDFREQUEST Request,
    _Out_ PULONG IoControlCode)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(WdfRequestWdmGetIrp(Request));

    *IoControlCode = Stack->Parameters.DeviceIoControl.IoControlCode;
    if (*IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
        return NULL;

    return (PURB)Stack->Parameters.Others.Argument1;
}

/* Legacy client behavior for failed selects */
static
VOID
NTAPI
HubPatchFailedSelect(
    _In_ HubChild* Child,
    _Inout_ PURB Urb)
{
    if (Urb->UrbHeader.Function == URB_FUNCTION_SELECT_INTERFACE)
    {
        Child->SetState(ChildState::ConfigurationValid);
        Urb->UrbSelectInterface.Interface.InterfaceHandle = (USBD_INTERFACE_HANDLE)(LONG_PTR)-1;
    }
    else if (Urb->UrbHeader.Function == URB_FUNCTION_SELECT_CONFIGURATION)
    {
        Urb->UrbSelectConfiguration.ConfigurationHandle = NULL;
    }
}

static
WDFREQUEST
NTAPI
HubTakeClientRequest(
    _In_ HubChild* Child)
{
    WDFREQUEST Request = Child->m_ClientRequest;

    Child->m_ClientRequest = NULL;
    Child->ClearState(ChildState::ActivityIdSet);
    return Request;
}

VOID
DeviceMachine::CompleteClientRequest()
{
    HubChild* Child = m_Device;
    HubPdo* Pdo = Child->m_Pdo;
    WDFREQUEST Request = Child->m_ClientRequest;
    PIO_STACK_LOCATION Stack;
    PUCXHUB_RESET_FLAGS Flags;
    BOOLEAN PowerLost;
    BOOLEAN ProgrammingLost;
    PURB Urb;
    ULONG Code;

    Urb = HubClientUrb(Request, &Code);

    if (Urb != NULL)
    {
        if ((Urb->UrbHeader.Function == URB_FUNCTION_SELECT_CONFIGURATION &&
             Urb->UrbSelectConfiguration.ConfigurationDescriptor != NULL) ||
            Urb->UrbHeader.Function == URB_FUNCTION_SELECT_INTERFACE)
        {
            Child->SetState(ChildState::ConfigurationValid);
        }
    }
    else if (Code == IOCTL_UCXHUB_RESET_PORT_ASYNC)
    {
        Stack = IoGetCurrentIrpStackLocation(WdfRequestWdmGetIrp(Request));
        Flags = (PUCXHUB_RESET_FLAGS)Stack->Parameters.Others.Argument1;
        PowerLost = Pdo->TestAndClearFlag(PdoFlag::PowerLostOnReset);
        ProgrammingLost = Pdo->TestAndClearFlag(PdoFlag::ProgrammingLostOnReset);

        if (Flags != NULL)
        {
            Flags->AsUlong = 0;
            Flags->PortPowerInterrupted = PowerLost;
            Flags->HostContextLost = ProgrammingLost;
        }
    }

    HubTakeClientRequest(Child);
    Child->m_Port->m_ConnectionStatus = DeviceConnected;
    DPRINT("Port %u client request %p done\n", Pdo->m_PortNumber, Request);
    WdfRequestComplete(Request, STATUS_SUCCESS);
}

VOID
DeviceMachine::CompleteClientRequestFailed()
{
    HubChild* Child = m_Device;
    WDFREQUEST Request = HubTakeClientRequest(Child);
    PURB Urb;
    ULONG Code;

    Urb = HubClientUrb(Request, &Code);
    if (Urb != NULL)
    {
        Urb->UrbHeader.Status = HubNtStatusToUsbd(STATUS_UNSUCCESSFUL);
        HubPatchFailedSelect(Child, Urb);
    }

    DPRINT1("Port %u client request %p IOCTL 0x%lx failed\n", Child->m_Port->Number(), Request, Code);
    WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
}

VOID
DeviceMachine::CompleteClientRequestLastStatus()
{
    HubChild* Child = m_Device;
    NTSTATUS Status = Child->m_LastNtStatus;
    WDFREQUEST Request;
    PURB Urb;
    ULONG Code;

    Request = HubTakeClientRequest(Child);
    Urb = HubClientUrb(Request, &Code);
    if (Urb != NULL)
    {
        Urb->UrbHeader.Status = Child->m_LastUsbdStatus;
        HubPatchFailedSelect(Child, Urb);
    }

    Child->m_LastNtStatus = STATUS_SUCCESS;
    Child->m_LastUsbdStatus = USBD_STATUS_SUCCESS;

    if (NT_SUCCESS(Status))
        Status = STATUS_UNSUCCESSFUL;

    DPRINT1("Port %u client request %p IOCTL 0x%lx failed 0x%lx, URB status 0x%lx\n",
            Child->m_Port->Number(),
            Request,
            Code,
            Status,
            Urb != NULL ? Urb->UrbHeader.Status : USBD_STATUS_SUCCESS);
    WdfRequestComplete(Request, Status);
}

/* The request pointer and the URB status are left as they are */
VOID
DeviceMachine::FailClientRequest()
{
    DPRINT1("Port %u client request %p failed\n", m_Device->m_Port->Number(), m_Device->m_ClientRequest);
    WdfRequestComplete(m_Device->m_ClientRequest, STATUS_UNSUCCESSFUL);
    m_Device->ClearState(ChildState::ActivityIdSet);
}

/* Configuration checks for SELECT_CONFIGURATION and SELECT_INTERFACE */

static
USBD_STATUS
NTAPI
HubCheckConfigDescriptor(
    _In_ HubChild* Child,
    _In_ PUSB_CONFIGURATION_DESCRIPTOR Descriptor)
{
    HubDescContext Context;
    volatile UCHAR Touch;

    if (Descriptor->bDescriptorType != USB_CONFIGURATION_DESCRIPTOR_TYPE ||
        Descriptor->bLength < sizeof(*Descriptor) ||
        Descriptor->wTotalLength == 0)
    {
        return USBD_STATUS_INVALID_CONFIGURATION_DESCRIPTOR;
    }

    /* A bad client buffer faults here rather than deep in the validator */
    Touch = ((volatile UCHAR*)Descriptor)[0];
    Touch = ((volatile UCHAR*)Descriptor)[Descriptor->wTotalLength - 1];
    (VOID)Touch;

    HubDescInitDeviceContext(&Context,
                             Child->m_DeviceDescriptor.bcdUSB,
                             Child->Speed(),
                             Child->HasHack(ChildHack::UseWin8DescriptorValidation),
                             Child->m_Port->m_Info.SspIsochBurstCount,
                             Child->m_ValidationBits);

    if (!HubDescCheckConfiguration(&Context, Descriptor, Descriptor->wTotalLength, NULL))
        return USBD_STATUS_INVALID_CONFIGURATION_DESCRIPTOR;

    return USBD_STATUS_SUCCESS;
}

static
BOOLEAN
NTAPI
HubFailClientCheck(
    _In_ HubChild* Child,
    _In_ USBD_STATUS UsbdStatus)
{
    Child->m_LastUsbdStatus = UsbdStatus;
    Child->m_LastNtStatus = HubUsbdStatusToNt(UsbdStatus);
    DPRINT1("Port %u client select check failed, USBD status 0x%lx\n", Child->m_Port->Number(), UsbdStatus);
    return FALSE;
}

/* SuperSpeed devices are measured in 2 mA units against the USB 2.0 budget */
BOOLEAN
DeviceMachine::SelectConfigValid()
{
    HubChild* Child = m_Device;
    HubPdo* Pdo = Child->m_Pdo;
    HubFdo* Hub = Child->m_Hub;
    ULONG Code;
    PURB Urb = HubClientUrb(Child->m_ClientRequest, &Code);
    PUSB_CONFIGURATION_DESCRIPTOR Descriptor = Urb->UrbSelectConfiguration.ConfigurationDescriptor;
    PUCHAR Entry;
    PUCHAR End;
    PUSBD_INTERFACE_INFORMATION Interface;
    USBD_STATUS UsbdStatus;
    ULONG Count = 0;

    PAGED_CODE();

    if (Descriptor == NULL)
    {
        Pdo->m_ConfigPowerDraw = 0;
        return TRUE;
    }

    UsbdStatus = HubCheckConfigDescriptor(Child, Descriptor);
    if (UsbdStatus != USBD_STATUS_SUCCESS)
        return HubFailClientCheck(Child, UsbdStatus);

    Pdo->m_ConfigPowerDraw = Descriptor->MaxPower * 2;
    if (!Hub->IsRootHub() && Pdo->m_ConfigPowerDraw > Hub->m_MaxPortPower)
    {
        DPRINT1("Port %u configuration needs %lu mA, the port has %lu mA\n",
                Pdo->m_PortNumber,
                Pdo->m_ConfigPowerDraw,
                Hub->m_MaxPortPower);
        Child->m_LastNtStatus = STATUS_INSUFFICIENT_RESOURCES;
        Child->m_LastUsbdStatus = USBD_STATUS_INSUFFICIENT_RESOURCES;
        Child->m_Port->m_ConnectionStatus = DeviceNotEnoughPower;
        HubWmiNotifyInsufficientPower(Hub, Pdo->m_PortNumber);
        return FALSE;
    }

    /* SuperSpeed units are 8 mA with a 150 or 900 mA budget; only reported, for compatibility */
    if ((Child->m_Kind & DSM_KIND_SUPER_SPEED) &&
        (ULONG)Descriptor->MaxPower * 8 > (Hub->m_MaxPortPower == 100 ? 150UL : 900UL))
    {
        DPRINT1("Port %u SuperSpeed configuration asks for more power than the port has\n", Pdo->m_PortNumber);
    }

    Entry = (PUCHAR)&Urb->UrbSelectConfiguration.Interface;
    End = (PUCHAR)Urb + Urb->UrbHeader.Length;

    while (Entry + sizeof(USHORT) < End && Count < Descriptor->bNumInterfaces)
    {
        Interface = (PUSBD_INTERFACE_INFORMATION)Entry;
        if (Interface->Length < FIELD_OFFSET(USBD_INTERFACE_INFORMATION, Pipes) ||
            Entry + Interface->Length > End)
        {
            DPRINT1("Port %u select config interface %lu has bad length %u\n",
                    Pdo->m_PortNumber,
                    Count,
                    Interface->Length);
            Child->m_LastNtStatus = STATUS_UNSUCCESSFUL;
            Child->m_LastUsbdStatus = USBD_STATUS_INVALID_CONFIGURATION_DESCRIPTOR;
            return FALSE;
        }

        Count++;
        Entry += Interface->Length;
    }

    if (Descriptor->bNumInterfaces == 0 || Count != Descriptor->bNumInterfaces)
        return HubFailClientCheck(Child, USBD_STATUS_INVALID_CONFIGURATION_DESCRIPTOR);

    return TRUE;
}

/* NULL unless the interface and all the endpoints it announces are in the configuration */
static
PUSB_INTERFACE_DESCRIPTOR
NTAPI
HubFindSelectedInterface(
    _In_ PUSB_CONFIGURATION_DESCRIPTOR Config,
    _In_ UCHAR Number,
    _In_ UCHAR Alternate)
{
    PUCHAR Position = (PUCHAR)Config;
    PUCHAR End;
    PUSB_COMMON_DESCRIPTOR Common;
    PUSB_INTERFACE_DESCRIPTOR Interface;
    PUSB_INTERFACE_DESCRIPTOR Found = NULL;
    BOOLEAN NumberSeen = FALSE;
    ULONG Endpoints = 0;

    if (Config->bLength < sizeof(*Config) ||
        Config->bDescriptorType != USB_CONFIGURATION_DESCRIPTOR_TYPE ||
        Config->wTotalLength < sizeof(*Config))
    {
        return NULL;
    }

    End = Position + Config->wTotalLength;

    for (; Position + sizeof(*Common) <= End; Position += Common->bLength)
    {
        Common = (PUSB_COMMON_DESCRIPTOR)Position;
        if (Common->bLength == 0 || Position + Common->bLength > End)
            break;

        if (Common->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
        {
            if (Common->bLength < sizeof(*Interface))
                continue;

            Interface = (PUSB_INTERFACE_DESCRIPTOR)Common;
            if (Interface->bInterfaceNumber != Number)
            {
                if (NumberSeen)
                    break;
                continue;
            }

            NumberSeen = TRUE;
            if (Found != NULL)
                break;
            if (Interface->bAlternateSetting == Alternate)
                Found = Interface;
        }
        else if (Common->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE && Found != NULL)
        {
            if (Common->bLength < sizeof(USB_ENDPOINT_DESCRIPTOR))
                return NULL;
            Endpoints++;
        }
    }

    if (Found != NULL && Found->bNumEndpoints > Endpoints)
        return NULL;

    return Found;
}

/* A missing interface sets the client's Length to 0; bad pipe flags still fill every pipe */
static
USBD_STATUS
NTAPI
HubInitInterfaceInformation(
    _In_ PUSB_CONFIGURATION_DESCRIPTOR Config,
    _Inout_ PUSBD_INTERFACE_INFORMATION Info)
{
    PUSB_INTERFACE_DESCRIPTOR Descriptor;
    PUSBD_PIPE_INFORMATION Pipe;
    PUCHAR Position;
    USBD_STATUS Status = USBD_STATUS_SUCCESS;
    ULONG Needed;
    ULONG Index;

    Descriptor = HubFindSelectedInterface(Config, Info->InterfaceNumber, Info->AlternateSetting);
    if (Descriptor == NULL)
    {
        Info->Length = 0;
        return USBD_STATUS_INTERFACE_NOT_FOUND;
    }

    Needed = FIELD_OFFSET(USBD_INTERFACE_INFORMATION, Pipes) +
             Descriptor->bNumEndpoints * sizeof(USBD_PIPE_INFORMATION);

    if (Info->Length < Needed)
    {
        Info->Length = (USHORT)Needed;
        return USBD_STATUS_BUFFER_TOO_SMALL;
    }

    Info->Class = Descriptor->bInterfaceClass;
    Info->SubClass = Descriptor->bInterfaceSubClass;
    Info->Protocol = Descriptor->bInterfaceProtocol;
    Info->Reserved = 0;
    Info->InterfaceHandle = NULL;
    Info->NumberOfPipes = Descriptor->bNumEndpoints;

    Position = (PUCHAR)Descriptor;
    for (Index = 0; Index < Descriptor->bNumEndpoints; Index++)
    {
        /* HubFindSelectedInterface proved this many endpoints follow */
        do
        {
            Position += ((PUSB_COMMON_DESCRIPTOR)Position)->bLength;
        } while (((PUSB_COMMON_DESCRIPTOR)Position)->bDescriptorType != USB_ENDPOINT_DESCRIPTOR_TYPE);

        Pipe = &Info->Pipes[Index];
        Pipe->EndpointAddress = ((PUSB_ENDPOINT_DESCRIPTOR)Position)->bEndpointAddress;
        Pipe->Interval = ((PUSB_ENDPOINT_DESCRIPTOR)Position)->bInterval;
        Pipe->PipeType = UsbdPipeTypeControl;
        Pipe->PipeHandle = NULL;

        if (Pipe->PipeFlags & ~USBD_PF_VALID_MASK)
            Status = USBD_STATUS_INVALID_PIPE_FLAGS;

        if (!(Pipe->PipeFlags & USBD_PF_CHANGE_MAX_PACKET))
            Pipe->MaximumPacketSize = 0;
    }

    Info->Length = (USHORT)Needed;
    return Status;
}

/* A NULL handle from an old style client fails when nothing is configured */
BOOLEAN
DeviceMachine::SelectInterfaceValid()
{
    HubChild* Child = m_Device;
    HubPdo* Pdo = Child->m_Pdo;
    HubConfiguration* Current = Child->m_CurrentConfig;
    ULONG Code;
    PURB Urb = HubClientUrb(Child->m_ClientRequest, &Code);
    struct _URB_SELECT_INTERFACE* Select = &Urb->UrbSelectInterface;
    USBD_CONFIGURATION_HANDLE Handle = Select->ConfigurationHandle;
    USHORT Expected;
    USBD_STATUS UsbdStatus;

    /* Old style clients (a KMDF usbser filter bug) send no handle */
    if (Handle == NULL && Pdo->m_ClientContractVersion == HUB_PDO_CONTRACT_UNKNOWN && Current != NULL)
    {
        DPRINT1("Select interface without a configuration handle\n");
        Handle = (USBD_CONFIGURATION_HANDLE)Current;
        Select->ConfigurationHandle = Handle;
    }

    if (Current == NULL || Handle != (USBD_CONFIGURATION_HANDLE)Current)
        return HubFailClientCheck(Child, USBD_STATUS_INVALID_PARAMETER);

    Select->Interface.InterfaceHandle = (USBD_INTERFACE_HANDLE)(LONG_PTR)-1;

    /* Clients commonly get the URB length wrong */
    Expected = (USHORT)(Select->Interface.Length + sizeof(struct _URB_HEADER) + sizeof(PVOID));
    if (Select->Hdr.Length != Expected)
        Select->Hdr.Length = Expected;

    UsbdStatus = HubInitInterfaceInformation(&Current->Descriptor, &Select->Interface);
    if (UsbdStatus != USBD_STATUS_SUCCESS)
        return HubFailClientCheck(Child, UsbdStatus);

    return TRUE;
}

BOOLEAN
DeviceMachine::FindClientPipe()
{
    HubChild* Child = m_Device;
    HubConfiguration* Config = Child->m_CurrentConfig;
    ULONG Code;
    PURB Urb = HubClientUrb(Child->m_ClientRequest, &Code);
    HubPipe* Pipe;

    Pipe = HubFindPipeByHandle(Config, Urb->UrbPipeRequest.PipeHandle);
    if (Pipe != NULL)
    {
        Child->m_TargetPipe = Pipe;
        return TRUE;
    }

    DPRINT1("Port %u client pipe handle %p not found\n", Child->m_Port->Number(), Urb->UrbPipeRequest.PipeHandle);
    Child->m_LastUsbdStatus = USBD_STATUS_INVALID_PIPE_HANDLE;
    Child->m_LastNtStatus = STATUS_INVALID_PARAMETER;
    return FALSE;
}

BOOLEAN
DeviceMachine::PipeIsZeroBandwidth()
{
    return m_Device->m_TargetPipe->ZeroBandwidth;
}

BOOLEAN
DeviceMachine::PipeIsIsochronous()
{
    return (m_Device->m_TargetPipe->Descriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_ISOCHRONOUS;
}

/* PDO creation and reporting */

BOOLEAN
DeviceMachine::CreatePdo()
{
    HubChild* Child = m_Device;

    PAGED_CODE();

    Child->m_EnumMessageId = 0;

    if (!HubPdoCreate(Child, TRUE))
    {
        Child->m_Port->m_ConnectionStatus = DeviceGeneralFailure;
        return FALSE;
    }

    HubChildSetPdo(Child, WdfDeviceWdmGetDeviceObject(Child->m_Pdo->m_Device));
    Child->m_Port->m_ConnectionStatus = DeviceConnected;
    return TRUE;
}

BOOLEAN
DeviceMachine::CreatePlaceholderPdo()
{
    PAGED_CODE();

    m_Device->m_Port->m_ConnectionStatus = DeviceFailedEnumeration;
    return HubPdoCreate(m_Device, FALSE);
}

VOID
DeviceMachine::ReportEnumFailure()
{
    HubChild* Child = m_Device;

    DPRINT1("Port %u enumeration failed, message %lu\n", Child->m_Port->Number(), Child->m_EnumMessageId);
    HubCreateReport(Child->m_Hub->m_Device, HubReport::DeviceEnumerationFailed, Child->m_EnumMessageId);
    WdfDeviceSetFailed(Child->m_Pdo->m_Device, WdfDeviceFailedAttemptRestart);
}

VOID
DeviceMachine::ReportDeviceMissing()
{
    NTSTATUS Status;
    KIRQL OldIrql;

    DPRINT("Port %u device marked missing\n", m_Device->m_Port->Number());

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    Status = WdfPdoMarkMissing(m_Device->m_Pdo->m_Device);
    KeLowerIrql(OldIrql);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Marking the PDO missing failed 0x%lx\n", Status);
        NT_ASSERT(FALSE);
    }
}

/* A cycle is how UXD redirects the PDO, so its settings stay then */
VOID
DeviceMachine::NotifyDisconnected()
{
    HubChild* Child = m_Device;
    HubPdo* Pdo = Child->m_Pdo;

    PAGED_CODE();

    DPRINT("Port %u device disconnected\n", Pdo->m_PortNumber);

    Pdo->SetFlag(PdoFlag::DeviceGone);
    Pdo->m_FailRequests = TRUE;

    if (!Pdo->HasFlag(PdoFlag::InBootPath) && Pdo->m_CycleQueued == 0)
        HubPurgeUxdState(Child, HubUxdEvent::Disconnect);
}

VOID
DeviceMachine::NotifyReconnected()
{
    HubPdo* Pdo = m_Device->m_Pdo;

    DPRINT("Port %u device reconnected\n", Pdo->m_PortNumber);

    Pdo->m_FailRequests = FALSE;
    Pdo->ClearFlag(PdoFlag::DeviceGone);
}

/* Hub power; DISPATCH_LEVEL keeps KMDF from calling a blocking PDO callback on this thread */

BOOLEAN
DeviceMachine::ReferenceHubPower()
{
    NTSTATUS Status;
    KIRQL OldIrql;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    Status = WdfDeviceStopIdle(m_Device->m_Hub->m_Device, FALSE);
    KeLowerIrql(OldIrql);

    if (NT_SUCCESS(Status))
        m_Device->m_HubPowerReferenceHeld = TRUE;
    else
        DPRINT1("Port %u hub power reference failed 0x%lx\n", m_Device->m_Port->Number(), Status);

    return TRUE;
}

VOID
DeviceMachine::DereferenceHubPower()
{
    KIRQL OldIrql;

    if (!m_Device->m_HubPowerReferenceHeld)
        return;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    WdfDeviceResumeIdle(m_Device->m_Hub->m_Device);
    KeLowerIrql(OldIrql);

    m_Device->m_HubPowerReferenceHeld = FALSE;
}

VOID
DeviceMachine::StartDriverWaitTimer()
{
    m_Device->m_Timer.Start(HUB_PDO_DRIVER_WAIT_TIME);
}

/* PnP waiters */

VOID
DeviceMachine::SignalPnpWaiter()
{
    m_Device->m_PnpStatus = STATUS_SUCCESS;
    KeSetEvent(&m_Device->m_PnpEvent, IO_NO_INCREMENT, FALSE);
    m_Device->ClearState(ChildState::ActivityIdSet);
}

VOID
DeviceMachine::SignalPnpPowerFailure()
{
    m_Device->m_PnpStatus = STATUS_UNSUCCESSFUL;
    KeSetEvent(&m_Device->m_PnpEvent, IO_NO_INCREMENT, FALSE);
    m_Device->ClearState(ChildState::ActivityIdSet);
}

VOID
DeviceMachine::ReleasePnpWaiter()
{
    KeSetEvent(&m_Device->m_PnpEvent, IO_NO_INCREMENT, FALSE);
}

VOID
DeviceMachine::CompletePreStart()
{
    KeSetEvent(&m_Device->m_PreStartEvent, IO_NO_INCREMENT, FALSE);
}

VOID
DeviceMachine::AckPreStart()
{
    KeSetEvent(&m_Device->m_PreStartEvent, IO_NO_INCREMENT, FALSE);
}

VOID
DeviceMachine::CompleteQueryText()
{
    KeSetEvent(&m_Device->m_QueryTextEvent, IO_NO_INCREMENT, FALSE);
}

/* No string was read, so the text query finds none */
VOID
DeviceMachine::SignalQueryText()
{
    KeSetEvent(&m_Device->m_QueryTextEvent, IO_NO_INCREMENT, FALSE);
}

VOID
DeviceMachine::CyclePortForClient()
{
    m_Device->m_Port->Post(PortEvent::CycleRequest);
}

/* PDO flags */

VOID
DeviceMachine::AllowIo()
{
    m_Device->m_Pdo->m_FailRequests = FALSE;
}

VOID
DeviceMachine::SetFailIo()
{
    m_Device->m_Pdo->m_FailRequests = TRUE;
}

VOID
DeviceMachine::ForceResetOnNextStart()
{
    m_Device->SetState(ChildState::HotResetOnEnumeration);
}

VOID
DeviceMachine::MarkPowerLost()
{
    m_Device->m_Pdo->SetFlag(PdoFlag::PowerLostOnReset);
}

VOID
DeviceMachine::MarkResetAtResume()
{
    m_Device->m_Pdo->SetFlag(PdoFlag::WasResetAtResume);
    m_Device->m_Pdo->SetFlag(PdoFlag::ReportPortDisabled);
}

VOID
DeviceMachine::ClearResetAtResume()
{
    m_Device->m_Pdo->ClearFlag(PdoFlag::WasResetAtResume);
    m_Device->m_Pdo->ClearFlag(PdoFlag::ReportPortDisabled);
}

BOOLEAN
DeviceMachine::ResetAtResumeMarked()
{
    return m_Device->m_Pdo->HasFlag(PdoFlag::WasResetAtResume);
}

BOOLEAN
DeviceMachine::PdoStarted()
{
    return m_Device->m_Pdo != NULL && m_Device->m_Pdo->HasFlag(PdoFlag::Started);
}

BOOLEAN
DeviceMachine::IsBootDevice()
{
    return m_Device->m_Pdo != NULL && m_Device->m_Pdo->HasFlag(PdoFlag::InBootPath);
}

/* Wake */

VOID
DeviceMachine::FinishWaitWake()
{
    HubPdoIndicateWake(m_Device->m_Pdo);
}

/* KMDF completes the wait wake IRP too early for PoSetSystemWake on it */
VOID
DeviceMachine::MarkSystemWakeSource()
{
    DPRINT("Port %u device is the system wake source\n", m_Device->m_Port->Number());

    if (HubSetSystemWakeDevice != NULL)
        HubSetSystemWakeDevice(WdfDeviceWdmGetDeviceObject(m_Device->m_Pdo->m_Device));
}
