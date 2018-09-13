/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmsubs2.c

Abstract:

    This module various support routines for the configuration manager.

    The routines in this module are independent enough to be linked into
    any other program.  The routines in cmsubs.c are not.

Author:

    Bryan M. Willman (bryanwi) 12-Sep-1991

Revision History:

--*/

#include    "cmp.h"

PUCHAR
CmpGetValueDataFromCache(
    IN PHHIVE  Hive,
    IN PPCM_CACHED_VALUE ContainingList,
    IN PCELL_DATA ValueKey,
    IN BOOLEAN    ValueCached
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpGetValueDataFromCache)
#pragma alloc_text(PAGE,CmpFreeValue)
#pragma alloc_text(PAGE,CmpQueryKeyData)
#pragma alloc_text(PAGE,CmpQueryKeyValueData)
#endif

//
// Define alignment macro.
//

#define ALIGN_OFFSET(Offset) (ULONG) \
        ((((ULONG)(Offset) + sizeof(ULONG)-1)) & (~(sizeof(ULONG) - 1)))

#define ALIGN_OFFSET64(Offset) (ULONG) \
        ((((ULONG)(Offset) + sizeof(ULONGLONG)-1)) & (~(sizeof(ULONGLONG) - 1)))


VOID
CmpFreeValue(
    PHHIVE Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Free the value entry Hive.Cell refers to, including
    its name and data cells.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of value to delete

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    PCELL_DATA value;
    ULONG       realsize;
    BOOLEAN     small;

    //
    // map in the cell
    //
    value = HvGetCell(Hive, Cell);

    //
    // free data if present
    //
    small = CmpIsHKeyValueSmall(realsize, value->u.KeyValue.DataLength);
    if ((! small) && (realsize > 0))
    {
        HvFreeCell(Hive, value->u.KeyValue.Data);
    }

    //
    // free the cell itself
    //
    HvFreeCell(Hive, Cell);

    return;
}


//
// Data transfer workers
//

NTSTATUS
CmpQueryKeyData(
    PHHIVE Hive,
    PCM_KEY_NODE Node,
    KEY_INFORMATION_CLASS KeyInformationClass,
    PVOID KeyInformation,
    ULONG Length,
    PULONG ResultLength
    )
/*++

Routine Description:

    Do the actual copy of data for a key into caller's buffer.

    If KeyInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Node - Supplies pointer to node whose subkeys are to be found

    KeyInformationClass - Specifies the type of information returned in
        Buffer.  One of the following types:

        KeyBasicInformation - return last write time, title index, and name.
            (see KEY_BASIC_INFORMATION structure)

        KeyNodeInformation - return last write time, title index, name, class.
            (see KEY_NODE_INFORMATION structure)

    KeyInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyInformation in bytes.

    ResultLength - Number of bytes actually written into KeyInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS    status;
    PCELL_DATA  pclass;
    ULONG       requiredlength;
    LONG        leftlength;
    ULONG       offset;
    ULONG       minimumlength;
    PKEY_INFORMATION pbuffer;
    USHORT      NameLength;

    pbuffer = (PKEY_INFORMATION)KeyInformation;
    NameLength = CmpHKeyNameLen(Node);

    switch (KeyInformationClass) {

    case KeyBasicInformation:

        //
        // LastWriteTime, TitleIndex, NameLength, Name

        requiredlength = FIELD_OFFSET(KEY_BASIC_INFORMATION, Name) +
                         NameLength;

        minimumlength = FIELD_OFFSET(KEY_BASIC_INFORMATION, Name);

        *ResultLength = requiredlength;

        status = STATUS_SUCCESS;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyBasicInformation.LastWriteTime =
                Node->LastWriteTime;

            pbuffer->KeyBasicInformation.TitleIndex = 0;

            pbuffer->KeyBasicInformation.NameLength =
                NameLength;

            leftlength = Length - minimumlength;

            requiredlength = NameLength;

            if (leftlength < (LONG)requiredlength) {
                requiredlength = leftlength;
                status = STATUS_BUFFER_OVERFLOW;
            }

            if (Node->Flags & KEY_COMP_NAME) {
                CmpCopyCompressedName(pbuffer->KeyBasicInformation.Name,
                                      leftlength,
                                      Node->Name,
                                      Node->NameLength);
            } else {
                RtlCopyMemory(
                    &(pbuffer->KeyBasicInformation.Name[0]),
                    &(Node->Name[0]),
                    requiredlength
                    );
            }
        }

        break;


    case KeyNodeInformation:

        //
        // LastWriteTime, TitleIndex, ClassOffset, ClassLength
        // NameLength, Name, Class
        //
        requiredlength = FIELD_OFFSET(KEY_NODE_INFORMATION, Name) +
                         NameLength +
                         Node->ClassLength;

        minimumlength = FIELD_OFFSET(KEY_NODE_INFORMATION, Name);

        *ResultLength = requiredlength;

        status = STATUS_SUCCESS;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyNodeInformation.LastWriteTime =
                Node->LastWriteTime;

            pbuffer->KeyNodeInformation.TitleIndex = 0;

            pbuffer->KeyNodeInformation.ClassLength =
                Node->ClassLength;

            pbuffer->KeyNodeInformation.NameLength =
                NameLength;

            leftlength = Length - minimumlength;
            requiredlength = NameLength;

            if (leftlength < (LONG)requiredlength) {
                requiredlength = leftlength;
                status = STATUS_BUFFER_OVERFLOW;
            }

            if (Node->Flags & KEY_COMP_NAME) {
                CmpCopyCompressedName(pbuffer->KeyNodeInformation.Name,
                                      leftlength,
                                      Node->Name,
                                      Node->NameLength);
            } else {
                RtlCopyMemory(
                    &(pbuffer->KeyNodeInformation.Name[0]),
                    &(Node->Name[0]),
                    requiredlength
                    );
            }

            if (Node->ClassLength > 0) {

                offset = FIELD_OFFSET(KEY_NODE_INFORMATION, Name) +
                            NameLength;
                offset = ALIGN_OFFSET(offset);

                pbuffer->KeyNodeInformation.ClassOffset = offset;

                pclass = HvGetCell(Hive, Node->Class);

                pbuffer = (PKEY_INFORMATION)((PUCHAR)pbuffer + offset);

                leftlength = (((LONG)Length - (LONG)offset) < 0) ?
                                    0 :
                                    Length - offset;

                requiredlength = Node->ClassLength;

                if (leftlength < (LONG)requiredlength) {
                    requiredlength = leftlength;
                    status = STATUS_BUFFER_OVERFLOW;
                }

                RtlMoveMemory(
                    pbuffer,
                    pclass,
                    requiredlength
                    );

            } else {
                pbuffer->KeyNodeInformation.ClassOffset = (ULONG)-1;
            }
        }

        break;


    case KeyFullInformation:

        //
        // LastWriteTime, TitleIndex, ClassOffset, ClassLength,
        // SubKeys, MaxNameLen, MaxClassLen, Values, MaxValueNameLen,
        // MaxValueDataLen, Class
        //
        requiredlength = FIELD_OFFSET(KEY_FULL_INFORMATION, Class) +
                         Node->ClassLength;

        minimumlength = FIELD_OFFSET(KEY_FULL_INFORMATION, Class);

        *ResultLength = requiredlength;

        status = STATUS_SUCCESS;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyFullInformation.LastWriteTime =
                Node->LastWriteTime;

            pbuffer->KeyFullInformation.TitleIndex = 0;

            pbuffer->KeyFullInformation.ClassLength =
                Node->ClassLength;

            if (Node->ClassLength > 0) {

                pbuffer->KeyFullInformation.ClassOffset =
                        FIELD_OFFSET(KEY_FULL_INFORMATION, Class);

                pclass = HvGetCell(Hive, Node->Class);

                leftlength = Length - minimumlength;
                requiredlength = Node->ClassLength;

                if (leftlength < (LONG)requiredlength) {
                    requiredlength = leftlength;
                    status = STATUS_BUFFER_OVERFLOW;
                }

                RtlMoveMemory(
                    &(pbuffer->KeyFullInformation.Class[0]),
                    pclass,
                    requiredlength
                    );

            } else {
                pbuffer->KeyFullInformation.ClassOffset = (ULONG)-1;
            }

            pbuffer->KeyFullInformation.SubKeys =
                Node->SubKeyCounts[Stable] +
                Node->SubKeyCounts[Volatile];

            pbuffer->KeyFullInformation.Values =
                Node->ValueList.Count;

            pbuffer->KeyFullInformation.MaxNameLen =
                Node->MaxNameLen;

            pbuffer->KeyFullInformation.MaxClassLen =
                Node->MaxClassLen;

            pbuffer->KeyFullInformation.MaxValueNameLen =
                Node->MaxValueNameLen;

            pbuffer->KeyFullInformation.MaxValueDataLen =
                Node->MaxValueDataLen;

        }

        break;


    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }
    return status;
}

PUCHAR
CmpGetValueDataFromCache(
    IN PHHIVE  Hive,
    IN PPCM_CACHED_VALUE ContainingList,
    IN PCELL_DATA ValueKey,
    IN BOOLEAN    ValueCached
)
/*++

Routine Description:

    Get the cached Value Data given a value node.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    ContainingList - Address that stores the allocation address of the value node.
                     We need to update this when we do a re-allocate to cache
                     both value key and value data.

    ValueKey - pointer to the Value Key

    ValueCached - Indicating whether Value key is cached or not.

Return Value:

    Pointer to the Valve Index.
--*/
{
    //
    // Cache the data if needed.
    //
    PCM_CACHED_VALUE OldEntry;
    PCM_CACHED_VALUE NewEntry;
    PUCHAR      datapointer;
    PUCHAR      Cacheddatapointer;
    ULONG       AllocSize;
    ULONG       CopySize;
    ULONG       DataSize;

    if (ValueCached) {
        OldEntry = (PCM_CACHED_VALUE) CMP_GET_CACHED_ADDRESS(*ContainingList);
        if (OldEntry->DataCacheType == CM_CACHE_DATA_CACHED) {
            //
            // Data is already cached, use it.
            //
            datapointer = (PUCHAR) ((ULONG_PTR) ValueKey + OldEntry->ValueKeySize);
        } else if (OldEntry->DataCacheType == CM_CACHE_DATA_TOO_BIG) {
            //
            // Data is too big to warrent caching, get it from the registry.
            //
            datapointer = (PUCHAR)HvGetCell(Hive, ValueKey->u.KeyValue.Data);
        } else {
            //
            // consistency check
            //
            ASSERT(OldEntry->DataCacheType == CM_CACHE_DATA_NOT_CACHED);

            //
            // Value data is not cached.
            // Check the size of value data, if it is smaller than MAXIMUM_CACHED_DATA, cache it.
            //
            datapointer = (PUCHAR)HvGetCell(Hive, ValueKey->u.KeyValue.Data);
            DataSize = (ULONG) HvGetCellSize(Hive, datapointer);
            if (DataSize <= MAXIMUM_CACHED_DATA) {
                //
                // Data is not cached and now we are going to do it.
                // Reallocate a new cached entry for both value key and value data.
                //
                CopySize = DataSize + OldEntry->ValueKeySize;
                AllocSize = CopySize + FIELD_OFFSET(CM_CACHED_VALUE, KeyValue);

                // Dragos: Changed to catch the memory violator
                // it didn't work
                //NewEntry = (PCM_CACHED_VALUE) ExAllocatePoolWithTagPriority(PagedPool, AllocSize, CM_CACHE_VALUE_DATA_TAG,NormalPoolPrioritySpecialPoolUnderrun);
                NewEntry = (PCM_CACHED_VALUE) ExAllocatePoolWithTag(PagedPool, AllocSize, CM_CACHE_VALUE_DATA_TAG);

                if (NewEntry) {
                    //
                    // Now fill the data to the new cached entry
                    //
                    NewEntry->DataCacheType = CM_CACHE_DATA_CACHED;
                    NewEntry->ValueKeySize = OldEntry->ValueKeySize;

                    RtlCopyMemory((PVOID)&(NewEntry->KeyValue),
                                  (PVOID)&(OldEntry->KeyValue),
                                  NewEntry->ValueKeySize);

                    Cacheddatapointer = (PUCHAR) ((ULONG_PTR) &(NewEntry->KeyValue) + OldEntry->ValueKeySize);
                    RtlCopyMemory(Cacheddatapointer, datapointer, DataSize);

                    // Trying to catch the BAD guy who writes over our pool.
                    CmpMakeSpecialPoolReadWrite( OldEntry );

                    *ContainingList = (PCM_CACHED_VALUE) CMP_MARK_CELL_CACHED(NewEntry);

                    // Trying to catch the BAD guy who writes over our pool.
                    CmpMakeSpecialPoolReadOnly( NewEntry );

                    //
                    // Free the old entry
                    //

                    ExFreePool(OldEntry);
                }
            } else {
                // Trying to catch the BAD guy who writes over our pool.
                CmpMakeSpecialPoolReadWrite( OldEntry );

                //
                // Mark the type and do not cache it.
                //
                OldEntry->DataCacheType = CM_CACHE_DATA_TOO_BIG;

                // Trying to catch the BAD guy who writes over our pool.
                CmpMakeSpecialPoolReadOnly( OldEntry );
            }
        }
    } else {
        datapointer = (PUCHAR)HvGetCell(Hive, ValueKey->u.KeyValue.Data);
    }

    return (datapointer);
}



NTSTATUS
CmpQueryKeyValueData(
    PHHIVE Hive,
    PPCM_CACHED_VALUE ContainingList,
    PCM_KEY_VALUE ValueKey,
    BOOLEAN     ValueCached,
    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    PVOID KeyValueInformation,
    ULONG Length,
    PULONG ResultLength
    )
/*++

Routine Description:

    Do the actual copy of data for a key value into caller's buffer.

    If KeyValueInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of node to whose sub keys are to be found

    KeyValueInformationClass - Specifies the type of information returned in
        KeyValueInformation.  One of the following types:

    KeyValueInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyInformation in bytes.

    ResultLength - Number of bytes actually written into KeyInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS    status;
    PKEY_VALUE_INFORMATION pbuffer;
    PCELL_DATA  pcell;
    LONG        leftlength;
    ULONG       requiredlength;
    ULONG       minimumlength;
    ULONG       offset;
    ULONG       base;
    ULONG       realsize;
    PUCHAR      datapointer;
    BOOLEAN     small;
    USHORT      NameLength;

    pbuffer = (PKEY_VALUE_INFORMATION)KeyValueInformation;

    pcell = (PCELL_DATA) ValueKey;
    NameLength = CmpValueNameLen(&pcell->u.KeyValue);

    switch (KeyValueInformationClass) {

    case KeyValueBasicInformation:

        //
        // TitleIndex, Type, NameLength, Name
        //
        requiredlength = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name) +
                         NameLength;

        minimumlength = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name);

        *ResultLength = requiredlength;

        status = STATUS_SUCCESS;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyValueBasicInformation.TitleIndex = 0;

            pbuffer->KeyValueBasicInformation.Type =
                pcell->u.KeyValue.Type;

            pbuffer->KeyValueBasicInformation.NameLength =
                NameLength;

            leftlength = Length - minimumlength;
            requiredlength = NameLength;

            if (leftlength < (LONG)requiredlength) {
                requiredlength = leftlength;
                status = STATUS_BUFFER_OVERFLOW;
            }

            if (pcell->u.KeyValue.Flags & VALUE_COMP_NAME) {
                CmpCopyCompressedName(pbuffer->KeyValueBasicInformation.Name,
                                      requiredlength,
                                      pcell->u.KeyValue.Name,
                                      pcell->u.KeyValue.NameLength);
            } else {
                RtlCopyMemory(&(pbuffer->KeyValueBasicInformation.Name[0]),
                              &(pcell->u.KeyValue.Name[0]),
                              requiredlength);
            }
        }

        break;



    case KeyValueFullInformation:
    case KeyValueFullInformationAlign64:

        //
        // TitleIndex, Type, DataOffset, DataLength, NameLength,
        // Name, Data
        //
        small = CmpIsHKeyValueSmall(realsize, pcell->u.KeyValue.DataLength);

        requiredlength = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name) +
                         NameLength +
                         realsize;

        minimumlength = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name);
        if (realsize > 0) {
            base = requiredlength - realsize;

#if defined(_WIN64)

            offset = ALIGN_OFFSET64(base);

#else

            if (KeyValueInformationClass == KeyValueFullInformationAlign64) {
                offset = ALIGN_OFFSET64(base);

            } else {
                offset = ALIGN_OFFSET(base);
            }

#endif

            if (offset > base) {
                requiredlength += (offset - base);
            }

#if DBG && defined(_WIN64)

            //
            // Some clients will have passed in a structure that they "know"
            // will be exactly the right size.  The fact that alignment
            // has changed on NT64 may cause these clients to have problems.
            //
            // The solution is to fix the client, but print out some debug
            // spew here if it looks like this is the case.  This problem
            // isn't particularly easy to spot from the client end.
            //

            if((KeyValueInformationClass == KeyValueFullInformation) &&
                (Length != minimumlength) &&
                (requiredlength > Length) &&
                ((requiredlength - Length) <=
                                (ALIGN_OFFSET64(base) - ALIGN_OFFSET(base)))) {

                KdPrint(("ntos/config-64 KeyValueFullInformation: "
                         "Possible client buffer size problem.\n"));

                KdPrint(("    Required size = %d\n", requiredlength));
                KdPrint(("    Supplied size = %d\n", Length));
            }

#endif

        }

        *ResultLength = requiredlength;

        status = STATUS_SUCCESS;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyValueFullInformation.TitleIndex = 0;

            pbuffer->KeyValueFullInformation.Type =
                pcell->u.KeyValue.Type;

            pbuffer->KeyValueFullInformation.DataLength =
                realsize;

            pbuffer->KeyValueFullInformation.NameLength =
                NameLength;

            leftlength = Length - minimumlength;
            requiredlength = NameLength;

            if (leftlength < (LONG)requiredlength) {
                requiredlength = leftlength;
                status = STATUS_BUFFER_OVERFLOW;
            }

            if (pcell->u.KeyValue.Flags & VALUE_COMP_NAME) {
                CmpCopyCompressedName(pbuffer->KeyValueFullInformation.Name,
                                      requiredlength,
                                      pcell->u.KeyValue.Name,
                                      pcell->u.KeyValue.NameLength);
            } else {
                RtlMoveMemory(
                    &(pbuffer->KeyValueFullInformation.Name[0]),
                    &(pcell->u.KeyValue.Name[0]),
                    requiredlength
                    );
            }

            if (realsize > 0) {

                if (small == TRUE) {
                    datapointer = (PUCHAR)(&(pcell->u.KeyValue.Data));
                } else {
                    datapointer = CmpGetValueDataFromCache(Hive, ContainingList, pcell, ValueCached);
                }

                pbuffer->KeyValueFullInformation.DataOffset = offset;

                leftlength = (((LONG)Length - (LONG)offset) < 0) ?
                                    0 :
                                    Length - offset;

                requiredlength = realsize;

                if (leftlength < (LONG)requiredlength) {
                    requiredlength = leftlength;
                    status = STATUS_BUFFER_OVERFLOW;
                }

                ASSERT((small ? (requiredlength <= CM_KEY_VALUE_SMALL) : TRUE));

                RtlMoveMemory(
                    ((PUCHAR)pbuffer + offset),
                    datapointer,
                    requiredlength
                    );

            } else {
                pbuffer->KeyValueFullInformation.DataOffset = (ULONG)-1;
            }
        }

        break;


    case KeyValuePartialInformation:

        //
        // TitleIndex, Type, DataLength, Data
        //
        small = CmpIsHKeyValueSmall(realsize, pcell->u.KeyValue.DataLength);
        requiredlength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) +
                         realsize;

        minimumlength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

        *ResultLength = requiredlength;

        status = STATUS_SUCCESS;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyValuePartialInformation.TitleIndex = 0;

            pbuffer->KeyValuePartialInformation.Type =
                pcell->u.KeyValue.Type;

            pbuffer->KeyValuePartialInformation.DataLength =
                realsize;

            leftlength = Length - minimumlength;
            requiredlength = realsize;

            if (leftlength < (LONG)requiredlength) {
                requiredlength = leftlength;
                status = STATUS_BUFFER_OVERFLOW;
            }

            if (realsize > 0) {

                if (small == TRUE) {
                    datapointer = (PUCHAR)(&(pcell->u.KeyValue.Data));
                } else {
                    datapointer = CmpGetValueDataFromCache(Hive, ContainingList, pcell, ValueCached);
                }

                ASSERT((small ? (requiredlength <= CM_KEY_VALUE_SMALL) : TRUE));

                RtlMoveMemory((PUCHAR)&(pbuffer->KeyValuePartialInformation.Data[0]),
                              datapointer,
                              requiredlength);
            }
        }

        break;
    case KeyValuePartialInformationAlign64:

        //
        // TitleIndex, Type, DataLength, Data
        //
        small = CmpIsHKeyValueSmall(realsize, pcell->u.KeyValue.DataLength);
        requiredlength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data) +
                         realsize;

        minimumlength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data);

        *ResultLength = requiredlength;

        status = STATUS_SUCCESS;

        if (Length < minimumlength) {

            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            pbuffer->KeyValuePartialInformationAlign64.Type =
                pcell->u.KeyValue.Type;

            pbuffer->KeyValuePartialInformationAlign64.DataLength =
                realsize;

            leftlength = Length - minimumlength;
            requiredlength = realsize;

            if (leftlength < (LONG)requiredlength) {
                requiredlength = leftlength;
                status = STATUS_BUFFER_OVERFLOW;
            }

            if (realsize > 0) {

                if (small == TRUE) {
                    datapointer = (PUCHAR)(&(pcell->u.KeyValue.Data));
                } else {
                    datapointer = CmpGetValueDataFromCache(Hive, ContainingList, pcell, ValueCached);
                }

                ASSERT((small ? (requiredlength <= CM_KEY_VALUE_SMALL) : TRUE));
                RtlMoveMemory((PUCHAR)&(pbuffer->KeyValuePartialInformationAlign64.Data[0]),
                              datapointer,
                              requiredlength);
            }
        }

        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }
    return status;
}
