/*

 ** Mouse class driver 0.0.1
 ** Written by Jason Filby (jasonfilby@yahoo.com)
 ** For ReactOS (www.reactos.com)

 ** The class driver between win32k and the various mouse port drivers

 ** TODO: Change interface to win32k to a callback instead of ReadFile IO
          Add support for multiple port devices

*/

#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include <rosrtl/string.h>
#include "mouclass.h"

#define NDEBUG
#include <debug.h>

BOOLEAN MouseClassCallBack(
   PDEVICE_OBJECT ClassDeviceObject, PMOUSE_INPUT_DATA MouseDataStart,
   PMOUSE_INPUT_DATA MouseDataEnd, PULONG ConsumedCount)
{
   PDEVICE_EXTENSION ClassDeviceExtension = ClassDeviceObject->DeviceExtension;
   PIRP Irp;
   KIRQL OldIrql;
   PIO_STACK_LOCATION Stack;
   ULONG InputCount = MouseDataEnd - MouseDataStart;
   ULONG ReadSize;

   DPRINT("Entering MouseClassCallBack\n");
   /* A filter driver might have consumed all the data already; I'm
    * not sure if they are supposed to move the packets when they
    * consume them though.
    */
   if (ClassDeviceExtension->ReadIsPending == TRUE &&
       InputCount)
   {
      Irp = ClassDeviceObject->CurrentIrp;
      ClassDeviceObject->CurrentIrp = NULL;
      Stack = IoGetCurrentIrpStackLocation(Irp);

      /* A read request is waiting for input, so go straight to it */
      RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer, MouseDataStart,
                    sizeof(MOUSE_INPUT_DATA));

      /* Go to next packet and complete this request with STATUS_SUCCESS */
      Irp->IoStatus.Status = STATUS_SUCCESS;
      Irp->IoStatus.Information = sizeof(MOUSE_INPUT_DATA);
      Stack->Parameters.Read.Length = sizeof(MOUSE_INPUT_DATA);

      IoStartNextPacket(ClassDeviceObject, FALSE);
      IoCompleteRequest(Irp, IO_MOUSE_INCREMENT);
      ClassDeviceExtension->ReadIsPending = FALSE;

      /* Skip the packet we just sent away */
      MouseDataStart++;
      (*ConsumedCount)++;
      InputCount--;
   }

   /* If we have data from the port driver and a higher service to send the data to */
   if (InputCount != 0)
   {
      KeAcquireSpinLock(&ClassDeviceExtension->SpinLock, &OldIrql);

      if (ClassDeviceExtension->InputCount + InputCount > MOUSE_BUFFER_SIZE)
      {
         ReadSize = MOUSE_BUFFER_SIZE - ClassDeviceExtension->InputCount;
      } else {
         ReadSize = InputCount;
      }

      /*
       * FIXME: If we exceed the buffer, mouse data gets thrown away.. better
       * solution?
       */

      /*
       * Move the mouse input data from the port data queue to our class data
       * queue.
       */
      RtlMoveMemory(ClassDeviceExtension->PortData, (PCHAR)MouseDataStart,
                    sizeof(MOUSE_INPUT_DATA) * ReadSize);

      /* Move the pointer and counter up */
      ClassDeviceExtension->PortData += ReadSize;
      ClassDeviceExtension->InputCount += ReadSize;

      KeReleaseSpinLock(&ClassDeviceExtension->SpinLock, OldIrql);
      (*ConsumedCount) += ReadSize;
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
   UNICODE_STRING PortName = ROS_STRING_INITIALIZER(L"\\Device\\PointerClass0");
   IO_STATUS_BLOCK ioStatus;
   KEVENT event;
   PIRP irp;
   CLASS_INFORMATION ClassInformation;
   PDEVICE_EXTENSION DeviceExtension = ClassDeviceObject->DeviceExtension;

   // Get the port driver's DeviceObject
   // FIXME: The name might change.. find a way to be more dynamic?

   status = IoGetDeviceObjectPointer(&PortName, FILE_READ_ATTRIBUTES, &FileObject, &PortDeviceObject);

   if(status != STATUS_SUCCESS)
   {
      DPRINT("MOUCLASS: Could not connect to mouse port driver\n");
      DPRINT("Status: %x\n", status);
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
         Status = STATUS_SUCCESS;
         break;

      case IRP_MJ_CLOSE:
         Status = STATUS_SUCCESS;
         break;

      case IRP_MJ_READ:
         if (Stack->Parameters.Read.Length < sizeof(MOUSE_INPUT_DATA))
         {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
         }
         IoMarkIrpPending(Irp);
         IoStartPacket(DeviceObject, Irp, NULL, NULL);
	 return STATUS_PENDING;

      default:
         DPRINT1("NOT IMPLEMENTED\n");
         Status = STATUS_NOT_IMPLEMENTED;
         break;
   }

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return Status;
}

VOID STDCALL
MouseClassStartIo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);

   if (DeviceExtension->InputCount > 0)
   {
      KIRQL oldIrql;

      KeAcquireSpinLock(&DeviceExtension->SpinLock, &oldIrql);

      RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer,
                    DeviceExtension->PortData - DeviceExtension->InputCount,
                    sizeof(MOUSE_INPUT_DATA));

      if (DeviceExtension->InputCount > 1)
      {
         RtlMoveMemory(
            DeviceExtension->PortData - DeviceExtension->InputCount,
            DeviceExtension->PortData - DeviceExtension->InputCount + 1,
            (DeviceExtension->InputCount - 1) * sizeof(MOUSE_INPUT_DATA));
      }
      DeviceExtension->PortData--;
      DeviceExtension->InputCount--;
      DeviceExtension->ReadIsPending = FALSE;

      /* Go to next packet and complete this request with STATUS_SUCCESS */
      Irp->IoStatus.Status = STATUS_SUCCESS;
      Irp->IoStatus.Information = sizeof(MOUSE_INPUT_DATA);
      Stack->Parameters.Read.Length = sizeof(MOUSE_INPUT_DATA);
      IoCompleteRequest(Irp, IO_MOUSE_INCREMENT);

      IoStartNextPacket(DeviceObject, FALSE);
      KeReleaseSpinLock(&DeviceExtension->SpinLock, oldIrql);
   } else {
      DeviceExtension->ReadIsPending = TRUE;
   }
}

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
   PDEVICE_OBJECT DeviceObject;
   UNICODE_STRING DeviceName = ROS_STRING_INITIALIZER(L"\\Device\\Mouse");
   UNICODE_STRING SymlinkName = ROS_STRING_INITIALIZER(L"\\??\\Mouse");
   NTSTATUS Status;

   DriverObject->MajorFunction[IRP_MJ_CREATE] = MouseClassDispatch;
   DriverObject->MajorFunction[IRP_MJ_CLOSE]  = MouseClassDispatch;
   DriverObject->MajorFunction[IRP_MJ_READ]   = MouseClassDispatch;
   DriverObject->DriverStartIo                = MouseClassStartIo;

   Status = IoCreateDevice(DriverObject,
			   sizeof(DEVICE_EXTENSION),
			   &DeviceName,
			   FILE_DEVICE_MOUSE,
			   0,
			   TRUE,
			   &DeviceObject);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   DeviceObject->Flags = DeviceObject->Flags | DO_BUFFERED_IO;

   Status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
   if (!NT_SUCCESS(Status))
   {
      IoDeleteDevice(DeviceObject);
      return Status;
   }

   Status = ConnectMousePortDriver(DeviceObject);
   if (!NT_SUCCESS(Status))
   {
      IoDeleteSymbolicLink(&SymlinkName);
      IoDeleteDevice(DeviceObject);
      return Status;
   }

   return STATUS_SUCCESS;
}
