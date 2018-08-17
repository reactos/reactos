/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/dispatch.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

LONG TimerDpcCount;

/* FUNCTIONS ******************************************************************/

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
    HEADLESS_RSP_GET_BYTE ByteValue;
    SIZE_T ValueSize;
    BOOLEAN GotChar;
    NTSTATUS Status;
    PSAC_DEVICE_EXTENSION SacExtension;

    /* Update our counter */
    _InterlockedExchangeAdd(&TimerDpcCount, 1);

    /* Set defaults and loop for new characters */
    GotChar = FALSE;
    ValueSize = sizeof(ByteValue);
    do
    {
        /* Ask the kernel for a byte */
        Status = HeadlessDispatch(HeadlessCmdGetByte,
                                  NULL,
                                  0,
                                  &ByteValue,
                                  &ValueSize);

        /* Break out if there's nothing interesting */
        if (!NT_SUCCESS(Status)) break;
        if (!ByteValue.Value) break;

        /* Update the serial port buffer */
        SerialPortBuffer[SerialPortProducerIndex] = ByteValue.Value;
        GotChar = TRUE;

        /* Update the index, let it roll-over if needed */
        _InterlockedExchange(&SerialPortProducerIndex,
                             (SerialPortProducerIndex + 1) &
                             (SAC_SERIAL_PORT_BUFFER_SIZE - 1));
    } while (ByteValue.Value);

    /* Did we get anything */
    if (GotChar)
    {
        /* Signal the worker thread that there is work to do */
        SacExtension = DeferredContext;
        KeSetEvent(&SacExtension->Event, SacExtension->PriorityBoost, FALSE);
    }
}

VOID
NTAPI
UnloadHandler(IN PDRIVER_OBJECT DriverObject)
{
    PDEVICE_OBJECT DeviceObject, NextDevice;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC UnloadHandler: Entering.\n");

    /* Go over every device part of the driver */
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
