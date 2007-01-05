/*
   ReactOS specific functions for UHCI module
   by Aleksey Bragin (aleksey@reactos.com)
   and Hervé Poussineau (hpoussin@reactos.org)
   Some parts of code are inspired (or even just copied) from ReactOS Videoport driver
*/
//#define NDEBUG
#include "uhci.h"

extern struct pci_driver uhci_pci_driver;
extern struct pci_device_id uhci_pci_ids[];
struct pci_device_id* pci_ids = &uhci_pci_ids[0];

NTSTATUS
InitLinuxWrapper(PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS Status;
	PUSBMP_DEVICE_EXTENSION DeviceExtension = (PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
	/* Create generic linux structure */
	struct pci_dev *dev;
	dev = ExAllocatePoolWithTag(PagedPool, sizeof(struct pci_dev), USB_UHCI_TAG);
	DeviceExtension->pdev = dev;
	
	/* Initialize generic linux structure */
	dev->irq = DeviceExtension->InterruptVector;
	dev->dev_ext = (PVOID)DeviceExtension;
	dev->dev.dev_ext = (PVOID)DeviceExtension; /*DeviceObject*/
	dev->slot_name = ExAllocatePoolWithTag(NonPagedPool, 128, USB_UHCI_TAG); // 128 max len for slot name
	dev->bus = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct usb_bus), USB_UHCI_TAG); //FIXME: Remove all this
								// small allocations from non-paged pool, move them to big device extension struct
	RtlZeroMemory(dev->bus, sizeof(struct usb_bus));

	/* Init wrapper */
	init_wrapper(dev);
	
	strcpy(dev->dev.name, "UnivHCI PCI-USB Controller");
	strcpy(dev->slot_name, "UHCD PCI Slot");
	
	/* Init the HCD. Probe will be called automatically, but will fail because id=NULL */
	Status = uhci_hcd_init();
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("UHCI: uhci_hcd_init() failed with status 0x%08lx\n", Status);
		/* FIXME: deinitialize linux wrapper */
		ExFreePoolWithTag(dev->slot_name, USB_UHCI_TAG);
		ExFreePoolWithTag(dev->bus, USB_UHCI_TAG);
		ExFreePoolWithTag(dev, USB_UHCI_TAG);
		return Status;
	}
	DPRINT("uhci_hcd_init() done\n");

	/* Init core usb */
	usb_init();
	DPRINT("usb_init() done\n");

	{
		/* Test code by Fireball */
		struct device dev;
		dma_addr_t dma_handle;
		void *res;

		dev.dev_ext = DeviceExtension;
		res = dma_alloc_coherent(&dev, 12, &dma_handle, 0);
	}

	/* Probe device with real id now */
	uhci_pci_driver.probe(dev, uhci_pci_ids);

	DPRINT("InitLinuxWrapper() done\n");
	return STATUS_SUCCESS; 
}

VOID STDCALL 
DriverUnload(PDRIVER_OBJECT DriverObject)
{
	PUSBMP_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_OBJECT DeviceObject;
	struct pci_dev *dev;

	DeviceObject = DriverObject->DeviceObject;
	DeviceExtension = (PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	dev = DeviceExtension->pdev;

	DPRINT1("UHCI: DriverUnload()\n");

	// Exit usb device
	usb_exit();

	// Remove device (uhci_pci_driver.remove)
	uhci_pci_driver.remove(dev);

	ExFreePool(dev->slot_name);
	ExFreePool(dev->bus);
	ExFreePool(dev);

	// Perform some cleanup
	uhci_hcd_cleanup();
}
