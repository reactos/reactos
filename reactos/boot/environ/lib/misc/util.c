/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/misc/util.c
 * PURPOSE:         Boot Library Utility Functions
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

PVOID UtlRsdt;
PVOID UtlXsdt;

PVOID UtlMcContext;
PVOID UtlMcDisplayMessageRoutine;
PVOID UtlMcUpdateMessageRoutine;

PVOID UtlProgressRoutine;
PVOID UtlProgressContext;
PVOID UtlProgressInfoRoutine;
ULONG UtlProgressGranularity;
ULONG UtlCurrentPercentComplete;
ULONG UtlNextUpdatePercentage;
BOOLEAN UtlProgressNeedsInfoUpdate;
PVOID UtlProgressInfo;

/* FUNCTIONS *****************************************************************/

/*++
 * @name EfiGetEfiStatusCode
 *
 *     The EfiGetEfiStatusCode routine converts an NT Status to an EFI status.
 *
 * @param  Status
 *         NT Status code to be converted.
 *
 * @remark Only certain, specific NT status codes are converted to EFI codes.
 *
 * @return The corresponding EFI Status code, EFI_NO_MAPPING otherwise.
 *
 *--*/
EFI_STATUS
EfiGetEfiStatusCode(
    _In_ NTSTATUS Status
    )
{
    switch (Status)
    {
        case STATUS_NOT_SUPPORTED:
            return EFI_UNSUPPORTED;
        case STATUS_DISK_FULL:
            return EFI_VOLUME_FULL;
        case STATUS_INSUFFICIENT_RESOURCES:
            return EFI_OUT_OF_RESOURCES;
        case STATUS_MEDIA_WRITE_PROTECTED:
            return EFI_WRITE_PROTECTED;
        case STATUS_DEVICE_NOT_READY:
            return EFI_NOT_STARTED;
        case STATUS_DEVICE_ALREADY_ATTACHED:
            return EFI_ALREADY_STARTED;
        case STATUS_MEDIA_CHANGED:
            return EFI_MEDIA_CHANGED;
        case STATUS_INVALID_PARAMETER:
            return EFI_INVALID_PARAMETER;
        case STATUS_ACCESS_DENIED:
            return EFI_ACCESS_DENIED;
        case STATUS_BUFFER_TOO_SMALL:
            return EFI_BUFFER_TOO_SMALL;
        case STATUS_DISK_CORRUPT_ERROR:
            return EFI_VOLUME_CORRUPTED;
        case STATUS_REQUEST_ABORTED:
            return EFI_ABORTED;
        case STATUS_NO_MEDIA:
            return EFI_NO_MEDIA;
        case STATUS_IO_DEVICE_ERROR:
            return EFI_DEVICE_ERROR;
        case STATUS_INVALID_BUFFER_SIZE:
            return EFI_BAD_BUFFER_SIZE;
        case STATUS_NOT_FOUND:
            return EFI_NOT_FOUND;
        case STATUS_DRIVER_UNABLE_TO_LOAD:
            return EFI_LOAD_ERROR;
        case STATUS_NO_MATCH:
            return EFI_NO_MAPPING;
        case STATUS_SUCCESS:
            return EFI_SUCCESS;
        case STATUS_TIMEOUT:
            return EFI_TIMEOUT;
        default:
            return EFI_NO_MAPPING;
    }
}

/*++
 * @name EfiGetNtStatusCode
 *
 *     The EfiGetNtStatusCode routine converts an EFI Status to an NT status.
 *
 * @param  EfiStatus
 *         EFI Status code to be converted.
 *
 * @remark Only certain, specific EFI status codes are converted to NT codes.
 *
 * @return The corresponding NT Status code, STATUS_UNSUCCESSFUL otherwise.
 *
 *--*/
NTSTATUS
EfiGetNtStatusCode (
    _In_ EFI_STATUS EfiStatus
    )
{
    switch (EfiStatus)
    {
        case EFI_NOT_READY:
        case EFI_NOT_FOUND:
            return STATUS_NOT_FOUND;
        case EFI_NO_MEDIA:
            return STATUS_NO_MEDIA;
        case EFI_MEDIA_CHANGED:
            return STATUS_MEDIA_CHANGED;
        case EFI_ACCESS_DENIED:
        case EFI_SECURITY_VIOLATION:
            return STATUS_ACCESS_DENIED;
        case EFI_TIMEOUT:
        case EFI_NO_RESPONSE:
            return STATUS_TIMEOUT;
        case EFI_NO_MAPPING:
            return STATUS_NO_MATCH;
        case EFI_NOT_STARTED:
            return STATUS_DEVICE_NOT_READY;
        case EFI_ALREADY_STARTED:
            return STATUS_DEVICE_ALREADY_ATTACHED;
        case EFI_ABORTED:
            return STATUS_REQUEST_ABORTED;
        case EFI_VOLUME_FULL:
            return STATUS_DISK_FULL;
        case EFI_DEVICE_ERROR:
            return STATUS_IO_DEVICE_ERROR;
        case EFI_WRITE_PROTECTED:
            return STATUS_MEDIA_WRITE_PROTECTED;
        /* @FIXME: ReactOS Headers don't yet have this */
        //case EFI_OUT_OF_RESOURCES:
            //return STATUS_INSUFFICIENT_NVRAM_RESOURCES;
        case EFI_VOLUME_CORRUPTED:
            return STATUS_DISK_CORRUPT_ERROR;
        case EFI_BUFFER_TOO_SMALL:
            return STATUS_BUFFER_TOO_SMALL;
        case EFI_SUCCESS:
            return STATUS_SUCCESS;
        case  EFI_LOAD_ERROR:
            return STATUS_DRIVER_UNABLE_TO_LOAD;
        case EFI_INVALID_PARAMETER:
            return STATUS_INVALID_PARAMETER;
        case EFI_UNSUPPORTED:
            return STATUS_NOT_SUPPORTED;
        case EFI_BAD_BUFFER_SIZE:
            return STATUS_INVALID_BUFFER_SIZE;
        default:
            return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
BlUtlInitialize (
    VOID
    )
{
    UtlRsdt = 0;
    UtlXsdt = 0;

    UtlMcContext = 0;
    UtlMcDisplayMessageRoutine = 0;
    UtlMcUpdateMessageRoutine = 0;

    UtlProgressRoutine = 0;
    UtlProgressContext = 0;
    UtlProgressInfoRoutine = 0;
    UtlProgressGranularity = 0;
    UtlCurrentPercentComplete = 0;
    UtlNextUpdatePercentage = 0;
    UtlProgressNeedsInfoUpdate = 0;
    UtlProgressInfo = 0;

    return STATUS_SUCCESS;
}