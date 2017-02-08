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
      
    /* Request the _PSR method */
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
      
    /* Request the _PSR method */
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattGetBstData(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                 PACPI_BST_DATA BstData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
