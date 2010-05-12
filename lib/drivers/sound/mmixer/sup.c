/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/sup.c
 * PURPOSE:         Mixer Support Functions
 * PROGRAMMER:      Johannes Anderwald
 */



#include "priv.h"

const GUID KSNODETYPE_SUM = {0xDA441A60L, 0xC556, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
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

const GUID KSPROPSETID_Audio = {0x45FFAAA0L, 0x6E1B, 0x11D0, {0xBC, 0xF2, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
const GUID KSPROPSETID_Pin                     = {0x8C134960L, 0x51AD, 0x11CF, {0x87, 0x8A, 0x94, 0xF8, 0x01, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_General                  = {0x1464EDA5L, 0x6A8F, 0x11D1, {0x9A, 0xA7, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSPROPSETID_Topology                 = {0x720D4AC0L, 0x7533, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSEVENTSETID_AudioControlChange      = {0xE85E9698L, 0xFA2F, 0x11D1, {0x95, 0xBD, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3}};

MIXER_STATUS
MMixerVerifyContext(
    IN PMIXER_CONTEXT MixerContext)
{
    if (MixerContext->SizeOfStruct != sizeof(MIXER_CONTEXT))
        return MM_STATUS_INVALID_PARAMETER;

    if (!MixerContext->Alloc || !MixerContext->Control || !MixerContext->Free || !MixerContext->Open ||
        !MixerContext->AllocEventData || !MixerContext->FreeEventData ||
        !MixerContext->Close || !MixerContext->OpenKey || !MixerContext->QueryKeyValue || !MixerContext->CloseKey)
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

LPMIXER_INFO
MMixerGetMixerInfoByIndex(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG MixerIndex)
{
    LPMIXER_INFO MixerInfo;
    PLIST_ENTRY Entry;
    PMIXER_LIST MixerList;
    ULONG Index = 0;

    // get mixer list
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    if (!MixerList->MixerListCount)
        return NULL;

    Entry = MixerList->MixerList.Flink;

    while(Entry != &MixerList->MixerList)
    {
        MixerInfo = (LPMIXER_INFO)CONTAINING_RECORD(Entry, MIXER_INFO, Entry);

        if (Index == MixerIndex)
            return MixerInfo;

        // move to next mixer entry
        Index++;
        Entry = Entry->Flink;
    }

    return NULL;
}

LPMIXERCONTROL_DATA
MMixerGetMixerControlDataById(
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
        DPRINT("dwLineID %x dwLineID %x MixerLineSrc %p\n", MixerLineSrc->Line.dwLineID, dwLineID, MixerLineSrc);
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

    if (!Count)
    {
        *NodeReferenceCount = 0;
        *NodeReference = NULL;
        return MM_STATUS_SUCCESS;
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

LPMIXERLINE_EXT
MMixerGetSourceMixerLineByComponentType(
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

MIXER_STATUS
MMixerGetMixerControlById(
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
                return MM_STATUS_SUCCESS;
            }
        }
        Entry = Entry->Flink;
    }

    return MM_STATUS_UNSUCCESSFUL;
}

ULONG
MMixerGetVolumeControlIndex(
    LPMIXERVOLUME_DATA VolumeData,
    LONG Value)
{
    ULONG Index;

    for(Index = 0; Index < VolumeData->ValuesCount; Index++)
    {
        if (VolumeData->Values[Index] > Value)
        {
            return VolumeData->InputSteppingDelta * Index;
        }
    }
    return VolumeData->InputSteppingDelta * (VolumeData->ValuesCount-1);
}

MIXER_STATUS
MMixerSetGetMuteControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN ULONG NodeId,
    IN ULONG dwLineID,
    IN LPMIXERCONTROLDETAILS MixerControlDetails,
    IN ULONG bSet)
{
    LPMIXERCONTROLDETAILS_BOOLEAN Input;
    LONG Value;
    MIXER_STATUS Status;

    if (MixerControlDetails->cbDetails != sizeof(MIXERCONTROLDETAILS_BOOLEAN))
        return MM_STATUS_INVALID_PARAMETER;

    /* get input */
    Input = (LPMIXERCONTROLDETAILS_BOOLEAN)MixerControlDetails->paDetails;

    /* FIXME SEH */
    if (bSet)
        Value = Input->fValue;

    /* set control details */
    Status = MMixerSetGetControlDetails(MixerContext, hMixer, NodeId, bSet, KSPROPERTY_AUDIO_MUTE, 0, &Value);

    if (Status != MM_STATUS_SUCCESS)
        return Status;

    /* FIXME SEH */
    if (!bSet)
    {
        Input->fValue = Value;
        return Status;
    }
    else
    {
        // FIXME notify wdmaud clients MM_MIXM_LINE_CHANGE dwLineID
    }

    return Status;
}

MIXER_STATUS
MMixerSetGetVolumeControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN ULONG NodeId,
    IN ULONG bSet,
    LPMIXERCONTROLW MixerControl,
    IN LPMIXERCONTROLDETAILS MixerControlDetails,
    LPMIXERLINE_EXT MixerLine)
{
    LPMIXERCONTROLDETAILS_UNSIGNED Input;
    LONG Value, Index, Channel = 0;
    ULONG dwValue;
    MIXER_STATUS Status;
    LPMIXERVOLUME_DATA VolumeData;

    if (MixerControlDetails->cbDetails != sizeof(MIXERCONTROLDETAILS_SIGNED))
        return MM_STATUS_INVALID_PARAMETER;

    VolumeData = (LPMIXERVOLUME_DATA)MMixerGetMixerControlDataById(&MixerLine->LineControlsExtraData, MixerControl->dwControlID);
    if (!VolumeData)
        return MM_STATUS_UNSUCCESSFUL;

    /* get input */
    Input = (LPMIXERCONTROLDETAILS_UNSIGNED)MixerControlDetails->paDetails;

    if (bSet)
    {
        /* FIXME SEH */
        Value = Input->dwValue;
        Index = Value / VolumeData->InputSteppingDelta;

        if (Index >= VolumeData->ValuesCount)
        {
            DPRINT1("Index %u out of bounds %u \n", Index, VolumeData->ValuesCount);
            DbgBreakPoint();
            return MM_STATUS_INVALID_PARAMETER;
        }

        Value = VolumeData->Values[Index];
    }

    /* set control details */
    if (bSet)
    {
        /* TODO */
        Status = MMixerSetGetControlDetails(MixerContext, hMixer, NodeId, bSet, KSPROPERTY_AUDIO_VOLUMELEVEL, 0, &Value);
        Status = MMixerSetGetControlDetails(MixerContext, hMixer, NodeId, bSet, KSPROPERTY_AUDIO_VOLUMELEVEL, 1, &Value);
    }
    else
    {
        Status = MMixerSetGetControlDetails(MixerContext, hMixer, NodeId, bSet, KSPROPERTY_AUDIO_VOLUMELEVEL, Channel, &Value);
    }

    if (!bSet)
    {
        dwValue = MMixerGetVolumeControlIndex(VolumeData, (LONG)Value);
        /* FIXME SEH */
        Input->dwValue = dwValue;
    }
    else
    {
        /* notify clients of a line change  MM_MIXM_CONTROL_CHANGE with MixerControl->dwControlID */
    }
    return Status;
}

LPMIXER_DATA
MMixerGetDataByDeviceId(
    IN PMIXER_LIST MixerList,
    IN ULONG DeviceId)
{
    PLIST_ENTRY Entry;
    LPMIXER_DATA MixerData;

    Entry = MixerList->MixerData.Flink;
    while(Entry != &MixerList->MixerData)
    {
        MixerData = (LPMIXER_DATA)CONTAINING_RECORD(Entry, MIXER_DATA, Entry);
        if (MixerData->DeviceId == DeviceId)
        {
            return MixerData;
        }
        Entry = Entry->Flink;
    }
    return NULL;
}

LPMIXER_DATA
MMixerGetDataByDeviceName(
    IN PMIXER_LIST MixerList,
    IN LPWSTR DeviceName)
{
    PLIST_ENTRY Entry;
    LPMIXER_DATA MixerData;

    Entry = MixerList->MixerData.Flink;
    while(Entry != &MixerList->MixerData)
    {
        MixerData = (LPMIXER_DATA)CONTAINING_RECORD(Entry, MIXER_DATA, Entry);
        if (wcsicmp(&DeviceName[2], &MixerData->DeviceName[2]) == 0)
        {
            // found entry
            return MixerData;
        }
        Entry = Entry->Flink;
    }
    return NULL;
}

MIXER_STATUS
MMixerCreateMixerData(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN ULONG DeviceId,
    IN LPWSTR DeviceName,
    IN HANDLE hDevice,
    IN HANDLE hKey)
{
    LPMIXER_DATA MixerData;

    MixerData = (LPMIXER_DATA)MixerContext->Alloc(sizeof(MIXER_DATA));
    if (!MixerData)
        return MM_STATUS_NO_MEMORY;

    MixerData->DeviceId = DeviceId;
    MixerData->DeviceName = DeviceName;
    MixerData->hDevice = hDevice;
    MixerData->hDeviceInterfaceKey = hKey;

    InsertTailList(&MixerList->MixerData, &MixerData->Entry);
    MixerList->MixerDataCount++;
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerGetDeviceName(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN HANDLE hKey)
{
    LPWSTR Name;
    HANDLE hTemp;
    ULONG Length;
    ULONG Type;
    MIXER_STATUS Status;

    Status = MixerContext->QueryKeyValue(hKey, L"FriendlyName", (PVOID*)&Name, &Length, &Type);
    if (Status == MM_STATUS_SUCCESS)
    {
        wcscpy(MixerInfo->MixCaps.szPname, Name);
        MixerContext->Free(Name);
        return Status;
    }

    Status = MixerContext->OpenKey(hKey, L"Device Parameters", KEY_READ, &hTemp);
    if (Status != MM_STATUS_SUCCESS)
        return Status;

    Status = MixerContext->QueryKeyValue(hKey, L"FriendlyName", (PVOID*)&Name, &Length, &Type);
    if (Status == MM_STATUS_SUCCESS)
    {
        wcscpy(MixerInfo->MixCaps.szPname, Name);
        MixerContext->Free(Name);
    }

    MixerContext->CloseKey(hTemp);
    return Status;
}
