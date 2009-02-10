#ifndef SYSAUDIO_H__
#define SYSAUDIO_H__

typedef struct
{
    LIST_ENTRY Entry;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    UNICODE_STRING DeviceName;

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
}SYSAUDIODEVEXT;

#endif
