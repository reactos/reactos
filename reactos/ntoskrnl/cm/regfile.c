/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/regfile.c
 * PURPOSE:         Registry file manipulation routines
 *
 * PROGRAMMERS:     Casper Hornstrup
 *                  Eric Kohl
 *                  Filip Navara
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>
#include "cm.h"

/* LOCAL MACROS *************************************************************/

#define ABS_VALUE(V) (((V) < 0) ? -(V) : (V))
#define REG_DATA_SIZE_MASK                 0x7FFFFFFF
#define REG_DATA_IN_OFFSET                 0x80000000

/* FUNCTIONS ****************************************************************/

NTSTATUS
CmiLoadHive(IN POBJECT_ATTRIBUTES KeyObjectAttributes,
            IN PCUNICODE_STRING FileName,
            IN ULONG Flags)
{
    PCMHIVE Hive = NULL;
    NTSTATUS Status;
    BOOLEAN Allocate = TRUE;

    DPRINT ("CmiLoadHive(Filename %wZ)\n", FileName);

    if (Flags & ~REG_NO_LAZY_FLUSH) return STATUS_INVALID_PARAMETER;

    Status = CmpInitHiveFromFile(FileName,
                                 (Flags & REG_NO_LAZY_FLUSH) ? HIVE_NO_SYNCH : 0,
                                 &Hive,
                                 &Allocate,
                                 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmpInitHiveFromFile() failed (Status %lx)\n", Status);
        if (Hive) ExFreePool(Hive);
        return Status;
    }

    Status = CmiConnectHive(KeyObjectAttributes, Hive);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1 ("CmiConnectHive() failed (Status %lx)\n", Status);
        //      CmiRemoveRegistryHive (Hive);
    }

    DPRINT ("CmiLoadHive() done\n");
    return Status;
}

VOID
CmCloseHiveFiles(PCMHIVE RegistryHive)
{
    ZwClose(RegistryHive->FileHandles[HFILE_TYPE_PRIMARY]);
    ZwClose(RegistryHive->FileHandles[HFILE_TYPE_LOG]);
}

NTSTATUS
CmiFlushRegistryHive(PCMHIVE RegistryHive)
{
    BOOLEAN Success;
    NTSTATUS Status;
    ULONG Disposition;

    ASSERT(!IsNoFileHive(RegistryHive));

    if (RtlFindSetBits(&RegistryHive->Hive.DirtyVector, 1, 0) == ~0)
    {
        return(STATUS_SUCCESS);
    }

    Status = CmpOpenHiveFiles(&RegistryHive->FileFullPath,
                              L".LOG",
                              &RegistryHive->FileHandles[HFILE_TYPE_PRIMARY],
                              &RegistryHive->FileHandles[HFILE_TYPE_LOG],
                              &Disposition,
                              &Disposition,
                              FALSE,
                              FALSE,
                              TRUE,
                              NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Success = HvSyncHive(&RegistryHive->Hive);

    CmCloseHiveFiles(RegistryHive);

    return Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS
CmiScanKeyForValue(IN PCMHIVE RegistryHive,
                   IN PCM_KEY_NODE KeyCell,
                   IN PUNICODE_STRING ValueName,
                   OUT PCM_KEY_VALUE *ValueCell,
                   OUT HCELL_INDEX *ValueCellOffset)
{
    HCELL_INDEX CellIndex;

    /* Assume failure */
    *ValueCell = NULL;
    if (ValueCellOffset) *ValueCellOffset = HCELL_NIL;

    /* Call newer Cm API */
    CellIndex = CmpFindValueByName(&RegistryHive->Hive, KeyCell, ValueName);
    if (CellIndex == HCELL_NIL) return STATUS_OBJECT_NAME_NOT_FOUND;

    /* Otherwise, get the cell data back too */
    if (ValueCellOffset) *ValueCellOffset = CellIndex;
    *ValueCell = (PCM_KEY_VALUE)HvGetCell(&RegistryHive->Hive, CellIndex);
    return STATUS_SUCCESS;
}

/* EOF */
