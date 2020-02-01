/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/midi.c
 * PURPOSE:         Midi Support Functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define YDEBUG
#include <debug.h>

MIXER_STATUS
MMixerGetPinDataFlowAndCommunication(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hDevice,
    IN ULONG PinId,
    OUT PKSPIN_DATAFLOW DataFlow,
    OUT PKSPIN_COMMUNICATION Communication)
{
    KSP_PIN Pin;
    ULONG BytesReturned;
    MIXER_STATUS Status;

    /* setup request */
    Pin.PinId = PinId;
    Pin.Reserved = 0;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.Property.Id = KSPROPERTY_PIN_DATAFLOW;
    Pin.Property.Set = KSPROPSETID_Pin;

    /* get pin dataflow */
    Status = MixerContext->Control(hDevice, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)DataFlow, sizeof(KSPIN_DATAFLOW), &BytesReturned);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to retrieve dataflow */
        return Status;
    }

    /* setup communication request */
    Pin.Property.Id = KSPROPERTY_PIN_COMMUNICATION;

    /* get pin communication */
    Status = MixerContext->Control(hDevice, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)Communication, sizeof(KSPIN_COMMUNICATION), &BytesReturned);

    return Status;
}

MIXER_STATUS
MMixerAddMidiPin(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN ULONG DeviceId,
    IN ULONG PinId,
    IN ULONG bInput,
    IN LPWSTR DeviceName)
{
    LPMIDI_INFO MidiInfo;

    /* allocate midi info */
    MidiInfo = MixerContext->Alloc(sizeof(MIDI_INFO));

    if (!MidiInfo)
    {
        /* no memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* initialize midi info */
    MidiInfo->DeviceId = DeviceId;
    MidiInfo->PinId = PinId;

    /* sanity check */
    ASSERT(!DeviceName || (wcslen(DeviceName) < MAXPNAMELEN));

    /* copy device name */
    if (bInput && DeviceName)
    {
        wcscpy(MidiInfo->u.InCaps.szPname, DeviceName);
    }
    else if (!bInput && DeviceName)
    {
        wcscpy(MidiInfo->u.OutCaps.szPname, DeviceName);
    }

   /* FIXME determine manufacturer / product id */
    if (bInput)
    {
        MidiInfo->u.InCaps.dwSupport = 0;
        MidiInfo->u.InCaps.wMid = MM_MICROSOFT;
        MidiInfo->u.InCaps.wPid = MM_PID_UNMAPPED;
        MidiInfo->u.InCaps.vDriverVersion = 1;
    }
    else
    {
        MidiInfo->u.OutCaps.dwSupport = 0;
        MidiInfo->u.OutCaps.wMid = MM_MICROSOFT;
        MidiInfo->u.OutCaps.wPid = MM_PID_UNMAPPED;
        MidiInfo->u.OutCaps.vDriverVersion = 1;
    }

    if (bInput)
    {
        /* insert into list */
        InsertTailList(&MixerList->MidiInList, &MidiInfo->Entry);
        MixerList->MidiInListCount++;
    }
    else
    {
        /* insert into list */
        InsertTailList(&MixerList->MidiOutList, &MidiInfo->Entry);
        MixerList->MidiOutListCount++;
    }

    return MM_STATUS_SUCCESS;
}

VOID
MMixerCheckFilterPinMidiSupport(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN ULONG PinId,
    IN PKSMULTIPLE_ITEM MultipleItem,
    IN LPWSTR szPname)
{
    ULONG Index;
    PKSDATARANGE DataRange;
    KSPIN_COMMUNICATION Communication;
    KSPIN_DATAFLOW DataFlow;

    /* get first datarange */
    DataRange = (PKSDATARANGE)(MultipleItem + 1);

    /* alignment assert */
    ASSERT(((ULONG_PTR)DataRange & 0x7) == 0);

    /* iterate through all data ranges */
    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        if (IsEqualGUIDAligned(&DataRange->MajorFormat, &KSDATAFORMAT_TYPE_MUSIC) &&
            IsEqualGUIDAligned(&DataRange->SubFormat, &KSDATAFORMAT_SUBTYPE_MIDI) &&
            IsEqualGUIDAligned(&DataRange->Specifier, &KSDATAFORMAT_SPECIFIER_NONE))
        {
            /* pin supports midi datarange */
            if (MMixerGetPinDataFlowAndCommunication(MixerContext, MixerData->hDevice, PinId, &DataFlow, &Communication) == MM_STATUS_SUCCESS)
            {
                if (DataFlow == KSPIN_DATAFLOW_IN && Communication == KSPIN_COMMUNICATION_SINK)
                {
                    MMixerAddMidiPin(MixerContext, MixerList, MixerData->DeviceId, PinId, FALSE, szPname);
                }
                else if (DataFlow == KSPIN_DATAFLOW_OUT && Communication == KSPIN_COMMUNICATION_SOURCE)
                {
                    MMixerAddMidiPin(MixerContext, MixerList, MixerData->DeviceId, PinId, TRUE, szPname);
                }
            }
        }

        /* move to next datarange */
        DataRange = (PKSDATARANGE)((ULONG_PTR)DataRange + DataRange->FormatSize);

        /* alignment assert */
        ASSERT(((ULONG_PTR)DataRange & 0x7) == 0);

        /* data ranges are 64-bit aligned */
        DataRange = (PVOID)(((ULONG_PTR)DataRange + 0x7) & ~0x7);
    }
}

VOID
MMixerInitializeMidiForFilter(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN PTOPOLOGY Topology)
{
    ULONG PinCount, Index;
    MIXER_STATUS Status;
    PKSMULTIPLE_ITEM MultipleItem;
    WCHAR szPname[MAXPNAMELEN];

    /* get filter pin count */
    MMixerGetTopologyPinCount(Topology, &PinCount);

    /* get mixer name */
    if (MMixerGetDeviceName(MixerContext, szPname, MixerData->hDeviceInterfaceKey) != MM_STATUS_SUCCESS)
    {
        /* clear name */
        szPname[0] = 0;
    }

    /* iterate all pins and check for KSDATARANGE_MUSIC support */
    for(Index = 0; Index < PinCount; Index++)
    {
        /* get audio pin data ranges */
        Status = MMixerGetAudioPinDataRanges(MixerContext, MixerData->hDevice, Index, &MultipleItem);

        /* check for success */
        if (Status == MM_STATUS_SUCCESS)
        {
            /* check if there is support KSDATARANGE_MUSIC */
            MMixerCheckFilterPinMidiSupport(MixerContext, MixerList, MixerData, Index, MultipleItem, szPname);
        }
    }
}

MIXER_STATUS
MMixerOpenMidiPin(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN ULONG DeviceId,
    IN ULONG PinId,
    IN ACCESS_MASK DesiredAccess,
    IN PIN_CREATE_CALLBACK CreateCallback,
    IN PVOID Context,
    OUT PHANDLE PinHandle)
{
    PKSPIN_CONNECT PinConnect;
    PKSDATAFORMAT DataFormat;
    LPMIXER_DATA MixerData;
    NTSTATUS Status;
    MIXER_STATUS MixerStatus;

    MixerData = MMixerGetDataByDeviceId(MixerList, DeviceId);
    if (!MixerData)
        return MM_STATUS_INVALID_PARAMETER;

    /* allocate pin connect */
    PinConnect = MMixerAllocatePinConnect(MixerContext, sizeof(KSDATAFORMAT));
    if (!PinConnect)
    {
        /* no memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* initialize pin connect struct */
    MMixerInitializePinConnect(PinConnect, PinId);

    /* get offset to dataformat */
    DataFormat = (PKSDATAFORMAT) (PinConnect + 1);

    /* initialize data format */
    RtlMoveMemory(&DataFormat->MajorFormat, &KSDATAFORMAT_TYPE_MUSIC, sizeof(GUID));
    RtlMoveMemory(&DataFormat->SubFormat, &KSDATAFORMAT_SUBTYPE_MIDI, sizeof(GUID));
    RtlMoveMemory(&DataFormat->Specifier, &KSDATAFORMAT_SPECIFIER_NONE, sizeof(GUID));

    if (CreateCallback)
    {
        /* let the callback handle the creation */
        MixerStatus = CreateCallback(Context, DeviceId, PinId, MixerData->hDevice, PinConnect, DesiredAccess, PinHandle);
    }
    else
    {
        /* now create the pin */
        Status = KsCreatePin(MixerData->hDevice, PinConnect, DesiredAccess, PinHandle);

        /* normalize status */
        if (Status == STATUS_SUCCESS)
            MixerStatus = MM_STATUS_SUCCESS;
        else
            MixerStatus = MM_STATUS_UNSUCCESSFUL;
    }

    /* free create info */
    MixerContext->Free(PinConnect);

    /* done */
    return MixerStatus;
}

MIXER_STATUS
MMixerGetMidiInfoByIndexAndType(
    IN  PMIXER_LIST MixerList,
    IN  ULONG DeviceIndex,
    IN  ULONG bMidiInputType,
    OUT LPMIDI_INFO *OutMidiInfo)
{
    ULONG Index = 0;
    PLIST_ENTRY Entry, ListHead;
    LPMIDI_INFO MidiInfo;

    if (bMidiInputType)
        ListHead = &MixerList->MidiInList;
    else
        ListHead = &MixerList->MidiOutList;

    /* get first entry */
    Entry = ListHead->Flink;

    while(Entry != ListHead)
    {
        MidiInfo = (LPMIDI_INFO)CONTAINING_RECORD(Entry, MIDI_INFO, Entry);

        if (Index == DeviceIndex)
        {
            *OutMidiInfo = MidiInfo;
            return MM_STATUS_SUCCESS;
        }
        Index++;
        Entry = Entry->Flink;
    }

    return MM_STATUS_INVALID_PARAMETER;
}

MIXER_STATUS
MMixerMidiOutCapabilities(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG DeviceIndex,
    OUT LPMIDIOUTCAPSW Caps)
{
    PMIXER_LIST MixerList;
    MIXER_STATUS Status;
    LPMIDI_INFO MidiInfo;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* grab mixer list */
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    /* find destination midi */
    Status = MMixerGetMidiInfoByIndexAndType(MixerList, DeviceIndex, FALSE, &MidiInfo);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to find midi info */
        return MM_STATUS_UNSUCCESSFUL;
    }

    /* copy capabilities */
    MixerContext->Copy(Caps, &MidiInfo->u.OutCaps, sizeof(MIDIOUTCAPSW));

    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerMidiInCapabilities(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG DeviceIndex,
    OUT LPMIDIINCAPSW Caps)
{
    PMIXER_LIST MixerList;
    MIXER_STATUS Status;
    LPMIDI_INFO MidiInfo;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* grab mixer list */
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    /* find destination midi */
    Status = MMixerGetMidiInfoByIndexAndType(MixerList, DeviceIndex, TRUE, &MidiInfo);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to find midi info */
        return MM_STATUS_UNSUCCESSFUL;
    }

    /* copy capabilities */
    MixerContext->Copy(Caps, &MidiInfo->u.InCaps, sizeof(MIDIINCAPSW));

    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerGetMidiDevicePath(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG bMidiIn,
    IN ULONG DeviceId,
    OUT LPWSTR * DevicePath)
{
    PMIXER_LIST MixerList;
    LPMIXER_DATA MixerData;
    LPMIDI_INFO MidiInfo;
    SIZE_T Length;
    MIXER_STATUS Status;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* grab mixer list */
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    /* find destination midi */
    Status = MMixerGetMidiInfoByIndexAndType(MixerList, DeviceId, bMidiIn, &MidiInfo);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to find midi info */
        return MM_STATUS_INVALID_PARAMETER;
    }

    /* get associated device id */
    MixerData = MMixerGetDataByDeviceId(MixerList, MidiInfo->DeviceId);
    if (!MixerData)
        return MM_STATUS_INVALID_PARAMETER;

    /* calculate length */
    Length = wcslen(MixerData->DeviceName)+1;

    /* allocate destination buffer */
    *DevicePath = MixerContext->Alloc(Length * sizeof(WCHAR));

    if (!*DevicePath)
    {
        /* no memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* copy device path */
    MixerContext->Copy(*DevicePath, MixerData->DeviceName, Length * sizeof(WCHAR));

    /* done */
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerSetMidiStatus(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE PinHandle,
    IN KSSTATE State)
{
    KSPROPERTY Property;
    ULONG Length;

    /* setup property request */
    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;

    return MixerContext->Control(PinHandle, IOCTL_KS_PROPERTY, &Property, sizeof(KSPROPERTY), &State, sizeof(KSSTATE), &Length);
}

MIXER_STATUS
MMixerOpenMidi(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG DeviceIndex,
    IN ULONG bMidiIn,
    IN PIN_CREATE_CALLBACK CreateCallback,
    IN PVOID Context,
    OUT PHANDLE PinHandle)
{
    PMIXER_LIST MixerList;
    MIXER_STATUS Status;
    LPMIDI_INFO MidiInfo;
    ACCESS_MASK DesiredAccess = 0;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* grab mixer list */
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    /* find destination midi */
    Status = MMixerGetMidiInfoByIndexAndType(MixerList, DeviceIndex, bMidiIn, &MidiInfo);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to find midi info */
        return MM_STATUS_INVALID_PARAMETER;
    }

    /* get desired access */
    if (bMidiIn)
    {
        DesiredAccess |= GENERIC_READ;
    }
     else
    {
        DesiredAccess |= GENERIC_WRITE;
    }

    /* now try open the pin */
    return MMixerOpenMidiPin(MixerContext, MixerList, MidiInfo->DeviceId, MidiInfo->PinId, DesiredAccess, CreateCallback, Context, PinHandle);
}

ULONG
MMixerGetMidiInCount(
    IN PMIXER_CONTEXT MixerContext)
{
    PMIXER_LIST MixerList;
    MIXER_STATUS Status;

     /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* grab mixer list */
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    return MixerList->MidiInListCount;
}

ULONG
MMixerGetMidiOutCount(
    IN PMIXER_CONTEXT MixerContext)
{
    PMIXER_LIST MixerList;
    MIXER_STATUS Status;

     /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* grab mixer list */
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    return MixerList->MidiOutListCount;
}
