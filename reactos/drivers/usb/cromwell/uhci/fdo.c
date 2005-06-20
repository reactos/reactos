/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS UHCI controller driver (Cromwell type)
 * FILE:            drivers/usb/cromwell/uhci/fdo.c
 * PURPOSE:         IRP_MJ_PNP/IRP_MJ_DEVICE_CONTROL operations for FDOs
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com),
 *                  James Tabor (jimtabor@adsl-64-217-116-74.dsl.hstntx.swbell.net)
 */

#define NDEBUG
#include "uhci.h"

/* declare basic init functions and structures */
int uhci_hcd_init(void);
int STDCALL usb_init(void);

extern struct pci_driver uhci_pci_driver;
extern struct pci_device_id uhci_pci_ids[];

static NTSTATUS
InitLinuxWrapper(PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS Status = STATUS_SUCCESS;

	POHCI_DEVICE_EXTENSION DeviceExtension = (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
	/* Create generic linux structure */
	struct pci_dev *dev;
	dev = ExAllocatePoolWithTag(PagedPool, sizeof(struct pci_dev), USB_UHCI_TAG);
	DeviceExtension->pdev = dev;
	
	/* Initialize generic linux structure */
	dev->irq = DeviceExtension->InterruptVector;
	dev->dev_ext = (PVOID)DeviceExtension;
	dev->slot_name = ExAllocatePoolWithTag(NonPagedPool, 128, USB_UHCI_TAG); // 128 max len for slot name
	init_wrapper(dev);
	
	strcpy(dev->dev.name, "UnivHCI PCI-USB Controller");
	strcpy(dev->slot_name, "UHCD PCI Slot");
	
	/* Init the OHCI HCD. Probe will be called automatically, but will fail because id=NULL */
	Status = uhci_hcd_init();
	if (!NT_SUCCESS(Status))
	{
		DPRINT("UHCI: uhci_hcd_init() failed with status 0x%08lx\n", Status);
		/* FIXME: deinitialize linux wrapper */
		ExFreePoolWithTag(dev, USB_UHCI_TAG);
		return Status;
	}

	/* Init core usb */
	usb_init();

	/* Probe device with real id now */
	uhci_pci_driver.probe(dev, uhci_pci_ids);

//	DPRINT1("UHCI :SysIoBusNumA %d\n",DeviceExtension->SystemIoBusNumber);
//	DeviceExtension->SystemIoBusNumber = dev->bus->number;
//	DPRINT1("UHCI: SysIoBusNumB %d\n",DeviceExtension->SystemIoBusNumber);

	return Status; 
}

#define IO_METHOD_FROM_CTL_CODE(ctlCode) (ctlCode&0x00000003)

static VOID
UhciGetUserBuffers(
	IN PIRP Irp,
	IN ULONG IoControlCode,
	OUT PVOID* BufferIn,
	OUT PVOID* BufferOut)
{
	ASSERT(Irp);
	ASSERT(BufferIn);
	ASSERT(BufferOut);

	switch (IO_METHOD_FROM_CTL_CODE(IoControlCode))
	{
		case METHOD_BUFFERED:
			*BufferIn = *BufferOut = Irp->AssociatedIrp.SystemBuffer;
			break;
		case METHOD_IN_DIRECT:
		case METHOD_OUT_DIRECT:
			*BufferIn = Irp->AssociatedIrp.SystemBuffer;
			*BufferOut = MmGetSystemAddressForMdl(Irp->MdlAddress);
			break;
		case METHOD_NEITHER:
			*BufferIn = IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.Type3InputBuffer;
			*BufferOut = Irp->UserBuffer;
			break;
		default:
			/* Should never happen */
			*BufferIn = NULL;
			*BufferOut = NULL;
			break;
	}
}

NTSTATUS STDCALL
UhciFdoStartDevice(
	IN PDEVICE_OBJECT DeviceObject,
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
	DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
	DeviceExtension = (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	/*
	* Store	some resources in the DeviceExtension.
	*/
	AllocatedResources = Stack->Parameters.StartDevice.AllocatedResources;
	if (AllocatedResources != NULL)
	{
		CM_FULL_RESOURCE_DESCRIPTOR    *FullList;
		CM_PARTIAL_RESOURCE_DESCRIPTOR *Descriptor;
		ULONG ResourceCount;
		ULONG ResourceListSize;

		/* Save the resource list */
		ResourceCount = AllocatedResources->List[0].PartialResourceList.Count;

		ResourceListSize = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.
			PartialDescriptors[ResourceCount]);

		DeviceExtension->AllocatedResources = ExAllocatePool(PagedPool, ResourceListSize);
		if (DeviceExtension->AllocatedResources == NULL)
		{
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		RtlCopyMemory(DeviceExtension->AllocatedResources,
			AllocatedResources,
			ResourceListSize);

		/* Get the interrupt level/vector -	needed by HwFindAdapter	sometimes */
		for	(FullList = AllocatedResources->List;
		   	FullList < AllocatedResources->List + AllocatedResources->Count;
		   	FullList++)
		{
			/* FIXME: Is this ASSERT ok	for	resources from the PNP manager?	*/
			/*ASSERT(FullList->InterfaceType == PCIBus &&
				FullList->BusNumber	== DeviceExtension->SystemIoBusNumber &&
				1 == FullList->PartialResourceList.Version &&
				1 == FullList->PartialResourceList.Revision);*/
			for	(Descriptor	= FullList->PartialResourceList.PartialDescriptors;
				Descriptor < FullList->PartialResourceList.PartialDescriptors + FullList->PartialResourceList.Count;
				Descriptor++)
			{
				if (Descriptor->Type == CmResourceTypeInterrupt)
				{
					DeviceExtension->InterruptLevel  = Descriptor->u.Interrupt.Level;
					DeviceExtension->InterruptVector = Descriptor->u.Interrupt.Vector;
				}
				else if (Descriptor->Type == CmResourceTypePort)
				{
					DeviceExtension->BaseAddress    = Descriptor->u.Port.Start;
					DeviceExtension->BaseAddrLength = Descriptor->u.Port.Length;
					DeviceExtension->Flags          = Descriptor->Flags;

					((struct hc_driver *)uhci_pci_ids->driver_data)->flags &= ~HCD_MEMORY;
				}
				else if (Descriptor->Type == CmResourceTypeMemory)
				{
					DeviceExtension->BaseAddress    = Descriptor->u.Memory.Start;
					DeviceExtension->BaseAddrLength = Descriptor->u.Memory.Length;
					DeviceExtension->Flags          = Descriptor->Flags;

					((struct hc_driver *)uhci_pci_ids->driver_data)->flags |= HCD_MEMORY;
				}
			}
		}
	}
	
	/* Print assigned resources */
	DPRINT("UHCI: Interrupt Vector 0x%lx, %S base 0x%lx, Length 0x%lx\n",
		DeviceExtension->InterruptVector,
		((struct hc_driver *)uhci_pci_ids->driver_data)->flags & HCD_MEMORY ? L"Memory" : L"I/O",
		DeviceExtension->BaseAddress,
		DeviceExtension->BaseAddrLength);

	/* Init wrapper with this object */
	return InitLinuxWrapper(DeviceObject);
}

static NTSTATUS
UhciFdoQueryBusRelations(
	IN PDEVICE_OBJECT DeviceObject,
	OUT PDEVICE_RELATIONS* pDeviceRelations)
{
	POHCI_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_RELATIONS DeviceRelations;
	NTSTATUS Status = STATUS_SUCCESS;
	
	DeviceExtension = (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
	/* Handling this IRP is easy, as we only
	 * have one child: the root hub
	 */
	DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(
		PagedPool,
		sizeof(DEVICE_RELATIONS));
	if (!DeviceRelations)
		return STATUS_INSUFFICIENT_RESOURCES;
	
	/* Fill returned structure */
	DeviceRelations->Count = 1;
	ObReferenceObject(DeviceExtension->RootHubPdo);
	DeviceRelations->Objects[0] = DeviceExtension->RootHubPdo;
	
	*pDeviceRelations = DeviceRelations;
	return Status;
}

NTSTATUS STDCALL
UhciPnpFdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp;
	NTSTATUS Status;
	ULONG MinorFunction;
	ULONG_PTR Information = 0;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	MinorFunction = IrpSp->MinorFunction;

	switch (MinorFunction)
	{
		case IRP_MN_START_DEVICE:
		{
			Status = ForwardIrpAndWait(DeviceObject, Irp);
			if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
				Status = UhciFdoStartDevice(DeviceObject, Irp);
			break;
		}

		case IRP_MN_REMOVE_DEVICE:
		case IRP_MN_QUERY_REMOVE_DEVICE:
		case IRP_MN_CANCEL_REMOVE_DEVICE:
		case IRP_MN_SURPRISE_REMOVAL:

		case IRP_MN_STOP_DEVICE:
		{
			Status = ForwardIrpAndWait(DeviceObject, Irp);
			if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
				Status = STATUS_SUCCESS;
			IoDeleteDevice(DeviceObject); // just delete device for now
			break;
		}
		case IRP_MN_QUERY_STOP_DEVICE:
		case IRP_MN_CANCEL_STOP_DEVICE:
		{
			Status = STATUS_SUCCESS;
			break;
		}
		case IRP_MN_QUERY_DEVICE_RELATIONS: /* (optional) 0x7 */
		{
			switch (IrpSp->Parameters.QueryDeviceRelations.Type)
			{
				case BusRelations:
				{
					PDEVICE_RELATIONS DeviceRelations;
					DPRINT("UHCI: IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");
					Status = UhciFdoQueryBusRelations(DeviceObject, &DeviceRelations);
					Information = (ULONG_PTR)DeviceRelations;
					break;
				}
				case RemovalRelations:
				{
					DPRINT1("UHCI: IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
					return ForwardIrpAndForget(DeviceObject, Irp);
				}
				default:
					DPRINT1("UHCI: IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
						IrpSp->Parameters.QueryDeviceRelations.Type);
					return ForwardIrpAndForget(DeviceObject, Irp);
			}
			break;
		}

		default:
		{
			DPRINT1("UHCI: unknown minor function 0x%lx\n", MinorFunction);
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
	}
	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

NTSTATUS
UhciDeviceControlFdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	ULONG IoControlCode;
	POHCI_DEVICE_EXTENSION DeviceExtension;
	ULONG LengthIn, LengthOut;
	ULONG_PTR Information = 0;
	PVOID BufferIn, BufferOut;
	NTSTATUS Status;

	DPRINT("UHCI: UsbDeviceControlFdo() called\n");

	Stack = IoGetCurrentIrpStackLocation(Irp);
	LengthIn = Stack->Parameters.DeviceIoControl.InputBufferLength;
	LengthOut = Stack->Parameters.DeviceIoControl.OutputBufferLength;
	DeviceExtension = (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	IoControlCode = Stack->Parameters.DeviceIoControl.IoControlCode;
	UhciGetUserBuffers(Irp, IoControlCode, &BufferIn, &BufferOut);

	switch (IoControlCode)
	{
		case IOCTL_GET_HCD_DRIVERKEY_NAME:
		{
			DPRINT("UHCI: IOCTL_GET_HCD_DRIVERKEY_NAME\n");
			if (LengthOut < sizeof(USB_HCD_DRIVERKEY_NAME))
				Status = STATUS_BUFFER_TOO_SMALL;
			else if (BufferOut == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				PUSB_HCD_DRIVERKEY_NAME StringDescriptor;
				ULONG StringSize;
				StringDescriptor = (PUSB_HCD_DRIVERKEY_NAME)BufferOut;
				Status = IoGetDeviceProperty(
					((POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->PhysicalDeviceObject,
					DevicePropertyDriverKeyName,
					LengthOut - FIELD_OFFSET(USB_HCD_DRIVERKEY_NAME, DriverKeyName),
					StringDescriptor->DriverKeyName,
					&StringSize);
				if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_TOO_SMALL)
				{
					DPRINT("UHCI: IOCTL_GET_HCD_DRIVERKEY_NAME returns '%S'\n", StringDescriptor->DriverKeyName);
					StringDescriptor->ActualLength = StringSize + FIELD_OFFSET(USB_HCD_DRIVERKEY_NAME, DriverKeyName);
					Information = LengthOut;
					Status = STATUS_SUCCESS;
				}
			}
			break;
		}
		case IOCTL_USB_GET_ROOT_HUB_NAME:
		{
			DPRINT("UHCI: IOCTL_USB_GET_ROOT_HUB_NAME\n");
			if (LengthOut < sizeof(USB_ROOT_HUB_NAME))
				Status = STATUS_BUFFER_TOO_SMALL;
			else if (BufferOut == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				PUSB_ROOT_HUB_NAME StringDescriptor;
				PUNICODE_STRING RootHubInterfaceName;
				StringDescriptor = (PUSB_ROOT_HUB_NAME)BufferOut;
				DeviceObject = ((POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->RootHubPdo;
				RootHubInterfaceName = &((POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->HcdInterfaceName;

				StringDescriptor->ActualLength = RootHubInterfaceName->Length + sizeof(WCHAR) + FIELD_OFFSET(USB_ROOT_HUB_NAME, RootHubName);
				if (StringDescriptor->ActualLength <= LengthOut)
				{
					/* Copy root hub name */
					RtlCopyMemory(
						StringDescriptor->RootHubName,
						RootHubInterfaceName->Buffer,
						RootHubInterfaceName->Length);
					StringDescriptor->RootHubName[RootHubInterfaceName->Length / sizeof(WCHAR)] = UNICODE_NULL;
					DPRINT("UHCI: IOCTL_USB_GET_ROOT_HUB_NAME returns '%S'\n", StringDescriptor->RootHubName);
					Information = StringDescriptor->ActualLength;
				}
				else
					Information = sizeof(USB_ROOT_HUB_NAME);
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_USB_GET_NODE_CONNECTION_INFORMATION:
		{
			DPRINT1("UHCI: IOCTL_USB_GET_NODE_CONNECTION_INFORMATION\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_USB_GET_NODE_CONNECTION_NAME:
		{
			DPRINT1("UHCI: IOCTL_USB_GET_NODE_CONNECTION_NAME\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}

		default:
		{
			/* Pass Irp to lower driver */
			DPRINT1("UHCI: Unknown IOCTL code 0x%lx\n", Stack->Parameters.DeviceIoControl.IoControlCode);
			IoSkipCurrentIrpStackLocation(Irp);
			return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
