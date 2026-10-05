/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Driver wide map of physical connectors to their USB 2 and USB 3 ports
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"
#include "hubid.h"

#define NDEBUG
#include <debug.h>

#define HUB_PROTOCOL_USB20          0x200
#define HUB_PROTOCOL_USB30          0x300

/**
 * One physical connector. A Type-C connector without a mux can carry two
 * USB 3 ports, one per orientation.
 */
struct HubConnectorNode
{
    LIST_ENTRY Link;
    HubConnectorId Id;
    HubPort* Usb20;
    HubPort* Usb30;
    HubPort* Usb30Second;
};

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubConnectorLock(VOID)
{
    WdfWaitLockAcquire(HubDriver.CompanionPortLock, NULL);
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubConnectorUnlock(VOID)
{
    WdfWaitLockRelease(HubDriver.CompanionPortLock);
}

/* Field by field, so padding never takes part */
static
BOOLEAN
NTAPI
HubConnectorIdsEqual(
    _In_ const HubConnectorId* First,
    _In_ const HubConnectorId* Second)
{
    ULONG Index;

    if (First->Type != Second->Type ||
        First->Depth != Second->Depth ||
        First->TypeCConnectorId != Second->TypeCConnectorId ||
        RtlCompareMemory(First->Location, Second->Location, sizeof(First->Location)) != sizeof(First->Location))
    {
        return FALSE;
    }

    for (Index = 0; Index < HUB_CONNECTOR_MAX_PATH; Index++)
    {
        if (First->Path[Index] != Second->Path[Index])
            return FALSE;
    }

    return TRUE;
}

/* Caller holds the map lock */
static
HubConnectorNode*
NTAPI
HubConnectorFindById(
    _In_ const HubConnectorId* Id)
{
    PLIST_ENTRY Entry;
    HubConnectorNode* Node;

    for (Entry = HubDriver.ConnectorMap.Flink; Entry != &HubDriver.ConnectorMap; Entry = Entry->Flink)
    {
        Node = CONTAINING_RECORD(Entry, HubConnectorNode, Link);
        if (HubConnectorIdsEqual(&Node->Id, Id))
            return Node;
    }

    return NULL;
}

/* Caller holds the map lock. Matching on the slots keeps a stale key from hiding a port */
static
HubConnectorNode*
NTAPI
HubConnectorFindByPort(
    _In_ HubPort* Port)
{
    PLIST_ENTRY Entry;
    HubConnectorNode* Node;

    if (!Port->HasProperty(PortProperty::InConnectorMap))
        return NULL;

    for (Entry = HubDriver.ConnectorMap.Flink; Entry != &HubDriver.ConnectorMap; Entry = Entry->Flink)
    {
        Node = CONTAINING_RECORD(Entry, HubConnectorNode, Link);
        if (Node->Usb20 == Port || Node->Usb30 == Port || Node->Usb30Second == Port)
            return Node;
    }

    return NULL;
}

/* REGISTRATION **************************************************************/

/* A Type-C connector without a mux names a serial less device by a port number */
static
NTSTATUS
NTAPI
HubConnectorAddUsb20(
    _Inout_ HubConnectorNode* Node,
    _In_ HubPort* Port)
{
    BOOLEAN TypeC = Port->HasProperty(PortProperty::TypeCWithoutSwitch);

    if (Node->Usb20 != NULL)
    {
        DPRINT1("Port %u: connector already has a USB 2 port (platform compliance)\n", Port->Number());
        return STATUS_UNSUCCESSFUL;
    }

    if (TypeC && Node->Usb30 != NULL && !Node->Usb30->HasProperty(PortProperty::TypeCWithoutSwitch))
    {
        DPRINT1("Port %u: Type-C USB 2 port paired with a different USB 3 connector type (platform compliance)\n",
                Port->Number());
        return STATUS_UNSUCCESSFUL;
    }

    if (TypeC)
        Port->m_InstancePortNumber = Port->Number();

    Node->Usb20 = Port;
    return STATUS_SUCCESS;
}

/* Both orientations of a Type-C connector share the lower port number */
static
NTSTATUS
NTAPI
HubConnectorAddUsb30(
    _Inout_ HubConnectorNode* Node,
    _In_ HubPort* Port)
{
    BOOLEAN TypeC = Port->HasProperty(PortProperty::TypeCWithoutSwitch);
    USHORT Shared;

    if (Node->Usb30 == NULL)
    {
        if (TypeC && Node->Usb20 != NULL && !Node->Usb20->HasProperty(PortProperty::TypeCWithoutSwitch))
        {
            DPRINT1("Port %u: Type-C USB 3 port paired with a different USB 2 connector type (platform compliance)\n",
                    Port->Number());
            return STATUS_UNSUCCESSFUL;
        }

        if (TypeC)
            Port->m_InstancePortNumber = Port->Number();

        Node->Usb30 = Port;
        return STATUS_SUCCESS;
    }

    if (!TypeC ||
        Node->Usb30Second != NULL ||
        !Node->Usb30->HasProperty(PortProperty::TypeCWithoutSwitch))
    {
        DPRINT1("Port %u: connector has no room for another USB 3 port (platform compliance)\n", Port->Number());
        return STATUS_UNSUCCESSFUL;
    }

    Shared = min(Port->Number(), Node->Usb30->Number());
    Port->m_InstancePortNumber = Shared;
    Node->Usb30->m_InstancePortNumber = Shared;
    Node->Usb30Second = Port;
    return STATUS_SUCCESS;
}

/* A port of unknown protocol succeeds without joining a node; a failed registration may leave an empty node */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubConnectorRegisterPort(
    _In_ HubPort* Port)
{
    HubWaitLockGuard Guard(HubDriver.CompanionPortLock);
    HubConnectorNode* Node;
    NTSTATUS Status;

    Node = HubConnectorFindById(&Port->m_ConnectorId);
    if (Node == NULL)
    {
        Node = (HubConnectorNode*)ExAllocatePoolWithTag(NonPagedPool, sizeof(*Node), HUB_TAG_PORT);
        if (Node == NULL)
        {
            DPRINT1("Port %u: connector node could not be allocated\n", Port->Number());
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(Node, sizeof(*Node));
        Node->Id = Port->m_ConnectorId;
        InsertTailList(&HubDriver.ConnectorMap, &Node->Link);
    }

    switch (Port->m_Info.Protocol)
    {
        case HUB_PROTOCOL_USB20:
            Status = HubConnectorAddUsb20(Node, Port);
            break;

        case HUB_PROTOCOL_USB30:
            Status = HubConnectorAddUsb30(Node, Port);
            break;

        default:
            DPRINT1("Port %u has unknown protocol 0x%x\n", Port->Number(), Port->m_Info.Protocol);
            return STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(Status))
        return Status;

    HubSetPortProperty(Port, PortProperty::InConnectorMap);
    return STATUS_SUCCESS;
}

/* Only the port's own slot is emptied; the node is freed once all three slots are empty */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubConnectorUnregisterPort(
    _In_ HubPort* Port)
{
    HubWaitLockGuard Guard(HubDriver.CompanionPortLock);
    HubConnectorNode* Node;

    if (!Port->HasProperty(PortProperty::InConnectorMap))
        return STATUS_SUCCESS;

    Node = HubConnectorFindByPort(Port);
    HubClearPortProperty(Port, PortProperty::InConnectorMap);

    if (Node == NULL)
    {
        DPRINT1("Port %u: registered port has no connector\n", Port->Number());
        return STATUS_UNSUCCESSFUL;
    }

    if (Node->Usb20 == Port)
    {
        Node->Usb20 = NULL;
    }
    else if (Node->Usb30 == Port)
    {
        Node->Usb30 = Node->Usb30Second;
        Node->Usb30Second = NULL;
    }
    else
    {
        Node->Usb30Second = NULL;
    }

    if (Node->Usb20 == NULL && Node->Usb30 == NULL && Node->Usb30Second == NULL)
    {
        RemoveEntryList(&Node->Link);
        ExFreePoolWithTag(Node, HUB_TAG_PORT);
    }

    return STATUS_SUCCESS;
}

/* LOOKUPS *******************************************************************/

/* Caller holds the map lock; the port returned is only safe to touch while it does */
HubPort*
NTAPI
HubConnectorFindCompanion(
    _In_ HubPort* Port,
    _In_ ULONG Index)
{
    HubConnectorNode* Node;

    if (Index > 1)
    {
        DPRINT1("Companion index %lu out of range\n", Index);
        return NULL;
    }

    Node = HubConnectorFindByPort(Port);
    if (Node == NULL)
    {
        if (Port->HasProperty(PortProperty::InConnectorMap))
            DPRINT1("Port %u: registered port has no connector\n", Port->Number());

        return NULL;
    }

    if (!Port->IsUsb30())
        return (Index == 0) ? Node->Usb30 : Node->Usb30Second;

    if (Index == 0)
        return Node->Usb20;

    return (Node->Usb30 == Port) ? Node->Usb30Second : Node->Usb30;
}

/* Only an identity value: the node may be gone by the time the caller looks */
PVOID
NTAPI
HubConnectorNodeForPort(
    _In_ HubPort* Port)
{
    HubConnectorNode* Node;

    if (!Port->HasProperty(PortProperty::InConnectorMap))
        return NULL;

    HubConnectorLock();
    Node = HubConnectorFindByPort(Port);
    HubConnectorUnlock();

    return Node;
}

/* MAPPING A HUB *************************************************************/

/** Root port key without ACPI: the root hub number and the USB 3 port number. */
static
VOID
NTAPI
HubConnectorSetRootId(
    _In_ HubFdo* Hub,
    _Inout_ HubPort* Port,
    _In_ ULONG Usb30Port)
{
    RtlZeroMemory(&Port->m_ConnectorId, sizeof(Port->m_ConnectorId));
    Port->m_ConnectorId.Type = HUB_CONNECTOR_ROOT;
    RtlCopyMemory(Port->m_ConnectorId.Location, &Hub->m_HubNumber, sizeof(Hub->m_HubNumber));
    Port->m_ConnectorId.Path[0] = Usb30Port;
}

/* The parent hub port's key; a hub hanging off an unmapped port gets an all zero key */
static
VOID
NTAPI
HubConnectorCopyParentId(
    _In_ HubFdo* Hub,
    _Out_ HubConnectorId* Id)
{
    const HubConnectorId* Parent = (const HubConnectorId*)Hub->m_Parent.ConnectorId;

    if (Parent != NULL)
        *Id = *Parent;
    else
        RtlZeroMemory(Id, sizeof(*Id));
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubConnectorMapAcpiPorts(
    _In_ HubFdo* Hub)
{
    PLIST_ENTRY Entry;
    HubPort* Port;
    HubConnectorId* Id;
    NTSTATUS Status;

    for (Entry = Hub->m_Mux.Ports.Flink; Entry != &Hub->m_Mux.Ports; Entry = Entry->Flink)
    {
        Port = CONTAINING_RECORD(Entry, HubPort, m_HubLink);
        if (!Port->HasProperty(PortProperty::AcpiPldValid) || !Port->HasProperty(PortProperty::Removable))
            continue;

        Id = &Port->m_ConnectorId;
        RtlZeroMemory(Id, sizeof(*Id));
        Id->Type = HUB_CONNECTOR_ACPI;
        RtlCopyMemory(Id->Location, &Port->m_AcpiPld, sizeof(Id->Location));
        Id->TypeCConnectorId = ((ULONG64)Port->m_AcpiPld.GroupToken << 8) | Port->m_AcpiPld.GroupPosition;

        Status = HubConnectorRegisterPort(Port);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    return STATUS_SUCCESS;
}

/* The n-th USB 2 root port pairs with the n-th USB 3 one; an integrated hub on the last USB 2 port takes the rest */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubConnectorMapRootPorts(
    _In_ HubFdo* Hub)
{
    ULONG Port20;
    ULONG Port30;
    ULONG Mapped;
    HubPort* Usb20;
    HubPort* Usb30;

    for (Port20 = Hub->m_First20Port, Port30 = Hub->m_First30Port;
         Port20 <= Hub->m_Last20Port && Port30 <= Hub->m_Last30Port;
         Port20++, Port30++)
    {
        Usb20 = Hub->FindPort(Port20);
        if (Usb20 == NULL)
            continue;

        if (Usb20->HasProperty(PortProperty::IntegratedHub))
        {
            if (Port20 != Hub->m_Last20Port)
            {
                DPRINT1("Hub %p: integrated hub on port %lu is not the last USB 2 port (platform compliance)\n",
                        Hub, Port20);
                return;
            }

            /* The integrated hub port itself keeps no path */
            Usb20->m_FirstCompanionPort = (USHORT)Port30;
            Usb20->m_LastCompanionPort = Hub->m_Last30Port;
            HubConnectorSetRootId(Hub, Usb20, 0);

            for (Mapped = Port30; Mapped <= Hub->m_Last30Port; Mapped++)
            {
                Usb30 = Hub->FindPort(Mapped);
                if (Usb30 == NULL)
                    continue;

                HubConnectorSetRootId(Hub, Usb30, Mapped);
                if (!NT_SUCCESS(HubConnectorRegisterPort(Usb30)))
                    return;
            }

            return;
        }

        HubConnectorSetRootId(Hub, Usb20, Port30);

        /* A fixed USB 2 port leaves its USB 3 partner unmapped too */
        if (!Usb20->HasProperty(PortProperty::Removable))
            continue;

        if (!NT_SUCCESS(HubConnectorRegisterPort(Usb20)))
            return;

        Usb30 = Hub->FindPort(Port30);
        if (Usb30 == NULL)
        {
            DPRINT1("Hub %p: no USB 3 partner %lu for port %lu (platform compliance)\n", Hub, Port30, Port20);
            continue;
        }

        HubConnectorSetRootId(Hub, Usb30, Port30);
        if (!Usb30->HasProperty(PortProperty::Removable))
            DPRINT1("Hub %p: USB 3 port %lu is fixed but its partner is not (platform compliance)\n", Hub, Port30);

        if (!NT_SUCCESS(HubConnectorRegisterPort(Usb30)))
            return;
    }
}

/* Ports past the mapped range keep an all zero key */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubConnectorMapIntegratedHubPorts(
    _In_ HubFdo* Hub)
{
    ULONG Number;
    ULONG Port30;
    HubPort* Port;

    for (Number = 1, Port30 = Hub->m_Parent.FirstCompanionPort;
         Number <= Hub->m_PortCount && Port30 <= Hub->m_Parent.LastCompanionPort;
         Number++, Port30++)
    {
        Port = Hub->FindPort(Number);
        if (Port == NULL)
            return;

        HubConnectorCopyParentId(Hub, &Port->m_ConnectorId);
        Port->m_ConnectorId.Path[0] = Port30;

        if (Port->HasProperty(PortProperty::Removable) && !NT_SUCCESS(HubConnectorRegisterPort(Port)))
            return;
    }
}

/*
 * Both halves of a USB 3 hub hang off one root connector key, so their
 * downstream ports come out with equal keys and pair up.
 */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubConnectorMapExternalHubPorts(
    _In_ HubFdo* Hub)
{
    HubConnectorId* Id;
    HubPort* Port;
    ULONG Number;

    for (Number = 1; Number <= Hub->m_PortCount; Number++)
    {
        Port = Hub->FindPort(Number);
        if (Port == NULL)
            return;

        Id = &Port->m_ConnectorId;
        HubConnectorCopyParentId(Hub, Id);

        /* USB allows five tiers below the root, so this never runs past Path[5] */
        if (Id->Depth + 1 >= HUB_CONNECTOR_MAX_PATH)
        {
            DPRINT1("Hub %p is nested too deep for the connector map\n", Hub);
            return;
        }

        Id->Depth++;
        Id->Path[Id->Depth] = Number;

        if (Port->HasProperty(PortProperty::Removable) && !NT_SUCCESS(HubConnectorRegisterPort(Port)))
            return;
    }
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubConnectorUnmapPorts(
    _In_ HubFdo* Hub)
{
    ULONG Number;
    HubPort* Port;

    PAGED_CODE();

    for (Number = Hub->m_First20Port; Number <= Hub->m_Last20Port; Number++)
    {
        Port = Hub->FindPort(Number);
        if (Port != NULL)
            HubConnectorUnregisterPort(Port);
    }

    for (Number = Hub->m_First30Port; Number <= Hub->m_Last30Port; Number++)
    {
        Port = Hub->FindPort(Number);
        if (Port != NULL)
            HubConnectorUnregisterPort(Port);
    }
}

/* ACPI keys first; if they do not form a consistent map, fall back to the topology */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubConnectorMapPorts(
    _In_ HubFdo* Hub)
{
    NTSTATUS Status;

    PAGED_CODE();

    if (Hub->HasFlag(HubFlag::InAcpiNamespace))
    {
        Status = HubConnectorMapAcpiPorts(Hub);
        if (NT_SUCCESS(Status))
        {
            DPRINT("Hub %p connectors mapped from ACPI\n", Hub);
            return;
        }

        DPRINT1("Hub %p: ACPI connector map failed 0x%lx, using the topology (platform compliance)\n",
                Hub, Status);
        HubConnectorUnmapPorts(Hub);
        Hub->ClearFlag(HubFlag::InAcpiNamespace);
    }

    if (Hub->IsRootHub())
        HubConnectorMapRootPorts(Hub);
    else if (Hub->m_Parent.FirstCompanionPort != 0)
        HubConnectorMapIntegratedHubPorts(Hub);
    else
        HubConnectorMapExternalHubPorts(Hub);
}
