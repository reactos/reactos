/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Root hub ports and the UCX root hub callbacks
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

class XhciController;

/** Root hub. */
class XhciRootHub
{
public:
    /** Creates the UCXROOTHUB with every callback. */
    NTSTATUS
    Create(
        _In_ XhciController* Controller);

    /** Walks the supported protocol capabilities; needs the registers prepared. */
    NTSTATUS Prepare();
    VOID Release();
    VOID D0Entry();
    VOID PostInterruptsEntry();
    VOID D0Exit();
    VOID PostReset();
    VOID OnHostLost();

    ULONG PortCount() const;
    USHORT HighestUsbVersion() const;

    /* From the interrupter DPC */
    VOID
    OnPortStatusChangeEvent(
        _In_ const XHCI_TRB* Event);


    /** Wake arm or disarm of every port for the controller. */
    VOID
    SetWakeEnables(
        _In_ BOOLEAN Arm);

    /** Tells UCX a root port changed, for the controller's wake from S0 path. */
    VOID SignalPortChange();

    /* Services for the usbdevice module */
    VOID
    Update20HardwareLpm(
        _In_ ULONG PortNumber,
        _In_ ULONG SlotId,
        _In_ const USBDEVICE_UPDATE_20_HARDWARE_LPM_PARAMETERS* Parameters);
    VOID
    DisableLpmForSlot(
        _In_ ULONG PortNumber,
        _In_ ULONG SlotId);
    VOID
    ArmPortResumeTimer(
        _In_ ULONG PortNumber,
        _In_ ULONG Milliseconds);
    VOID
    DisarmPortResumeTimer(
        _In_ ULONG PortNumber);

    /** TRUE for a USB 2.x port; everything else counts as 3.x for crash dump. */
    BOOLEAN
    IsUsb20Port(
        _In_ ULONG PortNumber) const;

private:
    /** One root hub port. Major revision 0 means no protocol capability covers it. */
    struct Port
    {
        KSPIN_LOCK Lock;
        KTIMER ResumeTimer;             /**< USB 2.x ports only */
        KDPC ResumeDpc;
        XhciRootHub* Hub;
        ULONG Number;
        ULONG SpeedTable;               /**< Extended capability offset of the first PSI dword */
        LONG ResumeOverrideCount;
        ULONG ResumeOverrideMs;
        UCHAR Major;
        UCHAR Minor;
        UCHAR HubDepth;
        UCHAR SpeedCount;
        UCHAR LpmCapabilities;
        UCHAR U1Timeout;
        UCHAR U2Timeout;
        BOOLEAN HighSpeedOnly;
        BOOLEAN IntegratedHub;
        BOOLEAN Removable;
        BOOLEAN ResumeHandled;
        BOOLEAN LpmDirty;
        BOOLEAN LpmEnabled;
        BOOLEAN TimerReady;
    };

    /** How a hub class request completes. */
    enum class Outcome : ULONG
    {
        Ok,
        Stall,
        Gone,
        Halted,
        OperationPending
    };

    static EVT_UCX_ROOTHUB_CONTROL_URB EvtClearHubFeature;
    static EVT_UCX_ROOTHUB_CONTROL_URB EvtClearPortFeature;
    static EVT_UCX_ROOTHUB_CONTROL_URB EvtGetHubStatus;
    static EVT_UCX_ROOTHUB_CONTROL_URB EvtGetPortStatus;
    static EVT_UCX_ROOTHUB_CONTROL_URB EvtSetHubFeature;
    static EVT_UCX_ROOTHUB_CONTROL_URB EvtSetPortFeature;
    static EVT_UCX_ROOTHUB_CONTROL_URB EvtGetPortErrorCount;
    static EVT_UCX_ROOTHUB_INTERRUPT_TX EvtInterruptTransfer;
    static EVT_UCX_ROOTHUB_GET_INFO EvtGetInfo;
    static EVT_UCX_ROOTHUB_GET_20PORT_INFO EvtGet20PortInfo;
    static EVT_UCX_ROOTHUB_GET_30PORT_INFO EvtGet30PortInfo;
    static KDEFERRED_ROUTINE ResumeTimerDpc;

    static XhciRootHub*
    FromUcx(
        _In_ UCXROOTHUB UcxRootHub);
    static VOID
    CompleteHubRequest(
        _In_ WDFREQUEST Request,
        _In_ PURB Urb,
        _In_ Outcome Result);

    NTSTATUS
    ParseProtocol(
        _In_ ULONG Capability,
        _Out_ PBOOLEAN Usable);

    Port*
    PortAt(
        _In_ ULONG PortNumber) const;
    ULONG
    ReadPortSc(
        _In_ const Port* P) const;
    ULONG
    UpdatePortSc(
        _In_ Port* P,
        _In_ ULONG Keep,
        _In_ ULONG Set);
    ULONG
    UpdatePortScLocked(
        _In_ Port* P,
        _In_ ULONG Keep,
        _In_ ULONG Set);
    VOID
    UpdatePortReg(
        _In_ Port* P,
        _In_ ULONG Offset,
        _In_ ULONG Clear,
        _In_ ULONG Set);
    VOID
    ClearUsb20LpmLocked(
        _In_ Port* P);
    BOOLEAN
    IsHiddenDebugPort(
        _In_ const Port* P) const;
    VOID
    StartResumeTimer(
        _In_ Port* P);
    BOOLEAN
    AcknowledgeResume(
        _In_ Port* P,
        _In_ BOOLEAN PowerUp);
    NTSTATUS
    WaitForU3(
        _In_ Port* P,
        _In_ BOOLEAN RepeatRequest);
    NTSTATUS
    WaitU0(
        _In_ Port* P);
    VOID
    SuspendPort(
        _In_ Port* P);
    VOID
    ResuspendResumedPorts();
    BOOLEAN
    IsPortConnected(
        _In_ const Port* P) const;

    Outcome
    CheckRunning();
    Outcome
    ClearHubFeature(
        _In_ PURB Urb);
    Outcome
    GetHubStatus(
        _In_ PURB Urb);
    Outcome
    GetPortStatus(
        _In_ PURB Urb);
    Outcome
    ClearPortFeature(
        _In_ PURB Urb);
    Outcome
    ClearUsb20PortFeature(
        _In_ Port* P,
        _In_ USHORT Feature,
        _In_ UCHAR Value);
    Outcome
    ClearUsb30PortFeature(
        _In_ Port* P,
        _In_ USHORT Feature,
        _In_ UCHAR Value);
    Outcome
    SetPortFeature(
        _In_ PURB Urb);
    Outcome
    SetUsb20PortFeature(
        _In_ Port* P,
        _In_ USHORT Feature,
        _In_ UCHAR Value);
    Outcome
    SetUsb30PortFeature(
        _In_ Port* P,
        _In_ USHORT Feature,
        _In_ UCHAR Value);
    Outcome
    SetUsb30LinkState(
        _In_ Port* P,
        _In_ UCHAR LinkState);
    Outcome
    GetPortErrorCount(
        _In_ PURB Urb);
    Outcome
    InterruptTransfer(
        _In_ PURB Urb);
    NTSTATUS
    FillSpeeds(
        _In_ const Port* P,
        _Inout_ PROOTHUB_30PORT_INFO_EX Entry) const;

    XhciController* m_Controller;
    UCXROOTHUB m_Ucx;
    Port* m_Ports;
    ULONG m_PortCount;
    ULONG m_DebugCapability;            /**< Extended capability offset, 0 when absent */
    USHORT m_Usb20PortCount;
    USHORT m_Usb30PortCount;
    USHORT m_U1ExitLatency;
    USHORT m_U2ExitLatency;
    BOOLEAN m_AwaitingStatusRead;       /**< No interrupt transfer seen since D0 entry */
};
