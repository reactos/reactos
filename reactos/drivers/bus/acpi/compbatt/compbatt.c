/*
 * PROJECT:         ReactOS Composite Battery Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/compbatt/compbatt.c
 * PURPOSE:         Main Initialization Code and IRP Handling
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "compbatt.h"

#include <debug.h>

/* GLOBALS ********************************************************************/

ULONG CompBattDebug;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
CompBattOpenClose(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)
{
    PAGED_CODE();
    if (CompBattDebug & 0x100) DbgPrint("CompBatt: ENTERING OpenClose\n");
    
    /* Complete the IRP with success */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    /* Return success */
    if (CompBattDebug & 0x100) DbgPrint("CompBatt: Exiting OpenClose\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CompBattSystemControl(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    PCOMPBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    PAGED_CODE();
    if (CompBattDebug & 1) DbgPrint("CompBatt: ENTERING System Control\n");
    
    /* Are we attached yet? */
    if (DeviceExtension->AttachedDevice)
    {
        /* Send it up the stack */
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
    }
    else
    {
        /* We don't support WMI */
        Status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    
    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CompBattMonitorIrpComplete(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp,
                           IN PKEVENT Event)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattMonitorIrpCompleteWorker(IN PCOMPBATT_BATTERY_DATA BatteryData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
CompBattRecalculateTag(IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    PCOMPBATT_BATTERY_DATA BatteryData;
    ULONG Tag;
    PLIST_ENTRY ListHead, NextEntry;
    if (CompBattDebug & 0x100) DbgPrint("CompBatt: ENTERING CompBattRecalculateTag\n");

    /* Loop the battery list */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the battery information and check if it has a tag */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        if (BatteryData->Flags & COMPBATT_TAG_ASSIGNED)
        {
            /* Generate the next tag and exit */
            Tag = DeviceExtension->NextTag;
            DeviceExtension->Flags |= COMPBATT_TAG_ASSIGNED;
            DeviceExtension->Tag = Tag;
            DeviceExtension->NextTag = Tag + 1;
            break;
       }
       
       /* No tag for this device extension, clear it */
       DeviceExtension->Tag = 0;
       NextEntry = NextEntry->Flink;
    }
    
    /* We're done */ 
    ExReleaseFastMutex(&DeviceExtension->Lock);
    if (CompBattDebug & 0x100) DbgPrint("CompBatt: EXITING CompBattRecalculateTag\n");
}

NTSTATUS
NTAPI
CompBattIoctl(IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp)
{
    PCOMPBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    if (CompBattDebug & 1) DbgPrint("CompBatt: ENTERING Ioctl\n");

    /* Let the class driver handle it */
    Status = BatteryClassIoctl(DeviceExtension->ClassData, Irp);
    if (Status == STATUS_NOT_SUPPORTED)
    {
        /* It failed, try the next driver up the stack */
        Irp->IoStatus.Status = Status;
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
    }

    /* Return status */
    if (CompBattDebug & 1) DbgPrint("CompBatt: EXITING Ioctl\n");
    return Status;
}

NTSTATUS
NTAPI
CompBattQueryTag(IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
                 OUT PULONG Tag)
{
    NTSTATUS Status;
    PAGED_CODE();
    if (CompBattDebug & 0x100) DbgPrint("CompBatt: ENTERING QueryTag\n");

    /* Was a tag assigned? */
    if (!(DeviceExtension->Flags & COMPBATT_TAG_ASSIGNED))
    {
        /* Assign one */
        CompBattRecalculateTag(DeviceExtension);
    }
      
    /* Do we have a tag now? */
    if ((DeviceExtension->Flags & COMPBATT_TAG_ASSIGNED) && (DeviceExtension->Tag))
    {
        /* Return the tag */
        *Tag = DeviceExtension->Tag;
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* No tag */
        *Tag = 0;
        Status = STATUS_NO_SUCH_DEVICE;
    }
    
    /* Return status */
    if (CompBattDebug & 0x100) DbgPrint("CompBatt: EXITING QueryTag\n");
    return Status;
}

NTSTATUS
NTAPI
CompBattDisableStatusNotify(IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    PCOMPBATT_BATTERY_DATA BatteryData;
    PLIST_ENTRY ListHead, NextEntry;
    if (CompBattDebug & 0x100) DbgPrint("CompBatt: ENTERING DisableStatusNotify\n");

    /* Loop the battery list */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the battery information and clear capacity data */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        BatteryData->WaitStatus.LowCapacity = 0;
        BatteryData->WaitStatus.HighCapacity = 0x7FFFFFFF;
        NextEntry = NextEntry->Flink;
    }

    /* Done */
    ExReleaseFastMutex(&DeviceExtension->Lock);
    if (CompBattDebug & 0x100) DbgPrint("CompBatt: EXITING DisableStatusNotify\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CompBattSetStatusNotify(IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
                        IN ULONG BatteryTag,
                        IN PBATTERY_NOTIFY BatteryNotify)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattQueryStatus(IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
                    IN ULONG Tag,
                    IN PBATTERY_STATUS BatteryStatus)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattGetBatteryInformation(OUT PBATTERY_INFORMATION BatteryInfo,
                              IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BATTERY_QUERY_INFORMATION InputBuffer;
    PCOMPBATT_BATTERY_DATA BatteryData;
    PLIST_ENTRY ListHead, NextEntry;
    if (CompBattDebug & 1) DbgPrint("CompBatt: ENTERING GetBatteryInformation\n");
    
    /* Set defaults */
    BatteryInfo->DefaultAlert1 = 0;
    BatteryInfo->DefaultAlert2 = 0;
    BatteryInfo->CriticalBias = 0;
    
    /* Loop the battery list */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Try to acquire the remove lock */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        if (NT_SUCCESS(IoAcquireRemoveLock(&BatteryData->RemoveLock, 0)))
        {
            /* Now release the device lock since the battery can't go away */
            ExReleaseFastMutex(&DeviceExtension->Lock);
            
            /* Build the query */
            InputBuffer.BatteryTag = BatteryData->Tag;
            InputBuffer.InformationLevel = BatteryInformation;
            InputBuffer.AtRate = 0;
            
            /* Make sure the battery has a tag */
            if (BatteryData->Tag)
            {
                /* Do we already have the data? */
                if (!(BatteryData->Flags & COMPBATT_BATTERY_INFORMATION_PRESENT))
                {
                    /* Send the IOCTL to query the information */
                    RtlZeroMemory(&BatteryData->BatteryInformation,
                                  sizeof(BatteryData->BatteryInformation));
                    Status = BatteryIoctl(IOCTL_BATTERY_QUERY_INFORMATION,
                                          BatteryData->DeviceObject,
                                          &InputBuffer,
                                          sizeof(InputBuffer),
                                          &BatteryData->BatteryInformation,
                                          sizeof(BatteryData->BatteryInformation),
                                          0);
                    if (!NT_SUCCESS(Status))
                    {
                        /* Fail if the query had a problem */
                        if (Status == STATUS_DEVICE_REMOVED) Status = STATUS_NO_SUCH_DEVICE;
                        ExAcquireFastMutex(&DeviceExtension->Lock);
                        IoReleaseRemoveLock(&BatteryData->RemoveLock, 0);
                        break;
                    }
                    
                    /* Next time we can use the static copy */
                    BatteryData->Flags |= COMPBATT_BATTERY_INFORMATION_PRESENT;
                    if (CompBattDebug & 2)
                        DbgPrint("CompBattGetBatteryInformation: Read individual BATTERY_INFORMATION\n"
                                 "--------  Capabilities = %x\n--------  Technology = %x\n"
                                 "--------  Chemistry[4] = %x\n--------  DesignedCapacity = %x\n"
                                 "--------  FullChargedCapacity = %x\n--------  DefaultAlert1 = %x\n"
                                 "--------  DefaultAlert2 = %x\n--------  CriticalBias = %x\n"
                                 "--------  CycleCount = %x\n",
                                 BatteryData->BatteryInformation.Capabilities,
                                 BatteryData->BatteryInformation.Technology,
                                 BatteryData->BatteryInformation.Chemistry,
                                 BatteryData->BatteryInformation.DesignedCapacity,
                                 BatteryData->BatteryInformation.FullChargedCapacity,
                                 BatteryData->BatteryInformation.DefaultAlert1,
                                 BatteryData->BatteryInformation.DefaultAlert2,
                                 BatteryData->BatteryInformation.CriticalBias,
                                 BatteryData->BatteryInformation.CycleCount);
                }

                /* Combine capabilities */
                BatteryInfo->Capabilities |= BatteryData->BatteryInformation.Capabilities;
                
                /* Add-on capacity */
                if (BatteryData->BatteryInformation.DesignedCapacity != BATTERY_UNKNOWN_CAPACITY)
                {
                    BatteryInfo->DesignedCapacity += BatteryData->BatteryInformation.DesignedCapacity;
                }

                /* Add on fully charged capacity */
                if (BatteryData->BatteryInformation.FullChargedCapacity != BATTERY_UNKNOWN_CAPACITY)
                {
                    BatteryInfo->FullChargedCapacity += BatteryData->BatteryInformation.FullChargedCapacity;
                }
                
                /* Choose the highest alert */
                BatteryInfo->DefaultAlert1 = max(BatteryInfo->DefaultAlert1,
                                                 BatteryData->BatteryInformation.DefaultAlert1);

                /* Choose the highest alert */
                BatteryInfo->DefaultAlert2 = max(BatteryInfo->DefaultAlert2,
                                                 BatteryData->BatteryInformation.DefaultAlert2);
                
                /* Choose the highest critical bias */
                BatteryInfo->CriticalBias = max(BatteryInfo->CriticalBias,
                                                BatteryData->BatteryInformation.CriticalBias);
            }
            
            /* Re-acquire the device extension lock and release the remove lock */
            ExAcquireFastMutex(&DeviceExtension->Lock);
            IoReleaseRemoveLock(&BatteryData->RemoveLock, 0);
        }
        
        /* Next entry */
        NextEntry = NextEntry->Flink;
    }
    
    /* We are done with the list, check if the information was queried okay */ 
    ExReleaseFastMutex(&DeviceExtension->Lock);
    if (NT_SUCCESS(Status))
    {
        /* If there's no fully charged capacity, use the design capacity */
        if (!BatteryInfo->FullChargedCapacity)
        {
            BatteryInfo->FullChargedCapacity = BatteryInfo->DesignedCapacity;
        }
        
        /* Print out final combined data */
        if (CompBattDebug & 2)
            DbgPrint("CompBattGetBatteryInformation: Returning BATTERY_INFORMATION\n"
                     "--------  Capabilities = %x\n--------  Technology = %x\n"
                     "--------  Chemistry[4] = %x\n--------  DesignedCapacity = %x\n"
                     "--------  FullChargedCapacity = %x\n--------  DefaultAlert1 = %x\n"
                     "--------  DefaultAlert2 = %x\n--------  CriticalBias = %x\n"
                     "--------  CycleCount = %x\n",
                     BatteryInfo->Capabilities,
                     BatteryInfo->Technology,
                     BatteryInfo->Chemistry,
                     BatteryInfo->DesignedCapacity,
                     BatteryInfo->FullChargedCapacity,
                     BatteryInfo->DefaultAlert1,
                     BatteryInfo->DefaultAlert2,
                     BatteryInfo->CriticalBias,
                     BatteryInfo->CycleCount);
                     
        /* Copy the data into the device extension */
        RtlCopyMemory(&DeviceExtension->BatteryInformation,
                      BatteryInfo,
                      sizeof(DeviceExtension->BatteryInformation));
        DeviceExtension->Flags |= COMPBATT_BATTERY_INFORMATION_PRESENT;
    }
    
    /* We are done */
    if (CompBattDebug & 1) DbgPrint("CompBatt: EXITING GetBatteryInformation\n");
    return Status;
}

NTSTATUS
NTAPI
CompBattGetBatteryGranularity(OUT PBATTERY_REPORTING_SCALE ReportingScale,
                              IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BATTERY_QUERY_INFORMATION InputBuffer;
    PCOMPBATT_BATTERY_DATA BatteryData;
    BATTERY_REPORTING_SCALE BatteryScale[4];
    PLIST_ENTRY ListHead, NextEntry;
    ULONG i;
    if (CompBattDebug & 1) DbgPrint("CompBatt: ENTERING GetBatteryGranularity\n");
    
    /* Set defaults */
    ReportingScale[0].Granularity = -1;
    ReportingScale[1].Granularity = -1;
    ReportingScale[2].Granularity = -1;
    ReportingScale[3].Granularity = -1;
    
    /* Loop the battery list */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Try to acquire the remove lock */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        if (NT_SUCCESS(IoAcquireRemoveLock(&BatteryData->RemoveLock, 0)))
        {
            /* Now release the device lock since the battery can't go away */
            ExReleaseFastMutex(&DeviceExtension->Lock);
            
            /* Build the query */
            InputBuffer.BatteryTag = BatteryData->Tag;
            InputBuffer.InformationLevel = BatteryGranularityInformation;
            
            /* Make sure the battery has a tag */
            if (BatteryData->Tag)
            {
                /* Send the IOCTL to query the information */
                RtlZeroMemory(&BatteryData->BatteryInformation,
                              sizeof(BatteryData->BatteryInformation));
                Status = BatteryIoctl(IOCTL_BATTERY_QUERY_INFORMATION,
                                      BatteryData->DeviceObject,
                                      &InputBuffer,
                                      sizeof(InputBuffer),
                                      &BatteryScale,
                                      sizeof(BatteryScale),
                                      0);
                if (!NT_SUCCESS(Status))
                {
                    /* Fail if the query had a problem */
                    ExAcquireFastMutex(&DeviceExtension->Lock);
                    IoReleaseRemoveLock(&BatteryData->RemoveLock, 0);
                    break;
                }
                
                /* Loop all 4 scales */
                for (i = 0; i < 4; i++)
                {
                    /* Check for valid granularity */
                    if (BatteryScale[i].Granularity)
                    {
                        /* If it's smaller, use it instead */
                        ReportingScale[i].Granularity = min(BatteryScale[i].Granularity,
                                                            ReportingScale[i].Granularity);
                    }
                    
                }
            }
            
            /* Re-acquire the device extension lock and release the remove lock */
            ExAcquireFastMutex(&DeviceExtension->Lock);
            IoReleaseRemoveLock(&BatteryData->RemoveLock, 0);
        }

        /* Next entry */
        NextEntry = NextEntry->Flink;
    }
    
    /* All done */
    ExReleaseFastMutex(&DeviceExtension->Lock);
    if (CompBattDebug & 1) DbgPrint("CompBatt: EXITING GetBatteryGranularity\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CompBattGetEstimatedTime(OUT PULONG Time,
                         IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
    
NTSTATUS
NTAPI
CompBattQueryInformation(IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
                         IN ULONG Tag,
                         IN BATTERY_QUERY_INFORMATION_LEVEL InfoLevel,
                         IN OPTIONAL LONG AtRate,
                         IN PVOID Buffer,
                         IN ULONG BufferLength,
                         OUT PULONG ReturnedLength)
{
    BATTERY_INFORMATION BatteryInfo;
    BATTERY_REPORTING_SCALE BatteryGranularity[4];
    PWCHAR BatteryName = L"Composite Battery";
    //BATTERY_MANUFACTURE_DATE Date;
    ULONG Dummy, Time;
    PVOID QueryData = NULL;
    ULONG QueryLength = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();
    if (CompBattDebug & 1)  DbgPrint("CompBatt: ENTERING QueryInformation\n");

    /* Check for valid/correct tag */
    if ((Tag != DeviceExtension->Tag) ||
        (!(DeviceExtension->Flags & COMPBATT_TAG_ASSIGNED)))
    {
        /* Not right, so fail */
        return STATUS_NO_SUCH_DEVICE;
    }
      
    /* Check what caller wants */
    switch (InfoLevel)
    {
        case BatteryInformation:
        
            /* Query combined battery information */
            RtlZeroMemory(&BatteryInfo, sizeof(BatteryInfo));
            Status = CompBattGetBatteryInformation(&BatteryInfo, DeviceExtension);
            if (NT_SUCCESS(Status))
            {
                /* Return the data if successful */
                QueryData = &BatteryInfo;
                QueryLength = sizeof(BatteryInfo);
            }
            break;
        
        case BatteryGranularityInformation:

            /* Query combined granularity information */
            RtlZeroMemory(&BatteryGranularity, sizeof(BatteryGranularity));
            Status = CompBattGetBatteryGranularity(BatteryGranularity, DeviceExtension);
            if (NT_SUCCESS(Status))
            {
                /* Return the data if successful */
                QueryLength = sizeof(BatteryGranularity);
                QueryData = &BatteryGranularity;
            }
            break;
            
        case BatteryEstimatedTime:
        
            /* Query combined time estimate information */
            RtlZeroMemory(&Time, sizeof(Time));
            Status = CompBattGetEstimatedTime(&Time, DeviceExtension);
            if (NT_SUCCESS(Status))
            {
                /* Return the data if successful */
                QueryLength = sizeof(Time);
                QueryData = &Time; 
            }
            break;
            
        case BatteryManufactureName:
        case BatteryDeviceName:
        
            /* Return the static buffer */
            QueryData = BatteryName;
            QueryLength = sizeof(L"Composite Battery");
            break;
    
        case BatteryManufactureDate:
        
            /* Static data */
            //Date.Day = 26;
            //Date.Month = 06;
            //Date.Year = 1997;
            break;

        case BatteryTemperature:
        case BatteryUniqueID:

            /* Return zero */
            Dummy = 0;
            QueryData = &Dummy;
            QueryLength = sizeof(Dummy);
            break;
            
        default:
            /* Everything else is unknown */
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    /* Return the required length and check if the caller supplied enough */
    *ReturnedLength = QueryLength;
    if (BufferLength < QueryLength) Status = STATUS_BUFFER_TOO_SMALL;

    /* Copy the data if there's enough space and it exists */
    if ((NT_SUCCESS(Status)) && (QueryData)) RtlCopyMemory(Buffer, QueryData, QueryLength);
      
    /* Return function result */
    if (CompBattDebug & 1) DbgPrint("CompBatt: EXITING QueryInformation\n");
    return Status;
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    /* Register add device routine */
    DriverObject->DriverExtension->AddDevice = CompBattAddDevice;
    
    /* Register other handlers */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CompBattOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CompBattOpenClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = CompBattIoctl;
    DriverObject->MajorFunction[IRP_MJ_POWER] = CompBattPowerDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = CompBattSystemControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = CompBattPnpDispatch;
    return STATUS_SUCCESS;
}

/* EOF */
