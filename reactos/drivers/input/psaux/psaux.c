/*

 ** PS/2 driver 0.0.1
 ** Written by Jason Filby (jasonfilby@yahoo.com)
 ** For ReactOS (www.reactos.com)

 ** Handles the keyboard and mouse on the PS/2 ports

 ** TODO: Fix detect_ps2_port(void) so that it works under BOCHs
          Implement mouse button support

*/

#include <ddk/ntddk.h>
#include "../include/mouse.h"
#include "mouse.h"
#include "psaux.h"

BOOLEAN STDCALL
MouseSynchronizeRoutine(PVOID Context)
{
   PIRP Irp = (PIRP)Context;
   PMOUSE_INPUT_DATA rec  = (PMOUSE_INPUT_DATA)Irp->AssociatedIrp.SystemBuffer;
   PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
   ULONG NrToRead         = stk->Parameters.Read.Length/sizeof(MOUSE_INPUT_DATA);
   int i;

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
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	IoStartNextPacket(DeviceObject, FALSE);
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
        DbgPrint("NOT IMPLEMENTED\n");
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

VOID PS2MouseInitializeDataQueue(PVOID Context)
{
  ;
/*   PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)DeviceExtension;

   DeviceExtension->InputDataCount = 0;
   DeviceExtension->MouseInputData = ExAllocatePool(NonPagedPool, sizeof(MOUSE_INPUT_DATA) * MOUSE_BUFFER_SIZE); */
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
            (PKSYNCHRONIZE_ROUTINE)PS2MouseInitializeDataQueue, DeviceExtension);

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

   (*(PSERVICE_CALLBACK_ROUTINE)DeviceExtension->ClassInformation.CallBack)(
			DeviceExtension->ClassInformation.DeviceObject,
			DeviceExtension->MouseInputData,
			NULL,
			&DeviceExtension->InputDataCount);

   DeviceExtension->InputDataCount = 0;
}

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
   PDEVICE_OBJECT DeviceObject;
   UNICODE_STRING DeviceName;
   UNICODE_STRING SymlinkName;
   PDEVICE_EXTENSION DeviceExtension;

   if(detect_ps2_port() == TRUE)
   {
   } else
     return STATUS_UNSUCCESSFUL;

   DriverObject->MajorFunction[IRP_MJ_CREATE] = PS2MouseDispatch;
   DriverObject->MajorFunction[IRP_MJ_CLOSE]  = PS2MouseDispatch;
   DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = PS2MouseInternalDeviceControl;
   DriverObject->DriverStartIo                = PS2MouseStartIo;

   RtlInitUnicodeString(&DeviceName,
                        L"\\Device\\Mouse"); // FIXME: find correct device name
   IoCreateDevice(DriverObject,
		  sizeof(DEVICE_EXTENSION),
		  &DeviceName,
		  FILE_DEVICE_SERIAL_MOUSE_PORT, // FIXME: this isn't really a serial mouse port driver
		  0,
		  TRUE,
		  &DeviceObject);
   DeviceObject->Flags = DeviceObject->Flags | DO_BUFFERED_IO;

   RtlInitUnicodeString(&SymlinkName,
                        L"\\??\\Mouse"); // FIXME: find correct device name
   IoCreateSymbolicLink(&SymlinkName, &DeviceName);

   DeviceExtension = DeviceObject->DeviceExtension;
   KeInitializeDpc(&DeviceExtension->IsrDpc, (PKDEFERRED_ROUTINE)PS2MouseIsrDpc, DeviceObject);
   KeInitializeDpc(&DeviceExtension->IsrDpcRetry, (PKDEFERRED_ROUTINE)PS2MouseIsrDpc, DeviceObject);

   mouse_init(DeviceObject);

   return(STATUS_SUCCESS);
}
