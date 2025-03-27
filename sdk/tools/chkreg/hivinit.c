/*
 * PROJECT:     ReactOS Tools
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Check Registry Utility I/O hive initialization code
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "chkreg.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
ChkRegInitializeHive(
    IN OUT PHHIVE RegistryHive,
    IN PVOID HiveData)
{
    NTSTATUS Status;

    Status = HvInitialize(RegistryHive,
                          HINIT_MEMORY,
                          HIVE_NOLAZYFLUSH,
                          HFILE_TYPE_PRIMARY,
                          HiveData,
                          CmpAllocate,
                          CmpFree,
                          CmpFileSetSize,
                          CmpFileWrite,
                          CmpFileRead,
                          CmpFileFlush,
                          1,
                          NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    return STATUS_SUCCESS;
}

/* EOF */
