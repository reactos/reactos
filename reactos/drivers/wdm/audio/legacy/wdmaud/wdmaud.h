#ifndef WDMAUD_H__
#define WDMAUD_H__

#define _NTDDK_
#include <ntddk.h>
#include <portcls.h>
#define YDEBUG
#include <debug.h>

typedef struct
{
    KSDEVICE_HEADER DeviceHeader;
    PVOID SysAudioNotification;

}WDMAUD_DEVICE_EXTENSION, *PWDMAUD_DEVICE_EXTENSION;











#endif
