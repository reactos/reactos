/*
 * PROJECT:         ReactOS ACPI-Compliant Control Method Battery
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/cmbatt/cmbwmi.c
 * PURPOSE:         WMI Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "cmbatt.h"

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
CmBattQueryWmiRegInfo(PDEVICE_OBJECT DeviceObject,
                      PULONG RegFlags,
                      PUNICODE_STRING InstanceName,
                      PUNICODE_STRING *RegistryPath,
                      PUNICODE_STRING MofResourceName,
                      PDEVICE_OBJECT *Pdo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
         
NTSTATUS
NTAPI
CmBattQueryWmiDataBlock(PDEVICE_OBJECT DeviceObject,
                        PIRP Irp,
                        ULONG GuidIndex,
                        ULONG InstanceIndex,
                        ULONG InstanceCount,
                        PULONG InstanceLengthArray,
                        ULONG BufferAvail,
                        PUCHAR Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattSetWmiDataBlock(PDEVICE_OBJECT DeviceObject,
                      PIRP Irp, 
                      ULONG GuidIndex,
                      ULONG InstanceIndex,
                      ULONG BufferSize,
                      PUCHAR Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattSetWmiDataItem(PDEVICE_OBJECT DeviceObject,
                     PIRP Irp,
                     ULONG GuidIndex,
                     ULONG InstanceIndex,
                     ULONG DataItemId,
                     ULONG BufferSize,
                     PUCHAR Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattWmiDeRegistration(PCMBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattWmiRegistration(PCMBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
    
/* EOF */
