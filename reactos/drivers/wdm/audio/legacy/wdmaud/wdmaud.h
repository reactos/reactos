#ifndef WDMAUD_H__
#define WDMAUD_H__

#include <ntddk.h>
#include <portcls.h>
#define YDEBUG
#include <debug.h>

typedef struct
{
    LIST_ENTRY Entry;
    HANDLE Handle;
    UNICODE_STRING SymbolicLink;
    PFILE_OBJECT FileObject;
}SYSAUDIO_ENTRY;


typedef struct
{
    KSDEVICE_HEADER DeviceHeader;
    PVOID SysAudioNotification;

    ULONG NumSysAudioDevices;
    LIST_ENTRY SysAudioDeviceList;

}WDMAUD_DEVICE_EXTENSION, *PWDMAUD_DEVICE_EXTENSION;











#endif
