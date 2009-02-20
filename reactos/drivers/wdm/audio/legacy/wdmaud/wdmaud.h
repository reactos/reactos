#ifndef WDMAUD_H__
#define WDMAUD_H__

#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#define YDEBUG
#include <debug.h>
#include <ksmedia.h>


#ifndef MAXPNAMELEN
#define MAXPNAMELEN      32
#endif

#ifndef WAVEOUTCAPS

typedef struct
{
    USHORT      wMid;
    USHORT      wPid;
    ULONG vDriverVersion;
    WCHAR     szPname[MAXPNAMELEN];
    ULONG     dwFormats;
    USHORT      wChannels;
    USHORT      wReserved1;
    ULONG     dwSupport;
} WAVEOUTCAPS;

#endif

#ifndef AUXCAPS

typedef struct { 
    USHORT      wMid; 
    USHORT      wPid; 
    ULONG vDriverVersion; 
    WCHAR     szPname[MAXPNAMELEN]; 
    USHORT      wTechnology; 
    USHORT      wReserved1; 
    ULONG     dwSupport; 
} AUXCAPS;

#endif

#ifndef WAVEINCAPS

typedef struct 
{
    USHORT      wMid;
    USHORT      wPid;
    ULONG vDriverVersion;
    WCHAR     szPname[MAXPNAMELEN];
    ULONG     dwFormats;
    USHORT      wChannels;
    USHORT      wReserved1;
} WAVEINCAPS; 
#endif

#include "interface.h"

typedef struct
{
    HANDLE hProcess;
    HANDLE hSysAudio;
    PFILE_OBJECT FileObject;

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
