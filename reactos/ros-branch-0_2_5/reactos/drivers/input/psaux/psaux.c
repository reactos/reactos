/*
 ** PS/2 driver 0.0.2
 ** Written by Jason Filby (jasonfilby@yahoo.com)
 ** For ReactOS (www.reactos.com)

 ** Handles the keyboard and mouse on the PS/2 ports

 ** TODO: Fix detect_ps2_port(void) so that it works under BOCHs
*/

#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include "mouse.h"
#include "psaux.h"

#define NDEBUG
#include <debug.h>

static PIRP  CurrentIrp;
static ULONG MouseDataRead;
static ULONG MouseDataRequired;
static BOOLEAN AlreadyOpened = FALSE;

BOOLEAN STDCALL
MouseSynchronizeRoutine(PVOID Context)
{
   PIRP Irp = (PIRP)Context;
   PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
   ULONG NrToRead         = stk->Parameters.Read.Length/sizeof(MOUSE_INPUT_DATA);

   if ((stk->Parameters.Read.Length/sizeof(MOUSE_INPUT_DATA))==NrToRead)
   {
      return(TRUE);
   }

   MouseDataRequired=stk->Parameters.Read.Length/sizeof(MOUSE_INPUT_DATA);
   MouseDataRead=NrToRead;
   CurrentIrp=Irp;

   return(FALSE);
}

VOID STDCALL
PS2MouseStartIo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

   if (KeSynchronizeExecution(DeviceExtension->MouseInterrupt, MouseSynchronizeRoutine, Irp))
     {
        KIRQL oldIrql;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	oldIrql = KeGetCurrentIrql();
        if (oldIrql < DISPATCH_LEVEL)
          {
            KeRaiseIrql (DISPATCH_LEVEL, &oldIrql);
            IoStartNextPacket (DeviceObject, FALSE);
            KeLowerIrql(oldIrql);
	  }
        else
          {
            IoStartNextPacket (DeviceObject, FALSE);
	  }
     }
}

NTSTATUS STDCALL
PS2MouseDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS Status;

   switch (stk->MajorFunction)
     {
      case IRP_MJ_CREATE:
	if (AlreadyOpened == TRUE)
	  {
	     Status = STATUS_SUCCESS;
	  }
	else
	  {
	     Status = STATUS_SUCCESS;
	     AlreadyOpened = TRUE;
	  }
	break;
	
      case IRP_MJ_CLOSE:
        Status = STATUS_SUCCESS;
	break;

      default:
        DPRINT1("NOT IMPLEMENTED\n");
        Status = STATUS_NOT_IMPLEMENTED;
	break;
     }

   if (Status==STATUS_PENDING)
     {
	IoMarkIrpPending(Irp);
     }
   else
     {
        Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
     }
   return(Status);
}

BOOLEAN STDCALL PS2MouseInitializeDataQueue(PVOID Context)
{
   return(TRUE);
}

NTSTATUS STDCALL
PS2MouseInternalDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS status;

   switch(Stack->Parameters.DeviceIoControl.IoControlCode)
   {
      case IOCTL_INTERNAL_MOUSE_CONNECT:

         DeviceExtension->ClassInformation =
            *((PCLASS_INFORMATION)Stack->Parameters.DeviceIoControl.Type3InputBuffer);

         // Reinitialize the port input data queue synchronously
         KeSynchronizeExecution(DeviceExtension->MouseInterrupt,
            PS2MouseInitializeDataQueue, DeviceExtension);

         status = STATUS_SUCCESS;
         break;

      default:
         status = STATUS_INVALID_DEVICE_REQUEST;
         break;
   }

   Irp->IoStatus.Status = status;
   if (status == STATUS_PENDING) {
      IoMarkIrpPending(Irp);
      IoStartPacket(DeviceObject, Irp, NULL, NULL);
   } else {
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
   }

   return status;
}

VOID PS2MouseIsrDpc(PKDPC Dpc, PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   ULONG Queue;

   Queue = DeviceExtension->ActiveQueue % 2;
   InterlockedIncrement(&DeviceExtension->ActiveQueue);
   (*(PSERVICE_CALLBACK_ROUTINE)DeviceExtension->ClassInformation.CallBack)(
			DeviceExtension->ClassInformation.DeviceObject,
			DeviceExtension->MouseInputData[Queue],
			NULL,
			&DeviceExtension->InputDataCount[Queue]);
   DeviceExtension->InputDataCount[Queue] = 0;
}

/* Maximum value plus one for \Device\PointerClass* device name */
#define POINTER_PORTS_MAXIMUM	8
/* Letter count for POINTER_PORTS_MAXIMUM variable * sizeof(WCHAR) */
#define SUFFIX_MAXIMUM_SIZE		(1 * sizeof(WCHAR))

/* This is almost the same routine as in sermouse.c. */
STATIC PDEVICE_OBJECT
AllocatePointerDevice(PDRIVER_OBJECT DriverObject)
{
	PDEVICE_OBJECT DeviceObject;
	UNICODE_STRING DeviceName;
	UNICODE_STRING SuffixString;
	UNICODE_STRING SymlinkName;
	PDEVICE_EXTENSION DeviceExtension;
	ULONG Suffix;
	NTSTATUS Status;

	/* Allocate buffer for full device name */   
	RtlInitUnicodeString(&DeviceName, NULL);
	DeviceName.MaximumLength = sizeof(DD_MOUSE_DEVICE_NAME_U) + SUFFIX_MAXIMUM_SIZE + sizeof(UNICODE_NULL);
	DeviceName.Buffer = ExAllocatePool(PagedPool, DeviceName.MaximumLength);
	RtlAppendUnicodeToString(&DeviceName, DD_MOUSE_DEVICE_NAME_U);

	/* Allocate buffer for device name suffix */
	RtlInitUnicodeString(&SuffixString, NULL);
	SuffixString.MaximumLength = SUFFIX_MAXIMUM_SIZE + sizeof(UNICODE_NULL);
	SuffixString.Buffer = ExAllocatePool(PagedPool, SuffixString.MaximumLength);

	/* Generate full qualified name with suffix */
	for (Suffix = 0; Suffix < POINTER_PORTS_MAXIMUM; ++Suffix)
	{
		ANSI_STRING DebugString;

		RtlIntegerToUnicodeString(Suffix, 10, &SuffixString);
		RtlAppendUnicodeToString(&DeviceName, SuffixString.Buffer);
		// FIXME: this isn't really a serial mouse port driver
		Status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION),
			&DeviceName, FILE_DEVICE_SERIAL_MOUSE_PORT, 0, TRUE, &DeviceObject);
		RtlUnicodeStringToAnsiString(&DebugString, &DeviceName, TRUE);
		DPRINT("%s", DebugString.Buffer);
		RtlFreeAnsiString(&DebugString);
		/* Device successfully created, leave the cyclus */
		if (NT_SUCCESS(Status))
			break;
		DeviceName.Length -= SuffixString.Length;
	}
 
	ExFreePool(DeviceName.Buffer);

	/* Couldn't create device */
	if (!NT_SUCCESS(Status))
	{
		ExFreePool(SuffixString.Buffer);
		return NULL;
	}

	DeviceObject->Flags = DeviceObject->Flags | DO_BUFFERED_IO;

	/* Create symlink */
	RtlInitUnicodeString(&SymlinkName, NULL);
	SymlinkName.MaximumLength = sizeof(L"\\??\\Mouse") + SUFFIX_MAXIMUM_SIZE + sizeof(UNICODE_NULL);
	SymlinkName.Buffer = ExAllocatePool(PagedPool, SymlinkName.MaximumLength);
	RtlAppendUnicodeToString(&SymlinkName, L"\\??\\Mouse");
	RtlAppendUnicodeToString(&DeviceName, SuffixString.Buffer);
	IoCreateSymbolicLink(&SymlinkName, &DeviceName);
	ExFreePool(SuffixString.Buffer);

	DeviceExtension = DeviceObject->DeviceExtension;
	KeInitializeDpc(&DeviceExtension->IsrDpc, (PKDEFERRED_ROUTINE)PS2MouseIsrDpc, DeviceObject);

	return DeviceObject;
}

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
   PDEVICE_OBJECT DeviceObject;

   if (DetectPS2Port() == TRUE) {
     DPRINT("PS2 Port Driver version 0.0.2\n");
   } else {
     DPRINT1("PS2 port not found.\n");
     return STATUS_UNSUCCESSFUL;
   }

   DriverObject->MajorFunction[IRP_MJ_CREATE] = PS2MouseDispatch;
   DriverObject->MajorFunction[IRP_MJ_CLOSE]  = PS2MouseDispatch;
   DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = PS2MouseInternalDeviceControl;
   DriverObject->DriverStartIo                = PS2MouseStartIo;

   DeviceObject = AllocatePointerDevice(DriverObject);

   SetupMouse(DeviceObject, RegistryPath);

   return(STATUS_SUCCESS);
}
