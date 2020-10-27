/*
 * PROJECT:     ReactOS Storage Stack / SCSIPORT storage port library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Logical Unit (PDO) functions
 * COPYRIGHT:   Eric Kohl (eric.kohl@reactos.org)
 *              Aleksey Bragin (aleksey@reactos.org)
 */

#include "scsiport.h"

#define NDEBUG
#include <debug.h>


PSCSI_PORT_LUN_EXTENSION
SpiAllocateLunExtension(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    ULONG LunExtensionSize;

    DPRINT("SpiAllocateLunExtension(%p)\n", DeviceExtension);

    /* Round LunExtensionSize first to the sizeof LONGLONG */
    LunExtensionSize = (DeviceExtension->LunExtensionSize +
                        sizeof(LONGLONG) - 1) & ~(sizeof(LONGLONG) - 1);

    LunExtensionSize += sizeof(SCSI_PORT_LUN_EXTENSION);
    DPRINT("LunExtensionSize %lu\n", LunExtensionSize);

    LunExtension = ExAllocatePoolWithTag(NonPagedPool, LunExtensionSize, TAG_SCSIPORT);
    if (LunExtension == NULL)
    {
        DPRINT1("Out of resources!\n");
        return NULL;
    }

    /* Zero everything */
    RtlZeroMemory(LunExtension, LunExtensionSize);

    /* Initialize a list of requests */
    InitializeListHead(&LunExtension->SrbInfo.Requests);

    /* Initialize timeout counter */
    LunExtension->RequestTimeout = -1;

    /* Set maximum queue size */
    LunExtension->MaxQueueCount = 256;

    /* Initialize request queue */
    KeInitializeDeviceQueue(&LunExtension->DeviceQueue);

    return LunExtension;
}

PSCSI_PORT_LUN_EXTENSION
SpiGetLunExtension(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;

    DPRINT("SpiGetLunExtension(%p %u %u %u) called\n",
        DeviceExtension, PathId, TargetId, Lun);

    /* Get appropriate list */
    LunExtension = DeviceExtension->LunExtensionList[(TargetId + Lun) % LUS_NUMBER];

    /* Iterate it until we find what we need */
    while (LunExtension)
    {
        if (LunExtension->TargetId == TargetId &&
            LunExtension->Lun == Lun &&
            LunExtension->PathId == PathId)
        {
            /* All matches, return */
            return LunExtension;
        }

        /* Advance to the next item */
        LunExtension = LunExtension->Next;
    }

    /* We did not find anything */
    DPRINT("Nothing found\n");
    return NULL;
}

PSCSI_REQUEST_BLOCK_INFO
SpiGetSrbData(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun,
    _In_ UCHAR QueueTag)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;

    if (QueueTag == SP_UNTAGGED)
    {
        /* Untagged request, get LU and return pointer to SrbInfo */
        LunExtension = SpiGetLunExtension(DeviceExtension,
                                          PathId,
                                          TargetId,
                                          Lun);

        /* Return NULL in case of error */
        if (!LunExtension)
            return(NULL);

        /* Return the pointer to SrbInfo */
        return &LunExtension->SrbInfo;
    }
    else
    {
        /* Make sure the tag is valid, if it is - return the data */
        if (QueueTag > DeviceExtension->SrbDataCount || QueueTag < 1)
            return NULL;
        else
            return &DeviceExtension->SrbInfo[QueueTag -1];
    }
}
