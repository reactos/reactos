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
    IN PVOID MixerEvent,
    IN PMIXER_EVENT MixerEventRoutine,
    OUT PHANDLE MixerHandle)
{
    MIXER_STATUS Status;

    // verify mixer context
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        // invalid context passed
        return Status;
    }

    return MM_STATUS_NOT_IMPLEMENTED;
}

MIXER_STATUS
MMixerGetLineInfo(
    IN PMIXER_CONTEXT MixerContext,
    IN  HANDLE MixerHandle,
    IN  ULONG Flags,
    OUT LPMIXERLINEW MixerLine)
{
    MIXER_STATUS Status;

    // verify mixer context
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        // invalid context passed
        return Status;
    }
    return MM_STATUS_NOT_IMPLEMENTED;
}

MIXER_STATUS
MMixerGetLineControls(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE MixerHandle,
    IN ULONG Flags,
    OUT LPMIXERLINECONTROLS MixerLineControls)
{
    MIXER_STATUS Status;

    // verify mixer context
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        // invalid context passed
        return Status;
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

    // verify mixer context
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        // invalid context passed
        return Status;
    }
    return MM_STATUS_NOT_IMPLEMENTED;
}

MIXER_STATUS
MMixerGetControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE MixerHandle,
    IN ULONG Flags,
    OUT LPMIXERCONTROLDETAILS MixerControlDetails)
{
    MIXER_STATUS Status;

    // verify mixer context
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        // invalid context passed
        return Status;
    }

    return MM_STATUS_NOT_IMPLEMENTED;
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
    PMIXER_LIST MixerList;

    if (!MixerContext || !EnumFunction || !EnumContext)
    {
        // invalid parameter
        return MM_STATUS_INVALID_PARAMETER;
    }

    if (!MixerContext->Alloc || !MixerContext->Control || !MixerContext->Free || !MixerContext->Open || !MixerContext->Close)
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
     InitializeListHead(&MixerList->MixerList);

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

        Status = MMixerSetupFilter(MixerContext, MixerList, hMixer, &Count, DeviceName);

        if (Status != MM_STATUS_SUCCESS)
            break;

    }while(TRUE);

    // done
    return Status;
}
