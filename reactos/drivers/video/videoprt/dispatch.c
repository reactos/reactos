/*
 * VideoPort driver
 *
 * Copyright (C) 2002, 2003, 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "videoprt.h"

/* GLOBAL VARIABLES ***********************************************************/

PVIDEO_PORT_DEVICE_EXTENSION ResetDisplayParametersDeviceExtension = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * Reset display to blue screen
 */

BOOLEAN NTAPI
IntVideoPortResetDisplayParameters(ULONG Columns, ULONG Rows)
{
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;

   if (ResetDisplayParametersDeviceExtension == NULL)
      return FALSE;

   DriverExtension = ResetDisplayParametersDeviceExtension->DriverExtension;

   if (DriverExtension->InitializationData.HwResetHw != NULL)
   {
      if (DriverExtension->InitializationData.HwResetHw(
             &ResetDisplayParametersDeviceExtension->MiniPortDeviceExtension,
             Columns, Rows))
      {
         ResetDisplayParametersDeviceExtension = NULL;
         return TRUE;
      }
   }

   ResetDisplayParametersDeviceExtension = NULL;
   return FALSE;
}

NTSTATUS NTAPI
IntVideoPortAddDevice(
   IN PDRIVER_OBJECT DriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject)
{
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;
   PDEVICE_OBJECT DeviceObject;
   NTSTATUS Status;

   /*
    * Get the initialization data we saved in VideoPortInitialize.
    */

   DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);

   /*
    * Create adapter device object.
    */

   Status = IntVideoPortCreateAdapterDeviceObject(
      DriverObject,
      DriverExtension,
      PhysicalDeviceObject,
      &DeviceObject);

   return Status;
}

/*
 * IntVideoPortDispatchOpen
 *
 * Answer requests for Open calls.
 *
 * Run Level
 *    PASSIVE_LEVEL
 */

NTSTATUS NTAPI
IntVideoPortDispatchOpen(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;

   TRACE_(VIDEOPRT, "IntVideoPortDispatchOpen\n");

   if (CsrssInitialized == FALSE)
   {
      /*
       * We know the first open call will be from the CSRSS process
       * to let us know its handle.
       */

      INFO_(VIDEOPRT, "Referencing CSRSS\n");
      Csrss = (PKPROCESS)PsGetCurrentProcess();
      INFO_(VIDEOPRT, "Csrss %p\n", Csrss);

      CsrssInitialized = TRUE;

      Irp->IoStatus.Information = FILE_OPENED;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return STATUS_SUCCESS;
   }

   DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   DriverExtension = DeviceExtension->DriverExtension;

   if (DriverExtension->InitializationData.HwInitialize(&DeviceExtension->MiniPortDeviceExtension))
   {
      Irp->IoStatus.Status = STATUS_SUCCESS;

      InterlockedIncrement((PLONG)&DeviceExtension->DeviceOpened);
   }
   else
   {
      Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
   }

   Irp->IoStatus.Information = FILE_OPENED;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return STATUS_SUCCESS;
}

/*
 * IntVideoPortDispatchClose
 *
 * Answer requests for Close calls.
 *
 * Run Level
 *    PASSIVE_LEVEL
 */

NTSTATUS NTAPI
IntVideoPortDispatchClose(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

   TRACE_(VIDEOPRT, "IntVideoPortDispatchClose\n");

   DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   if (DeviceExtension->DeviceOpened >= 1 &&
       InterlockedDecrement((PLONG)&DeviceExtension->DeviceOpened) == 0)
   {
      ResetDisplayParametersDeviceExtension = NULL;
      InbvNotifyDisplayOwnershipLost(NULL);
      ResetDisplayParametersDeviceExtension = DeviceExtension;
      IntVideoPortResetDisplayParameters(80, 50);
   }

   Irp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return STATUS_SUCCESS;
}

/*
 * IntVideoPortDispatchDeviceControl
 *
 * Answer requests for device control calls.
 *
 * Run Level
 *    PASSIVE_LEVEL
 */

NTSTATUS NTAPI
IntVideoPortDispatchDeviceControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   PIO_STACK_LOCATION IrpStack;
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   PVIDEO_REQUEST_PACKET vrp;
   NTSTATUS Status;

   TRACE_(VIDEOPRT, "IntVideoPortDispatchDeviceControl\n");

   IrpStack = IoGetCurrentIrpStackLocation(Irp);
   DeviceExtension = DeviceObject->DeviceExtension;
   DriverExtension = DeviceExtension->DriverExtension;

   /* Translate the IRP to a VRP */
   vrp = ExAllocatePool(NonPagedPool, sizeof(VIDEO_REQUEST_PACKET));
   if (NULL == vrp)
   {
      return STATUS_NO_MEMORY;
   }

   vrp->StatusBlock = (PSTATUS_BLOCK)&(Irp->IoStatus);
   vrp->IoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;

   INFO_(VIDEOPRT, "- IoControlCode: %x\n", vrp->IoControlCode);

   /* We're assuming METHOD_BUFFERED */
   vrp->InputBuffer = Irp->AssociatedIrp.SystemBuffer;
   vrp->InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
   vrp->OutputBuffer = Irp->AssociatedIrp.SystemBuffer;
   vrp->OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

   /* Call the Miniport Driver with the VRP */
   DriverExtension->InitializationData.HwStartIO(
      &DeviceExtension->MiniPortDeviceExtension,
      vrp);

   /* Free the VRP */
   ExFreePool(vrp);

   INFO_(VIDEOPRT, "- Returned status: %x\n", Irp->IoStatus.Status);

   if (Irp->IoStatus.Status != STATUS_SUCCESS)
   {
      /* Map from win32 error codes to NT status values. */
      switch (Irp->IoStatus.Status)
      {
         case ERROR_NOT_ENOUGH_MEMORY: Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES; break;
         case ERROR_MORE_DATA: Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW; break;
         case ERROR_INVALID_FUNCTION: Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED; break;
         case ERROR_INVALID_PARAMETER: Irp->IoStatus.Status = STATUS_INVALID_PARAMETER; break;
         case ERROR_INSUFFICIENT_BUFFER: Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL; break;
         case ERROR_DEV_NOT_EXIST: Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST; break;
         case ERROR_IO_PENDING: Irp->IoStatus.Status = STATUS_PENDING; break;
      }
   }

   Status = Irp->IoStatus.Status;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return Status;
}

/*
 * IntVideoPortWrite
 *
 * This is a bit of a hack. We want to take ownership of the display as late
 * as possible, just before the switch to graphics mode. Win32k knows when
 * this happens, we don't. So we need Win32k to inform us. This could be done
 * using an IOCTL, but there's no way of knowing which IOCTL codes are unused
 * in the communication between GDI driver and miniport driver. So we use
 * IRP_MJ_WRITE as the signal that win32k is ready to switch to graphics mode,
 * since we know for certain that there is no read/write activity going on
 * between GDI and miniport drivers.
 * We don't actually need the data that is passed, we just trigger on the fact
 * that an IRP_MJ_WRITE was sent.
 *
 * Run Level
 *    PASSIVE_LEVEL
 */

NTSTATUS NTAPI
IntVideoPortDispatchWrite(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   PIO_STACK_LOCATION piosStack = IoGetCurrentIrpStackLocation(Irp);
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   NTSTATUS nErrCode;

   DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

   /*
    * Storing the device extension pointer in a static variable is an
    * ugly hack. Unfortunately, we need it in IntVideoPortResetDisplayParameters
    * and InbvNotifyDisplayOwnershipLost doesn't allow us to pass a userdata
    * parameter. On the bright side, the DISPLAY device is opened
    * exclusively, so there can be only one device extension active at
    * any point in time.
    *
    * FIXME: We should process all opened display devices in
    * IntVideoPortResetDisplayParameters.
    */

   ResetDisplayParametersDeviceExtension = DeviceExtension;
   InbvNotifyDisplayOwnershipLost(IntVideoPortResetDisplayParameters);

   nErrCode = STATUS_SUCCESS;
   Irp->IoStatus.Information = piosStack->Parameters.Write.Length;
   Irp->IoStatus.Status = nErrCode;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return nErrCode;
}


NTSTATUS NTAPI
IntVideoPortPnPStartDevice(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PDRIVER_OBJECT DriverObject;
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   PCM_RESOURCE_LIST AllocatedResources;

   /*
    * Get the initialization data we saved in VideoPortInitialize.
    */

   DriverObject = DeviceObject->DriverObject;
   DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
   DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

   /*
    * Store some resources in the DeviceExtension.
    */

   AllocatedResources = Stack->Parameters.StartDevice.AllocatedResources;
   if (AllocatedResources != NULL)
   {
      CM_FULL_RESOURCE_DESCRIPTOR *FullList;
      CM_PARTIAL_RESOURCE_DESCRIPTOR *Descriptor;
      ULONG ResourceCount;
      ULONG ResourceListSize;

      /* Save the resource list */
      ResourceCount = AllocatedResources->List[0].PartialResourceList.Count;
      ResourceListSize =
         FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.
                      PartialDescriptors[ResourceCount]);
      DeviceExtension->AllocatedResources = ExAllocatePool(PagedPool, ResourceListSize);
      if (DeviceExtension->AllocatedResources == NULL)
      {
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      RtlCopyMemory(DeviceExtension->AllocatedResources,
                    AllocatedResources,
                    ResourceListSize);

      /* Get the interrupt level/vector - needed by HwFindAdapter sometimes */
      for (FullList = AllocatedResources->List;
           FullList < AllocatedResources->List + AllocatedResources->Count;
           FullList++)
      {
         /* FIXME: Is this ASSERT ok for resources from the PNP manager? */
         ASSERT(FullList->InterfaceType == PCIBus &&
                FullList->BusNumber == DeviceExtension->SystemIoBusNumber &&
                1 == FullList->PartialResourceList.Version &&
                1 == FullList->PartialResourceList.Revision);
	 for (Descriptor = FullList->PartialResourceList.PartialDescriptors;
              Descriptor < FullList->PartialResourceList.PartialDescriptors + FullList->PartialResourceList.Count;
              Descriptor++)
         {
            if (Descriptor->Type == CmResourceTypeInterrupt)
            {
               DeviceExtension->InterruptLevel = Descriptor->u.Interrupt.Level;
               DeviceExtension->InterruptVector = Descriptor->u.Interrupt.Vector;
               if (Descriptor->ShareDisposition == CmResourceShareShared)
                  DeviceExtension->InterruptShared = TRUE;
               else
                  DeviceExtension->InterruptShared = FALSE;
            }
         }
      }
   }
   INFO_(VIDEOPRT, "Interrupt level: 0x%x Interrupt Vector: 0x%x\n",
          DeviceExtension->InterruptLevel,
          DeviceExtension->InterruptVector);

   /*
    * Create adapter device object.
    */

   return IntVideoPortFindAdapter(
      DriverObject,
      DriverExtension,
      DeviceObject);
}


NTSTATUS
NTAPI
IntVideoPortForwardIrpAndWaitCompletionRoutine(
    PDEVICE_OBJECT Fdo,
    PIRP Irp,
    PVOID Context)
{
  PKEVENT Event = Context;

  if (Irp->PendingReturned)
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

  return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
NTAPI
IntVideoPortForwardIrpAndWait(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   KEVENT Event;
   NTSTATUS Status;
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension =
                   (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

   KeInitializeEvent(&Event, NotificationEvent, FALSE);
   IoCopyCurrentIrpStackLocationToNext(Irp);
   IoSetCompletionRoutine(Irp, IntVideoPortForwardIrpAndWaitCompletionRoutine,
                          &Event, TRUE, TRUE, TRUE);
   Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
   if (Status == STATUS_PENDING)
   {
      KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
      Status = Irp->IoStatus.Status;
   }
   return Status;
}


NTSTATUS NTAPI
IntVideoPortDispatchPnp(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   PIO_STACK_LOCATION IrpSp;
   NTSTATUS Status;

   IrpSp = IoGetCurrentIrpStackLocation(Irp);

   switch (IrpSp->MinorFunction)
   {
      case IRP_MN_START_DEVICE:
         Status = IntVideoPortForwardIrpAndWait(DeviceObject, Irp);
         if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
            Status = IntVideoPortPnPStartDevice(DeviceObject, Irp);
         Irp->IoStatus.Status = Status;
         Irp->IoStatus.Information = 0;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         break;


      case IRP_MN_REMOVE_DEVICE:
      case IRP_MN_QUERY_REMOVE_DEVICE:
      case IRP_MN_CANCEL_REMOVE_DEVICE:
      case IRP_MN_SURPRISE_REMOVAL:

      case IRP_MN_STOP_DEVICE:
         Status = IntVideoPortForwardIrpAndWait(DeviceObject, Irp);
         if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
            Status = STATUS_SUCCESS;
         Irp->IoStatus.Status = Status;
         Irp->IoStatus.Information = 0;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         break;

      case IRP_MN_QUERY_STOP_DEVICE:
      case IRP_MN_CANCEL_STOP_DEVICE:
         Status = STATUS_SUCCESS;
         Irp->IoStatus.Status = STATUS_SUCCESS;
         Irp->IoStatus.Information = 0;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         break;

      default:
         Status = Irp->IoStatus.Status;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         break;
   }

   return Status;
}

NTSTATUS NTAPI
IntVideoPortDispatchCleanup(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

   DeviceExtension = DeviceObject->DeviceExtension;
   RtlFreeUnicodeString(&DeviceExtension->RegistryPath);

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return STATUS_SUCCESS;
}

NTSTATUS NTAPI
IntVideoPortDispatchPower(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI
IntVideoPortDispatchSystemControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   return STATUS_NOT_IMPLEMENTED;
}

VOID NTAPI
IntVideoPortUnload(PDRIVER_OBJECT DriverObject)
{
}
