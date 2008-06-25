/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/multimedia/portcls/adapter.c
 * PURPOSE:         Port Class driver / DriverEntry and IRP handlers
 * PROGRAMMER:      Andrew Greenwood
 *
 * HISTORY:
 *                  27 Jan 07   Created
 */

#include "private.h"

/*
    This is called from DriverEntry so that PortCls can take care of some
    IRPs and map some others to the main KS driver. In most cases this will
    be the first function called by an audio driver.

    First 2 parameters are from DriverEntry.

    The AddDevice parameter is a driver-supplied pointer to a function which
    typically then calls PcAddAdapterDevice (see below.)
*/
NTSTATUS NTAPI
PcInitializeAdapterDriver(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPathName,
    IN  PDRIVER_ADD_DEVICE AddDevice)
{
    /*
        (* = implement here, otherwise KS default)
        IRP_MJ_CLOSE
        * IRP_MJ_CREATE
        IRP_MJ_DEVICE_CONTROL
        IRP_MJ_FLUSH_BUFFERS
        * IRP_MJ_PNP
        * IRP_MJ_POWER
        IRP_MJ_QUERY_SECURITY
        IRP_MJ_READ
        IRP_MJ_SET_SECURITY
        * IRP_MJ_SYSTEM_CONTROL
        IRP_MJ_WRITE
    */

    //NTSTATUS status;
    //ULONG i;

    DPRINT("PcInitializeAdapterDriver\n");

#if 0
    /* Set default stub - is this a good idea? */
    DPRINT1("Setting IRP stub\n");
    for ( i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i ++ )
    {
        DriverObject->MajorFunction[i] = IrpStub;
    }
#endif

    /* Our IRP handlers */
    DPRINT1("Setting IRP handlers\n");
    DriverObject->MajorFunction[IRP_MJ_CREATE] = PcDispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_PNP] = PcDispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = PcDispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PcDispatchIrp;

    /* The driver-supplied AddDevice */
    DriverObject->DriverExtension->AddDevice = AddDevice;

    /* KS handles these */
    DPRINT1("Setting KS function handlers\n");
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_FLUSH_BUFFERS);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_QUERY_SECURITY);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_READ);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_SET_SECURITY);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_WRITE);

    DPRINT1("PortCls has finished initializing the adapter driver\n");

    return STATUS_SUCCESS;
}

/*
    Typically called by a driver's AddDevice function, which is set when
    calling PcInitializeAdapterDriver. This performs some common driver
    operations, such as creating a device extension.

    The StartDevice parameter is a driver-supplied function which gets
    called in response to IRP_MJ_PNP / IRP_MN_START_DEVICE.
*/
NTSTATUS NTAPI
PcAddAdapterDevice(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    IN  PCPFNSTARTDEVICE StartDevice,
    IN  ULONG MaxObjects,
    IN  ULONG DeviceExtensionSize)
{
    /*
        Note - after this has been called, we can
        handle IRP_MN_START_DEVICE by calling StartDevice

        TODO:
        Validate DeviceExtensionSize!! (et al...)
    */

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PDEVICE_OBJECT fdo = NULL;
    PCExtension* portcls_ext;

    DPRINT1("PcAddAdapterDevice called\n");

    if ( ! DriverObject)
    {
        DPRINT("DriverObject is NULL!\n");
        return STATUS_INVALID_PARAMETER;
    }

    if ( ! PhysicalDeviceObject )
    {
        DPRINT("PhysicalDeviceObject is NULL!\n");
        return STATUS_INVALID_PARAMETER;
    }

    if ( ! StartDevice )
    {
        DPRINT("No StartDevice parameter!\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* TODO: Make sure this is right */
    if ( DeviceExtensionSize < PORT_CLASS_DEVICE_EXTENSION_SIZE )
    {
        if ( DeviceExtensionSize != 0 )
        {
            /* TODO: Error */
            DPRINT("DeviceExtensionSize is invalid\n");
            return STATUS_INVALID_PARAMETER;
        }
    }

    DPRINT("portcls is creating a device\n");
    status = IoCreateDevice(DriverObject,
                            DeviceExtensionSize,
                            NULL,
                            PhysicalDeviceObject->DeviceType,   /* FILE_DEVICE_KS ? */
                            PhysicalDeviceObject->Characteristics, /* TODO: Check */
                            FALSE,
                            &fdo);

    if ( ! NT_SUCCESS(status) )
    {
        DPRINT("IoCreateDevice() failed with status 0x%08lx\n", status);
        return status;
    }

    /* Obtain the new device extension */
    portcls_ext = (PCExtension*) fdo->DeviceExtension;

    ASSERT(portcls_ext);

    /* Initialize */
    portcls_ext->StartDevice = StartDevice;

    DPRINT("PcAddAdapterDriver succeeded\n");

    return STATUS_SUCCESS;
}
