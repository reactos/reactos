/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            lib/cmlib/cmkeydel.c
 * PURPOSE:         Configuration Manager Library - Key Body Deletion
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "cmlib.h"
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
CmpMarkKeyDirty(IN PHHIVE Hive,
                IN HCELL_INDEX Cell,
                IN BOOLEAN CheckNoSubkeys)
{
    PCM_KEY_NODE CellData;
    PCM_KEY_SECURITY SecurityData;
    PCELL_DATA ListData, ValueData;
    ULONG i;

    /* Get the cell data for our target */
    CellData = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if (!CellData) return FALSE;

    /* Check if sanity checks requested */
    if (CheckNoSubkeys)
    {
        /* Do them */
        ASSERT(CellData->SubKeyCounts[Stable] == 0);
        ASSERT(CellData->SubKeyCounts[Volatile] == 0);
    }

    /* If this is an exit node, there's nothing to do */
    if (CellData->Flags & KEY_HIVE_EXIT)
    {
        /* Release the cell and get out */
        HvReleaseCell(Hive, Cell);
        return TRUE;
    }

    /* Otherwise, mark it dirty and release it */
    HvMarkCellDirty(Hive, Cell, FALSE);
    HvReleaseCell(Hive, Cell);

    /* Check if we have a class */
    if (CellData->Class != HCELL_NIL)
    {
        /* Mark it dirty */
        HvMarkCellDirty(Hive, CellData->Class, FALSE);
    }

    /* Check if we have security */
    if (CellData->Security != HCELL_NIL)
    {
        /* Mark it dirty */
        HvMarkCellDirty(Hive, CellData->Security, FALSE);

        /* Get the security data and release it */
        SecurityData = (PCM_KEY_SECURITY)HvGetCell(Hive, CellData->Security);
        ASSERT(SecurityData);
        HvReleaseCell(Hive, CellData->Security);

        /* Mark the security links dirty too */
        HvMarkCellDirty(Hive, SecurityData->Flink, FALSE);
        HvMarkCellDirty(Hive, SecurityData->Blink, FALSE);
    }

    // TODO: Handle predefined keys (Flags: KEY_PREDEF_HANDLE)
    /* Check if we have any values */
    if (CellData->ValueList.Count > 0)
    {
        /* Dirty the value list */
        HvMarkCellDirty(Hive, CellData->ValueList.List, FALSE);

        /* Get the list data itself, and release it */
        ListData = HvGetCell(Hive, CellData->ValueList.List);
        ASSERT(ListData);
        HvReleaseCell(Hive, CellData->ValueList.List);

        /* Loop all values */
        for (i = 0; i < CellData->ValueList.Count; i++)
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

    /* If this is an entry node, we're done */
    if (CellData->Flags & KEY_HIVE_ENTRY) return TRUE;

    /* Otherwise mark the index dirty too */
    if (!CmpMarkIndexDirty(Hive, CellData->Parent, Cell))
    {
        /* Failure */
        return FALSE;
    }

    /* Finally, mark the parent dirty */
    HvMarkCellDirty(Hive, CellData->Parent, FALSE);
    return TRUE;
}

BOOLEAN
NTAPI
CmpFreeKeyBody(IN PHHIVE Hive,
               IN HCELL_INDEX Cell)
{
    PCM_KEY_NODE CellData;

    /* Get the key node */
    CellData = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    ASSERT(CellData);

    /* Check if we can delete the child cells */
    if (!(CellData->Flags & KEY_HIVE_EXIT))
    {
        /* Check if we have a security cell */
        if (CellData->Security != HCELL_NIL)
        {
            /* Free the security cell */
            HvFreeCell(Hive, CellData->Security);
        }

        /* Check if we have a class */
        if (CellData->ClassLength > 0)
        {
            /* Free it */
            HvFreeCell(Hive, CellData->Class);
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
    PCM_KEY_NODE CellData, ParentData;
    PCELL_DATA ListData;
    ULONG i;
    BOOLEAN Result;

    /* Mark the entire key dirty */
    CmpMarkKeyDirty(Hive, Cell, TRUE);

    /* Get the target node and release it */
    CellData = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    ASSERT(CellData);
    HvReleaseCell(Hive, Cell);

    /* Make sure we don't have subkeys */
    ASSERT(CellData->SubKeyCounts[Stable] + CellData->SubKeyCounts[Volatile] == 0);

    /* Check if we have to unlink */
    if (Unlink)
    {
        /* Remove the subkey */
        Result = CmpRemoveSubKey(Hive, CellData->Parent, Cell);
        if (!Result) return STATUS_INSUFFICIENT_RESOURCES;

        /* Get the parent node and release it */
        ParentData = (PCM_KEY_NODE)HvGetCell(Hive, CellData->Parent);
        ASSERT(ParentData);
        HvReleaseCell(Hive, CellData->Parent);

        /* Check if the parent node has no more subkeys */
        if (ParentData->SubKeyCounts[Stable] + ParentData->SubKeyCounts[Volatile] == 0)
        {
            /* Then free the cached name/class lengths */
            ParentData->MaxNameLen  = 0;
            ParentData->MaxClassLen = 0;
        }
    }

    // TODO: Handle predefined keys (Flags: KEY_PREDEF_HANDLE)
    /* If this is an exit node, we don't have values */
    if (!(CellData->Flags & KEY_HIVE_EXIT))
    {
        /* Check if we have any values */
        if (CellData->ValueList.Count > 0)
        {
            /* Get the value list and release it */
            ListData = HvGetCell(Hive, CellData->ValueList.List);
            ASSERT(ListData);
            HvReleaseCell(Hive, CellData->ValueList.List);

            /* Loop every value */
            for (i = 0; i < CellData->ValueList.Count; i++)
            {
                /* Free it */
                Result = CmpFreeValue(Hive, ListData->u.KeyList[i]);
                ASSERT(Result);
            }

            /* Free the value list */
            HvFreeCell(Hive, CellData->ValueList.List);
        }

        /* Free the key security descriptor */
        CmpFreeSecurityDescriptor(Hive, Cell);
    }

    /* Free the key body itself, and then return our status */
    if (!CmpFreeKeyBody(Hive, Cell)) return STATUS_INSUFFICIENT_RESOURCES;
    return STATUS_SUCCESS;
}
