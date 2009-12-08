/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/mmixer.c
 * PURPOSE:         Mixer Handling Functions
 * PROGRAMMER:      Johannes Anderwald
 */



#include "priv.h"

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


