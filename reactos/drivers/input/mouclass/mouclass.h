#include <ntifs.h>
#include <kbdmou.h>
#include <ntddmou.h>
#include <stdio.h>

#if defined(__GNUC__)
  NTSTATUS NTAPI
  IoAttachDeviceToDeviceStackSafe(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice,
    OUT PDEVICE_OBJECT *AttachedToDeviceObject);
#else
  #error Unknown compiler!
#endif

typedef enum
{
	dsStopped,
	dsStarted,
	dsPaused,
	dsRemoved,
	dsSurpriseRemoved
} MOUCLASS_DEVICE_STATE;

typedef struct _MOUCLASS_DRIVER_EXTENSION
{
	/* Registry settings */
	ULONG ConnectMultiplePorts;
	ULONG MouseDataQueueSize;
	UNICODE_STRING PointerDeviceBaseName;

	PDEVICE_OBJECT MainMouclassDeviceObject;
} MOUCLASS_DRIVER_EXTENSION, *PMOUCLASS_DRIVER_EXTENSION;

typedef struct _COMMON_DEVICE_EXTENSION
{
	BOOLEAN IsClassDO;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _MOUPORT_DEVICE_EXTENSION
{
	COMMON_DEVICE_EXTENSION Common;
} MOUPORT_DEVICE_EXTENSION, *PMOUPORT_DEVICE_EXTENSION;

typedef struct _MOUCLASS_DEVICE_EXTENSION
{
	COMMON_DEVICE_EXTENSION Common;

	MOUCLASS_DEVICE_STATE PnpState;
	PMOUCLASS_DRIVER_EXTENSION DriverExtension;
	PDEVICE_OBJECT LowerDevice;
	UNICODE_STRING MouseInterfaceName;

	KSPIN_LOCK SpinLock;
	BOOLEAN ReadIsPending;
	ULONG InputCount;
	PMOUSE_INPUT_DATA PortData;
} MOUCLASS_DEVICE_EXTENSION, *PMOUCLASS_DEVICE_EXTENSION;

/* misc.c */

NTSTATUS NTAPI
ForwardIrpAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);
