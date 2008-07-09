/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Hax

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
        6 July 2008 - Moved from mmebuddy to mmdrv
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddsnd.h>
#include <debug.h>

#include <ntddk.h>
#include <mmebuddy.h>



/*
    **** TESTING CODE ONLY ****
*/

PSOUND_DEVICE Device;
PSOUND_DEVICE_INSTANCE Instance;
WAVEHDR waveheader;
WORD JunkBuffer[65536];


#define IDS_WAVEOUT_PNAME   0x68



BOOLEAN TestCallback(
    UCHAR DeviceType,
    PWSTR DevicePath,
    HANDLE Handle)
{
/*    MessageBox(0, DevicePath, L"CALLBACK", MB_OK | MB_TASKMODAL);*/

    AddSoundDevice(DeviceType, DevicePath, NULL);

    return TRUE;
}


VOID
TestDeviceDetection()
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

    AddSoundDevice(WAVE_OUT_DEVICE_TYPE, L"\\\\.\\SBWaveOut0", NULL);
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

    MessageBox(0, DevInfo, L"Device caps", MB_OK | MB_TASKMODAL);
}


VOID
TestFormatQuery()
{
    WCHAR msg[1024];
    PSOUND_DEVICE Device;
    MMRESULT Result;
    WAVEFORMATEX fmt;

    AddSoundDevice(WAVE_OUT_DEVICE_TYPE, L"\\\\.\\SBWaveOut0", NULL);
    Result = GetSoundDevice(WAVE_OUT_DEVICE_TYPE, 0, &Device);

    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail 1", L"Fail", MB_OK | MB_TASKMODAL);
        return;
    }

    /* Request a valid format */
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = 1;
    fmt.nSamplesPerSec = 22050;
    fmt.wBitsPerSample = 16;
    fmt.nBlockAlign = fmt.nChannels * (fmt.wBitsPerSample / 8);
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    Result = QueryWaveDeviceFormatSupport(Device, &fmt, sizeof(WAVEFORMATEX));

    wsprintf(msg, L"Format support query result: %d", Result);
    MessageBox(0, msg, L"Result", MB_OK | MB_TASKMODAL);

    /* Send it some garbage */
    fmt.nChannels = 6969;

    Result = QueryWaveDeviceFormatSupport(Device, &fmt, sizeof(WAVEFORMATEX));

    wsprintf(msg, L"Format support query result: %d", Result);
    MessageBox(0, msg, L"Result", MB_OK | MB_TASKMODAL);
}


VOID CALLBACK
OverlappedCallback(
    IN  DWORD dwErrorCode,
    IN  DWORD dwNumberOfBytesTransferred,
    IN  LPOVERLAPPED lpOverlapped)
{
    MessageBox(0, L"Job done!", L"File IO Callback", MB_OK | MB_TASKMODAL);
}


APIENTRY VOID
TestDevEnum()
{
    EnumerateNt4ServiceSoundDevices(
        L"sndblst",
        WAVE_OUT_DEVICE_TYPE,
        TestCallback);
}


MMRESULT
TestThreadCallback(
    IN  struct _SOUND_DEVICE_INSTANCE* Instance,
    IN  DWORD RequestId,
    IN  PVOID Data)
{
    MessageBox(0, L"Thread Request Callback", L"Woot", MB_OK | MB_TASKMODAL);

    return MMSYSERR_NOERROR;
}


VOID
wodTest()
{
    //MMRESULT Result;
    DWORD NumWaveOuts;
    WAVEOUTCAPS Caps;
    MMRESULT Result;
    WCHAR String[1024];

    /* Report the number of wave output devices */
    NumWaveOuts = wodMessage(0, WODM_GETNUMDEVS, 0, 0, 0);
    SOUND_DEBUG_HEX(NumWaveOuts);

    if ( NumWaveOuts < 1 )
    {
        SOUND_DEBUG(L"Nothing to do as no waveout devices!");
        return;
    }

    Result = wodMessage(0, WODM_GETDEVCAPS, 0,
                        (DWORD) &Caps, sizeof(WAVEOUTCAPS));

    wsprintf(String, L"Device name: %hS\nManufacturer ID: %d\nProduct ID: %d\nDriver version: %x\nChannels: %d", Caps.szPname, Caps.wMid, Caps.wPid, Caps.vDriverVersion, Caps.wChannels);

    MessageBox(0, String, L"Device caps", MB_OK | MB_TASKMODAL);
}


int APIENTRY wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine,
    int nCmdShow)
{
    MessageBox(0, L"Le end", L"Bai", MB_OK | MB_TASKMODAL);
    return 0;
}
