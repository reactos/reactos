#ifndef _USBMP_COMMON_H_
#define _USBMP_COMMON_H_

// config and include core/hcd.h, for hc_device struct struct usb_interface *usb_ifnum_to_if(struct usb_device *dev, unsigned ifnum)

#include "../usb_wrapper.h"
#include <usbdi.h>
#include <usbiodef.h>
#include <initguid.h>

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define USB_MINIPORT_TAG TAG('u','s','b','m')

#include "../../usbport/hcd.h"
#include "usbcommon_types.h"

/* cleanup.c */
NTSTATUS STDCALL
UsbMpCleanup(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/* close.c */
NTSTATUS STDCALL
UsbMpClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/* create.c */
NTSTATUS STDCALL
UsbMpCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/* fdo.c */
NTSTATUS STDCALL
UsbMpPnpFdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UsbMpDeviceControlFdo(
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
UsbMpDuplicateUnicodeString(
	OUT PUNICODE_STRING Destination,
	IN PUNICODE_STRING Source,
	IN POOL_TYPE PoolType);

NTSTATUS
UsbMpInitMultiSzString(
	OUT PUNICODE_STRING Destination,
	... /* list of PCSZ */);

/* pdo.c */
NTSTATUS STDCALL
UsbMpPnpPdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UsbMpDeviceControlPdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/* Needed by this object library */
VOID STDCALL 
DriverUnload(PDRIVER_OBJECT DriverObject);

NTSTATUS
InitLinuxWrapper(PDEVICE_OBJECT DeviceObject);

extern struct pci_device_id** pci_ids;

#endif
