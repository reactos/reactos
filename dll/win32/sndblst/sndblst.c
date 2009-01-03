/*
    ReactOS Sound System
    Sound Blaster MME Driver

    Purpose:
        MME driver entry-point

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        6 July 2008 - Created

    TODO: Adhere to maximum device name length!
*/

#include <windows.h>
#include <ntddsnd.h>
#include <mmddk.h>
#include <mmebuddy.h>
#include <mment4.h>
//#include <debug.h>

PWSTR SBWaveOutDeviceName = L"ROS Sound Blaster Out";
PWSTR SBWaveInDeviceName  = L"ROS Sound Blaster In";
/* TODO: Mixer etc */

MMRESULT
GetSoundBlasterDeviceCapabilities(
    IN  PSOUND_DEVICE SoundDevice,
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

    if ( Result != MMSYSERR_NOERROR )
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

    if ( Result != MMSYSERR_NOERROR )
    {
        return TranslateInternalMmResult(Result);
        return FALSE;
    }

    /* Set up our function table */
    FuncTable.GetCapabilities = GetSoundBlasterDeviceCapabilities;
    FuncTable.QueryWaveFormatSupport = QueryNt4WaveDeviceFormatSupport;
    FuncTable.SetWaveFormat = SetNt4WaveDeviceFormat;
    FuncTable.Open = OpenNt4SoundDevice;
    FuncTable.Close = CloseNt4SoundDevice;

    SetSoundDeviceFunctionTable(SoundDevice, &FuncTable);

    return TRUE;
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
            SND_TRACE(L"DRV_LOAD\n");

            Result = InitEntrypointMutexes();

            if ( Result != MMSYSERR_NOERROR )
                return 0L;

            Result = EnumerateNt4ServiceSoundDevices(L"sndblst",
                                                     0,
                                                     FoundDevice);

            if ( Result != MMSYSERR_NOERROR )
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

#if 0
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
    PSOUND_DEVICE SoundDevice;
    MMFUNCTION_TABLE FuncTable;

    ZeroMemory(&FuncTable, sizeof(MMFUNCTION_TABLE));

    FuncTable.GetCapabilities = GetSoundBlasterDeviceCapabilities;

    /* Nothing particularly special required... */
    return ( ListSoundDevice(DeviceType, DevicePath, &SoundDevice)
             == MMSYSERR_NOERROR );
}

APIENTRY LONG
DriverProc(
    DWORD driver_id,
    HANDLE driver_handle,
    UINT message,
    LONG parameter1,
    LONG parameter2)
{
    MMRESULT Result;
/*
    WCHAR msg[1024];
    wsprintf(msg, L"DriverProc msg %d", message);
    MessageBox(0, msg, L"DriverProc", MB_OK | MB_TASKMODAL);
*/

    switch ( message )
    {
        case DRV_LOAD :
            TRACE_("DRV_LOAD");

            InitMmeBuddyLib();

            Result = EnumerateNt4ServiceSoundDevices(L"sndblst",
                                                     0,
                                                     FoundDevice);

            /* TODO: Check return value */
            //Result = StartSoundThread();
            //ASSERT(Result == MMSYSERR_NOERROR);

            return 1L;

        case DRV_FREE :
            TRACE_("DRV_FREE");

            //StopSoundThread();

            RemoveAllSoundDevices();

//            SOUND_DEBUG_HEX(GetMemoryAllocations());
            TRACE_("Leaving driver with %d memory allocations present\n", (int) GetMemoryAllocations());

            CleanupMmeBuddyLib();

            return 1L;

        default :
            return DefaultDriverProc(driver_id,
                                     driver_handle,
                                     message,
                                     parameter1,
                                     parameter2);
    }
}

#if 0
#include <stdio.h>

WORD Buffer[5347700 / 2];
WAVEHDR WaveHeaders[534];

VOID CALLBACK
callback(
    HWAVEOUT Handle,
    UINT Message,
    DWORD_PTR Instance,
    DWORD_PTR Parameter1,
    DWORD_PTR Parameter2)
{
    printf("Callback called! Handle %d, message %d, instance %d, parameters %d %d\n",
        (int) Handle, (int) Message, (int) Instance, (int) Parameter1,
        (int) Parameter2);
}

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
    Format.nSamplesPerSec = 44100;
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
    OpenDesc.dwCallback = (DWORD) &callback;
    OpenDesc.dwInstance = 0x69696969;
    Result = wodMessage(0, WODM_OPEN, (DWORD) &InstanceData, (DWORD) &OpenDesc, CALLBACK_FUNCTION);
    /*SOUND_DEBUG_HEX(Result);*/

    POPUP("Click for WODM_WRITE test");
    WaveHeaders[0].lpData = (PVOID) Buffer;
    WaveHeaders[0].dwBufferLength = 1000000;
    WaveHeaders[0].dwFlags = WHDR_PREPARED | WHDR_BEGINLOOP;
    WaveHeaders[0].dwLoops = 5;

    WaveHeaders[1].lpData = (PVOID) ((PCHAR)Buffer + 1000000);
    WaveHeaders[1].dwBufferLength = 1000000;
    WaveHeaders[1].dwFlags = WHDR_PREPARED | WHDR_ENDLOOP;

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

    POPUP("Click for WODM_BREAKLOOP");
    Result = wodMessage(0, WODM_BREAKLOOP, (DWORD) InstanceData, (DWORD) 0, 0);

    POPUP("Click for WODM_PAUSE");
    Result = wodMessage(0, WODM_PAUSE, (DWORD) InstanceData, (DWORD) 0, 0);

    POPUP("Click for WODM_RESTART");
    Result = wodMessage(0, WODM_RESTART, (DWORD) InstanceData, (DWORD) 0, 0);

    POPUP("Click for WODM_RESET");
    Result = wodMessage(0, WODM_RESET, (DWORD) InstanceData, (DWORD) 0, 0);

    POPUP("Click for WODM_CLOSE test");
    Result = wodMessage(0, WODM_CLOSE, (DWORD) InstanceData, (DWORD) 0, 0);
    /* SOUND_DEBUG_HEX(Result); */

    /* DRV_UNLOAD */
    DriverProc(0, 0, DRV_FREE, 0, 0);

    return 0;
}
#endif
#endif

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
