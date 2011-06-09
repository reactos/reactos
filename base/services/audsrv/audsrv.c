/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/audsrv.c
 * PURPOSE:          Audio Server
 * COPYRIGHT:        Copyright 2011 Neeraj Yadav

 */

#include "audsrv.h"

static VOID CALLBACK ServiceMain(DWORD,
                                 LPWSTR *);

static DWORD ServiceInit(VOID);

static BOOL WINAPI Close (DWORD dwCtrlType);

static WCHAR ServiceName[] = L"AudSrv";

static SERVICE_TABLE_ENTRYW ServiceTable[2] =
{
    { ServiceName, ServiceMain },
    { NULL, NULL }
};

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE ServiceStatusHandle;

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
const GUID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT      = {0x00000003L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_SPECIFIER_WAVEFORMATEX  = {0x05589f81L, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}};

/*Global Pointer to MixerEngine Structure*/
MixerEngine engine,*pengine;

static
VOID
UpdateServiceStatus(DWORD dwState)
{
    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwCurrentState = dwState;
    ServiceStatus.dwControlsAccepted = 0;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;

    if (dwState == SERVICE_START_PENDING ||
        dwState == SERVICE_STOP_PENDING ||
        dwState == SERVICE_PAUSE_PENDING ||
        dwState == SERVICE_CONTINUE_PENDING)
        ServiceStatus.dwWaitHint = 10000;
    else
        ServiceStatus.dwWaitHint = 0;

    SetServiceStatus(ServiceStatusHandle,
                     &ServiceStatus);
}

static DWORD WINAPI
ServiceControlHandler(DWORD dwControl,
                      DWORD dwEventType,
                      LPVOID lpEventData,
                      LPVOID lpContext)
{
    DPRINT("ServiceControlHandler() called\n");

    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
            DPRINT("  SERVICE_CONTROL_STOP received\n");
            Close(CTRL_CLOSE_EVENT);
            UpdateServiceStatus(SERVICE_STOPPED);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_PAUSE:
            DPRINT("  SERVICE_CONTROL_PAUSE received\n");
            UpdateServiceStatus(SERVICE_PAUSED);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_CONTINUE:
            DPRINT("  SERVICE_CONTROL_CONTINUE received\n");
            UpdateServiceStatus(SERVICE_RUNNING);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_INTERROGATE:
            DPRINT("  SERVICE_CONTROL_INTERROGATE received\n");
            SetServiceStatus(ServiceStatusHandle,
                             &ServiceStatus);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_SHUTDOWN:
            DPRINT("  SERVICE_CONTROL_SHUTDOWN received\n");
            Close(CTRL_CLOSE_EVENT);
            UpdateServiceStatus(SERVICE_STOPPED);
            return ERROR_SUCCESS;

        default :
            DPRINT1("  Control %lu received\n");
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

static
VOID
CALLBACK
ServiceMain(DWORD argc,
            LPWSTR *argv)
{
    DWORD dwError;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("ServiceMain() called\n");

    ServiceStatusHandle = RegisterServiceCtrlHandlerExW(ServiceName,
                                                        ServiceControlHandler,
                                                        NULL);
    if (!ServiceStatusHandle)
    {
        dwError = GetLastError();
        DPRINT1("RegisterServiceCtrlHandlerW() failed! (Error %lu)\n", dwError);
        return;
    }

    UpdateServiceStatus(SERVICE_START_PENDING);

    dwError = ServiceInit();
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("Service stopped (dwError: %lu\n", dwError);
        UpdateServiceStatus(SERVICE_START_PENDING);
    }
    else
    {
        DPRINT("Service started\n");
        UpdateServiceStatus(SERVICE_RUNNING);
    }

    DPRINT("ServiceMain() done\n");
}

/*This Function is called repeatedly by mixer thread.
  *This functions fetches FilteredBuffer from Every ClientStream Stucture
  *and mixes them in oder to feed player thread*/
void
MixAndFill(MixerEngine * mixer,
           int buffer)
{
    while(WaitForSingleObject(mixer->streampresent,
                              100) != 0)
    {
        if(mixer->dead)
            return;
    }

    /*Master Stream's Datatype is signed int,unsigned int or float when masterdatatype =0,1 and 2 respectively*/
    if(mixer->masterdatatype == 0)
    {
        switch(mixer->masterbitspersample)
        {
            case 8:
                MixS8(mixer,buffer);
                break;
            case 16:
                MixS16(mixer,buffer);
                break;
            case 32:
                MixS32(mixer,buffer);
                break;
            case 64:
                MixS64(mixer,buffer);
                break;
        }
    }
    else if (mixer->masterdatatype == 1)
    {
        switch(mixer->masterbitspersample)
        {
            case 8:
                MixU8(mixer,buffer);
                break;
            case 16:
                MixU16(mixer,buffer);
                break;
            case 32:
                MixU32(mixer,buffer);
                break;
            case 64:
                MixU64(mixer,buffer);
                break;
        }
    }
    else if(mixer->masterdatatype == 2)
    {
        switch(mixer->masterbitspersample)
        {
            case 32:
                MixFL32(mixer,buffer);
                break;
            case 64:
                MixFL64(mixer,buffer);
                break;
        }
    }
}

/*Frees the Buffer Created in mixer thread*/
void FreeBuffer()
{
    HeapFree(GetProcessHeap(),
             0,
            pengine->masterbuf[pengine->playcurrent]);

    pengine->masterbuf[pengine->playcurrent] = NULL;
}

/*Plays Master buffer pengine->masterbuf[pengine->playcurrent]*/
void PlayBuffer(MixerEngine * mixer,
                int buffer)
{
    SP_DEVICE_INTERFACE_DATA InterfaceData;
    SP_DEVINFO_DATA DeviceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DetailData;
    HDEVINFO DeviceHandle;
    PKSDATAFORMAT DataFormat;
    PWAVEFORMATEXTENSIBLE WaveFormat;
    PKSPIN_CONNECT PinConnect;
    KSSTATE State;
    DWORD Length;
    BOOL Result;
    NTSTATUS Status;

    if(mixer->masterbuf[buffer])
    {
    //
    // Get a handle to KS Audio Interfaces
    //
    DeviceHandle = SetupDiGetClassDevs(&CategoryGuid,
                                       NULL,
                                       NULL,
                                       DIGCF_DEVICEINTERFACE |DIGCF_PRESENT);


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


    //
    // Open a handle to the device
    //
    mixer->FilterHandle = CreateFile(DetailData->DevicePath,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);


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

    DataFormat->Flags = 0;
    DataFormat->Reserved = 0;
    DataFormat->MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    if(mixer->masterdatatype == 0 || mixer->masterdatatype == 1)
        DataFormat->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    else
        DataFormat->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
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
    if(mixer->masterdatatype == 0 || mixer->masterdatatype == 1)
        WaveFormat->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    else
        WaveFormat->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;


    //
    // Create the pin
    //
    Status = KsCreatePin(mixer->FilterHandle, PinConnect, GENERIC_READ|GENERIC_WRITE, &(mixer->PinHandle));

    Length = mixer->masterfreq * mixer->masterchannels * mixer->masterbitspersample / 8;
    //
    // Create and fill out the KS Stream Packet
    //
    mixer->Packet = (PKSSTREAM_HEADER)HeapAlloc(GetProcessHeap(),
                                         HEAP_ZERO_MEMORY,
                                         sizeof(KSSTREAM_HEADER));
    mixer->Packet->Data = mixer->masterbuf[buffer];
    mixer->Packet->FrameExtent = mixer->bytes_to_play;
    mixer->Packet->DataUsed = mixer->bytes_to_play;
    mixer->Packet->Size = sizeof(KSSTREAM_HEADER);
    mixer->Packet->PresentationTime.Numerator = 1;
    mixer->Packet->PresentationTime.Denominator = 1;

    //
    // Setup a KS Property to change the state
    //
    mixer->Property = (PKSPROPERTY)HeapAlloc(GetProcessHeap(), 0, sizeof(KSPROPERTY));
    mixer->Property->Set = KSPROPSETID_Connection;
    mixer->Property->Id = KSPROPERTY_CONNECTION_STATE;
    mixer->Property->Flags = KSPROPERTY_TYPE_SET;

    //
    // Change the state to run
    //
    State = KSSTATE_RUN;
    DeviceIoControl(mixer->PinHandle,
                    IOCTL_KS_PROPERTY,
                    mixer->Property,
                    sizeof(KSPROPERTY),
                    &State,
                    sizeof(State),
                    &Length,
                    NULL);

    DeviceIoControl(mixer->PinHandle,
                    IOCTL_KS_WRITE_STREAM,
                    NULL,
                    0,
                    mixer->Packet,
                    mixer->Packet->Size,
                    &Length,
                    NULL);

    State = KSSTATE_STOP;
    DeviceIoControl(mixer->PinHandle,
                    IOCTL_KS_PROPERTY,
                    mixer->Property,
                    sizeof(KSPROPERTY),
                    &State,
                    sizeof(State),
                    &Length,
                    NULL);

    CloseHandle(mixer->PinHandle);
    CloseHandle(mixer->FilterHandle);
}
}

/*Thread Routine For Mixer Thread*/
DWORD WINAPI RunMixerThread(LPVOID param)
{
    MixerEngine * mixer = (MixerEngine *) param;

    SetEvent(mixer->played);

    while(TRUE)
    {
        while(WaitForSingleObject(mixer->played,
                                  100)!=0)
        {
            if(mixer->dead)
                break;
        }

        if(mixer->dead)
            break;

        MixAndFill(mixer,
                   1-mixer->playcurrent);

        SetEvent(mixer->filled);
    }

    return 0;
}

/*Thread Routine For Player Thread*/
DWORD WINAPI RunPlayerThread(LPVOID param)
{
    MixerEngine * mixer = (MixerEngine *) param;

    while(1)
    {
        while(WaitForSingleObject(mixer->filled,
                                  100)!=0)
        {
            if(mixer->dead)
                break;
        }

        if(mixer->dead)
            break;

        SetEvent(mixer->played);
        PlayBuffer(mixer,
                   mixer->playcurrent);

        FreeBuffer();

        mixer->playcurrent=1-mixer->playcurrent;
    }

    return 0;
}

/*Create Mixer Thread*/
void SpawnMixerThread(MixerEngine * mixer)
{
    DWORD dwID;
    mixer->mixerthread=CreateThread(NULL,
                                    0,
                                    RunMixerThread,
                                    pengine,
                                    0,
                                    &dwID);
}

/*Create Player Thread*/
void SpawnPlayerThread(MixerEngine * mixer)
{
    DWORD dwID;
    mixer->playerthread=CreateThread(NULL,
                                     0,
                                     RunPlayerThread,
                                     pengine,
                                     0,
                                     &dwID);
}

/*Create RPC Thread*/
void SpawnRPCThread(MixerEngine * mixer)
{
    DWORD dwID;
    mixer->rpcthread=CreateThread(NULL,
                                  0,
                                  RunRPCThread,
                                  pengine,
                                  0,
                                  &dwID);
}

void ShutdownRPC(void)
{
    RPC_STATUS status;
 
    status = RpcMgmtStopServerListening(NULL);
 
    if (status) 
    {
       exit(status);
    }
 
    status = RpcServerUnregisterIf(NULL, NULL, FALSE);
 
    if (status) 
    {
       exit(status);
    }
}

/*This Functions Kills the application, in any condition*/
static BOOL WINAPI Close ( DWORD dwCtrlType )
{
    pengine->dead=1;

    WaitForSingleObject(pengine->mixerthread,
                        INFINITE);
    WaitForSingleObject(pengine->playerthread,
                        INFINITE);

    ShutdownRPC();

    WaitForSingleObject(pengine->rpcthread,INFINITE);

    CloseHandle(pengine->mixerthread);
    CloseHandle(pengine->playerthread);
    CloseHandle(pengine->rpcthread);

    return TRUE;
}

INT wmain(int argc, char *argv[])
{
    INT RetCode = 0;

    /*For Temporary Debugging purpose, If there is any argument it acts as simple command line app,otherwise it is a NTSERVICE*/
    if(argc==1)
    {
        StartServiceCtrlDispatcher(ServiceTable);
    }
    else
    {
        pengine=&engine;

        /*Later these will be loaded from a Conf file*/
        pengine->mastervolume=1000;
        pengine->mute=FALSE;

        pengine->dead=0;
        pengine->streamidpool=0;
        pengine->playcurrent=1;
        pengine->masterbuf[0] = NULL;
        pengine->masterbuf[1] = NULL;
        pengine->played=CreateEvent(NULL,
                                    FALSE,
                                    FALSE,
                                    NULL);

        pengine->filled=CreateEvent(NULL,
                                    FALSE,
                                    FALSE,
                                    NULL);

        pengine->streampresent=CreateEvent(NULL,
                                           TRUE,
                                           FALSE,
                                           NULL);

        SetConsoleCtrlHandler(Close,
                              TRUE);

        SpawnMixerThread(pengine);
        SpawnPlayerThread(pengine);
        SpawnRPCThread(pengine);

        WaitForSingleObject(pengine->mixerthread,
                            INFINITE);
        WaitForSingleObject(pengine->playerthread,
                            INFINITE);
        WaitForSingleObject(pengine->rpcthread,INFINITE);
    }

    CloseHandle(pengine->mixerthread);
    CloseHandle(pengine->playerthread);
    CloseHandle(pengine->rpcthread);

    return RetCode;
}

static DWORD
ServiceInit(VOID)
{
    pengine=&engine;

    /*Later these will be loaded from a Conf file*/
    pengine->mastervolume=1000;
    pengine->mute=FALSE;

    pengine->dead=0;
    pengine->streamidpool=0;
    pengine->playcurrent=1;
    pengine->masterbuf[0] = NULL;
    pengine->masterbuf[1] = NULL;
    pengine->played=CreateEvent(NULL,FALSE,FALSE,NULL);
    pengine->filled=CreateEvent(NULL,FALSE,FALSE,NULL);
    pengine->streampresent=CreateEvent(NULL,TRUE,FALSE,NULL);
    SetConsoleCtrlHandler(Close,TRUE);
    SpawnMixerThread(pengine);
    SpawnPlayerThread(pengine);
    SpawnRPCThread(pengine);

    return ERROR_SUCCESS;
}
