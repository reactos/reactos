/* $Id:
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/dd/serial/pnp.c
 * PURPOSE:         Serial IRP_MJ_PNP operations
 * 
 * PROGRAMMERS:     Hervé Poussineau (poussine@freesurf.fr)
 */
/* FIXME: call IoAcquireRemoveLock/IoReleaseRemoveLock around each I/O operation */

#define NDEBUG
#include "serial.h"

NTSTATUS STDCALL
SerialAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	PDEVICE_OBJECT Fdo;
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;
	WCHAR DeviceNameBuffer[32];
	UNICODE_STRING DeviceName;
	static ULONG DeviceNumber = 0;

	DPRINT("Serial: SerialAddDevice called\n");
   
	/* Create new device object */
	swprintf(DeviceNameBuffer, L"\\Device\\Serial%lu", DeviceNumber);
	RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);
	Status = IoCreateDevice(DriverObject,
	                        sizeof(SERIAL_DEVICE_EXTENSION),
	                        &DeviceName,
	                        FILE_DEVICE_SERIAL_PORT,
	                        FILE_DEVICE_SECURE_OPEN,
	                        FALSE,
	                        &Fdo);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Serial: IoCreateDevice() failed with status 0x%08x\n", Status);
		return Status;
	}
	
	/* Register device interface */
	/* FIXME */
	
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)Fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(SERIAL_DEVICE_EXTENSION));
	DeviceExtension->SerialPortNumber = DeviceNumber++;
	DeviceExtension->Pdo = Pdo;
	DeviceExtension->PnpState = dsStopped;
	IoInitializeRemoveLock(&DeviceExtension->RemoveLock, SERIAL_TAG, 0, 0);
	//Fdo->Flags |= DO_POWER_PAGEABLE (or DO_POWER_INRUSH?)
	Status = IoAttachDeviceToDeviceStackSafe(Fdo, Pdo, &DeviceExtension->LowerDevice);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Serial: IoAttachDeviceToDeviceStackSafe() failed with status 0x%08x\n", Status);
		IoDeleteDevice(Fdo);
		return Status;
	}
	Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}

NTSTATUS STDCALL
SerialPnpStartDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PCM_RESOURCE_LIST ResourceList;
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	WCHAR DeviceNameBuffer[32];
	UNICODE_STRING DeviceName;
	WCHAR LinkNameBuffer[32];
	UNICODE_STRING LinkName;
	ULONG Vector;
	KIRQL Dirql;
	KAFFINITY Affinity;
	NTSTATUS Status;
	
	Stack = IoGetCurrentIrpStackLocation(Irp);
	
	ResourceList = Stack->Parameters.StartDevice.AllocatedResources/*Translated*/;
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
	/* FIXME: actually, IRP_MN_START_DEVICE is sent twice to each serial device:
	 * - one when loading serial.sys
	 * - one when loading attached upper filter serenum.sys
	 * This behaviour MUST NOT exist.
	 * As PnP handling isn't right anyway, I didn't search how to correct this.
	 */
	if (DeviceExtension->PnpState == dsStarted) return STATUS_SUCCESS;
	
	if (ResourceList == NULL)
	{
		/* FIXME: PnP isn't correctly implemented and doesn't give us a list
		 * of our own resources. Use default values instead.
		 */
		switch (DeviceExtension->SerialPortNumber)
		{
			case 0:
				DPRINT("Serial: creating COM1:\n");
				DeviceExtension->ComPort = 3;
				DeviceExtension->BaudRate = 19200 | SERIAL_BAUD_USER;
				DeviceExtension->BaseAddress = 0x3F8;
				DeviceExtension->Irq = 4;
				break;
			case 1:
				DPRINT("Serial: creating COM2:\n");
				DeviceExtension->ComPort = 4;
				DeviceExtension->BaudRate = 19200 | SERIAL_BAUD_USER;
				DeviceExtension->BaseAddress = 0x2F8;
				DeviceExtension->Irq = 3;
				break;
			default:
				DPRINT1("Serial: unknown port?\n");
				return STATUS_INSUFFICIENT_RESOURCES;
		}
	}
#if 0
	else
	{
		DPRINT1("ResourceList %p, ResourceListTranslated %p\n", Stack->Parameters.StartDevice.AllocatedResources, Stack->Parameters.StartDevice.AllocatedResourcesTranslated);
		for (i = 0; i < ResourceList->Count; i++)
		{
			DPRINT1("Interface type = 0x%x\n", ResourceList->List[i].InterfaceType);
			DPRINT1("Bus number = 0x%x\n", ResourceList->List[i].BusNumber);
			for (j = 0; i < ResourceList->List[i].PartialResourceList.Count; j++)
			{
				PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor = &ResourceList->List[i].PartialResourceList.PartialDescriptors[j];
				DPRINT1("Type 0x%x, Share disposition 0x%x, Flags 0x%x\n",
					PartialDescriptor->Type,
					PartialDescriptor->ShareDisposition,
					PartialDescriptor->Flags);
				switch (PartialDescriptor->Type)
				{
					case CmResourceTypePort:
						DeviceExtension->BaseAddress = PartialDescriptor->u.Port.Start.u.LowPart;
						break;
					case CmResourceTypeInterrupt:
						/* FIXME: Detect if interrupt is shareable and/or latched */
						/* FIXME: use also ->u.Interrupt.Vector and ->u.Interrupt.Affinity
						 * to remove call to HalGetInterruptVector(...) */
						DeviceExtension->Irq = PartialDescriptor->u.Interrupt.Level;
						break;
				}
			}
		}
		/* FIXME: use polling if no interrupt was found? */
		DeviceExtension->ComPort = 5; /* FIXME: use incremental value, or find it in resource list*/
	}
#endif
	
	/* Get current settings */
	DeviceExtension->IER = READ_PORT_UCHAR(SER_IER((PUCHAR)DeviceExtension->BaseAddress));
	DeviceExtension->MCR = READ_PORT_UCHAR(SER_MCR((PUCHAR)DeviceExtension->BaseAddress));
	DeviceExtension->MSR = READ_PORT_UCHAR(SER_MSR((PUCHAR)DeviceExtension->BaseAddress));
	DeviceExtension->SerialLineControl.StopBits = STOP_BIT_1;
	DeviceExtension->SerialLineControl.Parity = NO_PARITY;
	DeviceExtension->SerialLineControl.WordLength = 8;
	DeviceExtension->WaitMask = 0;
	
	/* Set baud rate */
	Status = SerialSetBaudRate(DeviceExtension, 19200 | SERIAL_BAUD_USER); /* FIXME: real default value? */
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Serial: SerialSetBaudRate() failed with status 0x%08x\n", Status);
		return Status;
	}
	
	/* Set line control */
	Status = SerialSetLineControl(DeviceExtension, &DeviceExtension->SerialLineControl);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Serial: SerialSetLineControl() failed with status 0x%08x\n", Status);
		return Status;
	}
	
	/* Create link \DosDevices\COMX -> \Device\SerialX */
	swprintf(DeviceNameBuffer, L"\\Device\\Serial%lu", DeviceExtension->SerialPortNumber);
	RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);
	swprintf(LinkNameBuffer, L"\\DosDevices\\COM%lu", DeviceExtension->ComPort);
	RtlInitUnicodeString(&LinkName, LinkNameBuffer);
	Status = IoCreateSymbolicLink(&LinkName, &DeviceName);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Serial: IoCreateSymbolicLink() failed with status 0x%08x\n", Status);
		return Status;
	}
	
	/* Connect interrupt and enable them */
	Vector = HalGetInterruptVector(Internal, 0, 0, DeviceExtension->Irq, &Dirql, &Affinity);
	Status = IoConnectInterrupt(
		&DeviceExtension->Interrupt, SerialInterruptService,
		DeviceObject, NULL, Vector, Dirql, Dirql, Latched,
		FALSE /* FIXME: or TRUE to share interrupt on PCI bus? */,
		Affinity, FALSE);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Serial: IoConnectInterrupt() failed with status 0x%08x\n", Status);
		IoDeleteSymbolicLink(&LinkName);
		return Status;
	}
	DeviceExtension->IER |= 0x1; /* Activate interrupt mode */
	WRITE_PORT_UCHAR(SER_IER((PUCHAR)DeviceExtension->BaseAddress), DeviceExtension->IER);
	
	DeviceExtension->PnpState = dsStarted;
	
	return Status;
}

NTSTATUS STDCALL
SerialPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	ULONG MinorFunction;
	PIO_STACK_LOCATION Stack;
	PDEVICE_OBJECT LowerDevice;
	NTSTATUS Status;
	
	Stack = IoGetCurrentIrpStackLocation(Irp);
	MinorFunction = Stack->MinorFunction;
	LowerDevice = ((PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
	
	switch (MinorFunction)
	{
		case IRP_MN_START_DEVICE:
		{
			DPRINT("Serial: IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
			
			/* Call lower driver */
			Status = ForwardIrpAndWait(DeviceObject, Irp);
			if (NT_SUCCESS(Status))
				Status = SerialPnpStartDevice(DeviceObject, Irp);
			Irp->IoStatus.Status = Status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
		}
		/* IRP_MN_QUERY_STOP_DEVICE (FIXME: required) */
		/* IRP_MN_STOP_DEVICE (FIXME: required) */
		/* IRP_MN_CANCEL_STOP_DEVICE (FIXME: required) */
		/* IRP_MN_QUERY_REMOVE_DEVICE (FIXME: required) */
		/* case IRP_MN_REMOVE_DEVICE (FIXME: required) */
		/*{
			DPRINT("Serial: IRP_MJ_PNP / IRP_MN_REMOVE_DEVICE\n");
			IoAcquireRemoveLock
			IoReleaseRemoveLockAndWait
			pass request to DeviceExtension-LowerDriver
			IoDeleteDevice(Fdo) and/or IoDetachDevice
			break;
		}*/
		/* IRP_MN_CANCEL_REMOVE_DEVICE (FIXME: required) */
		/* IRP_MN_SURPRISE_REMOVAL (FIXME: required) */
		/* IRP_MN_QUERY_CAPABILITIES (optional) */
		/* IRP_MN_QUERY_PNP_DEVICE_STATE (optional) */
		/* IRP_MN_FILTER_RESOURCE_REQUIREMENTS (optional) */
		/* IRP_MN_DEVICE_USAGE_NOTIFICATION (FIXME: required or optional ???) */
		/* IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations (optional) */
		/* IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations (optional) */
		/* IRP_MN_QUERY_INTERFACE (optional) */
		default:
		{
			DPRINT1("Serial: unknown minor function 0x%x\n", MinorFunction);
			IoSkipCurrentIrpStackLocation(Irp);
			Status = IoCallDriver(LowerDevice, Irp);
			break;
		}
	}

	return Status;
}
