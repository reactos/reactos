/*
* PROJECT:         Filesystem Filter Manager
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/filters/fltmgr/Context.c
* PURPOSE:         Contains routines for the volume
* PROGRAMMERS:     Ged Murphy (gedmurphy@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "fltmgr.h"
#include "fltmgrint.h"

#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/



/* EXPORTED FUNCTIONS ******************************************************/

NTSTATUS
FLTAPI
FltGetVolumeProperties(
    _In_ PFLT_VOLUME Volume,
    _Out_writes_bytes_to_opt_(VolumePropertiesLength, *LengthReturned) PFLT_VOLUME_PROPERTIES VolumeProperties,
    _In_ ULONG VolumePropertiesLength,
    _Out_ PULONG LengthReturned
)
{
    ULONG BufferRequired;
    ULONG BytesWritten;
    PCHAR Ptr;
    NTSTATUS Status;

    /* Calculate the required buffer size */
    BufferRequired = sizeof(FLT_VOLUME_PROPERTIES) +
                     Volume->CDODriverName.Length +
                     Volume->DeviceName.Length +
                     Volume->CDODeviceName.Length;

    /* If we don't have enough buffer to fill in the fixed struct, return with the required size */
    if (VolumePropertiesLength < sizeof(FLT_VOLUME_PROPERTIES))
    {
        *LengthReturned = BufferRequired;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Clear out the buffer */
    RtlZeroMemory(VolumeProperties, sizeof(FLT_VOLUME_PROPERTIES));

    /* Fill in the fixed data */
    VolumeProperties->DeviceType = Volume->DeviceObject->DeviceType;
    VolumeProperties->DeviceObjectFlags = Volume->DeviceObject->Flags;
    VolumeProperties->AlignmentRequirement = Volume->DeviceObject->AlignmentRequirement;
    VolumeProperties->SectorSize = Volume->DeviceObject->SectorSize;
    if (Volume->DiskDeviceObject)
    {
        VolumeProperties->DeviceCharacteristics = Volume->DiskDeviceObject->Characteristics;
    }
    else
    {
        VolumeProperties->DeviceCharacteristics = Volume->DeviceObject->Characteristics;
    }

    /* So far we've written the fixed struct data */
    BytesWritten = sizeof(FLT_VOLUME_PROPERTIES);
    Ptr = (PCHAR)(VolumeProperties + 1);

    /* Make sure we have enough room to add the dynamic data */
    if (VolumePropertiesLength >= BufferRequired)
    {
        /* Add the FS device name */
        VolumeProperties->FileSystemDeviceName.Length = 0;
        VolumeProperties->FileSystemDeviceName.MaximumLength = Volume->CDODeviceName.Length;
        VolumeProperties->FileSystemDeviceName.Buffer = (PWCH)Ptr;
        RtlCopyUnicodeString(&VolumeProperties->FileSystemDeviceName, &Volume->CDODeviceName);
        Ptr += VolumeProperties->FileSystemDeviceName.Length;

        /* Add the driver name */
        VolumeProperties->FileSystemDriverName.Length = 0;
        VolumeProperties->FileSystemDriverName.MaximumLength = Volume->CDODriverName.Length;
        VolumeProperties->FileSystemDriverName.Buffer = (PWCH)Ptr;
        RtlCopyUnicodeString(&VolumeProperties->FileSystemDriverName, &Volume->CDODriverName);
        Ptr += VolumeProperties->FileSystemDriverName.Length;

        /* Add the volume name */
        VolumeProperties->RealDeviceName.Length = 0;
        VolumeProperties->RealDeviceName.MaximumLength = Volume->DeviceName.Length;
        VolumeProperties->RealDeviceName.Buffer = (PWCH)Ptr;
        RtlCopyUnicodeString(&VolumeProperties->RealDeviceName, &Volume->DeviceName);

        BytesWritten = BufferRequired;

        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_BUFFER_OVERFLOW;
    }

    /* Set the number of bytes we wrote and return */
    *LengthReturned = BytesWritten;
    return Status;
}


/* INTERNAL FUNCTIONS ******************************************************/