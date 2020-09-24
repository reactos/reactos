$if (_WDMDDK_)
/******************************************************************************
 *                     Power Management Support Functions                     *
 ******************************************************************************/

#define PoSetDeviceBusy(IdlePointer) ((void)(*(IdlePointer) = 0))

#if (NTDDI_VERSION >= NTDDI_WIN2K)

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PoCallDriver(
  _In_ struct _DEVICE_OBJECT *DeviceObject,
  _Inout_ __drv_aliasesMem struct _IRP *Irp);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PULONG
NTAPI
PoRegisterDeviceForIdleDetection(
  _In_ struct _DEVICE_OBJECT *DeviceObject,
  _In_ ULONG ConservationIdleTime,
  _In_ ULONG PerformanceIdleTime,
  _In_ DEVICE_POWER_STATE State);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PVOID
NTAPI
PoRegisterSystemState(
  _Inout_opt_ PVOID StateHandle,
  _In_ EXECUTION_STATE Flags);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PoRequestPowerIrp(
  _In_ struct _DEVICE_OBJECT *DeviceObject,
  _In_ UCHAR MinorFunction,
  _In_ POWER_STATE PowerState,
  _In_opt_ PREQUEST_POWER_COMPLETE CompletionFunction,
  _In_opt_ __drv_aliasesMem PVOID Context,
  _Outptr_opt_ struct _IRP **Irp);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
POWER_STATE
NTAPI
PoSetPowerState(
  _In_ struct _DEVICE_OBJECT *DeviceObject,
  _In_ POWER_STATE_TYPE Type,
  _In_ POWER_STATE State);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
PoSetSystemState(
  _In_ EXECUTION_STATE Flags);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
PoStartNextPowerIrp(
  _Inout_ struct _IRP *Irp);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
PoUnregisterSystemState(
  _Inout_ PVOID StateHandle);

NTKERNELAPI
NTSTATUS
NTAPI
PoRequestShutdownEvent(
  OUT PVOID *Event);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */
$endif (_WDMDDK_)

$if (_NTIFS_)
#if (NTDDI_VERSION >= NTDDI_WINXP)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PoQueueShutdownWorkItem(
  _Inout_ __drv_aliasesMem PWORK_QUEUE_ITEM WorkItem);
#endif
$endif (_NTIFS_)
$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_VISTA)

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKRNLVISTAAPI
VOID
NTAPI
PoSetSystemWake(
  _Inout_ struct _IRP *Irp);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKRNLVISTAAPI
BOOLEAN
NTAPI
PoGetSystemWake(
  _In_ struct _IRP *Irp);

_IRQL_requires_max_(APC_LEVEL)
NTKRNLVISTAAPI
NTSTATUS
NTAPI
PoRegisterPowerSettingCallback(
  _In_opt_ PDEVICE_OBJECT DeviceObject,
  _In_ LPCGUID SettingGuid,
  _In_ PPOWER_SETTING_CALLBACK Callback,
  _In_opt_ PVOID Context,
  _Outptr_opt_ PVOID *Handle);

_IRQL_requires_max_(APC_LEVEL)
NTKRNLVISTAAPI
NTSTATUS
NTAPI
PoUnregisterPowerSettingCallback(
  _Inout_ PVOID Handle);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
NTKERNELAPI
VOID
NTAPI
PoSetDeviceBusyEx(
  _Inout_ PULONG IdlePointer);
#endif /* (NTDDI_VERSION >= NTDDI_VISTASP1) */

#if (NTDDI_VERSION >= NTDDI_WIN7)

NTKERNELAPI
VOID
NTAPI
PoStartDeviceBusy(
  _Inout_ PULONG IdlePointer);

NTKERNELAPI
VOID
NTAPI
PoEndDeviceBusy(
  _Inout_ PULONG IdlePointer);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKRNLVISTAAPI
BOOLEAN
NTAPI
PoQueryWatchdogTime(
  _In_ PDEVICE_OBJECT Pdo,
  _Out_ PULONG SecondsRemaining);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
PoDeletePowerRequest(
  _Inout_ PVOID PowerRequest);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PoSetPowerRequest(
  _Inout_ PVOID PowerRequest,
  _In_ POWER_REQUEST_TYPE Type);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PoClearPowerRequest(
  _Inout_ PVOID PowerRequest,
  _In_ POWER_REQUEST_TYPE Type);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PoCreatePowerRequest(
  _Outptr_ PVOID *PowerRequest,
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_opt_ PCOUNTED_REASON_CONTEXT Context);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

