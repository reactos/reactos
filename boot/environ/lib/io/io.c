/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/io.c
 * PURPOSE:         Boot Library I/O Management Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

ULONG IoMgrRoutineEntries;
PVOID* IoMgrDestroyRoutineTable;

/* FUNCTIONS *****************************************************************/

NTSTATUS
BlpIoRegisterDestroyRoutine (
    _In_ PBL_IO_DESTROY_ROUTINE DestroyRoutine
    )
{
    ULONG Id;

    return BlTblSetEntry(&IoMgrDestroyRoutineTable,
                         &IoMgrRoutineEntries,
                         DestroyRoutine,
                         &Id,
                         TblDoNotPurgeEntry);
}

NTSTATUS
BlpIoInitialize (
    VOID
    )
{
    NTSTATUS Status;
    ULONG Size;

    /* Allocate the I/O table */
    IoMgrRoutineEntries = 4;
    Size = IoMgrRoutineEntries * sizeof(PVOID);
    IoMgrDestroyRoutineTable = BlMmAllocateHeap(Size);
    if (IoMgrDestroyRoutineTable)
    {
        /* Zero it out */
        RtlZeroMemory(IoMgrDestroyRoutineTable, Size);

        /* Initialize the device manager */
        Status = BlpDeviceInitialize();

        /* Initialize the file manager */
        if (NT_SUCCESS(Status))
        {
            Status = BlpFileInitialize();
        }
    }
    else
    {
        /* No memory */
        Status = STATUS_NO_MEMORY;
    }

    /* Return initialization status */
    return Status;
}


