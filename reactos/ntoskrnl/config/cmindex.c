/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmindex.c
 * PURPOSE:         Configuration Manager - Cell Indexes
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

LONG
NTAPI
CmpDoCompareKeyName(IN PHHIVE Hive,
                    IN PUNICODE_STRING SearchName,
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
                  IN PUNICODE_STRING SearchName,
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
                    IN PUNICODE_STRING SearchName,
                    IN PHCELL_INDEX SubKey)
{
    ULONG High, Low = 0, i, ReturnIndex;
    HCELL_INDEX LeafCell;
    PCM_KEY_INDEX Leaf;
    LONG Result;

    /* Verify Index for validity */
    ASSERTMSG("We don't do a linear search yet!\n", FALSE);
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
                    ReturnIndex = Low;
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
            ReturnIndex = 0x80000000;
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
        /* We got lucky...return it */
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
                    IN PUNICODE_STRING SearchName,
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
#ifdef SOMEONE_WAS_NICE_ENOUGH_TO_MAKE_OUR_CELLS_LEXICALLY_SORTED
    i = High / 2;
#else
    i = 0;
#endif

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
            return 0x80000000;
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

#ifdef SOMEONE_WAS_NICE_ENOUGH_TO_MAKE_OUR_CELLS_LEXICALLY_SORTED
        /* Check if this is the last entry, if so, break out and handle it */
        if ((High - Low) <= 1) break;

        /* Set the new index */
        i = ((High - Low) / 2) + Low;
#else
        if (++i > High)
        {
            /* Return failure */
            *SubKey = HCELL_NIL;
            return 0;
        }
#endif
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
        return 0x80000000;
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
        return 0x80000000;
    }

    /* Return the high */
    return High;
}

ULONG
NTAPI
CmpComputeHashKey(IN ULONG Hash,
                  IN PUNICODE_STRING Name,
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
CmpFindSubKeyByHash(IN PHHIVE Hive,
                    IN PCM_KEY_FAST_INDEX FastIndex,
                    IN PUNICODE_STRING SearchName)
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
                    IN PUNICODE_STRING SearchName)
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
#if 0
            /* Make sure we have one and that the cell is allocated */
            ASSERT((IndexRoot == NULL) ||
                   HvIsCellAllocated(Hive, Parent->SubKeyLists[i]));
#endif
            /* Fail if we don't actually have an index root */
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
                if (Found < 0) break;

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
                if (Found < 0) break;
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
