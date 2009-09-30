/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/mixer.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */
#include "wdmaud.h"

const GUID KSNODETYPE_DAC = {0x507AE360L, 0xC554, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_ADC = {0x4D837FE0L, 0xC555, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_AGC = {0xE88C9BA0L, 0xC557, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_LOUDNESS = {0x41887440L, 0xC558, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_MUTE =     {0x02B223C0L, 0xC557, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_TONE =     {0x7607E580L, 0xC557, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_VOLUME =   {0x3A5ACC00L, 0xC557, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_PEAKMETER = {0xa085651e, 0x5f0d, 0x4b36, {0xa8, 0x69, 0xd1, 0x95, 0xd6, 0xab, 0x4b, 0x9e}};
const GUID KSNODETYPE_MUX =       {0x2CEAF780, 0xC556, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_STEREO_WIDE = {0xA9E69800L, 0xC558, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_CHORUS =      {0x20173F20L, 0xC559, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_REVERB =      {0xEF0328E0L, 0xC558, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_SUPERMIX =    {0xE573ADC0L, 0xC555, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};

#define DESTINATION_LINE 0xFFFF0000

LPMIXERLINE_EXT
GetSourceMixerLine(
    LPMIXER_INFO MixerInfo,
    DWORD dwSource)
{
    PLIST_ENTRY Entry;
    LPMIXERLINE_EXT MixerLineSrc;

    /* get first entry */
    Entry = MixerInfo->LineList.Flink;

    while(Entry != &MixerInfo->LineList)
    {
        MixerLineSrc = (LPMIXERLINE_EXT)CONTAINING_RECORD(Entry, MIXERLINE_EXT, Entry);
        DPRINT("dwSource %x dwSource %x\n", MixerLineSrc->Line.dwSource, dwSource);
        if (MixerLineSrc->Line.dwSource == dwSource)
            return MixerLineSrc;

        Entry = Entry->Flink;
    }

    return NULL;
}

LPMIXERLINE_EXT
GetSourceMixerLineByLineId(
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
GetPinCount(
    IN PFILE_OBJECT FileObject)
{
    KSPROPERTY Pin;
    NTSTATUS Status;
    ULONG NumPins, BytesReturned;

    Pin.Flags = KSPROPERTY_TYPE_GET;
    Pin.Set = KSPROPSETID_Pin;
    Pin.Id = KSPROPERTY_PIN_CTYPES;

    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY), (PVOID)&NumPins, sizeof(ULONG), &BytesReturned);
    if (!NT_SUCCESS(Status))
        return 0;

    return NumPins;
}


ULONG
GetSysAudioDeviceCount(
    IN  PDEVICE_OBJECT DeviceObject)
{
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    KSPROPERTY Pin;
    ULONG Count, BytesReturned;
    NTSTATUS Status;

    /* setup the query request */
    Pin.Set = KSPROPSETID_Sysaudio;
    Pin.Id = KSPROPERTY_SYSAUDIO_DEVICE_COUNT;
    Pin.Flags = KSPROPERTY_TYPE_GET;

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* query sysaudio for the device count */
    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY), (PVOID)&Count, sizeof(ULONG), &BytesReturned);
    if (!NT_SUCCESS(Status))
        return 0;

    return Count;
}

NTSTATUS
GetSysAudioDevicePnpName(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  ULONG DeviceIndex,
    OUT LPWSTR * Device)
{
    ULONG BytesReturned;
    KSP_PIN Pin;
    NTSTATUS Status;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

   /* first check if the device index is within bounds */
   if (DeviceIndex >= GetSysAudioDeviceCount(DeviceObject))
       return STATUS_INVALID_PARAMETER;

    /* setup the query request */
    Pin.Property.Set = KSPROPSETID_Sysaudio;
    Pin.Property.Id = KSPROPERTY_SYSAUDIO_DEVICE_INTERFACE_NAME;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.PinId = DeviceIndex;

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* query sysaudio for the device path */
    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY) + sizeof(ULONG), NULL, 0, &BytesReturned);

    /* check if the request failed */
    if (Status != STATUS_BUFFER_TOO_SMALL || BytesReturned == 0)
        return STATUS_UNSUCCESSFUL;

    /* allocate buffer for the device */
    *Device = ExAllocatePool(NonPagedPool, BytesReturned);
    if (!Device)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* query sysaudio again for the device path */
    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY) + sizeof(ULONG), (PVOID)*Device, BytesReturned, &BytesReturned);

    if (!NT_SUCCESS(Status))
    {
        /* failed */
        ExFreePool(*Device);
        return Status;
    }

    return Status;
}

NTSTATUS
OpenDevice(
    IN LPWSTR Device,
    OUT PHANDLE DeviceHandle,
    OUT PFILE_OBJECT * FileObject)
{
    NTSTATUS Status;
    HANDLE hDevice;

    /* now open the device */
    Status = WdmAudOpenSysAudioDevice(Device, &hDevice);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    *DeviceHandle = hDevice;

    if (FileObject)
    {
        Status = ObReferenceObjectByHandle(hDevice, FILE_READ_DATA | FILE_WRITE_DATA, IoFileObjectType, KernelMode, (PVOID*)FileObject, NULL);

        if (!NT_SUCCESS(Status))
        {
            ZwClose(hDevice);
        }
    }

    return Status;

}


NTSTATUS
OpenSysAudioDeviceByIndex(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  ULONG DeviceIndex,
    IN  PHANDLE DeviceHandle,
    IN  PFILE_OBJECT * FileObject)
{
    LPWSTR Device = NULL;
    NTSTATUS Status;

    Status = GetSysAudioDevicePnpName(DeviceObject, DeviceIndex, &Device);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = OpenDevice(Device, DeviceHandle, FileObject);

    /* free device buffer */
    ExFreePool(Device);

    return Status;
}

NTSTATUS
GetFilterNodeProperty(
    IN PFILE_OBJECT FileObject,
    IN ULONG PropertyId,
    PKSMULTIPLE_ITEM * Item)
{
    NTSTATUS Status;
    ULONG BytesReturned;
    PKSMULTIPLE_ITEM MultipleItem;
    KSPROPERTY Property;

    /* setup query request */
    Property.Id = PropertyId;
    Property.Flags = KSPROPERTY_TYPE_GET;
    Property.Set = KSPROPSETID_Topology;

    /* query for required size */
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), NULL, 0, &BytesReturned);

    /* check for success */
    if (Status != STATUS_MORE_ENTRIES)
        return Status;

    /* allocate buffer */
    MultipleItem = (PKSMULTIPLE_ITEM)ExAllocatePool(NonPagedPool, BytesReturned);
    if (!MultipleItem)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* query for required size */
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)MultipleItem, BytesReturned, &BytesReturned);

    if (!NT_SUCCESS(Status))
    {
        /* failed */
        ExFreePool(MultipleItem);
        return Status;
    }

    *Item = MultipleItem;
    return Status;
}

ULONG
CountNodeType(
    PKSMULTIPLE_ITEM MultipleItem,
    LPGUID NodeType)
{
    ULONG Count;
    ULONG Index;
    LPGUID Guid;

    Count = 0;
    Guid = (LPGUID)(MultipleItem+1);

    /* iterate through node type array */
    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        if (IsEqualGUIDAligned(NodeType, Guid))
        {
            /* found matching guid */
            Count++;
        }
        Guid++;
    }
    return Count;
}

ULONG
GetNodeTypeIndex(
    PKSMULTIPLE_ITEM MultipleItem,
    LPGUID NodeType)
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

ULONG
GetControlTypeFromTopologyNode(
    IN LPGUID NodeType)
{
    if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_AGC))
    {
        // automatic gain control
        return MIXERCONTROL_CONTROLTYPE_ONOFF;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_LOUDNESS))
    {
        // loudness control
        return MIXERCONTROL_CONTROLTYPE_LOUDNESS;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_MUTE ))
    {
        // mute control
        return MIXERCONTROL_CONTROLTYPE_MUTE;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_TONE))
    {
        // tpne control
        //FIXME
        // MIXERCONTROL_CONTROLTYPE_ONOFF if KSPROPERTY_AUDIO_BASS_BOOST is supported
        // MIXERCONTROL_CONTROLTYPE_BASS if KSPROPERTY_AUDIO_BASS is supported
        // MIXERCONTROL_CONTROLTYPE_TREBLE if KSPROPERTY_AUDIO_TREBLE is supported
        UNIMPLEMENTED;
        return MIXERCONTROL_CONTROLTYPE_ONOFF;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_VOLUME))
    {
        // volume control
        return MIXERCONTROL_CONTROLTYPE_VOLUME;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_PEAKMETER))
    {
        // peakmeter control
        return MIXERCONTROL_CONTROLTYPE_PEAKMETER;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_MUX))
    {
        // mux control
        return MIXERCONTROL_CONTROLTYPE_MUX;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_MUX))
    {
        // mux control
        return MIXERCONTROL_CONTROLTYPE_MUX;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_STEREO_WIDE))
    {
        // stero wide control
        return MIXERCONTROL_CONTROLTYPE_FADER;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_CHORUS))
    {
        // chorus control
        return MIXERCONTROL_CONTROLTYPE_FADER;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_REVERB))
    {
        // reverb control
        return MIXERCONTROL_CONTROLTYPE_FADER;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_SUPERMIX))
    {
        // supermix control
        // MIXERCONTROL_CONTROLTYPE_MUTE if KSPROPERTY_AUDIO_MUTE is supported 
        UNIMPLEMENTED;
        return MIXERCONTROL_CONTROLTYPE_VOLUME;
    }
    UNIMPLEMENTED
    return 0;
}

NTSTATUS
GetPhysicalConnection(
    IN PFILE_OBJECT FileObject,
    IN ULONG PinId,
    OUT PKSPIN_PHYSICALCONNECTION *OutConnection)
{
    KSP_PIN Pin;
    NTSTATUS Status;
    ULONG BytesReturned;
    PKSPIN_PHYSICALCONNECTION Connection;

    /* setup the request */
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.Property.Id = KSPROPERTY_PIN_PHYSICALCONNECTION;
    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.PinId = PinId;

    /* query the pin for the physical connection */
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), NULL, 0, &BytesReturned);

    if (Status == STATUS_NOT_FOUND)
    {
        /* pin does not have a physical connection */
        return Status;
    }

    Connection = ExAllocatePool(NonPagedPool, BytesReturned);
    if (!Connection)
    {
        /* not enough memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* query the pin for the physical connection */
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)Connection, BytesReturned, &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        /* failed to query the physical connection */
        ExFreePool(Connection);
        return Status;
    }

    /* store connection */
    *OutConnection = Connection;
    return Status;
}

NTSTATUS
GetNodeIndexes(
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

    /* KSMULTIPLE_ITEM is followed by several KSTOPOLOGY_CONNECTION */
    Connection = (PKSTOPOLOGY_CONNECTION)(MultipleItem + 1);

    /* first count all referenced nodes */
    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        //DPRINT1("FromPin %u FromNode %u ToPin %u ToNode %u\n", Connection->FromNodePin, Connection->FromNode, Connection->ToNodePin, Connection->ToNode);
        if (bNode)
        {
            if (bFrom)
            {
                if (Connection->FromNode == NodeIndex)
                {
                    /* node id has a connection */
                    Count++;
                }
            }
            else
            {
                if (Connection->ToNode == NodeIndex)
                {
                    /* node id has a connection */
                    Count++;
                }
            }
        }
        else
        {
            if (bFrom)
            {
                if (Connection->FromNodePin == NodeIndex)
                {
                    /* node id has a connection */
                    Count++;
                }
            }
            else
            {
                if (Connection->ToNodePin == NodeIndex)
                {
                    /* node id has a connection */
                    Count++;
                }
            }
        }


        /* move to next connection */
        Connection++;
    }

    ASSERT(Count != 0);

    /* now allocate node index array */
    Refs = ExAllocatePool(NonPagedPool, sizeof(ULONG) * Count);
    if (!Refs)
    {
        /* not enough memory */
        return STATUS_INSUFFICIENT_RESOURCES;
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
                if (Connection->FromNodePin == NodeIndex)
                {
                    /* node id has a connection */
                    Refs[Count] = Index;
                    Count++;
                }
            }
            else
            {
                if (Connection->ToNodePin == NodeIndex)
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

    return STATUS_SUCCESS;
}


NTSTATUS
GetTargetPinsByNodeConnectionIndex(
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN ULONG bUpDirection,
    IN ULONG NodeConnectionIndex,
    OUT PULONG Pins)
{
    PKSTOPOLOGY_CONNECTION Connection;
    ULONG PinId, NodeConnectionCount, Index;
    PULONG NodeConnection;
    NTSTATUS Status;


    /* sanity check */
    ASSERT(NodeConnectionIndex < NodeConnections->Count);

    Connection = (PKSTOPOLOGY_CONNECTION)(NodeConnections + 1);

    DPRINT("FromNode %u FromNodePin %u -> ToNode %u ToNodePin %u\n", Connection[NodeConnectionIndex].FromNode, Connection[NodeConnectionIndex].FromNodePin, Connection[NodeConnectionIndex].ToNode, Connection[NodeConnectionIndex].ToNodePin );

    if ((Connection[NodeConnectionIndex].ToNode == KSFILTER_NODE && bUpDirection == FALSE) ||
        (Connection[NodeConnectionIndex].FromNode == KSFILTER_NODE && bUpDirection == TRUE))
    {
        /* iteration stops here */
       if (bUpDirection)
           PinId = Connection[NodeConnectionIndex].FromNodePin;
       else
           PinId = Connection[NodeConnectionIndex].ToNodePin;

       DPRINT("GetTargetPinsByNodeIndex FOUND Target Pin %u Parsed %u\n", PinId, Pins[PinId]);

       /* mark pin index as a target pin */
       Pins[PinId] = TRUE;
       return STATUS_SUCCESS;
    }

    /* get all node indexes referenced by that node */
    if (bUpDirection)
    {
        Status = GetNodeIndexes(NodeConnections, Connection[NodeConnectionIndex].FromNode, TRUE, FALSE, &NodeConnectionCount, &NodeConnection);
    }
    else
    {
        Status = GetNodeIndexes(NodeConnections, Connection[NodeConnectionIndex].ToNode, TRUE, TRUE, &NodeConnectionCount, &NodeConnection);
    }

    if (NT_SUCCESS(Status))
    {
        for(Index = 0; Index < NodeConnectionCount; Index++)
        {
            /* iterate recursively into the nodes */
            Status = GetTargetPinsByNodeConnectionIndex(NodeConnections, NodeTypes, bUpDirection, NodeConnection[Index], Pins);
            ASSERT(Status == STATUS_SUCCESS);
        }
        /* free node connection indexes */
        ExFreePool(NodeConnection);
    }

    return Status;
}



NTSTATUS
GetTargetPins(
    PKSMULTIPLE_ITEM NodeTypes,
    PKSMULTIPLE_ITEM NodeConnections,
    IN ULONG NodeIndex,
    IN ULONG bUpDirection,
    PULONG Pins,
    ULONG PinCount)
{
    ULONG NodeConnectionCount, Index;
    NTSTATUS Status;
    PULONG NodeConnection;

    /* sanity check */
    ASSERT(NodeIndex != (ULONG)-1);

    /* get all node indexes referenced by that pin */
    if (bUpDirection)
        Status = GetNodeIndexes(NodeConnections, NodeIndex, TRUE, FALSE, &NodeConnectionCount, &NodeConnection);
    else
        Status = GetNodeIndexes(NodeConnections, NodeIndex, TRUE, TRUE, &NodeConnectionCount, &NodeConnection);

    DPRINT("NodeIndex %u Status %x Count %u\n", NodeIndex, Status, NodeConnectionCount);

    if (NT_SUCCESS(Status))
    {
        for(Index = 0; Index < NodeConnectionCount; Index++)
        {
            Status = GetTargetPinsByNodeConnectionIndex(NodeConnections, NodeTypes, bUpDirection, NodeConnection[Index], Pins);
            ASSERT(Status == STATUS_SUCCESS);
        }
        ExFreePool(NodeConnection);
    }

    return Status;
}

PULONG
AllocatePinArray(
    ULONG PinCount)
{
    PULONG Pins = ExAllocatePool(NonPagedPool, PinCount * sizeof(ULONG));
    if (!Pins)
        return NULL;

    RtlZeroMemory(Pins, sizeof(ULONG) * PinCount);

    return Pins;
}

NTSTATUS
AddMixerSourceLine(
    IN OUT LPMIXER_INFO MixerInfo,
    IN PFILE_OBJECT FileObject,
    IN ULONG PinId)
{
    LPMIXERLINE_EXT SrcLine, DstLine;
    NTSTATUS Status;
    KSP_PIN Pin;
    LPWSTR PinName;
    GUID NodeType;
    ULONG BytesReturned;

    /* allocate src mixer line */
    SrcLine = (LPMIXERLINE_EXT)ExAllocatePool(NonPagedPool, sizeof(MIXERLINE_EXT));
    if (!SrcLine)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* zero struct */
    RtlZeroMemory(SrcLine, sizeof(MIXERLINE_EXT));

    /* initialize mixer src line */
    SrcLine->FileObject = FileObject;
    SrcLine->PinId = PinId;
    SrcLine->Line.cbStruct = sizeof(MIXERLINEW);

    /* get destination line */
    DstLine = GetSourceMixerLineByLineId(MixerInfo, DESTINATION_LINE);

    /* initialize mixer destination line */
    SrcLine->Line.cbStruct = sizeof(MIXERLINEW);
    SrcLine->Line.dwDestination = 0;
    SrcLine->Line.dwSource = DstLine->Line.cConnections;
    SrcLine->Line.dwLineID = (DstLine->Line.cConnections * 0x10000);
    SrcLine->Line.fdwLine = MIXERLINE_LINEF_ACTIVE | MIXERLINE_LINEF_SOURCE;
    SrcLine->Line.dwUser = 0;
    SrcLine->Line.cChannels = DstLine->Line.cChannels;
    SrcLine->Line.cConnections = 0;
    SrcLine->Line.cControls = 1; //FIXME

    //HACK
    SrcLine->LineControls = ExAllocatePool(NonPagedPool, SrcLine->Line.cControls * sizeof(MIXERCONTROLW));
    if (!SrcLine->LineControls)
    {
        /* not enough memory */
        ExFreePool(SrcLine);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* clear line controls */
    RtlZeroMemory(SrcLine->LineControls, sizeof(MIXERCONTROLW));

    /* fill in pseudo mixer control */
    SrcLine->LineControls->dwControlID = 1; //FIXME
    SrcLine->LineControls->cbStruct = sizeof(MIXERCONTROLW);
    SrcLine->LineControls->fdwControl = 0;
    SrcLine->LineControls->cMultipleItems = 0;
    wcscpy(SrcLine->LineControls->szName, L"test");
    wcscpy(SrcLine->LineControls->szShortName, L"test");


    /* get pin category */
    Pin.PinId = PinId;
    Pin.Reserved = 0;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_CATEGORY;

    /* try get pin category */
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (LPVOID)&NodeType, sizeof(GUID), &BytesReturned);
    if (NT_SUCCESS(Status))
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
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), NULL, 0, &BytesReturned);

    if (Status != STATUS_MORE_ENTRIES)
    {
        SrcLine->Line.szShortName[0] = L'\0';
        SrcLine->Line.szName[0] = L'\0';
    }
    else
    {
        PinName = (LPWSTR)ExAllocatePool(NonPagedPool, BytesReturned);
        if (PinName)
        {
            /* try get pin name */
            Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (LPVOID)PinName, BytesReturned, &BytesReturned);

            if (NT_SUCCESS(Status))
            {
                RtlMoveMemory(SrcLine->Line.szShortName, PinName, (min(MIXER_SHORT_NAME_CHARS, wcslen(PinName)+1)) * sizeof(WCHAR));
                SrcLine->Line.szShortName[MIXER_SHORT_NAME_CHARS-1] = L'\0';

                RtlMoveMemory(SrcLine->Line.szName, PinName, (min(MIXER_LONG_NAME_CHARS, wcslen(PinName)+1)) * sizeof(WCHAR));
                SrcLine->Line.szName[MIXER_LONG_NAME_CHARS-1] = L'\0';
            }
            ExFreePool(PinName);
        }
    }

    SrcLine->Line.Target.dwType = 1;
    SrcLine->Line.Target.dwDeviceID = DstLine->Line.Target.dwDeviceID;
    SrcLine->Line.Target.wMid = MixerInfo->MixCaps.wMid;
    SrcLine->Line.Target.wPid = MixerInfo->MixCaps.wPid;
    SrcLine->Line.Target.vDriverVersion = MixerInfo->MixCaps.vDriverVersion;
    wcscpy(SrcLine->Line.Target.szPname, MixerInfo->MixCaps.szPname);


    /* insert src line */
    InsertTailList(&MixerInfo->LineList, &SrcLine->Entry);
    DstLine->Line.cConnections++;

    return STATUS_SUCCESS;
}


NTSTATUS
AddMixerSourceLines(
    IN OUT LPMIXER_INFO MixerInfo,
    IN PFILE_OBJECT FileObject,
    IN ULONG PinsCount,
    IN PULONG Pins)
{
    ULONG Index;
    NTSTATUS Status = STATUS_SUCCESS;

    for(Index = PinsCount; Index > 0; Index--)
    {
        if (Pins[Index-1])
        {
            AddMixerSourceLine(MixerInfo, FileObject, Index-1);
        }
    }
    return Status;
}



NTSTATUS
HandlePhysicalConnection(
    IN OUT LPMIXER_INFO MixerInfo,
    IN ULONG bInput,
    IN PKSPIN_PHYSICALCONNECTION OutConnection)
{
    PULONG PinsRef = NULL, PinConnectionIndex = NULL, PinsSrcRef;
    ULONG PinsRefCount, Index, PinConnectionIndexCount;
    NTSTATUS Status;
    HANDLE hDevice = NULL;
    PFILE_OBJECT FileObject = NULL;
    PKSMULTIPLE_ITEM NodeTypes = NULL;
    PKSMULTIPLE_ITEM NodeConnections = NULL;
    PULONG MixerControls;
    ULONG MixerControlsCount;


    /* open the connected filter */
    Status = OpenDevice(OutConnection->SymbolicLinkName, &hDevice, &FileObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenDevice failed with %x\n", Status);
        return Status;
    }

    /* get connected filter pin count */
    PinsRefCount = GetPinCount(FileObject);
    ASSERT(PinsRefCount);

    PinsRef = AllocatePinArray(PinsRefCount);
    if (!PinsRef)
    {
        /* no memory */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    /* get topology node types */
    Status = GetFilterNodeProperty(FileObject, KSPROPERTY_TOPOLOGY_NODES, &NodeTypes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetFilterNodeProperty failed with %x\n", Status);
        goto cleanup;
    }

    /* get topology connections */
    Status = GetFilterNodeProperty(FileObject, KSPROPERTY_TOPOLOGY_CONNECTIONS, &NodeConnections);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetFilterNodeProperty failed with %x\n", Status);
        goto cleanup;
    }
    /*  gets connection index of the bridge pin which connects to a node */
    DPRINT("Pin %u\n", OutConnection->Pin);
    Status = GetNodeIndexes(NodeConnections, OutConnection->Pin, FALSE, !bInput, &PinConnectionIndexCount, &PinConnectionIndex);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetNodeIndexes failed with %x\n", Status);
        goto cleanup;
    }

    /* there should be no split in the bride pin */
    ASSERT(PinConnectionIndexCount == 1);

    /* find all target pins of this connection */
    Status = GetTargetPinsByNodeConnectionIndex(NodeConnections, NodeTypes, FALSE, PinConnectionIndex[0], PinsRef);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetTargetPinsByNodeConnectionIndex failed with %x\n", Status);
        goto cleanup;
    }

    for(Index = 0; Index < PinsRefCount; Index++)
    {
        if (PinsRef[Index])
        {

            /* found a target pin, now get all references */
            Status = GetNodeIndexes(NodeConnections, Index, FALSE, FALSE, &MixerControlsCount, &MixerControls);
            if (!NT_SUCCESS(Status))
                break;

            /* sanity check */
            ASSERT(MixerControlsCount == 1);


            PinsSrcRef = AllocatePinArray(PinsRefCount);
            if (!PinsSrcRef)
            {
                /* no memory */
                ExFreePool(MixerControls);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }
            /* now get all connected source pins */
            Status = GetTargetPinsByNodeConnectionIndex(NodeConnections, NodeTypes, TRUE, MixerControls[0], PinsSrcRef);
            if (!NT_SUCCESS(Status))
            {
                /* no memory */
                ExFreePool(MixerControls);
                ExFreePool(PinsSrcRef);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }

            /* add pins from target line */
            if (!bInput)
            {
                // dont add bridge pin for input mixers
                PinsSrcRef[Index] = TRUE;
                PinsSrcRef[OutConnection->Pin] = TRUE;
            }

            Status = AddMixerSourceLines(MixerInfo, FileObject, PinsRefCount, PinsSrcRef);

            ExFreePool(MixerControls);
            ExFreePool(PinsSrcRef);
        }
    }

cleanup:

    if (PinsRef)
        ExFreePool(PinsRef);

    if (NodeConnections)
        ExFreePool(NodeConnections);

    if (NodeTypes)
        ExFreePool(NodeTypes);

    if (FileObject)
        ObDereferenceObject(FileObject);

    if (hDevice)
        ZwClose(hDevice);

    if (PinConnectionIndex)
        ExFreePool(PinConnectionIndex);


    return Status;
}



NTSTATUS
InitializeMixer(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DeviceIndex,
    IN OUT LPMIXER_INFO MixerInfo,
    IN HANDLE hDevice,
    IN PFILE_OBJECT FileObject,
    IN ULONG PinCount,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN ULONG NodeIndex,
    IN ULONG bInput)
{
    WCHAR Buffer[100];
    LPWSTR Device;
    NTSTATUS Status;
    PULONG Pins;
    ULONG Index;
    PKSPIN_PHYSICALCONNECTION OutConnection;
    LPMIXERLINE_EXT DestinationLine;

    DestinationLine = ExAllocatePool(NonPagedPool, sizeof(MIXERLINE_EXT));
    if (!DestinationLine)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize mixer info */
    MixerInfo->hMixer = hDevice;
    MixerInfo->MixerFileObject = FileObject;

    /* intialize mixer caps */
    MixerInfo->MixCaps.wMid = MM_MICROSOFT; //FIXME
    MixerInfo->MixCaps.wPid = MM_PID_UNMAPPED; //FIXME
    MixerInfo->MixCaps.vDriverVersion = 1; //FIXME
    MixerInfo->MixCaps.fdwSupport = 0;
    MixerInfo->MixCaps.cDestinations = 1;

    /* get target pnp name */
    Status = GetSysAudioDevicePnpName(DeviceObject, DeviceIndex, &Device);
    if (NT_SUCCESS(Status))
    {
        /* find product name */
        Status = FindProductName(Device, sizeof(Buffer) / sizeof(WCHAR), Buffer);
        if (NT_SUCCESS(Status))
        {
            if (bInput)
                wcscat(Buffer, L" Input");
            else
                wcscat(Buffer, L" output");
            RtlMoveMemory(MixerInfo->MixCaps.szPname, Buffer, min(MAXPNAMELEN, wcslen(Buffer)+1) * sizeof(WCHAR));
            MixerInfo->MixCaps.szPname[MAXPNAMELEN-1] = L'\0';
        }
        ExFreePool(Device);
    }

    /* initialize mixer destination line */
    RtlZeroMemory(DestinationLine, sizeof(MIXERLINEW));
    DestinationLine->Line.cbStruct = sizeof(MIXERLINEW);
    DestinationLine->Line.dwSource = MAXULONG;
    DestinationLine->Line.dwLineID = DESTINATION_LINE;
    DestinationLine->Line.fdwLine = MIXERLINE_LINEF_ACTIVE;
    DestinationLine->Line.dwUser = 0;
    DestinationLine->Line.dwComponentType = (bInput == 0 ? MIXERLINE_COMPONENTTYPE_DST_SPEAKERS : MIXERLINE_COMPONENTTYPE_DST_WAVEIN);
    DestinationLine->Line.cChannels = 2; //FIXME
    DestinationLine->Line.cControls = 0; //FIXME
    wcscpy(DestinationLine->Line.szShortName, L"Summe"); //FIXME
    wcscpy(DestinationLine->Line.szName, L"Summe"); //FIXME
    DestinationLine->Line.Target.dwType = (bInput == 0 ? MIXERLINE_TARGETTYPE_WAVEOUT : MIXERLINE_TARGETTYPE_WAVEIN);
    DestinationLine->Line.Target.dwDeviceID = !bInput;
    DestinationLine->Line.Target.wMid = MixerInfo->MixCaps.wMid;
    DestinationLine->Line.Target.wPid = MixerInfo->MixCaps.wPid;
    DestinationLine->Line.Target.vDriverVersion = MixerInfo->MixCaps.vDriverVersion;
    wcscpy(DestinationLine->Line.Target.szPname, MixerInfo->MixCaps.szPname);

    /* initialize source line list */
    InitializeListHead(&MixerInfo->LineList);

    /* insert destination line */
    InsertHeadList(&MixerInfo->LineList, &DestinationLine->Entry);

    Pins = AllocatePinArray(PinCount);
    if (!Pins)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (bInput)
    {
        Status = GetTargetPins(NodeTypes, NodeConnections, NodeIndex, TRUE, Pins, PinCount);
    }
    else
    {
        Status = GetTargetPins(NodeTypes, NodeConnections, NodeIndex, FALSE, Pins, PinCount);
    }

    for(Index = 0; Index < PinCount; Index++)
    {
        if (Pins[Index])
        {
            Status = GetPhysicalConnection(FileObject, Index, &OutConnection);
            if (NT_SUCCESS(Status))
            {
                Status = HandlePhysicalConnection(MixerInfo, bInput, OutConnection);
            }
        }
    }
    ExFreePool(Pins);

    return STATUS_SUCCESS;
}

NTSTATUS
WdmAudMixerInitialize(
    IN PDEVICE_OBJECT DeviceObject)
{
    ULONG DeviceCount, Index, Count, NodeIndex, PinCount;
    NTSTATUS Status;
    HANDLE hDevice;
    PFILE_OBJECT FileObject;
    PKSMULTIPLE_ITEM NodeTypes, NodeConnections;
    BOOL bCloseHandle;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;


    /* get number of devices */
    DeviceCount = GetSysAudioDeviceCount(DeviceObject);

    if (!DeviceCount)
    {
        /* no audio devices available atm */
        DeviceExtension->MixerInfoCount = 0;
        DeviceExtension->MixerInfo = NULL;
        return STATUS_SUCCESS;
    }

    /* each virtual audio device can at most have an input + output mixer */
    DeviceExtension->MixerInfo = ExAllocatePool(NonPagedPool, sizeof(MIXER_INFO) * DeviceCount * 2);
    if (!DeviceExtension->MixerInfo)
    {
        /* not enough memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* clear mixer info */
    RtlZeroMemory(DeviceExtension->MixerInfo, sizeof(MIXER_INFO) * DeviceCount * 2);

    Index = 0;
    Count = 0;
    do
    {
        /* open the virtual audio device */
        Status = OpenSysAudioDeviceByIndex(DeviceObject, Index, &hDevice, &FileObject);

        if (NT_SUCCESS(Status))
        {
            /* retrieve all available node types */
            Status = GetFilterNodeProperty(FileObject, KSPROPERTY_TOPOLOGY_NODES, &NodeTypes);
            if (!NT_SUCCESS(Status))
            {
                ObDereferenceObject(FileObject);
                ZwClose(hDevice);
                break;
            }

            Status = GetFilterNodeProperty(FileObject, KSPROPERTY_TOPOLOGY_CONNECTIONS, &NodeConnections);
            if (!NT_SUCCESS(Status))
            {
                ObDereferenceObject(FileObject);
                ZwClose(hDevice);
                ExFreePool(NodeTypes);
                break;
            }

            /* get num of pins */
            PinCount = GetPinCount(FileObject);
            bCloseHandle = TRUE;
            /* get the first available dac node index */
            NodeIndex = GetNodeTypeIndex(NodeTypes, (LPGUID)&KSNODETYPE_DAC);
            if (NodeIndex != (ULONG)-1)
            {
                Status = InitializeMixer(DeviceObject, Index, &DeviceExtension->MixerInfo[Count], hDevice, FileObject, PinCount, NodeTypes, NodeConnections, NodeIndex, FALSE);
                if (NT_SUCCESS(Status))
                {
                    /* increment mixer offset */
                    Count++;
                    bCloseHandle = FALSE;
                }
            }

            /* get the first available adc node index */
            NodeIndex = GetNodeTypeIndex(NodeTypes, (LPGUID)&KSNODETYPE_ADC);
            if (NodeIndex != (ULONG)-1)
            {
                Status = InitializeMixer(DeviceObject, Index, &DeviceExtension->MixerInfo[Count], hDevice, FileObject, PinCount, NodeTypes, NodeConnections, NodeIndex, TRUE);
                if (NT_SUCCESS(Status))
                {
                    /* increment mixer offset */
                    Count++;
                    bCloseHandle = FALSE;
                }
            }

            /* free node connections array */
            ExFreePool(NodeTypes);
            ExFreePool(NodeConnections);

            if (bCloseHandle)
            {
                /* close virtual audio device */
                ObDereferenceObject(FileObject);
                ZwClose(hDevice);
            }
        }
        /* increment virtual audio device index */
        Index++;
    }while(Index < DeviceCount);

    /* store mixer count */
    DeviceExtension->MixerInfoCount = Count;

    return Status;
}



NTSTATUS
WdmAudMixerCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo,
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension)
{
    if ((ULONG)DeviceInfo->hDevice >= DeviceExtension->MixerInfoCount)
    {
        /* invalid parameter */
        return STATUS_INVALID_PARAMETER;
    }

    /* copy cached mixer caps */
    RtlMoveMemory(&DeviceInfo->u.MixCaps, &DeviceExtension->MixerInfo[(ULONG)DeviceInfo->hDevice].MixCaps, sizeof(MIXERCAPSW));

    return STATUS_SUCCESS;
}


NTSTATUS
WdmAudControlOpenMixer(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    ULONG Index;
    PWDMAUD_HANDLE Handels;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    DPRINT("WdmAudControlOpenMixer\n");

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;


    if (DeviceInfo->DeviceIndex >= DeviceExtension->MixerInfoCount)
    {
        /* mixer index doesnt exist */
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    for(Index = 0; Index < ClientInfo->NumPins; Index++)
    {
        if (ClientInfo->hPins[Index].Handle == (HANDLE)DeviceInfo->DeviceIndex && ClientInfo->hPins[Index].Type == MIXER_DEVICE_TYPE)
        {
            /* re-use pseudo handle */
            DeviceInfo->hDevice = (HANDLE)DeviceInfo->DeviceIndex;
            return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
        }
    }

    Handels = ExAllocatePool(NonPagedPool, sizeof(WDMAUD_HANDLE) * (ClientInfo->NumPins+1));

    if (Handels)
    {
        if (ClientInfo->NumPins)
        {
            RtlMoveMemory(Handels, ClientInfo->hPins, sizeof(WDMAUD_HANDLE) * ClientInfo->NumPins);
            ExFreePool(ClientInfo->hPins);
        }

        ClientInfo->hPins = Handels;
        ClientInfo->hPins[ClientInfo->NumPins].Handle = (HANDLE)DeviceInfo->DeviceIndex;
        ClientInfo->hPins[ClientInfo->NumPins].Type = MIXER_DEVICE_TYPE;
        ClientInfo->NumPins++;
    }
    else
    {
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, sizeof(WDMAUD_DEVICE_INFO));
    }
    DeviceInfo->hDevice = (HANDLE)DeviceInfo->DeviceIndex;

    return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
NTAPI
WdmAudGetLineInfo(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    LPMIXERLINE_EXT MixerLineSrc;

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (DeviceInfo->Flags == MIXER_GETLINEINFOF_DESTINATION)
    {
        if ((ULONG)DeviceInfo->hDevice >= DeviceExtension->MixerInfoCount)
        {
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }

        if (DeviceInfo->u.MixLine.dwDestination != 0)
        {
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }
        MixerLineSrc = GetSourceMixerLineByLineId(&DeviceExtension->MixerInfo[(ULONG)DeviceInfo->hDevice], DESTINATION_LINE);
        ASSERT(MixerLineSrc);

        /* copy cached data */
        RtlCopyMemory(&DeviceInfo->u.MixLine, &MixerLineSrc->Line, sizeof(MIXERLINEW));
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    }
    else if (DeviceInfo->Flags == MIXER_GETLINEINFOF_SOURCE)
    {
        if ((ULONG)DeviceInfo->hDevice >= DeviceExtension->MixerInfoCount)
        {
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }

        MixerLineSrc = GetSourceMixerLineByLineId(&DeviceExtension->MixerInfo[(ULONG)DeviceInfo->hDevice], DESTINATION_LINE);
        ASSERT(MixerLineSrc);

        if (DeviceInfo->u.MixLine.dwSource >= MixerLineSrc->Line.cConnections)
        {
            DPRINT1("dwSource %u Destinations %u\n", DeviceInfo->u.MixLine.dwSource, MixerLineSrc->Line.cConnections);
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }

        MixerLineSrc = GetSourceMixerLine(&DeviceExtension->MixerInfo[(ULONG)DeviceInfo->hDevice], DeviceInfo->u.MixLine.dwSource);
        if (MixerLineSrc)
        {
            DPRINT("Line %u Name %S\n", MixerLineSrc->Line.dwSource, MixerLineSrc->Line.szName);
            RtlCopyMemory(&DeviceInfo->u.MixLine, &MixerLineSrc->Line, sizeof(MIXERLINEW));
        }
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    }
    else if (DeviceInfo->Flags == MIXER_GETLINEINFOF_LINEID)
    {
        if ((ULONG)DeviceInfo->hDevice >= DeviceExtension->MixerInfoCount)
        {
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }

        MixerLineSrc = GetSourceMixerLineByLineId(&DeviceExtension->MixerInfo[(ULONG)DeviceInfo->hDevice], DeviceInfo->u.MixLine.dwLineID);
        ASSERT(MixerLineSrc);

        /* copy cached data */
        RtlCopyMemory(&DeviceInfo->u.MixLine, &MixerLineSrc->Line, sizeof(MIXERLINEW));
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    }


    DPRINT1("Flags %x\n", DeviceInfo->Flags);
    UNIMPLEMENTED;

    //DbgBreakPoint();
    return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);

}

NTSTATUS
NTAPI
WdmAudGetLineControls(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    LPMIXERLINE_EXT MixerLineSrc;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (DeviceInfo->Flags == MIXER_GETLINECONTROLSF_ALL)
    {
        if ((ULONG)DeviceInfo->hDevice >= DeviceExtension->MixerInfoCount)
        {
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }

        MixerLineSrc = GetSourceMixerLineByLineId(&DeviceExtension->MixerInfo[(ULONG)DeviceInfo->hDevice], DeviceInfo->u.MixControls.dwLineID);
        ASSERT(MixerLineSrc);
        if (MixerLineSrc)
        {
            RtlMoveMemory(DeviceInfo->u.MixControls.pamxctrl, MixerLineSrc->LineControls, min(MixerLineSrc->Line.cControls, DeviceInfo->u.MixControls.cControls) * sizeof(MIXERLINECONTROLSW));
        }
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    }
    else if (DeviceInfo->Flags == MIXER_GETLINECONTROLSF_ONEBYTYPE)
    {
        DPRINT1("dwLineID %u\n",DeviceInfo->u.MixControls.dwLineID);
        UNIMPLEMENTED
        //HACK
        if ((ULONG)DeviceInfo->hDevice >= DeviceExtension->MixerInfoCount)
        {
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }

        MixerLineSrc = GetSourceMixerLineByLineId(&DeviceExtension->MixerInfo[(ULONG)DeviceInfo->hDevice], DeviceInfo->u.MixControls.dwLineID);
        ASSERT(MixerLineSrc);
        if (MixerLineSrc)
        {
            RtlMoveMemory(DeviceInfo->u.MixControls.pamxctrl, MixerLineSrc->LineControls, min(MixerLineSrc->Line.cControls, DeviceInfo->u.MixControls.cControls) * sizeof(MIXERLINECONTROLSW));
        }
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    }

    UNIMPLEMENTED;
    //DbgBreakPoint();
    return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);

}

NTSTATUS
NTAPI
WdmAudSetControlDetails(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    UNIMPLEMENTED;
    //DbgBreakPoint();
    return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);

}

NTSTATUS
NTAPI
WdmAudGetControlDetails(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    UNIMPLEMENTED;
    //DbgBreakPoint();
    return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);

}

