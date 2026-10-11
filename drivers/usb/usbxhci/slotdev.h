/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     USB device (xHCI slot) object and the UCX USB device callbacks
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

class XhciController;
class XhciEndpoint;

/** Highest Device Context Index (xHCI 4.5.1). */
#define XHCI_MAX_DCI 31

/** One USB device and its xHCI slot. Lives in the UCXUSBDEVICE context. */
class XhciUsbDevice
{
public:
    static XhciUsbDevice*
    FromUcx(
        _In_ UCXUSBDEVICE UsbDevice);

    /* Context access for the endpoint module (input: control 0, slot 1, DCI n at n + 1) */
    PVOID
    InputContext(
        _In_ ULONG Index) const;
    PVOID
    OutputContext(
        _In_ ULONG Dci) const;
    ULONG64 InputContextAddress() const;
    VOID ZeroInputContext();

    /** Fills the hub fields of an input slot context and applies the multi TT errata rule. */
    VOID
    BuildSlotContext(
        _Out_writes_bytes_(ContextSize) PVOID SlotContext,
        _In_ ULONG ContextSize) const;

    /** Highest DCI the HCD considers configured (Context Entries). */
    ULONG ContextEntries() const;

    ULONG SlotId() const;
    USB_DEVICE_SPEED Speed() const;
    UCXUSBDEVICE Handle() const;

    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    RingDoorbell(
        _In_ ULONG Dci,
        _In_ ULONG StreamId);

    /* Endpoint table by DCI; adding the default endpoint records it at DCI 1 */
    _IRQL_requires_max_(DISPATCH_LEVEL)
    XhciEndpoint*
    EndpointAt(
        _In_ ULONG Dci) const;
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    AddEndpoint(
        _In_ XhciEndpoint* Endpoint);
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    RemoveEndpoint(
        _In_ XhciEndpoint* Endpoint);

    /**
     * Configure Endpoint for one endpoint (drop and add) on its own command block and input context,
     * serialized per device, for static streams enable and disable. Done runs once with the result.
     */
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    ReconfigureEndpoint(
        _In_ XhciEndpoint* Endpoint,
        _In_ VOID (NTAPI *Done)(_In_ PVOID Context, _In_ NTSTATUS Status),
        _In_ PVOID Context);

    /* From the slot table */
    VOID
    OnTransferEvent(
        _In_ const XHCI_TRB* Event);
    VOID
    OnDeviceNotificationEvent(
        _In_ const XHCI_TRB* Event);
    VOID ControllerResetStarting();
    _IRQL_requires_(PASSIVE_LEVEL)
    VOID ControllerResetDone();
    VOID OnHostLost();

    /** The slot was lost without a Disable Slot: drop it and finish any pending request. */
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID SetDisabled();

    XhciController* Controller() const;
    BOOLEAN IsSlotEnabled() const;
    const USB_DEVICE_DESCRIPTOR* DeviceDescriptor() const;
    const USB_DEVICE_PORT_PATH* PortPath() const;
    ULONG ContextSize() const;

    /* Output endpoint context reads (xHCI 6.2.3) */
    ULONG
    EndpointState(
        _In_ ULONG Dci) const;
    ULONG64
    HardwareDequeuePointer(
        _In_ ULONG Dci) const;

    /* Bodies of the UCX device callbacks */
    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS
    Initialize(
        _In_ XhciController* Controller,
        _In_ UCXUSBDEVICE UsbDevice,
        _In_ const UCXUSBDEVICE_INFO* Info);
    VOID
    Cleanup(
        _In_ WDFOBJECT Object);
    VOID
    Enable(
        _In_ WDFREQUEST Request,
        _Inout_ PUSBDEVICE_ENABLE Payload);
    VOID
    Address(
        _In_ WDFREQUEST Request,
        _Inout_ PUSBDEVICE_ADDRESS Payload);
    VOID
    Update(
        _In_ WDFREQUEST Request,
        _Inout_ PUSBDEVICE_UPDATE Payload);
    VOID
    HubInfo(
        _In_ WDFREQUEST Request,
        _In_ const USBDEVICE_HUB_INFO* Payload);
    VOID
    EndpointsConfigure(
        _In_ WDFREQUEST Request,
        _Inout_ PENDPOINTS_CONFIGURE Payload);
    VOID
    Disable(
        _In_ WDFREQUEST Request);
    VOID
    Reset(
        _In_ WDFREQUEST Request);

private:
    typedef VOID (NTAPI *PFN_RECONFIGURE_DONE)(_In_ PVOID Context, _In_ NTSTATUS Status);

    /** Which device request owns the device command block. */
    enum class PendingKind : ULONG
    {
        None,
        Enable,
        Address,
        Update,
        Configure,
        Disable,
        Reset
    };

    /** What a successful Disable Slot does next. */
    enum class AfterDisable : ULONG
    {
        CompleteSuccess,
        CompleteFailure,
        EnableSlot
    };

    struct ReconfigureWaiter
    {
        XhciEndpoint* Endpoint;
        PFN_RECONFIGURE_DONE Done;
        PVOID Context;
    };

    static
    VOID
    NTAPI
    CommandDone(
        _In_ XhciCommand* Command);
    static
    VOID
    NTAPI
    ReconfigureCommandDone(
        _In_ XhciCommand* Command);

    VOID
    SetPending(
        _In_ WDFREQUEST Request,
        _In_ PVOID Payload,
        _In_ PendingKind Kind,
        _In_ BOOLEAN MustSucceed);
    WDFREQUEST TakePending();
    VOID
    CompletePending(
        _In_ NTSTATUS Status);
    NTSTATUS LostSlotStatus() const;

    VOID
    PrepareCommand(
        _In_ ULONG Type);
    VOID SubmitEnableSlot();
    NTSTATUS
    SubmitAddressDevice(
        _In_ BOOLEAN BlockSetAddress);
    VOID
    SubmitDisableSlot(
        _In_ AfterDisable Next);
    VOID SubmitSecondConfigure();

    VOID
    OnEnableSlotDone(
        _In_ const XhciCommand* Command);
    VOID
    OnAddressDeviceDone(
        _In_ const XhciCommand* Command);
    VOID
    OnEvaluateContextDone(
        _In_ const XhciCommand* Command);
    VOID
    OnConfigureEndpointDone(
        _In_ const XhciCommand* Command);
    VOID
    OnDisableSlotDone(
        _In_ const XhciCommand* Command);
    VOID
    OnResetDeviceDone(
        _In_ const XhciCommand* Command);

    NTSTATUS BuildAddressContext();
    VOID
    ApplyUpdate(
        _Inout_ PUSBDEVICE_UPDATE Payload);

    /* Tunnel state from ACPI for controllers whose errata ask for it */
    static EVT_WDF_WORKITEM TunnelDsmWorker;
    BOOLEAN
    WantsTunnelStateDsm(
        _In_ const USBDEVICE_UPDATE* Payload) const;
    BOOLEAN
    QueueTunnelStateDsm(
        _Inout_ PUSBDEVICE_UPDATE Payload);
    _IRQL_requires_(PASSIVE_LEVEL)
    VOID
    RunTunnelStateDsm(
        _Inout_ PUSBDEVICE_UPDATE Payload);
    VOID DisableDefaultEndpoint();
    VOID
    DisableConfiguredEndpoints(
        _In_ BOOLEAN ClearTable);
    VOID
    DisableEndpointList(
        _In_ ULONG Count,
        _In_reads_(Count) UCXENDPOINT* Endpoints);
    VOID
    RecordEnabledEndpoints(
        _In_ const ENDPOINTS_CONFIGURE* Payload);
    VOID ForgetSlot();

    VOID
    StartReconfigure(
        _In_ const ReconfigureWaiter* Waiter);
    VOID
    FinishReconfigure(
        _In_ NTSTATUS Status);

    PXHCI_INPUT_CONTROL_CONTEXT InputControl() const;
    PXHCI_SLOT_CONTEXT InputSlot() const;
    const XHCI_SLOT_CONTEXT* OutputSlot() const;
    XhciUsbDevice* TtHub() const;

    UCXUSBDEVICE m_Handle;
    XhciController* m_Controller;
    UCXUSBDEVICE_INFO m_Info;
    ULONG m_ContextSize;
    XhciDmaBuffer* m_Output;
    XhciDmaBuffer* m_Input;

    USB_DEVICE_DESCRIPTOR m_DeviceDescriptor;
    BOOLEAN m_SlotEnabled;
    ULONG m_SlotId;
    BOOLEAN m_IsHub;
    ULONG m_NumberOfPorts;
    ULONG m_NumberOfTTs;
    ULONG m_TTThinkTime;
    BOOLEAN m_LpmEnabled;
    BOOLEAN m_ResumeTimeSet;

    /** Bit n set when DCI n is configured as far as this driver knows; bit 0 is the slot. */
    ULONG m_ContextMask;

    XhciEndpoint* m_Endpoints[XHCI_MAX_DCI + 1];
    volatile LONG m_EndpointCount;

    /* The one device level command and the request it serves */
    XhciCommand m_Command;
    WDFREQUEST m_PendingRequest;
    PVOID m_PendingPayload;
    PendingKind m_PendingKind;
    BOOLEAN m_MustSucceed;
    AfterDisable m_AfterDisable;

    /* Single endpoint reconfigure, kept apart from the device command block */
    KSPIN_LOCK m_ReconfigureLock;
    BOOLEAN m_ReconfigureBusy;
    ULONG m_ReconfigureQueued;
    ReconfigureWaiter m_ReconfigureWaiters[XHCI_MAX_DCI + 1];
    ReconfigureWaiter m_ReconfigureCurrent;
    XhciCommand m_ReconfigureCommand;
    XhciDmaBuffer* m_ReconfigureInput;
};
