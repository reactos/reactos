/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User mode and internal IOCTLs on the hub FDO queue
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"
#include "devreq.h"

#define NDEBUG
#include <debug.h>

/* ntifs.h only */
extern "C"
NTKERNELAPI
BOOLEAN
NTAPI
SeTokenIsAdmin(
    _In_ PACCESS_TOKEN Token);

/* Fixed parts of the user mode structures; our usbioctl.h leaves the USB 3 ones unpacked */
#define HUB_IO_NODE_INFO_SIZE       sizeof(USB_NODE_INFORMATION)
#define HUB_IO_CONNECTION_SIZE      FIELD_OFFSET(USB_NODE_CONNECTION_INFORMATION, PipeList)
#define HUB_IO_NAME_HEADER          FIELD_OFFSET(USB_NODE_CONNECTION_NAME, NodeName)
#define HUB_IO_NAME_SIZE            sizeof(USB_NODE_CONNECTION_NAME)
#define HUB_IO_DESCRIPTOR_HEADER    FIELD_OFFSET(USB_DESCRIPTOR_REQUEST, Data)
#define HUB_IO_ATTRIBUTES_SIZE      sizeof(USB_NODE_CONNECTION_ATTRIBUTES)
#define HUB_IO_HUB_INFO_EX_SIZE     (FIELD_OFFSET(USB_HUB_INFORMATION_EX, u) + sizeof(USB_HUB_DESCRIPTOR))
#define HUB_IO_CONNECTOR_SIZE       (FIELD_OFFSET(USB_PORT_CONNECTOR_PROPERTIES, CompanionHubSymbolicLinkName) + sizeof(WCHAR))
#define HUB_IO_CONNECTION_V2_SIZE   sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2)
#define HUB_IO_CYCLE_SIZE           sizeof(USB_CYCLE_PORT_PARAMS)

C_ASSERT(HUB_IO_NODE_INFO_SIZE == 76);
C_ASSERT(HUB_IO_CONNECTION_SIZE == 35);
C_ASSERT(sizeof(USB_PIPE_INFO) == 11);
C_ASSERT(HUB_IO_NAME_SIZE == 10);
C_ASSERT(sizeof(USB_NODE_CONNECTION_DRIVERKEY_NAME) == HUB_IO_NAME_SIZE);
C_ASSERT(HUB_IO_DESCRIPTOR_HEADER == 12);
C_ASSERT(HUB_IO_ATTRIBUTES_SIZE == 12);
C_ASSERT(HUB_IO_HUB_INFO_EX_SIZE == 77);
C_ASSERT(HUB_IO_CONNECTOR_SIZE == 18);
C_ASSERT(HUB_IO_CONNECTION_V2_SIZE == 16);
C_ASSERT(HUB_IO_CYCLE_SIZE == 8);

/* MaxPortPower of a bus powered hub, in mA */
#define HUB_BUS_POWERED_PORT_POWER  100

/* _UPC connector types of a Type-C connector */
#define HUB_UPC_TYPE_C_USB2         0x08
#define HUB_UPC_TYPE_C_SWITCH       0x09
#define HUB_UPC_TYPE_C_NO_SWITCH    0x0A

/* String descriptors are answered from the cache only in US English */
#define HUB_LANGID_US_ENGLISH       0x0409

/* IOCTL_USB_GET_PORT_STATUS, private to usbhub3 */
struct HubUserPortStatus
{
    ULONG ConnectionIndex;
    USHORT PortStatus;
    USHORT Reserved;
};

C_ASSERT(sizeof(HubUserPortStatus) == 8);

static const char HubTagUserRequest[] = "User Mode FDO Request";

/* Shared helpers */

/** STATUS_SUCCESS when the caller's token is an administrator's. */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubIoCallerIsAdmin(VOID)
{
    SECURITY_SUBJECT_CONTEXT Subject;
    PACCESS_TOKEN Token;
    BOOLEAN Admin;

    PAGED_CODE();

    SeCaptureSubjectContext(&Subject);
    SeLockSubjectContext(&Subject);

    Token = (Subject.ClientToken != NULL) ? Subject.ClientToken : Subject.PrimaryToken;
    Admin = (Token != NULL) && SeTokenIsAdmin(Token);

    SeUnlockSubjectContext(&Subject);
    SeReleaseSubjectContext(&Subject);

    return Admin ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

/** Skips a leading "\xx\" of a symbolic link; works on the counted length only. */
static
PWCHAR
NTAPI
HubIoStripLinkPrefix(
    _In_ PCUNICODE_STRING Link,
    _Out_ PULONG Bytes)
{
    PWCHAR Cursor = Link->Buffer;
    PWCHAR End;

    *Bytes = 0;
    if (Cursor == NULL)
        return NULL;

    End = Cursor + Link->Length / sizeof(WCHAR);

    if (Cursor < End && *Cursor == L'\\')
    {
        for (Cursor++; Cursor < End; Cursor++)
        {
            if (*Cursor == L'\\' || *Cursor == UNICODE_NULL)
                break;
        }

        if (Cursor < End && *Cursor == L'\\')
            Cursor++;
    }

    *Bytes = (ULONG)((End - Cursor) * sizeof(WCHAR));
    return Cursor;
}

/** Common IOCTL checks; Buffer holds the input when MinInput covers a port number. */
static
NTSTATUS
NTAPI
HubIoValidate(
    _In_ HubFdo* Hub,
    _In_ size_t MinInput,
    _In_ size_t InputLength,
    _In_opt_ PVOID Buffer,
    _In_ size_t MinOutput,
    _In_ size_t OutputLength)
{
    ULONG PortNumber;

    if (!Hub->HasFlag(HubFlag::Configured))
    {
        DPRINT1("Hub %p is not configured, user request refused\n", Hub);
        return STATUS_UNSUCCESSFUL;
    }

    if (InputLength < MinInput)
    {
        DPRINT1("Hub %p user request input %lu bytes, needs %lu\n", Hub, (ULONG)InputLength, (ULONG)MinInput);
        return STATUS_INVALID_PARAMETER;
    }

    if (OutputLength < MinOutput)
    {
        DPRINT1("Hub %p user request output %lu bytes, needs %lu\n", Hub, (ULONG)OutputLength, (ULONG)MinOutput);
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (MinInput >= sizeof(ULONG))
    {
        PortNumber = *(PULONG)Buffer;
        if (PortNumber == 0 || PortNumber > Hub->m_PortCount)
        {
            DPRINT1("Hub %p user request for invalid port %lu\n", Hub, PortNumber);
            return STATUS_INVALID_PARAMETER;
        }
    }

    return STATUS_SUCCESS;
}

/** Fetches the shared system buffer, then validates. Buffer is NULL only when the fetch failed. */
static
NTSTATUS
NTAPI
HubIoPrepare(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request,
    _In_ size_t MinInput,
    _In_ size_t InputLength,
    _In_ size_t MinOutput,
    _In_ size_t OutputLength,
    _Outptr_result_maybenull_ PVOID* Buffer)
{
    NTSTATUS Status;

    Status = WdfRequestRetrieveOutputBuffer(Request, OutputLength, Buffer, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p request %p has no output buffer 0x%lx\n", Hub, Request, Status);
        *Buffer = NULL;
        return Status;
    }

    return HubIoValidate(Hub, MinInput, InputLength, *Buffer, MinOutput, OutputLength);
}

/** Locks the static child list and finds the PDO on a port. Unlock with HubIoUnlockChildren on every path. */
static
HubPdo*
NTAPI
HubIoLockChild(
    _In_ HubFdo* Hub,
    _In_ USHORT PortNumber)
{
    WDFDEVICE Device = NULL;
    HubPdo* Pdo;

    WdfFdoLockStaticChildListForIteration(Hub->m_Device);

    while ((Device = WdfFdoRetrieveNextStaticChild(Hub->m_Device, Device, WdfRetrievePresentChildren)) != NULL)
    {
        Pdo = HubGetPdoContext(Device);
        if (Pdo->m_PortNumber == PortNumber)
            return Pdo;
    }

    return NULL;
}

static
VOID
NTAPI
HubIoUnlockChildren(
    _In_ HubFdo* Hub)
{
    WdfFdoUnlockStaticChildListFromIteration(Hub->m_Device);
}

static
VOID
NTAPI
HubIoForward(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request)
{
    WDF_REQUEST_SEND_OPTIONS Options;

    WdfRequestFormatRequestUsingCurrentType(Request);
    WDF_REQUEST_SEND_OPTIONS_INIT(&Options, WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

    if (!WdfRequestSend(Request, WdfDeviceGetIoTarget(Hub->m_Device), &Options))
    {
        DPRINT1("Hub %p forwarding request %p failed 0x%lx\n", Hub, Request, WdfRequestGetStatus(Request));
        WdfRequestComplete(Request, WdfRequestGetStatus(Request));
    }
}

/** A USB 2.0 hub descriptor for any hub, as the legacy node information reports it. */
static
VOID
NTAPI
HubIoBuild20Descriptor(
    _In_ HubFdo* Hub,
    _Out_ PUSB_HUB_DESCRIPTOR Descriptor)
{
    ULONG MaskBytes;

    RtlZeroMemory(Descriptor, sizeof(*Descriptor));

    if (!Hub->IsRootHub())
    {
        if (Hub->m_Parent.HubSpeed == UsbHighSpeed || Hub->m_Parent.HubSpeed == UsbFullSpeed)
        {
            RtlCopyMemory(Descriptor, &Hub->m_HubDescriptor.Usb20, sizeof(*Descriptor));
            return;
        }

        if (Hub->m_Parent.HubSpeed != UsbSuperSpeed)
            return;

        Descriptor->wHubCharacteristics = Hub->m_HubDescriptor.Usb30.wHubCharacteristics & 0x1F;
        Descriptor->bPowerOnToPowerGood = Hub->m_HubDescriptor.Usb30.bPowerOnToPowerGood;
        Descriptor->bHubControlCurrent = Hub->m_HubDescriptor.Usb30.bHubControlCurrent;
    }
    else
    {
        Descriptor->bPowerOnToPowerGood = 2;
    }

    /* QUIRK: the length stays 9 even when the port masks need more room */
    Descriptor->bDescriptorLength = 9;
    Descriptor->bDescriptorType = USB_20_HUB_DESCRIPTOR_TYPE;
    Descriptor->bNumberOfPorts = (UCHAR)Hub->m_PortCount;

    /* Every port removable, then the legacy power control mask all ones */
    MaskBytes = Hub->m_PortCount / 8 + 1;
    RtlFillMemory(&Descriptor->bRemoveAndPowerMask[MaskBytes], MaskBytes, 0xFF);
}

/** TRUE when the first port attribute with this id and direction is a SuperSpeedPlus one. */
static
BOOLEAN
NTAPI
HubIoPortSpeedIsPlus(
    _In_ HubPort* Port,
    _In_ ULONG SpeedId,
    _In_ ULONG Direction)
{
    const USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED* Attribute;
    ULONG Index;

    for (Index = 0; Index < Port->m_Info.SublinkSpeedAttrCount; Index++)
    {
        Attribute = &Port->m_Info.SublinkSpeedAttr[Index];
        if (Attribute->SublinkSpeedAttrID == SpeedId && Attribute->SublinkTypeDir == Direction)
            return Attribute->LinkProtocol != USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED_PROTOCOL_SS;
    }

    return FALSE;
}

/* The device has a SuperSpeedPlus capability exactly when its sublink speeds were cached */
static
BOOLEAN
NTAPI
HubIoDeviceSspCapable(
    _In_ HubChild* Child)
{
    ULONG Index;

    if (Child->m_SublinkSpeedAttr == NULL)
        return FALSE;

    for (Index = 0; Index < Child->m_SublinkSpeedAttrCount; Index++)
    {
        if (Child->m_SublinkSpeedAttr[Index].LinkProtocol != USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED_PROTOCOL_SS)
            return TRUE;
    }

    return FALSE;
}

static
BOOLEAN
NTAPI
HubIoDeviceRunsSsp(
    _In_ HubChild* Child)
{
    HubPort* Port = Child->m_Port;
    USB_PORT_EXT_STATUS Extended;

    if (Child->m_SublinkSpeedAttr == NULL || !Port->HasProperty(PortProperty::EnhancedSuperSpeed))
        return FALSE;

    Extended.AsUlong32 = Port->m_Current.ExtendedStatus;

    /* QUIRK: the Tx lookup also uses the Rx sublink speed id */
    return HubIoPortSpeedIsPlus(Port, Extended.RxSublinkSpeedID, USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED_DIR_RX) ||
           HubIoPortSpeedIsPlus(Port, Extended.RxSublinkSpeedID, USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED_DIR_TX);
}

/* Section 6.8a without the device reference */
static
VOID
NTAPI
HubIoCompleteDescriptor(
    _In_ WDFREQUEST Request,
    _In_ NTSTATUS Status,
    _In_ ULONG Size)
{
    if (NT_SUCCESS(Status))
        WdfRequestCompleteWithInformation(Request, Status, Size + HUB_IO_DESCRIPTOR_HEADER);
    else
        WdfRequestComplete(Request, Status);
}

VOID
NTAPI
HubCompleteFdoDescriptorRequest(
    _In_ HubChild* Child,
    _In_ WDFREQUEST Request,
    _In_ NTSTATUS Status,
    _In_ ULONG Size)
{
    HubIoCompleteDescriptor(Request, Status, Size);
    WdfObjectDereferenceWithTag(Child->m_Object, (PVOID)HubTagUserRequest);
}

VOID
DeviceMachine::FailHubDescriptorRequest()
{
    HubChild* Child = m_Device;
    WDFREQUEST Request = Child->m_FdoRequest;

    Child->m_FdoRequest = NULL;
    DPRINT1("Device %p descriptor request %p failed by the device machine\n", Child, Request);
    HubCompleteFdoDescriptorRequest(Child, Request, STATUS_UNSUCCESSFUL, 0);
}

/* Handlers */

static
VOID
NTAPI
HubIoGetPortStatus(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputLength,
    _In_ size_t InputLength)
{
    HubUserPortStatus* Info;
    ULONG Index;
    HubPort* Port;
    NTSTATUS Status;

    Status = HubIoPrepare(Hub, Request, sizeof(ULONG), InputLength, sizeof(*Info), OutputLength, (PVOID*)&Info);
    if (!NT_SUCCESS(Status))
    {
        WdfRequestComplete(Request, Status);
        return;
    }

    Index = Info->ConnectionIndex;
    RtlZeroMemory(Info, OutputLength);
    Info->ConnectionIndex = Index;

    Port = Hub->FindPort((USHORT)Index);
    if (Port == NULL)
    {
        DPRINT1("Hub %p port status: no port %lu\n", Hub, Index);
        WdfRequestCompleteWithInformation(Request, STATUS_INVALID_PARAMETER, sizeof(*Info));
        return;
    }

    /* The hub machine completes it through ReplyPortStatus or FailPortStatus */
    Hub->m_PortStatusTarget = Port;
    Hub->m_PortStatusRequest = Request;
    Hub->Post(HubEvent::PortStatusRequest);
}

static
VOID
NTAPI
HubIoGetNodeInformation(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputLength,
    _In_ size_t InputLength)
{
    PUSB_NODE_INFORMATION Info;
    NTSTATUS Status;

    Status = HubIoValidate(Hub, 0, InputLength, NULL, HUB_IO_NODE_INFO_SIZE, OutputLength);
    if (NT_SUCCESS(Status))
    {
        Status = WdfRequestRetrieveOutputBuffer(Request, OutputLength, (PVOID*)&Info, NULL);
        if (!NT_SUCCESS(Status))
            DPRINT1("Hub %p node information has no output buffer 0x%lx\n", Hub, Status);
    }

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(Info, OutputLength);
        Info->NodeType = UsbHub;
        Info->u.HubInformation.HubIsBusPowered = (Hub->m_MaxPortPower == HUB_BUS_POWERED_PORT_POWER);
        HubIoBuild20Descriptor(Hub, &Info->u.HubInformation.HubDescriptor);
    }

    /* QUIRK: the size is reported on failure too */
    WdfRequestCompleteWithInformation(Request, Status, HUB_IO_NODE_INFO_SIZE);
}

static
VOID
NTAPI
HubIoGetDriverKeyName(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputLength,
    _In_ size_t InputLength)
{
    PUSB_NODE_CONNECTION_DRIVERKEY_NAME Name;
    ULONG Index;
    ULONG Returned = 0;
    HubPdo* Pdo;
    NTSTATUS Status;

    Status = HubIoPrepare(Hub, Request, sizeof(ULONG), InputLength, HUB_IO_NAME_SIZE, OutputLength, (PVOID*)&Name);
    if (!NT_SUCCESS(Status))
        goto Fail;

    Index = Name->ConnectionIndex;

    Pdo = HubIoLockChild(Hub, (USHORT)Index);
    if (Pdo == NULL)
    {
        HubIoUnlockChildren(Hub);
        DPRINT1("Hub %p driver key name: no device on port %lu\n", Hub, Index);
        Status = STATUS_INVALID_PARAMETER;
        goto Fail;
    }

    RtlZeroMemory(Name, OutputLength);
    Name->ConnectionIndex = Index;

    Status = WdfDeviceQueryProperty(Pdo->m_Device,
                                    DevicePropertyDriverKeyName,
                                    (ULONG)(OutputLength - HUB_IO_NAME_HEADER),
                                    Name->DriverKeyName,
                                    &Returned);
    HubIoUnlockChildren(Hub);

    /* A name that does not fit still reports its size */
    if (Status == STATUS_BUFFER_TOO_SMALL)
        Status = STATUS_SUCCESS;

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p driver key name of port %lu failed 0x%lx\n", Hub, Index, Status);
        goto Fail;
    }

    Name->ActualLength = Returned + HUB_IO_NAME_SIZE;
    if (OutputLength >= Name->ActualLength)
    {
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, Name->ActualLength);
        return;
    }

    /* A name that does not fit is returned empty */
    Name->DriverKeyName[0] = UNICODE_NULL;
    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, HUB_IO_NAME_SIZE);
    return;

Fail:
    /* Some callers read ActualLength without looking at the status */
    if (Name != NULL && OutputLength >= HUB_IO_NAME_SIZE)
    {
        Name->DriverKeyName[0] = UNICODE_NULL;
        Name->ActualLength = HUB_IO_NAME_SIZE;
    }

    WdfRequestComplete(Request, Status);
}

static
VOID
NTAPI
HubIoGetConnectionInformation(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputLength,
    _In_ size_t InputLength,
    _In_ BOOLEAN Extended)
{
    PUSB_NODE_CONNECTION_INFORMATION_EX Info;
    PUSB_PIPE_INFO Pipe;
    HubConfiguration* Config;
    HubInterface* Interface;
    PLIST_ENTRY Entry;
    USB_DEVICE_SPEED Speed;
    USHORT PortNumber;
    HubPort* Port;
    HubPdo* Pdo;
    HubChild* Child;
    size_t Remaining;
    size_t Information;
    ULONG Index;
    KIRQL Irql;
    NTSTATUS Status;

    Status = HubIoPrepare(Hub, Request, sizeof(ULONG), InputLength, HUB_IO_CONNECTION_SIZE, OutputLength, (PVOID*)&Info);
    if (!NT_SUCCESS(Status))
    {
        WdfRequestComplete(Request, Status);
        return;
    }

    PortNumber = (USHORT)Info->ConnectionIndex;
    RtlZeroMemory(Info, OutputLength);

    Port = Hub->FindPort(PortNumber);
    Info->ConnectionStatus = (Port != NULL) ? Port->m_ConnectionStatus : DeviceGeneralFailure;

    Pdo = HubIoLockChild(Hub, PortNumber);
    Child = (Pdo != NULL) ? Pdo->m_Child : NULL;
    if (Child == NULL)
    {
        /* QUIRK: ConnectionIndex stays 0 for an empty port */
        HubIoUnlockChildren(Hub);
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, HUB_IO_CONNECTION_SIZE);
        return;
    }

    Info->ConnectionIndex = PortNumber;
    RtlCopyMemory(&Info->DeviceDescriptor, &Child->m_DeviceDescriptor, sizeof(Info->DeviceDescriptor));

    Speed = Child->Speed();
    if (Extended)
    {
        /* Callers of this form predate SuperSpeed */
        Info->Speed = (UCHAR)((Speed == UsbSuperSpeed) ? UsbHighSpeed : Speed);
    }
    else
    {
        ((PUSB_NODE_CONNECTION_INFORMATION)Info)->LowSpeed = (Speed == UsbLowSpeed);
    }

    Info->DeviceAddress = Child->m_Address;
    Info->DeviceIsHub = Child->HasProperty(ChildProperty::IsHub);

    Information = HUB_IO_CONNECTION_SIZE;

    KeAcquireSpinLock(&Child->m_ConfigLock, &Irql);

    Config = Child->m_CurrentConfig;
    if (Child->HasState(ChildState::ConfigurationValid) && Config != NULL)
    {
        Info->CurrentConfigurationValue = Config->Descriptor.bConfigurationValue;

        /* QUIRK: the full count even when fewer pipes fit */
        Info->NumberOfOpenPipes = Config->EndpointCount;

        Remaining = OutputLength - HUB_IO_CONNECTION_SIZE;
        Pipe = &Info->PipeList[0];

        for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
        {
            Interface = CONTAINING_RECORD(Entry, HubInterface, Link);

            for (Index = 0; Index < Interface->PipeCount && Remaining >= sizeof(*Pipe); Index++)
            {
                RtlCopyMemory(&Pipe->EndpointDescriptor,
                              Interface->Pipes[Index].Descriptor,
                              sizeof(Pipe->EndpointDescriptor));
                Pipe->ScheduleOffset = 0;
                Pipe++;
                Remaining -= sizeof(*Pipe);
            }
        }

        Information = OutputLength - Remaining;
    }

    KeReleaseSpinLock(&Child->m_ConfigLock, Irql);
    HubIoUnlockChildren(Hub);

    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, Information);
}

static
VOID
NTAPI
HubIoGetConnectionName(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputLength,
    _In_ size_t InputLength)
{
    PUSB_NODE_CONNECTION_NAME Name;
    WDF_OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING Path;
    WDFSTRING String;
    PWCHAR Text;
    ULONG TextBytes;
    ULONG Index;
    HubPdo* Pdo;
    HubChild* Child;
    NTSTATUS Status;

    Status = HubIoPrepare(Hub, Request, sizeof(ULONG), InputLength, HUB_IO_NAME_SIZE, OutputLength, (PVOID*)&Name);
    if (!NT_SUCCESS(Status))
    {
        WdfRequestComplete(Request, Status);
        return;
    }

    Index = Name->ConnectionIndex;

    Pdo = HubIoLockChild(Hub, (USHORT)Index);
    Child = (Pdo != NULL) ? Pdo->m_Child : NULL;
    if (Child == NULL || !Child->HasProperty(ChildProperty::IsHub))
    {
        /* QUIRK: the rest of the buffer is left as the caller sent it */
        HubIoUnlockChildren(Hub);
        Name->ActualLength = HUB_IO_NAME_SIZE;
        Name->NodeName[0] = UNICODE_NULL;
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, HUB_IO_NAME_SIZE);
        return;
    }

    RtlZeroMemory(Name, OutputLength);
    Name->ConnectionIndex = Index;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Pdo->m_Device;

    Status = WdfStringCreate(NULL, &Attributes, &String);
    if (NT_SUCCESS(Status))
    {
        Status = WdfDeviceRetrieveDeviceInterfaceString(Pdo->m_Device, &GUID_DEVINTERFACE_USB_HUB, NULL, String);
        if (!NT_SUCCESS(Status))
            WdfObjectDelete(String);
    }

    HubIoUnlockChildren(Hub);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p connection name of port %lu failed 0x%lx\n", Hub, Index, Status);
        WdfRequestComplete(Request, Status);
        return;
    }

    WdfStringGetUnicodeString(String, &Path);
    Text = HubIoStripLinkPrefix(&Path, &TextBytes);
    Name->ActualLength = TextBytes + HUB_IO_NAME_SIZE;

    if (TextBytes <= OutputLength - HUB_IO_NAME_HEADER)
    {
        RtlCopyMemory(Name->NodeName, Text, TextBytes);

        /* ActualLength can exceed the caller's buffer by two bytes */
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, min((size_t)Name->ActualLength, OutputLength));
    }
    else
    {
        Name->NodeName[0] = UNICODE_NULL;
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, HUB_IO_NAME_SIZE);
    }

    WdfObjectDelete(String);
}

static
VOID
NTAPI
HubIoGetDescriptor(
    _In_ HubFdo* Hub,
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputLength,
    _In_ size_t InputLength)
{
    PUSB_DESCRIPTOR_REQUEST Descriptor;
    struct _URB_CONTROL_TRANSFER_EX* Urb;
    IO_STACK_LOCATION Stack;
    PCWSTR Serial;
    ULONG SerialBytes;
    ULONG DataLength;
    ULONG Size = 0;
    UCHAR Type;
    UCHAR Index;
    HubPdo* Pdo;
    HubChild* Child;
    NTSTATUS Status;

    Status = HubIoPrepare(Hub, Request, sizeof(ULONG), InputLength, HUB_IO_DESCRIPTOR_HEADER + 1, OutputLength, (PVOID*)&Descriptor);
    if (!NT_SUCCESS(Status))
    {
        HubIoCompleteDescriptor(Request, Status, 0);
        return;
    }

    DataLength = (ULONG)(OutputLength - HUB_IO_DESCRIPTOR_HEADER);
    RtlZeroMemory(Descriptor->Data, DataLength);

    Pdo = HubIoLockChild(Hub, (USHORT)Descriptor->ConnectionIndex);
    Child = (Pdo != NULL) ? Pdo->m_Child : NULL;
    if (Child == NULL || !Child->HasState(ChildState::WorkingDevice))
    {
        HubIoUnlockChildren(Hub);
        DPRINT1("Hub %p descriptor request: no known device on port %lu\n", Hub, Descriptor->ConnectionIndex);
        HubIoCompleteDescriptor(Request, STATUS_NO_SUCH_DEVICE, 0);
        return;
    }

    /* Only a standard GET_DESCRIPTOR goes through, whatever the caller asked */
    Descriptor->SetupPacket.bmRequest = (BMREQUEST_DEVICE_TO_HOST << 7) | (BMREQUEST_STANDARD << 5) | BMREQUEST_TO_DEVICE;
    Descriptor->SetupPacket.bRequest = USB_REQUEST_GET_DESCRIPTOR;

    Type = HIBYTE(Descriptor->SetupPacket.wValue);
    Index = LOBYTE(Descriptor->SetupPacket.wValue);

    DPRINT("Hub %p descriptor request port %lu type %u index %u\n", Hub, Descriptor->ConnectionIndex, Type, Index);

    switch (Type)
    {
        case USB_DEVICE_DESCRIPTOR_TYPE:
            Size = min((ULONG)sizeof(Child->m_DeviceDescriptor), DataLength);
            RtlCopyMemory(Descriptor->Data, &Child->m_DeviceDescriptor, Size);
            goto Cached;

        case USB_CONFIGURATION_DESCRIPTOR_TYPE:
            if (Child->m_ConfigDescriptor == NULL || Index != 0)
                break;

            Size = min((ULONG)Child->m_ConfigDescriptor->wTotalLength, DataLength);
            RtlCopyMemory(Descriptor->Data, Child->m_ConfigDescriptor, Size);
            goto Cached;

        case USB_BOS_DESCRIPTOR_TYPE:
            if (Child->m_Bos == NULL)
                break;

            Size = min((ULONG)Child->m_Bos->wTotalLength, DataLength);
            RtlCopyMemory(Descriptor->Data, Child->m_Bos, Size);
            goto Cached;

        case USB_STRING_DESCRIPTOR_TYPE:
            if (Index == 0 ||
                Index != Child->m_DeviceDescriptor.iSerialNumber ||
                Descriptor->SetupPacket.wIndex != HUB_LANGID_US_ENGLISH)
            {
                break;
            }

            if (!HubIdGetSerialNumberText(Child, &Serial, &SerialBytes))
            {
                HubIoUnlockChildren(Hub);
                DPRINT1("Hub %p device %p has no stored serial number\n", Hub, Child);
                HubIoCompleteDescriptor(Request, STATUS_UNSUCCESSFUL, 0);
                return;
            }

            /* QUIRK: answered from the cache, without the MSFT decoration */
            Size = min(SerialBytes + (ULONG)sizeof(WCHAR), DataLength);
            if (Size > Descriptor->SetupPacket.wLength)
                break;

            Descriptor->Data[0] = (UCHAR)Size;
            if (Size >= sizeof(WCHAR))
            {
                Descriptor->Data[1] = USB_STRING_DESCRIPTOR_TYPE;
                RtlCopyMemory(&Descriptor->Data[2], Serial, Size - sizeof(WCHAR));
            }
            goto Cached;

        default:
            break;
    }

    /* QUIRK: wLength is sent as given, even when it exceeds the buffer */
    Urb = &HubGetQueueContext(Queue)->Urb;
    RtlZeroMemory(Urb, sizeof(*Urb));
    RtlCopyMemory(Urb->SetupPacket, &Descriptor->SetupPacket, sizeof(Urb->SetupPacket));
    Urb->Hdr.Length = sizeof(*Urb);
    Urb->Hdr.Function = URB_FUNCTION_CONTROL_TRANSFER_EX;
    Urb->Hdr.UsbdDeviceHandle = Child->m_UsbDevice;
    Urb->TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK | USBD_DEFAULT_PIPE_TRANSFER;
    Urb->TransferBufferLength = DataLength;
    Urb->TransferBuffer = Descriptor->Data;
    Urb->Timeout = HUB_SETUP_REQUEST_TIMEOUT_MS;

    /* Dropped by HubCompleteFdoDescriptorRequest */
    WdfObjectReferenceWithTag(Child->m_Object, (PVOID)HubTagUserRequest);
    HubIoUnlockChildren(Hub);

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    Stack.Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    Stack.Parameters.Others.Argument1 = Urb;
    WdfRequestWdmFormatUsingStackLocation(Request, &Stack);

    Child->m_FdoRequest = Request;
    Child->Post(DsmEvent::HubGetDescriptor);
    return;

Cached:
    HubIoUnlockChildren(Hub);
    HubIoCompleteDescriptor(Request, STATUS_SUCCESS, Size);
}

static
VOID
NTAPI
HubIoGetHubCapabilities(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputLength,
    _In_ size_t InputLength,
    _In_ BOOLEAN Extended)
{
    PUSB_HUB_CAP_FLAGS Caps;
    BOOLEAN HighSpeedCapable;
    NTSTATUS Status;

    Status = HubIoPrepare(Hub, Request, 0, InputLength, sizeof(*Caps), OutputLength, (PVOID*)&Caps);
    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(Caps, OutputLength);
        HighSpeedCapable = (Hub->m_ParentInfo.Flags & HUB_PARENT_HIGH_SPEED_CAPABLE) != 0;

        if (!Extended)
        {
            ((PUSB_HUB_CAPABILITIES)Caps)->HubIs2xCapable = HighSpeedCapable;
        }
        else
        {
            /* QUIRK: HubIsBusPowered is never reported */
            Caps->HubIsHighSpeedCapable = HighSpeedCapable;
            Caps->HubIsHighSpeed = Hub->IsRootHub() || Hub->m_Parent.HubSpeed == UsbHighSpeed;
            Caps->HubIsMultiTtCapable = Hub->HasFlag(HubFlag::MultiTtHub);
            Caps->HubIsMultiTt = Hub->HasFlag(HubFlag::MultiTtHub);
            Caps->HubIsRoot = Hub->IsRootHub();
            Caps->HubIsArmedWakeOnConnect = Hub->HasFlag(HubFlag::WakeOnConnect);
        }
    }

    /* QUIRK: the size is reported on failure too */
    WdfRequestCompleteWithInformation(Request, Status, sizeof(*Caps));
}

static
VOID
NTAPI
HubIoGetConnectionAttributes(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputLength,
    _In_ size_t InputLength)
{
    PUSB_NODE_CONNECTION_ATTRIBUTES Attributes;
    USHORT PortNumber;
    HubPort* Port;
    NTSTATUS Status;

    Status = HubIoPrepare(Hub, Request, sizeof(ULONG), InputLength, HUB_IO_ATTRIBUTES_SIZE, OutputLength, (PVOID*)&Attributes);
    if (NT_SUCCESS(Status))
    {
        PortNumber = (USHORT)Attributes->ConnectionIndex;
        RtlZeroMemory(Attributes, OutputLength);

        Port = Hub->FindPort(PortNumber);
        Attributes->ConnectionIndex = PortNumber;
        Attributes->PortAttributes = 0;
        Attributes->ConnectionStatus = (Port != NULL) ? Port->m_ConnectionStatus : DeviceGeneralFailure;
    }

    WdfRequestCompleteWithInformation(Request, Status, HUB_IO_ATTRIBUTES_SIZE);
}

static
VOID
NTAPI
HubIoGetHubInformationEx(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputLength,
    _In_ size_t InputLength)
{
    PUSB_HUB_INFORMATION_EX Info;
    NTSTATUS Status;

    Status = HubIoPrepare(Hub, Request, 0, InputLength, HUB_IO_HUB_INFO_EX_SIZE, OutputLength, (PVOID*)&Info);
    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(Info, OutputLength);
        Info->HighestPortNumber = (USHORT)Hub->m_PortCount;

        if (Hub->IsRootHub())
        {
            Info->HubType = UsbRootHub;
        }
        else if (Hub->m_Parent.HubSpeed == UsbSuperSpeed)
        {
            Info->HubType = Usb30Hub;
            RtlCopyMemory(&Info->u.Usb30HubDescriptor, &Hub->m_HubDescriptor.Usb30, sizeof(USB_30_HUB_DESCRIPTOR));
        }
        else
        {
            Info->HubType = Usb20Hub;
            RtlCopyMemory(&Info->u.UsbHubDescriptor, &Hub->m_HubDescriptor.Usb20, sizeof(USB_HUB_DESCRIPTOR));
        }
    }

    WdfRequestCompleteWithInformation(Request, Status, HUB_IO_HUB_INFO_EX_SIZE);
}

static
VOID
NTAPI
HubIoGetConnectorProperties(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputLength,
    _In_ size_t InputLength)
{
    PUSB_PORT_CONNECTOR_PROPERTIES Props;
    USHORT PortNumber;
    USHORT CompanionIndex;
    HubPort* Port;
    HubPort* Companion;
    PWCHAR Text;
    ULONG TextBytes;
    size_t Information = HUB_IO_CONNECTOR_SIZE;
    UCHAR Connector;
    NTSTATUS Status;

    /* The minimum input covers the companion index, so the port number is checked too */
    Status = HubIoPrepare(Hub, Request, HUB_IO_CONNECTOR_SIZE, InputLength, HUB_IO_CONNECTOR_SIZE, OutputLength, (PVOID*)&Props);
    if (!NT_SUCCESS(Status))
    {
        WdfRequestComplete(Request, Status);
        return;
    }

    PortNumber = (USHORT)Props->ConnectionIndex;
    CompanionIndex = Props->CompanionIndex;

    RtlZeroMemory(Props, OutputLength);
    Props->ConnectionIndex = PortNumber;
    Props->CompanionIndex = CompanionIndex;
    Props->ActualLength = HUB_IO_CONNECTOR_SIZE;

    Port = Hub->FindPort(PortNumber);
    if (Port == NULL)
    {
        DPRINT1("Hub %p connector properties: no port %u\n", Hub, PortNumber);
        WdfRequestCompleteWithInformation(Request, STATUS_INVALID_PARAMETER, HUB_IO_CONNECTOR_SIZE);
        return;
    }

    Connector = Port->m_AcpiUpc.ConnectorType;
    Props->UsbPortProperties.PortIsUserConnectable = Port->HasProperty(PortProperty::Removable);
    Props->UsbPortProperties.PortIsDebugCapable = Port->HasProperty(PortProperty::DebugCapable);
    Props->UsbPortProperties.PortHasMultipleCompanions = Port->HasProperty(PortProperty::TypeCWithoutSwitch);
    Props->UsbPortProperties.PortConnectorIsTypeC = (Connector == HUB_UPC_TYPE_C_USB2 ||
                                                     Connector == HUB_UPC_TYPE_C_SWITCH ||
                                                     Connector == HUB_UPC_TYPE_C_NO_SWITCH);

    if (CompanionIndex > 1)
    {
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, HUB_IO_CONNECTOR_SIZE);
        return;
    }

    HubConnectorLock();

    Companion = HubConnectorFindCompanion(Port, CompanionIndex);
    if (Companion != NULL)
    {
        Props->CompanionPortNumber = Companion->Number();

        Text = HubIoStripLinkPrefix(&Companion->m_Hub->m_SymbolicLinkName, &TextBytes);
        if (Text != NULL)
        {
            Props->ActualLength = TextBytes + HUB_IO_CONNECTOR_SIZE;
            if (OutputLength - HUB_IO_CONNECTOR_SIZE >= TextBytes)
            {
                RtlCopyMemory(Props->CompanionHubSymbolicLinkName, Text, TextBytes);

                /* QUIRK: the whole output length, not ActualLength */
                Information = OutputLength;
            }
        }
    }

    HubConnectorUnlock();

    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, Information);
}

static
VOID
NTAPI
HubIoGetConnectionInformationV2(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputLength,
    _In_ size_t InputLength)
{
    PUSB_NODE_CONNECTION_INFORMATION_EX_V2 Info;
    USB_PROTOCOLS Wanted;
    USHORT PortNumber;
    HubPort* Port;
    HubPdo* Pdo;
    HubChild* Child;
    NTSTATUS Status;

    Status = HubIoPrepare(Hub, Request, HUB_IO_CONNECTION_V2_SIZE, InputLength, HUB_IO_CONNECTION_V2_SIZE, OutputLength, (PVOID*)&Info);
    if (NT_SUCCESS(Status) && Info->Length < HUB_IO_CONNECTION_V2_SIZE)
    {
        DPRINT1("Hub %p connection information V2 length %lu too small\n", Hub, Info->Length);
        Status = STATUS_INVALID_PARAMETER;
    }

    if (!NT_SUCCESS(Status))
    {
        WdfRequestComplete(Request, Status);
        return;
    }

    PortNumber = (USHORT)Info->ConnectionIndex;
    Wanted = Info->SupportedUsbProtocols;

    RtlZeroMemory(Info, OutputLength);
    Info->ConnectionIndex = PortNumber;

    if (!Wanted.Usb300)
    {
        DPRINT1("Hub %p connection information V2 caller does not claim USB 3\n", Hub);
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return;
    }

    Info->Length = HUB_IO_CONNECTION_V2_SIZE;

    Port = Hub->FindPort(PortNumber);
    if (Port == NULL)
    {
        DPRINT1("Hub %p connection information V2: no port %u\n", Hub, PortNumber);
        WdfRequestCompleteWithInformation(Request, STATUS_INVALID_PARAMETER, HUB_IO_CONNECTION_V2_SIZE);
        return;
    }

    if (Port->IsUsb30())
    {
        Info->SupportedUsbProtocols.Usb300 = 1;
    }
    else
    {
        Info->SupportedUsbProtocols.Usb110 = 1;
        if (Hub->IsRootHub() || Hub->m_Parent.HubSpeed == UsbHighSpeed)
            Info->SupportedUsbProtocols.Usb200 = 1;
    }

    Pdo = HubIoLockChild(Hub, PortNumber);
    Child = (Pdo != NULL) ? Pdo->m_Child : NULL;
    if (Child != NULL)
    {
        Info->Flags.DeviceIsSuperSpeedCapableOrHigher = (Child->m_SqmFlags & (LONG)ChildSqm::SuperSpeedCapable) != 0;
        Info->Flags.DeviceIsSuperSpeedPlusCapableOrHigher = HubIoDeviceSspCapable(Child);

        if (Port->IsUsb30())
        {
            Info->Flags.DeviceIsOperatingAtSuperSpeedOrHigher = 1;
            Info->Flags.DeviceIsOperatingAtSuperSpeedPlusOrHigher = HubIoDeviceRunsSsp(Child);
        }
    }

    HubIoUnlockChildren(Hub);

    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, HUB_IO_CONNECTION_V2_SIZE);
}

static
VOID
NTAPI
HubIoCyclePort(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputLength,
    _In_ size_t InputLength)
{
    PUSB_CYCLE_PORT_PARAMS Params;
    BOOLEAN UxdUpdate = FALSE;
    HubPdo* Pdo;
    HubChild* Child;
    NTSTATUS IdleStatus;
    NTSTATUS Status;

    Status = HubIoPrepare(Hub, Request, sizeof(ULONG), InputLength, HUB_IO_CYCLE_SIZE, OutputLength, (PVOID*)&Params);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Params->StatusReturned = USBD_STATUS_SUCCESS;

    Pdo = HubIoLockChild(Hub, (USHORT)Params->ConnectionIndex);
    Child = (Pdo != NULL) ? Pdo->m_Child : NULL;
    if (Child == NULL)
    {
        HubIoUnlockChildren(Hub);
        DPRINT1("Hub %p cycle: no device on port %lu\n", Hub, Params->ConnectionIndex);
        Params->StatusReturned = USBD_STATUS_DEVICE_GONE;
        Status = STATUS_NO_SUCH_DEVICE;
        goto Exit;
    }

    HubSaveUxdState(Hub, Child, &UxdUpdate);

    WdfObjectReferenceWithTag(Child->m_Object, (PVOID)HubTagUserRequest);
    HubIoUnlockChildren(Hub);

    /* QUIRK: a non admin caller with a UXD update cycles the port but still fails */
    Status = HubIoCallerIsAdmin();
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p cycle of port %lu: caller is not an administrator\n", Hub, Params->ConnectionIndex);

    if (NT_SUCCESS(Status) || UxdUpdate)
    {
        IdleStatus = WdfDeviceStopIdle(Hub->m_Device, TRUE);
        if (!NT_SUCCESS(IdleStatus))
        {
            DPRINT1("Hub %p cycle of port %lu: stop idle failed 0x%lx\n", Hub, Params->ConnectionIndex, IdleStatus);
            Status = IdleStatus;
        }
        else
        {
            DPRINT("Hub %p cycling port %lu\n", Hub, Params->ConnectionIndex);

            /* One queued cycle is enough */
            if (InterlockedCompareExchange(&Pdo->m_CycleQueued, 1, 0) == 0)
                Child->m_Port->Post(PortEvent::CycleRequest);

            WdfDeviceResumeIdle(Hub->m_Device);
        }
    }

    WdfObjectDereferenceWithTag(Child->m_Object, (PVOID)HubTagUserRequest);

Exit:
    WdfRequestCompleteWithInformation(Request, Status, HUB_IO_CYCLE_SIZE);
}

static
VOID
NTAPI
HubIoResetHub(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request)
{
    NTSTATUS Status;

    Status = HubIoCallerIsAdmin();
    if (NT_SUCCESS(Status) && !Hub->RequestHubReset())
        Status = STATUS_UNSUCCESSFUL;

    if (NT_SUCCESS(Status))
        DPRINT("Hub %p reset request queued\n", Hub);
    else
        DPRINT1("Hub %p reset request not done 0x%lx\n", Hub, Status);

    /* QUIRK: success whatever happened */
    WdfRequestComplete(Request, STATUS_SUCCESS);
}

/* Queue callbacks */

VOID
NTAPI
HubEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    HubFdo* Hub = HubFdo::FromDevice(WdfIoQueueGetDevice(Queue));

    PAGED_CODE();

    switch (IoControlCode)
    {
        case IOCTL_USB_GET_PORT_STATUS:
            HubIoGetPortStatus(Hub, Request, OutputBufferLength, InputBufferLength);
            break;

        case IOCTL_USB_GET_NODE_INFORMATION:
            HubIoGetNodeInformation(Hub, Request, OutputBufferLength, InputBufferLength);
            break;

        case IOCTL_USB_GET_NODE_CONNECTION_INFORMATION:
            HubIoGetConnectionInformation(Hub, Request, OutputBufferLength, InputBufferLength, FALSE);
            break;

        case IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX:
            HubIoGetConnectionInformation(Hub, Request, OutputBufferLength, InputBufferLength, TRUE);
            break;

        case IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION:
            HubIoGetDescriptor(Hub, Queue, Request, OutputBufferLength, InputBufferLength);
            break;

        case IOCTL_USB_GET_NODE_CONNECTION_NAME:
            HubIoGetConnectionName(Hub, Request, OutputBufferLength, InputBufferLength);
            break;

        case IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME:
            HubIoGetDriverKeyName(Hub, Request, OutputBufferLength, InputBufferLength);
            break;

        case IOCTL_USB_GET_HUB_CAPABILITIES:
            HubIoGetHubCapabilities(Hub, Request, OutputBufferLength, InputBufferLength, FALSE);
            break;

        case IOCTL_USB_GET_HUB_CAPABILITIES_EX:
            HubIoGetHubCapabilities(Hub, Request, OutputBufferLength, InputBufferLength, TRUE);
            break;

        case IOCTL_USB_GET_NODE_CONNECTION_ATTRIBUTES:
            HubIoGetConnectionAttributes(Hub, Request, OutputBufferLength, InputBufferLength);
            break;

        case IOCTL_USB_HUB_CYCLE_PORT:
            HubIoCyclePort(Hub, Request, OutputBufferLength, InputBufferLength);
            break;

        case IOCTL_USB_RESET_HUB:
            HubIoResetHub(Hub, Request);
            break;

        case IOCTL_USB_GET_HUB_INFORMATION_EX:
            HubIoGetHubInformationEx(Hub, Request, OutputBufferLength, InputBufferLength);
            break;

        case IOCTL_USB_GET_PORT_CONNECTOR_PROPERTIES:
            HubIoGetConnectorProperties(Hub, Request, OutputBufferLength, InputBufferLength);
            break;

        case IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX_V2:
            HubIoGetConnectionInformationV2(Hub, Request, OutputBufferLength, InputBufferLength);
            break;

        default:
            HubIoForward(Hub, Request);
            break;
    }
}

/* A child PDO forwards its client's port status request here and waits for it */
VOID
NTAPI
HubEvtIoInternalDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    HubFdo* Hub = HubFdo::FromDevice(WdfIoQueueGetDevice(Queue));
    WDF_REQUEST_PARAMETERS Params;
    HubPdo* Pdo;
    HubPort* Port;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    if (IoControlCode != IOCTL_INTERNAL_USB_GET_PORT_STATUS)
    {
        /* QUIRK: not passed down */
        DPRINT1("Hub %p unexpected internal IOCTL 0x%lx\n", Hub, IoControlCode);
        WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
        return;
    }

    WDF_REQUEST_PARAMETERS_INIT(&Params);
    WdfRequestGetParameters(Request, &Params);

    Pdo = (HubPdo*)Params.Parameters.Others.Arg2;
    Port = (Pdo->m_Child != NULL) ? Pdo->m_Child->m_Port : NULL;
    if (Port == NULL)
    {
        DPRINT1("Hub %p port status for PDO %p without a device\n", Hub, Pdo);
        *(PULONG)Params.Parameters.Others.Arg1 = 0;
        WdfRequestComplete(Request, STATUS_NO_SUCH_DEVICE);
        return;
    }

    Hub->m_PortStatusTarget = Port;
    Hub->m_PortStatusRequest = Request;
    Hub->Post(HubEvent::PortStatusRequest);
}

/* Diagnostics */

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubReportPnpProblem(
    _In_ WDFDEVICE Device,
    _In_ ULONG MessageId)
{
    PAGED_CODE();

    /* The problem is only logged; DEVPKEY_Device_DriverProblemDesc is not set */
    if (MessageId != 0)
        DPRINT1("Hub device %p problem 0x%lx\n", Device, MessageId);
}
