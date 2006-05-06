#ifndef _USBMP_COMMON_H_
#define _USBMP_COMMON_H_

// config and include core/hcd.h, for hc_device struct struct usb_interface *usb_ifnum_to_if(struct usb_device *dev, unsigned ifnum)

#include "../usb_wrapper.h"
#include <usbdi.h>
#include <usbiodef.h>
#include <initguid.h>
#include <ntdd8042.h>

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define USB_MINIPORT_TAG TAG('u','s','b','m')

#include "../../usbport/hcd.h"
#include "usbcommon_types.h"

extern CONNECT_DATA KbdClassInformation;
extern CONNECT_DATA MouseClassInformation;
extern PDEVICE_OBJECT KeyboardFdo;
extern PDEVICE_OBJECT MouseFdo;

/* fdo.c */
NTSTATUS
UsbMpFdoCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UsbMpFdoClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UsbMpFdoCleanup(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UsbMpFdoPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UsbMpFdoDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/* misc.c */
NTSTATUS
ForwardIrpAndWait(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
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
NTSTATUS
UsbMpPdoCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UsbMpPdoClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UsbMpPdoCleanup(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UsbMpPdoPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UsbMpPdoDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
UsbMpPdoInternalDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/*
 * Needed by this object library, but not
 * present in any file of this library
 */
VOID NTAPI 
DriverUnload(PDRIVER_OBJECT DriverObject);

NTSTATUS
InitLinuxWrapper(PDEVICE_OBJECT DeviceObject);

extern struct pci_device_id* pci_ids;

#endif
