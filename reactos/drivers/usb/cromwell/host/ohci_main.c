/*
   ReactOS specific functions for ohci module
   by Aleksey Bragin (aleksey@reactos.com)
*/

#include <ddk/ntddk.h>
#include <debug.h>

NTSTATUS AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT pdo)
{
	PDEVICE_OBJECT fdo;
	NTSTATUS Status;
	WCHAR DeviceBuffer[20];
	UNICODE_STRING DeviceName;

	DbgPrint("ohci: AddDevice called\n");
   
	/* Create a unicode device name. */
	swprintf(DeviceBuffer, L"\\Device\\usbohci");
	RtlInitUnicodeString(&DeviceName, DeviceBuffer);


	Status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_VIDEO, 0, FALSE,&fdo);

	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoCreateDevice call failed with status 0x%08x\n", Status);
		return Status;
	}

	return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	// nothing to do here yet
}

// Dispatch PNP
NTSTATUS DispatchPnp(PDEVICE_OBJECT fdo, PIRP Irp)
{
	ULONG fcn;
	PIO_STACK_LOCATION stack;
	
	stack = IoGetCurrentIrpStackLocation(Irp);
	fcn = stack->MinorFunction; 
	DbgPrint("IRP_MJ_PNP, fcn=%d\n", fcn);

	if (fcn == IRP_MN_REMOVE_DEVICE)
	{
		IoDeleteDevice(fdo);
    }

	return STATUS_SUCCESS;
}

NTSTATUS DispatchPower(PDEVICE_OBJECT fido, PIRP Irp)
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
