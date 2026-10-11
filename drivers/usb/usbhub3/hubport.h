/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     One downstream port of a hub
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Port properties fixed at creation */
enum class PortProperty : ULONG
{
    Removable                = 0x00000001,
    AcpiUpcValid             = 0x00000002,
    AcpiPldValid             = 0x00000004,
    DebugCapable             = 0x00000008,
    IntegratedHub            = 0x00000010,
    InConnectorMap           = 0x00000020,
    Usb20LpmCapable          = 0x00000040,
    BeslCapable              = 0x00000080,
    EnhancedSuperSpeed       = 0x00000100,
    TypeCWithoutSwitch       = 0x00000200
};

/* Port state flags, changed at run time */
enum class PortFlag : ULONG
{
    DeviceConnected          = 0x00000001,
    SupportsReattach         = 0x00000002,
    OverCurrentResetArmed = 0x00000004,
    SsInactiveSeenForBoot    = 0x00000008,
    PendingRecoveryUpdate    = 0x00000010,
    PendingHubPowerReference = 0x00000020,

    /* Bound to a USB4 host router name, which m_Usb4HostName holds */
    Usb4Host                 = 0x00000040,

    /* The port _DSM asks for U2 to stay off */
    AcpiNoU2                 = 0x00000080
};

/* Mux bookkeeping per port */
enum class PortMuxFlag : ULONG
{
    PortChangePending        = 0x00000001,
    InterruptReference       = 0x00000002,
    PnpOperationPending      = 0x00000004,
    PowerReference           = 0x00000008,
    ResetPending             = 0x00000010,
    WdfPowerReference        = 0x00000020
};

/* What a port GET_STATUS returns; 8 bytes on an enhanced SuperSpeed port */
struct HubPortStatus
{
    USB_PORT_STATUS_AND_CHANGE StatusChange;
    ULONG ExtendedStatus;
};

/* What creation copies in; built by CreateChildPorts */
struct HubPortInfo
{
    USHORT PortNumber;
    USHORT Protocol;
    ULONG Properties;
    ULONG TotalHubDepth;
    ULONG SspIsochBurstCount;
    PUSB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED SublinkSpeedAttr;
    ULONG SublinkSpeedAttrCount;
};

/* identity */

/* HubConnectorId.Type */
#define HUB_CONNECTOR_NONE              0
#define HUB_CONNECTOR_ROOT              1
#define HUB_CONNECTOR_ACPI              2

/* Root port level plus five external hub tiers */
#define HUB_CONNECTOR_MAX_PATH          6

/** Key of the physical connector a port leads to, shared by its USB 2 and USB 3 halves. */
struct HubConnectorId
{
    ULONG Type;

    /* _PLD of an ACPI port, or the root hub number in the first ULONG */
    UCHAR Location[16];

    /* External hub tiers between the root connector and this port */
    USHORT Depth;

    /* Path[0] is the root port, Path[n] the port at tier n */
    ULONG Path[HUB_CONNECTOR_MAX_PATH];

    /* (GroupToken << 8) | GroupPosition from the _PLD; ACPI ports only */
    ULONG64 TypeCConnectorId;
};

/** _UPC package of an ACPI port. */
struct HubAcpiUpc
{
    UCHAR Connectable;
    UCHAR ConnectorType;
    ULONG Reserved0;
    ULONG Reserved1;
};

/** First 16 bytes of a _PLD buffer (revision 1 layout). */
struct HubAcpiPld
{
    ULONG Revision:7;
    ULONG IgnoreColor:1;
    ULONG Color:24;
    ULONG Width:16;
    ULONG Height:16;
    ULONG UserVisible:1;
    ULONG Dock:1;
    ULONG Lid:1;
    ULONG Panel:3;
    ULONG VerticalPosition:2;
    ULONG HorizontalPosition:2;
    ULONG Shape:4;
    ULONG GroupOrientation:1;
    ULONG GroupToken:8;
    ULONG GroupPosition:8;
    ULONG Bay:1;
    ULONG Ejectable:1;
    ULONG OspmEjectionRequired:1;
    ULONG CabinetNumber:8;
    ULONG CardCageNumber:8;
    ULONG Reference:1;
    ULONG Rotation:4;
    ULONG Order:5;
    ULONG Reserved:4;
};

C_ASSERT(sizeof(HubAcpiPld) == 16);

class HubPort
{
public:
    /** Creates the port object and its resources under the hub FDO. */
    _IRQL_requires_(PASSIVE_LEVEL)
    static
    HubPort*
    Create(
        _In_ HubFdo* Hub,
        _In_ const HubPortInfo* Info);

    static
    HubPort*
    FromObject(
        _In_ WDFOBJECT Object);

    VOID
    Post(
        _In_ PortEvent Event)
    {
        m_Machine.SmPost(Event);
    }

    BOOLEAN
    HasMuxFlag(
        _In_ PortMuxFlag Flag) const
    {
        return (m_MuxFlags & (LONG)Flag) != 0;
    }

    VOID
    SetMuxFlag(
        _In_ PortMuxFlag Flag)
    {
        InterlockedOr(&m_MuxFlags, (LONG)Flag);
    }

    VOID
    ClearMuxFlag(
        _In_ PortMuxFlag Flag)
    {
        InterlockedAnd(&m_MuxFlags, ~(LONG)Flag);
    }

    BOOLEAN
    HasFlag(
        _In_ PortFlag Flag) const
    {
        return (m_Flags & (LONG)Flag) != 0;
    }

    VOID
    SetFlag(
        _In_ PortFlag Flag)
    {
        InterlockedOr(&m_Flags, (LONG)Flag);
    }

    VOID
    ClearFlag(
        _In_ PortFlag Flag)
    {
        InterlockedAnd(&m_Flags, ~(LONG)Flag);
    }

    BOOLEAN
    HasProperty(
        _In_ PortProperty Property) const
    {
        return (m_Info.Properties & (ULONG)Property) != 0;
    }

    USHORT
    Number() const
    {
        return m_Info.PortNumber;
    }

    BOOLEAN
    IsUsb30() const
    {
        return m_Info.Protocol == 0x300;
    }

public:
    PortMachine m_Machine;

    WDFOBJECT m_Object;
    HubFdo* m_Hub;
    HubPortInfo m_Info;
    LIST_ENTRY m_HubLink;

    volatile LONG m_MuxFlags;
    volatile LONG m_Flags;

    /* Current device on the port, or NULL */
    HubChild* m_Child;

    HubTimer m_Timer;
    WDFTIMER m_ResetPollTimer;
    PUCXHUB_WORKITEM m_WorkItem;
    HubControlRequest m_Control;

    /* Last GET_STATUS: status and change words, then the USB 3 extended status */
    HubPortStatus m_Current;
    USHORT m_PreviousStatus;
    USHORT m_ChangeAccumulator;
    USHORT m_SelectedFeature;
    PortEvent m_LastChange;
    ULONG m_LinkErrorCount;
    UCHAR m_OverCurrentCount;
    LARGE_INTEGER m_LastOverCurrentTime;
    BOOLEAN m_InitialConnectPowerReference;

    USB_CONNECTION_STATUS m_ConnectionStatus;
    ULONG m_D3ColdReconnectTimeout;
    ULONG m_TotalResets;

    /* identity */

    /* ACPI description, valid with the AcpiUpcValid and AcpiPldValid properties */
    HubAcpiUpc m_AcpiUpc;
    HubAcpiPld m_AcpiPld;

    /* _DSM interconnect type: 0 standard, 1 HSIC, 2 SSIC. Only traced */
    USHORT m_InterconnectType;

    /* Connector map key; registered while InConnectorMap is set */
    HubConnectorId m_ConnectorId;

    /* Port number a serial less device on a Type-C connector without a switch is named by */
    USHORT m_InstancePortNumber;

    /* Integrated hub port: root USB 3 ports its child hub maps onto */
    USHORT m_FirstCompanionPort;
    USHORT m_LastCompanionPort;

    /* USB4 */

    /* Escaped name of the USB4 host router the port belongs to, with PortFlag::Usb4Host */
    WDFSTRING m_Usb4HostName;

private:
    static
    VOID
    NTAPI
    TimerFired(
        _In_ PVOID Context);

    static EVT_WDF_TIMER ResetPollFired;
    static EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtCleanup;
    static EVT_WDF_OBJECT_CONTEXT_DESTROY EvtDestroy;
    static UCXHUB_WORKITEM_ROUTINE MachineWorkItem;

    friend class PortMachine;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HubPort, HubGetPortContext);
