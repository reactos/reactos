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
SerialAddDeviceInternal(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo,
	IN UART_TYPE UartType,
	OUT PDEVICE_OBJECT* pFdo OPTIONAL)
{
	PDEVICE_OBJECT Fdo = NULL;
	PSERIAL_DEVICE_EXTENSION DeviceExtension = NULL;
	NTSTATUS Status;
	WCHAR DeviceNameBuffer[32];
	UNICODE_STRING DeviceName;
	//UNICODE_STRING SymbolicLinkName;
	static ULONG DeviceNumber = 0;

	DPRINT("Serial: SerialAddDeviceInternal called\n");
   
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
	DeviceExtension->UartType = UartType;
	Status = InitializeCircularBuffer(&DeviceExtension->InputBuffer, 16);
	if (!NT_SUCCESS(Status)) goto ByeBye;
	Status = InitializeCircularBuffer(&DeviceExtension->OutputBuffer, 16);
	if (!NT_SUCCESS(Status)) goto ByeBye;
	IoInitializeRemoveLock(&DeviceExtension->RemoveLock, SERIAL_TAG, 0, 0);
	KeInitializeSpinLock(&DeviceExtension->InputBufferLock);
	KeInitializeSpinLock(&DeviceExtension->OutputBufferLock);
	KeInitializeDpc(&DeviceExtension->ReceivedByteDpc, SerialReceiveByte, DeviceExtension);
	KeInitializeDpc(&DeviceExtension->SendByteDpc, SerialSendByte, DeviceExtension);
	Fdo->Flags |= DO_POWER_PAGABLE;
	Status = IoAttachDeviceToDeviceStackSafe(Fdo, Pdo, &DeviceExtension->LowerDevice);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Serial: IoAttachDeviceToDeviceStackSafe() failed with status 0x%08x\n", Status);
		goto ByeBye;
	}
	Fdo->Flags |= DO_BUFFERED_IO;
	Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
	if (pFdo)
	{
		*pFdo = Fdo;
	}
	
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
SerialAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	/* Serial.sys is a legacy driver. AddDevice is called once
	 * with a NULL Pdo just after the driver initialization.
	 * Detect this case and return success.
	 */
	if (Pdo == NULL)
		return STATUS_SUCCESS;
	
	/* We have here a PDO that does not correspond to a legacy
	 * serial port. So call the internal AddDevice function.
	 */
	DPRINT1("Serial: SerialAddDevice() called. Pdo 0x%p (should be NULL)\n", Pdo);
	/* FIXME: due to a bug, previously described AddDevice is
	 * not called with a NULL Pdo. Block this call (blocks
	 * unfortunately all the other PnP serial ports devices).
	 */
	return SerialAddDeviceInternal(DriverObject, Pdo, UartUnknown, NULL);
	//return STATUS_UNSUCCESSFUL;
}

NTSTATUS STDCALL
SerialPnpStartDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PCM_RESOURCE_LIST ResourceList)
{
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	WCHAR DeviceNameBuffer[32];
	UNICODE_STRING DeviceName;
	WCHAR LinkNameBuffer[32];
	UNICODE_STRING LinkName;
	WCHAR ComPortBuffer[32];
	UNICODE_STRING ComPort;
	ULONG Vector = 0;
	ULONG i, j;
	KIRQL Dirql;
	KAFFINITY Affinity = 0;
	KINTERRUPT_MODE InterruptMode = Latched;
	BOOLEAN ShareInterrupt = TRUE;
	OBJECT_ATTRIBUTES objectAttributes;
	PUCHAR ComPortBase;
	UNICODE_STRING KeyName;
	HANDLE hKey;
	NTSTATUS Status;
	
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
	ASSERT(DeviceExtension->PnpState == dsStopped);
	
	DeviceExtension->ComPort = DeviceExtension->SerialPortNumber + 1;
	DeviceExtension->BaudRate = 19200 | SERIAL_BAUD_USER;
	DeviceExtension->BaseAddress = 0;
	Dirql = 0;
	for (i = 0; i < ResourceList->Count; i++)
	{
		for (j = 0; j < ResourceList->List[i].PartialResourceList.Count; j++)
		{
			PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor = &ResourceList->List[i].PartialResourceList.PartialDescriptors[j];
			switch (PartialDescriptor->Type)
			{
				case CmResourceTypePort:
					if (PartialDescriptor->u.Port.Length < 8)
						return STATUS_INSUFFICIENT_RESOURCES;
					if (DeviceExtension->BaseAddress != 0)
						return STATUS_UNSUCCESSFUL;
					DeviceExtension->BaseAddress = PartialDescriptor->u.Port.Start.u.LowPart;
					break;
				case CmResourceTypeInterrupt:
					if (Dirql != 0)
						return STATUS_UNSUCCESSFUL;
					Dirql = (KIRQL)PartialDescriptor->u.Interrupt.Level;
					Vector = PartialDescriptor->u.Interrupt.Vector;
					Affinity = PartialDescriptor->u.Interrupt.Affinity;
					if (PartialDescriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
						InterruptMode = Latched;
					else
						InterruptMode = LevelSensitive;
					ShareInterrupt = (PartialDescriptor->ShareDisposition == CmResourceShareShared);
					break;
			}
		}
	}
	DPRINT("Serial: New COM port. Base = 0x%lx, Irql = %u\n",
		DeviceExtension->BaseAddress, Dirql);
	if (!DeviceExtension->BaseAddress)
		return STATUS_INSUFFICIENT_RESOURCES;
	/* FIXME: we should be able to continue and use polling method
	 * for read/write if we don't have an interrupt */
	if (!Dirql)
		return STATUS_INSUFFICIENT_RESOURCES;
	ComPortBase = (PUCHAR)DeviceExtension->BaseAddress;
	
	if (DeviceExtension->UartType == UartUnknown)
		DeviceExtension->UartType = SerialDetectUartType(ComPortBase);
	
	/* Get current settings */
	DeviceExtension->IER = READ_PORT_UCHAR(SER_IER(ComPortBase));
	DeviceExtension->MCR = READ_PORT_UCHAR(SER_MCR(ComPortBase));
	DeviceExtension->MSR = READ_PORT_UCHAR(SER_MSR(ComPortBase));
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
	
	/* Clear receive/transmit buffers */
	if (DeviceExtension->UartType >= Uart16550A)
	{
		/* 16550 UARTs also have FIFO queues, but they are unusable due to a bug */
		WRITE_PORT_UCHAR(SER_FCR(ComPortBase),
			SR_FCR_CLEAR_RCVR | SR_FCR_CLEAR_XMIT);
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
	Status = IoConnectInterrupt(
		&DeviceExtension->Interrupt, SerialInterruptService,
		DeviceObject, NULL,
		Vector, Dirql, Dirql,
		InterruptMode, ShareInterrupt,
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
	WRITE_PORT_UCHAR(SER_IER(ComPortBase), DeviceExtension->IER);
	
	DeviceExtension->MCR |= 0x03; /* Activate DTR, RTS */
	WRITE_PORT_UCHAR(SER_MCR(ComPortBase), DeviceExtension->MCR);
	
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
			
			/* FIXME: first HACK: PnP manager can send multiple
			 * IRP_MN_START_DEVICE for one device
			 */
			if (((PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->PnpState != dsStopped)
			{
				DPRINT1("Serial: device already started. Ignoring this irp!\n");
				Status = STATUS_SUCCESS;
				break;
			}
			/* FIXME: AllocatedResources MUST never be NULL ;
			 * that's the second HACK because resource arbitration
			 * doesn't exist in ReactOS yet...
			 */
			if (Stack->Parameters.StartDevice.AllocatedResources == NULL)
			{
				ULONG ResourceListSize;
				PCM_RESOURCE_LIST ResourceList;
				PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;
				KIRQL Dirql;
				ULONG ComPortBase;
				ULONG Irq;
				
				DPRINT1("Serial: no allocated resources for this device! Creating fake list\n");
				/* These values are resources of the ONLY serial
				 * port that will be managed by this driver
				 * (default is COM2) */
				ComPortBase = 0x2f8;
				Irq = 3;
				
				/* Create resource list */
				ResourceListSize = sizeof(CM_RESOURCE_LIST) + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
				ResourceList = (PCM_RESOURCE_LIST)ExAllocatePoolWithTag(PagedPool, ResourceListSize, SERIAL_TAG);
				if (!ResourceList)
					return STATUS_INSUFFICIENT_RESOURCES;
				ResourceList->Count = 1;
				ResourceList->List[0].InterfaceType = Isa;
				ResourceList->List[0].BusNumber = -1; /* FIXME */
				ResourceList->List[0].PartialResourceList.Version = 1;
				ResourceList->List[0].PartialResourceList.Revision = 1;
				ResourceList->List[0].PartialResourceList.Count = 2;
				ResourceDescriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[0];
				ResourceDescriptor->Type = CmResourceTypePort;
				ResourceDescriptor->ShareDisposition = CmResourceShareDriverExclusive;
				ResourceDescriptor->Flags = CM_RESOURCE_PORT_IO;
				ResourceDescriptor->u.Port.Start.u.HighPart = 0;
				ResourceDescriptor->u.Port.Start.u.LowPart = ComPortBase;
				ResourceDescriptor->u.Port.Length = 8;
				
				ResourceDescriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[1];
				ResourceDescriptor->Type = CmResourceTypeInterrupt;
				ResourceDescriptor->ShareDisposition = CmResourceShareShared;
				ResourceDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
				ResourceDescriptor->u.Interrupt.Vector = HalGetInterruptVector(
					Internal, 0, 0, Irq,
					&Dirql,
					&ResourceDescriptor->u.Interrupt.Affinity);
				ResourceDescriptor->u.Interrupt.Level = (ULONG)Dirql;
				
				Stack->Parameters.StartDevice.AllocatedResources =
					Stack->Parameters.StartDevice.AllocatedResourcesTranslated =
					ResourceList;
			}
			
			/* Call lower driver */
			Status = ForwardIrpAndWait(DeviceObject, Irp);
			if (NT_SUCCESS(Status))
				Status = SerialPnpStartDevice(
					DeviceObject,
					Stack->Parameters.StartDevice.AllocatedResources);
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
