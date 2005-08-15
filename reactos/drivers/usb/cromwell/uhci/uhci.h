//#include <ddk/ntddk.h>
// config and include core/hcd.h, for hc_device struct struct usb_interface *usb_ifnum_to_if(struct usb_device *dev, unsigned ifnum)

#include "../usb_wrapper.h"
#include <ddk/usbdi.h>
#include <ddk/usbiodef.h>
#include <initguid.h>

#include "../core/hcd.h"

#include "../host/ohci_main.h"

#define USB_UHCI_TAG TAG('u','s','b','u')

/* cleanup.c */
NTSTATUS STDCALL
UhciCleanup(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/* close.c */
NTSTATUS STDCALL
UhciClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/* create.c */
NTSTATUS STDCALL
UhciCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/* fdo.c */
NTSTATUS STDCALL
UhciPnpFdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UhciDeviceControlFdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/* misc.c */
NTSTATUS
ForwardIrpAndWait(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS STDCALL
ForwardIrpAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UhciDuplicateUnicodeString(
	OUT PUNICODE_STRING Destination,
	IN PUNICODE_STRING Source,
	IN POOL_TYPE PoolType);

NTSTATUS
UhciInitMultiSzString(
	OUT PUNICODE_STRING Destination,
	... /* list of PCSZ */);

/* pdo.c */
NTSTATUS STDCALL
UhciPnpPdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UhciDeviceControlPdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);
