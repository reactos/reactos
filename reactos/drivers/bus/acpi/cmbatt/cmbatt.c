/*
 * PROJECT:         ReactOS ACPI-Compliant Control Method Battery
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/cmbatt/cmbatt.c
 * PURPOSE:         Main Initialization Code and IRP Handling
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "cmbatt.h"

/* GLOBALS ********************************************************************/

ULONG CmBattDebug;
PCALLBACK_OBJECT CmBattPowerCallBackObject;
PVOID CmBattPowerCallBackRegistration;
UNICODE_STRING GlobalRegistryPath;
KTIMER CmBattWakeDpcTimerObject;
KDPC CmBattWakeDpcObject;
 
/* FUNCTIONS ******************************************************************/

VOID
NTAPI
CmBattPowerCallBack(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                    PVOID Argument1,
                    PVOID Argument2)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
CmBattWakeDpc(PKDPC Dpc,
              PCMBATT_DEVICE_EXTENSION FdoExtension,
              PVOID SystemArgument1,
              PVOID SystemArgument2)
{

}

VOID
NTAPI
CmBattNotifyHandler(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                    ULONG NotifyValue)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
CmBattUnload(IN PDRIVER_OBJECT DriverObject)
{
    if (CmBattDebug & CMBATT_GENERIC_INFO) DPRINT("CmBattUnload: \n");
    
    /* Check if we have a registered power callback */
    if (CmBattPowerCallBackObject)
    {
        /* Get rid of it */
        ExUnregisterCallback(CmBattPowerCallBackRegistration);
        ObfDereferenceObject(CmBattPowerCallBackObject);
    }
    
    /* Free the registry buffer if it exists */
    if (GlobalRegistryPath.Buffer) ExFreePool(GlobalRegistryPath.Buffer);

    /* Make sure we don't still have references to the DO */
    if ((DriverObject->DeviceObject) && (CmBattDebug & CMBATT_GENERIC_WARNING))
    {
        DbgPrint("Unload called before all devices removed.\n");
    }
}

NTSTATUS
NTAPI
CmBattVerifyStaticInfo(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                       ULONG BatteryTag)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;   
}

NTSTATUS
NTAPI
CmBattOpenClose(IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;
    UCHAR Major;
    ULONG Count;
    PCMBATT_DEVICE_EXTENSION DeviceExtension;
    PAGED_CODE();
    if (CmBattDebug & CMBATT_GENERIC_INFO) DPRINT("CmBattOpenClose\n");

    /* Grab the device extension and lock it */
    DeviceExtension = DeviceObject->DeviceExtension;
    ExAcquireFastMutex(&DeviceExtension->FastMutex);
    
    /* Check if someone is trying to open a device that doesn't exist yet */
    Count = DeviceExtension->HandleCount;
    if (Count == 0xFFFFFFFF)
    {
        /* Fail the request */
        Status = STATUS_NO_SUCH_DEVICE;
        if (CmBattDebug & CMBATT_PNP_INFO)
        {
            DbgPrint("CmBattOpenClose: Failed (UID = %x)(device being removed).\n",
                     DeviceExtension->Tag);
        }
        goto Complete;
    }
    
    /* Check if this is an open or close */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    Major = IoStackLocation->MajorFunction;
    if (Major == IRP_MJ_CREATE)
    {
        /* Increment the open count */
        DeviceExtension->HandleCount = Count + 1;  
        if (CmBattDebug & CMBATT_PNP_INFO)
        {
            DbgPrint("CmBattOpenClose: Open (DeviceNumber = %x)(count = %x).\n",
                     DeviceExtension->DeviceId, Count + 1);
        }
    }
    else if (Major == IRP_MJ_CLOSE)
    {
        /* Decrement the open count */
        DeviceExtension->HandleCount = Count - 1;
        if (CmBattDebug & CMBATT_PNP_INFO)
        {
            DbgPrint("CmBattOpenClose: Close (DeviceNumber = %x)(count = %x).\n",
                     DeviceExtension->DeviceId, Count + 1);
        }
    }

Complete:
    /* Release lock and complete request */
    ExReleaseFastMutex(&DeviceExtension->FastMutex);
    Irp->IoStatus.Status = Status;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;  
}

NTSTATUS
NTAPI
CmBattIoctl(PDEVICE_OBJECT DeviceObject,
            PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattQueryTag(PCMBATT_DEVICE_EXTENSION DeviceExtension,
               PULONG BatteryTag)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;   
}

NTSTATUS
NTAPI
CmBattDisableStatusNotify(PCMBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattSetStatusNotify(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                      ULONG BatteryTag,
                      PBATTERY_NOTIFY BatteryNotify)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattGetBatteryStatus(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                       ULONG BatteryTag)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattQueryInformation(IN PCMBATT_DEVICE_EXTENSION FdoExtension,
                       IN ULONG Tag,
                       IN BATTERY_QUERY_INFORMATION_LEVEL InfoLevel,
                       IN OPTIONAL LONG AtRate,
                       IN PVOID Buffer,
                       IN ULONG BufferLength,
                       OUT PULONG ReturnedLength)
{
    NTSTATUS Status;
    PVOID QueryData = NULL;
    ULONG QueryLength = 0;
    ULONG RemainingTime = 0;
    ANSI_STRING TempString;
    UNICODE_STRING TempString2;
    WCHAR InfoBuffer[256];
    WCHAR TempBuffer[256];
    UNICODE_STRING InfoString;
    ULONG RemainingCapacity;
    BATTERY_REPORTING_SCALE BatteryReportingScale[2];
    LONG Rate;
    PAGED_CODE();
    if (CmBattDebug & (CMBATT_ACPI_WARNING | CMBATT_GENERIC_INFO))
        DbgPrint("CmBattQueryInformation - Tag (%d) Device %d, Informationlevel %d\n",
                 Tag,
                 FdoExtension->DeviceId,
                 InfoLevel);

    /* Check ACPI Data */
    Status = CmBattVerifyStaticInfo(FdoExtension, Tag);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check what caller wants */
    switch (InfoLevel)
    {
        case BatteryInformation:
            /* Just return our static information */
            QueryData = &FdoExtension->BatteryInformation;
            QueryLength = sizeof(BATTERY_INFORMATION);
            break;
        
        case BatteryGranularityInformation:
        
            /* Return our static information, we have two scales */
            BatteryReportingScale[0].Granularity = FdoExtension->BatteryCapacityGranularity1;
            BatteryReportingScale[0].Capacity = FdoExtension->BatteryInformation.DefaultAlert1;
            BatteryReportingScale[1].Granularity = FdoExtension->BatteryCapacityGranularity2;
            BatteryReportingScale[1].Capacity = FdoExtension->BatteryInformation.DesignedCapacity;
            QueryData = BatteryReportingScale;
            QueryLength = sizeof(BATTERY_REPORTING_SCALE) * 2;
            break;
            
        case BatteryEstimatedTime:
        
            /* Check if it's been more than 2 1/2 minutes since the last change */
            if ((KeQueryInterruptTime() - 150000000) > (FdoExtension->InterruptTime))
            {
                /* Get new battery status */
                CmBattGetBatteryStatus(FdoExtension, FdoExtension->Tag);
                
                /* If the caller didn't specify a rate, use our static one */
                Rate = AtRate;
                if (!Rate) Rate = FdoExtension->Rate;
                
                /* If we don't have a valid negative rate, use unknown value */
                if (Rate >= 0) Rate = BATTERY_UNKNOWN_RATE;
                
                /* Grab the remaining capacity */
                RemainingCapacity = FdoExtension->RemainingCapacity;
                
                /* See if we don't know one or the other */
                if ((Rate == BATTERY_UNKNOWN_RATE) ||
                    (RemainingCapacity == BATTERY_UNKNOWN_CAPACITY))
                {
                    /* If the battery is discharging, we can't give out a time */
                    if ((FdoExtension->BstData.State & ACPI_BATT_STAT_DISCHARG) &&
                        (CmBattDebug & CMBATT_GENERIC_WARNING))
                            DbgPrint("CmBattQueryInformation: Can't calculate EstimatedTime.\n");
                    
                    /* Check if we don't have a rate and capacity is going down */
                    if ((FdoExtension->Rate == BATTERY_UNKNOWN_RATE) &&
                        (FdoExtension->BstData.State & ACPI_BATT_STAT_DISCHARG))
                    {
                        /* We have to fail, since we lack data */
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        if (CmBattDebug & CMBATT_GENERIC_WARNING)
                            DbgPrint("----------------------   PresentRate = BATTERY_UNKNOWN_RATE\n");
                    }
                    
                    /* If we don't have capacity, the rate is useless */
                    if (RemainingCapacity == BATTERY_UNKNOWN_CAPACITY)
                    {
                        /* We have to fail the request */
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        if (CmBattDebug & CMBATT_GENERIC_WARNING)
                            DbgPrint("----------------------   RemainingCapacity = BATTERY_UNKNOWN_CAPACITY\n");
                    }
                }
                else
                {
                    /* We have data, but is it valid? */
                    if (RemainingCapacity > 0x123456)
                    {
                        /* The capacity seems bogus, so don't use it */
                        if (CmBattDebug & CMBATT_ACPI_WARNING)
                            DbgPrint("CmBattQueryInformation: Data Overflow in calculating Remaining Capacity.\n");
                    }
                    else
                    {
                        /* Compute the remaining time in seconds, based on rate */
                        RemainingTime = (RemainingCapacity * 3600) / -Rate;
                    }
                }
            }
            
            /* Return the remaining time */
            QueryData = &RemainingTime;
            QueryLength = sizeof(ULONG);
            break;
            
        case BatteryDeviceName:
        
            /* Build the model number string */
            RtlInitAnsiString(&TempString, FdoExtension->ModelNumber);

            /* Convert it to Unicode */
            InfoString.Buffer = InfoBuffer;
            InfoString.MaximumLength = sizeof(InfoBuffer);            
            Status = RtlAnsiStringToUnicodeString(&InfoString, &TempString, 0);
            
            /* Return the unicode buffer */
            QueryData = InfoString.Buffer;
            QueryLength = InfoString.Length;
            break;
            
        case BatteryTemperature:
        case BatteryManufactureDate:
        
            /* We don't support these */
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
            
        case BatteryManufactureName:
            
            /* Build the OEM info string */
            RtlInitAnsiString(&TempString, FdoExtension->OemInfo);
            
            /* Convert it to Unicode */
            InfoString.Buffer = InfoBuffer;
            InfoString.MaximumLength = sizeof(InfoBuffer);
            Status = RtlAnsiStringToUnicodeString(&InfoString, &TempString, 0);
            
            /* Return the unicode buffer */
            QueryData = InfoString.Buffer;
            QueryLength = InfoString.Length;
            break;
            
        case BatteryUniqueID:

            /* Build the serial number string */
            RtlInitAnsiString(&TempString, FdoExtension->SerialNumber);

            /* Convert it to Unicode */
            InfoString.Buffer = InfoBuffer;
            InfoString.MaximumLength = sizeof(InfoBuffer);
            RtlAnsiStringToUnicodeString(&InfoString, &TempString, 0);

            /* Setup a temporary string for concatenation */
            TempString2.Buffer = TempBuffer;
            TempString2.MaximumLength = sizeof(TempBuffer);
            
            /* Check if there's an OEM string */
            if (FdoExtension->OemInfo[0])
            {
                /* Build the OEM info string */
                RtlInitAnsiString(&TempString, FdoExtension->OemInfo);
              
                /* Convert it to Unicode and append it */
                RtlAnsiStringToUnicodeString(&TempString2, &TempString, 0);
                RtlAppendUnicodeStringToString(&InfoString, &TempString2);
            }
            
            /* Build the model number string */
            RtlInitAnsiString(&TempString, FdoExtension->ModelNumber);
            
            /* Convert it to Unicode and append it */
            RtlAnsiStringToUnicodeString(&TempString2, &TempString, 0);
            RtlAppendUnicodeStringToString(&InfoString, &TempString2);
            
            /* Return the final appended string */
            QueryData = InfoString.Buffer;
            QueryLength = InfoString.Length;
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
    return Status;
}

NTSTATUS
NTAPI
CmBattQueryStatus(IN PCMBATT_DEVICE_EXTENSION DeviceExtension,
                  IN ULONG Tag,
                  IN PBATTERY_STATUS BatteryStatus)
{
    NTSTATUS Status;
    PAGED_CODE();
    if (CmBattDebug & (CMBATT_ACPI_WARNING | CMBATT_GENERIC_INFO))
        DbgPrint("CmBattQueryStatus - Tag (%d) Device %x\n", Tag, DeviceExtension->DeviceId);
    
    /* Query ACPI information */
    Status = CmBattGetBatteryStatus(DeviceExtension, Tag);
    if (NT_SUCCESS(Status))
    {
        BatteryStatus->PowerState = DeviceExtension->State;
        BatteryStatus->Capacity = DeviceExtension->RemainingCapacity;
        BatteryStatus->Voltage = DeviceExtension->PresentVoltage;
        BatteryStatus->Rate = DeviceExtension->Rate;
    }
    
    /* Return status */
    if (CmBattDebug & (CMBATT_GENERIC_INFO))
        DbgPrint("CmBattQueryStatus: Returning [%#08lx][%#08lx][%#08lx][%#08lx]\n",
                 BatteryStatus->PowerState,
                 BatteryStatus->Capacity,
                 BatteryStatus->Voltage,
                 BatteryStatus->Rate);
    return Status;
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    PDRIVER_EXTENSION DriverExtension;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING CallbackName;

    /* Allocate registry path */
    GlobalRegistryPath.MaximumLength = RegistryPath->Length + sizeof(UNICODE_NULL);
    GlobalRegistryPath.Length = RegistryPath->Length;
    GlobalRegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                      GlobalRegistryPath.MaximumLength,
                                                      'MtaB');
    if (!GlobalRegistryPath.Buffer)
    {
        /* Fail if we're out of memory this early */
        if (CmBattDebug & CMBATT_GENERIC_WARNING)
            DbgPrint("CmBatt: Couldn't allocate pool for registry path.");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    /* Buffer allocated, copy the string */
    RtlCopyUnicodeString(&GlobalRegistryPath, RegistryPath);
    if (CmBattDebug & CMBATT_GENERIC_INFO)
        DbgPrint("CmBatt DriverEntry - Obj (%08x) Path \"%ws\"\n",
                 DriverObject,
                 RegistryPath->Buffer);
    
    /* Setup the major dispatchers */
    DriverObject->MajorFunction[0] = CmBattOpenClose;
    DriverObject->MajorFunction[2] = CmBattOpenClose;
    DriverObject->MajorFunction[14] = CmBattIoctl;
    DriverObject->MajorFunction[22] = CmBattPowerDispatch;
    DriverObject->MajorFunction[27] = CmBattPnpDispatch;
    DriverObject->MajorFunction[23] = CmBattSystemControl;

    /* And the unload routine */
    DriverObject->DriverUnload = CmBattUnload;
    
    /* And the add device routine */
    DriverExtension = DriverObject->DriverExtension;
    DriverExtension->AddDevice = CmBattAddDevice;

    /* Create a power callback */    
    RtlInitUnicodeString(&CallbackName, L"\\Callback\\PowerState");
    InitializeObjectAttributes(&ObjectAttributes,
                               &CallbackName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ExCreateCallback(&CmBattPowerCallBackObject, &ObjectAttributes, 0, TRUE);
    if (!NT_SUCCESS(Status))
    {
        /* No callback, fail */
        CmBattPowerCallBackObject = 0;
        if (CmBattDebug & CMBATT_GENERIC_WARNING)
            DbgPrint("CmBattRegisterPowerCallBack: failed status=0x%08x\n", Status);
    }
    else
    {
        /* Register the power callback now */
        CmBattPowerCallBackRegistration = ExRegisterCallback(CmBattPowerCallBackObject,
                                                             (PVOID)CmBattPowerCallBack,
                                                             DriverObject);
        if (CmBattPowerCallBackRegistration)
        {
            /* Last thing: setup our DPC and timer for battery wake */
            KeInitializeDpc(&CmBattWakeDpcObject, (PVOID)CmBattWakeDpc, DriverObject);
            KeInitializeTimer(&CmBattWakeDpcTimerObject);
        }
        else
        {
            ObfDereferenceObject(CmBattPowerCallBackObject);
            if (CmBattDebug & CMBATT_GENERIC_WARNING)
                DbgPrint("CmBattRegisterPowerCallBack: ExRegisterCallback failed.\n");
        }
        
        /* All good */
        Status = STATUS_SUCCESS;
    }

    /* Return failure or success */
    return Status;
}

/* EOF */
