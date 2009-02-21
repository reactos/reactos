#ifndef SYSAUDIO_H__
#define SYSAUDIO_H__

typedef struct
{
    ULONG NumDevices;
    PULONG Devices;

}SYSAUDIO_CLIENT, *PSYSAUDIO_CLIENT;


typedef struct
{
    LIST_ENTRY Entry;
    HANDLE Handle;
    PFILE_OBJECT FileObject;
    UNICODE_STRING DeviceName;
    ULONG NumberOfClients;

    ULONG NumberOfPins;
    HANDLE * Pins;

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
    PIRP Irp;
    PKSAUDIO_DEVICE_ENTRY Entry;
    KSPIN_CONNECT * PinConnect;

}PIN_WORKER_CONTEXT, *PPIN_WORKER_CONTEXT;

typedef struct
{
    HANDLE Handle;
    PFILE_OBJECT FileObject;
}DISPATCH_CONTEXT, *PDISPATCH_CONTEXT;

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
    IN PIRP Irp,
    IN HANDLE Handle,
    IN PFILE_OBJECT FileObject);

#endif
