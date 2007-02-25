/*
 * ReactOS USB hub driver
 * Copyright (C) 2004 Aleksey Bragin
 *           (C) 2005 Mark Tempel
 *           (C) 2005 Hervé Poussineau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

/* INCLUDES *******************************************************************/
//#define NDEBUG
#include "usbhub.h"

/* PUBLIC AND PRIVATE FUNCTIONS ***********************************************/

static NTSTATUS
GetRootHubPointer(
	IN PDEVICE_OBJECT Pdo,
	OUT PVOID* RootHubPointer)
{
	KEVENT Event;
	PIRP Irp;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS Status;
	
	KeInitializeEvent (&Event, NotificationEvent, FALSE);
	
	Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO,
		Pdo,
		NULL, sizeof(NULL),
		RootHubPointer, sizeof(*RootHubPointer),
		FALSE,
		&Event,
		&IoStatus);
	if (Irp == NULL)
	{
		DPRINT("Usbhub: IoBuildDeviceIoControlRequest() failed\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	
	/* Initialize the status block before sending the IRP */
	IoGetNextIrpStackLocation(Irp)->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	IoStatus.Status = STATUS_NOT_SUPPORTED;
	IoStatus.Information = 0;
	
	Status = IoCallDriver(Pdo, Irp);
	
	if (Status == STATUS_PENDING)
	{
		DPRINT("Usbhub: Operation pending\n");
		KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
		Status = IoStatus.Status;
	}
	
	return Status;
}

NTSTATUS STDCALL 
UsbhubAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	PDEVICE_OBJECT Fdo;
	PHUB_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;
	
	Status = IoCreateDevice(DriverObject,
		sizeof(HUB_DEVICE_EXTENSION),
		NULL, /* DeviceName */
		FILE_DEVICE_BUS_EXTENDER,
		0,
		FALSE,
		&Fdo);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("Usbhub: IoCreateDevice() failed with status 0x%08lx\n", Status);
		return Status;
	}
	
	// zerofill device extension
	DeviceExtension = (PHUB_DEVICE_EXTENSION)Fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(HUB_DEVICE_EXTENSION));
	
	/* Get a pointer to the linux structure created by the USB controller,
	 * by sending IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO to lower device.
	 */
	Status = GetRootHubPointer(Pdo, (PVOID*)&DeviceExtension->dev);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Usbhub: GetRootHubPointer() failed with status 0x%08lx\n", Status);
		IoDeleteDevice(Fdo);
		return Status;
	}
	DeviceExtension->dev->dev.dev_ext = Pdo;
	
	DeviceExtension->IsFDO = TRUE;
	Fdo->Flags |= DO_POWER_PAGABLE;
	Status = IoAttachDeviceToDeviceStackSafe(Fdo, Pdo, &DeviceExtension->LowerDevice);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Usbhub: IoAttachDeviceToDeviceStackSafe() failed with status 0x%08lx\n", Status);
		IoDeleteDevice(Fdo);
		return Status;
	}
	Fdo->Flags |= DO_BUFFERED_IO;
	Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
	
	return STATUS_SUCCESS;
}

static NTSTATUS STDCALL
IrpStub(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	NTSTATUS Status;

	if (((PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
	{
		DPRINT1("Usbhub: FDO stub for major function 0x%lx\n",
			IoGetCurrentIrpStackLocation(Irp)->MajorFunction);
#ifndef NDEBUG
		DbgBreakPoint();
#endif
		return ForwardIrpAndForget(DeviceObject, Irp);
	}
	else
	{
		/* We can't forward request to the lower driver, because
		 * we are a Pdo, so we don't have lower driver...
		 */
		DPRINT1("Usbhub: PDO stub for major function 0x%lx\n",
			IoGetCurrentIrpStackLocation(Irp)->MajorFunction);
#ifndef NDEBUG
		DbgBreakPoint();
#endif
	}

	Status = Irp->IoStatus.Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

static NTSTATUS STDCALL
DispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (((PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return UsbhubDeviceControlFdo(DeviceObject, Irp);
	else
		return IrpStub(DeviceObject, Irp);
}

static NTSTATUS STDCALL
DispatchInternalDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (((PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return IrpStub(DeviceObject, Irp);
	else
		return UsbhubInternalDeviceControlPdo(DeviceObject, Irp);
}

static NTSTATUS STDCALL
DispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (((PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return UsbhubPnpFdo(DeviceObject, Irp);
	else
		return UsbhubPnpPdo(DeviceObject, Irp);
}

/*
 * Standard DriverEntry method.
 */
NTSTATUS STDCALL
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
{
	ULONG i;
	
	DriverObject->DriverExtension->AddDevice = UsbhubAddDevice;
	
	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = IrpStub;
	
	DriverObject->MajorFunction[IRP_MJ_CREATE] = UsbhubCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = UsbhubClose;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = UsbhubCleanup;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
	DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = DispatchInternalDeviceControl;
	DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
	
	return STATUS_SUCCESS;
}

