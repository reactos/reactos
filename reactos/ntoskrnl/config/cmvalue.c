/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmvalue.c
 * PURPOSE:         Configuration Manager - Cell Values
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

HCELL_INDEX
NTAPI
CmpFindValueByName(IN PHHIVE Hive,
                   IN PCM_KEY_NODE KeyNode,
                   IN PUNICODE_STRING Name)
{
    HCELL_INDEX CellIndex;

    /* Call the main function */
    if (!CmpFindNameInList(Hive,
                           &KeyNode->ValueList,
                           Name,
                           NULL,
                           &CellIndex))
    {
        /* Santy check */
        ASSERT(CellIndex == HCELL_NIL);
    }

    /* Return the index */
    return CellIndex;
}

BOOLEAN
NTAPI
CmpGetValueData(IN PHHIVE Hive,
                IN PCM_KEY_VALUE Value,
                IN PULONG Length,
                OUT PVOID *Buffer,
                OUT PBOOLEAN BufferAllocated,
                OUT PHCELL_INDEX CellToRelease)
{
    PAGED_CODE();

    /* Sanity check */
    ASSERT(Value->Signature == CM_KEY_VALUE_SIGNATURE);

    /* Set failure defaults */
    *BufferAllocated = FALSE;
    *Buffer = NULL;
    *CellToRelease = HCELL_NIL;

    /* Check if this is a small key */
    if (CmpIsKeyValueSmall(Length, Value->DataLength))
    {
        /* Return the data immediately */
        *Buffer = &Value->Data;
        return TRUE;
    }

    /* Check if this is a big cell */
    if (CmpIsKeyValueBig(Hive, *Length))
    {
        /* FIXME: We don't support big cells */
        DPRINT1("Unsupported cell type!\n");
        while (TRUE);
    }

    /* Get the data from the cell */
    *Buffer = HvGetCell(Hive, Value->Data);
    if (!(*Buffer)) return FALSE;

    /* Return success and the cell to be released */
    *CellToRelease = Value->Data;
    return TRUE;
}

PCELL_DATA
NTAPI
CmpValueToData(IN PHHIVE Hive,
               IN PCM_KEY_VALUE Value,
               OUT PULONG Length)
{
    PCELL_DATA Buffer;
    BOOLEAN BufferAllocated;
    HCELL_INDEX CellToRelease;
    PAGED_CODE();

    /* Sanity check */
    ASSERT(Hive->ReleaseCellRoutine == NULL);

    /* Get the actual data */
    if (!CmpGetValueData(Hive,
                         Value,
                         Length,
                         (PVOID)&Buffer,
                         &BufferAllocated,
                         &CellToRelease))
    {
        /* We failed */
        ASSERT(BufferAllocated == FALSE);
        ASSERT(Buffer == NULL);
        return NULL;
    }

    /* This should never happen!*/
    if (BufferAllocated)
    {
        /* Free the buffer and bugcheck */
        ExFreePool(Buffer);
        KEBUGCHECKEX(REGISTRY_ERROR, 8, 0, (ULONG_PTR)Hive, (ULONG_PTR)Value);
    }

    /* Otherwise, return the cell data */
    return Buffer;
}
