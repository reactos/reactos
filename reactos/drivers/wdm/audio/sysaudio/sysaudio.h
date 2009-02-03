#ifndef SYSAUDIO_H__
#define SYSAUDIO_H__



typedef struct
{
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT NextDeviceObject;
    KSDEVICE_HEADER KsDeviceHeader;
    PVOID KsAudioNotificationEntry;
    PVOID EchoCancelNotificationEntry;
    KMUTEX Mutex;
}SYSAUDIODEVEXT;





#endif
