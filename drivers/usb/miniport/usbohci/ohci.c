#define NDEBUG
#include <debug.h>

#include "ohci.h"

extern struct pci_driver ohci_pci_driver;
extern struct pci_device_id ohci_pci_ids[];
struct pci_device_id* pci_ids = &ohci_pci_ids[0];

NTSTATUS
InitLinuxWrapper(PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS Status;
	PUSBMP_DEVICE_EXTENSION DeviceExtension = (PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	/* Create generic linux structure */
	struct pci_dev *dev;
	dev = ExAllocatePoolWithTag(PagedPool, sizeof(struct pci_dev), USB_OHCI_TAG);
	DeviceExtension->pdev = dev;
	
	/* Initialize generic linux structure */
	dev->irq = DeviceExtension->InterruptVector;
	dev->dev_ext = (PVOID)DeviceExtension;
	dev->dev.dev_ext = DeviceObject;
	dev->slot_name = ExAllocatePoolWithTag(NonPagedPool, 128, USB_OHCI_TAG); // 128 max len for slot name

	/* Init wrapper */
	init_wrapper(dev);

	strcpy(dev->dev.name, "OpenHCI PCI-USB Controller");
	strcpy(dev->slot_name, "OHCD PCI Slot");

	/* Init the OHCI HCD. Probe will be called automatically, but will fail because id=NULL */
	Status = ohci_hcd_pci_init();
	if (!NT_SUCCESS(Status))
	{
		DPRINT("OHCI: ohci_hcd_pci_init() failed with status 0x%08lx\n", Status);
		/* FIXME: deinitialize linux wrapper */
		ExFreePoolWithTag(dev, USB_OHCI_TAG);
		return Status;
	}

	/* Init core usb */
	usb_init();

	/* Probe device with real id now */
	ohci_pci_driver.probe(dev, ohci_pci_ids);

	return STATUS_SUCCESS;
}

VOID STDCALL DriverUnload(PDRIVER_OBJECT DriverObject)
{
	PUSBMP_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_OBJECT DeviceObject;
	struct pci_dev *dev;

	DeviceObject = DriverObject->DeviceObject;
	DeviceExtension = (PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	dev = DeviceExtension->pdev;

	DPRINT1("DriverUnload()\n");

	// Exit usb device
	usb_exit();

	// Remove device (ohci_pci_driver.remove)
	ohci_pci_driver.remove(dev);

	ExFreePool(dev->slot_name);
	ExFreePool(dev);

	// Perform some cleanup
	ohci_hcd_pci_cleanup();
}
