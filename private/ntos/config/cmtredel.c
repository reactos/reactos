/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cmtredel.c

Abstract:

    This file contains code for CmpDeleteTree, and support.

Author:

    Bryan M. Willman (bryanwi) 24-Jan-92

Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpDeleteTree)
#pragma alloc_text(PAGE,CmpFreeKeyByCell)
#pragma alloc_text(PAGE,CmpMarkKeyDirty)
#endif

//
// Routine to actually call to do a tree delete
//

VOID
CmpDeleteTree(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Delete all child subkeys, recursively, of Hive.Cell.  Delete all
    value entries of Hive.Cell.  Do NOT delete Hive.Cell itself.

    NOTE:   If this call fails part way through, it will NOT undo
            any successfully completed deletes.

    NOTE:   This algorithm can deal with a hive of any depth, at the
            cost of some "redundent" scaning and mapping.

Arguments:

    Hive - pointer to hive control structure for hive to delete from

    Cell - index of cell at root of tree to delete

Return Value:

    BOOLEAN - Result code from call, among the following:
        TRUE - it worked
        FALSE - the tree delete was not completed (though more than 0
                keys may have been deleted)

--*/
{
    ULONG  count;
    HCELL_INDEX ptr1;
    HCELL_INDEX ptr2;
    HCELL_INDEX parent;
    PCM_KEY_NODE Node;

    CMLOG(CML_MAJOR, CMS_SAVRES) {
        KdPrint(("CmpDeleteTree:\n"));
        KdPrint(("\tHive=%08lx Cell=%08lx\n",Hive,Cell));
    }

    ptr1 = Cell;

    while(TRUE) {

        Node = (PCM_KEY_NODE)HvGetCell(Hive, ptr1);
        count = Node->SubKeyCounts[Stable] +
                Node->SubKeyCounts[Volatile];
        parent = Node->Parent;

        if (count > 0) {                // ptr1->count > 0?

            //
            // subkeys exist, find and delete them
            //
            ptr2 = CmpFindSubKeyByNumber(Hive, Node, 0);

            Node = (PCM_KEY_NODE)HvGetCell(Hive, ptr2);
            count = Node->SubKeyCounts[Stable] +
                    Node->SubKeyCounts[Volatile];

            if (count > 0) {            // ptr2->count > 0?

                //
                // subkey has subkeys, descend to next level
                //
                ptr1 = ptr2;
                continue;

            } else {

                //
                // have found leaf, delete it
                //
                CmpFreeKeyByCell(Hive, ptr2, TRUE);
                continue;
            }

        } else {

            //
            // no more subkeys at this level, we are now a leaf.
            //
            if (ptr1 != Cell) {

                //
                // we are not at the root cell, so ascend to parent
                //
                ptr1 = parent;          // ptr1 = ptr1->parent
                continue;

            } else {

                //
                // we are at the root cell, we are done
                //
                return;
            }
        } // outer if
    } // while
}


NTSTATUS
CmpFreeKeyByCell(
    PHHIVE Hive,
    HCELL_INDEX Cell,
    BOOLEAN Unlink
    )
/*++

Routine Description:

    Actually free the storage for the specified cell.  We will first
    remove it from its parent's child key list, then free all of its
    values, then the key body itself.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Cell - index for cell to free storage for (the target)

    Unlink - if TRUE, target cell will be removed from parent cell's
             subkeylist, if FALSE, it will NOT be.

Return Value:

    NONE.

--*/
{
    PCELL_DATA  ptarget;
    PCELL_DATA  pparent;
    PCELL_DATA  plist;
    ULONG       i;

    //
    // Mark dirty everything that we might touch
    //
    if (! CmpMarkKeyDirty(Hive, Cell)) {
        return STATUS_NO_LOG_SPACE;
    }

    //
    // Map in the target
    //
    ptarget = HvGetCell(Hive, Cell);
    ASSERT((ptarget->u.KeyNode.SubKeyCounts[Stable] +
            ptarget->u.KeyNode.SubKeyCounts[Volatile]) == 0);


    if (Unlink == TRUE) {
        BOOLEAN Success;

        Success = CmpRemoveSubKey(Hive, ptarget->u.KeyNode.Parent, Cell);
        if (!Success) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        pparent = HvGetCell(Hive, ptarget->u.KeyNode.Parent);
        if ( (pparent->u.KeyNode.SubKeyCounts[Stable] +
              pparent->u.KeyNode.SubKeyCounts[Volatile]) == 0)
        {
            pparent->u.KeyNode.MaxNameLen = 0;
            pparent->u.KeyNode.MaxClassLen = 0;
        }
    }

    //
    // Target is now an unreferenced key, free it's actual storage
    //

    //
    // Free misc stuff
    //
    if (!(ptarget->u.KeyNode.Flags & KEY_HIVE_EXIT) &&
        !(ptarget->u.KeyNode.Flags & KEY_PREDEF_HANDLE) ) {

        //
        // First, free the value entries
        //
        if (ptarget->u.KeyNode.ValueList.Count > 0) {

            // target list
            plist = HvGetCell(Hive, ptarget->u.KeyNode.ValueList.List);

            for (i = 0; i < ptarget->u.KeyNode.ValueList.Count; i++) {
                CmpFreeValue(Hive, plist->u.KeyList[i]);
            }

            HvFreeCell(Hive, ptarget->u.KeyNode.ValueList.List);
        }

        //
        // Free the security descriptor
        //
        CmpFreeSecurityDescriptor(Hive, Cell);
    }

    //
    // Free the key body itself, and Class data.
    //
    CmpFreeKeyBody(Hive, Cell);

    return STATUS_SUCCESS;
}


BOOLEAN
CmpMarkKeyDirty(
    PHHIVE Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Mark all of the cells related to a key being deleted dirty.
    Includes the parent, the parent's child list, the key body,
    class, security, all value entry bodies, and all of their data cells.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Cell - index for cell holding keybody to make dirty

Return Value:

    TRUE - it worked

    FALSE - some error, most likely cannot get log space

--*/
{
    PCELL_DATA  ptarget;
    PCELL_DATA  plist;
    PCELL_DATA  security;
    PCELL_DATA  pvalue;
    ULONG       i;
    ULONG realsize;


    //
    // Map in the target
    //
    ptarget = HvGetCell(Hive, Cell);
    ASSERT(ptarget->u.KeyNode.SubKeyCounts[Stable] == 0);
    ASSERT(ptarget->u.KeyNode.SubKeyCounts[Volatile] == 0);

    if (ptarget->u.KeyNode.Flags & KEY_HIVE_EXIT) {

        //
        // If this is a link node, we are done.  Link nodes never have
        // classes, values, subkeys, or security descriptors.  Since
        // they always reside in the master hive, they're always volatile.
        //
        return(TRUE);
    }

    //
    // mark cell itself
    //
    if (! HvMarkCellDirty(Hive, Cell)) {
        return FALSE;
    }

    //
    // Mark the class
    //
    if (ptarget->u.KeyNode.Class != HCELL_NIL) {
        if (! HvMarkCellDirty(Hive, ptarget->u.KeyNode.Class)) {
            return FALSE;
        }
    }

    //
    // Mark security
    //
    if (ptarget->u.KeyNode.Security != HCELL_NIL) {
        if (! HvMarkCellDirty(Hive, ptarget->u.KeyNode.Security)) {
            return FALSE;
        }

        security = HvGetCell(Hive, ptarget->u.KeyNode.Security);
        if (! (HvMarkCellDirty(Hive, security->u.KeySecurity.Flink) &&
               HvMarkCellDirty(Hive, security->u.KeySecurity.Blink)))
        {
            return FALSE;
        }
    }

    //
    // Mark the value entries and their data
    //
    if ( !(ptarget->u.KeyNode.Flags & KEY_PREDEF_HANDLE) && 
		  (ptarget->u.KeyNode.ValueList.Count > 0) 
	   ) {

        // target list
        if (! HvMarkCellDirty(Hive, ptarget->u.KeyNode.ValueList.List)) {
            return FALSE;
        }
        plist = HvGetCell(Hive, ptarget->u.KeyNode.ValueList.List);

        for (i = 0; i < ptarget->u.KeyNode.ValueList.Count; i++) {
            if (! HvMarkCellDirty(Hive, plist->u.KeyList[i])) {
                return FALSE;
            }

            pvalue = HvGetCell(Hive, plist->u.KeyList[i]);

            if (!CmpIsHKeyValueSmall(realsize, pvalue->u.KeyValue.DataLength)) {
                if (! HvMarkCellDirty(Hive, pvalue->u.KeyValue.Data)) {
                    return(FALSE);
                }
            }
        }
    }

    if (ptarget->u.KeyNode.Flags & KEY_HIVE_ENTRY) {

        //
        // if this is an entry node, we are done.  our parent will
        // be in the master hive (and thus volatile)
        //
        return TRUE;
    }

    //
    // Mark the parent's Subkey list
    //
    if (! CmpMarkIndexDirty(Hive, ptarget->u.KeyNode.Parent, Cell)) {
        return FALSE;
    }

    //
    // Mark the parent
    //
    if (! HvMarkCellDirty(Hive, ptarget->u.KeyNode.Parent)) {
        return FALSE;
    }


    return TRUE;
}
