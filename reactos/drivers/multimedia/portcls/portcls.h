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

#include <ddk/ntddk.h>

#define PORTCLASSAPI extern

PORTCLASSAPI NTSTATUS STDCALL
PcAddAdapterDevice(
 DWORD DriverObject,
 DWORD PhysicalDeviceObject,
 DWORD StartDevice,
 DWORD MaxObjects,
 DWORD DeviceExtensionSize
);

PORTCLASSAPI NTSTATUS STDCALL
PcAddContentHandlers(
 DWORD  ContentId,
 DWORD  paHandlers,
 DWORD  NumHandlers
);

PORTCLASSAPI NTSTATUS STDCALL
PcCompleteIrp(
 DWORD  DeviceObject,
 DWORD  Irp,
 DWORD  Status
);

PORTCLASSAPI NTSTATUS STDCALL
PcCompletePendingPropertyRequest(
 DWORD PropertyRequest,
 DWORD NtStatus
);

PORTCLASSAPI NTSTATUS STDCALL 
PcCreateContentMixed(
 DWORD paContentId,
 DWORD cContentId,
 DWORD pMixedContentId
);

PORTCLASSAPI NTSTATUS STDCALL
PcDestroyContent(
 DWORD ContentId
);

PORTCLASSAPI NTSTATUS STDCALL
PcDispatchIrp(
 DWORD DeviceObject,
 DWORD Irp
);

PORTCLASSAPI NTSTATUS STDCALL
PcForwardContentToDeviceObject(
 DWORD ContentId,
 DWORD Reserved,
 DWORD DrmForward
);

PORTCLASSAPI NTSTATUS STDCALL 
PcForwardContentToFileObject(
 DWORD ContentId,
 DWORD FileObject
);

PORTCLASSAPI NTSTATUS STDCALL 
PcForwardContentToInterface(
 DWORD ContentId,
 DWORD Unknown,
 DWORD NumMethods
);

PORTCLASSAPI NTSTATUS STDCALL
PcForwardIrpSynchronous(
 DWORD DeviceObject,
 DWORD Irp 
);

PORTCLASSAPI NTSTATUS STDCALL 
PcGetContentRights(
 DWORD ContentId,
 DWORD DrmRights
); 

PORTCLASSAPI NTSTATUS STDCALL
PcGetDeviceProperty(
 DWORD DeviceObject,
 DWORD DeviceProperty,
 DWORD BufferLength,
 DWORD PropertyBuffer,
 DWORD ResultLength
);

PORTCLASSAPI ULONGLONG STDCALL
PcGetTimeInterval(
  ULONGLONG  Timei
);

PORTCLASSAPI NTSTATUS STDCALL
PcInitializeAdapterDriver(
 DWORD DriverObject,
 DWORD RegistryPathName,
 DWORD AddDevice
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewDmaChannel(
 DWORD OutDmaChannel,
 DWORD Unknown,
 DWORD PoolType,
 DWORD DeviceDescription,
 DWORD DeviceObject
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewInterruptSync(
 DWORD OutInterruptSync,
 DWORD Unknown,
 DWORD ResourceList,
 DWORD ResourceIndex,
 DWORD Mode
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewMiniport(
 DWORD OutMiniport,
 DWORD ClassId
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewPort(
 DWORD OutPort,
 DWORD ClassId
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewRegistryKey(
 DWORD OutRegistryKey,
 DWORD Unknown,
 DWORD RegistryKeyType,
 DWORD DesiredAccess,
 DWORD DeviceObject,
 DWORD SubDevice,
 DWORD ObjectAttributes,
 DWORD CreateOptions,
 DWORD Disposition
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewResourceList(
 DWORD OutResourceList,
 DWORD Unknown,
 DWORD PoolType,
 DWORD TranslatedResources,
 DWORD UntranslatedResources
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewResourceSublist(
 DWORD OutResourceList,
 DWORD Unknown,
 DWORD PoolType,
 DWORD ParentList,
 DWORD MaximumEntries
);

PORTCLASSAPI NTSTATUS STDCALL
PcNewServiceGroup(
 DWORD OutServiceGroup,
 DWORD Unknown
);

PORTCLASSAPI NTSTATUS STDCALL
PcRegisterAdapterPowerManagement(
 DWORD Unknown,
 DWORD pvContext
);

PORTCLASSAPI NTSTATUS STDCALL
PcRegisterIoTimeout(
 DWORD pDeviceObject,
 DWORD pTimerRoutine,
 DWORD pContext
);

PORTCLASSAPI NTSTATUS STDCALL
PcRegisterPhysicalConnection(
 DWORD DeviceObject,
 DWORD FromUnknown,
 DWORD FromPin,
 DWORD ToUnknown,
 DWORD ToPin
);

PORTCLASSAPI NTSTATUS STDCALL
PcRegisterPhysicalConnectionFromExternal(
 DWORD DeviceObject,
 DWORD FromString,
 DWORD FromPin,
 DWORD ToUnknown,
 DWORD ToPin
);

PORTCLASSAPI NTSTATUS STDCALL
PcRegisterPhysicalConnectionToExternal(
 DWORD DeviceObject,
 DWORD FromUnknown,
 DWORD FromPin,
 DWORD ToString,
 DWORD ToPin
);

PORTCLASSAPI NTSTATUS STDCALL
PcRegisterSubdevice(
 DWORD DeviceObject,
 DWORD SubdevName,
 DWORD Unknown
);

PORTCLASSAPI NTSTATUS STDCALL
PcRequestNewPowerState(
 DWORD pDeviceObject,
 DWORD RequestedNewState
);

PORTCLASSAPI NTSTATUS STDCALL
PcUnregisterIoTimeout(
 DWORD pDeviceObject,
 DWORD pTimerRoutine,
 DWORD pContext
);

#ifdef __cplusplus
}
#endif                          

#endif
