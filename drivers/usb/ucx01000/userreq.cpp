/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User mode and legacy host controller IOCTLs, and the WMI node info
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"
#include <usbuser.h>

#define NDEBUG
#include <debug.h>

/* What usbview and friends get told about the port driver */
#define UCX_DRIVER_TRACKING_CODE   4
#define UCX_REPORTED_USBDI_VERSION 0x600

/* Fake bus bandwidth: 7000 bytes per frame in bits, times 8 microframes */
#define UCX_BUS_BANDWIDTH          (7000 * 8 * 8)

#define UCX_PASS_THRU_MAX_LENGTH   0x10000
#define UCX_LEGACY_NAME_ATTEMPTS   5
#define UCX_SYNTHETIC_FRAME_MASK        0x7FF

/* Header, then the name length; what a name request needs before any text */
#define UCX_USER_HEADER_SIZE       sizeof(USBUSER_REQUEST_HEADER)
#define UCX_USER_NAME_MIN          sizeof(USB_UNICODE_NAME)

#define UCX_NODE_INFO_SIGNATURE    'USBN'

C_ASSERT(UCX_USER_HEADER_SIZE == 16);
C_ASSERT(UCX_USER_NAME_MIN == 6);
C_ASSERT(sizeof(USB_ROOT_HUB_NAME) == 6);

/** One IOCTL_USB_USER_REQUEST: the header and the payload right after it. */
class UcxUserRequest
{
public:
    UcxUserRequest(
        _In_ PUSBUSER_REQUEST_HEADER Header,
        _In_ ULONG BufferLength)
        : m_Header(Header),
          m_BufferLength(BufferLength)
    {
    }

    template <typename T>
    T*
    Payload() const
    {
        return (T*)(m_Header + 1);
    }

    PUSBUSER_REQUEST_HEADER
    Header() const
    {
        return m_Header;
    }

    ULONG
    NameSpace() const
    {
        return m_BufferLength - UCX_USER_HEADER_SIZE;
    }

    VOID
    SetStatus(
        _In_ USB_USER_ERROR_CODE Status)
    {
        m_Header->UsbUserStatusCode = Status;
    }

    /** The total size is reported even when it does not fit. */
    BOOLEAN
    CheckParameterLength(
        _In_ ULONG ParameterLength)
    {
        m_Header->ActualBufferLength = UCX_USER_HEADER_SIZE + ParameterLength;

        if (m_Header->ActualBufferLength > m_Header->RequestBufferLength)
        {
            DPRINT1("USBUSER request 0x%lx needs %lu bytes, got %lu\n",
                    m_Header->UsbUserRequest, m_Header->ActualBufferLength, m_Header->RequestBufferLength);
            SetStatus(UsbUserBufferTooSmall);
            return FALSE;
        }

        return TRUE;
    }

private:
    PUSBUSER_REQUEST_HEADER m_Header;
    ULONG m_BufferLength;
};

/* Power state maps */

static
WDMUSB_POWER_STATE
NTAPI
UcxMapDevicePowerState(
    _In_ DEVICE_POWER_STATE State)
{
    if (State >= PowerDeviceUnspecified && State <= PowerDeviceD3)
        return (WDMUSB_POWER_STATE)(WdmUsbPowerDeviceUnspecified + State);

    return WdmUsbPowerNotMapped;
}

/* Unspecified is not mapped either */
static
WDMUSB_POWER_STATE
NTAPI
UcxMapSystemPowerState(
    _In_ SYSTEM_POWER_STATE State)
{
    if (State >= PowerSystemWorking && State <= PowerSystemShutdown)
        return (WDMUSB_POWER_STATE)(WdmUsbPowerSystemUnspecified + State);

    return WdmUsbPowerNotMapped;
}

/* Request handlers */

static
VOID
NTAPI
UcxUserGetControllerInfo(
    _Inout_ UcxUserRequest& User,
    _In_ UcxController* Controller)
{
    PUSB_CONTROLLER_INFO_0 Info = User.Payload<USB_CONTROLLER_INFO_0>();
    const UCX_CONTROLLER_PCI_INFORMATION* Pci = &Controller->m_Config.PciDeviceInfo;

    Info->PciVendorId = Pci->VendorId;
    Info->PciDeviceId = Pci->DeviceId;
    Info->PciRevision = Pci->RevisionId;

    /* USB 2.0 ports only, known once the hub asked for the root hub info */
    Info->NumberOfRootPorts = Controller->m_RootHub->m_NumberOf20Ports;
    Info->ControllerFlavor = USB_HcGeneric;
    Info->HcFeatureFlags = 0;

    User.SetStatus(UsbUserSuccess);
}

/* Shared by the USBUSER request and the legacy IOCTL */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
UcxUserGetDriverKeyName(
    _Inout_ UcxUserRequest& User,
    _In_ WDFDEVICE Fdo)
{
    PUSB_UNICODE_NAME Name = User.Payload<USB_UNICODE_NAME>();
    ULONG Space = User.NameSpace();
    ULONG ResultLength = 0;
    NTSTATUS Status;

    if (Space < UCX_USER_NAME_MIN)
    {
        User.SetStatus(UsbUserBufferTooSmall);
        return;
    }

    Space -= UCX_USER_NAME_MIN;
    RtlZeroMemory(Name, Space);

    Status = WdfDeviceQueryProperty(Fdo, DevicePropertyDriverKeyName, Space, Name->String, &ResultLength);
    if (NT_SUCCESS(Status))
    {
        User.SetStatus(UsbUserSuccess);
        Name->Length = ResultLength + sizeof(WCHAR);
    }
    else if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        User.SetStatus(UsbUserBufferTooSmall);
    }
    else
    {
        DPRINT1("Driver key name query on %p failed 0x%lx\n", Fdo, Status);
        User.SetStatus(UsbUserInvalidParameter);
    }

    User.Header()->ActualBufferLength = UCX_USER_HEADER_SIZE + UCX_USER_NAME_MIN + ResultLength;
}

/** The root hub name without its prefix; NULL when the PDO has no name. */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
UcxGetStrippedRootHubName(
    _In_ UcxRootHub* RootHub,
    _Out_ WDFSTRING* Stripped)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING Name;
    WDFSTRING Original;
    USHORT Skip = 0;
    USHORT Count;
    NTSTATUS Status;

    *Stripped = NULL;

    if (!NT_SUCCESS(RootHub->ReferenceSymbolicName(&Original)))
    {
        DPRINT("Root hub %p has no symbolic name yet\n", RootHub);
        return STATUS_SUCCESS;
    }

    WdfStringGetUnicodeString(Original, &Name);
    Count = Name.Length / sizeof(WCHAR);

    if (Count == 0 || Name.Buffer == NULL)
    {
        DPRINT1("Root hub %p symbolic name is empty\n", RootHub);
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        /* Drop "\xxx\"; without a second backslash everything scanned goes */
        if (Name.Buffer[0] == L'\\')
        {
            for (Skip = 1; Skip < Count; Skip++)
            {
                if (Name.Buffer[Skip] == L'\\' || Name.Buffer[Skip] == UNICODE_NULL)
                    break;
            }

            if (Skip < Count && Name.Buffer[Skip] == L'\\')
                Skip++;
        }

        Name.Buffer += Skip;
        Name.Length -= Skip * sizeof(WCHAR);
        Name.MaximumLength -= Skip * sizeof(WCHAR);

        WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
        Attributes.ParentObject = RootHub->m_Handle;

        Status = WdfStringCreate(&Name, &Attributes, Stripped);
        if (!NT_SUCCESS(Status))
            DPRINT1("Copying root hub %p name failed 0x%lx\n", RootHub, Status);
    }

    WdfObjectDereferenceWithTag(Original, (PVOID)UCX_POOL_TAG);
    return Status;
}

/* A name that does not fit is still copied as far as it goes */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
UcxUserGetRootHubName(
    _Inout_ UcxUserRequest& User,
    _In_ UcxRootHub* RootHub)
{
    PUSB_UNICODE_NAME Name = User.Payload<USB_UNICODE_NAME>();
    ULONG Space = User.NameSpace();
    UNICODE_STRING Text;
    WDFSTRING String;
    ULONG Size;

    if (Space < UCX_USER_NAME_MIN)
    {
        User.SetStatus(UsbUserBufferTooSmall);
        return;
    }

    Space -= UCX_USER_NAME_MIN;
    RtlZeroMemory(Name, Space);

    if (!NT_SUCCESS(UcxGetStrippedRootHubName(RootHub, &String)))
    {
        User.SetStatus(UsbUserInvalidParameter);
        User.Header()->ActualBufferLength = UCX_USER_HEADER_SIZE + UCX_USER_NAME_MIN;
        return;
    }

    /* Not started: success with nothing written */
    if (String == NULL)
        return;

    WdfStringGetUnicodeString(String, &Text);
    Size = Text.Length;

    RtlCopyMemory(Name->String, Text.Buffer, min(Size, Space));

    if (Size > Space)
    {
        User.SetStatus(UsbUserBufferTooSmall);
    }
    else
    {
        User.SetStatus(UsbUserSuccess);
        Name->Length = Size + sizeof(WCHAR);
    }

    User.Header()->ActualBufferLength = UCX_USER_HEADER_SIZE + UCX_USER_NAME_MIN + Size;
    WdfObjectDelete(String);
}

/* No pass through function is serviced; only the sizes are checked */
static
VOID
NTAPI
UcxUserPassThru(
    _Inout_ UcxUserRequest& User)
{
    ULONG Length = User.Payload<USB_PASS_THRU_PARAMETERS>()->ParameterLength;

    if (Length > UCX_PASS_THRU_MAX_LENGTH)
    {
        DPRINT1("Pass through parameter length %lu is too large\n", Length);
        User.SetStatus(UsbUserInvalidParameter);
    }
    else if (UCX_USER_HEADER_SIZE + FIELD_OFFSET(USB_PASS_THRU_PARAMETERS, Parameters) + Length >
             User.Header()->RequestBufferLength)
    {
        DPRINT1("Pass through buffer too small for %lu parameter bytes\n", Length);
        User.SetStatus(UsbUserBufferTooSmall);
    }
    else
    {
        DPRINT("Pass through request is not serviced\n");
        User.SetStatus(UsbUserMiniportError);
        User.Header()->ActualBufferLength = UCX_USER_HEADER_SIZE + sizeof(USB_PASS_THRU_PARAMETERS) + Length;
    }
}

/** Whether the controller can wake the system from Sx, and stays powered in it. */
static
VOID
NTAPI
UcxMapWakeAndPower(
    _In_ const DEVICE_CAPABILITIES* HcCaps,
    _In_ SYSTEM_POWER_STATE SleepState,
    _Out_ PUSB_POWER_INFO Info)
{
    DEVICE_POWER_STATE DeviceState = HcCaps->DeviceState[SleepState];

    if (SleepState <= HcCaps->SystemWake)
    {
        Info->CanWakeup = (DeviceState != PowerDeviceUnspecified);
        Info->IsPowered = Info->CanWakeup;
    }
    else
    {
        Info->CanWakeup = FALSE;
        Info->IsPowered = (DeviceState != PowerDeviceD3 && DeviceState != PowerDeviceUnspecified);
    }
}

/* System states outside Working..Hibernate leave the state fields as sent */
static
VOID
NTAPI
UcxUserGetPowerStateMap(
    _Inout_ UcxUserRequest& User,
    _In_ UcxController* Controller)
{
    PUSB_POWER_INFO Info = User.Payload<USB_POWER_INFO>();
    UcxRootHub* RootHub = Controller->m_RootHub;
    const WDF_DEVICE_POWER_CAPABILITIES* RhCaps = &RootHub->m_PowerCaps;
    const DEVICE_CAPABILITIES* HcCaps = &Controller->m_HcCaps;
    SYSTEM_POWER_STATE State;

    if (!RootHub->m_PdoStarted)
    {
        DPRINT1("Power state map requested before root hub %p started\n", RootHub);
        User.SetStatus(UsbUserDeviceNotStarted);
        return;
    }

    Info->RhDeviceWake = UcxMapDevicePowerState(RhCaps->DeviceWake);
    Info->RhSystemWake = UcxMapSystemPowerState(RhCaps->SystemWake);
    Info->HcDeviceWake = UcxMapDevicePowerState(HcCaps->DeviceWake);
    Info->HcSystemWake = UcxMapSystemPowerState(HcCaps->SystemWake);
    Info->LastSystemSleepState = UcxMapSystemPowerState(RootHub->m_LastSystemSleepState);

    if (Info->SystemState < WdmUsbPowerSystemWorking || Info->SystemState > WdmUsbPowerSystemHibernate)
        return;

    State = (SYSTEM_POWER_STATE)(Info->SystemState - WdmUsbPowerSystemUnspecified);

    Info->RhDevicePowerState = UcxMapDevicePowerState(RhCaps->DeviceState[State]);
    Info->HcDevicePowerState = UcxMapDevicePowerState(HcCaps->DeviceState[State]);

    if (State == PowerSystemWorking)
    {
        Info->CanWakeup = FALSE;
        Info->IsPowered = FALSE;
    }
    else
    {
        UcxMapWakeAndPower(HcCaps, State, Info);
    }
}

/* Constant figures, the controller driver is never asked */
static
VOID
NTAPI
UcxUserGetBandwidthInformation(
    _Inout_ UcxUserRequest& User)
{
    PUSB_BANDWIDTH_INFO Info = User.Payload<USB_BANDWIDTH_INFO>();

    RtlZeroMemory(Info, sizeof(*Info));
    Info->TotalBusBandwidth = UCX_BUS_BANDWIDTH;
    Info->Total32secBandwidth = UCX_BUS_BANDWIDTH * 32;
    Info->AllocedBulkAndControl = (UCX_BUS_BANDWIDTH / 5) * 32;
}

/** Uses the cached frame during a reset; failures report a counter that just moves on. */
static
VOID
NTAPI
UcxReadFrameForStatistics(
    _In_ UcxController* Controller,
    _Inout_ PULONG Frame)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    if (Controller->m_Config.EvtControllerGetCurrentFrameNumber != NULL)
        Status = Controller->GetCurrentFrameNumber(Frame);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Frame number read on controller %p failed 0x%lx, faking one\n", Controller, Status);
        Controller->m_SyntheticFrame = (Controller->m_SyntheticFrame + 1) & UCX_SYNTHETIC_FRAME_MASK;
        *Frame = Controller->m_SyntheticFrame;
    }
}

/* Unused and NameIndex keep whatever the caller sent */
static
VOID
NTAPI
UcxUserGetBusStatistics(
    _Inout_ UcxUserRequest& User,
    _In_ UcxController* Controller)
{
    PUSB_BUS_STATISTICS_0 Stats = User.Payload<USB_BUS_STATISTICS_0>();
    UcxRootHub* RootHub = Controller->m_RootHub;
    LARGE_INTEGER Now;
    ULONG Frame;

    Stats->DeviceCount = Controller->m_ChildDeviceCount;

    KeQuerySystemTime(&Now);
    RtlCopyMemory(&Stats->CurrentSystemTime, &Now, sizeof(Now));

    Frame = Stats->CurrentUsbFrame;
    UcxReadFrameForStatistics(Controller, &Frame);
    Stats->CurrentUsbFrame = Frame;

    Stats->BulkBytes = 0;
    Stats->IsoBytes = 0;
    Stats->InterruptBytes = 0;
    Stats->ControlDataBytes = 0;
    Stats->CommonBufferBytes = 0;

    /* Keeps tools from deciding the controller is stuck */
    Stats->PciInterruptCount = 1;

    Stats->HardResetCount = 0;
    Stats->WorkerSignalCount = 0;
    Stats->WorkerIdleTimeMs = 0;

    if (!RootHub->m_PdoStarted)
    {
        Stats->RootHubEnabled = FALSE;
        Stats->RootHubDevicePowerState = 4;
    }
    else
    {
        Stats->RootHubEnabled = TRUE;
        Stats->RootHubDevicePowerState = Controller->m_RootHubInD0 ? 0 : 2;
    }
}

/* USB_Version is left alone */
static
VOID
NTAPI
UcxUserGetDriverVersion(
    _Inout_ UcxUserRequest& User)
{
    PUSB_DRIVER_VERSION_PARAMETERS Version = User.Payload<USB_DRIVER_VERSION_PARAMETERS>();

    Version->DriverTrackingCode = UCX_DRIVER_TRACKING_CODE;
    Version->USBDI_Version = UCX_REPORTED_USBDI_VERSION;
    Version->USBUSER_Version = USBUSER_VERSION;
#if DBG
    Version->CheckedPortDriver = TRUE;
    Version->CheckedMiniportDriver = TRUE;
#else
    Version->CheckedPortDriver = FALSE;
    Version->CheckedMiniportDriver = FALSE;
#endif
}

/* Parameter block sizes the length check enforces */
struct UcxUserRequestSize
{
    ULONG Request;
    ULONG ParameterLength;
};

static const UcxUserRequestSize UcxUserRequestSizes[] =
{
    { USBUSER_GET_CONTROLLER_INFO_0,     sizeof(USB_CONTROLLER_INFO_0) },
    { USBUSER_GET_CONTROLLER_DRIVER_KEY, sizeof(USB_UNICODE_NAME) },
    { USBUSER_PASS_THRU,                 sizeof(USB_PASS_THRU_PARAMETERS) },
    { USBUSER_GET_POWER_STATE_MAP,       sizeof(USB_POWER_INFO) },
    { USBUSER_GET_BANDWIDTH_INFORMATION, sizeof(USB_BANDWIDTH_INFO) },
    { USBUSER_GET_BUS_STATISTICS_0,      sizeof(USB_BUS_STATISTICS_0) },
    { USBUSER_GET_ROOTHUB_SYMBOLIC_NAME, sizeof(USB_UNICODE_NAME) },
    { USBUSER_GET_USB_DRIVER_VERSION,    sizeof(USB_DRIVER_VERSION_PARAMETERS) },
    { USBUSER_USB_REFRESH_HCT_REG,       sizeof(ULONG) },
    { USBUSER_OP_SEND_ONE_PACKET,        sizeof(PACKET_PARAMETERS) },
    { USBUSER_OP_RAW_RESET_PORT,         sizeof(RAW_RESET_PORT_PARAMETERS) },
    { USBUSER_SET_ROOTPORT_FEATURE,      sizeof(RAW_ROOTPORT_FEATURE) },
    { USBUSER_CLEAR_ROOTPORT_FEATURE,    sizeof(RAW_ROOTPORT_FEATURE) },
    { USBUSER_GET_ROOTPORT_STATUS,       sizeof(RAW_ROOTPORT_FEATURE) }
};

/** Parameter size of a request UCX knows, or FALSE for anything else. */
static
BOOLEAN
NTAPI
UcxLookupUserRequestSize(
    _In_ ULONG Request,
    _Out_ PULONG ParameterLength)
{
    ULONG Index;

    for (Index = 0; Index < RTL_NUMBER_OF(UcxUserRequestSizes); Index++)
    {
        if (UcxUserRequestSizes[Index].Request == Request)
        {
            *ParameterLength = UcxUserRequestSizes[Index].ParameterLength;
            return TRUE;
        }
    }

    return FALSE;
}

static
VOID
NTAPI
UcxRunUserRequest(
    _Inout_ UcxUserRequest& User,
    _In_ WDFDEVICE Fdo,
    _In_ UcxController* Controller)
{
    ULONG ParameterLength;

    switch (User.Header()->UsbUserRequest)
    {
        case USBUSER_GET_CONTROLLER_INFO_0:
            UcxUserGetControllerInfo(User, Controller);
            break;

        case USBUSER_GET_CONTROLLER_DRIVER_KEY:
            UcxUserGetDriverKeyName(User, Fdo);
            break;

        case USBUSER_PASS_THRU:
            UcxUserPassThru(User);
            break;

        case USBUSER_GET_POWER_STATE_MAP:
            UcxUserGetPowerStateMap(User, Controller);
            break;

        case USBUSER_GET_BANDWIDTH_INFORMATION:
            UcxUserGetBandwidthInformation(User);
            break;

        case USBUSER_GET_BUS_STATISTICS_0:
            UcxUserGetBusStatistics(User, Controller);
            break;

        case USBUSER_GET_ROOTHUB_SYMBOLIC_NAME:
            UcxUserGetRootHubName(User, Controller->m_RootHub);
            break;

        case USBUSER_GET_USB_DRIVER_VERSION:
            UcxUserGetDriverVersion(User);
            break;

        /* Accepted and ignored */
        case USBUSER_USB_REFRESH_HCT_REG:
            break;

        /* Unlike the other test operations */
        case USBUSER_OP_SEND_ONE_PACKET:
            DPRINT1("USBUSER send one packet is not supported\n");
            User.SetStatus(UsbUserInvalidHeaderParameter);
            break;

        case USBUSER_OP_RAW_RESET_PORT:
        case USBUSER_SET_ROOTPORT_FEATURE:
        case USBUSER_CLEAR_ROOTPORT_FEATURE:
        case USBUSER_GET_ROOTPORT_STATUS:
            DPRINT1("USBUSER test request 0x%lx is disabled\n", User.Header()->UsbUserRequest);
            User.SetStatus(UsbUserFeatureDisabled);
            break;

        default:
            DPRINT1("Unknown USBUSER request 0x%lx\n", User.Header()->UsbUserRequest);
            NT_ASSERT(!UcxLookupUserRequestSize(User.Header()->UsbUserRequest, &ParameterLength));
            User.SetStatus(UsbUserInvalidRequestCode);
            break;
    }
}

static
VOID
NTAPI
UcxHandleUserRequest(
    _In_ WDFDEVICE Fdo,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength)
{
    UcxController* Controller = UcxController::FromFdo(Fdo);
    PUSBUSER_REQUEST_HEADER Header;
    ULONG ParameterLength;
    ULONG_PTR Information;

    if (InputBufferLength != OutputBufferLength)
    {
        DPRINT1("USBUSER IOCTL input length %Iu differs from output length %Iu\n", InputBufferLength, OutputBufferLength);
        WdfRequestCompleteWithInformation(Request, STATUS_INVALID_PARAMETER, 0);
        return;
    }

    if (!NT_SUCCESS(WdfRequestRetrieveOutputBuffer(Request, UCX_USER_HEADER_SIZE, (PVOID*)&Header, NULL)))
    {
        DPRINT1("USBUSER IOCTL buffer of %Iu bytes is too small\n", OutputBufferLength);
        WdfRequestCompleteWithInformation(Request, STATUS_BUFFER_TOO_SMALL, 0);
        return;
    }

    UcxUserRequest User(Header, (ULONG)OutputBufferLength);

    /* The request itself succeeds from here; problems go into the header */
    Header->UsbUserStatusCode = UsbUserSuccess;
    Header->ActualBufferLength = UCX_USER_HEADER_SIZE;

    if (Header->RequestBufferLength != OutputBufferLength)
    {
        DPRINT1("USBUSER header length %lu differs from buffer length %Iu\n",
                Header->RequestBufferLength, OutputBufferLength);
        User.SetStatus(UsbUserInvalidHeaderParameter);
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, UCX_USER_HEADER_SIZE);
        return;
    }

    /* A short parameter block reports the needed size but returns only the header */
    if (UcxLookupUserRequestSize(Header->UsbUserRequest, &ParameterLength) &&
        !User.CheckParameterLength(ParameterLength))
    {
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, UCX_USER_HEADER_SIZE);
        return;
    }

    UcxRunUserRequest(User, Fdo, Controller);

    Information = min(Header->RequestBufferLength, Header->ActualBufferLength);
    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, Information);
}

/* Legacy name IOCTLs */

/** Grows a private buffer to what the helper asks for. */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
UcxHandleLegacyName(
    _In_ WDFDEVICE Fdo,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ ULONG IoControlCode)
{
    UcxController* Controller = UcxController::FromFdo(Fdo);
    PUSBUSER_REQUEST_HEADER Block = NULL;
    PUSB_UNICODE_NAME Name;
    PUSB_ROOT_HUB_NAME Output;
    ULONG Size = UCX_USER_HEADER_SIZE + UCX_USER_NAME_MIN;
    ULONG Attempt;
    ULONG ActualLength;
    BOOLEAN Fits;

    PAGED_CODE();

    if (OutputBufferLength < sizeof(USB_ROOT_HUB_NAME) ||
        !NT_SUCCESS(WdfRequestRetrieveOutputBuffer(Request, OutputBufferLength, (PVOID*)&Output, NULL)))
    {
        DPRINT1("Name IOCTL 0x%lx buffer of %Iu bytes is too small\n", IoControlCode, OutputBufferLength);
        WdfRequestCompleteWithInformation(Request, STATUS_BUFFER_TOO_SMALL, 0);
        return;
    }

    for (Attempt = 0; Attempt < UCX_LEGACY_NAME_ATTEMPTS; Attempt++)
    {
        Block = (PUSBUSER_REQUEST_HEADER)ExAllocatePoolWithTag(PagedPool, Size, UCX_POOL_TAG);
        if (Block == NULL)
        {
            DPRINT1("Name IOCTL 0x%lx could not allocate %lu bytes\n", IoControlCode, Size);
            WdfRequestCompleteWithInformation(Request, STATUS_INSUFFICIENT_RESOURCES, 0);
            return;
        }

        RtlZeroMemory(Block, Size);
        Block->RequestBufferLength = Size;

        UcxUserRequest User(Block, Size);

        if (IoControlCode == IOCTL_GET_HCD_DRIVERKEY_NAME)
        {
            Block->UsbUserRequest = USBUSER_GET_CONTROLLER_DRIVER_KEY;
            UcxUserGetDriverKeyName(User, Fdo);
        }
        else
        {
            Block->UsbUserRequest = USBUSER_GET_ROOTHUB_SYMBOLIC_NAME;
            UcxUserGetRootHubName(User, Controller->m_RootHub);
        }

        if (Block->UsbUserStatusCode != UsbUserBufferTooSmall)
            break;

        Size = Block->ActualBufferLength;
        ExFreePoolWithTag(Block, UCX_POOL_TAG);
        Block = NULL;
    }

    if (Block == NULL || Block->UsbUserStatusCode != UsbUserSuccess)
    {
        DPRINT1("Name IOCTL 0x%lx failed, USBUSER status %d\n",
                IoControlCode, Block != NULL ? (INT)Block->UsbUserStatusCode : -1);

        if (Block != NULL)
            ExFreePoolWithTag(Block, UCX_POOL_TAG);

        WdfRequestCompleteWithInformation(Request, STATUS_UNSUCCESSFUL, 0);
        return;
    }

    Name = (PUSB_UNICODE_NAME)(Block + 1);

    /* An overflowing length never fits */
    if (Name->Length > MAXULONG - FIELD_OFFSET(USB_ROOT_HUB_NAME, RootHubName))
    {
        ActualLength = MAXULONG;
        Fits = FALSE;
    }
    else
    {
        ActualLength = Name->Length + FIELD_OFFSET(USB_ROOT_HUB_NAME, RootHubName);
        Fits = OutputBufferLength >= ActualLength;
    }

    Output->ActualLength = ActualLength;

    if (Fits)
    {
        RtlCopyMemory(Output->RootHubName, Name->String, Name->Length);
    }
    else
    {
        DPRINT("Name IOCTL 0x%lx needs %lu bytes, got %Iu\n", IoControlCode, ActualLength, OutputBufferLength);
        Output->RootHubName[0] = UNICODE_NULL;
        ActualLength = sizeof(USB_ROOT_HUB_NAME);
    }

    ExFreePoolWithTag(Block, UCX_POOL_TAG);
    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, ActualLength);
}

BOOLEAN
NTAPI
UcxDispatchUserIoctl(
    _In_ WDFDEVICE Fdo,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    switch (IoControlCode)
    {
        /* Accepted, nothing changes */
        case IOCTL_USB_DIAGNOSTIC_MODE_ON:
        case IOCTL_USB_DIAGNOSTIC_MODE_OFF:
            WdfRequestComplete(Request, STATUS_SUCCESS);
            return TRUE;

        case IOCTL_USB_GET_ROOT_HUB_NAME:
        case IOCTL_GET_HCD_DRIVERKEY_NAME:
            UcxHandleLegacyName(Fdo, Request, OutputBufferLength, IoControlCode);
            return TRUE;

        case IOCTL_USB_USER_REQUEST:
            UcxHandleUserRequest(Fdo, Request, OutputBufferLength, InputBufferLength);
            return TRUE;

        default:
            return FALSE;
    }
}

/* WMI node info */

C_ASSERT(FIELD_OFFSET(USB_DEVICE_NODE_INFO, NodeType) == 88);
C_ASSERT(FIELD_OFFSET(USB_DEVICE_NODE_INFO, BusAddress) == 92);
C_ASSERT(FIELD_OFFSET(USB_DEVICE_NODE_INFO, ControllerDeviceInfo) == 124);
C_ASSERT(sizeof(USB_DEVICE_NODE_INFO) == 1202);

/* The ANSI description goes into the WCHAR field byte for byte */
NTSTATUS
NTAPI
UcxEvtWmiNodeInfoQueryInstance(
    _In_ WDFWMIINSTANCE WmiInstance,
    _In_ ULONG OutBufferSize,
    _Out_writes_bytes_to_(OutBufferSize, *BufferUsed) PVOID OutBuffer,
    _Out_ PULONG BufferUsed)
{
    UcxController* Controller = UcxController::FromFdo(WdfWmiInstanceGetDevice(WmiInstance));
    const UCX_CONTROLLER_PCI_INFORMATION* Pci = &Controller->m_Config.PciDeviceInfo;
    PUSB_DEVICE_NODE_INFO Node = (PUSB_DEVICE_NODE_INFO)OutBuffer;
    UcxRootHub* RootHub = Controller->m_RootHub;

    *BufferUsed = sizeof(*Node);

    if (OutBufferSize < sizeof(*Node))
    {
        DPRINT1("WMI node info buffer of %lu bytes is too small\n", OutBufferSize);
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlZeroMemory(OutBuffer, OutBufferSize);

    Node->Sig = UCX_NODE_INFO_SIGNATURE;
    Node->LengthInBytes = sizeof(*Node);
    RtlStringCbCopyA((PSTR)Node->DeviceDescription,
                     sizeof(Node->DeviceDescription),
                     (PCSTR)Controller->m_Config.DeviceDescription);

    Node->NodeType = UsbController;
    Node->BusAddress.PciBusNumber = Pci->BusNumber;
    Node->BusAddress.PciDeviceNumber = Pci->DeviceNumber;
    Node->BusAddress.PciFunctionNumber = Pci->FunctionNumber;

    Node->ControllerDeviceInfo.PciVendorId = Pci->VendorId;
    Node->ControllerDeviceInfo.PciDeviceId = Pci->DeviceId;
    Node->ControllerDeviceInfo.PciRevision = Pci->RevisionId;
    Node->ControllerDeviceInfo.NumberOfRootPorts = RootHub->m_NumberOf20Ports + RootHub->m_NumberOf30Ports;
    Node->ControllerDeviceInfo.HcFeatureFlags = USB_HC_FEATURE_FLAG_SEL_SUSPEND;

    return STATUS_SUCCESS;
}
