/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmsubs.c

Abstract:

    This module various support routines for the configuration manager.

    The routines in this module are not independent enough to be linked
    into any other program.  The routines in cmsubs2.c are.

Author:

    Bryan M. Willman (bryanwi) 12-Sep-1991

Revision History:

--*/

#include    "cmp.h"

FAST_MUTEX CmpKcbLock;
FAST_MUTEX CmpPostLock;

PCM_KEY_HASH *CmpCacheTable;
ULONG CmpHashTableSize = 2048;
ULONG CmpDelayedCloseSize = 512;
PCM_KEY_CONTROL_BLOCK *CmpDelayedCloseTable;
PCM_KEY_CONTROL_BLOCK *CmpDelayedCloseCurrent;
ULONG_PTR CmpDelayedFreeIndex=0;
PCM_NAME_HASH *CmpNameCacheTable;

VOID
CmpRemoveKeyHash(
    IN PCM_KEY_HASH KeyHash
    );

PCM_KEY_CONTROL_BLOCK
CmpInsertKeyHash(
    IN PCM_KEY_HASH KeyHash,
    IN BOOLEAN      FakeKey
    );

//
// private prototype for recursive worker
//


VOID
CmpDereferenceNameControlBlockWithLock(
    PCM_NAME_CONTROL_BLOCK   Ncb
    );

#ifdef KCB_TO_KEYBODY_LINK
VOID
CmpDumpKeyBodyList(
    IN PCM_KEY_CONTROL_BLOCK   kcb,
    IN PULONG                  Count
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpSearchForOpenSubKeys)
#pragma alloc_text(PAGE,CmpReferenceKeyControlBlock)
#pragma alloc_text(PAGE,CmpGetNameControlBlock)
#pragma alloc_text(PAGE,CmpDereferenceNameControlBlockWithLock)
#pragma alloc_text(PAGE,CmpCleanUpSubKeyInfo)
#pragma alloc_text(PAGE,CmpCleanUpKcbValueCache)
#pragma alloc_text(PAGE,CmpCleanUpKcbCacheWithLock)
#pragma alloc_text(PAGE,CmpConstructName)
#pragma alloc_text(PAGE,CmpRemoveFromDelayedClose)
#pragma alloc_text(PAGE,CmpCreateKeyControlBlock)
#pragma alloc_text(PAGE,CmpSearchKeyControlBlockTree)
#pragma alloc_text(PAGE,CmpDereferenceKeyControlBlock)
#pragma alloc_text(PAGE,CmpDereferenceKeyControlBlockWithLock)
#pragma alloc_text(PAGE,CmpRemoveKeyControlBlock)
#pragma alloc_text(PAGE,CmpFreeKeyBody)
#pragma alloc_text(PAGE,CmpInsertKeyHash)
#pragma alloc_text(PAGE,CmpRemoveKeyHash)
#pragma alloc_text(PAGE,CmpInitializeCache)

#ifdef KCB_TO_KEYBODY_LINK
#pragma alloc_text(PAGE,CmpDumpKeyBodyList)
#pragma alloc_text(PAGE,CmpFlushNotifiesOnKeyBodyList)
#endif

#endif

#ifdef KCB_TO_KEYBODY_LINK
VOID
CmpDumpKeyBodyList(
    IN PCM_KEY_CONTROL_BLOCK   kcb,
    IN PULONG                  Count
    )
{
        
    PCM_KEY_BODY    KeyBody;
    PUNICODE_STRING Name;

    if( IsListEmpty(&(kcb->KeyBodyListHead)) == TRUE ) {
        //
        // Nobody has this subkey open, but for sure some subkey must be 
        // open. nicely return.
        //
        return;
    }


    Name = CmpConstructName(kcb);
    if( !Name ){
        // oops, we're low on resources
        return;
    }
    
    //
    // now iterate through the list of KEY_BODYs referencing this kcb
    //
    KeyBody = (PCM_KEY_BODY)kcb->KeyBodyListHead.Flink;
    while( KeyBody != (PCM_KEY_BODY)(&(kcb->KeyBodyListHead)) ) {
        KeyBody = CONTAINING_RECORD(KeyBody,
                                    CM_KEY_BODY,
                                    KeyBodyList);
        //
        // sanity check: this should be a KEY_BODY
        //
        ASSERT_KEY_OBJECT(KeyBody);
        
        //
        // dump it's name and owning process
        //
        DbgPrint("Process %lx :: Key %wZ \n",KeyBody->Process,Name);
        
        // count it
        (*Count)++;
        
        KeyBody = (PCM_KEY_BODY)KeyBody->KeyBodyList.Flink;
    }

    ExFreePoolWithTag(Name, CM_NAME_TAG | PROTECTED_POOL);

}

VOID
CmpFlushNotifiesOnKeyBodyList(
    IN PCM_KEY_CONTROL_BLOCK   kcb
    )
{
    PCM_KEY_BODY    KeyBody;
    
    if( IsListEmpty(&(kcb->KeyBodyListHead)) == FALSE ) {
        //
        // now iterate through the list of KEY_BODYs referencing this kcb
        //
        KeyBody = (PCM_KEY_BODY)kcb->KeyBodyListHead.Flink;
        while( KeyBody != (PCM_KEY_BODY)(&(kcb->KeyBodyListHead)) ) {
            KeyBody = CONTAINING_RECORD(KeyBody,
                                        CM_KEY_BODY,
                                        KeyBodyList);
            //
            // sanity check: this should be a KEY_BODY
            //
            ASSERT_KEY_OBJECT(KeyBody);

            //
            // flush any notifies that might be set on it
            //
            CmpFlushNotify(KeyBody);

            KeyBody = (PCM_KEY_BODY)KeyBody->KeyBodyList.Flink;
        }
    }
}
#endif

ULONG
CmpSearchForOpenSubKeys(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock,
    SUBKEY_SEARCH_TYPE      SearchType
    )
/*++
Routine Description:

    This routine searches the KCB tree for any open handles to keys that
    are subkeys of the given key.

    It is used by CmRestoreKey to verify that the tree being restored to
    has no open handles.

Arguments:

    KeyControlBlock - Supplies the key control block for the key for which
        open subkeys are to be found.

    SearchType - the type of the search
        SearchIfExist - exits at the first open subkey found ==> returns 1 if any subkey is open
        
        SearchAndDeref - Forces the keys underneath the Key referenced KeyControlBlock to 
                be marked as not referenced (see the REG_FORCE_RESTORE flag in CmRestoreKey) 
                returns 1 if at least one deref was made
        
        SearchAndCount - Counts all open subkeys - returns the number of them

Return Value:

    TRUE  - open handles to subkeys of the given key exist

    FALSE - open handles to subkeys of the given key do not exist.
--*/
{
    ULONG i;
    PCM_KEY_HASH *Current;
    PCM_KEY_CONTROL_BLOCK kcb;
    PCM_KEY_CONTROL_BLOCK Realkcb;
    PCM_KEY_CONTROL_BLOCK Parent;
    USHORT LevelDiff, l;
    ULONG   Count = 0;
    
    //
    // Registry lock should be held exclusively, so no need to KCB lock
    //
    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();


    //
    // First, clean up all subkeys in the cache
    //
    for (i=0; i<CmpHashTableSize; i++) {
        Current = &CmpCacheTable[i];
        while (*Current) {
            kcb = CONTAINING_RECORD(*Current, CM_KEY_CONTROL_BLOCK, KeyHash);
            if (kcb->RefCount == 0) {
                //
                // This kcb is in DelayClose case, remove it.
                //
                CmpRemoveFromDelayedClose(kcb);
                CmpCleanUpKcbCacheWithLock(kcb);

                //
                // The HashTable is changed, start over in this index again.
                //
                Current = &CmpCacheTable[i];
                continue;
            }
            Current = &kcb->NextHash;
        }
    }

    if (KeyControlBlock->RefCount == 1) {
        //
        // There is only one open handle, so there must be no open subkeys.
        //
        Count = 0;
    } else {
        //
        // Now search for an open subkey handle.
        //
        Count = 0;

     
#ifdef KCB_TO_KEYBODY_LINK
        //
        // dump the root first if we were asked to do so.
        //
        if(SearchType == SearchAndCount) {
            CmpDumpKeyBodyList(KeyControlBlock,&Count);
        }
#endif
        for (i=0; i<CmpHashTableSize; i++) {

StartDeref:
            Current = &CmpCacheTable[i];
            while (*Current) {
                kcb = CONTAINING_RECORD(*Current, CM_KEY_CONTROL_BLOCK, KeyHash);
                if (kcb->TotalLevels > KeyControlBlock->TotalLevels) {
                    LevelDiff = kcb->TotalLevels - KeyControlBlock->TotalLevels;
                
                    Parent = kcb;
                    for (l=0; l<LevelDiff; l++) {
                        Parent = Parent->ParentKcb;
                    }
    
                    if (Parent == KeyControlBlock) {
                        //
                        // Found a match;
                        //
                        if( SearchType == SearchIfExist ) {
                            Count = 1;
                            break;
                        } else if(SearchType == SearchAndDeref) {
                            //
                            // Mark the key as deleted, remove it from cache, but don't add it
                            // to the Delay Close table (we want the key to be visible only to
                            // the one(s) that have open handles on it.
                            //

                            ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

                            //
                            // flush any pending notifies as the kcb won't be around any longer
                            //
#ifdef KCB_TO_KEYBODY_LINK
                            CmpFlushNotifiesOnKeyBodyList(kcb);
#endif
                            
                            CmpCleanUpSubKeyInfo(kcb->ParentKcb);
                            kcb->Delete = TRUE;
                            CmpRemoveKeyControlBlock(kcb);
                            kcb->KeyCell = HCELL_NIL;
                            kcb->KeyNode = NULL;
                            Count++;
                         
                            //
                            // Restart the search 
                            // 
                            goto StartDeref;
                        
                        } else if(SearchType == SearchAndCount) {
                            //
                            // here do the dumping and count incrementing stuff
                            //
                        #ifdef KCB_TO_KEYBODY_LINK
                            CmpDumpKeyBodyList(kcb,&Count);
                        #else
                            Count++;
                        #endif
                        }
                    }   

                }
                Current = &kcb->NextHash;
            }
        }
    }
    
                           
    return Count;
}


BOOLEAN
CmpReferenceKeyControlBlock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
{
    if (KeyControlBlock->RefCount == 0) {
        CmpRemoveFromDelayedClose(KeyControlBlock);
    }

    if ((USHORT)(KeyControlBlock->RefCount + 1) == 0) {
        //
        // We have maxed out the ref count on this key. Probably
        // some bogus app has opened the same key 64K times without
        // ever closing it. Just fail the call
        //
        return (FALSE);
    } else {
        ++(KeyControlBlock->RefCount);
        return (TRUE);
    }
}


PCM_NAME_CONTROL_BLOCK
CmpGetNameControlBlock(
    PUNICODE_STRING NodeName
    )
{
    PCM_NAME_CONTROL_BLOCK   Ncb;
    ULONG  Cnt;
    WCHAR *Cp;
    WCHAR *Cp2;
    ULONG Index;
    ULONG i;
    ULONG      Size;
    PCM_NAME_HASH CurrentName;
    ULONG rc;
    BOOLEAN NameFound = FALSE;
    USHORT NameSize;
    BOOLEAN NameCompressed;
    ULONG NameConvKey=0;
    LONG Result;

    //
    // Calculate the ConvKey for this NadeName;
    //

    Cp = NodeName->Buffer;
    for (Cnt=0; Cnt<NodeName->Length; Cnt += sizeof(WCHAR)) {
        if (*Cp != OBJ_NAME_PATH_SEPARATOR) {
            NameConvKey = 37 * NameConvKey + (ULONG) RtlUpcaseUnicodeChar(*Cp);
        }
        ++Cp;
    }

    //
    // Find the Name Size;
    // 
    NameCompressed = TRUE;
    NameSize = NodeName->Length / sizeof(WCHAR);
    for (i=0;i<NodeName->Length/sizeof(WCHAR);i++) {
        if ((USHORT)NodeName->Buffer[i] > (UCHAR)-1) {
            NameSize = NodeName->Length;
            NameCompressed = FALSE;
        }
    }

    Index = GET_HASH_INDEX(NameConvKey);
    CurrentName = CmpNameCacheTable[Index];

    while (CurrentName) {
        Ncb =  CONTAINING_RECORD(CurrentName, CM_NAME_CONTROL_BLOCK, NameHash);

        if ((NameConvKey == CurrentName->ConvKey) &&
            (NameSize == Ncb->NameLength)) {
            //
            // Hash value matches, compare the names.
            //
            NameFound = TRUE;
            if (Ncb->Compressed) {
                if (CmpCompareCompressedName(NodeName, Ncb->Name, NameSize)) {
                    NameFound = FALSE;
                }
            } else {
                Cp = (WCHAR *) NodeName->Buffer;
                Cp2 = (WCHAR *) Ncb->Name;
                for (i=0 ;i<Ncb->NameLength; i+= sizeof(WCHAR)) {
                    if (RtlUpcaseUnicodeChar(*Cp) != RtlUpcaseUnicodeChar(*Cp2)) {
                        NameFound = FALSE;
                        break;
                    }
                    ++Cp;
                    ++Cp2;
                }
            }
            if (NameFound) {
                //
                // Found it, increase the refcount.
                //
                if ((USHORT) (Ncb->RefCount + 1) == 0) {
                    //
                    // We have maxed out the ref count.
                    // fail the call.
                    //
                    Ncb = NULL;
                } else {
                    ++Ncb->RefCount;
                }
                break;
            }
        }
        CurrentName = CurrentName->NextHash;
    }
    
    if (NameFound == FALSE) {
        //
        // Now need to create one Name block for this string.
        //
        Size = FIELD_OFFSET(CM_NAME_CONTROL_BLOCK, Name) + NameSize;
 
        Ncb = ExAllocatePoolWithTag(PagedPool,
                                    Size,
                                    CM_NAME_TAG | PROTECTED_POOL);
 
        if (Ncb == NULL) {
            return(NULL);
        }
        RtlZeroMemory(Ncb, Size);
 
        //
        // Update all the info for this newly created Name block.
        //
        if (NameCompressed) {
            Ncb->Compressed = TRUE;
            for (i=0;i<NameSize;i++) {
                ((PUCHAR)Ncb->Name)[i] = (UCHAR)(NodeName->Buffer[i]);
            }
        } else {
            Ncb->Compressed = FALSE;
            RtlCopyMemory((PVOID)(Ncb->Name), (PVOID)(NodeName->Buffer), NameSize);
        }

        Ncb->ConvKey = NameConvKey;
        Ncb->RefCount = 1;
        Ncb->NameLength = NameSize;
        
        CurrentName = &(Ncb->NameHash);
        //
        // Insert into Name Hash table.
        //
        CurrentName->NextHash = CmpNameCacheTable[Index];
        CmpNameCacheTable[Index] = CurrentName;
    }

    return(Ncb);
}


VOID
CmpDereferenceNameControlBlockWithLock(
    PCM_NAME_CONTROL_BLOCK   Ncb
    )
{
    PCM_NAME_HASH *Prev;
    PCM_NAME_HASH Current;

    if (--Ncb->RefCount == 0) {

        //
        // Remove it from the the Hash Table
        //
        Prev = &(GET_HASH_ENTRY(CmpNameCacheTable, Ncb->ConvKey));
        
        while (TRUE) {
            Current = *Prev;
            ASSERT(Current != NULL);
            if (Current == &(Ncb->NameHash)) {
                *Prev = Current->NextHash;
                break;
            }
            Prev = &Current->NextHash;
        }

        //
        // Free storage
        //
        ExFreePoolWithTag(Ncb, CM_NAME_TAG | PROTECTED_POOL);
    }
    return;
}


VOID
CmpCleanUpSubKeyInfo(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
/*++
Routine Description:

    Clean up the subkey information cache due to create or delete keys.
    Registry is locked exclusively and no need to lock the KCB.

Arguments:

    KeyControlBlock - pointer to a key control block.

Return Value:

    NONE.

--*/
{
    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    if (KeyControlBlock->ExtFlags & (CM_KCB_NO_SUBKEY | CM_KCB_SUBKEY_ONE | CM_KCB_SUBKEY_HINT)) {
        if (KeyControlBlock->ExtFlags & (CM_KCB_SUBKEY_HINT)) {
            ExFreePoolWithTag(KeyControlBlock->IndexHint, CM_CACHE_INDEX_TAG | PROTECTED_POOL);
        }
        KeyControlBlock->ExtFlags &= ~((CM_KCB_NO_SUBKEY | CM_KCB_SUBKEY_ONE | CM_KCB_SUBKEY_HINT));
    }
}


VOID
CmpCleanUpKcbValueCache(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
/*++

Routine Description:

    Clean up cached value/data that are associated to this key.

Arguments:

    KeyControlBlock - pointer to a key control block.

Return Value:

    NONE.

--*/
{
    ULONG i;
    PULONG_PTR CachedList;
    PCELL_DATA pcell;
    ULONG      realsize;
    BOOLEAN    small;

    if (CMP_IS_CELL_CACHED(KeyControlBlock->ValueCache.ValueList)) {
        CachedList = (PULONG_PTR) CMP_GET_CACHED_CELLDATA(KeyControlBlock->ValueCache.ValueList);
        for (i = 0; i < KeyControlBlock->ValueCache.Count; i++) {
            if (CMP_IS_CELL_CACHED(CachedList[i])) {

                // Trying to catch the BAD guy who writes over our pool.
                CmpMakeSpecialPoolReadWrite( CMP_GET_CACHED_ADDRESS(CachedList[i]) );

                ExFreePool((PVOID) CMP_GET_CACHED_ADDRESS(CachedList[i]));
               
            }
        }

        // Trying to catch the BAD guy who writes over our pool.
        CmpMakeSpecialPoolReadWrite( CMP_GET_CACHED_ADDRESS(KeyControlBlock->ValueCache.ValueList) );

        ExFreePool((PVOID) CMP_GET_CACHED_ADDRESS(KeyControlBlock->ValueCache.ValueList));

        // Mark the ValueList as NULL 
        KeyControlBlock->ValueCache.ValueList = HCELL_NIL;

    } else if (KeyControlBlock->ExtFlags & CM_KCB_SYM_LINK_FOUND) {
        //
        // This is a symbolic link key with symbolic name resolved.
        // Dereference to its real kcb and clear the bit.
        //
        if ((KeyControlBlock->ValueCache.RealKcb->RefCount == 1) && !(KeyControlBlock->ValueCache.RealKcb->Delete)) {
            KeyControlBlock->ValueCache.RealKcb->ExtFlags |= CM_KCB_NO_DELAY_CLOSE;
        }
        CmpDereferenceKeyControlBlockWithLock(KeyControlBlock->ValueCache.RealKcb);
        KeyControlBlock->ExtFlags &= ~CM_KCB_SYM_LINK_FOUND;
    }
}


VOID
CmpCleanUpKcbCacheWithLock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
/*++

Routine Description:

    Clean up all cached allocations that are associated to this key.
    If the parent is still open just because of this one, Remove the parent as well.

Arguments:

    KeyControlBlock - pointer to a key control block.

Return Value:

    NONE.

--*/
{
    PCM_KEY_CONTROL_BLOCK   Kcb;
    PCM_KEY_CONTROL_BLOCK   ParentKcb;

    Kcb = KeyControlBlock;

    ASSERT(KeyControlBlock->RefCount == 0);

    while (Kcb->RefCount == 0) {
        //
        // First, free allocations for Value/data.
        //
    
        CmpCleanUpKcbValueCache(Kcb);
    
        //
        // Free the kcb and dereference parentkcb and nameblock.
        //
    
        CmpDereferenceNameControlBlockWithLock(Kcb->NameBlock);
    
        if (Kcb->ExtFlags & CM_KCB_SUBKEY_HINT) {
            //
            // Now free the HintIndex allocation
            //
            ExFreePoolWithTag(Kcb->IndexHint, CM_CACHE_INDEX_TAG | PROTECTED_POOL);
        }

        //
        // Save the ParentKcb before we free the Kcb
        //
        ParentKcb = Kcb->ParentKcb;
        
        //
        // We cannot call CmpDereferenceKeyControlBlockWithLock so we can avoid recurrsion.
        //
        
        if (!Kcb->Delete) {
            CmpRemoveKeyControlBlock(Kcb);
        }
        SET_KCB_SIGNATURE(Kcb, '4FmC');
        ASSERT_KEYBODY_LIST_EMPTY(Kcb);
        ExFreePoolWithTag(Kcb, CM_KCB_TAG | PROTECTED_POOL);

        Kcb = ParentKcb;
        Kcb->RefCount--;
    }
}


PUNICODE_STRING
CmpConstructName(
    PCM_KEY_CONTROL_BLOCK kcb
)
/*++

Routine Description:

    Construct the name given a kcb.

Arguments:

    kcb - Kcb for the key

Return Value:

    Pointer to the unicode string constructed.  
    Caller is responsible to free this storage space.

--*/
{
    PUNICODE_STRING FullName;
    PCM_KEY_CONTROL_BLOCK TmpKcb;
    USHORT Length;
    USHORT size;
    USHORT i;
    USHORT BeginPosition;
    WCHAR *w1, *w2;
    UCHAR *u2;

    //
    // Calculate the total string length.
    //
    Length = 0;
    TmpKcb = kcb;
    while (TmpKcb) {
        if (TmpKcb->NameBlock->Compressed) {
            Length += TmpKcb->NameBlock->NameLength * sizeof(WCHAR);
        } else {
            Length += TmpKcb->NameBlock->NameLength; 
        }
        //
        // Add the sapce for OBJ_NAME_PATH_SEPARATOR;
        //
        Length += sizeof(WCHAR);

        TmpKcb = TmpKcb->ParentKcb;
    }

    //
    // Allocate the pool for the unicode string
    //
    size = sizeof(UNICODE_STRING) + Length;

    FullName = (PUNICODE_STRING) ExAllocatePoolWithTag(PagedPool,
                                                       size,
                                                       CM_NAME_TAG | PROTECTED_POOL);

    if (FullName) {
        FullName->Buffer = (USHORT *) ((ULONG_PTR) FullName + sizeof(UNICODE_STRING));
        FullName->Length = Length;
        FullName->MaximumLength = Length;

        //
        // Now fill the name into the buffer.
        //
        TmpKcb = kcb;
        BeginPosition = Length;

        while (TmpKcb) {
            //
            // Calculate the begin position of each subkey. Then fill in the char.
            //
            //
            if (TmpKcb->NameBlock->Compressed) {
                BeginPosition -= (TmpKcb->NameBlock->NameLength + 1) * sizeof(WCHAR);
                w1 = &(FullName->Buffer[BeginPosition/sizeof(WCHAR)]);
                *w1 = OBJ_NAME_PATH_SEPARATOR;
                w1++;

                u2 = (UCHAR *) &(TmpKcb->NameBlock->Name[0]);

                for (i=0; i<TmpKcb->NameBlock->NameLength; i++) {
                    *w1 = (WCHAR)(*u2);
                    w1++;
                    u2++;
                }
            } else {
                BeginPosition -= (TmpKcb->NameBlock->NameLength + sizeof(WCHAR));
                w1 = &(FullName->Buffer[BeginPosition/sizeof(WCHAR)]);
                *w1 = OBJ_NAME_PATH_SEPARATOR;
                w1++;

                w2 = TmpKcb->NameBlock->Name;

                for (i=0; i<TmpKcb->NameBlock->NameLength; i=i+sizeof(WCHAR)) {
                    *w1 = *w2;
                    w1++;
                    w2++;
                }
            }
            TmpKcb = TmpKcb->ParentKcb;
        }
    }
    return (FullName);
}


VOID
CmpRemoveFromDelayedClose(
    IN PCM_KEY_CONTROL_BLOCK kcb
    )
{
    ULONG i;
    i = kcb->DelayedCloseIndex;

    ASSERT (CmpDelayedCloseTable[i] == kcb);
    ASSERT(i != CmpDelayedCloseSize);

    CmpDelayedCloseTable[i] = (PCM_KEY_CONTROL_BLOCK)CmpDelayedFreeIndex;
    CmpDelayedFreeIndex = i;
    kcb->DelayedCloseIndex = 0;
}


PCM_KEY_CONTROL_BLOCK
CmpCreateKeyControlBlock(
    PHHIVE          Hive,
    HCELL_INDEX     Cell,
    PCM_KEY_NODE    Node,
    PCM_KEY_CONTROL_BLOCK ParentKcb,
    BOOLEAN         FakeKey,
    PUNICODE_STRING KeyName
    )
/*++

Routine Description:

    Allocate and initialize a key control block, insert it into
    the kcb tree.

    Full path will be BaseName + '\' + KeyName, unless BaseName
    NULL, in which case the full path is simply KeyName.

    RefCount of returned KCB WILL have been incremented to reflect
    callers ref.

Arguments:

    Hive - Supplies Hive that holds the key we are creating a KCB for.

    Cell - Supplies Cell that contains the key we are creating a KCB for.

    Node - Supplies pointer to key node.

    ParentKcb - Parent kcb of the kcb to be created

    FakeKey - Whether the kcb to be create is a fake one or not

    KeyName - the subkey name to of the KCB to be created.
 
    NOTE:  We need the parameter instead of just using the name in the KEY_NODE 
           because there is no name in the root cell of a hive.

Return Value:

    NULL - failure (insufficient memory)
    else a pointer to the new kcb.

--*/
{
    PCM_KEY_CONTROL_BLOCK   kcb;
    PCM_KEY_CONTROL_BLOCK   kcbmatch=NULL;
    PCMHIVE CmHive;
    ULONG namelength;
    PUNICODE_STRING         fullname;
    ULONG       Size;
    ULONG i;

    UNICODE_STRING         NodeName;
    ULONG ConvKey=0;
    ULONG Cnt;
    WCHAR *Cp;

    //
    // ParentKCb has the base hash value.
    //
    if (ParentKcb) {
        ConvKey = ParentKcb->ConvKey;
    }

    NodeName = *KeyName;

    while ((NodeName.Length > 0) && (NodeName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR)) {
        //
        // This must be the \REGISTRY.
        // Strip off the leading OBJ_NAME_PATH_SEPARATOR
        //
        NodeName.Buffer++;
        NodeName.Length -= sizeof(WCHAR);
    }

    //
    // Manually compute the hash to use.
    //
    ASSERT(NodeName.Length > 0);

    if (NodeName.Length) {
        Cp = NodeName.Buffer;
        for (Cnt=0; Cnt<NodeName.Length; Cnt += sizeof(WCHAR)) {
            if ((*Cp != OBJ_NAME_PATH_SEPARATOR) &&
                (*Cp != UNICODE_NULL)) {
                ConvKey = 37 * ConvKey + (ULONG)RtlUpcaseUnicodeChar(*Cp);
            }
            ++Cp;
        }
    }

    //
    // Create a new kcb, which we will free if one already exists
    // for this key.
    // Now it is a fixed size structure.
    //
    kcb = ExAllocatePoolWithTag(PagedPool,
                                sizeof(CM_KEY_CONTROL_BLOCK),
                                CM_KCB_TAG | PROTECTED_POOL);


    if (kcb == NULL) {
        return(NULL);
    } else {
        SET_KCB_SIGNATURE(kcb, KCB_SIGNATURE);
        INIT_KCB_KEYBODY_LIST(kcb);
        kcb->Delete = FALSE;
        kcb->RefCount = 1;
        kcb->KeyHive = Hive;
        kcb->KeyCell = Cell;
        kcb->KeyNode = Node;
        kcb->ConvKey = ConvKey;

    }

    ASSERT_KCB(kcb);
    //
    // Find location to insert kcb in kcb tree.
    //


    LOCK_KCB_TREE();

    //
    // Add the KCB to the hash table
    //
    kcbmatch = CmpInsertKeyHash(&kcb->KeyHash, FakeKey);
    if (kcbmatch != NULL) {
        //
        // A match was found.
        //
        ASSERT(!kcbmatch->Delete);
        SET_KCB_SIGNATURE(kcb, '1FmC');
        ASSERT_KEYBODY_LIST_EMPTY(kcb);
        ExFreePoolWithTag(kcb, CM_KCB_TAG | PROTECTED_POOL);
        ASSERT_KCB(kcbmatch);
        kcb = kcbmatch;
        if (kcb->RefCount == 0) {
            //
            // This kcb is on the delayed close list. Remove it from that
            // list.
            //
            CmpRemoveFromDelayedClose(kcb);
        }
        if ((USHORT)(kcb->RefCount + 1) == 0) {
            //
            // We have maxed out the ref count on this key. Probably
            // some bogus app has opened the same key 64K times without
            // ever closing it. Just fail the open, they've got enough
            // handles already.
            //
            ASSERT(kcb->RefCount + 1 != 0);
            kcb = NULL;
        } else {
            ++kcb->RefCount;
        }
    }
    else {
        //
        // No kcb created previously, fill in all the data.
        //

        //
        // Now try to reference the parentkcb
        //
        
        if (ParentKcb) {
            if (CmpReferenceKeyControlBlock(ParentKcb)) {
                kcb->ParentKcb = ParentKcb;
                kcb->TotalLevels = ParentKcb->TotalLevels + 1;
            } else {
                //
                // We have maxed out the ref count on the parent.
                // Since it has been cached in the cachetable,
                // remove it first before we free the allocation.
                //
                CmpRemoveKeyControlBlock(kcb);
                SET_KCB_SIGNATURE(kcb, '2FmC');
                ASSERT_KEYBODY_LIST_EMPTY(kcb);
                ExFreePoolWithTag(kcb, CM_KCB_TAG | PROTECTED_POOL);
                kcb = NULL;
            }
        } else {
            //
            // It is the \REGISTRY node.
            //
            kcb->ParentKcb = NULL;
            kcb->TotalLevels = 1;
        }

        if (kcb) {
            //
            // Now try to find the Name Control block that has the name for this node.
            //
    
            kcb->NameBlock = CmpGetNameControlBlock (&NodeName);

            if (kcb->NameBlock) {
                //
                // Now fill in all the data needed for the cache.
                //
                kcb->ValueCache.Count = Node->ValueList.Count;
                kcb->ValueCache.ValueList = (ULONG_PTR) Node->ValueList.List;
        
                kcb->Flags = Node->Flags;
                kcb->ExtFlags = 0;
                kcb->DelayedCloseIndex = 0;
        
                //
                // Cache the security cells in the kcb
                //
                kcb->Security = Node->Security;
        
                if (FakeKey) {
                    //
                    // The KCb to be created is a fake one
                    //
                    kcb->ExtFlags |= CM_KCB_KEY_NON_EXIST;
                }
            } else {
                //
                // We have maxed out the ref count on the Name.
                //
                
                //
                // First dereference the parent KCB.
                //
                CmpDereferenceKeyControlBlockWithLock(ParentKcb);

                CmpRemoveKeyControlBlock(kcb);
                SET_KCB_SIGNATURE(kcb, '3FmC');
                ASSERT_KEYBODY_LIST_EMPTY(kcb);
                ExFreePoolWithTag(kcb, CM_KCB_TAG | PROTECTED_POOL);
                kcb = NULL;
            }
        }
    }


    UNLOCK_KCB_TREE();
    return kcb;
}



VOID
CmpSearchKeyControlBlockTree(
    PKCB_WORKER_ROUTINE WorkerRoutine,
    PVOID               Context1,
    PVOID               Context2
    )
/*++

Routine Description:

    Traverse the kcb tree.  We will visit all nodes unless WorkerRoutine
    tells us to stop part way through.

    For each node, call WorkerRoutine(..., Context1, Contex2).  If it returns
    KCB_WORKER_DONE, we are done, simply return.  If it returns
    KCB_WORKER_CONTINUE, just continue the search. If it returns KCB_WORKER_DELETE,
    the specified KCB is marked as deleted.

    This routine has the side-effect of removing all delayed-close KCBs.

Arguments:

    WorkerRoutine - applied to nodes witch Match.

    Context1 - data we pass through

    Context2 - data we pass through


Return Value:

    NONE.

--*/
{
    PCM_KEY_CONTROL_BLOCK   Current;
    PCM_KEY_CONTROL_BLOCK   Next;
    PCM_KEY_HASH *Prev;
    ULONG                   WorkerResult;
    ULONG                   i;

    //
    // Walk the hash table
    //
    for (i=0; i<CmpHashTableSize; i++) {
        Prev = &CmpCacheTable[i];
        while (*Prev) {
            Current = CONTAINING_RECORD(*Prev,
                                        CM_KEY_CONTROL_BLOCK,
                                        KeyHash);
            ASSERT_KCB(Current);
            ASSERT(!Current->Delete);
            if (Current->RefCount == 0) {
                //
                // This kcb is in DelayClose case, remove it.
                //
                CmpRemoveFromDelayedClose(Current);
                CmpCleanUpKcbCacheWithLock(Current);

                //
                // The HashTable is changed, start over in this index again.
                //
                Prev = &CmpCacheTable[i];
                continue;
            }

            WorkerResult = (WorkerRoutine)(Current, Context1, Context2);
            if (WorkerResult == KCB_WORKER_DONE) {
                return;
            } else if (WorkerResult == KCB_WORKER_DELETE) {
                ASSERT(Current->Delete);
                *Prev = Current->NextHash;
                continue;
            } else {
                ASSERT(WorkerResult == KCB_WORKER_CONTINUE);
                Prev = &Current->NextHash;
            }
        }
    }
}


VOID
CmpDereferenceKeyControlBlock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
/*++

Routine Description:

    Decrements the reference count on a key control block, and frees it if it
    becomes zero.

    It is expected that no notify control blocks remain if the reference count
    becomes zero.

Arguments:

    KeyControlBlock - pointer to a key control block.

Return Value:

    NONE.

--*/
{
    LOCK_KCB_TREE();
    CmpDereferenceKeyControlBlockWithLock(KeyControlBlock) ;
    UNLOCK_KCB_TREE();
    return;
}


VOID
CmpDereferenceKeyControlBlockWithLock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
{
    ULONG_PTR FreeIndex;
    PCM_KEY_CONTROL_BLOCK KcbToFree;



    ASSERT_KCB(KeyControlBlock);

    if (--KeyControlBlock->RefCount == 0) {
        //
        // Remove kcb from the tree
        //
        if (KeyControlBlock->ExtFlags & CM_KCB_NO_DELAY_CLOSE) {
            //
            // Free storage directly so we can clean up junk quickly.
            //
            //
            // Need to free all cached Index List, Index Leaf, Value, etc.
            //
            CmpCleanUpKcbCacheWithLock(KeyControlBlock);
        } else if (!KeyControlBlock->Delete) {

            //
            // Put this kcb on our delayed close list.
            //
            // First check the free list for a free slot.
            //
            FreeIndex = CmpDelayedFreeIndex;
            if (FreeIndex != -1) {
                ASSERT(FreeIndex < CmpDelayedCloseSize);
                CmpDelayedFreeIndex = (ULONG_PTR)CmpDelayedCloseTable[FreeIndex];
                KeyControlBlock->DelayedCloseIndex = (USHORT) FreeIndex;
                ASSERT((CmpDelayedFreeIndex == -1) ||
                       (CmpDelayedFreeIndex < CmpDelayedCloseSize));
                CmpDelayedCloseTable[FreeIndex] = KeyControlBlock;
            } else {

                //
                // Nothing is free, we have to get rid of something.
                //
                ASSERT(*CmpDelayedCloseCurrent != NULL);
                ASSERT(!(*CmpDelayedCloseCurrent)->Delete);
                //
                // Need to free all cached Index List, Index Leaf, Value, etc.
                // Change the code sequence for the recurrsive call to dereference the parent.
                //
                KcbToFree = *CmpDelayedCloseCurrent;
                *CmpDelayedCloseCurrent = KeyControlBlock;
                KeyControlBlock->DelayedCloseIndex = (USHORT)(CmpDelayedCloseCurrent - CmpDelayedCloseTable);
                ++CmpDelayedCloseCurrent;
                if ((ULONG)(CmpDelayedCloseCurrent - CmpDelayedCloseTable) == CmpDelayedCloseSize) {
                    CmpDelayedCloseCurrent = CmpDelayedCloseTable;
                }

                CmpCleanUpKcbCacheWithLock(KcbToFree);
            }
        } else {
            //
            // Free storage directly as there is no point in putting this on
            // our delayed close list.
            //
            //
            // Need to free all cached Index List, Index Leaf, Value, etc.
            //
            CmpCleanUpKcbCacheWithLock(KeyControlBlock);
        }
    }

    return;
}


VOID
CmpRemoveKeyControlBlock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
/*++

Routine Description:

    Remove a key control block from the KCB tree.

    It is expected that no notify control blocks remain.

    The kcb will NOT be freed, call DereferenceKeyControlBlock for that.

    This call assumes the KCB tree is already locked or registry is locked exclusively.

Arguments:

    KeyControlBlock - pointer to a key control block.

Return Value:

    NONE.

--*/
{
    ASSERT_KCB(KeyControlBlock);

    //
    // Remove the KCB from the hash table
    //
    CmpRemoveKeyHash(&KeyControlBlock->KeyHash);

    return;
}


VOID
CmpFreeKeyBody(
    PHHIVE Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Free storage for the key entry Hive.Cell refers to, including
    its class and security data.  Will NOT free child list or value list.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of key to free

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    PCELL_DATA key;

    //
    // map in the cell
    //
    key = HvGetCell(Hive, Cell);

    if (!(key->u.KeyNode.Flags & KEY_HIVE_EXIT)) {
        if (key->u.KeyNode.Security != HCELL_NIL) {
            HvFreeCell(Hive, key->u.KeyNode.Security);
        }

        if (key->u.KeyNode.ClassLength > 0) {
            HvFreeCell(Hive, key->u.KeyNode.Class);
        }
    }

    //
    // unmap the cell itself and free it
    //
    HvFreeCell(Hive, Cell);

    return;
}



PCM_KEY_CONTROL_BLOCK
CmpInsertKeyHash(
    IN PCM_KEY_HASH KeyHash,
    IN BOOLEAN      FakeKey
    )
/*++

Routine Description:

    Adds a key hash structure to the hash table. The hash table
    will be checked to see if a duplicate entry already exists. If
    a duplicate is found, its kcb will be returned. If a duplicate is not
    found, NULL will be returned.

Arguments:

    KeyHash - Supplies the key hash structure to be added.

Return Value:

    NULL - if the supplied key has was added
    PCM_KEY_HASH - The duplicate hash entry, if one was found

--*/

{
    HASH_VALUE Hash;
    ULONG Index;
    PCM_KEY_HASH Current;

    ASSERT_KEY_HASH(KeyHash);
    Index = GET_HASH_INDEX(KeyHash->ConvKey);

    //
    // If this is a fake key, we will use the cell and hive from its 
    // parent for uniqeness.  To deal with the case when the fake
    // has the same ConvKey as its parent (in which case we cannot distingish 
    // between the two), we set the lowest bit of the fake key's cell.
    //
    // It's possible (unlikely) that we cannot distingish two fake keys 
    // (when their Convkey's are the same) under the same key.  It is not breaking
    // anything, we just cannot find the other one in cache lookup.
    //
    //
    if (FakeKey) {
        KeyHash->KeyCell++;
    }

    //
    // First look for duplicates.
    //
    Current = CmpCacheTable[Index];
    while (Current) {
        ASSERT_KEY_HASH(Current);
        //
        // We must check ConvKey since we can create a fake kcb
        // for keys that does not exist.
        // We will use the Hive and Cell from the parent.
        //

        if ((KeyHash->ConvKey == Current->ConvKey) &&
            (KeyHash->KeyCell == Current->KeyCell) &&
            (KeyHash->KeyHive == Current->KeyHive)) {
            //
            // Found a match
            //
            return(CONTAINING_RECORD(Current,
                                     CM_KEY_CONTROL_BLOCK,
                                     KeyHash));
        }
        Current = Current->NextHash;
    }

#if DBG
    // 
    // Make sure this key is not somehow cached in the wrong spot.
    //
    {
        ULONG DbgIndex;
        PCM_KEY_CONTROL_BLOCK kcb;
        
        for (DbgIndex = 0; DbgIndex < CmpHashTableSize; DbgIndex++) {
            Current = CmpCacheTable[DbgIndex];
            while (Current) {
                kcb = CONTAINING_RECORD(Current,
                                        CM_KEY_CONTROL_BLOCK,
                                        KeyHash);
                
                ASSERT_KEY_HASH(Current);
                ASSERT((KeyHash->KeyHive != Current->KeyHive) ||
                       FakeKey ||
                       (kcb->ExtFlags & CM_KCB_KEY_NON_EXIST) ||
                       (KeyHash->KeyCell != Current->KeyCell));
                Current = Current->NextHash;
            }
        }
    }
    
#endif

    //
    // No duplicate was found, add this entry at the head of the list
    //
    KeyHash->NextHash = CmpCacheTable[Index];
    CmpCacheTable[Index] = KeyHash;
    return(NULL);
}


VOID
CmpRemoveKeyHash(
    IN PCM_KEY_HASH KeyHash
    )
/*++

Routine Description:

    Removes a key hash structure from the hash table.

Arguments:

    KeyHash - Supplies the key hash structure to be deleted.

Return Value:

    None

--*/

{
    HASH_VALUE Hash;
    ULONG Index;
    PCM_KEY_HASH *Prev;
    PCM_KEY_HASH Current;

    ASSERT_KEY_HASH(KeyHash);

    Hash = HASH_KEY(KeyHash->ConvKey);
    Index = Hash % CmpHashTableSize;

    //
    // Find this entry.
    //
    Prev = &CmpCacheTable[Index];
    while (TRUE) {
        Current = *Prev;
        ASSERT(Current != NULL);
        ASSERT_KEY_HASH(Current);
        if (Current == KeyHash) {
            *Prev = Current->NextHash;
#if DBG
            if (*Prev) {
                ASSERT_KEY_HASH(*Prev);
            }
#endif
            break;
        }
        Prev = &Current->NextHash;
    }
}


VOID
CmpInitializeCache()
{
    ULONG TotalCmCacheSize;
    ULONG i;

    TotalCmCacheSize = CmpHashTableSize * sizeof(PCM_KEY_HASH);

    CmpCacheTable = ExAllocatePoolWithTag(PagedPool,
                                          TotalCmCacheSize,
                                          'aCMC');
    if (CmpCacheTable == NULL) {
        KeBugCheckEx(CONFIG_INITIALIZATION_FAILED,6,1,0,0);
        return;
    }
    RtlZeroMemory(CmpCacheTable, TotalCmCacheSize);

    TotalCmCacheSize = CmpHashTableSize * sizeof(PCM_NAME_HASH);
    CmpNameCacheTable = ExAllocatePoolWithTag(PagedPool,
                                              TotalCmCacheSize,
                                              'aCMC');
    if (CmpNameCacheTable == NULL) {
        KeBugCheckEx(CONFIG_INITIALIZATION_FAILED,6,1,0,0);
        return;
    }
    RtlZeroMemory(CmpNameCacheTable, TotalCmCacheSize);

    CmpDelayedCloseTable = ExAllocatePoolWithTag(PagedPool,
                                                 CmpDelayedCloseSize * sizeof(PCM_KEY_CONTROL_BLOCK),
                                                 'cDMC');
    if (CmpDelayedCloseTable == NULL) {
        KeBugCheckEx(CONFIG_INITIALIZATION_FAILED,6,2,0,0);
        return;
    }
    CmpDelayedFreeIndex = 0;
    for (i=0; i<CmpDelayedCloseSize-1; i++) {
        CmpDelayedCloseTable[i] = (PCM_KEY_CONTROL_BLOCK)ULongToPtr(i+1);
    }
    CmpDelayedCloseTable[CmpDelayedCloseSize-1] = (PCM_KEY_CONTROL_BLOCK)-1;
    CmpDelayedCloseCurrent = CmpDelayedCloseTable;
}
