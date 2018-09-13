/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmapi.c

Abstract:

    This module contains CM level entry points for the registry.

Author:

    Bryan M. Willman (bryanwi) 30-Aug-1991

Revision History:

--*/

#include "cmp.h"



extern  BOOLEAN     CmpNoWrite;

extern  LIST_ENTRY  CmpHiveListHead;

extern  BOOLEAN CmpProfileLoaded;
extern  BOOLEAN CmpWasSetupBoot;

extern  PUCHAR  CmpStashBuffer;
extern  ULONG   CmpStashBufferSize;

extern  UNICODE_STRING CmSymbolicLinkValueName;

extern ULONG   CmpGlobalQuotaAllowed;
extern ULONG   CmpGlobalQuotaWarning;

//
// procedures private to this file
//
NTSTATUS
CmpSetValueKeyExisting(
    IN PHHIVE  Hive,
    IN HCELL_INDEX OldChild,
    IN PCELL_DATA Value,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize,
    IN ULONG StorageType,
    IN ULONG TempData
    );


NTSTATUS
CmpSetValueKeyNew(
    IN PHHIVE  Hive,
    IN PCM_KEY_NODE Parent,
    IN PUNICODE_STRING ValueName,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize,
    IN ULONG StorageType,
    IN ULONG TempData
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmDeleteValueKey)
#pragma alloc_text(PAGE,CmEnumerateKey)
#pragma alloc_text(PAGE,CmEnumerateValueKey)
#pragma alloc_text(PAGE,CmFlushKey)
#pragma alloc_text(PAGE,CmQueryKey)
#pragma alloc_text(PAGE,CmQueryValueKey)
#pragma alloc_text(PAGE,CmQueryMultipleValueKey)
#pragma alloc_text(PAGE,CmSetValueKey)
#pragma alloc_text(PAGE,CmpSetValueKeyExisting)
#pragma alloc_text(PAGE,CmpSetValueKeyNew)
#pragma alloc_text(PAGE,CmSetLastWriteTimeKey)
#pragma alloc_text(PAGE,CmLoadKey)
#pragma alloc_text(PAGE,CmUnloadKey)
#pragma alloc_text(PAGE,CmpDoFlushAll)
#pragma alloc_text(PAGE,CmReplaceKey)

#ifdef _WRITE_PROTECTED_REGISTRY_POOL
#pragma alloc_text(PAGE,CmpMarkAllBinsReadOnly)
#endif

#endif

NTSTATUS
CmDeleteValueKey(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock,
    IN UNICODE_STRING ValueName         // RAW
    )
/*++

Routine Description:

    One of the value entries of a registry key may be removed with this call.

    The value entry with ValueName matching ValueName is removed from the key.
    If no such entry exists, an error is returned.

Arguments:

    KeyControlBlock - pointer to kcb for key to operate on

    ValueName - The name of the value to be deleted.  NULL is a legal name.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS status;
    PCM_KEY_NODE pcell;
    PCHILD_LIST plist;
    PCELL_DATA targetaddress;
    ULONG  targetindex;
    ULONG   newcount;
    HCELL_INDEX newcell;
    HCELL_INDEX ChildCell;
    PHHIVE      Hive;
    HCELL_INDEX Cell;
    ULONG realsize;
    LARGE_INTEGER systemtime;

    CMLOG(CML_WORKER, CMS_CM) KdPrint(("CmDeleteValueKey\n"));

    CmpLockRegistryExclusive();

    try {
        //
        // no edits, not even this one, on keys marked for deletion
        //
        if (KeyControlBlock->Delete) {
            return STATUS_KEY_DELETED;
        }

        Hive = KeyControlBlock->KeyHive;
        Cell = KeyControlBlock->KeyCell;
        pcell = KeyControlBlock->KeyNode;

        // Mark the hive as read only
        CmpMarkAllBinsReadOnly(Hive);

        status = STATUS_OBJECT_NAME_NOT_FOUND;

        plist = &(pcell->ValueList);
        ChildCell = HCELL_NIL;

        if (plist->Count != 0) {

            //
            // The parent has at least one value, map in the list of
            // values and call CmpFindChildInList
            //

            //
            // plist -> the CHILD_LIST structure
            // pchild -> the child node structure being examined
            //

            ChildCell = CmpFindNameInList(Hive,
                                          plist,
                                          &ValueName,
                                          &targetaddress,
                                          &targetindex);

            if (ChildCell != HCELL_NIL) {

                //
                // 1. the desired target was found
                // 2. ChildCell is it's HCELL_INDEX
                // 3. targetaddress points to it
                // 4. targetindex is it's index
                //

                //
                // attempt to mark all relevent cells dirty
                //
                if (!(HvMarkCellDirty(Hive, Cell) &&
                      HvMarkCellDirty(Hive, pcell->ValueList.List) &&
                      HvMarkCellDirty(Hive, ChildCell)))

                {
                    // Mark the hive as read only
                    CmpMarkAllBinsReadOnly(Hive);

                    return STATUS_NO_LOG_SPACE;
                }

                if (!CmpIsHKeyValueSmall(realsize,targetaddress->u.KeyValue.DataLength)) {

                    if (!HvMarkCellDirty(Hive, targetaddress->u.KeyValue.Data)) {

                        // Mark the hive as read only
                        CmpMarkAllBinsReadOnly(Hive);

                        return(STATUS_NO_LOG_SPACE);
                    }
                }

                newcount = plist->Count - 1;

                if (newcount > 0) {
                    PCELL_DATA pvector;

                    //
                    // more than one entry list, squeeze
                    //
                    pvector = HvGetCell(Hive, plist->List);
                    for ( ; targetindex < newcount; targetindex++) {
                        pvector->u.KeyList[ targetindex ] =
                            pvector->u.KeyList[ targetindex+1 ];
                    }

                    newcell = HvReallocateCell(
                                Hive,
                                plist->List,
                                newcount * sizeof(HCELL_INDEX)
                                );
                    ASSERT(newcell != HCELL_NIL);
                    plist->List = newcell;

                } else {

                    //
                    // list is empty, free it
                    //
                    HvFreeCell(Hive, plist->List);
                }
                plist->Count = newcount;
                CmpFreeValue(Hive, ChildCell);

                KeQuerySystemTime(&systemtime);
                pcell->LastWriteTime = systemtime;

                if (pcell->ValueList.Count == 0) {
                    pcell->MaxValueNameLen = 0;
                    pcell->MaxValueDataLen = 0;
                }

                //
                // We are changing the KCB cache. Since the registry is locked exclusively,
                // we do not need a KCB lock.
                //
                ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

                //
                // Invalidate the cache
                //
                CmpCleanUpKcbValueCache(KeyControlBlock);

                KeyControlBlock->ValueCache.Count = plist->Count;
                KeyControlBlock->ValueCache.ValueList = (ULONG_PTR) plist->List;
    
                CmpReportNotify(
                        KeyControlBlock,
                        KeyControlBlock->KeyHive,
                        KeyControlBlock->KeyCell,
                        REG_NOTIFY_CHANGE_LAST_SET
                        );
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
        }
    } finally {
        CmpUnlockRegistry();
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    return status;
}


NTSTATUS
CmEnumerateKey(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    Enumerate sub keys, return data on Index'th entry.

    CmEnumerateKey returns the name of the Index'th sub key of the open
    key specified.  The value STATUS_NO_MORE_ENTRIES will be
    returned if value of Index is larger than the number of sub keys.

    Note that Index is simply a way to select among child keys.  Two calls
    to CmEnumerateKey with the same Index are NOT guaranteed to return
    the same results.

    If KeyInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    KeyControlBlock - pointer to the KCB that describes the key

    Index - Specifies the (0-based) number of the sub key to be returned.

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
    HCELL_INDEX childcell;
    PHHIVE Hive;
    HCELL_INDEX Cell;
    PCM_KEY_NODE Node;

    CMLOG(CML_WORKER, CMS_CM) KdPrint(("CmEnumerateKey\n"));


    CmpLockRegistry();

    if (KeyControlBlock->Delete) {
        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }

    Hive = KeyControlBlock->KeyHive;
    Cell = KeyControlBlock->KeyCell;

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    //
    // fetch the child of interest
    //

    childcell = CmpFindSubKeyByNumber(Hive, KeyControlBlock->KeyNode, Index);
    if (childcell == HCELL_NIL) {
        //
        // no such child, clean up and return error
        //

        CmpUnlockRegistry();

        // Mark the hive as read only
        CmpMarkAllBinsReadOnly(Hive);

        return STATUS_NO_MORE_ENTRIES;
    }

    Node = (PCM_KEY_NODE)HvGetCell(Hive,childcell);

    try {

        //
        // call a worker to perform data transfer
        //

        status = CmpQueryKeyData(Hive,
                                 Node,
                                 KeyInformationClass,
                                 KeyInformation,
                                 Length,
                                 ResultLength);

     } except (EXCEPTION_EXECUTE_HANDLER) {
        CmpUnlockRegistry();
        status = GetExceptionCode();

        // Mark the hive as read only
        CmpMarkAllBinsReadOnly(Hive);

        return status;
    }

    CmpUnlockRegistry();

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    return status;
}



NTSTATUS
CmEnumerateValueKey(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    The value entries of an open key may be enumerated.

    CmEnumerateValueKey returns the name of the Index'th value
    entry of the open key specified by KeyHandle.  The value
    STATUS_NO_MORE_ENTRIES will be returned if value of Index is
    larger than the number of sub keys.

    Note that Index is simply a way to select among value
    entries.  Two calls to NtEnumerateValueKey with the same Index
    are NOT guaranteed to return the same results.

    If KeyValueInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    KeyControlBlock - pointer to the KCB that describes the key

    Index - Specifies the (0-based) number of the sub key to be returned.

    KeyValueInformationClass - Specifies the type of information returned
    in Buffer. One of the following types:

        KeyValueBasicInformation - return time of last write,
            title index, and name.  (See KEY_VALUE_BASIC_INFORMATION)

        KeyValueFullInformation - return time of last write,
            title index, name, class.  (See KEY_VALUE_FULL_INFORMATION)

    KeyValueInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyValueInformation in bytes.

    ResultLength - Number of bytes actually written into KeyValueInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS    status;
    PHHIVE Hive;
    PCM_KEY_NODE Node;
    PCELL_DATA ChildList;
    PCM_KEY_VALUE ValueData;
    BOOLEAN    IndexCached;
    BOOLEAN    ValueCached;
    PPCM_CACHED_VALUE ContainingList;

    CMLOG(CML_WORKER, CMS_CM) KdPrint(("CmEnumerateValueKey\n"));


    //
    // lock the parent cell
    //

    CmpLockRegistry();

    if (KeyControlBlock->Delete) {
        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }
    Hive = KeyControlBlock->KeyHive;
    Node = KeyControlBlock->KeyNode;

    //
    // fetch the child of interest
    //
    //
    // Do it using the cache
    //
    if (Index >= KeyControlBlock->ValueCache.Count) {
        //
        // No such child, clean up and return error.
        //
        CmpUnlockRegistry();
        return(STATUS_NO_MORE_ENTRIES);
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    LOCK_KCB_TREE();
    if (KeyControlBlock->ExtFlags & CM_KCB_SYM_LINK_FOUND) {
        //
        // The value list is now set to the KCB for symbolic link,
        // Clean it up and set the value right before we do the query.
        //
        CmpDereferenceKeyControlBlockWithLock(KeyControlBlock->ValueCache.RealKcb);
        KeyControlBlock->ExtFlags &= ~CM_KCB_SYM_LINK_FOUND;

        KeyControlBlock->ValueCache.Count = KeyControlBlock->KeyNode->ValueList.Count;
        KeyControlBlock->ValueCache.ValueList = (ULONG_PTR) KeyControlBlock->KeyNode->ValueList.List;
    }

    ChildList = CmpGetValueListFromCache(Hive, &(KeyControlBlock->ValueCache), &IndexCached);
    ValueData = CmpGetValueKeyFromCache(Hive, ChildList, Index, &ContainingList, IndexCached, &ValueCached);    

    try {

        // Trying to catch the BAD guy who writes over our pool.
        CmpMakeValueCacheReadWrite(ValueCached,CMP_GET_CACHED_ADDRESS(KeyControlBlock->ValueCache.ValueList));

        //
        // call a worker to perform data transfer
        //
        status = CmpQueryKeyValueData(Hive,
                                  ContainingList,
                                  ValueData,
                                  ValueCached,
                                  KeyValueInformationClass,
                                  KeyValueInformation,
                                  Length,
                                  ResultLength);

         // Trying to catch the BAD guy who writes over our pool.
        CmpMakeValueCacheReadOnly(ValueCached,CMP_GET_CACHED_ADDRESS(KeyControlBlock->ValueCache.ValueList));
    } finally {

        UNLOCK_KCB_TREE();
        CmpUnlockRegistry();
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    return status;
}



NTSTATUS
CmFlushKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
    )
/*++

Routine Description:

    Forces changes made to a key to disk.

    CmFlushKey will not return to its caller until any changed data
    associated with the key has been written out.

    WARNING: CmFlushKey will flush the entire registry tree, and thus will
    burn cycles and I/O.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of node to whose sub keys are to be found

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    PCMHIVE CmHive;
    NTSTATUS    status = STATUS_SUCCESS;
    extern PCMHIVE CmpMasterHive;

    CMLOG(CML_WORKER, CMS_CM) KdPrint(("CmFlushKey\n"));

    //
    // If writes are not working, lie and say we succeeded, will
    // clean up in a short time.  Only early system init code
    // will ever know the difference.
    //
    if (CmpNoWrite) {
        return STATUS_SUCCESS;
    }


    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    CmHive = CONTAINING_RECORD(Hive, CMHIVE, Hive);

    //
    // Don't flush the master hive.  If somebody asks for a flushkey on
    // the master hive, do a CmpDoFlushAll instead.  CmpDoFlushAll flushes
    // every hive except the master hive, which is what they REALLY want.
    //
    if (CmHive == CmpMasterHive) {
        CmpDoFlushAll();
    } else {
        DCmCheckRegistry(CONTAINING_RECORD(Hive, CMHIVE, Hive));

        CmLockHive (CmHive);

        if (! HvSyncHive(Hive)) {

            status = STATUS_REGISTRY_IO_FAILED;

            CMLOG(CML_MAJOR, CMS_IO_ERROR) {
                KdPrint(("CmFlushKey: HvSyncHive failed\n"));
            }
        }

        CmUnlockHive (CmHive);
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    return  status;
}


NTSTATUS
CmQueryKey(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    Data about the class of a key, and the numbers and sizes of its
    children and value entries may be queried with CmQueryKey.

    NOTE: The returned lengths are guaranteed to be at least as
          long as the described values, but may be longer in
          some circumstances.

Arguments:

    KeyControlBlock - pointer to the KCB that describes the key

    KeyInformationClass - Specifies the type of information
        returned in Buffer.  One of the following types:

        KeyBasicInformation - return last write time, title index, and name.
            (See KEY_BASIC_INFORMATION)

        KeyNodeInformation - return last write time, title index, name, class.
            (See KEY_NODE_INFORMATION)

        KeyFullInformation - return all data except for name and security.
            (See KEY_FULL_INFORMATION)

    KeyInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyInformation in bytes.

    ResultLength - Number of bytes actually written into KeyInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS    status;

    CMLOG(CML_WORKER, CMS_CM) KdPrint(("CmQueryKey\n"));



    CmpLockRegistry();

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

    try {

        //
        // request for the FULL path of the key
        //
        if( KeyInformationClass == KeyNameInformation ) {
            if (KeyControlBlock->Delete ) {
                //
                // special case: return key deleted status, but still fill the full name of the key.
                //
                status = STATUS_KEY_DELETED;
            } else {
                status = STATUS_SUCCESS;
            }
            
            if( KeyControlBlock->NameBlock ) {

                PUNICODE_STRING Name;
                Name = CmpConstructName(KeyControlBlock);
                if (Name == NULL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    ULONG       requiredlength;
                    ULONG       minimumlength;
                    USHORT      NameLength;
                    LONG        leftlength;
                    PKEY_INFORMATION pbuffer = (PKEY_INFORMATION)KeyInformation;

                    NameLength = Name->Length;

                    requiredlength = FIELD_OFFSET(KEY_NAME_INFORMATION, Name) + NameLength;
                    
                    minimumlength = FIELD_OFFSET(KEY_NAME_INFORMATION, Name);

                    *ResultLength = requiredlength;
                    if (Length < minimumlength) {

                        status = STATUS_BUFFER_TOO_SMALL;

                    } else {
                        //
                        // Fill in the length of the name
                        //
                        pbuffer->KeyNameInformation.NameLength = NameLength;
                        
                        //
                        // Now copy the full name into the user buffer, if enough space
                        //
                        leftlength = Length - minimumlength;
                        requiredlength = NameLength;
                        if (leftlength < (LONG)requiredlength) {
                            requiredlength = leftlength;
                            status = STATUS_BUFFER_OVERFLOW;
                        }

                        //
                        // If not enough space, copy how much we can and return overflow
                        //
                        RtlCopyMemory(
                            &(pbuffer->KeyNameInformation.Name[0]),
                            Name->Buffer,
                            requiredlength
                            );
                    }

                    ExFreePoolWithTag(Name, CM_NAME_TAG | PROTECTED_POOL);
                }
            }
        } else if(KeyControlBlock->Delete ) {
            // 
            // key already deleted
            //
            status = STATUS_KEY_DELETED;
        } else {
            //
            // call a worker to perform data transfer
            //

            status = CmpQueryKeyData(KeyControlBlock->KeyHive,
                                     KeyControlBlock->KeyNode,
                                     KeyInformationClass,
                                     KeyInformation,
                                     Length,
                                     ResultLength);
        }

    } finally {
        CmpUnlockRegistry();
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

    return status;
}


NTSTATUS
CmQueryValueKey(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock,
    IN UNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    The ValueName, TitleIndex, Type, and Data for any one of a key's
    value entries may be queried with CmQueryValueKey.

    If KeyValueInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    KeyControlBlock - pointer to the KCB that describes the key

    ValueName  - The name of the value entry to return data for.

    KeyValueInformationClass - Specifies the type of information
        returned in KeyValueInformation.  One of the following types:

        KeyValueBasicInformation - return time of last write, title
            index, and name.  (See KEY_VALUE_BASIC_INFORMATION)

        KeyValueFullInformation - return time of last write, title
            index, name, class.  (See KEY_VALUE_FULL_INFORMATION)

    KeyValueInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyValueInformation in bytes.

    ResultLength - Number of bytes actually written into KeyValueInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS    status;
    HCELL_INDEX childcell;
    PHCELL_INDEX  childindex;
    HCELL_INDEX Cell;
    PCM_KEY_VALUE ValueData;
    ULONG       Index;
    BOOLEAN     ValueCached;
    PPCM_CACHED_VALUE ContainingList;

    PAGED_CODE();
    CMLOG(CML_WORKER, CMS_CM) KdPrint(("CmQueryValueKey\n"));


    CmpLockRegistry();

    if (KeyControlBlock->Delete) {
        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

    LOCK_KCB_TREE();

    if (KeyControlBlock->ExtFlags & CM_KCB_SYM_LINK_FOUND) {
        //
        // The value list is now set to the KCB for symbolic link,
        // Clean it up and set the value right before we do the query.
        //
        CmpDereferenceKeyControlBlockWithLock(KeyControlBlock->ValueCache.RealKcb);
        KeyControlBlock->ExtFlags &= ~CM_KCB_SYM_LINK_FOUND;

        KeyControlBlock->ValueCache.Count = KeyControlBlock->KeyNode->ValueList.Count;
        KeyControlBlock->ValueCache.ValueList = (ULONG_PTR) KeyControlBlock->KeyNode->ValueList.List;
    }


    try {
        //
        // Find the data
        //

        ValueData = CmpFindValueByNameFromCache(KeyControlBlock->KeyHive,
                                                &(KeyControlBlock->ValueCache),
                                                &ValueName,
                                                &ContainingList,
                                                &Index,
                                                &ValueCached
                                                );
        if (ValueData) {

            // Trying to catch the BAD guy who writes over our pool.
            CmpMakeValueCacheReadWrite(ValueCached,CMP_GET_CACHED_ADDRESS(KeyControlBlock->ValueCache.ValueList));

            //
            // call a worker to perform data transfer
            //

            status = CmpQueryKeyValueData(KeyControlBlock->KeyHive,
                                          ContainingList,
                                          ValueData,
                                          ValueCached,
                                          KeyValueInformationClass,
                                          KeyValueInformation,
                                          Length,
                                          ResultLength);

            // Trying to catch the BAD guy who writes over our pool.
            CmpMakeValueCacheReadOnly(ValueCached,CMP_GET_CACHED_ADDRESS(KeyControlBlock->ValueCache.ValueList));

        } else {
            status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

    } finally {
        UNLOCK_KCB_TREE();
        CmpUnlockRegistry();
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

    return status;
}


NTSTATUS
CmQueryMultipleValueKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN PKEY_VALUE_ENTRY ValueEntries,
    IN ULONG EntryCount,
    IN PVOID ValueBuffer,
    IN OUT PULONG BufferLength,
    IN OPTIONAL PULONG ResultLength
    )
/*++

Routine Description:

    Multiple values of any key may be queried atomically with
    this api.

Arguments:

    KeyControlBlock - Supplies the key to be queried.

    ValueEntries - Returns an array of KEY_VALUE_ENTRY structures, one for each value.

    EntryCount - Supplies the number of entries in the ValueNames and ValueEntries arrays

    ValueBuffer - Returns the value data for each value.

    BufferLength - Supplies the length of the ValueBuffer array in bytes.
                   Returns the length of the ValueBuffer array that was filled in.

    ResultLength - if present, Returns the length in bytes of the ValueBuffer
                    array required to return the requested values of this key.

Return Value:

    NTSTATUS

--*/

{
    PHHIVE Hive;
    NTSTATUS Status;
    ULONG i;
    UNICODE_STRING CurrentName;
    HCELL_INDEX ValueCell;
    PCM_KEY_VALUE ValueNode;
    ULONG RequiredLength = 0;
    ULONG UsedLength = 0;
    ULONG DataLength;
    BOOLEAN BufferFull = FALSE;
    BOOLEAN Small;
    PUCHAR Data;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();
    CMLOG(CML_WORKER, CMS_CM) KdPrint(("CmQueryMultipleValueKey\n"));


    CmpLockRegistry();
    if (KeyControlBlock->Delete) {
        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }
    Hive = KeyControlBlock->KeyHive;
    Status = STATUS_SUCCESS;

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    PreviousMode = KeGetPreviousMode();
    try {
        for (i=0; i < EntryCount; i++) {
            //
            // find the data
            //
            if (PreviousMode == UserMode) {
                CurrentName = ProbeAndReadUnicodeString(ValueEntries[i].ValueName);
                ProbeForRead(CurrentName.Buffer,CurrentName.Length,sizeof(WCHAR));
            } else {
                CurrentName = *(ValueEntries[i].ValueName);
            }
            ValueCell = CmpFindValueByName(Hive,
                                           KeyControlBlock->KeyNode,
                                           &CurrentName);
            if (ValueCell != HCELL_NIL) {

                ValueNode = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
                Small = CmpIsHKeyValueSmall(DataLength, ValueNode->DataLength);

                //
                // Round up UsedLength and RequiredLength to a ULONG boundary
                //
                UsedLength = (UsedLength + sizeof(ULONG)-1) & ~(sizeof(ULONG)-1);
                RequiredLength = (RequiredLength + sizeof(ULONG)-1) & ~(sizeof(ULONG)-1);

                //
                // If there is enough room for this data value in the buffer,
                // fill it in now. Otherwise, mark the buffer as full. We must
                // keep iterating through the values in order to determine the
                // RequiredLength.
                //
                if ((UsedLength + DataLength <= *BufferLength) &&
                    (!BufferFull)) {
                    if (DataLength > 0) {
                        if (Small) {
                            Data = (PUCHAR)&ValueNode->Data;
                        } else {
                            Data = (PUCHAR)HvGetCell(Hive, ValueNode->Data);
                        }
                        RtlCopyMemory((PUCHAR)ValueBuffer + UsedLength,
                                      Data,
                                      DataLength);
                    }
                    ValueEntries[i].Type = ValueNode->Type;
                    ValueEntries[i].DataLength = DataLength;
                    ValueEntries[i].DataOffset = UsedLength;
                    UsedLength += DataLength;
                } else {
                    BufferFull = TRUE;
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                RequiredLength += DataLength;

            } else {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                break;
            }
        }

        if (NT_SUCCESS(Status) ||
            (Status == STATUS_BUFFER_OVERFLOW)) {
            *BufferLength = UsedLength;
            if (ARGUMENT_PRESENT(ResultLength)) {
                *ResultLength = RequiredLength;
            }
        }

    } finally {
        CmpUnlockRegistry();
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    return Status;
}


NTSTATUS
CmSetValueKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN PUNICODE_STRING ValueName,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    )
/*++

Routine Description:

    A value entry may be created or replaced with CmSetValueKey.

    If a value entry with a Value ID (i.e. name) matching the
    one specified by ValueName exists, it is deleted and replaced
    with the one specified.  If no such value entry exists, a new
    one is created.  NULL is a legal Value ID.  While Value IDs must
    be unique within any given key, the same Value ID may appear
    in many different keys.

Arguments:

    KeyControlBlock - pointer to kcb for the key to operate on

    ValueName - The unique (relative to the containing key) name
        of the value entry.  May be NULL.

    Type - The integer type number of the value entry.

    Data - Pointer to buffer with actual data for the value entry.

    DataSize - Size of Data buffer.


Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS    status;
    PCM_KEY_NODE parent;
    HCELL_INDEX oldchild;
    ULONG       count;
    PHHIVE      Hive;
    HCELL_INDEX Cell;
    ULONG       StorageType;
    ULONG       TempData;
    BOOLEAN     found;
    PCELL_DATA  pdata;
    LARGE_INTEGER systemtime;
    ULONG compareSize,mustChange=FALSE;

    CMLOG(CML_WORKER, CMS_CM) KdPrint(("CmSetValueKey\n"));

    CmpLockRegistry();
    ASSERT(sizeof(ULONG) == CM_KEY_VALUE_SMALL);

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

    while (TRUE) {
        //
        // Check that we are not being asked to add a value to a key
        // that has been deleted
        //
        if (KeyControlBlock->Delete == TRUE) {
            status = STATUS_KEY_DELETED;
            goto Exit;
        }

        //
        // Check to see if this is a symbolic link node.  If so caller
        // is only allowed to create/change the SymbolicLinkValue
        // value name
        //

        if (KeyControlBlock->KeyNode->Flags & KEY_SYM_LINK &&
            (Type != REG_LINK ||
             ValueName == NULL ||
             !RtlEqualUnicodeString(&CmSymbolicLinkValueName, ValueName, TRUE)))
        {
            //
            // Disallow attempts to manipulate any value names under a symbolic link
            // except for the "SymbolicLinkValue" value name or type other than REG_LINK
            //

            // Mark the hive as read only
            CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

            status = STATUS_ACCESS_DENIED;
            goto Exit;
        }

        //
        // get reference to parent key,
        //
        Hive = KeyControlBlock->KeyHive;
        Cell = KeyControlBlock->KeyCell;
        parent = KeyControlBlock->KeyNode;

        //
        // try to find an existing value entry by the same name
        //
        count = parent->ValueList.Count;
        found = FALSE;

        if (count > 0) {
            oldchild = CmpFindNameInList(Hive,
                                         &parent->ValueList,
                                         ValueName,
                                         &pdata,
                                         NULL);

            if (oldchild != HCELL_NIL) {
                found = TRUE;
            }
        }
        //
        // Performance Hack:
        // If a Set is asking us to set a key to the current value (IE does this a lot)
        // drop it (and, therefore, the last modified time) on the floor, but return success
        // this stops the page from being dirtied, and us having to flush the registry.
        //
        //
        if (mustChange == TRUE) {
            break; //while
        }
        if ((found) &&
            (Type == pdata->u.KeyValue.Type)) {

            PCELL_DATA pcmpdata;

            if (DataSize == (pdata->u.KeyValue.DataLength & ~CM_KEY_VALUE_SPECIAL_SIZE)) {

                if (DataSize > 0) {
                    //
                    //check for small values
                    //
                    if (DataSize <= CM_KEY_VALUE_SMALL ) {
                        pcmpdata = (PCELL_DATA)&pdata->u.KeyValue.Data;
                    } else {
                        pcmpdata = HvGetCell(Hive, pdata->u.KeyValue.Data);
                    }

                    try {
                        compareSize = (ULONG)RtlCompareMemory ((PVOID)pcmpdata,Data,(DataSize & ~CM_KEY_VALUE_SPECIAL_SIZE));
                    } except (EXCEPTION_EXECUTE_HANDLER) {
                        
                        CMLOG(CML_API, CMS_EXCEPTION) {
                            KdPrint(("!!CmSetValueKey: code:%08lx\n", GetExceptionCode()));
                        }
                        status = GetExceptionCode();
                        goto Exit;
                    }
                }else {
                    compareSize = 0;
                }

                if (compareSize == DataSize) {
                    status = STATUS_SUCCESS;
                    goto Exit;
                }
            }

        }

        //
        // To Get here, we must either be changing a value, or setting a new one
        //
        mustChange=TRUE;
        //
        // We're going through these gyrations so that if someone does come in and try and delete the
        // key we're setting we're safe. Once we know we have to change the key, take the
        // Exclusive (write) lock then restart
        //
        //
        CmpUnlockRegistry();
        CmpLockRegistryExclusive();

    }// while

    // It's a different or new value, mark it dirty, since we'll
    // at least set its time stamp

    if (! HvMarkCellDirty(Hive, Cell)) {
        status = STATUS_NO_LOG_SPACE;
        goto Exit;
    }

    StorageType = HvGetCellType(Cell);

    //
    // stash small data if relevent
    //
    TempData = 0;
    if ((DataSize <= CM_KEY_VALUE_SMALL) &&
        (DataSize > 0))
    {
        try {
            RtlMoveMemory(          // yes, move memory, could be 1 byte
                &TempData,          // at the end of a page.
                Data,
                DataSize
                );
         } except (EXCEPTION_EXECUTE_HANDLER) {
            CMLOG(CML_API, CMS_EXCEPTION) {
                KdPrint(("!!CmSetValueKey: code:%08lx\n", GetExceptionCode()));
            }
            status = GetExceptionCode();
            goto Exit;
        }
    }

    if (found) {

        //
        // ----- Existing Value Entry Path -----
        //

        //
        // An existing value entry of the specified name exists,
        // set our data into it.
        //
        status = CmpSetValueKeyExisting(Hive,
                                        oldchild,
                                        pdata,
                                        Type,
                                        Data,
                                        DataSize,
                                        StorageType,
                                        TempData);

    } else {

        //
        // ----- New Value Entry Path -----
        //

        //
        // Either there are no existing value entries, or the one
        // specified is not in the list.  In either case, create and
        // fill a new one, and add it to the list
        //
        status = CmpSetValueKeyNew(Hive,
                                   parent,
                                   ValueName,
                                   Type,
                                   Data,
                                   DataSize,
                                   StorageType,
                                   TempData);
    }

    if (NT_SUCCESS(status)) {

        if (parent->MaxValueNameLen < ValueName->Length) {
            parent->MaxValueNameLen = ValueName->Length;
        }

        if (parent->MaxValueDataLen < DataSize) {
            parent->MaxValueDataLen = DataSize;
        }

        KeQuerySystemTime(&systemtime);
        parent->LastWriteTime = systemtime;

        //
        // Update the cache, no need for KCB lock as the registry is locked exclusively.
        //
        ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

        CmpCleanUpKcbValueCache(KeyControlBlock);

        KeyControlBlock->ValueCache.Count = parent->ValueList.Count;
        KeyControlBlock->ValueCache.ValueList = (ULONG_PTR) parent->ValueList.List;

        CmpReportNotify(KeyControlBlock,
                        KeyControlBlock->KeyHive,
                        KeyControlBlock->KeyCell,
                        REG_NOTIFY_CHANGE_LAST_SET);
    }

Exit:
    CmpUnlockRegistry();
  
    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

    return status;
}


NTSTATUS
CmpSetValueKeyExisting(
    IN PHHIVE  Hive,
    IN HCELL_INDEX OldChild,
    IN PCELL_DATA pvalue,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize,
    IN ULONG StorageType,
    IN ULONG TempData
    )
/*++

Routine Description:

    Helper for CmSetValueKey, implements the case where the value entry
    being set already exists.

Arguments:

    Hive - hive of interest

    OldChild - hcell_index of the value entry body to which we are to
                    set new data

    Type - The integer type number of the value entry.

    Data - Pointer to buffer with actual data for the value entry.

    DataSize - Size of Data buffer.

    StorageType - stable or volatile

    TempData - small values are passed here

Return Value:

    STATUS_SUCCESS if it worked, appropriate status code if it did not

--*/
{
    HCELL_INDEX DataCell;
    PCELL_DATA pdata;
    HCELL_INDEX NewCell;
    ULONG realsize;
    BOOLEAN small;
    PUCHAR      StashBuffer;
    ULONG BufferSize;

    ASSERT_CM_LOCK_OWNED();


    //
    // value entry by the specified name already exists
    // oldchild is hcell_index of its value entry body
    //  which we will always edit, so mark it dirty
    //
    if (! HvMarkCellDirty(Hive, OldChild)) {
        return STATUS_NO_LOG_SPACE;
    }

    small = CmpIsHKeyValueSmall(realsize, pvalue->u.KeyValue.DataLength);

    if (DataSize <= CM_KEY_VALUE_SMALL) {               // small

        //
        // We are storing a small datum, TempData has data.
        //
        if ((! small) && (realsize > 0))
        {
            //
            // value entry has existing external data to free
            //
            if (! HvMarkCellDirty(Hive, pvalue->u.KeyValue.Data)) {
                return STATUS_NO_LOG_SPACE;
            }
            HvFreeCell(Hive, pvalue->u.KeyValue.Data);
        }

        //
        // write our new small data into value entry body
        //
        pvalue->u.KeyValue.DataLength = DataSize + CM_KEY_VALUE_SPECIAL_SIZE;
        pvalue->u.KeyValue.Data = TempData;
        pvalue->u.KeyValue.Type = Type;

        return STATUS_SUCCESS;

    } else {                                            // large

        //
        // We are storing a "large" datum
        //

        //
        // See if we can write data on top of existing cell
        //
        if ((! small) && (realsize > 0)) {

            DataCell = pvalue->u.KeyValue.Data;
            ASSERT(DataCell != HCELL_NIL);
            pdata = HvGetCell(Hive, DataCell);

            ASSERT(HvGetCellSize(Hive, pdata) > 0);

            if (DataSize <= (ULONG)(HvGetCellSize(Hive, pdata))) {

                //
                // The existing data cell is big enough to hold the
                // new data.  Attempt to copy to stash buffer. if
                // we succeed we can copy directly onto the old cell.
                // if we fail, we must allocate and fill a new cell,
                // and replace the old one with it.
                //
                if (! HvMarkCellDirty(Hive, DataCell)) {
                    return STATUS_NO_LOG_SPACE;
                }

                StashBuffer = NULL;
                if (DataSize <= CmpStashBufferSize) {

                    StashBuffer = CmpStashBuffer;

                } else if (DataSize <= CM_MAX_STASH) {

                    //
                    // Try to allocate a bigger stash buffer.  If it works, keep it and
                    // free the old one.  This prevents pool from becoming too fragmented
                    // if somebody (like SAM) is repeatedly setting very large values
                    //
                    BufferSize = ((DataSize + PAGE_SIZE) & ~(PAGE_SIZE-1));

                    StashBuffer = ExAllocatePoolWithTag(PagedPool, BufferSize, 'bSmC');
                    if (StashBuffer != NULL) {
                        ExFreePool(CmpStashBuffer);
                        CmpStashBuffer = StashBuffer;
                        CmpStashBufferSize = BufferSize;
                    }
                }

                if (StashBuffer != NULL) {
                    //
                    // We have a stash buffer
                    //
                    try {

                        RtlCopyMemory(
                            StashBuffer,
                            Data,
                            DataSize
                            );

                    } except (EXCEPTION_EXECUTE_HANDLER) {
                        CMLOG(CML_API, CMS_EXCEPTION) {
                            KdPrint(("!!CmSetValueKey: code:%08lx\n", GetExceptionCode()));
                        }
                        return GetExceptionCode();
                    }


                    //
                    // We have filled the stash buffer, copy data and finish
                    //
                    RtlCopyMemory(
                        pdata,
                        StashBuffer,
                        DataSize
                        );

                    ASSERT(StashBuffer != NULL);

                    pvalue->u.KeyValue.DataLength = DataSize;
                    pvalue->u.KeyValue.Type = Type;

                    return STATUS_SUCCESS;

                 } // else stashbuffer == null
            } // else existing cell is too small
        } // else there is no existing cell
    } // new cell needed (always large)

    //
    // Either the existing cell is not large enough, or does
    // not exist, or we couldn't stash successfully.
    //
    // Allocate and fill a new cell.  Free the old one.  Store
    // the new's index into the value entry body.
    //
    NewCell = HvAllocateCell(Hive, DataSize, StorageType);

    if (NewCell == HCELL_NIL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // fill the new data cell
    //
    pdata = HvGetCell(Hive, NewCell);
    try {

        RtlMoveMemory(
            pdata,
            Data,
            DataSize
            );

    } except (EXCEPTION_EXECUTE_HANDLER) {
        CMLOG(CML_API, CMS_EXCEPTION) {
            KdPrint(("!!CmSetValueKey: code:%08lx\n", GetExceptionCode()));
        }
        HvFreeCell(Hive, NewCell);
        return GetExceptionCode();
    }

    //
    // free the old data cell
    //
    if ((! small) && (realsize > 0)) {
        ASSERT(pvalue->u.KeyValue.Data != HCELL_NIL);
        if (! HvMarkCellDirty(Hive, pvalue->u.KeyValue.Data)) {
            HvFreeCell(Hive, NewCell);
            return STATUS_NO_LOG_SPACE;
        }
        HvFreeCell(Hive, pvalue->u.KeyValue.Data);
    }

    //
    // set body
    //
    pvalue->u.KeyValue.DataLength = DataSize;
    pvalue->u.KeyValue.Data = NewCell;
    pvalue->u.KeyValue.Type = Type;

    return STATUS_SUCCESS;
}

NTSTATUS
CmpSetValueKeyNew(
    IN PHHIVE  Hive,
    IN PCM_KEY_NODE Parent,
    IN PUNICODE_STRING ValueName,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize,
    IN ULONG StorageType,
    IN ULONG TempData
    )
/*++

Routine Description:

    Helper for CmSetValueKey, implements the case where the value entry
    being set does not exist.  Will create new value entry and data,
    place in list (which may be created)

Arguments:

    Hive - hive of interest

    Parent - pointer to key node value entry is for

    ValueName - The unique (relative to the containing key) name
        of the value entry.  May be NULL.

    TitleIndex - Supplies the title index for ValueName.  The title
        index specifies the index of the localized alias for the ValueName.

    Type - The integer type number of the value entry.

    Data - Pointer to buffer with actual data for the value entry.

    DataSize - Size of Data buffer.

    StorageType - stable or volatile

    TempData - small data values passed here


Return Value:

    STATUS_SUCCESS if it worked, appropriate status code if it did not

--*/
{
    PCELL_DATA pvalue;
    HCELL_INDEX ValueCell;
    HCELL_INDEX DataCell;
    PCELL_DATA pdata;
    ULONG   count;
    HCELL_INDEX NewCell;
    ULONG AllocateSize;

    //
    // Either Count == 0 (no list) or our entry is simply not in
    // the list.  Create a new value entry body, and data.  Add to list.
    // (May create the list.)
    //
    if (Parent->ValueList.Count != 0) {
        ASSERT(Parent->ValueList.List != HCELL_NIL);
        if (! HvMarkCellDirty(Hive, Parent->ValueList.List)) {
            return STATUS_NO_LOG_SPACE;
        }
    }

    //
    // allocate the body of the value entry, and the data
    //
    ValueCell = HvAllocateCell(
                    Hive,
                    CmpHKeyValueSize(Hive, ValueName),
                    StorageType
                    );

    if (ValueCell == HCELL_NIL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DataCell = HCELL_NIL;
    if (DataSize > CM_KEY_VALUE_SMALL) {
        DataCell = HvAllocateCell(Hive, DataSize, StorageType);
        if (DataCell == HCELL_NIL) {
            HvFreeCell(Hive, ValueCell);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // map in the body, and fill in its fixed portion
    //
    pvalue = HvGetCell(Hive, ValueCell);
    pvalue->u.KeyValue.Signature = CM_KEY_VALUE_SIGNATURE;

    //
    // fill in the variable portions of the new value entry,  name and
    // and data are copied from caller space, could fault.
    //
    try {

        //
        // fill in the name
        //
        pvalue->u.KeyValue.NameLength = CmpCopyName(Hive,
                                                    pvalue->u.KeyValue.Name,
                                                    ValueName);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        CMLOG(CML_API, CMS_EXCEPTION) {
            KdPrint(("!!CmSetValueKey: code:%08lx\n", GetExceptionCode()));
        }

        //
        // We have bombed out loading user data, clean up and exit.
        //
        if (DataCell != HCELL_NIL) {
            HvFreeCell(Hive, DataCell);
        }
        HvFreeCell(Hive, ValueCell);
        return GetExceptionCode();
    }

    if (pvalue->u.KeyValue.NameLength < ValueName->Length) {
        pvalue->u.KeyValue.Flags = VALUE_COMP_NAME;
    } else {
        pvalue->u.KeyValue.Flags = 0;
    }

    //
    // fill in the data
    //
    if (DataSize > CM_KEY_VALUE_SMALL) {
        pdata = HvGetCell(Hive, DataCell);

        try {

            RtlMoveMemory(pdata, Data, DataSize);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            CMLOG(CML_API, CMS_EXCEPTION) {
                KdPrint(("!!CmSetValueKey: code:%08lx\n", GetExceptionCode()));
            }

            //
            // We have bombed out loading user data, clean up and exit.
            //
            if (DataCell != HCELL_NIL) {
                HvFreeCell(Hive, DataCell);
            }
            HvFreeCell(Hive, ValueCell);
            return GetExceptionCode();
        }

        pvalue->u.KeyValue.DataLength = DataSize;
        pvalue->u.KeyValue.Data = DataCell;

    } else {
        pvalue->u.KeyValue.DataLength = DataSize + CM_KEY_VALUE_SPECIAL_SIZE;
        pvalue->u.KeyValue.Data = TempData;
    }

    //
    // either add ourselves to list, or make new one with us in it.
    //
    count = Parent->ValueList.Count;
    count++;
    if (count > 1) {

        if (count < CM_MAX_REASONABLE_VALUES) {

            //
            // A reasonable number of values, allocate just enough
            // space.
            //

            AllocateSize = count * sizeof(HCELL_INDEX);
        } else {

            //
            // An excessive number of values, pad the allocation out
            // to avoid fragmentation. (if there's this many values,
            // there'll probably be more pretty soon)
            //
            AllocateSize = ROUND_UP(count, CM_MAX_REASONABLE_VALUES) * sizeof(HCELL_INDEX);
            if (AllocateSize > HBLOCK_SIZE) {
                AllocateSize = ROUND_UP(AllocateSize, HBLOCK_SIZE);
            }
        }
        NewCell = HvReallocateCell(
                        Hive,
                        Parent->ValueList.List,
                        AllocateSize
                        );
    } else {
        NewCell = HvAllocateCell(Hive, sizeof(HCELL_INDEX), StorageType);
    }

    //
    // put ourselves on the list
    //
    if (NewCell != HCELL_NIL) {
        Parent->ValueList.List = NewCell;
        pdata = HvGetCell(Hive, NewCell);

        pdata->u.KeyList[count-1] = ValueCell;
        Parent->ValueList.Count = count;
        pvalue->u.KeyValue.Type = Type;

        return STATUS_SUCCESS;

    } else {
        // out of space, free all allocated stuff
        if (DataCell != HCELL_NIL) {
            HvFreeCell(Hive, DataCell);
        }
        HvFreeCell(Hive, ValueCell);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
}


NTSTATUS
CmSetLastWriteTimeKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN PLARGE_INTEGER LastWriteTime
    )
/*++

Routine Description:

    The LastWriteTime associated with a key node can be set with
    CmSetLastWriteTimeKey

Arguments:

    KeyControlBlock - pointer to kcb for the key to operate on

    LastWriteTime - new time for key

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    PCM_KEY_NODE parent;
    PHHIVE      Hive;
    HCELL_INDEX Cell;
    NTSTATUS    status = STATUS_SUCCESS;

    CMLOG(CML_WORKER, CMS_CM) KdPrint(("CmSetLastWriteTimeKey\n"));

    CmpLockRegistryExclusive();

    //
    // Check that we are not being asked to modify a key
    // that has been deleted
    //
    if (KeyControlBlock->Delete == TRUE) {
        status = STATUS_KEY_DELETED;
        goto Exit;
    }

    Hive = KeyControlBlock->KeyHive;
    Cell = KeyControlBlock->KeyCell;
    parent = KeyControlBlock->KeyNode;
    if (! HvMarkCellDirty(Hive, Cell)) {
        status = STATUS_NO_LOG_SPACE;
        goto Exit;
    }

    parent->LastWriteTime = *LastWriteTime;

Exit:
    CmpUnlockRegistry();
    return status;
}


NTSTATUS
CmLoadKey(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES SourceFile,
    IN ULONG Flags
    )

/*++

Routine Description:

    A hive (file in the format created by NtSaveKey) may be linked
    into the active registry with this call.  UNLIKE NtRestoreKey,
    the file specified to NtLoadKey will become the actual backing
    store of part of the registry (that is, it will NOT be copied.)

    The file may have an associated .log file.

    If the hive file is marked as needing a .log file, and one is
    not present, the call will fail.

    The name specified by SourceFile must be such that ".log" can
    be appended to it to generate the name of the log file.  Thus,
    on FAT file systems, the hive file may not have an extension.

    This call is used by logon to make the user's profile available
    in the registry.  It is not intended for use doing backup,
    restore, etc.  Use NtRestoreKey for that.

    N.B.  This routine assumes that the object attributes for the file
          to be opened have been captured into kernel space so that
          they can safely be passed to the worker thread to open the file
          and do the actual I/O.

Arguments:

    TargetKey - specifies the path to a key to link the hive to.
                path must be of the form "\registry\user\<username>"

    SourceFile - specifies a file.  while file could be remote,
                that is strongly discouraged.

    Flags - specifies any flags that should be used for the load operation.
            The only valid flag is REG_NO_LAZY_FLUSH.

Return Value:

    NTSTATUS - values TBS.

--*/
{
    PCMHIVE NewHive;
    NTSTATUS Status;
    BOOLEAN Allocate;
    REGISTRY_COMMAND Command;
    SECURITY_QUALITY_OF_SERVICE ServiceQos;
    SECURITY_CLIENT_CONTEXT ClientSecurityContext;


    //
    // Obtain the security context here so we can use it
    // later to impersonate the user, which we will do
    // if we cannot access the file as SYSTEM.  This
    // usually occurs if the file is on a remote machine.
    //
    ServiceQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    ServiceQos.ImpersonationLevel = SecurityImpersonation;
    ServiceQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    ServiceQos.EffectiveOnly = TRUE;
    Status = SeCreateClientSecurity(CONTAINING_RECORD(KeGetCurrentThread(),ETHREAD,Tcb),
                                    &ServiceQos,
                                    FALSE,
                                    &ClientSecurityContext);
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Do not lock the registry; Instead set the RegistryLockAquired member 
    // of REGISTRY_COMMAND so CmpWorker can lock it after opening the hive files
    //
    //CmpLockRegistryExclusive();
    //

    Command.RegistryLockAquired = FALSE;
    Command.Command = REG_CMD_HIVE_OPEN;
    Command.Allocate = TRUE;
    Command.FileAttributes = SourceFile;
    Command.ImpersonationContext = &ClientSecurityContext;

    CmpWorker(&Command);
    Status = Command.Status;

    SeDeleteClientSecurity( &ClientSecurityContext );

    NewHive = Command.CmHive;
    Allocate = Command.Allocate;

    if (!NT_SUCCESS(Status)) {
        if( Command.RegistryLockAquired ) {
            // if CmpWorker has locked the registry, unlock it now.
            CmpUnlockRegistry();
        }
        return(Status);
    } else {
        //
        // if we got here, CmpWorker should have locked the registry exclusive.
        //
        ASSERT( Command.RegistryLockAquired );
    }

    //
    // if this is a NO_LAZY_FLUSH hive, set the appropriate bit.
    //
    if (Flags & REG_NO_LAZY_FLUSH) {
        NewHive->Hive.HiveFlags |= HIVE_NOLAZYFLUSH;
    }

    //
    // We now have a succesfully loaded and initialized CmHive, so we
    // just need to link that into the appropriate spot in the master hive.
    //
    Status = CmpLinkHiveToMaster(TargetKey->ObjectName,
                                 TargetKey->RootDirectory,
                                 NewHive,
                                 Allocate,
                                 TargetKey->SecurityDescriptor);

    Command.CmHive = NewHive;
    if (NT_SUCCESS(Status)) {
        //
        // add new hive to hivelist
        //
        Command.Command = REG_CMD_ADD_HIVE_LIST;

    } else {
        //
        // Close the files we've opened.
        //
        Command.Command = REG_CMD_HIVE_CLOSE;
    }
    CmpWorker(&Command);

    //
    // We've given user chance to log on, so turn on quota
    //
    if ((CmpProfileLoaded == FALSE) &&
        (CmpWasSetupBoot == FALSE)) {
        CmpProfileLoaded = TRUE;
        CmpSetGlobalQuotaAllowed();
    }

    CmpUnlockRegistry();
    return(Status);
}

#if DBG
ULONG
CmpUnloadKeyWorker(
    PCM_KEY_CONTROL_BLOCK Current,
    PVOID                 Context1,
    PVOID                 Context2
    )
{
    PUNICODE_STRING ConstructedName;
    if (Current->KeyHive == Context1) {
        ConstructedName = CmpConstructName(Current);

        if (ConstructedName) {
            KdPrint(("%wZ\n", ConstructedName));
            ExFreePoolWithTag(ConstructedName, CM_NAME_TAG | PROTECTED_POOL);
        }
    }
    return KCB_WORKER_CONTINUE;   // always keep searching
}
#endif


NTSTATUS
CmUnloadKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PCM_KEY_CONTROL_BLOCK Kcb
    )

/*++

Routine Description:

    Unlinks a hive from its location in the registry, closes its file
    handles, and deallocates all its memory.

    There must be no key control blocks currently referencing the hive
    to be unloaded.

Arguments:

    Hive - Supplies a pointer to the hive control structure for the
           hive to be unloaded

    Cell - supplies the HCELL_INDEX for the root cell of the hive.

    Kcb - Supplies the key control block

Return Value:

    NTSTATUS

--*/

{
    PCMHIVE CmHive;
    REGISTRY_COMMAND Command;
    BOOLEAN Success;

    CMLOG(CML_WORKER, CMS_CM) KdPrint(("CmUnloadKey\n"));

    CmpLockRegistryExclusive();

    //
    // Make sure the cell passed in is the root cell of the hive.
    //
    if (Cell != Hive->BaseBlock->RootCell) {
        CmpUnlockRegistry();
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Make sure there are no open references to key control blocks
    // for this hive.  If there are none, then we can unload the hive.
    //

    CmHive = CONTAINING_RECORD(Hive, CMHIVE, Hive);
    if ((CmpSearchForOpenSubKeys(Kcb,SearchIfExist) != 0) || (Kcb->RefCount != 1)) {
#if DBG
        KdPrint(("List of keys open against hive unload was attempted on:\n"));
        CmpSearchKeyControlBlockTree(
            CmpUnloadKeyWorker,
            Hive,
            NULL
            );
#endif
        CmpUnlockRegistry();
        return(STATUS_CANNOT_DELETE);
    }

    //
    // Flush any dirty data to disk. If this fails, too bad.
    //
    Command.Command = REG_CMD_FLUSH_KEY;
    Command.Hive = Hive;
    Command.Cell = Cell;
    CmpWorker(&Command);

    //
    // Remove the hive from the HiveFileList
    //
    Command.Command = REG_CMD_REMOVE_HIVE_LIST;
    Command.CmHive = (PCMHIVE)Hive;
    CmpWorker(&Command);

    //
    // Unlink from master hive, remove from list
    //
    Success = CmpDestroyHive(Hive, Cell);

    CmpUnlockRegistry();

    if (Success) {
        HvFreeHive(Hive);

        //
        // Close the hive files
        //
        Command.Command = REG_CMD_HIVE_CLOSE;
        Command.CmHive = CmHive;
        CmpWorker(&Command);

        //
        // free the cm level structure
        //
        ASSERT( CmHive->HiveLock );
        ExFreePool(CmHive->HiveLock);
        CmpFree(CmHive, sizeof(CMHIVE));

        return(STATUS_SUCCESS);
    } else {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

}



BOOLEAN
CmpDoFlushAll(
    VOID
    )
/*++

Routine Description:

    Flush all hives.

    Runs in the context of the CmpWorkerThread.

    Runs down list of Hives and applies HvSyncHive to them.

    NOTE: Hives which are marked as HV_NOLAZYFLUSH are *NOT* flushed
          by this call.  You must call HvSyncHive explicitly to flush
          a hive marked as HV_NOLAZYFLUSH.

Arguments:

Return Value:

    NONE

--*/
{
    NTSTATUS    Status;
    PLIST_ENTRY p;
    PCMHIVE     h;
    BOOLEAN     Result = TRUE;    
/*
    ULONG rc;
*/
    extern PCMHIVE CmpMasterHive;

    //
    // If writes are not working, lie and say we succeeded, will
    // clean up in a short time.  Only early system init code
    // will ever know the difference.
    //
    if (CmpNoWrite) {
        return TRUE;
    }

    //
    // traverse list of hives, sync each one
    //
    p = CmpHiveListHead.Flink;
    while (p != &CmpHiveListHead) {

        h = CONTAINING_RECORD(p, CMHIVE, HiveList);

/*
#if DBG
        if (h!=CmpMasterHive) {
            rc = CmCheckRegistry(h, FALSE);

            if (rc!=0) {
                KdPrint(("CmpDoFlushAll: corrupt hive, rc = %08lx\n",rc));
                DbgBreakPoint();
            }
        }
#endif
*/
        if (!(h->Hive.HiveFlags & HIVE_NOLAZYFLUSH)) {

            //
            //Lock the hive before we flush it.
            //-- since we now allow multiple readers
            // during a flush (a flush is considered a read)
            // we have to force a serialization on the vector table
            //
            CmLockHive (h);
            
            Status = HvSyncHive((PHHIVE)h);

            if( !NT_SUCCESS( Status ) ) {
                Result = FALSE;
            }
            CmUnlockHive (h);
            //
            // WARNNOTE - the above means that a lazy flush or
            //            or shutdown flush did not work.  we don't
            //            know why.  there is noone to report an error
            //            to, so continue on and hope for the best.
            //            (in theory, worst that can happen is user changes
            //             are lost.)
            //
        }


        p = p->Flink;
    }
    
    return Result;
}


NTSTATUS
CmReplaceKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PUNICODE_STRING NewHiveName,
    IN PUNICODE_STRING OldFileName
    )

/*++

Routine Description:

    Renames the hive file for a running system and replaces it with a new
    file.  The new file is not actually used until the next boot.

Arguments:

    Hive - Supplies a hive control structure for the hive to be replaced.

    Cell - Supplies the HCELL_INDEX of the root cell of the hive to be
           replaced.

    NewHiveName - Supplies the name of the file which is to be installed
            as the new hive.

    OldFileName - Supplies the name of the file which the existing hive
            file is to be renamed to.

Return Value:

    NTSTATUS

--*/

{
    REGISTRY_COMMAND Command;
    CHAR ObjectInfoBuffer[512];
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Attributes;
    PCMHIVE NewHive;
    POBJECT_NAME_INFORMATION NameInfo;
    ULONG OldQuotaAllowed;
    ULONG OldQuotaWarning;

    CmpLockRegistryExclusive();

    if (Hive->HiveFlags & HIVE_HAS_BEEN_REPLACED) {
        CmpUnlockRegistry();
        return STATUS_FILE_RENAMED;
    }

    //
    // temporarily disable registry quota as we will be giving this memory back immediately!
    //
    OldQuotaAllowed = CmpGlobalQuotaAllowed;
    OldQuotaWarning = CmpGlobalQuotaWarning;
    CmpGlobalQuotaAllowed = CM_WRAP_LIMIT;
    CmpGlobalQuotaWarning = CM_WRAP_LIMIT;

    //
    // First open the new hive file and check to make sure it is valid.
    //
    InitializeObjectAttributes(&Attributes,
                               NewHiveName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Command.RegistryLockAquired = TRUE;
    Command.Command = REG_CMD_HIVE_OPEN;
    Command.FileAttributes = &Attributes;
    Command.Allocate = FALSE;
    Command.ImpersonationContext = NULL;
    CmpWorker(&Command);
    Status = Command.Status;
    if (!NT_SUCCESS(Status)) {
        goto ErrorExit;
    }
    ASSERT(Command.Allocate == FALSE);

    NewHive = Command.CmHive;

    //
    // The new hive exists, and is consistent, and we have it open.
    // Now rename the current hive file.
    //
    Command.Command = REG_CMD_RENAME_HIVE;
    Command.NewName = OldFileName;
    Command.OldName = (POBJECT_NAME_INFORMATION)ObjectInfoBuffer;
    Command.NameInfoLength = sizeof(ObjectInfoBuffer);
    Command.CmHive = CONTAINING_RECORD(Hive, CMHIVE, Hive);
    CmpWorker(&Command);
    Status = Command.Status;
    if (!NT_SUCCESS(Status)) {
            //
        // rename failed, close the files associated with the new hive
        //
        Command.CmHive = NewHive;
        Command.Command = REG_CMD_HIVE_CLOSE;
        CmpWorker(&Command);
        goto ErrorExit;
    }

    //
    // The existing hive was successfully renamed, so try to rename the
    // new file to what the old hive file was named.  (which was returned
    // into ObjectInfoBuffer by the worker thread)
    //
    Hive->HiveFlags |= HIVE_HAS_BEEN_REPLACED;
    NameInfo = (POBJECT_NAME_INFORMATION)ObjectInfoBuffer;
    Command.Command = REG_CMD_RENAME_HIVE;
    Command.NewName = &NameInfo->Name;
    Command.OldName = NULL;
    Command.NameInfoLength = 0;
    Command.CmHive = NewHive;
    CmpWorker(&Command);
    Status = Command.Status;
    if (!NT_SUCCESS(Status)) {

        //
        // Close the handles to the new hive
        //
        Command.Command = REG_CMD_HIVE_CLOSE;
        Command.CmHive = NewHive;
        CmpWorker(&Command);

        //
        // We are in trouble now.  We have renamed the existing hive file,
        // but we couldn't rename the new hive file!  Try to rename the
        // existing hive file back to where it was.
        //
        Command.Command = REG_CMD_RENAME_HIVE;
        Command.NewName = &NameInfo->Name;
        Command.OldName = NULL;
        Command.NameInfoLength = 0;
        Command.CmHive = CONTAINING_RECORD(Hive, CMHIVE, Hive);
        CmpWorker(&Command);
        if (!NT_SUCCESS(Command.Status)) {
            CMLOG(CML_BUGCHECK, CMS_SAVRES) {
                KdPrint(("CmReplaceKey: renamed existing hive file, but couldn't\n"));
                KdPrint(("              rename new hive file (%08lx) ",Status));
                KdPrint((" or replace old hive file (%08lx)!\n",Command.Status));
            }

            //
            // WARNNOTE:
            //      To get into this state, the user must have relevent
            //      privileges, deliberately screw with system in an attempt
            //      to defeat it, AND get it done in a narrow timing window.
            //
            //      Further, if it's a user profile, the system will
            //      still come up.
            //
            //      Therefore, return an error code and go on.
            //

            Status = STATUS_REGISTRY_CORRUPT;

        }
    }

    //
    // All of the renaming is done.  However, we are holding an in-memory
    // image of the new hive.  Release it, since it will not actually
    // be used until next boot.
    //
    // Do not close the open file handles to the new hive, we need to
    // keep it locked exclusively until the system is rebooted to prevent
    // people from mucking with it.
    //
    RemoveEntryList(&(NewHive->HiveList));

    HvFreeHive((PHHIVE)NewHive);

    ASSERT( NewHive->HiveLock );
    ExFreePool(NewHive->HiveLock);
    CmpFree(NewHive, sizeof(CMHIVE));

ErrorExit:
    //
    // Set global quota back to what it was.
    //
    CmpGlobalQuotaAllowed = OldQuotaAllowed;
    CmpGlobalQuotaWarning = OldQuotaWarning;

    CmpUnlockRegistry();
    return(Status);
}

#ifdef _WRITE_PROTECTED_REGISTRY_POOL

VOID
CmpMarkAllBinsReadOnly(
    PHHIVE      Hive
    )
/*++

Routine Description:

    Marks the memory allocated for all the stable bins in this hive as read only.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

Return Value:

    NONE (It should work!)

--*/
{
    PHMAP_ENTRY t;
    PHBIN       Bin;
    HCELL_INDEX p;
    ULONG       Length;

    //
    // we are only interested in the stable storage
    //
    Length = Hive->Storage[Stable].Length;

    p = 0;

    //
    // for each bin in the space
    //
    while (p < Length) {
        t = HvpGetCellMap(Hive, p);
        VALIDATE_CELL_MAP(__LINE__,t,Hive,p);

        Bin = (PHBIN)((t->BinAddress) & HMAP_BASE);

        if (t->BinAddress & HMAP_NEWALLOC) {

            //
            // Mark it as read Only
            //
            HvpChangeBinAllocation(Bin,TRUE);
        }

        // next one, please
        p = (ULONG)p + Bin->Size;

    }

}

#endif

