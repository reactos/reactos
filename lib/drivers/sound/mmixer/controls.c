/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/controls.c
 * PURPOSE:         Mixer Control Iteration Functions
 * PROGRAMMER:      Johannes Anderwald
 */
 
#include "precomp.h"

#define YDEBUG
#include <debug.h>

const GUID KSNODETYPE_DESKTOP_MICROPHONE = {0xDFF21BE2, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_LEGACY_AUDIO_CONNECTOR = {0xDFF21FE4, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_TELEPHONE = {0xDFF21EE2, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_PHONE_LINE = {0xDFF21EE1, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_DOWN_LINE_PHONE = {0xDFF21EE3, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_DESKTOP_SPEAKER = {0xDFF21CE4, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_ROOM_SPEAKER = {0xDFF21CE5, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_COMMUNICATION_SPEAKER = {0xDFF21CE6, 0xF70F, 0x11D0, {0xB9,0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_HEADPHONES = {0xDFF21CE2, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_HEAD_MOUNTED_DISPLAY_AUDIO = {0xDFF21CE3, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_MICROPHONE = {0xDFF21BE1, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9,0x22, 0x31, 0x96}};
const GUID KSCATEGORY_AUDIO = {0x6994AD04L, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_SPDIF_INTERFACE = {0xDFF21FE5, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_ANALOG_CONNECTOR = {0xDFF21FE1, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_SPEAKER = {0xDFF21CE1, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_CD_PLAYER = {0xDFF220E3, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_SYNTHESIZER = {0xDFF220F3, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSNODETYPE_LINE_CONNECTOR = {0xDFF21FE3, 0xF70F, 0x11D0, {0xB9, 0x17, 0x00, 0xA0,0xC9, 0x22, 0x31, 0x96}};
const GUID PINNAME_VIDEO_CAPTURE  = {0xfb6c4281, 0x353, 0x11d1, {0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba}};

MIXER_STATUS
MMixerAddMixerControl(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN HANDLE hMixer,
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    IN LPMIXERLINE_EXT MixerLine,
    IN ULONG MaxChannels)
{
    LPGUID NodeType;
    KSP_NODE Node;
    ULONG BytesReturned;
    MIXER_STATUS Status;
    LPWSTR Name;
    LPMIXERCONTROL_EXT MixerControl;

    /* allocate mixer control */
    MixerControl = MixerContext->Alloc(sizeof(MIXERCONTROL_EXT));
    if (!MixerControl)
    {
        /* no memory */
        return MM_STATUS_NO_MEMORY;
    }


    /* initialize mixer control */
    MixerControl->hDevice = hMixer;
    MixerControl->NodeID = NodeIndex;
    MixerControl->ExtraData = NULL;

    MixerControl->Control.cbStruct = sizeof(MIXERCONTROLW);
    MixerControl->Control.dwControlID = MixerInfo->ControlId;

    /* get node type */
    NodeType = MMixerGetNodeTypeFromTopology(Topology, NodeIndex);
    /* store control type */
    MixerControl->Control.dwControlType = MMixerGetControlTypeFromTopologyNode(NodeType);

    MixerControl->Control.fdwControl = (MaxChannels > 1 ? 0 : MIXERCONTROL_CONTROLF_UNIFORM);
    MixerControl->Control.cMultipleItems = 0;

    /* setup request to retrieve name */
    Node.NodeId = NodeIndex;
    Node.Property.Id = KSPROPERTY_TOPOLOGY_NAME;
    Node.Property.Flags = KSPROPERTY_TYPE_GET;
    Node.Property.Set = KSPROPSETID_Topology;
    Node.Reserved = 0;

    /* get node name size */
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), NULL, 0, &BytesReturned);

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
        Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), (LPVOID)Name, BytesReturned, &BytesReturned);

        if (Status == MM_STATUS_SUCCESS)
        {
            MixerContext->Copy(MixerControl->Control.szShortName, Name, (min(MIXER_SHORT_NAME_CHARS, wcslen(Name)+1)) * sizeof(WCHAR));
            MixerControl->Control.szShortName[MIXER_SHORT_NAME_CHARS-1] = L'\0';

            MixerContext->Copy(MixerControl->Control.szName, Name, (min(MIXER_LONG_NAME_CHARS, wcslen(Name)+1)) * sizeof(WCHAR));
            MixerControl->Control.szName[MIXER_LONG_NAME_CHARS-1] = L'\0';
        }

        /* free name buffer */
        MixerContext->Free(Name);
    }

    /* increment control count */
    MixerInfo->ControlId++;

    /* insert control */
    InsertTailList(&MixerLine->ControlsList, &MixerControl->Entry);

    if (MixerControl->Control.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX)
    {
        ULONG NodesCount;
        PULONG Nodes;

        /* allocate topology nodes array */
        Status = MMixerAllocateTopologyNodeArray(MixerContext, Topology, &Nodes);

        if (Status != MM_STATUS_SUCCESS)
        {
            /* out of memory */
            return STATUS_NO_MEMORY;
        }

        /* get connected node count */
        MMixerGetNextNodesFromNodeIndex(MixerContext, Topology, NodeIndex, TRUE, &NodesCount, Nodes);

        /* TODO */
        MixerContext->Free(Nodes);

        /* setup mux bounds */
        MixerControl->Control.Bounds.dwMinimum = 0;
        MixerControl->Control.Bounds.dwMaximum = NodesCount - 1;
        MixerControl->Control.Metrics.dwReserved[0] = NodesCount;
        MixerControl->Control.cMultipleItems = NodesCount;
        MixerControl->Control.fdwControl |= MIXERCONTROL_CONTROLF_UNIFORM | MIXERCONTROL_CONTROLF_MULTIPLE;
    }
    else if (MixerControl->Control.dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE)
    {
        MixerControl->Control.Bounds.dwMinimum = 0;
        MixerControl->Control.Bounds.dwMaximum = 1;
    }
    else if (MixerControl->Control.dwControlType == MIXERCONTROL_CONTROLTYPE_ONOFF)
    {
        /* only needs to set bounds */
        MixerControl->Control.Bounds.dwMinimum = 0;
        MixerControl->Control.Bounds.dwMaximum = 1;
    }
    else if (MixerControl->Control.dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME)
    {
        KSNODEPROPERTY_AUDIO_CHANNEL Property;
        ULONG Length;
        PKSPROPERTY_DESCRIPTION Desc;
        PKSPROPERTY_MEMBERSHEADER Members;
        PKSPROPERTY_STEPPING_LONG Range;

        MixerControl->Control.Bounds.dwMinimum = 0;
        MixerControl->Control.Bounds.dwMaximum = 0xFFFF;
        MixerControl->Control.Metrics.cSteps = 0xC0; /* FIXME */

        Length = sizeof(KSPROPERTY_DESCRIPTION) + sizeof(KSPROPERTY_MEMBERSHEADER) + sizeof(KSPROPERTY_STEPPING_LONG);
        Desc = (PKSPROPERTY_DESCRIPTION)MixerContext->Alloc(Length);
        ASSERT(Desc);

        /* setup the request */
        RtlZeroMemory(&Property, sizeof(KSNODEPROPERTY_AUDIO_CHANNEL));

        Property.NodeProperty.NodeId = NodeIndex;
        Property.NodeProperty.Property.Id = KSPROPERTY_AUDIO_VOLUMELEVEL;
        Property.NodeProperty.Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_TOPOLOGY;
        Property.NodeProperty.Property.Set = KSPROPSETID_Audio;

        /* get node volume level info */
        Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSNODEPROPERTY_AUDIO_CHANNEL), Desc, Length, &BytesReturned);

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
                VolumeData->Header.dwControlID = MixerControl->Control.dwControlID;
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
                MixerControl->ExtraData = VolumeData;
           }
       }
       MixerContext->Free(Desc);
    }

    DPRINT("Status %x Name %S\n", Status, MixerControl->Control.szName);
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
    DestinationLine->Line.cChannels = 2; /* FIXME */
    DestinationLine->Line.cConnections = 0;
    DestinationLine->Line.cControls = 0;
    DestinationLine->Line.dwComponentType = (bInputMixer == 0 ? MIXERLINE_COMPONENTTYPE_DST_SPEAKERS : MIXERLINE_COMPONENTTYPE_DST_WAVEIN);
    DestinationLine->Line.dwDestination = MixerInfo->MixCaps.cDestinations;
    DestinationLine->Line.dwLineID = MixerInfo->MixCaps.cDestinations + DESTINATION_LINE;
    DestinationLine->Line.dwSource = MAXULONG;
    DestinationLine->Line.dwUser = 0;
    DestinationLine->Line.fdwLine = MIXERLINE_LINEF_ACTIVE;


    if (LineName)
    {
        MixerContext->Copy(DestinationLine->Line.szShortName, LineName, (min(MIXER_SHORT_NAME_CHARS, wcslen(LineName)+1)) * sizeof(WCHAR));
        DestinationLine->Line.szShortName[MIXER_SHORT_NAME_CHARS-1] = L'\0';

        MixerContext->Copy(DestinationLine->Line.szName, LineName, (min(MIXER_LONG_NAME_CHARS, wcslen(LineName)+1)) * sizeof(WCHAR));
        DestinationLine->Line.szName[MIXER_LONG_NAME_CHARS-1] = L'\0';

    }

    DestinationLine->Line.Target.dwType = (bInputMixer == 0 ? MIXERLINE_TARGETTYPE_WAVEOUT : MIXERLINE_TARGETTYPE_WAVEIN);
    DestinationLine->Line.Target.dwDeviceID = 0; //FIXME
    DestinationLine->Line.Target.wMid = MixerInfo->MixCaps.wMid;
    DestinationLine->Line.Target.wPid = MixerInfo->MixCaps.wPid;
    DestinationLine->Line.Target.vDriverVersion = MixerInfo->MixCaps.vDriverVersion;

    ASSERT(MixerInfo->MixCaps.szPname[MAXPNAMELEN-1] == 0);
    wcscpy(DestinationLine->Line.Target.szPname, MixerInfo->MixCaps.szPname);

    /* initialize extra line */
    InitializeListHead(&DestinationLine->ControlsList);

    /* insert into mixer info */
    InsertTailList(&MixerInfo->LineList, &DestinationLine->Entry);

    /* increment destination count */
    MixerInfo->MixCaps.cDestinations++;

    /* done */
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerGetPinName(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN HANDLE hMixer,
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
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), NULL, 0, &BytesReturned);

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
        Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)Buffer, BytesReturned, &BytesReturned);
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
    IN HANDLE hMixer,
    IN ULONG PinId,
    IN ULONG bInput)
{
    LPWSTR PinName;
    MIXER_STATUS Status;

    /* try get pin name */
    Status = MMixerGetPinName(MixerContext, MixerInfo, hMixer, PinId, &PinName);
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
    IN ULONG bInputMixer,
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

    if (NodesCount == 0)
    {
        /* a pin which is not connected from any node
         * a) it is a topology bug (driver bug)
         * b) the request is from an alternative mixer
              alternative mixer code scans all pins which have not been used and tries to build lines
         */
        DPRINT1("MMixerCountMixerControls PinId %lu is not connected by any node\n", PinId);
        MMixerPrintTopology(Topology);
        return MM_STATUS_UNSUCCESSFUL;
    }

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
            if (bInputMixer)
            {
                /* add mux source for source destination line */
                OutNodes[Count] = NodeIndex;
                Count++;
            }
            break;
        }

        /* store node id */
        OutNodes[Count] = NodeIndex;

        /* increment node count */
        Count++;

        /* get next nodes upstream */
        MMixerGetNextNodesFromNodeIndex(MixerContext, Topology, NodeIndex, bUpStream, &NodesCount, Nodes);

        if (NodesCount != 1)
        {
            DPRINT("PinId %lu bInputMixer %lu bUpStream %lu NodeIndex %lu is not connected", PinId, bInputMixer, bUpStream, NodeIndex);
            break;
        }

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
MMixerGetChannelCountEnhanced(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN HANDLE hMixer,
    IN ULONG NodeId,
    OUT PULONG MaxChannels)
{
    KSPROPERTY_DESCRIPTION Description;
    PKSPROPERTY_DESCRIPTION NewDescription;
    PKSPROPERTY_MEMBERSHEADER Header;
    ULONG BytesReturned;
    KSP_NODE Request;
    MIXER_STATUS Status;

    /* try #1 obtain it via description */
    Request.NodeId = NodeId;
    Request.Reserved = 0;
    Request.Property.Set = KSPROPSETID_Audio;
    Request.Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_TOPOLOGY;
    Request.Property.Id = KSPROPERTY_AUDIO_VOLUMELEVEL;


    /* get description */
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Request, sizeof(KSP_NODE), (PVOID)&Description, sizeof(KSPROPERTY_DESCRIPTION), &BytesReturned);
    if (Status == MM_STATUS_SUCCESS)
    {
        if (Description.DescriptionSize >= sizeof(KSPROPERTY_DESCRIPTION) + sizeof(KSPROPERTY_MEMBERSHEADER) && (Description.MembersListCount > 0))
        {
            /* allocate new description */
            NewDescription = MixerContext->Alloc(Description.DescriptionSize);

            if (!NewDescription)
            {
                /* not enough memory */
                return MM_STATUS_NO_MEMORY;
            }

            /* get description */
            Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Request, sizeof(KSP_NODE), (PVOID)NewDescription, Description.DescriptionSize, &BytesReturned);
            if (Status == MM_STATUS_SUCCESS)
            {
                /* get header */
                Header = (PKSPROPERTY_MEMBERSHEADER)(NewDescription + 1);

                if (Header->Flags & KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_MULTICHANNEL)
                {
                    /* found enhanced flag */
                    ASSERT(Header->MembersCount > 1);

                    /* store channel count */
                    *MaxChannels = Header->MembersCount;

                    /* free description */
                    MixerContext->Free(NewDescription);

                    /* done */
                    return MM_STATUS_SUCCESS;
                }
            }

            /* free description */
            MixerContext->Free(NewDescription);
        }
    }

    /* failed to get channel count enhanced */
    return MM_STATUS_UNSUCCESSFUL;
}

VOID
MMixerGetChannelCountLegacy(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN HANDLE hMixer,
    IN ULONG NodeId,
    OUT PULONG MaxChannels)
{
    ULONG BytesReturned;
    MIXER_STATUS Status;
    KSNODEPROPERTY_AUDIO_CHANNEL Channel;
    LONG Volume;

    /* setup request */
    Channel.Reserved = 0;
    Channel.NodeProperty.NodeId = NodeId;
    Channel.NodeProperty.Reserved = 0;
    Channel.NodeProperty.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
    Channel.NodeProperty.Property.Set = KSPROPSETID_Audio;
    Channel.Channel = 0;
    Channel.NodeProperty.Property.Id = KSPROPERTY_AUDIO_VOLUMELEVEL;

    do
    {
        /* get channel volume */
        Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Channel, sizeof(KSNODEPROPERTY_AUDIO_CHANNEL), (PVOID)&Volume, sizeof(LONG), &BytesReturned);
        if (Status != MM_STATUS_SUCCESS)
            break;

        /* increment channel count */
        Channel.Channel++;

    }while(TRUE);

    /* store channel count */
    *MaxChannels = Channel.Channel;

}

VOID
MMixerGetMaxChannelsForNode(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN HANDLE hMixer,
    IN ULONG NodeId,
    OUT PULONG MaxChannels)
{
    MIXER_STATUS Status;

    /* try to get it enhanced */
    Status = MMixerGetChannelCountEnhanced(MixerContext, MixerInfo, hMixer, NodeId, MaxChannels);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* get it old-fashioned way */
        MMixerGetChannelCountLegacy(MixerContext, MixerInfo, hMixer, NodeId, MaxChannels);
    }
}

MIXER_STATUS
MMixerAddMixerControlsToMixerLineByNodeIndexArray(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN HANDLE hMixer,
    IN PTOPOLOGY Topology,
    IN OUT LPMIXERLINE_EXT DstLine,
    IN ULONG NodesCount,
    IN PULONG Nodes)
{
    ULONG Index, Count, bReserved;
    MIXER_STATUS Status;
    LPGUID NodeType;
    ULONG MaxChannels;

    /* initialize control count */
    Count = 0;

    for(Index = 0; Index < NodesCount; Index++)
    {
        /* check if the node has already been reserved to a line */
        MMixerIsTopologyNodeReserved(Topology, Nodes[Index], &bReserved);
#if 0 /* MS lies */
        if (bReserved)
        {
            /* node is already used, skip it */
            continue;
        }
#endif
        /* set node status as used */
        MMixerSetTopologyNodeReserved(Topology, Nodes[Index]);

        /* query node type */
        NodeType = MMixerGetNodeTypeFromTopology(Topology, Nodes[Index]);

        if (IsEqualGUIDAligned(NodeType, &KSNODETYPE_VOLUME))
        {
            /* calculate maximum channel count for node */
            MMixerGetMaxChannelsForNode(MixerContext, MixerInfo, hMixer, Nodes[Index], &MaxChannels);

            DPRINT("NodeId %lu MaxChannels %lu Line %S Id %lu\n", Nodes[Index], MaxChannels, DstLine->Line.szName, DstLine->Line.dwLineID);
            /* calculate maximum channels */
            DstLine->Line.cChannels = min(DstLine->Line.cChannels, MaxChannels);
        }
        else
        {
            /* use default of one channel */
            MaxChannels = 1;
        }

        /* now add the mixer control */
        Status = MMixerAddMixerControl(MixerContext, MixerInfo, hMixer, Topology, Nodes[Index], DstLine, MaxChannels);

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
MMixerGetComponentAndTargetType(
    IN PMIXER_CONTEXT MixerContext,
    IN OUT LPMIXER_INFO MixerInfo,
    IN HANDLE hMixer,
    IN ULONG PinId,
    OUT PULONG ComponentType,
    OUT PULONG TargetType)
{
    KSPIN_DATAFLOW DataFlow;
    KSPIN_COMMUNICATION Communication;
    MIXER_STATUS Status;
    KSP_PIN Request;
    ULONG BytesReturned;
    GUID Guid;
    BOOLEAN BridgePin = FALSE;
    PKSPIN_PHYSICALCONNECTION Connection;

    /* first dataflow type */
    Status = MMixerGetPinDataFlowAndCommunication(MixerContext, hMixer, PinId, &DataFlow, &Communication);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to get dataflow */
        return Status;
    }

    /* now get pin category guid */
    Request.PinId = PinId;
    Request.Reserved = 0;
    Request.Property.Flags = KSPROPERTY_TYPE_GET;
    Request.Property.Set = KSPROPSETID_Pin;
    Request.Property.Id = KSPROPERTY_PIN_CATEGORY;


    /* get pin category */
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Request, sizeof(KSP_PIN), &Guid, sizeof(GUID), &BytesReturned);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to get dataflow */
        return Status;
    }

    /* check if it has a physical connection */
    Status = MMixerGetPhysicalConnection(MixerContext, hMixer, PinId, &Connection);
    if (Status == MM_STATUS_SUCCESS)
    {
        /* pin is a brige pin */
        BridgePin = TRUE;

        /* free physical connection */
        MixerContext->Free(Connection);
    }

    if (DataFlow == KSPIN_DATAFLOW_IN)
    {
        if (IsEqualGUIDAligned(&Guid, &KSNODETYPE_MICROPHONE) ||
            IsEqualGUIDAligned(&Guid, &KSNODETYPE_DESKTOP_MICROPHONE))
        {
            /* type microphone */
            *TargetType = MIXERLINE_TARGETTYPE_WAVEIN;
            *ComponentType = MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE;
        }
        else if (IsEqualGUIDAligned(&Guid, &KSNODETYPE_LEGACY_AUDIO_CONNECTOR) ||
                 IsEqualGUIDAligned(&Guid, &KSCATEGORY_AUDIO) ||
                 IsEqualGUIDAligned(&Guid, &KSNODETYPE_SPEAKER))
        {
            /* type waveout */
            *TargetType = MIXERLINE_TARGETTYPE_WAVEOUT;
            *ComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
        }
        else if (IsEqualGUIDAligned(&Guid, &KSNODETYPE_CD_PLAYER))
        {
            /* type cd player */
            *TargetType = MIXERLINE_TARGETTYPE_UNDEFINED;
            *ComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;
        }
        else if (IsEqualGUIDAligned(&Guid, &KSNODETYPE_SYNTHESIZER))
        {
            /* type synthesizer */
            *TargetType = MIXERLINE_TARGETTYPE_MIDIOUT;
            *ComponentType = MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER;
        }
        else if (IsEqualGUIDAligned(&Guid, &KSNODETYPE_LINE_CONNECTOR))
        {
            /* type line */
            *TargetType = MIXERLINE_TARGETTYPE_UNDEFINED;
            *ComponentType = MIXERLINE_COMPONENTTYPE_SRC_LINE;
        }
        else if (IsEqualGUIDAligned(&Guid, &KSNODETYPE_TELEPHONE) ||
                 IsEqualGUIDAligned(&Guid, &KSNODETYPE_PHONE_LINE) ||
                 IsEqualGUIDAligned(&Guid, &KSNODETYPE_DOWN_LINE_PHONE))
        {
            /* type telephone */
            *TargetType =  MIXERLINE_TARGETTYPE_UNDEFINED;
            *ComponentType =  MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE;
        }
        else if (IsEqualGUIDAligned(&Guid, &KSNODETYPE_ANALOG_CONNECTOR))
        {
            /* type analog */
            if (BridgePin)
                *TargetType = MIXERLINE_TARGETTYPE_WAVEIN;
            else
                *TargetType = MIXERLINE_TARGETTYPE_WAVEOUT;

            *ComponentType = MIXERLINE_COMPONENTTYPE_SRC_ANALOG;
        }
        else if (IsEqualGUIDAligned(&Guid, &KSNODETYPE_SPDIF_INTERFACE))
        {
            /* type analog */
            if (BridgePin)
                *TargetType = MIXERLINE_TARGETTYPE_WAVEIN;
            else
                *TargetType = MIXERLINE_TARGETTYPE_WAVEOUT;

            *ComponentType = MIXERLINE_COMPONENTTYPE_SRC_DIGITAL;
        }
        else
        {
            /* unknown type */
            *TargetType = MIXERLINE_TARGETTYPE_UNDEFINED;
            *ComponentType = MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED;
            DPRINT1("Unknown Category for PinId %lu BridgePin %lu\n", PinId, BridgePin);
        }
    }
    else
    {
        if (IsEqualGUIDAligned(&Guid, &KSNODETYPE_SPEAKER) ||
                 IsEqualGUIDAligned(&Guid, &KSNODETYPE_DESKTOP_SPEAKER) ||
                 IsEqualGUIDAligned(&Guid, &KSNODETYPE_ROOM_SPEAKER) ||
                 IsEqualGUIDAligned(&Guid, &KSNODETYPE_COMMUNICATION_SPEAKER))
        {
            /* type waveout */
            *TargetType =  MIXERLINE_TARGETTYPE_WAVEOUT;
            *ComponentType =  MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
        }
        else if (IsEqualGUIDAligned(&Guid, &KSCATEGORY_AUDIO) ||
                 IsEqualGUIDAligned(&Guid, &PINNAME_CAPTURE))
        {
            /* type wavein */
            *TargetType =  MIXERLINE_TARGETTYPE_WAVEIN;
            *ComponentType =  MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
        }
        else if (IsEqualGUIDAligned(&Guid, &KSNODETYPE_HEADPHONES) ||
                 IsEqualGUIDAligned(&Guid, &KSNODETYPE_HEAD_MOUNTED_DISPLAY_AUDIO))
        {
            /* type head phones */
            *TargetType =  MIXERLINE_TARGETTYPE_WAVEOUT;
            *ComponentType =  MIXERLINE_COMPONENTTYPE_DST_HEADPHONES;
        }
        else if (IsEqualGUIDAligned(&Guid, &KSNODETYPE_TELEPHONE) ||
                 IsEqualGUIDAligned(&Guid, &KSNODETYPE_PHONE_LINE) ||
                 IsEqualGUIDAligned(&Guid, &KSNODETYPE_DOWN_LINE_PHONE))
        {
            /* type waveout */
            *TargetType =   MIXERLINE_TARGETTYPE_UNDEFINED;
            *ComponentType =   MIXERLINE_COMPONENTTYPE_DST_TELEPHONE;
        }
        else if (IsEqualGUIDAligned(&Guid, &KSNODETYPE_ANALOG_CONNECTOR))
        {
            /* type analog */
            if (BridgePin)
            {
                *TargetType =  MIXERLINE_TARGETTYPE_WAVEOUT;
                *ComponentType =  MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
            }
            else
            {
                *TargetType =  MIXERLINE_TARGETTYPE_WAVEIN;
                *ComponentType =  MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
            }
        }
        else if (IsEqualGUIDAligned(&Guid, &KSNODETYPE_SPDIF_INTERFACE))
        {
            /* type spdif */
            if (BridgePin)
            {
                *TargetType =  MIXERLINE_TARGETTYPE_WAVEOUT;
                *ComponentType =  MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
            }
            else
            {
                *TargetType =  MIXERLINE_TARGETTYPE_WAVEIN;
                *ComponentType =  MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
            }
        }
        else
        {
            /* unknown type */
            *TargetType = MIXERLINE_TARGETTYPE_UNDEFINED;
            *ComponentType = MIXERLINE_COMPONENTTYPE_DST_UNDEFINED;
            DPRINT1("Unknown Category for PinId %lu BridgePin %lu\n", PinId, BridgePin);
        }
    }

    /* done */
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerBuildMixerSourceLine(
    IN PMIXER_CONTEXT MixerContext,
    IN OUT LPMIXER_INFO MixerInfo,
    IN HANDLE hMixer,
    IN PTOPOLOGY Topology,
    IN ULONG PinId,
    IN ULONG NodesCount,
    IN PULONG Nodes,
    IN ULONG DestinationLineID,
    OUT LPMIXERLINE_EXT * OutSrcLine)
{
    LPMIXERLINE_EXT SrcLine, DstLine;
    LPWSTR PinName;
    MIXER_STATUS Status;
    ULONG ComponentType, TargetType;

    /* get component and target type */
    Status = MMixerGetComponentAndTargetType(MixerContext, MixerInfo, hMixer, PinId, &ComponentType, &TargetType);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to get component status */
        TargetType = MIXERLINE_TARGETTYPE_UNDEFINED;
        ComponentType = MIXERLINE_COMPONENTTYPE_DST_UNDEFINED;
    }

    /* construct source line */
    SrcLine = (LPMIXERLINE_EXT)MixerContext->Alloc(sizeof(MIXERLINE_EXT));

    if (!SrcLine)
    {
        /* no memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* get destination line */
    DstLine = MMixerGetSourceMixerLineByLineId(MixerInfo, DestinationLineID);
    ASSERT(DstLine);

    /* initialize mixer src line */
    SrcLine->PinId = PinId;

    /* initialize mixer line */
    SrcLine->Line.cbStruct = sizeof(MIXERLINEW);
    SrcLine->Line.dwDestination = MixerInfo->MixCaps.cDestinations-1;
    SrcLine->Line.dwSource = DstLine->Line.cConnections;
    SrcLine->Line.dwLineID = (DstLine->Line.cConnections * SOURCE_LINE)+ (MixerInfo->MixCaps.cDestinations-1);
    SrcLine->Line.fdwLine = MIXERLINE_LINEF_ACTIVE | MIXERLINE_LINEF_SOURCE;
    SrcLine->Line.dwComponentType = ComponentType;
    SrcLine->Line.dwUser = 0;
    SrcLine->Line.cChannels = DstLine->Line.cChannels;
    SrcLine->Line.cConnections = 0;
    SrcLine->Line.Target.dwType = TargetType;
    SrcLine->Line.Target.dwDeviceID = DstLine->Line.Target.dwDeviceID;
    SrcLine->Line.Target.wMid = MixerInfo->MixCaps.wMid;
    SrcLine->Line.Target.wPid = MixerInfo->MixCaps.wPid;
    SrcLine->Line.Target.vDriverVersion = MixerInfo->MixCaps.vDriverVersion;
    InitializeListHead(&SrcLine->ControlsList);

    /* copy name */
    ASSERT(MixerInfo->MixCaps.szPname[MAXPNAMELEN-1] == L'\0');
    wcscpy(SrcLine->Line.Target.szPname, MixerInfo->MixCaps.szPname);

    /* get pin name */
    Status = MMixerGetPinName(MixerContext, MixerInfo, hMixer, PinId, &PinName);

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
    Status = MMixerAddMixerControlsToMixerLineByNodeIndexArray(MixerContext, MixerInfo, hMixer, Topology, SrcLine, NodesCount, Nodes);
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
    IN HANDLE hMixer,
    IN PTOPOLOGY Topology,
    IN ULONG DestinationLineID,
    IN ULONG LineTerminator)
{
    PULONG AllNodes, AllPins, AllPinNodes;
    ULONG AllNodesCount, AllPinsCount, AllPinNodesCount;
    ULONG Index, SubIndex, PinId, CurNode, bConnected;
    MIXER_STATUS Status;
    LPMIXERLINE_EXT DstLine, SrcLine;

    /* get destination line */
    DstLine = MMixerGetSourceMixerLineByLineId(MixerInfo, DestinationLineID);
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
#ifdef MMIXER_DEBUG
            ULONG TempIndex;
#endif
            /* now build the mixer source line */
            Status = MMixerBuildMixerSourceLine(MixerContext, MixerInfo, hMixer, Topology, PinId, AllPinNodesCount, AllPinNodes, DestinationLineID, &SrcLine);

             if (Status == MM_STATUS_SUCCESS)
             {
                 /* insert into line list */
                 InsertTailList(&MixerInfo->LineList, &SrcLine->Entry);

                 /* increment destination line count */
                 DstLine->Line.cConnections++;

                 /* mark pin as reserved */
                 MMixerSetTopologyPinReserved(Topology, PinId);

#ifdef MMIXER_DEBUG
                 DPRINT1("Adding PinId %lu AllPinNodesCount %lu to DestinationLine %lu\n", PinId, AllPinNodesCount, DestinationLineID);
                 for(TempIndex = 0; TempIndex < AllPinNodesCount; TempIndex++)
                     DPRINT1("NodeIndex %lu\n", AllPinNodes[TempIndex]);
#endif
             }
        }
        else
        {
#ifdef MMIXER_DEBUG
            DPRINT1("Discarding DestinationLineID %lu PinId %lu NO NODES!\n", DestinationLineID, PinId);
#endif
        }

    }while(Index != 0);

    return MM_STATUS_SUCCESS;
}


MIXER_STATUS
MMixerAddMixerControlsToDestinationLine(
    IN PMIXER_CONTEXT MixerContext,
    IN OUT LPMIXER_INFO MixerInfo,
    IN HANDLE hMixer,
    IN PTOPOLOGY Topology,
    IN ULONG PinId,
    IN ULONG bInput,
    IN ULONG DestinationLineId,
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
        return Status;
    }

    /* get all destination line controls */
    Status = MMixerCountMixerControls(MixerContext, Topology, PinId, bInput, TRUE, &NodesCount, Nodes, &LineTerminator);

    /* check for success */
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to count controls */
        MixerContext->Free(Nodes);
        return Status;
    }

    /* get destination mixer line */
    DstLine = MMixerGetSourceMixerLineByLineId(MixerInfo, DestinationLineId);

    /* sanity check */
    ASSERT(DstLine);

    if (NodesCount > 0)
    {
        /* add all nodes as mixer controls to the destination line */
        Status = MMixerAddMixerControlsToMixerLineByNodeIndexArray(MixerContext, MixerInfo, hMixer, Topology, DstLine, NodesCount, Nodes);
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

VOID
MMixerApplyOutputFilterHack(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_DATA MixerData,
    IN HANDLE hMixer,
    IN OUT PULONG PinsCount,
    IN OUT PULONG Pins)
{
    ULONG Count = 0, Index;
    MIXER_STATUS Status;
    PKSPIN_PHYSICALCONNECTION Connection;

    for(Index = 0; Index < *PinsCount; Index++)
    {
        /* check if it has a physical connection */
        Status = MMixerGetPhysicalConnection(MixerContext, hMixer, Pins[Index], &Connection);

        if (Status == MM_STATUS_SUCCESS)
        {
            /* remove pin */
            MixerContext->Copy(&Pins[Index], &Pins[Index + 1], (*PinsCount - (Index + 1)) * sizeof(ULONG));

            /* free physical connection */
            MixerContext->Free(Connection);

            /* decrement index */
            Index--;

            /* decrement pin count */
            (*PinsCount)--;
        }
        else
        {
            /* simple pin */
            Count++;
        }
    }

    /* store result */
    *PinsCount = Count;
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
    ULONG PinsCount, LineTerminator, DestinationLineID;
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

    /* sanity check */
    ASSERT(MixerData->MixerInfo == NULL || MixerData->MixerInfo == MixerInfo);

    /* associate with mixer */
    MixerData->MixerInfo = MixerInfo;

    if (MixerData->Topology == NULL)
    {
        /* construct new topology */
        Status = MMixerBuildTopology(MixerContext, MixerData, &Topology);
        if (Status != MM_STATUS_SUCCESS)
        {
            /* failed to create topology */
            return Status;
        }

        /* store topology */
        MixerData->Topology = Topology;
    }
    else
    {
        /* re-use existing topology */
        Topology = MixerData->Topology;
    }

    /* mark pin as consumed */
    MMixerSetTopologyPinReserved(Topology, OutConnection->Pin);

    if (!bInput)
    {
        /* allocate pin index array which will hold all referenced pins */
        Status = MMixerAllocateTopologyPinArray(MixerContext, Topology, &Pins);
        if (Status != MM_STATUS_SUCCESS)
        {
            /* failed to create topology */
            return Status;
        }

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
        /* HACK:
         * some topologies do not have strict boundaries
         * WorkArround: remove all pin ids which have a physical connection
         * because bridge pins may belong to different render paths
         */
        MMixerApplyOutputFilterHack(MixerContext, MixerData, MixerData->hDevice, &PinsCount, Pins);

        /* sanity checks */
        ASSERT(PinsCount != 0);
        ASSERT(PinsCount == 1);

        /* create destination line */
        Status = MMixerBuildMixerDestinationLine(MixerContext, MixerInfo, MixerData->hDevice, Pins[0], bInput);

        /* calculate destination line id */
        DestinationLineID = (DESTINATION_LINE + MixerInfo->MixCaps.cDestinations-1);

        if (Status != MM_STATUS_SUCCESS)
        {
            /* failed to build destination line */
            MixerContext->Free(Pins);

            /* return error code */
            return Status;
        }

        /* add mixer controls to destination line */
        Status = MMixerAddMixerControlsToDestinationLine(MixerContext, MixerInfo, MixerData->hDevice, Topology, Pins[0], bInput, DestinationLineID,  &LineTerminator);

        if (Status == MM_STATUS_SUCCESS)
        {
            /* now add the rest of the source lines */
            Status = MMixerAddMixerSourceLines(MixerContext, MixerInfo, MixerData->hDevice, Topology, DestinationLineID, LineTerminator);
        }

        /* mark pin as consumed */
        MMixerSetTopologyPinReserved(Topology, Pins[0]);

        /* free topology pin array */
        MixerContext->Free(Pins);
    }
    else
    {
        /* calculate destination line id */
        DestinationLineID = (DESTINATION_LINE + MixerInfo->MixCaps.cDestinations-1);

        /* add mixer controls */
        Status = MMixerAddMixerControlsToDestinationLine(MixerContext, MixerInfo, MixerData->hDevice, Topology, OutConnection->Pin, bInput, DestinationLineID, &LineTerminator);

        if (Status == MM_STATUS_SUCCESS)
        {
            /* now add the rest of the source lines */
            Status = MMixerAddMixerSourceLines(MixerContext, MixerInfo, MixerData->hDevice, Topology, DestinationLineID, LineTerminator);
        }
    }

    return Status;
}

MIXER_STATUS
MMixerInitializeFilter(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN LPMIXER_INFO MixerInfo,
    IN PTOPOLOGY Topology,
    IN ULONG NodeIndex,
    IN ULONG bInputMixer,
    IN OUT LPMIXER_INFO * OutMixerInfo)
{
    ULONG Index;
    MIXER_STATUS Status;
    PKSPIN_PHYSICALCONNECTION OutConnection;
    ULONG * Pins;
    ULONG PinsFound;
    ULONG NewMixerInfo = FALSE;

    if (MixerInfo == NULL)
    {
        /* allocate a mixer info struct */
        MixerInfo = (LPMIXER_INFO) MixerContext->Alloc(sizeof(MIXER_INFO));
        if (!MixerInfo)
        {
            /* no memory */
            return MM_STATUS_NO_MEMORY;
        }

        /* new mixer info */
        NewMixerInfo = TRUE;

        /* intialize mixer caps */
        MixerInfo->MixCaps.wMid = MM_MICROSOFT; /* FIXME */
        MixerInfo->MixCaps.wPid = MM_PID_UNMAPPED; /* FIXME */
        MixerInfo->MixCaps.vDriverVersion = 1; /* FIXME */
        MixerInfo->MixCaps.fdwSupport = 0;
        MixerInfo->MixCaps.cDestinations = 0;

        /* get mixer name */
        MMixerGetDeviceName(MixerContext, MixerInfo->MixCaps.szPname, MixerData->hDeviceInterfaceKey);

        /* initialize line list */
        InitializeListHead(&MixerInfo->LineList);
        InitializeListHead(&MixerInfo->EventList);

        /* associate with mixer data */
        MixerData->MixerInfo = MixerInfo;
    }

    /* store mixer info */
    *OutMixerInfo = MixerInfo;

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

    /* if there is no pin found, we have a broken topology */
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

    /* mark all found pins as reserved */
    for(Index = 0; Index < PinsFound; Index++)
    {
        MMixerSetTopologyPinReserved(Topology, Pins[Index]);
    }

    if (bInputMixer)
    {
        /* pre create the mixer destination line for input mixers */
        Status = MMixerBuildMixerDestinationLine(MixerContext, MixerInfo, MixerData->hDevice, Pins[0], bInputMixer);

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
        /* mark pin as reserved */
        MMixerSetTopologyPinReserved(Topology, Pins[0]);

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

    if (NewMixerInfo)
    {
        /* insert mixer */
        InsertHeadList(&MixerList->MixerList, &MixerInfo->Entry);
        /* increment mixer count */
        MixerList->MixerListCount++;
    }

    /* done */
    return Status;
}

VOID
MMixerHandleAlternativeMixers(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN PTOPOLOGY Topology)
{
    ULONG Index, PinCount, Reserved;
    MIXER_STATUS Status;
    ULONG DestinationLineID, LineTerminator;
    LPMIXERLINE_EXT DstLine;

    DPRINT("DeviceName %S\n", MixerData->DeviceName);

    /* get topology pin count */
    MMixerGetTopologyPinCount(Topology, &PinCount);

    for(Index = 0; Index < PinCount; Index++)
    {
        MMixerIsTopologyPinReserved(Topology, Index, &Reserved);

        /* check if it has already been reserved */
        if (Reserved)
        {
            /* pin has already been reserved */
            continue;
        }

        DPRINT("MixerName %S Available PinID %lu\n", MixerData->DeviceName, Index);

        /* sanity check */
        //ASSERT(MixerData->MixerInfo);

        if (!MixerData->MixerInfo)
        {
            DPRINT1("Expected mixer info\n");
            continue;
        }

        /* build the destination line */
        Status = MMixerBuildMixerDestinationLine(MixerContext, MixerData->MixerInfo, MixerData->hDevice, Index, TRUE);
        if (Status != MM_STATUS_SUCCESS)
        {
            /* failed to build destination line */
            continue;
        }

        /* calculate destination line id */
        DestinationLineID = (DESTINATION_LINE + MixerData->MixerInfo->MixCaps.cDestinations-1);

        /* add mixer controls to destination line */
        Status = MMixerAddMixerControlsToDestinationLine(MixerContext, MixerData->MixerInfo, MixerData->hDevice, MixerData->Topology, Index, TRUE, DestinationLineID,  &LineTerminator);
        if (Status == MM_STATUS_SUCCESS)
        {
            /* now add the rest of the source lines */
            Status = MMixerAddMixerSourceLines(MixerContext, MixerData->MixerInfo, MixerData->hDevice, MixerData->Topology, DestinationLineID, LineTerminator);
        }

        /* mark pin as consumed */
        MMixerSetTopologyPinReserved(Topology, Index);

        /* now grab destination line */
        DstLine = MMixerGetSourceMixerLineByLineId(MixerData->MixerInfo, DestinationLineID);

        /* set type and target as undefined */
        DstLine->Line.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_UNDEFINED;
        DstLine->Line.Target.dwType = MIXERLINE_TARGETTYPE_UNDEFINED;
        DstLine->Line.Target.vDriverVersion = 0;
        DstLine->Line.Target.wMid = 0;
        DstLine->Line.Target.wPid = 0;
    }
}

MIXER_STATUS
MMixerSetupFilter(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN PULONG DeviceCount)
{
    MIXER_STATUS Status = MM_STATUS_SUCCESS;
    PTOPOLOGY Topology;
    ULONG NodeIndex;
    LPMIXER_INFO MixerInfo = NULL;

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
        Status = MMixerInitializeFilter(MixerContext, MixerList, MixerData, NULL, Topology, NodeIndex, FALSE, &MixerInfo);

        /* check for success */
        if (Status == MM_STATUS_SUCCESS)
        {
            /* increment mixer count */
            (*DeviceCount)++;
        }
        else
        {
            /* reset mixer info in case of error */
            MixerInfo = NULL;
        }
    }

    /* check if the filter has an wave in node */
    NodeIndex = MMixerGetNodeIndexFromGuid(Topology, &KSNODETYPE_ADC);
    if (NodeIndex != MAXULONG)
    {
        /* it has */
        Status = MMixerInitializeFilter(MixerContext, MixerList, MixerData, MixerInfo, Topology, NodeIndex, TRUE, &MixerInfo);

        /* check for success */
        if (Status == MM_STATUS_SUCCESS)
        {
            /* increment mixer count */
            (*DeviceCount)++;
        }

    }

    /* TODO: apply hacks for Wave source line */

    /* activate midi devices */
    //MMixerInitializeMidiForFilter(MixerContext, MixerList, MixerData, Topology);

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
    EventData->MixerEventRoutine = MixerEventRoutine;

    /* store event */
    InsertTailList(&MixerInfo->EventList, &EventData->Entry);
    return MM_STATUS_SUCCESS;
}
