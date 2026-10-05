/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Root hub ports and the UCX root hub callbacks
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "xhcidrv.h"
#include <drivers/usb3/hubucx.h>

#define NDEBUG
#include <debug.h>

/* Hub class requests (USB 2.0 11.24.2, USB 3.x 10.16.2) */
static const UCHAR HubRequestToHub = 0x20;
static const UCHAR HubRequestFromHub = 0xA0;
static const UCHAR HubRequestToPort = 0x23;
static const UCHAR HubRequestFromPort = 0xA3;

enum : USHORT
{
    FeatureHubLocalPowerChange = 0,
    FeatureHubOverCurrentChange = 1,
    FeaturePortEnable = 1,
    FeaturePortSuspend = 2,
    FeaturePortReset = 4,
    FeaturePortLinkState = 5,
    FeaturePortPower = 8,
    FeatureConnectChange = 16,
    FeatureEnableChange = 17,
    FeatureSuspendChange = 18,
    FeatureOverCurrentChange = 19,
    FeatureResetChange = 20,
    FeaturePortTest = 21,
    FeaturePortIndicator = 22,
    FeatureU1Timeout = 23,
    FeatureU2Timeout = 24,
    FeatureLinkStateChange = 25,
    FeatureConfigErrorChange = 26,
    FeatureRemoteWakeMask = 27,
    FeatureWarmReset = 28,
    FeatureWarmResetChange = 29,
    FeatureForceLinkPmAccept = 30
};

/* Default TDRSMDN; USB 2.0 7.1.7.7 asks for at least 20 ms */
static const ULONG XhciResumeSignalMs = 50;

/* Busy wait budgets in 10 us steps */
static const ULONG XhciU3WaitSteps = 2400;
static const ULONG XhciU0WaitSteps = 1200;
static const ULONG XhciResumeWaitSteps = 1200;

static const ULONG XhciPortGone = 0xFFFFFFFF;

typedef struct _XHCI_ROOTHUB_CONTEXT
{
    XhciRootHub* RootHub;
} XHCI_ROOTHUB_CONTEXT, *PXHCI_ROOTHUB_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XHCI_ROOTHUB_CONTEXT, XhciGetRootHubContext);

FORCEINLINE
ULONG
NTAPI
XhciLinkState(
    _In_ ULONG PortSc)
{
    return (PortSc & XHCI_PORTSC_PLS_MASK) >> XHCI_PORTSC_PLS_SHIFT;
}

FORCEINLINE
ULONG
NTAPI
XhciPortSpeed(
    _In_ ULONG PortSc)
{
    return (PortSc & XHCI_PORTSC_SPEED_MASK) >> XHCI_PORTSC_SPEED_SHIFT;
}

/** PORTSC bits that move the link to State. */
FORCEINLINE
ULONG
NTAPI
XhciLinkWrite(
    _In_ ULONG State)
{
    return (State << XHCI_PORTSC_PLS_SHIFT) | XHCI_PORTSC_LWS;
}

/** Powered, connected, enabled, no pending connect change and not SS.Disabled. */
FORCEINLINE
BOOLEAN
NTAPI
XhciPortActive(
    _In_ ULONG PortSc)
{
    const ULONG Needed = XHCI_PORTSC_PP | XHCI_PORTSC_CCS | XHCI_PORTSC_PED;

    if ((PortSc & Needed) != Needed || (PortSc & XHCI_PORTSC_CSC))
        return FALSE;

    return XhciLinkState(PortSc) != PORT_LINK_STATE_DISABLED;
}

static
PVOID
NTAPI
XhciRequestArgument(
    _In_ WDFREQUEST Request)
{
    WDF_REQUEST_PARAMETERS Parameters;

    WDF_REQUEST_PARAMETERS_INIT(&Parameters);
    WdfRequestGetParameters(Request, &Parameters);
    return Parameters.Parameters.Others.Arg1;
}

FORCEINLINE
PUSB_DEFAULT_PIPE_SETUP_PACKET
NTAPI
XhciSetupOf(
    _In_ PURB Urb)
{
    /* Same offset in the plain and the _EX control transfer URBs */
    return (PUSB_DEFAULT_PIPE_SETUP_PACKET)Urb->UrbControlTransfer.SetupPacket;
}

/** Data buffer of a transfer with room for Needed bytes, or NULL. */
static
PVOID
NTAPI
XhciTransferBuffer(
    _In_opt_ PVOID Buffer,
    _In_opt_ PMDL Mdl,
    _In_ ULONG Length,
    _In_ ULONG Needed)
{
    if (Length < Needed)
        return NULL;

    if (Buffer == NULL && Mdl != NULL)
        Buffer = MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority);

    return Buffer;
}

static
PVOID
NTAPI
XhciHubRequestBuffer(
    _In_ PURB Urb,
    _In_ ULONG Needed)
{
    struct _URB_CONTROL_TRANSFER* Transfer = &Urb->UrbControlTransfer;

    return XhciTransferBuffer(Transfer->TransferBuffer,
                              Transfer->TransferBufferMDL,
                              Transfer->TransferBufferLength,
                              Needed);
}

static
VOID
NTAPI
XhciTraceBadRequest(
    _In_ PCSTR Callback,
    _In_ PURB Urb)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET Setup = XhciSetupOf(Urb);

    DPRINT1("Root hub %s: rejecting setup %02x %02x %04x %04x %04x\n",
            Callback,
            Setup->bmRequestType.B,
            Setup->bRequest,
            Setup->wValue.W,
            Setup->wIndex.W,
            Setup->wLength);
}

/* Lifetime *******************************************************************/

XhciRootHub*
XhciRootHub::FromUcx(
    _In_ UCXROOTHUB UcxRootHub)
{
    return XhciGetRootHubContext(UcxRootHub)->RootHub;
}

NTSTATUS
XhciRootHub::Create(
    _In_ XhciController* Controller)
{
    UCX_ROOTHUB_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;
    UCXROOTHUB Handle;
    NTSTATUS Status;

    m_Controller = Controller;
    m_Ucx = NULL;
    m_Ports = NULL;
    m_PortCount = 0;
    m_DebugCapability = 0;
    m_Usb20PortCount = 0;
    m_Usb30PortCount = 0;
    m_AwaitingStatusRead = FALSE;

    UCX_ROOTHUB_CONFIG_INIT(&Config,
                            EvtClearHubFeature,
                            EvtClearPortFeature,
                            EvtGetHubStatus,
                            EvtGetPortStatus,
                            EvtSetHubFeature,
                            EvtSetPortFeature,
                            EvtGetPortErrorCount,
                            EvtInterruptTransfer,
                            EvtGetInfo,
                            EvtGet20PortInfo,
                            EvtGet30PortInfo);

    /* UCX gives this context to every request it sends down for the root hub and its devices */
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Config.WdfRequestAttributes, XhciRequestData);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XHCI_ROOTHUB_CONTEXT);

    Status = UcxRootHubCreate(Controller->m_Ucx, &Config, &Attributes, &Handle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UcxRootHubCreate failed 0x%lx\n", Status);
        return Status;
    }

    XhciGetRootHubContext(Handle)->RootHub = this;
    m_Ucx = Handle;
    return STATUS_SUCCESS;
}

/** Applies one Supported Protocol Capability; Usable is set for a USB 2.x or 3.x one. */
NTSTATUS
XhciRootHub::ParseProtocol(
    _In_ ULONG Capability,
    _Out_ PBOOLEAN Usable)
{
    XhciRegisters* Registers = &m_Controller->m_Registers;
    ULONG Revision;
    ULONG Name;
    ULONG Ports;
    ULONG First;
    ULONG Count;
    ULONG Number;
    UCHAR Major;

    *Usable = FALSE;

    Revision = Registers->ReadExtended32(Capability + XHCI_PROTOCOL_REVISION);
    Name = Registers->ReadExtended32(Capability + XHCI_PROTOCOL_NAME);
    Ports = Registers->ReadExtended32(Capability + XHCI_PROTOCOL_PORTS);

    if (Name != XHCI_PROTOCOL_NAME_USB)
    {
        DPRINT1("Protocol capability at 0x%lx has name 0x%08lx, skipped\n", Capability, Name);
        return STATUS_SUCCESS;
    }

    Major = (UCHAR)(Revision >> XHCI_PROTOCOL_MAJOR_SHIFT);
    if (Major != 2 && Major != 3)
    {
        DPRINT1("USB protocol capability at 0x%lx with major revision %u, skipped\n", Capability, Major);
        return STATUS_SUCCESS;
    }

    First = Ports & XHCI_PROTOCOL_OFFSET_MASK;
    Count = (Ports & XHCI_PROTOCOL_COUNT_MASK) >> XHCI_PROTOCOL_COUNT_SHIFT;
    if (First == 0 || Count == 0 || First + Count - 1 > m_PortCount)
    {
        DPRINT1("USB %u protocol covers ports %lu+%lu, controller has %lu\n", Major, First, Count, m_PortCount);
        return STATUS_INVALID_PARAMETER;
    }

    *Usable = TRUE;

    if (Major == 3)
        m_FirstUsb30Port = First;

    for (Number = First; Number < First + Count; Number++)
    {
        Port* P = &m_Ports[Number - 1];

        if (P->Major != 0)
        {
            DPRINT1("Port %lu claimed by USB %u and USB %u protocol capabilities\n", Number, P->Major, Major);
            return STATUS_INVALID_PARAMETER;
        }

        if (Major == 2)
            m_Usb20PortCount++;
        else
            m_Usb30PortCount++;

        P->Major = Major;
        P->Minor = (UCHAR)(Revision >> XHCI_PROTOCOL_MINOR_SHIFT);
        P->HighSpeedOnly = (Ports & XHCI_PROTOCOL_HSO) != 0;
        P->IntegratedHub = (Ports & XHCI_PROTOCOL_IHI) != 0;
        P->LpmCapabilities = 0;
        if (Ports & XHCI_PROTOCOL_HLC)
            P->LpmCapabilities |= 0x01;
        if (Ports & XHCI_PROTOCOL_BLC)
            P->LpmCapabilities |= 0x02;
        P->HubDepth = (UCHAR)((Ports & XHCI_PROTOCOL_MHD_MASK) >> XHCI_PROTOCOL_MHD_SHIFT);
        P->SpeedCount = (UCHAR)(Ports >> XHCI_PROTOCOL_PSIC_SHIFT);
        P->SpeedTable = Capability + XHCI_PROTOCOL_PSI;
        P->Removable = (Registers->ReadPort32(Number, XHCI_PORT_SC) & XHCI_PORTSC_DR) == 0;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
XhciRootHub::Prepare()
{
    XhciRegisters* Registers = &m_Controller->m_Registers;
    BOOLEAN AnyUsable = FALSE;
    BOOLEAN Usable;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Capability;
    ULONG Number;
    ULONG Params;

    m_PortCount = Registers->MaxPorts();
    m_Usb20PortCount = 0;
    m_Usb30PortCount = 0;

    Params = Registers->Hcsparams3();
    m_U1ExitLatency = (USHORT)(Params & XHCI_HCS3_U1_LATENCY_MASK);
    m_U2ExitLatency = (USHORT)(Params >> XHCI_HCS3_U2_LATENCY_SHIFT);

    Capability = Registers->FindExtendedCapability(XHCI_EXTCAP_DEBUG, 0);
    if (Capability != 0)
        m_DebugCapability = Capability;

    Capability = Registers->FindExtendedCapability(XHCI_EXTCAP_USB4_TUNNELING, 0);
    m_PortScTunnelValid = Capability != 0 &&
                          (Registers->ReadExtended32(Capability) & XHCI_USB4_CAP_PORTSC_TUNNEL_VALID) != 0;
    m_FirstUsb30Port = 0;

    if (m_PortCount == 0)
    {
        DPRINT1("Controller reports no root hub ports\n");
        return STATUS_INVALID_PARAMETER;
    }

    m_Ports = (Port*)ExAllocatePoolWithTag(NonPagedPool, m_PortCount * sizeof(*m_Ports), XHCI_TAG_ROOTHUB);
    if (m_Ports == NULL)
    {
        DPRINT1("No memory for %lu root hub ports\n", m_PortCount);
        m_PortCount = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(m_Ports, m_PortCount * sizeof(*m_Ports));

    for (Capability = Registers->FindExtendedCapability(XHCI_EXTCAP_SUPPORTED_PROTOCOL, 0);
         Capability != 0;
         Capability = Registers->FindExtendedCapability(XHCI_EXTCAP_SUPPORTED_PROTOCOL, Capability))
    {
        Status = ParseProtocol(Capability, &Usable);
        if (!NT_SUCCESS(Status))
            break;

        AnyUsable |= Usable;
    }

    if (NT_SUCCESS(Status) && !AnyUsable)
    {
        DPRINT1("No USB 2.x or 3.x protocol capability found\n");
        Status = STATUS_INVALID_PARAMETER;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub prepare failed 0x%lx\n", Status);
        ExFreePoolWithTag(m_Ports, XHCI_TAG_ROOTHUB);
        m_Ports = NULL;
        m_PortCount = 0;
        m_Usb20PortCount = 0;
        m_Usb30PortCount = 0;
        return Status;
    }

    for (Number = 1; Number <= m_PortCount; Number++)
    {
        Port* P = &m_Ports[Number - 1];

        KeInitializeSpinLock(&P->Lock);
        P->Hub = this;
        P->Number = Number;

        if (P->Major == 2)
        {
            KeInitializeTimer(&P->ResumeTimer);
            KeInitializeDpc(&P->ResumeDpc, ResumeTimerDpc, P);
            P->TimerReady = TRUE;
        }
    }

    DPRINT("Root hub: %lu ports, %u USB 2.x, %u USB 3.x\n", m_PortCount, m_Usb20PortCount, m_Usb30PortCount);
    return STATUS_SUCCESS;
}

VOID
XhciRootHub::Release()
{
    ULONG Index;

    if (m_Ports == NULL)
        return;

    for (Index = 0; Index < m_PortCount; Index++)
    {
        if (m_Ports[Index].TimerReady)
            KeCancelTimer(&m_Ports[Index].ResumeTimer);
    }

    /* Wait for a running resume timer callback before freeing the ports */
    KeFlushQueuedDpcs();

    ExFreePoolWithTag(m_Ports, XHCI_TAG_ROOTHUB);
    m_Ports = NULL;
    m_PortCount = 0;
    m_Usb20PortCount = 0;
    m_Usb30PortCount = 0;
}

/* Port register helpers ******************************************************/

XhciRootHub::Port*
XhciRootHub::PortAt(
    _In_ ULONG PortNumber) const
{
    if (m_Ports == NULL || PortNumber == 0 || PortNumber > m_PortCount)
        return NULL;

    return &m_Ports[PortNumber - 1];
}

ULONG
XhciRootHub::ReadPortSc(
    _In_ const Port* P) const
{
    return m_Controller->m_Registers.ReadPort32(P->Number, XHCI_PORT_SC);
}

/** PORTSC read and write back keeping only the Keep bits, plus Set. Caller holds the port lock. */
ULONG
XhciRootHub::UpdatePortScLocked(
    _In_ Port* P,
    _In_ ULONG Keep,
    _In_ ULONG Set)
{
    ULONG PortSc = ReadPortSc(P);

    m_Controller->m_Registers.WritePort32(P->Number, XHCI_PORT_SC, (PortSc & Keep) | Set);
    return PortSc;
}

ULONG
XhciRootHub::UpdatePortSc(
    _In_ Port* P,
    _In_ ULONG Keep,
    _In_ ULONG Set)
{
    XhciSpinLockGuard Guard(&P->Lock);

    return UpdatePortScLocked(P, Keep, Set);
}

/** Plain field replace for PORTPMSC and PORTHLPMC. */
VOID
XhciRootHub::UpdatePortReg(
    _In_ Port* P,
    _In_ ULONG Offset,
    _In_ ULONG Clear,
    _In_ ULONG Set)
{
    XhciRegisters* Registers = &m_Controller->m_Registers;
    ULONG Value;

    Value = Registers->ReadPort32(P->Number, Offset);
    Registers->WritePort32(P->Number, Offset, (Value & ~Clear) | Set);
}

VOID
XhciRootHub::ClearUsb20LpmLocked(
    _In_ Port* P)
{
    UpdatePortReg(P,
                  XHCI_PORT_PMSC,
                  XHCI_PORTPMSC_BESL_MASK | XHCI_PORTPMSC_L1_SLOT_MASK |
                  XHCI_PORTPMSC_RWE | XHCI_PORTPMSC_HLE,
                  0);
    UpdatePortReg(P,
                  XHCI_PORT_HLPMC,
                  XHCI_PORTHLPMC_BESLD_MASK | XHCI_PORTHLPMC_HIRDM_MASK | XHCI_PORTHLPMC_L1_TIMEOUT_MASK,
                  0);
    P->LpmDirty = FALSE;
}

/** The kernel debugger owns this 3.x port through the Debug Capability. */
BOOLEAN
XhciRootHub::IsHiddenDebugPort(
    _In_ const Port* P) const
{
    ULONG Status;

    if (P->Major != 3 || !KD_DEBUGGER_ENABLED || m_DebugCapability == 0)
        return FALSE;

    if (!m_Controller->HasErrata(XhciErrata::PortMaskDebugPortNoise))
        return FALSE;

    Status = m_Controller->m_Registers.ReadExtended32(m_DebugCapability + XHCI_DBC_DCST);
    return (Status >> XHCI_DBC_DCST_PORT_SHIFT) == P->Number;
}

BOOLEAN
XhciRootHub::IsPortConnected(
    _In_ const Port* P) const
{
    ULONG PortSc = ReadPortSc(P);
    ULONG Link = XhciLinkState(PortSc);

    if (!(PortSc & XHCI_PORTSC_PP) || !(PortSc & XHCI_PORTSC_CCS))
        return FALSE;

    return Link != PORT_LINK_STATE_DISABLED &&
           Link != PORT_LINK_STATE_INACTIVE &&
           Link != PORT_LINK_STATE_COMPLIANCE_MODE;
}

/* Resume handling ************************************************************/

VOID
XhciRootHub::StartResumeTimer(
    _In_ Port* P)
{
    ULONG Milliseconds = XhciResumeSignalMs;

    if (P->ResumeOverrideCount > 0)
        Milliseconds = P->ResumeOverrideMs;

    KeSetTimer(&P->ResumeTimer, XhciRelativeMs(Milliseconds), &P->ResumeDpc);
}

/** Ends USB 2.x resume signaling once TDRSMDN has passed. */
VOID
NTAPI
XhciRootHub::ResumeTimerDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID Context,
    _In_opt_ PVOID Argument1,
    _In_opt_ PVOID Argument2)
{
    Port* P = (Port*)Context;
    XhciRootHub* Hub = P->Hub;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Argument1);
    UNREFERENCED_PARAMETER(Argument2);

    if (!Hub->m_Controller->IsAccessible())
        return;

    /* U0 ends resume signaling and the PLC write acknowledges the start of resume */
    Hub->UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_PLC | XhciLinkWrite(PORT_LINK_STATE_U0));

    if (Hub->m_Controller->HasErrata(XhciErrata::PortHighSpeedLpmOffForSuspend) && P->LpmEnabled)
    {
        XhciSpinLockGuard Guard(&P->Lock);

        Hub->UpdatePortReg(P, XHCI_PORT_PMSC, 0, XHCI_PORTPMSC_HLE);
    }
}

/**
 * Acknowledges a device or power up resume (xHCI 4.15.2.1). TRUE means the
 * change stays hidden from the hub until the link is back in U0.
 */
BOOLEAN
XhciRootHub::AcknowledgeResume(
    _In_ Port* P,
    _In_ BOOLEAN PowerUp)
{
    KIRQL OldIrql;
    ULONG PortSc;

    if (P->Major == 0)
        return FALSE;

    KeAcquireSpinLock(&P->Lock, &OldIrql);

    PortSc = ReadPortSc(P);
    if (PortSc == XhciPortGone)
    {
        KeReleaseSpinLock(&P->Lock, OldIrql);
        return FALSE;
    }

    if (P->Major == 3 &&
        (PortSc & XHCI_PORTSC_PLC) &&
        XhciLinkState(PortSc) == PORT_LINK_STATE_U0 &&
        m_Controller->HasErrata(XhciErrata::PortWakeToU0BeforeSuspend))
    {
        /* Put back the U1/U2 timeouts WaitU0 zeroed */
        UpdatePortReg(P,
                      XHCI_PORT_PMSC,
                      XHCI_PORTPMSC_U1_TIMEOUT_MASK | XHCI_PORTPMSC_U2_TIMEOUT_MASK,
                      P->U1Timeout | ((ULONG)P->U2Timeout << XHCI_PORTPMSC_U2_TIMEOUT_SHIFT));
    }

    if (XhciLinkState(PortSc) != XHCI_PLS_RESUME ||
        (!PowerUp && !(PortSc & XHCI_PORTSC_PLC)))
    {
        KeReleaseSpinLock(&P->Lock, OldIrql);
        return FALSE;
    }

    if (P->ResumeHandled)
    {
        KeReleaseSpinLock(&P->Lock, OldIrql);
        return TRUE;
    }

    P->ResumeHandled = TRUE;

    if (P->Major == 2)
    {
        KeReleaseSpinLock(&P->Lock, OldIrql);
        DPRINT1("Port %lu: USB 2.x device initiated resume\n", P->Number);

        /* PLC stays set until the timer, so a power down in between still wakes */
        StartResumeTimer(P);
        return TRUE;
    }

    /* PLC rides along so it is cleared only if it was set */
    UpdatePortScLocked(P, XHCI_PORTSC_PRESERVE | XHCI_PORTSC_PLC, XhciLinkWrite(PORT_LINK_STATE_U0));
    KeReleaseSpinLock(&P->Lock, OldIrql);

    DPRINT1("Port %lu: USB 3.x device initiated resume\n", P->Number);
    return TRUE;
}

/* Link state helpers *********************************************************/

/** Waits up to 24 ms for a requested U3 to land. */
NTSTATUS
XhciRootHub::WaitForU3(
    _In_ Port* P,
    _In_ BOOLEAN RepeatRequest)
{
    ULONG LastSeen = XhciPortGone;
    ULONG Step;

    for (Step = 0; Step < XhciU3WaitSteps; Step++)
    {
        ULONG PortSc;
        ULONG Link;

        /* Some controllers drop the first U3 request; ask again every 30 us */
        if (RepeatRequest && Step != 0 && (Step % 3) == 0)
            UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XhciLinkWrite(PORT_LINK_STATE_U3));

        PortSc = ReadPortSc(P);
        if (PortSc != LastSeen)
        {
            LastSeen = PortSc;

            if (P->Major == 0)
            {
                DPRINT1("Port %lu: U3 wait on a port with no protocol\n", P->Number);
                return STATUS_SUCCESS;
            }

            Link = XhciLinkState(PortSc);
            if (Link == PORT_LINK_STATE_U3 ||
                !XhciPortActive(PortSc) ||
                Link == XHCI_PLS_RESUME ||
                Link == PORT_LINK_STATE_INACTIVE ||
                Link == PORT_LINK_STATE_COMPLIANCE_MODE ||
                Link == PORT_LINK_STATE_HOT_RESET ||
                Link == PORT_LINK_STATE_TEST_MODE ||
                (PortSc & XHCI_PORTSC_PR) ||
                P->ResumeHandled)
            {
                return STATUS_SUCCESS;
            }
        }

        KeStallExecutionProcessor(10);
    }

    DPRINT1("Port %lu: U3 transition still pending after 24 ms, PORTSC 0x%08lx\n", P->Number, LastSeen);
    return STATUS_UNSUCCESSFUL;
}

/** Brings the link to U0 first for controllers that cannot go to U3 from a low power state. */
NTSTATUS
XhciRootHub::WaitU0(
    _In_ Port* P)
{
    ULONG Remaining = XhciU0WaitSteps;

    if (P->Major == 2)
    {
        if (!P->LpmEnabled)
            return STATUS_SUCCESS;

        XhciSpinLockGuard Guard(&P->Lock);

        UpdatePortReg(P, XHCI_PORT_PMSC, XHCI_PORTPMSC_HLE, 0);
    }
    else if (P->Major == 3)
    {
        XhciSpinLockGuard Guard(&P->Lock);

        UpdatePortReg(P, XHCI_PORT_PMSC, XHCI_PORTPMSC_U1_TIMEOUT_MASK | XHCI_PORTPMSC_U2_TIMEOUT_MASK, 0);
        UpdatePortScLocked(P, XHCI_PORTSC_PRESERVE, XhciLinkWrite(PORT_LINK_STATE_U0));
    }
    else
    {
        return STATUS_SUCCESS;
    }

    for (;;)
    {
        ULONG PortSc = ReadPortSc(P);
        ULONG Link = XhciLinkState(PortSc);

        if (PortSc == XhciPortGone)
            break;
        if (Link == PORT_LINK_STATE_U0)
            return STATUS_SUCCESS;
        if (Link == PORT_LINK_STATE_RX_DETECT ||
            Link == PORT_LINK_STATE_INACTIVE ||
            Link == PORT_LINK_STATE_COMPLIANCE_MODE ||
            !XhciPortActive(PortSc))
        {
            break;
        }
        if (Link == PORT_LINK_STATE_HOT_RESET || Link == PORT_LINK_STATE_TEST_MODE)
        {
            DPRINT1("Port %lu: link state %lu while forcing U0\n", P->Number, Link);
            break;
        }
        if ((PortSc & XHCI_PORTSC_PR) || Remaining == 0)
            break;

        Remaining--;
        KeStallExecutionProcessor(10);
    }

    return STATUS_UNSUCCESSFUL;
}

/** Requests U3 (USB 2.x suspend); the transition is not waited for. */
VOID
XhciRootHub::SuspendPort(
    _In_ Port* P)
{
    XhciSpinLockGuard Guard(&P->Lock);
    ULONG PortSc;

    P->ResumeHandled = FALSE;

    PortSc = UpdatePortScLocked(P, XHCI_PORTSC_PRESERVE, XhciLinkWrite(PORT_LINK_STATE_U3));
    if (!(PortSc & XHCI_PORTSC_PED) || XhciLinkState(PortSc) >= PORT_LINK_STATE_U3)
        DPRINT1("Port %lu: suspend requested with PORTSC 0x%08lx\n", P->Number, PortSc);
}

/** Puts every active, not suspended port back in U3 before power down. */
VOID
XhciRootHub::ResuspendResumedPorts()
{
    ULONG Number;

    for (Number = 1; Number <= m_PortCount; Number++)
    {
        Port* P = &m_Ports[Number - 1];
        ULONG Remaining = XhciResumeWaitSteps;
        ULONG PortSc;
        ULONG Link;

        if (P->Major == 2 && KeCancelTimer(&P->ResumeTimer))
        {
            /* Left in Resume with PLC set, which wakes the controller once it is down */
            P->ResumeHandled = FALSE;
            continue;
        }

        if (P->Major == 0)
            continue;

        for (;;)
        {
            PortSc = ReadPortSc(P);
            Link = XhciLinkState(PortSc);

            if (PortSc == XhciPortGone ||
                !P->ResumeHandled ||
                !XhciPortActive(PortSc) ||
                (Link != XHCI_PLS_RESUME && Link != PORT_LINK_STATE_RECOVERY))
            {
                break;
            }

            if (Remaining == 0)
            {
                DPRINT1("Port %lu: acknowledged resume not done after 12 ms\n", Number);
                break;
            }

            Remaining--;
            KeStallExecutionProcessor(10);
        }

        if (PortSc == XhciPortGone || !XhciPortActive(PortSc))
            continue;

        if (Link != PORT_LINK_STATE_U0 &&
            Link != PORT_LINK_STATE_U1 &&
            Link != PORT_LINK_STATE_U2 &&
            Link != PORT_LINK_STATE_RECOVERY)
        {
            continue;
        }

        if ((P->Major == 2 && m_Controller->HasErrata(XhciErrata::PortHighSpeedLpmOffForSuspend)) ||
            (P->Major == 3 && m_Controller->HasErrata(XhciErrata::PortWakeToU0BeforeSuspend)))
        {
            if (!NT_SUCCESS(WaitU0(P)))
                continue;
        }

        SuspendPort(P);
    }
}

/* Power **********************************************************************/

VOID
XhciRootHub::D0Entry()
{
    ULONG Number;

    m_AwaitingStatusRead = TRUE;

    for (Number = 1; Number <= m_PortCount; Number++)
    {
        Port* P = &m_Ports[Number - 1];

        if (P->Major == 0)
        {
            DPRINT1("Port %lu has no USB protocol, skipped at D0 entry\n", Number);
            continue;
        }

        /* Some controllers miss connect and disconnect in D0 without the wake enables */
        if (m_Controller->HasErrata(XhciErrata::PortHoldWakeMaskAwake))
            UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_WAKE_MASK);
    }
}

VOID
XhciRootHub::PostInterruptsEntry()
{
    ULONG Number;

    /* A port already in Resume at power up may never raise an event of its own */
    for (Number = 1; Number <= m_PortCount; Number++)
        AcknowledgeResume(&m_Ports[Number - 1], TRUE);
}

VOID
XhciRootHub::D0Exit()
{
    XhciController::SystemAction Action = m_Controller->m_SystemAction;
    BOOLEAN FinalPowerOff = (m_Controller->m_PowerState == WdfPowerDeviceD3Final);
    BOOLEAN SystemPoweringDown = (Action >= XhciController::SystemAction::Sleep &&
                                  Action <= XhciController::SystemAction::Shutdown);
    BOOLEAN S0Idle = (Action == XhciController::SystemAction::None);
    BOOLEAN AnyConnected = FALSE;
    ULONG Number;

    ResuspendResumedPorts();

    for (Number = 1; Number <= m_PortCount; Number++)
    {
        Port* P = &m_Ports[Number - 1];

        if (!FinalPowerOff)
        {
            /* Let the hub recover the port instead of leaving a dead device behind */
            if (!NT_SUCCESS(WaitForU3(P, FALSE)) && S0Idle)
                UcxRootHubPortChanged(m_Ucx);

            if (IsPortConnected(P))
                AnyConnected = TRUE;
        }

        if (m_Controller->HasErrata(XhciErrata::PortAckCscOnPowerDown))
            UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_CSC);

        if (SystemPoweringDown && m_Controller->HasErrata(XhciErrata::PortHoldWakeMaskAwake))
            UpdatePortSc(P, XHCI_PORTSC_PP | XHCI_PORTSC_PIC_MASK, 0);
    }

    m_Controller->NotifyPortConnectState(AnyConnected);
}

VOID
XhciRootHub::PostReset()
{
    /* Resume, LPM and U1/U2 bookkeeping deliberately survive the reset */
    D0Entry();
}

VOID
XhciRootHub::OnHostLost()
{
    ULONG Index;

    if (m_Ports == NULL)
        return;

    for (Index = 0; Index < m_PortCount; Index++)
    {
        if (m_Ports[Index].TimerReady)
            KeCancelTimer(&m_Ports[Index].ResumeTimer);
    }
}

VOID
XhciRootHub::SetWakeEnables(
    _In_ BOOLEAN Arm)
{
    ULONG Number;

    if (!Arm && m_Controller->HasErrata(XhciErrata::PortHoldWakeMaskAwake))
        return;

    for (Number = 1; Number <= m_PortCount; Number++)
    {
        Port* P = &m_Ports[Number - 1];

        if (Arm)
            UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_WAKE_MASK);
        else
            UpdatePortSc(P, XHCI_PORTSC_PP | XHCI_PORTSC_PIC_MASK, 0);
    }
}

VOID
XhciRootHub::SignalPortChange()
{
    if (m_Ucx != NULL)
        UcxRootHubPortChanged(m_Ucx);
}

/* Queries ********************************************************************/

ULONG
XhciRootHub::PortCount() const
{
    return m_PortCount;
}

/** BCD major in the high byte, minor in the low byte. */
USHORT
XhciRootHub::HighestUsbVersion() const
{
    UCHAR Major = 0;
    UCHAR Minor = 0;
    ULONG Index;

    for (Index = 0; Index < m_PortCount; Index++)
    {
        const Port* P = &m_Ports[Index];

        /* A later port with the same major and a higher minor does not count */
        if (P->Major > Major)
        {
            Major = P->Major;
            if (P->Minor > Minor)
                Minor = P->Minor;
        }
    }

    return (USHORT)((Major << 8) | Minor);
}

BOOLEAN
XhciRootHub::IsUsb20Port(
    _In_ ULONG PortNumber) const
{
    const Port* P = PortAt(PortNumber);

    return P != NULL && P->Major == 2;
}

/* QUIRK: no accessibility check, so a removed controller reads as tunneled */
UCHAR
XhciRootHub::QueryTunnelState(
    _In_ ULONG PortNumber) const
{
    const XhciRegisters* Registers = &m_Controller->m_Registers;
    const Port* P = PortAt(PortNumber);
    ULONG Value;
    ULONG Mask;

    if (P == NULL || P->Major != 3)
        return UCXHUB_TUNNEL_STATE_UNKNOWN;

    if (m_Controller->HasErrata(XhciErrata::Usb4TunnelFromVendorPortReg))
    {
        Value = Registers->ReadExtended32(XHCI_VENDOR_PORT_TUNNEL_BASE +
                                          XHCI_VENDOR_PORT_TUNNEL_STRIDE * (PortNumber - m_FirstUsb30Port));
        Mask = XHCI_VENDOR_PORT_TUNNEL_ACTIVE;
    }
    else if (m_Controller->HasErrata(XhciErrata::Usb4TunnelFromVendorStatusReg))
    {
        Value = Registers->ReadExtended32(XHCI_VENDOR_TUNNEL_STATUS);
        Mask = XHCI_VENDOR_TUNNEL_STATUS_ACTIVE;
    }
    else if (m_PortScTunnelValid)
    {
        Value = ReadPortSc(P);
        Mask = XHCI_PORTSC_TUNNELED;
    }
    else
    {
        return UCXHUB_TUNNEL_STATE_UNKNOWN;
    }

    DPRINT("Root port %lu tunnel register 0x%08lx\n", PortNumber, Value);
    return (Value & Mask) ? UCXHUB_TUNNEL_STATE_TUNNELED : UCXHUB_TUNNEL_STATE_NATIVE;
}

/* Port change events *********************************************************/

VOID
XhciRootHub::OnPortStatusChangeEvent(
    _In_ const XHCI_TRB* Event)
{
    ULONG PortNumber = Event->Dword[0] >> XHCI_EVENT_PORT_ID_SHIFT;
    Port* P = PortAt(PortNumber);

    /* PortAt rejects a Port ID outside the port array */
    if (P == NULL)
    {
        DPRINT1("Port status change event for port %lu, controller has %lu\n", PortNumber, m_PortCount);
        return;
    }

    if (IsHiddenDebugPort(P))
    {
        UpdatePortSc(P,
                     XHCI_PORTSC_PRESERVE,
                     XHCI_PORTSC_CSC | XHCI_PORTSC_WRC | XHCI_PORTSC_OCC |
                     XHCI_PORTSC_PRC | XHCI_PORTSC_PLC | XHCI_PORTSC_CEC);
        return;
    }

    if (AcknowledgeResume(P, FALSE))
        return;

    UcxRootHubPortChanged(m_Ucx);
}

/* Services for usbdevice *****************************************************/

VOID
XhciRootHub::Update20HardwareLpm(
    _In_ ULONG PortNumber,
    _In_ ULONG SlotId,
    _In_ const USBDEVICE_UPDATE_20_HARDWARE_LPM_PARAMETERS* Parameters)
{
    Port* P = PortAt(PortNumber);
    ULONG Value;

    if (P == NULL)
    {
        DPRINT1("LPM update for bad root port %lu\n", PortNumber);
        return;
    }

    NT_ASSERT(P->Major == 2);

    XhciSpinLockGuard Guard(&P->Lock);

    /* Parameters before the enable bit (xHCI 4.23.5.1.1.1) */
    Value = Parameters->HostInitiatedResumeDurationMode |
            (Parameters->L1Timeout << XHCI_PORTHLPMC_L1_TIMEOUT_SHIFT) |
            (Parameters->BestEffortServiceLatencyDeep << XHCI_PORTHLPMC_BESLD_SHIFT);
    UpdatePortReg(P,
                  XHCI_PORT_HLPMC,
                  XHCI_PORTHLPMC_BESLD_MASK | XHCI_PORTHLPMC_HIRDM_MASK | XHCI_PORTHLPMC_L1_TIMEOUT_MASK,
                  Value);

    Value = (Parameters->BestEffortServiceLatency << XHCI_PORTPMSC_BESL_SHIFT) |
            ((SlotId << XHCI_PORTPMSC_L1_SLOT_SHIFT) & XHCI_PORTPMSC_L1_SLOT_MASK);
    if (Parameters->RemoteWakeEnable)
        Value |= XHCI_PORTPMSC_RWE;
    if (Parameters->HardwareLpmEnable)
        Value |= XHCI_PORTPMSC_HLE;
    UpdatePortReg(P,
                  XHCI_PORT_PMSC,
                  XHCI_PORTPMSC_BESL_MASK | XHCI_PORTPMSC_L1_SLOT_MASK | XHCI_PORTPMSC_RWE | XHCI_PORTPMSC_HLE,
                  Value);

    P->LpmEnabled = Parameters->HardwareLpmEnable != 0;
    P->LpmDirty = TRUE;
}

VOID
XhciRootHub::DisableLpmForSlot(
    _In_ ULONG PortNumber,
    _In_ ULONG SlotId)
{
    Port* P = PortAt(PortNumber);
    ULONG Owner;

    if (P == NULL)
        return;

    XhciSpinLockGuard Guard(&P->Lock);

    if (!P->LpmDirty)
        return;

    /* The port may already belong to a newer device */
    Owner = m_Controller->m_Registers.ReadPort32(PortNumber, XHCI_PORT_PMSC);
    if (((Owner & XHCI_PORTPMSC_L1_SLOT_MASK) >> XHCI_PORTPMSC_L1_SLOT_SHIFT) == SlotId)
        ClearUsb20LpmLocked(P);
}

VOID
XhciRootHub::ArmPortResumeTimer(
    _In_ ULONG PortNumber,
    _In_ ULONG Milliseconds)
{
    Port* P = PortAt(PortNumber);

    if (P == NULL)
        return;

    XhciSpinLockGuard Guard(&P->Lock);

    P->ResumeOverrideMs = Milliseconds;
    P->ResumeOverrideCount++;
}

VOID
XhciRootHub::DisarmPortResumeTimer(
    _In_ ULONG PortNumber)
{
    Port* P = PortAt(PortNumber);

    if (P == NULL)
        return;

    XhciSpinLockGuard Guard(&P->Lock);

    P->ResumeOverrideCount--;
}

/* Hub class requests *********************************************************/

VOID
XhciRootHub::CompleteHubRequest(
    _In_ WDFREQUEST Request,
    _In_ PURB Urb,
    _In_ Outcome Result)
{
    NTSTATUS Status;
    USBD_STATUS UsbdStatus;

    switch (Result)
    {
        case Outcome::Ok:
            Status = STATUS_SUCCESS;
            UsbdStatus = USBD_STATUS_SUCCESS;
            break;

        case Outcome::OperationPending:
            Status = STATUS_SUCCESS;
            UsbdStatus = USBD_STATUS_PORT_OPERATION_PENDING;
            break;

        case Outcome::Gone:
            Status = STATUS_NO_SUCH_DEVICE;
            UsbdStatus = USBD_STATUS_DEVICE_GONE;
            break;

        case Outcome::Halted:
            Status = STATUS_ADAPTER_HARDWARE_ERROR;
            UsbdStatus = USBD_STATUS_XACT_ERROR;
            break;

        case Outcome::Stall:
        default:
            Status = STATUS_UNSUCCESSFUL;
            UsbdStatus = USBD_STATUS_STALL_PID;
            break;
    }

    Urb->UrbHeader.Status = UsbdStatus;
    WdfRequestComplete(Request, Status);
}

/** PORTSC writes need a running controller (xHCI 5.4.8). */
XhciRootHub::Outcome
XhciRootHub::CheckRunning()
{
    ULONG Status = m_Controller->m_Registers.ReadOperational32(XHCI_OP_USBSTS);

    if (Status == XhciPortGone)
    {
        DPRINT1("Root hub port feature: USBSTS reads all ones, controller gone\n");
        m_Controller->MarkGone();
        return Outcome::Gone;
    }

    if (Status & XHCI_USBSTS_HCH)
    {
        DPRINT1("Root hub port feature: controller halted, USBSTS 0x%08lx\n", Status);
        return Outcome::Halted;
    }

    return Outcome::Ok;
}

XhciRootHub::Outcome
XhciRootHub::ClearHubFeature(
    _In_ PURB Urb)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET Setup = XhciSetupOf(Urb);

    if (Setup->bmRequestType.B == HubRequestToHub &&
        Setup->bRequest == USB_REQUEST_CLEAR_FEATURE &&
        Setup->wIndex.W == 0 &&
        Setup->wLength == 0 &&
        (Setup->wValue.W == FeatureHubLocalPowerChange || Setup->wValue.W == FeatureHubOverCurrentChange))
    {
        return Outcome::Ok;
    }

    XhciTraceBadRequest("ClearHubFeature", Urb);
    return Outcome::Stall;
}

XhciRootHub::Outcome
XhciRootHub::GetHubStatus(
    _In_ PURB Urb)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET Setup = XhciSetupOf(Urb);
    PULONG Buffer;

    if (Setup->bmRequestType.B != HubRequestFromHub ||
        Setup->bRequest != USB_REQUEST_GET_STATUS ||
        Setup->wValue.W != 0 ||
        Setup->wIndex.W != 0 ||
        Setup->wLength != sizeof(*Buffer))
    {
        XhciTraceBadRequest("GetHubStatus", Urb);
        return Outcome::Stall;
    }

    Buffer = (PULONG)XhciHubRequestBuffer(Urb, sizeof(*Buffer));
    if (Buffer == NULL)
    {
        XhciTraceBadRequest("GetHubStatus buffer", Urb);
        return Outcome::Stall;
    }

    /* No local power or over current status, no changes */
    *Buffer = 0;
    return Outcome::Ok;
}

XhciRootHub::Outcome
XhciRootHub::GetPortStatus(
    _In_ PURB Urb)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET Setup = XhciSetupOf(Urb);
    PUSB_PORT_EXT_STATUS_AND_CHANGE Out;
    BOOLEAN Extended;
    ULONG Length;
    ULONG PortSc;
    ULONG Link;
    Port* P;

    if (!m_Controller->IsAccessible())
    {
        DPRINT1("GetPortStatus: controller not accessible\n");
        return Outcome::Gone;
    }

    Extended = (Setup->wValue.W == USB_STATUS_EXT_PORT_STATUS);
    Length = Extended ? sizeof(Out->AsUlong64) : sizeof(Out->PortStatusChange);
    P = PortAt(Setup->wIndex.W);

    if (Setup->bmRequestType.B != HubRequestFromPort ||
        Setup->bRequest != USB_REQUEST_GET_STATUS ||
        (Setup->wValue.W != USB_STATUS_PORT_STATUS && !Extended) ||
        P == NULL ||
        Setup->wLength != Length ||
        !((P->Major == 2 && !Extended) || P->Major == 3))
    {
        XhciTraceBadRequest("GetPortStatus", Urb);
        return Outcome::Stall;
    }

    Out = (PUSB_PORT_EXT_STATUS_AND_CHANGE)XhciHubRequestBuffer(Urb, Length);
    if (Out == NULL)
    {
        XhciTraceBadRequest("GetPortStatus buffer", Urb);
        return Outcome::Stall;
    }

    PortSc = ReadPortSc(P);
    if (PortSc == XhciPortGone)
    {
        DPRINT1("GetPortStatus: port %lu PORTSC reads all ones\n", P->Number);
        m_Controller->MarkGone();
        return Outcome::Gone;
    }

    Out->PortStatusChange.AsUlong32 = 0;
    Link = XhciLinkState(PortSc);

    if (P->Major == 2)
    {
        PUSB_20_PORT_STATUS Status = &Out->PortStatusChange.PortStatus.Usb20PortStatus;
        PUSB_20_PORT_CHANGE Change = &Out->PortStatusChange.PortChange.Usb20PortChange;
        ULONG Speed = XhciPortSpeed(PortSc);

        Status->CurrentConnectStatus = (PortSc & XHCI_PORTSC_CCS) != 0;
        Status->PortEnabledDisabled = (PortSc & XHCI_PORTSC_PED) != 0;
        Status->Suspend = (Link == PORT_LINK_STATE_U3 || Link == XHCI_PLS_RESUME);
        Status->OverCurrent = (PortSc & XHCI_PORTSC_OCA) != 0;
        Status->Reset = (PortSc & XHCI_PORTSC_PR) != 0;
        Status->L1 = (Link == PORT_LINK_STATE_U2);
        Status->PortPower = (PortSc & XHCI_PORTSC_PP) != 0;
        Status->LowSpeedDeviceAttached = (Speed == XHCI_SPEED_LOW);
        Status->HighSpeedDeviceAttached = (Speed == XHCI_SPEED_HIGH);
        Status->PortIndicatorControl = (PortSc & XHCI_PORTSC_PIC_MASK) != 0;
        if (Link == PORT_LINK_STATE_TEST_MODE)
        {
            DPRINT1("GetPortStatus: port %lu is in test mode\n", P->Number);
            Status->PortTestMode = 1;
        }

        Change->ConnectStatusChange = (PortSc & XHCI_PORTSC_CSC) != 0;
        Change->PortEnableDisableChange = (PortSc & XHCI_PORTSC_PEC) != 0;
        Change->OverCurrentIndicatorChange = (PortSc & XHCI_PORTSC_OCC) != 0;
        Change->ResetChange = (PortSc & XHCI_PORTSC_PRC) != 0;

        /* A resume this driver is still timing shows as suspended with no change yet */
        if (Link == XHCI_PLS_RESUME)
            return Outcome::OperationPending;

        Change->SuspendChange = (PortSc & XHCI_PORTSC_PLC) != 0;
        return Outcome::Ok;
    }

    PUSB_30_PORT_STATUS Status = &Out->PortStatusChange.PortStatus.Usb30PortStatus;
    PUSB_30_PORT_CHANGE Change = &Out->PortStatusChange.PortChange.Usb30PortChange;

    if (IsHiddenDebugPort(P))
    {
        /* An empty powered port; the extended dword is left as the caller had it */
        Status->PortPower = 1;
        Status->PortLinkState = PORT_LINK_STATE_RX_DETECT;
        return Outcome::Ok;
    }

    /* Resume is no hub link state; CAS means the device likely sits in Compliance */
    if (Link == XHCI_PLS_RESUME)
        Link = PORT_LINK_STATE_RECOVERY;
    if (PortSc & XHCI_PORTSC_CAS)
        Link = PORT_LINK_STATE_COMPLIANCE_MODE;

    Status->CurrentConnectStatus = (PortSc & XHCI_PORTSC_CCS) != 0;
    Status->PortEnabledDisabled = (PortSc & XHCI_PORTSC_PED) != 0;
    Status->OverCurrent = (PortSc & XHCI_PORTSC_OCA) != 0;
    Status->Reset = (PortSc & XHCI_PORTSC_PR) != 0;
    Status->PortLinkState = Link;
    Status->PortPower = (PortSc & XHCI_PORTSC_PP) != 0;

    Change->ConnectStatusChange = (PortSc & XHCI_PORTSC_CSC) != 0;
    Change->OverCurrentIndicatorChange = (PortSc & XHCI_PORTSC_OCC) != 0;
    Change->ResetChange = (PortSc & XHCI_PORTSC_PRC) != 0;
    Change->BHResetChange = (PortSc & XHCI_PORTSC_WRC) != 0;
    Change->PortLinkStateChange = (PortSc & XHCI_PORTSC_PLC) != 0;
    Change->PortConfigErrorChange = (PortSc & XHCI_PORTSC_CEC) != 0;

    if (Extended)
    {
        PUSB_PORT_EXT_STATUS ExtStatus = &Out->PortExtStatus;
        ULONG LinkInfo = m_Controller->m_Registers.ReadPort32(P->Number, XHCI_PORT_LI);
        ULONG Speed = XhciPortSpeed(PortSc);

        /* Without a PSI table, speed 1 on a 3.x port means the controller did not report it */
        if (P->SpeedCount == 0 && Speed == XHCI_SPEED_FULL)
            Speed = XHCI_SPEED_SUPER;

        ExtStatus->AsUlong32 = 0;
        ExtStatus->RxSublinkSpeedID = Speed;
        ExtStatus->TxSublinkSpeedID = Speed;
        ExtStatus->RxLaneCount = (LinkInfo & XHCI_PORTLI_RX_LANES_MASK) >> XHCI_PORTLI_RX_LANES_SHIFT;
        ExtStatus->TxLaneCount = (LinkInfo & XHCI_PORTLI_TX_LANES_MASK) >> XHCI_PORTLI_TX_LANES_SHIFT;
    }

    return Outcome::Ok;
}

XhciRootHub::Outcome
XhciRootHub::ClearPortFeature(
    _In_ PURB Urb)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET Setup = XhciSetupOf(Urb);
    Outcome Result;
    Port* P;

    if (!m_Controller->IsAccessible())
    {
        DPRINT1("ClearPortFeature: controller not accessible\n");
        return Outcome::Gone;
    }

    Result = CheckRunning();
    if (Result != Outcome::Ok)
        return Result;

    P = PortAt(Setup->wIndex.LowByte);
    if (Setup->bmRequestType.B != HubRequestToPort ||
        Setup->bRequest != USB_REQUEST_CLEAR_FEATURE ||
        P == NULL ||
        Setup->wLength != 0)
    {
        XhciTraceBadRequest("ClearPortFeature", Urb);
        return Outcome::Stall;
    }

    if (P->Major == 2)
        Result = ClearUsb20PortFeature(P, Setup->wValue.W, Setup->wIndex.HiByte);
    else if (P->Major == 3)
        Result = ClearUsb30PortFeature(P, Setup->wValue.W, Setup->wIndex.HiByte);
    else
        Result = Outcome::Stall;

    if (Result == Outcome::Stall)
        XhciTraceBadRequest("ClearPortFeature", Urb);

    return Result;
}

XhciRootHub::Outcome
XhciRootHub::ClearUsb20PortFeature(
    _In_ Port* P,
    _In_ USHORT Feature,
    _In_ UCHAR Value)
{
    ULONG PortSc;

    if (Value != 0 && Feature != FeaturePortIndicator)
        return Outcome::Stall;

    switch (Feature)
    {
        case FeaturePortEnable:
            if (m_Controller->HasErrata(XhciErrata::PortHighSpeedDisableSuspends))
            {
                XhciSpinLockGuard Guard(&P->Lock);

                PortSc = UpdatePortScLocked(P, XHCI_PORTSC_PRESERVE, XhciLinkWrite(PORT_LINK_STATE_U3));
                if (!(PortSc & XHCI_PORTSC_PED) || XhciLinkState(PortSc) >= PORT_LINK_STATE_U3)
                    DPRINT1("Port %lu: disable mapped to suspend with PORTSC 0x%08lx\n", P->Number, PortSc);
            }
            else
            {
                UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_PED);
            }
            return Outcome::Ok;

        case FeaturePortSuspend:
        {
            /* Host initiated resume (xHCI 4.15.2.2) */
            if (!NT_SUCCESS(WaitForU3(P, FALSE)))
                return Outcome::Stall;

            {
                XhciSpinLockGuard Guard(&P->Lock);
                ULONG Link;

                PortSc = UpdatePortScLocked(P, XHCI_PORTSC_PRESERVE, XhciLinkWrite(XHCI_PLS_RESUME));
                Link = XhciLinkState(PortSc);
                if (!(PortSc & XHCI_PORTSC_PED) || (Link != PORT_LINK_STATE_U3 && Link != XHCI_PLS_RESUME))
                    DPRINT1("Port %lu: resume requested with PORTSC 0x%08lx\n", P->Number, PortSc);
            }

            StartResumeTimer(P);
            return Outcome::Ok;
        }

        case FeaturePortPower:
            UpdatePortSc(P, XHCI_PORTSC_PIC_MASK | XHCI_PORTSC_WAKE_MASK, 0);
            return Outcome::Ok;

        case FeaturePortIndicator:
            if (Value > 3)
                return Outcome::Stall;

            UpdatePortSc(P, XHCI_PORTSC_PP | XHCI_PORTSC_WAKE_MASK, 0);
            return Outcome::Ok;

        case FeatureConnectChange:
            UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_CSC);
            return Outcome::Ok;

        case FeatureEnableChange:
            UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_PEC);
            return Outcome::Ok;

        case FeatureSuspendChange:
            UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_PLC);
            return Outcome::Ok;

        case FeatureOverCurrentChange:
            UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_OCC);
            return Outcome::Ok;

        case FeatureResetChange:
            UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_PRC);
            return Outcome::Ok;

        default:
            return Outcome::Stall;
    }
}

XhciRootHub::Outcome
XhciRootHub::ClearUsb30PortFeature(
    _In_ Port* P,
    _In_ USHORT Feature,
    _In_ UCHAR Value)
{
    ULONG ChangeBit;

    if (Value != 0)
        return Outcome::Stall;

    switch (Feature)
    {
        case FeaturePortPower:
            UpdatePortSc(P, XHCI_PORTSC_PIC_MASK | XHCI_PORTSC_WAKE_MASK, 0);
            return Outcome::Ok;

        case FeatureForceLinkPmAccept:
        {
            XhciSpinLockGuard Guard(&P->Lock);

            UpdatePortReg(P, XHCI_PORT_PMSC, XHCI_PORTPMSC_FLA, 0);
            return Outcome::Ok;
        }

        case FeatureConnectChange:
            ChangeBit = XHCI_PORTSC_CSC;
            break;

        case FeatureOverCurrentChange:
            ChangeBit = XHCI_PORTSC_OCC;
            break;

        case FeatureResetChange:
            ChangeBit = XHCI_PORTSC_PRC;
            break;

        case FeatureLinkStateChange:
            ChangeBit = XHCI_PORTSC_PLC;
            break;

        case FeatureConfigErrorChange:
            ChangeBit = XHCI_PORTSC_CEC;
            break;

        case FeatureWarmResetChange:
            ChangeBit = XHCI_PORTSC_WRC;
            break;

        default:
            return Outcome::Stall;
    }

    UpdatePortSc(P, XHCI_PORTSC_PRESERVE, ChangeBit);
    return Outcome::Ok;
}

XhciRootHub::Outcome
XhciRootHub::SetPortFeature(
    _In_ PURB Urb)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET Setup = XhciSetupOf(Urb);
    Outcome Result;
    Port* P;

    if (!m_Controller->IsAccessible())
    {
        DPRINT1("SetPortFeature: controller not accessible\n");
        return Outcome::Gone;
    }

    Result = CheckRunning();
    if (Result != Outcome::Ok)
        return Result;

    P = PortAt(Setup->wIndex.LowByte);
    if (Setup->bmRequestType.B != HubRequestToPort ||
        Setup->bRequest != USB_REQUEST_SET_FEATURE ||
        P == NULL ||
        Setup->wLength != 0)
    {
        XhciTraceBadRequest("SetPortFeature", Urb);
        return Outcome::Stall;
    }

    if (P->Major == 2)
        Result = SetUsb20PortFeature(P, Setup->wValue.W, Setup->wIndex.HiByte);
    else if (P->Major == 3)
        Result = SetUsb30PortFeature(P, Setup->wValue.W, Setup->wIndex.HiByte);
    else
        Result = Outcome::Stall;

    if (Result == Outcome::Stall)
        XhciTraceBadRequest("SetPortFeature", Urb);

    return Result;
}

XhciRootHub::Outcome
XhciRootHub::SetUsb20PortFeature(
    _In_ Port* P,
    _In_ USHORT Feature,
    _In_ UCHAR Value)
{
    if (Value != 0 && Feature != FeaturePortTest && Feature != FeaturePortIndicator)
        return Outcome::Stall;

    switch (Feature)
    {
        case FeaturePortReset:
        {
            XhciSpinLockGuard Guard(&P->Lock);

            /* LpmEnabled is kept on purpose, the resume timer may turn HLE back on */
            if (P->LpmDirty)
                ClearUsb20LpmLocked(P);

            UpdatePortScLocked(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_PR);
            return Outcome::Ok;
        }

        case FeaturePortSuspend:
            /* xHCI 4.15.1; some controllers only enter U3 from L0 */
            if (m_Controller->HasErrata(XhciErrata::PortHighSpeedLpmOffForSuspend) &&
                !NT_SUCCESS(WaitU0(P)))
            {
                return Outcome::Stall;
            }

            SuspendPort(P);
            return Outcome::Ok;

        case FeaturePortPower:
            UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_PP);
            return Outcome::Ok;

        case FeaturePortTest:
        {
            if (Value > 5)
                return Outcome::Stall;

            XhciSpinLockGuard Guard(&P->Lock);

            /* No xHCI 4.19.6 preconditions are enforced */
            UpdatePortReg(P, XHCI_PORT_PMSC, XHCI_PORTPMSC_TEST_MASK, (ULONG)Value << XHCI_PORTPMSC_TEST_SHIFT);
            return Outcome::Ok;
        }

        case FeaturePortIndicator:
            if (Value > 3)
                return Outcome::Stall;

            /* Hub "off" (3) and "automatic" (0) both become xHCI off */
            if (Value == 3)
                Value = 0;

            UpdatePortSc(P, XHCI_PORTSC_PP | XHCI_PORTSC_WAKE_MASK, (ULONG)Value << XHCI_PORTSC_PIC_SHIFT);
            return Outcome::Ok;

        default:
            return Outcome::Stall;
    }
}

XhciRootHub::Outcome
XhciRootHub::SetUsb30PortFeature(
    _In_ Port* P,
    _In_ USHORT Feature,
    _In_ UCHAR Value)
{
    if (Value != 0 &&
        Feature != FeatureU1Timeout &&
        Feature != FeatureU2Timeout &&
        Feature != FeaturePortLinkState &&
        Feature != FeatureRemoteWakeMask)
    {
        return Outcome::Stall;
    }

    switch (Feature)
    {
        case FeaturePortReset:
            UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_PR);
            return Outcome::Ok;

        case FeatureWarmReset:
            UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_WPR);
            return Outcome::Ok;

        case FeaturePortPower:
            UpdatePortSc(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_PP);
            return Outcome::Ok;

        case FeatureU1Timeout:
        {
            XhciSpinLockGuard Guard(&P->Lock);

            UpdatePortReg(P, XHCI_PORT_PMSC, XHCI_PORTPMSC_U1_TIMEOUT_MASK, Value);
            P->U1Timeout = Value;
            return Outcome::Ok;
        }

        case FeatureU2Timeout:
        {
            XhciSpinLockGuard Guard(&P->Lock);

            UpdatePortReg(P,
                          XHCI_PORT_PMSC,
                          XHCI_PORTPMSC_U2_TIMEOUT_MASK,
                          (ULONG)Value << XHCI_PORTPMSC_U2_TIMEOUT_SHIFT);
            P->U2Timeout = Value;
            return Outcome::Ok;
        }

        case FeaturePortLinkState:
            return SetUsb30LinkState(P, Value);

        case FeatureRemoteWakeMask:
            if (Value & ~0x07)
                return Outcome::Stall;

            UpdatePortSc(P, XHCI_PORTSC_PP | XHCI_PORTSC_PIC_MASK, (ULONG)Value << XHCI_PORTSC_WAKE_SHIFT);
            return Outcome::Ok;

        case FeatureForceLinkPmAccept:
        {
            XhciSpinLockGuard Guard(&P->Lock);

            UpdatePortReg(P, XHCI_PORT_PMSC, 0, XHCI_PORTPMSC_FLA);
            return Outcome::Ok;
        }

        default:
            return Outcome::Stall;
    }
}

XhciRootHub::Outcome
XhciRootHub::SetUsb30LinkState(
    _In_ Port* P,
    _In_ UCHAR LinkState)
{
    if (LinkState > PORT_LINK_STATE_RX_DETECT)
        return Outcome::Stall;

    /* Applies to every non U0 target, not only U3 */
    if (LinkState != PORT_LINK_STATE_U0 &&
        m_Controller->HasErrata(XhciErrata::PortWakeToU0BeforeSuspend) &&
        !NT_SUCCESS(WaitU0(P)))
    {
        return Outcome::Stall;
    }

    /* A U3 request completes before the link gets there */
    if (LinkState == PORT_LINK_STATE_U0 && !NT_SUCCESS(WaitForU3(P, FALSE)))
        return Outcome::Stall;

    {
        XhciSpinLockGuard Guard(&P->Lock);

        if (LinkState == PORT_LINK_STATE_U3)
            P->ResumeHandled = FALSE;

        /* SS.Disabled is entered through PED (xHCI 4.19.1.2) */
        if (LinkState == PORT_LINK_STATE_DISABLED)
            UpdatePortScLocked(P, XHCI_PORTSC_PRESERVE, XHCI_PORTSC_PED);
        else
            UpdatePortScLocked(P, XHCI_PORTSC_PRESERVE, XhciLinkWrite(LinkState));
    }

    if (LinkState == PORT_LINK_STATE_U3 &&
        m_Controller->HasErrata(XhciErrata::PortRetrySuspendEntry) &&
        !NT_SUCCESS(WaitForU3(P, TRUE)))
    {
        return Outcome::Stall;
    }

    return Outcome::Ok;
}

XhciRootHub::Outcome
XhciRootHub::GetPortErrorCount(
    _In_ PURB Urb)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET Setup = XhciSetupOf(Urb);
    PUSHORT Buffer;
    ULONG Errors;
    Port* P;

    if (!m_Controller->IsAccessible())
    {
        DPRINT("GetPortErrorCount: controller not accessible\n");
        return Outcome::Gone;
    }

    /* UCX only routes 0x80 here, so this check fails every request it sends */
    P = PortAt(Setup->wIndex.W);
    if (Setup->bmRequestType.B != HubRequestFromPort ||
        Setup->bRequest != USB_REQUEST_GET_PORT_ERR_COUNT ||
        Setup->wValue.W != 0 ||
        P == NULL ||
        Setup->wLength != sizeof(*Buffer) ||
        P->Major != 3)
    {
        XhciTraceBadRequest("GetPortErrorCount", Urb);
        return Outcome::Stall;
    }

    Buffer = (PUSHORT)XhciHubRequestBuffer(Urb, sizeof(*Buffer));
    if (Buffer == NULL)
    {
        XhciTraceBadRequest("GetPortErrorCount buffer", Urb);
        return Outcome::Stall;
    }

    Errors = m_Controller->m_Registers.ReadPort32(P->Number, XHCI_PORT_LI) & XHCI_PORTLI_ERROR_COUNT_MASK;
    if (Errors > 10)
        DPRINT1("Port %lu: %lu link errors\n", P->Number, Errors);

    *Buffer = (USHORT)Errors;
    return Outcome::Ok;
}

/** Answers at once with the change bitmap; UCX holds the next transfer until a port changes. */
XhciRootHub::Outcome
XhciRootHub::InterruptTransfer(
    _In_ PURB Urb)
{
    struct _URB_BULK_OR_INTERRUPT_TRANSFER* Transfer = &Urb->UrbBulkOrInterruptTransfer;
    ULONG Length = Transfer->TransferBufferLength;
    PUCHAR Bitmap;
    ULONG Last;
    ULONG Number;

    if (!m_Controller->IsAccessible())
    {
        DPRINT1("Status change transfer: controller not accessible\n");
        return Outcome::Gone;
    }

    /* UCX has no callback for the root hub PDO reaching D0; this is the first sign of it */
    if (m_AwaitingStatusRead)
    {
        m_Controller->OnRootHubD0Entry();
        m_AwaitingStatusRead = FALSE;
    }

    if (Length == 0)
        return Outcome::Ok;

    Bitmap = (PUCHAR)XhciTransferBuffer(Transfer->TransferBuffer, Transfer->TransferBufferMDL, Length, Length);
    if (Bitmap == NULL)
    {
        DPRINT1("Status change transfer without a buffer\n");
        return Outcome::Stall;
    }

    RtlZeroMemory(Bitmap, Length);

    Last = (Length > 32) ? 255 : (Length * 8 - 1);
    if (Last > m_PortCount)
        Last = m_PortCount;

    for (Number = 1; Number <= Last; Number++)
    {
        Port* P = &m_Ports[Number - 1];
        ULONG Changes;
        ULONG PortSc;

        if (P->Major == 0)
            continue;

        PortSc = ReadPortSc(P);
        if (PortSc == XhciPortGone)
        {
            DPRINT1("Status change transfer: port %lu PORTSC reads all ones\n", Number);
            m_Controller->MarkGone();
            return Outcome::Gone;
        }

        if (P->Major == 2)
        {
            Changes = PortSc & (XHCI_PORTSC_CSC | XHCI_PORTSC_PEC | XHCI_PORTSC_PLC |
                                XHCI_PORTSC_OCC | XHCI_PORTSC_PRC);

            /* This driver finishes a resume itself; its PLC is not for the hub */
            if (XhciLinkState(PortSc) == XHCI_PLS_RESUME)
                Changes &= ~XHCI_PORTSC_PLC;
        }
        else
        {
            Changes = PortSc & (XHCI_PORTSC_CSC | XHCI_PORTSC_OCC | XHCI_PORTSC_PRC |
                                XHCI_PORTSC_WRC | XHCI_PORTSC_PLC | XHCI_PORTSC_CEC);

            if (Changes != 0 && IsHiddenDebugPort(P))
                Changes = 0;
        }

        if (Changes != 0)
            Bitmap[Number / 8] |= (UCHAR)(1 << (Number % 8));
    }

    return Outcome::Ok;
}

typedef struct _XHCI_SPEED_LIST
{
    PUSB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED Speeds;
    USHORT Capacity;
    USHORT Count;
    UCHAR Minor;
} XHCI_SPEED_LIST, *PXHCI_SPEED_LIST;

/** Appends one sublink speed (Mode 1 asymmetric, Direction 1 TX); counts past capacity. */
static
VOID
NTAPI
XhciAddSpeed(
    _Inout_ PXHCI_SPEED_LIST List,
    _In_ ULONG Id,
    _In_ ULONG Exponent,
    _In_ ULONG Mantissa,
    _In_ ULONG Mode,
    _In_ ULONG Direction)
{
    if (List->Count < List->Capacity)
    {
        PUSB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED Speed = &List->Speeds[List->Count];

        Speed->AsUlong32 = 0;
        Speed->SublinkSpeedAttrID = Id;
        Speed->LaneSpeedExponent = Exponent;
        Speed->SublinkTypeMode = Mode;
        Speed->SublinkTypeDir = Direction;
        Speed->LinkProtocol = (List->Minor != 0 && Id > 4) ? 1 : 0;
        Speed->LaneSpeedMantissa = Mantissa;
    }

    List->Count++;
}

/**
 * Sublink speed list of a 3.x port from its PSI table.
 * Entries past the caller's capacity are counted but not written.
 */
NTSTATUS
XhciRootHub::FillSpeeds(
    _In_ const Port* P,
    _Inout_ PROOTHUB_30PORT_INFO_EX Entry) const
{
    XhciRegisters* Registers = &m_Controller->m_Registers;
    XHCI_SPEED_LIST List;
    ULONG SeenIds = 0;
    ULONG Index;

    List.Speeds = Entry->Speeds;
    List.Capacity = (Entry->Speeds != NULL) ? Entry->MaxSpeedsCount : 0;
    List.Count = 0;
    List.Minor = P->Minor;

    for (Index = 0; Index < P->SpeedCount; Index++)
    {
        ULONG Psi = Registers->ReadExtended32(P->SpeedTable + Index * sizeof(ULONG));
        ULONG Id = Psi & XHCI_PSI_ID_MASK;
        ULONG Exponent = (Psi & XHCI_PSI_EXPONENT_MASK) >> XHCI_PSI_EXPONENT_SHIFT;
        ULONG Mantissa = Psi >> XHCI_PSI_MANTISSA_SHIFT;
        ULONG Next;

        if (SeenIds & (1UL << Id))
        {
            DPRINT1("Port %lu: speed ID %lu listed twice\n", P->Number, Id);
            return STATUS_INVALID_PARAMETER;
        }
        SeenIds |= 1UL << Id;

        switch ((Psi & XHCI_PSI_TYPE_MASK) >> XHCI_PSI_TYPE_SHIFT)
        {
            case XHCI_PSI_TYPE_SYMMETRIC:
                XhciAddSpeed(&List, Id, Exponent, Mantissa, 0, 0);
                XhciAddSpeed(&List, Id, Exponent, Mantissa, 0, 1);
                break;

            case XHCI_PSI_TYPE_ASYMMETRIC_RX:
                XhciAddSpeed(&List, Id, Exponent, Mantissa, 1, 0);

                if (++Index >= P->SpeedCount)
                {
                    DPRINT1("Port %lu: speed ID %lu has RX but no TX entry\n", P->Number, Id);
                    return STATUS_INVALID_PARAMETER;
                }

                Next = Registers->ReadExtended32(P->SpeedTable + Index * sizeof(ULONG));
                if ((Next & XHCI_PSI_ID_MASK) != Id ||
                    ((Next & XHCI_PSI_TYPE_MASK) >> XHCI_PSI_TYPE_SHIFT) != XHCI_PSI_TYPE_ASYMMETRIC_TX)
                {
                    DPRINT1("Port %lu: speed ID %lu RX entry followed by 0x%08lx\n", P->Number, Id, Next);
                    return STATUS_INVALID_PARAMETER;
                }

                XhciAddSpeed(&List, Id,
                    (Next & XHCI_PSI_EXPONENT_MASK) >> XHCI_PSI_EXPONENT_SHIFT,
                    Next >> XHCI_PSI_MANTISSA_SHIFT,
                    1,
                    1);
                break;

            case XHCI_PSI_TYPE_ASYMMETRIC_TX:
                DPRINT1("Port %lu: speed ID %lu TX entry without RX\n", P->Number, Id);
                return STATUS_INVALID_PARAMETER;

            default:
                /* Reserved type, nothing reported */
                break;
        }
    }

    /*
     * Default 5 Gb/s and 10 Gb/s pairs. The gates test the ID bitmask against
     * the raw IDs instead of their bits, so both pairs are nearly always added.
     */
    if ((SeenIds & 4) == 0)
    {
        XhciAddSpeed(&List, 4, 3, 5, 0, 0);
        XhciAddSpeed(&List, 4, 3, 5, 0, 1);
    }
    if ((SeenIds & 5) == 0)
    {
        XhciAddSpeed(&List, 5, 3, 10, 0, 0);
        XhciAddSpeed(&List, 5, 3, 10, 0, 1);
    }

    Entry->SpeedsCount = List.Count;
    return STATUS_SUCCESS;
}

/* UCX callbacks **************************************************************/

VOID
NTAPI
XhciRootHub::EvtClearHubFeature(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    PURB Urb = (PURB)XhciRequestArgument(Request);

    CompleteHubRequest(Request, Urb, FromUcx(UcxRootHub)->ClearHubFeature(Urb));
}

VOID
NTAPI
XhciRootHub::EvtClearPortFeature(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    PURB Urb = (PURB)XhciRequestArgument(Request);

    CompleteHubRequest(Request, Urb, FromUcx(UcxRootHub)->ClearPortFeature(Urb));
}

VOID
NTAPI
XhciRootHub::EvtGetHubStatus(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    PURB Urb = (PURB)XhciRequestArgument(Request);

    CompleteHubRequest(Request, Urb, FromUcx(UcxRootHub)->GetHubStatus(Urb));
}

VOID
NTAPI
XhciRootHub::EvtGetPortStatus(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    PURB Urb = (PURB)XhciRequestArgument(Request);

    CompleteHubRequest(Request, Urb, FromUcx(UcxRootHub)->GetPortStatus(Urb));
}

VOID
NTAPI
XhciRootHub::EvtSetHubFeature(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    PURB Urb = (PURB)XhciRequestArgument(Request);

    UNREFERENCED_PARAMETER(UcxRootHub);

    /* The root hub has no settable hub feature; no validation either */
    DPRINT("SetHubFeature stalled\n");
    CompleteHubRequest(Request, Urb, Outcome::Stall);
}

VOID
NTAPI
XhciRootHub::EvtSetPortFeature(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    PURB Urb = (PURB)XhciRequestArgument(Request);

    CompleteHubRequest(Request, Urb, FromUcx(UcxRootHub)->SetPortFeature(Urb));
}

VOID
NTAPI
XhciRootHub::EvtGetPortErrorCount(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    PURB Urb = (PURB)XhciRequestArgument(Request);

    CompleteHubRequest(Request, Urb, FromUcx(UcxRootHub)->GetPortErrorCount(Urb));
}

VOID
NTAPI
XhciRootHub::EvtInterruptTransfer(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    PURB Urb = (PURB)XhciRequestArgument(Request);

    CompleteHubRequest(Request, Urb, FromUcx(UcxRootHub)->InterruptTransfer(Urb));
}

VOID
NTAPI
XhciRootHub::EvtGetInfo(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    XhciRootHub* Hub = FromUcx(UcxRootHub);
    PROOTHUB_INFO Info = (PROOTHUB_INFO)XhciRequestArgument(Request);

    if (Info == NULL || Info->Size < sizeof(*Info))
    {
        DPRINT1("GetInfo: buffer too small\n");
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return;
    }

    Info->ControllerType = ControllerTypeXhci;
    Info->NumberOf20Ports = Hub->m_Usb20PortCount;
    Info->NumberOf30Ports = Hub->m_Usb30PortCount;
    Info->MaxU1ExitLatency = Hub->m_U1ExitLatency;
    Info->MaxU2ExitLatency = Hub->m_U2ExitLatency;

    WdfRequestComplete(Request, STATUS_SUCCESS);
}

VOID
NTAPI
XhciRootHub::EvtGet20PortInfo(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    XhciRootHub* Hub = FromUcx(UcxRootHub);
    PROOTHUB_20PORTS_INFO Info = (PROOTHUB_20PORTS_INFO)XhciRequestArgument(Request);
    BOOLEAN NoLpm;
    ULONG Next = 0;
    ULONG Index;

    if (Info == NULL ||
        Info->Size < sizeof(*Info) ||
        Info->NumberOfPorts != Hub->m_Usb20PortCount ||
        Info->PortInfoSize < sizeof(ROOTHUB_20PORT_INFO))
    {
        DPRINT1("Get20PortInfo: bad request, %u ports of %u bytes\n",
                Info ? Info->NumberOfPorts : 0,
                Info ? Info->PortInfoSize : 0);
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return;
    }

    NoLpm = Hub->m_Controller->HasErrata(XhciErrata::PortHighSpeedLpmOff);

    for (Index = 0; Index < Hub->m_PortCount; Index++)
    {
        const Port* P = &Hub->m_Ports[Index];
        PROOTHUB_20PORT_INFO Entry;

        if (P->Major != 2)
            continue;

        Entry = Info->PortInfoArray[Next++];
        Entry->PortNumber = (USHORT)P->Number;
        Entry->MinorRevision = P->Minor;
        Entry->HubDepth = P->HubDepth;
        Entry->Removable = P->Removable ? TriStateTrue : TriStateFalse;
        Entry->IntegratedHubImplemented = P->IntegratedHub ? TriStateTrue : TriStateFalse;

        /* Report no LPM flags when 2.0 LPM is disabled */
        Entry->ControllerUsb20HardwareLpmFlags.AsUchar = NoLpm ? 0 : P->LpmCapabilities;
    }

    WdfRequestComplete(Request, STATUS_SUCCESS);
}

VOID
NTAPI
XhciRootHub::EvtGet30PortInfo(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    XhciRootHub* Hub = FromUcx(UcxRootHub);
    PROOTHUB_30PORTS_INFO Info = (PROOTHUB_30PORTS_INFO)XhciRequestArgument(Request);
    NTSTATUS Status;
    ULONG Next = 0;
    ULONG Index;

    if (Info == NULL ||
        Info->Size < sizeof(*Info) ||
        Info->NumberOfPorts != Hub->m_Usb30PortCount ||
        Info->PortInfoSize < sizeof(ROOTHUB_30PORT_INFO))
    {
        DPRINT1("Get30PortInfo: bad request, %u ports of %u bytes\n",
                Info ? Info->NumberOfPorts : 0,
                Info ? Info->PortInfoSize : 0);
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return;
    }

    for (Index = 0; Index < Hub->m_PortCount; Index++)
    {
        const Port* P = &Hub->m_Ports[Index];
        PROOTHUB_30PORT_INFO Entry;

        if (P->Major != 3)
            continue;

        Entry = Info->PortInfoArray[Next++];
        Entry->PortNumber = (USHORT)P->Number;
        Entry->MinorRevision = P->Minor;
        Entry->HubDepth = P->HubDepth;
        Entry->Removable = P->Removable ? TriStateTrue : TriStateFalse;
        Entry->DebugCapable = (Hub->m_DebugCapability != 0) ? TriStateTrue : TriStateFalse;

        if (Info->PortInfoSize >= sizeof(ROOTHUB_30PORT_INFO_EX))
        {
            /* One bad speed table fails the whole request */
            Status = Hub->FillSpeeds(P, (PROOTHUB_30PORT_INFO_EX)Entry);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Get30PortInfo: port %lu speed list failed 0x%lx\n", P->Number, Status);
                WdfRequestComplete(Request, Status);
                return;
            }
        }
    }

    WdfRequestComplete(Request, STATUS_SUCCESS);
}
