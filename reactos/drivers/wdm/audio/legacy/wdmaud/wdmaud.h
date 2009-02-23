#ifndef WDMAUD_H__
#define WDMAUD_H__

#include <pseh/pseh2.h>
#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#define YDEBUG
#include <debug.h>
#include <ksmedia.h>
#include <mmsystem.h>	

#include "interface.h"

typedef struct
{
    HANDLE hProcess;
    HANDLE hSysAudio;
    PFILE_OBJECT FileObject;
    ULONG NumPins;
    HANDLE * hPins;

}WDMAUD_CLIENT, *PWDMAUD_CLIENT;

typedef struct
{
    LIST_ENTRY Entry;
    UNICODE_STRING SymbolicLink;
}SYSAUDIO_ENTRY, *PSYSAUDIO_ENTRY;

typedef struct
{
    KSDEVICE_HEADER DeviceHeader;
    PVOID SysAudioNotification;

    BOOL DeviceInterfaceSupport;

    KSPIN_LOCK Lock;
    ULONG NumSysAudioDevices;
    LIST_ENTRY SysAudioDeviceList;

}WDMAUD_DEVICE_EXTENSION, *PWDMAUD_DEVICE_EXTENSION;

typedef struct
{
    PIRP Irp;
    IO_STATUS_BLOCK StatusBlock;
    ULONG Length;
}WRITE_CONTEXT, *PWRITE_CONTEXT;


NTSTATUS
WdmAudRegisterDeviceInterface(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
WdmAudOpenSysAudioDevices(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
WdmAudOpenSysaudio(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWDMAUD_CLIENT *pClient);

NTSTATUS
NTAPI
WdmAudDeviceControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

#endif
