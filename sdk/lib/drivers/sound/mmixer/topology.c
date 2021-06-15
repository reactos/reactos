/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/topology.c
 * PURPOSE:         Topology Handling Functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define YDEBUG
#include <debug.h>

VOID
MMixerPrintTopology(
    PTOPOLOGY Topology)
{
    ULONG Index, SubIndex;

    DPRINT("Num Pins %lu NumNodes %lu\n", Topology->TopologyPinsCount, Topology->TopologyNodesCount);

    for(Index = 0; Index < Topology->TopologyPinsCount; Index++)
    {
        DPRINT("PinId %lu NodesConnectedFromCount %lu NodesConnectedToCount %lu Visited %lu\n", Topology->TopologyPins[Index].PinId,
            Topology->TopologyPins[Index].NodesConnectedFromCount, Topology->TopologyPins[Index].NodesConnectedToCount, Topology->TopologyPins[Index].Visited);

        for(SubIndex = 0; SubIndex < Topology->TopologyPins[Index].NodesConnectedFromCount; SubIndex++)
            DPRINT("NodesConnectedFrom Index %lu NodeId %lu\n", SubIndex, Topology->TopologyPins[Index].NodesConnectedFrom[SubIndex]->NodeIndex);

        for(SubIndex = 0; SubIndex < Topology->TopologyPins[Index].NodesConnectedToCount; SubIndex++)
            DPRINT("NodesConnectedTo Index %lu NodeId %lu\n", SubIndex, Topology->TopologyPins[Index].NodesConnectedTo[SubIndex]->NodeIndex);
    }

    for(Index = 0; Index < Topology->TopologyNodesCount; Index++)
    {
        DPRINT("NodeId %lu NodesConnectedFromCount %lu NodesConnectedToCount %lu Visited %lu PinConnectedFromCount %lu PinConnectedToCount %lu\n", Topology->TopologyNodes[Index].NodeIndex,
            Topology->TopologyNodes[Index].NodeConnectedFromCount, Topology->TopologyNodes[Index].NodeConnectedToCount, Topology->TopologyNodes[Index].Visited,
            Topology->TopologyNodes[Index].PinConnectedFromCount, Topology->TopologyNodes[Index].PinConnectedToCount);
    }


}


MIXER_STATUS
MMixerAllocateTopology(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG NodesCount,
    IN ULONG PinCount,
    OUT PTOPOLOGY * OutTopology)
{
    PTOPOLOGY Topology;

    /* allocate topology */
    Topology = (PTOPOLOGY)MixerContext->Alloc(sizeof(TOPOLOGY));

    if (!Topology)
    {
        /* out of memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* allocate topology pins */
    Topology->TopologyPins = (PPIN) MixerContext->Alloc(sizeof(PIN) * PinCount);

    if (!Topology->TopologyPins)
    {
        /* release memory */
        MixerContext->Free(Topology);

        /* out of memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* allocate topology nodes */
    if (NodesCount)
    {
        Topology->TopologyNodes = (PTOPOLOGY_NODE) MixerContext->Alloc(sizeof(TOPOLOGY_NODE) * NodesCount);

        if (!Topology->TopologyNodes)
        {
            /* release memory */
            MixerContext->Free(Topology->TopologyPins);
            MixerContext->Free(Topology);

            /* out of memory */
            return MM_STATUS_NO_MEMORY;
        }
    }

    /* initialize topology */
    Topology->TopologyPinsCount = PinCount;
    Topology->TopologyNodesCount = NodesCount;

    /* store result */
    *OutTopology = Topology;

    /* done */
    return MM_STATUS_SUCCESS;
}

VOID
MMixerResetTopologyVisitStatus(
    IN OUT PTOPOLOGY Topology)
{
    ULONG Index;

    for(Index = 0; Index < Topology->TopologyNodesCount; Index++)
    {
        /* reset visited status */
        Topology->TopologyNodes[Index].Visited = FALSE;
    }

    for(Index = 0; Index < Topology->TopologyPinsCount; Index++)
    {
        /* reset visited status */
        Topology->TopologyPins[Index].Visited = FALSE;
    }
}

VOID
MMixerInitializeTopologyNodes(
    IN PMIXER_CONTEXT MixerContext,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN OUT PTOPOLOGY Topology)
{
    ULONG Index;
    LPGUID Guids;

    /* sanity check */
    ASSERT(Topology->TopologyNodesCount == NodeTypes->Count);

    /* get topology node types */
    Guids = (LPGUID)(NodeTypes + 1);

    for(Index = 0; Index < Topology->TopologyNodesCount; Index++)
    {
        /* store node connection index */
        Topology->TopologyNodes[Index].NodeIndex = Index;

        /* store topology node type */
        MixerContext->Copy(&Topology->TopologyNodes[Index].NodeType, &Guids[Index], sizeof(GUID));
    }
}

MIXER_STATUS
MMixerAddPinConnection(
    IN PMIXER_CONTEXT MixerContext,
    IN PPIN Pin,
    IN PTOPOLOGY_NODE Node,
    IN ULONG bPinToNode)
{
    ULONG Count;
    PULONG NewPinsIndex, OldPinsIndex;
    PTOPOLOGY_NODE * NewNodes, *OldNodes;

    if (bPinToNode)
    {
        /* get existing count */
        Count = Pin->NodesConnectedToCount;
        OldNodes = Pin->NodesConnectedTo;
    }
    else
    {
        /* get existing count */
        Count = Pin->NodesConnectedFromCount;
        OldNodes = Pin->NodesConnectedFrom;
    }

    /* allocate new nodes array */
    NewNodes = MixerContext->Alloc(sizeof(PTOPOLOGY_NODE) * (Count + 1));

    if (!NewNodes)
    {
        /* out of memory */
        return MM_STATUS_NO_MEMORY;
    }

    if (Count)
    {
        /* copy existing nodes */
        MixerContext->Copy(NewNodes, OldNodes, sizeof(PTOPOLOGY) * Count);

        /* release old nodes array */
        MixerContext->Free(OldNodes);
    }

    /* add new topology node */
    NewNodes[Count] = Node;

    if (bPinToNode)
    {
        /* replace old nodes array */
        Pin->NodesConnectedTo = NewNodes;

        /* increment nodes count */
        Pin->NodesConnectedToCount++;

        /* now enlarge PinConnectedFromCount*/
        Count = Node->PinConnectedFromCount;

        /* connected pin count for node */
        OldPinsIndex = Node->PinConnectedFrom;
    }
    else
    {
        /* replace old nodes array */
        Pin->NodesConnectedFrom = NewNodes;

        /* increment nodes count */
        Pin->NodesConnectedFromCount++;

        /* now enlarge PinConnectedFromCount*/
        Count = Node->PinConnectedToCount;

        /* connected pin count for node */
        OldPinsIndex = Node->PinConnectedTo;
    }

    /* allocate pin connection index */
    NewPinsIndex = MixerContext->Alloc(sizeof(ULONG) * (Count + 1));

    if (!NewPinsIndex)
    {
        /* out of memory */
        return MM_STATUS_NO_MEMORY;
    }

    if (Count)
    {
        /* copy existing nodes */
        MixerContext->Copy(NewPinsIndex, OldPinsIndex, sizeof(ULONG) * Count);

        /* release old nodes array */
        MixerContext->Free(OldPinsIndex);
    }

    /* add new topology node */
    NewPinsIndex[Count] = Pin->PinId;

    if (bPinToNode)
    {
        /* replace old nodes array */
        Node->PinConnectedFrom = NewPinsIndex;

        /* increment pin count */
        Node->PinConnectedFromCount++;
    }
    else
    {
        /* replace old nodes array */
        Node->PinConnectedTo = NewPinsIndex;

        /* increment pin count */
        Node->PinConnectedToCount++;
    }

    /* done */
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerHandleNodeToNodeConnection(
    IN PMIXER_CONTEXT MixerContext,
    IN PKSTOPOLOGY_CONNECTION Connection,
    IN OUT PTOPOLOGY Topology)
{
    PTOPOLOGY_NODE InNode, OutNode;
    PTOPOLOGY_NODE * NewNodes;
    PULONG NewLogicalPinNodeConnectedFrom;
    ULONG Count;
    ULONG LogicalPinId;

    /* sanity checks */
    ASSERT(Topology->TopologyNodesCount > Connection->ToNode);
    ASSERT(Topology->TopologyNodesCount > Connection->FromNode);

    /* get node */
    InNode = &Topology->TopologyNodes[Connection->FromNode];
    OutNode = &Topology->TopologyNodes[Connection->ToNode];

    /* get logical pin node id */
    LogicalPinId = Connection->ToNodePin;

    /* get existing count */
    Count = OutNode->NodeConnectedFromCount;

    /* allocate new nodes array */
    NewNodes = MixerContext->Alloc(sizeof(PTOPOLOGY_NODE) * (Count + 1));

    if (!NewNodes)
    {
        /* out of memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* allocate logical pin nodes array */
    NewLogicalPinNodeConnectedFrom = MixerContext->Alloc((Count + 1) * sizeof(ULONG));
    if (!NewLogicalPinNodeConnectedFrom)
    {
        /* out of memory */
        MixerContext->Free(NewNodes);
        return MM_STATUS_NO_MEMORY;
    }

    if (Count)
    {
        /* copy existing nodes */
        MixerContext->Copy(NewNodes, OutNode->NodeConnectedFrom, sizeof(PTOPOLOGY) * Count);

        /* copy existing logical pin node array */
        MixerContext->Copy(NewLogicalPinNodeConnectedFrom, OutNode->LogicalPinNodeConnectedFrom, sizeof(ULONG) * Count);

        /* release old nodes array */
        MixerContext->Free(OutNode->NodeConnectedFrom);

        /* release old logical pin node array */
        MixerContext->Free(OutNode->LogicalPinNodeConnectedFrom);
    }

    /* add new topology node */
    NewNodes[OutNode->NodeConnectedFromCount] = InNode;

    /* add logical node id */
    NewLogicalPinNodeConnectedFrom[OutNode->NodeConnectedFromCount] = LogicalPinId;

    /* replace old nodes array */
    OutNode->NodeConnectedFrom = NewNodes;

    /* replace old logical pin node array */
    OutNode->LogicalPinNodeConnectedFrom = NewLogicalPinNodeConnectedFrom;

    /* increment nodes count */
    OutNode->NodeConnectedFromCount++;

    /* get existing count */
    Count = InNode->NodeConnectedToCount;

    /* allocate new nodes array */
    NewNodes = MixerContext->Alloc(sizeof(PTOPOLOGY_NODE) * (Count + 1));

    if (!NewNodes)
    {
        /* out of memory */
        return MM_STATUS_NO_MEMORY;
    }

    if (Count)
    {
        /* copy existing nodes */
        MixerContext->Copy(NewNodes, InNode->NodeConnectedTo, sizeof(PTOPOLOGY) * Count);

        /* release old nodes array */
        MixerContext->Free(InNode->NodeConnectedTo);
    }

    /* add new topology node */
    NewNodes[InNode->NodeConnectedToCount] = OutNode;

    /* replace old nodes array */
    InNode->NodeConnectedTo = NewNodes;

    /* increment nodes count */
    InNode->NodeConnectedToCount++;

    /* done */
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerAddPinToPinConnection(
    IN PMIXER_CONTEXT MixerContext,
    IN OUT PPIN InPin,
    IN OUT PPIN OutPin)
{
    ULONG Count;
    PULONG NewPinsIndex;

    /* now enlarge PinConnectedTo */
    Count = InPin->PinConnectedToCount;

    /* allocate pin connection index */
    NewPinsIndex = MixerContext->Alloc(sizeof(ULONG) * (Count + 1));

    if (!NewPinsIndex)
    {
        /* out of memory */
        return MM_STATUS_NO_MEMORY;
    }

    if (Count)
    {
        /* copy existing nodes */
        MixerContext->Copy(NewPinsIndex, InPin->PinConnectedTo, sizeof(ULONG) * Count);

        /* release old nodes array */
        MixerContext->Free(InPin->PinConnectedTo);
    }

    /* add new topology node */
    NewPinsIndex[Count] = OutPin->PinId;

    /* replace old nodes array */
    InPin->PinConnectedTo = NewPinsIndex;

    /* increment pin count */
    InPin->PinConnectedToCount++;

    /* now enlarge PinConnectedFrom */
    Count = OutPin->PinConnectedFromCount;

    /* allocate pin connection index */
    NewPinsIndex = MixerContext->Alloc(sizeof(ULONG) * (Count + 1));

    if (!NewPinsIndex)
    {
        /* out of memory */
        return MM_STATUS_NO_MEMORY;
    }

    if (Count)
    {
        /* copy existing nodes */
        MixerContext->Copy(NewPinsIndex, OutPin->PinConnectedFrom, sizeof(ULONG) * Count);

        /* release old nodes array */
        MixerContext->Free(OutPin->PinConnectedFrom);
    }

    /* add new topology node */
    NewPinsIndex[Count] = InPin->PinId;

    /* replace old nodes array */
    OutPin->PinConnectedFrom = NewPinsIndex;

    /* increment pin count */
    OutPin->PinConnectedFromCount++;

    /* done */
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerHandleNodePinConnection(
    IN PMIXER_CONTEXT MixerContext,
    IN PKSTOPOLOGY_CONNECTION Connection,
    IN OUT PTOPOLOGY Topology)
{
    PPIN Pin;
    PTOPOLOGY_NODE Node;

    /* check type */
    if (Connection->FromNode == KSFILTER_NODE &&
        Connection->ToNode == KSFILTER_NODE)
    {
        /* Pin -> Pin direction */

        /* sanity checks */
        ASSERT(Topology->TopologyPinsCount > Connection->FromNodePin);
        ASSERT(Topology->TopologyPinsCount > Connection->ToNodePin);

        /* add connection */
        return MMixerAddPinToPinConnection(MixerContext,
                                           &Topology->TopologyPins[Connection->FromNodePin],
                                           &Topology->TopologyPins[Connection->ToNodePin]);

    }
    else if (Connection->FromNode == KSFILTER_NODE)
    {
        /* Pin -> Node direction */

        /* sanity checks */
        ASSERT(Topology->TopologyPinsCount > Connection->FromNodePin);
        ASSERT(Topology->TopologyNodesCount > Connection->ToNode);
        ASSERT(Connection->ToNode != KSFILTER_NODE);

        /* get pin */
        Pin = &Topology->TopologyPins[Connection->FromNodePin];

        /* get node */
        Node = &Topology->TopologyNodes[Connection->ToNode];

        /* initialize pin */
        Pin->PinId = Connection->FromNodePin;

        /* mark as visited */
        Pin->Visited = TRUE;
        Node->Visited = TRUE;

        /* add connection */
        return MMixerAddPinConnection(MixerContext, Pin, Node, TRUE);
    }
    else if (Connection->ToNode == KSFILTER_NODE)
    {
         /* Node -> Pin direction */

        /* sanity checks */
        ASSERT(Topology->TopologyPinsCount > Connection->ToNodePin);
        ASSERT(Topology->TopologyNodesCount > Connection->FromNode);
        ASSERT(Connection->FromNode != KSFILTER_NODE);

        /* get pin */
        Pin = &Topology->TopologyPins[Connection->ToNodePin];

        /* get node */
        Node = &Topology->TopologyNodes[Connection->FromNode];

        /* initialize pin */
        Pin->PinId = Connection->ToNodePin;

        /* mark as visited */
        Pin->Visited = TRUE;
        Node->Visited = TRUE;

        /* add connection */
        return MMixerAddPinConnection(MixerContext, Pin, Node, FALSE);
    }
    /* invalid call */
    ASSERT(0);
    return MM_STATUS_INVALID_PARAMETER;
}

MIXER_STATUS
MMixerExploreTopology(
    IN PMIXER_CONTEXT MixerContext,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN OUT PTOPOLOGY Topology)
{
    ULONG Index;
    PKSTOPOLOGY_CONNECTION Connection;
    MIXER_STATUS Status;

    /* sanity check */
    ASSERT(Topology->TopologyNodesCount == NodeTypes->Count);

    /* get node connections */
    Connection = (PKSTOPOLOGY_CONNECTION)(NodeConnections + 1);

    for(Index = 0; Index < NodeConnections->Count; Index++)
    {
        if (Connection[Index].FromNode == KSFILTER_NODE ||
            Connection[Index].ToNode == KSFILTER_NODE)
        {
            /* handle connection from Pin -> Node / Node->Pin */
            Status = MMixerHandleNodePinConnection(MixerContext,
                                                   &Connection[Index],
                                                   Topology);

        }
        else
        {
            /* handle connection from Node -> Node */
            Status = MMixerHandleNodeToNodeConnection(MixerContext,
                                                      &Connection[Index],
                                                      Topology);
        }

        if (Status != MM_STATUS_SUCCESS)
        {
            /* failed to handle connection */
            return Status;
        }
    }

    /* done */
    return MM_STATUS_SUCCESS;
}

VOID
MMixerAddPinIndexToArray(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG PinId,
    IN ULONG MaxPins,
    OUT PULONG OutPinCount,
    OUT PULONG OutPins)
{
    ULONG Index;

    for(Index = 0; Index < MaxPins; Index++)
    {
        if (OutPins[Index] != MAXULONG)
        {
            if (OutPins[Index] > PinId)
            {
                /* shift entries up */
                MixerContext->Copy(&OutPins[Index + 1], &OutPins[Index], (MaxPins - (Index + 1)) * sizeof(ULONG));

                /* store pin id */
                OutPins[Index] = PinId;

                /* increment pin count */
                (*OutPinCount)++;

                /* done */
                return;
            }
        }
        else
        {
            /* store pin id */
            OutPins[Index] = PinId;

            /* increment pin count */
            (*OutPinCount)++;

            /* done */
            return;
        }
    }
}

VOID
MMixerGetUpOrDownStreamPins(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN PTOPOLOGY_NODE TopologyNode,
    IN ULONG bUpStream,
    OUT PULONG OutPinCount,
    OUT PULONG OutPins)
{
    ULONG Index, TopologyNodesCount, PinsCount;
    PTOPOLOGY_NODE *TopologyNodes;
    PULONG Pins;
    PPIN Pin;

    /* sanity check */
    ASSERT(TopologyNode->Visited == FALSE);

    if (bUpStream)
    {
        /* use pins to which a node is attached to */
        PinsCount = TopologyNode->PinConnectedFromCount;
        Pins = TopologyNode->PinConnectedFrom;

        TopologyNodesCount = TopologyNode->NodeConnectedFromCount;
        TopologyNodes = TopologyNode->NodeConnectedFrom;
    }
    else
    {
        /* use pins which are attached to a node */
        PinsCount = TopologyNode->PinConnectedToCount;
        Pins = TopologyNode->PinConnectedTo;

        TopologyNodesCount = TopologyNode->NodeConnectedToCount;
        TopologyNodes = TopologyNode->NodeConnectedTo;
    }

    /* add all diretly connected pins */
    for(Index = 0; Index < PinsCount; Index++)
    {
        /* sanity check */
        ASSERT(Pins[Index] < Topology->TopologyPinsCount);

        /* get pin */
        Pin = &Topology->TopologyPins[Pins[Index]];

        /* pin should not have been visited */
        ASSERT(Pin->Visited == FALSE);
        ASSERT(Pins[Index] == Pin->PinId);

        /* FIXME support Pin -> Pin connections in iteration */
        if (bUpStream)
        {
            /* indicates a very broken topology Pin -> Pin -> Node <-... */
            ASSERT(Pin->PinConnectedFromCount == 0);
        }
        else
        {
            /* indicates a very broken topology -> Node -> Pin -> Pin */
            ASSERT(Pin->PinConnectedToCount == 0);
        }

        /* add them to pin array */
        MMixerAddPinIndexToArray(MixerContext, Pin->PinId, Topology->TopologyPinsCount, OutPinCount, OutPins);

        /* mark pin as visited */
        Pin->Visited = TRUE;
    }

    /* mark node as visited */
    TopologyNode->Visited = TRUE;

    /* now visit all connected nodes */
    for(Index = 0; Index < TopologyNodesCount; Index++)
    {
        /* recursively visit them */
        MMixerGetUpOrDownStreamPins(MixerContext, Topology, TopologyNodes[Index], bUpStream, OutPinCount, OutPins);
    }

}

ULONG
MMixerGetNodeIndexFromGuid(
    IN PTOPOLOGY Topology,
    IN const GUID * NodeType)
{
    ULONG Index;

    for(Index = 0; Index < Topology->TopologyNodesCount; Index++)
    {
        if (IsEqualGUIDAligned(NodeType, &Topology->TopologyNodes[Index].NodeType))
        {
            return Index;
        }
    }

    return MAXULONG;
}


VOID
MMixerGetAllUpOrDownstreamPinsFromNodeIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    IN ULONG bUpStream,
    OUT PULONG OutPinsCount,
    OUT PULONG OutPins)
{
    PTOPOLOGY_NODE TopologyNode;

    /* reset visited status */
    MMixerResetTopologyVisitStatus(Topology);

    /* sanity check */
    ASSERT(Topology->TopologyNodesCount > NodeIndex);

    /* get topology node */
    TopologyNode = &Topology->TopologyNodes[NodeIndex];

    /* now visit all upstream pins & nodes */
    MMixerGetUpOrDownStreamPins(MixerContext, Topology, TopologyNode, bUpStream, OutPinsCount, OutPins);
}

VOID
MMixerGetUpOrDownstreamNodes(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN PTOPOLOGY_NODE TopologyNode,
    IN ULONG bUpStream,
    OUT PULONG OutNodeCount,
    OUT PULONG OutNodes)
{
    ULONG Index, TopologyNodesCount;
    PTOPOLOGY_NODE Node, *TopologyNodes;

    if (bUpStream)
    {
        /* use nodes to which a node is attached to */
        TopologyNodesCount = TopologyNode->NodeConnectedFromCount;
        TopologyNodes = TopologyNode->NodeConnectedFrom;
    }
    else
    {
        /* use nodes which are attached to a node */
        TopologyNodesCount = TopologyNode->NodeConnectedToCount;
        TopologyNodes = TopologyNode->NodeConnectedTo;
    }

    /* sanity check */
    ASSERT(TopologyNode->Visited == FALSE);

    /* add all connected nodes */
    for(Index = 0; Index < TopologyNodesCount; Index++)
    {
        /* get node */
        Node = TopologyNodes[Index];

        /* node should not have been visited */
        ASSERT(Node->Visited == FALSE);

        /* mark node as visited */
        TopologyNode->Visited = TRUE;

        /* add them to node array */
        MMixerAddPinIndexToArray(MixerContext, Node->NodeIndex, Topology->TopologyNodesCount, OutNodeCount, OutNodes);

        /* recursively visit them */
        MMixerGetUpOrDownstreamNodes(MixerContext, Topology, TopologyNodes[Index], bUpStream, OutNodeCount, OutNodes);
    }
}

MIXER_STATUS
MMixerGetAllUpOrDownstreamNodesFromNodeIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    IN ULONG bUpStream,
    OUT PULONG OutNodesCount,
    OUT PULONG OutNodes)
{
    PTOPOLOGY_NODE TopologyNode;

    /* reset visited status */
    MMixerResetTopologyVisitStatus(Topology);

    /* sanity check */
    ASSERT(Topology->TopologyNodesCount > NodeIndex);

    /* get topology node */
    TopologyNode = &Topology->TopologyNodes[NodeIndex];

    /* now visit all upstream pins & nodes */
    MMixerGetUpOrDownstreamNodes(MixerContext, Topology, TopologyNode, bUpStream, OutNodesCount, OutNodes);

    /* done */
    return MM_STATUS_SUCCESS;

}

MIXER_STATUS
MMixerGetAllUpOrDownstreamPinsFromPinIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG PinIndex,
    IN ULONG bUpStream,
    OUT PULONG OutPinsCount,
    OUT PULONG OutPins)
{
    ULONG Index, TopologyNodesCount, TopologyPinsCount;
    PPIN Pin;
    PTOPOLOGY_NODE *TopologyNodes;
    PULONG TopologyPins;

    /* get pin */
    Pin = &Topology->TopologyPins[PinIndex];

    if (bUpStream)
    {
        /* use nodes to which this pin is attached to */
        TopologyNodes = Pin->NodesConnectedFrom;
        TopologyNodesCount = Pin->NodesConnectedFromCount;

        /* use pins to which this pin is attached to */
        TopologyPins = Pin->PinConnectedFrom;
        TopologyPinsCount = Pin->PinConnectedFromCount;

    }
    else
    {
        /* use nodes which are attached to a pin */
        TopologyNodes = Pin->NodesConnectedTo;
        TopologyNodesCount = Pin->NodesConnectedToCount;

        /* use pins which are attached to this pin */
        TopologyPins = Pin->PinConnectedTo;
        TopologyPinsCount = Pin->PinConnectedToCount;
    }


    /* reset visited status */
    MMixerResetTopologyVisitStatus(Topology);

    /* sanity check */
    ASSERT(Topology->TopologyPinsCount > PinIndex);

    /* add pins which are directly connected to this pin */
    for(Index = 0; Index < TopologyPinsCount; Index++)
    {
        /* add them to pin array */
        MMixerAddPinIndexToArray(MixerContext, TopologyPins[Index], Topology->TopologyPinsCount, OutPinsCount, OutPins);
    }

    /* now visit all up / down stream pins & nodes */
    for(Index = 0; Index < TopologyNodesCount; Index++)
    {
        /* explore all connected pins with helper */
        MMixerGetAllUpOrDownstreamPinsFromNodeIndex(MixerContext, Topology, TopologyNodes[Index]->NodeIndex, bUpStream, OutPinsCount, OutPins);
    }

    /* done */
    return MM_STATUS_SUCCESS;

}

VOID
MMixerGetAllUpOrDownstreamNodesFromPinIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG PinIndex,
    IN ULONG bUpStream,
    OUT PULONG OutNodesCount,
    OUT PULONG OutNodes)
{
    ULONG Index, TopologyNodesCount;
    PPIN Pin;
    PTOPOLOGY_NODE *TopologyNodes;

    /* mark them as empty */
    *OutNodesCount = 0;

    /* get pin */
    Pin = &Topology->TopologyPins[PinIndex];

    if (bUpStream)
    {
        /* use nodes to which a pin is attached to */
        TopologyNodes = Pin->NodesConnectedFrom;
        TopologyNodesCount = Pin->NodesConnectedFromCount;
    }
    else
    {
        /* use nodes which are attached to a node */
        TopologyNodes = Pin->NodesConnectedTo;
        TopologyNodesCount = Pin->NodesConnectedToCount;
    }


    /* reset visited status */
    MMixerResetTopologyVisitStatus(Topology);

    /* sanity check */
    ASSERT(Topology->TopologyPinsCount > PinIndex);

    /* now visit all up / down stream pins & nodes */
    for(Index = 0; Index < TopologyNodesCount; Index++)
    {
        /* add node to array */
        MMixerAddPinIndexToArray(MixerContext, TopologyNodes[Index]->NodeIndex, Topology->TopologyNodesCount, OutNodesCount, OutNodes);

        /* explore all connected nodes with helper */
        MMixerGetAllUpOrDownstreamNodesFromNodeIndex(MixerContext, Topology, TopologyNodes[Index]->NodeIndex, bUpStream, OutNodesCount, OutNodes);
    }
}


VOID
MMixerGetNextNodesFromPinIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG PinIndex,
    IN ULONG bUpStream,
    OUT PULONG OutNodesCount,
    OUT PULONG OutNodes)
{
    PPIN Pin;
    TOPOLOGY_NODE **TopologyNodes;
    ULONG TopologyNodesCount;
    ULONG Index;

    /* sanity check */
    ASSERT(PinIndex < Topology->TopologyPinsCount);

    /* get pin */
    Pin = &Topology->TopologyPins[PinIndex];

    if (bUpStream)
    {
        /* get up stream nodes */
        TopologyNodes = Pin->NodesConnectedFrom;
        TopologyNodesCount = Pin->NodesConnectedFromCount;
    }
    else
    {
        /* get down stream nodes */
        TopologyNodes = Pin->NodesConnectedTo;
        TopologyNodesCount = Pin->NodesConnectedToCount;
    }

    /* store topology nodes ids */
    for(Index = 0; Index < TopologyNodesCount; Index++)
    {
        OutNodes[Index] = TopologyNodes[Index]->NodeIndex;
    }

    /* store topology nodes count */
    *OutNodesCount = TopologyNodesCount;
}

VOID
MMixerGetNextNodesFromNodeIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    IN ULONG bUpStream,
    OUT PULONG OutNodesCount,
    OUT PULONG OutNodes)
{
    TOPOLOGY_NODE **TopologyNodes;
    ULONG TopologyNodesCount;
    ULONG Index;

    /* sanity check */
    ASSERT(NodeIndex < Topology->TopologyNodesCount);

    if (bUpStream)
    {
        /* get up stream nodes */
        TopologyNodes = Topology->TopologyNodes[NodeIndex].NodeConnectedFrom;
        TopologyNodesCount = Topology->TopologyNodes[NodeIndex].NodeConnectedFromCount;
    }
    else
    {
        /* get down stream nodes */
        TopologyNodes = Topology->TopologyNodes[NodeIndex].NodeConnectedTo;
        TopologyNodesCount = Topology->TopologyNodes[NodeIndex].NodeConnectedToCount;
    }

    /* store topology nodes ids */
    for(Index = 0; Index < TopologyNodesCount; Index++)
    {
        OutNodes[Index] = TopologyNodes[Index]->NodeIndex;
    }

    /* store topology nodes count */
    *OutNodesCount = TopologyNodesCount;
}

VOID
MMixerGetTopologyPinCount(
    IN PTOPOLOGY Topology,
    OUT PULONG PinCount)
{
    /* store pin count */
    *PinCount = Topology->TopologyPinsCount;
}

MIXER_STATUS
MMixerAllocateTopologyPinArray(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    OUT PULONG * OutPins)
{
    PULONG Pins;
    ULONG Index;

    /* sanity check */
    ASSERT(Topology->TopologyPinsCount != 0);

    /* allocate topology pins */
    Pins = MixerContext->Alloc(Topology->TopologyPinsCount * sizeof(ULONG));

    if (!Pins)
    {
        /* out of memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* mark index as unused */
    for(Index = 0; Index < Topology->TopologyPinsCount; Index++)
        Pins[Index] = MAXULONG;

    /* store result */
    *OutPins = Pins;

    /* done */
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerAllocateTopologyNodeArray(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    OUT PULONG * OutNodes)
{
    PULONG Nodes;
    ULONG Index;

    /* sanity check */
    ASSERT(Topology->TopologyNodesCount != 0);

    /* allocate topology pins */
    Nodes = MixerContext->Alloc(Topology->TopologyNodesCount * sizeof(ULONG));

    if (!Nodes)
    {
        /* out of memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* mark index as unused */
    for(Index = 0; Index < Topology->TopologyNodesCount; Index++)
        Nodes[Index] = MAXULONG;

    /* store result */
    *OutNodes = Nodes;

    /* done */
    return MM_STATUS_SUCCESS;
}

VOID
MMixerIsNodeTerminator(
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    OUT ULONG * bTerminator)
{
    /* sanity check */
    ASSERT(NodeIndex < Topology->TopologyNodesCount);

    /* check if node has multiple parents */
    if (Topology->TopologyNodes[NodeIndex].NodeConnectedFromCount > 1)
    {
        /* node is connected to multiple other nodes */
        *bTerminator = TRUE;

        /* done */
        return;
    }

    /* check if node is mux / sum node */
    if (IsEqualGUIDAligned(&Topology->TopologyNodes[NodeIndex].NodeType, &KSNODETYPE_SUM) ||
        IsEqualGUIDAligned(&Topology->TopologyNodes[NodeIndex].NodeType, &KSNODETYPE_MUX))
    {
        /* classic terminator */
        *bTerminator = TRUE;

        /* done */
        return;

    }

    /* node is not a terminator */
    *bTerminator = FALSE;
}

MIXER_STATUS
MMixerIsNodeConnectedToPin(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    IN ULONG PinId,
    IN ULONG bUpStream,
    OUT PULONG bConnected)
{
    MIXER_STATUS Status;
    ULONG Index, PinsCount;
    PULONG Pins;

    /* allocate pin index array */
    Status = MMixerAllocateTopologyPinArray(MixerContext, Topology, &Pins);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to allocate */
        return Status;
    }

    /* now get connected pins */
    PinsCount = 0;
    MMixerGetAllUpOrDownstreamPinsFromNodeIndex(MixerContext, Topology, NodeIndex, bUpStream, &PinsCount, Pins);

    /* set to false */
    *bConnected = FALSE;

    for(Index = 0; Index < PinsCount; Index++)
    {
        if (Pins[Index] == PinId)
        {
            /* pin is connected */
            *bConnected = TRUE;
            break;
        }
    }

    /* free pin index array */
    MixerContext->Free(Pins);

    /* done */
    return MM_STATUS_SUCCESS;
}

VOID
MMixerGetConnectedFromLogicalTopologyPins(
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    OUT PULONG OutPinCount,
    OUT PULONG OutPins)
{
    ULONG Index;
    PTOPOLOGY_NODE Node;

    /* sanity check */
    ASSERT(NodeIndex < Topology->TopologyNodesCount);

    /* get node */
    Node = &Topology->TopologyNodes[NodeIndex];

    for(Index = 0; Index < Node->NodeConnectedFromCount; Index++)
    {
        /* copy logical pin id */
        OutPins[Index] = Node->LogicalPinNodeConnectedFrom[Index];
    }

    /* store pin count */
    *OutPinCount = Node->NodeConnectedFromCount;
}

LPGUID
MMixerGetNodeTypeFromTopology(
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex)
{
    /* sanity check */
    ASSERT(NodeIndex < Topology->TopologyNodesCount);

    return &Topology->TopologyNodes[NodeIndex].NodeType;
}

VOID
MMixerSetTopologyPinReserved(
    IN PTOPOLOGY Topology,
    IN ULONG PinId)
{
    /* sanity check */
    ASSERT(PinId < Topology->TopologyPinsCount);

    /* set reserved */
    Topology->TopologyPins[PinId].Reserved = TRUE;
}

VOID
MMixerIsTopologyPinReserved(
    IN PTOPOLOGY Topology,
    IN ULONG PinId,
    OUT PULONG bReserved)
{
    /* sanity check */
    ASSERT(PinId < Topology->TopologyPinsCount);

    /* get reserved status */
    *bReserved = Topology->TopologyPins[PinId].Reserved;
}

VOID
MMixerSetTopologyNodeReserved(
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex)
{
    /* sanity check */
    ASSERT(NodeIndex < Topology->TopologyNodesCount);

    /* set reserved */
    Topology->TopologyNodes[NodeIndex].Reserved = TRUE;
}

VOID
MMixerIsTopologyNodeReserved(
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    OUT PULONG bReserved)
{
    /* sanity check */
    ASSERT(NodeIndex < Topology->TopologyNodesCount);

    /* get reserved status */
    *bReserved = Topology->TopologyNodes[NodeIndex].Reserved;
}


MIXER_STATUS
MMixerCreateTopology(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG PinCount,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    OUT PTOPOLOGY *OutTopology)
{
    MIXER_STATUS Status;
    PTOPOLOGY Topology;

    /* allocate topology */
    Status = MMixerAllocateTopology(MixerContext, NodeTypes->Count, PinCount, &Topology);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to allocate topology */
        return Status;
    }

    /* initialize topology nodes */
    MMixerInitializeTopologyNodes(MixerContext, NodeTypes, Topology);

    /* explore topology */
    Status = MMixerExploreTopology(MixerContext, NodeConnections, NodeTypes, Topology);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to allocate topology */
        return Status;
    }

    MMixerPrintTopology(Topology);

    /* store result */
    *OutTopology = Topology;

    /* done */
    return MM_STATUS_SUCCESS;
}
