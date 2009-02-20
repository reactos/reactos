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
}KSAUDIO_DEVICE_ENTRY, *PKSAUDIO_DEVICE_ENTRY;


typedef struct
{
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT NextDeviceObject;
    KSDEVICE_HEADER KsDeviceHeader;
    ULONG NumberOfKsAudioDevices;

    LIST_ENTRY KsAudioDeviceList;
    PVOID KsAudioNotificationEntry;
    PVOID EchoCancelNotificationEntry;
    KMUTEX Mutex;
}SYSAUDIODEVEXT, *PSYSAUDIODEVEXT;

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

#endif
