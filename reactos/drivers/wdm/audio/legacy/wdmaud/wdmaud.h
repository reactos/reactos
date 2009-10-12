#ifndef WDMAUD_H__
#define WDMAUD_H__

#include <pseh/pseh2.h>
#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#define YDEBUG
#include <debug.h>
#include <ksmedia.h>
#include <mmreg.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#include "interface.h"

typedef struct
{
    HANDLE Handle;
    SOUND_DEVICE_TYPE Type;
    ULONG FilterId;
    ULONG PinId;
}WDMAUD_HANDLE, *PWDMAUD_HANDLE;


typedef struct
{
    HANDLE hProcess;
    ULONG NumPins;
    WDMAUD_HANDLE * hPins;

}WDMAUD_CLIENT, *PWDMAUD_CLIENT;

typedef struct
{
    LIST_ENTRY Entry;
    ULONG dwControlID;
}MIXERCONTROL_DATA, *LPMIXERCONTROL_DATA;

typedef struct
{
    MIXERCONTROL_DATA Header;
    LONG SignedMinimum;
    LONG SignedMaximum;
    LONG SteppingDelta;
    ULONG InputSteppingDelta;
    ULONG ValuesCount;
    PLONG Values;
}MIXERVOLUME_DATA, *LPMIXERVOLUME_DATA;



typedef struct
{
    LIST_ENTRY Entry;
    ULONG PinId;
    ULONG DeviceIndex;
    MIXERLINEW Line;
    LPMIXERCONTROLW LineControls;
    PULONG          NodeIds;
    LIST_ENTRY LineControlsExtraData;
}MIXERLINE_EXT, *LPMIXERLINE_EXT;


typedef struct
{
    MIXERCAPSW    MixCaps;

    LIST_ENTRY    LineList;
    ULONG ControlId;
}MIXER_INFO, *LPMIXER_INFO;


typedef struct
{
    LIST_ENTRY Entry;
    ULONG FilterId;
    ULONG PinId;
    ULONG bInput;
    union
    {
        WAVEOUTCAPSW OutCaps;
        WAVEINCAPSW  InCaps;
    }u;
}WAVE_INFO, *LPWAVE_INFO;


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
    HANDLE hSysAudio;
    PFILE_OBJECT FileObject;

    ULONG MixerInfoCount;
    LPMIXER_INFO MixerInfo;

    ULONG WaveInDeviceCount;
    LIST_ENTRY WaveInList;

    ULONG WaveOutDeviceCount;
    LIST_ENTRY WaveOutList;


}WDMAUD_DEVICE_EXTENSION, *PWDMAUD_DEVICE_EXTENSION;

NTSTATUS
NTAPI
OpenWavePin(
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension,
    IN ULONG FilterId,
    IN ULONG PinId,
    IN LPWAVEFORMATEX WaveFormatEx,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE PinHandle);

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

NTSTATUS
NTAPI
WdmAudReadWrite(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
WdmAudWrite(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
WdmAudControlOpenMixer(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo);

NTSTATUS
WdmAudControlOpenWave(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo);


ULONG
GetNumOfMixerDevices(
    IN  PDEVICE_OBJECT DeviceObject);

NTSTATUS
SetIrpIoStatus(
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG Length);

NTSTATUS
WdmAudOpenSysAudioDevice(
    IN LPWSTR DeviceName,
    OUT PHANDLE Handle);

NTSTATUS
FindProductName(
    IN LPWSTR PnpName,
    IN ULONG ProductNameSize,
    OUT LPWSTR ProductName);

NTSTATUS
WdmAudMixerCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo,
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
WdmAudWaveCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo,
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
NTAPI
WdmAudFrameSize(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo);

NTSTATUS
NTAPI
WdmAudGetLineInfo(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo);

NTSTATUS
NTAPI
WdmAudGetLineControls(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo);

NTSTATUS
NTAPI
WdmAudSetControlDetails(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo);

NTSTATUS
NTAPI
WdmAudGetControlDetails(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo);

NTSTATUS
WdmAudMixerInitialize(
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
NTAPI
WdmAudWaveInitialize(
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
ClosePin(
    IN  PWDMAUD_CLIENT ClientInfo,
    IN  ULONG FilterId,
    IN  ULONG PinId,
    IN  SOUND_DEVICE_TYPE DeviceType);

NTSTATUS
InsertPinHandle(
    IN  PWDMAUD_CLIENT ClientInfo,
    IN  ULONG FilterId,
    IN  ULONG PinId,
    IN  SOUND_DEVICE_TYPE DeviceType,
    IN  HANDLE PinHandle,
    IN  ULONG FreeIndex);


#endif
