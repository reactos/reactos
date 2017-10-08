/*
 * PROJECT:     ReactOS Sound System
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/sndblst/sndblst.c
 *
 * PURPOSE:     Sound Blaster MME User-Mode Driver
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
 *
 * NOTES:       Currently very experimental and being used as a guinea-pig for
 *              changes to the MME-Buddy libraries.
 *              TODO: Adhere to maximum device name length!
*/

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <stdarg.h>

#include <windef.h>
#include <winuser.h>

//#include <ntddsnd.h>
#include <sndtypes.h>
#include <mmddk.h>
#include <mmebuddy.h>
#include <mment4.h>
#include <mmebuddy_debug.h>
//#include <debug.h>

/* TODO: Give individual device names if someone has > 1 card */
PWSTR SBWaveOutDeviceName = L"ROS Sound Blaster Wave Out";
PWSTR SBWaveInDeviceName  = L"ROS Sound Blaster Wave In";
PWSTR SBMidiOutDeviceName = L"ROS Sound Blaster Midi Out";
PWSTR SBMidiInDeviceName  = L"ROS Sound Blaster Midi In";
PWSTR SBAuxDeviceName     = L"ROS Sound Blaster Aux";
PWSTR SBMixerDeviceName   = L"ROS Sound Blaster Mixer";
/* TODO: Mixer etc */

MMRESULT
GetSoundBlasterDeviceCapabilities(
    IN  PSOUND_DEVICE SoundDevice,
    IN  DWORD DeviceId,
    OUT PVOID Capabilities,
    IN  DWORD CapabilitiesSize)
{
    MMRESULT Result;
    MMDEVICE_TYPE DeviceType;

    SND_ASSERT( SoundDevice );
    SND_ASSERT( Capabilities );

    SND_TRACE(L"Sndblst - GetSoundBlasterDeviceCapabilities\n");

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    /* Use the default method of obtaining device capabilities */
    Result = GetNt4SoundDeviceCapabilities(SoundDevice,
                                           Capabilities,
                                           CapabilitiesSize);

    if ( ! MMSUCCESS(Result) )
        return Result;

    /* Inject the appropriate device name */
    switch ( DeviceType )
    {
        case WAVE_OUT_DEVICE_TYPE :
        {
            LPWAVEOUTCAPS WaveOutCaps = (LPWAVEOUTCAPS) Capabilities;
            CopyWideString(WaveOutCaps->szPname, SBWaveOutDeviceName);
            break;
        }
        case WAVE_IN_DEVICE_TYPE :
        {
            LPWAVEINCAPS WaveInCaps = (LPWAVEINCAPS) Capabilities;
            CopyWideString(WaveInCaps->szPname, SBWaveInDeviceName);
            break;
        }
        case MIDI_OUT_DEVICE_TYPE :
        {
            LPMIDIOUTCAPS MidiOutCaps = (LPMIDIOUTCAPS) Capabilities;
            CopyWideString(MidiOutCaps->szPname, SBMidiOutDeviceName);
            break;
        }
        case MIDI_IN_DEVICE_TYPE :
        {
            LPMIDIINCAPS MidiInCaps = (LPMIDIINCAPS) Capabilities;
            CopyWideString(MidiInCaps->szPname, SBMidiInDeviceName);
            break;
        }
        case AUX_DEVICE_TYPE :
        {
            LPAUXCAPS AuxCaps = (LPAUXCAPS) Capabilities;
            CopyWideString(AuxCaps->szPname, SBAuxDeviceName);
            break;
        }
        case MIXER_DEVICE_TYPE :
        {
            LPMIXERCAPS MixerCaps = (LPMIXERCAPS) Capabilities;
            CopyWideString(MixerCaps->szPname, SBMixerDeviceName);
            break;
        }
    }

    return MMSYSERR_NOERROR;
}

BOOLEAN FoundDevice(
    UCHAR DeviceType,
    PWSTR DevicePath)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice = NULL;
    MMFUNCTION_TABLE FuncTable;
    PWSTR PathCopy;

    SND_TRACE(L"(Callback) Found device: %wS\n", DevicePath);

    PathCopy = AllocateWideString(wcslen(DevicePath));

    if ( ! PathCopy )
        return FALSE;

    CopyWideString(PathCopy, DevicePath);

    Result = ListSoundDevice(DeviceType, (PVOID) PathCopy, &SoundDevice);

    if ( ! MMSUCCESS(Result) )
        return FALSE;

    /* Set up our function table */
    ZeroMemory(&FuncTable, sizeof(MMFUNCTION_TABLE));
    FuncTable.GetCapabilities = GetSoundBlasterDeviceCapabilities;
    FuncTable.QueryWaveFormatSupport = QueryNt4WaveDeviceFormatSupport;
    FuncTable.SetWaveFormat = SetNt4WaveDeviceFormat;
    FuncTable.Open = OpenNt4SoundDevice;
    FuncTable.Close = CloseNt4SoundDevice;
    FuncTable.CommitWaveBuffer = WriteFileEx_Committer;
    //FuncTable.SubmitWaveHeaderToDevice = SubmitWaveHeaderToDevice;

    SetSoundDeviceFunctionTable(SoundDevice, &FuncTable);

    return TRUE;
}

LONG APIENTRY
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

            Result = EnumerateNt4ServiceSoundDevices(L"sndblst",
                                                     0,
                                                     FoundDevice);

            if ( ! MMSUCCESS(Result) )
            {
                CleanupEntrypointMutexes();

                UnlistAllSoundDevices();

                return 0L;
            }

/*
            PSOUND_DEVICE snd;
            GetSoundDevice(WAVE_OUT_DEVICE_TYPE, 0, &snd);
            GetSoundDevice(AUX_DEVICE_TYPE, 0, &snd);
            GetSoundDevice(AUX_DEVICE_TYPE, 1, &snd);
            GetSoundDevice(AUX_DEVICE_TYPE, 2, &snd);
*/

            SND_TRACE(L"Initialisation complete\n");

            return 1L;
        }

        case DRV_FREE :
        {
            SND_TRACE(L"DRV_FREE\n");

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
            SND_TRACE(L"DRV_QUERYCONFIGURE");
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
            SND_TRACE(L"DLL_PROCESS_ATTACH\n");
            break;
        case DLL_PROCESS_DETACH :
            SND_TRACE(L"DLL_PROCESS_DETACH\n");
            break;
        case DLL_THREAD_ATTACH :
            SND_TRACE(L"DLL_THREAD_ATTACH\n");
            break;
        case DLL_THREAD_DETACH :
            SND_TRACE(L"DLL_THREAD_DETACH\n");
            break;
    }

    return TRUE;
}
