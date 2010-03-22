/*
 * PROJECT:         ReactOS ACPI-Compliant Control Method Battery
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/cmbatt/cmbwmi.c
 * PURPOSE:         WMI Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "cmbatt.h"

/* GLOBALS ********************************************************************/

WMIGUIDREGINFO CmBattWmiGuidList[1] =
{
    {&GUID_POWER_DEVICE_WAKE_ENABLE, 1, 0}
};

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
CmBattWmiDeRegistration(IN PCMBATT_DEVICE_EXTENSION DeviceExtension)
{
    PAGED_CODE();
    
    /* De-register */
    return IoWMIRegistrationControl(DeviceExtension->FdoDeviceObject,
                                    WMIREG_ACTION_DEREGISTER); 
}

NTSTATUS
NTAPI
CmBattWmiRegistration(IN PCMBATT_DEVICE_EXTENSION DeviceExtension)
{
    PAGED_CODE();
    
    /* GUID information */
    DeviceExtension->WmiLibInfo.GuidCount = sizeof(CmBattWmiGuidList) /
                                            sizeof(WMIGUIDREGINFO);
    DeviceExtension->WmiLibInfo.GuidList = CmBattWmiGuidList;
    
    /* Callbacks */
    DeviceExtension->WmiLibInfo.QueryWmiRegInfo = CmBattQueryWmiRegInfo;
    DeviceExtension->WmiLibInfo.QueryWmiDataBlock = CmBattQueryWmiDataBlock;
    DeviceExtension->WmiLibInfo.SetWmiDataBlock = CmBattSetWmiDataBlock;
    DeviceExtension->WmiLibInfo.SetWmiDataItem = CmBattSetWmiDataItem;
    DeviceExtension->WmiLibInfo.ExecuteWmiMethod = NULL;
    DeviceExtension->WmiLibInfo.WmiFunctionControl = NULL;
    
    /* Register */
    return IoWMIRegistrationControl(DeviceExtension->FdoDeviceObject,
                                    WMIREG_ACTION_REGISTER);
}

NTSTATUS
NTAPI
CmBattSystemControl(PDEVICE_OBJECT DeviceObject,
                    PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
    
/* EOF */
