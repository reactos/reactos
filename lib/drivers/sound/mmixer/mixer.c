/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/mmixer.c
 * PURPOSE:         Mixer Handling Functions
 * PROGRAMMER:      Johannes Anderwald
 */



#include "priv.h"

ULONG
MMixerGetCount(
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

    // return number of mixers
    return MixerList->MixerListCount;
}

MIXER_STATUS
MMixerGetCapabilities(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG MixerIndex,
    OUT LPMIXERCAPSW MixerCaps)
{
    MIXER_STATUS Status;
    LPMIXER_INFO MixerInfo;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* get mixer info */
    MixerInfo = MMixerGetMixerInfoByIndex(MixerContext, MixerIndex);

    if (!MixerInfo)
    {
        // invalid device index
        return MM_STATUS_INVALID_PARAMETER;
    }

    MixerCaps->wMid = MixerInfo->MixCaps.wMid;
    MixerCaps->wPid = MixerInfo->MixCaps.wPid;
    MixerCaps->vDriverVersion = MixerInfo->MixCaps.vDriverVersion;
    MixerCaps->fdwSupport = MixerInfo->MixCaps.fdwSupport;
    MixerCaps->cDestinations = MixerInfo->MixCaps.cDestinations;

    ASSERT(MixerInfo->MixCaps.szPname[MAXPNAMELEN-1] == 0);
    wcscpy(MixerCaps->szPname, MixerInfo->MixCaps.szPname);

    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerOpen(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG MixerId,
    IN PVOID MixerEventContext,
    IN PMIXER_EVENT MixerEventRoutine,
    OUT PHANDLE MixerHandle)
{
    MIXER_STATUS Status;
    LPMIXER_INFO MixerInfo;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        DPRINT1("invalid context\n");
        return Status;
    }

    /* get mixer info */
    MixerInfo = (LPMIXER_INFO)MMixerGetMixerInfoByIndex(MixerContext, MixerId);
    if (!MixerInfo)
    {
        /* invalid mixer id */
        DPRINT1("invalid mixer id %lu\n", MixerId);
        return MM_STATUS_INVALID_PARAMETER;
    }

    /* add the event */
    Status = MMixerAddEvent(MixerContext, MixerInfo, MixerEventContext, MixerEventRoutine);


    /* store result */
    *MixerHandle = (HANDLE)MixerInfo;

    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerGetLineInfo(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE MixerHandle,
    IN ULONG MixerId,
    IN ULONG Flags,
    OUT LPMIXERLINEW MixerLine)
{
    MIXER_STATUS Status;
    LPMIXER_INFO MixerInfo;
    LPMIXERLINE_EXT MixerLineSrc;
    ULONG DestinationLineID;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }
    if ((Flags & (MIXER_OBJECTF_MIXER | MIXER_OBJECTF_HMIXER)) == MIXER_OBJECTF_MIXER)
    {
        /* caller passed mixer id */
        MixerHandle = (HANDLE)MMixerGetMixerInfoByIndex(MixerContext, MixerId);

        if (!MixerHandle)
        {
            /* invalid parameter */
            return MM_STATUS_INVALID_PARAMETER;
        }
    }

    if (MixerLine->cbStruct != sizeof(MIXERLINEW))
	{
		DPRINT1("MixerLine Expected %lu but got %lu\n", sizeof(MIXERLINEW), MixerLine->cbStruct);
		return MM_STATUS_INVALID_PARAMETER;
	}


    /* clear hmixer from flags */
    Flags &=~MIXER_OBJECTF_HMIXER;

    DPRINT1("MMixerGetLineInfo MixerId %lu Flags %lu\n", MixerId, Flags);

    if (Flags == MIXER_GETLINEINFOF_DESTINATION)
    {
        /* cast to mixer info */
        MixerInfo = (LPMIXER_INFO)MixerHandle;

        /* calculate destination line id */
        DestinationLineID = (MixerLine->dwDestination + DESTINATION_LINE);

        /* get destination line */
        MixerLineSrc = MMixerGetSourceMixerLineByLineId(MixerInfo, DestinationLineID);

        if (MixerLineSrc == NULL)
        {
            DPRINT1("MixerCaps Name %S DestinationLineCount %lu dwDestination %lu not found\n", MixerInfo->MixCaps.szPname, MixerInfo->MixCaps.cDestinations, MixerLine->dwDestination);
            return MM_STATUS_UNSUCCESSFUL;
        }
        /* copy mixer line */
        MixerContext->Copy(MixerLine, &MixerLineSrc->Line, sizeof(MIXERLINEW));

        /* make sure it is null terminated */
        MixerLine->szName[MIXER_LONG_NAME_CHARS-1] = L'\0';
        MixerLine->szShortName[MIXER_SHORT_NAME_CHARS-1] = L'\0';
        MixerLine->Target.szPname[MAXPNAMELEN-1] = L'\0';

        /* done */
        return MM_STATUS_SUCCESS;
    }
    else if (Flags == MIXER_GETLINEINFOF_SOURCE)
    {
        /* cast to mixer info */
        MixerInfo = (LPMIXER_INFO)MixerHandle;

        /* calculate destination line id */
        DestinationLineID = (MixerLine->dwDestination + DESTINATION_LINE);

        /* get destination line */
        MixerLineSrc = MMixerGetSourceMixerLineByLineId(MixerInfo, DestinationLineID);

        if (MixerLineSrc == NULL)
        {
            DPRINT1("MixerCaps Name %S DestinationLineCount %lu dwDestination %lu not found\n", MixerInfo->MixCaps.szPname, MixerInfo->MixCaps.cDestinations, MixerLine->dwDestination);
            return MM_STATUS_UNSUCCESSFUL;
        }

        /* check if dwSource is out of bounds */
        if (MixerLine->dwSource >= MixerLineSrc->Line.cConnections)
        {
            DPRINT1("MixerCaps Name %S MixerLineName %S Connections %lu dwSource %lu not found\n", MixerInfo->MixCaps.szPname, MixerLineSrc->Line.szName, MixerLineSrc->Line.cConnections, MixerLine->dwSource);
            return MM_STATUS_UNSUCCESSFUL;
        }

        /* calculate destination line id */
        DestinationLineID = (MixerLine->dwSource * SOURCE_LINE) + MixerLine->dwDestination;

        DPRINT("MixerName %S cDestinations %lu MixerLineName %S cConnections %lu dwSource %lu dwDestination %lu ID %lx\n", MixerInfo->MixCaps.szPname, MixerInfo->MixCaps.cDestinations,
                                                                                                                            MixerLineSrc->Line.szName, MixerLineSrc->Line.cConnections,
                                                                                                                            MixerLine->dwSource, MixerLine->dwDestination,
                                                                                                                            DestinationLineID);
        /* get target destination line id */
        MixerLineSrc = MMixerGetSourceMixerLineByLineId(MixerInfo, DestinationLineID);

        /* sanity check */
        ASSERT(MixerLineSrc);

        DPRINT("Line %u Name %S\n", MixerLineSrc->Line.dwSource, MixerLineSrc->Line.szName);

        /* copy mixer line */
        MixerContext->Copy(MixerLine, &MixerLineSrc->Line, sizeof(MIXERLINEW));

        /* make sure it is null terminated */
        MixerLine->szName[MIXER_LONG_NAME_CHARS-1] = L'\0';
        MixerLine->szShortName[MIXER_SHORT_NAME_CHARS-1] = L'\0';
        MixerLine->Target.szPname[MAXPNAMELEN-1] = L'\0';

        /* done */
        return MM_STATUS_SUCCESS;
    }
    else if (Flags == MIXER_GETLINEINFOF_LINEID)
    {
        /* cast to mixer info */
        MixerInfo = (LPMIXER_INFO)MixerHandle;

        /* try to find line */
        MixerLineSrc = MMixerGetSourceMixerLineByLineId(MixerInfo, MixerLine->dwLineID);
        if (!MixerLineSrc)
        {
            /* invalid parameter */
            DPRINT1("MMixerGetLineInfo: MixerName %S Line not found %lu\n", MixerInfo->MixCaps.szPname, MixerLine->dwLineID);
            return MM_STATUS_INVALID_PARAMETER;
        }

        DPRINT("Line %u Name %S\n", MixerLineSrc->Line.dwSource, MixerLineSrc->Line.szName);

        /* copy mixer line*/
        MixerContext->Copy(MixerLine, &MixerLineSrc->Line, sizeof(MIXERLINEW));

        /* make sure it is null terminated */
        MixerLine->szName[MIXER_LONG_NAME_CHARS-1] = L'\0';
        MixerLine->szShortName[MIXER_SHORT_NAME_CHARS-1] = L'\0';
        MixerLine->Target.szPname[MAXPNAMELEN-1] = L'\0';

        return MM_STATUS_SUCCESS;
    }
    else if (Flags == MIXER_GETLINEINFOF_COMPONENTTYPE)
    {
        /* cast to mixer info */
        MixerInfo = (LPMIXER_INFO)MixerHandle;

        /* find mixer line by component type */
        MixerLineSrc = MMixerGetSourceMixerLineByComponentType(MixerInfo, MixerLine->dwComponentType);
        if (!MixerLineSrc)
        {
            DPRINT1("Failed to find component type %x\n", MixerLine->dwComponentType);
            return MM_STATUS_UNSUCCESSFUL;
        }

        /* copy mixer line */
        MixerContext->Copy(MixerLine, &MixerLineSrc->Line, sizeof(MIXERLINEW));

        /* make sure it is null terminated */
        MixerLine->szName[MIXER_LONG_NAME_CHARS-1] = L'\0';
        MixerLine->szShortName[MIXER_SHORT_NAME_CHARS-1] = L'\0';
        MixerLine->Target.szPname[MAXPNAMELEN-1] = L'\0';

        /* done */
        return MM_STATUS_SUCCESS;
    }
    else if (Flags == MIXER_GETLINEINFOF_TARGETTYPE)
    {
        DPRINT1("MIXER_GETLINEINFOF_TARGETTYPE handling is unimplemented\n");
    }
    else
    {
        DPRINT1("Unknown Flags %lx handling is unimplemented\n", Flags);
    }

    return MM_STATUS_NOT_IMPLEMENTED;
}

MIXER_STATUS
MMixerGetLineControls(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE MixerHandle,
    IN ULONG MixerId,
    IN ULONG Flags,
    OUT LPMIXERLINECONTROLSW MixerLineControls)
{
    LPMIXER_INFO MixerInfo;
    LPMIXERLINE_EXT MixerLineSrc;
    LPMIXERCONTROLW MixerControl;
    MIXER_STATUS Status;
    ULONG Index;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    if ((Flags & (MIXER_OBJECTF_MIXER | MIXER_OBJECTF_HMIXER)) == MIXER_OBJECTF_MIXER)
    {
        /* caller passed mixer id */
        MixerHandle = (HANDLE)MMixerGetMixerInfoByIndex(MixerContext, MixerId);

        if (!MixerHandle)
        {
            /* invalid parameter */
            return MM_STATUS_INVALID_PARAMETER;
        }
    }

    Flags &= ~MIXER_OBJECTF_HMIXER;

    DPRINT("MMixerGetLineControls MixerId %lu Flags %lu\n", MixerId, Flags);


    if (Flags == MIXER_GETLINECONTROLSF_ALL)
    {
        /* cast to mixer info */
        MixerInfo = (LPMIXER_INFO)MixerHandle;

        MixerLineSrc = MMixerGetSourceMixerLineByLineId(MixerInfo, MixerLineControls->dwLineID);

        if (!MixerLineSrc)
        {
            /* invalid line id */
            DPRINT("MMixerGetLineControls Line not found %lx\n", MixerLineControls->dwLineID);
            return MM_STATUS_INVALID_PARAMETER;
        }
        /* copy line control(s) */
        MixerContext->Copy(MixerLineControls->pamxctrl, MixerLineSrc->LineControls, min(MixerLineSrc->Line.cControls, MixerLineControls->cControls) * sizeof(MIXERCONTROLW));

        return MM_STATUS_SUCCESS;
    }
    else if (Flags == MIXER_GETLINECONTROLSF_ONEBYTYPE)
    {
        /* cast to mixer info */
        MixerInfo = (LPMIXER_INFO)MixerHandle;

        /* get mixer line */
        MixerLineSrc = MMixerGetSourceMixerLineByLineId(MixerInfo, MixerLineControls->dwLineID);

        if (!MixerLineSrc)
        {
            /* invalid line id */
            DPRINT1("MMixerGetLineControls Line not found %lx\n", MixerLineControls->dwLineID);
            return MM_STATUS_INVALID_PARAMETER;
        }

        /* sanity checks */
        ASSERT(MixerLineControls->cControls == 1);
        ASSERT(MixerLineControls->cbmxctrl == sizeof(MIXERCONTROLW));
        ASSERT(MixerLineControls->pamxctrl != NULL);

        Index = 0;
        for(Index = 0; Index < MixerLineSrc->Line.cControls; Index++)
        {
            DPRINT1("dwControlType %x\n", MixerLineSrc->LineControls[Index].dwControlType);
            if (MixerLineControls->dwControlType == MixerLineSrc->LineControls[Index].dwControlType)
            {
                /* found a control with that type */
                MixerContext->Copy(MixerLineControls->pamxctrl, &MixerLineSrc->LineControls[Index], sizeof(MIXERCONTROLW));
                return MM_STATUS_SUCCESS;
            }
        }
        DPRINT("DeviceInfo->u.MixControls.dwControlType %x not found in Line %x cControls %u \n", MixerLineControls->dwControlType, MixerLineControls->dwLineID, MixerLineSrc->Line.cControls);
        return MM_STATUS_UNSUCCESSFUL;
    }
    else if (Flags == MIXER_GETLINECONTROLSF_ONEBYID)
    {
        /* cast to mixer info */
        MixerInfo = (LPMIXER_INFO)MixerHandle;

        Status = MMixerGetMixerControlById(MixerInfo, MixerLineControls->dwControlID, NULL, &MixerControl, NULL);

        if (Status != MM_STATUS_SUCCESS)
        {
            /* invalid parameter */
            DPRINT("MMixerGetLineControls ControlID not found %lx\n", MixerLineControls->dwLineID);
            return MM_STATUS_INVALID_PARAMETER;
        }

        ASSERT(MixerLineControls->cControls == 1);
        ASSERT(MixerLineControls->cbmxctrl == sizeof(MIXERCONTROLW));
        ASSERT(MixerLineControls->pamxctrl != NULL);

       DPRINT("MMixerGetLineControls ControlID %lx ControlType %lx Name %S\n", MixerControl->dwControlID, MixerControl->dwControlType, MixerControl->szName);

        /* copy the controls */
        MixerContext->Copy(MixerLineControls->pamxctrl, MixerControl, sizeof(MIXERCONTROLW));
        MixerLineControls->pamxctrl->szName[MIXER_LONG_NAME_CHARS-1] = L'\0';
        MixerLineControls->pamxctrl->szShortName[MIXER_SHORT_NAME_CHARS-1] = L'\0';

        return MM_STATUS_SUCCESS;
    }
    UNIMPLEMENTED
    return MM_STATUS_NOT_IMPLEMENTED;
}

MIXER_STATUS
MMixerSetControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE MixerHandle,
    IN ULONG MixerId,
    IN ULONG Flags,
    OUT LPMIXERCONTROLDETAILS MixerControlDetails)
{
    MIXER_STATUS Status;
    ULONG NodeId;
    LPMIXER_INFO MixerInfo;
    LPMIXERLINE_EXT MixerLine;
    LPMIXERCONTROLW MixerControl;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    if ((Flags & (MIXER_OBJECTF_MIXER | MIXER_OBJECTF_HMIXER)) == MIXER_OBJECTF_MIXER)
    {
        /* caller passed mixer id */
        MixerHandle = (HANDLE)MMixerGetMixerInfoByIndex(MixerContext, MixerId);

        if (!MixerHandle)
        {
            /* invalid parameter */
            return MM_STATUS_INVALID_PARAMETER;
        }
    }

    /* get mixer info */
    MixerInfo = (LPMIXER_INFO)MixerHandle;

    /* get mixer control */
     Status = MMixerGetMixerControlById(MixerInfo, MixerControlDetails->dwControlID, &MixerLine, &MixerControl, &NodeId);

    /* check for success */
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to find control id */
        return MM_STATUS_INVALID_PARAMETER;
    }

    switch(MixerControl->dwControlType)
    {
        case MIXERCONTROL_CONTROLTYPE_MUTE:
            Status = MMixerSetGetMuteControlDetails(MixerContext, MixerInfo->hMixer, NodeId, MixerLine->Line.dwLineID, MixerControlDetails, TRUE);
            break;
        case MIXERCONTROL_CONTROLTYPE_VOLUME:
            Status = MMixerSetGetVolumeControlDetails(MixerContext, MixerInfo->hMixer, NodeId, TRUE, MixerControl, MixerControlDetails, MixerLine);
            break;
        default:
            Status = MM_STATUS_NOT_IMPLEMENTED;
    }

    return Status;
}

MIXER_STATUS
MMixerGetControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE MixerHandle,
    IN ULONG MixerId,
    IN ULONG Flags,
    OUT LPMIXERCONTROLDETAILS MixerControlDetails)
{
    MIXER_STATUS Status;
    ULONG NodeId;
    LPMIXER_INFO MixerInfo;
    LPMIXERLINE_EXT MixerLine;
    LPMIXERCONTROLW MixerControl;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    if ((Flags & (MIXER_OBJECTF_MIXER | MIXER_OBJECTF_HMIXER)) == MIXER_OBJECTF_MIXER)
    {
        /* caller passed mixer id */
        MixerHandle = (HANDLE)MMixerGetMixerInfoByIndex(MixerContext, MixerId);

        if (!MixerHandle)
        {
            /* invalid parameter */
            return MM_STATUS_INVALID_PARAMETER;
        }
    }

    /* get mixer info */
    MixerInfo = (LPMIXER_INFO)MixerHandle;

    /* get mixer control */
     Status = MMixerGetMixerControlById(MixerInfo, MixerControlDetails->dwControlID, &MixerLine, &MixerControl, &NodeId);

    /* check for success */
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to find control id */
        return MM_STATUS_INVALID_PARAMETER;
    }

    switch(MixerControl->dwControlType)
    {
        case MIXERCONTROL_CONTROLTYPE_MUTE:
            Status = MMixerSetGetMuteControlDetails(MixerContext, MixerInfo, NodeId, MixerLine->Line.dwLineID, MixerControlDetails, FALSE);
            break;
        case MIXERCONTROL_CONTROLTYPE_VOLUME:
            Status = MMixerSetGetVolumeControlDetails(MixerContext, MixerInfo, NodeId, FALSE, MixerControl, MixerControlDetails, MixerLine);
            break;
        default:
            Status = MM_STATUS_NOT_IMPLEMENTED;
    }

    return Status;
}

VOID
MMixerPrintMixers(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList)
{
    ULONG Index, SubIndex, DestinationLineID;
    LPMIXER_INFO MixerInfo;
    LPMIXERLINE_EXT DstMixerLine;

    DPRINT1("MixerList %p\n", MixerList);
    DPRINT1("MidiInCount %lu\n", MixerList->MidiInListCount);
    DPRINT1("MidiOutCount %lu\n", MixerList->MidiOutListCount);
    DPRINT1("WaveInCount %lu\n", MixerList->WaveInListCount);
    DPRINT1("WaveOutCount %lu\n", MixerList->WaveOutListCount);
    DPRINT1("MixerCount %p\n", MixerList->MixerListCount);


    for(Index = 0; Index < MixerList->MixerListCount; Index++)
    {
        /* get mixer info */
        MixerInfo = MMixerGetMixerInfoByIndex(MixerContext, Index);

        ASSERT(MixerInfo);
        DPRINT1("\n");
        DPRINT1("Name :%S\n", MixerInfo->MixCaps.szPname);
        DPRINT1("cDestinations: %lu\n", MixerInfo->MixCaps.cDestinations);
        DPRINT1("fdwSupport %lu\n", MixerInfo->MixCaps.fdwSupport);
        DPRINT1("vDriverVersion %lx\n", MixerInfo->MixCaps.vDriverVersion);
        DPRINT1("wMid %lx\n", MixerInfo->MixCaps.wMid);
        DPRINT1("wPid %lx\n", MixerInfo->MixCaps.wPid);

        for(SubIndex = 0; SubIndex < MixerInfo->MixCaps.cDestinations; SubIndex++)
        {
            /* calculate destination line id */
            DestinationLineID = (SubIndex + DESTINATION_LINE);

            /* get destination line */
            DstMixerLine = MMixerGetSourceMixerLineByLineId(MixerInfo, DestinationLineID);
            DPRINT1("\n");
            DPRINT1("cChannels %lu\n", DstMixerLine->Line.cChannels);
            DPRINT1("cConnections %lu\n", DstMixerLine->Line.cConnections);
            DPRINT1("cControls %lu\n", DstMixerLine->Line.cControls);
            DPRINT1("dwComponentType %lx\n", DstMixerLine->Line.dwComponentType);
            DPRINT1("dwDestination %lu\n", DstMixerLine->Line.dwDestination);
            DPRINT1("dwLineID %lx\n", DstMixerLine->Line.dwLineID);
            DPRINT1("dwSource %lx\n", DstMixerLine->Line.dwSource);
            DPRINT1("dwUser %lu\n", DstMixerLine->Line.dwUser);
            DPRINT1("fdwLine %lu\n", DstMixerLine->Line.fdwLine);
            DPRINT1("szName %S\n", DstMixerLine->Line.szName);
            DPRINT1("szShortName %S\n", DstMixerLine->Line.szShortName);
            DPRINT1("Target.dwDeviceId %lu\n", DstMixerLine->Line.Target.dwDeviceID);
            DPRINT1("Target.dwType %lu\n", DstMixerLine->Line.Target.dwType);
            DPRINT1("Target.szName %S\n", DstMixerLine->Line.Target.szPname);
            DPRINT1("Target.vDriverVersion %lx\n", DstMixerLine->Line.Target.vDriverVersion);
            DPRINT1("Target.wMid %lx\n", DstMixerLine->Line.Target.wMid );
            DPRINT1("Target.wPid %lx\n", DstMixerLine->Line.Target.wPid);
        }
    }
}

MIXER_STATUS
MMixerInitialize(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_ENUM EnumFunction,
    IN PVOID EnumContext)
{
    MIXER_STATUS Status;
    HANDLE hMixer, hKey;
    ULONG DeviceIndex, Count;
    LPWSTR DeviceName;
    LPMIXER_DATA MixerData;
    PMIXER_LIST MixerList;
    PLIST_ENTRY Entry;

    if (!MixerContext || !EnumFunction || !EnumContext)
    {
        /* invalid parameter */
        return MM_STATUS_INVALID_PARAMETER;
    }

    if (!MixerContext->Alloc || !MixerContext->Control || !MixerContext->Free || !MixerContext->Open ||
        !MixerContext->AllocEventData || !MixerContext->FreeEventData ||
        !MixerContext->Close || !MixerContext->OpenKey || !MixerContext->QueryKeyValue || !MixerContext->CloseKey)
    {
        /* invalid parameter */
        return MM_STATUS_INVALID_PARAMETER;
    }

    /* allocate a mixer list */
    MixerList = (PMIXER_LIST)MixerContext->Alloc(sizeof(MIXER_LIST));
    if (!MixerList)
    {
        /* no memory */
        return MM_STATUS_NO_MEMORY;
    }

     /* initialize mixer list */
     MixerList->MixerListCount = 0;
     MixerList->MixerDataCount = 0;
     MixerList->WaveInListCount = 0;
     MixerList->WaveOutListCount = 0;
     MixerList->MidiInListCount = 0;
     MixerList->MidiOutListCount = 0;
     InitializeListHead(&MixerList->MixerList);
     InitializeListHead(&MixerList->MixerData);
     InitializeListHead(&MixerList->WaveInList);
     InitializeListHead(&MixerList->WaveOutList);
     InitializeListHead(&MixerList->MidiInList);
     InitializeListHead(&MixerList->MidiOutList);

     /* store mixer list */
     MixerContext->MixerContext = (PVOID)MixerList;

    /* start enumerating all available devices */
    Count = 0;
    DeviceIndex = 0;

    do
    {
        /* enumerate a device */
        Status = EnumFunction(EnumContext, DeviceIndex, &DeviceName, &hMixer, &hKey);

        if (Status != MM_STATUS_SUCCESS)
        {
            /* check error code */
            if (Status == MM_STATUS_NO_MORE_DEVICES)
            {
                /* enumeration has finished */
                break;
            }
            else
            {
                DPRINT1("Failed to enumerate device %lu\n", DeviceIndex);

                /* TODO cleanup */
                return Status;
            }
        }
        else
        {
            /* create a mixer data entry */
            Status = MMixerCreateMixerData(MixerContext, MixerList, DeviceIndex, DeviceName, hMixer, hKey);
            if (Status != MM_STATUS_SUCCESS)
                break;
        }

        /* increment device index */
        DeviceIndex++;
    }while(TRUE);

    /* now all filters have been pre-opened
     * lets enumerate the filters
     */
    Entry = MixerList->MixerData.Flink;
    while(Entry != &MixerList->MixerData)
    {
        MixerData = (LPMIXER_DATA)CONTAINING_RECORD(Entry, MIXER_DATA, Entry);
        MMixerSetupFilter(MixerContext, MixerList, MixerData, &Count);
        Entry = Entry->Flink;
    }

    MMixerPrintMixers(MixerContext, MixerList);

    /* done */
    return MM_STATUS_SUCCESS;
}
