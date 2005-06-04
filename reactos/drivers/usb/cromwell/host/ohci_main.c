/*
   ReactOS specific functions for OHCI module
   by Aleksey Bragin (aleksey@reactos.com)
   Some parts of code are inspired (or even just copied) from ReactOS Videoport driver
*/

#include <ddk/ntddk.h>
#include <debug.h>
#include "../usb_wrapper.h"
#include "../core/hcd.h"
#include "ohci_main.h"

// declare basic init funcs
void init_wrapper(struct pci_dev *probe_dev);
int ohci_hcd_pci_init (void);
void ohci_hcd_pci_cleanup (void);
int STDCALL usb_init(void);
void STDCALL usb_exit(void);
extern struct pci_driver ohci_pci_driver;
extern const struct pci_device_id pci_ids[];



// This should be removed, but for testing purposes it's here
struct pci_dev *dev;
//struct pci_device_id *dev_id;


#define USB_OHCI_TAG TAG('u','s','b','o')

NTSTATUS STDCALL AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT pdo)
{
	PDEVICE_OBJECT fdo;
	NTSTATUS Status;
	WCHAR DeviceBuffer[20];
	UNICODE_STRING DeviceName;
	POHCI_DRIVER_EXTENSION DriverExtension;
	POHCI_DEVICE_EXTENSION DeviceExtension;
	ULONG Size, DeviceNumber;

	DPRINT1("ohci: AddDevice called\n");

	// Allocate driver extension now
	DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
	if (DriverExtension == NULL)
	{
		Status = IoAllocateDriverObjectExtension(
					DriverObject,
					DriverObject,
					sizeof(OHCI_DRIVER_EXTENSION),
					(PVOID *)&DriverExtension);

		if (!NT_SUCCESS(Status))
		{
			DPRINT1("Allocating DriverObjectExtension failed.\n");
			return Status;
		}
	}
   
	// Create a unicode device name
	DeviceNumber = 0; //TODO: Allocate new device number every time
	swprintf(DeviceBuffer, L"\\Device\\USBFDO-%lu", DeviceNumber);
	RtlInitUnicodeString(&DeviceName, DeviceBuffer);

	Status = IoCreateDevice(DriverObject,
		                    sizeof(OHCI_DEVICE_EXTENSION)/* + DriverExtension->InitializationData.HwDeviceExtensionSize*/,
							&DeviceName,
							FILE_DEVICE_CONTROLLER,
							0,
							FALSE,
							&fdo);

	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoCreateDevice call failed with status 0x%08x\n", Status);
		return Status;
	}

	// zerofill device extension
	DeviceExtension = (POHCI_DEVICE_EXTENSION)pdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(OHCI_DEVICE_EXTENSION));
	DeviceExtension->NextDeviceObject = IoAttachDeviceToDeviceStack(fdo, pdo);

	fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	// Initialize device extension
	DeviceExtension->DeviceNumber = DeviceNumber;
	DeviceExtension->PhysicalDeviceObject = pdo;
	DeviceExtension->FunctionalDeviceObject = fdo;
	DeviceExtension->DriverExtension = DriverExtension;

	/* Get bus number from the upper level bus driver. */
	Size = sizeof(ULONG);
	Status = IoGetDeviceProperty(
				pdo,
				DevicePropertyBusNumber,
				Size,
				&DeviceExtension->SystemIoBusNumber,
				&Size);
	
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Couldn't get an information from bus driver. Panic!!!\n");
		return Status;
	}

	DPRINT("Done AddDevice\n");
	return STATUS_SUCCESS;
}

VOID STDCALL DriverUnload(PDRIVER_OBJECT DriverObject)
{
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

NTSTATUS
InitLinuxWrapper(PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS Status;
	POHCI_DEVICE_EXTENSION DeviceExtension = (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	// Fill generic linux structs
	dev = ExAllocatePoolWithTag(PagedPool, sizeof(struct pci_dev), USB_OHCI_TAG);
	
	init_wrapper(dev);
	dev->irq = DeviceExtension->InterruptVector;
	dev->dev_ext = (PVOID)DeviceExtension;
	dev->slot_name = ExAllocatePoolWithTag(NonPagedPool, 128, USB_OHCI_TAG); // 128 max len for slot name

	strcpy(dev->dev.name, "OpenHCI PCI-USB Controller");
	strcpy(dev->slot_name, "OHCD PCI Slot");

	// Init the OHCI HCD. Probe will be called automatically, but will fail because id=NULL
	Status = ohci_hcd_pci_init();
	//FIXME: Check status returned value

	// Init core usb
	usb_init();

	// Probe device with real id now
	ohci_pci_driver.probe(dev, pci_ids);

	DPRINT1("InitLinuxWrapper() done\n");

	return STATUS_SUCCESS;
}

NTSTATUS STDCALL
OHCD_PnPStartDevice(IN PDEVICE_OBJECT DeviceObject,
					IN PIRP	Irp)
{
	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
	PDRIVER_OBJECT DriverObject;
	POHCI_DRIVER_EXTENSION DriverExtension;
	POHCI_DEVICE_EXTENSION DeviceExtension;
	PCM_RESOURCE_LIST AllocatedResources;

	/*
	* Get the initialization data we saved in VideoPortInitialize.
	*/
	DriverObject = DeviceObject->DriverObject;
	DriverExtension	= IoGetDriverObjectExtension(DriverObject, DriverObject);
	DeviceExtension	= (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	/*
	* Store	some resources in the DeviceExtension.
	*/
	AllocatedResources = Stack->Parameters.StartDevice.AllocatedResources;
	if (AllocatedResources != NULL)
	{
		CM_FULL_RESOURCE_DESCRIPTOR	*FullList;
		CM_PARTIAL_RESOURCE_DESCRIPTOR *Descriptor;
		ULONG ResourceCount;
		ULONG ResourceListSize;

		/* Save	the	resource list */
		ResourceCount =	AllocatedResources->List[0].PartialResourceList.Count;
		ResourceListSize =
			FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.
			PartialDescriptors[ResourceCount]);
		DeviceExtension->AllocatedResources	= ExAllocatePool(PagedPool,	ResourceListSize);
		if (DeviceExtension->AllocatedResources	== NULL)
		{
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		RtlCopyMemory(DeviceExtension->AllocatedResources,
			AllocatedResources,
			ResourceListSize);

		/* Get the interrupt level/vector -	needed by HwFindAdapter	sometimes */
		for	(FullList =	AllocatedResources->List;
			FullList < AllocatedResources->List	+ AllocatedResources->Count;
			FullList++)
		{
			/* FIXME: Is this ASSERT ok	for	resources from the PNP manager?	*/
			/*ASSERT(FullList->InterfaceType == PCIBus &&
				FullList->BusNumber	== DeviceExtension->SystemIoBusNumber &&
				1 == FullList->PartialResourceList.Version &&
				1 == FullList->PartialResourceList.Revision);*/
			for	(Descriptor	= FullList->PartialResourceList.PartialDescriptors;
				Descriptor < FullList->PartialResourceList.PartialDescriptors +	FullList->PartialResourceList.Count;
				Descriptor++)
			{
				if (Descriptor->Type ==	CmResourceTypeInterrupt)
				{
					DeviceExtension->InterruptLevel	= Descriptor->u.Interrupt.Level;
					DeviceExtension->InterruptVector = Descriptor->u.Interrupt.Vector;
				}
				else if (Descriptor->Type ==	CmResourceTypeMemory)
				{
					DeviceExtension->BaseAddress	= Descriptor->u.Memory.Start;
					DeviceExtension->BaseAddrLength = Descriptor->u.Memory.Length;
				}
			}
		}
	}
	DPRINT1("Interrupt level: 0x%x Interrupt	Vector:	0x%x\n",
		DeviceExtension->InterruptLevel,
		DeviceExtension->InterruptVector);

	/*
	* Init wrapper with this object
	*/
	return InitLinuxWrapper(DeviceObject);
}

// Dispatch PNP
NTSTATUS STDCALL DispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION IrpSp;
   NTSTATUS Status;

   IrpSp = IoGetCurrentIrpStackLocation(Irp);

   switch (IrpSp->MinorFunction)
   {
      case IRP_MN_START_DEVICE:
         //Status = IntVideoPortForwardIrpAndWait(DeviceObject, Irp);
         //if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
         
		 Status = OHCD_PnPStartDevice(DeviceObject, Irp);
         Irp->IoStatus.Status = Status;
         Irp->IoStatus.Information = 0;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         break;


      case IRP_MN_REMOVE_DEVICE:
      case IRP_MN_QUERY_REMOVE_DEVICE:
      case IRP_MN_CANCEL_REMOVE_DEVICE:
      case IRP_MN_SURPRISE_REMOVAL:

      case IRP_MN_STOP_DEVICE:
         //Status = IntVideoPortForwardIrpAndWait(DeviceObject, Irp);
         //if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
            Status = STATUS_SUCCESS;
         Irp->IoStatus.Status = Status;
         Irp->IoStatus.Information = 0;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);

		 IoDeleteDevice(DeviceObject); // just delete device for now
         break;

      case IRP_MN_QUERY_STOP_DEVICE:
      case IRP_MN_CANCEL_STOP_DEVICE:
         Status = STATUS_SUCCESS;
         Irp->IoStatus.Status = STATUS_SUCCESS;
         Irp->IoStatus.Information = 0;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         break;
         
      default:
         return STATUS_NOT_IMPLEMENTED;
         break;
   }
   
   return Status;
}

NTSTATUS STDCALL DispatchPower(PDEVICE_OBJECT fido, PIRP Irp)
{
	DbgPrint("IRP_MJ_POWER dispatch\n");
	return STATUS_SUCCESS;
}

/*
 * Standard DriverEntry method.
 */
NTSTATUS STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegPath)
{
	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = AddDevice;
	DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
	DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;

	return STATUS_SUCCESS;
}
