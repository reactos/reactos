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
 * $Id$
 */

#include "videoprt.h"

/* EXTERNAL FUNCTIONS *********************************************************/

typedef PVOID PHAL_RESET_DISPLAY_PARAMETERS;
VOID STDCALL HalAcquireDisplayOwnership(IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters);
VOID STDCALL HalReleaseDisplayOwnership();

/* GLOBAL VARIABLES ***********************************************************/

PVIDEO_PORT_DEVICE_EXTENSION ResetDisplayParametersDeviceExtension = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * Reset display to blue screen
 */

BOOLEAN STDCALL
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

NTSTATUS STDCALL
IntVideoPortAddDevice(
   IN PDRIVER_OBJECT DriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject)
{
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;

   /*
    * Get the initialization data we saved in VideoPortInitialize.
    */

   DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);

   /*
    * Create adapter device object.
    */

   return IntVideoPortCreateAdapterDeviceObject(
      DriverObject,
      DriverExtension,
      PhysicalDeviceObject,
      NULL);
}

/*
 * IntVideoPortDispatchOpen
 *
 * Answer requests for Open calls.
 *
 * Run Level
 *    PASSIVE_LEVEL
 */

NTSTATUS STDCALL
IntVideoPortDispatchOpen(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;

   DPRINT("IntVideoPortDispatchOpen\n");

   if (CsrssInitialized == FALSE)
   {
      /*
       * We know the first open call will be from the CSRSS process
       * to let us know its handle.
       */

      DPRINT("Referencing CSRSS\n");
      Csrss = PsGetCurrentProcess();
      DPRINT("Csrss %p\n", Csrss);

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

      /*
       * Storing the device extension pointer in a static variable is an
       * ugly hack. Unfortunately, we need it in VideoPortResetDisplayParameters
       * and HalAcquireDisplayOwnership doesn't allow us to pass a userdata
       * parameter. On the bright side, the DISPLAY device is opened
       * exclusively, so there can be only one device extension active at
       * any point in time.
       */

      ResetDisplayParametersDeviceExtension = DeviceExtension;
      HalAcquireDisplayOwnership(IntVideoPortResetDisplayParameters);
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

NTSTATUS STDCALL
IntVideoPortDispatchClose(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

   DPRINT("IntVideoPortDispatchClose\n");

   DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   if (DeviceExtension->DeviceOpened >= 1 &&
       InterlockedDecrement((PLONG)&DeviceExtension->DeviceOpened) == 0)
   {
      ResetDisplayParametersDeviceExtension = DeviceExtension;
      HalReleaseDisplayOwnership();
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

NTSTATUS STDCALL
IntVideoPortDispatchDeviceControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   PIO_STACK_LOCATION IrpStack;
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   PVIDEO_REQUEST_PACKET vrp;
   NTSTATUS Status;

   DPRINT("IntVideoPortDispatchDeviceControl\n");

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

   DPRINT("- IoControlCode: %x\n", vrp->IoControlCode);

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

   DPRINT("- Returned status: %x\n", Irp->IoStatus.Status);

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

NTSTATUS STDCALL
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
            }
         }
      }
   }
   DPRINT("Interrupt level: 0x%x Interrupt Vector: 0x%x\n",
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
STDCALL
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
STDCALL
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


NTSTATUS STDCALL
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
         return STATUS_NOT_IMPLEMENTED;
         break;
   }
   
   return Status;
}

NTSTATUS STDCALL
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

NTSTATUS STDCALL
IntVideoPortDispatchPower(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   return STATUS_NOT_IMPLEMENTED;
}

VOID STDCALL
IntVideoPortUnload(PDRIVER_OBJECT DriverObject)
{
}
