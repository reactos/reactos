/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/mmixer.c
 * PURPOSE:         Mixer Handling Functions
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

ULONG
MMixerGetFilterPinCount(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer)
{
    KSPROPERTY Pin;
    MIXER_STATUS Status;
    ULONG NumPins, BytesReturned;

    // setup property request
    Pin.Flags = KSPROPERTY_TYPE_GET;
    Pin.Set = KSPROPSETID_Pin;
    Pin.Id = KSPROPERTY_PIN_CTYPES;

    // query pin count
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY), (PVOID)&NumPins, sizeof(ULONG), (PULONG)&BytesReturned);

    // check for success
    if (Status != MM_STATUS_SUCCESS)
        return 0;

    return NumPins;
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


MIXER_STATUS
MMixerGetFilterTopologyProperty(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN ULONG PropertyId,
    OUT PKSMULTIPLE_ITEM * OutMultipleItem)
{
    KSPROPERTY Property;
    PKSMULTIPLE_ITEM MultipleItem;
    MIXER_STATUS Status;
    ULONG BytesReturned;

    // setup property request
    Property.Id = PropertyId;
    Property.Flags = KSPROPERTY_TYPE_GET;
    Property.Set = KSPROPSETID_Topology;

    // query for the size
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), NULL, 0, &BytesReturned);

    if (Status != MM_STATUS_MORE_ENTRIES)
        return Status;

    // allocate an result buffer
    MultipleItem = (PKSMULTIPLE_ITEM)MixerContext->Alloc(BytesReturned);

    if (!MultipleItem)
    {
        // not enough memory
        return MM_STATUS_NO_MEMORY;
    }

    // query again with allocated buffer
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)MultipleItem, BytesReturned, &BytesReturned);

    if (Status != MM_STATUS_SUCCESS)
    {
        // failed
        MixerContext->Free((PVOID)MultipleItem);
        return Status;
    }

    // store result
    *OutMultipleItem = MultipleItem;

    // done
    return Status;
}

MIXER_STATUS
MMixerCreateDestinationLine(
    IN PMIXER_CONTEXT MixerContext,
    IN LPMIXER_INFO MixerInfo,
    IN ULONG bInputMixer)
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
    wcscpy(DestinationLine->Line.szShortName, L"Summe"); //FIXME
    wcscpy(DestinationLine->Line.szName, L"Summe"); //FIXME
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
MMixerInitializeFilter(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN LPWSTR DeviceName,
    IN PKSMULTIPLE_ITEM NodeTypes,
    IN PKSMULTIPLE_ITEM NodeConnections,
    IN ULONG PinCount,
    IN ULONG NodeIndex,
    IN ULONG bInputMixer)
{
    LPMIXER_INFO MixerInfo;
    MIXER_STATUS Status;
    ULONG * Pins;

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
    MixerInfo->hMixer = hMixer;

    // initialize line list
    InitializeListHead(&MixerInfo->LineList);

    /* FIXME find mixer name */

    Status = MMixerCreateDestinationLine(MixerContext, MixerInfo, bInputMixer);
    if (Status != MM_STATUS_SUCCESS)
    {
        // failed to create destination line
        MixerContext->Free(MixerInfo);
        return Status;
    }


    // now allocate an array which will receive the indices of the pin 
    // which has a ADC / DAC nodetype in its path
    Pins = (PULONG)MixerContext->Alloc(PinCount * sizeof(ULONG));

    if (!Pins)
    {
        // no memory
        MMixerFreeMixerInfo(MixerContext, MixerInfo);
        return MM_STATUS_NO_MEMORY;
    }


    //UNIMPLEMENTED
    // get target pins and find all nodes
    return MM_STATUS_NOT_IMPLEMENTED;
}

MIXER_STATUS
MMixerSetupFilter(
    IN PMIXER_CONTEXT MixerContext, 
    IN HANDLE hMixer,
    IN PULONG DeviceCount,
    IN LPWSTR DeviceName)
{
    PKSMULTIPLE_ITEM NodeTypes, NodeConnections;
    MIXER_STATUS Status;
    ULONG PinCount;
    ULONG NodeIndex;

    // get number of pins
    PinCount = MMixerGetFilterPinCount(MixerContext, hMixer);
    ASSERT(PinCount);


    // get filter node types
    Status = MMixerGetFilterTopologyProperty(MixerContext, hMixer, KSPROPERTY_TOPOLOGY_NODES, &NodeTypes);
    if (Status != MM_STATUS_SUCCESS)
    {
        // failed
        return Status;
    }

    // get filter node connections
    Status = MMixerGetFilterTopologyProperty(MixerContext, hMixer, KSPROPERTY_TOPOLOGY_CONNECTIONS, &NodeConnections);
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
        Status = MMixerInitializeFilter(MixerContext, hMixer, DeviceName, NodeTypes, NodeConnections, PinCount, NodeIndex, FALSE);

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
        Status = MMixerInitializeFilter(MixerContext, hMixer, DeviceName, NodeTypes, NodeConnections, PinCount, NodeIndex, TRUE);

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


MIXER_STATUS
MMixerInitialize(
    IN PMIXER_CONTEXT MixerContext, 
    IN PMIXER_ENUM EnumFunction,
    IN PVOID EnumContext)
{
    MIXER_STATUS Status;
    HANDLE hMixer;
    ULONG DeviceIndex, Count;
    LPWSTR DeviceName;

    if (!MixerContext || !EnumFunction || !EnumContext)
    {
        // invalid parameter
        return MM_STATUS_INVALID_PARAMETER;
    }

    if (!MixerContext->Alloc || !MixerContext->Control || !MixerContext->Free)
    {
        // invalid parameter
        return MM_STATUS_INVALID_PARAMETER;
    }


    // start enumerating all available devices
    Count = 0;
    DeviceIndex = 0;

    do
    {
        // enumerate a device
        Status = EnumFunction(EnumContext, DeviceIndex, &DeviceName, &hMixer);

        if (Status != MM_STATUS_SUCCESS)
        {
            //check error code
            if (Status != MM_STATUS_NO_MORE_DEVICES)
            {
                // enumeration has failed
                return Status;
            }
            // last device
            break;
        }


        // increment device index
        DeviceIndex++;

        Status = MMixerSetupFilter(MixerContext, hMixer, &Count, DeviceName);

        if (Status != MM_STATUS_SUCCESS)
            break;

    }while(TRUE);

    // done
    return Status;
}


