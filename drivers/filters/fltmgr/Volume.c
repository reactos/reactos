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


NTSTATUS
FLTAPI
FltEnumerateVolumes(
    _In_ PFLT_FILTER Filter,
    _Out_writes_to_opt_(VolumeListSize,*NumberVolumesReturned) PFLT_VOLUME *VolumeList,
    _In_ ULONG VolumeListSize,
    _Out_ PULONG NumberVolumesReturned)
{
    ULONG i;
    PFLTP_FRAME Frame;
    PFLT_VOLUME Volume;
    PLIST_ENTRY ListEntry;
    ULONG NumberOfVolumes = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    Frame = Filter->Frame;

    /* Lock the attached volumes list */
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&Frame->AttachedVolumes.rLock, TRUE);

    /* If it's not empty */
    if (!IsListEmpty(&Frame->AttachedVolumes.rList))
    {
        /* Browse every entry */
        for (ListEntry = Frame->AttachedVolumes.rList.Flink;
             ListEntry != &Frame->AttachedVolumes.rList;
             ListEntry = ListEntry->Flink)
        {
            /* Get the volume */
            Volume = CONTAINING_RECORD(ListEntry, FLT_VOLUME, Base.PrimaryLink);

            /* If there's still room in the output buffer */
            if (NumberOfVolumes < VolumeListSize)
            {
                /* Reference the volume and return it */
                FltObjectReference(Volume);
                VolumeList[NumberOfVolumes] = Volume;
            }

            /* We returned one more volume */
            ++NumberOfVolumes;
        }
    }

    /* Release the list */
    ExReleaseResourceLite(&Frame->AttachedVolumes.rLock);
    KeLeaveCriticalRegion();

    /* If we want to return more volumes than we can */
    if (NumberOfVolumes > VolumeListSize)
    {
        /* We will clear output */
        for (i = 0; i < VolumeListSize; ++i)
        {
            FltObjectDereference(VolumeList[i]);
            VolumeList[i] = NULL;
        }

        /* And set failure status */
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    /* Always return the max amount of volumes we want to return */
    *NumberVolumesReturned = NumberOfVolumes;

    /* Done */
    return Status;
}

NTSTATUS
FLTAPI
FltDetachVolume(
    _Inout_ PFLT_FILTER Filter,
    _Inout_ PFLT_VOLUME Volume,
    _In_opt_ PCUNICODE_STRING InstanceName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
FLTAPI
FltAttachVolume(
    _Inout_ PFLT_FILTER Filter,
    _Inout_ PFLT_VOLUME Volume,
    _In_opt_ PCUNICODE_STRING InstanceName,
    _Outptr_opt_result_maybenull_ PFLT_INSTANCE *RetInstance)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
FLTAPI
FltGetVolumeName(
    _In_ PFLT_VOLUME Volume,
    _Inout_opt_ PUNICODE_STRING VolumeName,
    _Out_opt_ PULONG BufferSizeNeeded)
{
    NTSTATUS Status;

    /* Check if caller just probes for size */
    if (VolumeName == NULL)
    {
        /* Totally broken call */
        if (BufferSizeNeeded == NULL)
        {
            return STATUS_INVALID_PARAMETER;
        }

        /* Return the appropriate size and quit */
        *BufferSizeNeeded = Volume->DeviceName.Length;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* We have an output buffer! Assume it's too small */
    Status = STATUS_BUFFER_TOO_SMALL;

    /* If we have output size, fill it */
    if (BufferSizeNeeded != NULL)
    {
        *BufferSizeNeeded = Volume->DeviceName.Length;
    }

    /* Init that we didn't return a thing */
    VolumeName->Length = 0;

    /* If we have enough room, copy and return success */
    if (VolumeName->MaximumLength >= Volume->DeviceName.Length)
    {
        RtlCopyUnicodeString(VolumeName, &Volume->DeviceName);
        Status = STATUS_SUCCESS;
    }

    return Status;
}


/* INTERNAL FUNCTIONS ******************************************************/
