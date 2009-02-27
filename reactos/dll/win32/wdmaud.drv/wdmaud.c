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

#include <windows.h>
#include <ntddsnd.h>
#include <sndtypes.h>
#include <mmddk.h>
#include <mmebuddy.h>

#include <ks.h>
#include <ksmedia.h>
#include "interface.h"

#define KERNEL_DEVICE_NAME      L"\\\\.\\wdmaud"

PWSTR UnknownWaveIn = L"Wave Input";
PWSTR UnknownWaveOut = L"Wave Output";
PWSTR UnknownMidiIn = L"Midi Input";
PWSTR UnknownMidiOut = L"Midi Output";

HANDLE KernelHandle = INVALID_HANDLE_VALUE;
DWORD OpenCount = 0;


MMRESULT
GetNumWdmDevs(
    IN  HANDLE Handle,
    IN  MMDEVICE_TYPE DeviceType,
    OUT DWORD* DeviceCount)
{
    MMRESULT Result;
    WDMAUD_DEVICE_INFO DeviceInfo;

    VALIDATE_MMSYS_PARAMETER( Handle != INVALID_HANDLE_VALUE );
    VALIDATE_MMSYS_PARAMETER( IS_VALID_SOUND_DEVICE_TYPE(DeviceType) );
    VALIDATE_MMSYS_PARAMETER( DeviceCount );

    ZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));
    DeviceInfo.DeviceType = DeviceType;

    Result = SyncOverlappedDeviceIoControl(Handle,
                                           IOCTL_GETNUMDEVS_TYPE,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           NULL);

    if ( ! MMSUCCESS( Result ) )
    {
        SND_ERR(L"Call to IOCTL_GETNUMDEVS_TYPE failed\n");
        *DeviceCount = 0;
        return TranslateInternalMmResult(Result);
    }

    *DeviceCount = DeviceInfo.DeviceCount;

    return MMSYSERR_NOERROR;
}

MMRESULT
GetWdmDeviceCapabilities(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PVOID Capabilities,
    IN  DWORD CapabilitiesSize)
{
    /* NOTE - At this time, WDMAUD does not support this properly */

    MMRESULT Result;
    MMDEVICE_TYPE DeviceType;

    SND_ASSERT( SoundDevice );
    SND_ASSERT( Capabilities );

    SND_TRACE(L"WDMAUD - GetWdmDeviceCapabilities\n");

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if ( ! MMSUCCESS(Result) )
        return Result;

    /* This is pretty much a big hack right now */
    switch ( DeviceType )
    {
        case WAVE_OUT_DEVICE_TYPE :
        {
            LPWAVEOUTCAPS WaveOutCaps = (LPWAVEOUTCAPS) Capabilities;
            WaveOutCaps->wMid = 0;
            WaveOutCaps->wPid = 0;
            WaveOutCaps->vDriverVersion = 0x0001;
            CopyWideString(WaveOutCaps->szPname, UnknownWaveOut);

            /* HACK: We may not really support all formats! */
            WaveOutCaps->dwFormats = 0xffffffff;
            WaveOutCaps->wChannels = 2;
            WaveOutCaps->dwSupport = 0;
            break;
        }
        case WAVE_IN_DEVICE_TYPE :
        {
            LPWAVEINCAPS WaveInCaps = (LPWAVEINCAPS) Capabilities;
            CopyWideString(WaveInCaps->szPname, UnknownWaveIn);
            /* TODO... other fields */
            break;
        }
    }

    return MMSYSERR_NOERROR;
}


MMRESULT
OpenWdmSoundDevice(
    IN  struct _SOUND_DEVICE* SoundDevice,  /* NOT USED */
    OUT PVOID* Handle)
{
    /* Only open this if it's not already open */
    if ( KernelHandle == INVALID_HANDLE_VALUE )
    {
        SND_TRACE(L"Opening wdmaud device\n");
        KernelHandle = CreateFile(KERNEL_DEVICE_NAME,
                                  GENERIC_READ | GENERIC_WRITE,
                                  0,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_FLAG_OVERLAPPED,
                                  NULL);
    }

    if ( KernelHandle == INVALID_HANDLE_VALUE )
        return MMSYSERR_ERROR;

    SND_ASSERT( Handle );

    *Handle = KernelHandle;
    ++ OpenCount;

    return MMSYSERR_NOERROR;
}

MMRESULT
CloseWdmSoundDevice(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance, /* NOT USED */
    IN  PVOID Handle)
{
    if ( OpenCount == 0 )
    {
        return MMSYSERR_NOERROR;
    }

    SND_ASSERT( KernelHandle != INVALID_HANDLE_VALUE );

    -- OpenCount;

    if ( OpenCount < 1 )
    {
        CloseHandle(KernelHandle);
        KernelHandle = INVALID_HANDLE_VALUE;
    }

    return MMSYSERR_NOERROR;
}


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
SetWdmWaveDeviceFormat(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;
    PVOID Identifier;
    WDMAUD_DEVICE_INFO DeviceInfo;

    Result = GetSoundDeviceFromInstance(Instance, &SoundDevice);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    Result = GetSoundDeviceIdentifier(SoundDevice, &Identifier);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    ZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));
    DeviceInfo.u.WaveFormatEx.cbSize = WaveFormat->cbSize;
    DeviceInfo.u.WaveFormatEx.wFormatTag = WaveFormat->wFormatTag;
    DeviceInfo.u.WaveFormatEx.nChannels = WaveFormat->nChannels;
    DeviceInfo.u.WaveFormatEx.nSamplesPerSec = WaveFormat->nSamplesPerSec;
    DeviceInfo.u.WaveFormatEx.nBlockAlign = WaveFormat->nBlockAlign;
    DeviceInfo.u.WaveFormatEx.nAvgBytesPerSec = WaveFormat->nAvgBytesPerSec;
    DeviceInfo.u.WaveFormatEx.wBitsPerSample = WaveFormat->wBitsPerSample;

    Result = SyncOverlappedDeviceIoControl(KernelHandle,
                                           IOCTL_OPEN_WDMAUD,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           NULL);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    return MMSYSERR_NOERROR;
}


MMRESULT
PopulateWdmDeviceList(
    HANDLE Handle,
    MMDEVICE_TYPE DeviceType)
{
    MMRESULT Result;
    DWORD DeviceCount = 0;
    PSOUND_DEVICE SoundDevice = NULL;
    MMFUNCTION_TABLE FuncTable;
    DWORD i;

    VALIDATE_MMSYS_PARAMETER( Handle != INVALID_HANDLE_VALUE );
    VALIDATE_MMSYS_PARAMETER( IS_VALID_SOUND_DEVICE_TYPE(DeviceType) );

    Result = GetNumWdmDevs(Handle, DeviceType, &DeviceCount);

    if ( ! MMSUCCESS(Result) )
    {
        SND_ERR(L"Error %d while obtaining number of devices\n", Result);
        return TranslateInternalMmResult(Result);
    }

    SND_TRACE(L"%d devices of type %d found\n", DeviceCount, DeviceType);


    for ( i = 0; i < DeviceCount; ++ i )
    {
        Result = ListSoundDevice(DeviceType, (PVOID) i, &SoundDevice);

        if ( ! MMSUCCESS(Result) )
        {
            SND_ERR(L"Failed to list sound device - error %d\n", Result);
            return TranslateInternalMmResult(Result);
        }

        /* Set up our function table */
        ZeroMemory(&FuncTable, sizeof(MMFUNCTION_TABLE));
        FuncTable.GetCapabilities = GetWdmDeviceCapabilities;
        FuncTable.QueryWaveFormatSupport = QueryWdmWaveDeviceFormatSupport;
        FuncTable.SetWaveFormat = SetWdmWaveDeviceFormat;
        FuncTable.Open = OpenWdmSoundDevice;
        FuncTable.Close = CloseWdmSoundDevice;
        //FuncTable.CommitWaveBuffer = WriteFileEx_Committer;

        SetSoundDeviceFunctionTable(SoundDevice, &FuncTable);
    }

    return MMSYSERR_NOERROR;
}



APIENTRY LONG
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
            HANDLE Handle;
            SND_TRACE(L"DRV_LOAD\n");

            Result = InitEntrypointMutexes();

            if ( ! MMSUCCESS(Result) )
                return 0L;

            OpenWdmSoundDevice(NULL, &Handle);

            if ( Handle == INVALID_HANDLE_VALUE )
            {
                SND_ERR(L"Failed to open %s\n", KERNEL_DEVICE_NAME);
                CleanupEntrypointMutexes();

                //UnlistAllSoundDevices();

                return 0L;
            }

            /* Populate the device lists */
            SND_TRACE(L"Populating device lists\n");
            PopulateWdmDeviceList(KernelHandle, WAVE_OUT_DEVICE_TYPE);
            PopulateWdmDeviceList(KernelHandle, WAVE_IN_DEVICE_TYPE);
            PopulateWdmDeviceList(KernelHandle, MIDI_OUT_DEVICE_TYPE);
            PopulateWdmDeviceList(KernelHandle, MIDI_IN_DEVICE_TYPE);
            PopulateWdmDeviceList(KernelHandle, AUX_DEVICE_TYPE);
            PopulateWdmDeviceList(KernelHandle, MIXER_DEVICE_TYPE);

            CloseWdmSoundDevice(NULL, Handle);

            SND_TRACE(L"Initialisation complete\n");

            return 1L;
        }

        case DRV_FREE :
        {
            SND_TRACE(L"DRV_FREE\n");

            if ( KernelHandle != INVALID_HANDLE_VALUE )
            {
                CloseHandle(KernelHandle);
                KernelHandle = INVALID_HANDLE_VALUE;
            }

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
