/*
 * PROJECT:         ReactOS Registry Library
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            lib/cmlib/cmindex.c
 * PURPOSE:         Configuration Manager - Cell Indexes
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "cmlib.h"
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define INVALID_INDEX   0x80000000

#define CmpMaxFastIndexPerHblock                        \
    ((HBLOCK_SIZE - (sizeof(HBIN) + sizeof(HCELL) +     \
                     FIELD_OFFSET(CM_KEY_FAST_INDEX, List))) / sizeof(CM_INDEX))

#define CmpMaxIndexPerHblock                            \
    ((HBLOCK_SIZE - (sizeof(HBIN) + sizeof(HCELL) +     \
                     FIELD_OFFSET(CM_KEY_INDEX, List))) / sizeof(HCELL_INDEX) - 1)

/* FUNCTIONS *****************************************************************/

LONG
NTAPI
CmpDoCompareKeyName(IN PHHIVE Hive,
                    IN PCUNICODE_STRING SearchName,
                    IN HCELL_INDEX Cell)
{
    PCM_KEY_NODE Node;
    UNICODE_STRING KeyName;
    LONG Result;

    /* Get the node */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if (!Node) return 2;

    /* Check if it's compressed */
    if (Node->Flags & KEY_COMP_NAME)
    {
        /* Compare compressed names */
        Result = CmpCompareCompressedName(SearchName,
                                          Node->Name,
                                          Node->NameLength);
    }
    else
    {
        /* Compare the Unicode name directly */
        KeyName.Buffer = Node->Name;
        KeyName.Length = Node->NameLength;
        KeyName.MaximumLength = KeyName.Length;
        Result = RtlCompareUnicodeString(SearchName, &KeyName, TRUE);
    }

    /* Release the cell and return the normalized result */
    HvReleaseCell(Hive, Cell);
    return (Result == 0) ? Result : ((Result > 0) ? 1 : -1);
}

LONG
NTAPI
CmpCompareInIndex(IN PHHIVE Hive,
                  IN PCUNICODE_STRING SearchName,
                  IN ULONG Count,
                  IN PCM_KEY_INDEX Index,
                  IN PHCELL_INDEX SubKey)
{
    PCM_KEY_FAST_INDEX FastIndex;
    PCM_INDEX FastEntry;
    LONG Result;
    ULONG i;
    ULONG ActualNameLength = 4, CompareLength, NameLength;
    WCHAR p, pp;

    /* Assume failure */
    *SubKey = HCELL_NIL;

    /* Check if we are a fast or hashed leaf */
    if ((Index->Signature == CM_KEY_FAST_LEAF) ||
        (Index->Signature == CM_KEY_HASH_LEAF))
    {
        /* Get the Fast/Hash Index */
        FastIndex = (PCM_KEY_FAST_INDEX)Index;
        FastEntry = &FastIndex->List[Count];

        /* Check if we are a hash leaf, in which case we skip all this */
        if (Index->Signature == CM_KEY_FAST_LEAF)
        {
            /* Find out just how much of the name is there */
            for (i = 0; i < 4; i++)
            {
                /* Check if this entry is empty */
                if (!FastEntry->NameHint[i])
                {
                    /* Only this much! */
                    ActualNameLength = i;
                    break;
                }
            }

            /* How large is the name and how many characters to compare */
            NameLength = SearchName->Length / sizeof(WCHAR);
            CompareLength = min(NameLength, ActualNameLength);

            /* Loop all the chars we'll test */
            for (i = 0; i < CompareLength; i++)
            {
                /* Get one char from each buffer */
                p = SearchName->Buffer[i];
                pp = FastEntry->NameHint[i];

                /* See if they match and return result if they don't */
                Result = (LONG)RtlUpcaseUnicodeChar(p) -
                         (LONG)RtlUpcaseUnicodeChar(pp);
                if (Result) return (Result > 0) ? 1 : -1;
            }
        }

        /* If we got here then we have to do a full compare */
        Result = CmpDoCompareKeyName(Hive, SearchName, FastEntry->Cell);
        if (Result == 2) return Result;
        if (!Result) *SubKey = FastEntry->Cell;
    }
    else
    {
        /* We aren't, so do a name compare and return the subkey found */
        Result = CmpDoCompareKeyName(Hive, SearchName, Index->List[Count]);
        if (Result == 2) return Result;
        if (!Result) *SubKey = Index->List[Count];
    }

    /* Return the comparison result */
    return Result;
}

ULONG
NTAPI
CmpFindSubKeyInRoot(IN PHHIVE Hive,
                    IN PCM_KEY_INDEX Index,
                    IN PCUNICODE_STRING SearchName,
                    IN PHCELL_INDEX SubKey)
{
    ULONG High, Low = 0, i = 0, ReturnIndex;
    HCELL_INDEX LeafCell;
    PCM_KEY_INDEX Leaf;
    LONG Result;

    /* Verify Index for validity */
    ASSERT(Index->Count != 0);
    ASSERT(Index->Signature == CM_KEY_INDEX_ROOT);

    /* Set high limit and loop */
    High = Index->Count - 1;
    while (TRUE)
    {
        /* Choose next entry */
        i = ((High - Low) / 2) + Low;

        /* Get the leaf cell and then the leaf itself */
        LeafCell = Index->List[i];
        Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
        if (Leaf)
        {
            /* Make sure the leaf is valid */
            ASSERT((Leaf->Signature == CM_KEY_INDEX_LEAF) ||
                   (Leaf->Signature == CM_KEY_FAST_LEAF) ||
                   (Leaf->Signature == CM_KEY_HASH_LEAF));
            ASSERT(Leaf->Count != 0);

            /* Do the compare */
            Result = CmpCompareInIndex(Hive,
                                       SearchName,
                                       Leaf->Count - 1,
                                       Leaf,
                                       SubKey);
            if (Result == 2) goto Big;

            /* Check if we found the leaf */
            if (!Result)
            {
                /* We found the leaf */
                *SubKey = LeafCell;
                ReturnIndex = i;
                goto Return;
            }

            /* Check for negative result */
            if (Result < 0)
            {
                /* If we got here, we should be at -1 */
                ASSERT(Result == -1);

                /* Do another lookup, since we might still be in the right leaf */
                Result = CmpCompareInIndex(Hive,
                                           SearchName,
                                           0,
                                           Leaf,
                                           SubKey);
                if (Result == 2) goto Big;

                /* Check if it's not below */
                if (Result >= 0)
                {
                    /*
                     * If the name was first below, and now it is above,
                     * then this means that it is somewhere in this leaf.
                     * Make sure we didn't get some weird result
                     */
                    ASSERT((Result == 1) || (Result == 0));

                    /* Return it */
                    *SubKey = LeafCell;
                    ReturnIndex = i;
                    goto Return;
                }

                /* Update the limit to this index, since we know it's not higher. */
                High = i;
            }
            else
            {
                /* Update the base to this index, since we know it's not lower. */
                Low = i;
            }
        }
        else
        {
Big:
            /* This was some sort of special key */
            ReturnIndex = INVALID_INDEX;
            goto ReturnFailure;
        }

        /* Check if there is only one entry left */
        if ((High - Low) <= 1) break;

        /* Release the leaf cell */
        HvReleaseCell(Hive, LeafCell);
    }

    /* Make sure we got here for the right reasons */
    ASSERT((High - Low == 1) || (High == Low));

    /* Get the leaf cell and the leaf */
    LeafCell = Index->List[Low];
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    if (!Leaf) goto Big;

    /* Do the compare */
    Result = CmpCompareInIndex(Hive,
                               SearchName,
                               Leaf->Count-1,
                               Leaf,
                               SubKey);
    if (Result == 2) goto Big;

    /* Check if we found it */
    if (!Result)
    {
        /* We got lucky... return it */
        *SubKey = LeafCell;
        ReturnIndex = Low;
        goto Return;
    }

    /* It's below, so could still be in this leaf */
    if (Result < 0)
    {
        /* Make sure we're -1 */
        ASSERT(Result == -1);

        /* Do a search from the bottom */
        Result = CmpCompareInIndex(Hive, SearchName, 0, Leaf, SubKey);
        if (Result == 2) goto Big;

        /*
         * Check if it's above, which means that it's within the ranges of our
         * leaf (since we were below before).
         */
        if (Result >= 0)
        {
            /* Sanity check */
            ASSERT((Result == 1) || (Result == 0));

            /* Yep, so we're in the right leaf; return it. */
            *SubKey = LeafCell;
            ReturnIndex = Low;
            goto Return;
        }

        /* It's still below us, so fail */
        ReturnIndex = Low;
        goto ReturnFailure;
    }

    /* Release the leaf cell */
    HvReleaseCell(Hive, LeafCell);

    /* Well the low didn't work too well, so try the high. */
    LeafCell = Index->List[High];
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    if (!Leaf) goto Big;

    /* Do the compare */
    Result = CmpCompareInIndex(Hive,
                               SearchName,
                               Leaf->Count - 1,
                               Leaf,
                               SubKey);
    if (Result == 2) goto Big;

    /* Check if we found it */
    if (Result == 0)
    {
        /* We got lucky... return it */
        *SubKey = LeafCell;
        ReturnIndex = High;
        goto Return;
    }

    /* Check if we are too high */
    if (Result < 0)
    {
        /* Make sure we're -1 */
        ASSERT(Result == -1);

        /*
         * Once again... since we were first too low and now too high, then
         * this means we are within the range of this leaf... return it.
         */
        *SubKey = LeafCell;
        ReturnIndex = High;
        goto Return;
    }

    /* If we got here, then we are too low, again. */
    ReturnIndex = High;

    /* Failure path */
ReturnFailure:
    *SubKey = HCELL_NIL;

    /* Return path...check if we have a leaf to free */
Return:
    if (Leaf) HvReleaseCell(Hive, LeafCell);

    /* Return the index */
    return ReturnIndex;
}

ULONG
NTAPI
CmpFindSubKeyInLeaf(IN PHHIVE Hive,
                    IN PCM_KEY_INDEX Index,
                    IN PCUNICODE_STRING SearchName,
                    IN PHCELL_INDEX SubKey)
{
    ULONG High, Low = 0, i;
    LONG Result;

    /* Verify Index for validity */
    ASSERT((Index->Signature == CM_KEY_INDEX_LEAF) ||
           (Index->Signature == CM_KEY_FAST_LEAF) ||
           (Index->Signature == CM_KEY_HASH_LEAF));

    /* Get the upper bound and middle entry */
    High = Index->Count - 1;
    i = High / 2;

    /* Check if we don't actually have any entries */
    if (!Index->Count)
    {
        /* Return failure */
        *SubKey = HCELL_NIL;
        return 0;
    }

    /* Start compare loop */
    while (TRUE)
    {
        /* Do the actual comparison and check the result */
        Result = CmpCompareInIndex(Hive, SearchName, i, Index, SubKey);
        if (Result == 2)
        {
            /* Fail with special value */
            *SubKey = HCELL_NIL;
            return INVALID_INDEX;
        }

        /* Check if we got lucky and found it */
        if (!Result) return i;

        /* Check if the result is below us */
        if (Result < 0)
        {
            /* Set the new bound; it can't be higher then where we are now. */
            ASSERT(Result == -1);
            High = i;
        }
        else
        {
            /* Set the new bound... it can't be lower then where we are now. */
            ASSERT(Result == 1);
            Low = i;
        }

        /* Check if this is the last entry, if so, break out and handle it */
        if ((High - Low) <= 1) break;

        /* Set the new index */
        i = ((High - Low) / 2) + Low;
    }

    /*
     * If we get here, High - Low = 1 or High == Low
     * Simply look first at Low, then at High
     */
    Result = CmpCompareInIndex(Hive, SearchName, Low, Index, SubKey);
    if (Result == 2)
    {
        /* Fail with special value */
        *SubKey = HCELL_NIL;
        return INVALID_INDEX;
    }

    /* Check if we got lucky and found it */
    if (!Result) return Low;

    /* Check if the result is below us */
    if (Result < 0)
    {
        /* Return the low entry */
        ASSERT(Result == -1);
        return Low;
    }

    /*
     * If we got here, then just check the high and return it no matter what
     * the result is (since we're a leaf, it has to be near there anyway).
     */
    Result = CmpCompareInIndex(Hive, SearchName, High, Index, SubKey);
    if (Result == 2)
    {
        /* Fail with special value */
        *SubKey = HCELL_NIL;
        return INVALID_INDEX;
    }

    /* Return the high */
    return High;
}

ULONG
NTAPI
CmpComputeHashKey(IN ULONG Hash,
                  IN PCUNICODE_STRING Name,
                  IN BOOLEAN AllowSeparators)
{
    LPWSTR Cp;
    ULONG Value, i;

    /* Make some sanity checks on our parameters */
    ASSERT((Name->Length == 0) ||
           (AllowSeparators) ||
           (Name->Buffer[0] != OBJ_NAME_PATH_SEPARATOR));

    /* If the name is empty, there is nothing to hash! */
    if (!Name->Length) return Hash;

    /* Set the buffer and loop every character */
    Cp = Name->Buffer;
    for (i = 0; i < Name->Length; i += sizeof(WCHAR), Cp++)
    {
        /* Make sure we don't have a separator when we shouldn't */
        ASSERT(AllowSeparators || (*Cp != OBJ_NAME_PATH_SEPARATOR));

        /* Check what kind of char we have */
        if (*Cp >= L'a')
        {
            /* In the lower case region... is it truly lower case? */
            if (*Cp < L'z')
            {
                /* Yes! Calculate it ourselves! */
                Value = *Cp - L'a' + L'A';
            }
            else
            {
                /* No, use the API */
                Value = RtlUpcaseUnicodeChar(*Cp);
            }
        }
        else
        {
            /* Reuse the char, it's already upcased */
            Value = *Cp;
        }

        /* Multiply by a prime and add our value */
        Hash *= 37;
        Hash += Value;
    }

    /* Return the hash */
    return Hash;
}

HCELL_INDEX
NTAPI
CmpDoFindSubKeyByNumber(IN PHHIVE Hive,
                        IN PCM_KEY_INDEX Index,
                        IN ULONG Number)
{
    ULONG i;
    HCELL_INDEX LeafCell = 0;
    PCM_KEY_INDEX Leaf = NULL;
    PCM_KEY_FAST_INDEX FastIndex;
    HCELL_INDEX Result;

    /* Check if this is a root */
    if (Index->Signature == CM_KEY_INDEX_ROOT)
    {
        /* Loop the index */
        for (i = 0; i < Index->Count; i++)
        {
            /* Check if this isn't the first iteration */
            if (i)
            {
                /* Make sure we had something valid, and release it */
                ASSERT(Leaf != NULL );
                ASSERT(LeafCell == Index->List[i - 1]);
                HvReleaseCell(Hive, LeafCell);
            }

            /* Get the leaf cell and the leaf for this index */
            LeafCell = Index->List[i];
            Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
            if (!Leaf) return HCELL_NIL;

            /* Check if the index may be inside it */
            if (Number < Leaf->Count)
            {
                /* Check if this is a fast or hash leaf */
                if ((Leaf->Signature == CM_KEY_FAST_LEAF) ||
                    (Leaf->Signature == CM_KEY_HASH_LEAF))
                {
                    /* Get the fast index */
                    FastIndex = (PCM_KEY_FAST_INDEX)Leaf;

                    /* Look inside the list to get our actual cell */
                    Result = FastIndex->List[Number].Cell;
                    HvReleaseCell(Hive, LeafCell);
                    return Result;
                }
                else
                {
                    /* Regular leaf, so just use the index directly */
                    Result = Leaf->List[Number];

                    /*  Release and return it */
                    HvReleaseCell(Hive,LeafCell);
                    return Result;
                }
            }
            else
            {
                /* Otherwise, skip this leaf */
                Number = Number - Leaf->Count;
            }
        }

        /* Should never get here */
        ASSERT(FALSE);
    }

    /* If we got here, then the cell is in this index */
    ASSERT(Number < Index->Count);

    /* Check if we're a fast or hash leaf */
    if ((Index->Signature == CM_KEY_FAST_LEAF) ||
        (Index->Signature == CM_KEY_HASH_LEAF))
    {
        /* We are, get the fast index and get the cell from the list */
        FastIndex = (PCM_KEY_FAST_INDEX)Index;
        return FastIndex->List[Number].Cell;
    }
    else
    {
        /* We aren't, so use the index directly to grab the cell */
        return Index->List[Number];
    }
}

HCELL_INDEX
NTAPI
CmpFindSubKeyByNumber(IN PHHIVE Hive,
                      IN PCM_KEY_NODE Node,
                      IN ULONG Number)
{
    PCM_KEY_INDEX Index;
    HCELL_INDEX Result = HCELL_NIL;

    /* Check if it's in the stable list */
    if (Number < Node->SubKeyCounts[Stable])
    {
        /* Get the actual key index */
        Index = (PCM_KEY_INDEX)HvGetCell(Hive, Node->SubKeyLists[Stable]);
        if (!Index) return HCELL_NIL;

        /* Do a search inside it */
        Result = CmpDoFindSubKeyByNumber(Hive, Index, Number);

        /* Release the cell and return the result */
        HvReleaseCell(Hive, Node->SubKeyLists[Stable]);
        return Result;
    }
    else if (Hive->StorageTypeCount > Volatile)
    {
        /* It's in the volatile list */
        Number = Number - Node->SubKeyCounts[Stable];
        if (Number < Node->SubKeyCounts[Volatile])
        {
            /* Get the actual key index */
            Index = (PCM_KEY_INDEX)HvGetCell(Hive, Node->SubKeyLists[Volatile]);
            if (!Index) return HCELL_NIL;

            /* Do a search inside it */
            Result = CmpDoFindSubKeyByNumber(Hive, Index, Number);

            /* Release the cell and return the result */
            HvReleaseCell(Hive, Node->SubKeyLists[Volatile]);
            return Result;
        }
    }

    /* Nothing was found */
    return HCELL_NIL;
}

static HCELL_INDEX
NTAPI
CmpFindSubKeyByHash(IN PHHIVE Hive,
                    IN PCM_KEY_FAST_INDEX FastIndex,
                    IN PCUNICODE_STRING SearchName)
{
    ULONG HashKey, i;
    PCM_INDEX FastEntry;

    /* Make sure it's really a hash */
    ASSERT(FastIndex->Signature == CM_KEY_HASH_LEAF);

    /* Compute the hash key for the name */
    HashKey = CmpComputeHashKey(0, SearchName, FALSE);

    /* Loop all the entries */
    for (i = 0; i < FastIndex->Count; i++)
    {
        /* Get the entry */
        FastEntry = &FastIndex->List[i];

        /* Compare the hash first */
        if (FastEntry->HashKey == HashKey)
        {
            /* Go ahead for a full compare */
            if (!(CmpDoCompareKeyName(Hive, SearchName, FastEntry->Cell)))
            {
                /* It matched, return the cell */
                return FastEntry->Cell;
            }
        }
    }

    /* If we got here then we failed */
    return HCELL_NIL;
}

HCELL_INDEX
NTAPI
CmpFindSubKeyByName(IN PHHIVE Hive,
                    IN PCM_KEY_NODE Parent,
                    IN PCUNICODE_STRING SearchName)
{
    ULONG i;
    PCM_KEY_INDEX IndexRoot;
    HCELL_INDEX SubKey, CellToRelease;
    ULONG Found;

    /* Loop each storage type */
    for (i = 0; i < Hive->StorageTypeCount; i++)
    {
        /* Make sure the parent node has subkeys */
        if (Parent->SubKeyCounts[i])
        {
            /* Get the Index */
            IndexRoot = (PCM_KEY_INDEX)HvGetCell(Hive, Parent->SubKeyLists[i]);
            if (!IndexRoot) return HCELL_NIL;

            /* Get the cell we'll need to release */
            CellToRelease = Parent->SubKeyLists[i];

            /* Check if this is another index root */
            if (IndexRoot->Signature == CM_KEY_INDEX_ROOT)
            {
                /* Lookup the name in the root */
                Found = CmpFindSubKeyInRoot(Hive,
                                            IndexRoot,
                                            SearchName,
                                            &SubKey);

                /* Release the previous cell */
                ASSERT(CellToRelease != HCELL_NIL);
                HvReleaseCell(Hive, CellToRelease);

                /* Make sure we found something valid */
                if (Found & INVALID_INDEX) break;

                /* Get the new Index Root and set the new cell to be released */
                if (SubKey == HCELL_NIL) continue;
                CellToRelease = SubKey;
                IndexRoot = (PCM_KEY_INDEX)HvGetCell(Hive, SubKey);
            }

            /* Make sure the signature is what we expect it to be */
            ASSERT((IndexRoot->Signature == CM_KEY_INDEX_LEAF) ||
                   (IndexRoot->Signature == CM_KEY_FAST_LEAF) ||
                   (IndexRoot->Signature == CM_KEY_HASH_LEAF));

            /* Check if this isn't a hashed leaf */
            if (IndexRoot->Signature != CM_KEY_HASH_LEAF)
            {
                /* Find the subkey in the leaf */
                Found = CmpFindSubKeyInLeaf(Hive,
                                            IndexRoot,
                                            SearchName,
                                            &SubKey);

                /* Release the previous cell */
                ASSERT(CellToRelease != HCELL_NIL);
                HvReleaseCell(Hive, CellToRelease);

                /* Make sure we found a valid index */
                if (Found & INVALID_INDEX) break;
            }
            else
            {
                /* Find the subkey in the hash */
                SubKey = CmpFindSubKeyByHash(Hive,
                                             (PCM_KEY_FAST_INDEX)IndexRoot,
                                             SearchName);

                /* Release the previous cell */
                ASSERT(CellToRelease != HCELL_NIL);
                HvReleaseCell(Hive, CellToRelease);
            }

            /* Make sure we got a valid subkey and return it */
            if (SubKey != HCELL_NIL) return SubKey;
        }
    }

    /* If we got here, then we failed */
    return HCELL_NIL;
}

BOOLEAN
NTAPI
CmpMarkIndexDirty(IN PHHIVE Hive,
                  IN HCELL_INDEX ParentKey,
                  IN HCELL_INDEX TargetKey)
{
    PCM_KEY_NODE Node;
    UNICODE_STRING SearchName;
    BOOLEAN IsCompressed;
    ULONG i, Result;
    PCM_KEY_INDEX Index;
    HCELL_INDEX IndexCell, Child = HCELL_NIL, CellToRelease = HCELL_NIL;

    /* Get the target key node */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, TargetKey);
    if (!Node) return FALSE;

    /* Check if it's compressed */
    if (Node->Flags & KEY_COMP_NAME)
    {
        /* Remember this for later */
        IsCompressed = TRUE;

        /* Build the search name */
        SearchName.Length = CmpCompressedNameSize(Node->Name,
                                                  Node->NameLength);
        SearchName.MaximumLength = SearchName.Length;
        SearchName.Buffer = Hive->Allocate(SearchName.Length, TRUE, TAG_CM);
        if (!SearchName.Buffer)
        {
            /* Fail */
            HvReleaseCell(Hive, TargetKey);
            return FALSE;
        }

        /* Copy it */
        CmpCopyCompressedName(SearchName.Buffer,
                              SearchName.MaximumLength,
                              Node->Name,
                              Node->NameLength);
    }
    else
    {
        /* Name isn't compressed, build it directly from the node */
        IsCompressed = FALSE;
        SearchName.Length = Node->NameLength;
        SearchName.MaximumLength = Node->NameLength;
        SearchName.Buffer = Node->Name;
    }

    /* We can release the target key now */
    HvReleaseCell(Hive, TargetKey);

    /* Now get the parent key node */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, ParentKey);
    if (!Node) goto Quickie;

    /* Loop all hive storage */
    for (i = 0; i < Hive->StorageTypeCount; i++)
    {
        /* Check if any subkeys are in this index */
        if (Node->SubKeyCounts[i])
        {
            /* Get the cell index */
            //ASSERT(HvIsCellAllocated(Hive, Node->SubKeyLists[i]));
            IndexCell = Node->SubKeyLists[i];

            /* Check if we had anything to release from before */
            if (CellToRelease != HCELL_NIL)
            {
                /* Release it now */
                HvReleaseCell(Hive, CellToRelease);
                CellToRelease = HCELL_NIL;
            }

            /* Get the key index for the cell */
            Index = (PCM_KEY_INDEX)HvGetCell(Hive, IndexCell);
            if (!Index) goto Quickie;

            /* Release it at the next iteration or below */
            CellToRelease = IndexCell;

            /* Check if this is a root */
            if (Index->Signature == CM_KEY_INDEX_ROOT)
            {
                /* Get the child inside the root */
                Result = CmpFindSubKeyInRoot(Hive, Index, &SearchName, &Child);
                if (Result & INVALID_INDEX) goto Quickie;
                if (Child == HCELL_NIL) continue;

                /* We found it, mark the cell dirty */
                HvMarkCellDirty(Hive, IndexCell, FALSE);

                /* Check if we had anything to release from before */
                if (CellToRelease != HCELL_NIL)
                {
                    /* Release it now */
                    HvReleaseCell(Hive, CellToRelease);
                    CellToRelease = HCELL_NIL;
                }

                /* Now this child is the index, get the actual key index */
                IndexCell = Child;
                Index = (PCM_KEY_INDEX)HvGetCell(Hive, Child);
                if (!Index) goto Quickie;

                /* Release it later */
                CellToRelease = Child;
            }

            /* Make sure this is a valid index */
            ASSERT((Index->Signature == CM_KEY_INDEX_LEAF) ||
                   (Index->Signature == CM_KEY_FAST_LEAF) ||
                   (Index->Signature == CM_KEY_HASH_LEAF));

            /* Find the child in the leaf */
            Result = CmpFindSubKeyInLeaf(Hive, Index, &SearchName, &Child);
            if (Result & INVALID_INDEX) goto Quickie;
            if (Child != HCELL_NIL)
            {
                /* We found it, free the name now */
                if (IsCompressed) Hive->Free(SearchName.Buffer, 0);

                /* Release the parent key */
                HvReleaseCell(Hive, ParentKey);

                /* Check if we had a left over cell to release */
                if (CellToRelease != HCELL_NIL)
                {
                    /* Release it */
                    HvReleaseCell(Hive, CellToRelease);
                }

                /* And mark the index cell dirty */
                HvMarkCellDirty(Hive, IndexCell, FALSE);
                return TRUE;
            }
        }
    }

Quickie:
    /* Release any cells that we still hold */
    if (Node) HvReleaseCell(Hive, ParentKey);
    if (CellToRelease != HCELL_NIL) HvReleaseCell(Hive, CellToRelease);

    /* Free the search name and return failure */
    if (IsCompressed) Hive->Free(SearchName.Buffer, 0);
    return FALSE;
}

HCELL_INDEX
NTAPI
CmpAddToLeaf(IN PHHIVE Hive,
             IN HCELL_INDEX LeafCell,
             IN HCELL_INDEX NewKey,
             IN PCUNICODE_STRING Name)
{
    PCM_KEY_INDEX Leaf;
    PCM_KEY_FAST_INDEX FastLeaf;
    ULONG Size, OldSize, EntrySize, i, j;
    HCELL_INDEX NewCell, Child;
    LONG Result;

    /* Mark the leaf dirty */
    HvMarkCellDirty(Hive, LeafCell, FALSE);

    /* Get the leaf cell */
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    if (!Leaf)
    {
        /* Shouldn't happen */
        ASSERT(FALSE);
        return HCELL_NIL;
    }

    /* Release it */
    HvReleaseCell(Hive, LeafCell);

    /* Check if this is an index leaf */
    if (Leaf->Signature == CM_KEY_INDEX_LEAF)
    {
        /* This is an old-style leaf */
        FastLeaf = NULL;
        EntrySize = sizeof(HCELL_INDEX);
    }
    else
    {
        /* Sanity check */
        ASSERT((Leaf->Signature == CM_KEY_FAST_LEAF) ||
               (Leaf->Signature == CM_KEY_HASH_LEAF));

        /* This is a new-style optimized fast (or hash) leaf */
        FastLeaf = (PCM_KEY_FAST_INDEX)Leaf;
        EntrySize = sizeof(CM_INDEX);
    }

    /* Get the current size of the leaf */
    OldSize = HvGetCellSize(Hive, Leaf);

    /* Calculate the size of the free entries */
    Size = OldSize;
    Size -= EntrySize * Leaf->Count + FIELD_OFFSET(CM_KEY_INDEX, List);

    /* Assume we'll re-use the same leaf */
    NewCell = LeafCell;

    /* Check if we're out of space */
    if ((Size / EntrySize) < 1)
    {
        /* Grow the leaf by 1.5x, making sure we can at least fit this entry */
        Size = OldSize + OldSize / 2;
        if (Size < (OldSize + EntrySize)) Size = OldSize + EntrySize;

        /* Re-allocate the leaf */
        NewCell = HvReallocateCell(Hive, LeafCell, Size);
        if (NewCell == HCELL_NIL) return HCELL_NIL;

        /* Get the leaf cell */
        Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, NewCell);
        if (!Leaf)
        {
            /* This shouldn't happen */
            ASSERT(FALSE);
            return HCELL_NIL;
        }

        /* Release the cell */
        HvReleaseCell(Hive, NewCell);

        /* Update the fast leaf pointer if we had one */
        if (FastLeaf) FastLeaf = (PCM_KEY_FAST_INDEX)Leaf;
    }

    /* Find the insertion point for our entry */
    i = CmpFindSubKeyInLeaf(Hive, Leaf, Name, &Child);
    if (i & INVALID_INDEX) return HCELL_NIL;
    ASSERT(Child == HCELL_NIL);

    /* Check if we're not last */
    if (i != Leaf->Count)
    {
        /* Find out where we should go */
        Result = CmpCompareInIndex(Hive,
                                   Name,
                                   i,
                                   Leaf,
                                   &Child);
        if (Result == 2) return HCELL_NIL;
        ASSERT(Result != 0);

        /* Check if we come after */
        if (Result > 0)
        {
            /* We do, insert us after the key */
            ASSERT(Result == 1);
            i++;
        }

        /* Check if we're still not last */
        if (i != Leaf->Count)
        {
            /* Check if we had a fast leaf or not */
            if (FastLeaf)
            {
                /* Copy the fast indexes */
                RtlMoveMemory(&FastLeaf->List[i + 1],
                              &FastLeaf->List[i],
                              (FastLeaf->Count - i) * sizeof(CM_INDEX));
            }
            else
            {
                /* Copy the indexes themselves */
                RtlMoveMemory(&Leaf->List[i + 1],
                              &Leaf->List[i],
                              (Leaf->Count - i) * sizeof(HCELL_INDEX));
            }
        }
    }

    /* Check if this is a new-style leaf */
    if (FastLeaf)
    {
        /* Set our cell */
        FastLeaf->List[i].Cell = NewKey;

        /* Check if this is a hash leaf */
        if( FastLeaf->Signature == CM_KEY_HASH_LEAF )
        {
            /* Set our hash key */
            FastLeaf->List[i].HashKey = CmpComputeHashKey(0, Name, FALSE);
        }
        else
        {
            /* First, clear the name */
            FastLeaf->List[i].NameHint[0] = 0;
            FastLeaf->List[i].NameHint[1] = 0;
            FastLeaf->List[i].NameHint[2] = 0;
            FastLeaf->List[i].NameHint[3] = 0;

            /* Now, figure out if we can fit */
            if (Name->Length / sizeof(WCHAR) < 4)
            {
                /* We can fit, use our length */
                j = Name->Length / sizeof(WCHAR);
            }
            else
            {
                /* We can't, use a maximum of 4 */
                j = 4;
            }

            /* Now fill out the name hint */
            do
            {
                /* Look for invalid characters and break out if we found one */
                if ((USHORT)Name->Buffer[j - 1] > (UCHAR)-1) break;

                /* Otherwise, copy the a character */
                FastLeaf->List[i].NameHint[j - 1] = (UCHAR)Name->Buffer[j - 1];
            } while (--j > 0);
        }
    }
    else
    {
        /* This is an old-style leaf, just set our index directly */
        Leaf->List[i] = NewKey;
    }

    /* Update the leaf count and return the new cell */
    Leaf->Count += 1;
    return NewCell;
}

HCELL_INDEX
NTAPI
CmpSplitLeaf(IN PHHIVE Hive,
             IN HCELL_INDEX RootCell,
             IN ULONG RootSelect,
             IN HSTORAGE_TYPE Type)
{
    PCM_KEY_INDEX IndexKey, LeafKey, NewKey;
    PCM_KEY_FAST_INDEX FastLeaf;
    HCELL_INDEX LeafCell, NewCell;
    USHORT FirstHalf, LastHalf;
    ULONG EntrySize, TotalSize;

    /* Get the cell */
    IndexKey = (PCM_KEY_INDEX)HvGetCell(Hive, RootCell);

    /* Check if we've got valid IndexKey */
    if (!IndexKey) return HCELL_NIL;

    /* Get the leaf cell and key */
    LeafCell = IndexKey->List[RootSelect];
    LeafKey = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

    /* Check if we've got valid LeafKey */
    if (!LeafKey) return HCELL_NIL;

    /* We are going to divide this leaf into two halves */
    FirstHalf = (LeafKey->Count / 2);
    LastHalf = LeafKey->Count - FirstHalf;

    /* Now check what kind of hive we're dealing with,
     * and compute entry size
     */
    if (Hive->Version >= 5)
    {
        /* XP Hive: Use hash leaf */
        ASSERT(LeafKey->Signature == CM_KEY_HASH_LEAF);
        EntrySize = sizeof(CM_INDEX);
    }
    else
    {
        /* Use index leaf */
        ASSERT(LeafKey->Signature == CM_KEY_INDEX_LEAF);
        EntrySize = sizeof(HCELL_INDEX);
    }

    /* Compute the total size */
    TotalSize = (EntrySize * LastHalf) + FIELD_OFFSET(CM_KEY_INDEX, List) + 1;

    /* Mark the leaf cell dirty */
    HvMarkCellDirty(Hive, LeafCell, FALSE);

    /* Make sure its type is the same */
    ASSERT(HvGetCellType(LeafCell) == Type);

    /* Allocate the cell, fail in case of error */
    NewCell = HvAllocateCell(Hive, TotalSize, Type, LeafCell);
    if (NewCell == HCELL_NIL) return NewCell;

    /* Get the key */
    NewKey = (PCM_KEY_INDEX)HvGetCell(Hive, NewCell);
    if (!NewKey)
    {
        /* Free the cell and exit - should not happen! */
        ASSERT(FALSE);
        HvFreeCell(Hive, NewCell);
        return HCELL_NIL;
    }

    /* Release the newly created cell */
    HvReleaseCell(Hive, NewCell);

    /* Set its signature according to the version of the hive */
    if (Hive->Version >= 5)
    {
        /* XP Hive: Use hash leaf signature */
        NewKey->Signature = CM_KEY_HASH_LEAF;
    }
    else
    {
        /* Use index leaf signature */
        NewKey->Signature = CM_KEY_INDEX_LEAF;
    }

    /* Calculate the size of the free entries in the root key */
    TotalSize = HvGetCellSize(Hive, IndexKey) -
        (IndexKey->Count * sizeof(HCELL_INDEX)) -
        FIELD_OFFSET(CM_KEY_INDEX, List);

    /* Check if we're out of space */
    if (TotalSize / sizeof(HCELL_INDEX) < 1)
    {
        /* Grow the leaf by one more entry */
        TotalSize = HvGetCellSize(Hive, IndexKey) + sizeof(HCELL_INDEX);

        /* Re-allocate the root */
        RootCell = HvReallocateCell(Hive, RootCell, TotalSize);
        if (RootCell == HCELL_NIL)
        {
            /* Free the cell and exit */
            HvFreeCell(Hive, NewCell);
            return HCELL_NIL;
        }

        /* Get the leaf cell */
        IndexKey = (PCM_KEY_INDEX)HvGetCell(Hive, RootCell);
        if (!IndexKey)
        {
            /* This shouldn't happen */
            ASSERT(FALSE);
            return HCELL_NIL;
        }
    }

    /* Splitting is done, now we need to copy the contents,
     * according to the hive version
     */
    if (Hive->Version >= 5)
    {
        /* Copy the fast indexes */
        FastLeaf = (PCM_KEY_FAST_INDEX)LeafKey;
        RtlMoveMemory(&NewKey->List[0],
                      &FastLeaf->List[FirstHalf],
                      LastHalf * EntrySize);
    }
    else
    {
        /* Copy the indexes themselves */
        RtlMoveMemory(&NewKey->List[0],
                      &LeafKey->List[FirstHalf],
                      LastHalf * EntrySize);
    }

    /* Shift the data inside the root key */
    if ((RootSelect + 1) < IndexKey->Count)
    {
        RtlMoveMemory(&IndexKey->List[RootSelect + 2],
                      &IndexKey->List[RootSelect + 1],
                      (IndexKey->Count -
                      (RootSelect + 1)) * sizeof(HCELL_INDEX));
    }

    /* Make sure both old and new computed counts are valid */
    ASSERT(FirstHalf != 0);
    ASSERT(LastHalf != 0);

    /* Update the count value of old and new keys */
    LeafKey->Count = FirstHalf;
    NewKey->Count = LastHalf;

    /* Increase the count value of the root key */
    IndexKey->Count++;

    /* Set the new cell in root's list */
    IndexKey->List[RootSelect + 1] = NewCell;

    /* Return the root cell */
    return RootCell;
}

HCELL_INDEX
NTAPI
CmpSelectLeaf(IN PHHIVE Hive,
              IN PCM_KEY_NODE KeyNode,
              IN PCUNICODE_STRING Name,
              IN HSTORAGE_TYPE Type,
              IN PHCELL_INDEX *RootCell)
{
    PCM_KEY_INDEX IndexKey, LeafKey;
    PCM_KEY_FAST_INDEX FastLeaf;
    HCELL_INDEX LeafCell, CurrentCell;
    ULONG SubKeyIndex;
    LONG Result;

    /* Mark it as dirty */
    HvMarkCellDirty(Hive, KeyNode->SubKeyLists[Type], FALSE);

    /* Get the cell */
    IndexKey = (PCM_KEY_INDEX)HvGetCell(Hive, KeyNode->SubKeyLists[Type]);

    /* Check if we've got a valid key */
    if (!IndexKey)
    {
        /* Should not happen! */
        ASSERT(IndexKey != NULL);
        return HCELL_NIL;
    }

    /* Sanity check */
    ASSERT(IndexKey->Signature == CM_KEY_INDEX_ROOT);

    while (TRUE)
    {
        /* Find subkey */
        SubKeyIndex = CmpFindSubKeyInRoot(Hive, IndexKey, Name, &LeafCell);

        /* Make sure we found something valid */
        if (SubKeyIndex & INVALID_INDEX) return HCELL_NIL;

        /* Try to fit it into the LeafCell, if it was found */
        if (LeafCell != HCELL_NIL)
        {
            /* Get the leaf key */
            LeafKey = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

            /* Check for failure */
            if (!LeafKey) return HCELL_NIL;

            /* Check if it fits into this leaf and break */
            if (LeafKey->Count < CmpMaxIndexPerHblock)
            {
                /* Fill in the result and return it */
                *RootCell = &IndexKey->List[SubKeyIndex];
                return LeafCell;
            }

            /* It didn't fit, so proceed to splitting */
        }
        else
        {
            /* Get the leaf cell at the very end */
            LeafCell = IndexKey->List[SubKeyIndex];
            LeafKey = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

            /* Return an error in case of problems */
            if (!LeafKey) return HCELL_NIL;

            /* Choose the cell to search from depending on the key type */
            if ((LeafKey->Signature == CM_KEY_FAST_LEAF) ||
                (LeafKey->Signature == CM_KEY_HASH_LEAF))
            {
                FastLeaf = (PCM_KEY_FAST_INDEX)LeafKey;
                CurrentCell = FastLeaf->List[0].Cell;
            }
            else
            {
                /* Make sure it's an index leaf */
                ASSERT(LeafKey->Signature == CM_KEY_INDEX_LEAF);
                CurrentCell = LeafKey->List[0];
            }

            /* Do a name compare */
            Result = CmpDoCompareKeyName(Hive, Name, CurrentCell);

            /* Check for failure */
            if (Result == 2) return HCELL_NIL;

            /* Names can't be equal, ensure that */
            ASSERT(Result != 0);

            /* Check if it's above */
            if (Result >= 0)
            {
                /* Get the cell in the index */
                LeafCell = IndexKey->List[SubKeyIndex];
                LeafKey = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

                /* Return an error in case of problems */
                if (!LeafKey) return HCELL_NIL;

                /* Check if it fits into this leaf */
                if (LeafKey->Count < CmpMaxIndexPerHblock)
                {
                    /* Fill in the result and return the cell */
                    *RootCell = &IndexKey->List[SubKeyIndex];
                    return LeafCell;
                }

                /* No, it doesn't fit, check the next adjacent leaf */
                if ((SubKeyIndex + 1) < IndexKey->Count)
                {
                    /* Yes, there is space */
                    LeafCell = IndexKey->List[SubKeyIndex + 1];
                    LeafKey = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

                    /* Return an error in case of problems */
                    if (!LeafKey) return HCELL_NIL;

                    /* Check if it fits and break */
                    if (LeafKey->Count < CmpMaxIndexPerHblock)
                    {
                        /* Fill in the result and return the cell */
                        *RootCell = &IndexKey->List[SubKeyIndex + 1];
                        return LeafCell;
                    }
                }

                /* It didn't fit, so proceed to splitting */
            }
            else
            {
                /* No, it's below, check the subkey index */
                if (SubKeyIndex > 0)
                {
                    /* There should be space at the leaf one before that */
                    LeafCell = IndexKey->List[SubKeyIndex - 1];
                    LeafKey = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

                    /* Return an error in case of problems */
                    if (!LeafKey) return HCELL_NIL;

                    /* Check if it fits and break */
                    if (LeafKey->Count < CmpMaxIndexPerHblock)
                    {
                        /* Fill in the result and return the cell */
                        *RootCell = &IndexKey->List[SubKeyIndex - 1];
                        return LeafCell;
                    }
                }
                else
                {
                    /* Use the first leaf, if possible */
                    LeafCell = IndexKey->List[0];
                    LeafKey = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

                    /* Return an error in case of problems */
                    if (!LeafKey) return HCELL_NIL;

                    /* Check if it fits and break */
                    if (LeafKey->Count < CmpMaxIndexPerHblock)
                    {
                        /* Fill in the result and return the cell */
                        *RootCell = &IndexKey->List[0];
                        return LeafCell;
                    }
                }

                /* It didn't fit into either, so proceed to splitting */
            }
        }

        /* We need to split */
        CurrentCell = CmpSplitLeaf(Hive,
                                   KeyNode->SubKeyLists[Type],
                                   SubKeyIndex,
                                   Type);

        /* Return failure in case splitting failed */
        if (CurrentCell == HCELL_NIL) return CurrentCell;

        /* Set the SubKeyLists value with the new key */
        KeyNode->SubKeyLists[Type] = CurrentCell;

        /* Get the new cell */
        IndexKey = (PCM_KEY_INDEX)HvGetCell(Hive, KeyNode->SubKeyLists[Type]);

        /* Return in case of failure */
        if (!IndexKey) return HCELL_NIL;

        /* Make sure the new key became index root */
        ASSERT(IndexKey->Signature == CM_KEY_INDEX_ROOT);

        /* Now loop over with the new IndexKey value, which definately
         * has the space now
         */
    }

    /* Shouldn't come here */
    return HCELL_NIL;
}

BOOLEAN
NTAPI
CmpAddSubKey(IN PHHIVE Hive,
             IN HCELL_INDEX Parent,
             IN HCELL_INDEX Child)
{
    PCM_KEY_NODE KeyNode;
    PCM_KEY_INDEX Index;
    PCM_KEY_FAST_INDEX OldIndex;
    UNICODE_STRING Name;
    HCELL_INDEX IndexCell = HCELL_NIL, CellToRelease = HCELL_NIL, LeafCell;
    PHCELL_INDEX RootPointer = NULL;
    ULONG Type, i;
    BOOLEAN IsCompressed;
    PAGED_CODE();

    /* Get the key node */
    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, Child);
    if (!KeyNode)
    {
        /* Shouldn't happen */
        ASSERT(FALSE);
        return FALSE;
    }

    /* Check if the name is compressed */
    if (KeyNode->Flags & KEY_COMP_NAME)
    {
        /* Remember for later */
        IsCompressed = TRUE;

        /* Create the compressed name and allocate it */
        Name.Length = CmpCompressedNameSize(KeyNode->Name, KeyNode->NameLength);
        Name.MaximumLength = Name.Length;
        Name.Buffer = Hive->Allocate(Name.Length, TRUE, TAG_CM);
        if (!Name.Buffer)
        {
            /* Release the cell and fail */
            HvReleaseCell(Hive, Child);
            ASSERT(FALSE);
            return FALSE;
        }

        /* Copy the compressed name */
        CmpCopyCompressedName(Name.Buffer,
                              Name.MaximumLength,
                              KeyNode->Name,
                              KeyNode->NameLength);
    }
    else
    {
        /* Remember for later */
        IsCompressed = FALSE;

        /* Build the unicode string */
        Name.Length = KeyNode->NameLength;
        Name.MaximumLength = KeyNode->NameLength;
        Name.Buffer = &KeyNode->Name[0];
    }

    /* Release the cell */
    HvReleaseCell(Hive, Child);

    /* Get the parent node */
    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, Parent);
    if (!KeyNode)
    {
        /* Not handled */
        ASSERT(FALSE);
    }

    /* Find out the type of the cell, and check if this is the first subkey */
    Type = HvGetCellType(Child);
    if (!KeyNode->SubKeyCounts[Type])
    {
        /* Allocate a fast leaf */
        IndexCell = HvAllocateCell(Hive, sizeof(CM_KEY_FAST_INDEX), Type, HCELL_NIL);
        if (IndexCell == HCELL_NIL)
        {
            /* Not handled */
            ASSERT(FALSE);
        }

        /* Get the leaf cell */
        Index = (PCM_KEY_INDEX)HvGetCell(Hive, IndexCell);
        if (!Index)
        {
            /* Shouldn't happen */
            ASSERT(FALSE);
        }

        /* Now check what kind of hive we're dealing with */
        if (Hive->Version >= 5)
        {
            /* XP Hive: Use hash leaf */
            Index->Signature = CM_KEY_HASH_LEAF;
        }
        else if (Hive->Version >= 3)
        {
            /* Windows 2000 and ReactOS: Use fast leaf */
            Index->Signature = CM_KEY_FAST_LEAF;
        }
        else
        {
            /* NT 4: Use index leaf */
            Index->Signature = CM_KEY_INDEX_LEAF;
        }

        /* Setup the index list */
        Index->Count = 0;
        KeyNode->SubKeyLists[Type] = IndexCell;
    }
    else
    {
        /* We already have an index, get it */
        Index = (PCM_KEY_INDEX)HvGetCell(Hive, KeyNode->SubKeyLists[Type]);
        if (!Index)
        {
            /* Not handled */
            ASSERT(FALSE);
        }

        /* Remember to release the cell later */
        CellToRelease = KeyNode->SubKeyLists[Type];

        /* Check if this is a fast leaf that's gotten too full */
        if ((Index->Signature == CM_KEY_FAST_LEAF) &&
            (Index->Count >= CmpMaxFastIndexPerHblock))
        {
            DPRINT("Doing Fast->Slow Leaf conversion\n");

            /* Mark this cell as dirty */
            HvMarkCellDirty(Hive, CellToRelease, FALSE);

            /* Convert */
            OldIndex = (PCM_KEY_FAST_INDEX)Index;

            for (i = 0; i < OldIndex->Count; i++)
            {
                Index->List[i] = OldIndex->List[i].Cell;
            }

            /* Set the new type value */
            Index->Signature = CM_KEY_INDEX_LEAF;
        }
        else if (((Index->Signature == CM_KEY_INDEX_LEAF) ||
                  (Index->Signature == CM_KEY_HASH_LEAF)) &&
                  (Index->Count >= CmpMaxIndexPerHblock))
        {
            /* This is an old/hashed leaf that's gotten too large, root it */
            IndexCell = HvAllocateCell(Hive,
                                      sizeof(CM_KEY_INDEX) +
                                      sizeof(HCELL_INDEX),
                                      Type,
                                      HCELL_NIL);
            if (IndexCell == HCELL_NIL)
            {
                /* Not handled */
                ASSERT(FALSE);
            }

            /* Get the index cell */
            Index = (PCM_KEY_INDEX)HvGetCell(Hive, IndexCell);
            if (!Index)
            {
                /* Shouldn't happen */
                ASSERT(FALSE);
            }

            /* Mark the index as a root, and set the index cell */
            Index->Signature = CM_KEY_INDEX_ROOT;
            Index->Count = 1;
            Index->List[0] = KeyNode->SubKeyLists[Type];
            KeyNode->SubKeyLists[Type] = IndexCell;
        }
    }

    /* Now we can choose the leaf cell */
    LeafCell = KeyNode->SubKeyLists[Type];

    /* Check if we turned the index into a root */
    if (Index->Signature == CM_KEY_INDEX_ROOT)
    {
        DPRINT("Leaf->Root Index Conversion\n");

        /* Get the leaf where to add the new entry (the routine will do
         * the splitting if necessary)
         */
        LeafCell = CmpSelectLeaf(Hive, KeyNode, &Name, Type, &RootPointer);
        if (LeafCell == HCELL_NIL)
        {
            /* Not handled */
            ASSERT(FALSE);
        }
    }

    /* Add our leaf cell */
    LeafCell = CmpAddToLeaf(Hive, LeafCell, Child, &Name);
    if (LeafCell == HCELL_NIL)
    {
        /* Not handled */
        ASSERT(FALSE);
    }

    /* Update the key counts */
    KeyNode->SubKeyCounts[Type]++;

    /* Check if caller wants us to return the leaf */
    if (RootPointer)
    {
        /* Return it */
        *RootPointer = LeafCell;
    }
    else
    {
        /* Otherwise, mark it as the list index for the cell */
        KeyNode->SubKeyLists[Type] = LeafCell;
    }

    /* If the name was compressed, free our copy */
    if (IsCompressed) Hive->Free(Name.Buffer, 0);

    /* Release all our cells */
    if (IndexCell != HCELL_NIL) HvReleaseCell(Hive, IndexCell);
    if (CellToRelease != HCELL_NIL) HvReleaseCell(Hive, CellToRelease);
    HvReleaseCell(Hive, Parent);
    return TRUE;
}

BOOLEAN
NTAPI
CmpRemoveSubKey(IN PHHIVE Hive,
                IN HCELL_INDEX ParentKey,
                IN HCELL_INDEX TargetKey)
{
    PCM_KEY_NODE Node;
    UNICODE_STRING SearchName;
    BOOLEAN IsCompressed;
    WCHAR Buffer[50];
    HCELL_INDEX RootCell = HCELL_NIL, LeafCell, ChildCell;
    PCM_KEY_INDEX Root = NULL, Leaf;
    PCM_KEY_FAST_INDEX Child;
    ULONG Storage, RootIndex = INVALID_INDEX, LeafIndex;
    BOOLEAN Result = FALSE;
    HCELL_INDEX CellToRelease1 = HCELL_NIL, CellToRelease2  = HCELL_NIL;

    /* Get the target key node */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, TargetKey);
    if (!Node) return FALSE;

    /* Make sure it's dirty, then release it */
    ASSERT(HvIsCellDirty(Hive, TargetKey));
    HvReleaseCell(Hive, TargetKey);

    /* Check if the name is compressed */
    if (Node->Flags & KEY_COMP_NAME)
    {
        /* Remember for later */
        IsCompressed = TRUE;

        /* Build the search name */
        SearchName.Length = CmpCompressedNameSize(Node->Name,
                                                  Node->NameLength);
        SearchName.MaximumLength = SearchName.Length;

        /* Do we need an extra bufer? */
        if (SearchName.MaximumLength > sizeof(Buffer))
        {
            /* Allocate one */
            SearchName.Buffer = Hive->Allocate(SearchName.Length, TRUE, TAG_CM);
            if (!SearchName.Buffer) return FALSE;
        }
        else
        {
            /* Otherwise, use our local stack buffer */
            SearchName.Buffer = Buffer;
        }

        /* Copy the compressed name */
        CmpCopyCompressedName(SearchName.Buffer,
                              SearchName.MaximumLength,
                              Node->Name,
                              Node->NameLength);
    }
    else
    {
        /* It's not compressed, build the name directly from the node */
        IsCompressed = FALSE;
        SearchName.Length = Node->NameLength;
        SearchName.MaximumLength = Node->NameLength;
        SearchName.Buffer = Node->Name;
    }

    /* Now get the parent node */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, ParentKey);
    if (!Node) goto Exit;

    /* Make sure it's dirty, then release it */
    ASSERT(HvIsCellDirty(Hive, ParentKey));
    HvReleaseCell(Hive, ParentKey);

    /* Get the storage type and make sure it's not empty */
    Storage = HvGetCellType(TargetKey);
    ASSERT(Node->SubKeyCounts[Storage] != 0);
    //ASSERT(HvIsCellAllocated(Hive, Node->SubKeyLists[Storage]));

    /* Get the leaf cell now */
    LeafCell = Node->SubKeyLists[Storage];
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    if (!Leaf) goto Exit;

    /* Remember to release it later */
    CellToRelease1 = LeafCell;

    /* Check if the leaf is a root */
    if (Leaf->Signature == CM_KEY_INDEX_ROOT)
    {
        /* Find the child inside the root */
        RootIndex = CmpFindSubKeyInRoot(Hive, Leaf, &SearchName, &ChildCell);
        if (RootIndex & INVALID_INDEX) goto Exit;
        ASSERT(ChildCell != FALSE);

        /* The root cell is now this leaf */
        Root = Leaf;
        RootCell = LeafCell;

        /* And the new leaf is now this child */
        LeafCell = ChildCell;
        Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
        if (!Leaf) goto Exit;

        /* Remember to release it later */
        CellToRelease2 = LeafCell;
    }

    /* Make sure the leaf is valid */
    ASSERT((Leaf->Signature == CM_KEY_INDEX_LEAF) ||
           (Leaf->Signature == CM_KEY_FAST_LEAF) ||
           (Leaf->Signature == CM_KEY_HASH_LEAF));

    /* Now get the child in the leaf */
    LeafIndex = CmpFindSubKeyInLeaf(Hive, Leaf, &SearchName, &ChildCell);
    if (LeafIndex & INVALID_INDEX) goto Exit;
    ASSERT(ChildCell != HCELL_NIL);

    /* Decrement key counts and check if this was the last leaf entry */
    Node->SubKeyCounts[Storage]--;
    if (!(--Leaf->Count))
    {
        /* Free the leaf */
        HvFreeCell(Hive, LeafCell);

        /* Check if we were inside a root */
        if (Root)
        {
            /* Decrease the root count too, since the leaf is going away */
            if (!(--Root->Count))
            {
                /* The root is gone too,n ow */
                HvFreeCell(Hive, RootCell);
                Node->SubKeyLists[Storage] = HCELL_NIL;
            }
            else if (RootIndex < Root->Count)
            {
                /* Bring everything up by one */
                RtlMoveMemory(&Root->List[RootIndex],
                              &Root->List[RootIndex + 1],
                              (Root->Count - RootIndex) * sizeof(HCELL_INDEX));
            }
        }
        else
        {
            /* Otherwise, just clear the cell */
            Node->SubKeyLists[Storage] = HCELL_NIL;
        }
    }
    else if (LeafIndex < Leaf->Count)
    {
        /* Was the leaf a normal index? */
        if (Leaf->Signature == CM_KEY_INDEX_LEAF)
        {
            /* Bring everything up by one */
            RtlMoveMemory(&Leaf->List[LeafIndex],
                          &Leaf->List[LeafIndex + 1],
                          (Leaf->Count - LeafIndex) * sizeof(HCELL_INDEX));
        }
        else
        {
            /* This is a fast index, bring everything up by one */
            Child = (PCM_KEY_FAST_INDEX)Leaf;
            RtlMoveMemory(&Child->List[LeafIndex],
                          &Child->List[LeafIndex+1],
                          (Child->Count - LeafIndex) * sizeof(CM_INDEX));
        }
    }

    /* If we got here, now we're done */
    Result = TRUE;

Exit:
    /* Release any cells we may have been holding */
    if (CellToRelease1 != HCELL_NIL) HvReleaseCell(Hive, CellToRelease1);
    if (CellToRelease2 != HCELL_NIL) HvReleaseCell(Hive, CellToRelease2);

    /* Check if the name was compressed and not inside our local buffer */
    if ((IsCompressed) && (SearchName.MaximumLength > sizeof(Buffer)))
    {
        /* Free the buffer we allocated */
        Hive->Free(SearchName.Buffer, 0);
    }

    /* Return the result */
    return Result;
}
