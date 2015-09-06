/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/device.c
 * PURPOSE:         Boot Library Device Management Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

typedef struct _BL_DEVICE_INFORMATION
{
    ULONG Unknown0;
    ULONG Unknown1;
    ULONG Unknown2;
    ULONG Unknown3;
} BL_DEVICE_INFORMATION, *PBL_DEVICE_INFORMATION;

LIST_ENTRY DmRegisteredDevices;
ULONG DmTableEntries;
LIST_ENTRY DmRegisteredDevices;
PVOID* DmDeviceTable;

BL_DEVICE_INFORMATION DmDeviceIoInformation;

/* FUNCTIONS *****************************************************************/

NTSTATUS
BlpDeviceInitialize (
    VOID
    )
{
    NTSTATUS Status;

    /* Initialize the table count and list of devices */
    DmTableEntries = 8;
    InitializeListHead(&DmRegisteredDevices);

    /* Initialize device information */
    DmDeviceIoInformation.Unknown0 = 0;
    DmDeviceIoInformation.Unknown1 = 0;
    DmDeviceIoInformation.Unknown2 = 0;
    DmDeviceIoInformation.Unknown3 = 0;

    /* Allocate the device table */
    DmDeviceTable = BlMmAllocateHeap(DmTableEntries * sizeof(PVOID));
    if (DmDeviceTable)
    {
        /* Clear it */
        RtlZeroMemory(DmDeviceTable, DmTableEntries * sizeof(PVOID));

#if 0
        /* Initialize BitLocker support */
        Status = FvebInitialize();
#else
        Status = STATUS_SUCCESS;
#endif
    }
    else
    {
        /* No memory, we'll fail */
        Status = STATUS_NO_MEMORY;
    }

    /* Return initialization state */
    return Status;
}

