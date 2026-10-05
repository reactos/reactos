/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     SuperSpeed link power: U1 and U2 timeouts, exit latency and link delays
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"
#include "devhcd.h"

#define NDEBUG
#include <debug.h>

/* Timeout values the port understands */
#define TIMEOUT_DISABLED        0x00
#define TIMEOUT_SHORTEST        0x01
#define U1_TIMEOUT_LONGEST      0x7F
#define U2_TIMEOUT_LONGEST      0xFE
#define TIMEOUT_ACCEPT_ONLY     0xFF

/* An endpoint's timeout is this many times its exit latency */
#define LATENCY_TIMEOUT_FACTOR  5

/* U2 timeouts count in units of 256 us */
#define U2_TIMEOUT_UNIT         256

/* Interrupt endpoint usage type bits 5:4, 0 is periodic */
#define INTERRUPT_USAGE_PERIODIC 0x00

/* Microframe length; bInterval is taken raw */
#define MICROFRAME_US           125

/* Exit latency terms, in ns */
#define TP_DELAY_5GBPS_NS       40
#define MAX_PACKET_FORWARD_NS   2100
#define PING_RESPONSE_NS        400

/* Missed pings raise the latency by 10, 20, 40, 80, 160 percent, then link power goes off */
#define NO_PING_FIRST_PERCENT   10
#define NO_PING_LAST_PERCENT    160

/* Below this the link speed is not believable */
#define MIN_LINK_SPEED_BPS      50000000ULL

/* One TP is 20 symbols; this over the speed in bit/s gives ns */
#define TP_SYMBOL_NS_SCALE      200000000000ULL

static
BOOLEAN
NTAPI
HubChildLpmBlocked(
    _In_ HubChild* Child)
{
    return Child->HasHack(ChildHack::DisableLpm) || Child->m_Hub->HasFlag(HubFlag::DisableLpm);
}

static
BOOLEAN
NTAPI
HubChildHasPolicy(
    _In_ HubChild* Child,
    _In_ ULONG Bit)
{
    return (Child->m_LpmPolicy & Bit) != 0;
}

static
VOID
NTAPI
HubChildSetLink(
    _In_ HubChild* Child,
    _In_ ULONG Bit,
    _In_ BOOLEAN Set)
{
    if (Set)
        Child->m_LinkFlags |= Bit;
    else
        Child->m_LinkFlags &= ~Bit;
}

/* Any pipe of the current configuration that is not a control pipe */
static
BOOLEAN
NTAPI
HubChildHasNonControlPipe(
    _In_ HubConfiguration* Config)
{
    PLIST_ENTRY Entry;
    HubInterface* Interface;
    ULONG Index;

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
    {
        Interface = CONTAINING_RECORD(Entry, HubInterface, Link);

        for (Index = 0; Index < Interface->PipeCount; Index++)
        {
            if ((Interface->Pipes[Index].Descriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK) !=
                USB_ENDPOINT_TYPE_CONTROL)
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

VOID
DeviceMachine::InitLinkStates()
{
    m_Device->m_LinkFlags &= ~(HUB_LINK_U1_OFF_FOR_LATENCY |
                               HUB_LINK_U2_OFF_FOR_LATENCY |
                               HUB_LINK_OFF_FOR_NO_PING);
    m_Device->m_LatencyAdjustPercent = 0;
}

/* A tunneled device does not use the port's own link, so the platform's U2 rule for the port is skipped */
static
BOOLEAN
NTAPI
HubChildPortKeepsU2Off(
    _In_ HubChild* Child)
{
    HubPort* Port = Child->m_Port;

    if (!Port->HasFlag(PortFlag::AcpiNoU2))
        return FALSE;

    return !(Port->HasFlag(PortFlag::Usb4Host) && Child->m_TunnelState == HUB_TUNNEL_STATE_TUNNELED);
}

BOOLEAN
NTAPI
HubChildU2ForEnumerated(
    _In_ HubChild* Child)
{
    if (!HubChildHasPolicy(Child, HUB_LPM_POLICY_U2_ACCEPT) ||
        HubChildLpmBlocked(Child) ||
        HubChildPortKeepsU2Off(Child))
    {
        return FALSE;
    }

    Child->m_U2Timeout = HubChildHasPolicy(Child, HUB_LPM_POLICY_U2_INITIATE) ? U2_TIMEOUT_LONGEST
                                                                             : TIMEOUT_ACCEPT_ONLY;
    return TRUE;
}

BOOLEAN
DeviceMachine::U2NeededForEnumerated()
{
    return HubChildU2ForEnumerated(m_Device);
}

/*
 * Endpoint driven U1 timeout. FALSE when an isochronous endpoint cannot
 * live with the exit latency, which turns U1 off.
 */
static
BOOLEAN
NTAPI
HubChildU1FromPipes(
    _In_ HubChild* Child,
    _Out_ PUCHAR Timeout)
{
    HubConfiguration* Config = Child->m_CurrentConfig;
    PUSB_ENDPOINT_DESCRIPTOR Endpoint;
    HubInterface* Interface;
    PLIST_ENTRY Entry;
    BOOLEAN NonControl;
    USHORT Latency;
    USHORT Value;
    USHORT Longest = 0;
    ULONG Index;

    *Timeout = TIMEOUT_DISABLED;
    if (Config == NULL)
        return TRUE;

    NonControl = HubChildHasNonControlPipe(Config);

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
    {
        Interface = CONTAINING_RECORD(Entry, HubInterface, Link);

        for (Index = 0; Index < Interface->PipeCount; Index++)
        {
            Endpoint = Interface->Pipes[Index].Descriptor;
            Latency = USB_ENDPOINT_DIRECTION_IN(Endpoint->bEndpointAddress) ? Child->m_Sel.U1Sel
                                                                            : Child->m_HostU1ExitLatency;

            switch (Endpoint->bmAttributes & USB_ENDPOINT_TYPE_MASK)
            {
                case USB_ENDPOINT_TYPE_CONTROL:
                    Value = NonControl ? LATENCY_TIMEOUT_FACTOR : (USHORT)(LATENCY_TIMEOUT_FACTOR * Latency);
                    break;

                case USB_ENDPOINT_TYPE_ISOCHRONOUS:
                    if (Latency > Endpoint->bInterval * MICROFRAME_US)
                        return FALSE;
                    Value = TIMEOUT_SHORTEST;
                    break;

                default:
                    Value = (USHORT)(LATENCY_TIMEOUT_FACTOR * Latency);
                    break;
            }

            Longest = max(Longest, Value);
        }
    }

    /* Only the low byte is kept before capping: 0x105 gives 5 */
    *Timeout = min((UCHAR)Longest, U1_TIMEOUT_LONGEST);
    return TRUE;
}

VOID
DeviceMachine::CalcU1Timeout()
{
    HubChild* Child = m_Device;
    UCHAR Timeout;

    if (HubChildLpmBlocked(Child) ||
        (Child->m_LinkFlags & (HUB_LINK_U1_OFF_FOR_LATENCY | HUB_LINK_OFF_FOR_NO_PING)))
    {
        Timeout = TIMEOUT_DISABLED;
        HubChildSetLink(Child, HUB_LINK_TARGET_U1, FALSE);
    }
    else
    {
        HubChildSetLink(Child, HUB_LINK_TARGET_U1, HubChildHasPolicy(Child, HUB_LPM_POLICY_U1_ENABLED));

        if (!HubChildHasPolicy(Child, HUB_LPM_POLICY_U1_ACCEPT))
            Timeout = TIMEOUT_DISABLED;
        else if (!HubChildHasPolicy(Child, HUB_LPM_POLICY_U1_INITIATE))
            Timeout = TIMEOUT_ACCEPT_ONLY;
        else if (HubChildHasPolicy(Child, HUB_LPM_POLICY_AGGRESSIVE))
            Timeout = TIMEOUT_SHORTEST;
        else if (HubChildHasPolicy(Child, HUB_LPM_POLICY_CONSERVATIVE))
            Timeout = U1_TIMEOUT_LONGEST;
        else if (!HubChildU1FromPipes(Child, &Timeout))
            HubChildSetLink(Child, HUB_LINK_TARGET_U1, FALSE);
    }

    /* Some hubs cannot take U2 accept only unless U1 is fully on; hubs as devices get accept only */
    if (Child->m_Hub->HasFlag(HubFlag::DisallowU2AcceptOnly) &&
        Child->HasProperty(ChildProperty::IsHub) &&
        Timeout != TIMEOUT_DISABLED)
    {
        Timeout = TIMEOUT_ACCEPT_ONLY;
    }

    Child->m_TargetU1Timeout = Timeout;
    DPRINT("Device %p target U1 timeout 0x%x\n", Child, Timeout);
}

/* What the endpoints of the configuration allow for U2 */
enum class HubU2Verdict
{
    Computed,
    Off,
    AcceptOnly
};

static
HubU2Verdict
NTAPI
HubChildU2FromPipes(
    _In_ HubChild* Child,
    _Out_ PUSHORT Longest,
    _Out_ PBOOLEAN PeriodicInterrupt)
{
    HubConfiguration* Config = Child->m_CurrentConfig;
    PUSB_ENDPOINT_DESCRIPTOR Endpoint;
    HubInterface* Interface;
    PLIST_ENTRY Entry;
    BOOLEAN NonControl;
    BOOLEAN NoHostInitiate = FALSE;
    USHORT Latency;
    USHORT Value;
    ULONG Index;

    *Longest = 0;
    *PeriodicInterrupt = FALSE;
    if (Config == NULL)
        return HubU2Verdict::Computed;

    NonControl = HubChildHasNonControlPipe(Config);

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
    {
        Interface = CONTAINING_RECORD(Entry, HubInterface, Link);

        for (Index = 0; Index < Interface->PipeCount; Index++)
        {
            Endpoint = Interface->Pipes[Index].Descriptor;
            Latency = USB_ENDPOINT_DIRECTION_IN(Endpoint->bEndpointAddress) ? Child->m_Sel.U2Sel
                                                                            : Child->m_HostU2ExitLatency;
            Value = (USHORT)(LATENCY_TIMEOUT_FACTOR * Latency);

            switch (Endpoint->bmAttributes & USB_ENDPOINT_TYPE_MASK)
            {
                case USB_ENDPOINT_TYPE_CONTROL:
                    if (NonControl)
                        Value = TIMEOUT_SHORTEST;
                    break;

                case USB_ENDPOINT_TYPE_ISOCHRONOUS:
                    if (Latency > Endpoint->bInterval * MICROFRAME_US)
                        return HubU2Verdict::Off;
                    Value = TIMEOUT_SHORTEST;
                    break;

                case USB_ENDPOINT_TYPE_INTERRUPT:
                    if ((Endpoint->bmAttributes & USB_30_ENDPOINT_TYPE_INTERRUPT_USAGE_MASK) != INTERRUPT_USAGE_PERIODIC)
                        break;

                    *PeriodicInterrupt = TRUE;
                    if (Latency > Endpoint->bInterval * MICROFRAME_US)
                    {
                        NoHostInitiate = TRUE;
                        Value = TIMEOUT_ACCEPT_ONLY;
                    }
                    break;

                default:
                    break;
            }

            *Longest = max(*Longest, Value);
        }
    }

    return NoHostInitiate ? HubU2Verdict::AcceptOnly : HubU2Verdict::Computed;
}

/* Runs after CalcU1Timeout: a periodic interrupt endpoint moves the U1 target to accept only */
VOID
DeviceMachine::CalcU2Timeout()
{
    HubChild* Child = m_Device;
    HubU2Verdict Verdict;
    BOOLEAN Periodic;
    USHORT Longest;
    UCHAR Timeout;

    if (HubChildLpmBlocked(Child) ||
        (Child->m_LinkFlags & (HUB_LINK_U2_OFF_FOR_LATENCY | HUB_LINK_OFF_FOR_NO_PING)))
    {
        Timeout = TIMEOUT_DISABLED;
        HubChildSetLink(Child, HUB_LINK_TARGET_U2, FALSE);
    }
    else
    {
        HubChildSetLink(Child, HUB_LINK_TARGET_U2, HubChildHasPolicy(Child, HUB_LPM_POLICY_U2_ENABLED));

        if (!HubChildHasPolicy(Child, HUB_LPM_POLICY_U2_ACCEPT))
        {
            Timeout = TIMEOUT_DISABLED;
        }
        else if (Child->HasProperty(ChildProperty::IsHub) ||
                 !HubChildHasPolicy(Child, HUB_LPM_POLICY_U2_INITIATE))
        {
            Timeout = TIMEOUT_ACCEPT_ONLY;
        }
        else if (HubChildHasPolicy(Child, HUB_LPM_POLICY_AGGRESSIVE))
        {
            Timeout = TIMEOUT_SHORTEST;
        }
        else if (HubChildHasPolicy(Child, HUB_LPM_POLICY_CONSERVATIVE))
        {
            Timeout = U2_TIMEOUT_LONGEST;
        }
        else
        {
            Verdict = HubChildU2FromPipes(Child, &Longest, &Periodic);

            if (Verdict == HubU2Verdict::Off)
            {
                Timeout = TIMEOUT_DISABLED;
                HubChildSetLink(Child, HUB_LINK_TARGET_U2, FALSE);
            }
            else if (Verdict == HubU2Verdict::AcceptOnly)
            {
                Timeout = TIMEOUT_ACCEPT_ONLY;
            }
            else
            {
                if (Periodic && Child->m_TargetU1Timeout != TIMEOUT_DISABLED)
                    Child->m_TargetU1Timeout = TIMEOUT_ACCEPT_ONLY;

                if (Longest < U2_TIMEOUT_LONGEST * U2_TIMEOUT_UNIT)
                    Timeout = (UCHAR)(Longest / U2_TIMEOUT_UNIT + 1);
                else
                    Timeout = U2_TIMEOUT_LONGEST;
            }
        }
    }

    /* Accept only U2 next to a host initiated U1 upsets some hubs; turn U2 off instead */
    if (Child->m_Hub->HasFlag(HubFlag::DisallowU2AcceptOnly) &&
        !Child->HasProperty(ChildProperty::IsHub) &&
        Child->m_TargetU1Timeout != TIMEOUT_DISABLED &&
        Child->m_TargetU1Timeout != TIMEOUT_ACCEPT_ONLY &&
        Timeout == TIMEOUT_ACCEPT_ONLY)
    {
        Timeout = TIMEOUT_DISABLED;
        HubChildSetLink(Child, HUB_LINK_TARGET_U2, FALSE);
    }

    Child->m_TargetU2Timeout = Timeout;
    DPRINT("Device %p target U2 timeout 0x%x, U1 0x%x\n", Child, Timeout, Child->m_TargetU1Timeout);
}

/* A state is off when its timeout is 0, or accept only while the device may not use it */
static
BOOLEAN
NTAPI
HubLinkStateOff(
    _In_ UCHAR Timeout,
    _In_ BOOLEAN Target)
{
    return Timeout == TIMEOUT_DISABLED || (!Target && Timeout == TIMEOUT_ACCEPT_ONLY);
}

/* Worked out in ns with 32 bit unsigned arithmetic, stored in us */
VOID
DeviceMachine::ComputeExitLatency()
{
    HubChild* Child = m_Device;
    HubFdo* Hub = Child->m_Hub;
    BOOLEAN U1Off;
    BOOLEAN U2Off;
    ULONG HostLatency;
    ULONG PathDelay;
    ULONG Latency;

    U1Off = HubLinkStateOff(Child->m_TargetU1Timeout, (Child->m_LinkFlags & HUB_LINK_TARGET_U1) != 0);
    U2Off = HubLinkStateOff(Child->m_TargetU2Timeout, (Child->m_LinkFlags & HUB_LINK_TARGET_U2) != 0);

    if (U1Off && U2Off)
    {
        Child->m_TargetExitLatency = 0;
        return;
    }

    HostLatency = (U2Off ? Child->m_HostU1ExitLatency : Child->m_HostU2ExitLatency) * 1000UL;
    PathDelay = Hub->m_ParentInfo.TotalTpPropagationDelay +
                Hub->m_HubDescriptor.Usb30.wHubDelay +
                Child->m_TxTpDelay;

    /* Out to the device and back, plus the device's ping response */
    Latency = max(HostLatency, (ULONG)MAX_PACKET_FORWARD_NS) + PathDelay + PING_RESPONSE_NS + PathDelay;
    Latency += Latency * Child->m_LatencyAdjustPercent / 100;

    Child->m_TargetExitLatency = (USHORT)((Latency + 500) / 1000);
    DPRINT("Device %p target exit latency %u us\n", Child, Child->m_TargetExitLatency);
}

BOOLEAN
DeviceMachine::U1TimeoutChanged()
{
    if (m_Device->m_TargetU1Timeout == m_Device->m_U1Timeout)
        return FALSE;

    m_Device->m_U1Timeout = m_Device->m_TargetU1Timeout;
    return TRUE;
}

BOOLEAN
DeviceMachine::U2TimeoutChanged()
{
    if (m_Device->m_TargetU2Timeout == m_Device->m_U2Timeout)
        return FALSE;

    m_Device->m_U2Timeout = m_Device->m_TargetU2Timeout;
    return TRUE;
}

static
DSM_FEATURE_CHANGE
NTAPI
HubLinkChange(
    _In_ BOOLEAN Wanted,
    _In_ BOOLEAN Enabled)
{
    if (Wanted == Enabled)
        return DsmFeatureKeep;

    return Wanted ? DsmFeatureEnable : DsmFeatureDisable;
}

DSM_FEATURE_CHANGE
DeviceMachine::U1Change()
{
    return HubLinkChange((m_Device->m_LinkFlags & HUB_LINK_TARGET_U1) != 0,
                         m_Device->HasState(ChildState::U1EnabledUpstream));
}

DSM_FEATURE_CHANGE
DeviceMachine::U2Change()
{
    return HubLinkChange((m_Device->m_LinkFlags & HUB_LINK_TARGET_U2) != 0,
                         m_Device->HasState(ChildState::U2EnabledUpstream));
}

VOID
DeviceMachine::MarkU1Enabled()
{
    m_Device->SetState(ChildState::U1EnabledUpstream);
}

VOID
DeviceMachine::MarkU1Disabled()
{
    m_Device->ClearState(ChildState::U1EnabledUpstream);
}

VOID
DeviceMachine::MarkU2Enabled()
{
    m_Device->SetState(ChildState::U2EnabledUpstream);
}

VOID
DeviceMachine::MarkU2Disabled()
{
    m_Device->ClearState(ChildState::U2EnabledUpstream);
}

BOOLEAN
DeviceMachine::ExitLatencyMustRise()
{
    return m_Device->m_TargetExitLatency > m_Device->m_EffectiveExitLatency;
}

BOOLEAN
DeviceMachine::ExitLatencyMayDrop()
{
    return m_Device->m_TargetExitLatency < m_Device->m_EffectiveExitLatency;
}

/* U2 goes first since it costs the most latency; FALSE when both are already off */
BOOLEAN
DeviceMachine::DisableLinkStatesForLatency()
{
    if (m_Device->m_TargetU2Timeout != TIMEOUT_DISABLED)
    {
        m_Device->m_LinkFlags |= HUB_LINK_U2_OFF_FOR_LATENCY;
        return TRUE;
    }

    if (m_Device->m_TargetU1Timeout != TIMEOUT_DISABLED)
    {
        m_Device->m_LinkFlags |= HUB_LINK_U1_OFF_FOR_LATENCY;
        return TRUE;
    }

    DPRINT1("Device %p exit latency still too large with U1 and U2 off\n", m_Device);
    return FALSE;
}

/* FALSE when link power is already off and the controller still misses pings */
BOOLEAN
DeviceMachine::AdjustLatencyForNoPing()
{
    HubChild* Child = m_Device;
    ULONG Doubled;

    if (Child->m_LatencyAdjustPercent == 0)
    {
        Child->m_LatencyAdjustPercent = NO_PING_FIRST_PERCENT;
        return TRUE;
    }

    Doubled = Child->m_LatencyAdjustPercent * 2UL;
    if (Doubled <= NO_PING_LAST_PERCENT)
    {
        Child->m_LatencyAdjustPercent = (UCHAR)Doubled;
        return TRUE;
    }

    if (Child->m_U1Timeout == TIMEOUT_DISABLED && Child->m_U2Timeout == TIMEOUT_DISABLED)
    {
        DPRINT1("Device %p misses pings with link power already off\n", Child);
        return FALSE;
    }

    DPRINT1("Device %p misses pings, turning U1 and U2 off\n", Child);
    Child->m_LinkFlags |= HUB_LINK_OFF_FOR_NO_PING;
    return TRUE;
}

/* TP delay of one direction of the link; FALSE when the attribute gives an unusable speed */
static
BOOLEAN
NTAPI
HubTpDelayFromAttribute(
    _In_ const USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED* Attribute,
    _In_ ULONG LaneCount,
    _Out_ PUSHORT Delay)
{
    static const ULONG64 Scale[] = { 1, 1000, 1000000, 1000000000 };
    ULONG64 Speed;

    Speed = Scale[Attribute->LaneSpeedExponent] * Attribute->LaneSpeedMantissa * (LaneCount + 1);
    if (Speed < MIN_LINK_SPEED_BPS)
        return FALSE;

    *Delay = (USHORT)(TP_SYMBOL_NS_SCALE / Speed);
    return TRUE;
}

/* First attribute of the port matching the sublink speed id in that direction */
static
const USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED*
NTAPI
HubPortFindSpeed(
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
            return Attribute;
    }

    return NULL;
}

/* Gen 1 delays unless the enhanced SuperSpeed port reports something else */
BOOLEAN
DeviceMachine::SetSpeedFor30()
{
    HubChild* Child = m_Device;
    HubPort* Port = Child->m_Port;
    const USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED* Attribute;
    USB_PORT_EXT_STATUS Extended;

    Child->m_RxTpDelay = TP_DELAY_5GBPS_NS;
    Child->m_TxTpDelay = TP_DELAY_5GBPS_NS;

    if (!Port->HasProperty(PortProperty::EnhancedSuperSpeed))
        return TRUE;

    Extended.AsUlong32 = Port->m_Current.ExtendedStatus;

    /* No matching attribute keeps the Gen 1 value; a bad Rx one leaves Tx untouched */
    Attribute = HubPortFindSpeed(Port, Extended.RxSublinkSpeedID, USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED_DIR_RX);
    if (Attribute != NULL && !HubTpDelayFromAttribute(Attribute, Extended.RxLaneCount, &Child->m_RxTpDelay))
    {
        DPRINT1("Port %u receive sublink speed %u is unusable\n", Port->Number(), Extended.RxSublinkSpeedID);
        return FALSE;
    }

    Attribute = HubPortFindSpeed(Port, Extended.TxSublinkSpeedID, USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED_DIR_TX);
    if (Attribute != NULL && !HubTpDelayFromAttribute(Attribute, Extended.TxLaneCount, &Child->m_TxTpDelay))
    {
        DPRINT1("Port %u transmit sublink speed %u is unusable\n", Port->Number(), Extended.TxSublinkSpeedID);
        return FALSE;
    }

    return TRUE;
}
