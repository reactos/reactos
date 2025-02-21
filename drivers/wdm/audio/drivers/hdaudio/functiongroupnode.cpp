/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Function group nodes
 * COPYRIGHT:       Copyright 2025-2026 Johannes Anderwald <johannes.anderwald@reactos.org>
 *                  Copyright 2025-2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#include "private.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
CFunctionGroupNode::QueryWidgetCount()
{
    NTSTATUS Status;
    ULONG NodeResponse = 0;
    UINT32 VerbCmd = (m_CodecAddress << 28) | (m_StartNodeId << 20) | (AC_VERB_PARAMETERS << 8) | AC_PAR_NODE_COUNT;
    Status = m_Adapter->TransferVerb(VerbCmd, &NodeResponse);
    if (NT_SUCCESS(Status))
    {
        m_StartSubNode = (NodeResponse >> 16) & 0xFF;
        m_SubNodeCount = NodeResponse & 0xFF;
        DPRINT(
            "HDAUDIO: StartNodeId %x StartSubNode %x WidgetCount %u\n", m_StartNodeId, m_StartSubNode, m_SubNodeCount);
    }
    return Status;
}

NTSTATUS
NTAPI
CFunctionGroupNode::EnumWidgets()
{
    ULONG NodeIndex;
    for (NodeIndex = 0; NodeIndex < m_SubNodeCount; NodeIndex++)
    {
        AddNode(m_StartSubNode + NodeIndex);
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetEAPD(
    _In_ ULONG NodeId,
    _Out_ PUCHAR Value)
{
    NTSTATUS Status;
    ULONG Verb;
    ULONG Response = 0;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_GET_EAPD_BTLENABLE << 8);
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetEAPD failed with %x\n", Status);
        return Status;
    }
    if (Response == 0)
        return STATUS_UNSUCCESSFUL;
    *Value = (Response >> 1) & 0x7F;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::SetEAPD(
    _In_ ULONG NodeId,
    _In_ UCHAR Enable)
{
    NTSTATUS Status;
    ULONG Verb;
    ULONG Response = 0;
    ULONG Value;

    Value = Enable ? 0x02 : 0x00;
    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_SET_EAPD_BTLENABLE << 8) | Value;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetEAPD failed with %x\n", Status);
        return Status;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::SetEAPDPower(
    _In_ UCHAR Value)
{
    NTSTATUS Status;
    ULONG Verb;
    ULONG Response = 0;

    Verb = (m_CodecAddress << 28) | (m_StartNodeId << 20) | (AC_VERB_SET_EAPD << 8) | Value;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetEAPD failed with %x\n", Status);
        return Status;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetGPIO(
    _In_ ULONG NodeId,
    _Out_ PULONG GPIOData,
    _Out_ PULONG GPIOEnableMask,
    _Out_ PULONG GPIODirection)
{
    NTSTATUS Status;
    ULONG Verb;
    ULONG Response = 0;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_GET_GPIO_DATA << 8);
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetGPIO failed with %x\n", Status);
        return Status;
    }
    *GPIOData = Response;
    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_GET_GPIO_MASK << 8);
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetGPIO failed with %x\n", Status);
        return Status;
    }
    *GPIOEnableMask = Response;
    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_GET_GPIO_DIRECTION << 8);
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetGPIO failed with %x\n", Status);
        return Status;
    }
    *GPIODirection = Response;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::SetGPIO(
    _In_ ULONG NodeId,
    _In_ ULONG Delay,
    _In_ ULONG GPIOData,
    _In_ ULONG GPIOEnableMask,
    _In_ ULONG GPIODirection)
{
    NTSTATUS Status;
    ULONG Verb;
    ULONG Response = 0;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_SET_GPIO_MASK << 8) | GPIOEnableMask;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetGPIO failed with %x\n", Status);
        return Status;
    }
    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_SET_GPIO_DIRECTION << 8) | GPIODirection;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetGPIO failed with %x\n", Status);
        return Status;
    }
    if (Delay)
        KeStallExecutionProcessor(Delay);
    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_SET_GPIO_DATA << 8) | GPIOData;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetGPIO failed with %x\n", Status);
        return Status;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::AddNode(ULONG NodeId)
{
    NTSTATUS Status;
    UINT32 VerbTemplate = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_PARAMETERS << 8);
    ULONG WidgetCaps = 0;

    Status = m_Adapter->TransferVerb(VerbTemplate | AC_PAR_AUDIO_WIDGET_CAP, &WidgetCaps);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: Warning failed to get caps for NodeId %x Status %x\n", NodeId, Status);
        return STATUS_UNSUCCESSFUL;
    }
    ASSERT(WidgetCaps != 0);
    // extract node type
    UCHAR NodeType = (WidgetCaps >> 20) & 0xF;
    PrintNodeInfo(NodeType, NodeId);

    UCHAR Digital = (WidgetCaps >> 9) & 0x1;
    if (Digital)
    {
        DPRINT("HDAUDIO: NodeId %u is digital\n", NodeId);
    }
    UCHAR ChanCountLSB = (WidgetCaps & 0x1);
    UCHAR ChanCountExt = (WidgetCaps >> 13) & 0x7;
    UCHAR ChannelCount = (ChanCountExt << 1 | ChanCountLSB) + 1;

    DPRINT("HDAUDIO: NodeId %u Channels Supported %u\n", NodeId, ChannelCount);

    if (NodeType == 0x00)
    {
        UCHAR Stripe = (WidgetCaps >> 5) & 0x1;
        DPRINT("HDAUDIO: Striping support %x\n", Stripe);
        SetStripeBit(Stripe);
    }

    UCHAR UnsolCapable = (WidgetCaps >> 7) & 0x1;
    DPRINT("HDAUDIO: Unsolicited support %x\n", UnsolCapable);

    UCHAR PowerControl = (WidgetCaps >> 10) & 0x1;
    DPRINT("HDAUDIO: Power control support %x\n", PowerControl);

    UCHAR PinDefaultAssociation = 0xFF;
    if (NodeType == 0x04)
    {
        PIN_CONFIGURATION_DEFAULT PinConfigurationDefault;
        Status = GetPinConfigurationDefault(NodeId, &PinConfigurationDefault);
        if (NT_SUCCESS(Status))
        {
            PinDefaultAssociation = PinConfigurationDefault.DefaultAssociation;
        }
    }

    // check connection list
    ULONG HasConnectionList = (WidgetCaps >> 8) & 0x1;

    // get connection list length
    ULONG ConnectionCount = 0;
    ULONG LongFormat = 0;
    DPRINT("HDAUDIO: NodeId %x HasConnectionList %x\n", NodeId, HasConnectionList);
    if (HasConnectionList)
    {
        m_Adapter->TransferVerb(VerbTemplate | AC_PAR_CONNLIST_LEN, &ConnectionCount);
        if (NT_SUCCESS(Status))
        {
            LongFormat = (ConnectionCount & 0x80);
            ConnectionCount = (ConnectionCount & 0x7F);
        }
    }

    PNODE_CONTEXT NodeContext = (PNODE_CONTEXT)ExAllocatePoolZero(
        NonPagedPool, sizeof(NODE_CONTEXT) + (ConnectionCount * sizeof(USHORT)), TAG_HDAUDIO);
    if (!NodeContext)
    {
        // no mem
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    // init node context
    NodeContext->Visited = FALSE;
    NodeContext->NodeId = NodeId;
    NodeContext->Digital = Digital;
    NodeContext->PinDefaultAssociation = PinDefaultAssociation;
    NodeContext->ChannelCount = ChannelCount;
    NodeContext->NodeType = NodeType;
    NodeContext->UnsolCapable = UnsolCapable;
    NodeContext->ConnectionCount = ConnectionCount;

    if (NodeContext->ConnectionCount)
    {
        ULONG ConnectionIndex = 0;
        do
        {
            ULONG ListTemplate = (m_CodecAddress << 28) | (NodeContext->NodeId << 20) | (AC_VERB_GET_CONNECT_LIST << 8);
            ULONG ConnectionEntry = 0;
            Status = m_Adapter->TransferVerb(ListTemplate | ConnectionIndex, &ConnectionEntry);
            if (!LongFormat)
            {
                if (ConnectionIndex < NodeContext->ConnectionCount)
                {
                    NodeContext->Connections[ConnectionIndex] = (ConnectionEntry & 0x7F);
                    ConnectionIndex++;
                }
                if (ConnectionIndex < NodeContext->ConnectionCount)
                {
                    NodeContext->Connections[ConnectionIndex] = (ConnectionEntry >> 8) & 0x7F;
                    ConnectionIndex++;
                }
                if (ConnectionIndex < NodeContext->ConnectionCount)
                {
                    NodeContext->Connections[ConnectionIndex] = (ConnectionEntry >> 16) & 0x7F;
                    ConnectionIndex++;
                }
                if (ConnectionIndex < NodeContext->ConnectionCount)
                {
                    NodeContext->Connections[ConnectionIndex] = (ConnectionEntry >> 24) & 0x7F;
                    ConnectionIndex++;
                }
            }
            else
            {
                if (ConnectionIndex < NodeContext->ConnectionCount)
                {
                    NodeContext->Connections[ConnectionIndex] = (ConnectionEntry & 0x7FFF);
                    ConnectionIndex++;
                }
                if (ConnectionIndex < NodeContext->ConnectionCount)
                {
                    NodeContext->Connections[ConnectionIndex] = (ConnectionEntry >> 16) & 0x7FFF;
                    ConnectionIndex++;
                }
            }

        } while (ConnectionIndex < NodeContext->ConnectionCount);
    }
    InsertTailList(&m_Nodes, &NodeContext->ListEntry);
    return STATUS_SUCCESS;
}

PNODE_CONTEXT
NTAPI
CFunctionGroupNode::FindNodeId(_In_ ULONG NodeId)
{
    PLIST_ENTRY Entry;
    PNODE_CONTEXT NodeContext;

    // enumerate nodes
    Entry = m_Nodes.Flink;
    /* loop all items */
    while (Entry != &m_Nodes)
    {
        /* get node context */
        NodeContext = (PNODE_CONTEXT)CONTAINING_RECORD(Entry, NODE_CONTEXT, ListEntry);
        if (NodeContext->NodeId == NodeId)
        {
            return NodeContext;
        }
        /* get next entry */
        Entry = Entry->Flink;
    }
    return NULL;
}

NTSTATUS
NTAPI
CFunctionGroupNode::EnumConnections()
{
    PLIST_ENTRY Entry;
    PNODE_CONTEXT NodeContext;
    if (IsListEmpty(&m_Nodes))
    {
        return STATUS_SUCCESS;
    }

    ULONG NeedRevisit = FALSE;
    do
    {
        // clear revisit flag
        NeedRevisit = FALSE;

        // enumerate nodes
        Entry = m_Nodes.Flink;

        /* loop all items */
        while (Entry != &m_Nodes)
        {
            /* get node context */
            NodeContext = (PNODE_CONTEXT)CONTAINING_RECORD(Entry, NODE_CONTEXT, ListEntry);

            if (!NodeContext->Visited)
            {
                // set visited flag
                NodeContext->Visited = TRUE;
                // enum connections
                for (ULONG NodeConnectionIndex = 0; NodeConnectionIndex < NodeContext->ConnectionCount;
                     NodeConnectionIndex++)
                {
                    ULONG NodeId = NodeContext->Connections[NodeConnectionIndex];
                    // find entry
                    PNODE_CONTEXT SubNode = FindNodeId(NodeId);
                    if (!SubNode)
                    {
                        AddNode(NodeId);
                        NeedRevisit = TRUE;
                    }
                }
                if (NeedRevisit)
                    break;
            }

            /* get next entry */
            Entry = Entry->Flink;
        }
    } while (NeedRevisit);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetPinNodesWithDefaultAssociation(
    _In_ UCHAR DefaultAssociation,
    _In_ UCHAR Digital,
    _Out_ PULONG NodeCount,
    _Out_ PULONG * Nodes)
{
    ULONG NodeTypeCount = 0;
    PULONG ResultNode;
    PLIST_ENTRY Entry;
    PNODE_CONTEXT NodeContext;
    if (IsListEmpty(&m_Nodes))
    {
        *NodeCount = 0;
        *Nodes = NULL;
        return STATUS_SUCCESS;
    }

    // enumerate nodes
    Entry = m_Nodes.Flink;
    /* loop all items */
    while (Entry != &m_Nodes)
    {
        /* get node context */
        NodeContext = (PNODE_CONTEXT)CONTAINING_RECORD(Entry, NODE_CONTEXT, ListEntry);

        if (NodeContext->NodeType == 0x04 && NodeContext->PinDefaultAssociation == DefaultAssociation &&
            NodeContext->Digital == Digital)
        {
            NodeTypeCount++;
        }

        /* get next entry */
        Entry = Entry->Flink;
    }

    *NodeCount = NodeTypeCount;
    if (NodeTypeCount != 0)
    {
        ResultNode = (PULONG)ExAllocatePoolZero(NonPagedPool, sizeof(ULONG) * NodeTypeCount, TAG_HDAUDIO);
        if (!ResultNode)
        {
            // no memory
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        NodeTypeCount = 0;

        // enumerate nodes
        Entry = m_Nodes.Flink;
        /* loop all items */
        while (Entry != &m_Nodes)
        {
            /* get node context */
            NodeContext = (PNODE_CONTEXT)CONTAINING_RECORD(Entry, NODE_CONTEXT, ListEntry);

            if (NodeContext->NodeType == 0x04 && NodeContext->PinDefaultAssociation == DefaultAssociation &&
                NodeContext->Digital == Digital)
            {
                ResultNode[NodeTypeCount] = NodeContext->NodeId;
                NodeTypeCount++;
            }

            /* get next entry */
            Entry = Entry->Flink;
        }
        *Nodes = ResultNode;
    }
    else
    {
        *Nodes = NULL;
    }
    return STATUS_SUCCESS;
}

VOID
NTAPI
CFunctionGroupNode::ClearVisitedState()
{
    PLIST_ENTRY Entry;
    PNODE_CONTEXT NodeContext;

    // enumerate nodes
    Entry = m_Nodes.Flink;
    /* loop all items */
    while (Entry != &m_Nodes)
    {
        /* get node context */
        NodeContext = (PNODE_CONTEXT)CONTAINING_RECORD(Entry, NODE_CONTEXT, ListEntry);
        /* clear visited flag */
        NodeContext->Visited = FALSE;

        /* get next entry */
        Entry = Entry->Flink;
    }
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetNodesWithType(_In_ UCHAR NodeType, _Out_ PULONG NodeCount, _Out_ PULONG *NodesAddress)
{
    ULONG NodeTypeCount = 0;
    PULONG ResultNode;
    PLIST_ENTRY Entry;
    PNODE_CONTEXT NodeContext;
    if (IsListEmpty(&m_Nodes))
    {
        *NodeCount = 0;
        *NodesAddress = NULL;
        return STATUS_SUCCESS;
    }

    // enumerate nodes
    Entry = m_Nodes.Flink;
    /* loop all items */
    while (Entry != &m_Nodes)
    {
        /* get node context */
        NodeContext = (PNODE_CONTEXT)CONTAINING_RECORD(Entry, NODE_CONTEXT, ListEntry);

        if (NodeContext->NodeType == NodeType)
        {
            NodeTypeCount++;
        }

        /* get next entry */
        Entry = Entry->Flink;
    }

    *NodeCount = NodeTypeCount;
    if (NodeTypeCount != 0 && NodesAddress)
    {
        ResultNode = (PULONG)ExAllocatePoolZero(NonPagedPool, sizeof(ULONG) * NodeTypeCount, TAG_HDAUDIO);
        if (!ResultNode)
        {
            // no memory
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        NodeTypeCount = 0;

        // enumerate nodes
        Entry = m_Nodes.Flink;
        /* loop all items */
        while (Entry != &m_Nodes)
        {
            /* get node context */
            NodeContext = (PNODE_CONTEXT)CONTAINING_RECORD(Entry, NODE_CONTEXT, ListEntry);

            if (NodeContext->NodeType == NodeType)
            {
                ResultNode[NodeTypeCount] = NodeContext->NodeId;
                NodeTypeCount++;
            }

            /* get next entry */
            Entry = Entry->Flink;
        }
        *NodesAddress = ResultNode;
    }
    else if (NodesAddress)
    {
        *NodesAddress = NULL;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetVolumeKnob(
    _In_ ULONG NodeId,
    _Out_ PUCHAR Direct,
    _Out_ PUCHAR Volume)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_GET_VOLUME_KNOB_CONTROL << 8);
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetVolumeKnob failed with %x\n", Status);
        return Status;
    }
    *Direct = (Response >> 7) & 0x1;
    *Volume = (Response & 0x7F);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::SetVolumeKnob(
    _In_ ULONG NodeId,
    _In_ UCHAR Direct,
    _In_ UCHAR Volume)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_SET_VOLUME_KNOB_CONTROL << 8) | (Direct << 7) | Volume;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetVolumeKnob failed with %x\n", Status);
    }
    return Status;
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetVolumeCapabilities(
    _In_ ULONG NodeId,
    _In_ PUCHAR Delta,
    _In_ PUCHAR NumSteps)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_PARAMETERS << 8) | AC_PAR_VOL_KNB_CAP;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetVolumeCapabilities failed with %x\n", Status);
        return Status;
    }
    if (Response == 0)
        return STATUS_UNSUCCESSFUL;
    *Delta = (Response >> 7) & 0x1;
    *NumSteps = (Response & 0x7F);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetFunctionGroupType(
    _In_ ULONG NodeId,
    _Out_ PUCHAR UnsolCapable,
    _Out_ PUCHAR NodeType)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_PARAMETERS << 8) | AC_PAR_FUNCTION_TYPE;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetFunctionGroupType failed with %x\n", Status);
        return Status;
    }
    if (Response == 0)
        return STATUS_UNSUCCESSFUL;
    *UnsolCapable = (Response >> 8) & 0x1;
    *NodeType = (Response & 0xF);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetAmplifierDetails(
    _In_ ULONG NodeId,
    _In_ ULONG Input,
    _Out_ PAMPLIFIER_CAPABILITIES Caps)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_PARAMETERS << 8);
    if (Input)
    {
        Verb |= AC_PAR_AMP_IN_CAP;
    }
    else
    {
        Verb |= AC_PAR_AMP_OUT_CAP;
    }
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetAmplifierDetails failed with %x\n", Status);
        return Status;
    }
    if (Response == 0)
    {
        // retry with root node
        Verb = (m_CodecAddress << 28) | (m_StartNodeId << 20) | (AC_VERB_PARAMETERS << 8);
        if (Input)
        {
            Verb |= AC_PAR_AMP_IN_CAP;
        }
        else
        {
            Verb |= AC_PAR_AMP_OUT_CAP;
        }
        Status = m_Adapter->TransferVerb(Verb, &Response);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HDAUDIO: GetAmplifierDetails failed with %x\n", Status);
            return Status;
        }
        if (Response == 0)
            return STATUS_UNSUCCESSFUL;
    }

    Caps->MuteCapable = (Response >> 31) & 0x1;
    Caps->Steps = (Response >> 16) & 0x7F;
    Caps->NumSteps = (Response >> 8) & 0x7F;
    Caps->Offset = (Response & 0x7F);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetAmplifierGainMute(
    _In_ ULONG NodeId,
    _In_ UCHAR Input,
    _In_ UCHAR Left,
    _In_ UCHAR Index,
    _Out_ PUCHAR Mute,
    _Out_ PUCHAR Gain)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_GET_AMP_GAIN_MUTE << 8) | (!Input << 15) | (Left << 13) | Index;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetAmplifierGainMute failed with %x\n", Status);
        return Status;
    }
    *Mute = (Response >> 7) & 0x1;
    *Gain = (Response & 0x7F);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::SetAmplifierGainMute(
    _In_ ULONG NodeId,
    _In_ PAMPLIFIER_GAIN_MUTE_SET Set)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_SET_AMP_GAIN_MUTE << 8) |
    (Set->Output << 15) | (Set->Input << 14) | (Set->Left << 13) | (Set->Right << 12) | (Set->Index << 8) | (Set->Mute << 7) | Set->Gain;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetAmplifierGainMute failed with %x\n", Status);
        return Status;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::EnableUnsolicitedResponse(
    _In_ ULONG NodeId,
    _In_ UCHAR Enable)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_SET_UNSOLICITED_ENABLE << 8) | (Enable << 7);
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: EnableUnsolicitedResponse failed with %x\n", Status);
        return Status;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::SetPinWidgetControl(
    _In_ ULONG NodeId,
    _In_ UCHAR HPhnEnable,
    _In_ UCHAR OutEnable,
    _In_ UCHAR InEnable)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_SET_PIN_WIDGET_CONTROL << 8) |
    (HPhnEnable << 7) | (OutEnable << 6) | (InEnable << 5);
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetPinWidgetControl failed with %x\n", Status);
        return Status;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::SetPowerState(
    _In_ ULONG NodeId,
    _In_ UCHAR Enable)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    UCHAR Value = Enable ? AC_PWRST_D0 : AC_PWRST_D3;
    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_SET_POWER_STATE << 8) | Value;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetPowerState failed with %x\n", Status);
        return Status;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::SetStreamFormat(
    _In_ ULONG NodeId,
    _In_ USHORT Format)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_SET_STREAM_FORMAT << 8) | Format;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetStreamFormat failed with %x\n", Status);
        return Status;
    }
    return Status;
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetPinWidgetControlValue(
    _In_ ULONG NodeId,
    _Out_ PUCHAR Value)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_GET_PIN_WIDGET_CONTROL << 8);
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetPinWidgetControlValue failed with %x\n", Status);
        return Status;
    }
    *Value = (UCHAR)(Response & 0xFF);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::SetPinWidgetControlValue(
    _In_ ULONG NodeId,
    _In_ UCHAR Value)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_SET_PIN_WIDGET_CONTROL << 8) | Value;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetPinWidgetControlValue failed with %x\n", Status);
        return Status;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::SetConverterStream(
    _In_ ULONG NodeId,
    _In_ UCHAR StreamId,
    _In_ UCHAR Channel)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    /* the converter takes the channels Channel and Channel + 1 of the stream */
    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_SET_CHANNEL_STREAMID << 8) |
           ((StreamId & 0xF) << AC_CONV_STREAM_SHIFT) | (Channel & AC_CONV_CHANNEL);
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetConverterStream failed with %x\n", Status);
        return Status;
    }
    return Status;
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetConnectionIndex(
    _In_ ULONG NodeId,
    _Out_ PUCHAR Index)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_GET_CONNECT_SEL << 8);
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetConnectionIndex failed with %x\n", Status);
        return Status;
    }
    if (Response == 0)
        return STATUS_UNSUCCESSFUL;
    *Index = (Response & 0x7F);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::SetConnectionIndex(
    _In_ ULONG NodeId,
    _In_ UCHAR Index)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_SET_CONNECT_SEL << 8) | Index;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetConnectionIndex failed with %x\n", Status);
        return Status;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::SetCustomVerb(
    _In_ ULONG NodeId,
    _In_ ULONG VerbId,
    _In_ UCHAR Value)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (VerbId << 8) | Value;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: SetCustomVerb failed with %x\n", Status);
    }
    return Status;
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetPinConfigurationDefault(_In_ ULONG NodeId, _In_ PPIN_CONFIGURATION_DEFAULT PinConfiguration)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_GET_CONFIG_DEFAULT << 8);
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetPinConfigurationDefault failed with %x\n", Status);
        return Status;
    }
    PinConfiguration->PortConnectivity = (Response >> 30) & 0xF;
    PinConfiguration->Location = (Response >> 24) & 0x3F;
    PinConfiguration->DefaultDevice = (Response >> 20) & 0xF;
    PinConfiguration->ConnectionType = (Response >> 16) & 0xF;
    PinConfiguration->Color = (Response >> 12) & 0xF;
    PinConfiguration->Misc = (Response >> 8) & 0xF;
    PinConfiguration->DefaultAssociation = (Response >> 4) & 0xF;
    PinConfiguration->Sequence = (Response & 0xF);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetPinSense(
    _In_ ULONG NodeId,
    _In_ PULONG DevicePresent)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_GET_PIN_SENSE << 8);
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetPinSense failed with %x\n", Status);
        return Status;
    }
    if (Response == 0)
        return STATUS_UNSUCCESSFUL;
    *DevicePresent = (Response >> 31) & 0x1;
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
CFunctionGroupNode::GetPinCapabilities(
    _In_ ULONG NodeId,
    _Out_ PPIN_CAPABILITIES PinCaps)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_PARAMETERS << 8) | AC_PAR_PIN_CAP;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetPinCapabilities failed with %x\n", Status);
        return Status;
    }
    if (Response == 0)
        return STATUS_UNSUCCESSFUL;
    PinCaps->EAPDCapable = (Response >> 16) & 0x1;
    PinCaps->InputCapable = (Response >> 5) & 0x1;
    PinCaps->OutputCapable = (Response >> 4) & 0x1;
    PinCaps->HeadphoneDriveCapable = (Response >> 3) & 0x1;
    PinCaps->PresenceDetectCapable = (Response >> 2) & 0x1;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetSupportedPCMSizeRates(_In_ ULONG NodeId, _Out_ PNODE_PCM_RATES OutRates)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_PARAMETERS << 8) | AC_PAR_PCM;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetSupportedPCMSizeRates failed with %x\n", Status);
        return Status;
    }

    if (Response == 0)
    {
        // last try with AFG
        Verb = (m_CodecAddress << 28) | (m_StartNodeId << 20) | (AC_VERB_PARAMETERS << 8) | AC_PAR_PCM;
        Status = m_Adapter->TransferVerb(Verb, &Response);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HDAUDIO: GetSupportedPCMSizeRates failed with %x\n", Status);
            return Status;
        }
        if (Response == 0)
            return STATUS_UNSUCCESSFUL;
    }

    ULONG BitDepth = (Response >> 16) & 0x1F;

    OutRates->AudioFormatSupported32Bit = (BitDepth & 0x10) ? 1 : 0;
    OutRates->AudioFormatSupported24Bit = (BitDepth & 0x8) ? 1 : 0;
    OutRates->AudioFormatSupported20Bit = (BitDepth & 0x4) ? 1 : 0;
    OutRates->AudioFormatSupported16Bit = (BitDepth & 0x2) ? 1 : 0;
    OutRates->AudioFormatSupported8Bit = (BitDepth & 0x1) ? 1 : 0;

    ULONG Rates = (Response & 0xFFF);

    OutRates->Supported8Khz = (Rates & 0x1) ? 1 : 0;
    OutRates->Supported11Khz = (Rates & 0x2) ? 1 : 0;
    OutRates->Supported16Khz = (Rates & 0x4) ? 1 : 0;
    OutRates->Supported22Khz = (Rates & 0x8) ? 1 : 0;
    OutRates->Supported32Khz = (Rates & 0x10) ? 1 : 0;
    OutRates->Supported44Khz = (Rates & 0x20) ? 1 : 0;
    OutRates->Supported48Khz = (Rates & 0x40) ? 1 : 0;
    OutRates->Supported88Khz = (Rates & 0x80) ? 1 : 0;
    OutRates->Supported96Khz = (Rates & 0x100) ? 1 : 0;
    OutRates->Supported176Khz = (Rates & 0x200) ? 1 : 0;
    OutRates->Supported192Khz = (Rates & 0x400) ? 1 : 0;
    OutRates->Supported384Khz = (Rates & 0x800) ? 1 : 0;

    // now get supported stream formats
    Response = 0;
    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_PARAMETERS << 8) | AC_PAR_STREAM;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetSupportedPCMSizeRates failed with %x\n", Status);
        return Status;
    }
    if (Response == 0)
    {
        // retry with AFG node
        Verb = (m_CodecAddress << 28) | (m_StartNodeId << 20) | (AC_VERB_PARAMETERS << 8) | AC_PAR_STREAM;
        Status = m_Adapter->TransferVerb(Verb, &Response);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HDAUDIO: GetSupportedPCMSizeRates failed with %x\n", Status);
            return Status;
        }
        if (Response == 0)
            return STATUS_UNSUCCESSFUL;
    }
    OutRates->AC3FormatSupported = (Response & 0x4) ? 1 : 0;
    OutRates->Float32FormatSupported = (Response & 0x2) ? 1 : 0;
    OutRates->PCMFormatSupported = (Response & 0x1) ? 1 : 0;
    return STATUS_SUCCESS;
}

VOID
CFunctionGroupNode::PrintNodeInfo(UCHAR NodeType, ULONG NodeId)
{
    switch (NodeType)
    {
        case 0x00:
            DPRINT("HDAUDIO: Found AudioOutput at NodeId %u\n", NodeId);
            break;
        case 0x01:
            DPRINT("HDAUDIO: Found AudioInput at NodeId %u\n", NodeId);
            break;
        case 0x02:
            DPRINT("HDAUDIO: Found AudioMixer at NodeId %u\n", NodeId);
            break;
        case 0x03:
            DPRINT("HDAUDIO: Found AudioSelector at NodeId %u\n", NodeId);
            break;
        case 0x04:
            DPRINT("HDAUDIO: Found PinComplex at NodeId %u\n", NodeId);
            break;
        case 0x05:
            DPRINT("HDAUDIO: Found Power Widget at NodeId %u\n", NodeId);
            break;
        case 0x06:
            DPRINT("HDAUDIO: Found Volume Knob at NodeId %u\n", NodeId);
            break;
        case 0x07:
            DPRINT("HDAUDIO: Found Beep Generator at NodeId %u\n", NodeId);
            break;
        case 0x0F:
            DPRINT("HDAUDIO: Found Vendor Defined Widget at NodeId %u\n", NodeId);
            break;
        default:
            DPRINT("HDAUDIO: Found Reserved type %x at NodeId %x\n", NodeType, NodeId);
            break;
    }
}

NTSTATUS
HDAUDIO_EnumFunctionGroupWidgets(
    _In_ UCHAR CodecAddress,
    _In_ UCHAR FunctionGroupStartNodeId,
    _In_ CAdapterCommon *Adapter,
    _Out_ CFunctionGroupNode **OutFunctionGroupNode)
{
    NTSTATUS Status;
    CFunctionGroupNode *Node;

    Node = new (NonPagedPool, TAG_HDAUDIO) CFunctionGroupNode(CodecAddress, FunctionGroupStartNodeId, Adapter);
    if (!Node)
    {
        // insufficient resources
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Status = Node->QueryWidgetCount();
    if (!NT_SUCCESS(Status))
    {
        // failed to get widget count
        delete Node;
        return Status;
    }

    // now enum nodes;
    Status = Node->EnumWidgets();
    if (!NT_SUCCESS(Status))
    {
        // failed to enum widgets
        delete Node;
        return Status;
    }
    // now enum connections;
    Status = Node->EnumConnections();
    if (!NT_SUCCESS(Status))
    {
        // failed to enum widget connections
        delete Node;
        return Status;
    }

    *OutFunctionGroupNode = Node;
    return STATUS_SUCCESS;
}
