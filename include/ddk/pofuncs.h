#ifndef __INCLUDE_DDK_POFUNCS_H
#define __INCLUDE_DDK_POFUNCS_H

NTSTATUS
STDCALL
PoCallDriver(
  IN PDEVICE_OBJECT DeviceObject,
  IN OUT PIRP Irp);

PULONG
STDCALL
PoRegisterDeviceForIdleDetection(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG ConservationIdleTime,
  IN ULONG PerformanceIdleTime,
  IN DEVICE_POWER_STATE State);

PVOID
STDCALL
PoRegisterSystemState(
  IN PVOID StateHandle,
  IN EXECUTION_STATE Flags);

NTSTATUS
STDCALL
PoRequestPowerIrp(
  IN PDEVICE_OBJECT DeviceObject,
  IN UCHAR MinorFunction,  
  IN POWER_STATE PowerState,
  IN PREQUEST_POWER_COMPLETE CompletionFunction,
  IN PVOID Context,
  OUT PIRP *Irp   OPTIONAL);

VOID
STDCALL
PoSetDeviceBusy(
  PULONG IdlePointer);

POWER_STATE
STDCALL
PoSetPowerState(
  IN PDEVICE_OBJECT DeviceObject,
  IN POWER_STATE_TYPE Type,
  IN POWER_STATE State);

VOID
STDCALL
PoSetSystemState(
  IN EXECUTION_STATE Flags);

VOID
STDCALL
PoStartNextPowerIrp(
  IN PIRP Irp);

VOID
STDCALL
PoUnregisterSystemState(
  IN PVOID StateHandle);

#endif /* __INCLUDE_DDK_POFUNCS_H */
