/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Vendor-specific quirks
 * COPYRIGHT:       Copyright 2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#include "private.h"

#define NDEBUG
#include <debug.h>

// The following code is based on the Linux HdAduio quirks implementation

static ULONG Stac92HD83XXXPowerNodes[7] =
{
    0x0a, 0x0b, 0x0c, 0xd, 0x0e, 0x0f, 0x10
};

static
NTSTATUS
StacTogglePowerMap(
    PVOID Node,
    ULONG NodeId,
    ULONG NodeCount,
    PULONG TargetWidgets,
    PULONG PoweMapBits,
    BOOL Enable,
    BOOL DoWrite)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG NodeIndex;

    for (NodeIndex = 0; NodeIndex < NodeCount; NodeIndex++)
    {
        if (TargetWidgets[NodeIndex] == NodeId)
            break;
    }

	if (NodeIndex >= NodeCount)
	{
        DPRINT1("Node %u is out of range\n", TargetWidgets[NodeIndex]);
        return STATUS_UNSUCCESSFUL;
    }

    NodeIndex = 1 << NodeIndex;

    ULONG Value = *PoweMapBits;
    if (Enable)
        Value &= ~NodeIndex;
    else
        Value |= NodeIndex;

    if (Value != *PoweMapBits)
    {
        *PoweMapBits = Value;
        if (DoWrite)
        {
            Status = OutNode->SetCustomVerb(0x01,
                                            AC_VERB_IDT_SET_POWER_MAP,
                                            Value);
            DPRINT1("HDAUDIO: SetCustomVerb Status %x Node %u Value %x\n", Status, 0x01, Value);
        }
    }
    return Status;
}

static
NTSTATUS
JackUpdatePower(
    PVOID Node,
    ULONG NodeId,
    ULONG NodeCount,
    PULONG TargetWidgets,
    PULONG PowerMapBits)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    NTSTATUS Status;

    for (ULONG NodeIndex = 0;
         NodeIndex < NodeCount;
         NodeIndex++)
    {
        ULONG NodeId = TargetWidgets[NodeIndex];
        Status = StacTogglePowerMap(Node, NodeId, NodeCount, TargetWidgets, PowerMapBits, TRUE, TRUE);
        DPRINT1("HDAUDIO: StacTogglePowerMap Status %x Node %u PowerMapBits %x\n", Status, TargetWidgets[NodeIndex], *PowerMapBits);
    }

    Status = OutNode->SetCustomVerb(0x01,
                                    AC_VERB_IDT_SET_POWER_MAP,
                                    *PowerMapBits);
    DPRINT1("HDAUDIO: SetCustomVerb Status %x Node %u PowerMapBits %x\n", Status, 0x01, *PowerMapBits);
    return Status;
}

static
NTSTATUS
StacInitPowerMap(
    PVOID Node,
    ULONG NodeCount,
    PULONG TargetWidgets,
    PULONG PowerMapBits)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    NTSTATUS Status;

    for (ULONG NodeIndex = 0;
         NodeIndex < NodeCount;
         NodeIndex++)
    {
        ULONG NodeId = TargetWidgets[NodeIndex];

        PIN_CONFIGURATION_DEFAULT PinConfiguration;
        Status = OutNode->GetPinConfigurationDefault(NodeId, &PinConfiguration);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HDAUDIO: GetPinConfigurationDefault failed with %x, Node %u\n", Status, NodeId);
            continue;
        }

        if (PinConfiguration.PortConnectivity == 0x0) // Jack
        {
            Status = JackUpdatePower((PVOID)OutNode, NodeId, NodeCount, TargetWidgets, PowerMapBits);
	    }
	    else // Other
	    {
            Status = StacTogglePowerMap((PVOID)OutNode,
                                        NodeId,
                                        NodeCount,
                                        TargetWidgets,
                                        PowerMapBits,
                                        PinConfiguration.PortConnectivity != 0x1, // Unknown / disconnected
                                        FALSE);
        }
    }
    return Status;
}

static
NTSTATUS
StacGPIOSetup(
    PVOID Node,
    ULONG GPIOData,
    ULONG GPIOEnableMask,
    ULONG GPIODirection)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    ULONG Data, EnableMask, Direction;
    NTSTATUS Status;

    Status = OutNode->GetGPIO(0x01, &Data, &EnableMask, &Direction);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetGPIO failed with %x Node %u\n", Status, 0x01);
        return Status;
    }

    Data = (Data & ~GPIODirection) | (GPIOData & GPIODirection);
    EnableMask |= GPIOEnableMask;
    Direction |= GPIODirection;

    // Configure GPIOx as CMOS
    Status = OutNode->SetCustomVerb(0x01, 0x7e7, 0x0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetCustomVerb failed with %x Node %u\n", Status, 0x01);
        return Status;
    }

    Status = OutNode->SetGPIO(0x01, 1000, Data, EnableMask, Direction);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetGPIO failed with %x Node %u\n", Status, 0x01);
        return Status;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
AlcInitializeNodes(
    PVOID Node,
    ULONG NodeCount,
    PULONG Nodes)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    NTSTATUS Status;

    for (ULONG NodeIndex = 0; NodeIndex < NodeCount; NodeIndex++)
    {
        // Disable unsolicited responses
        Status = OutNode->EnableUnsolicitedResponse(Nodes[NodeIndex], 0x00);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HDAUDIO: EnableUnsolicitedResponse failed %x node %u\n", Status, Nodes[NodeIndex]);
            continue;
        }

        // Set format
        Status = OutNode->SetStreamFormat(Nodes[NodeIndex], 0x11);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HDAUDIO: SetStreamFormat failed %x node %u\n", Status, Nodes[NodeIndex]);
            continue;
        }

        // Turn on the power
        Status = OutNode->SetPowerState(Nodes[NodeIndex], TRUE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HDAUDIO: SetPowerState failed %x node %u\n", Status, Nodes[NodeIndex]);
            continue;
        }
    }
    return Status;
}

static
NTSTATUS
AlcInitializePinOutput(
    PVOID Node,
    ULONG NodeId)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;

    PIN_CAPABILITIES PinCapabilities;
    NTSTATUS Status = OutNode->GetPinCapabilities(NodeId, &PinCapabilities);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetPinCapabilities failed with %x node %u\n", Status, NodeId);
        return Status;
    }

    Status = OutNode->SetPinWidgetControl(NodeId,
                                          PinCapabilities.HeadphoneDriveCapable,
                                          TRUE,
                                          FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetPinWidgetControl failed with %x node %u\n", Status, NodeId);
    }

    return Status;
}

static
NTSTATUS
AlcSetOutputAmpifier(
    PVOID Node,
    ULONG NodeId,
    BOOL Mute,
    UCHAR Gain)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;

    AMPLIFIER_GAIN_MUTE_SET Set;
    Set.Input = 0x0;
    Set.Output = 0x1;
    Set.Left = 0x1;
    Set.Right = 0x0;
    Set.Index = 0;
    Set.Mute = Mute;
    Set.Gain = Gain;

    NTSTATUS Status = OutNode->SetAmplifierGainMute(NodeId, &Set);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetAmplifierGainMute failed with %x node %u\n", Status, NodeId);
        return Status;
    }

    Set.Input = 0x0;
    Set.Output = 0x1;
    Set.Left = 0x0;
    Set.Right = 0x1;
    Set.Index = 0;
    Set.Mute = Mute;
    Set.Gain = Gain;

    Status = OutNode->SetAmplifierGainMute(NodeId, &Set);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetAmplifierGainMute failed with %x node %u\n", Status, NodeId);
    }
    return Status;
}

NTSTATUS
NTAPI
CAdapterCommon::ExecuteVendorSpecificQuirks(
    IN PVOID Node)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    NTSTATUS Status;

    // TODO: handle more vendors and devices
    switch (VendorId)
    {
        case 0x111d: // IDT
        {
            switch (DeviceId)
            {
                // STAC92HD83XXX
                case 0x7604: // 92HD83C1X5
                case 0x7605: // 92HD81B1X5
                case 0x7666: // 92HD88B3
                case 0x7667: // 92HD88B1
                case 0x7668: // 92HD88B2
                case 0x7669: // 92HD88B4
                case 0x76d1: // 92HD87B1/3
                case 0x76d4: // 92HD83C1C5
                case 0x76d5: // 92HD81B1C5
                case 0x76d9: // 92HD87B2/4
                {
                    // Power on EAPD amplifiers
                    Status = OutNode->SetEAPDPower(1 << 2);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: SetEAPDPower failed %x\n", Status);
                        break;
                    }

                    Status = OutNode->SetCustomVerb(0x22, 0x785, 0x43);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: SetCustomVerb failed %x\n", Status);
                        break;
                    }
                    Status = OutNode->SetCustomVerb(0x22, 0x782, 0xe0);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: SetCustomVerb failed %x\n", Status);
                        break;
                    }
                    Status = OutNode->SetCustomVerb(0x22, 0x795, 0x00);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: SetCustomVerb failed %x\n", Status);
                        break;
                    }

                    // Toggle GPIO registers
                    Status = StacGPIOSetup((PVOID)OutNode, 0x1, 0x1, 0x1);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: StacGPIOSetup failed %x\n", Status);
                        break;
                    }

                    // Init power map
                    ULONG PowerMapBits = 0;
                    Status = StacInitPowerMap((PVOID)OutNode, SIZEOF_ARRAY(Stac92HD83XXXPowerNodes), Stac92HD83XXXPowerNodes, &PowerMapBits);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: StacInitPowerMap failed %x\n", Status);
                        break;
                    }

                    // Sync power map
                    DPRINT1("HDAUDIO: PowerMapBits %x\n", PowerMapBits);
                    Status = OutNode->SetCustomVerb(m_FunctionGroupStartNode, AC_VERB_IDT_SET_POWER_MAP, PowerMapBits);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: SetCustomVerb failed %x\n", Status);
                        break;
                    }

                    ULONG NodeCount = 0;
                    PULONG Nodes = NULL;
                    Status = OutNode->GetNodesWithType(0x00, &NodeCount, &Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: GetNodesWithType failed %x\n", Status);
                        break;
                    }

                    Status = AlcInitializeNodes((PVOID)OutNode, NodeCount, Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcInitializeNodes failed %x\n", Status);
                        if (Nodes)
                            ExFreePool(Nodes);
                        break;
                    }

                    if (Nodes)
                        ExFreePool(Nodes);
                    Status = OutNode->GetNodesWithType(0x01, &NodeCount, &Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: GetNodesWithType failed %x\n", Status);
                        break;
                    }

                    Status = AlcInitializeNodes((PVOID)OutNode, NodeCount, Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcInitializeNodes failed %x\n", Status);
                        if (Nodes)
                            ExFreePool(Nodes);
                        break;
                    }

                    if (Nodes)
                        ExFreePool(Nodes);
                    Status = OutNode->GetNodesWithType(0x02, &NodeCount, &Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: GetNodesWithType failed %x\n", Status);
                        break;
                    }

                    Status = AlcInitializeNodes((PVOID)OutNode, NodeCount, Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcInitializeNodes failed %x\n", Status);
                        if (Nodes)
                            ExFreePool(Nodes);
                        break;
                    }

                    if (Nodes)
                        ExFreePool(Nodes);
                    Status = OutNode->GetNodesWithType(0x03, &NodeCount, &Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: GetNodesWithType failed %x\n", Status);
                        break;
                    }

                    Status = AlcInitializeNodes((PVOID)OutNode, NodeCount, Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcInitializeNodes failed %x\n", Status);
                        if (Nodes)
                            ExFreePool(Nodes);
                        break;
                    }

                    if (Nodes)
                        ExFreePool(Nodes);
                    Status = OutNode->GetNodesWithType(0x04, &NodeCount, &Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: GetNodesWithType failed %x\n", Status);
                        break;
                    }

                    Status = AlcInitializeNodes((PVOID)OutNode, NodeCount, Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcInitializeNodes failed %x\n", Status);
                        if (Nodes)
                            ExFreePool(Nodes);
                        break;
                    }

                    if (Nodes)
                        ExFreePool(Nodes);
                    Status = OutNode->GetNodesWithType(0x05, &NodeCount, &Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: GetNodesWithType failed %x\n", Status);
                        break;
                    }

                    Status = AlcInitializeNodes((PVOID)OutNode, NodeCount, Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcInitializeNodes failed %x\n", Status);
                        if (Nodes)
                            ExFreePool(Nodes);
                        break;
                    }

                    if (Nodes)
                        ExFreePool(Nodes);
                    Status = OutNode->GetNodesWithType(0x0F, &NodeCount, &Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: GetNodesWithType failed %x\n", Status);
                        break;
                    }

                    if (NodeCount)
                    {
                        Status = AlcInitializeNodes((PVOID)OutNode, NodeCount, Nodes);
                        if (!NT_SUCCESS(Status))
                        {
                            DPRINT1("HDAUDIO: AlcInitializeNodes failed %x\n", Status);
                            if (Nodes)
                                ExFreePool(Nodes);
                            break;
                        }
                    }

                    if (Nodes)
                        ExFreePool(Nodes);
                    break;
                }

                default:
                    DPRINT1("HDAUDIO: unhanlded DeviceId 0x%x\n", DeviceId);
                    Status = STATUS_NOT_SUPPORTED;
                    break;
            }
            break;
        }

        case 0x10ec: // Realtek
        {
            switch (DeviceId)
            {
                // ALCXXX
                case 0x0660: // ALC660
                case 0x0662: // ALC662
                {
                    // Toggle GPIO registers
                    Status = OutNode->SetGPIO(m_FunctionGroupStartNode, 0, 0x03, 0x03, 0x03);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: SetGPIO failed %x\n", Status);
                        break;
                    }
                    KeStallExecutionProcessor(100000);
                    Status = OutNode->SetGPIO(m_FunctionGroupStartNode, 0, 0x00, 0x03, 0x03);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: SetGPIO failed %x\n", Status);
                        break;
                    }

                    // Avoid D3 to keep GPIO up
                    Status = OutNode->SetPowerState(m_FunctionGroupStartNode, TRUE);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: SetPowerState failed %x\n", Status);
                        break;
                    }

                    // Disable boost for Mic-in
                    AMPLIFIER_GAIN_MUTE_SET Set;
                    Set.Input = 0x1;
                    Set.Output = 0x0;
                    Set.Left = 0x1;
                    Set.Right = 0x1;
                    Set.Index = 0;
                    Set.Mute = 0x0;
                    Set.Gain = 0x70;
                    Status = OutNode->SetAmplifierGainMute(0x1b, &Set);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: SetAmplifierGainMute failed %x\n", Status);
                        break;
                    }

                    ULONG NodeCount = 0;
                    PULONG Nodes = NULL;
                    Status = OutNode->GetNodesWithType(0x00, &NodeCount, &Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: GetNodesWithType failed %x\n", Status);
                        break;
                    }

                    Status = AlcInitializeNodes((PVOID)OutNode, NodeCount, Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcInitializeNodes failed %x\n", Status);
                        if (Nodes)
                            ExFreePool(Nodes);
                        break;
                    }

                    if (Nodes)
                        ExFreePool(Nodes);
                    Status = OutNode->GetNodesWithType(0x01, &NodeCount, &Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: GetNodesWithType failed %x\n", Status);
                        break;
                    }

                    Status = AlcInitializeNodes((PVOID)OutNode, NodeCount, Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcInitializeNodes failed %x\n", Status);
                        if (Nodes)
                            ExFreePool(Nodes);
                        break;
                    }

                    if (Nodes)
                        ExFreePool(Nodes);
                    Status = OutNode->GetNodesWithType(0x02, &NodeCount, &Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: GetNodesWithType failed %x\n", Status);
                        break;
                    }

                    Status = AlcInitializeNodes((PVOID)OutNode, NodeCount, Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcInitializeNodes failed %x\n", Status);
                        if (Nodes)
                            ExFreePool(Nodes);
                        break;
                    }

                    if (Nodes)
                        ExFreePool(Nodes);
                    Status = OutNode->GetNodesWithType(0x03, &NodeCount, &Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: GetNodesWithType failed %x\n", Status);
                        break;
                    }

                    Status = AlcInitializeNodes((PVOID)OutNode, NodeCount, Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcInitializeNodes failed %x\n", Status);
                        if (Nodes)
                            ExFreePool(Nodes);
                        break;
                    }

                    if (Nodes)
                        ExFreePool(Nodes);
                    Status = OutNode->GetNodesWithType(0x04, &NodeCount, &Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: GetNodesWithType failed %x\n", Status);
                        break;
                    }

                    Status = AlcInitializeNodes((PVOID)OutNode, NodeCount, Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcInitializeNodes failed %x\n", Status);
                        if (Nodes)
                            ExFreePool(Nodes);
                        break;
                    }

                    if (Nodes)
                        ExFreePool(Nodes);
                    Status = OutNode->GetNodesWithType(0x05, &NodeCount, &Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: GetNodesWithType failed %x\n", Status);
                        break;
                    }

                    Status = AlcInitializeNodes((PVOID)OutNode, NodeCount, Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcInitializeNodes failed %x\n", Status);
                        if (Nodes)
                            ExFreePool(Nodes);
                        break;
                    }

                    if (Nodes)
                        ExFreePool(Nodes);
                    Status = OutNode->GetNodesWithType(0x0F, &NodeCount, &Nodes);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: GetNodesWithType failed %x\n", Status);
                        break;
                    }

                    if (NodeCount)
                    {
                        Status = AlcInitializeNodes((PVOID)OutNode, NodeCount, Nodes);
                        if (!NT_SUCCESS(Status))
                        {
                            DPRINT1("HDAUDIO: AlcInitializeNodes failed %x\n", Status);
                            if (Nodes)
                                ExFreePool(Nodes);
                            break;
                        }
                    }

                    if (Nodes)
                        ExFreePool(Nodes);

                    // Turn on pins 20 and 27
                    Status = AlcInitializePinOutput((PVOID)OutNode, 20);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcInitializePinOutput failed %x\n", Status);
                        break;
                    }

                    Status = AlcInitializePinOutput((PVOID)OutNode, 27);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcInitializePinOutput failed %x\n", Status);
                        break;
                    }

                    // Use 1st connection
                    Status = OutNode->SetConnectionIndex(20, 0x00);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: SetConnectionIndex failed %x\n", Status);
                        break;
                    }

                    Status = OutNode->SetConnectionIndex(27, 0x00);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: SetConnectionIndex failed %x\n", Status);
                        break;
                    }

                    // Enable output amplifier
                    Status = AlcSetOutputAmpifier((PVOID)OutNode, 20, FALSE, 0x40);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcSetOutputAmpifier failed %x\n", Status);
                        break;
                    }

                    Status = AlcSetOutputAmpifier((PVOID)OutNode, 27, FALSE, 0x40);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcSetOutputAmpifier failed %x\n", Status);
                        break;
                    }

                    Status = AlcSetOutputAmpifier((PVOID)OutNode, 2, FALSE, 0x00);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcSetOutputAmpifier failed %x\n", Status);
                        break;
                    }
                    Status = AlcSetOutputAmpifier((PVOID)OutNode, 3, FALSE, 0x00);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcSetOutputAmpifier failed %x\n", Status);
                        break;
                    }
                    Status = AlcSetOutputAmpifier((PVOID)OutNode, 4, FALSE, 0x00);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("HDAUDIO: AlcSetOutputAmpifier failed %x\n", Status);
                    }
                    break;
                }

                default:
                    DPRINT1("HDAUDIO: unhanlded DeviceId 0x%x\n", DeviceId);
                    Status = STATUS_NOT_SUPPORTED;
                    break;
            }
            break;
        }

        default:
            DPRINT1("HDAUDIO: unhanlded VendorId 0x%x\n", VendorId);
            Status = STATUS_NOT_SUPPORTED;
            break;
    }
    return Status;
}
