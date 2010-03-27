/*
 * PROJECT:         ReactOS Composite Battery Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/compbatt/compmisc.c
 * PURPOSE:         Miscellaneous Support Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "compbatt.h"

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
BatteryIoctl(IN ULONG IoControlCode, 
             IN PDEVICE_OBJECT DeviceObject,
             IN PVOID InputBuffer,
             IN ULONG InputBufferLength,
             IN PVOID OutputBuffer,
             IN ULONG OutputBufferLength,
             IN BOOLEAN InternalDeviceIoControl)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattGetDeviceObjectPointer(IN PCUNICODE_STRING DeviceName,
                               IN ACCESS_MASK DesiredAccess,
                               OUT PFILE_OBJECT *FileObject,
                               OUT PDEVICE_OBJECT *DeviceObject)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
