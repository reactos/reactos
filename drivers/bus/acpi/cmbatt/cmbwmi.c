/*
 * PROJECT:         ReactOS ACPI-Compliant Control Method Battery
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/cmbatt/cmbwmi.c
 * PURPOSE:         WMI Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "cmbatt.h"

#include <debug.h>

/* GLOBALS ********************************************************************/

WMIGUIDREGINFO CmBattWmiGuidList[1] =
{
    {&GUID_POWER_DEVICE_WAKE_ENABLE, 1, 0}
};

/* FUNCTIONS ******************************************************************/

PCHAR
NTAPI
WMIMinorFunctionString(IN UCHAR MinorFunction)
{
    switch (MinorFunction)
    {
        case IRP_MN_CHANGE_SINGLE_INSTANCE:
            return "IRP_MN_CHANGE_SINGLE_INSTANCE";
        case IRP_MN_CHANGE_SINGLE_ITEM:
            return "IRP_MN_CHANGE_SINGLE_ITEM";
        case IRP_MN_DISABLE_COLLECTION:
            return "IRP_MN_DISABLE_COLLECTION";
        case IRP_MN_DISABLE_EVENTS:
            return "IRP_MN_DISABLE_EVENTS";
        case IRP_MN_ENABLE_COLLECTION:
            return "IRP_MN_ENABLE_COLLECTION";
        case IRP_MN_ENABLE_EVENTS:
            return "IRP_MN_ENABLE_EVENTS";
        case IRP_MN_EXECUTE_METHOD:
            return "IRP_MN_EXECUTE_METHOD";
        case IRP_MN_QUERY_ALL_DATA:
            return "IRP_MN_QUERY_ALL_DATA";
        case IRP_MN_QUERY_SINGLE_INSTANCE:
            return "IRP_MN_QUERY_SINGLE_INSTANCE";
        case IRP_MN_REGINFO:
            return "IRP_MN_REGINFO";
        default:
            return "IRP_MN_?????";
    }
}

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
CmBattSystemControl(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    NTSTATUS Status;
    PCMBATT_DEVICE_EXTENSION DeviceExtension;
    PWMILIB_CONTEXT WmiLibContext;
    SYSCTL_IRP_DISPOSITION Disposition = IrpForward;
    PAGED_CODE();
    if (CmBattDebug & 2)
        DbgPrint("CmBatt: SystemControl: %s\n",
                 WMIMinorFunctionString(IoGetCurrentIrpStackLocation(Irp)->MinorFunction));
    
    /* Acquire the remove lock */
    DeviceExtension = DeviceObject->DeviceExtension;
    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, 0);
    if (!NT_SUCCESS(Status))
    {
        /* It's too late, fail */
        Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_REMOVED;
    }
    
    /* What kind of device is this? */
    WmiLibContext = &DeviceExtension->WmiLibInfo;
    if (DeviceExtension->FdoType == CmBattBattery)
    {
        /* For batteries, let the class driver handle it */
        Status = BatteryClassSystemControl(DeviceExtension->ClassData,
                                           WmiLibContext,
                                           DeviceObject,
                                           Irp,
                                           &Disposition);
    }
    else
    {
        /* Otherwise, call the wmi library directly */
        Status = WmiSystemControl(WmiLibContext,
                                  DeviceObject,
                                  Irp,
                                  &Disposition);
    }

    /* Check what happened */
    switch (Disposition)
    {
        case IrpNotCompleted:
        
            /* Complete it here */
            if (CmBattDebug & 2) DbgPrint("CmBatt: SystemControl: Irp Not Completed.\n");
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
            
        case IrpForward:
        
            /* Forward it to ACPI */
            if (CmBattDebug & 2) DbgPrint("CmBatt: SystemControl: Irp Forward.\n");
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
            break;
            
        case IrpProcessed:
        
            /* Nothing to do */
            if (CmBattDebug & 2) DbgPrint("CmBatt: SystemControl: Irp Processed.\n");
            break;
            
        default:
            ASSERT(FALSE);
    }

    /* Release the lock and return */
    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, 0);
    return Status;
}
    
/* EOF */
