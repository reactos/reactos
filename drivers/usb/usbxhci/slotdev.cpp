/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     USB device object: slot commands, input contexts and the UCX device callbacks
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "xhcidrv.h"
#include <acpiioct.h>
#include <drivers/usb3/hubucx.h>

#define NDEBUG
#include <debug.h>

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XhciUsbDevice, XhciGetUsbDeviceContext);

/* USB _DSM {CE2EE385-00E6-48CB-9F05-2EDB927C4899}; function 10 takes a root port number */
static const GUID XhciGuidUsbDsm =
    { 0xce2ee385, 0x00e6, 0x48cb, { 0x9f, 0x05, 0x2e, 0xdb, 0x92, 0x7c, 0x48, 0x99 } };

#define XHCI_USB_DSM_REVISION       0
#define XHCI_USB_DSM_TUNNEL_STATE   10
#define XHCI_USB_DSM_PORT_NATIVE    1
#define XHCI_USB_DSM_PORT_TUNNELED  2

/** Context of the work item that asks ACPI for a root port's tunnel state. */
typedef struct _XHCI_TUNNEL_DSM_WORK
{
    XhciUsbDevice* Device;
    PUSBDEVICE_UPDATE Payload;
} XHCI_TUNNEL_DSM_WORK, *PXHCI_TUNNEL_DSM_WORK;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XHCI_TUNNEL_DSM_WORK, XhciGetTunnelDsmWork);

/* Reason codes for the controller's fatal error report */
enum : ULONG
{
    XhciDeviceFatalSlotInUse = 0x1014,
    XhciDeviceFatalAddressDefault = 0x1015,
    XhciDeviceFatalEnableSlot = 0x1016,
    XhciDeviceFatalDeconfigure = 0x1017,
    XhciDeviceFatalDisableSlot = 0x1018,
    XhciDeviceFatalResetDevice = 0x1019
};

/* Hardware verifier conditions */
enum : ULONG
{
    XhciDeviceCheckSlotInUse = 0x00020000,
    XhciDeviceCheckAddressDefault = 0x00040000,
    XhciDeviceCheckDeconfigure = 0x00080000,
    XhciDeviceCheckDisableSlot = 0x00100000,
    XhciDeviceCheckResetDevice = 0x00200000
};

/** Slot context bit 0 plus the default control endpoint at DCI 1. */
#define XHCI_DEFAULT_CONTEXT_MASK   0x00000003

#define XHCI_ROUTE_TIER_MAX         15
#define XHCI_ROUTE_TIER_BITS        4

static EVT_WDF_OBJECT_CONTEXT_CLEANUP XhciEvtUsbDeviceCleanup;

static
PVOID
NTAPI
XhciRequestPayload(
    _In_ WDFREQUEST Request)
{
    WDF_REQUEST_PARAMETERS Parameters;

    WDF_REQUEST_PARAMETERS_INIT(&Parameters);
    WdfRequestGetParameters(Request, &Parameters);
    return Parameters.Parameters.Others.Arg1;
}

/** Evaluates _DSM on the controller ACPI object with a buffer as Arg3; returns up to 8 result bytes, 0 on failure. */
_IRQL_requires_(PASSIVE_LEVEL)
static
ULONG64
NTAPI
XhciEvaluateDsmValue(
    _In_ WDFDEVICE Device,
    _In_ const GUID* Uuid,
    _In_ ULONG Revision,
    _In_ ULONG Function,
    _In_reads_bytes_opt_(ArgumentLength) const VOID* Argument,
    _In_ USHORT ArgumentLength)
{
    union
    {
        ACPI_EVAL_OUTPUT_BUFFER Header;
        UCHAR Bytes[32];
    } Output;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX Input;
    PACPI_METHOD_ARGUMENT Next;
    WDF_MEMORY_DESCRIPTOR InputDescriptor;
    WDF_MEMORY_DESCRIPTOR OutputDescriptor;
    ULONG64 Value = 0;
    ULONG Length;
    ULONG Size;
    NTSTATUS Status;

    PAGED_CODE();

    Size = FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument) +
           ACPI_METHOD_ARGUMENT_LENGTH(sizeof(GUID)) +
           2 * ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) +
           ACPI_METHOD_ARGUMENT_LENGTH(ArgumentLength);

    Input = (PACPI_EVAL_INPUT_BUFFER_COMPLEX)ExAllocatePoolWithTag(PagedPool, Size, XHCI_TAG_CONTROLLER);
    if (Input == NULL)
    {
        DPRINT1("No memory for _DSM function %lu\n", Function);
        return 0;
    }

    RtlZeroMemory(Input, Size);
    Input->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    Input->MethodNameAsUlong = 'MSD_';
    Input->Size = Size;
    Input->ArgumentCount = 4;

    Next = &Input->Argument[0];
    ACPI_METHOD_SET_ARGUMENT_BUFFER(Next, Uuid, sizeof(GUID));
    Next = ACPI_METHOD_NEXT_ARGUMENT(Next);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(Next, Revision);
    Next = ACPI_METHOD_NEXT_ARGUMENT(Next);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(Next, Function);
    Next = ACPI_METHOD_NEXT_ARGUMENT(Next);
    ACPI_METHOD_SET_ARGUMENT_BUFFER(Next, Argument, ArgumentLength);

    RtlZeroMemory(&Output, sizeof(Output));
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&InputDescriptor, Input, Size);
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&OutputDescriptor, &Output, sizeof(Output));

    Status = WdfIoTargetSendIoctlSynchronously(WdfDeviceGetIoTarget(Device),
                                               NULL,
                                               IOCTL_ACPI_EVAL_METHOD,
                                               &InputDescriptor,
                                               &OutputDescriptor,
                                               NULL,
                                               NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("_DSM %08lx function %lu failed 0x%lx\n", Uuid->Data1, Function, Status);
    }
    else if (Output.Header.Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
    {
        DPRINT1("_DSM function %lu returned signature 0x%08lx\n", Function, Output.Header.Signature);
    }
    else if (Output.Header.Count != 0 && Output.Header.Argument[0].DataLength != 0)
    {
        /* QUIRK: the result type is not checked, any 1 to 8 byte value decodes */
        Length = min(Output.Header.Argument[0].DataLength, sizeof(Value));
        RtlCopyMemory(&Value,
                      &Output.Bytes[FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
                                    FIELD_OFFSET(ACPI_METHOD_ARGUMENT, Data)],
                      Length);
    }

    ExFreePoolWithTag(Input, XHCI_TAG_CONTROLLER);
    return Value;
}

static
BOOLEAN
NTAPI
XhciCommandSucceeded(
    _In_ const XhciCommand* Command)
{
    return Command->Status != STATUS_NO_SUCH_DEVICE &&
           Command->Code == XhciCompletionCode::Success;
}

/* Object lifetime **********************************************************/

XhciUsbDevice*
XhciUsbDevice::FromUcx(
    _In_ UCXUSBDEVICE UsbDevice)
{
    return XhciGetUsbDeviceContext(UsbDevice);
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
XhciUsbDevice::Initialize(
    _In_ XhciController* Controller,
    _In_ UCXUSBDEVICE UsbDevice,
    _In_ const UCXUSBDEVICE_INFO* Info)
{
    PAGED_CODE();

    m_Controller = Controller;
    m_Handle = UsbDevice;
    RtlCopyMemory(&m_Info, Info, sizeof(m_Info));
    m_ContextSize = Controller->m_Registers.ContextSize();
    KeInitializeSpinLock(&m_ReconfigureLock);

    /* Allocated once so that enabling after a controller reset never needs memory */
    m_Output = Controller->m_Buffers.Acquire(m_ContextSize * XHCI_DEVICE_CONTEXT_COUNT);
    if (m_Output == NULL)
    {
        DPRINT1("No output device context for USB device %p\n", UsbDevice);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_Input = Controller->m_Buffers.Acquire(m_ContextSize * XHCI_INPUT_CONTEXT_COUNT);
    if (m_Input == NULL)
    {
        DPRINT1("No input context for USB device %p\n", UsbDevice);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DPRINT("USB device %p: speed %d, root port %lu, depth %lu, TT hub %p\n",
           UsbDevice, m_Info.DeviceSpeed, m_Info.PortPath.PortPath[0],
           m_Info.PortPath.PortPathDepth, m_Info.TtHub);
    return STATUS_SUCCESS;
}

VOID
XhciUsbDevice::Cleanup(
    _In_ WDFOBJECT Object)
{
    /* Creation never got far enough to own anything */
    if (m_Handle != (UCXUSBDEVICE)Object)
        return;

    ASSERT(m_SlotId == 0);

    if (m_Output != NULL)
    {
        m_Controller->m_Buffers.Free(m_Output);
        m_Output = NULL;
    }

    if (m_Input != NULL)
    {
        m_Controller->m_Buffers.Free(m_Input);
        m_Input = NULL;
    }

    if (m_ReconfigureInput != NULL)
    {
        m_Controller->m_Buffers.Free(m_ReconfigureInput);
        m_ReconfigureInput = NULL;
    }
}

static
VOID
NTAPI
XhciEvtUsbDeviceCleanup(
    _In_ WDFOBJECT Object)
{
    XhciGetUsbDeviceContext(Object)->Cleanup(Object);
}

/* Accessors ****************************************************************/

PVOID
XhciUsbDevice::InputContext(
    _In_ ULONG Index) const
{
    ASSERT(Index < XHCI_INPUT_CONTEXT_COUNT);
    return (PUCHAR)m_Input->VirtualAddress + Index * m_ContextSize;
}

PVOID
XhciUsbDevice::OutputContext(
    _In_ ULONG Dci) const
{
    ASSERT(Dci < XHCI_DEVICE_CONTEXT_COUNT);
    return (PUCHAR)m_Output->VirtualAddress + Dci * m_ContextSize;
}

ULONG64
XhciUsbDevice::InputContextAddress() const
{
    return m_Input->LogicalAddress.QuadPart;
}

VOID
XhciUsbDevice::ZeroInputContext()
{
    RtlZeroMemory(m_Input->VirtualAddress, m_ContextSize * XHCI_INPUT_CONTEXT_COUNT);
}

PXHCI_INPUT_CONTROL_CONTEXT
XhciUsbDevice::InputControl() const
{
    return (PXHCI_INPUT_CONTROL_CONTEXT)InputContext(0);
}

PXHCI_SLOT_CONTEXT
XhciUsbDevice::InputSlot() const
{
    return (PXHCI_SLOT_CONTEXT)InputContext(1);
}

const XHCI_SLOT_CONTEXT*
XhciUsbDevice::OutputSlot() const
{
    return (const XHCI_SLOT_CONTEXT*)OutputContext(0);
}

ULONG
XhciUsbDevice::EndpointState(
    _In_ ULONG Dci) const
{
    return ((const XHCI_ENDPOINT_CONTEXT*)OutputContext(Dci))->EndpointState;
}

ULONG64
XhciUsbDevice::HardwareDequeuePointer(
    _In_ ULONG Dci) const
{
    return ((const XHCI_ENDPOINT_CONTEXT*)OutputContext(Dci))->TRDequeuePointer;
}

ULONG
XhciUsbDevice::ContextEntries() const
{
    ULONG Mask = m_ContextMask;
    ULONG Highest = 0;

    ASSERT(Mask != 0);

    while ((Mask >>= 1) != 0)
        Highest++;

    return Highest;
}

ULONG
XhciUsbDevice::SlotId() const
{
    return m_SlotId;
}

USB_DEVICE_SPEED
XhciUsbDevice::Speed() const
{
    return m_Info.DeviceSpeed;
}

UCXUSBDEVICE
XhciUsbDevice::Handle() const
{
    return m_Handle;
}

XhciController*
XhciUsbDevice::Controller() const
{
    return m_Controller;
}

BOOLEAN
XhciUsbDevice::IsSlotEnabled() const
{
    return m_SlotEnabled;
}

const USB_DEVICE_DESCRIPTOR*
XhciUsbDevice::DeviceDescriptor() const
{
    return &m_DeviceDescriptor;
}

const USB_DEVICE_PORT_PATH*
XhciUsbDevice::PortPath() const
{
    return &m_Info.PortPath;
}

ULONG
XhciUsbDevice::ContextSize() const
{
    return m_ContextSize;
}

XhciUsbDevice*
XhciUsbDevice::TtHub() const
{
    if (m_Info.TtHub == NULL)
        return NULL;

    return FromUcx(m_Info.TtHub);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XhciUsbDevice::RingDoorbell(
    _In_ ULONG Dci,
    _In_ ULONG StreamId)
{
    ULONG SlotId = m_SlotId;

    if (SlotId == 0)
    {
        DPRINT("Doorbell for DCI %lu of device %p without a slot\n", Dci, this);
        return;
    }

    /* DB Target in bits 7:0, DB Stream ID in bits 31:16 (xHCI 5.6) */
    m_Controller->m_Registers.RingDoorbell(SlotId, (Dci & 0xFF) | (StreamId << 16));
}

/* Endpoint table ***********************************************************/

_IRQL_requires_max_(DISPATCH_LEVEL)
XhciEndpoint*
XhciUsbDevice::EndpointAt(
    _In_ ULONG Dci) const
{
    if (Dci > XHCI_MAX_DCI)
        return NULL;

    return m_Endpoints[Dci];
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XhciUsbDevice::AddEndpoint(
    _In_ XhciEndpoint* Endpoint)
{
    WdfObjectReferenceWithTag(m_Handle, Endpoint);
    WdfObjectReferenceWithTag(Endpoint->Handle(), Endpoint);

    /* The default endpoint stays in the table for the life of the device */
    if (Endpoint->Dci() == 1)
        m_Endpoints[1] = Endpoint;

    InterlockedIncrement(&m_EndpointCount);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XhciUsbDevice::RemoveEndpoint(
    _In_ XhciEndpoint* Endpoint)
{
    UCXUSBDEVICE UsbDevice = m_Handle;
    ULONG Dci = Endpoint->Dci();

    if (Dci <= XHCI_MAX_DCI && m_Endpoints[Dci] == Endpoint)
        m_Endpoints[Dci] = NULL;

    InterlockedDecrement(&m_EndpointCount);

    /* The device reference goes last; this object may be freed with it */
    WdfObjectDereferenceWithTag(Endpoint->Handle(), Endpoint);
    WdfObjectDereferenceWithTag(UsbDevice, Endpoint);
}

VOID
XhciUsbDevice::DisableDefaultEndpoint()
{
    if (m_Endpoints[1] != NULL)
        m_Endpoints[1]->Disable(TRUE);
}

VOID
XhciUsbDevice::DisableConfiguredEndpoints(
    _In_ BOOLEAN ClearTable)
{
    ULONG Dci;

    for (Dci = 2; Dci <= XHCI_MAX_DCI; Dci++)
    {
        if (m_Endpoints[Dci] == NULL)
            continue;

        m_Endpoints[Dci]->Disable(ClearTable);
        if (ClearTable)
            m_Endpoints[Dci] = NULL;
    }
}

VOID
XhciUsbDevice::DisableEndpointList(
    _In_ ULONG Count,
    _In_reads_(Count) UCXENDPOINT* Endpoints)
{
    ULONG Index;

    for (Index = 0; Index < Count; Index++)
        XhciEndpoint::FromUcx(Endpoints[Index])->Disable();
}

VOID
XhciUsbDevice::RecordEnabledEndpoints(
    _In_ const ENDPOINTS_CONFIGURE* Payload)
{
    XhciEndpoint* Endpoint;
    XhciEndpoint* Previous;
    ULONG Index;
    ULONG Dci;

    for (Index = 0; Index < Payload->EndpointsToEnableCount; Index++)
    {
        Endpoint = XhciEndpoint::FromUcx(Payload->EndpointsToEnable[Index]);
        Dci = Endpoint->Dci();

        Previous = m_Endpoints[Dci];
        if (Previous != NULL && Previous != Endpoint)
        {
            DPRINT("DCI %lu of device %p moves from endpoint %p to %p\n", Dci, this, Previous, Endpoint);
            Previous->Disable(TRUE);
            m_Endpoints[Dci] = NULL;
        }

        m_Endpoints[Dci] = Endpoint;
    }
}

/* Pending request and command plumbing **************************************/

VOID
XhciUsbDevice::SetPending(
    _In_ WDFREQUEST Request,
    _In_ PVOID Payload,
    _In_ PendingKind Kind,
    _In_ BOOLEAN MustSucceed)
{
    /* UCX and the hub send one device management request at a time */
    ASSERT(m_PendingRequest == NULL);

    m_PendingPayload = Payload;
    m_PendingKind = Kind;
    m_MustSucceed = MustSucceed;
    m_PendingRequest = Request;
}

WDFREQUEST
XhciUsbDevice::TakePending()
{
    return (WDFREQUEST)InterlockedExchangePointer((PVOID volatile*)&m_PendingRequest, NULL);
}

VOID
XhciUsbDevice::CompletePending(
    _In_ NTSTATUS Status)
{
    WDFREQUEST Request = TakePending();

    if (Request != NULL)
        WdfRequestComplete(Request, Status);
}

NTSTATUS
XhciUsbDevice::LostSlotStatus() const
{
    return m_MustSucceed ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

VOID
XhciUsbDevice::PrepareCommand(
    _In_ ULONG Type)
{
    RtlZeroMemory(&m_Command, sizeof(m_Command));
    m_Command.Trb.Dword[3] = (Type << XHCI_TRB_TYPE_SHIFT) | (m_SlotId << XHCI_TRB_SLOT_SHIFT);
    m_Command.Done = CommandDone;
    m_Command.Context = this;
}

VOID
NTAPI
XhciUsbDevice::CommandDone(
    _In_ XhciCommand* Command)
{
    XhciUsbDevice* Device = static_cast<XhciUsbDevice*>(Command->Context);

    /* Controller gone or a lost slot already finished the request */
    if (Device->m_PendingRequest == NULL)
    {
        DPRINT("Device %p command type %lu completed with no request left\n",
               Device, XhciTrbType(&Command->Trb));
        return;
    }

    switch (XhciTrbType(&Command->Trb))
    {
        case static_cast<ULONG>(XhciTrbType::EnableSlot):
            Device->OnEnableSlotDone(Command);
            break;

        case static_cast<ULONG>(XhciTrbType::AddressDevice):
            Device->OnAddressDeviceDone(Command);
            break;

        case static_cast<ULONG>(XhciTrbType::EvaluateContext):
            Device->OnEvaluateContextDone(Command);
            break;

        case static_cast<ULONG>(XhciTrbType::ConfigureEndpoint):
            Device->OnConfigureEndpointDone(Command);
            break;

        case static_cast<ULONG>(XhciTrbType::DisableSlot):
            Device->OnDisableSlotDone(Command);
            break;

        case static_cast<ULONG>(XhciTrbType::ResetDevice):
            Device->OnResetDeviceDone(Command);
            break;

        default:
            ASSERT(FALSE);
            break;
    }
}

VOID
XhciUsbDevice::ForgetSlot()
{
    if (m_SlotId != 0 && m_Controller->m_Slots.LookupSlot(m_SlotId) == this)
        m_Controller->m_Slots.SetSlot(m_SlotId, this, 0);

    m_SlotEnabled = FALSE;
    m_SlotId = 0;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XhciUsbDevice::SetDisabled()
{
    WDFREQUEST Request;

    DisableDefaultEndpoint();
    DisableConfiguredEndpoints(TRUE);
    ForgetSlot();

    Request = TakePending();
    if (Request != NULL)
        WdfRequestComplete(Request, LostSlotStatus());
}

/* Enable Slot and Address Device ********************************************/

VOID
XhciUsbDevice::SubmitEnableSlot()
{
    ASSERT(m_SlotId == 0);

    /* QUIRK: Slot Type is always 0 */
    PrepareCommand(static_cast<ULONG>(XhciTrbType::EnableSlot));
    m_Controller->m_Commands.Submit(&m_Command);
}

NTSTATUS
XhciUsbDevice::BuildAddressContext()
{
    const USB_DEVICE_PORT_PATH* Path = &m_Info.PortPath;
    PXHCI_SLOT_CONTEXT Slot;
    XhciUsbDevice* Hub;
    ULONG Speed;
    ULONG Route;
    ULONG Tier;

    switch (m_Info.DeviceSpeed)
    {
        case UsbLowSpeed:
            Speed = XHCI_SPEED_LOW;
            break;
        case UsbFullSpeed:
            Speed = XHCI_SPEED_FULL;
            break;
        case UsbHighSpeed:
            Speed = XHCI_SPEED_HIGH;
            break;
        case UsbSuperSpeed:
            Speed = XHCI_SPEED_SUPER;
            break;
        default:
            DPRINT1("Device %p has speed %d, which a slot cannot describe\n", this, m_Info.DeviceSpeed);
            return STATUS_INVALID_PARAMETER;
    }

    ZeroInputContext();
    InputControl()->AddFlags = XHCI_DEFAULT_CONTEXT_MASK;
    m_ContextMask = XHCI_DEFAULT_CONTEXT_MASK;

    /* The root port is not part of the route string (USB 3.2 8.9) */
    Route = 0;
    for (Tier = 1; Tier < Path->PortPathDepth && Tier < MAX_USB_DEVICE_DEPTH; Tier++)
        Route |= min(Path->PortPath[Tier], (ULONG)XHCI_ROUTE_TIER_MAX) << ((Tier - 1) * XHCI_ROUTE_TIER_BITS);

    Slot = InputSlot();
    Slot->RouteString = Route;
    Slot->Speed = Speed;
    Slot->ContextEntries = 1;
    Slot->RootHubPortNumber = Path->PortPath[0];

    Hub = TtHub();
    if (Hub != NULL)
    {
        Slot->MultiTT = (Hub->m_NumberOfTTs > 1);
        Slot->TTHubSlotId = Hub->m_SlotId;
    }

    if (Path->TTHubDepth != 0 && Path->TTHubDepth < MAX_USB_DEVICE_DEPTH)
        Slot->TTPortNumber = Path->PortPath[Path->TTHubDepth];

    /* QUIRK: Average TRB Length stays 0 for EP0 */
    m_Endpoints[1]->BuildContext(InputContext(1 + 1), m_ContextSize);
    return STATUS_SUCCESS;
}

NTSTATUS
XhciUsbDevice::SubmitAddressDevice(
    _In_ BOOLEAN BlockSetAddress)
{
    ULONG64 InputAddress = InputContextAddress();
    NTSTATUS Status;

    Status = BuildAddressContext();
    if (!NT_SUCCESS(Status))
        return Status;

    PrepareCommand(static_cast<ULONG>(XhciTrbType::AddressDevice));
    m_Command.Trb.Dword[0] = (ULONG)InputAddress;
    m_Command.Trb.Dword[1] = (ULONG)(InputAddress >> 32);
    if (BlockSetAddress)
        m_Command.Trb.Dword[3] |= XHCI_CMD_BLOCK_SET_ADDRESS;

    m_Controller->m_Commands.Submit(&m_Command);
    return STATUS_SUCCESS;
}

VOID
XhciUsbDevice::Enable(
    _In_ WDFREQUEST Request,
    _Inout_ PUSBDEVICE_ENABLE Payload)
{
    NTSTATUS Status;

    if (m_Endpoints[1] == NULL)
    {
        DPRINT1("ENABLE for device %p before its default endpoint exists\n", this);
        WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
        return;
    }

    Status = m_Endpoints[1]->Enable();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Default endpoint of device %p failed to enable: 0x%lx\n", this, Status);
        WdfRequestComplete(Request, Status);
        return;
    }

    if (!m_Controller->IsAccessible())
    {
        DPRINT1("ENABLE for device %p with the controller gone\n", this);
        DisableDefaultEndpoint();
        WdfRequestComplete(Request, STATUS_NO_SUCH_DEVICE);
        return;
    }

    SetPending(Request, Payload, PendingKind::Enable, FALSE);
    SubmitEnableSlot();
}

VOID
XhciUsbDevice::OnEnableSlotDone(
    _In_ const XhciCommand* Command)
{
    ULONG Code = static_cast<ULONG>(Command->Code);
    ULONG SlotId = XhciTrbSlotId(&Command->Completion);
    NTSTATUS Status;

    if (Command->Status == STATUS_NO_SUCH_DEVICE)
    {
        DisableDefaultEndpoint();
        CompletePending(LostSlotStatus());
        return;
    }

    if (Command->Code != XhciCompletionCode::Success)
    {
        DPRINT1("Enable Slot for device %p failed with completion code %lu\n", this, Code);

        /* A RESET payload has no failure flags, only ENABLE does */
        if (Command->Code == XhciCompletionCode::NoSlotsAvailable && m_PendingKind == PendingKind::Enable)
            ((PUSBDEVICE_ENABLE)m_PendingPayload)->FailureFlags.InsufficientHardwareResourcesForDevice = 1;

        DisableDefaultEndpoint();

        if (m_MustSucceed)
        {
            m_Controller->RaiseControllerFault(XhciRecovery::ResetHost, XhciDeviceFatalEnableSlot);

            /* Complete the RESET here; the device holds no slot, so no reset walk will */
            CompletePending(STATUS_SUCCESS);
            return;
        }

        CompletePending(STATUS_UNSUCCESSFUL);
        return;
    }

    m_SlotEnabled = TRUE;
    m_SlotId = SlotId;

    RtlZeroMemory(m_Output->VirtualAddress, m_ContextSize * XHCI_DEVICE_CONTEXT_COUNT);

    Status = m_Controller->m_Slots.SetSlot(SlotId, this, m_Output->LogicalAddress.QuadPart);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Enable Slot gave device %p slot %lu, which is already in use (0x%lx)\n",
                this, SlotId, Status);
        m_Controller->VerifierCheck(XhciDeviceCheckSlotInUse);
        m_Controller->RaiseControllerFault(XhciRecovery::ResetHost, XhciDeviceFatalSlotInUse);

        /* Undo the slot state and complete the request, since nothing else would */
        m_SlotEnabled = FALSE;
        m_SlotId = 0;
        DisableDefaultEndpoint();
        CompletePending(LostSlotStatus());
        return;
    }

    DPRINT("Device %p got slot %lu\n", this, SlotId);

    Status = SubmitAddressDevice(TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Address Device for slot %lu not built: 0x%lx\n", SlotId, Status);
        RtlZeroMemory(&m_Command, sizeof(m_Command));

        if (m_MustSucceed)
            m_Controller->RaiseControllerFault(XhciRecovery::ResetHost, XhciDeviceFatalAddressDefault);
        else
            SubmitDisableSlot(AfterDisable::CompleteFailure);
    }
}

VOID
XhciUsbDevice::Address(
    _In_ WDFREQUEST Request,
    _Inout_ PUSBDEVICE_ADDRESS Payload)
{
    NTSTATUS Status;

    if (!m_SlotEnabled)
    {
        DPRINT1("ADDRESS for device %p without a slot\n", this);
        WdfRequestComplete(Request, STATUS_UNSUCCESSFUL);
        return;
    }

    if (!m_Controller->IsAccessible())
    {
        DPRINT1("ADDRESS for device %p with the controller gone\n", this);
        WdfRequestComplete(Request, STATUS_NO_SUCH_DEVICE);
        return;
    }

    SetPending(Request, Payload, PendingKind::Address, FALSE);

    Status = SubmitAddressDevice(FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Address Device for slot %lu not built: 0x%lx\n", m_SlotId, Status);
        CompletePending(Status);
    }
}

VOID
XhciUsbDevice::OnAddressDeviceDone(
    _In_ const XhciCommand* Command)
{
    BOOLEAN BlockSetAddress = (Command->Trb.Dword[3] & XHCI_CMD_BLOCK_SET_ADDRESS) != 0;

    if (Command->Status == STATUS_NO_SUCH_DEVICE)
    {
        if (BlockSetAddress)
            SetDisabled();
        else
            CompletePending(LostSlotStatus());
        return;
    }

    if (Command->Code == XhciCompletionCode::Success)
    {
        if (!BlockSetAddress && m_PendingKind == PendingKind::Address)
            ((PUSBDEVICE_ADDRESS)m_PendingPayload)->Address = OutputSlot()->UsbDeviceAddress;

        DPRINT("Slot %lu addressed (BSR %u), USB address %u\n",
               m_SlotId, BlockSetAddress, OutputSlot()->UsbDeviceAddress);
        CompletePending(STATUS_SUCCESS);
        return;
    }

    DPRINT1("Address Device (BSR %u) for slot %lu failed with completion code %lu\n",
            BlockSetAddress, m_SlotId, static_cast<ULONG>(Command->Code));

    /* The slot stays enabled until the hub sends DISABLE */
    if (!BlockSetAddress)
    {
        CompletePending(STATUS_UNSUCCESSFUL);
        return;
    }

    if (m_MustSucceed)
    {
        m_Controller->VerifierCheck(XhciDeviceCheckAddressDefault);
        m_Controller->RaiseControllerFault(XhciRecovery::ResetHost, XhciDeviceFatalAddressDefault);
        return;
    }

    SubmitDisableSlot(AfterDisable::CompleteFailure);
}

/* Update and hub information ***********************************************/

VOID
XhciUsbDevice::ApplyUpdate(
    _Inout_ PUSBDEVICE_UPDATE Payload)
{
    ULONG RootPort = m_Info.PortPath.PortPath[0];
    PUCXHUB_UPDATE_TUNNEL_ANSWER Answer;

    /* QUIRK: a device never stops being a hub */
    if (Payload->Flags.UpdateIsHub && Payload->IsHub)
        m_IsHub = TRUE;

    if (Payload->Flags.UpdateDeviceDescriptor && Payload->DeviceDescriptor != NULL)
        RtlCopyMemory(&m_DeviceDescriptor, Payload->DeviceDescriptor, sizeof(m_DeviceDescriptor));

    if (Payload->Flags.Update20HardwareLpmParameters)
    {
        ASSERT(!m_IsHub);
        m_LpmEnabled = (BOOLEAN)Payload->Usb20HardwareLpmParameters.HardwareLpmEnable;
        m_Controller->m_RootHub.Update20HardwareLpm(RootPort, m_SlotId, &Payload->Usb20HardwareLpmParameters);
    }

    if (Payload->Flags.UpdateRootPortResumeTime)
    {
        m_Controller->m_RootHub.ArmPortResumeTimer(RootPort, Payload->RootPortResumeTime);
        m_ResumeTimeSet = TRUE;
    }

    /* The ACPI path fills the answer before it gets here */
    if (Payload->Flags.UpdateTunnelState && !m_Controller->HasErrata(XhciErrata::Usb4TunnelFromAcpiDsm))
    {
        Answer = UcxHubUpdateTunnelAnswer(Payload);
        Answer->TunnelState = m_Controller->m_RootHub.QueryTunnelState(RootPort);
        Answer->NativeLink = Answer->TunnelState != UCXHUB_TUNNEL_STATE_TUNNELED &&
                             Answer->TunnelState != UCXHUB_TUNNEL_STATE_UNKNOWN;
    }
}

BOOLEAN
XhciUsbDevice::WantsTunnelStateDsm(
    _In_ const USBDEVICE_UPDATE* Payload) const
{
    return Payload->Flags.UpdateTunnelState && m_Controller->HasErrata(XhciErrata::Usb4TunnelFromAcpiDsm);
}

/** FALSE when the work item could not be created; the caller then completes the request itself. */
BOOLEAN
XhciUsbDevice::QueueTunnelStateDsm(
    _Inout_ PUSBDEVICE_UPDATE Payload)
{
    WDF_WORKITEM_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;
    PXHCI_TUNNEL_DSM_WORK Work;
    WDFWORKITEM Item;
    NTSTATUS Status;

    WDF_WORKITEM_CONFIG_INIT(&Config, TunnelDsmWorker);

    /* QUIRK: the controller device has no synchronization scope, so this serializes nothing */
    Config.AutomaticSerialization = TRUE;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XHCI_TUNNEL_DSM_WORK);
    Attributes.ParentObject = m_Controller->m_Device;

    Status = WdfWorkItemCreate(&Config, &Attributes, &Item);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("No tunnel state work item for root port %lu, 0x%lx\n", m_Info.PortPath.PortPath[0], Status);

        /* QUIRK: the native link flag keeps the hub's value */
        UcxHubUpdateTunnelAnswer(Payload)->TunnelState = UCXHUB_TUNNEL_STATE_UNKNOWN;
        return FALSE;
    }

    /* Keeps this context alive even if a removal completes the request first */
    WdfObjectReference(m_Handle);

    Work = XhciGetTunnelDsmWork(Item);
    Work->Device = this;
    Work->Payload = Payload;
    WdfWorkItemEnqueue(Item);
    return TRUE;
}

VOID
NTAPI
XhciUsbDevice::TunnelDsmWorker(
    _In_ WDFWORKITEM WorkItem)
{
    PXHCI_TUNNEL_DSM_WORK Work = XhciGetTunnelDsmWork(WorkItem);
    XhciUsbDevice* Device = Work->Device;

    Device->RunTunnelStateDsm(Work->Payload);
    WdfObjectDereference(Device->m_Handle);
    WdfObjectDelete(WorkItem);
}

VOID
XhciUsbDevice::RunTunnelStateDsm(
    _Inout_ PUSBDEVICE_UPDATE Payload)
{
    ULONG RootPort = m_Info.PortPath.PortPath[0];
    PUCXHUB_UPDATE_TUNNEL_ANSWER Answer;
    WDFREQUEST Request;
    ULONG64 Result;

    PAGED_CODE();

    /* QUIRK: function 10 is used without asking function 0 whether it exists */
    Result = XhciEvaluateDsmValue(m_Controller->m_Device,
                                  &XhciGuidUsbDsm,
                                  XHCI_USB_DSM_REVISION,
                                  XHCI_USB_DSM_TUNNEL_STATE,
                                  &RootPort,
                                  sizeof(RootPort));
    DPRINT("Root port %lu tunnel state _DSM returned %I64u\n", RootPort, Result);

    /* A removal may have completed the request, and the payload with it, while _DSM ran */
    Request = TakePending();
    if (Request == NULL)
        return;

    Answer = UcxHubUpdateTunnelAnswer(Payload);
    if (Result == XHCI_USB_DSM_PORT_NATIVE)
    {
        Answer->TunnelState = UCXHUB_TUNNEL_STATE_NATIVE;
    }
    else if (Result == XHCI_USB_DSM_PORT_TUNNELED)
    {
        Answer->TunnelState = UCXHUB_TUNNEL_STATE_TUNNELED;
    }
    else
    {
        DPRINT1("Root port %lu tunnel state _DSM gave %I64u, state unknown\n", RootPort, Result);
        Answer->TunnelState = UCXHUB_TUNNEL_STATE_UNKNOWN;
    }
    Answer->NativeLink = (Result == XHCI_USB_DSM_PORT_NATIVE);

    ApplyUpdate(Payload);
    WdfRequestComplete(Request, STATUS_SUCCESS);
}

VOID
XhciUsbDevice::Update(
    _In_ WDFREQUEST Request,
    _Inout_ PUSBDEVICE_UPDATE Payload)
{
    PXHCI_SLOT_CONTEXT Slot;
    ULONG64 InputAddress;

    if (!Payload->Flags.UpdateMaxExitLatency)
    {
        if (WantsTunnelStateDsm(Payload))
        {
            /* Record the request first so the work item has one to complete */
            SetPending(Request, Payload, PendingKind::Update, FALSE);
            if (QueueTunnelStateDsm(Payload))
                return;

            TakePending();
        }

        ApplyUpdate(Payload);
        WdfRequestComplete(Request, STATUS_SUCCESS);
        return;
    }

    /* Evaluate Context with only the slot flagged (xHCI 4.6.7) */
    ZeroInputContext();
    InputControl()->AddFlags = 1;
    Slot = InputSlot();
    Slot->MaxExitLatency = Payload->MaxExitLatency & 0xFFFF;
    Slot->InterrupterTarget = 0;

    /* QUIRK: a device without a slot sends this with Slot ID 0 */
    InputAddress = InputContextAddress();
    PrepareCommand(static_cast<ULONG>(XhciTrbType::EvaluateContext));
    m_Command.Trb.Dword[0] = (ULONG)InputAddress;
    m_Command.Trb.Dword[1] = (ULONG)(InputAddress >> 32);

    if (!m_Controller->IsAccessible())
    {
        DPRINT1("UPDATE for device %p with the controller gone\n", this);
        WdfRequestComplete(Request, STATUS_NO_SUCH_DEVICE);
        return;
    }

    SetPending(Request, Payload, PendingKind::Update, FALSE);
    m_Controller->m_Commands.Submit(&m_Command);
}

VOID
XhciUsbDevice::OnEvaluateContextDone(
    _In_ const XhciCommand* Command)
{
    PUSBDEVICE_UPDATE Payload = (PUSBDEVICE_UPDATE)m_PendingPayload;
    NTSTATUS Status;

    if (Command->Status == STATUS_NO_SUCH_DEVICE)
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    else if (Command->Code == XhciCompletionCode::Success)
    {
        /* The work item completes the request */
        if (WantsTunnelStateDsm(Payload) && QueueTunnelStateDsm(Payload))
            return;

        ApplyUpdate(Payload);
        Status = STATUS_SUCCESS;
    }
    else
    {
        DPRINT1("Max Exit Latency %lu for slot %lu rejected with completion code %lu\n",
                Payload->MaxExitLatency, m_SlotId, static_cast<ULONG>(Command->Code));

        if (Command->Code == XhciCompletionCode::MaxExitLatencyTooLarge)
            Payload->FailureFlags.MaxExitLatencyTooLarge = 1;

        Status = STATUS_UNSUCCESSFUL;
    }

    CompletePending(Status);
}

VOID
XhciUsbDevice::HubInfo(
    _In_ WDFREQUEST Request,
    _In_ const USBDEVICE_HUB_INFO* Payload)
{
    /* QUIRK: accepted whether or not the device is a hub */
    m_NumberOfPorts = Payload->NumberOfPorts;
    m_NumberOfTTs = Payload->NumberOfTTs;
    m_TTThinkTime = Payload->TTThinkTime;

    DPRINT("Hub %p: %lu ports, %lu TTs, think time %lu\n",
           this, m_NumberOfPorts, m_NumberOfTTs, m_TTThinkTime);
    WdfRequestComplete(Request, STATUS_SUCCESS);
}

/* Configure Endpoint *******************************************************/

VOID
XhciUsbDevice::BuildSlotContext(
    _Out_writes_bytes_(ContextSize) PVOID SlotContext,
    _In_ ULONG ContextSize) const
{
    PXHCI_SLOT_CONTEXT Slot = (PXHCI_SLOT_CONTEXT)SlotContext;
    XhciUsbDevice* Hub;

    /* Only the hub fields; the caller zeroed the context and owns Context Entries */
    ASSERT(ContextSize >= sizeof(*Slot));
    UNREFERENCED_PARAMETER(ContextSize);

    if (m_IsHub)
    {
        Slot->Hub = 1;
        Slot->NumberOfPorts = m_NumberOfPorts;

        if (m_Info.DeviceSpeed == UsbHighSpeed)
        {
            Slot->MultiTT = (m_NumberOfTTs > 1);
            Slot->TTThinkTime = m_TTThinkTime;
        }
    }

    /* Some controllers need MTT repeated for LS and FS devices behind a multi TT hub */
    if (m_Controller->HasErrata(XhciErrata::CfgMultiTtFromHubData) &&
        !m_IsHub &&
        (m_Info.DeviceSpeed == UsbLowSpeed || m_Info.DeviceSpeed == UsbFullSpeed))
    {
        Hub = TtHub();
        if (Hub != NULL && Hub->m_NumberOfTTs > 1)
            Slot->MultiTT = 1;
    }
}

VOID
XhciUsbDevice::EndpointsConfigure(
    _In_ WDFREQUEST Request,
    _Inout_ PENDPOINTS_CONFIGURE Payload)
{
    XhciRequestData* Data = XhciGetRequestData(Request);
    PXHCI_INPUT_CONTROL_CONTEXT Control;
    XhciEndpoint* Endpoint;
    ULONG EnableCount = Payload->EndpointsToEnableCount;
    ULONG DisableCount = Payload->EndpointsToDisableCount;
    ULONG64 InputAddress;
    ULONG AddFlags;
    ULONG DropFlags;
    ULONG Index;
    NTSTATUS Status;

    if (Data == NULL)
    {
        DPRINT1("ENDPOINTS_CONFIGURE for device %p carries no request context\n", this);
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_STATE);
        return;
    }

    Data->EndpointsConfigure.EnableFailed = FALSE;
    Data->EndpointsConfigure.FirstCommandFailed = FALSE;

    /* Nothing is disabled here: the endpoints to drop never reached the hardware */
    if (!m_SlotEnabled)
    {
        DPRINT1("ENDPOINTS_CONFIGURE for device %p without a slot\n", this);
        WdfRequestComplete(Request, EnableCount != 0 ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS);
        return;
    }

    if (!m_Controller->IsAccessible())
    {
        DPRINT1("ENDPOINTS_CONFIGURE for device %p with the controller gone\n", this);
        DisableEndpointList(DisableCount, Payload->EndpointsToDisable);
        WdfRequestComplete(Request, EnableCount != 0 ? STATUS_NO_SUCH_DEVICE : STATUS_SUCCESS);
        return;
    }

    SetPending(Request, Payload, PendingKind::Configure, EnableCount == 0);

    ZeroInputContext();
    BuildSlotContext(InputSlot(), m_ContextSize);

    for (Index = 0; Index < EnableCount; Index++)
    {
        Status = XhciEndpoint::FromUcx(Payload->EndpointsToEnable[Index])->Enable();
        if (NT_SUCCESS(Status))
            continue;

        DPRINT1("Endpoint %lu of %lu for device %p failed to enable: 0x%lx\n",
                Index, EnableCount, this, Status);

        while (Index-- > 0)
            XhciEndpoint::FromUcx(Payload->EndpointsToEnable[Index])->Disable();

        if (DisableCount == 0)
        {
            CompletePending(Status);
            return;
        }

        /* The drops still have to reach the controller */
        Data->EndpointsConfigure.EnableFailed = TRUE;
        break;
    }

    AddFlags = 1;
    if (!Data->EndpointsConfigure.EnableFailed)
    {
        for (Index = 0; Index < EnableCount; Index++)
        {
            Endpoint = XhciEndpoint::FromUcx(Payload->EndpointsToEnable[Index]);
            Endpoint->BuildContext(InputContext(Endpoint->Dci() + 1), m_ContextSize);
            AddFlags |= 1UL << Endpoint->Dci();
        }
    }

    DropFlags = 0;
    for (Index = 0; Index < DisableCount; Index++)
        DropFlags |= 1UL << XhciEndpoint::FromUcx(Payload->EndpointsToDisable[Index])->Dci();

    Control = InputControl();
    Control->AddFlags = AddFlags;
    Control->DropFlags = DropFlags;

    /* QUIRK: updated before the command result is known and never rolled back */
    m_ContextMask = (m_ContextMask & ~DropFlags) | AddFlags;
    InputSlot()->ContextEntries = ContextEntries();

    InputAddress = InputContextAddress();
    PrepareCommand(static_cast<ULONG>(XhciTrbType::ConfigureEndpoint));
    m_Command.Trb.Dword[0] = (ULONG)InputAddress;
    m_Command.Trb.Dword[1] = (ULONG)(InputAddress >> 32);
    m_Controller->m_Commands.Submit(&m_Command);
}

VOID
XhciUsbDevice::SubmitSecondConfigure()
{
    PENDPOINTS_CONFIGURE Payload = (PENDPOINTS_CONFIGURE)m_PendingPayload;
    XhciRequestData* Data = XhciGetRequestData(m_PendingRequest);
    ULONG64 InputAddress;
    ULONG DropFlags;
    ULONG Index;

    Data->EndpointsConfigure.FirstCommandFailed = TRUE;
    DisableEndpointList(Payload->EndpointsToEnableCount, Payload->EndpointsToEnable);

    /* QUIRK: Context Entries stays 0 in this one */
    ZeroInputContext();
    BuildSlotContext(InputSlot(), m_ContextSize);

    DropFlags = 0;
    for (Index = 0; Index < Payload->EndpointsToDisableCount; Index++)
        DropFlags |= 1UL << XhciEndpoint::FromUcx(Payload->EndpointsToDisable[Index])->Dci();

    InputControl()->AddFlags = 1;
    InputControl()->DropFlags = DropFlags;

    InputAddress = InputContextAddress();
    PrepareCommand(static_cast<ULONG>(XhciTrbType::ConfigureEndpoint));
    m_Command.Trb.Dword[0] = (ULONG)InputAddress;
    m_Command.Trb.Dword[1] = (ULONG)(InputAddress >> 32);
    m_Controller->m_Commands.Submit(&m_Command);
}

VOID
XhciUsbDevice::OnConfigureEndpointDone(
    _In_ const XhciCommand* Command)
{
    PENDPOINTS_CONFIGURE Payload = (PENDPOINTS_CONFIGURE)m_PendingPayload;
    XhciRequestData* Data = XhciGetRequestData(m_PendingRequest);
    ULONG EnableCount = Payload->EndpointsToEnableCount;
    ULONG DisableCount = Payload->EndpointsToDisableCount;
    ULONG Parameter = Command->Completion.Dword[2] & XHCI_COMPLETION_PARAMETER_MASK;
    BOOLEAN EarlierFailure;
    BOOLEAN SlotLost;

    EarlierFailure = Data->EndpointsConfigure.EnableFailed || Data->EndpointsConfigure.FirstCommandFailed;

    if (Command->Status != STATUS_NO_SUCH_DEVICE && Command->Code == XhciCompletionCode::Success)
    {
        DisableEndpointList(DisableCount, Payload->EndpointsToDisable);
        if (!EarlierFailure)
            RecordEnabledEndpoints(Payload);

        Payload->ExitLatencyDelta = Parameter;
        CompletePending(EarlierFailure ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS);
        return;
    }

    SlotLost = (Command->Status == STATUS_NO_SUCH_DEVICE ||
                Command->Code == XhciCompletionCode::SlotNotEnabled);

    if (Command->Status != STATUS_NO_SUCH_DEVICE)
    {
        DPRINT1("Configure Endpoint for slot %lu failed with completion code %lu (enable %lu, disable %lu)\n",
                m_SlotId, static_cast<ULONG>(Command->Code), EnableCount, DisableCount);

        if (!SlotLost && EnableCount != 0)
        {
            switch (Command->Code)
            {
                case XhciCompletionCode::BandwidthError:
                case XhciCompletionCode::SecondaryBandwidthError:
                    Payload->FailureFlags.InsufficientBandwidth = 1;
                    break;
                case XhciCompletionCode::ResourceError:
                    Payload->FailureFlags.InsufficientHardwareResourcesForEndpoints = 1;
                    break;
                case XhciCompletionCode::MaxExitLatencyTooLarge:
                    Payload->FailureFlags.MaxExitLatencyTooLarge = 1;
                    break;
                default:
                    break;
            }
        }

        if (Command->Code == XhciCompletionCode::MaxExitLatencyTooLarge)
            Payload->ExitLatencyDelta = Parameter;
    }

    /* Endpoints to enable that an earlier failure already disabled are not disabled again */
    if (SlotLost)
    {
        DisableEndpointList(DisableCount, Payload->EndpointsToDisable);
        if (EnableCount != 0 && !EarlierFailure)
            DisableEndpointList(EnableCount, Payload->EndpointsToEnable);

        CompletePending(EnableCount != 0 ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS);
        return;
    }

    /* QUIRK: with nothing to enable or disable this fails although UCX expects success */
    if (DisableCount == 0)
    {
        DisableEndpointList(EnableCount, Payload->EndpointsToEnable);
        CompletePending(STATUS_UNSUCCESSFUL);
        return;
    }

    if (EnableCount == 0 || EarlierFailure)
    {
        m_Controller->VerifierCheck(XhciDeviceCheckDeconfigure);
        m_Controller->RaiseControllerFault(XhciRecovery::ResetHost, XhciDeviceFatalDeconfigure);
        return;
    }

    /* The enables failed in hardware; drop the endpoints to disable on their own */
    SubmitSecondConfigure();
}

/* Disable Slot and Reset Device ********************************************/

VOID
XhciUsbDevice::SubmitDisableSlot(
    _In_ AfterDisable Next)
{
    m_AfterDisable = Next;
    PrepareCommand(static_cast<ULONG>(XhciTrbType::DisableSlot));
    m_Controller->m_Commands.Submit(&m_Command);
}

VOID
XhciUsbDevice::Disable(
    _In_ WDFREQUEST Request)
{
    ULONG RootPort = m_Info.PortPath.PortPath[0];

    if (!m_SlotEnabled)
    {
        DisableDefaultEndpoint();
        DisableConfiguredEndpoints(TRUE);
        WdfRequestComplete(Request, STATUS_SUCCESS);
        return;
    }

    /* QUIRK: neither flag is cleared afterwards */
    if (m_LpmEnabled)
        m_Controller->m_RootHub.DisableLpmForSlot(RootPort, m_SlotId);
    if (m_ResumeTimeSet)
        m_Controller->m_RootHub.DisarmPortResumeTimer(RootPort);

    if (!m_Controller->IsAccessible())
    {
        DPRINT("DISABLE for device %p with the controller gone\n", this);
        SetDisabled();
        WdfRequestComplete(Request, STATUS_SUCCESS);
        return;
    }

    SetPending(Request, NULL, PendingKind::Disable, TRUE);
    SubmitDisableSlot(AfterDisable::CompleteSuccess);
}

VOID
XhciUsbDevice::OnDisableSlotDone(
    _In_ const XhciCommand* Command)
{
    AfterDisable Next = m_AfterDisable;

    if (Command->Status == STATUS_NO_SUCH_DEVICE)
    {
        SetDisabled();
        return;
    }

    if (Command->Code != XhciCompletionCode::Success)
    {
        DPRINT1("Disable Slot %lu completed with code %lu\n", m_SlotId, static_cast<ULONG>(Command->Code));

        if (Command->Code != XhciCompletionCode::SlotNotEnabled)
        {
            m_Controller->VerifierCheck(XhciDeviceCheckDisableSlot);
            m_Controller->RaiseControllerFault(XhciRecovery::ResetHost, XhciDeviceFatalDisableSlot);
            return;
        }
    }

    /* The default endpoint keeps its table entry; a re-enable follows for RESET */
    if (Next != AfterDisable::EnableSlot)
        DisableDefaultEndpoint();

    DisableConfiguredEndpoints(TRUE);
    ForgetSlot();

    if (Next == AfterDisable::EnableSlot)
    {
        SubmitEnableSlot();
        return;
    }

    CompletePending(Next == AfterDisable::CompleteSuccess ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

VOID
XhciUsbDevice::Reset(
    _In_ WDFREQUEST Request)
{
    ULONG Type;

    if (!m_SlotEnabled)
    {
        WdfRequestComplete(Request, STATUS_SUCCESS);
        return;
    }

    /* Enabled or Default: nothing to reset */
    if (OutputSlot()->SlotState <= XHCI_SLOT_STATE_DEFAULT)
    {
        WdfRequestComplete(Request, STATUS_SUCCESS);
        return;
    }

    if (!m_Controller->IsAccessible())
    {
        DPRINT("RESET for device %p with the controller gone\n", this);
        DisableConfiguredEndpoints(FALSE);
        m_ContextMask = XHCI_DEFAULT_CONTEXT_MASK;
        WdfRequestComplete(Request, STATUS_SUCCESS);
        return;
    }

    SetPending(Request, NULL, PendingKind::Reset, TRUE);

    if (m_Controller->HasErrata(XhciErrata::EpResetViaSlotCycle))
    {
        Type = static_cast<ULONG>(XhciTrbType::DisableSlot);
        m_AfterDisable = AfterDisable::EnableSlot;
    }
    else
    {
        Type = static_cast<ULONG>(XhciTrbType::ResetDevice);
    }

    /* QUIRK: the block is not cleared, so the reserved TRB fields keep the last command's bits */
    m_Command.Trb.Dword[3] &= ~(XHCI_TRB_TYPE_MASK | (0xFFUL << XHCI_TRB_SLOT_SHIFT));
    m_Command.Trb.Dword[3] |= (Type << XHCI_TRB_TYPE_SHIFT) | (m_SlotId << XHCI_TRB_SLOT_SHIFT);
    m_Command.Done = CommandDone;
    m_Command.Context = this;
    m_Controller->m_Commands.Submit(&m_Command);
}

VOID
XhciUsbDevice::OnResetDeviceDone(
    _In_ const XhciCommand* Command)
{
    if (Command->Status != STATUS_NO_SUCH_DEVICE)
    {
        if (Command->Code != XhciCompletionCode::Success)
        {
            DPRINT1("Reset Device for slot %lu failed with completion code %lu\n",
                    m_SlotId, static_cast<ULONG>(Command->Code));
            m_Controller->VerifierCheck(XhciDeviceCheckResetDevice);
            m_Controller->RaiseControllerFault(XhciRecovery::ResetHost, XhciDeviceFatalResetDevice);
            return;
        }

        if (EndpointState(1) == XHCI_ENDPOINT_STATE_DISABLED)
            DPRINT1("Reset Device for slot %lu left the default endpoint disabled\n", m_SlotId);
    }

    /* The table keeps the endpoints; the hub configures them again */
    DisableConfiguredEndpoints(FALSE);
    m_ContextMask = XHCI_DEFAULT_CONTEXT_MASK;
    CompletePending(STATUS_SUCCESS);
}

/* Single endpoint reconfigure **********************************************/

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XhciUsbDevice::ReconfigureEndpoint(
    _In_ XhciEndpoint* Endpoint,
    _In_ VOID (NTAPI *Done)(_In_ PVOID Context, _In_ NTSTATUS Status),
    _In_ PVOID Context)
{
    ReconfigureWaiter Waiter;
    ULONG Dci = Endpoint->Dci();
    KIRQL OldIrql;

    Waiter.Endpoint = Endpoint;
    Waiter.Done = Done;
    Waiter.Context = Context;

    /* Reconfigures run one at a time on their own command block, apart from device requests */
    KeAcquireSpinLock(&m_ReconfigureLock, &OldIrql);
    if (m_ReconfigureBusy)
    {
        ASSERT((m_ReconfigureQueued & (1UL << Dci)) == 0);
        m_ReconfigureWaiters[Dci] = Waiter;
        m_ReconfigureQueued |= 1UL << Dci;
        KeReleaseSpinLock(&m_ReconfigureLock, OldIrql);
        return;
    }

    m_ReconfigureBusy = TRUE;
    KeReleaseSpinLock(&m_ReconfigureLock, OldIrql);

    StartReconfigure(&Waiter);
}

VOID
XhciUsbDevice::StartReconfigure(
    _In_ const ReconfigureWaiter* Waiter)
{
    PXHCI_INPUT_CONTROL_CONTEXT Control;
    PXHCI_SLOT_CONTEXT Slot;
    XhciEndpoint* Endpoint;
    PUCHAR Base;
    ULONG64 InputAddress;
    ULONG Dci;

    m_ReconfigureCurrent = *Waiter;
    Endpoint = m_ReconfigureCurrent.Endpoint;
    Dci = Endpoint->Dci();

    if (m_ReconfigureInput == NULL)
    {
        m_ReconfigureInput = m_Controller->m_Buffers.Acquire(m_ContextSize * XHCI_INPUT_CONTEXT_COUNT);
        if (m_ReconfigureInput == NULL)
        {
            DPRINT1("No input context to reconfigure DCI %lu of slot %lu\n", Dci, m_SlotId);
            FinishReconfigure(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }
    }

    Base = (PUCHAR)m_ReconfigureInput->VirtualAddress;
    RtlZeroMemory(Base, m_ContextSize * XHCI_INPUT_CONTEXT_COUNT);

    Control = (PXHCI_INPUT_CONTROL_CONTEXT)Base;
    Slot = (PXHCI_SLOT_CONTEXT)(Base + m_ContextSize);

    BuildSlotContext(Slot, m_ContextSize);
    Endpoint->BuildContext(Base + (Dci + 1) * m_ContextSize, m_ContextSize);
    Slot->ContextEntries = ContextEntries();

    /* Drop and add of the same DCI reconfigures it in place (xHCI 4.6.6) */
    Control->AddFlags = 1 | (1UL << Dci);
    Control->DropFlags = 1UL << Dci;

    InputAddress = m_ReconfigureInput->LogicalAddress.QuadPart;
    RtlZeroMemory(&m_ReconfigureCommand, sizeof(m_ReconfigureCommand));
    m_ReconfigureCommand.Trb.Dword[0] = (ULONG)InputAddress;
    m_ReconfigureCommand.Trb.Dword[1] = (ULONG)(InputAddress >> 32);
    m_ReconfigureCommand.Trb.Dword[3] =
        (static_cast<ULONG>(XhciTrbType::ConfigureEndpoint) << XHCI_TRB_TYPE_SHIFT) |
        (m_SlotId << XHCI_TRB_SLOT_SHIFT);
    m_ReconfigureCommand.Done = ReconfigureCommandDone;
    m_ReconfigureCommand.Context = this;
    m_Controller->m_Commands.Submit(&m_ReconfigureCommand);
}

VOID
NTAPI
XhciUsbDevice::ReconfigureCommandDone(
    _In_ XhciCommand* Command)
{
    XhciUsbDevice* Device = static_cast<XhciUsbDevice*>(Command->Context);
    NTSTATUS Status = STATUS_SUCCESS;

    if (!XhciCommandSucceeded(Command))
    {
        DPRINT1("Reconfigure of DCI %lu on slot %lu failed: status 0x%lx, code %lu\n",
                Device->m_ReconfigureCurrent.Endpoint->Dci(), Device->m_SlotId,
                Command->Status, static_cast<ULONG>(Command->Code));
        Status = STATUS_UNSUCCESSFUL;
    }

    Device->FinishReconfigure(Status);
}

VOID
XhciUsbDevice::FinishReconfigure(
    _In_ NTSTATUS Status)
{
    ReconfigureWaiter Finished = m_ReconfigureCurrent;
    ReconfigureWaiter Next;
    BOOLEAN HaveNext = FALSE;
    KIRQL OldIrql;
    ULONG Dci;

    RtlZeroMemory(&m_ReconfigureCurrent, sizeof(m_ReconfigureCurrent));
    Finished.Done(Finished.Context, Status);

    KeAcquireSpinLock(&m_ReconfigureLock, &OldIrql);
    if (m_ReconfigureQueued != 0)
    {
        Dci = 0;
        while ((m_ReconfigureQueued & (1UL << Dci)) == 0)
            Dci++;

        Next = m_ReconfigureWaiters[Dci];
        m_ReconfigureQueued &= ~(1UL << Dci);
        HaveNext = TRUE;
    }
    else
    {
        m_ReconfigureBusy = FALSE;
    }
    KeReleaseSpinLock(&m_ReconfigureLock, OldIrql);

    if (HaveNext)
        StartReconfigure(&Next);
}

/* Events and controller walks **********************************************/

VOID
XhciUsbDevice::OnTransferEvent(
    _In_ const XHCI_TRB* Event)
{
    ULONG Dci = (Event->Dword[3] & XHCI_TRB_ENDPOINT_MASK) >> XHCI_TRB_ENDPOINT_SHIFT;
    XhciEndpoint* Endpoint = EndpointAt(Dci);

    if (Endpoint != NULL)
    {
        Endpoint->OnTransferEvent(Event);
        return;
    }

    /* QUIRK: isochronous leftovers after a deconfigure are dropped without a word */
    if ((Event->Dword[0] & XHCI_EVENT_DATA_TYPE_MASK) != USB_ENDPOINT_TYPE_ISOCHRONOUS)
    {
        DPRINT1("Transfer event for slot %lu DCI %lu code %lu has no endpoint, dropped\n",
                m_SlotId, Dci, XhciTrbCompletionCode(Event));
    }
}

VOID
XhciUsbDevice::OnDeviceNotificationEvent(
    _In_ const XHCI_TRB* Event)
{
    ULONG Type = (Event->Dword[0] & XHCI_NOTIFICATION_TYPE_MASK) >> XHCI_NOTIFICATION_TYPE_SHIFT;
    ULONG Interface;

    if (Type != XHCI_NOTIFICATION_FUNCTION_WAKE)
    {
        DPRINT("Device notification type %lu for slot %lu ignored\n", Type, m_SlotId);
        return;
    }

    Interface = (Event->Dword[0] & XHCI_NOTIFICATION_DATA_MASK) >> XHCI_NOTIFICATION_DATA_SHIFT;
    DPRINT("Function wake from interface %lu of slot %lu\n", Interface, m_SlotId);
    UcxUsbDeviceRemoteWakeNotification(m_Handle, Interface);
}

VOID
XhciUsbDevice::ControllerResetStarting()
{
    ULONG Dci;

    for (Dci = 1; Dci <= XHCI_MAX_DCI; Dci++)
    {
        if (m_Endpoints[Dci] != NULL)
            m_Endpoints[Dci]->ControllerResetStarting();
    }
}

VOID
XhciUsbDevice::ControllerResetDone()
{
    ULONG Dci;

    PAGED_CODE();

    /* One endpoint at a time; each may wait for its machine to acknowledge */
    for (Dci = 1; Dci <= XHCI_MAX_DCI; Dci++)
    {
        if (m_Endpoints[Dci] != NULL)
            m_Endpoints[Dci]->ControllerResetDone();
    }
}

VOID
XhciUsbDevice::OnHostLost()
{
    WDFREQUEST Request;
    ULONG Dci;

    for (Dci = 1; Dci <= XHCI_MAX_DCI; Dci++)
    {
        if (m_Endpoints[Dci] != NULL)
            m_Endpoints[Dci]->ControllerRemoved();
    }

    Request = TakePending();
    if (Request != NULL)
        WdfRequestComplete(Request, m_MustSucceed ? STATUS_SUCCESS : STATUS_INTERNAL_ERROR);
}

/* UCX callbacks ************************************************************/

NTSTATUS
NTAPI
XhciEvtControllerUsbDeviceAdd(
    _In_ UCXCONTROLLER UcxController,
    _In_ PUCXUSBDEVICE_INFO UcxUsbDeviceInfo,
    _In_ PUCXUSBDEVICE_INIT UsbDeviceInit)
{
    UCX_USBDEVICE_EVENT_CALLBACKS Callbacks;
    WDF_OBJECT_ATTRIBUTES Attributes;
    UCXUSBDEVICE UsbDevice;
    NTSTATUS Status;

    PAGED_CODE();

    UCX_USBDEVICE_EVENT_CALLBACKS_INIT(&Callbacks,
                                       XhciEvtUsbDeviceEndpointsConfigure,
                                       XhciEvtUsbDeviceEnable,
                                       XhciEvtUsbDeviceDisable,
                                       XhciEvtUsbDeviceReset,
                                       XhciEvtUsbDeviceAddress,
                                       XhciEvtUsbDeviceUpdate,
                                       XhciEvtUsbDeviceHubInfo,
                                       XhciEvtUsbDeviceDefaultEndpointAdd,
                                       XhciEvtUsbDeviceEndpointAdd);

    /* Suspend, resume and characteristics are not offered, so the structure ends before them */
    Callbacks.Size = FIELD_OFFSET(UCX_USBDEVICE_EVENT_CALLBACKS, EvtUsbDeviceSuspend);
    UcxUsbDeviceInitSetEventCallbacks(UsbDeviceInit, &Callbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XhciUsbDevice);
    Attributes.EvtCleanupCallback = XhciEvtUsbDeviceCleanup;

    Status = UcxUsbDeviceCreate(UcxController, &UsbDeviceInit, &Attributes, &UsbDevice);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UcxUsbDeviceCreate failed 0x%lx\n", Status);
        return Status;
    }

    /* On failure UCX deletes the object and the cleanup callback frees what was acquired */
    return XhciUsbDevice::FromUcx(UsbDevice)->Initialize(XhciController::FromUcx(UcxController),
                                                         UsbDevice,
                                                         UcxUsbDeviceInfo);
}

VOID
NTAPI
XhciEvtUsbDeviceEndpointsConfigure(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    PENDPOINTS_CONFIGURE Payload = (PENDPOINTS_CONFIGURE)XhciRequestPayload(Request);

    UNREFERENCED_PARAMETER(UcxController);
    XhciUsbDevice::FromUcx(Payload->Header.UsbDevice)->EndpointsConfigure(Request, Payload);
}

VOID
NTAPI
XhciEvtUsbDeviceEnable(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    PUSBDEVICE_ENABLE Payload = (PUSBDEVICE_ENABLE)XhciRequestPayload(Request);

    UNREFERENCED_PARAMETER(UcxController);
    XhciUsbDevice::FromUcx(Payload->Header.UsbDevice)->Enable(Request, Payload);
}

VOID
NTAPI
XhciEvtUsbDeviceDisable(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    PUSBDEVICE_DISABLE Payload = (PUSBDEVICE_DISABLE)XhciRequestPayload(Request);

    UNREFERENCED_PARAMETER(UcxController);
    XhciUsbDevice::FromUcx(Payload->Header.UsbDevice)->Disable(Request);
}

VOID
NTAPI
XhciEvtUsbDeviceReset(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    PUSBDEVICE_RESET Payload = (PUSBDEVICE_RESET)XhciRequestPayload(Request);

    UNREFERENCED_PARAMETER(UcxController);
    XhciUsbDevice::FromUcx(Payload->Header.UsbDevice)->Reset(Request);
}

VOID
NTAPI
XhciEvtUsbDeviceAddress(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    PUSBDEVICE_ADDRESS Payload = (PUSBDEVICE_ADDRESS)XhciRequestPayload(Request);

    UNREFERENCED_PARAMETER(UcxController);
    XhciUsbDevice::FromUcx(Payload->Header.UsbDevice)->Address(Request, Payload);
}

VOID
NTAPI
XhciEvtUsbDeviceUpdate(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    PUSBDEVICE_UPDATE Payload = (PUSBDEVICE_UPDATE)XhciRequestPayload(Request);

    UNREFERENCED_PARAMETER(UcxController);
    XhciUsbDevice::FromUcx(Payload->Header.UsbDevice)->Update(Request, Payload);
}

VOID
NTAPI
XhciEvtUsbDeviceHubInfo(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    PUSBDEVICE_HUB_INFO Payload = (PUSBDEVICE_HUB_INFO)XhciRequestPayload(Request);

    UNREFERENCED_PARAMETER(UcxController);
    XhciUsbDevice::FromUcx(Payload->Header.UsbDevice)->HubInfo(Request, Payload);
}
