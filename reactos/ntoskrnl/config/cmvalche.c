/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmvalche.c
 * PURPOSE:         Configuration Manager - Value Cell Cache
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

FORCEINLINE
BOOLEAN
CmpIsValueCached(IN HCELL_INDEX CellIndex)
{
    /* Make sure that the cell is valid in the first place */
    if (CellIndex == HCELL_NIL) return FALSE;

    /*Is this cell actually a pointer to the cached value data? */
    if (CellIndex & 1) return TRUE;

    /* This is a regular cell */
    return FALSE;
}

FORCEINLINE
VOID
CmpSetValueCached(IN PHCELL_INDEX CellIndex)
{
    /* Set the cached bit */
    *CellIndex |= 1;
}

#define ASSERT_VALUE_CACHE() \
    ASSERTMSG("Cached Values Not Yet Supported!", FALSE);

/* FUNCTIONS *****************************************************************/

VALUE_SEARCH_RETURN_TYPE
NTAPI
CmpGetValueListFromCache(IN PCM_KEY_CONTROL_BLOCK Kcb,
                         OUT PCELL_DATA *CellData,
                         OUT BOOLEAN *IndexIsCached,
                         OUT PHCELL_INDEX ValueListToRelease)
{
    PHHIVE Hive;
    PCACHED_CHILD_LIST ChildList;
    HCELL_INDEX CellToRelease;
    PCM_KEY_NODE KeyNode;

    /* Set defaults */
    *ValueListToRelease = HCELL_NIL;
    *IndexIsCached = FALSE;

    /* Get the hive */
    Hive = Kcb->KeyHive;
    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, Kcb->KeyCell);

    /* Get the child value cache */
    //ChildList = &Kcb->ValueCache;
    ChildList = (PCACHED_CHILD_LIST)&KeyNode->ValueList;

    /* Check if the value is cached */
    if (CmpIsValueCached(ChildList->ValueList))
    {
        /* It is: we don't expect this yet! */
        ASSERT_VALUE_CACHE();
        *IndexIsCached = TRUE;
        *CellData = NULL;
    }
    else
    {
        /* Select the value list as our cell, and get the actual list array */
        CellToRelease = ChildList->ValueList;
        *CellData = (PCELL_DATA)HvGetCell(Hive, CellToRelease);
        if (!(*CellData)) return SearchFail;

        /* Return the cell to be released */
        *ValueListToRelease = CellToRelease;
    }

    /* If we got here, then the value list was found */
    return SearchSuccess;
}

VALUE_SEARCH_RETURN_TYPE
NTAPI
CmpGetValueKeyFromCache(IN PCM_KEY_CONTROL_BLOCK Kcb,
                        IN PCELL_DATA CellData,
                        IN ULONG Index,
                        OUT PCM_CACHED_VALUE **CachedValue,
                        OUT PCM_KEY_VALUE *Value,
                        IN BOOLEAN IndexIsCached,
                        OUT BOOLEAN *ValueIsCached,
                        OUT PHCELL_INDEX CellToRelease)
{
    PHHIVE Hive;
    PCM_KEY_VALUE KeyValue;
    HCELL_INDEX Cell;

    /* Set defaults */
    *CellToRelease = HCELL_NIL;
    *Value = NULL;
    *ValueIsCached = FALSE;

    /* Get the hive */
    Hive = Kcb->KeyHive;

    /* Check if the index was cached */
    if (IndexIsCached)
    {
        /* Not expected yet! */
        ASSERT_VALUE_CACHE();
        *ValueIsCached = TRUE;
    }
    else
    {
        /* Get the cell index and the key value associated to it */
        Cell = CellData->u.KeyList[Index];
        KeyValue = (PCM_KEY_VALUE)HvGetCell(Hive, Cell);
        if (!KeyValue) return SearchFail;

        /* Return the cell and the actual key value */
        *CellToRelease = Cell;
        *Value = KeyValue;
    }

    /* If we got here, then we found the key value */
    return SearchSuccess;
}

VALUE_SEARCH_RETURN_TYPE
NTAPI
CmpGetValueDataFromCache(IN PCM_KEY_CONTROL_BLOCK Kcb,
                         IN PCM_CACHED_VALUE *CachedValue,
                         IN PCELL_DATA ValueKey,
                         IN BOOLEAN ValueIsCached,
                         OUT PVOID *DataPointer,
                         OUT PBOOLEAN Allocated,
                         OUT PHCELL_INDEX CellToRelease)
{
    PHHIVE Hive;
    ULONG Length;

    /* Sanity checks */
    ASSERT(MAXIMUM_CACHED_DATA < CM_KEY_VALUE_BIG);
    ASSERT((ValueKey->u.KeyValue.DataLength & CM_KEY_VALUE_SPECIAL_SIZE) == 0);

    /* Set defaults */
    *DataPointer = NULL;
    *Allocated = FALSE;
    *CellToRelease = HCELL_NIL;

    /* Get the hive */
    Hive = Kcb->KeyHive;

    /* Check it the value is cached */
    if (ValueIsCached)
    {
        /* This isn't expected! */
        ASSERT_VALUE_CACHE();
    }
    else
    {
        /* It's not, get the value data using the typical routine */
        if (!CmpGetValueData(Hive,
                             &ValueKey->u.KeyValue,
                             &Length,
                             DataPointer,
                             Allocated,
                             CellToRelease))
        {
            /* Nothing found: make sure no data was allocated */
            ASSERT(*Allocated == FALSE);
            ASSERT(*DataPointer == NULL);
            return SearchFail;
        }
    }

    /* We found the actual data, return success */
    return SearchSuccess;
}

VALUE_SEARCH_RETURN_TYPE
NTAPI
CmpFindValueByNameFromCache(IN PCM_KEY_CONTROL_BLOCK Kcb,
                            IN PCUNICODE_STRING Name,
                            OUT PCM_CACHED_VALUE **CachedValue,
                            OUT ULONG *Index,
                            OUT PCM_KEY_VALUE *Value,
                            OUT BOOLEAN *ValueIsCached,
                            OUT PHCELL_INDEX CellToRelease)
{
    PHHIVE Hive;
    VALUE_SEARCH_RETURN_TYPE SearchResult = SearchFail;
    LONG Result;
    UNICODE_STRING SearchName;
    PCELL_DATA CellData;
    PCACHED_CHILD_LIST ChildList;
    PCM_KEY_VALUE KeyValue;
    BOOLEAN IndexIsCached;
    ULONG i = 0;
    HCELL_INDEX Cell = HCELL_NIL;
    PCM_KEY_NODE KeyNode;

    /* Set defaults */
    *CellToRelease = HCELL_NIL;
    *Value = NULL;

    /* Get the hive */
    Hive = Kcb->KeyHive;
    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, Kcb->KeyCell);
    
    /* Get the child value cache */
    //ChildList = &Kcb->ValueCache;
    ChildList = (PCACHED_CHILD_LIST)&KeyNode->ValueList;

    /* Check if the child list has any entries */
    if (ChildList->Count != 0)
    {
        /* Get the value list associated to this child list */
        SearchResult = CmpGetValueListFromCache(Kcb,
                                                &CellData,
                                                &IndexIsCached,
                                                &Cell);
        if (SearchResult != SearchSuccess) return SearchResult;

        /* The index shouldn't be cached right now */
        if (IndexIsCached) ASSERT_VALUE_CACHE();

        /* Loop every value */
        while (TRUE)
        {
            /* Check if there's any cell to release */
            if (*CellToRelease != HCELL_NIL)
            {
                /* Release it now */
                HvReleaseCell(Hive, *CellToRelease);
                *CellToRelease = HCELL_NIL;
            }

            /* Get the key value for this index */
            SearchResult =  CmpGetValueKeyFromCache(Kcb,
                                                    CellData,
                                                    i,
                                                    CachedValue,
                                                    Value,
                                                    IndexIsCached,
                                                    ValueIsCached,
                                                    CellToRelease);
            if (SearchResult != SearchSuccess) return SearchResult;

            /* Check if the both the index and the value are cached */
            if ((IndexIsCached) && (*ValueIsCached))
            {
                /* We don't expect this yet */
                ASSERT_VALUE_CACHE();
                Result = -1;
            }
            else
            {
                /* No cache, so try to compare the name. Is it compressed? */
                KeyValue = *Value;
                if (KeyValue->Flags & VALUE_COMP_NAME)
                {
                    /* It is, do a compressed name comparison */
                    Result = CmpCompareCompressedName(Name,
                                                      KeyValue->Name,
                                                      KeyValue->NameLength);
                }
                else
                {
                    /* It's not compressed, so do a standard comparison */
                    SearchName.Length = KeyValue->NameLength;
                    SearchName.MaximumLength = SearchName.Length;
                    SearchName.Buffer = KeyValue->Name;
                    Result = RtlCompareUnicodeString(Name, &SearchName, TRUE);
                }
            }

            /* Check if we found the value data */
            if (!Result)
            {
                /* We have, return the index of the value and success */
                *Index = i;
                SearchResult = SearchSuccess;
                goto Quickie;
            }

            /* We didn't find it, try the next entry */
            if (++i == ChildList->Count)
            {
                /* The entire list was parsed, fail */
                *Value = NULL;
                SearchResult = SearchFail;
                goto Quickie;
            }
        }
    }

    /* We should only get here if the child list is empty */
    ASSERT(ChildList->Count == 0);

Quickie:
    /* Release the value list cell if required, and return search result */
    if (Cell != HCELL_NIL) HvReleaseCell(Hive, Cell);
    return SearchResult;
}

VALUE_SEARCH_RETURN_TYPE
NTAPI
CmpQueryKeyValueData(IN PCM_KEY_CONTROL_BLOCK Kcb,
                     IN PCM_CACHED_VALUE *CachedValue,
                     IN PCM_KEY_VALUE ValueKey,
                     IN BOOLEAN ValueIsCached,
                     IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
                     IN PVOID KeyValueInformation,
                     IN ULONG Length,
                     OUT PULONG ResultLength,
                     OUT PNTSTATUS Status)
{
    PHHIVE Hive;
    PKEY_VALUE_INFORMATION Info = (PKEY_VALUE_INFORMATION)KeyValueInformation;
    PCELL_DATA CellData;
    USHORT NameSize;
    ULONG Size, MinimumSize, SizeLeft, KeySize, AlignedData = 0, DataOffset;
    PVOID Buffer;
    BOOLEAN IsSmall, BufferAllocated = FALSE;
    HCELL_INDEX CellToRelease = HCELL_NIL;
    VALUE_SEARCH_RETURN_TYPE Result = SearchSuccess;

    /* Get the hive and cell data */
    Hive = Kcb->KeyHive;
    CellData = (PCELL_DATA)ValueKey;

    /* Check if the value is compressed */
    if (CellData->u.KeyValue.Flags & VALUE_COMP_NAME)
    {
        /* Get the compressed name size */
        NameSize = CmpCompressedNameSize(CellData->u.KeyValue.Name,
                                         CellData->u.KeyValue.NameLength);
    }
    else
    {
        /* Get the real size */
        NameSize = CellData->u.KeyValue.NameLength;
    }

    /* Check what kind of information the caller is requesting */
    switch (KeyValueInformationClass)
    {
        /* Basic information */
        case KeyValueBasicInformation:

            /* This is how much size we'll need */
            Size = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name) + NameSize;

            /* This is the minimum we can work with */
            MinimumSize = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name);

            /* Return the size we'd like, and assume success */
            *ResultLength = Size;
            *Status = STATUS_SUCCESS;

            /* Check if the caller gave us below our minimum */
            if (Length < MinimumSize)
            {
                /* Then we must fail */
                *Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* Fill out the basic information */
            Info->KeyValueBasicInformation.TitleIndex = 0;
            Info->KeyValueBasicInformation.Type = CellData->u.KeyValue.Type;
            Info->KeyValueBasicInformation.NameLength = NameSize;

            /* Now only the name is left */
            SizeLeft = Length - MinimumSize;
            Size = NameSize;

            /* Check if the remaining buffer is too small for the name */
            if (SizeLeft < Size)
            {
                /* Copy only as much as can fit, and tell the caller */
                Size = SizeLeft;
                *Status = STATUS_BUFFER_OVERFLOW;
            }

            /* Check if this is a compressed name */
            if (CellData->u.KeyValue.Flags & VALUE_COMP_NAME)
            {
                /* Copy as much as we can of the compressed name */
                CmpCopyCompressedName(Info->KeyValueBasicInformation.Name,
                                      Size,
                                      CellData->u.KeyValue.Name,
                                      CellData->u.KeyValue.NameLength);
            }
            else
            {
                /* Copy as much as we can of the raw name */
                RtlCopyMemory(Info->KeyValueBasicInformation.Name,
                              CellData->u.KeyValue.Name,
                              Size);
            }

            /* We're all done */
            break;

        /* Full key information */
        case KeyValueFullInformation:

            /* Check if this is a small key and compute key size */
            IsSmall = CmpIsKeyValueSmall(&KeySize,
                                         CellData->u.KeyValue.DataLength);

            /* Calculate the total size required */
            Size = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name) +
                   NameSize +
                   KeySize;

            /* And this is the least we can work with */
            MinimumSize = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name);

            /* Check if there's any key data */
            if (KeySize > 0)
            {
                /* Calculate the data offset */
                DataOffset = Size - KeySize;

                /* Align the offset to 4 bytes */
                AlignedData = ALIGN_UP(DataOffset, ULONG);

                /* If alignment was required, we'll need more space */
                if (AlignedData > DataOffset) Size += (AlignedData-DataOffset);
            }

            /* Tell the caller the size we'll finally need, and set success */
            *ResultLength = Size;
            *Status = STATUS_SUCCESS;

            /* Check if the caller is giving us too little */
            if (Length < MinimumSize)
            {
                /* Then fail right now */
                *Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* Fill out the basic information */
            Info->KeyValueFullInformation.TitleIndex = 0;
            Info->KeyValueFullInformation.Type = CellData->u.KeyValue.Type;
            Info->KeyValueFullInformation.DataLength = KeySize;
            Info->KeyValueFullInformation.NameLength = NameSize;

            /* Only the name is left now */
            SizeLeft = Length - MinimumSize;
            Size = NameSize;

            /* Check if the name fits */
            if (SizeLeft < Size)
            {
                /* It doesn't, truncate what we'll copy, and tell the caller */
                Size = SizeLeft;
                *Status = STATUS_BUFFER_OVERFLOW;
            }

            /* Check if this key value is compressed */
            if (CellData->u.KeyValue.Flags & VALUE_COMP_NAME)
            {
                /* It is, copy the compressed name */
                CmpCopyCompressedName(Info->KeyValueFullInformation.Name,
                                      Size,
                                      CellData->u.KeyValue.Name,
                                      CellData->u.KeyValue.NameLength);
            }
            else
            {
                /* It's not, copy the raw name */
                RtlCopyMemory(Info->KeyValueFullInformation.Name,
                              CellData->u.KeyValue.Name,
                              Size);
            }

            /* Now check if the key had any data */
            if (KeySize > 0)
            {
                /* Was it a small key? */
                if (IsSmall)
                {
                    /* Then the data is directly into the cell */
                    Buffer = &CellData->u.KeyValue.Data;
                }
                else
                {
                    /* Otherwise, we must retrieve it from the value cache */
                    Result = CmpGetValueDataFromCache(Kcb,
                                                      CachedValue,
                                                      CellData,
                                                      ValueIsCached,
                                                      &Buffer,
                                                      &BufferAllocated,
                                                      &CellToRelease);
                    if (Result != SearchSuccess)
                    {
                        /* We failed, nothing should be allocated */
                        ASSERT(Buffer == NULL);
                        ASSERT(BufferAllocated == FALSE);
                        *Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                /* Now that we know we truly have data, set its offset */
                Info->KeyValueFullInformation.DataOffset = AlignedData;

                /* Only the data remains to be copied */
                SizeLeft = (((LONG)Length - (LONG)AlignedData) < 0) ?
                           0 : (Length - AlignedData);
                Size = KeySize;

                /* Check if the caller has no space for it */
                if (SizeLeft < Size)
                {
                    /* Truncate what we'll copy, and tell the caller */
                    Size = SizeLeft;
                    *Status = STATUS_BUFFER_OVERFLOW;
                }

                /* Sanity check */
                ASSERT((IsSmall ? (Size <= CM_KEY_VALUE_SMALL) : TRUE));

                /* Make sure we have a valid buffer */
                if (Buffer)
                {
                    /* Copy the data into the aligned offset */
                    RtlCopyMemory((PVOID)((ULONG_PTR)Info + AlignedData),
                                  Buffer,
                                  Size);
                }
            }
            else
            {
                /* We don't have any data, set the offset to -1, not 0! */
                Info->KeyValueFullInformation.DataOffset = 0xFFFFFFFF;
            }

            /* We're done! */
            break;

        /* Partial information requested (no name or alignment!) */
        case KeyValuePartialInformation:

            /* Check if this is a small key and compute key size */
            IsSmall = CmpIsKeyValueSmall(&KeySize,
                                         CellData->u.KeyValue.DataLength);

            /* Calculate the total size required */
            Size = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + KeySize;

            /* And this is the least we can work with */
            MinimumSize = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

            /* Tell the caller the size we'll finally need, and set success */
            *ResultLength = Size;
            *Status = STATUS_SUCCESS;

            /* Check if the caller is giving us too little */
            if (Length < MinimumSize)
            {
                /* Then fail right now */
                *Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* Fill out the basic information */
            Info->KeyValuePartialInformation.TitleIndex = 0;
            Info->KeyValuePartialInformation.Type = CellData->u.KeyValue.Type;
            Info->KeyValuePartialInformation.DataLength = KeySize;

            /* Now check if the key had any data */
            if (KeySize > 0)
            {
                /* Was it a small key? */
                if (IsSmall)
                {
                    /* Then the data is directly into the cell */
                    Buffer = &CellData->u.KeyValue.Data;
                }
                else
                {
                    /* Otherwise, we must retrieve it from the value cache */
                    Result = CmpGetValueDataFromCache(Kcb,
                                                      CachedValue,
                                                      CellData,
                                                      ValueIsCached,
                                                      &Buffer,
                                                      &BufferAllocated,
                                                      &CellToRelease);
                    if (Result != SearchSuccess)
                    {
                        /* We failed, nothing should be allocated */
                        ASSERT(Buffer == NULL);
                        ASSERT(BufferAllocated == FALSE);
                        *Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                /* Only the data remains to be copied */
                SizeLeft = Length - MinimumSize;
                Size = KeySize;

                /* Check if the caller has no space for it */
                if (SizeLeft < Size)
                {
                    /* Truncate what we'll copy, and tell the caller */
                    Size = SizeLeft;
                    *Status = STATUS_BUFFER_OVERFLOW;
                }

                /* Sanity check */
                ASSERT((IsSmall ? (Size <= CM_KEY_VALUE_SMALL) : TRUE));

                /* Make sure we have a valid buffer */
                if (Buffer)
                {
                    /* Copy the data into the aligned offset */
                    RtlCopyMemory(Info->KeyValuePartialInformation.Data,
                                  Buffer,
                                  Size);
                }
            }

            /* We're done! */
            break;

        /* Other information class */
        default:

            /* We got some class that we don't support */
            *Status = STATUS_INVALID_PARAMETER;
            break;
    }

    /* Return the search result as well */
    return Result;
}
