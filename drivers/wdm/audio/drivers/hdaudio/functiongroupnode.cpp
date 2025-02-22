/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/drivers/hdaudio/functiongroupnode.cpp
 * PURPOSE:         HDAudio Driver
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

#define YDEBUG
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
        DPRINT1(
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
CFunctionGroupNode::AddNode(ULONG NodeId)
{
    NTSTATUS Status;
    UINT32 VerbTemplate = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_PARAMETERS << 8);
    ULONG WidgetCaps = 0;

    Status = m_Adapter->TransferVerb(VerbTemplate | AC_PAR_AUDIO_WIDGET_CAP, &WidgetCaps);
    if (!NT_SUCCESS(Status))
    {
        //DPRINT1("HDAUDIO: Warning failed to get caps for NodeId %x Status %x\n", NodeId, Status);
        return STATUS_UNSUCCESSFUL;
    }
    ASSERT(WidgetCaps != 0);
    // extract node type
    UCHAR NodeType = (WidgetCaps >> 20) & 0xF;
    PrintNodeInfo(NodeType, NodeId);

    UCHAR Digital = (WidgetCaps >> 9) & 0x1;
    if (Digital)
    {
        //DPRINT1("HDAUDIO: NodeId %u is digital\n", NodeId);
    }
    UCHAR ChanCountLSB = (WidgetCaps & 0x1);
    UCHAR ChanCountExt = (WidgetCaps >> 13) & 0x7;
    UCHAR ChannelCount = (ChanCountExt << 1 | ChanCountLSB) + 1; 

    //DPRINT("HDAUDIO: NodeId %u Channels Supported %u\n", NodeId, ChannelCount);

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
    if (HasConnectionList)
    {
        // DPRINT1("HDAUDIO: NodeId %x has ConnectionList %x\n", NodeId, ConnectionList);
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
CFunctionGroupNode::FindNodeId(IN ULONG NodeId)
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
    IN UCHAR DefaultAssociation,
    IN UCHAR Digital,
    OUT PULONG NodeCount,
    OUT PULONG * Nodes)
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
CFunctionGroupNode::GetNodesWithType(IN UCHAR NodeType, OUT PULONG NodeCount, OUT PULONG *NodesAddress)
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
CFunctionGroupNode::GetVolumeCapabilities(
    IN ULONG NodeId,
    IN PUCHAR Delta,
    IN PUCHAR NumSteps)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_PARAMETERS << 8) | AC_PAR_VOL_KNB_CAP;
    Status = m_Adapter->TransferVerb(Verb, &Response);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetPinConfigurationDefault failed with %x\n", Status);
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
CFunctionGroupNode::GetAmplifierDetails(
    IN ULONG NodeId,
    IN ULONG Input,
    OUT PAMPLIFIER_CAPABILITIES Caps)
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
CFunctionGroupNode::SetStreamFormat(
    IN ULONG NodeId,
    IN USHORT Format)
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
CFunctionGroupNode::SetConverterStream(
    IN ULONG NodeId,
    IN UCHAR StreamId)
{
    ULONG Verb;
    ULONG Response = 0;
    NTSTATUS Status;

    Verb = (m_CodecAddress << 28) | (NodeId << 20) | (AC_VERB_SET_CHANNEL_STREAMID << 8) | StreamId;
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
CFunctionGroupNode::GetPinConfigurationDefault(IN ULONG NodeId, IN PPIN_CONFIGURATION_DEFAULT PinConfiguration)
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
    PinConfiguration->PortConnectivity = (Response >> 30) & 0x3;
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
    IN ULONG NodeId,
    IN PULONG DevicePresent)
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

    *DevicePresent = (Response >> 31) & 0x1;
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
CFunctionGroupNode::GetPinCapabilities(
    IN ULONG NodeId,
    OUT PPIN_CAPABILITIES PinCaps)
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
    PinCaps->PresenceDetectCapable = (Response >> 2) & 0x1;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CFunctionGroupNode::GetSupportedPCMSizeRates(IN ULONG NodeId, OUT PNODE_PCM_RATES OutRates)
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
    OutRates->PCMFormatSupported = (Response & 0x1);
    return STATUS_SUCCESS;
}

VOID
CFunctionGroupNode::PrintNodeInfo(UCHAR NodeType, ULONG NodeId)
{
    switch (NodeType)
    {
        case 0x00:
            DPRINT1("HDAUDIO: Found AudioOutput at NodeId %u\n", NodeId);
            break;
        case 0x01:
            DPRINT1("HDAUDIO: Found AudioInput at NodeId %u\n", NodeId);
            break;
        case 0x02:
            DPRINT1("HDAUDIO: Found AudioMixer at NodeId %u\n", NodeId);
            break;
        case 0x03:
            DPRINT1("HDAUDIO: Found AudioSelector at NodeId %u\n", NodeId);
            break;
        case 0x04:
            DPRINT1("HDAUDIO: Found PinComplex at NodeId %u\n", NodeId);
            break;
        case 0x05:
            DPRINT1("HDAUDIO: Found Power Widget at NodeId %u\n", NodeId);
            break;
        case 0x06:
            DPRINT1("HDAUDIO: Found Volume Knob at NodeId %u\n", NodeId);
            break;
        case 0x07:
            DPRINT1("HDAUDIO: Found Beep Generator at NodeId %u\n", NodeId);
            break;
        case 0x0F:
            DPRINT1("HDAUDIO: Found Vendor Defined Widget at NodeId %u\n", NodeId);
            break;
        default:
            DPRINT1("HDAUDIO: Found Reserved type %x at NodeId %x\n", NodeType, NodeId);
            break;
    }
}

NTSTATUS
HDAUDIO_EnumFunctionGroupWidgets(
    IN UCHAR CodecAddress,
    IN UCHAR FunctionGroupStartNodeId,
    IN CAdapterCommon *Adapter,
    OUT CFunctionGroupNode **OutFunctionGroupNode)
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
        return Status;
    }

    // now enum nodes;
    Status = Node->EnumWidgets();
    if (!NT_SUCCESS(Status))
    {
        // failed to enum widgets
        return Status;
    }
    // now enum connections;
    Status = Node->EnumConnections();
    if (!NT_SUCCESS(Status))
    {
        // failed to enum widget connections
        return Status;
    }

    *OutFunctionGroupNode = Node;
    return STATUS_SUCCESS;
}
