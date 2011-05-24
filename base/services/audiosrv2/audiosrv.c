#define _UNICODE
#define UNICODE
#define WIN32_NO_STATUS
#define _KSDDK_

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <math.h>
#include <setupapi.h>
#include <ndk/ntndk.h>
#include <ks.h>
#include <ksmedia.h>
#include <audiosrv.h>
#define _2pi                6.283185307179586476925286766559



#include <ks.h>


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

void fill(MixerEngine * mixer)
{
	DWORD Length = mixer->masterfreq * mixer->masterchannels * mixer->masterbitspersample / 8;
	UINT i = 0;
	mixer->masterchannels=2;
	mixer->masterfreq=48000;
	mixer->masterbitspersample=16;
	mixer->masterchannelmask = KSAUDIO_SPEAKER_STEREO;
	mixer->masterdoublebuf[0] = (PSHORT)HeapAlloc(GetProcessHeap(), 0, Length);
    while (i < Length / 2)
    {
        mixer->masterdoublebuf[0][i] = 0x7FFF * sin(0.5 * (i - 1) * 500 * _2pi / 48000);
        i++;
        mixer->masterdoublebuf[0][i] = 0x7FFF * sin((0.5 * i - 2) * 500 * _2pi / 48000);
        i++;
    }
}

void Playbuffer(MixerEngine * mixer)
{
	SP_DEVICE_INTERFACE_DATA InterfaceData;
    SP_DEVINFO_DATA DeviceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DetailData;
    HDEVINFO DeviceHandle;
    PKSDATAFORMAT DataFormat;
    PWAVEFORMATEXTENSIBLE WaveFormat;
    PKSPIN_CONNECT PinConnect;
    PKSSTREAM_HEADER Packet;
    PKSPROPERTY Property;
    KSSTATE State;
    DWORD Length;
    HANDLE FilterHandle; 
    HANDLE PinHandle;
    BOOL Result;
    NTSTATUS Status;


  //
    // Get a handle to KS Audio Interfaces
    //
    DeviceHandle = SetupDiGetClassDevs(&CategoryGuid,
                                       NULL,
                                       NULL,
                                       DIGCF_DEVICEINTERFACE |DIGCF_PRESENT);

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
                                                             HEAP_ZERO_MEMORY,
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
    //SetupDiDestroyDeviceInfoList(DeviceHandle);

    //
    // Allocate a KS Pin Connection Request Structure
    //
    Length = sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT_WAVEFORMATEX) + sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
    PinConnect = (PKSPIN_CONNECT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Length);
    DataFormat = (PKSDATAFORMAT)(PinConnect + 1);
    WaveFormat = (PWAVEFORMATEXTENSIBLE)(DataFormat + 1);

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
    printf("DataFormat %p %p\n", DataFormat,(PVOID)((((ULONG_PTR)DataFormat + 7)) & ~7));

    DataFormat->Flags = 0;
    DataFormat->Reserved = 0;
    DataFormat->MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    DataFormat->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    DataFormat->Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
    DataFormat->SampleSize = mixer->masterchannels * mixer->masterbitspersample / 8;
    DataFormat->FormatSize = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEXTENSIBLE);

    WaveFormat->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	WaveFormat->Format.nChannels = mixer->masterchannels;
    WaveFormat->Format.nSamplesPerSec = mixer->masterfreq;
    WaveFormat->Format.nBlockAlign = (mixer->masterchannels * mixer->masterbitspersample)/8;;
    WaveFormat->Format.nAvgBytesPerSec = WaveFormat->Format.nSamplesPerSec * WaveFormat->Format.nBlockAlign;
    WaveFormat->Format.wBitsPerSample = mixer->masterbitspersample;
    WaveFormat->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
	WaveFormat->dwChannelMask = mixer->masterchannelmask;
    WaveFormat->Samples.wValidBitsPerSample = mixer->masterbitspersample;
    WaveFormat->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    printf("Creating pin\n");

	//
    // Create the pin
    //
    Status = KsCreatePin(FilterHandle, PinConnect, GENERIC_READ|GENERIC_WRITE, &PinHandle);

    printf("PinHandle %p Status %lx\n", PinHandle, Status);


    Length = mixer->masterfreq * mixer->masterchannels * mixer->masterbitspersample / 8;
    //
    // Create and fill out the KS Stream Packet
    //
    Packet = (PKSSTREAM_HEADER)HeapAlloc(GetProcessHeap(),
                                         HEAP_ZERO_MEMORY,
                                         sizeof(KSSTREAM_HEADER));
	Packet->Data = mixer->masterdoublebuf[0];
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
DWORD WINAPI RunMixerThread(LPVOID param)
{
	MixerEngine * mixer = (MixerEngine *) param;
	SetEvent(mixer->EventPool[0]);
	while(1)
	{
	while(WaitForSingleObject(mixer->EventPool[0],100)!=0){if(mixer->dead)goto DEAD;}
	fill(mixer);
	SetEvent(mixer->EventPool[1]);
	}
DEAD:
	printf("Mixer Thread Ended\n");
	return 0;
}
DWORD WINAPI RunPlayerThread(LPVOID param)
{
	MixerEngine * mixer = (MixerEngine *) param;
	while(1)
	{
		while(WaitForSingleObject(mixer->EventPool[0],100)!=0){if(mixer->dead)goto DEAD;}
		Playbuffer(mixer);
		SetEvent(mixer->EventPool[0]);
	}
DEAD:
	printf("Player Thread Ended\n");
	return 0;
}
void SpawnMixerThread(MixerEngine * mixer)
{
	DWORD dwID;
	mixer->mixerthread=CreateThread(NULL,0,RunMixerThread,MIXER,0,&dwID);
}
void SpawnPlayerThread(MixerEngine * mixer)
{
	DWORD dwID;
	mixer->playerthread=CreateThread(NULL,0,RunPlayerThread,MIXER,0,&dwID);
}

int
__cdecl
wmain(int argc, char* argv[])
{
	SpawnMixerThread(MIXER);
	SpawnPlayerThread(MIXER);
	WaitForSingleObject(MIXER->mixerthread,INFINITE);
	WaitForSingleObject(MIXER->playerthread,INFINITE);
    return 0;
}
