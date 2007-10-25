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

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
CmpMarkValueDataDirty(IN PHHIVE Hive,
                      IN PCM_KEY_VALUE Value)
{
    ULONG KeySize;
    PAGED_CODE();

    /* Make sure there's actually any data */
    if (Value->Data != HCELL_NIL)
    {
        /* If this is a small key, there's no need to have it dirty */
        if (CmpIsKeyValueSmall(&KeySize, Value->DataLength)) return TRUE;

        /* Check if this is a big key */
        ASSERT_VALUE_BIG(Hive, KeySize);

        /* Normal value, just mark it dirty */
        HvMarkCellDirty(Hive, Value->Data, FALSE);
    }

    /* Operation complete */
    return TRUE;
}

BOOLEAN
NTAPI
CmpFreeValueData(IN PHHIVE Hive,
                 IN HCELL_INDEX DataCell,
                 IN ULONG DataLength)
{
    ULONG KeySize;
    PAGED_CODE();

    /* If this is a small key, the data is built-in */
    if (!CmpIsKeyValueSmall(&KeySize, DataLength))
    {
        /* If there's no data cell, there's nothing to do */
        if (DataCell == HCELL_NIL) return TRUE;

        /* Make sure the data cell is allocated */
        //ASSERT(HvIsCellAllocated(Hive, DataCell));

        /* Unsupported value type */
        ASSERT_VALUE_BIG(Hive, KeySize);

        /* Normal value, just free the data cell */
        HvFreeCell(Hive, DataCell);
    }

    /* Operation complete */
    return TRUE;
}

BOOLEAN
NTAPI
CmpFreeValue(IN PHHIVE Hive,
             IN HCELL_INDEX Cell)
{
    PCM_KEY_VALUE Value;
    PAGED_CODE();

    /* Get the cell data */
    Value = (PCM_KEY_VALUE)HvGetCell(Hive, Cell);
    if (!Value) ASSERT(FALSE);

    /* Free it */
    if (!CmpFreeValueData(Hive, Value->Data, Value->DataLength))
    {
        /* We failed to free the data, return failure */
        HvReleaseCell(Hive, Cell);
        return FALSE;
    }

    /* Release the cell and free it */
    HvReleaseCell(Hive, Cell);
    HvFreeCell(Hive, Cell);
    return TRUE;
}

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

    /* Unsupported */
    ASSERT_VALUE_BIG(Hive, *Length);

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

NTSTATUS
NTAPI
CmpAddValueToList(IN PHHIVE Hive,
                  IN HCELL_INDEX ValueCell,
                  IN ULONG Index,
                  IN ULONG Type,
                  IN OUT PCHILD_LIST ChildList)
{
    HCELL_INDEX ListCell;
    ULONG ChildCount, Length, i;
    PCELL_DATA CellData;
    PAGED_CODE();

    /* Sanity check */
    ASSERT((((LONG)Index) >= 0) && (Index <= ChildList->Count));

    /* Get the number of entries in the child list */
    ChildCount = ChildList->Count;
    ChildCount++;
    if (ChildCount > 1)
    {
        /* The cell should be dirty at this point */
        ASSERT(HvIsCellDirty(Hive, ChildList->List));

        /* Check if we have less then 100 children */
        if (ChildCount < 100)
        {
            /* Allocate just enough as requested */
            Length = ChildCount * sizeof(HCELL_INDEX);
        }
        else
        {
            /* Otherwise, we have quite a few, so allocate a batch */
            Length = ROUND_UP(ChildCount, 100) * sizeof(HCELL_INDEX);
            if (Length > HBLOCK_SIZE)
            {
                /* But make sure we don't allocate beyond our block size */
                Length = ROUND_UP(Length, HBLOCK_SIZE);
            }
        }

        /* Perform the allocation */
        ListCell = HvReallocateCell(Hive, ChildList->List, Length);
    }
    else
    {
        /* This is our first child, so allocate a single cell */
        ListCell = HvAllocateCell(Hive, sizeof(HCELL_INDEX), Type, HCELL_NIL);
    }

    /* Fail if we couldn't get a cell */
    if (ListCell == HCELL_NIL) return STATUS_INSUFFICIENT_RESOURCES;

    /* Set this cell as the child list's list cell */
    ChildList->List = ListCell;

    /* Get the actual key list memory */
    CellData = HvGetCell(Hive, ListCell);
    ASSERT(CellData != NULL);

    /* Loop all the children */
    for (i = ChildCount - 1; i > Index; i--)
    {
        /* Move them all down */
        CellData->u.KeyList[i] = CellData->u.KeyList[i - 1];
    }

    /* Insert us on top now */
    CellData->u.KeyList[Index] = ValueCell;
    ChildList->Count = ChildCount;

    /* Release the list cell and make sure the value cell is dirty */
    HvReleaseCell(Hive, ListCell);
    ASSERT(HvIsCellDirty(Hive, ValueCell));

    /* We're done here */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmpSetValueDataNew(IN PHHIVE Hive,
                   IN PVOID Data,
                   IN ULONG DataSize,
                   IN ULONG StorageType,
                   IN HCELL_INDEX ValueCell,
                   OUT PHCELL_INDEX DataCell)
{
    PCELL_DATA CellData;
    PAGED_CODE();
    ASSERT(DataSize > CM_KEY_VALUE_SMALL);

    /* Check if this is a big key */
    ASSERT_VALUE_BIG(Hive, DataSize);

    /* Allocate a data cell */
    *DataCell = HvAllocateCell(Hive, DataSize, StorageType, HCELL_NIL);
    if (*DataCell == HCELL_NIL) return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the actual data */
    CellData = HvGetCell(Hive, *DataCell);
    if (!CellData) ASSERT(FALSE);

    /* Copy our buffer into it */
    RtlCopyMemory(CellData, Data, DataSize);

    /* All done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmpRemoveValueFromList(IN PHHIVE Hive,
                       IN ULONG Index,
                       IN OUT PCHILD_LIST ChildList)
{
    ULONG Count;
    PCELL_DATA CellData;
    HCELL_INDEX NewCell;
    PAGED_CODE();

    /* Sanity check */
    ASSERT((((LONG)Index) >= 0) && (Index <= ChildList->Count));

    /* Get the new count after removal */
    Count = ChildList->Count - 1;
    if (Count > 0)
    {
        /* Get the actual list array */
        CellData = HvGetCell(Hive, ChildList->List);
        if (!CellData) return STATUS_INSUFFICIENT_RESOURCES;

        /* Make sure cells data have been made dirty */
        ASSERT(HvIsCellDirty(Hive, ChildList->List));
        ASSERT(HvIsCellDirty(Hive, CellData->u.KeyList[Index]));

        /* Loop the list */
        while (Index < Count)
        {
            /* Move everything up */
            CellData->u.KeyList[Index] = CellData->u.KeyList[Index + 1];
            Index++;
        }

        /* Re-allocate the cell for the list by decreasing the count */
        NewCell = HvReallocateCell(Hive,
                                   ChildList->List,
                                   Count * sizeof(HCELL_INDEX));
        ASSERT(NewCell != HCELL_NIL);
        HvReleaseCell(Hive,ChildList->List);

        /* Update the list cell */
        ChildList->List = NewCell;
    }
    else
    {
        /* Otherwise, we were the last entry, so free the list entirely */
        HvFreeCell(Hive, ChildList->List);
        ChildList->List = HCELL_NIL;
    }

    /* Update the child list with the new count */
    ChildList->Count = Count;
    return STATUS_SUCCESS;
}
