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
	IN PULONG pComPortNumber OPTIONAL,
	OUT PDEVICE_OBJECT* pFdo OPTIONAL)
{
	PDEVICE_OBJECT Fdo = NULL;
	PSERIAL_DEVICE_EXTENSION DeviceExtension = NULL;
	NTSTATUS Status;
	WCHAR DeviceNameBuffer[32];
	UNICODE_STRING DeviceName;
	static ULONG DeviceNumber = 0;
	static ULONG ComPortNumber = 1;

	DPRINT("Serial: SerialAddDeviceInternal called\n");

	ASSERT(DeviceObject);
	ASSERT(Pdo);

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
	Status = IoRegisterDeviceInterface(Pdo, &GUID_DEVINTERFACE_COMPORT, NULL, &DeviceExtension->SerialInterfaceName);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Serial: IoRegisterDeviceInterface() failed with status 0x%08x\n", Status);
		goto ByeBye;
	}

	DeviceExtension->SerialPortNumber = DeviceNumber++;
	if (pComPortNumber == NULL)
		DeviceExtension->ComPort = ComPortNumber++;
	else
		DeviceExtension->ComPort = *pComPortNumber;
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
	KeInitializeEvent(&DeviceExtension->InputBufferNotEmpty, NotificationEvent, FALSE);
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

	/* We have here a PDO not null. It represents a real serial
	 * port. So call the internal AddDevice function.
	 */
	return SerialAddDeviceInternal(DriverObject, Pdo, UartUnknown, NULL, NULL);
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
	UCHAR IER;
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

	ASSERT(ResourceList);
	ASSERT(DeviceExtension);
	ASSERT(DeviceExtension->PnpState == dsStopped);

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
	if (!Dirql)
		return STATUS_INSUFFICIENT_RESOURCES;
	ComPortBase = (PUCHAR)DeviceExtension->BaseAddress;

	if (DeviceExtension->UartType == UartUnknown)
		DeviceExtension->UartType = SerialDetectUartType(ComPortBase);

	/* Get current settings */
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

	/* Activate serial interface */
	Status = IoSetDeviceInterfaceState(&DeviceExtension->SerialInterfaceName, TRUE);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Serial: IoSetDeviceInterfaceState() failed with status 0x%08x\n", Status);
		IoDeleteSymbolicLink(&LinkName);
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
		IoSetDeviceInterfaceState(&DeviceExtension->SerialInterfaceName, FALSE);
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

	/* Activate interrupt modes */
	IER = READ_PORT_UCHAR(SER_IER(ComPortBase));
	IER |= SR_IER_DATA_RECEIVED | SR_IER_THR_EMPTY | SR_IER_LSR_CHANGE | SR_IER_MSR_CHANGE;
	WRITE_PORT_UCHAR(SER_IER(ComPortBase), IER);

	/* Activate DTR, RTS */
	DeviceExtension->MCR |= SR_MCR_DTR | SR_MCR_RTS;
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
	ULONG_PTR Information = 0;
	NTSTATUS Status;

	Stack = IoGetCurrentIrpStackLocation(Irp);
	MinorFunction = Stack->MinorFunction;

	switch (MinorFunction)
	{
		case IRP_MN_START_DEVICE:
		{
			BOOLEAN ConflictDetected;
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
			/* FIXME: second HACK: verify that we have some allocated resources.
			 * It seems not to be always the case on some hardware
			 */
			if (Stack->Parameters.StartDevice.AllocatedResources == NULL)
			{
				DPRINT1("Serial: no allocated resources. Can't start COM%lu\n",
					((PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->ComPort);
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
			/* FIXME: third HACK: verify that we don't have resource conflict,
			 * because PnP manager doesn't do it automatically
			 */
			Status = IoReportResourceForDetection(
				DeviceObject->DriverObject, Stack->Parameters.StartDevice.AllocatedResources, 0,
				NULL, NULL, 0,
				&ConflictDetected);
			if (!NT_SUCCESS(Status))
			{
				Irp->IoStatus.Information = 0;
				Irp->IoStatus.Status = Status;
				IoCompleteRequest(Irp, IO_NO_INCREMENT);
				return Status;
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
			disable interface
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
