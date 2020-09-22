/*
 * PROJECT:     ReactOS Sound System
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wdmaud.drv/wdmaud.c
 *
 * PURPOSE:     WDM Audio Driver (User-mode part)
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
 *
 * NOTES:       Looking for wodMessage & co? You won't find them here. Try
 *              the MME Buddy library, which is where these routines are
 *              actually implemented.
 *
 */

#include "wdmaud.h"

#define NDEBUG
#include <debug.h>
#include <mmebuddy_debug.h>

#define USE_MMIXER_LIB
#ifndef USE_MMIXER_LIB
#define FUNC_NAME(x) x##ByLegacy
#else
#define FUNC_NAME(x) x##ByMMixer
#endif

MMRESULT
QueryWdmWaveDeviceFormatSupport(
    IN  PSOUND_DEVICE Device,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize)
{
    /* Whatever... */
    return MMSYSERR_NOERROR;
}

MMRESULT
PopulateWdmDeviceList(
    MMDEVICE_TYPE DeviceType)
{
    MMRESULT Result;
    DWORD DeviceCount = 0;
    PSOUND_DEVICE SoundDevice = NULL;
    MMFUNCTION_TABLE FuncTable;
    DWORD i;

    VALIDATE_MMSYS_PARAMETER( IS_VALID_SOUND_DEVICE_TYPE(DeviceType) );

    Result = FUNC_NAME(WdmAudGetNumWdmDevs)(DeviceType, &DeviceCount);

    if ( ! MMSUCCESS(Result) )
    {
        SND_ERR(L"Error %d while obtaining number of devices\n", Result);
        return TranslateInternalMmResult(Result);
    }

    SND_TRACE(L"%d devices of type %d found\n", DeviceCount, DeviceType);


    for ( i = 0; i < DeviceCount; ++ i )
    {
        Result = ListSoundDevice(DeviceType, UlongToPtr(i), &SoundDevice);

        if ( ! MMSUCCESS(Result) )
        {
            SND_ERR(L"Failed to list sound device - error %d\n", Result);
            return TranslateInternalMmResult(Result);
        }

        /* Set up our function table */
        ZeroMemory(&FuncTable, sizeof(MMFUNCTION_TABLE));
        FuncTable.GetCapabilities = FUNC_NAME(WdmAudGetCapabilities);
        FuncTable.QueryWaveFormatSupport = QueryWdmWaveDeviceFormatSupport; //FIXME
        FuncTable.Open = FUNC_NAME(WdmAudOpenSoundDevice);
        FuncTable.Close = FUNC_NAME(WdmAudCloseSoundDevice);
        FuncTable.GetDeviceInterfaceString = FUNC_NAME(WdmAudGetDeviceInterfaceString);

        if (DeviceType == MIXER_DEVICE_TYPE)
        {
            FuncTable.SetWaveFormat = FUNC_NAME(WdmAudSetMixerDeviceFormat);
            FuncTable.QueryMixerInfo = FUNC_NAME(WdmAudQueryMixerInfo);
        }
        else if (DeviceType == WAVE_IN_DEVICE_TYPE || DeviceType == WAVE_OUT_DEVICE_TYPE)
        {
            FuncTable.SetWaveFormat = FUNC_NAME(WdmAudSetWaveDeviceFormat);
            FuncTable.SetState = FUNC_NAME(WdmAudSetWaveState);
            FuncTable.ResetStream = FUNC_NAME(WdmAudResetStream);
            FuncTable.GetPos = FUNC_NAME(WdmAudGetWavePosition);

#ifndef USERMODE_MIXER
            FuncTable.CommitWaveBuffer = FUNC_NAME(WdmAudCommitWaveBuffer);
#else
            FuncTable.CommitWaveBuffer = WriteFileEx_Remixer;
#endif
        }
        else if (DeviceType == MIDI_IN_DEVICE_TYPE || DeviceType == MIDI_OUT_DEVICE_TYPE)
        {
            FuncTable.SetWaveFormat = FUNC_NAME(WdmAudSetMixerDeviceFormat);
            FuncTable.SetState = FUNC_NAME(WdmAudSetWaveState);
            FuncTable.GetPos = FUNC_NAME(WdmAudGetWavePosition);
        }

        SetSoundDeviceFunctionTable(SoundDevice, &FuncTable);
    }

    return MMSYSERR_NOERROR;
}



LONG
APIENTRY
DriverProc(
    DWORD DriverId,
    HANDLE DriverHandle,
    UINT Message,
    LONG Parameter1,
    LONG Parameter2)
{
    switch ( Message )
    {
        case DRV_LOAD :
        {
            HANDLE Handle;
            MMRESULT Result;
            SND_TRACE(L"DRV_LOAD\n");

            Result = InitEntrypointMutexes();

            if ( ! MMSUCCESS(Result) )
                return 0L;

            Result = FUNC_NAME(WdmAudOpenSoundDevice)(NULL, &Handle);

            if ( Result != MMSYSERR_NOERROR )
            {
                SND_ERR(L"Failed to open \\\\.\\wdmaud\n");
                //UnlistAllSoundDevices();

                return 0L;
            }

            /* Populate the device lists */
            SND_TRACE(L"Populating device lists\n");
            PopulateWdmDeviceList(WAVE_OUT_DEVICE_TYPE);
            PopulateWdmDeviceList(WAVE_IN_DEVICE_TYPE);
            PopulateWdmDeviceList(MIDI_OUT_DEVICE_TYPE);
            PopulateWdmDeviceList(MIDI_IN_DEVICE_TYPE);
            PopulateWdmDeviceList(AUX_DEVICE_TYPE);
            PopulateWdmDeviceList(MIXER_DEVICE_TYPE);

            SND_TRACE(L"Initialisation complete\n");

            return 1L;
        }

        case DRV_FREE :
        {
            SND_TRACE(L"DRV_FREE\n");

            FUNC_NAME(WdmAudCleanup)();

            /* TODO: Clean up the path names! */
            UnlistAllSoundDevices();

            CleanupEntrypointMutexes();

            SND_TRACE(L"Unfreed memory blocks: %d\n",
                      GetMemoryAllocationCount());

            return 1L;
        }

        case DRV_ENABLE :
        case DRV_DISABLE :
        {
            SND_TRACE(L"DRV_ENABLE / DRV_DISABLE\n");
            return 1L;
        }

        case DRV_OPEN :
        case DRV_CLOSE :
        {
            SND_TRACE(L"DRV_OPEN / DRV_CLOSE\n");
            return 1L;
        }

        case DRV_QUERYCONFIGURE :
        {
            SND_TRACE(L"DRV_QUERYCONFIGURE\n");
            return 0L;
        }
        case DRV_CONFIGURE :
            return DRVCNF_OK;

        default :
            SND_TRACE(L"Unhandled message %d\n", Message);
            return DefDriverProc(DriverId,
                                 DriverHandle,
                                 Message,
                                 Parameter1,
                                 Parameter2);
    }
}


BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    switch ( fdwReason )
    {
        case DLL_PROCESS_ATTACH :
            SND_TRACE(L"WDMAUD.DRV - Process attached\n");
            break;
        case DLL_PROCESS_DETACH :
            SND_TRACE(L"WDMAUD.DRV - Process detached\n");
            break;
        case DLL_THREAD_ATTACH :
            SND_TRACE(L"WDMAUD.DRV - Thread attached\n");
            break;
        case DLL_THREAD_DETACH :
            SND_TRACE(L"WDMAUD.DRV - Thread detached\n");
            break;
    }

    return TRUE;
}
