/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/sup.c
 * PURPOSE:         Mixer Support Functions
 * PROGRAMMER:      Johannes Anderwald
 */



#include "priv.h"

MIXER_STATUS
MMixerVerifyContext(
    IN PMIXER_CONTEXT MixerContext)
{
    if (MixerContext->SizeOfStruct != sizeof(MIXER_CONTEXT))
        return MM_STATUS_INVALID_PARAMETER;

    if (!MixerContext->Alloc || !MixerContext->Control || !MixerContext->Free)
        return MM_STATUS_INVALID_PARAMETER;

    if (!MixerContext->MixerContext)
        return MM_STATUS_INVALID_PARAMETER;

    return MM_STATUS_SUCCESS;
}

VOID
MMixerFreeMixerInfo(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo)
{
    //UNIMPLEMENTED
    // FIXME
    // free all lines

    MixerContext->Free((PVOID)MixerInfo);
}

LPMIXERLINE_EXT
MMixerGetSourceMixerLineByLineId(
    LPMIXER_INFO MixerInfo,
    DWORD dwLineID)
{
    PLIST_ENTRY Entry;
    LPMIXERLINE_EXT MixerLineSrc;

    /* get first entry */
    Entry = MixerInfo->LineList.Flink;

    while(Entry != &MixerInfo->LineList)
    {
        MixerLineSrc = (LPMIXERLINE_EXT)CONTAINING_RECORD(Entry, MIXERLINE_EXT, Entry);
        DPRINT("dwLineID %x dwLineID %x\n", MixerLineSrc->Line.dwLineID, dwLineID);
        if (MixerLineSrc->Line.dwLineID == dwLineID)
            return MixerLineSrc;

        Entry = Entry->Flink;
    }

    return NULL;
}

ULONG
MMixerGetIndexOfGuid(
    PKSMULTIPLE_ITEM MultipleItem,
    LPCGUID NodeType)
{
    ULONG Index;
    LPGUID Guid;

    Guid = (LPGUID)(MultipleItem+1);

    /* iterate through node type array */
    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        if (IsEqualGUIDAligned(NodeType, Guid))
        {
            /* found matching guid */
            return Index;
        }
        Guid++;
    }
    return MAXULONG;
}

PKSTOPOLOGY_CONNECTION
MMixerGetConnectionByIndex(
    IN PKSMULTIPLE_ITEM MultipleItem,
    IN ULONG Index)
{
    PKSTOPOLOGY_CONNECTION Descriptor;

    ASSERT(Index < MultipleItem->Count);

    Descriptor = (PKSTOPOLOGY_CONNECTION)(MultipleItem + 1);
    return &Descriptor[Index];
}

LPGUID
MMixerGetNodeType(
    IN PKSMULTIPLE_ITEM MultipleItem,
    IN ULONG Index)
{
    LPGUID NodeType;

    ASSERT(Index < MultipleItem->Count);

    NodeType = (LPGUID)(MultipleItem + 1);
    return &NodeType[Index];
}

MIXER_STATUS
MMixerGetNodeIndexes(
    IN PMIXER_CONTEXT MixerContext,
    IN PKSMULTIPLE_ITEM MultipleItem,
    IN ULONG NodeIndex,
    IN ULONG bNode,
    IN ULONG bFrom,
    OUT PULONG NodeReferenceCount,
    OUT PULONG *NodeReference)
{
    ULONG Index, Count = 0;
    PKSTOPOLOGY_CONNECTION Connection;
    PULONG Refs;

    // KSMULTIPLE_ITEM is followed by several KSTOPOLOGY_CONNECTION
    Connection = (PKSTOPOLOGY_CONNECTION)(MultipleItem + 1);

    // first count all referenced nodes
    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        if (bNode)
        {
            if (bFrom)
            {
                if (Connection->FromNode == NodeIndex)
                {
                    // node id has a connection
                    Count++;
                }
            }
            else
            {
                if (Connection->ToNode == NodeIndex)
                {
                    // node id has a connection
                    Count++;
                }
            }
        }
        else
        {
            if (bFrom)
            {
                if (Connection->FromNodePin == NodeIndex && Connection->FromNode == KSFILTER_NODE)
                {
                    // node id has a connection
                    Count++;
                }
            }
            else
            {
                if (Connection->ToNodePin == NodeIndex && Connection->ToNode == KSFILTER_NODE)
                {
                    // node id has a connection
                    Count++;
                }
            }
        }


        // move to next connection
        Connection++;
    }

    ASSERT(Count != 0);

    /* now allocate node index array */
    Refs = (PULONG)MixerContext->Alloc(sizeof(ULONG) * Count);
    if (!Refs)
    {
        // not enough memory
        return MM_STATUS_NO_MEMORY;
    }

    Count = 0;
    Connection = (PKSTOPOLOGY_CONNECTION)(MultipleItem + 1);
    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        if (bNode)
        {
            if (bFrom)
            {
                if (Connection->FromNode == NodeIndex)
                {
                    /* node id has a connection */
                    Refs[Count] = Index;
                    Count++;
                }
            }
            else
            {
                if (Connection->ToNode == NodeIndex)
                {
                    /* node id has a connection */
                    Refs[Count] = Index;
                    Count++;
                }
            }
        }
        else
        {
            if (bFrom)
            {
                if (Connection->FromNodePin == NodeIndex && Connection->FromNode == KSFILTER_NODE)
                {
                    /* node id has a connection */
                    Refs[Count] = Index;
                    Count++;
                }
            }
            else
            {
                if (Connection->ToNodePin == NodeIndex && Connection->ToNode == KSFILTER_NODE)
                {
                    /* node id has a connection */
                    Refs[Count] = Index;
                    Count++;
                }
            }
        }

        /* move to next connection */
        Connection++;
    }

    /* store result */
    *NodeReference = Refs;
    *NodeReferenceCount = Count;

    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerGetTargetPins(
    IN PMIXER_CONTEXT MixerContext,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN ULONG NodeIndex,
    IN ULONG bUpDirection,
    OUT PULONG Pins,
    IN ULONG PinCount)
{
    ULONG NodeConnectionCount, Index;
    MIXER_STATUS Status;
    PULONG NodeConnection;

    // sanity check */
    ASSERT(NodeIndex != (ULONG)-1);

    /* get all node indexes referenced by that pin */
    if (bUpDirection)
        Status = MMixerGetNodeIndexes(MixerContext, NodeConnections, NodeIndex, TRUE, FALSE, &NodeConnectionCount, &NodeConnection);
    else
        Status = MMixerGetNodeIndexes(MixerContext, NodeConnections, NodeIndex, TRUE, TRUE, &NodeConnectionCount, &NodeConnection);

    //DPRINT("NodeIndex %u Status %x Count %u\n", NodeIndex, Status, NodeConnectionCount);

    if (Status == MM_STATUS_SUCCESS)
    {
        for(Index = 0; Index < NodeConnectionCount; Index++)
        {
            Status = MMixerGetTargetPinsByNodeConnectionIndex(MixerContext, NodeConnections, NodeTypes, bUpDirection, NodeConnection[Index], Pins);
            ASSERT(Status == STATUS_SUCCESS);
        }
        MixerContext->Free((PVOID)NodeConnection);
    }

    return Status;
}
