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
 * $Id: dispatch.c,v 1.2 2004/03/19 20:58:32 navaraf Exp $
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

   ASSERT(DriverExtension->InitializationData.HwResetHw != NULL);

   if (!DriverExtension->InitializationData.HwResetHw(
          &ResetDisplayParametersDeviceExtension->MiniPortDeviceExtension,
          Columns, Rows))
   {
      return FALSE;
   }

   ResetDisplayParametersDeviceExtension = NULL;

   return TRUE;
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
    * Use generic routine to find the adapter and create device object.
    */

   return IntVideoPortFindAdapter(
      DriverObject,
      DriverExtension,
      PhysicalDeviceObject);      
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
   DPRINT("IntVideoPortDispatchClose\n");

   if (ResetDisplayParametersDeviceExtension != NULL)
      HalReleaseDisplayOwnership();

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
IntVideoPortDispatchPnp(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   PIO_STACK_LOCATION IrpSp;

   IrpSp = IoGetCurrentIrpStackLocation(Irp);

   switch (IrpSp->MinorFunction)
   {
      case IRP_MN_START_DEVICE:

      case IRP_MN_REMOVE_DEVICE:
      case IRP_MN_QUERY_REMOVE_DEVICE:
      case IRP_MN_CANCEL_REMOVE_DEVICE:
      case IRP_MN_SURPRISE_REMOVAL:

      case IRP_MN_STOP_DEVICE:
      case IRP_MN_QUERY_STOP_DEVICE:
      case IRP_MN_CANCEL_STOP_DEVICE:
         Irp->IoStatus.Status = STATUS_SUCCESS;
         Irp->IoStatus.Information = 0;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);

         return STATUS_SUCCESS;
   }
   
   return STATUS_NOT_IMPLEMENTED;
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
