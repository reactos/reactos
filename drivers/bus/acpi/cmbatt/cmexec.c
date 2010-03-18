/*
 * PROJECT:         ReactOS ACPI-Compliant Control Method Battery
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/cmbatt/cmexec.c
 * PURPOSE:         ACPI Method Execution/Evaluation Glue
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "cmbatt.h"

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
GetDwordElement(PACPI_METHOD_ARGUMENT Argument,
                PULONG Value)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
GetStringElement(PACPI_METHOD_ARGUMENT Argument,
                 PCHAR Value)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattGetPsrData(PDEVICE_OBJECT DeviceObject,
                 PULONG PsrData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED; 
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

NTSTATUS
NTAPI
CmBattGetStaData(PDEVICE_OBJECT DeviceObject,
                 PULONG StaData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattGetUniqueId(PDEVICE_OBJECT DeviceObject,
                  PULONG UniqueId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;  
}

NTSTATUS
NTAPI
CmBattSetTripPpoint(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                    ULONG AlarmValue)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattSendDownStreamIrp(PDEVICE_OBJECT DeviceObject,
                        ULONG IoControlCode,
                        PVOID InputBuffer,
                        ULONG InputBufferLength,
                        PACPI_EVAL_OUTPUT_BUFFER OutputBuffer,
                        ULONG OutputBufferLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
     
/* EOF */
