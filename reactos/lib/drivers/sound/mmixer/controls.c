/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/controls.c
 * PURPOSE:         Mixer Control Iteration Functions
 * PROGRAMMER:      Johannes Anderwald
 */
 
#include "priv.h"

MIXER_STATUS
MMixerGetTargetPinsByNodeConnectionIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN ULONG bUpDirection,
    IN ULONG NodeConnectionIndex,
    OUT PULONG Pins)
{
    PKSTOPOLOGY_CONNECTION Connection;
    ULONG PinId, NodeConnectionCount, Index;
    PULONG NodeConnection;
    MIXER_STATUS Status;


    /* sanity check */
    ASSERT(NodeConnectionIndex < NodeConnections->Count);

    Connection = (PKSTOPOLOGY_CONNECTION)(NodeConnections + 1);

    //DPRINT("FromNode %u FromNodePin %u -> ToNode %u ToNodePin %u\n", Connection[NodeConnectionIndex].FromNode, Connection[NodeConnectionIndex].FromNodePin, Connection[NodeConnectionIndex].ToNode, Connection[NodeConnectionIndex].ToNodePin );

    if ((Connection[NodeConnectionIndex].ToNode == KSFILTER_NODE && bUpDirection == FALSE) ||
        (Connection[NodeConnectionIndex].FromNode == KSFILTER_NODE && bUpDirection == TRUE))
    {
        /* iteration stops here */
       if (bUpDirection)
           PinId = Connection[NodeConnectionIndex].FromNodePin;
       else
           PinId = Connection[NodeConnectionIndex].ToNodePin;

       //DPRINT("GetTargetPinsByNodeIndex FOUND Target Pin %u Parsed %u\n", PinId, Pins[PinId]);

       /* mark pin index as a target pin */
       Pins[PinId] = TRUE;
       return MM_STATUS_SUCCESS;
    }

    // get all node indexes referenced by that node
    if (bUpDirection)
    {
        Status = MMixerGetNodeIndexes(MixerContext, NodeConnections, Connection[NodeConnectionIndex].FromNode, TRUE, FALSE, &NodeConnectionCount, &NodeConnection);
    }
    else
    {
        Status = MMixerGetNodeIndexes(MixerContext, NodeConnections, Connection[NodeConnectionIndex].ToNode, TRUE, TRUE, &NodeConnectionCount, &NodeConnection);
    }

    if (Status == MM_STATUS_SUCCESS)
    {
        for(Index = 0; Index < NodeConnectionCount; Index++)
        {
            // iterate recursively into the nodes
            Status = MMixerGetTargetPinsByNodeConnectionIndex(MixerContext, NodeConnections, NodeTypes, bUpDirection, NodeConnection[Index], Pins);
            ASSERT(Status == MM_STATUS_SUCCESS);
        }
        // free node connection indexes
        MixerContext->Free(NodeConnection);
    }

    return Status;
}

MIXER_STATUS
MMixerGetControlsFromPinByConnectionIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN ULONG bUpDirection,
    IN ULONG NodeConnectionIndex,
    OUT PULONG Nodes)
{
    PKSTOPOLOGY_CONNECTION CurConnection;
    LPGUID NodeType;
    ULONG NodeIndex;
    MIXER_STATUS Status;
    ULONG NodeConnectionCount, Index;
    PULONG NodeConnection;


    /* get current connection */
    CurConnection = MMixerGetConnectionByIndex(NodeConnections, NodeConnectionIndex);

    if (bUpDirection)
        NodeIndex = CurConnection->FromNode;
    else
        NodeIndex = CurConnection->ToNode;

    if (NodeIndex > NodeTypes->Count)
    {
        // reached end of pin connection
        return MM_STATUS_SUCCESS;
    }

    /* get target node type of current connection */
    NodeType = MMixerGetNodeType(NodeTypes, NodeIndex);

    if (IsEqualGUIDAligned(NodeType, &KSNODETYPE_SUM) || IsEqualGUIDAligned(NodeType, &KSNODETYPE_MUX))
    {
        if (bUpDirection)
        {
            /* add the sum / mux node to destination line */
            Nodes[NodeIndex] = TRUE;
        }

        return MM_STATUS_SUCCESS;
    }

    /* now add the node */
    Nodes[NodeIndex] = TRUE;


    /* get all node indexes referenced by that node */
    if (bUpDirection)
    {
        Status = MMixerGetNodeIndexes(MixerContext, NodeConnections, NodeIndex, TRUE, FALSE, &NodeConnectionCount, &NodeConnection);
    }
    else
    {
        Status = MMixerGetNodeIndexes(MixerContext, NodeConnections, NodeIndex, TRUE, TRUE, &NodeConnectionCount, &NodeConnection);
    }

    if (Status == MM_STATUS_SUCCESS)
    {
        for(Index = 0; Index < NodeConnectionCount; Index++)
        {
            /* iterate recursively into the nodes */
            Status = MMixerGetControlsFromPinByConnectionIndex(MixerContext, NodeConnections, NodeTypes, bUpDirection, NodeConnection[Index], Nodes);
            ASSERT(Status == MM_STATUS_SUCCESS);
        }
        /* free node connection indexes */
        MixerContext->Free(NodeConnection);
    }

    return Status;
}

MIXER_STATUS
MMixerAddMixerControl(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN HANDLE hDevice,
    IN PKSMULTIPLE_ITEM NodeTypes,
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
    NodeType = MMixerGetNodeType(NodeTypes, NodeIndex);
    /* store control type */
    MixerControl->dwControlType = MMixerGetControlTypeFromTopologyNode(NodeType);

    MixerControl->fdwControl = MIXERCONTROL_CONTROLF_UNIFORM; //FIXME
    MixerControl->cMultipleItems = 0; //FIXME

    if (MixerControl->dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE)
    {
        MixerControl->Bounds.dwMinimum = 0;
        MixerControl->Bounds.dwMaximum = 1;
    }
    else if (MixerControl->dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME)
    {
        MixerControl->Bounds.dwMinimum = 0;
        MixerControl->Bounds.dwMaximum = 0xFFFF;
        MixerControl->Metrics.cSteps = 0xC0; //FIXME
    }

    /* setup request to retrieve name */
    Node.NodeId = NodeIndex;
    Node.Property.Id = KSPROPERTY_TOPOLOGY_NAME;
    Node.Property.Flags = KSPROPERTY_TYPE_GET;
    Node.Property.Set = KSPROPSETID_Topology;
    Node.Reserved = 0;

    /* get node name size */
    Status = MixerContext->Control(hDevice, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), NULL, 0, &BytesReturned);

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
        Status = MixerContext->Control(hDevice, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), (LPVOID)Name, BytesReturned, &BytesReturned);
        if (Status != MM_STATUS_SUCCESS)
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
        Status = MixerContext->Control(hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSNODEPROPERTY_AUDIO_CHANNEL), Desc, Length, &BytesReturned);

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
MMixerAddMixerSourceLine(
    IN PMIXER_CONTEXT MixerContext,
    IN OUT LPMIXER_INFO MixerInfo,
    IN HANDLE hDevice,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN ULONG PinId,
    IN ULONG bBridgePin,
    IN ULONG bTargetPin)
{
    LPMIXERLINE_EXT SrcLine, DstLine;
    MIXER_STATUS Status;
    KSP_PIN Pin;
    LPWSTR PinName;
    GUID NodeType;
    ULONG BytesReturned, ControlCount, Index;
    LPGUID Node;
    PULONG Nodes;

    if (!bTargetPin)
    {
        /* allocate src mixer line */
        SrcLine = (LPMIXERLINE_EXT)MixerContext->Alloc(sizeof(MIXERLINE_EXT));

        if (!SrcLine)
            return MM_STATUS_NO_MEMORY;

        /* zero struct */
        RtlZeroMemory(SrcLine, sizeof(MIXERLINE_EXT));

    }
    else
    {
        ASSERT(!IsListEmpty(&MixerInfo->LineList));
        SrcLine = MMixerGetSourceMixerLineByLineId(MixerInfo, DESTINATION_LINE);
    }

    /* get destination line */
    DstLine = MMixerGetSourceMixerLineByLineId(MixerInfo, DESTINATION_LINE);
    ASSERT(DstLine);


    if (!bTargetPin)
    {
        /* initialize mixer src line */
        SrcLine->hDevice = hDevice;
        SrcLine->PinId = PinId;
        SrcLine->Line.cbStruct = sizeof(MIXERLINEW);

        /* initialize mixer destination line */
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
        wcscpy(SrcLine->Line.Target.szPname, MixerInfo->MixCaps.szPname);

    }

    /* allocate a node arrary */
    Nodes = (PULONG)MixerContext->Alloc(sizeof(ULONG) * NodeTypes->Count);

    if (!Nodes)
    {
        /* not enough memory */
        if (!bTargetPin)
        {
            MixerContext->Free(SrcLine);
        }
        return MM_STATUS_NO_MEMORY;
    }

    Status = MMixerGetControlsFromPin(MixerContext, NodeConnections, NodeTypes, PinId, bTargetPin, Nodes);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* something went wrong */
        if (!bTargetPin)
        {
            MixerContext->Free(SrcLine);
        }
        MixerContext->Free(Nodes);
        return Status;
    }

    /* now count all nodes controlled by that pin */
    ControlCount = 0;
    for(Index = 0; Index < NodeTypes->Count; Index++)
    {
        if (Nodes[Index])
        {
            // get node type
            Node = MMixerGetNodeType(NodeTypes, Index);

            if (MMixerGetControlTypeFromTopologyNode(Node))
            {
                // found a node which can be resolved to a type
                ControlCount++;
            }
        }
    }

    /* now allocate the line controls */
    if (ControlCount)
    {
        SrcLine->LineControls = (LPMIXERCONTROLW)MixerContext->Alloc(sizeof(MIXERCONTROLW) * ControlCount);

        if (!SrcLine->LineControls)
        {
            /* no memory available */
            if (!bTargetPin)
            {
                MixerContext->Free(SrcLine);
            }
            MixerContext->Free(Nodes);
            return MM_STATUS_NO_MEMORY;
        }

        SrcLine->NodeIds = (PULONG)MixerContext->Alloc(sizeof(ULONG) * ControlCount);
        if (!SrcLine->NodeIds)
        {
            /* no memory available */
            MixerContext->Free(SrcLine->LineControls);
            if (!bTargetPin)
            {
                MixerContext->Free(SrcLine);
            }
            MixerContext->Free(Nodes);
            return MM_STATUS_NO_MEMORY;
        }

        /* zero line controls */
        RtlZeroMemory(SrcLine->LineControls, sizeof(MIXERCONTROLW) * ControlCount);
        RtlZeroMemory(SrcLine->NodeIds, sizeof(ULONG) * ControlCount);

        ControlCount = 0;
        for(Index = 0; Index < NodeTypes->Count; Index++)
        {
            if (Nodes[Index])
            {
                // get node type
                Node = MMixerGetNodeType(NodeTypes, Index);

                if (MMixerGetControlTypeFromTopologyNode(Node))
                {
                    /* store the node index for retrieving / setting details */
                    SrcLine->NodeIds[ControlCount] = Index;

                    Status = MMixerAddMixerControl(MixerContext, MixerInfo, hDevice, NodeTypes, Index, SrcLine, &SrcLine->LineControls[ControlCount]);
                    if (Status == MM_STATUS_SUCCESS)
                    {
                        /* increment control count on success */
                        ControlCount++;
                    }
                }
            }
        }
        /* store control count */
        SrcLine->Line.cControls = ControlCount;
    }

    /* release nodes array */
    MixerContext->Free(Nodes);

    /* get pin category */
    Pin.PinId = PinId;
    Pin.Reserved = 0;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_CATEGORY;

    /* try get pin category */
    Status = MixerContext->Control(hDevice, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (LPVOID)&NodeType, sizeof(GUID), &BytesReturned);
    if (Status != MM_STATUS_SUCCESS)
    {
        //FIXME
        //map component type
    }

    /* retrieve pin name */
    Pin.PinId = PinId;
    Pin.Reserved = 0;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_NAME;

    /* try get pin name size */
    Status = MixerContext->Control(hDevice, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), NULL, 0, &BytesReturned);

    if (Status == MM_STATUS_MORE_ENTRIES)
    {
        PinName = (LPWSTR)MixerContext->Alloc(BytesReturned);
        if (PinName)
        {
            /* try get pin name */
            Status = MixerContext->Control(hDevice, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (LPVOID)PinName, BytesReturned, &BytesReturned);

            if (Status == MM_STATUS_SUCCESS)
            {
                MixerContext->Copy(SrcLine->Line.szShortName, PinName, (min(MIXER_SHORT_NAME_CHARS, wcslen(PinName)+1)) * sizeof(WCHAR));
                SrcLine->Line.szShortName[MIXER_SHORT_NAME_CHARS-1] = L'\0';

                MixerContext->Copy(SrcLine->Line.szName, PinName, (min(MIXER_LONG_NAME_CHARS, wcslen(PinName)+1)) * sizeof(WCHAR));
                SrcLine->Line.szName[MIXER_LONG_NAME_CHARS-1] = L'\0';
            }
            MixerContext->Free(PinName);
        }
    }

    /* insert src line */
    if (!bTargetPin)
    {
        InsertTailList(&MixerInfo->LineList, &SrcLine->Entry);
        DstLine->Line.cConnections++;
    }

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

    // allocate a mixer destination line
    DestinationLine = (LPMIXERLINE_EXT) MixerContext->Alloc(sizeof(MIXERLINE_EXT));
    if (!MixerInfo)
    {
        // no memory
        return MM_STATUS_NO_MEMORY;
    }

    /* initialize mixer destination line */
    DestinationLine->Line.cbStruct = sizeof(MIXERLINEW);
    DestinationLine->Line.dwSource = MAXULONG;
    DestinationLine->Line.dwLineID = DESTINATION_LINE;
    DestinationLine->Line.fdwLine = MIXERLINE_LINEF_ACTIVE;
    DestinationLine->Line.dwUser = 0;
    DestinationLine->Line.dwComponentType = (bInputMixer == 0 ? MIXERLINE_COMPONENTTYPE_DST_SPEAKERS : MIXERLINE_COMPONENTTYPE_DST_WAVEIN);
    DestinationLine->Line.cChannels = 2; //FIXME

    if (LineName)
    {
        wcscpy(DestinationLine->Line.szShortName, LineName);
        wcscpy(DestinationLine->Line.szName, LineName);
    }
    else
    {
        /* FIXME no name was found for pin */
        wcscpy(DestinationLine->Line.szShortName, L"Summe");
        wcscpy(DestinationLine->Line.szName, L"Summe");
    }

    DestinationLine->Line.Target.dwType = (bInputMixer == 0 ? MIXERLINE_TARGETTYPE_WAVEOUT : MIXERLINE_TARGETTYPE_WAVEIN);
    DestinationLine->Line.Target.dwDeviceID = !bInputMixer;
    DestinationLine->Line.Target.wMid = MixerInfo->MixCaps.wMid;
    DestinationLine->Line.Target.wPid = MixerInfo->MixCaps.wPid;
    DestinationLine->Line.Target.vDriverVersion = MixerInfo->MixCaps.vDriverVersion;
    wcscpy(DestinationLine->Line.Target.szPname, MixerInfo->MixCaps.szPname);


    // insert into mixer info
    InsertHeadList(&MixerInfo->LineList, &DestinationLine->Entry);

    // done
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerGetControlsFromPin(
    IN PMIXER_CONTEXT MixerContext,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN ULONG PinId,
    IN ULONG bUpDirection,
    OUT PULONG Nodes)
{
    ULONG NodeConnectionCount, Index;
    MIXER_STATUS Status;
    PULONG NodeConnection;

    /* sanity check */
    ASSERT(PinId != (ULONG)-1);

    /* get all node indexes referenced by that pin */
    if (bUpDirection)
        Status = MMixerGetNodeIndexes(MixerContext, NodeConnections, PinId, FALSE, FALSE, &NodeConnectionCount, &NodeConnection);
    else
        Status = MMixerGetNodeIndexes(MixerContext, NodeConnections, PinId, FALSE, TRUE, &NodeConnectionCount, &NodeConnection);

    for(Index = 0; Index < NodeConnectionCount; Index++)
    {
        /* get all associated controls */
        Status = MMixerGetControlsFromPinByConnectionIndex(MixerContext, NodeConnections, NodeTypes, bUpDirection, NodeConnection[Index], Nodes);
    }

    MixerContext->Free(NodeConnection);

    return Status;
}




MIXER_STATUS
MMixerAddMixerSourceLines(
    IN PMIXER_CONTEXT MixerContext,
    IN OUT LPMIXER_INFO MixerInfo,
    IN HANDLE hDevice,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN ULONG PinsCount,
    IN ULONG BridgePinIndex,
    IN ULONG TargetPinIndex,
    IN PULONG Pins)
{
    ULONG Index;

    for(Index = PinsCount; Index > 0; Index--)
    {
        DPRINT("MMixerAddMixerSourceLines Index %lu Pin %lu\n", Index-1, Pins[Index-1]);
        if (Pins[Index-1])
        {
            MMixerAddMixerSourceLine(MixerContext, MixerInfo, hDevice, NodeConnections, NodeTypes, Index-1, (Index -1 == BridgePinIndex), (Index -1 == TargetPinIndex));
        }
    }
    return MM_STATUS_SUCCESS;
}


MIXER_STATUS
MMixerHandlePhysicalConnection(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN OUT LPMIXER_INFO MixerInfo,
    IN ULONG bInput,
    IN PKSPIN_PHYSICALCONNECTION OutConnection)
{
    PULONG PinsRef = NULL, PinConnectionIndex = NULL, PinsSrcRef;
    ULONG PinsRefCount, Index, PinConnectionIndexCount;
    MIXER_STATUS Status;
    PKSMULTIPLE_ITEM NodeTypes = NULL;
    PKSMULTIPLE_ITEM NodeConnections = NULL;
    PULONG MixerControls;
    ULONG MixerControlsCount;
    LPMIXER_DATA MixerData;


    // open the connected filter
    OutConnection->SymbolicLinkName[1] = L'\\';
    MixerData = MMixerGetDataByDeviceName(MixerList, OutConnection->SymbolicLinkName);
    ASSERT(MixerData);

    // get connected filter pin count
    PinsRefCount = MMixerGetFilterPinCount(MixerContext, MixerData->hDevice);
    ASSERT(PinsRefCount);

    PinsRef = (PULONG)MixerContext->Alloc(sizeof(ULONG) * PinsRefCount);
    if (!PinsRef)
    {
        // no memory
        return MM_STATUS_UNSUCCESSFUL;
    }

    // get topology node types
    Status = MMixerGetFilterTopologyProperty(MixerContext, MixerData->hDevice, KSPROPERTY_TOPOLOGY_NODES, &NodeTypes);
    if (Status != MM_STATUS_SUCCESS)
    {
        MixerContext->Free(PinsRef);
        return Status;
    }

    // get topology connections
    Status = MMixerGetFilterTopologyProperty(MixerContext, MixerData->hDevice, KSPROPERTY_TOPOLOGY_CONNECTIONS, &NodeConnections);
    if (Status != MM_STATUS_SUCCESS)
    {
        MixerContext->Free(PinsRef);
        MixerContext->Free(NodeTypes);
        return Status;
    }
    //  gets connection index of the bridge pin which connects to a node
    DPRINT("Pin %lu\n", OutConnection->Pin);

    Status = MMixerGetNodeIndexes(MixerContext, NodeConnections, OutConnection->Pin, FALSE, !bInput, &PinConnectionIndexCount, &PinConnectionIndex);
    if (Status != MM_STATUS_SUCCESS)
    {
        MixerContext->Free(PinsRef);
        MixerContext->Free(NodeTypes);
        MixerContext->Free(NodeConnections);
        return Status;
    }

    /* there should be no split in the bride pin */
    ASSERT(PinConnectionIndexCount == 1);

    /* find all target pins of this connection */
    Status = MMixerGetTargetPinsByNodeConnectionIndex(MixerContext, NodeConnections, NodeTypes, FALSE, PinConnectionIndex[0], PinsRef);
    if (Status != MM_STATUS_SUCCESS)
    {
        MixerContext->Free(PinsRef);
        MixerContext->Free(NodeTypes);
        MixerContext->Free(NodeConnections);
        MixerContext->Free(PinConnectionIndex);
        return Status;
    }

    for(Index = 0; Index < PinsRefCount; Index++)
    {
        DPRINT("PinsRefCount %lu Index %lu Value %lu\n", PinsRefCount, Index, PinsRef[Index]);
        if (PinsRef[Index])
        {
            // found a target pin, now get all references
            Status = MMixerGetNodeIndexes(MixerContext, NodeConnections, Index, FALSE, FALSE, &MixerControlsCount, &MixerControls);
            if (Status != MM_STATUS_SUCCESS)
            {
                DPRINT("MMixerGetNodeIndexes failed with %u\n", Status);
                break;
            }

            /* sanity check */
            ASSERT(MixerControlsCount == 1);

            PinsSrcRef = (PULONG)MixerContext->Alloc(PinsRefCount * sizeof(ULONG));
            if (!PinsSrcRef)
            {
                /* no memory */
                MixerContext->Free(PinsRef);
                MixerContext->Free(NodeTypes);
                MixerContext->Free(NodeConnections);
                MixerContext->Free(PinConnectionIndex);
                MixerContext->Free(MixerControls);
                return MM_STATUS_NO_MEMORY;
            }

            // now get all connected source pins
            Status = MMixerGetTargetPinsByNodeConnectionIndex(MixerContext, NodeConnections, NodeTypes, TRUE, MixerControls[0], PinsSrcRef);
            if (Status != MM_STATUS_SUCCESS)
            {
                // failed */
                MixerContext->Free(PinsRef);
                MixerContext->Free(NodeTypes);
                MixerContext->Free(NodeConnections);
                MixerContext->Free(PinConnectionIndex);
                MixerContext->Free(MixerControls);
                MixerContext->Free(PinsSrcRef);
                return Status;
            }

            /* add pins from target line */
            if (!bInput)
            {
                // dont add bridge pin for input mixers
                PinsSrcRef[Index] = TRUE;
                PinsSrcRef[OutConnection->Pin] = TRUE;
            }
            PinsSrcRef[OutConnection->Pin] = TRUE;

            Status = MMixerAddMixerSourceLines(MixerContext, MixerInfo, MixerData->hDevice, NodeConnections, NodeTypes, PinsRefCount, OutConnection->Pin, Index, PinsSrcRef);

            MixerContext->Free(MixerControls);
            MixerContext->Free(PinsSrcRef);
        }
    }

    return Status;
}


MIXER_STATUS
MMixerInitializeFilter(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN ULONG PinCount,
    IN ULONG NodeIndex,
    IN ULONG bInputMixer)
{
    LPMIXER_INFO MixerInfo;
    MIXER_STATUS Status;
    PKSPIN_PHYSICALCONNECTION OutConnection;
    ULONG Index;
    ULONG * Pins;
    ULONG bUsed;
    ULONG BytesReturned;
    KSP_PIN Pin;
    LPWSTR Buffer = NULL;

    // allocate a mixer info struct
    MixerInfo = (LPMIXER_INFO) MixerContext->Alloc(sizeof(MIXER_INFO));
    if (!MixerInfo)
    {
        // no memory
        return MM_STATUS_NO_MEMORY;
    }

    // intialize mixer caps */
    MixerInfo->MixCaps.wMid = MM_MICROSOFT; //FIXME
    MixerInfo->MixCaps.wPid = MM_PID_UNMAPPED; //FIXME
    MixerInfo->MixCaps.vDriverVersion = 1; //FIXME
    MixerInfo->MixCaps.fdwSupport = 0;
    MixerInfo->MixCaps.cDestinations = 1;
    MixerInfo->hMixer = MixerData->hDevice;

    // get mixer name
    MMixerGetDeviceName(MixerContext, MixerInfo, MixerData->hDeviceInterfaceKey);

    // initialize line list
    InitializeListHead(&MixerInfo->LineList);

    // now allocate an array which will receive the indices of the pin 
    // which has a ADC / DAC nodetype in its path
    Pins = (PULONG)MixerContext->Alloc(PinCount * sizeof(ULONG));

    if (!Pins)
    {
        // no memory
        MMixerFreeMixerInfo(MixerContext, MixerInfo);
        return MM_STATUS_NO_MEMORY;
    }

    // now get the target pins of the ADC / DAC node
    Status = MMixerGetTargetPins(MixerContext, NodeTypes, NodeConnections, NodeIndex, !bInputMixer, Pins, PinCount);

    for(Index = 0; Index < PinCount; Index++)
    {
        if (Pins[Index])
        {
            /* retrieve pin name */
            Pin.PinId = Index;
            Pin.Reserved = 0;
            Pin.Property.Flags = KSPROPERTY_TYPE_GET;
            Pin.Property.Set = KSPROPSETID_Pin;
            Pin.Property.Id = KSPROPERTY_PIN_NAME;

            /* try get pin name size */
            Status = MixerContext->Control(MixerData->hDevice, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), NULL, 0, &BytesReturned);

            if (Status == MM_STATUS_MORE_ENTRIES)
            {
                Buffer = (LPWSTR)MixerContext->Alloc(BytesReturned);
                if (Buffer)
                {
                    /* try get pin name */
                    Status = MixerContext->Control(MixerData->hDevice, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)Buffer, BytesReturned, &BytesReturned);
                    if (Status != MM_STATUS_SUCCESS)
                    {
                        MixerContext->Free((PVOID)Buffer);
                        Buffer = NULL;
                    }
                    else
                    {
                        // found name, done
                        break;
                    }
                }
            }
        }
    }

    Status = MMixerCreateDestinationLine(MixerContext, MixerInfo, bInputMixer, Buffer);

    if (Buffer)
    {
        // free name
        MixerContext->Free(Buffer);
    }

    if (Status != MM_STATUS_SUCCESS)
    {
        // failed to create destination line
        MixerContext->Free(MixerInfo);
        MixerContext->Free(Pins);

        return Status;
    }

    RtlZeroMemory(Pins, sizeof(ULONG) * PinCount);
    // now get the target pins of the ADC / DAC node
    Status = MMixerGetTargetPins(MixerContext, NodeTypes, NodeConnections, NodeIndex, bInputMixer, Pins, PinCount);

    if (Status != MM_STATUS_SUCCESS)
    {
        // failed to locate target pins
        MixerContext->Free(Pins);
        MMixerFreeMixerInfo(MixerContext, MixerInfo);
        DPRINT("MMixerGetTargetPins failed with %u\n", Status);
        return Status;
    }

    // filter hasnt been used
    bUsed = FALSE;

    // now check all pins and generate new lines for destination lines
    for(Index = 0; Index < PinCount; Index++)
    {
        DPRINT("Index %lu TargetPin %lu\n", Index, Pins[Index]);
        // is the current index a target pin
        if (Pins[Index])
        {
            // check if the pin has a physical connection
            Status = MMixerGetPhysicalConnection(MixerContext, MixerData->hDevice, Index, &OutConnection);
            if (Status == MM_STATUS_SUCCESS)
            {
                // the pin has a physical connection
                Status = MMixerHandlePhysicalConnection(MixerContext, MixerList, MixerInfo, bInputMixer, OutConnection);
                DPRINT("MMixerHandlePhysicalConnection status %u\n", Status);
                MixerContext->Free(OutConnection);
                bUsed = TRUE;
            }
            else
            {
                // filter exposes the topology on the same filter
                MMixerAddMixerSourceLine(MixerContext, MixerInfo, MixerData->hDevice, NodeConnections, NodeTypes, Index, FALSE, FALSE);
                bUsed = TRUE;
            }
        }
    }
    MixerContext->Free(Pins);

    if (bUsed)
    {
        // store mixer info in list
        if (!bInputMixer && MixerList->MixerListCount == 1)
        {
            //FIXME preferred device should be inserted at front
            //windows always inserts output mixer in front
            InsertHeadList(&MixerList->MixerList, &MixerInfo->Entry);
        }
        else
        {
            InsertTailList(&MixerList->MixerList, &MixerInfo->Entry);
        }
        MixerList->MixerListCount++;
        DPRINT("New MixerCount %lu\n", MixerList->MixerListCount);
    }
    else
    {
        // failed to create a mixer topology
        MMixerFreeMixerInfo(MixerContext, MixerInfo);
    }

    // done
    return Status;
}

MIXER_STATUS
MMixerSetupFilter(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN PULONG DeviceCount)
{
    PKSMULTIPLE_ITEM NodeTypes, NodeConnections;
    MIXER_STATUS Status;
    ULONG PinCount;
    ULONG NodeIndex;

    // get number of pins
    PinCount = MMixerGetFilterPinCount(MixerContext, MixerData->hDevice);
    ASSERT(PinCount);
    DPRINT("NumOfPins: %lu\n", PinCount);

    // get filter node types
    Status = MMixerGetFilterTopologyProperty(MixerContext, MixerData->hDevice, KSPROPERTY_TOPOLOGY_NODES, &NodeTypes);
    if (Status != MM_STATUS_SUCCESS)
    {
        // failed
        return Status;
    }

    // get filter node connections
    Status = MMixerGetFilterTopologyProperty(MixerContext, MixerData->hDevice, KSPROPERTY_TOPOLOGY_CONNECTIONS, &NodeConnections);
    if (Status != MM_STATUS_SUCCESS)
    {
        // failed
        MixerContext->Free(NodeTypes);
        return Status;
    }

    // check if the filter has an wave out node
    NodeIndex = MMixerGetIndexOfGuid(NodeTypes, &KSNODETYPE_DAC);
    if (NodeIndex != MAXULONG)
    {
        // it has
        Status = MMixerInitializeFilter(MixerContext, MixerList, MixerData, NodeTypes, NodeConnections, PinCount, NodeIndex, FALSE);
        DPRINT("MMixerInitializeFilter Status %u\n", Status);
        // check for success
        if (Status == MM_STATUS_SUCCESS)
        {
            // increment mixer count
            (*DeviceCount)++;
        }

    }

    // check if the filter has an wave in node
    NodeIndex = MMixerGetIndexOfGuid(NodeTypes, &KSNODETYPE_ADC);
    if (NodeIndex != MAXULONG)
    {
        // it has
        Status = MMixerInitializeFilter(MixerContext, MixerList, MixerData, NodeTypes, NodeConnections, PinCount, NodeIndex, TRUE);
        DPRINT("MMixerInitializeFilter Status %u\n", Status);
        // check for success
        if (Status == MM_STATUS_SUCCESS)
        {
            // increment mixer count
            (*DeviceCount)++;
        }

    }

    //free resources
    MixerContext->Free((PVOID)NodeTypes);
    MixerContext->Free((PVOID)NodeConnections);

    // done
    return Status;
}
