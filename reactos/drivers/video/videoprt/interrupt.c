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
 * $Id: interrupt.c,v 1.1.2.2 2004/03/17 20:16:22 navaraf Exp $
 */

#include "videoprt.h"

/* PRIVATE FUNCTIONS **********************************************************/

BOOLEAN STDCALL
IntVideoPortInterruptRoutine(
   IN struct _KINTERRUPT *Interrupt,
   IN PVOID ServiceContext)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension = ServiceContext;
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;

   DriverExtension = IoGetDriverObjectExtension(
      DeviceExtension->FunctionalDeviceObject->DriverObject,
      DeviceExtension->FunctionalDeviceObject->DriverObject);
  
   ASSERT(DriverExtension->InitializationData.HwInterrupt != NULL);

   return DriverExtension->InitializationData.HwInterrupt(
      DeviceExtension->MiniPortDeviceExtension);
}

BOOLEAN STDCALL
IntVideoPortSetupInterrupt(
   IN PDEVICE_OBJECT DeviceObject,
   IN PVIDEO_PORT_DRIVER_EXTENSION DriverExtension,
   IN PVIDEO_PORT_CONFIG_INFO ConfigInfo)
{
   NTSTATUS Status;
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

   DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

   if ((ConfigInfo->BusInterruptVector != 0 ||
        ConfigInfo->BusInterruptLevel != 0) &&
       DriverExtension->InitializationData.HwInterrupt != NULL)
   {
      ULONG InterruptVector;
      KIRQL Irql;
      KAFFINITY Affinity;

      if (ConfigInfo->BusInterruptVector != 0)
         DeviceExtension->InterruptVector = ConfigInfo->BusInterruptVector;

      if (ConfigInfo->BusInterruptLevel != 0)
         DeviceExtension->InterruptLevel = ConfigInfo->BusInterruptLevel;

      InterruptVector = HalGetInterruptVector(
         ConfigInfo->AdapterInterfaceType,
         ConfigInfo->SystemIoBusNumber,
         ConfigInfo->BusInterruptLevel,
         ConfigInfo->BusInterruptVector,
         &Irql,
         &Affinity);

      if (InterruptVector == 0)
      {
         DPRINT("HalGetInterruptVector failed\n");
         return FALSE;
      }

      KeInitializeSpinLock(&DeviceExtension->InterruptSpinLock);
      Status = IoConnectInterrupt(
         &DeviceExtension->InterruptObject,
         IntVideoPortInterruptRoutine,
         DeviceExtension,
         &DeviceExtension->InterruptSpinLock,
         InterruptVector,
         Irql,
         Irql,
         ConfigInfo->InterruptMode,
         FALSE,
         Affinity,
         FALSE);

      if (!NT_SUCCESS(Status))
      {
         DPRINT("IoConnectInterrupt failed with status 0x%08x\n", Status);
         return FALSE;
      }
   }

   return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortEnableInterrupt(IN PVOID HwDeviceExtension)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   BOOLEAN Status;

   DPRINT("VideoPortEnableInterrupt\n");

   DeviceExtension = CONTAINING_RECORD(
      HwDeviceExtension,
      VIDEO_PORT_DEVICE_EXTENSION,
      MiniPortDeviceExtension);

   Status = HalEnableSystemInterrupt(
      DeviceExtension->InterruptVector,
      0,
      DeviceExtension->InterruptLevel);

   return Status ? NO_ERROR : ERROR_INVALID_ACCESS;
}

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortDisableInterrupt(IN PVOID HwDeviceExtension)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   BOOLEAN Status;

   DPRINT("VideoPortDisableInterrupt\n");

   DeviceExtension = CONTAINING_RECORD(
      HwDeviceExtension,
      VIDEO_PORT_DEVICE_EXTENSION,
      MiniPortDeviceExtension);

   Status = HalDisableSystemInterrupt(
      DeviceExtension->InterruptVector,
      0);

   return Status ? NO_ERROR : ERROR_INVALID_ACCESS;
}
