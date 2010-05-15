#pragma once

#include <ntifs.h>
#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#include <ksmedia.h>
#include <math.h>
#define NDEBUG
#include <debug.h>
#include <stdio.h>

typedef struct
{
    HANDLE PinHandle;                       // handle to audio irp pin
    ULONG References;                       // number of clients having a reference to this audio irp pin
}PIN_INFO;

typedef struct
{
    LIST_ENTRY Entry;                                  // device entry for KsAudioDeviceList
    UNICODE_STRING DeviceName;                         // symbolic link of audio device

    HANDLE Handle;                          // handle to audio device
    PFILE_OBJECT FileObject;                // file objecto to audio device

    //PIN_INFO * Pins;                        // array of PIN_INFO
}KSAUDIO_DEVICE_ENTRY, *PKSAUDIO_DEVICE_ENTRY;

typedef struct
{
    KSDEVICE_HEADER KsDeviceHeader;                     // ks streaming header - must always be first item in device extension
    PDEVICE_OBJECT PhysicalDeviceObject;                // pdo
    PDEVICE_OBJECT NextDeviceObject;                    // lower device object
    ULONG NumberOfKsAudioDevices;                       // number of audio devices

    LIST_ENTRY KsAudioDeviceList;                       // audio device list
    PVOID KsAudioNotificationEntry;                     // ks audio notification hook
    PVOID EchoCancelNotificationEntry;                  // ks echo cancel notification hook
    KSPIN_LOCK Lock;                                    // audio device list mutex

    PFILE_OBJECT KMixerFileObject;                      // mixer file object
    HANDLE KMixerHandle;                                // mixer file handle

}SYSAUDIODEVEXT, *PSYSAUDIODEVEXT;

// struct DISPATCH_CONTEXT
//
// This structure is used to dispatch read / write / device io requests
// It is stored in the file object FsContext2 member
// Note: FsContext member is reserved for ks object header

typedef struct
{
    KSOBJECT_HEADER ObjectHeader;                        // pin object header
    HANDLE Handle;                                       // audio irp pin handle
    ULONG PinId;                                         // pin id of device
    PKSAUDIO_DEVICE_ENTRY AudioEntry;                 // pointer to audio device entry

    HANDLE hMixerPin;                                    // handle to mixer pin
}DISPATCH_CONTEXT, *PDISPATCH_CONTEXT;

NTSTATUS
SysAudioAllocateDeviceHeader(
    IN SYSAUDIODEVEXT *DeviceExtension);

NTSTATUS
SysAudioRegisterDeviceInterfaces(
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
SysAudioRegisterNotifications(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
SysAudioHandleProperty(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS
SysAudioOpenKMixer(
    IN SYSAUDIODEVEXT *DeviceExtension);

NTSTATUS
OpenDevice(
    IN PUNICODE_STRING DeviceName,
    IN PHANDLE HandleOut,
    IN PFILE_OBJECT * FileObjectOut);

PKSAUDIO_DEVICE_ENTRY
GetListEntry(
    IN PLIST_ENTRY Head,
    IN ULONG Index);

NTSTATUS
NTAPI
DispatchCreateSysAudioPin(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

ULONG
GetDeviceCount(
    PSYSAUDIODEVEXT DeviceExtension,
    BOOL WaveIn);

NTSTATUS
GetPinInstanceCount(
    PKSAUDIO_DEVICE_ENTRY Entry,
    PKSPIN_CINSTANCES PinInstances,
    PKSPIN_CONNECT PinConnect);

NTSTATUS
ComputeCompatibleFormat(
    IN PKSAUDIO_DEVICE_ENTRY Entry,
    IN ULONG PinId,
    IN PKSDATAFORMAT_WAVEFORMATEX ClientFormat,
    OUT PKSDATAFORMAT_WAVEFORMATEX MixerFormat);
