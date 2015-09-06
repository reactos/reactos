/*
* COPYRIGHT:       See COPYING.ARM in the top level directory
* PROJECT:         ReactOS UEFI Boot Library
* FILE:            boot/environ/lib/io/fat.c
* PURPOSE:         Boot Library FAT File System Management Routines
* PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

PVOID* FatDeviceTable;
ULONG FatDeviceTableEntries;
PWCHAR FatpLongFileName;

/* FUNCTIONS *****************************************************************/

NTSTATUS
FatInitialize (
    VOID
    )
{
    NTSTATUS Status;

    /* Allocate the device table with 2 entries*/
    FatDeviceTableEntries = 2;
    FatDeviceTable = BlMmAllocateHeap(sizeof(PBL_FILE_ENTRY) *
                                      FatDeviceTableEntries);
    if (FatDeviceTable)
    {
        /* Zero it out */
        RtlZeroMemory(FatDeviceTable,
                      sizeof(PBL_FILE_ENTRY) * FatDeviceTableEntries);

        /* Allocate a 512 byte buffer for long file name conversion */
        FatpLongFileName = BlMmAllocateHeap(512);
        Status = FatpLongFileName != NULL ? STATUS_SUCCESS : STATUS_NO_MEMORY;
    }
    else
    {
        /* No memory, fail */
        Status = STATUS_NO_MEMORY;
    }

    /* Return back to caller */
    return Status;
}

