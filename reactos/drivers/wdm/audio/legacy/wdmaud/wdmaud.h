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
