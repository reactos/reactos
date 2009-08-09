#include <ntddk.h>
#include <usbdi.h>
#include <usbiodef.h>
#include <initguid.h>
#include <debug.h>

#define USB_STOR_TAG TAG('u','s','b','s')
#define USB_MAXCHILDREN              (16)

NTSTATUS NTAPI
IoAttachDeviceToDeviceStackSafe(
  IN PDEVICE_OBJECT SourceDevice,
  IN PDEVICE_OBJECT TargetDevice,
  OUT PDEVICE_OBJECT *AttachedToDeviceObject);

typedef struct _USBSTOR_DEVICE_EXTENSION
{
	BOOLEAN IsFDO;
	struct usb_device* dev;
	PDEVICE_OBJECT LowerDevice;

	PDEVICE_OBJECT Children[USB_MAXCHILDREN];

	/* Fields valid only when IsFDO == FALSE */
	UNICODE_STRING DeviceDescription; // REG_SZ
	UNICODE_STRING DeviceId;          // REG_SZ
	UNICODE_STRING InstanceId;        // REG_SZ
	UNICODE_STRING HardwareIds;       // REG_MULTI_SZ
	UNICODE_STRING CompatibleIds;     // REG_MULTI_SZ
	UNICODE_STRING SymbolicLinkName;
} USBSTOR_DEVICE_EXTENSION, *PUSBSTOR_DEVICE_EXTENSION;


/* cleanup.c */
NTSTATUS NTAPI
UsbStorCleanup(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/* fdo.c */
NTSTATUS NTAPI
UsbStorPnpFdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UsbStorDeviceControlFdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/* misc.c */
NTSTATUS
ForwardIrpAndWait(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS NTAPI
ForwardIrpAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UsbStorDuplicateUnicodeString(
	OUT PUNICODE_STRING Destination,
	IN PUNICODE_STRING Source,
	IN POOL_TYPE PoolType);

NTSTATUS
UsbStorInitMultiSzString(
	OUT PUNICODE_STRING Destination,
	... /* list of PCSZ */);

/* pdo.c */
NTSTATUS NTAPI
UsbStorPnpPdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UsbStorDeviceControlPdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

