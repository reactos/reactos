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

    // verify mixer context
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        // invalid context passed
        return Status;
    }

    // grab mixer list
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

    // verify mixer context
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        // invalid context passed
        return Status;
    }

    // get mixer info
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
    wcscpy(MixerCaps->szPname, MixerInfo->MixCaps.szPname);

    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerOpen(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG MixerId,
    IN PVOID MixerEvent,
    IN PMIXER_EVENT MixerEventRoutine,
    OUT PHANDLE MixerHandle)
{
    MIXER_STATUS Status;
    LPMIXER_INFO MixerInfo;

    // verify mixer context
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        // invalid context passed
        return Status;
    }

    MixerInfo = (LPMIXER_INFO)MMixerGetMixerInfoByIndex(MixerContext, MixerId);
    if (!MixerInfo)
    {
        // invalid mixer id
        return MM_STATUS_INVALID_PARAMETER;
    }

    // FIXME
    // handle event notification

    // store result
    *MixerHandle = (HANDLE)MixerInfo;

    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerGetLineInfo(
    IN PMIXER_CONTEXT MixerContext,
    IN  HANDLE MixerHandle,
    IN  ULONG Flags,
    OUT LPMIXERLINEW MixerLine)
{
    MIXER_STATUS Status;
    LPMIXER_INFO MixerInfo;
    LPMIXERLINE_EXT MixerLineSrc;

    // verify mixer context
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        // invalid context passed
        return Status;
    }

    // clear hmixer from flags
    Flags &=~MIXER_OBJECTF_HMIXER;

    if (Flags == MIXER_GETLINEINFOF_DESTINATION)
    {
        // cast to mixer info
        MixerInfo = (LPMIXER_INFO)MixerHandle;

        if (MixerLine->dwDestination != 0)
        {
            // destination line member must be zero
            return MM_STATUS_INVALID_PARAMETER;
        }

        MixerLineSrc = MMixerGetSourceMixerLineByLineId(MixerInfo, DESTINATION_LINE);
        ASSERT(MixerLineSrc);
        MixerContext->Copy(MixerLine, &MixerLineSrc->Line, sizeof(MIXERLINEW));

        return MM_STATUS_SUCCESS;
    }
    else if (Flags == MIXER_GETLINEINFOF_SOURCE)
    {
        // cast to mixer info
        MixerInfo = (LPMIXER_INFO)MixerHandle;


        MixerLineSrc = MMixerGetSourceMixerLineByLineId(MixerInfo, DESTINATION_LINE);
        ASSERT(MixerLineSrc);

        if (MixerLine->dwSource >= MixerLineSrc->Line.cConnections)
        {
            DPRINT1("dwSource %u > Destinations %u\n", MixerLine->dwSource, MixerLineSrc->Line.cConnections);

            // invalid parameter
            return MM_STATUS_INVALID_PARAMETER;
        }

        MixerLineSrc = MMixerGetSourceMixerLineByLineId(MixerInfo, MixerLine->dwSource);
        if (MixerLineSrc)
        {
            DPRINT("Line %u Name %S\n", MixerLineSrc->Line.dwSource, MixerLineSrc->Line.szName);
            MixerContext->Copy(MixerLine, &MixerLineSrc->Line, sizeof(MIXERLINEW));
            return MM_STATUS_SUCCESS;
        }
        return MM_STATUS_UNSUCCESSFUL;
    }
    else if (Flags == MIXER_GETLINEINFOF_LINEID)
    {
        // cast to mixer info
        MixerInfo = (LPMIXER_INFO)MixerHandle;

        MixerLineSrc = MMixerGetSourceMixerLineByLineId(MixerInfo, MixerLine->dwLineID);
        if (!MixerLineSrc)
        {
            // invalid parameter
            return MM_STATUS_INVALID_PARAMETER;
        }

        /* copy cached data */
        MixerContext->Copy(MixerLine, &MixerLineSrc->Line, sizeof(MIXERLINEW));
        return MM_STATUS_SUCCESS;
    }
    else if (Flags == MIXER_GETLINEINFOF_COMPONENTTYPE)
    {
        // cast to mixer info
        MixerInfo = (LPMIXER_INFO)MixerHandle;

        MixerLineSrc = MMixerGetSourceMixerLineByComponentType(MixerInfo, MixerLine->dwComponentType);
        if (!MixerLineSrc)
        {
            DPRINT1("Failed to find component type %x\n", MixerLine->dwComponentType);
            return MM_STATUS_UNSUCCESSFUL;
        }

        ASSERT(MixerLineSrc);

        /* copy cached data */
        MixerContext->Copy(MixerLine, &MixerLineSrc->Line, sizeof(MIXERLINEW));
        return MM_STATUS_SUCCESS;
    }

    return MM_STATUS_NOT_IMPLEMENTED;
}

MIXER_STATUS
MMixerGetLineControls(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE MixerHandle,
    IN ULONG Flags,
    OUT LPMIXERLINECONTROLSW MixerLineControls)
{
    LPMIXER_INFO MixerInfo;
    LPMIXERLINE_EXT MixerLineSrc;
    LPMIXERCONTROLW MixerControl;
    MIXER_STATUS Status;
    ULONG Index;

    // verify mixer context
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        // invalid context passed
        return Status;
    }

    Flags &= ~MIXER_OBJECTF_HMIXER;

    if (Flags == MIXER_GETLINECONTROLSF_ALL)
    {
        // cast to mixer info
        MixerInfo = (LPMIXER_INFO)MixerHandle;

        MixerLineSrc = MMixerGetSourceMixerLineByLineId(MixerInfo, MixerLineControls->dwLineID);

        if (!MixerLineSrc)
        {
            // invalid line id
            return MM_STATUS_INVALID_PARAMETER;
        }
        // copy line control(s)
        MixerContext->Copy(MixerLineControls->pamxctrl, MixerLineSrc->LineControls, min(MixerLineSrc->Line.cControls, MixerLineControls->cControls) * sizeof(MIXERCONTROLW));

        return MM_STATUS_SUCCESS;
    }
    else if (Flags == MIXER_GETLINECONTROLSF_ONEBYTYPE)
    {
        // cast to mixer info
        MixerInfo = (LPMIXER_INFO)MixerHandle;

        MixerLineSrc = MMixerGetSourceMixerLineByLineId(MixerInfo, MixerLineControls->dwLineID);

        if (!MixerLineSrc)
        {
            // invalid line id
            return MM_STATUS_INVALID_PARAMETER;
        }

        ASSERT(MixerLineSrc);

        Index = 0;
        for(Index = 0; Index < MixerLineSrc->Line.cControls; Index++)
        {
            DPRINT("dwControlType %x\n", MixerLineSrc->LineControls[Index].dwControlType);
            if (MixerLineControls->dwControlType == MixerLineSrc->LineControls[Index].dwControlType)
            {
                // found a control with that type
                MixerContext->Copy(MixerLineControls->pamxctrl, &MixerLineSrc->LineControls[Index], sizeof(MIXERCONTROLW));
                return MM_STATUS_SUCCESS;
            }
        }
        DPRINT("DeviceInfo->u.MixControls.dwControlType %x not found in Line %x cControls %u \n", MixerLineControls->dwControlType, MixerLineControls->dwLineID, MixerLineSrc->Line.cControls);
        return MM_STATUS_UNSUCCESSFUL;
    }
    else if (Flags == MIXER_GETLINECONTROLSF_ONEBYID)
    {
        // cast to mixer info
        MixerInfo = (LPMIXER_INFO)MixerHandle;

        Status = MMixerGetMixerControlById(MixerInfo, MixerLineControls->dwControlID, NULL, &MixerControl, NULL);

        if (Status != MM_STATUS_SUCCESS)
        {
            // invalid parameter
            return MM_STATUS_INVALID_PARAMETER;
        }

        // copy the controls
        MixerContext->Copy(MixerLineControls->pamxctrl, MixerControl, sizeof(MIXERCONTROLW));
        return MM_STATUS_SUCCESS;
    }


    return MM_STATUS_NOT_IMPLEMENTED;
}

MIXER_STATUS
MMixerSetControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE MixerHandle,
    IN ULONG Flags,
    OUT LPMIXERCONTROLDETAILS MixerControlDetails)
{
    MIXER_STATUS Status;
    ULONG NodeId;
    LPMIXER_INFO MixerInfo;
    LPMIXERLINE_EXT MixerLine;
    LPMIXERCONTROLW MixerControl;

    // verify mixer context
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        // invalid context passed
        return Status;
    }

    // get mixer info
    MixerInfo = (LPMIXER_INFO)MixerHandle;

    // get mixer control
     Status = MMixerGetMixerControlById(MixerInfo, MixerControlDetails->dwControlID, &MixerLine, &MixerControl, &NodeId);

    // check for success
    if (Status != MM_STATUS_SUCCESS)
    {
        // failed to find control id
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
    IN ULONG Flags,
    OUT LPMIXERCONTROLDETAILS MixerControlDetails)
{
    MIXER_STATUS Status;
    ULONG NodeId;
    LPMIXER_INFO MixerInfo;
    LPMIXERLINE_EXT MixerLine;
    LPMIXERCONTROLW MixerControl;

    // verify mixer context
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        // invalid context passed
        return Status;
    }

    // get mixer info
    MixerInfo = (LPMIXER_INFO)MixerHandle;

    // get mixer control
     Status = MMixerGetMixerControlById(MixerInfo, MixerControlDetails->dwControlID, &MixerLine, &MixerControl, &NodeId);

    // check for success
    if (Status != MM_STATUS_SUCCESS)
    {
        // failed to find control id
        return MM_STATUS_INVALID_PARAMETER;
    }

    switch(MixerControl->dwControlType)
    {
        case MIXERCONTROL_CONTROLTYPE_MUTE:
            Status = MMixerSetGetMuteControlDetails(MixerContext, MixerInfo->hMixer, NodeId, MixerLine->Line.dwLineID, MixerControlDetails, FALSE);
            break;
        case MIXERCONTROL_CONTROLTYPE_VOLUME:
            Status = MMixerSetGetVolumeControlDetails(MixerContext, MixerInfo->hMixer, NodeId, FALSE, MixerControl, MixerControlDetails, MixerLine);
            break;
        default:
            Status = MM_STATUS_NOT_IMPLEMENTED;
    }

    return Status;
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
        // invalid parameter
        return MM_STATUS_INVALID_PARAMETER;
    }

    if (!MixerContext->Alloc || !MixerContext->Control || !MixerContext->Free || !MixerContext->Open || 
        !MixerContext->Close || !MixerContext->OpenKey || !MixerContext->QueryKeyValue || !MixerContext->CloseKey)
    {
        // invalid parameter
        return MM_STATUS_INVALID_PARAMETER;
    }

    // allocate a mixer list
    MixerList = (PMIXER_LIST)MixerContext->Alloc(sizeof(MIXER_LIST));
    if (!MixerList)
    {
        // no memory
        return MM_STATUS_NO_MEMORY;
    }

     //initialize mixer list
     MixerList->MixerListCount = 0;
     MixerList->MixerDataCount = 0;
     MixerList->WaveInListCount = 0;
     MixerList->WaveOutListCount = 0;
     InitializeListHead(&MixerList->MixerList);
     InitializeListHead(&MixerList->MixerData);
     InitializeListHead(&MixerList->WaveInList);
     InitializeListHead(&MixerList->WaveOutList);


     // store mixer list
     MixerContext->MixerContext = (PVOID)MixerList;

    // start enumerating all available devices
    Count = 0;
    DeviceIndex = 0;

    do
    {
        // enumerate a device
        Status = EnumFunction(EnumContext, DeviceIndex, &DeviceName, &hMixer, &hKey);

        if (Status != MM_STATUS_SUCCESS)
        {
            //check error code
            if (Status == MM_STATUS_NO_MORE_DEVICES)
            {
                // enumeration has finished
                break;
            }
        }
        else
        {
            // create a mixer data entry
            Status = MMixerCreateMixerData(MixerContext, MixerList, DeviceIndex, DeviceName, hMixer, hKey);
            if (Status != MM_STATUS_SUCCESS)
                break;
        }

        // increment device index
        DeviceIndex++;
    }while(TRUE);

    //now all filters have been pre-opened
    // lets enumerate the filters
    Entry = MixerList->MixerData.Flink;
    while(Entry != &MixerList->MixerData)
    {
        MixerData = (LPMIXER_DATA)CONTAINING_RECORD(Entry, MIXER_DATA, Entry);
        MMixerSetupFilter(MixerContext, MixerList, MixerData, &Count);
        Entry = Entry->Flink;
    }

    // done
    return MM_STATUS_SUCCESS;
}
