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
const GUID KSNODETYPE_SUM = {0xDA441A60L, 0xC556, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSPROPSETID_Audio = {0x45FFAAA0L, 0x6E1B, 0x11D0, {0xBC, 0xF2, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};

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

LPMIXERCONTROL_DATA
GetMixerControlDataById(
    PLIST_ENTRY ListHead,
    DWORD dwControlId)
{
    PLIST_ENTRY Entry;
    LPMIXERCONTROL_DATA Control;

    /* get first entry */
    Entry = ListHead->Flink;

    while(Entry != ListHead)
    {
        Control = (LPMIXERCONTROL_DATA)CONTAINING_RECORD(Entry, MIXERCONTROL_DATA, Entry);
        DPRINT("dwSource %x dwSource %x\n", Control->dwControlID, dwControlId);
        if (Control->dwControlID == dwControlId)
            return Control;

        Entry = Entry->Flink;
    }
    return NULL;
}

NTSTATUS
GetMixerControlById(
    LPMIXER_INFO MixerInfo,
    DWORD dwControlID,
    LPMIXERLINE_EXT *MixerLine,
    LPMIXERCONTROLW *MixerControl,
    PULONG NodeId)
{
    PLIST_ENTRY Entry;
    LPMIXERLINE_EXT MixerLineSrc;
    ULONG Index;

    /* get first entry */
    Entry = MixerInfo->LineList.Flink;

    while(Entry != &MixerInfo->LineList)
    {
        MixerLineSrc = (LPMIXERLINE_EXT)CONTAINING_RECORD(Entry, MIXERLINE_EXT, Entry);

        for(Index = 0; Index < MixerLineSrc->Line.cControls; Index++)
        {
            if (MixerLineSrc->LineControls[Index].dwControlID == dwControlID)
            {
                if (MixerLine)
                    *MixerLine = MixerLineSrc;
                if (MixerControl)
                    *MixerControl = &MixerLineSrc->LineControls[Index];
                if (NodeId)
                    *NodeId = MixerLineSrc->NodeIds[Index];
                return STATUS_SUCCESS;
            }
        }
        Entry = Entry->Flink;
    }

    return STATUS_NOT_FOUND;
}

LPMIXERLINE_EXT
GetSourceMixerLineByComponentType(
    LPMIXER_INFO MixerInfo,
    DWORD dwComponentType)
{
    PLIST_ENTRY Entry;
    LPMIXERLINE_EXT MixerLineSrc;

    /* get first entry */
    Entry = MixerInfo->LineList.Flink;

    while(Entry != &MixerInfo->LineList)
    {
        MixerLineSrc = (LPMIXERLINE_EXT)CONTAINING_RECORD(Entry, MIXERLINE_EXT, Entry);
        if (MixerLineSrc->Line.dwComponentType == dwComponentType)
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

ULONG
GetDeviceIndexFromPnpName(
    IN PDEVICE_OBJECT DeviceObject,
    LPWSTR Device)
{
    ULONG Count, Index;
    LPWSTR DeviceName;
    NTSTATUS Status;

    /* get device count */
    Count = GetSysAudioDeviceCount(DeviceObject);

    if (!Count)
        return MAXULONG;

    for(Index = 0; Index < Count; Index++)
    {
        /* get device name */
        Status = GetSysAudioDevicePnpName(DeviceObject, Index, &DeviceName);
        if (NT_SUCCESS(Status))
        {
            if (!wcsicmp(Device, DeviceName))
            {
                /* found device index */
                ExFreePool(DeviceName);
                return Index;
            }
            ExFreePool(DeviceName);
        }
    }

    return MAXULONG;
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
        //DbgPrint("FromPin %u FromNode %u ToPin %u ToNode %u\n", Connection->FromNodePin, Connection->FromNode, Connection->ToNodePin, Connection->ToNode);
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
                if (Connection->FromNodePin == NodeIndex && Connection->FromNode == KSFILTER_NODE)
                {
                    /* node id has a connection */
                    Count++;
                }
            }
            else
            {
                if (Connection->ToNodePin == NodeIndex && Connection->ToNode == KSFILTER_NODE)
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

    /* clear node index array */
    RtlZeroMemory(Refs, Count * sizeof(ULONG));

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

PKSTOPOLOGY_CONNECTION
GetConnectionByIndex(
    IN PKSMULTIPLE_ITEM MultipleItem,
    IN ULONG Index)
{
    PKSTOPOLOGY_CONNECTION Descriptor;

    ASSERT(Index < MultipleItem->Count);

    Descriptor = (PKSTOPOLOGY_CONNECTION)(MultipleItem + 1);
    return &Descriptor[Index];
}

LPGUID
GetNodeType(
    IN PKSMULTIPLE_ITEM MultipleItem,
    IN ULONG Index)
{
    LPGUID NodeType;

    ASSERT(Index < MultipleItem->Count);

    NodeType = (LPGUID)(MultipleItem + 1);
    return &NodeType[Index];
}

NTSTATUS
GetControlsFromPinByConnectionIndex(
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN ULONG bUpDirection,
    IN ULONG NodeConnectionIndex,
    OUT PULONG Nodes)
{
    PKSTOPOLOGY_CONNECTION CurConnection;
    LPGUID NodeType;
    ULONG NodeIndex;
    NTSTATUS Status;
    ULONG NodeConnectionCount, Index;
    PULONG NodeConnection;


    /* get current connection */
    CurConnection = GetConnectionByIndex(NodeConnections, NodeConnectionIndex);

    if (bUpDirection)
        NodeIndex = CurConnection->FromNode;
    else
        NodeIndex = CurConnection->ToNode;

    /* get target node type of current connection */
    NodeType = GetNodeType(NodeTypes, NodeIndex);

    if (IsEqualGUIDAligned(NodeType, &KSNODETYPE_SUM) || IsEqualGUIDAligned(NodeType, &KSNODETYPE_MUX))
    {
        if (bUpDirection)
        {
            /* add the sum / mux node to destination line */
            Nodes[NodeIndex] = TRUE;
        }

        return STATUS_SUCCESS;
    }

    /* now add the node */
    Nodes[NodeIndex] = TRUE;


    /* get all node indexes referenced by that node */
    if (bUpDirection)
    {
        Status = GetNodeIndexes(NodeConnections, NodeIndex, TRUE, FALSE, &NodeConnectionCount, &NodeConnection);
    }
    else
    {
        Status = GetNodeIndexes(NodeConnections, NodeIndex, TRUE, TRUE, &NodeConnectionCount, &NodeConnection);
    }

    if (NT_SUCCESS(Status))
    {
        for(Index = 0; Index < NodeConnectionCount; Index++)
        {
            /* iterate recursively into the nodes */
            Status = GetControlsFromPinByConnectionIndex(NodeConnections, NodeTypes, bUpDirection, NodeConnection[Index], Nodes);
            ASSERT(Status == STATUS_SUCCESS);
        }
        /* free node connection indexes */
        ExFreePool(NodeConnection);
    }

    return Status;
}

NTSTATUS
GetControlsFromPin(
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN ULONG PinId,
    IN ULONG bUpDirection,
    OUT PULONG Nodes)
{
    ULONG NodeConnectionCount, Index;
    NTSTATUS Status;
    PULONG NodeConnection;

    /* sanity check */
    ASSERT(PinId != (ULONG)-1);

    /* get all node indexes referenced by that pin */
    if (bUpDirection)
        Status = GetNodeIndexes(NodeConnections, PinId, FALSE, FALSE, &NodeConnectionCount, &NodeConnection);
    else
        Status = GetNodeIndexes(NodeConnections, PinId, FALSE, TRUE, &NodeConnectionCount, &NodeConnection);

    for(Index = 0; Index < NodeConnectionCount; Index++)
    {
        /* get all associated controls */
        Status = GetControlsFromPinByConnectionIndex(NodeConnections, NodeTypes, bUpDirection, NodeConnection[Index], Nodes);
    }

    ExFreePool(NodeConnection);

    return Status;
}

NTSTATUS
AddMixerControl(
    IN LPMIXER_INFO MixerInfo,
    IN PFILE_OBJECT FileObject,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN ULONG NodeIndex,
    IN LPMIXERLINE_EXT MixerLine,
    OUT LPMIXERCONTROLW MixerControl)
{
    LPGUID NodeType;
    KSP_NODE Node;
    ULONG BytesReturned;
    NTSTATUS Status;
    LPWSTR Name;

    /* initialize mixer control */
    MixerControl->cbStruct = sizeof(MIXERCONTROLW);
    MixerControl->dwControlID = MixerInfo->ControlId;

    /* get node type */
    NodeType = GetNodeType(NodeTypes, NodeIndex);
    /* store control type */
    MixerControl->dwControlType = GetControlTypeFromTopologyNode(NodeType);

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
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), NULL, 0, &BytesReturned);

    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        ASSERT(BytesReturned != 0);
        Name = ExAllocatePool(NonPagedPool, BytesReturned);
        if (!Name)
        {
            /* not enough memory */
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* get node name */
        Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), (LPVOID)Name, BytesReturned, &BytesReturned);
        if (NT_SUCCESS(Status))
        {
            RtlMoveMemory(MixerControl->szShortName, Name, (min(MIXER_SHORT_NAME_CHARS, wcslen(Name)+1)) * sizeof(WCHAR));
            MixerControl->szShortName[MIXER_SHORT_NAME_CHARS-1] = L'\0';

            RtlMoveMemory(MixerControl->szName, Name, (min(MIXER_LONG_NAME_CHARS, wcslen(Name)+1)) * sizeof(WCHAR));
            MixerControl->szName[MIXER_LONG_NAME_CHARS-1] = L'\0';
        }

        /* free name buffer */
        ExFreePool(Name);
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
        Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSNODEPROPERTY), (PVOID)&PinId, sizeof(ULONG), &BytesReturned);

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
        Desc = ExAllocatePool(NonPagedPool, Length);
        ASSERT(Desc);
        RtlZeroMemory(Desc, Length);

        /* setup the request */
        RtlZeroMemory(&Property, sizeof(KSNODEPROPERTY_AUDIO_CHANNEL));

        Property.NodeProperty.NodeId = NodeIndex;
        Property.NodeProperty.Property.Id = KSPROPERTY_AUDIO_VOLUMELEVEL;
        Property.NodeProperty.Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;
        Property.NodeProperty.Property.Set = KSPROPSETID_Audio;

        /* get node volume level info */
        Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSNODEPROPERTY_AUDIO_CHANNEL), Desc, Length, &BytesReturned);

        if (NT_SUCCESS(Status))
        {
            LPMIXERVOLUME_DATA VolumeData;
            ULONG Steps, MaxRange, Index;
            LONG Value;

            Members = (PKSPROPERTY_MEMBERSHEADER)(Desc + 1);
            Range = (PKSPROPERTY_STEPPING_LONG)(Members + 1); //98304

            DPRINT("NodeIndex %u Range Min %d Max %d Steps %x UMin %x UMax %x\n", NodeIndex, Range->Bounds.SignedMinimum, Range->Bounds.SignedMaximum, Range->SteppingDelta, Range->Bounds.UnsignedMinimum, Range->Bounds.UnsignedMaximum);

            MaxRange = Range->Bounds.UnsignedMaximum  - Range->Bounds.UnsignedMinimum;

            if (MaxRange)
            {
                ASSERT(MaxRange);
                VolumeData = ExAllocatePool(NonPagedPool, sizeof(MIXERVOLUME_DATA));
                if (!VolumeData)
                    return STATUS_INSUFFICIENT_RESOURCES;

                Steps = MaxRange / Range->SteppingDelta + 1;

                /* store mixer control info there */
                VolumeData->Header.dwControlID = MixerControl->dwControlID;
                VolumeData->SignedMaximum = Range->Bounds.SignedMaximum;
                VolumeData->SignedMinimum = Range->Bounds.SignedMinimum;
                VolumeData->SteppingDelta = Range->SteppingDelta;
                VolumeData->ValuesCount = Steps;
                VolumeData->InputSteppingDelta = 0x10000 / Steps;

                VolumeData->Values = ExAllocatePool(NonPagedPool, sizeof(LONG) * Steps);
                if (!VolumeData->Values)
                {
                    ExFreePool(Desc);
                    ExFreePool(VolumeData);

                    return STATUS_INSUFFICIENT_RESOURCES;
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
       ExFreePool(Desc);
    }


    DPRINT("Status %x Name %S\n", Status, MixerControl->szName);
    return STATUS_SUCCESS;
}

NTSTATUS
AddMixerSourceLine(
    IN OUT LPMIXER_INFO MixerInfo,
    IN PFILE_OBJECT FileObject,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN ULONG DeviceIndex,
    IN ULONG PinId,
    IN ULONG bBridgePin,
    IN ULONG bTargetPin)
{
    LPMIXERLINE_EXT SrcLine, DstLine;
    NTSTATUS Status;
    KSP_PIN Pin;
    LPWSTR PinName;
    GUID NodeType;
    ULONG BytesReturned, ControlCount, Index;
    PULONG Nodes;

    if (!bTargetPin)
    {
        /* allocate src mixer line */
        SrcLine = (LPMIXERLINE_EXT)ExAllocatePool(NonPagedPool, sizeof(MIXERLINE_EXT));

        if (!SrcLine)
            return STATUS_INSUFFICIENT_RESOURCES;

        /* zero struct */
        RtlZeroMemory(SrcLine, sizeof(MIXERLINE_EXT));

    }
    else
    {
        ASSERT(!IsListEmpty(&MixerInfo->LineList));
        SrcLine = GetSourceMixerLineByLineId(MixerInfo, DESTINATION_LINE);
    }

    /* get destination line */
    DstLine = GetSourceMixerLineByLineId(MixerInfo, DESTINATION_LINE);
    ASSERT(DstLine);


    if (!bTargetPin)
    {
        /* initialize mixer src line */
        SrcLine->DeviceIndex = DeviceIndex;
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
    Nodes = ExAllocatePool(NonPagedPool, sizeof(ULONG) * NodeTypes->Count);

    if (!Nodes)
    {
        /* not enough memory */
        if (!bTargetPin)
        {
            ExFreePool(SrcLine);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* clear nodes array */
    RtlZeroMemory(Nodes, sizeof(ULONG) * NodeTypes->Count);

    Status = GetControlsFromPin(NodeConnections, NodeTypes, PinId, bTargetPin, Nodes);
    if (!NT_SUCCESS(Status))
    {
        /* something went wrong */
        if (!bTargetPin)
        {
            ExFreePool(SrcLine);
        }
        ExFreePool(Nodes);
        return Status;
    }

    /* now count all nodes controlled by that pin */
    ControlCount = 0;
    for(Index = 0; Index < NodeTypes->Count; Index++)
    {
        if (Nodes[Index])
            ControlCount++;
    }

    /* now allocate the line controls */
    if (ControlCount)
    {
        SrcLine->LineControls = ExAllocatePool(NonPagedPool, sizeof(MIXERCONTROLW) * ControlCount);

        if (!SrcLine->LineControls)
        {
            /* no memory available */
            if (!bTargetPin)
            {
                ExFreePool(SrcLine);
            }
            ExFreePool(Nodes);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        SrcLine->NodeIds = ExAllocatePool(NonPagedPool, sizeof(ULONG) * ControlCount);
        if (!SrcLine->NodeIds)
        {
            /* no memory available */
            ExFreePool(SrcLine->LineControls);
            if (!bTargetPin)
            {
                ExFreePool(SrcLine);
            }
            ExFreePool(Nodes);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* zero line controls */
        RtlZeroMemory(SrcLine->LineControls, sizeof(MIXERCONTROLW) * ControlCount);
        RtlZeroMemory(SrcLine->NodeIds, sizeof(ULONG) * ControlCount);

        ControlCount = 0;
        for(Index = 0; Index < NodeTypes->Count; Index++)
        {
            if (Nodes[Index])
            {
                /* store the node index for retrieving / setting details */
                SrcLine->NodeIds[ControlCount] = Index;

                Status = AddMixerControl(MixerInfo, FileObject, NodeTypes, Index, SrcLine, &SrcLine->LineControls[ControlCount]);
                if (NT_SUCCESS(Status))
                {
                    /* increment control count on success */
                    ControlCount++;
                }
            }
        }
        /* store control count */
        SrcLine->Line.cControls = ControlCount;
    }

    /* release nodes array */
    ExFreePool(Nodes);

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

    /* insert src line */
    if (!bTargetPin)
    {
        InsertTailList(&MixerInfo->LineList, &SrcLine->Entry);
        DstLine->Line.cConnections++;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
AddMixerSourceLines(
    IN OUT LPMIXER_INFO MixerInfo,
    IN PFILE_OBJECT FileObject,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN ULONG DeviceIndex,
    IN ULONG PinsCount,
    IN ULONG BridgePinIndex,
    IN ULONG TargetPinIndex,
    IN PULONG Pins)
{
    ULONG Index;
    NTSTATUS Status = STATUS_SUCCESS;

    for(Index = PinsCount; Index > 0; Index--)
    {
        if (Pins[Index-1])
        {
            AddMixerSourceLine(MixerInfo, FileObject, NodeConnections, NodeTypes, DeviceIndex, Index-1, (Index -1 == BridgePinIndex), (Index -1 == TargetPinIndex));
        }
    }
    return Status;
}



NTSTATUS
HandlePhysicalConnection(
    IN OUT LPMIXER_INFO MixerInfo,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG bInput,
    IN PKSPIN_PHYSICALCONNECTION OutConnection)
{
    PULONG PinsRef = NULL, PinConnectionIndex = NULL, PinsSrcRef;
    ULONG PinsRefCount, Index, PinConnectionIndexCount, DeviceIndex;
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

    /* get device index */
    DeviceIndex = GetDeviceIndexFromPnpName(DeviceObject, OutConnection->SymbolicLinkName);

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
            PinsSrcRef[OutConnection->Pin] = TRUE;

            Status = AddMixerSourceLines(MixerInfo, FileObject, NodeConnections, NodeTypes, DeviceIndex, PinsRefCount, OutConnection->Pin, Index, PinsSrcRef);

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

    /* intialize mixer caps */
    MixerInfo->MixCaps.wMid = MM_MICROSOFT; //FIXME
    MixerInfo->MixCaps.wPid = MM_PID_UNMAPPED; //FIXME
    MixerInfo->MixCaps.vDriverVersion = 1; //FIXME
    MixerInfo->MixCaps.fdwSupport = 0;
    MixerInfo->MixCaps.cDestinations = 1;
    MixerInfo->DeviceIndex = DeviceIndex;

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
    RtlZeroMemory(DestinationLine, sizeof(MIXERLINE_EXT));
    DestinationLine->Line.cbStruct = sizeof(MIXERLINEW);
    DestinationLine->Line.dwSource = MAXULONG;
    DestinationLine->Line.dwLineID = DESTINATION_LINE;
    DestinationLine->Line.fdwLine = MIXERLINE_LINEF_ACTIVE;
    DestinationLine->Line.dwUser = 0;
    DestinationLine->Line.dwComponentType = (bInput == 0 ? MIXERLINE_COMPONENTTYPE_DST_SPEAKERS : MIXERLINE_COMPONENTTYPE_DST_WAVEIN);
    DestinationLine->Line.cChannels = 2; //FIXME
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
    InitializeListHead(&DestinationLine->LineControlsExtraData);

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
                Status = HandlePhysicalConnection(MixerInfo, DeviceObject, bInput, OutConnection);
                ExFreePool(OutConnection);
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
            /* get the first available dac node index */
            NodeIndex = GetNodeTypeIndex(NodeTypes, (LPGUID)&KSNODETYPE_DAC);
            if (NodeIndex != (ULONG)-1)
            {
                Status = InitializeMixer(DeviceObject, Index, &DeviceExtension->MixerInfo[Count], hDevice, FileObject, PinCount, NodeTypes, NodeConnections, NodeIndex, FALSE);
                if (NT_SUCCESS(Status))
                {
                    /* increment mixer offset */
                    Count++;
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
                }
            }

            /* free node connections array */
            ExFreePool(NodeTypes);
            ExFreePool(NodeConnections);

            /* close virtual audio device */
            ObDereferenceObject(FileObject);
            ZwClose(hDevice);

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
    if ((ULONG)DeviceInfo->DeviceIndex >= DeviceExtension->MixerInfoCount)
    {
        /* invalid parameter */
        return STATUS_INVALID_PARAMETER;
    }

    /* copy cached mixer caps */
    RtlMoveMemory(&DeviceInfo->u.MixCaps, &DeviceExtension->MixerInfo[(ULONG)DeviceInfo->DeviceIndex].MixCaps, sizeof(MIXERCAPSW));

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

    DeviceInfo->Flags &= ~MIXER_OBJECTF_HMIXER;

    if (DeviceInfo->Flags == MIXER_GETLINEINFOF_DESTINATION)
    {
        if ((ULONG_PTR)DeviceInfo->hDevice >= DeviceExtension->MixerInfoCount)
        {
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }

        if (DeviceInfo->u.MixLine.dwDestination != 0)
        {
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }
        MixerLineSrc = GetSourceMixerLineByLineId(&DeviceExtension->MixerInfo[(ULONG_PTR)DeviceInfo->hDevice], DESTINATION_LINE);
        ASSERT(MixerLineSrc);

        /* copy cached data */
        RtlCopyMemory(&DeviceInfo->u.MixLine, &MixerLineSrc->Line, sizeof(MIXERLINEW));
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    }
    else if (DeviceInfo->Flags == MIXER_GETLINEINFOF_SOURCE)
    {
        if ((ULONG_PTR)DeviceInfo->hDevice >= DeviceExtension->MixerInfoCount)
        {
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }

        MixerLineSrc = GetSourceMixerLineByLineId(&DeviceExtension->MixerInfo[(ULONG_PTR)DeviceInfo->hDevice], DESTINATION_LINE);
        ASSERT(MixerLineSrc);

        if (DeviceInfo->u.MixLine.dwSource >= MixerLineSrc->Line.cConnections)
        {
            DPRINT1("dwSource %u Destinations %u\n", DeviceInfo->u.MixLine.dwSource, MixerLineSrc->Line.cConnections);
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }

        MixerLineSrc = GetSourceMixerLine(&DeviceExtension->MixerInfo[(ULONG_PTR)DeviceInfo->hDevice], DeviceInfo->u.MixLine.dwSource);
        if (MixerLineSrc)
        {
            DPRINT("Line %u Name %S\n", MixerLineSrc->Line.dwSource, MixerLineSrc->Line.szName);
            RtlCopyMemory(&DeviceInfo->u.MixLine, &MixerLineSrc->Line, sizeof(MIXERLINEW));
        }
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    }
    else if (DeviceInfo->Flags == MIXER_GETLINEINFOF_LINEID)
    {
        if ((ULONG_PTR)DeviceInfo->hDevice >= DeviceExtension->MixerInfoCount)
        {
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }

        MixerLineSrc = GetSourceMixerLineByLineId(&DeviceExtension->MixerInfo[(ULONG_PTR)DeviceInfo->hDevice], DeviceInfo->u.MixLine.dwLineID);
        if (!MixerLineSrc)
        {
            DPRINT1("Failed to find Line with id %u\n", DeviceInfo->u.MixLine.dwLineID);
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }

        /* copy cached data */
        RtlCopyMemory(&DeviceInfo->u.MixLine, &MixerLineSrc->Line, sizeof(MIXERLINEW));
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    }
    else if (DeviceInfo->Flags == MIXER_GETLINEINFOF_COMPONENTTYPE)
    {
        if ((ULONG_PTR)DeviceInfo->hDevice >= DeviceExtension->MixerInfoCount)
        {
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }

        MixerLineSrc = GetSourceMixerLineByComponentType(&DeviceExtension->MixerInfo[(ULONG_PTR)DeviceInfo->hDevice], DeviceInfo->u.MixLine.dwComponentType);
        if (!MixerLineSrc)
        {
            DPRINT1("Failed to find component type %x\n", DeviceInfo->u.MixLine.dwComponentType);
            return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
        }

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
    LPMIXERCONTROLW MixerControl;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    ULONG Index;
    NTSTATUS Status;

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    DeviceInfo->Flags &= ~MIXER_OBJECTF_HMIXER;

    if (DeviceInfo->Flags == MIXER_GETLINECONTROLSF_ALL)
    {
        if ((ULONG_PTR)DeviceInfo->hDevice >= DeviceExtension->MixerInfoCount)
        {
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }

        MixerLineSrc = GetSourceMixerLineByLineId(&DeviceExtension->MixerInfo[(ULONG_PTR)DeviceInfo->hDevice], DeviceInfo->u.MixControls.dwLineID);
        ASSERT(MixerLineSrc);
        if (MixerLineSrc)
        {
            RtlMoveMemory(DeviceInfo->u.MixControls.pamxctrl, MixerLineSrc->LineControls, min(MixerLineSrc->Line.cControls, DeviceInfo->u.MixControls.cControls) * sizeof(MIXERCONTROLW));
        }
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    }
    else if (DeviceInfo->Flags == MIXER_GETLINECONTROLSF_ONEBYTYPE)
    {
        if ((ULONG_PTR)DeviceInfo->hDevice >= DeviceExtension->MixerInfoCount)
        {
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }

        MixerLineSrc = GetSourceMixerLineByLineId(&DeviceExtension->MixerInfo[(ULONG_PTR)DeviceInfo->hDevice], DeviceInfo->u.MixControls.dwLineID);
        ASSERT(MixerLineSrc);

        Index = 0;
        for(Index = 0; Index < MixerLineSrc->Line.cControls; Index++)
        {
            DPRINT1("dwControlType %x\n", MixerLineSrc->LineControls[Index].dwControlType);
            if (DeviceInfo->u.MixControls.dwControlType == MixerLineSrc->LineControls[Index].dwControlType)
            {
                RtlMoveMemory(DeviceInfo->u.MixControls.pamxctrl, &MixerLineSrc->LineControls[Index], sizeof(MIXERCONTROLW));
                return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
            }
        }
        DPRINT1("DeviceInfo->u.MixControls.dwControlType %x not found in Line %x cControls %u \n", DeviceInfo->u.MixControls.dwControlType, DeviceInfo->u.MixControls.dwLineID, MixerLineSrc->Line.cControls);
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, sizeof(WDMAUD_DEVICE_INFO));
    }
    else if (DeviceInfo->Flags == MIXER_GETLINECONTROLSF_ONEBYID)
    {
        if ((ULONG_PTR)DeviceInfo->hDevice >= DeviceExtension->MixerInfoCount)
        {
            /* invalid parameter */
            return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
        }
        DPRINT1("MixerId %u ControlId %u\n",(ULONG_PTR)DeviceInfo->hDevice,  DeviceInfo->u.MixControls.dwControlID);
        Status = GetMixerControlById(&DeviceExtension->MixerInfo[(ULONG_PTR)DeviceInfo->hDevice], DeviceInfo->u.MixControls.dwControlID, NULL, &MixerControl, NULL);
        if (NT_SUCCESS(Status))
        {
            RtlMoveMemory(DeviceInfo->u.MixControls.pamxctrl, MixerControl, sizeof(MIXERCONTROLW));
        }
        return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
    }

    UNIMPLEMENTED;
    //DbgBreakPoint();
    return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);

}

NTSTATUS
SetGetControlDetails(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DeviceId,
    IN ULONG NodeId,
    IN PWDMAUD_DEVICE_INFO DeviceInfo,
    IN ULONG bSet,
    IN ULONG PropertyId,
    IN ULONG Channel,
    IN PLONG InputValue)
{
    KSNODEPROPERTY_AUDIO_CHANNEL Property;
    NTSTATUS Status;
    HANDLE hDevice;
    PFILE_OBJECT FileObject;
    LONG Value;
    ULONG BytesReturned;

    if (bSet)
        Value = *InputValue;

    /* open virtual audio device */
    Status = OpenSysAudioDeviceByIndex(DeviceObject, DeviceId, &hDevice, &FileObject);

    if (!NT_SUCCESS(Status))
    {
        /* failed */
        return Status;
    }

    /* setup the request */
    RtlZeroMemory(&Property, sizeof(KSNODEPROPERTY_AUDIO_CHANNEL));

    Property.NodeProperty.NodeId = NodeId;
    Property.NodeProperty.Property.Id = PropertyId;
    Property.NodeProperty.Property.Flags = KSPROPERTY_TYPE_TOPOLOGY;
    Property.NodeProperty.Property.Set = KSPROPSETID_Audio;
    Property.Channel = Channel;

    if (bSet)
        Property.NodeProperty.Property.Flags |= KSPROPERTY_TYPE_SET;
    else
        Property.NodeProperty.Property.Flags |= KSPROPERTY_TYPE_GET;

    /* send the request */
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSNODEPROPERTY_AUDIO_CHANNEL), (PVOID)&Value, sizeof(LONG), &BytesReturned);

    ObDereferenceObject(FileObject);
    ZwClose(hDevice);

    if (!bSet)
    {
        *InputValue = Value;
    }

    DPRINT1("Status %x bSet %u NodeId %u Value %d PropertyId %u\n", Status, bSet, NodeId, Value, PropertyId);
    return Status;
}

NTSTATUS
SetGetMuteControlDetails(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DeviceId,
    IN ULONG NodeId,
    IN PWDMAUD_DEVICE_INFO DeviceInfo,
    IN ULONG bSet)
{
    LPMIXERCONTROLDETAILS_BOOLEAN Input;
    LONG Value;
    NTSTATUS Status;

    if (DeviceInfo->u.MixDetails.cbDetails != sizeof(MIXERCONTROLDETAILS_BOOLEAN))
        return STATUS_INVALID_PARAMETER;

    /* get input */
    Input = (LPMIXERCONTROLDETAILS_BOOLEAN)DeviceInfo->u.MixDetails.paDetails;

    /* FIXME SEH */
    if (bSet)
        Value = Input->fValue;

    /* set control details */
    Status = SetGetControlDetails(DeviceObject, DeviceId, NodeId, DeviceInfo, bSet, KSPROPERTY_AUDIO_MUTE, MAXULONG, &Value);

    /* FIXME SEH */
    if (!bSet)
        Input->fValue = Value;

    return Status;
}

NTSTATUS
SetGetVolumeControlDetails(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DeviceId,
    IN ULONG NodeId,
    IN PWDMAUD_DEVICE_INFO DeviceInfo,
    IN ULONG bSet,
    LPMIXERCONTROLW MixerControl,
    LPMIXERLINE_EXT MixerLine)
{
    LPMIXERCONTROLDETAILS_UNSIGNED Input;
    LONG Value, Index, Channel = 0;
    NTSTATUS Status;
    LPMIXERVOLUME_DATA VolumeData;

    if (DeviceInfo->u.MixDetails.cbDetails != sizeof(MIXERCONTROLDETAILS_SIGNED))
        return STATUS_INVALID_PARAMETER;

    VolumeData = (LPMIXERVOLUME_DATA)GetMixerControlDataById(&MixerLine->LineControlsExtraData, MixerControl->dwControlID);
    if (!VolumeData)
        return STATUS_INSUFFICIENT_RESOURCES;


    /* get input */
    Input = (LPMIXERCONTROLDETAILS_UNSIGNED)DeviceInfo->u.MixDetails.paDetails;

    if (bSet)
    {
        /* FIXME SEH */
        Value = Input->dwValue;
        Index = Value / VolumeData->InputSteppingDelta;

        if (Index >= VolumeData->ValuesCount)
        {
            DPRINT1("Index %u out of bounds %u \n", Index, VolumeData->ValuesCount);
            DbgBreakPoint();
            return STATUS_INVALID_PARAMETER;
        }

        Value = VolumeData->Values[Index];
    }

    /* set control details */
    if (bSet)
    {
        Status = SetGetControlDetails(DeviceObject, DeviceId, NodeId, DeviceInfo, bSet, KSPROPERTY_AUDIO_VOLUMELEVEL, 0, &Value);
        Status = SetGetControlDetails(DeviceObject, DeviceId, NodeId, DeviceInfo, bSet, KSPROPERTY_AUDIO_VOLUMELEVEL, 1, &Value);
    }
    else
    {
        Status = SetGetControlDetails(DeviceObject, DeviceId, NodeId, DeviceInfo, bSet, KSPROPERTY_AUDIO_VOLUMELEVEL, Channel, &Value);
    }

    if (!bSet)
    {
        for(Index = 0; Index < VolumeData->ValuesCount; Index++)
        {
            if (VolumeData->Values[Index] > Value)
            {
                /* FIXME SEH */
                Input->dwValue = VolumeData->InputSteppingDelta * Index;
                return Status;
            }
        }
        Input->dwValue = VolumeData->InputSteppingDelta * (VolumeData->ValuesCount-1);
    }

    return Status;
}

NTSTATUS
NTAPI
WdmAudSetControlDetails(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    LPMIXERLINE_EXT MixerLine;
    LPMIXERCONTROLW MixerControl;
    ULONG NodeId;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    DeviceInfo->Flags &= ~MIXER_OBJECTF_HMIXER;

    DPRINT("cbStruct %u Expected %u dwControlID %u cChannels %u cMultipleItems %u cbDetails %u paDetails %p Flags %x\n", 
            DeviceInfo->u.MixDetails.cbStruct, sizeof(MIXERCONTROLDETAILS), DeviceInfo->u.MixDetails.dwControlID, DeviceInfo->u.MixDetails.cChannels, DeviceInfo->u.MixDetails.cMultipleItems, DeviceInfo->u.MixDetails.cbDetails, DeviceInfo->u.MixDetails.paDetails, DeviceInfo->Flags);

    if (DeviceInfo->Flags & MIXER_GETCONTROLDETAILSF_LISTTEXT)
    {
        UNIMPLEMENTED;
        return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);
    }

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* get mixer control */
     Status = GetMixerControlById(&DeviceExtension->MixerInfo[(ULONG_PTR)DeviceInfo->hDevice], DeviceInfo->u.MixDetails.dwControlID, &MixerLine, &MixerControl, &NodeId);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MixerControl %x not found\n", DeviceInfo->u.MixDetails.dwControlID);
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    Status = STATUS_NOT_IMPLEMENTED;
    DPRINT("dwLineId %x dwControlID %x dwControlType %x\n", MixerLine->Line.dwLineID, MixerControl->dwControlID, MixerControl->dwControlType);
    if (MixerControl->dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE)
    {
        /* send the request */
        Status = SetGetMuteControlDetails(DeviceObject, MixerLine->DeviceIndex, NodeId, DeviceInfo, TRUE);
    }
    else if (MixerControl->dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME)
    {
        Status = SetGetVolumeControlDetails(DeviceObject, MixerLine->DeviceIndex, NodeId, DeviceInfo, TRUE, MixerControl, MixerLine);
    }
    return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));

}

NTSTATUS
NTAPI
WdmAudGetControlDetails(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    LPMIXERLINE_EXT MixerLine;
    LPMIXERCONTROLW MixerControl;
    ULONG NodeId;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    DeviceInfo->Flags &= ~MIXER_OBJECTF_HMIXER;

    DPRINT("cbStruct %u Expected %u dwControlID %u cChannels %u cMultipleItems %u cbDetails %u paDetails %p Flags %x\n", 
            DeviceInfo->u.MixDetails.cbStruct, sizeof(MIXERCONTROLDETAILS), DeviceInfo->u.MixDetails.dwControlID, DeviceInfo->u.MixDetails.cChannels, DeviceInfo->u.MixDetails.cMultipleItems, DeviceInfo->u.MixDetails.cbDetails, DeviceInfo->u.MixDetails.paDetails, DeviceInfo->Flags);

    if (DeviceInfo->Flags & MIXER_GETCONTROLDETAILSF_LISTTEXT)
    {
        UNIMPLEMENTED;
        return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);
    }

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* get mixer control */
     Status = GetMixerControlById(&DeviceExtension->MixerInfo[(ULONG_PTR)DeviceInfo->hDevice], DeviceInfo->u.MixDetails.dwControlID, &MixerLine, &MixerControl, &NodeId);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MixerControl %x not found\n", DeviceInfo->u.MixDetails.dwControlID);
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    Status = STATUS_NOT_IMPLEMENTED;
    DPRINT("dwLineId %x dwControlID %x dwControlType %x\n", MixerLine->Line.dwLineID, MixerControl->dwControlID, MixerControl->dwControlType);
    if (MixerControl->dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE)
    {
        /* send the request */
        Status = SetGetMuteControlDetails(DeviceObject, MixerLine->DeviceIndex, NodeId, DeviceInfo, FALSE);
    }
    else if (MixerControl->dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME)
    {
        Status = SetGetVolumeControlDetails(DeviceObject, MixerLine->DeviceIndex, NodeId, DeviceInfo, FALSE, MixerControl, MixerLine);
    }

    return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));

}

