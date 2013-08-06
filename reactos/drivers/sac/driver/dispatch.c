/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/dispatch.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include "sacdrv.h"

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
DispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
DispatchShutdownControl(IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
DispatchCreate(IN PSAC_DEVICE_EXTENSION DeviceExtension,
               IN PIRP Irp)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
DispatchClose(IN PSAC_DEVICE_EXTENSION DeviceExtension,
              IN PIRP Irp)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
Dispatch(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp)
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
TimerDpcRoutine(IN PKDPC Dpc,
                IN PVOID DeferredContext,
                IN PVOID SystemArgument1,
                IN PVOID SystemArgument2)
{

}

VOID
NTAPI
UnloadHandler(IN PDRIVER_OBJECT DriverObject)
{
    PDEVICE_OBJECT DeviceObject, NextDevice;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC UnloadHandler: Entering.\n");

    /* Go overy ever device part of the driver */
    DeviceObject = DriverObject->DeviceObject;
    while (DeviceObject)
    {
        /* Free and delete the information about this device */
        NextDevice = DeviceObject->NextDevice;
        FreeDeviceData(DeviceObject);
        IoDeleteDevice(DeviceObject);

        /* Move on to the next one */
        DeviceObject = NextDevice;
    }

    /* Free the driver data and exit */
    FreeGlobalData();
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC UnloadHandler: Exiting.\n");
}
