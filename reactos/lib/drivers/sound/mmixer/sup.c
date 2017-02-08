/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/sup.c
 * PURPOSE:         Mixer Support Functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define YDEBUG
#include <debug.h>

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

const GUID KSDATAFORMAT_TYPE_MUSIC = {0xE725D360L, 0x62CC, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSDATAFORMAT_SUBTYPE_MIDI = {0x1D262760L, 0xE957, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSDATAFORMAT_SPECIFIER_NONE = {0x0F6417D6L, 0xC318, 0x11D0, {0xA4, 0x3F, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};


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

LPMIXERLINE_EXT
MMixerGetMixerLineContainingNodeId(
    IN LPMIXER_INFO MixerInfo,
    IN ULONG NodeID)
{
    PLIST_ENTRY Entry, ControlEntry;
    LPMIXERLINE_EXT MixerLineSrc;
    LPMIXERCONTROL_EXT MixerControl;

    /* get first entry */
    Entry = MixerInfo->LineList.Flink;

    while(Entry != &MixerInfo->LineList)
    {
        MixerLineSrc = (LPMIXERLINE_EXT)CONTAINING_RECORD(Entry, MIXERLINE_EXT, Entry);

        ControlEntry = MixerLineSrc->ControlsList.Flink;
        while(ControlEntry != &MixerLineSrc->ControlsList)
        {
            MixerControl = (LPMIXERCONTROL_EXT)CONTAINING_RECORD(ControlEntry, MIXERCONTROL_EXT, Entry);
            if (MixerControl->NodeID == NodeID)
            {
                return MixerLineSrc;
            }
            ControlEntry = ControlEntry->Flink;
        }
        Entry = Entry->Flink;
    }

    return NULL;
}

VOID
MMixerGetLowestLogicalTopologyPinOffsetFromArray(
    IN ULONG LogicalPinArrayCount,
    IN PULONG LogicalPinArray,
    OUT PULONG PinOffset)
{
    ULONG Index;
    ULONG LowestId = 0;

    for(Index = 1; Index < LogicalPinArrayCount; Index++)
    {
        if (LogicalPinArray[Index] != MAXULONG)
        {
            /* sanity check: logical pin id must be unique */
            ASSERT(LogicalPinArray[Index] != LogicalPinArray[LowestId]);
        }

        if (LogicalPinArray[Index] < LogicalPinArray[LowestId])
            LowestId = Index;
    }

    /* store result */
    *PinOffset = LowestId;
}

VOID
MMixerFreeMixerInfo(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo)
{
    /* UNIMPLEMENTED
     * FIXME
     * free all lines
     */

    MixerContext->Free((PVOID)MixerInfo);
}


LPMIXER_DATA
MMixerGetMixerDataByDeviceHandle(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hDevice)
{
    LPMIXER_DATA MixerData;
    PLIST_ENTRY Entry;
    PMIXER_LIST MixerList;

    /* get mixer list */
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    if (!MixerList->MixerDataCount)
        return NULL;

    Entry = MixerList->MixerData.Flink;

    while(Entry != &MixerList->MixerData)
    {
        MixerData = (LPMIXER_DATA)CONTAINING_RECORD(Entry, MIXER_DATA, Entry);

        if (MixerData->hDevice == hDevice)
            return MixerData;

        /* move to next mixer entry */
        Entry = Entry->Flink;
    }
    return NULL;
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

    /* get mixer list */
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    if (!MixerList->MixerListCount)
        return NULL;

    Entry = MixerList->MixerList.Flink;

    while(Entry != &MixerList->MixerList)
    {
        MixerInfo = (LPMIXER_INFO)CONTAINING_RECORD(Entry, MIXER_INFO, Entry);

        if (Index == MixerIndex)
            return MixerInfo;

        /* move to next mixer entry */
        Index++;
        Entry = Entry->Flink;
    }

    return NULL;
}

MIXER_STATUS
MMixerGetMixerByName(
    IN PMIXER_LIST MixerList,
    IN LPWSTR MixerName,
    OUT LPMIXER_INFO *OutMixerInfo)
{
    LPMIXER_INFO MixerInfo;
    PLIST_ENTRY Entry;

    Entry = MixerList->MixerList.Flink;
    while(Entry != &MixerList->MixerList)
    {
        MixerInfo = (LPMIXER_INFO)CONTAINING_RECORD(Entry, MIXER_INFO, Entry);

        DPRINT1("MixerName %S MixerName %S\n", MixerInfo->MixCaps.szPname, MixerName);
        if (wcsicmp(MixerInfo->MixCaps.szPname, MixerName) == 0)
        {
            *OutMixerInfo = MixerInfo;
            return MM_STATUS_SUCCESS;
        }
        /* move to next mixer entry */
        Entry = Entry->Flink;
    }

    return MM_STATUS_UNSUCCESSFUL;
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
    LPMIXERLINE_EXT *OutMixerLine,
    LPMIXERCONTROL_EXT *OutMixerControl,
    PULONG NodeId)
{
    PLIST_ENTRY Entry, ControlEntry;
    LPMIXERLINE_EXT MixerLineSrc;
    LPMIXERCONTROL_EXT MixerControl;

    /* get first entry */
    Entry = MixerInfo->LineList.Flink;

    while(Entry != &MixerInfo->LineList)
    {
        MixerLineSrc = (LPMIXERLINE_EXT)CONTAINING_RECORD(Entry, MIXERLINE_EXT, Entry);

        ControlEntry = MixerLineSrc->ControlsList.Flink;
        while(ControlEntry != &MixerLineSrc->ControlsList)
        {
            MixerControl = (LPMIXERCONTROL_EXT)CONTAINING_RECORD(ControlEntry, MIXERCONTROL_EXT, Entry);
            if (MixerControl->Control.dwControlID == dwControlID)
            {
                if (OutMixerLine)
                    *OutMixerLine = MixerLineSrc;
                if (OutMixerControl)
                    *OutMixerControl = MixerControl;
                if (NodeId)
                    *NodeId = MixerControl->NodeID;
                return MM_STATUS_SUCCESS;
            }
            ControlEntry = ControlEntry->Flink;
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

VOID
MMixerNotifyControlChange(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN ULONG NotificationType,
    IN ULONG Value)
{
    PLIST_ENTRY Entry;
    PEVENT_NOTIFICATION_ENTRY NotificationEntry;

    /* enumerate list and perform notification */
    Entry = MixerInfo->EventList.Flink;
    while(Entry != &MixerInfo->EventList)
    {
        /* get notification entry offset */
        NotificationEntry = (PEVENT_NOTIFICATION_ENTRY)CONTAINING_RECORD(Entry, EVENT_NOTIFICATION_ENTRY, Entry);

        if (NotificationEntry->MixerEventRoutine)
        {
            /* now perform the callback */
            NotificationEntry->MixerEventRoutine(NotificationEntry->MixerEventContext, (HANDLE)MixerInfo, NotificationType, Value);
        }

        /* move to next notification entry */
        Entry = Entry->Flink;
    }
}

MIXER_STATUS
MMixerSetGetMuteControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN LPMIXERCONTROL_EXT MixerControl,
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
    Status = MMixerSetGetControlDetails(MixerContext, MixerControl->hDevice, MixerControl->NodeID, bSet, KSPROPERTY_AUDIO_MUTE, 0, &Value);

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
        /* notify wdmaud clients MM_MIXM_LINE_CHANGE dwLineID */
        MMixerNotifyControlChange(MixerContext, MixerInfo, MM_MIXM_LINE_CHANGE, dwLineID);
    }

    return Status;
}

MIXER_STATUS
MMixerSetGetMuxControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN ULONG NodeId,
    IN ULONG bSet,
    IN ULONG Flags,
    IN LPMIXERCONTROL_EXT MixerControl,
    IN LPMIXERCONTROLDETAILS MixerControlDetails,
    IN LPMIXERLINE_EXT MixerLine)
{
    MIXER_STATUS Status;
    PULONG LogicalNodes, ConnectedNodes;
    ULONG LogicalNodesCount, ConnectedNodesCount, Index, CurLogicalPinOffset, BytesReturned, OldLogicalPinOffset;
    LPMIXER_DATA MixerData;
    LPMIXERCONTROLDETAILS_LISTTEXTW ListText;
    LPMIXERCONTROLDETAILS_BOOLEAN Values;
    LPMIXERLINE_EXT SourceLine;
    KSNODEPROPERTY Request;

    DPRINT("MixerControlDetails %p\n", MixerControlDetails);
    DPRINT("bSet %lx\n", bSet);
    DPRINT("Flags %lx\n", Flags);
    DPRINT("NodeId %lu\n", MixerControl->NodeID);
    DPRINT("MixerControlDetails dwControlID %lu\n", MixerControlDetails->dwControlID);
    DPRINT("MixerControlDetails cChannels %lu\n", MixerControlDetails->cChannels);
    DPRINT("MixerControlDetails cMultipleItems %lu\n", MixerControlDetails->cMultipleItems);
    DPRINT("MixerControlDetails cbDetails %lu\n", MixerControlDetails->cbDetails);
    DPRINT("MixerControlDetails paDetails %p\n", MixerControlDetails->paDetails);

    if (MixerControl->Control.fdwControl & MIXERCONTROL_CONTROLF_UNIFORM)
    {
        /* control acts uniform */
        if (MixerControlDetails->cChannels != 1)
        {
            /* expected 1 channel */
            DPRINT1("Expected 1 channel but got %lu\n", MixerControlDetails->cChannels);
            return MM_STATUS_UNSUCCESSFUL;
        }
    }

    /* check if multiple items match */
    if (MixerControlDetails->cMultipleItems != MixerControl->Control.cMultipleItems)
    {
        DPRINT1("MultipleItems mismatch %lu expected %lu\n", MixerControlDetails->cMultipleItems, MixerControl->Control.cMultipleItems);
        return MM_STATUS_UNSUCCESSFUL;
    }

    if (bSet)
    {
        if ((Flags & MIXER_SETCONTROLDETAILSF_QUERYMASK) == MIXER_SETCONTROLDETAILSF_CUSTOM)
        {
            /* tell me when this is hit */
            ASSERT(FALSE);
        }
        else if ((Flags & (MIXER_SETCONTROLDETAILSF_VALUE | MIXER_SETCONTROLDETAILSF_CUSTOM)) == MIXER_SETCONTROLDETAILSF_VALUE)
        {
            /* sanity check */
            ASSERT(bSet == TRUE);
            ASSERT(MixerControlDetails->cbDetails == sizeof(MIXERCONTROLDETAILS_BOOLEAN));

            Values = (LPMIXERCONTROLDETAILS_BOOLEAN)MixerControlDetails->paDetails;
            CurLogicalPinOffset = MAXULONG;
            for(Index = 0; Index < MixerControlDetails->cMultipleItems; Index++)
            {
                if (Values[Index].fValue)
                {
                    /* mux can only activate one line at a time */
                    ASSERT(CurLogicalPinOffset == MAXULONG);
                    CurLogicalPinOffset = Index;
                }
            }

            /* setup request */
            Request.NodeId = NodeId;
            Request.Reserved = 0;
            Request.Property.Flags = KSPROPERTY_TYPE_TOPOLOGY | KSPROPERTY_TYPE_GET;
            Request.Property.Id = KSPROPERTY_AUDIO_MUX_SOURCE;
            Request.Property.Set = KSPROPSETID_Audio;

            /* perform getting source */
            Status = MixerContext->Control(MixerControl->hDevice, IOCTL_KS_PROPERTY, (PVOID)&Request, sizeof(KSNODEPROPERTY), &OldLogicalPinOffset, sizeof(ULONG), &BytesReturned);
            if (Status != MM_STATUS_SUCCESS)
            {
                /* failed to get source */
                return Status;
            }

            DPRINT("OldLogicalPinOffset %lu CurLogicalPinOffset %lu\n", OldLogicalPinOffset, CurLogicalPinOffset);

            if (OldLogicalPinOffset == CurLogicalPinOffset)
            {
                /* cannot be unselected */
                return MM_STATUS_UNSUCCESSFUL;
            }

            /* perform setting source */
            Request.Property.Flags = KSPROPERTY_TYPE_TOPOLOGY | KSPROPERTY_TYPE_SET;
            Status = MixerContext->Control(MixerControl->hDevice, IOCTL_KS_PROPERTY, (PVOID)&Request, sizeof(KSNODEPROPERTY), &CurLogicalPinOffset, sizeof(ULONG), &BytesReturned);
            if (Status != MM_STATUS_SUCCESS)
            {
                /* failed to set source */
                return Status;
            }

            /* notify control change */
            MMixerNotifyControlChange(MixerContext, MixerInfo, MM_MIXM_CONTROL_CHANGE, MixerControl->Control.dwControlID );

            return Status;
        }
    }
    else
    {
        if ((Flags & MIXER_GETCONTROLDETAILSF_QUERYMASK) == MIXER_GETCONTROLDETAILSF_VALUE)
        {
            /* setup request */
            Request.NodeId = NodeId;
            Request.Reserved = 0;
            Request.Property.Flags = KSPROPERTY_TYPE_TOPOLOGY | KSPROPERTY_TYPE_GET;
            Request.Property.Id = KSPROPERTY_AUDIO_MUX_SOURCE;
            Request.Property.Set = KSPROPSETID_Audio;

            /* perform getting source */
            Status = MixerContext->Control(MixerControl->hDevice, IOCTL_KS_PROPERTY, (PVOID)&Request, sizeof(KSNODEPROPERTY), &OldLogicalPinOffset, sizeof(ULONG), &BytesReturned);
            if (Status != MM_STATUS_SUCCESS)
            {
                /* failed to get source */
                return Status;
            }

            /* gets the corresponding mixer data */
            MixerData = MMixerGetMixerDataByDeviceHandle(MixerContext, MixerControl->hDevice);

            /* sanity check */
            ASSERT(MixerData);
            ASSERT(MixerData->Topology);
            ASSERT(MixerData->MixerInfo == MixerInfo);

            /* now allocate logical pin array */
            Status = MMixerAllocateTopologyNodeArray(MixerContext, MixerData->Topology, &LogicalNodes);
            if (Status != MM_STATUS_SUCCESS)
            {
                /* no memory */
                return MM_STATUS_NO_MEMORY;
            }

            /* get logical pin nodes */
            MMixerGetConnectedFromLogicalTopologyPins(MixerData->Topology, MixerControl->NodeID, &LogicalNodesCount, LogicalNodes);

            /* sanity check */
            ASSERT(LogicalNodesCount == MixerControlDetails->cMultipleItems);
            ASSERT(LogicalNodesCount == MixerControl->Control.Metrics.dwReserved[0]);

            Values = (LPMIXERCONTROLDETAILS_BOOLEAN)MixerControlDetails->paDetails;
            for(Index = 0; Index < LogicalNodesCount; Index++)
            {
                /* getting logical pin offset */
                MMixerGetLowestLogicalTopologyPinOffsetFromArray(LogicalNodesCount, LogicalNodes, &CurLogicalPinOffset);

                if (CurLogicalPinOffset == OldLogicalPinOffset)
                {
                    /* mark index as active */
                    Values[Index].fValue = TRUE;
                }
                else
                {
                    /* index not active */
                    Values[Index].fValue = FALSE;
                }

                /* mark offset as consumed */
                LogicalNodes[CurLogicalPinOffset] = MAXULONG;
            }

            /* cleanup */
            MixerContext->Free(LogicalNodes);

            /* done */
            return MM_STATUS_SUCCESS;
        }
        else if ((Flags & MIXER_GETCONTROLDETAILSF_QUERYMASK) == MIXER_GETCONTROLDETAILSF_LISTTEXT)
        {
            /* sanity check */
            ASSERT(bSet == FALSE);

            /* gets the corresponding mixer data */
            MixerData = MMixerGetMixerDataByDeviceHandle(MixerContext, MixerControl->hDevice);

            /* sanity check */
            ASSERT(MixerData);
            ASSERT(MixerData->Topology);
            ASSERT(MixerData->MixerInfo == MixerInfo);

            /* now allocate logical pin array */
            Status = MMixerAllocateTopologyNodeArray(MixerContext, MixerData->Topology, &LogicalNodes);
            if (Status != MM_STATUS_SUCCESS)
            {
                /* no memory */
                return MM_STATUS_NO_MEMORY;
            }

            /* allocate connected node array */
            Status = MMixerAllocateTopologyNodeArray(MixerContext, MixerData->Topology, &ConnectedNodes);
            if (Status != MM_STATUS_SUCCESS)
            {
                /* no memory */
                MixerContext->Free(LogicalNodes);
                return MM_STATUS_NO_MEMORY;
            }

            /* get logical pin nodes */
            MMixerGetConnectedFromLogicalTopologyPins(MixerData->Topology, MixerControl->NodeID, &LogicalNodesCount, LogicalNodes);

            /* get connected nodes */
            MMixerGetNextNodesFromNodeIndex(MixerContext, MixerData->Topology, MixerControl->NodeID, TRUE, &ConnectedNodesCount, ConnectedNodes);

            /* sanity check */
            ASSERT(ConnectedNodesCount == LogicalNodesCount);
            ASSERT(ConnectedNodesCount == MixerControlDetails->cMultipleItems);
            ASSERT(ConnectedNodesCount == MixerControl->Control.Metrics.dwReserved[0]);

            ListText = (LPMIXERCONTROLDETAILS_LISTTEXTW)MixerControlDetails->paDetails;

            for(Index = 0; Index < ConnectedNodesCount; Index++)
            {
                /* getting logical pin offset */
                MMixerGetLowestLogicalTopologyPinOffsetFromArray(LogicalNodesCount, LogicalNodes, &CurLogicalPinOffset);

                /* get mixer line with that node */
                SourceLine = MMixerGetMixerLineContainingNodeId(MixerInfo, ConnectedNodes[CurLogicalPinOffset]);

                /* sanity check */
                ASSERT(SourceLine);

                DPRINT1("PinOffset %lu LogicalPin %lu NodeId %lu LineName %S\n", CurLogicalPinOffset, LogicalNodes[CurLogicalPinOffset], ConnectedNodes[CurLogicalPinOffset], SourceLine->Line.szName);

                /* copy details */
                ListText[Index].dwParam1 = SourceLine->Line.dwLineID;
                ListText[Index].dwParam2 = SourceLine->Line.dwComponentType;
                MixerContext->Copy(ListText[Index].szName, SourceLine->Line.szName, (wcslen(SourceLine->Line.szName) + 1) * sizeof(WCHAR));

                /* mark offset as consumed */
                LogicalNodes[CurLogicalPinOffset] = MAXULONG;
            }

            /* cleanup */
            MixerContext->Free(LogicalNodes);
            MixerContext->Free(ConnectedNodes);

            /* done */
            return MM_STATUS_SUCCESS;
        }
    }

    return MM_STATUS_NOT_IMPLEMENTED;
}

MIXER_STATUS
MMixerSetGetVolumeControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN ULONG NodeId,
    IN ULONG bSet,
    LPMIXERCONTROL_EXT MixerControl,
    IN LPMIXERCONTROLDETAILS MixerControlDetails,
    LPMIXERLINE_EXT MixerLine)
{
    LPMIXERCONTROLDETAILS_UNSIGNED Input;
    LONG Value;
    ULONG Index, Channel = 0;
    ULONG dwValue;
    MIXER_STATUS Status;
    LPMIXERVOLUME_DATA VolumeData;

    if (MixerControlDetails->cbDetails != sizeof(MIXERCONTROLDETAILS_SIGNED))
        return MM_STATUS_INVALID_PARAMETER;

    VolumeData = (LPMIXERVOLUME_DATA)MixerControl->ExtraData;
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
            return MM_STATUS_INVALID_PARAMETER;
        }

        Value = VolumeData->Values[Index];
    }

    /* set control details */
    if (bSet)
    {
        /* TODO */
        Status = MMixerSetGetControlDetails(MixerContext, MixerControl->hDevice, NodeId, bSet, KSPROPERTY_AUDIO_VOLUMELEVEL, 0, &Value);
        Status = MMixerSetGetControlDetails(MixerContext, MixerControl->hDevice, NodeId, bSet, KSPROPERTY_AUDIO_VOLUMELEVEL, 1, &Value);
    }
    else
    {
        Status = MMixerSetGetControlDetails(MixerContext, MixerControl->hDevice, NodeId, bSet, KSPROPERTY_AUDIO_VOLUMELEVEL, Channel, &Value);
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
        MMixerNotifyControlChange(MixerContext, MixerInfo, MM_MIXM_CONTROL_CHANGE, MixerControl->Control.dwControlID);
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
            /* found entry */
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
    MixerData->Topology = NULL;

    InsertTailList(&MixerList->MixerData, &MixerData->Entry);
    MixerList->MixerDataCount++;
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerGetDeviceName(
    IN PMIXER_CONTEXT MixerContext,
    OUT LPWSTR DeviceName,
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
        /* copy device name */
        MixerContext->Copy(DeviceName, Name, min(wcslen(Name), MAXPNAMELEN-1) * sizeof(WCHAR));

        /* make sure its null terminated */
        DeviceName[MAXPNAMELEN-1] = L'\0';

        /* free device name */
        MixerContext->Free(Name);

        /* done */
        return Status;
    }

    Status = MixerContext->OpenKey(hKey, L"Device Parameters", KEY_READ, &hTemp);
    if (Status != MM_STATUS_SUCCESS)
        return Status;

    Status = MixerContext->QueryKeyValue(hTemp, L"FriendlyName", (PVOID*)&Name, &Length, &Type);
    if (Status == MM_STATUS_SUCCESS)
    {
        /* copy device name */
        MixerContext->Copy(DeviceName, Name, min(wcslen(Name), MAXPNAMELEN-1) * sizeof(WCHAR));

        /* make sure its null terminated */
        DeviceName[MAXPNAMELEN-1] = L'\0';

        /* free device name */
        MixerContext->Free(Name);
    }

    MixerContext->CloseKey(hTemp);
    return Status;
}

VOID
MMixerInitializePinConnect(
    IN OUT PKSPIN_CONNECT PinConnect,
    IN ULONG PinId)
{
    PinConnect->Interface.Set = KSINTERFACESETID_Standard;
    PinConnect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
    PinConnect->Interface.Flags = 0;
    PinConnect->Medium.Set = KSMEDIUMSETID_Standard;
    PinConnect->Medium.Id = KSMEDIUM_TYPE_ANYINSTANCE;
    PinConnect->Medium.Flags = 0;
    PinConnect->PinToHandle = NULL;
    PinConnect->PinId = PinId;
    PinConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    PinConnect->Priority.PrioritySubClass = 1;
}
