/*
   ReactOS specific functions for usbcore module
   by Aleksey Bragin (aleksey@reactos.com)
*/

#include <ddk/ntddk.h>
#include <debug.h>

NTSTATUS AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT pdo)
{
	DbgPrint("usbcore: AddDevice called\n");
	
	/* we need to do kind of this stuff here (as usual)
	PDEVICE_OBJECT fdo;
	IoCreateDevice(..., &fdo);
	pdx->LowerDeviceObject = 
	IoAttachDeviceToDeviceStack(fdo, pdo);*/

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
