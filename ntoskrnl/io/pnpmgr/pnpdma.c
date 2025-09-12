/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpdma.c
 * PURPOSE:         PnP manager DMA routines
 * PROGRAMMERS:     Filip Navara (xnavara@volny.cz)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include <wdmguid.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
PDMA_ADAPTER
NTAPI
IoGetDmaAdapter(IN PDEVICE_OBJECT PhysicalDeviceObject,
                IN PDEVICE_DESCRIPTION DeviceDescription,
                IN OUT PULONG NumberOfMapRegisters)
{
    NTSTATUS Status;
    ULONG ResultLength;
    BUS_INTERFACE_STANDARD BusInterface;
    IO_STATUS_BLOCK IoStatusBlock;
    IO_STACK_LOCATION Stack;
    DEVICE_DESCRIPTION PrivateDeviceDescription;
    PDMA_ADAPTER Adapter = NULL;

    DPRINT("IoGetDmaAdapter called\n");


    /* Try to create DMA adapter through bus driver */
    if (PhysicalDeviceObject)
    {
        if (DeviceDescription->InterfaceType == PNPBus ||
            DeviceDescription->InterfaceType == InterfaceTypeUndefined)
        {
            RtlCopyMemory(&PrivateDeviceDescription,
                          DeviceDescription,
                          sizeof(DEVICE_DESCRIPTION));

            Status = IoGetDeviceProperty(PhysicalDeviceObject,
                                         DevicePropertyLegacyBusType,
                                         sizeof(INTERFACE_TYPE),
                                         &PrivateDeviceDescription.InterfaceType,
                                         &ResultLength);

            if (!NT_SUCCESS(Status))
                PrivateDeviceDescription.InterfaceType = Internal;

            DeviceDescription = &PrivateDeviceDescription;
        }

        Stack.Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
        Stack.Parameters.QueryInterface.Version = 1;
        Stack.Parameters.QueryInterface.Interface = (PINTERFACE)&BusInterface;
        Stack.Parameters.QueryInterface.InterfaceType =
            &GUID_BUS_INTERFACE_STANDARD;

        Status = IopInitiatePnpIrp(PhysicalDeviceObject,
                                   &IoStatusBlock,
                                   IRP_MN_QUERY_INTERFACE,
                                   &Stack);

        if (NT_SUCCESS(Status))
        {
            Adapter = BusInterface.GetDmaAdapter(BusInterface.Context,
                                                 DeviceDescription,
                                                 NumberOfMapRegisters);

            BusInterface.InterfaceDereference(BusInterface.Context);
            if (Adapter) return Adapter;
        }
    }

    /* Fall back to HAL */
    return HalGetDmaAdapter(PhysicalDeviceObject,
                            DeviceDescription,
                            NumberOfMapRegisters);
}
