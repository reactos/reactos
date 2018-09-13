/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmchek.c

Abstract:

    This module implements consistency checking for the registry.
    This module can be linked standalone, cmchek2.c cannot.

Author:

    Bryan M. Willman (bryanwi) 27-Jan-92

Environment:


Revision History:

--*/

#include    "cmp.h"

#define     REG_MAX_PLAUSIBLE_KEY_SIZE \
                ((FIELD_OFFSET(CM_KEY_NODE, Name)) + \
                 (sizeof(WCHAR) * MAX_KEY_NAME_LENGTH) + 16)

//
// Global data used to support checkout
//
extern ULONG   CmpUsedStorage;

extern PCMHIVE CmpMasterHive;

//
// Private prototypes
//

ULONG
CmpCheckRegistry2(
    HCELL_INDEX Cell,
    HCELL_INDEX ParentCell
    );

ULONG
CmpCheckKey(
    HCELL_INDEX Cell,
    HCELL_INDEX ParentCell
    );

ULONG
CmpCheckValueList(
    PHHIVE      Hive,
    PCELL_DATA List,
    ULONG       Count
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmCheckRegistry)
#pragma alloc_text(PAGE,CmpCheckRegistry2)
#pragma alloc_text(PAGE,CmpCheckKey)
#pragma alloc_text(PAGE,CmpCheckValueList)
#endif

//
// debug structures
//

extern struct {
    PHHIVE      Hive;
    ULONG       Status;
} CmCheckRegistryDebug;

extern struct {
    PHHIVE      Hive;
    ULONG       Status;
} CmpCheckRegistry2Debug;

extern struct {
    PHHIVE      Hive;
    ULONG       Status;
    HCELL_INDEX Cell;
    PCELL_DATA  CellPoint;
    PVOID       RootPoint;
    ULONG       Index;
} CmpCheckKeyDebug;

extern struct {
    PHHIVE      Hive;
    ULONG       Status;
    PCELL_DATA  List;
    ULONG       Index;
    HCELL_INDEX Cell;
    PCELL_DATA  CellPoint;
} CmpCheckValueListDebug;

//
// globals private to check code
//
extern PHHIVE  CmpCheckHive;
extern BOOLEAN CmpCheckClean;


ULONG
CmCheckRegistry(
    PCMHIVE CmHive,
    BOOLEAN Clean
    )
/*++

Routine Description:

    Check consistency of the registry within a given hive.  Start from
    root, and check that:
        .   Each child key points back to its parent.
        .   All allocated cells are refered to exactly once
            (requires looking inside the hive structure...)
            [This also detects space leaks.]
        .   All allocated cells are reachable from the root.

    NOTE:   Exactly 1 ref rule may change with security.

Arguments:

    CmHive - supplies a pointer to the CM hive control structure for the
            hive of interest.

    Clean   - if TRUE, references to volatile cells will be zapped
              (done at startup only to avoid groveling hives twice.)
              if FALSE, nothing will be changed.

Return Value:

    0 if Hive is OK.  Error return indicator if not.

    RANGE:  3000 - 3999

--*/
{
    PHHIVE  Hive;
    ULONG   rc = 0;
    ULONG   Storage;

    if (CmHive == CmpMasterHive) {
        return(0);
    }

    CmpUsedStorage = 0;

    CmCheckRegistryDebug.Hive = (PHHIVE)CmHive;
    CmCheckRegistryDebug.Status = 0;


    //
    // check the underlying hive and get storage use
    //
    Hive = &CmHive->Hive;

    rc = HvCheckHive(Hive, &Storage);
    if (rc != 0) {
        CmCheckRegistryDebug.Status = rc;
        return rc;
    }

    //
    // Call recursive helper to check out the values and subkeys
    //
    CmpCheckHive = (PHHIVE)CmHive;
    CmpCheckClean = Clean;
    rc = CmpCheckRegistry2(Hive->BaseBlock->RootCell, HCELL_NIL);

    //
    // Validate all the security descriptors in the hive
    //
    if (!CmpValidateHiveSecurityDescriptors(Hive)) {
        KdPrint(("CmCheckRegistry:"));
        KdPrint((" CmpValidateHiveSecurityDescriptors failed\n"));
        rc = 3040;
        CmCheckRegistryDebug.Status = rc;
    }

#if 0
    //
    // Check allocated storage against used storage
    //
    if (CmpUsedStorage != Storage) {
        KdPrint(("CmCheckRegistry:"));
        KdPrint((" Storage Used:%08lx  Allocated:%08lx\n", UsedStorage, Storage));
        rc = 3042;
        CmCheckRegistryDebug.Status = rc;
    }
#endif

    //
    // Print a bit of a summary (make sure this data avail in all error cases)
    //
    if (rc > 0) {
        KdPrint(("CmCheckRegistry Failed (%d): CmHive:%08lx\n", rc, CmHive));
        KdPrint((" Hive:%08lx Root:%08lx\n", Hive, Hive->BaseBlock->RootCell));
    }

    return rc;
}


ULONG
CmpCheckRegistry2(
    HCELL_INDEX Cell,
    HCELL_INDEX ParentCell
    )
/*++

Routine Description:

    Check consistency of the registry, from a particular cell on down.

        .   Check that the cell's value list, child key list, class,
            security are OK.
        .   Check that each value entry IN the list is OK.
        .   Apply self to each child key list.

Globals:

    CmpCheckHive - hive we're working on

    CmpCheckClean - flag TRUE -> clean off volatiles, FALSE -> don't

Arguments:

    Cell - HCELL_INDEX of subkey to work on.

    ParentCell - expected value of parent cell for Cell, unless
                 HCELL_NIL, in which case ignore.

Return Value:

    0 if Hive is OK.  Error return indicator if not.

    RANGE:  4000 - 4999

--*/
{
    ULONG       Index = 0;
    HCELL_INDEX StartCell = Cell;
    HCELL_INDEX SubKey;
    ULONG       rc = 0;
    PCELL_DATA  pcell;


    CmpCheckRegistry2Debug.Hive = CmpCheckHive;
    CmpCheckRegistry2Debug.Status = 0;

    //
    // A jump to NewKey amounts to a virtual call to check the
    // next child cell. (a descent into the tree)
    //
    // Cell, ParentCell, Index, and globals are defined
    //
    NewKey:
        rc = CmpCheckKey(Cell, ParentCell);
        if (rc != 0) {
            KdPrint(("\tChild is list entry #%08lx\n", Index));
            CmpCheckRegistry2Debug.Status = rc;
            return rc;
        }

        //
        // save Index and check out children
        //
        pcell = HvGetCell(CmpCheckHive, Cell);
        pcell->u.KeyNode.WorkVar = Index;

        for (Index = 0; Index<pcell->u.KeyNode.SubKeyCounts[Stable]; Index++) {

            SubKey = CmpFindSubKeyByNumber(CmpCheckHive,
                                           (PCM_KEY_NODE)HvGetCell(CmpCheckHive,Cell),
                                           Index);

            //
            // "recurse" onto child
            //
            ParentCell = Cell;
            Cell = SubKey;
            goto NewKey;

            ResumeKey:;                 // A jump here is a virtual return
                                        // Cell, ParentCell and Index
                                        // must be defined
        }

        //
        // since we're here, we've checked out all the children
        // of the current cell.
        //
        if (Cell == StartCell) {

            //
            // we are done
            //
            return 0;
        }

        //
        // "return" to "parent instance"
        //
        pcell = HvGetCell(CmpCheckHive, Cell);
        Index = pcell->u.KeyNode.WorkVar;

        Cell = ParentCell;

        pcell = HvGetCell(CmpCheckHive, Cell);
        ParentCell = pcell->u.KeyNode.Parent;

        goto ResumeKey;
}



ULONG
CmpCheckKey(
    HCELL_INDEX Cell,
    HCELL_INDEX ParentCell
    )
/*++

Routine Description:

    Check consistency of the registry, for a particular cell

        .   Check that the cell's value list, child key list, class,
            security are OK.
        .   Check that each value entry IN the list is OK.

Globals:

    CmpCheckHive - hive we're working on

    CmpCheckClean - flag TRUE -> clean off volatiles, FALSE -> don't

Arguments:

    Cell - HCELL_INDEX of subkey to work on.

    ParentCell - expected value of parent cell for Cell, unless
                 HCELL_NIL, in which case ignore.

Return Value:

    0 if Hive is OK.  Error return indicator if not.

    RANGE:  4000 - 4999

--*/
{
    PCELL_DATA      pcell;
    ULONG           size;
    ULONG           usedlen;
    ULONG           ClassLength;
    HCELL_INDEX     Class;
    ULONG           ValueCount;
    HCELL_INDEX     ValueList;
    HCELL_INDEX     Security;
    ULONG           rc = 0;
    ULONG           nrc = 0;
    ULONG           i;
    PCM_KEY_INDEX   Root;
    PCM_KEY_INDEX   Leaf;
    ULONG           SubCount;

    CmpCheckKeyDebug.Hive = CmpCheckHive;
    CmpCheckKeyDebug.Status = 0;
    CmpCheckKeyDebug.Cell = Cell;
    CmpCheckKeyDebug.CellPoint = NULL;
    CmpCheckKeyDebug.RootPoint = NULL;
    CmpCheckKeyDebug.Index = (ULONG)-1;

    //
    // Check key itself
    //
    if (! HvIsCellAllocated(CmpCheckHive, Cell)) {
        KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
        KdPrint(("\tNot allocated\n"));
        rc = 4010;
        CmpCheckKeyDebug.Status = rc;
        return rc;
    }
    pcell = HvGetCell(CmpCheckHive, Cell);
    SetUsed(CmpCheckHive, Cell);

    CmpCheckKeyDebug.CellPoint = pcell;

    size = HvGetCellSize(CmpCheckHive, pcell);
    if (size > REG_MAX_PLAUSIBLE_KEY_SIZE) {
        KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
        KdPrint(("\tImplausible size %lx\n", size));
        rc = 4020;
        CmpCheckKeyDebug.Status = rc;
        return rc;
    }
    usedlen = FIELD_OFFSET(CM_KEY_NODE, Name) + pcell->u.KeyNode.NameLength;
    if (usedlen > size) {
        KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
        KdPrint(("\tKey is bigger than containing cell.\n"));
        rc = 4030;
        CmpCheckKeyDebug.Status = rc;
        return rc;
    }
    if (pcell->u.KeyNode.Signature != CM_KEY_NODE_SIGNATURE) {
        KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
        KdPrint(("\tNo key signature\n"));
        rc = 4040;
        CmpCheckKeyDebug.Status = rc;
        return rc;
    }
    if (ParentCell != HCELL_NIL) {
        if (pcell->u.KeyNode.Parent != ParentCell) {
            KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
            KdPrint(("\tWrong parent value.\n"));
            rc = 4045;
            CmpCheckKeyDebug.Status = rc;
            return rc;
        }
    }
    ClassLength = pcell->u.KeyNode.ClassLength;
    Class = pcell->u.KeyNode.Class;
    ValueCount = pcell->u.KeyNode.ValueList.Count;
    ValueList = pcell->u.KeyNode.ValueList.List;
    Security = pcell->u.KeyNode.Security;

    //
    // Check simple non-empty cases
    //
    if (ClassLength > 0) {
        if( Class == HCELL_NIL ) {
            pcell->u.KeyNode.ClassLength = 0;
            HvMarkCellDirty(CmpCheckHive, Cell);
            KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx has ClassLength = %lu and Class == HCELL_NIL\n", CmpCheckHive, Cell,ClassLength));
        } else {
            if (HvIsCellAllocated(CmpCheckHive, Class) == FALSE) {
                    KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
                    KdPrint(("\tClass:%08lx - unallocated class\n", Class));
                    rc = 4080;
                    CmpCheckKeyDebug.Status = rc;
                    return rc;
                } else {
                    SetUsed(CmpCheckHive, Class);
                }
        }
    }

    if (Security != HCELL_NIL) {
        if (HvIsCellAllocated(CmpCheckHive, Security) == FALSE) {
            KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
            KdPrint(("\tSecurity:%08lx - unallocated security\n", Security));
            rc = 4090;
            CmpCheckKeyDebug.Status = rc;
            return rc;
        } else if (HvGetCellType(Security) == Volatile) {
            SetUsed(CmpCheckHive, Security);
        }
        //
        // Else CmpValidateHiveSecurityDescriptors must do computation
        //
    }

    //
    // Check value list case
    //
    if (ValueCount > 0) {
        if (HvIsCellAllocated(CmpCheckHive, ValueList) == FALSE) {
            KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
            KdPrint(("\tValueList:%08lx - unallocated valuelist\n", ValueList));
            rc = 4100;
            CmpCheckKeyDebug.Status = rc;
            return rc;
        } else {
            SetUsed(CmpCheckHive, ValueList);
            pcell = HvGetCell(CmpCheckHive, ValueList);
            nrc = CmpCheckValueList(CmpCheckHive, pcell, ValueCount);
            if (nrc != 0) {
                KdPrint(("List was for CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
                rc = nrc;
                CmpCheckKeyDebug.CellPoint = pcell;
                CmpCheckKeyDebug.Status = rc;
                return rc;
            }
        }
    }


    //
    // Check subkey list case
    //

    pcell = HvGetCell(CmpCheckHive, Cell);
    CmpCheckKeyDebug.CellPoint = pcell;
    if ((HvGetCellType(Cell) == Volatile) &&
        (pcell->u.KeyNode.SubKeyCounts[Stable] != 0))
    {
        KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
        KdPrint(("\tVolatile Cell has Stable children\n"));
        rc = 4108;
        CmpCheckKeyDebug.Status = rc;
        return rc;
    } else if (pcell->u.KeyNode.SubKeyCounts[Stable] > 0) {
        if (! HvIsCellAllocated(CmpCheckHive, pcell->u.KeyNode.SubKeyLists[Stable])) {
            KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
            KdPrint(("\tStableKeyList:%08lx - unallocated\n", pcell->u.KeyNode.SubKeyLists[Stable]));
            rc = 4110;
            CmpCheckKeyDebug.Status = rc;
            return rc;
        } else {
            SetUsed(CmpCheckHive, pcell->u.KeyNode.SubKeyLists[Stable]);

            //
            // Prove that the index is OK
            //
            Root = (PCM_KEY_INDEX)HvGetCell(
                                    CmpCheckHive,
                                    pcell->u.KeyNode.SubKeyLists[Stable]
                                    );
            CmpCheckKeyDebug.RootPoint = Root;
            if ((Root->Signature == CM_KEY_INDEX_LEAF) ||
                (Root->Signature == CM_KEY_FAST_LEAF)) {
                if ((ULONG)Root->Count != pcell->u.KeyNode.SubKeyCounts[Stable]) {
                    KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
                    KdPrint(("\tBad Index count @%08lx\n", Root));
                    rc = 4120;
                    CmpCheckKeyDebug.Status = rc;
                    return rc;
                }
            } else if (Root->Signature == CM_KEY_INDEX_ROOT) {
                SubCount = 0;
                for (i = 0; i < Root->Count; i++) {
                    CmpCheckKeyDebug.Index = i;
                    if (! HvIsCellAllocated(CmpCheckHive, Root->List[i])) {
                        KdPrint(("CmpCheckKey: Hive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
                        KdPrint(("\tBad Leaf Cell %08lx Root@%08lx\n", Root->List[i], Root));
                        rc = 4130;
                        CmpCheckKeyDebug.Status = rc;
                        return rc;
                    }
                    Leaf = (PCM_KEY_INDEX)HvGetCell(CmpCheckHive,
                                                    Root->List[i]);
                    if ((Leaf->Signature != CM_KEY_INDEX_LEAF) &&
                        (Leaf->Signature != CM_KEY_FAST_LEAF)) {
                        KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
                        KdPrint(("\tBad Leaf Index @%08lx Root@%08lx\n", Leaf, Root));
                        rc = 4140;
                        CmpCheckKeyDebug.Status = rc;
                        return rc;
                    }
                    SetUsed(CmpCheckHive, Root->List[i]);
                    SubCount += Leaf->Count;
                }
                if (pcell->u.KeyNode.SubKeyCounts[Stable] != SubCount) {
                    KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
                    KdPrint(("\tBad count in index, SubCount=%08lx\n", SubCount));
                    rc = 4150;
                    CmpCheckKeyDebug.Status = rc;
                    return rc;
                }
            } else {
                KdPrint(("CmpCheckKey: CmpCheckHive:%08lx Cell:%08lx\n", CmpCheckHive, Cell));
                KdPrint(("\tBad Root index signature @%08lx\n", Root));
                rc = 4120;
                CmpCheckKeyDebug.Status = rc;
                return rc;
            }
        }
    }
    //
    // force volatiles to be empty, if this is a load operation
    //
    if (CmpCheckClean == TRUE) {
        pcell->u.KeyNode.SubKeyCounts[Volatile] = 0;
        pcell->u.KeyNode.SubKeyLists[Volatile] = HCELL_NIL;
    }

    return rc;
}


ULONG
CmpCheckValueList(
    PHHIVE      Hive,
    PCELL_DATA List,
    ULONG       Count
    )
/*++

Routine Description:

    Check consistency of a value list.
        .   Each element allocated?
        .   Each element have valid signature?
        .   Data properly allocated?

Arguments:

    Hive - containing Hive.

    List - pointer to an array of HCELL_INDEX entries.

    Count - number of entries in list.

Return Value:

    0 if Hive is OK.  Error return indicator if not.

    RANGE:  5000 - 5999

--*/
{
    ULONG   i;
    HCELL_INDEX Cell;
    PCELL_DATA pcell;
    ULONG   size;
    ULONG   usedlen;
    ULONG   DataLength;
    HCELL_INDEX Data;
    ULONG   rc = 0;

    CmpCheckValueListDebug.Hive = Hive;
    CmpCheckValueListDebug.Status = 0;
    CmpCheckValueListDebug.List = List;
    CmpCheckValueListDebug.Index = (ULONG)-1;
    CmpCheckValueListDebug.Cell = 0;   // NOT HCELL_NIL
    CmpCheckValueListDebug.CellPoint = NULL;

    for (i = 0; i < Count; i++) {

        //
        // Check out value entry's refs.
        //
        Cell = List->u.KeyList[i];
        if (Cell == HCELL_NIL) {
            KdPrint(("CmpCheckValueList: List:%08lx i:%08lx\n", List, i));
            KdPrint(("\tEntry is null\n"));
            rc = 5010;
            CmpCheckValueListDebug.Status = rc;
            CmpCheckValueListDebug.Index = i;
            CmpCheckValueListDebug.Cell = Cell;
            return rc;
        }
        if (HvIsCellAllocated(Hive, Cell) == FALSE) {
            KdPrint(("CmpCheckValueList: List:%08lx i:%08lx\n", List, i));
            KdPrint(("\tEntry is not allocated\n"));
            rc = 5020;
            CmpCheckValueListDebug.Status = rc;
            CmpCheckValueListDebug.Index = i;
            CmpCheckValueListDebug.Cell = Cell;
            return rc;
        } else {
            SetUsed(Hive, Cell);
        }

        //
        // Check out the value entry itself
        //
        pcell = HvGetCell(Hive, Cell);
        size = HvGetCellSize(Hive, pcell);
        if (pcell->u.KeyValue.Signature != CM_KEY_VALUE_SIGNATURE) {
            KdPrint(("CmpCheckValueList: List:%08lx i:%08lx\n", List, i));
            KdPrint(("\tCell:%08lx - invalid value signature\n", Cell));
            rc = 5030;
            CmpCheckValueListDebug.Status = rc;
            CmpCheckValueListDebug.Index = i;
            CmpCheckValueListDebug.Cell = Cell;
            CmpCheckValueListDebug.CellPoint = pcell;
            return rc;
        }
        usedlen = FIELD_OFFSET(CM_KEY_VALUE, Name) + pcell->u.KeyValue.NameLength;
        if (usedlen > size) {
            KdPrint(("CmpCheckValueList: List:%08lx i:%08lx\n", List, i));
            KdPrint(("\tCell:%08lx - value bigger than containing cell\n", Cell));
            rc = 5040;
            CmpCheckValueListDebug.Status = rc;
            CmpCheckValueListDebug.Index = i;
            CmpCheckValueListDebug.Cell = Cell;
            CmpCheckValueListDebug.CellPoint = pcell;
            return rc;
        }

        //
        // Check out value entry's data
        //
        DataLength = pcell->u.KeyValue.DataLength;
        if (DataLength < CM_KEY_VALUE_SPECIAL_SIZE) {
            Data = pcell->u.KeyValue.Data;
            if ((DataLength == 0) && (Data != HCELL_NIL)) {
                KdPrint(("CmpCheckValueList: List:%08lx i:%08lx\n", List, i));
                KdPrint(("\tCell:%08lx Data:%08lx - data not null\n", Cell, Data));
                rc = 5050;
                CmpCheckValueListDebug.Status = rc;
                CmpCheckValueListDebug.Index = i;
                CmpCheckValueListDebug.Cell = Cell;
                CmpCheckValueListDebug.CellPoint = pcell;
                return rc;
            }
            if (DataLength > 0) {
                if (HvIsCellAllocated(Hive, Data) == FALSE) {
                    KdPrint(("CmpCheckValueList: List:%08lx i:%08lx\n", List, i));
                    KdPrint(("\tCell:%08lx Data:%08lx - unallocated\n", Cell, Data));
                    rc = 5060;
                    CmpCheckValueListDebug.Status = rc;
                    CmpCheckValueListDebug.Index = i;
                    CmpCheckValueListDebug.Cell = Cell;
                    CmpCheckValueListDebug.CellPoint = pcell;
                    return rc;
                } else {
                    SetUsed(Hive, Data);
                }
            }
        }
    }
    return rc;
}
