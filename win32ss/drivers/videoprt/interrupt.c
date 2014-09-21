/*
 * VideoPort driver
 *
 * Copyright (C) 2002, 2003, 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "videoprt.h"

#include <ndk/halfuncs.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

BOOLEAN NTAPI
IntVideoPortInterruptRoutine(
   IN struct _KINTERRUPT *Interrupt,
   IN PVOID ServiceContext)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension = ServiceContext;

   ASSERT(DeviceExtension->DriverExtension->InitializationData.HwInterrupt != NULL);

   return DeviceExtension->DriverExtension->InitializationData.HwInterrupt(
      &DeviceExtension->MiniPortDeviceExtension);
}

BOOLEAN NTAPI
IntVideoPortSetupInterrupt(
   IN PDEVICE_OBJECT DeviceObject,
   IN PVIDEO_PORT_DRIVER_EXTENSION DriverExtension,
   IN PVIDEO_PORT_CONFIG_INFO ConfigInfo)
{
   NTSTATUS Status;
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

   DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

   /*
    * MSDN documentation for VIDEO_PORT_CONFIG_INFO states: "If a miniport driver's
    * HwVidFindAdapter function finds that the video adapter does not generate
    * interrupts or that it cannot determine a valid interrupt vector/level for
    * the adapter, HwVidFindAdapter should set both BusInterruptVector and
    * BusInterruptLevel to zero.
    */

   if (DriverExtension->InitializationData.HwInterrupt != NULL &&
       (ConfigInfo->BusInterruptLevel != 0 ||
       ConfigInfo->BusInterruptVector != 0))
   {
      ULONG InterruptVector;
      KIRQL Irql;
      KAFFINITY Affinity;

      InterruptVector = HalGetInterruptVector(
         ConfigInfo->AdapterInterfaceType,
         ConfigInfo->SystemIoBusNumber,
         ConfigInfo->BusInterruptLevel,
         ConfigInfo->BusInterruptVector,
         &Irql,
         &Affinity);

      if (InterruptVector == 0)
      {
         WARN_(VIDEOPRT, "HalGetInterruptVector failed\n");
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
         DeviceExtension->InterruptShared,
         Affinity,
         FALSE);

      if (!NT_SUCCESS(Status))
      {
         WARN_(VIDEOPRT, "IoConnectInterrupt failed with status 0x%08x\n", Status);
         return FALSE;
      }
   }

   return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
VP_STATUS
NTAPI
VideoPortEnableInterrupt(IN PVOID HwDeviceExtension)
{
#ifndef _M_AMD64
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    BOOLEAN InterruptValid;

    /* Get the device extension */
    DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

    /* Fail if the driver didn't register an ISR */
    if (!DeviceExtension->DriverExtension->InitializationData.HwInterrupt)
    {
        /* No ISR, no interrupts */
        return ERROR_INVALID_FUNCTION;
    }

    /* Re-enable the interrupt and return */
    InterruptValid = HalEnableSystemInterrupt(DeviceExtension->InterruptVector,
                                              0,
                                              DeviceExtension->InterruptLevel);

    /* Make sure the interrupt was valid */
    ASSERT(InterruptValid == TRUE);

    /* Return to caller */
    return NO_ERROR;
#else
    /* FIXME: Function still present? If so what to use instead of HalEnableSystemInterrupt? */
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
#endif
}

/*
 * @implemented
 */
VP_STATUS
NTAPI
VideoPortDisableInterrupt(IN PVOID HwDeviceExtension)
{
#ifndef _M_AMD64
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

    /* Get the device extension */
    DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

    /* Fail if the driver didn't register an ISR */
    if (!DeviceExtension->DriverExtension->InitializationData.HwInterrupt)
    {
        /* No ISR, no interrupts */
        return ERROR_INVALID_FUNCTION;
    }

    /* Disable the interrupt and return */
    HalDisableSystemInterrupt(DeviceExtension->InterruptVector, 0);
    return NO_ERROR;
#else
    /* FIXME: Function still present? If so what to use instead of HalDisableSystemInterrupt? */
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
#endif
}
