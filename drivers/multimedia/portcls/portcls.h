/*

    Move to w32api when it is ready.

 */
#ifndef _PORTCLS_H
#define _PORTCLS_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* This header is total bull**** */

#include <ntddk.h>

#define PORTCLASSAPI extern

PORTCLASSAPI NTSTATUS STDCALL
PcAddAdapterDevice(
 ULONG DriverObject,
 ULONG PhysicalDeviceObject,
 ULONG StartDevice,
 ULONG MaxObjects,
 ULONG DeviceExtensionSize
);

PORTCLASSAPI NTSTATUS STDCALL
PcAddContentHandlers(
 ULONG  ContentId,
 ULONG  paHandlers,
 ULONG  NumHandlers
);

PORTCLASSAPI NTSTATUS STDCALL
PcCompleteIrp(
 ULONG  DeviceObject,
 ULONG  Irp,
 ULONG  Status
);

PORTCLASSAPI NTSTATUS STDCALL
PcCompletePendingPropertyRequest(
 ULONG PropertyRequest,
 ULONG NtStatus
);

PORTCLASSAPI NTSTATUS STDCALL 
PcCreateContentMixed(
 ULONG paContentId,
 ULONG cContentId,
 ULONG pMixedContentId
);

PORTCLASSAPI NTSTATUS STDCALL
PcDestroyContent(
 ULONG ContentId
);

PORTCLASSAPI NTSTATUS STDCALL
PcDispatchIrp(
 ULONG DeviceObject,
 ULONG Irp
);

PORTCLASSAPI NTSTATUS STDCALL
PcForwardContentToDeviceObject(
 ULONG ContentId,
 ULONG Reserved,
 ULONG DrmForward
);

PORTCLASSAPI NTSTATUS STDCALL 
PcForwardContentToFileObject(
 ULONG ContentId,
 ULONG FileObject
);

PORTCLASSAPI NTSTATUS STDCALL 
PcForwardContentToInterface(
 ULONG ContentId,
 ULONG Unknown,
 ULONG NumMethods
);

PORTCLASSAPI NTSTATUS STDCALL
PcForwardIrpSynchronous(
 ULONG DeviceObject,
 ULONG Irp 
);

PORTCLASSAPI NTSTATUS STDCALL 
PcGetContentRights(
 ULONG ContentId,
 ULONG DrmRights
); 

PORTCLASSAPI NTSTATUS STDCALL
PcGetDeviceProperty(
 ULONG DeviceObject,
 ULONG DeviceProperty,
 ULONG BufferLength,
 ULONG PropertyBuffer,
 ULONG ResultLength
);

PORTCLASSAPI ULONGLONG STDCALL
PcGetTimeInterval(
  ULONGLONG  Timei
);

PORTCLASSAPI NTSTATUS STDCALL
PcInitializeAdapterDriver(
 ULONG DriverObject,
 ULONG RegistryPathName,
 ULONG AddDevice
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewDmaChannel(
 ULONG OutDmaChannel,
 ULONG Unknown,
 ULONG PoolType,
 ULONG DeviceDescription,
 ULONG DeviceObject
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewInterruptSync(
 ULONG OutInterruptSync,
 ULONG Unknown,
 ULONG ResourceList,
 ULONG ResourceIndex,
 ULONG Mode
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewMiniport(
 ULONG OutMiniport,
 ULONG ClassId
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewPort(
 ULONG OutPort,
 ULONG ClassId
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewRegistryKey(
 ULONG OutRegistryKey,
 ULONG Unknown,
 ULONG RegistryKeyType,
 ULONG DesiredAccess,
 ULONG DeviceObject,
 ULONG SubDevice,
 ULONG ObjectAttributes,
 ULONG CreateOptions,
 ULONG Disposition
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewResourceList(
 ULONG OutResourceList,
 ULONG Unknown,
 ULONG PoolType,
 ULONG TranslatedResources,
 ULONG UntranslatedResources
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewResourceSublist(
 ULONG OutResourceList,
 ULONG Unknown,
 ULONG PoolType,
 ULONG ParentList,
 ULONG MaximumEntries
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewServiceGroup(
 ULONG OutServiceGroup,
 ULONG Unknown
);

PORTCLASSAPI NTSTATUS STDCALL
PcRegisterAdapterPowerManagement(
 ULONG Unknown,
 ULONG pvContext
);

PORTCLASSAPI NTSTATUS STDCALL
PcRegisterIoTimeout(
 ULONG pDeviceObject,
 ULONG pTimerRoutine,
 ULONG pContext
);

PORTCLASSAPI NTSTATUS STDCALL
PcRegisterPhysicalConnection(
 ULONG DeviceObject,
 ULONG FromUnknown,
 ULONG FromPin,
 ULONG ToUnknown,
 ULONG ToPin
);

PORTCLASSAPI NTSTATUS STDCALL
PcRegisterPhysicalConnectionFromExternal(
 ULONG DeviceObject,
 ULONG FromString,
 ULONG FromPin,
 ULONG ToUnknown,
 ULONG ToPin
);

PORTCLASSAPI NTSTATUS STDCALL
PcRegisterPhysicalConnectionToExternal(
 ULONG DeviceObject,
 ULONG FromUnknown,
 ULONG FromPin,
 ULONG ToString,
 ULONG ToPin
);

PORTCLASSAPI NTSTATUS STDCALL
PcRegisterSubdevice(
 ULONG DeviceObject,
 ULONG SubdevName,
 ULONG Unknown
);

PORTCLASSAPI NTSTATUS STDCALL
PcRequestNewPowerState(
 ULONG pDeviceObject,
 ULONG RequestedNewState
);

PORTCLASSAPI NTSTATUS STDCALL
PcUnregisterIoTimeout(
 ULONG pDeviceObject,
 ULONG pTimerRoutine,
 ULONG pContext
);

#ifdef __cplusplus
}
#endif                          

#endif
