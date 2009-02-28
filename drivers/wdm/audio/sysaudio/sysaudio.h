#ifndef SYSAUDIO_H__
#define SYSAUDIO_H__

typedef struct
{
    BOOL bHandle;
    ULONG PinId;
    HANDLE hPin;
}SYSAUDIO_PIN_HANDLE, *PSYSAUDIO_PIN_HANDLE;


typedef struct
{
    ULONG DeviceId;
    ULONG ClientHandlesCount;
    PSYSAUDIO_PIN_HANDLE ClientHandles;

}SYSAUDIO_CLIENT_HANDELS, *PSYSAUDIO_CLIENT_HANDELS;

typedef struct
{
    ULONG NumDevices;
    PSYSAUDIO_CLIENT_HANDELS Devs;

}SYSAUDIO_CLIENT, *PSYSAUDIO_CLIENT;

typedef struct
{
    ULONG MaxPinInstanceCount;
    HANDLE PinHandle;
    ULONG References;
}PIN_INFO;


typedef struct
{
    LIST_ENTRY Entry;
    HANDLE Handle;
    PFILE_OBJECT FileObject;
    UNICODE_STRING DeviceName;
    ULONG NumberOfClients;

    ULONG NumberOfPins;
    PIN_INFO * Pins;

}KSAUDIO_DEVICE_ENTRY, *PKSAUDIO_DEVICE_ENTRY;


typedef struct
{
    KSDEVICE_HEADER KsDeviceHeader;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT NextDeviceObject;
    ULONG NumberOfKsAudioDevices;

    LIST_ENTRY KsAudioDeviceList;
    PVOID KsAudioNotificationEntry;
    PVOID EchoCancelNotificationEntry;
    KMUTEX Mutex;
}SYSAUDIODEVEXT, *PSYSAUDIODEVEXT;

typedef struct
{
    HANDLE Handle;
    PFILE_OBJECT FileObject;
    ULONG PinId;
    PKSAUDIO_DEVICE_ENTRY AudioEntry;

}DISPATCH_CONTEXT, *PDISPATCH_CONTEXT;

typedef struct
{
    PIRP Irp;
    BOOL CreateRealPin;
    PKSAUDIO_DEVICE_ENTRY Entry;
    KSPIN_CONNECT * PinConnect;
    PDISPATCH_CONTEXT DispatchContext;
    PSYSAUDIO_CLIENT AudioClient;
}PIN_WORKER_CONTEXT, *PPIN_WORKER_CONTEXT;

NTSTATUS
SysAudioAllocateDeviceHeader(
    IN SYSAUDIODEVEXT *DeviceExtension);

NTSTATUS
SysAudioRegisterDeviceInterfaces(
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
SysAudioRegisterNotifications(
    IN  PDRIVER_OBJECT  DriverObject,
    SYSAUDIODEVEXT *DeviceExtension);

NTSTATUS
SysAudioHandleProperty(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

PKSAUDIO_DEVICE_ENTRY
GetListEntry(
    IN PLIST_ENTRY Head,
    IN ULONG Index);

NTSTATUS
CreateDispatcher(
    IN PIRP Irp);

#endif
