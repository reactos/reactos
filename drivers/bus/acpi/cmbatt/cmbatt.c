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
CmBattVerifyStaticInfo(ULONG StaData,
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
CmBattQueryInformation(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                       ULONG BatteryTag,
                       BATTERY_QUERY_INFORMATION_LEVEL Level,
                       OPTIONAL LONG AtRate,
                       PVOID Buffer,
                       ULONG BufferLength,
                       PULONG ReturnedLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED; 
}

NTSTATUS
NTAPI
CmBattQueryStatus(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                  ULONG BatteryTag,
                  PBATTERY_STATUS BatteryStatus)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
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
        {
            DbgPrint("CmBatt: Couldn't allocate pool for registry path.");
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    /* Buffer allocated, copy the string */
    RtlCopyUnicodeString(&GlobalRegistryPath, RegistryPath);
    if (CmBattDebug & CMBATT_GENERIC_INFO)
    {
        DbgPrint("CmBatt DriverEntry - Obj (%08x) Path \"%ws\"\n",
                 DriverObject,
                 RegistryPath->Buffer);
    }
    
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
        {
            DbgPrint("CmBattRegisterPowerCallBack: failed status=0x%08x\n", Status);
        }
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
            {
                DbgPrint("CmBattRegisterPowerCallBack: ExRegisterCallback failed.\n");
            }
        }
        
        /* All good */
        Status = STATUS_SUCCESS;
    }

    /* Return failure or success */
    return Status;
}

/* EOF */
