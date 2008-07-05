/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Hax

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/

#include <windows.h>
#include <mmsystem.h>
#include <ntddsnd.h>
#include <debug.h>

#include <mmebuddy.h>


/*
    **** TESTING CODE ONLY ****
*/

#define IDS_WAVEOUT_PNAME   0x68



BOOLEAN TestCallback(
    UCHAR DeviceType,
    PWSTR DevicePath,
    HANDLE Handle)
{
    MessageBox(0, DevicePath, L"CALLBACK", MB_OK | MB_TASKMODAL);

    CreateSoundDevice(DeviceType, DevicePath);

    return TRUE;
}


APIENTRY VOID
Test()
{
    ULONG WaveInCount, WaveOutCount;
    ULONG MidiInCount, MidiOutCount;
    ULONG MixerCount, AuxCount;
    WCHAR Message[1024];

    DetectNt4SoundDevices(WAVE_IN_DEVICE_TYPE,
                          L"\\\\.\\SBWaveIn",
                          TestCallback);

    DetectNt4SoundDevices(WAVE_OUT_DEVICE_TYPE,
                          L"\\\\.\\SBWaveOut",
                          TestCallback);

    DetectNt4SoundDevices(MIDI_IN_DEVICE_TYPE,
                          L"\\\\.\\MidiIn",
                          TestCallback);

    DetectNt4SoundDevices(MIDI_OUT_DEVICE_TYPE,
                          L"\\\\.\\MidiOut",
                          TestCallback);

    DetectNt4SoundDevices(MIXER_DEVICE_TYPE,
                          L"\\\\.\\SBMixer",
                          TestCallback);

    DetectNt4SoundDevices(AUX_DEVICE_TYPE,
                          L"\\\\.\\SBAux",
                          TestCallback);

    WaveInCount = GetSoundDeviceCount(WAVE_IN_DEVICE_TYPE);
    WaveOutCount = GetSoundDeviceCount(WAVE_OUT_DEVICE_TYPE);
    MidiInCount = GetSoundDeviceCount(MIDI_IN_DEVICE_TYPE);
    MidiOutCount = GetSoundDeviceCount(MIDI_OUT_DEVICE_TYPE);
    MixerCount = GetSoundDeviceCount(MIXER_DEVICE_TYPE);
    AuxCount = GetSoundDeviceCount(AUX_DEVICE_TYPE);

    wsprintf(Message, L"Found devices:\n- %d wave inputs\n- %d wave outputs\n- %d midi inputs\n- %d midi outputs\n- %d mixers\n- %d aux devices",
        WaveInCount, WaveOutCount,
        MidiInCount, MidiOutCount,
        MixerCount, AuxCount);

    MessageBox(0, Message, L"Result", MB_OK | MB_TASKMODAL);
}

APIENTRY VOID
TestGetCaps()
{
    UNIVERSAL_CAPS Caps;
    WCHAR DevInfo[1024];
    PSOUND_DEVICE Device;
    MMRESULT Result;

    CreateSoundDevice(WAVE_OUT_DEVICE_TYPE, L"\\\\.\\SBWaveOut0");
    Result = GetSoundDevice(WAVE_OUT_DEVICE_TYPE, 0, &Device);

    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail 1", L"Fail", MB_OK | MB_TASKMODAL);
        return;
    }

    Result = GetSoundDeviceCapabilities(Device, &Caps);
    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail 2", L"Fail", MB_OK | MB_TASKMODAL);
        return;
    }

    wsprintf(DevInfo, L"Device name: %hS\nManufacturer ID: %d\nProduct ID: %d\nDriver version: %x\nChannels: %d", Caps.WaveOut.szPname, Caps.WaveOut.wMid, Caps.WaveOut.wPid, Caps.WaveOut.vDriverVersion, Caps.WaveOut.wChannels);

    MessageBox(0, DevInfo, L"Result", MB_OK | MB_TASKMODAL);

#if 0
    HANDLE Handle;
    MMRESULT Result;
    WAVEOUTCAPS Caps;
    DWORD BytesReturned = 0;
    WCHAR DevInfo[1024];

    Result = OpenKernelSoundDevice(
        L"\\\\.\\SBWaveOut0",
        GENERIC_READ,
        &Handle);

    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail open", L"Fail open", MB_OK | MB_TASKMODAL);
        return;
    }

    ZeroMemory(&Caps, sizeof(WAVEOUTCAPS));

    if ( !
    DeviceIoControl(Handle,
                    IOCTL_WAVE_GET_CAPABILITIES,
                    NULL,
                    0,
                    (LPVOID) &Caps,
                    sizeof(WAVEOUTCAPS),
                    &BytesReturned,
                    NULL) )
    {
        MessageBox(0, L"Fail", L"Fail", MB_OK | MB_TASKMODAL);
    }
    else
    {
        wsprintf(DevInfo, L"%02x %02x %02x %02x %02x %02x", Caps.szPname[0], Caps.szPname[1], Caps.szPname[2], Caps.szPname[3], Caps.szPname[4], Caps.szPname[5]);
/*
        wsprintf(DevInfo, L"Device name: %hS\nManufacturer ID: %d\nProduct ID: %d\nDriver version: %x\nChannels: %d", Caps.szPname, Caps.wMid, Caps.wPid, Caps.vDriverVersion, Caps.wChannels);
*/

        MessageBox(0, DevInfo, L"Result", MB_OK | MB_TASKMODAL);
    }

    CloseHandle(Handle);
#endif
}
 


APIENTRY VOID
TestDevEnum()
{
    EnumerateNt4ServiceSoundDevices(
        L"sndblst",
        WAVE_OUT_DEVICE_TYPE,
        TestCallback);
}


WINAPI VOID
TestThreading()
{
    MMRESULT Result;
    PSOUND_DEVICE Device;
    PSOUND_DEVICE_INSTANCE Instance;

    CreateSoundDevice(WAVE_OUT_DEVICE_TYPE, L"\\\\.\\SBWaveOut0");
    Result = GetSoundDevice(WAVE_OUT_DEVICE_TYPE, 0, &Device);
    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail 1", L"Fail 1", MB_OK | MB_TASKMODAL);
        return;
    }

    Result = CreateSoundDeviceInstance(Device, &Instance);
    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail 2", L"Fail 2", MB_OK | MB_TASKMODAL);
        return;
    }

    Result = StartSoundThread(Instance);
    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail 2", L"Fail 2", MB_OK | MB_TASKMODAL);
        return;
    }

    MessageBox(0, L"Click to kill thread", L"Bai", MB_OK | MB_TASKMODAL);

    StopSoundThread(Instance);
/*
    P

MMRESULT
CreateSoundThread(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    OUT PSOUND_THREAD* Thread);
*/
}


int APIENTRY wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine,
    int nCmdShow)
{
    TestThreading();
    MessageBox(0, L"Le end", L"Bai", MB_OK | MB_TASKMODAL);
    return 0;
}
