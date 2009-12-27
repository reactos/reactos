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

HANDLE KernelHandle = INVALID_HANDLE_VALUE;

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

#ifdef USE_MMIXER_LIB
    Result = WdmAudGetNumDevsByMMixer(DeviceType, &DeviceCount);
#else
    Result = WdmAudGetNumWdmDevsByLegacy(DeviceType, &DeviceCount);
#endif

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
#ifdef USE_MMIXER_LIB
        FuncTable.GetCapabilities = WdmAudGetCapabilitiesByMMixer;
        FuncTable.Open = WdmAudOpenSoundDeviceByMMixer;
        FuncTable.Close = WdmAudCloseSoundDeviceByMMixer;
        FuncTable.GetDeviceInterfaceString = WdmAudGetDeviceInterfaceStringByMMixer;
#else
        FuncTable.GetCapabilities = WdmAudGetCapabilitiesByLegacy;
        FuncTable.Open = WdmAudOpenSoundDeviceByLegacy;
        FuncTable.Close = WdmAudCloseSoundDeviceByLegacy;
        FuncTable.GetDeviceInterfaceString = WdmAudGetDeviceInterfaceStringByLegacy;
#endif

        FuncTable.QueryWaveFormatSupport = QueryWdmWaveDeviceFormatSupport;
        if (DeviceType == MIXER_DEVICE_TYPE)
        {
#ifdef USE_MMIXER_LIB
            FuncTable.SetWaveFormat = WdmAudSetMixerDeviceFormatByMMixer;
            FuncTable.QueryMixerInfo = WdmAudQueryMixerInfoByMMixer;
#else
            FuncTable.SetWaveFormat = WdmAudSetMixerDeviceFormatByLegacy;
            FuncTable.QueryMixerInfo = WdmAudQueryMixerInfoByLegacy;
#endif
        }

        if (DeviceType == WAVE_IN_DEVICE_TYPE || DeviceType == WAVE_OUT_DEVICE_TYPE)
        {
#ifdef USE_MMIXER_LIB
            FuncTable.SetWaveFormat = WdmAudSetWdmWaveDeviceFormatByMMixer;
            FuncTable.SetState = WdmAudSetWdmWaveStateByMMixer;
            FuncTable.ResetStream = WdmAudResetStreamByMMixer;
            FuncTable.GetPos = WdmAudGetWdmPositionByMMixer;
#else
            FuncTable.SetWaveFormat = WdmAudSetWaveDeviceFormatByLegacy;
            FuncTable.SetState = WdmAudSetWaveStateByLegacy;
            FuncTable.ResetStream = WdmAudResetStreamByLegacy;
            FuncTable.GetPos = WdmAudGetWavePositionByLegacy;
#endif

#ifdef USE_MMIXER_LIB
            FuncTable.CommitWaveBuffer = WdmAudCommitWaveBufferByMMixer;
#elif defined (USERMODE_MIXER)
            FuncTable.CommitWaveBuffer = WriteFileEx_Remixer;
#else
            FuncTable.CommitWaveBuffer = WriteFileEx_Committer2;
#endif
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
    MMRESULT Result;

    switch ( Message )
    {
        case DRV_LOAD :
        {
            SND_TRACE(L"DRV_LOAD\n");

            Result = InitEntrypointMutexes();

            if ( ! MMSUCCESS(Result) )
                return 0L;

#ifdef USE_MMIXER_LIB
            if (!WdmAudInitUserModeMixer())
            {
                SND_ERR(L"Failed to initialize mmixer lib\n");
                return 0;
            }
#else
            if (WdmAudOpenSoundDeviceByLegacy() != MMSYSERR_NOERROR)
            {
                SND_ERR(L"Failed to open \\\\.\\wdmaud\n");
                CleanupEntrypointMutexes();

                //UnlistAllSoundDevices();

                return 0L;
            }
#endif

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

#ifdef USE_MMIXER_LIB
            WdmAudCleanupMMixer();
#else
            WdmAudCleanupLegacy();
#endif

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
