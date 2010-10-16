/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/controls.c
 * PURPOSE:         Mixer Control Iteration Functions
 * PROGRAMMER:      Johannes Anderwald
 */
 
#include "priv.h"

MIXER_STATUS
MMixerAddMixerControl(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    IN LPMIXERLINE_EXT MixerLine,
    OUT LPMIXERCONTROLW MixerControl)
{
    LPGUID NodeType;
    KSP_NODE Node;
    ULONG BytesReturned;
    MIXER_STATUS Status;
    LPWSTR Name;

    /* initialize mixer control */
    MixerControl->cbStruct = sizeof(MIXERCONTROLW);
    MixerControl->dwControlID = MixerInfo->ControlId;

    /* get node type */
    NodeType = MMixerGetNodeTypeFromTopology(Topology, NodeIndex);
    /* store control type */
    MixerControl->dwControlType = MMixerGetControlTypeFromTopologyNode(NodeType);

    MixerControl->fdwControl = MIXERCONTROL_CONTROLF_UNIFORM; /* FIXME */
    MixerControl->cMultipleItems = 0; /* FIXME */

    if (MixerControl->dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE)
    {
        MixerControl->Bounds.dwMinimum = 0;
        MixerControl->Bounds.dwMaximum = 1;
    }
    else if (MixerControl->dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME)
    {
        MixerControl->Bounds.dwMinimum = 0;
        MixerControl->Bounds.dwMaximum = 0xFFFF;
        MixerControl->Metrics.cSteps = 0xC0; /* FIXME */
    }

    /* setup request to retrieve name */
    Node.NodeId = NodeIndex;
    Node.Property.Id = KSPROPERTY_TOPOLOGY_NAME;
    Node.Property.Flags = KSPROPERTY_TYPE_GET;
    Node.Property.Set = KSPROPSETID_Topology;
    Node.Reserved = 0;

    /* get node name size */
    Status = MixerContext->Control(MixerInfo->hMixer, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), NULL, 0, &BytesReturned);

    if (Status == MM_STATUS_MORE_ENTRIES)
    {
        ASSERT(BytesReturned != 0);
        Name = (LPWSTR)MixerContext->Alloc(BytesReturned);
        if (!Name)
        {
            /* not enough memory */
            return MM_STATUS_NO_MEMORY;
        }

        /* get node name */
        Status = MixerContext->Control(MixerInfo->hMixer, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), (LPVOID)Name, BytesReturned, &BytesReturned);

        if (Status == MM_STATUS_SUCCESS)
        {
            MixerContext->Copy(MixerControl->szShortName, Name, (min(MIXER_SHORT_NAME_CHARS, wcslen(Name)+1)) * sizeof(WCHAR));
            MixerControl->szShortName[MIXER_SHORT_NAME_CHARS-1] = L'\0';

            MixerContext->Copy(MixerControl->szName, Name, (min(MIXER_LONG_NAME_CHARS, wcslen(Name)+1)) * sizeof(WCHAR));
            MixerControl->szName[MIXER_LONG_NAME_CHARS-1] = L'\0';
        }

        /* free name buffer */
        MixerContext->Free(Name);
    }

    MixerInfo->ControlId++;
#if 0
    if (MixerControl->dwControlType == MIXERCONTROL_CONTROLTYPE_MUX)
    {
        KSNODEPROPERTY Property;
        ULONG PinId = 2;

        /* setup the request */
        RtlZeroMemory(&Property, sizeof(KSNODEPROPERTY));

        Property.NodeId = NodeIndex;
        Property.Property.Id = KSPROPERTY_AUDIO_MUX_SOURCE;
        Property.Property.Flags = KSPROPERTY_TYPE_SET;
        Property.Property.Set = KSPROPSETID_Audio;

        /* get node volume level info */
        Status = MixerContext->Control(hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSNODEPROPERTY), (PVOID)&PinId, sizeof(ULONG), &BytesReturned);

        DPRINT1("Status %x NodeIndex %u PinId %u\n", Status, NodeIndex, PinId);
        //DbgBreakPoint();
    }else
#endif
    if (MixerControl->dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME)
    {
        KSNODEPROPERTY_AUDIO_CHANNEL Property;
        ULONG Length;
        PKSPROPERTY_DESCRIPTION Desc;
        PKSPROPERTY_MEMBERSHEADER Members;
        PKSPROPERTY_STEPPING_LONG Range;

        Length = sizeof(KSPROPERTY_DESCRIPTION) + sizeof(KSPROPERTY_MEMBERSHEADER) + sizeof(KSPROPERTY_STEPPING_LONG);
        Desc = (PKSPROPERTY_DESCRIPTION)MixerContext->Alloc(Length);
        ASSERT(Desc);

        /* setup the request */
        RtlZeroMemory(&Property, sizeof(KSNODEPROPERTY_AUDIO_CHANNEL));

        Property.NodeProperty.NodeId = NodeIndex;
        Property.NodeProperty.Property.Id = KSPROPERTY_AUDIO_VOLUMELEVEL;
        Property.NodeProperty.Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;
        Property.NodeProperty.Property.Set = KSPROPSETID_Audio;

        /* get node volume level info */
        Status = MixerContext->Control(MixerInfo->hMixer, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSNODEPROPERTY_AUDIO_CHANNEL), Desc, Length, &BytesReturned);

        if (Status == MM_STATUS_SUCCESS)
        {
            LPMIXERVOLUME_DATA VolumeData;
            ULONG Steps, MaxRange, Index;
            LONG Value;

            Members = (PKSPROPERTY_MEMBERSHEADER)(Desc + 1);
            Range = (PKSPROPERTY_STEPPING_LONG)(Members + 1);

            DPRINT("NodeIndex %u Range Min %d Max %d Steps %x UMin %x UMax %x\n", NodeIndex, Range->Bounds.SignedMinimum, Range->Bounds.SignedMaximum, Range->SteppingDelta, Range->Bounds.UnsignedMinimum, Range->Bounds.UnsignedMaximum);

            MaxRange = Range->Bounds.UnsignedMaximum  - Range->Bounds.UnsignedMinimum;

            if (MaxRange)
            {
                ASSERT(MaxRange);
                VolumeData = (LPMIXERVOLUME_DATA)MixerContext->Alloc(sizeof(MIXERVOLUME_DATA));
                if (!VolumeData)
                    return MM_STATUS_NO_MEMORY;

                Steps = MaxRange / Range->SteppingDelta + 1;

                /* store mixer control info there */
                VolumeData->Header.dwControlID = MixerControl->dwControlID;
                VolumeData->SignedMaximum = Range->Bounds.SignedMaximum;
                VolumeData->SignedMinimum = Range->Bounds.SignedMinimum;
                VolumeData->SteppingDelta = Range->SteppingDelta;
                VolumeData->ValuesCount = Steps;
                VolumeData->InputSteppingDelta = 0x10000 / Steps;

                VolumeData->Values = (PLONG)MixerContext->Alloc(sizeof(LONG) * Steps);
                if (!VolumeData->Values)
                {
                    MixerContext->Free(Desc);
                    MixerContext->Free(VolumeData);
                    return MM_STATUS_NO_MEMORY;
                }

                Value = Range->Bounds.SignedMinimum;
                for(Index = 0; Index < Steps; Index++)
                {
                    VolumeData->Values[Index] = Value;
                    Value += Range->SteppingDelta;
                }
                InsertTailList(&MixerLine->LineControlsExtraData, &VolumeData->Header.Entry);
           }
       }
       MixerContext->Free(Desc);
    }

    DPRINT("Status %x Name %S\n", Status, MixerControl->szName);
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerCreateDestinationLine(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN ULONG bInputMixer,
    IN LPWSTR LineName)
{
    LPMIXERLINE_EXT DestinationLine;

    /* allocate a mixer destination line */
    DestinationLine = (LPMIXERLINE_EXT) MixerContext->Alloc(sizeof(MIXERLINE_EXT));
    if (!MixerInfo)
    {
        /* no memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* initialize mixer destination line */
    DestinationLine->Line.cbStruct = sizeof(MIXERLINEW);
    DestinationLine->Line.dwSource = MAXULONG;
    DestinationLine->Line.dwLineID = DESTINATION_LINE;
    DestinationLine->Line.fdwLine = MIXERLINE_LINEF_ACTIVE;
    DestinationLine->Line.dwUser = 0;
    DestinationLine->Line.dwComponentType = (bInputMixer == 0 ? MIXERLINE_COMPONENTTYPE_DST_SPEAKERS : MIXERLINE_COMPONENTTYPE_DST_WAVEIN);
    DestinationLine->Line.cChannels = 2; /* FIXME */

    if (LineName)
    {
        MixerContext->Copy(DestinationLine->Line.szShortName, LineName, (min(MIXER_SHORT_NAME_CHARS, wcslen(LineName)+1)) * sizeof(WCHAR));
        DestinationLine->Line.szShortName[MIXER_SHORT_NAME_CHARS-1] = L'\0';

        MixerContext->Copy(DestinationLine->Line.szName, LineName, (min(MIXER_LONG_NAME_CHARS, wcslen(LineName)+1)) * sizeof(WCHAR));
        DestinationLine->Line.szName[MIXER_LONG_NAME_CHARS-1] = L'\0';

    }

    DestinationLine->Line.Target.dwType = (bInputMixer == 0 ? MIXERLINE_TARGETTYPE_WAVEOUT : MIXERLINE_TARGETTYPE_WAVEIN);
    DestinationLine->Line.Target.dwDeviceID = !bInputMixer;
    DestinationLine->Line.Target.wMid = MixerInfo->MixCaps.wMid;
    DestinationLine->Line.Target.wPid = MixerInfo->MixCaps.wPid;
    DestinationLine->Line.Target.vDriverVersion = MixerInfo->MixCaps.vDriverVersion;

    ASSERT(MixerInfo->MixCaps.szPname[MAXPNAMELEN-1] == 0);
    wcscpy(DestinationLine->Line.Target.szPname, MixerInfo->MixCaps.szPname);

    /* initialize extra line */
    InitializeListHead(&DestinationLine->LineControlsExtraData);

    /* insert into mixer info */
    InsertHeadList(&MixerInfo->LineList, &DestinationLine->Entry);

    /* done */
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerGetPinName(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN ULONG PinId,
    IN OUT LPWSTR * OutBuffer)
{
    KSP_PIN Pin;
    ULONG BytesReturned;
    LPWSTR Buffer;
    MIXER_STATUS Status;

    /* prepare pin */
    Pin.PinId = PinId;
    Pin.Reserved = 0;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_NAME;

    /* try get pin name size */
    Status = MixerContext->Control(MixerInfo->hMixer, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), NULL, 0, &BytesReturned);

    /* check if buffer overflowed */
    if (Status == MM_STATUS_MORE_ENTRIES)
    {
        /* allocate buffer */
        Buffer = (LPWSTR)MixerContext->Alloc(BytesReturned);
        if (!Buffer)
        {
            /* out of memory */
            return MM_STATUS_NO_MEMORY;
        }

        /* try get pin name */
        Status = MixerContext->Control(MixerInfo->hMixer, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)Buffer, BytesReturned, &BytesReturned);
        if (Status != MM_STATUS_SUCCESS)
        {
            /* failed to get pin name */
            MixerContext->Free((PVOID)Buffer);
            return Status;
        }

        /* successfully obtained pin name */
        *OutBuffer = Buffer;
        return MM_STATUS_SUCCESS;
    }

    /* failed to get pin name */
    return Status;
}

MIXER_STATUS
MMixerBuildMixerDestinationLine(
    IN PMIXER_CONTEXT MixerContext,
    IN OUT LPMIXER_INFO MixerInfo,
    IN ULONG PinId,
    IN ULONG bInput)
{
    LPWSTR PinName;
    MIXER_STATUS Status;

    /* try get pin name */
    Status = MMixerGetPinName(MixerContext, MixerInfo, PinId, &PinName);
    if (Status == MM_STATUS_SUCCESS)
    {
        /* create mixer destination line */

        Status = MMixerCreateDestinationLine(MixerContext, MixerInfo, bInput, PinName);

        /* free pin name */
        MixerContext->Free(PinName);
    }
    else
    {
        /* create mixer destination line unlocalized */
        Status = MMixerCreateDestinationLine(MixerContext, MixerInfo, bInput, L"No Name");
    }

    return Status;
}

MIXER_STATUS
MMixerBuildTopology(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_DATA MixerData,
    OUT PTOPOLOGY * OutTopology)
{
    ULONG PinsCount;
    PKSMULTIPLE_ITEM NodeTypes = NULL;
    PKSMULTIPLE_ITEM NodeConnections = NULL;
    MIXER_STATUS Status;

    if (MixerData->Topology)
    {
        /* re-use existing topology */
        *OutTopology = MixerData->Topology;

        return MM_STATUS_SUCCESS;
    }

    /* get connected filter pin count */
    PinsCount = MMixerGetFilterPinCount(MixerContext, MixerData->hDevice);

    if (!PinsCount)
    {
        /* referenced filter does not have any pins */
        return MM_STATUS_UNSUCCESSFUL;
    }

    /* get topology node types */
    Status = MMixerGetFilterTopologyProperty(MixerContext, MixerData->hDevice, KSPROPERTY_TOPOLOGY_NODES, &NodeTypes);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to get topology node types */
        return Status;
    }

    /* get topology connections */
    Status = MMixerGetFilterTopologyProperty(MixerContext, MixerData->hDevice, KSPROPERTY_TOPOLOGY_CONNECTIONS, &NodeConnections);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to get topology connections */
        MixerContext->Free(NodeTypes);
        return Status;
    }

    /* create a topology */
    Status = MMixerCreateTopology(MixerContext, PinsCount, NodeConnections, NodeTypes, OutTopology);

    /* free node types & connections */
    MixerContext->Free(NodeConnections);
    MixerContext->Free(NodeTypes);

    if (Status == MM_STATUS_SUCCESS)
    {
        /* store topology object */
        MixerData->Topology = *OutTopology;
    }

    /* done */
    return Status;
}

MIXER_STATUS
MMixerCountMixerControls(
    IN PMIXER_CONTEXT MixerContext,
    IN PTOPOLOGY Topology,
    IN ULONG PinId,
    IN ULONG bUpStream,
    OUT PULONG OutNodesCount,
    OUT PULONG OutNodes,
    OUT PULONG OutLineTerminator)
{
    PULONG Nodes;
    ULONG NodesCount, NodeIndex, Count, bTerminator;
    MIXER_STATUS Status;

    /* allocate an array to store all nodes which are upstream of this pin */
    Status = MMixerAllocateTopologyNodeArray(MixerContext, Topology, &Nodes);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* out of memory */
        return STATUS_NO_MEMORY;
    }

    /* mark result array as zero */
    *OutNodesCount = 0;

    /* get next nodes */
    MMixerGetNextNodesFromPinIndex(MixerContext, Topology, PinId, bUpStream, &NodesCount, Nodes);

    /* assume no topology split before getting line terminator */
    ASSERT(NodesCount == 1);

    /* get first node */
    NodeIndex = Nodes[0];
    Count = 0;

    do
    {
        /* check if the node is a terminator */
        MMixerIsNodeTerminator(Topology, NodeIndex, &bTerminator);

        if (bTerminator)
        {
            /* found terminator */
            break;
        }

        /* store node id */
        OutNodes[Count] = NodeIndex;

        /* increment node count */
        Count++;

        /* get next nodes upstream */
        MMixerGetNextNodesFromNodeIndex(MixerContext, Topology, NodeIndex, bUpStream, &NodesCount, Nodes);

        /* assume there is a node connected */
        ASSERT(NodesCount != 0);
        ASSERT(NodesCount == 1);

        /* use first index */
        NodeIndex = Nodes[0];

    }while(TRUE);

    /* free node index */
    MixerContext->Free(Nodes);

    /* store nodes count */
    *OutNodesCount = Count;

    /* store line terminator */
    *OutLineTerminator = NodeIndex;

    /* done */
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerAddMixerControlsToMixerLineByNodeIndexArray(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN PTOPOLOGY Topology,
    IN OUT LPMIXERLINE_EXT DstLine,
    IN ULONG NodesCount,
    IN PULONG Nodes)
{
    ULONG Index, Count, bReserved;
    MIXER_STATUS Status;

    /* store nodes array */
    DstLine->NodeIds = Nodes;

    /* allocate MIXERCONTROLSW array */
    DstLine->LineControls = MixerContext->Alloc(NodesCount * sizeof(MIXERCONTROLW));

    if (!DstLine->LineControls)
    {
        /* out of memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* initialize control count */
    Count = 0;

    for(Index = 0; Index < NodesCount; Index++)
    {
        /* check if the node has already been reserved to a line */
        MMixerIsTopologyNodeReserved(Topology, Nodes[Index], &bReserved);

        if (bReserved)
        {
            /* node is already used, skip it */
            continue;
        }

        /* set node status as used */
        MMixerSetTopologyNodeReserved(Topology, Nodes[Index]);

        /* now add the mixer control */
        Status = MMixerAddMixerControl(MixerContext, MixerInfo, Topology, Nodes[Index], DstLine, &DstLine->LineControls[Count]);

        if (Status == MM_STATUS_SUCCESS)
        {
            /* increment control count */
            Count++;
        }
    }

    /* store control count */
    DstLine->Line.cControls = Count;

    /* done */
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerBuildMixerSourceLine(
    IN PMIXER_CONTEXT MixerContext,
    IN OUT LPMIXER_INFO MixerInfo,
    IN PTOPOLOGY Topology,
    IN ULONG PinId,
    IN ULONG NodesCount,
    IN PULONG Nodes,
    OUT LPMIXERLINE_EXT * OutSrcLine)
{
    LPMIXERLINE_EXT SrcLine, DstLine;
    LPWSTR PinName;
    MIXER_STATUS Status;

    /* construct source line */
    SrcLine = (LPMIXERLINE_EXT)MixerContext->Alloc(sizeof(MIXERLINE_EXT));

    if (!SrcLine)
    {
        /* no memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* get destination line */
    DstLine = MMixerGetSourceMixerLineByLineId(MixerInfo, DESTINATION_LINE);
    ASSERT(DstLine);

    /* initialize mixer src line */
    SrcLine->hDevice = MixerInfo->hMixer;
    SrcLine->PinId = PinId;
    SrcLine->NodeIds = Nodes;

    /* initialize mixer line */
    SrcLine->Line.cbStruct = sizeof(MIXERLINEW);
    SrcLine->Line.dwDestination = 0;
    SrcLine->Line.dwSource = DstLine->Line.cConnections;
    SrcLine->Line.dwLineID = (DstLine->Line.cConnections * 0x10000);
    SrcLine->Line.fdwLine = MIXERLINE_LINEF_ACTIVE | MIXERLINE_LINEF_SOURCE;
    SrcLine->Line.dwUser = 0;
    SrcLine->Line.cChannels = DstLine->Line.cChannels;
    SrcLine->Line.cConnections = 0;
    SrcLine->Line.Target.dwType = 1;
    SrcLine->Line.Target.dwDeviceID = DstLine->Line.Target.dwDeviceID;
    SrcLine->Line.Target.wMid = MixerInfo->MixCaps.wMid;
    SrcLine->Line.Target.wPid = MixerInfo->MixCaps.wPid;
    SrcLine->Line.Target.vDriverVersion = MixerInfo->MixCaps.vDriverVersion;
    InitializeListHead(&SrcLine->LineControlsExtraData);

    /* copy name */
    ASSERT(MixerInfo->MixCaps.szPname[MAXPNAMELEN-1] == L'\0');
    wcscpy(SrcLine->Line.Target.szPname, MixerInfo->MixCaps.szPname);

    /* get pin name */
    Status = MMixerGetPinName(MixerContext, MixerInfo, PinId, &PinName);

    if (Status == MM_STATUS_SUCCESS)
    {
        /* store pin name as line name */
        MixerContext->Copy(SrcLine->Line.szShortName, PinName, (min(MIXER_SHORT_NAME_CHARS, wcslen(PinName)+1)) * sizeof(WCHAR));
        SrcLine->Line.szShortName[MIXER_SHORT_NAME_CHARS-1] = L'\0';

        MixerContext->Copy(SrcLine->Line.szName, PinName, (min(MIXER_LONG_NAME_CHARS, wcslen(PinName)+1)) * sizeof(WCHAR));
        SrcLine->Line.szName[MIXER_LONG_NAME_CHARS-1] = L'\0';

        /* free pin name buffer */
        MixerContext->Free(PinName);
    }

    /* add the controls to mixer line */
    Status = MMixerAddMixerControlsToMixerLineByNodeIndexArray(MixerContext, MixerInfo, Topology, SrcLine, NodesCount, Nodes);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed */
        return Status;
    }

    /* store result */
    *OutSrcLine = SrcLine;

    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerAddMixerSourceLines(
    IN PMIXER_CONTEXT MixerContext,
    IN OUT LPMIXER_INFO MixerInfo,
    IN PTOPOLOGY Topology,
    IN ULONG LineTerminator)
{
    PULONG AllNodes, AllPins, AllPinNodes;
    ULONG AllNodesCount, AllPinsCount, AllPinNodesCount;
    ULONG Index, SubIndex, PinId, CurNode, bConnected;
    MIXER_STATUS Status;
    LPMIXERLINE_EXT DstLine, SrcLine;

    /* get destination line */
    DstLine = MMixerGetSourceMixerLineByLineId(MixerInfo, DESTINATION_LINE);
    ASSERT(DstLine);

    /* allocate an array to store all nodes which are upstream of the line terminator */
    Status = MMixerAllocateTopologyNodeArray(MixerContext, Topology, &AllNodes);

    /* check for success */
    if (Status != MM_STATUS_SUCCESS)
    {
        /* out of memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* allocate an array to store all nodes which are downstream of a particular pin */
    Status = MMixerAllocateTopologyNodeArray(MixerContext, Topology, &AllPinNodes);

    /* allocate an array to store all pins which are upstream of this pin */
    Status = MMixerAllocateTopologyPinArray(MixerContext, Topology, &AllPins);

    /* check for success */
    if (Status != MM_STATUS_SUCCESS)
    {
        /* out of memory */
        MixerContext->Free(AllNodes);
        return MM_STATUS_NO_MEMORY;
    }

     /* get all nodes which indirectly / directly connect to this node */
    AllNodesCount = 0;
    MMixerGetAllUpOrDownstreamNodesFromNodeIndex(MixerContext, Topology, LineTerminator, TRUE, &AllNodesCount, AllNodes);

    /* get all pins which indirectly / directly connect to this node */
    AllPinsCount = 0;
    MMixerGetAllUpOrDownstreamPinsFromNodeIndex(MixerContext, Topology, LineTerminator, TRUE, &AllPinsCount, AllPins);

    DPRINT("LineTerminator %lu\n", LineTerminator);
    DPRINT("PinCount %lu\n", AllPinsCount);
    DPRINT("AllNodesCount %lu\n", AllNodesCount);

    /* now construct the source lines which are attached to the destination line */
    Index = AllPinsCount;

    do
    {
        /* get current pin id */
        PinId = AllPins[Index - 1];

        /* reset nodes count */
        AllPinNodesCount = 0;

        /* now scan all nodes and add them to AllPinNodes array when they are connected to this pin */
        for(SubIndex = 0; SubIndex < AllNodesCount; SubIndex++)
        {
            /* get current node index */
            CurNode = AllNodes[SubIndex];

            if (CurNode != MAXULONG && CurNode != LineTerminator)
            {
                /* check if that node is connected in some way to the current pin */
                Status = MMixerIsNodeConnectedToPin(MixerContext, Topology, CurNode, PinId, TRUE, &bConnected);

                if (Status != MM_STATUS_SUCCESS)
                    break;

                if (bConnected)
                {
                    /* it is connected */
                    AllPinNodes[AllPinNodesCount] = CurNode;
                    AllPinNodesCount++;

                    /* clear current index */
                    AllNodes[SubIndex] = MAXULONG;
                }
            }
        }

        /* decrement pin index */
        Index--;

        if (AllPinNodesCount)
        {
            /* now build the mixer source line */
            Status = MMixerBuildMixerSourceLine(MixerContext, MixerInfo, Topology, PinId, AllPinNodesCount, AllPinNodes, &SrcLine);

             if (Status == MM_STATUS_SUCCESS)
             {
                 /* insert into line list */
                 InsertTailList(&MixerInfo->LineList, &SrcLine->Entry);

                 /* increment destination line count */
                 DstLine->Line.cConnections++;
             }
        }

    }while(Index != 0);

    return MM_STATUS_SUCCESS;
}


MIXER_STATUS
MMixerAddMixerControlsToDestinationLine(
    IN PMIXER_CONTEXT MixerContext,
    IN OUT LPMIXER_INFO MixerInfo,
    IN PTOPOLOGY Topology,
    IN ULONG PinId,
    IN ULONG bInput,
    OUT PULONG OutLineTerminator)
{
    PULONG Nodes;
    ULONG NodesCount, LineTerminator;
    MIXER_STATUS Status;
    LPMIXERLINE_EXT DstLine;

    /* allocate nodes index array */
    Status = MMixerAllocateTopologyNodeArray(MixerContext, Topology, &Nodes);

    /* check for success */
    if (Status != MM_STATUS_SUCCESS)
    {
        /* out of memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* get all destination line controls */
    Status = MMixerCountMixerControls(MixerContext, Topology, PinId, TRUE, &NodesCount, Nodes, &LineTerminator);

    /* check for success */
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to count controls */
        MixerContext->Free(Nodes);
        return Status;
    }

    /* get destination mixer line */
    DstLine = MMixerGetSourceMixerLineByLineId(MixerInfo, DESTINATION_LINE);

    /* sanity check */
    ASSERT(DstLine);

    if (NodesCount > 0)
    {
        /* add all nodes as mixer controls to the destination line */
        Status = MMixerAddMixerControlsToMixerLineByNodeIndexArray(MixerContext, MixerInfo, Topology, DstLine, NodesCount, Nodes);
        if (Status != MM_STATUS_SUCCESS)
        {
            /* failed to add controls */
            MixerContext->Free(Nodes);
            return Status;
        }
    }

    /* store result */
    *OutLineTerminator = LineTerminator;

    /* return result */
    return Status;
}

MIXER_STATUS
MMixerHandlePhysicalConnection(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN OUT LPMIXER_INFO MixerInfo,
    IN ULONG bInput,
    IN PKSPIN_PHYSICALCONNECTION OutConnection)
{
    MIXER_STATUS Status;
    ULONG PinsCount, LineTerminator;
    PULONG Pins;
    PTOPOLOGY Topology;

    /* first try to open the connected filter */
    OutConnection->SymbolicLinkName[1] = L'\\';
    MixerData = MMixerGetDataByDeviceName(MixerList, OutConnection->SymbolicLinkName);

     /* check if the linked connection is found */
     if (!MixerData)
     {
         /* filter references invalid physical connection */
         return MM_STATUS_UNSUCCESSFUL;
     }

     DPRINT("Name %S, Pin %lu bInput %lu\n", OutConnection->SymbolicLinkName, OutConnection->Pin, bInput);

    /* store connected mixer handle */
    MixerInfo->hMixer = MixerData->hDevice;


    Status = MMixerBuildTopology(MixerContext, MixerData, &Topology);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to create topology */
        return Status;
    }

    /* allocate pin index array which will hold all referenced pins */
    Status = MMixerAllocateTopologyPinArray(MixerContext, Topology, &Pins);
    ASSERT(Status == MM_STATUS_SUCCESS);

    if (!bInput)
    {
        /* the mixer is an output mixer
         * find end pin of the node path
         */
        PinsCount = 0;
        Status = MMixerGetAllUpOrDownstreamPinsFromPinIndex(MixerContext, Topology, OutConnection->Pin, FALSE, &PinsCount, Pins);

        /* check for success */
        if (Status != MM_STATUS_SUCCESS)
        {
            /* failed to get end pin */
            MixerContext->Free(Pins);
            //MMixerFreeTopology(Topology);

            /* return error code */
            return Status;
        }

        /* sanity checks */
        ASSERT(PinsCount != 0);
        ASSERT(PinsCount == 1);

        /* create destination line */
        Status = MMixerBuildMixerDestinationLine(MixerContext, MixerInfo, Pins[0], bInput);

        if (Status != MM_STATUS_SUCCESS)
        {
            MixerContext->Free(Pins);
            //MMixerFreeTopology(Topology);

            /* return error code */
            return Status;
        }

        /* add mixer controls to destination line */
        Status = MMixerAddMixerControlsToDestinationLine(MixerContext, MixerInfo, Topology, Pins[0], bInput, &LineTerminator);

        if (Status == MM_STATUS_SUCCESS)
        {
            /* now add the rest of the source lines */
            Status = MMixerAddMixerSourceLines(MixerContext, MixerInfo, Topology, LineTerminator);
        }
    }
    else
    {
        Status = MMixerAddMixerControlsToDestinationLine(MixerContext, MixerInfo, Topology, OutConnection->Pin, bInput, &LineTerminator);

        if (Status == MM_STATUS_SUCCESS)
        {
            /* now add the rest of the source lines */
            Status = MMixerAddMixerSourceLines(MixerContext, MixerInfo, Topology, LineTerminator);
        }
    }

    /* free topology */
    //MMixerFreeTopology(Topology);

    return Status;
}


MIXER_STATUS
MMixerInitializeFilter(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    IN ULONG bInputMixer)
{
    LPMIXER_INFO MixerInfo;
    MIXER_STATUS Status;
    PKSPIN_PHYSICALCONNECTION OutConnection;
    ULONG * Pins;
    ULONG PinsFound;

    /* allocate a mixer info struct */
    MixerInfo = (LPMIXER_INFO) MixerContext->Alloc(sizeof(MIXER_INFO));
    if (!MixerInfo)
    {
        /* no memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* intialize mixer caps */
    MixerInfo->MixCaps.wMid = MM_MICROSOFT; /* FIXME */
    MixerInfo->MixCaps.wPid = MM_PID_UNMAPPED; /* FIXME */
    MixerInfo->MixCaps.vDriverVersion = 1; /* FIXME */
    MixerInfo->MixCaps.fdwSupport = 0;
    MixerInfo->MixCaps.cDestinations = 1;
    MixerInfo->hMixer = MixerData->hDevice;

    /* get mixer name */
    MMixerGetDeviceName(MixerContext, MixerInfo, MixerData->hDeviceInterfaceKey);

    /* initialize line list */
    InitializeListHead(&MixerInfo->LineList);
    InitializeListHead(&MixerInfo->EventList);

    /* now allocate an array which will receive the indices of the pin 
     * which has a ADC / DAC nodetype in its path
     */
    Status = MMixerAllocateTopologyPinArray(MixerContext, Topology, &Pins);
    ASSERT(Status == MM_STATUS_SUCCESS);

    PinsFound = 0;

    /* now get all sink / source pins, which are attached to the ADC / DAC node
     * For sink pins (wave out) search up stream
     * For source pins (wave in) search down stream
     * The search direction is always the opposite of the current mixer type
     */
    PinsFound = 0;
    MMixerGetAllUpOrDownstreamPinsFromNodeIndex(MixerContext, Topology, NodeIndex, !bInputMixer, &PinsFound, Pins);

    /* if there is now pin found, we have a broken topology */
    ASSERT(PinsFound != 0);

    /* now create a wave info struct */
    Status = MMixerInitializeWaveInfo(MixerContext, MixerList, MixerData, MixerInfo->MixCaps.szPname, bInputMixer, PinsFound, Pins);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to create wave info struct */
        MixerContext->Free(MixerInfo);
        MixerContext->Free(Pins);
        return Status;
    }

    if (bInputMixer)
    {
        /* pre create the mixer destination line for input mixers */
        Status = MMixerBuildMixerDestinationLine(MixerContext, MixerInfo, Pins[0], bInputMixer);

        if (Status != MM_STATUS_SUCCESS)
        {
            /* failed to create mixer destination line */
            return Status;
        }
    }


    /* now get the bridge pin which is at the end of node path 
     * For sink pins (wave out) search down stream
     * For source pins (wave in) search up stream
     */
    MixerContext->Free(Pins);
    Status = MMixerAllocateTopologyPinArray(MixerContext, Topology, &Pins);
    ASSERT(Status == MM_STATUS_SUCCESS);

    PinsFound = 0;
    MMixerGetAllUpOrDownstreamPinsFromNodeIndex(MixerContext, Topology, NodeIndex, bInputMixer, &PinsFound, Pins);

    /* if there is no pin found, we have a broken topology */
    ASSERT(PinsFound != 0);

    /* there should be exactly one bridge pin */
    ASSERT(PinsFound == 1);

    DPRINT("BridgePin %lu bInputMixer %lu\n", Pins[0], bInputMixer);

    /* does the pin have a physical connection */
    Status = MMixerGetPhysicalConnection(MixerContext, MixerData->hDevice, Pins[0], &OutConnection);

    if (Status == MM_STATUS_SUCCESS)
    {
        /* topology on the topoloy filter */
        Status = MMixerHandlePhysicalConnection(MixerContext, MixerList, MixerData, MixerInfo, bInputMixer, OutConnection);

        /* free physical connection data */
        MixerContext->Free(OutConnection);
    }
    else
    {
        /* FIXME
         * handle drivers which expose their topology on the same filter
         */
        ASSERT(0);
    }

    /* free pins */
    MixerContext->Free(Pins);

    if (!bInputMixer && MixerList->MixerListCount == 1)
    {
        /* FIXME preferred device should be inserted at front
         * windows always inserts output mixer in front
         */
        InsertHeadList(&MixerList->MixerList, &MixerInfo->Entry);
    }
    else
    {
        /* insert at back */
        InsertTailList(&MixerList->MixerList, &MixerInfo->Entry);
    }

    /* increment mixer count */
    MixerList->MixerListCount++;

    /* done */
    return Status;
}

MIXER_STATUS
MMixerSetupFilter(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN PULONG DeviceCount)
{
    MIXER_STATUS Status;
    PTOPOLOGY Topology;
    ULONG NodeIndex;

    /* check if topology has already been built */
    if (MixerData->Topology == NULL)
    {
        /* build topology */
        Status = MMixerBuildTopology(MixerContext, MixerData, &Topology);

        if (Status != MM_STATUS_SUCCESS)
        {
            /* failed to build topology */
            return Status;
        }

        /* store topology */
        MixerData->Topology = Topology;
    }
    else
    {
        /* re-use topology */
        Topology = MixerData->Topology;
    }

    /* check if the filter has an wave out node */
    NodeIndex = MMixerGetNodeIndexFromGuid(Topology, &KSNODETYPE_DAC);
    if (NodeIndex != MAXULONG)
    {
        /* it has */
        Status = MMixerInitializeFilter(MixerContext, MixerList, MixerData, Topology, NodeIndex, FALSE);

        /* check for success */
        if (Status == MM_STATUS_SUCCESS)
        {
            /* increment mixer count */
            (*DeviceCount)++;
        }

    }

    /* check if the filter has an wave in node */
    NodeIndex = MMixerGetNodeIndexFromGuid(Topology, &KSNODETYPE_ADC);
    if (NodeIndex != MAXULONG)
    {
        /* it has */
        Status = MMixerInitializeFilter(MixerContext, MixerList, MixerData, Topology, NodeIndex, TRUE);

        /* check for success */
        if (Status == MM_STATUS_SUCCESS)
        {
            /* increment mixer count */
            (*DeviceCount)++;
        }

    }

    /* done */
    return Status;
}


MIXER_STATUS
MMixerAddEvent(
    IN PMIXER_CONTEXT MixerContext,
    IN OUT LPMIXER_INFO MixerInfo,
    IN PVOID MixerEventContext,
    IN PMIXER_EVENT MixerEventRoutine)
{
    //KSE_NODE Property;
    PEVENT_NOTIFICATION_ENTRY EventData;
    //ULONG BytesReturned;
    //MIXER_STATUS Status;

    EventData = (PEVENT_NOTIFICATION_ENTRY)MixerContext->AllocEventData(sizeof(EVENT_NOTIFICATION_ENTRY));
    if (!EventData)
    {
        /* not enough memory */
        return MM_STATUS_NO_MEMORY;
    }

#if 0
    /* setup request */
    Property.Event.Set = KSEVENTSETID_AudioControlChange;
    Property.Event.Flags = KSEVENT_TYPE_TOPOLOGY|KSEVENT_TYPE_ENABLE;
    Property.Event.Id = KSEVENT_CONTROL_CHANGE;

    Property.NodeId = NodeId;
    Property.Reserved = 0;

    Status = MixerContext->Control(MixerInfo->hMixer, IOCTL_KS_ENABLE_EVENT, (PVOID)&Property, sizeof(KSP_NODE), (PVOID)EventData, sizeof(KSEVENTDATA), &BytesReturned);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to add event */
        MixerContext->FreeEventData(EventData);
        return Status;
    }
#endif

    /* initialize notification entry */
    EventData->MixerEventContext = MixerEventContext;
    EventData->MixerEventRoutine;

    /* store event */
    InsertTailList(&MixerInfo->EventList, &EventData->Entry);
    return MM_STATUS_SUCCESS;
}

