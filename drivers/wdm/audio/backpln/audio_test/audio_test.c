#define _UNICODE
#define UNICODE
#define WIN32_NO_STATUS
#define _KSDDK_

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <setupapi.h>
#include <ndk/umtypes.h>
#include <ks.h>
#include <ksmedia.h>
#include "interface.h"

#define _2pi                6.283185307179586476925286766559

GUID CategoryGuid = {STATIC_KSCATEGORY_AUDIO};

const GUID KSPROPSETID_Pin                     = {0x8C134960L, 0x51AD, 0x11CF, {0x87, 0x8A, 0x94, 0xF8, 0x01, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Connection               = {0x1D58C920L, 0xAC9B, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Sysaudio                 = {0xCBE3FAA0L, 0xCC75, 0x11D0, {0xB4, 0x65, 0x00, 0x00, 0x1A, 0x18, 0x18, 0xE6}};
const GUID KSPROPSETID_General                  = {0x1464EDA5L, 0x6A8F, 0x11D1, {0x9A, 0xA7, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSINTERFACESETID_Standard            = {0x1A8766A0L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSMEDIUMSETID_Standard               = {0x4747B320L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSDATAFORMAT_TYPE_AUDIO              = {0x73647561L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_SUBTYPE_PCM             = {0x00000001L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_SPECIFIER_WAVEFORMATEX  = {0x05589f81L, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}};

VOID
TestKs()
{
    SP_DEVICE_INTERFACE_DATA InterfaceData;
    SP_DEVINFO_DATA DeviceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DetailData;
    HDEVINFO DeviceHandle;
    PKSDATAFORMAT_WAVEFORMATEX DataFormat;
    PKSPIN_CONNECT PinConnect;
    PKSSTREAM_HEADER Packet;
    PKSPROPERTY Property;
    KSSTATE State;
    DWORD Length;
    HANDLE FilterHandle;
    HANDLE PinHandle;
    PSHORT SoundBuffer;
    UINT i = 0;
    BOOL Result;
    NTSTATUS Status;

  //
    // Get a handle to KS Audio Interfaces
    //
    DeviceHandle = SetupDiGetClassDevs(&CategoryGuid,
                                       NULL,
                                       NULL,
                                       DIGCF_DEVICEINTERFACE); //DIGCF_PRESENT

   printf("DeviceHandle %p\n", DeviceHandle);

    //
    // Enumerate the first interface
    //
    InterfaceData.cbSize = sizeof(InterfaceData);
    InterfaceData.Reserved = 0;
    Result = SetupDiEnumDeviceInterfaces(DeviceHandle,
                                NULL,
                                &CategoryGuid,
                                1,
                                &InterfaceData);

   printf("SetupDiEnumDeviceInterfaces %u Error %ld\n", Result, GetLastError());

    //
    // Get the interface details (namely the device path)
    //
    Length = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + MAX_PATH * sizeof(WCHAR);
    DetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(),
                                                             0,
                                                             Length);
    DetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    DeviceData.cbSize = sizeof(DeviceData);
    DeviceData.Reserved = 0;
    Result = SetupDiGetDeviceInterfaceDetail(DeviceHandle,
                                    &InterfaceData,
                                    DetailData,
                                    Length,
                                    NULL,
                                    &DeviceData);

    wprintf(L"SetupDiGetDeviceInterfaceDetail %u Path DetailData %s\n", Result, (LPWSTR)&DetailData->DevicePath[0]);

    //
    // Open a handle to the device
    //
    FilterHandle = CreateFile(DetailData->DevicePath,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

    printf("Handle %p\n", FilterHandle);

    //
    // Close the interface handle and clean up
    //
    SetupDiDestroyDeviceInfoList(DeviceHandle);

    //
    // Allocate a KS Pin Connection Request Structure
    //
    Length = sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT_WAVEFORMATEX);
	printf("Length %ld KSPIN %Iu DATAFORMAT %Iu\n", Length, sizeof(KSPIN_CONNECT), sizeof(KSDATAFORMAT_WAVEFORMATEX));
    PinConnect = (PKSPIN_CONNECT)HeapAlloc(GetProcessHeap(), 0, Length);
    DataFormat = (PKSDATAFORMAT_WAVEFORMATEX)(PinConnect + 1);

    //
    // Setup the KS Pin Data
    //
    PinConnect->Interface.Set = KSINTERFACESETID_Standard;
    PinConnect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
    PinConnect->Interface.Flags = 0;
    PinConnect->Medium.Set = KSMEDIUMSETID_Standard;
    PinConnect->Medium.Id = KSMEDIUM_TYPE_ANYINSTANCE;
    PinConnect->Medium.Flags = 0;
    PinConnect->PinId = 0;
    PinConnect->PinToHandle = NULL;
    PinConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    PinConnect->Priority.PrioritySubClass = 1;

    //
    // Setup the KS Data Format Information
    //
    DataFormat->WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
    DataFormat->WaveFormatEx.nChannels = 2;
    DataFormat->WaveFormatEx.nSamplesPerSec = 48000;
    DataFormat->WaveFormatEx.nBlockAlign = 4;
    DataFormat->WaveFormatEx.nAvgBytesPerSec = 48000 * 4;
    DataFormat->WaveFormatEx.wBitsPerSample = 16;
    DataFormat->WaveFormatEx.cbSize = 0;
    DataFormat->DataFormat.FormatSize = sizeof(KSDATAFORMAT) +
                                        sizeof(WAVEFORMATEX);
    DataFormat->DataFormat.Flags = KSDATAFORMAT_ATTRIBUTES;
    DataFormat->DataFormat.Reserved = 0;
    DataFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    DataFormat->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    DataFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
    DataFormat->DataFormat.SampleSize = 4;

    //
    // Create the pin
    //
    Status = KsCreatePin(FilterHandle, PinConnect, GENERIC_WRITE, &PinHandle);

    printf("PinHandle %p Status %lx\n", PinHandle, Status);

    //
    // Allocate a buffer for 1 second
    //
    Length = 48000 * 4;
    SoundBuffer = (PSHORT)HeapAlloc(GetProcessHeap(), 0, Length);

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

    //
    // Create and fill out the KS Stream Packet
    //
    Packet = (PKSSTREAM_HEADER)HeapAlloc(GetProcessHeap(),
                                         HEAP_ZERO_MEMORY,
                                         sizeof(KSSTREAM_HEADER));
    Packet->Data = SoundBuffer;
    Packet->FrameExtent = Length;
    Packet->DataUsed = Length;
    Packet->Size = sizeof(KSSTREAM_HEADER);
    Packet->PresentationTime.Numerator = 1;
    Packet->PresentationTime.Denominator = 1;

    //
    // Setup a KS Property to change the state
    //
    Property = (PKSPROPERTY)HeapAlloc(GetProcessHeap(), 0, sizeof(KSPROPERTY));
    Property->Set = KSPROPSETID_Connection;
    Property->Id = KSPROPERTY_CONNECTION_STATE;
    Property->Flags = KSPROPERTY_TYPE_SET;

    //
    // Change the state to run
    //
    State = KSSTATE_RUN;
    DeviceIoControl(PinHandle,
                    IOCTL_KS_PROPERTY,
                    Property,
                    sizeof(KSPROPERTY),
                    &State,
                    sizeof(State),
                    &Length,
                    NULL);

    //
    // Play our 1-second buffer
    //
    DeviceIoControl(PinHandle,
                    IOCTL_KS_WRITE_STREAM,
                    NULL,
                    0,
                    Packet,
                    Packet->Size,
                    &Length,
                    NULL);

    //
    // Change the state to stop
    //
    State = KSSTATE_STOP;
    DeviceIoControl(PinHandle,
                    IOCTL_KS_PROPERTY,
                    Property,
                    sizeof(KSPROPERTY),
                    &State,
                    sizeof(State),
                    &Length,
                    NULL);

    CloseHandle(PinHandle);
    CloseHandle(FilterHandle);
}

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

    TestKs();
    return 0;

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

    Status = DeviceIoControl(hWdmAud, IOCTL_GETCAPABILITIES, (LPVOID)&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), (LPVOID)&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), &BytesReturned, &Overlapped);

    if (!Status)
    {
        if (WaitForSingleObject(&Overlapped.hEvent, 5000) != WAIT_OBJECT_0)
        {
           printf("Failed to get iocaps %lx\n", GetLastError());
        }
    }
    printf("WDMAUD: Capabilities NumChannels %x dwFormats %lx\n", DeviceInfo.u.WaveOutCaps.wChannels, DeviceInfo.u.WaveOutCaps.dwFormats);

    DeviceInfo.u.WaveFormatEx.cbSize = sizeof(WAVEFORMATEX);
    DeviceInfo.u.WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
    DeviceInfo.u.WaveFormatEx.nChannels = 2;
    DeviceInfo.u.WaveFormatEx.nSamplesPerSec = 48000;
    DeviceInfo.u.WaveFormatEx.nBlockAlign = 4;
    DeviceInfo.u.WaveFormatEx.nAvgBytesPerSec = 48000 * 4;
    DeviceInfo.u.WaveFormatEx.wBitsPerSample = 16;



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

     printf("WDMAUD: opened device\n");

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

    DeviceInfo.u.State = KSSTATE_RUN;
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
    DeviceInfo.Header.Data = (PUCHAR)SoundBuffer;
    DeviceInfo.Header.DataUsed = DeviceInfo.Header.FrameExtent = Length;
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

    DeviceInfo.u.State = KSSTATE_STOP;
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
    CloseHandle(&Overlapped.hEvent);
    CloseHandle(hWdmAud);
    printf("WDMAUD:  COMPLETE\n");
    return 0;
}
