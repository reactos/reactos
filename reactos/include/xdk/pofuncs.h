/******************************************************************************
 *                     Power Management Support Functions                     *
 ******************************************************************************/

#define PoSetDeviceBusy(IdlePointer) ((void)(*(IdlePointer) = 0))

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
NTSTATUS
NTAPI
PoCallDriver(
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN OUT struct _IRP *Irp);

NTKERNELAPI
PULONG
NTAPI
PoRegisterDeviceForIdleDetection(
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN ULONG ConservationIdleTime,
  IN ULONG PerformanceIdleTime,
  IN DEVICE_POWER_STATE State);

NTKERNELAPI
PVOID
NTAPI
PoRegisterSystemState(
  IN OUT PVOID StateHandle OPTIONAL,
  IN EXECUTION_STATE Flags);

NTKERNELAPI
NTSTATUS
NTAPI
PoRequestPowerIrp(
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN UCHAR MinorFunction,
  IN POWER_STATE PowerState,
  IN PREQUEST_POWER_COMPLETE CompletionFunction OPTIONAL,
  IN PVOID Context OPTIONAL,
  OUT struct _IRP **Irp OPTIONAL);

NTKERNELAPI
POWER_STATE
NTAPI
PoSetPowerState(
  IN struct _DEVICE_OBJECT *DeviceObject,
  IN POWER_STATE_TYPE Type,
  IN POWER_STATE State);

NTKERNELAPI
VOID
NTAPI
PoSetSystemState(
  IN EXECUTION_STATE Flags);

NTKERNELAPI
VOID
NTAPI
PoStartNextPowerIrp(
  IN OUT struct _IRP *Irp);

NTKERNELAPI
VOID
NTAPI
PoUnregisterSystemState(
  IN OUT PVOID StateHandle);

NTKERNELAPI
NTSTATUS
NTAPI
PoRequestShutdownEvent(
  OUT PVOID *Event);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTKERNELAPI
VOID
NTAPI
PoSetSystemWake(
  IN OUT struct _IRP *Irp);

NTKERNELAPI
BOOLEAN
NTAPI
PoGetSystemWake(
  IN struct _IRP *Irp);

NTKERNELAPI
NTSTATUS
NTAPI
PoRegisterPowerSettingCallback(
  IN PDEVICE_OBJECT DeviceObject OPTIONAL,
  IN LPCGUID SettingGuid,
  IN PPOWER_SETTING_CALLBACK Callback,
  IN PVOID Context OPTIONAL,
  OUT PVOID *Handle OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
PoUnregisterPowerSettingCallback(
  IN OUT PVOID Handle);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
NTKERNELAPI
VOID
NTAPI
PoSetDeviceBusyEx(
  IN OUT PULONG IdlePointer);
#endif /* (NTDDI_VERSION >= NTDDI_VISTASP1) */

#if (NTDDI_VERSION >= NTDDI_WIN7)

NTKERNELAPI
VOID
NTAPI
PoStartDeviceBusy(
  IN OUT PULONG IdlePointer);

NTKERNELAPI
VOID
NTAPI
PoEndDeviceBusy(
  IN OUT PULONG IdlePointer);

NTKERNELAPI
BOOLEAN
NTAPI
PoQueryWatchdogTime(
  IN PDEVICE_OBJECT Pdo,
  OUT PULONG SecondsRemaining);

NTKERNELAPI
VOID
NTAPI
PoDeletePowerRequest(
  IN OUT PVOID PowerRequest);

NTKERNELAPI
NTSTATUS
NTAPI
PoSetPowerRequest(
  IN OUT PVOID PowerRequest,
  IN POWER_REQUEST_TYPE Type);

NTKERNELAPI
NTSTATUS
NTAPI
PoClearPowerRequest(
  IN OUT PVOID PowerRequest,
  IN POWER_REQUEST_TYPE Type);

NTKERNELAPI
NTSTATUS
NTAPI
PoCreatePowerRequest(
  OUT PVOID *PowerRequest,
  IN PDEVICE_OBJECT DeviceObject,
  IN PCOUNTED_REASON_CONTEXT Context);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

