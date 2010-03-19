/*
 * PROJECT:         ReactOS ACPI-Compliant Control Method Battery
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/cmbatt/cmbpnp.c
 * PURPOSE:         Plug-and-Play IOCTL/IRP Handling
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "cmbatt.h"

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
CmBattIoCompletion(PDEVICE_OBJECT DeviceObject,
                   PIRP Irp,
                   PKEVENT Event)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattGetAcpiInterfaces(PDEVICE_OBJECT DeviceObject,
                        PACPI_INTERFACE_STANDARD2 *AcpiInterface)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
CmBattDestroyFdo(PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattRemoveDevice(PDEVICE_OBJECT DeviceObject,
                   PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattPowerDispatch(PDEVICE_OBJECT DeviceObject,
                    PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattCreateFdo(PDRIVER_OBJECT DriverObject,
                PDEVICE_OBJECT DeviceObject,
                ULONG DeviceExtensionSize,
                PDEVICE_OBJECT *NewDeviceObject)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattPnpDispatch(PDEVICE_OBJECT DeviceObject,
                  PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattAddBattery(PDRIVER_OBJECT DriverObject,
                 PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattAddAcAdapter(PDRIVER_OBJECT DriverObject,
                   PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattAddDevice(PDRIVER_OBJECT DriverObject,
                PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
