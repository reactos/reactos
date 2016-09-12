#ifndef _WDMAUD_PCH_
#define _WDMAUD_PCH_

#include <portcls.h>
#include <mmsystem.h>

#include "interface.h"

typedef struct
{
    PMDL Mdl;
    ULONG Length;
    ULONG Function;
    PFILE_OBJECT FileObject;
}WDMAUD_COMPLETION_CONTEXT, *PWDMAUD_COMPLETION_CONTEXT;


typedef struct
{
    HANDLE Handle;
    SOUND_DEVICE_TYPE Type;
    ULONG FilterId;
    ULONG PinId;
    PRKEVENT NotifyEvent;
}WDMAUD_HANDLE, *PWDMAUD_HANDLE;

typedef struct
{
    LIST_ENTRY Entry;
    HANDLE hProcess;
    ULONG NumPins;
    WDMAUD_HANDLE * hPins;

    LIST_ENTRY MixerEventList;
}WDMAUD_CLIENT, *PWDMAUD_CLIENT;

typedef struct
{
    LIST_ENTRY Entry;
    ULONG NotificationType;
    ULONG Value;
    HANDLE hMixer;
}EVENT_ENTRY, *PEVENT_ENTRY;

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
    LIST_ENTRY WdmAudClientList;

    ULONG SysAudioDeviceCount;
    PIO_WORKITEM WorkItem;
    KEVENT InitializationCompletionEvent;
    ULONG WorkItemActive;

    PDEVICE_OBJECT NextDeviceObject;
}WDMAUD_DEVICE_EXTENSION, *PWDMAUD_DEVICE_EXTENSION;

typedef struct
{
    PWDMAUD_CLIENT ClientInfo;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    SOUND_DEVICE_TYPE DeviceType;
}PIN_CREATE_CONTEXT, *PPIN_CREATE_CONTEXT;


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

NTSTATUS
WdmAudControlOpenMidi(
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
WdmAudMidiCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWDMAUD_DEVICE_INFO DeviceInfo,
    IN PWDMAUD_CLIENT ClientInfo,
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
WdmAudGetMixerEvent(
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

ULONG
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

NTSTATUS
GetSysAudioDevicePnpName(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  ULONG DeviceIndex,
    OUT LPWSTR * Device);

NTSTATUS
OpenSysAudioDeviceByIndex(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  ULONG DeviceIndex,
    IN  PHANDLE DeviceHandle,
    IN  PFILE_OBJECT * FileObject);

NTSTATUS
OpenDevice(
    IN LPWSTR Device,
    OUT PHANDLE DeviceHandle,
    OUT PFILE_OBJECT * FileObject);

ULONG
WdmAudGetMixerDeviceCount(VOID);

ULONG
WdmAudGetWaveInDeviceCount(VOID);

ULONG
WdmAudGetWaveOutDeviceCount(VOID);

ULONG
WdmAudGetMidiInDeviceCount(VOID);

ULONG
WdmAudGetMidiOutDeviceCount(VOID);

NTSTATUS
WdmAudGetPnpNameByIndexAndType(
    IN ULONG DeviceIndex, 
    IN SOUND_DEVICE_TYPE DeviceType, 
    OUT LPWSTR *Device);


/* sup.c */

ULONG
GetSysAudioDeviceCount(
    IN  PDEVICE_OBJECT DeviceObject);


PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes);

VOID
FreeItem(
    IN PVOID Item);

#endif /* _WDMAUD_PCH_ */
