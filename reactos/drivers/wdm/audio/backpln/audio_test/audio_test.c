#define _UNICODE
#define UNICODE
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <math.h>

#define _2pi                6.283185307179586476925286766559

#include <ks.h>
#include <ksmedia.h>
#include "interface.h"

int
__cdecl
main(int argc, char* argv[])
{
    ULONG Length;
    PSHORT SoundBuffer;
    ULONG i = 0;
    BOOL Status;
    OVERLAPPED Overlapped;
    DWORD BytesReturned;
    HANDLE hWdmAud;
    WDMAUD_DEVICE_INFO DeviceInfo;


    hWdmAud = CreateFileW(L"\\\\.\\wdmaud",
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          FILE_FLAG_OVERLAPPED,
                          NULL);
     if (!hWdmAud)
     {
         printf("Failed to open wdmaud with %lx\n", GetLastError());
         return -1;
     }

     printf("WDMAUD: opened\n");
     /* clear device info */
     RtlZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));

     ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
     Overlapped.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

     DeviceInfo.DeviceType = WAVE_OUT_DEVICE_TYPE;


     Status = DeviceIoControl(hWdmAud, IOCTL_GETNUMDEVS_TYPE, (LPVOID)&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), (LPVOID)&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), &BytesReturned, &Overlapped);

     if (!Status)
     {
         if (WaitForSingleObject(&Overlapped.hEvent, 5000) != WAIT_OBJECT_0)
         {
            printf("Failed to get num of wave out devices with %lx\n", GetLastError());
            CloseHandle(hWdmAud);
            return -1;
         }
     }

     printf("WDMAUD: Num Devices %lu\n", DeviceInfo.DeviceCount);
     if (!DeviceInfo.DeviceCount)
     {
        CloseHandle(hWdmAud);
        return 0;
    }

    DeviceInfo.u.WaveFormatEx.cbSize = sizeof(WAVEFORMATEX);
    DeviceInfo.u.WaveFormatEx.wFormatTag = 0x1; //WAVE_FORMAT_PCM;
    DeviceInfo.u.WaveFormatEx.nChannels = 2;
    DeviceInfo.u.WaveFormatEx.nSamplesPerSec = 48000;
    DeviceInfo.u.WaveFormatEx.nBlockAlign = 4;
    DeviceInfo.u.WaveFormatEx.nAvgBytesPerSec = 48000 * 4;
    DeviceInfo.u.WaveFormatEx.wBitsPerSample = 16;

    Status = DeviceIoControl(hWdmAud, IOCTL_GETCAPABILITIES, (LPVOID)&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), (LPVOID)&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), &BytesReturned, &Overlapped);

    if (!Status)
    {
        if (WaitForSingleObject(&Overlapped.hEvent, 5000) != WAIT_OBJECT_0)
        {
           printf("Failed to get iocaps %lx\n", GetLastError());
        }
    }


     Status = DeviceIoControl(hWdmAud, IOCTL_OPEN_WDMAUD, (LPVOID)&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), &DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), &BytesReturned, &Overlapped);
     if (!Status)
     {
         if (WaitForSingleObject(&Overlapped.hEvent, 5000) != WAIT_OBJECT_0)
         {
             printf("Failed to open device with %lx\n", GetLastError());
             CloseHandle(hWdmAud);
             return -1;
         }
     }




    //
    // Allocate a buffer for 1 second
    //
    Length = 48000 * 4;
    SoundBuffer = (PSHORT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Length);

    //
    // Fill the buffer with a 500 Hz sine tone
    //
    while (i < Length / 2)
    {
        //
        // Generate the wave for each channel:
        // Amplitude * sin( Sample * Frequency * 2PI / SamplesPerSecond )
        //
        SoundBuffer[i] = 0x7FFF * sin(0.5 * (i - 1) * 500 * _2pi / 48000);
        i++;
        SoundBuffer[i] = 0x7FFF * sin((0.5 * i - 2) * 500 * _2pi / 48000);
        i++;
    }

    DeviceInfo.State = KSSTATE_RUN;
    Status = DeviceIoControl(hWdmAud, IOCTL_SETDEVICE_STATE, (LPVOID)&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), &DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), &BytesReturned, &Overlapped);
    if (!Status)
    {
         if (WaitForSingleObject(&Overlapped.hEvent, 5000) != WAIT_OBJECT_0)
         {
             printf("Failed to set device into run state %lx\n", GetLastError());
             CloseHandle(hWdmAud);
             return -1;
         }
    }
    
    //
    // Play our 1-second buffer
    //
    DeviceInfo.Buffer = (PUCHAR)SoundBuffer;
    DeviceInfo.BufferSize = Length;
    Status = DeviceIoControl(hWdmAud, IOCTL_WRITEDATA, (LPVOID)&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), &DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), &BytesReturned, &Overlapped);
    if (!Status)
    {
         if (WaitForSingleObject(&Overlapped.hEvent, 5000) != WAIT_OBJECT_0)
         {
             printf("Failed to play buffer %lx\n", GetLastError());
             CloseHandle(hWdmAud);
             return -1;
         }
    }

    printf("WDMAUD:  Played buffer\n");

    DeviceInfo.State = KSSTATE_STOP;
    Status = DeviceIoControl(hWdmAud, IOCTL_SETDEVICE_STATE, (LPVOID)&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), &DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), &BytesReturned, &Overlapped);
    if (!Status)
    {
         if (WaitForSingleObject(&Overlapped.hEvent, 5000) != WAIT_OBJECT_0)
         {
             printf("Failed to set device into stop state %lx\n", GetLastError());
             CloseHandle(hWdmAud);
            return -1;
         }
    }
    printf("WDMAUD:  STOPPED\n");
    CloseHandle(hWdmAud);
    CloseHandle(&Overlapped.hEvent);
    printf("WDMAUD:  COMPLETE\n");
    return 0;
}
