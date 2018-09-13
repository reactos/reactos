/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmindex.c

Abstract:

    This module contains cm routines that understand the structure
    of child subkey indicies.

Author:

    Bryan M. Willman (bryanwi) 21-Apr-92

Revision History:

--*/

/*

The Structure:

    Use a 1 or 2 level tree.  Leaf nodes are arrays of pointers to
    cells, sorted.  Binary search to find cell of interest.  Directory
    node (can be only one) is an array of pointers to leaf blocks.
    Do compare on last entry of each leaf block.

    One Level:

        Key--->+----+
               |    |
               |  x----------><key whose name is "apple", string in key>
               |    |
               +----+
               |    |
               |  x----------><as above, but key named "banana">
               |    |
               +----+
               |    |
               |    |
               |    |
               +----+
               |    |
               |    |
               |    |
               +----+
               |    |
               |  x----------><as above, but key named "zumwat">
               |    |
               +----+


    Two Level:

        Key--->+----+
               |    |    +-----+
               |  x----->|     |
               |    |    |  x----------------->"aaa"
               +----+    |     |
               |    |    +-----+
               |    |    |     |
               |    |    |     |
               +----+    |     |
               |    |    +-----+
               |    |    |     |
               |    |    |  x----------------->"abc"
               +----+    |     |
               |    |    +-----+
               |    |
               |    |
               +----+
               |    |    +-----+
               |  x----->|     |
               |    |    |  x----------------->"w"
               +----+    |     |
                         +-----+
                         |     |
                         |     |
                         |     |
                         +-----+
                         |     |
                         |  x----------------->"z"
                         |     |
                         +-----+


    Never more than two levels.

    Each block must fix in on HBLOCK_SIZE Cell.  Allows about 1000
    entries.  Max of 1 million total, best case.  Worst case something
    like 1/4 of that.

*/

#include    "cmp.h"

ULONG
CmpFindSubKeyInRoot(
    PHHIVE          Hive,
    PCM_KEY_INDEX   Index,
    PUNICODE_STRING SearchName,
    PHCELL_INDEX    Child
    );

ULONG
CmpFindSubKeyInLeaf(
    PHHIVE          Hive,
    PCM_KEY_INDEX   Index,
    ULONG           IndexHint,
    PUNICODE_STRING SearchName,
    PHCELL_INDEX    Child
    );

LONG
CmpCompareInIndex(
    PHHIVE          Hive,
    PUNICODE_STRING SearchName,
    ULONG           Count,
    PCM_KEY_INDEX   Index,
    PHCELL_INDEX    Child
    );

LONG
CmpDoCompareKeyName(
    PHHIVE          Hive,
    PUNICODE_STRING SearchName,
    HCELL_INDEX     Cell
    );

HCELL_INDEX
CmpDoFindSubKeyByNumber(
    PHHIVE          Hive,
    PCM_KEY_INDEX   Index,
    ULONG           Number
    );

HCELL_INDEX
CmpAddToLeaf(
    PHHIVE          Hive,
    HCELL_INDEX     LeafCell,
    HCELL_INDEX     NewKey,
    PUNICODE_STRING NewName
    );

HCELL_INDEX
CmpSelectLeaf(
    PHHIVE          Hive,
    PCM_KEY_NODE    ParentKey,
    PUNICODE_STRING NewName,
    HSTORAGE_TYPE   Type,
    PHCELL_INDEX    *RootPointer
    );

HCELL_INDEX
CmpSplitLeaf(
    PHHIVE          Hive,
    HCELL_INDEX     RootCell,
    ULONG           RootSelect,
    HSTORAGE_TYPE   Type
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpFindSubKeyByName)
#pragma alloc_text(PAGE,CmpFindSubKeyInRoot)
#pragma alloc_text(PAGE,CmpFindSubKeyInLeaf)
#pragma alloc_text(PAGE,CmpDoCompareKeyName)
#pragma alloc_text(PAGE,CmpCompareInIndex)
#pragma alloc_text(PAGE,CmpFindSubKeyByNumber)
#pragma alloc_text(PAGE,CmpDoFindSubKeyByNumber)
#pragma alloc_text(PAGE,CmpAddSubKey)
#pragma alloc_text(PAGE,CmpAddToLeaf)
#pragma alloc_text(PAGE,CmpSelectLeaf)
#pragma alloc_text(PAGE,CmpSplitLeaf)
#pragma alloc_text(PAGE,CmpMarkIndexDirty)
#pragma alloc_text(PAGE,CmpRemoveSubKey)
#endif

ULONG CmpHintHits=0;
ULONG CmpHintMisses=0;


HCELL_INDEX
CmpFindSubKeyByName(
    PHHIVE          Hive,
    PCM_KEY_NODE    Parent,
    PUNICODE_STRING SearchName
    )
/*++

Routine Description:

    Find the child cell (either subkey or value) specified by name.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Parent - cell of key body which is parent of child of interest

    SearchName - name of child of interest

Return Value:

    Cell of matching child key, or HCELL_NIL if none.

--*/
{
    PCM_KEY_INDEX   IndexRoot;
    HCELL_INDEX     Child;
    ULONG           i;
    ULONG           FoundIndex;

    CMLOG(CML_MAJOR, CMS_INDEX) {
        KdPrint(("CmpFindSubKeyByName:\n\t"));
        KdPrint(("Hive=%08lx Parent=%08lx SearchName=%08lx\n", Hive, Parent, SearchName));
    }

    //
    // Try first the Stable, then the Volatile store.  Assumes that
    // all Volatile refs in Stable space are zeroed out at boot.
    //
    for (i = 0; i < Hive->StorageTypeCount; i++) {
        if (Parent->SubKeyCounts[i] != 0) {
            ASSERT(HvIsCellAllocated(Hive, Parent->SubKeyLists[i]));
            IndexRoot = (PCM_KEY_INDEX)HvGetCell(Hive, Parent->SubKeyLists[i]);

            if (IndexRoot->Signature == CM_KEY_INDEX_ROOT) {
                CmpFindSubKeyInRoot(Hive, IndexRoot, SearchName, &Child);
                if (Child == HCELL_NIL) {
                    continue;
                }
                IndexRoot = (PCM_KEY_INDEX)HvGetCell(Hive, Child);
            }
            ASSERT((IndexRoot->Signature == CM_KEY_INDEX_LEAF) ||
                   (IndexRoot->Signature == CM_KEY_FAST_LEAF));

            FoundIndex = CmpFindSubKeyInLeaf(Hive,
                                             IndexRoot,
                                             Parent->WorkVar,
                                             SearchName,
                                             &Child);
            if (Child != HCELL_NIL) {
                //
                // WorkVar is used as a hint for the last successful lookup
                // to improve our locality when similar keys are opened
                // repeatedly.
                //
                if (FoundIndex == Parent->WorkVar) {
                    CmpHintHits++;
                } else {
                    CmpHintMisses++;
                }
                Parent->WorkVar = FoundIndex;
                return Child;
            }
        }
    }
    return HCELL_NIL;
}


ULONG
CmpFindSubKeyInRoot(
    PHHIVE          Hive,
    PCM_KEY_INDEX   Index,
    PUNICODE_STRING SearchName,
    PHCELL_INDEX    Child
    )
/*++

Routine Description:

    Find the leaf index that would contain a key, if there is one.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Index - pointer to root index block

    SearchName - pointer to name of key of interest

    Child - pointer to variable to receive hcell_index of found leaf index
            block, HCELL_NIL if none.  Non nil does not necessarily mean
            the key is present, call FindSubKeyInLeaf to decide that.

Return Value:

    Index in List of last Leaf Cell entry examined.  If Child != HCELL_NIL,
    Index is entry that matched, else, index is for last entry we looked
    at.  (Target Leaf will be this value plus or minus 1)

--*/
{
    ULONG           High;
    ULONG           Low;
    ULONG           CanCount;
    HCELL_INDEX     LeafCell;
    PCM_KEY_INDEX   Leaf;
    LONG            Result;

    CMLOG(CML_MAJOR, CMS_INDEX) {
        KdPrint(("CmpFindSubKeyInRoot:\n\t"));
        KdPrint(("Hive=%08lx Index=%08lx SearchName=%08lx\n",Hive,Index,SearchName));
    }


    ASSERT(Index->Count != 0);
    ASSERT(Index->Signature == CM_KEY_INDEX_ROOT);

    High = Index->Count - 1;
    Low = 0;

    while (TRUE) {

        //
        // Compute where to look next, get correct pointer, do compare
        //
        CanCount = ((High-Low)/2)+Low;
        LeafCell = Index->List[CanCount];
        Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

        ASSERT((Leaf->Signature == CM_KEY_INDEX_LEAF) ||
               (Leaf->Signature == CM_KEY_FAST_LEAF));
        ASSERT(Leaf->Count != 0);

        Result = CmpCompareInIndex(Hive,
                                   SearchName,
                                   Leaf->Count-1,
                                   Leaf,
                                   Child);

        if (Result == 0) {

            //
            // SearchName == KeyName of last key in leaf, so
            //  this is our leaf
            //
            *Child = LeafCell;
            return CanCount;
        }

        if (Result < 0) {

            //
            // SearchName < KeyName, so this may still be our leaf
            //
            Result = CmpCompareInIndex(Hive,
                                       SearchName,
                                       0,
                                       Leaf,
                                       Child);

            if (Result >= 0) {

                //
                // we know from above that SearchName is less than
                // last key in leaf.
                // since it is also >= first key in leaf, it must
                // reside in leaf somewhere, and we are done
                //
                *Child = LeafCell;
                return CanCount;
            }

            High = CanCount;

        } else {

            //
            // SearchName > KeyName
            //
            Low = CanCount;
        }

        if ((High - Low) <= 1) {
            break;
        }
    }

    //
    // If we get here, High - Low = 1 or High == Low
    //
    ASSERT((High - Low == 1) || (High == Low));
    LeafCell = Index->List[Low];
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    Result = CmpCompareInIndex(Hive,
                               SearchName,
                               Leaf->Count-1,
                               Leaf,
                               Child);

    if (Result == 0) {

        //
        // found it
        //
        *Child = LeafCell;
        return Low;
    }

    if (Result < 0) {

        //
        // SearchName < KeyName, so this may still be our leaf
        //
        Result = CmpCompareInIndex(Hive,
                                   SearchName,
                                   0,
                                   Leaf,
                                   Child);

        if (Result >= 0) {

            //
            // we know from above that SearchName is less than
            // last key in leaf.
            // since it is also >= first key in leaf, it must
            // reside in leaf somewhere, and we are done
            //
            *Child = LeafCell;
            return Low;
        }

        //
        // does not exist, but belongs in Low or Leaf below low
        //
        *Child = HCELL_NIL;
        return Low;
    }

    //
    // see if High matches
    //
    LeafCell = Index->List[High];
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    Result = CmpCompareInIndex(Hive,
                               SearchName,
                               Leaf->Count - 1,
                               Leaf,
                               Child);
    if (Result == 0) {

        //
        // found it
        //
        *Child = LeafCell;
        return High;

    } else if (Result < 0) {

        //
        // Clearly greater than low, or we wouldn't be here.
        // So regardless of whether it's below the start
        // of this leaf, it would be in this leaf if it were
        // where, so report this leaf.
        //
        *Child = LeafCell;
        return High;

    }

    //
    // Off the high end
    //
    *Child = HCELL_NIL;
    return High;
}


ULONG
CmpFindSubKeyInLeaf(
    PHHIVE          Hive,
    PCM_KEY_INDEX   Index,
    ULONG           HintIndex,
    PUNICODE_STRING SearchName,
    PHCELL_INDEX    Child
    )
/*++

Routine Description:

    Find a named key in a leaf index, if it exists. The supplied index
    may be either a fast index or a slow one.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Index - pointer to leaf block

    HintIndex - Supplies hint for first key to check.

    SearchName - pointer to name of key of interest

    Child - pointer to variable to receive hcell_index of found key
            HCELL_NIL if none found

Return Value:

    Index in List of last cell.  If Child != HCELL_NIL, is offset in
    list at which Child was found.  Else, is offset of last place
    we looked.

--*/
{
    ULONG       High;
    ULONG       Low;
    ULONG       CanCount;
    LONG        Result;

    CMLOG(CML_MAJOR, CMS_INDEX) {
        KdPrint(("CmpFindSubKeyInLeaf:\n\t"));
        KdPrint(("Hive=%08lx Index=%08lx SearchName=%08lx\n",Hive,Index,SearchName));
    }

    ASSERT((Index->Signature == CM_KEY_INDEX_LEAF) ||
           (Index->Signature == CM_KEY_FAST_LEAF));

    High = Index->Count - 1;
    Low = 0;
    if (HintIndex < High) {
        CanCount = HintIndex;
    } else {
        CanCount = High/2;
    }

    if (Index->Count == 0) {
        *Child = HCELL_NIL;
        return 0;
    }

    while (TRUE) {

        //
        // Compute where to look next, get correct pointer, do compare
        //
        Result = CmpCompareInIndex(Hive,
                                   SearchName,
                                   CanCount,
                                   Index,
                                   Child);

        if (Result == 0) {

            //
            // SearchName == KeyName
            //
            return CanCount;
        }

        if (Result < 0) {

            //
            // SearchName < KeyName
            //
            High = CanCount;

        } else {

            //
            // SearchName > KeyName
            //
            Low = CanCount;
        }

        if ((High - Low) <= 1) {
            break;
        }
        CanCount = ((High-Low)/2)+Low;
    }

    //
    // If we get here, High - Low = 1 or High == Low
    // Simply look first at Low, then at High
    //
    Result = CmpCompareInIndex(Hive,
                               SearchName,
                               Low,
                               Index,
                               Child);
    if (Result == 0) {

        //
        // found it
        //
        return Low;
    }

    if (Result < 0) {

        //
        // does not exist, under
        //
        return Low;
    }

    //
    // see if High matches, we will return High as the
    // closest key regardless.
    //
    Result = CmpCompareInIndex(Hive,
                               SearchName,
                               High,
                               Index,
                               Child);
    return High;
}


LONG
CmpCompareInIndex(
    PHHIVE          Hive,
    PUNICODE_STRING SearchName,
    ULONG           Count,
    PCM_KEY_INDEX   Index,
    PHCELL_INDEX    Child
    )
/*++

Routine Description:

    Do a compare of a name in an index. This routine handles both
    fast leafs and slow ones.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    SearchName - pointer to name of key we are searching for

    Count - supplies index that we are searching at.

    Index - Supplies pointer to either a CM_KEY_INDEX or
            a CM_KEY_FAST_INDEX. This routine will determine which
            type of index it is passed.

    Child - pointer to variable to receive hcell_index of found key
            HCELL_NIL if result != 0

Return Value:

    0 = SearchName == KeyName (of Cell)

    < 0 = SearchName < KeyName

    > 0 = SearchName > KeyName

--*/
{
    PCM_KEY_FAST_INDEX FastIndex;
    LONG Result;
    ULONG i;
    WCHAR c1;
    WCHAR c2;
    ULONG HintLength;
    ULONG ValidChars;
    ULONG NameLength;
    PCM_INDEX Hint;

    *Child = HCELL_NIL;
    if (Index->Signature == CM_KEY_FAST_LEAF) {
        FastIndex = (PCM_KEY_FAST_INDEX)Index;
        Hint = &FastIndex->List[Count];

        //
        // Compute the number of valid characters in the hint to compare.
        //
        HintLength = 4;
        for (i=0;i<4;i++) {
            if (Hint->NameHint[i] == 0) {
                HintLength = i;
                break;
            }
        }
        NameLength = SearchName->Length / sizeof(WCHAR);
        if (NameLength < HintLength) {
            ValidChars = NameLength;
        } else {
            ValidChars = HintLength;
        }
        for (i=0; i<ValidChars; i++) {
            c1 = SearchName->Buffer[i];
            c2 = FastIndex->List[Count].NameHint[i];
            Result = (LONG)RtlUpcaseUnicodeChar(c1) -
                     (LONG)RtlUpcaseUnicodeChar(c2);
            if (Result != 0) {

                //
                // We have found a mismatched character in the hint,
                // we can now tell which direction to go.
                //
                return(Result);
            }
        }

        //
        // We have compared all the available characters without a
        // discrepancy. Go ahead and do the actual comparison now.
        //
        Result = CmpDoCompareKeyName(Hive,SearchName,FastIndex->List[Count].Cell);
        if (Result == 0) {
            *Child = Hint->Cell;
        }
    } else {
        //
        // This is just a normal old slow index.
        //
        Result = CmpDoCompareKeyName(Hive,SearchName,Index->List[Count]);
        if (Result == 0) {
            *Child = Index->List[Count];
        }
    }
    return(Result);
}


LONG
CmpDoCompareKeyName(
    PHHIVE          Hive,
    PUNICODE_STRING SearchName,
    HCELL_INDEX     Cell
    )
/*++

Routine Description:

    Do a compare of a name with a key.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    SearchName - pointer to name of key we are searching for

    Cell - cell of key we are to compare with

Return Value:

    0 = SearchName == KeyName (of Cell)

    < 0 = SearchName < KeyName

    > 0 = SearchName > KeyName

--*/
{
    PCM_KEY_NODE    Pcan;
    UNICODE_STRING  KeyName;

    Pcan = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if (Pcan->Flags & KEY_COMP_NAME) {
        return(CmpCompareCompressedName(SearchName,
                                        Pcan->Name,
                                        Pcan->NameLength));
    } else {
        KeyName.Buffer = &(Pcan->Name[0]);
        KeyName.Length = Pcan->NameLength;
        KeyName.MaximumLength = KeyName.Length;
        return (RtlCompareUnicodeString(SearchName,
                                        &KeyName,
                                        TRUE));
    }
}


HCELL_INDEX
CmpFindSubKeyByNumber(
    PHHIVE          Hive,
    PCM_KEY_NODE    Node,
    ULONG           Number
    )
/*++

Routine Description:

    Find the Number'th entry in the index, starting from 0.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Node - pointer to key body which is parent of child of interest

    Number - ordinal of child key to return

Return Value:

    Cell of matching child key, or HCELL_NIL if none.

--*/
{
    PCM_KEY_INDEX   Index;

    CMLOG(CML_MAJOR, CMS_INDEX) {
        KdPrint(("CmpFindSubKeyByNumber:\n\t"));
        KdPrint(("Hive=%08lx Node=%08lx Number=%08lx\n",Hive,Node,Number));
    }

    if (Number < Node->SubKeyCounts[Stable]) {

        //
        // It's in the stable set
        //
        Index = (PCM_KEY_INDEX)HvGetCell(Hive, Node->SubKeyLists[Stable]);
        return (CmpDoFindSubKeyByNumber(Hive, Index, Number));

    } else if (Hive->StorageTypeCount > Volatile) {

        //
        // It's in the volatile set
        //
        Number = Number - Node->SubKeyCounts[Stable];
        if (Number < Node->SubKeyCounts[Volatile]) {

            Index = (PCM_KEY_INDEX)HvGetCell(Hive, Node->SubKeyLists[Volatile]);
            return (CmpDoFindSubKeyByNumber(Hive, Index, Number));

        }
    }

    //
    // It's nowhere
    //
    return HCELL_NIL;
}


HCELL_INDEX
CmpDoFindSubKeyByNumber(
    PHHIVE          Hive,
    PCM_KEY_INDEX   Index,
    ULONG           Number
    )
/*++

Routine Description:

    Helper for CmpFindSubKeyByNumber,
    Find the Number'th entry in the index, starting from 0.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Index - root or leaf of the index

    Number - ordinal of child key to return

Return Value:

    Cell of requested entry.

--*/
{
    ULONG           i;
    HCELL_INDEX     LeafCell;
    PCM_KEY_INDEX   Leaf;
    PCM_KEY_FAST_INDEX FastIndex;

    if (Index->Signature == CM_KEY_INDEX_ROOT) {
        //
        // step through root, till we find the right leaf
        //
        for (i = 0; i < Index->Count; i++) {
            LeafCell = Index->List[i];
            Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
            if (Number < Leaf->Count) {
                if (Leaf->Signature == CM_KEY_FAST_LEAF) {
                    FastIndex = (PCM_KEY_FAST_INDEX)Leaf;
                    return (FastIndex->List[Number].Cell);
                } else {
                    return (Leaf->List[Number]);
                }
            } else {
                Number = Number - Leaf->Count;
            }
        }
        ASSERT(FALSE);
    }
    ASSERT(Number < Index->Count);
    if (Index->Signature == CM_KEY_FAST_LEAF) {
        FastIndex = (PCM_KEY_FAST_INDEX)Index;
        return(FastIndex->List[Number].Cell);
    } else {
        return (Index->List[Number]);
    }
}


BOOLEAN
CmpAddSubKey(
    PHHIVE          Hive,
    HCELL_INDEX     Parent,
    HCELL_INDEX     Child
    )
/*++

Routine Description:

    Add a new child subkey to the subkey index for a cell.  The
    child MUST NOT already be present (bugcheck if so.)

    NOTE:   We expect Parent to already be marked dirty.
            We will mark stuff in Index dirty

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Parent - cell of key that will be parent of new key

    Child - new key to put in Paren't sub key list

Return Value:

    TRUE - it worked

    FALSE - resource problem

--*/
{
    PCM_KEY_NODE    pcell;
    HCELL_INDEX     WorkCell;
    PCM_KEY_INDEX   Index;
    PCM_KEY_FAST_INDEX FastIndex;
    UNICODE_STRING  NewName;
    HCELL_INDEX     LeafCell;
    PHCELL_INDEX    RootPointer = NULL;
    ULONG           cleanup = 0;
    ULONG           Type;
    BOOLEAN         IsCompressed;
    ULONG           i;

    CMLOG(CML_MAJOR, CMS_INDEX) {
        KdPrint(("CmpAddSubKey:\n\t"));
        KdPrint(("Hive=%08lx Parent=%08lx Child=%08lx\n",Hive,Parent,Child));
    }

    //
    // build a name string
    //
    pcell = (PCM_KEY_NODE)HvGetCell(Hive, Child);
    if (pcell->Flags & KEY_COMP_NAME) {
        IsCompressed = TRUE;
        NewName.Length = CmpCompressedNameSize(pcell->Name, pcell->NameLength);
        NewName.MaximumLength = NewName.Length;
        NewName.Buffer = (Hive->Allocate)(NewName.Length, FALSE);
        if (NewName.Buffer==NULL) {
            return(FALSE);
        }
        CmpCopyCompressedName(NewName.Buffer,
                              NewName.MaximumLength,
                              pcell->Name,
                              pcell->NameLength);
    } else {
        IsCompressed = FALSE;
        NewName.Length = pcell->NameLength;
        NewName.MaximumLength = pcell->NameLength;
        NewName.Buffer = &(pcell->Name[0]);
    }

    pcell = (PCM_KEY_NODE)HvGetCell(Hive, Parent);

    Type = HvGetCellType(Child);

    if (pcell->SubKeyCounts[Type] == 0) {

        ULONG Signature;

        //
        // we must allocate a leaf
        //
        WorkCell = HvAllocateCell(Hive, sizeof(CM_KEY_FAST_INDEX), Type);
        if (WorkCell == HCELL_NIL) {
            goto ErrorExit;
        }
        Index = (PCM_KEY_INDEX)HvGetCell(Hive, WorkCell);
        Index->Signature = UseFastIndex(Hive) ? CM_KEY_FAST_LEAF : CM_KEY_INDEX_LEAF;
        Index->Count = 0;
        pcell->SubKeyLists[Type] = WorkCell;
        cleanup = 1;
    } else {

        Index = (PCM_KEY_INDEX)HvGetCell(Hive, pcell->SubKeyLists[Type]);
        if ((Index->Signature == CM_KEY_FAST_LEAF) &&
            (Index->Count >= (CM_MAX_FAST_INDEX))) {

            //
            // We must change fast index to a slow index to accomodate
            // growth.
            //

            FastIndex = (PCM_KEY_FAST_INDEX)Index;
            for (i=0; i<Index->Count; i++) {
                Index->List[i] = FastIndex->List[i].Cell;
            }
            Index->Signature = CM_KEY_INDEX_LEAF;

        } else if ((Index->Signature == CM_KEY_INDEX_LEAF) &&
                   (Index->Count >= (CM_MAX_INDEX - 1) )) {
            //
            // We must change flat entry to a root/leaf tree
            //
            WorkCell = HvAllocateCell(
                         Hive,
                         sizeof(CM_KEY_INDEX) + sizeof(HCELL_INDEX), // allow for 2
                         Type
                         );
            if (WorkCell == HCELL_NIL) {
                goto ErrorExit;
            }

            Index = (PCM_KEY_INDEX)HvGetCell(Hive, WorkCell);
            Index->Signature = CM_KEY_INDEX_ROOT;
            Index->Count = 1;
            Index->List[0] = pcell->SubKeyLists[Type];
            pcell->SubKeyLists[Type] = WorkCell;
            cleanup = 2;

        }
    }
    LeafCell = pcell->SubKeyLists[Type];

    //
    // LeafCell is target for add, or perhaps root
    // Index is pointer to fast leaf, slow Leaf or Root, whichever applies
    //
    if (Index->Signature == CM_KEY_INDEX_ROOT) {
        LeafCell = CmpSelectLeaf(Hive, pcell, &NewName, Type, &RootPointer);
        if (LeafCell == HCELL_NIL) {
            goto ErrorExit;
        }
    }

    //
    // Add new cell to Leaf, update pointers
    //
    LeafCell = CmpAddToLeaf(Hive, LeafCell, Child, &NewName);

    if (LeafCell == HCELL_NIL) {
        goto ErrorExit;
    }

    pcell->SubKeyCounts[Type] += 1;

    if (RootPointer != NULL) {
        *RootPointer = LeafCell;
    } else {
        pcell->SubKeyLists[Type] = LeafCell;
    }

    if (IsCompressed) {
        (Hive->Free)(NewName.Buffer, NewName.Length);
    }

    return TRUE;



ErrorExit:
    if (IsCompressed) {
        (Hive->Free)(NewName.Buffer, NewName.Length);
    }

    switch (cleanup) {
    case 1:
        HvFreeCell(Hive, pcell->SubKeyLists[Type]);
        pcell->SubKeyLists[Type] = HCELL_NIL;
        break;

    case 2:
        Index = (PCM_KEY_INDEX)HvGetCell(Hive, pcell->SubKeyLists[Type]);
        pcell->SubKeyLists[Type] = Index->List[0];
        HvFreeCell(Hive, pcell->SubKeyLists[Type]);
        break;
    }

    return  FALSE;
}


HCELL_INDEX
CmpAddToLeaf(
    PHHIVE          Hive,
    HCELL_INDEX     LeafCell,
    HCELL_INDEX     NewKey,
    PUNICODE_STRING NewName
    )
/*++

Routine Description:

    Insert a new subkey into a Leaf index. Supports both fast and slow
    leaf indexes and will determine which sort of index the given leaf is.

    NOTE:   We expect Root to already be marked dirty by caller if non NULL.
            We expect Leaf to always be marked dirty by caller.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    LeafCell - cell of index leaf node we are to add entry too

    NewKey - cell of KEY_NODE we are to add

    NewName - pointer to unicode string with name to we are to add

Return Value:

    HCELL_NIL - some resource problem

    Else - cell of Leaf index when are done, caller is expected to
            set this into Root index or Key body.

--*/
{
    PCM_KEY_INDEX   Leaf;
    PCM_KEY_FAST_INDEX FastLeaf;
    ULONG           Size;
    ULONG           OldSize;
    ULONG           freecount;
    HCELL_INDEX     NewCell;
    HCELL_INDEX     Child;
    ULONG           Select;
    LONG            Result;
    ULONG           EntrySize;
    ULONG           i;

    CMLOG(CML_MAJOR, CMS_INDEX) {
        KdPrint(("CmpAddToLeaf:\n\t"));
        KdPrint(("Hive=%08lx LeafCell=%08lx NewKey=%08lx\n",Hive,LeafCell,NewKey));
    }

    if (!HvMarkCellDirty(Hive, LeafCell)) {
        return HCELL_NIL;
    }

    //
    // compute number free slots left in the leaf
    //
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    if (Leaf->Signature == CM_KEY_INDEX_LEAF) {
        FastLeaf = NULL;
        EntrySize = sizeof(HCELL_INDEX);
    } else {
        ASSERT(Leaf->Signature == CM_KEY_FAST_LEAF);
        FastLeaf = (PCM_KEY_FAST_INDEX)Leaf;
        EntrySize = sizeof(CM_INDEX);
    }
    OldSize = HvGetCellSize(Hive, Leaf);
    Size = OldSize - ((EntrySize * Leaf->Count) +
              FIELD_OFFSET(CM_KEY_INDEX, List));
    freecount = Size / EntrySize;

    //
    // grow the leaf if it isn't big enough
    //
    NewCell = LeafCell;
    if (freecount < 1) {
        Size = OldSize + OldSize / 2;
        if (Size < (OldSize + EntrySize)) {
            Size = OldSize + EntrySize;
        }
        NewCell = HvReallocateCell(Hive, LeafCell, Size);
        if (NewCell == HCELL_NIL) {
            return HCELL_NIL;
        }
        Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, NewCell);
        if (FastLeaf != NULL) {
            FastLeaf = (PCM_KEY_FAST_INDEX)Leaf;
        }
    }

    //
    // Find where to put the new entry
    //
    Select = CmpFindSubKeyInLeaf(Hive, Leaf, (ULONG)-1, NewName, &Child);
    ASSERT(Child == HCELL_NIL);

    //
    // Select is the index in List of the entry nearest where the
    // new entry should go.
    // Decide wether the new entry goes before or after Offset entry,
    // and then ripple copy and set.
    // If Select == Count, then the leaf is empty, so simply set our entry
    //
    if (Select != Leaf->Count) {

        Result = CmpCompareInIndex(Hive,
                                   NewName,
                                   Select,
                                   Leaf,
                                   &Child);
        ASSERT(Result != 0);

        //
        // Result < 0 - NewName/NewKey less than selected key, insert before
        //        > 0 - NewName/NewKey greater than selected key, insert after
        //
        if (Result > 0) {
            Select++;
        }

        if (Select != Leaf->Count) {

            //
            // ripple copy to make space and insert
            //

            if (FastLeaf != NULL) {
                RtlMoveMemory((PVOID)&(FastLeaf->List[Select+1]),
                              (PVOID)&(FastLeaf->List[Select]),
                              sizeof(CM_INDEX)*(FastLeaf->Count - Select));
            } else {
                RtlMoveMemory((PVOID)&(Leaf->List[Select+1]),
                              (PVOID)&(Leaf->List[Select]),
                              sizeof(HCELL_INDEX)*(Leaf->Count - Select));
            }
        }
    }
    if (FastLeaf != NULL) {
        FastLeaf->List[Select].Cell = NewKey;
        FastLeaf->List[Select].NameHint[0] = 0;
        FastLeaf->List[Select].NameHint[1] = 0;
        FastLeaf->List[Select].NameHint[2] = 0;
        FastLeaf->List[Select].NameHint[3] = 0;
        if (NewName->Length/sizeof(WCHAR) < 4) {
            i = NewName->Length/sizeof(WCHAR);
        } else {
            i = 4;
        }
        do {
            if ((USHORT)NewName->Buffer[i-1] > (UCHAR)-1) {
                //
                // Can't compress this name. Leave NameHint[0]==0
                // to force the name to be looked up in the key.
                //
                break;
            }
            FastLeaf->List[Select].NameHint[i-1] = (UCHAR)NewName->Buffer[i-1];
            i--;
        } while ( i>0 );
    } else {
        Leaf->List[Select] = NewKey;
    }
    Leaf->Count += 1;
    return NewCell;
}


HCELL_INDEX
CmpSelectLeaf(
    PHHIVE          Hive,
    PCM_KEY_NODE    ParentKey,
    PUNICODE_STRING NewName,
    HSTORAGE_TYPE   Type,
    PHCELL_INDEX    *RootPointer
    )
/*++

Routine Description:

    This routine is only called if the subkey index for a cell is NOT
    simply a single Leaf index block.

    It selects the Leaf index block to which a new entry is to be
    added.  It may create this block by splitting an existing Leaf
    block.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    ParentKey - mapped pointer to parent key

    NewName - pointer to unicode string naming entry to add

    Type - Stable or Volatile, describes Child's storage

    RootPointer - pointer to variable to receive address of HCELL_INDEX
                that points to Leaf block returned as function argument.
                Used for updates.

Return Value:

    HCELL_NIL - resource problem

    Else, cell index of Leaf index block to add entry to

--*/
{
    HCELL_INDEX     LeafCell;
    HCELL_INDEX     WorkCell;
    PCM_KEY_INDEX   Index;
    PCM_KEY_INDEX   Leaf;
    ULONG           RootSelect;
    LONG            Result;

    CMLOG(CML_MAJOR, CMS_INDEX) {
        KdPrint(("CmpSelectLeaf:\n\t"));
        KdPrint(("Hive=%08lx ParentKey=%08lx\n", Hive, ParentKey));
    }


    //
    // Force root to always be dirty, since we'll either grow it or edit it,
    // and it needs to be marked dirty for BOTH cases.  (Edit may not
    // occur until after we leave
    //
    if (! HvMarkCellDirty(Hive, ParentKey->SubKeyLists[Type])) {
        return HCELL_NIL;
    }

    //
    // must find the proper leaf
    //
    Index = (PCM_KEY_INDEX)HvGetCell(Hive, ParentKey->SubKeyLists[Type]);
    ASSERT(Index->Signature == CM_KEY_INDEX_ROOT);

    while (TRUE) {

        RootSelect = CmpFindSubKeyInRoot(Hive, Index, NewName, &LeafCell);

        if (LeafCell == HCELL_NIL) {

            //
            // Leaf of interest is somewhere near RootSelect
            //
            // . Always use lowest order leaf we can get away with
            // . Never split a leaf if there's one with space we can use
            // . When we split a leaf, we have to repeat search
            //
            // If (NewKey is below lowest key in selected)
            //    If there's a Leaf below selected with space
            //       use the leaf below
            //    else
            //       use the leaf (split it if not enough space)
            // Else
            //    must be above highest key in selected, less than
            //      lowest key in Leaf to right of selected
            //       if space in selected
            //          use selected
            //       else if space in leaf above selected
            //          use leaf above
            //       else
            //          split selected
            //
            LeafCell = Index->List[RootSelect];
            Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
            WorkCell = Leaf->List[0];
            Result = CmpDoCompareKeyName(Hive, NewName, WorkCell);
            ASSERT(Result != 0);

            if (Result < 0) {

                //
                // new is off the left end of Selected
                //
                if (RootSelect > 0) {

                    //
                    // there's a Leaf to the left, try to use it
                    //
                    LeafCell = Index->List[RootSelect-1];
                    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

                    if (Leaf->Count < (CM_MAX_INDEX - 1)) {
                        RootSelect--;
                        *RootPointer = &(Index->List[RootSelect]);
                        break;
                    }

                } else {
                    //
                    // new key is off the left end of the leftmost leaf.
                    // Use the leftmost leaf, if there's enough room
                    //
                    LeafCell = Index->List[0];
                    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
                    if (Leaf->Count < (CM_MAX_INDEX - 1)) {
                        *RootPointer = &(Index->List[0]);
                        break;
                    }
                }

                //
                // else fall to split case
                //

            } else {

                //
                // since new key is not in a Leaf, and is not off
                // the left end of the ResultSelect Leaf, it must
                // be off the right end.
                //
                LeafCell = Index->List[RootSelect];
                Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

                if (Leaf->Count < (CM_MAX_INDEX - 1)) {
                    *RootPointer = &(Index->List[RootSelect]);
                    break;
                }

                //
                // No space, see if there's a leaf to the rigth
                // and if it has space
                //
                if (RootSelect < (ULONG)(Index->Count - 1)) {

                    LeafCell = Index->List[RootSelect+1];
                    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

                    if (Leaf->Count < (CM_MAX_INDEX - 1)) {
                        *RootPointer = &(Index->List[RootSelect+1]);
                        break;
                    }
                }

                //
                // fall to split case
                //
            }

        } else {   // LeafCell != HCELL_NIL

            //
            // Since newkey cannot already be in tree, it must be
            // greater than the bottom of Leaf and less than the top,
            // therefore it must go in Leaf.  If no space, split it.
            //
            Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
            if (Leaf->Count < (CM_MAX_INDEX - 1)) {

                *RootPointer = &(Index->List[RootSelect]);
                break;
            }

            //
            // fall to split case
            //
        }

        //
        // either no neigbor, or no space in neighbor, so split
        //
        WorkCell = CmpSplitLeaf(
                        Hive,
                        ParentKey->SubKeyLists[Type],       // root cell
                        RootSelect,
                        Type
                        );
        if (WorkCell == HCELL_NIL) {
            return HCELL_NIL;
        }

        ParentKey->SubKeyLists[Type] = WorkCell;
        Index = (PCM_KEY_INDEX)HvGetCell(Hive, WorkCell);
        ASSERT(Index->Signature == CM_KEY_INDEX_ROOT);

    } // while(true)
    return LeafCell;
}


HCELL_INDEX
CmpSplitLeaf(
    PHHIVE          Hive,
    HCELL_INDEX     RootCell,
    ULONG           RootSelect,
    HSTORAGE_TYPE   Type
    )
/*++

Routine Description:

    Split the Leaf index block specified by RootSelect, causing both
    of the split out Leaf blocks to appear in the Root index block
    specified by RootCell.

    Caller is expected to have marked old root cell dirty.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    RootCell - cell of the Root index block of index being grown

    RootSelect - indicates which child of Root to split

    Type - Stable or Volatile

Return Value:

    HCELL_NIL - some resource problem

    Else - cell of new (e.g. reallocated) Root index block

--*/
{
    PCM_KEY_INDEX   Root;
    HCELL_INDEX     LeafCell;
    PCM_KEY_INDEX   Leaf;
    HCELL_INDEX     NewLeafCell;
    PCM_KEY_INDEX   NewLeaf;
    ULONG           Size;
    ULONG           freecount;
    USHORT          OldCount;
    USHORT          KeepCount;
    USHORT          NewCount;

    CMLOG(CML_MAJOR, CMS_INDEX) {
        KdPrint(("CmpSplitLeaf:\n\t"));
        KdPrint(("Hive=%08lx RootCell=%08lx RootSelect\n", Hive, RootCell, RootSelect));
    }

    //
    // allocate new Leaf index block
    //
    Root = (PCM_KEY_INDEX)HvGetCell(Hive, RootCell);
    LeafCell = Root->List[RootSelect];
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    OldCount = Leaf->Count;

    KeepCount = (USHORT)(OldCount / 2);     // # of entries to keep in org. Leaf
    NewCount = (OldCount - KeepCount);      // # of entries to move

    Size = (sizeof(HCELL_INDEX) * NewCount) +
            FIELD_OFFSET(CM_KEY_INDEX, List) + 1;   // +1 to assure room for add

    if (!HvMarkCellDirty(Hive, LeafCell)) {
        return HCELL_NIL;
    }

    NewLeafCell = HvAllocateCell(Hive, Size, Type);
    if (NewLeafCell == HCELL_NIL) {
        return HCELL_NIL;
    }
    NewLeaf = (PCM_KEY_INDEX)HvGetCell(Hive, NewLeafCell);
    NewLeaf->Signature = CM_KEY_INDEX_LEAF;


    //
    // compute number of free slots left in the root
    //
    Size = HvGetCellSize(Hive, Root);
    Size = Size - ((sizeof(HCELL_INDEX) * Root->Count) +
              FIELD_OFFSET(CM_KEY_INDEX, List));
    freecount = Size / sizeof(HCELL_INDEX);


    //
    // grow the root if it isn't big enough
    //
    if (freecount < 1) {
        Size = HvGetCellSize(Hive, Root) + sizeof(HCELL_INDEX);
        RootCell = HvReallocateCell(Hive, RootCell, Size);
        if (RootCell == HCELL_NIL) {
            HvFreeCell(Hive, NewLeafCell);
            return HCELL_NIL;
        }
        Root = (PCM_KEY_INDEX)HvGetCell(Hive, RootCell);
    }


    //
    // copy data from one Leaf to the other
    //
    RtlMoveMemory(
        (PVOID)&(NewLeaf->List[0]),
        (PVOID)&(Leaf->List[KeepCount]),
        sizeof(HCELL_INDEX) * NewCount
        );

    ASSERT(KeepCount != 0);
    ASSERT(NewCount  != 0);

    Leaf->Count = KeepCount;
    NewLeaf->Count = NewCount;


    //
    // make an open slot in the root
    //
    if (RootSelect < (ULONG)(Root->Count-1)) {
        RtlMoveMemory(
            (PVOID)&(Root->List[RootSelect+2]),
            (PVOID)&(Root->List[RootSelect+1]),
            (Root->Count - (RootSelect + 1)) * sizeof(HCELL_INDEX)
            );
    }

    //
    // update the root
    //
    Root->Count += 1;
    Root->List[RootSelect+1] = NewLeafCell;
    return RootCell;
}


BOOLEAN
CmpMarkIndexDirty(
    PHHIVE          Hive,
    HCELL_INDEX     ParentKey,
    HCELL_INDEX     TargetKey
    )
/*++

Routine Description:

    Mark as dirty relevent cells of a subkey index.  The Leaf that
    points to TargetKey, and the Root index block, if applicable,
    will be marked dirty.  This call assumes we are setting up
    for a subkey delete.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    ParentKey - key from whose subkey list delete is to be performed

    TargetKey - key being deleted

Return Value:

    TRUE - it worked, FALSE - it didn't, some resource problem

--*/
{
    PCM_KEY_NODE    pcell;
    ULONG           i;
    HCELL_INDEX     IndexCell;
    PCM_KEY_INDEX   Index;
    HCELL_INDEX     Child = HCELL_NIL;
    UNICODE_STRING  SearchName;
    BOOLEAN         IsCompressed;

    pcell = (PCM_KEY_NODE)HvGetCell(Hive, TargetKey);
    if (pcell->Flags & KEY_COMP_NAME) {
        IsCompressed = TRUE;
        SearchName.Length = CmpCompressedNameSize(pcell->Name, pcell->NameLength);
        SearchName.MaximumLength = SearchName.Length;
        SearchName.Buffer = (Hive->Allocate)(SearchName.Length, FALSE);
        if (SearchName.Buffer==NULL) {
            return(FALSE);
        }
        CmpCopyCompressedName(SearchName.Buffer,
                              SearchName.MaximumLength,
                              pcell->Name,
                              pcell->NameLength);
    } else {
        IsCompressed = FALSE;
        SearchName.Length = pcell->NameLength;
        SearchName.MaximumLength = pcell->NameLength;
        SearchName.Buffer = &(pcell->Name[0]);
    }

    pcell = (PCM_KEY_NODE)HvGetCell(Hive, ParentKey);

    for (i = 0; i < Hive->StorageTypeCount; i++) {
        if (pcell->SubKeyCounts[i] != 0) {
            ASSERT(HvIsCellAllocated(Hive, pcell->SubKeyLists[i]));
            IndexCell = pcell->SubKeyLists[i];
            Index = (PCM_KEY_INDEX)HvGetCell(Hive, IndexCell);

            if (Index->Signature == CM_KEY_INDEX_ROOT) {

                //
                // target even in index?
                //
                CmpFindSubKeyInRoot(Hive, Index, &SearchName, &Child);
                if (Child == HCELL_NIL) {
                    continue;
                }

                //
                // mark root dirty
                //
                if (! HvMarkCellDirty(Hive, IndexCell)) {
                    goto ErrorExit;
                }

                IndexCell = Child;
                Index = (PCM_KEY_INDEX)HvGetCell(Hive, Child);
            }
            ASSERT((Index->Signature == CM_KEY_INDEX_LEAF) ||
                   (Index->Signature == CM_KEY_FAST_LEAF));

            CmpFindSubKeyInLeaf(Hive, Index, pcell->WorkVar, &SearchName, &Child);
            if (Child != HCELL_NIL) {
                if (IsCompressed) {
                    (Hive->Free)(SearchName.Buffer, SearchName.Length);
                }
                return(HvMarkCellDirty(Hive, IndexCell));
            }
        }
    }

ErrorExit:
    if (IsCompressed) {
        (Hive->Free)(SearchName.Buffer, SearchName.Length);
    }
    return FALSE;
}


BOOLEAN
CmpRemoveSubKey(
    PHHIVE          Hive,
    HCELL_INDEX     ParentKey,
    HCELL_INDEX     TargetKey
    )
/*++

Routine Description:

    Remove the subkey TargetKey refers to from ParentKey's list.

    NOTE:   Assumes that caller has marked relevent cells dirty,
            see CmpMarkIndexDirty.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    ParentKey - key from whose subkey list delete is to be performed

    TargetKey - key being deleted

Return Value:

    TRUE - it worked, FALSE - it didn't, some resource problem

--*/
{
    PCM_KEY_NODE    pcell;
    HCELL_INDEX     LeafCell;
    PCM_KEY_INDEX   Leaf;
    PCM_KEY_FAST_INDEX FastIndex;
    HCELL_INDEX     RootCell = HCELL_NIL;
    PCM_KEY_INDEX   Root = NULL;
    HCELL_INDEX     Child;
    ULONG           Type;
    ULONG           RootSelect;
    ULONG           LeafSelect;
    UNICODE_STRING  SearchName;
    BOOLEAN         IsCompressed;
    WCHAR           CompressedBuffer[50];

    pcell = (PCM_KEY_NODE)HvGetCell(Hive, TargetKey);
    if (pcell->Flags & KEY_COMP_NAME) {
        IsCompressed = TRUE;
        SearchName.Length = CmpCompressedNameSize(pcell->Name, pcell->NameLength);
        SearchName.MaximumLength = SearchName.Length;
        if (SearchName.MaximumLength > sizeof(CompressedBuffer)) {
            SearchName.Buffer = (Hive->Allocate)(SearchName.Length, FALSE);
            if (SearchName.Buffer==NULL) {
                return(FALSE);
            }
        } else {
            SearchName.Buffer = CompressedBuffer;
        }
        CmpCopyCompressedName(SearchName.Buffer,
                              SearchName.MaximumLength,
                              pcell->Name,
                              pcell->NameLength);
    } else {
        IsCompressed = FALSE;
        SearchName.Length = pcell->NameLength;
        SearchName.MaximumLength = pcell->NameLength;
        SearchName.Buffer = &(pcell->Name[0]);
    }

    pcell = (PCM_KEY_NODE)HvGetCell(Hive, ParentKey);
    Type = HvGetCellType(TargetKey);

    ASSERT(pcell->SubKeyCounts[Type] != 0);
    ASSERT(HvIsCellAllocated(Hive, pcell->SubKeyLists[Type]));

    LeafCell = pcell->SubKeyLists[Type];
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

    if (Leaf->Signature == CM_KEY_INDEX_ROOT) {
        RootSelect = CmpFindSubKeyInRoot(Hive, Leaf, &SearchName, &Child);

        ASSERT(Child != FALSE);

        Root = Leaf;
        RootCell = LeafCell;
        LeafCell = Child;
        Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

    }

    ASSERT((Leaf->Signature == CM_KEY_INDEX_LEAF) ||
           (Leaf->Signature == CM_KEY_FAST_LEAF));

    LeafSelect = CmpFindSubKeyInLeaf(Hive, Leaf, pcell->WorkVar, &SearchName, &Child);

    ASSERT(Child != HCELL_NIL);

    //
    // Leaf points to Index Leaf block
    // Child is Index Leaf block cell
    // LeafSelect is Index for List[]
    //
    pcell->SubKeyCounts[Type] -= 1;

    Leaf->Count -= 1;
    if (Leaf->Count == 0) {

        //
        // Empty Leaf, drop it.
        //
        HvFreeCell(Hive, LeafCell);

        if (Root != NULL) {

            Root->Count -= 1;
            if (Root->Count == 0) {

                //
                // Root is empty, free it too.
                //
                HvFreeCell(Hive, RootCell);
                pcell->SubKeyLists[Type] = HCELL_NIL;

            } else if (RootSelect < (ULONG)(Root->Count)) {

                //
                // Middle entry, squeeze root
                //
                RtlMoveMemory(
                    (PVOID)&(Root->List[RootSelect]),
                    (PVOID)&(Root->List[RootSelect+1]),
                    (Root->Count - RootSelect) * sizeof(HCELL_INDEX)
                    );
            }
            //
            // Else RootSelect == last entry, so decrementing count
            // was all we needed to do
            //

        } else {

            pcell->SubKeyLists[Type] = HCELL_NIL;

        }

    } else if (LeafSelect < (ULONG)(Leaf->Count)) {

        if (Leaf->Signature == CM_KEY_INDEX_LEAF) {
            RtlMoveMemory((PVOID)&(Leaf->List[LeafSelect]),
                          (PVOID)&(Leaf->List[LeafSelect+1]),
                          (Leaf->Count - LeafSelect) * sizeof(HCELL_INDEX));
        } else {
            FastIndex = (PCM_KEY_FAST_INDEX)Leaf;
            RtlMoveMemory((PVOID)&(FastIndex->List[LeafSelect]),
                          (PVOID)&(FastIndex->List[LeafSelect+1]),
                          (FastIndex->Count - LeafSelect) * sizeof(CM_INDEX));
        }
    }
    //
    // Else LeafSelect == last entry, so decrementing count was enough
    //

    if ((IsCompressed) &&
        (SearchName.MaximumLength > sizeof(CompressedBuffer))) {
        (Hive->Free)(SearchName.Buffer, SearchName.Length);
    }
    return TRUE;
}
