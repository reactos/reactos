/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS USB miniport driver (Cromwell type)
 * FILE:            drivers/usb/miniport/common/fdo.c
 * PURPOSE:         IRP_MJ_PNP/IRP_MJ_DEVICE_CONTROL operations for FDOs
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org),
 *                  James Tabor (jimtabor@adsl-64-217-116-74.dsl.hstntx.swbell.net)
 */

#define NDEBUG
#include <debug.h>

#include "usbcommon.h"

#define IO_METHOD_FROM_CTL_CODE(ctlCode) (ctlCode&0x00000003)

static VOID
UsbMpGetUserBuffers(
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
UsbMpFdoStartDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP	Irp)
{
	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
	PDRIVER_OBJECT DriverObject;
	PUSBMP_DRIVER_EXTENSION DriverExtension;
	PUSBMP_DEVICE_EXTENSION DeviceExtension;
	PCM_RESOURCE_LIST AllocatedResources;
	ULONG Size;
	NTSTATUS Status;

	if (DeviceObject == KeyboardFdo || DeviceObject == MouseFdo)
		return STATUS_SUCCESS;

	/*
	* Get the initialization data we saved in VideoPortInitialize.
	*/
	DriverObject = DeviceObject->DriverObject;
	DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
	DeviceExtension = (PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

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

					((struct hc_driver *)pci_ids->driver_data)->flags &= ~HCD_MEMORY;
				}
				else if (Descriptor->Type == CmResourceTypeMemory)
				{
					DeviceExtension->BaseAddress    = Descriptor->u.Memory.Start;
					DeviceExtension->BaseAddrLength = Descriptor->u.Memory.Length;
					DeviceExtension->Flags          = Descriptor->Flags;

					((struct hc_driver *)pci_ids->driver_data)->flags |= HCD_MEMORY;
				}
			}
		}
	}
	
	/* Print assigned resources */
	DPRINT("USBMP: Interrupt Vector 0x%lx, %S base 0x%lx, Length 0x%lx\n",
		DeviceExtension->InterruptVector,
		((struct hc_driver *)pci_ids->driver_data)->flags & HCD_MEMORY ? L"Memory" : L"I/O",
		DeviceExtension->BaseAddress,
		DeviceExtension->BaseAddrLength);

	/* Get bus number from the upper level bus driver. */
	Size = sizeof(ULONG);
	Status = IoGetDeviceProperty(
		DeviceExtension->PhysicalDeviceObject,
		DevicePropertyBusNumber,
		Size,
		&DeviceExtension->SystemIoBusNumber,
		&Size);

	if (!NT_SUCCESS(Status))
	{
		DPRINT1("USBMP: IoGetDeviceProperty DevicePropertyBusNumber failed\n");
		DeviceExtension->SystemIoBusNumber = 0;
	}

	DPRINT("USBMP: Busnumber %d\n", DeviceExtension->SystemIoBusNumber);

	/* Init wrapper with this object */
	return InitLinuxWrapper(DeviceObject);
}

static NTSTATUS
UsbMpFdoQueryBusRelations(
	IN PDEVICE_OBJECT DeviceObject,
	OUT PDEVICE_RELATIONS* pDeviceRelations)
{
	PUSBMP_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_RELATIONS DeviceRelations;
	NTSTATUS Status = STATUS_SUCCESS;
	
	DeviceExtension = (PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
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
UsbMpPnpFdo(
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
		case IRP_MN_START_DEVICE: /* 0x00 */
		{
			if (((PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->NextDeviceObject != NULL)
				/* HACK due to the lack of lower device for legacy USB keyboard and mouse */
				Status = ForwardIrpAndWait(DeviceObject, Irp);
			else
				Status = STATUS_SUCCESS;
			if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
				Status = UsbMpFdoStartDevice(DeviceObject, Irp);
			break;
		}

		case IRP_MN_QUERY_REMOVE_DEVICE: /* 0x01 */
		case IRP_MN_CANCEL_REMOVE_DEVICE: /* 0x03 */
		{
			return ForwardIrpAndForget(DeviceObject, Irp);
		}

		case IRP_MN_REMOVE_DEVICE: /* 0x02 */
		case IRP_MN_STOP_DEVICE: /* 0x04 */
		case IRP_MN_SURPRISE_REMOVAL: /* 0x17 */
		{
			if (((PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->NextDeviceObject != NULL)
				/* HACK due to the lack of lower device for legacy USB keyboard and mouse */
				Status = ForwardIrpAndWait(DeviceObject, Irp);
			else
				Status = STATUS_SUCCESS;
			if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
				Status = STATUS_SUCCESS;
			IoDeleteDevice(DeviceObject); // just delete device for now
			break;
		}

		case IRP_MN_QUERY_STOP_DEVICE: /* 0x05 */
		case IRP_MN_CANCEL_STOP_DEVICE: /* 0x06 */
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
					PDEVICE_RELATIONS DeviceRelations = NULL;
					DPRINT("USBMP: IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");
					Status = UsbMpFdoQueryBusRelations(DeviceObject, &DeviceRelations);
					Information = (ULONG_PTR)DeviceRelations;
					break;
				}
				case RemovalRelations:
				{
					DPRINT1("USBMP: IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
					return ForwardIrpAndForget(DeviceObject, Irp);
				}
				default:
					DPRINT1("USBMP: IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
						IrpSp->Parameters.QueryDeviceRelations.Type);
					return ForwardIrpAndForget(DeviceObject, Irp);
			}
			break;
		}

		default:
		{
			DPRINT1("USBMP: IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
	}
	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

NTSTATUS
UsbMpDeviceControlFdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	ULONG IoControlCode;
	PUSBMP_DEVICE_EXTENSION DeviceExtension;
	ULONG LengthIn, LengthOut;
	ULONG_PTR Information = 0;
	PVOID BufferIn, BufferOut;
	NTSTATUS Status;

	DPRINT("USBMP: UsbDeviceControlFdo() called\n");

	Stack = IoGetCurrentIrpStackLocation(Irp);
	LengthIn = Stack->Parameters.DeviceIoControl.InputBufferLength;
	LengthOut = Stack->Parameters.DeviceIoControl.OutputBufferLength;
	DeviceExtension = (PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	IoControlCode = Stack->Parameters.DeviceIoControl.IoControlCode;
	UsbMpGetUserBuffers(Irp, IoControlCode, &BufferIn, &BufferOut);

	switch (IoControlCode)
	{
		case IOCTL_GET_HCD_DRIVERKEY_NAME:
		{
			DPRINT("USBMP: IOCTL_GET_HCD_DRIVERKEY_NAME\n");
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
					((PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->PhysicalDeviceObject,
					DevicePropertyDriverKeyName,
					LengthOut - FIELD_OFFSET(USB_HCD_DRIVERKEY_NAME, DriverKeyName),
					StringDescriptor->DriverKeyName,
					&StringSize);
				if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_TOO_SMALL)
				{
					StringDescriptor->ActualLength = StringSize + FIELD_OFFSET(USB_HCD_DRIVERKEY_NAME, DriverKeyName);
					Information = LengthOut;
					Status = STATUS_SUCCESS;
				}
			}
			break;
		}
		case IOCTL_USB_GET_ROOT_HUB_NAME:
		{
			DPRINT("USBMP: IOCTL_USB_GET_ROOT_HUB_NAME\n");
			if (LengthOut < sizeof(USB_ROOT_HUB_NAME))
				Status = STATUS_BUFFER_TOO_SMALL;
			else if (BufferOut == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				PUSB_ROOT_HUB_NAME StringDescriptor;
				PUNICODE_STRING RootHubInterfaceName;
				StringDescriptor = (PUSB_ROOT_HUB_NAME)BufferOut;
				DeviceObject = ((PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->RootHubPdo;
				RootHubInterfaceName = &((PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->HcdInterfaceName;

				StringDescriptor->ActualLength = RootHubInterfaceName->Length + sizeof(WCHAR) + FIELD_OFFSET(USB_ROOT_HUB_NAME, RootHubName);
				if (StringDescriptor->ActualLength <= LengthOut)
				{
					/* Copy root hub name */
					RtlCopyMemory(
						StringDescriptor->RootHubName,
						RootHubInterfaceName->Buffer,
						RootHubInterfaceName->Length);
					StringDescriptor->RootHubName[RootHubInterfaceName->Length / sizeof(WCHAR)] = UNICODE_NULL;
					DPRINT("USBMP: IOCTL_USB_GET_ROOT_HUB_NAME returns '%S'\n", StringDescriptor->RootHubName);
					Information = StringDescriptor->ActualLength;
				}
				else
					Information = sizeof(USB_ROOT_HUB_NAME);
				Status = STATUS_SUCCESS;
			}
			break;
		}

		default:
		{
			/* Pass Irp to lower driver */
			DPRINT1("USBMP: Unknown IOCTL code 0x%lx\n", Stack->Parameters.DeviceIoControl.IoControlCode);
			IoSkipCurrentIrpStackLocation(Irp);
			return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

NTSTATUS
UsbMpInternalDeviceControlFdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

	DPRINT("USBMP: UsbMpDeviceInternalControlFdo(DO %p, code 0x%lx) called\n",
		DeviceObject,
		IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode);

	if (DeviceObject == KeyboardFdo)
	{
		// it's keyboard's IOCTL
		PIO_STACK_LOCATION Stk;

		Irp->IoStatus.Information = 0;
		Stk = IoGetCurrentIrpStackLocation(Irp);

		switch (Stk->Parameters.DeviceIoControl.IoControlCode)
		{
			case IOCTL_INTERNAL_KEYBOARD_CONNECT:
				DPRINT("USBMP: IOCTL_INTERNAL_KEYBOARD_CONNECT\n");
				if (Stk->Parameters.DeviceIoControl.InputBufferLength <	sizeof(CONNECT_DATA)) {
					DPRINT1("USBMP: Keyboard IOCTL_INTERNAL_KEYBOARD_CONNECT "
							"invalid buffer size\n");
					Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
					goto intcontfailure;
				}

				RtlCopyMemory(&KbdClassInformation,
					Stk->Parameters.DeviceIoControl.Type3InputBuffer,
					sizeof(CONNECT_DATA));
	
				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;

		case IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER:
			DPRINT("USBMP: IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER\n");
			if (Stk->Parameters.DeviceIoControl.InputBufferLength <	1) {
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
				goto intcontfailure;
			}
/*			if (!DevExt->KeyboardInterruptObject) {
				Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
				goto intcontfailure;
			}*/

			Irp->IoStatus.Status = STATUS_SUCCESS;
			break;
		case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
			DPRINT("USBMP: IOCTL_KEYBOARD_QUERY_ATTRIBUTES\n");
			if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(KEYBOARD_ATTRIBUTES)) {
					DPRINT("USBMP: Keyboard IOCTL_KEYBOARD_QUERY_ATTRIBUTES "
						"invalid buffer size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}
				/*RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
					&DevExt->KeyboardAttributes,
					sizeof(KEYBOARD_ATTRIBUTES));*/

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;
		case IOCTL_KEYBOARD_QUERY_INDICATORS:
			DPRINT("USBMP: IOCTL_KEYBOARD_QUERY_INDICATORS\n");
			if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {
					DPRINT("USBMP: Keyboard IOCTL_KEYBOARD_QUERY_INDICATORS "
						"invalid buffer size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}
				/*RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
					&DevExt->KeyboardIndicators,
					sizeof(KEYBOARD_INDICATOR_PARAMETERS));*/

				Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
				break;
		case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
			DPRINT("USBMP: IOCTL_KEYBOARD_QUERY_TYPEMATIC\n");
			if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {
					DPRINT("USBMP: Keyboard IOCTL_KEYBOARD_QUERY_TYPEMATIC	"
						"invalid buffer size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}
				/*RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
					&DevExt->KeyboardTypematic,
					sizeof(KEYBOARD_TYPEMATIC_PARAMETERS));*/

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;
		case IOCTL_KEYBOARD_SET_INDICATORS:
			DPRINT("USBMP: IOCTL_KEYBOARD_SET_INDICATORS\n");
			if (Stk->Parameters.DeviceIoControl.InputBufferLength <
				sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {
					DPRINT("USBMP: Keyboard IOCTL_KEYBOARD_SET_INDICTATORS	"
						"invalid buffer size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}

				/*RtlCopyMemory(&DevExt->KeyboardIndicators,
					Irp->AssociatedIrp.SystemBuffer,
					sizeof(KEYBOARD_INDICATOR_PARAMETERS));*/

				//DPRINT("%x\n", DevExt->KeyboardIndicators.LedFlags);

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;
		case IOCTL_KEYBOARD_SET_TYPEMATIC:
			DPRINT("USBMP: IOCTL_KEYBOARD_SET_TYPEMATIC\n");
			if (Stk->Parameters.DeviceIoControl.InputBufferLength <
				sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {
					DPRINT("USBMP: Keyboard IOCTL_KEYBOARD_SET_TYPEMATIC "
						"invalid buffer size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}

				/*RtlCopyMemory(&DevExt->KeyboardTypematic,
					Irp->AssociatedIrp.SystemBuffer,
					sizeof(KEYBOARD_TYPEMATIC_PARAMETERS));*/

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;
		case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
			/* We should check the UnitID, but it's	kind of	pointless as
			* all keyboards are supposed to have the same one
			*/
#if 0
			DPRINT("USBMP: IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION\n");
			if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION)) {
					DPRINT("USBMP: IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:	"
						"invalid buffer size (expected)\n");
					/* It's to query the buffer size */
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}
				Irp->IoStatus.Information =
					sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION);
#endif
				/*RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
					&IndicatorTranslation,
					sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION));*/

				Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
				break;
		case IOCTL_INTERNAL_I8042_HOOK_KEYBOARD:
			/* Nothing to do here */
			Irp->IoStatus.Status = STATUS_SUCCESS;
			break;
		default:
			Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}

	intcontfailure:
			Status = Irp->IoStatus.Status;
	}
	else if (DeviceObject == MouseFdo)
	{
		// it's mouse's IOCTL
		PIO_STACK_LOCATION Stk;

		Irp->IoStatus.Information = 0;
		Stk = IoGetCurrentIrpStackLocation(Irp);

		switch (Stk->Parameters.DeviceIoControl.IoControlCode)
		{
			case IOCTL_INTERNAL_MOUSE_CONNECT:
				DPRINT("USBMP: IOCTL_INTERNAL_MOUSE_CONNECT\n");
				if (Stk->Parameters.DeviceIoControl.InputBufferLength <	sizeof(CONNECT_DATA)) {
					DPRINT1("USBMP: IOCTL_INTERNAL_MOUSE_CONNECT "
							"invalid buffer size\n");
					Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
					goto intcontfailure2;
				}

				RtlCopyMemory(&MouseClassInformation,
					Stk->Parameters.DeviceIoControl.Type3InputBuffer,
					sizeof(CONNECT_DATA));
	
				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;

		default:
			Irp->IoStatus.Status = STATUS_SUCCESS;//STATUS_INVALID_DEVICE_REQUEST;
			break;
		}
	intcontfailure2:
		Status = Irp->IoStatus.Status;
	}
	else
	{
		DPRINT("USBMP: We got IOCTL for UsbCore\n");
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}


	if (Status == STATUS_INVALID_DEVICE_REQUEST) {
		DPRINT1("USBMP: Invalid internal device request!\n");
	}

	if (Status != STATUS_PENDING)
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}
