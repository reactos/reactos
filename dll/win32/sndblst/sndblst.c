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

            /* TODO: Check return value */
            StartSoundThread();

            return 1L;

        case DRV_FREE :
            SOUND_DEBUG(L"DRV_FREE");

            StopSoundThread();

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


#include <stdio.h>

WORD Buffer[5347700 / 2];
WAVEHDR WaveHeaders[534];

int APIENTRY wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine,
    int nCmdShow)
{
//    WCHAR msg[1024];
//    WAVEOUTCAPS Caps;
    WAVEOPENDESC OpenDesc;
    WAVEFORMATEX Format;
    MMRESULT Result;
    PVOID InstanceData;
//    int i;

    FILE* f;

    f = fopen("27may2_a.wav", "rb");
    fseek(f, 48, SEEK_SET);
    fread(Buffer, 1, 5340000, f);
    fclose(f);

    /* DRV_LOAD */
    DriverProc(0, 0, DRV_LOAD, 0, 0);

    /* WODM_GETNUMDEVS */
    //SOUND_DEBUG_HEX(wodMessage(0, WODM_GETNUMDEVS, 0, 0, 0));

    //Result = wodMessage(0, WODM_GETDEVCAPS, 0,
                        //(DWORD) &Caps, sizeof(WAVEOUTCAPS));

    /* WODM_GETDEVCAPS */
    //wsprintf(msg, L"Device name: %ls\nManufacturer ID: %d\nProduct ID: %d\nDriver version: %x\nChannels: %d", Caps.szPname, Caps.wMid, Caps.wPid, Caps.vDriverVersion, Caps.wChannels);

    //MessageBox(0, msg, L"Device capabilities", MB_OK | MB_TASKMODAL);

    /* WODM_OPEN */
    Format.wFormatTag = WAVE_FORMAT_PCM;
    Format.nChannels = 2;
    Format.nSamplesPerSec = 22050;
    Format.wBitsPerSample = 16;
    Format.nBlockAlign = Format.nChannels * (Format.wBitsPerSample / 8);
    Format.nAvgBytesPerSec = Format.nSamplesPerSec * Format.nBlockAlign;
    Format.cbSize = 0;

    //SOUND_DEBUG(L"WODM_OPEN test 1 (query format support only)");
    //OpenDesc.lpFormat = &Format;
    //Result = wodMessage(0, WODM_OPEN, 0, (DWORD) &OpenDesc, WAVE_FORMAT_QUERY);
    //SOUND_DEBUG_HEX(Result);

    //SOUND_DEBUG(L"WODM_OPEN test 2");
    OpenDesc.lpFormat = &Format;
    Result = wodMessage(0, WODM_OPEN, (DWORD) &InstanceData, (DWORD) &OpenDesc, 0);
    SOUND_DEBUG_HEX(Result);

    SOUND_DEBUG(L"WODM_WRITE test");
    WaveHeaders[0].lpData = (PVOID) Buffer;
    WaveHeaders[0].dwBufferLength = 1000000;
    WaveHeaders[0].dwFlags = WHDR_PREPARED;

    WaveHeaders[1].lpData = (PVOID) ((PCHAR)Buffer + 1000000);
    WaveHeaders[1].dwBufferLength = 1000000;
    WaveHeaders[1].dwFlags = WHDR_PREPARED;

    WaveHeaders[2].lpData = (PVOID) ((PCHAR)Buffer + (1000000 *2));
    WaveHeaders[2].dwBufferLength = 1000000;
    WaveHeaders[2].dwFlags = WHDR_PREPARED;

    WaveHeaders[3].lpData = (PVOID) ((PCHAR)Buffer + (1000000 *3));
    WaveHeaders[3].dwBufferLength = 1000000;
    WaveHeaders[3].dwFlags = WHDR_PREPARED;

    WaveHeaders[4].lpData = (PVOID) ((PCHAR)Buffer + (1000000 *4));
    WaveHeaders[4].dwBufferLength = 1000000;
    WaveHeaders[4].dwFlags = WHDR_PREPARED;

//    WaveHeader2.lpData = (PVOID) Buffer2;
//    WaveHeader2.dwBufferLength = 10;
//    WaveHeader2.dwFlags = WHDR_PREPARED;

    Result = wodMessage(0, WODM_WRITE, (DWORD) InstanceData, (DWORD) &WaveHeaders[0], 0);
    Result = wodMessage(0, WODM_WRITE, (DWORD) InstanceData, (DWORD) &WaveHeaders[1], 0);
    Result = wodMessage(0, WODM_WRITE, (DWORD) InstanceData, (DWORD) &WaveHeaders[2], 0);
    Result = wodMessage(0, WODM_WRITE, (DWORD) InstanceData, (DWORD) &WaveHeaders[3], 0);
    Result = wodMessage(0, WODM_WRITE, (DWORD) InstanceData, (DWORD) &WaveHeaders[4], 0);

    //Result = wodMessage(0, WODM_WRITE, (DWORD) InstanceData, (DWORD) &WaveHeader2, 0);

//    SOUND_DEBUG_HEX(Result);

    SOUND_DEBUG(L"WODM_CLOSE test");
    Result = wodMessage(0, WODM_CLOSE, (DWORD) InstanceData, (DWORD) 0, 0);
    SOUND_DEBUG_HEX(Result);

    /* DRV_UNLOAD */
    DriverProc(0, 0, DRV_FREE, 0, 0);

    return 0;
}
