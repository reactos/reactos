/*
 * PROJECT:         ReactOS ACPI-Compliant Control Method Battery
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/cmbatt/cmexec.c
 * PURPOSE:         ACPI Method Execution/Evaluation Glue
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "cmbatt.h"

#include <acpiioct.h>
#include <debug.h>

typedef struct
{
    LPSTR Name;
    BOOLEAN IsString;
    PVOID Data;
} ACPI_PACKAGE_FIELD, *PACPI_PACKAGE_FIELD;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
GetDwordElement(IN PACPI_METHOD_ARGUMENT Argument,
                OUT PULONG Value)
{
    NTSTATUS Status;

    /* Must have an integer */
    if (Argument->Type != ACPI_METHOD_ARGUMENT_INTEGER)
    {
        /* Not an integer, fail */
        Status = STATUS_ACPI_INVALID_DATA;
        if (CmBattDebug & 0x4C)
            DbgPrint("GetDwordElement: Object contained wrong data type - %d\n",
                     Argument->Type);
    }
    else
    {
        /* Read the integer value */
        *Value = Argument->Argument;
        Status = STATUS_SUCCESS;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
GetStringElement(IN PACPI_METHOD_ARGUMENT Argument,
                 OUT PCHAR Value)
{
    NTSTATUS Status;

    /* Must have a string of buffer */
    if ((Argument->Type == ACPI_METHOD_ARGUMENT_STRING) ||
        (Argument->Type == ACPI_METHOD_ARGUMENT_BUFFER))
    {
        /* String must be less than 256 characters */
        if (Argument->DataLength < 256)
        {
            /* Copy the buffer */
            RtlCopyMemory(Value, Argument->Data, Argument->DataLength);
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* The buffer is too small (the string is too large) */
            Status = STATUS_BUFFER_TOO_SMALL;
            if (CmBattDebug & 0x4C)
                DbgPrint("GetStringElement: return buffer not big enough - %d\n", Argument->DataLength);
        }
    }
    else
    {
        /* Not valid string data */
        Status = STATUS_ACPI_INVALID_DATA;
        if (CmBattDebug & 0x4C)
            DbgPrint("GetStringElement: Object contained wrong data type - %d\n", Argument->Type);
    }

    /* Return the status */
    return Status;
}

NTSTATUS
NTAPI
CmBattSendDownStreamIrp(IN PDEVICE_OBJECT DeviceObject,
                        IN ULONG IoControlCode,
                        IN PVOID InputBuffer,
                        IN ULONG InputBufferLength,
                        IN PACPI_EVAL_OUTPUT_BUFFER OutputBuffer,
                        IN ULONG OutputBufferLength)
{
    PIRP Irp;
    NTSTATUS Status;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PAGED_CODE();

    /* Initialize our wait event */
    KeInitializeEvent(&Event, SynchronizationEvent, 0);

    /* Allocate the IRP */
    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        0,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        /* No IRP, fail */
        if (CmBattDebug & 0x4C)
            DbgPrint("CmBattSendDownStreamIrp: Failed to allocate Irp\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Call ACPI */
    if (CmBattDebug & 0x40)
        DbgPrint("CmBattSendDownStreamIrp: Irp %x [Tid] %x\n",
                 Irp, KeGetCurrentThread());
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = Irp->IoStatus.Status;
    }

    /* Check if caller wanted output */
    if (OutputBuffer)
    {
        /* Make sure it's valid ACPI output buffer */
        if ((OutputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) ||
            !(OutputBuffer->Count))
        {
            /* It isn't, so set failure code */
            Status = STATUS_ACPI_INVALID_DATA;
        }
    }

    /* Return status */
    if (CmBattDebug & 0x40)
        DbgPrint("CmBattSendDownStreamIrp: Irp %x completed %x! [Tid] %x\n",
                 Irp, Status, KeGetCurrentThread());
    return Status;
}

static
NTSTATUS
CmBattCallAcpiPackage(
    _In_ LPCSTR FunctionName,
    _In_ PCMBATT_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG PackageName,
    _In_ ULONG OutputBufferSize,
    _In_ PACPI_PACKAGE_FIELD PackageFields,
    _In_ ULONG PackageFieldCount)
{
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER OutputBuffer;
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    PACPI_METHOD_ARGUMENT Argument;
    ULONG i;
    PAGED_CODE();

    OutputBuffer = ExAllocatePoolWithTag(PagedPool,
                                         OutputBufferSize,
                                         'MtaB');
    if (!OutputBuffer)
    {
        if (CmBattDebug & (CMBATT_ACPI_ENTRY_EXIT | CMBATT_ACPI_WARNING | CMBATT_GENERIC_WARNING))
            DbgPrint("%s: Failed to allocate Buffer\n", FunctionName);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize to zero */
    RtlZeroMemory(OutputBuffer, OutputBufferSize);

    /* Request the ACPI method */
    *(PULONG)InputBuffer.MethodName = PackageName;
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    /* Send it to ACPI */
    Status = CmBattSendDownStreamIrp(DeviceExtension->AttachedDevice,
                                     IOCTL_ACPI_EVAL_METHOD,
                                     &InputBuffer,
                                     sizeof(InputBuffer),
                                     OutputBuffer,
                                     OutputBufferSize);
    if (!NT_SUCCESS(Status))
    {
        if (CmBattDebug & (CMBATT_ACPI_ASSERT | CMBATT_ACPI_ENTRY_EXIT | CMBATT_ACPI_WARNING | CMBATT_GENERIC_WARNING))
            DbgPrint("%s: Failed 0x%08x method on device %x - Status (0x%x)\n",
                     FunctionName, PackageName, DeviceExtension->DeviceId, Status);
        ExFreePoolWithTag(OutputBuffer, 'MtaB');
        return Status;
    }

    /* Check if we got the right number of elements */
    if (OutputBuffer->Count != PackageFieldCount)
    {
        if (CmBattDebug & (CMBATT_ACPI_ASSERT | CMBATT_ACPI_ENTRY_EXIT | CMBATT_ACPI_WARNING | CMBATT_GENERIC_WARNING))
            DbgPrint("%s: 0x%08x method returned %d elements (requires %d)\n",
                     FunctionName, PackageName, OutputBuffer->Count, PackageFieldCount);
        ExFreePoolWithTag(OutputBuffer, 'MtaB');
        return STATUS_ACPI_INVALID_DATA;
    }

    Argument = OutputBuffer->Argument;
    for (i = 0; i < PackageFieldCount && NT_SUCCESS(Status); i++)
    {
        if (PackageFields[i].IsString)
            Status = GetStringElement(Argument, PackageFields[i].Data);
        else
            Status = GetDwordElement(Argument, PackageFields[i].Data);
        if (!NT_SUCCESS(Status))
        {
            if (CmBattDebug & (CMBATT_ACPI_ASSERT | CMBATT_ACPI_ENTRY_EXIT | CMBATT_ACPI_WARNING | CMBATT_GENERIC_WARNING))
                DbgPrint("%s: Failed to get %s\n", FunctionName, PackageFields[i].Name);
            break;
        }
        Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    }

    ExFreePoolWithTag(OutputBuffer, 'MtaB');
    return Status;
}

NTSTATUS
NTAPI
CmBattGetPsrData(IN PDEVICE_OBJECT DeviceObject,
                 OUT PULONG PsrData)
{
    NTSTATUS Status;
    ACPI_EVAL_OUTPUT_BUFFER OutputBuffer;
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    PAGED_CODE();
    if (CmBattDebug & 0x40)
        DbgPrint("CmBattGetPsrData: Entered with Pdo %x Tid %x\n",
                 DeviceObject, KeGetCurrentThread());

    /* Initialize to zero */
    ASSERT(PsrData != NULL);
    *PsrData = 0;

    /* Request the _PSR method */
    *(PULONG)InputBuffer.MethodName = 'RSP_';
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    /* Send it to ACPI */
    Status = CmBattSendDownStreamIrp(DeviceObject,
                                     IOCTL_ACPI_EVAL_METHOD,
                                     &InputBuffer,
                                     sizeof(InputBuffer),
                                     &OutputBuffer,
                                     sizeof(OutputBuffer));
    if (NT_SUCCESS(Status))
    {
        /* Read the result */
        Status = GetDwordElement(OutputBuffer.Argument, PsrData);
        if (CmBattDebug & 0x440)
            DbgPrint("CmBattGetPsrData: _PSR method returned %x \n", *PsrData);
    }
    else if (CmBattDebug & 0x44C)
    {
        /* Failure */
        DbgPrint("CmBattGetPsrData: Failed _PSR method - Status (0x%x)\n", Status);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CmBattGetStaData(IN PDEVICE_OBJECT DeviceObject,
                 OUT PULONG StaData)
{
    NTSTATUS Status;
    ACPI_EVAL_OUTPUT_BUFFER OutputBuffer;
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    PAGED_CODE();
    if (CmBattDebug & 0x40)
        DbgPrint("CmBattGetStaData: Entered with Pdo %x Tid %x\n",
                 DeviceObject, KeGetCurrentThread());

    /* Initialize to zero */
    ASSERT(StaData != NULL);
    *StaData = 0;

    /* Request the _STA method */
    *(PULONG)InputBuffer.MethodName = 'ATS_';
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    /* Send it to ACPI */
    Status = CmBattSendDownStreamIrp(DeviceObject,
                                     IOCTL_ACPI_EVAL_METHOD,
                                     &InputBuffer,
                                     sizeof(InputBuffer),
                                     &OutputBuffer,
                                     sizeof(OutputBuffer));
    if (NT_SUCCESS(Status))
    {
        /* Read the result */
        Status = GetDwordElement(OutputBuffer.Argument, StaData);
        if (CmBattDebug & 0x440)
            DbgPrint("CmBattGetStaData: _STA method returned %x \n", *StaData);
    }
    else if (CmBattDebug & 0x44C)
    {
        /* Failure */
        DbgPrint("CmBattGetStaData: Failed _STA method - Status (0x%x)\n", Status);
        Status = STATUS_NO_SUCH_DEVICE;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CmBattGetUniqueId(IN PDEVICE_OBJECT DeviceObject,
                  OUT PULONG UniqueId)
{
    NTSTATUS Status;
    ACPI_EVAL_OUTPUT_BUFFER OutputBuffer;
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    PAGED_CODE();
    if (CmBattDebug & 0x40)
        DbgPrint("CmBattGetUniqueId: Entered with Pdo %x Tid %x\n",
                 DeviceObject, KeGetCurrentThread());

    /* Initialize to zero */
    ASSERT(UniqueId != NULL);
    *UniqueId = 0;

    /* Request the _UID method */
    *(PULONG)InputBuffer.MethodName = 'DIU_';
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    /* Send it to ACPI */
    Status = CmBattSendDownStreamIrp(DeviceObject,
                                     IOCTL_ACPI_EVAL_METHOD,
                                     &InputBuffer,
                                     sizeof(InputBuffer),
                                     &OutputBuffer,
                                     sizeof(OutputBuffer));
    if (NT_SUCCESS(Status))
    {
        /* Read the result */
        Status = GetDwordElement(OutputBuffer.Argument, UniqueId);
        if (CmBattDebug & 0x440)
            DbgPrint("CmBattGetUniqueId: _UID method returned %x \n", *UniqueId);
    }
    else if (CmBattDebug & 0x44C)
    {
        /* Failure */
        DbgPrint("CmBattGetUniqueId: Failed _UID method - Status (0x%x)\n", Status);
        Status = STATUS_NO_SUCH_DEVICE;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CmBattSetTripPpoint(IN PCMBATT_DEVICE_EXTENSION DeviceExtension,
                    IN ULONG AlarmValue)
{
    NTSTATUS Status;
    ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER InputBuffer;
    PAGED_CODE();
    if (CmBattDebug & 0x440)
        DbgPrint("CmBattSetTripPpoint: _BTP Alarm Value %x Device %x Tid %x\n",
                 AlarmValue, DeviceExtension->DeviceId, KeGetCurrentThread);

    /* Request the _BTP method */
    *(PULONG)InputBuffer.MethodName = 'PTB_';
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE;
    InputBuffer.IntegerArgument = AlarmValue;

    /* Send it to ACPI */
    Status = CmBattSendDownStreamIrp(DeviceExtension->AttachedDevice,
                                     IOCTL_ACPI_EVAL_METHOD,
                                     &InputBuffer,
                                     sizeof(InputBuffer),
                                     NULL,
                                     0);
    if (!(NT_SUCCESS(Status)) && (CmBattDebug & 0x440))
        DbgPrint("CmBattSetTripPpoint: Failed _BTP method on device %x - Status (0x%x)\n",
                 DeviceExtension->DeviceId, Status);

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CmBattGetBifData(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                 PACPI_BIF_DATA BifData)
{
    ACPI_PACKAGE_FIELD BifFields[] = {
        { "PowerUnit", FALSE, &BifData->PowerUnit },
        { "DesignCapacity", FALSE, &BifData->DesignCapacity },
        { "LastFullCapacity", FALSE, &BifData->LastFullCapacity },
        { "BatteryTechnology", FALSE, &BifData->BatteryTechnology },
        { "DesignVoltage", FALSE, &BifData->DesignVoltage },
        { "DesignCapacityWarning", FALSE, &BifData->DesignCapacityWarning },
        { "DesignCapacityLow", FALSE, &BifData->DesignCapacityLow },
        { "BatteryCapacityGranularity1", FALSE, &BifData->BatteryCapacityGranularity1 },
        { "BatteryCapacityGranularity2", FALSE, &BifData->BatteryCapacityGranularity2 },
        { "ModelNumber", TRUE, &BifData->ModelNumber },
        { "SerialNumber", TRUE, &BifData->SerialNumber },
        { "BatteryType", TRUE, &BifData->BatteryType },
        { "OemInfo", TRUE, &BifData->OemInfo },
    };
    PAGED_CODE();

    if (CmBattDebug & CMBATT_ACPI_ENTRY_EXIT)
        DbgPrint("CmBattGetBifData: Buffer (0x%x) Device %x Tid %x\n",
                 BifData, DeviceExtension->DeviceId, KeGetCurrentThread());

    /* Request the _BIF method */
    /* Note that _BIF method is deprecated since ACPI 4.0, and replaced by _BIX method.
     * However, VirtualBox 7.0 only support _BIF method.
     */
    return CmBattCallAcpiPackage("CmBattGetBifData",
                                 DeviceExtension,
                                 'FIB_',
                                 512,
                                 BifFields,
                                 RTL_NUMBER_OF(BifFields));
}

/**
 * @brief
 * Retrieves the eXtended static battery information from the
 * ACPI _BIX method.
 *
 * @param[in] DeviceExtension
 * A pointer to a Control Method (CM) battery device extension.
 * It is used to send the ACPI method evaluation operation
 * to the ACPI driver of which it is attached to this CM battery.
 *
 * @param[out] BixData
 * A pointer to a structure that contains the _BIX data fields,
 * returned to caller.
 *
 * @return
 * Returns STATUS_SUCCESS if the operation has succeeded successfully,
 * otherwise a failure NTSTATUS code is returned.
 */
NTSTATUS
NTAPI
CmBattGetBixData(
    _In_ PCMBATT_DEVICE_EXTENSION DeviceExtension,
    _Out_ PACPI_BIX_DATA BixData)
{
    ACPI_PACKAGE_FIELD BixFields[] = {
        { "Revision", FALSE, &BixData->Revision },
        { "PowerUnit", FALSE, &BixData->PowerUnit },
        { "DesignCapacity", FALSE, &BixData->DesignCapacity },
        { "LastFullCapacity", FALSE, &BixData->LastFullCapacity },
        { "BatteryTechnology", FALSE, &BixData->BatteryTechnology },
        { "DesignVoltage", FALSE, &BixData->DesignVoltage },
        { "DesignCapacityWarning", FALSE, &BixData->DesignCapacityWarning },
        { "DesignCapacityLow", FALSE, &BixData->DesignCapacityLow },
        { "CycleCount", FALSE, &BixData->CycleCount },
        { "Accuracy", FALSE, &BixData->Accuracy },
        { "MaxSampleTime", FALSE, &BixData->MaxSampleTime },
        { "MinSampleTime", FALSE, &BixData->MinSampleTime },
        { "MaxAverageInterval", FALSE, &BixData->MaxAverageInterval },
        { "MinAverageInterval", FALSE, &BixData->MinAverageInterval },
        { "BatteryCapacityGranularity1", FALSE, &BixData->BatteryCapacityGranularity1 },
        { "BatteryCapacityGranularity2", FALSE, &BixData->BatteryCapacityGranularity2 },
        { "ModelNumber", TRUE, &BixData->ModelNumber },
        { "SerialNumber", TRUE, &BixData->SerialNumber },
        { "BatteryType", TRUE, &BixData->BatteryType },
        { "OemInfo", TRUE, &BixData->OemInfo },
        { "SwapCapability", FALSE, &BixData->SwapCapability },
    };
    PAGED_CODE();

    if (CmBattDebug & CMBATT_ACPI_ENTRY_EXIT)
    {
        DbgPrint("CmBattGetBixData: Buffer (0x%x) Device %x Tid %x\n",
                 BixData, DeviceExtension->DeviceId, KeGetCurrentThread());
    }

    /* Request the ACPI driver to get the _BIX data for us */
    return CmBattCallAcpiPackage("CmBattGetBifData",
                                 DeviceExtension,
                                 'XIB_',
                                 512,
                                 BixFields,
                                 RTL_NUMBER_OF(BixFields));
}

NTSTATUS
NTAPI
CmBattGetBstData(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                 PACPI_BST_DATA BstData)
{
    ACPI_PACKAGE_FIELD BstFields[] = {
        { "State", FALSE, &BstData->State },
        { "PresentRate", FALSE, &BstData->PresentRate },
        { "RemainingCapacity", FALSE, &BstData->RemainingCapacity },
        { "PresentVoltage", FALSE, &BstData->PresentVoltage },
    };
    PAGED_CODE();

    if (CmBattDebug & CMBATT_ACPI_ENTRY_EXIT)
        DbgPrint("CmBattGetBstData: Buffer (0x%x) Device %x Tid %x\n",
                 BstData, DeviceExtension->DeviceId, KeGetCurrentThread());


    return CmBattCallAcpiPackage("CmBattGetBstData",
                                 DeviceExtension,
                                 'TSB_',
                                 512,
                                 BstFields,
                                 RTL_NUMBER_OF(BstFields));
}

/* EOF */
