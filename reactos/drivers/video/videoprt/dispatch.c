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
 * $Id: dispatch.c,v 1.1.2.6 2004/03/15 20:21:50 navaraf Exp $
 */

#include "videoprt.h"

typedef PVOID PHAL_RESET_DISPLAY_PARAMETERS;
VOID STDCALL HalAcquireDisplayOwnership(IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters);
VOID STDCALL HalReleaseDisplayOwnership();

/* GLOBAL VARIABLES ***********************************************************/

PVIDEO_PORT_DEVICE_EXTENSION ResetDisplayParametersDeviceExtension = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

VOID STDCALL
VideoPortDeferredRoutine(
   IN PKDPC Dpc,
   IN PVOID DeferredContext,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2)
{
   PVOID HwDeviceExtension = 
      ((PVIDEO_PORT_DEVICE_EXTENSION)DeferredContext)->MiniPortDeviceExtension;
   ((PMINIPORT_DPC_ROUTINE)SystemArgument1)(HwDeviceExtension, SystemArgument2);
}

/*
 * Reset display to blue screen
 */

BOOLEAN STDCALL
VideoPortResetDisplayParameters(ULONG Columns, ULONG Rows)
{
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;

   if (ResetDisplayParametersDeviceExtension == NULL)
      return FALSE;

   DriverExtension = IoGetDriverObjectExtension(
      ResetDisplayParametersDeviceExtension->FunctionalDeviceObject->DriverObject,
      ResetDisplayParametersDeviceExtension->FunctionalDeviceObject->DriverObject);

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
VideoPortAddDevice(
   IN PDRIVER_OBJECT DriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject)
{
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;
   ULONG DeviceNumber;
   ULONG Size;
   NTSTATUS Status;
   VIDEO_PORT_CONFIG_INFO ConfigInfo;
   SYSTEM_BASIC_INFORMATION SystemBasicInfo;
   UCHAR Again = FALSE;
   WCHAR DeviceBuffer[20];
   UNICODE_STRING DeviceName;
   WCHAR SymlinkBuffer[20];
   UNICODE_STRING SymlinkName;
   PDEVICE_OBJECT DeviceObject;
   WCHAR DeviceVideoBuffer[20];
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

   /*
    * Get the initialization data we saved in VideoPortInitialize.
    */

   DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);

   /*
    * Find the first free device number that can be used for video device
    * object names and symlinks.
    */

   for (DeviceNumber = 0;;)
   {
      OBJECT_ATTRIBUTES Obj;
      HANDLE ObjHandle;

      swprintf(SymlinkBuffer, L"\\??\\DISPLAY%lu", DeviceNumber + 1);
      RtlInitUnicodeString(&SymlinkName, SymlinkBuffer);
      InitializeObjectAttributes(&Obj, &SymlinkName, 0, NULL, NULL);
      Status = ZwOpenSymbolicLinkObject(&ObjHandle, GENERIC_READ, &Obj);
      if (NT_SUCCESS(Status))
      {
         ZwClose(ObjHandle);
         DeviceNumber++;
         continue;
      }
      else if (Status == STATUS_NOT_FOUND || Status == STATUS_UNSUCCESSFUL)
         break;
      else
         return Status;
   }

   /*
    * Initialize the configuration information structures passed
    * to miniport HwVidFindAdapter.
    */

   RtlZeroMemory(&ConfigInfo, sizeof(VIDEO_PORT_CONFIG_INFO));
   ConfigInfo.Length = sizeof(VIDEO_PORT_CONFIG_INFO);

   ConfigInfo.AdapterInterfaceType = 
      DriverExtension->InitializationData.AdapterInterfaceType;

   if (ConfigInfo.AdapterInterfaceType == PCIBus)
      ConfigInfo.InterruptMode = LevelSensitive;
   else
      ConfigInfo.InterruptMode = Latched;

   ConfigInfo.DriverRegistryPath = DriverExtension->RegistryPath.Buffer;
   ConfigInfo.VideoPortGetProcAddress = VideoPortGetProcAddress;

   /* Get bus number from the upper level bus driver. */
   Size = sizeof(ULONG);
   IoGetDeviceProperty(
      PhysicalDeviceObject,
      DevicePropertyBusNumber,
      Size,
      &ConfigInfo.SystemIoBusNumber,
      &Size);

   Size = sizeof(SystemBasicInfo);
   Status = ZwQuerySystemInformation(
      SystemBasicInformation,
      &SystemBasicInfo,
      Size,
      &Size);

   if (NT_SUCCESS(Status))
   {
      ConfigInfo.SystemMemorySize =
         SystemBasicInfo.NumberOfPhysicalPages * 
         SystemBasicInfo.PhysicalPageSize;
   }
   
   /*
    * The device was found, create the Io device object, symlinks, ...
    */

   /* Create a unicode device name. */
   swprintf(DeviceBuffer, L"\\Device\\Video%lu", DeviceNumber);
   RtlInitUnicodeString(&DeviceName, DeviceBuffer);

   /* Create the device object. */
   Status = IoCreateDevice(
      DriverObject,
      sizeof(VIDEO_PORT_DEVICE_EXTENSION) +
      DriverExtension->InitializationData.HwDeviceExtensionSize,
      &DeviceName,
      FILE_DEVICE_VIDEO,
      0,
      TRUE,
      &DeviceObject);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("IoCreateDevice call failed with status 0x%08x\n", Status);
      return Status;
   }

   DriverObject->DeviceObject = DeviceObject;

   /* 
    * Set the buffering strategy here. If you change this, remember
    * to change VidDispatchDeviceControl too.
    */

   DeviceObject->Flags |= DO_BUFFERED_IO;

   /*
    * Initialize device extension.
    */

   DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   DeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
   DeviceExtension->FunctionalDeviceObject = DeviceObject;
   DeviceExtension->SystemIoBusNumber = ConfigInfo.SystemIoBusNumber;
   DeviceExtension->AdapterInterfaceType = 
      DriverExtension->InitializationData.AdapterInterfaceType;

   /* Get bus device address from the upper level bus driver. */
   Size = sizeof(ULONG);
   IoGetDeviceProperty(
      PhysicalDeviceObject,
      DevicePropertyAddress,
      Size,
      &DeviceExtension->SystemIoSlotNumber,
      &Size);   

   InitializeListHead(&DeviceExtension->AddressMappingListHead);
   KeInitializeDpc(
      &DeviceExtension->DpcObject,
      VideoPortDeferredRoutine,
      DeviceExtension);

   DeviceExtension->RegistryPath.Length = 
   DeviceExtension->RegistryPath.MaximumLength = 
      DriverExtension->RegistryPath.Length + (8 * sizeof(WCHAR));
   DeviceExtension->RegistryPath.Buffer = ExAllocatePoolWithTag(
      PagedPool,
      DeviceExtension->RegistryPath.MaximumLength,
      TAG_VIDEO_PORT);      
   swprintf(DeviceExtension->RegistryPath.Buffer, L"%s\\Device0",
      DriverExtension->RegistryPath.Buffer);

   /*
    * Call miniport HwVidFindAdapter entry point to detect if
    * particular device is present.
    */
    
   /* FIXME: Need to figure out what string to pass as param 3. */
   Status = DriverExtension->InitializationData.HwFindAdapter(
      &DeviceExtension->MiniPortDeviceExtension,
      DriverExtension->HwContext,
      NULL,
      &ConfigInfo,
      &Again);

   if (Status != NO_ERROR)
   {
      DPRINT("HwFindAdapter call failed with error %X\n", Status);
      IoDeleteDevice(DeviceObject);

      return Status;
   }

   /* Create symbolic link "\??\DISPLAYx" */
   swprintf(SymlinkBuffer, L"\\??\\DISPLAY%lu", DeviceNumber + 1);
   RtlInitUnicodeString(&SymlinkName, SymlinkBuffer);
   IoCreateSymbolicLink(&SymlinkName, &DeviceName);

   /* Add entry to DEVICEMAP\VIDEO key in registry. */
   swprintf(DeviceVideoBuffer, L"\\Device\\Video%d", DeviceNumber);
   RtlWriteRegistryValue(
      RTL_REGISTRY_DEVICEMAP,
      L"VIDEO",
      DeviceVideoBuffer,
      REG_SZ,
      DeviceExtension->RegistryPath.Buffer,
      DeviceExtension->RegistryPath.Length + sizeof(WCHAR));

   /* FIXME: Allocate hardware resources for device. */

   /*
    * Allocate interrupt for device.
    */

   if ((ConfigInfo.BusInterruptVector != 0 ||
        ConfigInfo.BusInterruptLevel != 0) &&
       DriverExtension->InitializationData.HwInterrupt != NULL)
   {
      ULONG InterruptVector;
      KIRQL Irql;
      KAFFINITY Affinity;

      if (ConfigInfo.BusInterruptVector != 0)
         DeviceExtension->InterruptVector = ConfigInfo.BusInterruptVector;

      if (ConfigInfo.BusInterruptLevel != 0)
         DeviceExtension->InterruptLevel = ConfigInfo.BusInterruptLevel;

      InterruptVector = HalGetInterruptVector(
         ConfigInfo.AdapterInterfaceType,
         ConfigInfo.SystemIoBusNumber,
         ConfigInfo.BusInterruptLevel,
         ConfigInfo.BusInterruptVector,
         &Irql,
         &Affinity);

      if (InterruptVector == 0)
      {
         DPRINT("HalGetInterruptVector failed\n");
         IoDeleteDevice(DeviceObject);
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      KeInitializeSpinLock(&DeviceExtension->InterruptSpinLock);
      Status = IoConnectInterrupt(
         &DeviceExtension->InterruptObject,
         VideoPortInterruptRoutine,
         DeviceExtension,
         &DeviceExtension->InterruptSpinLock,
         InterruptVector,
         Irql,
         Irql,
         ConfigInfo.InterruptMode,
         FALSE,
         Affinity,
         FALSE);

      if (!NT_SUCCESS(Status))
      {
         DPRINT("IoConnectInterrupt failed with status 0x%08x\n", Status);
         IoDeleteDevice(DeviceObject);
              
         return Status;
      }
   }

   /*
    * Allocate timer for device.
    */

   if (DriverExtension->InitializationData.HwTimer != NULL)
   {
      DPRINT("Initializing timer\n");

      Status = IoInitializeTimer(
         DeviceObject,
         VideoPortTimerRoutine,
         DeviceExtension);

      if (!NT_SUCCESS(Status))
      {
         DPRINT("IoInitializeTimer failed with status 0x%08x\n", Status);
          
         if (DriverExtension->InitializationData.HwInterrupt != NULL)
            IoDisconnectInterrupt(DeviceExtension->InterruptObject);

         IoDeleteDevice(DeviceObject);
         return Status;
      }
   }

   IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);

   return STATUS_SUCCESS;
}

/*
 * VideoPortDispatchOpen
 *
 * Answer requests for Open calls.
 *
 * Run Level
 *    PASSIVE_LEVEL
 */

NTSTATUS STDCALL
VideoPortDispatchOpen(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;

   DPRINT("VidDispatchOpen\n");

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

   DriverExtension = IoGetDriverObjectExtension(
      DeviceObject->DriverObject,
      DeviceObject->DriverObject);
   DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

   if (DriverExtension->InitializationData.HwInitialize(DeviceExtension->MiniPortDeviceExtension))
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
      HalAcquireDisplayOwnership(VideoPortResetDisplayParameters);
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
 * VideoPortDispatchClose
 *
 * Answer requests for Close calls.
 *
 * Run Level
 *    PASSIVE_LEVEL
 */

NTSTATUS STDCALL
VideoPortDispatchClose(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   DPRINT("VidDispatchClose\n");

   if (ResetDisplayParametersDeviceExtension != NULL)
      HalReleaseDisplayOwnership();

   Irp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return STATUS_SUCCESS;
}

/*
 * VidDispatchDeviceControl
 *
 * Answer requests for device control calls.
 *
 * Run Level
 *    PASSIVE_LEVEL
 */

NTSTATUS STDCALL
VideoPortDispatchDeviceControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   PIO_STACK_LOCATION IrpStack;
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   PVIDEO_REQUEST_PACKET vrp;
   NTSTATUS Status;

   DPRINT("VidDispatchDeviceControl\n");
   IrpStack = IoGetCurrentIrpStackLocation(Irp);
   DeviceExtension = DeviceObject->DeviceExtension;
   DriverExtension = IoGetDriverObjectExtension(
      DeviceObject->DriverObject,
      DeviceObject->DriverObject);

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
      (PVOID)&DeviceExtension->MiniPortDeviceExtension,
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
VideoPortDispatchPnp(
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
         return STATUS_SUCCESS;
   }
   
   return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS STDCALL
VideoPortDispatchPower(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp)
{
   return STATUS_NOT_IMPLEMENTED;
}

VOID STDCALL
VideoPortUnload(PDRIVER_OBJECT DriverObject)
{
}
