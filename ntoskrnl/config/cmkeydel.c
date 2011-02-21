/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmkeydel.c
 * PURPOSE:         Configuration Manager - Key Body Deletion
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
CmpMarkKeyDirty(IN PHHIVE Hive,
                IN HCELL_INDEX Cell,
                IN BOOLEAN CheckNoSubkeys)
{
    PCELL_DATA CellData, ListData, SecurityData, ValueData;
    ULONG i;

    /* Get the cell data for our target */
    CellData = HvGetCell(Hive, Cell);
    if (!CellData) return FALSE;

    /* Check if sanity checks requested */
    if (CheckNoSubkeys)
    {
        /* Do them */
        ASSERT(CellData->u.KeyNode.SubKeyCounts[Stable] == 0);
        ASSERT(CellData->u.KeyNode.SubKeyCounts[Volatile] == 0);
    }

    /* If this is an exit hive, there's nothing to do */
    if (CellData->u.KeyNode.Flags & KEY_HIVE_EXIT)
    {
        /* Release the cell and get out */
        HvReleaseCell(Hive, Cell);
        return TRUE;
    }

    /* Otherwise, mark it dirty and release it */
    HvMarkCellDirty(Hive, Cell, FALSE);
    HvReleaseCell(Hive, Cell);

    /* Check if we have a class */
    if (CellData->u.KeyNode.Class != HCELL_NIL)
    {
        /* Mark it dirty */
        HvMarkCellDirty(Hive, CellData->u.KeyNode.Class, FALSE);
    }

    /* Check if we have security */
    if (CellData->u.KeyNode.Security != HCELL_NIL)
    {
        /* Mark it dirty */
        HvMarkCellDirty(Hive, CellData->u.KeyNode.Security, FALSE);

        /* Get the security data and release it */
        SecurityData = HvGetCell(Hive, CellData->u.KeyNode.Security);
        if (!SecurityData) ASSERT(FALSE);
        HvReleaseCell(Hive, CellData->u.KeyNode.Security);

        /* Mark the security links dirty too */
        HvMarkCellDirty(Hive, SecurityData->u.KeySecurity.Flink, FALSE);
        HvMarkCellDirty(Hive, SecurityData->u.KeySecurity.Blink, FALSE);
    }

    /* Check if we have any values */
    if (CellData->u.KeyNode.ValueList.Count > 0)
    {
        /* Dirty the value list */
        HvMarkCellDirty(Hive, CellData->u.KeyNode.ValueList.List, FALSE);

        /* Get the list data itself, and release it */
        ListData = HvGetCell(Hive, CellData->u.KeyNode.ValueList.List);
        if (!ListData) ASSERT(FALSE);
        HvReleaseCell(Hive, CellData->u.KeyNode.ValueList.List);

        /* Loop all values */
        for (i = 0; i < CellData->u.KeyNode.ValueList.Count; i++)
        {
            /* Dirty each value */
            HvMarkCellDirty(Hive, ListData->u.KeyList[i], FALSE);

            /* Get the value data and release it */
            ValueData = HvGetCell(Hive, ListData->u.KeyList[i]);
            ASSERT(ValueData);
            HvReleaseCell(Hive,ListData->u.KeyList[i]);

            /* Mark the value data dirty too */
            if (!CmpMarkValueDataDirty(Hive, &ValueData->u.KeyValue))
            {
                /* Failure */
                return FALSE;
            }
        }
    }

    /* If this is an entry hive, we're done */
    if (CellData->u.KeyNode.Flags & KEY_HIVE_ENTRY) return TRUE;

    /* Otherwise mark the index dirty too */
    if (!CmpMarkIndexDirty(Hive, CellData->u.KeyNode.Parent, Cell))
    {
        /* Failure */
        return FALSE;
    }

    /* Finally, mark the parent dirty */
    HvMarkCellDirty(Hive, CellData->u.KeyNode.Parent, FALSE);
    return TRUE;
}

BOOLEAN
NTAPI
CmpFreeKeyBody(IN PHHIVE Hive,
               IN HCELL_INDEX Cell)
{
    PCELL_DATA CellData;

    /* Get the key node */
    CellData = HvGetCell(Hive, Cell);
    if (!CellData) ASSERT(FALSE);

    /* Check if we can delete the child cells */
    if (!(CellData->u.KeyNode.Flags & KEY_HIVE_EXIT))
    {
        /* Check if we have a security cell */
        if (CellData->u.KeyNode.Security != HCELL_NIL)
        {
            /* Free the security cell */
            HvFreeCell(Hive, CellData->u.KeyNode.Security);
        }

        /* Check if we have a class */
        if (CellData->u.KeyNode.ClassLength > 0)
        {
            /* Free it */
            HvFreeCell(Hive, CellData->u.KeyNode.Class);
        }
    }

    /* Release and free the cell */
    HvReleaseCell(Hive, Cell);
    HvFreeCell(Hive, Cell);
    return TRUE;
}

NTSTATUS
NTAPI
CmpFreeKeyByCell(IN PHHIVE Hive,
                 IN HCELL_INDEX Cell,
                 IN BOOLEAN Unlink)
{
    PCELL_DATA CellData, ParentData, ListData;
    ULONG i;
    BOOLEAN Result;

    /* Mark the entire key dirty */
    CmpMarkKeyDirty(Hive, Cell ,TRUE);

    /* Get the target node and release it */
    CellData = HvGetCell(Hive, Cell);
    if (!CellData) ASSERT(FALSE);
    HvReleaseCell(Hive, Cell);

    /* Make sure we don't have subkeys */
    ASSERT((CellData->u.KeyNode.SubKeyCounts[Stable] +
            CellData->u.KeyNode.SubKeyCounts[Volatile]) == 0);

    /* Check if we have to unlink */
    if (Unlink)
    {
        /* Remove the subkey */
        Result = CmpRemoveSubKey(Hive, CellData->u.KeyNode.Parent, Cell);
        if (!Result) return STATUS_INSUFFICIENT_RESOURCES;

        /* Get the parent node and release it */
        ParentData = HvGetCell(Hive, CellData->u.KeyNode.Parent);
        if (!ParentData) ASSERT(FALSE);
        HvReleaseCell(Hive, CellData->u.KeyNode.Parent);

        /* Check if the parent node has no more subkeys */
        if (!(ParentData->u.KeyNode.SubKeyCounts[Stable] +
              ParentData->u.KeyNode.SubKeyCounts[Volatile]))
        {
            /* Then free the cached name/class lengths */
            ParentData->u.KeyNode.MaxNameLen = 0;
            ParentData->u.KeyNode.MaxClassLen = 0;
        }
    }

    /* Check if we have any values */
    if (CellData->u.KeyNode.ValueList.Count > 0)
    {
        /* Get the value list and release it */
        ListData = HvGetCell(Hive, CellData->u.KeyNode.ValueList.List);
        if (!ListData) ASSERT(FALSE);
        HvReleaseCell(Hive, CellData->u.KeyNode.ValueList.List);

        /* Loop every value */
        for (i = 0; i < CellData->u.KeyNode.ValueList.Count; i++)
        {
            /* Free it */
            if (!CmpFreeValue(Hive, ListData->u.KeyList[i])) ASSERT(FALSE);
        }

        /* Free the value list */
        HvFreeCell(Hive, CellData->u.KeyNode.ValueList.List);
    }

    /* Free the key body itself, and then return our status */
    if (!CmpFreeKeyBody(Hive, Cell)) return STATUS_INSUFFICIENT_RESOURCES;
    return STATUS_SUCCESS;
}
