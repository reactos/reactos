#include <ntifs.h>
#include <kbdmou.h>
#include <ntddkbd.h>
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
} KBDCLASS_DEVICE_STATE;

typedef struct _KBDCLASS_DRIVER_EXTENSION
{
	/* Registry settings */
	ULONG ConnectMultiplePorts;
	ULONG KeyboardDataQueueSize;
	UNICODE_STRING KeyboardDeviceBaseName;

	PDEVICE_OBJECT MainKbdclassDeviceObject;
} KBDCLASS_DRIVER_EXTENSION, *PKBDCLASS_DRIVER_EXTENSION;

typedef struct _COMMON_DEVICE_EXTENSION
{
	BOOLEAN IsClassDO;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _KBDPORT_DEVICE_EXTENSION
{
	COMMON_DEVICE_EXTENSION Common;
} KBDPORT_DEVICE_EXTENSION, *PKBDPORT_DEVICE_EXTENSION;

typedef struct _KBDCLASS_DEVICE_EXTENSION
{
	COMMON_DEVICE_EXTENSION Common;

	KBDCLASS_DEVICE_STATE PnpState;
	PKBDCLASS_DRIVER_EXTENSION DriverExtension;
	PDEVICE_OBJECT LowerDevice;
	UNICODE_STRING KeyboardInterfaceName;

	KSPIN_LOCK SpinLock;
	BOOLEAN ReadIsPending;
	ULONG InputCount;
	PKEYBOARD_INPUT_DATA PortData;
} KBDCLASS_DEVICE_EXTENSION, *PKBDCLASS_DEVICE_EXTENSION;

/* misc.c */

NTSTATUS NTAPI
ForwardIrpAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);
