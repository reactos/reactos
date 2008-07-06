/*
    ReactOS Sound System
    Sound Blaster MME Driver

    Purpose:
        MME driver entry-point

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        6 July 2008 - Created
*/

#include <windows.h>
#include <ntddsnd.h>
#include <mmddk.h>
#include <mmebuddy.h>
#include <debug.h>


PWSTR SBWaveOutDeviceName = L"Sound Blaster Playback";
PWSTR SBWaveInDeviceName  = L"Sound Blaster Recording";
/* TODO: Mixer etc */


MMRESULT
GetSoundBlasterDeviceCapabilities(
    IN  PSOUND_DEVICE Device,
    OUT PUNIVERSAL_CAPS Capabilities)
{
    MMRESULT Result;

    if ( ! Device )
        return MMSYSERR_INVALPARAM;

    if ( ! Capabilities )
        return MMSYSERR_INVALPARAM;

    Result = DefaultGetSoundDeviceCapabilities(Device, Capabilities);
    if ( Result != MMSYSERR_NOERROR )
        return Result;

    switch ( Device->DeviceType )
    {
        case WAVE_OUT_DEVICE_TYPE :
            CopyWideString(Capabilities->WaveOut.szPname,
                           SBWaveOutDeviceName);
            break;

        case WAVE_IN_DEVICE_TYPE :
            CopyWideString(Capabilities->WaveOut.szPname,
                           SBWaveInDeviceName);
            break;

        /* ... TODO ... */

        //default :
            /* Do nothing special */
    }

    return MMSYSERR_NOERROR;
}


BOOLEAN FoundDevice(
    UCHAR DeviceType,
    PWSTR DevicePath,
    HANDLE Handle)
{
    MMFUNCTION_TABLE FuncTable;

    ZeroMemory(&FuncTable, sizeof(MMFUNCTION_TABLE));

    FuncTable.GetCapabilities = GetSoundBlasterDeviceCapabilities;

    /* Nothing particularly special required... */
    return ( AddSoundDevice(DeviceType, DevicePath, &FuncTable) == MMSYSERR_NOERROR );
}


APIENTRY LONG
DriverProc(
    DWORD driver_id,
    HANDLE driver_handle,
    UINT message,
    LONG parameter1,
    LONG parameter2)
{
    switch ( message )
    {
        case DRV_LOAD :
            SOUND_DEBUG(L"DRV_LOAD");

            EnumerateNt4ServiceSoundDevices(L"sndblst",
                                            0,
                                            FoundDevice);

            return 1L;

        case DRV_FREE :
            SOUND_DEBUG(L"DRV_FREE");

            RemoveAllSoundDevices();

            SOUND_DEBUG_HEX(GetMemoryAllocations());

            return 1L;

        default :
            return DefaultDriverProc(driver_id,
                                     driver_handle,
                                     message,
                                     parameter1,
                                     parameter2);
    }
}


WORD Buffer[65536];
WAVEHDR WaveHeader;

int APIENTRY wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine,
    int nCmdShow)
{
    WCHAR msg[1024];
    WAVEOUTCAPS Caps;
    WAVEOPENDESC OpenDesc;
    WAVEFORMATEX Format;
    MMRESULT Result;
    PVOID InstanceData;

    /* DRV_LOAD */
    DriverProc(0, 0, DRV_LOAD, 0, 0);

    /* WODM_GETNUMDEVS */
    SOUND_DEBUG_HEX(wodMessage(0, WODM_GETNUMDEVS, 0, 0, 0));

    Result = wodMessage(0, WODM_GETDEVCAPS, 0,
                        (DWORD) &Caps, sizeof(WAVEOUTCAPS));

    /* WODM_GETDEVCAPS */
    wsprintf(msg, L"Device name: %ls\nManufacturer ID: %d\nProduct ID: %d\nDriver version: %x\nChannels: %d", Caps.szPname, Caps.wMid, Caps.wPid, Caps.vDriverVersion, Caps.wChannels);

    MessageBox(0, msg, L"Device capabilities", MB_OK | MB_TASKMODAL);

    /* WODM_OPEN */
    Format.wFormatTag = WAVE_FORMAT_PCM;
    Format.nChannels = 1;
    Format.nSamplesPerSec = 22050;
    Format.wBitsPerSample = 16;
    Format.nBlockAlign = Format.nChannels * (Format.wBitsPerSample / 8);
    Format.nAvgBytesPerSec = Format.nSamplesPerSec * Format.nBlockAlign;
    Format.cbSize = 0;

    SOUND_DEBUG(L"WODM_OPEN test 1 (query format support only)");
    OpenDesc.lpFormat = &Format;
    Result = wodMessage(0, WODM_OPEN, 0, (DWORD) &OpenDesc, WAVE_FORMAT_QUERY);
    SOUND_DEBUG_HEX(Result);

    SOUND_DEBUG(L"WODM_OPEN test 2");
    OpenDesc.lpFormat = &Format;
    Result = wodMessage(0, WODM_OPEN, (DWORD) &InstanceData, (DWORD) &OpenDesc, 0);
    SOUND_DEBUG_HEX(Result);

    SOUND_DEBUG(L"WODM_WRITE test");
    WaveHeader.lpData = (PVOID) Buffer;
    WaveHeader.dwBufferLength = 65536;
    WaveHeader.dwFlags = WHDR_PREPARED;

    Result = wodMessage(0, WODM_WRITE, (DWORD) InstanceData, (DWORD) &WaveHeader, 0);
    SOUND_DEBUG_HEX(Result);

    SOUND_DEBUG(L"WODM_CLOSE test");
    Result = wodMessage(0, WODM_CLOSE, (DWORD) InstanceData, (DWORD) 0, 0);
    SOUND_DEBUG_HEX(Result);

    /* DRV_UNLOAD */
    DriverProc(0, 0, DRV_FREE, 0, 0);

    return 0;
}
