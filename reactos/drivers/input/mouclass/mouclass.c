/*

 ** Mouse class driver 0.0.1
 ** Written by Jason Filby (jasonfilby@yahoo.com)
 ** For ReactOS (www.reactos.com)

 ** The class driver between win32k and the various mouse port drivers

 ** TODO: Change interface to win32k to a callback instead of ReadFile IO
          Add support for multiple port devices

*/

#include <ddk/ntddk.h>
#include "../include/mouse.h"
#include "mouclass.h"

#define NDEBUG
#include <debug.h>

BOOLEAN AlreadyOpened = FALSE;

VOID MouseClassPassiveCallback(PDEVICE_OBJECT ClassDeviceObject, PVOID Context)
{
  PDEVICE_EXTENSION ClassDeviceExtension = ClassDeviceObject->DeviceExtension;
  MOUSE_INPUT_DATA PortData[MOUSE_BUFFER_SIZE];
  ULONG InputCount;
  KIRQL OldIrql;

  assert(NULL != ClassDeviceExtension->GDIInformation.CallBack);
  KeAcquireSpinLock(&(ClassDeviceExtension->SpinLock), &OldIrql);
  DPRINT("Entering MouseClassPassiveCallback\n");
  while (0 != ClassDeviceExtension->InputCount) {
    ClassDeviceExtension->PortData -= ClassDeviceExtension->InputCount;
    RtlMoveMemory(PortData, ClassDeviceExtension->PortData,
                  ClassDeviceExtension->InputCount * sizeof(MOUSE_INPUT_DATA));
    InputCount = ClassDeviceExtension->InputCount;
    ClassDeviceExtension->InputCount = 0;
    KeReleaseSpinLock(&(ClassDeviceExtension->SpinLock), OldIrql);

    DPRINT("MouseClassPassiveCallBack() Calling GDI callback at %p\n",
           ClassDeviceExtension->GDIInformation.CallBack);
    /* We're jumping through hoops to get to run at PASSIVE_LEVEL, let's make
       sure we succeeded */
    ASSERT_IRQL(PASSIVE_LEVEL);
    (*(PGDI_SERVICE_CALLBACK_ROUTINE)ClassDeviceExtension->GDIInformation.CallBack)
          (PortData, InputCount);

    KeAcquireSpinLock(&(ClassDeviceExtension->SpinLock), &OldIrql);
  }

  ClassDeviceExtension->PassiveCallbackQueued = FALSE;
  DPRINT("Leaving MouseClassPassiveCallback\n");
  KeReleaseSpinLock(&(ClassDeviceExtension->SpinLock), OldIrql);
}

BOOLEAN MouseClassCallBack(PDEVICE_OBJECT ClassDeviceObject, PMOUSE_INPUT_DATA MouseDataStart,
			PMOUSE_INPUT_DATA MouseDataEnd, PULONG InputCount)
{
   PDEVICE_EXTENSION ClassDeviceExtension = ClassDeviceObject->DeviceExtension;
   PIRP Irp;
   ULONG ReadSize;
   PIO_STACK_LOCATION Stack;
   KIRQL OldIrql;

   // In classical NT, you would take the input data and pipe it through the IO system, for the GDI to read.
   // In ReactOS, however, we use a GDI callback for increased mouse responsiveness. The reason we don't
   // simply call from the port driver is so that our mouse class driver can support NT mouse port drivers.

   DPRINT("Entering MouseClassCallBack\n");
/*   if(ClassDeviceExtension->ReadIsPending == TRUE)
   {
      Irp = ClassDeviceObject->CurrentIrp;
      ClassDeviceObject->CurrentIrp = NULL;
      Stack = IoGetCurrentIrpStackLocation(Irp);

      ReadSize = sizeof(MOUSE_INPUT_DATA) * (*InputCount);

      // A read request is waiting for input, so go straight to it
      RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer, (PCHAR)MouseDataStart, ReadSize);

      // Go to next packet and complete this request with STATUS_SUCCESS
      Irp->IoStatus.Status = STATUS_SUCCESS;
      Irp->IoStatus.Information = ReadSize;
      Stack->Parameters.Read.Length = ReadSize;

      IoStartNextPacket(ClassDeviceObject, FALSE);
      IoCompleteRequest(Irp, IO_MOUSE_INCREMENT);      
      ClassDeviceExtension->ReadIsPending = FALSE;
   } */

  // If we have data from the port driver and a higher service to send the data to
  if((*InputCount>0) && (*(PGDI_SERVICE_CALLBACK_ROUTINE)ClassDeviceExtension->GDIInformation.CallBack != NULL))
  {
    KeAcquireSpinLock(&(ClassDeviceExtension->SpinLock), &OldIrql);

    if(ClassDeviceExtension->InputCount + *InputCount > MOUSE_BUFFER_SIZE)
    {
       ReadSize = MOUSE_BUFFER_SIZE - ClassDeviceExtension->InputCount;
    } else {
       ReadSize = *InputCount;
    }

    // FIXME: If we exceed the buffer, mouse data gets thrown away.. better solution?


    // Move the mouse input data from the port data queue to our class data queue
    RtlMoveMemory(ClassDeviceExtension->PortData, (PCHAR)MouseDataStart,
                  sizeof(MOUSE_INPUT_DATA) * ReadSize);

    // Move the pointer and counter up
    ClassDeviceExtension->PortData += ReadSize;
    ClassDeviceExtension->InputCount += ReadSize;

    if(*(PGDI_SERVICE_CALLBACK_ROUTINE)ClassDeviceExtension->GDIInformation.CallBack != NULL) {
      if (! ClassDeviceExtension->PassiveCallbackQueued) {
	if (NULL == ClassDeviceExtension->WorkItem) {
	  ClassDeviceExtension->WorkItem = IoAllocateWorkItem(ClassDeviceObject);
	}
	if (NULL != ClassDeviceExtension->WorkItem) {
	  DPRINT("Queueing workitem\n");
          IoQueueWorkItem(ClassDeviceExtension->WorkItem, MouseClassPassiveCallback, CriticalWorkQueue, NULL);
	  ClassDeviceExtension->PassiveCallbackQueued = TRUE;
	}
      }
    } else {
      DPRINT("MouseClassCallBack() NO GDI callback installed\n");
    }
    KeReleaseSpinLock(&(ClassDeviceExtension->SpinLock), OldIrql);
  } else {
    DPRINT("MouseClassCallBack() entered, InputCount = %d - DOING NOTHING\n", *InputCount);
  }

  DPRINT("Leaving MouseClassCallBack\n");
  return TRUE;
}

NTSTATUS ConnectMousePortDriver(PDEVICE_OBJECT ClassDeviceObject)
{
   PDEVICE_OBJECT PortDeviceObject = NULL;
   PFILE_OBJECT FileObject = NULL;
   NTSTATUS status;
   UNICODE_STRING PortName = UNICODE_STRING_INITIALIZER(L"\\Device\\Mouse");
   IO_STATUS_BLOCK ioStatus;
   KEVENT event;
   PIRP irp;
   CLASS_INFORMATION ClassInformation;
   PDEVICE_EXTENSION DeviceExtension = ClassDeviceObject->DeviceExtension;

   DeviceExtension->GDIInformation.CallBack = NULL;

   // Get the port driver's DeviceObject
   // FIXME: The name might change.. find a way to be more dynamic?

   status = IoGetDeviceObjectPointer(&PortName, FILE_READ_ATTRIBUTES, &FileObject, &PortDeviceObject);

   if(status != STATUS_SUCCESS)
   {
      DbgPrint("MOUCLASS: Could not connect to mouse port driver\n");
      return status;
   }

   DeviceExtension->PortDeviceObject = PortDeviceObject;
   DeviceExtension->PortData = ExAllocatePool(NonPagedPool, MOUSE_BUFFER_SIZE * sizeof(MOUSE_INPUT_DATA));
   DeviceExtension->InputCount = 0;
   DeviceExtension->ReadIsPending = FALSE;
   DeviceExtension->WorkItem = NULL;
   KeInitializeSpinLock(&(DeviceExtension->SpinLock));
   DeviceExtension->PassiveCallbackQueued = FALSE;

   // Connect our callback to the port driver

   KeInitializeEvent(&event, NotificationEvent, FALSE);

   ClassInformation.DeviceObject = ClassDeviceObject;
   ClassInformation.CallBack     = MouseClassCallBack;

   irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_MOUSE_CONNECT,
      PortDeviceObject, &ClassInformation, sizeof(CLASS_INFORMATION), NULL, 0, TRUE, &event, &ioStatus);

   status = IoCallDriver(DeviceExtension->PortDeviceObject, irp);

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
   } else {
      ioStatus.Status = status;
   }

   return ioStatus.Status;
}

NTSTATUS STDCALL MouseClassDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS Status;

   switch (Stack->MajorFunction)
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

      case IRP_MJ_READ:

       if (Stack->Parameters.Read.Length == 0) {
           Status = STATUS_SUCCESS;
        } else {
	   Status = STATUS_PENDING;
        }
	break;

      default:
        DbgPrint("NOT IMPLEMENTED\n");
        Status = STATUS_NOT_IMPLEMENTED;
	break;
     }

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   if (Status==STATUS_PENDING)
   {
      IoMarkIrpPending(Irp);
      IoStartPacket(DeviceObject, Irp, NULL, NULL);
   } else {
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
   }
   return(Status);
}

VOID MouseClassStartIo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   ULONG ReadSize;

   if(DeviceExtension->InputCount>0)
   {
      // FIXME: We should not send too much input data.. depends on the max buffer size of the win32k
      ReadSize = DeviceExtension->InputCount * sizeof(MOUSE_INPUT_DATA);

      // Bring the PortData back to base so that it can be copied
      DeviceExtension->PortData -= DeviceExtension->InputCount;
      DeviceExtension->InputCount = 0;
      DeviceExtension->ReadIsPending = FALSE;

      RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer, (PCHAR)DeviceExtension->PortData, ReadSize);

      // Go to next packet and complete this request with STATUS_SUCCESS
      Irp->IoStatus.Status = STATUS_SUCCESS;

      Irp->IoStatus.Information = ReadSize;
      Stack->Parameters.Read.Length = ReadSize;

      IoStartNextPacket(DeviceObject, FALSE);
      IoCompleteRequest(Irp, IO_MOUSE_INCREMENT);
   } else {
      DeviceExtension->ReadIsPending = TRUE;
   }
}

NTSTATUS STDCALL MouseClassInternalDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   // Retrieve GDI's callback

   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS status;

   switch(Stack->Parameters.DeviceIoControl.IoControlCode)
   {
      case IOCTL_INTERNAL_MOUSE_CONNECT:

         DeviceExtension->GDIInformation =
            *((PGDI_INFORMATION)Stack->Parameters.DeviceIoControl.Type3InputBuffer);

         DbgPrint("MouseClassInternalDeviceControl() installed GDI callback at %p\n", DeviceExtension->GDIInformation.CallBack);

         status = STATUS_SUCCESS;
         break;

      case IOCTL_INTERNAL_MOUSE_DISCONNECT:

         DeviceExtension->GDIInformation.CallBack = NULL;

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

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
   PDEVICE_OBJECT DeviceObject;
   UNICODE_STRING DeviceName = UNICODE_STRING_INITIALIZER(L"\\Device\\MouseClass");
   UNICODE_STRING SymlinkName = UNICODE_STRING_INITIALIZER(L"\\??\\MouseClass");

   DriverObject->MajorFunction[IRP_MJ_CREATE] = MouseClassDispatch;
//   DriverObject->MajorFunction[IRP_MJ_CLOSE]  = MouseClassDispatch;
//   DriverObject->MajorFunction[IRP_MJ_READ]   = MouseClassDispatch;
   DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = MouseClassInternalDeviceControl; // to get GDI callback
//   DriverObject->DriverStartIo                = MouseClassStartIo;

   IoCreateDevice(DriverObject,
		  sizeof(DEVICE_EXTENSION),
		  &DeviceName,
		  FILE_DEVICE_MOUSE,
		  0,
		  TRUE,
		  &DeviceObject);
   DeviceObject->Flags = DeviceObject->Flags | DO_BUFFERED_IO;

   IoCreateSymbolicLink(&SymlinkName, &DeviceName);

   return ConnectMousePortDriver(DeviceObject);
}
