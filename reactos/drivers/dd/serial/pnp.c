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

#define INITGUID
#define NDEBUG
#include "serial.h"

NTSTATUS STDCALL
SerialAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	PDEVICE_OBJECT Fdo = NULL;
	PSERIAL_DEVICE_EXTENSION DeviceExtension = NULL;
	NTSTATUS Status;
	WCHAR DeviceNameBuffer[32];
	UNICODE_STRING DeviceName;
	//UNICODE_STRING SymbolicLinkName;
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
		Fdo = NULL;
		goto ByeBye;
	}
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)Fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(SERIAL_DEVICE_EXTENSION));
	
	/* Register device interface */
#if 0 /* FIXME: activate */
	Status = IoRegisterDeviceInterface(Pdo, &GUID_DEVINTERFACE_COMPORT, NULL, &SymbolicLinkName);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Serial: IoRegisterDeviceInterface() failed with status 0x%08x\n", Status);
		goto ByeBye;
	}
	DPRINT1("Serial: IoRegisterDeviceInterface() returned '%wZ'\n", &SymbolicLinkName);
	Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Serial: IoSetDeviceInterfaceState() failed with status 0x%08x\n", Status);
		goto ByeBye;
	}
	RtlFreeUnicodeString(&SymbolicLinkName);
#endif

	DeviceExtension->SerialPortNumber = DeviceNumber++;
	DeviceExtension->Pdo = Pdo;
	DeviceExtension->PnpState = dsStopped;
	Status = InitializeCircularBuffer(&DeviceExtension->InputBuffer, 16);
	if (!NT_SUCCESS(Status)) goto ByeBye;
	Status = InitializeCircularBuffer(&DeviceExtension->OutputBuffer, 16);
	if (!NT_SUCCESS(Status)) goto ByeBye;
	IoInitializeRemoveLock(&DeviceExtension->RemoveLock, SERIAL_TAG, 0, 0);
	//Fdo->Flags |= DO_POWER_PAGEABLE (or DO_POWER_INRUSH?)
	Status = IoAttachDeviceToDeviceStackSafe(Fdo, Pdo, &DeviceExtension->LowerDevice);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Serial: IoAttachDeviceToDeviceStackSafe() failed with status 0x%08x\n", Status);
		goto ByeBye;
	}
	Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
	
	return STATUS_SUCCESS;

ByeBye:
	if (Fdo)
	{
		FreeCircularBuffer(&DeviceExtension->InputBuffer);
		FreeCircularBuffer(&DeviceExtension->OutputBuffer);
		IoDeleteDevice(Fdo);
	}
	return Status;
}

NTSTATUS STDCALL
SerialPnpStartDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	//PCM_RESOURCE_LIST ResourceList;
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	WCHAR DeviceNameBuffer[32];
	UNICODE_STRING DeviceName;
	WCHAR LinkNameBuffer[32];
	UNICODE_STRING LinkName;
	WCHAR ComPortBuffer[32];
	UNICODE_STRING ComPort;
	ULONG Vector;
	//ULONG i, j;
	KIRQL Dirql;
	KAFFINITY Affinity;
	OBJECT_ATTRIBUTES objectAttributes;
	UNICODE_STRING KeyName;
	HANDLE hKey;
	NTSTATUS Status;
	
	Stack = IoGetCurrentIrpStackLocation(Irp);
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
	/* FIXME: actually, IRP_MN_START_DEVICE is sent twice to each serial device:
	 * - one when loading serial.sys
	 * - one when loading attached upper filter serenum.sys
	 * This behaviour MUST NOT exist.
	 * As PnP handling isn't right anyway, I didn't search how to correct this.
	 */
	if (DeviceExtension->PnpState == dsStarted) return STATUS_SUCCESS;
	
#if 1
	/* FIXME: PnP isn't correctly implemented and doesn't give us a list
	 * of our own resources. Use default values instead.
	 */
	switch (DeviceExtension->SerialPortNumber)
	{
		case 0:
			DPRINT("Serial: creating COM1:\n");
			DeviceExtension->ComPort = 1;
			DeviceExtension->BaudRate = 19200 | SERIAL_BAUD_USER;
			DeviceExtension->BaseAddress = 0x3F8;
			DeviceExtension->Irq = 4;
			break;
		case 1:
			DPRINT("Serial: creating COM2:\n");
			DeviceExtension->ComPort = 2;
			DeviceExtension->BaudRate = 19200 | SERIAL_BAUD_USER;
			DeviceExtension->BaseAddress = 0x2F8;
			DeviceExtension->Irq = 3;
			break;
		default:
			DPRINT1("Serial: too much ports detected. Forgetting this one...\n");
			return STATUS_INSUFFICIENT_RESOURCES;
	}
#else
	DPRINT1("Serial: ResourceList %p, ResourceListTranslated %p\n",
		Stack->Parameters.StartDevice.AllocatedResources,
		Stack->Parameters.StartDevice.AllocatedResourcesTranslated);
	ResourceList = Stack->Parameters.StartDevice.AllocatedResourcesTranslated;
	for (i = 0; i < ResourceList->Count; i++)
	{
		DPRINT1("Serial: Interface type = 0x%x\n", ResourceList->List[i].InterfaceType);
		DPRINT1("Serial: Bus number = 0x%x\n", ResourceList->List[i].BusNumber);
		for (j = 0; i < ResourceList->List[i].PartialResourceList.Count; j++)
		{
			PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor = &ResourceList->List[i].PartialResourceList.PartialDescriptors[j];
			DPRINT1("Serial: Type 0x%x, Share disposition 0x%x, Flags 0x%x\n",
				PartialDescriptor->Type,
				PartialDescriptor->ShareDisposition,
				PartialDescriptor->Flags);
			switch (PartialDescriptor->Type)
			{
				case CmResourceTypePort:
					DeviceExtension->BaseAddress = PartialDescriptor->u.Port.Start.u.LowPart;
					DPRINT1("Serial: CmResourceTypePort = %lu\n", DeviceExtension->BaseAddress);
					break;
				case CmResourceTypeInterrupt:
					/* FIXME: Detect if interrupt is shareable and/or latched */
					/* FIXME: use also ->u.Interrupt.Vector and ->u.Interrupt.Affinity
					 * to remove call to HalGetInterruptVector(...) */
					DeviceExtension->Irq = PartialDescriptor->u.Interrupt.Level;
					DPRINT1("Serial: Irq = %lu\n", DeviceExtension->Irq);
					break;
			}
		}
	}
	DeviceExtension->BaudRate = 19200 | SERIAL_BAUD_USER;
	/* FIXME: use polling if no interrupt was found? */
	DeviceExtension->ComPort = 5; /* FIXME: use incremental value, or find it in resource list */
#endif
	
	/* Get current settings */
	DeviceExtension->IER = READ_PORT_UCHAR(SER_IER((PUCHAR)DeviceExtension->BaseAddress));
	DeviceExtension->MCR = READ_PORT_UCHAR(SER_MCR((PUCHAR)DeviceExtension->BaseAddress));
	DeviceExtension->MSR = READ_PORT_UCHAR(SER_MSR((PUCHAR)DeviceExtension->BaseAddress));
	DeviceExtension->WaitMask = 0;
	
	/* Set baud rate */
	Status = SerialSetBaudRate(DeviceExtension, DeviceExtension->BaudRate);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Serial: SerialSetBaudRate() failed with status 0x%08x\n", Status);
		return Status;
	}
	
	/* Set line control */
	DeviceExtension->SerialLineControl.StopBits = STOP_BIT_1;
	DeviceExtension->SerialLineControl.Parity = NO_PARITY;
	DeviceExtension->SerialLineControl.WordLength = 8;
	Status = SerialSetLineControl(DeviceExtension, &DeviceExtension->SerialLineControl);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Serial: SerialSetLineControl() failed with status 0x%08x\n", Status);
		return Status;
	}
	
	/* Create link \DosDevices\COMX -> \Device\SerialX */
	swprintf(DeviceNameBuffer, L"\\Device\\Serial%lu", DeviceExtension->SerialPortNumber);
	swprintf(LinkNameBuffer, L"\\DosDevices\\COM%lu", DeviceExtension->ComPort);
	swprintf(ComPortBuffer, L"COM%lu", DeviceExtension->ComPort);
	RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);
	RtlInitUnicodeString(&LinkName, LinkNameBuffer);
	RtlInitUnicodeString(&ComPort, ComPortBuffer);
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
	
	/* Write an entry value under HKLM\HARDWARE\DeviceMap\SERIALCOMM */
	/* This step is not mandatory, so don't exit in case of error */
	RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\HARDWARE\\DeviceMap\\SERIALCOMM");
	InitializeObjectAttributes(&objectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = ZwCreateKey(&hKey, KEY_SET_VALUE, &objectAttributes, 0, NULL, REG_OPTION_VOLATILE, NULL);
	if (NT_SUCCESS(Status))
	{
		/* Key = \Device\Serialx, Value = COMx */
		ZwSetValueKey(hKey, &DeviceName, 0, REG_SZ, &ComPortBuffer, ComPort.Length + sizeof(WCHAR));
		ZwClose(hKey);
	}
	
	DeviceExtension->PnpState = dsStarted;
	
	DeviceExtension->IER |= 0x1f; /* Activate interrupt mode */
	DeviceExtension->IER &= ~1; /* FIXME: Disable receive byte interrupt */
	WRITE_PORT_UCHAR(SER_IER((PUCHAR)DeviceExtension->BaseAddress), DeviceExtension->IER);
	
	DeviceExtension->MCR |= 0x03; /* Activate DTR, RTS */
	WRITE_PORT_UCHAR(SER_MCR((PUCHAR)DeviceExtension->BaseAddress), DeviceExtension->MCR);
	
	return STATUS_SUCCESS;
}

NTSTATUS STDCALL
SerialPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	ULONG MinorFunction;
	PIO_STACK_LOCATION Stack;
	ULONG Information = 0;
	NTSTATUS Status;
	
	Stack = IoGetCurrentIrpStackLocation(Irp);
	MinorFunction = Stack->MinorFunction;
	
	switch (MinorFunction)
	{
		case IRP_MN_START_DEVICE:
		{
			DPRINT("Serial: IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
			
			/* Call lower driver */
			Status = ForwardIrpAndWait(DeviceObject, Irp);
			if (NT_SUCCESS(Status))
				Status = SerialPnpStartDevice(DeviceObject, Irp);
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
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
	}
	
	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
