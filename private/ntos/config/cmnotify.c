/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmnotify.c

Abstract:

    This module contains support for NtNotifyChangeKey.

Author:

    Bryan M. Willman (bryanwi) 03-Feb-1992

Revision History:

    Dragos C. Sambotin (dragoss) 16-Mar-1999
        - fixing race conditions that when more than one thread simultaneously operates over the post list
--*/


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                      //
//   "The" POST BLOCK RULE :                                                                                            //
//                                                                                                                      //
//      To operate on a post block (i.e. add or remove it from a list - notify,thread,slave),                           //
//      you should at least:                                                                                            //
//          1. Hold the registry lock exclusively                                                                       //
//                     OR                                                                                               //
//          2. Hold the registry lock shared and aquire the postblock mutex.                                            //
//                                                                                                                      //
//                                                                                                                      //
//      WARNING!!!                                                                                                      //
//          Failing to do that could arise in obscure registry deadlocks or usage of already freed memory (bugcheck)    //
//                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                      //
//  Other important rules to follow:                                                                                    //
//                                                                                                                      //
//      1. We DO NOT dereference objects in CmpPostApc !                                                                //
//      2. We DO NOT dereference objects while walking the notify list!                                                 //
//      3. All operations with Thread PostList are done in CmpPostApc or at APC level. This should avoid two threads    //
//          operating on the same list at the same time                                                                 //
//                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include    "cmp.h"

#if DBG
/*++
Routine Description:
    Check if the post block or it's slave (if any) has no reference 
    to any key body object
++*/

#define CmpCheckPostBlock(PostBlock )                                               \
    {                                                                               \
        PCM_POST_BLOCK  SlavePostBlock;                                             \
                                                                                    \
        /* this post block should have the link with key body already broken*/      \
        ASSERT( PostBlock->PostKeyBody == NULL );                                   \
                                                                                    \
        /* only masters get to CmpPostApc */                                        \
        ASSERT( IsMasterPostBlock(PostBlock) );                             \
                                                                                    \
        if (IsListEmpty(&(PostBlock->CancelPostList)) == FALSE) {                   \
                                                                                    \
            /* get the slave and verify him too */                                  \
            SlavePostBlock = (PCM_POST_BLOCK)PostBlock->CancelPostList.Flink;       \
            SlavePostBlock = CONTAINING_RECORD(SlavePostBlock,                      \
                                               CM_POST_BLOCK,                       \
                                               CancelPostList);                     \
            /* This should be true !*/                                              \
            ASSERT( !IsMasterPostBlock(SlavePostBlock) );                           \
                                                                                    \
            /* this post block shoul have the link with key body already broken */  \
            ASSERT( SlavePostBlock->PostKeyBody == NULL );                          \
        }                                                                           \
    }
#else
#define CmpCheckPostBlock(a) //nothing
#endif


//
// "Back Side" of notify
//

extern  PCMHIVE  CmpMasterHive;

VOID
CmpReportNotifyHelper(
    PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN PHHIVE SearchHive,
    IN PHHIVE Hive,
    IN PCM_KEY_NODE Node,
    IN ULONG Filter
    );

VOID
CmpCancelSlavePost(
    PCM_POST_BLOCK  PostBlock,
    PLIST_ENTRY     DelayedDeref
    );

VOID
CmpFreeSlavePost(
    PCM_POST_BLOCK  MasterPostBlock
    );

VOID
CmpAddToDelayedDeref(
    PCM_POST_BLOCK  PostBlock,
    PLIST_ENTRY     DelayedDeref
    );

VOID
CmpDelayedDerefKeys(
                    PLIST_ENTRY DelayedDeref
                    );

BOOLEAN
CmpNotifyTriggerCheck(
    IN PCM_NOTIFY_BLOCK NotifyBlock,
    IN PHHIVE Hive,
    IN PCM_KEY_NODE Node
    );

VOID
CmpDummyApc(
    struct _KAPC *Apc,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpReportNotify)
#pragma alloc_text(PAGE,CmpReportNotifyHelper)
#pragma alloc_text(PAGE,CmpPostNotify)
#pragma alloc_text(PAGE,CmpPostApc)
#pragma alloc_text(PAGE,CmpPostApcRunDown)
#pragma alloc_text(PAGE,CmNotifyRunDown)
#pragma alloc_text(PAGE,CmpFlushNotify)
#pragma alloc_text(PAGE,CmpNotifyChangeKey)
#pragma alloc_text(PAGE,CmpCancelSlavePost)
#pragma alloc_text(PAGE,CmpFreeSlavePost)
#pragma alloc_text(PAGE,CmpAddToDelayedDeref)
#pragma alloc_text(PAGE,CmpDelayedDerefKeys)
#pragma alloc_text(PAGE,CmpNotifyTriggerCheck)
#pragma alloc_text(PAGE,CmpDummyApc)
#endif


VOID
CmpDummyApc(
    struct _KAPC *Apc,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
    )
/*++

Routine Description:

    Dummy routine to prevent user-mode callers to set special kernel apcs

Arguments:

    Apc - pointer to apc object

    SystemArgument1 -  IN: Status value for IoStatusBlock
                      OUT: Ptr to IoStatusBlock (2nd arg to user apc routine)

    SystemArgument2 - Pointer to the PostBlock

Return Value:

    NONE.

--*/
{
    UNREFERENCED_PARAMETER(Apc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);
}

VOID
CmpReportNotify(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock,
    PHHIVE                  Hive,
    HCELL_INDEX             Cell,
    ULONG                   Filter
    )
/*++

Routine Description:

    This routine is called when a notifiable event occurs. It will
    apply CmpReportNotifyHelper to the hive the event occured in,
    and the master hive if different.

Arguments:

    KeyControlBlock - KCB of the key at which the event occured.
            For create or delete this is the created or deleted key.

    Hive - pointer to hive containing cell of Key at which event occured.

    Cell - cell of Key at which event occured

            (hive and cell correspond with name.)

    Filter - event to be reported

Return Value:

    NONE.

--*/
{
    PCM_KEY_NODE pcell;
    ULONG       flags;
    ULONG       i;

    PAGED_CODE();
    CMLOG(CML_WORKER, CMS_NOTIFY) {
        KdPrint(("CmpReportNotify:\n"));
        KdPrint(("\tHive:%08lx Cell:%08lx Filter:%08lx\n", Hive, Cell, Filter));
    }

    pcell = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    //
    // If the operation was create or delete, treat it as a change
    // to the parent.
    //
    if (Filter == REG_NOTIFY_CHANGE_NAME) {
        flags = pcell->Flags;
        Cell = pcell->Parent;
        if (flags & KEY_HIVE_ENTRY) {
            Hive = &(CmpMasterHive->Hive);
            pcell = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
        }


        KeyControlBlock = KeyControlBlock->ParentKcb;

        //
        // if we're at an exit/link node, back up the real node
        // that MUST be it's parent.
        //
        if (pcell->Flags & KEY_HIVE_EXIT) {
            Cell = pcell->Parent;
        }
        pcell = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    }

    //
    // Report to notifies waiting on the event's hive
    //
    CmpReportNotifyHelper(KeyControlBlock, Hive, Hive, pcell, Filter);


    //
    // If containing hive is not the master hive, apply to master hive
    //
    if (Hive != &(CmpMasterHive->Hive)) {
        CmpReportNotifyHelper(KeyControlBlock,
                              &(CmpMasterHive->Hive),
                              Hive,
                              pcell,
                              Filter);
    }

    return;
}

BOOLEAN
CmpNotifyTriggerCheck(
    IN PCM_NOTIFY_BLOCK NotifyBlock,
    IN PHHIVE Hive,
    IN PCM_KEY_NODE Node
    )
/*++

Routine Description:

    Checks if a notify can be triggered

Arguments:

    NotifyBlock - the notify block

    Hive - Supplies hive containing node to match with.

    Node - pointer to key to match with (and check access to)


Return Value:

    TRUE - yes.
    FALSE - no

--*/
{
    PCM_POST_BLOCK PostBlock;
    POST_BLOCK_TYPE NotifyType;

    PAGED_CODE();

    if(IsListEmpty(&(NotifyBlock->PostList)) == FALSE) {

        //
        // check if it is a kernel notify. Look at the first post block
        // to see that. If is a kernel post-block, then all posts in 
        // the list should be kernel notifies
        //
        PostBlock = (PCM_POST_BLOCK)NotifyBlock->PostList.Flink;
        PostBlock = CONTAINING_RECORD(PostBlock,
                                      CM_POST_BLOCK,
                                      NotifyList);

        NotifyType = PostBlockType(PostBlock);

        if( NotifyType == PostAsyncKernel ) {
            // this is a kernel notify; always trigger it
#if DBG
            //
            // DEBUG only code: All post blocks should be of the same type
            // (kernel/user)
            //
            while( PostBlock->NotifyList.Flink != &(NotifyBlock->PostList) ) {
                PostBlock = (PCM_POST_BLOCK)PostBlock->NotifyList.Flink;
                PostBlock = CONTAINING_RECORD(PostBlock,
                                            CM_POST_BLOCK,
                                            NotifyList);
                
                //KdPrint(("CmpNotifyTriggerCheck : NotifyBlock = %lx\n",NotifyBlock));
                
                ASSERT( PostBlockType(PostBlock) == NotifyType );
            }
#endif
        
            return TRUE;
        }
    }

    //
    // else, check if the caller has the right access
    //
    return CmpCheckNotifyAccess(NotifyBlock,Hive,Node);
}

VOID
CmpReportNotifyHelper(
    PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN PHHIVE SearchHive,
    IN PHHIVE Hive,
    IN PCM_KEY_NODE Node,
    IN ULONG Filter
    )
/*++

Routine Description:

    Scan the list of active notifies for the specified hive.  For
    any with scope including KeyControlBlock and filter matching
    Filter, and with proper security access, post the notify.

Arguments:

    Name - canonical path name (as in a key control block) of the key
            at which the event occured.  (This is the name for
            reporting purposes.)

    SearchHive - hive to search for matches (which notify list to check)

    Hive - Supplies hive containing node to match with.

    Node - pointer to key to match with (and check access to)

    Filter - type of event

Return Value:

    NONE.

--*/
{
    PLIST_ENTRY         NotifyPtr;
    PCM_NOTIFY_BLOCK    NotifyBlock;
    PCMHIVE             CmSearchHive;
    PUNICODE_STRING     NotifyName;
    KIRQL               OldIrql;
    LIST_ENTRY          DelayedDeref;

    PAGED_CODE();

    KeRaiseIrql(APC_LEVEL, &OldIrql);

    CmSearchHive = CONTAINING_RECORD(SearchHive, CMHIVE, Hive);

    NotifyPtr = &(CmSearchHive->NotifyList);

    InitializeListHead(&(DelayedDeref));

    while (NotifyPtr->Flink != NULL) {

        NotifyPtr = NotifyPtr->Flink;

        NotifyBlock = CONTAINING_RECORD(NotifyPtr, CM_NOTIFY_BLOCK, HiveList);
        if (NotifyBlock->KeyControlBlock->TotalLevels > KeyControlBlock->TotalLevels) {
            //
            // list is level sorted, we're past all shorter entries
            //
            break;
        } else {
            PCM_KEY_CONTROL_BLOCK kcb;
            USHORT LevelDiff, l;

            LevelDiff = KeyControlBlock->TotalLevels - NotifyBlock->KeyControlBlock->TotalLevels;

            kcb = KeyControlBlock;
            for (l=0; l<LevelDiff; l++) {
                kcb = kcb->ParentKcb;
            }

            if (kcb == NotifyBlock->KeyControlBlock) {
                //
                // This Notify path is the prefix of this kcb.
                //
                if ((NotifyBlock->Filter & Filter)
                            &&
                    ((NotifyBlock->WatchTree == TRUE) ||
                     (Node == NotifyBlock->KeyControlBlock->KeyNode))
                   )
                {
                    // Filter matches, this event is relevent to this notify
                    //                  AND
                    // Either the notify spans the whole subtree, or the cell
                    // (key) of interest is the one it applies to
                    //
                    // THEREFORE:   The notify is relevent.
                    //

                    //
                    // Correct scope, does caller have access?
                    //
                    if (CmpNotifyTriggerCheck(NotifyBlock,Hive,Node)) {
                        //
                        // Notify block has KEY_NOTIFY access to the node
                        // the event occured at.  It is relevent.  Therefore,
                        // it gets to see this event.  Post and be done.
                        //
                        // we specify that we want no key body dereferenciation 
                        // during the CmpPostNotify call. This is to prevent the 
                        // deletion of the current notify block
                        //
                        CmpPostNotify(
                            NotifyBlock,
                            NULL,
                            Filter,
                            STATUS_NOTIFY_ENUM_DIR,
                            &DelayedDeref
                            );

                    }  // else no KEY_NOTIFY access to node event occured at
                } // else not relevent (wrong scope, filter, etc)
            }
        }
    }
    
    //
    // finish the job started in CmpPostNotify (i.e. dereference the keybodies
    // we prevented. this may cause some notifyblocks to be freed
    //
    CmpDelayedDerefKeys(&DelayedDeref);
    
    KeLowerIrql(OldIrql);
    return;
}


VOID
CmpPostNotify(
    PCM_NOTIFY_BLOCK    NotifyBlock,
    PUNICODE_STRING     Name OPTIONAL,
    ULONG               Filter,
    NTSTATUS            Status,
    PLIST_ENTRY         ExternalKeyDeref OPTIONAL
    )
/*++

Routine Description:

    Actually report the notify event by signalling events, enqueing
    APCs, and so forth.

    When Status is STATUS_NOTIFY_CLEANUP:

      - if the post block is a slave one, just cancel it.
      - if the post block is a master one, cancel all slave post blocks
        and trigger event on the master block.

Comments:
    
    This routine is using a "delayed dereferencing" technique to prevent
    deadlocks that may appear when a keybody is dereferenced while holding
    the post block lock. As for this, a list with keybodies that have to be 
    dereferenced is constructed while walking the list of postblocks attached
    to the current notify block and the related (slave or master) post blocks.
    The list is built by tricking postblocks. For all postblock about to be 
    freed the PostKeyBody member is added to the local list and then set to NULL
    on the postblock. This will avoid the key body dereferencing in CmpFreePostBlock.
    Instead, after the postblock lock is released, the local list is iterated and 
    the keybodies are dereferenced and the storage for associated CM_POST_KEY_BODY 
    objects is freed.

  
Arguments:

    NotifyBlock - pointer to structure that describes the notify
                  operation.  (Where to post to)

    Name - name of key at which event occurred.

    Filter - nature of event

    Status - completion status to report

    ExternalKeyDeref - this parameter (when not NULL) specifies that the caller doesn't 
                    want any keybody to be dereferenced while in this routine

Return Value:

    NONE.

--*/
{
    PCM_POST_BLOCK      PostBlock;
    PCM_POST_BLOCK      SlavePostBlock;
    LIST_ENTRY          LocalDelayedDeref;
    KIRQL               OldIrql;
    PLIST_ENTRY         DelayedDeref;

    Filter;
    Name;

    PAGED_CODE();
    CMLOG(CML_MAJOR, CMS_NOTIFY) {
        KdPrint(("CmpPostNotify:\n"));
        KdPrint(("\tNotifyBlock:%08lx  ", NotifyBlock));
        KdPrint(("\tName = %wZ\n", Name));
        KdPrint(("\tFilter:%08lx  Status=%08lx\n", Filter, Status));
    }
    ASSERT_CM_LOCK_OWNED();

    if( ARGUMENT_PRESENT(ExternalKeyDeref) ) {
        //
        // The caller want to do all keybody dereferencing by himself
        //
        DelayedDeref = ExternalKeyDeref;
    } else {
        // local delayed dereferencing (the caller doesn't care!)
        DelayedDeref = &LocalDelayedDeref;
        InitializeListHead(DelayedDeref);
    }

    //
    // Aquire exclusive access over the postlist(s)
    //
    LOCK_POST_LIST();

    if (IsListEmpty(&(NotifyBlock->PostList)) == TRUE) {
        //
        // Nothing to post, set a mark and return
        //
        NotifyBlock->NotifyPending = TRUE;
        UNLOCK_POST_LIST();
        return;
    }
    NotifyBlock->NotifyPending = FALSE;

    //
    // IMPLEMENTATION NOTE:
    //      If we ever want to actually implement the code that returns
    //      names of things that changed, this is the place to add the
    //      name and operation type to the buffer.
    //

    //
    // Pull and post all the entries in the post list
    //
    while (IsListEmpty(&(NotifyBlock->PostList)) == FALSE) {

        //
        // Remove from the notify block list, and enqueue the apc.
        // The apc will remove itself from the thread list
        //
        PostBlock = (PCM_POST_BLOCK)RemoveHeadList(&(NotifyBlock->PostList));
        PostBlock = CONTAINING_RECORD(PostBlock,
                                      CM_POST_BLOCK,
                                      NotifyList);

        // Protect for multiple deletion of the same object
        CmpClearListEntry(&(PostBlock->NotifyList));
        
        CMLOG(CML_MAJOR, CMS_NOTIFY) {
#if DBG
            if(PostBlock->TraceIntoDebugger) {
                WCHAR                   *NameBuffer = NULL;
                UNICODE_STRING          KeyName;

                NameBuffer = ExAllocatePool(PagedPool, MAX_KEY_NAME_LENGTH);
                if(NameBuffer) {
                   CmpInitializeKeyNameString(NotifyBlock->KeyControlBlock->KeyNode,&KeyName,NameBuffer);
                   KdPrint(("[CM]CmpPostNotify: NotifyBlock:%08lx\tKey = %.*S\n",NotifyBlock,KeyName.Length / sizeof(WCHAR),KeyName.Buffer));
                   ExFreePool(NameBuffer);
                }
                KdPrint(("[CM]\tCmpPostNotify: PostBlock:%08lx\n", PostBlock));
            }
#endif
        }

        if( (Status == STATUS_NOTIFY_CLEANUP) && !IsMasterPostBlock(PostBlock) ) {
            //
            // Cleanup notification (i.e. the key handle was closed or the key was deleted)
            // When the post is a slave one, just cancel it. Canceling means:
            //      1. Removing from the notify PostList (aldready done at this point - see above)
            //      2. Unchaining from the Master Block CancelPostList
            //      3. Delisting from the thread PostBlockList
            //      4. Actually freeing the memory
            //

            // Use Cmp variant to protect for multiple deletion of the same object
            CmpRemoveEntryList(&(PostBlock->CancelPostList));
            //
            // FIX 289351
            //
            // Use Cmp variant to protect for multiple deletion of the same object
            KeRaiseIrql(APC_LEVEL, &OldIrql);
            CmpRemoveEntryList(&(PostBlock->ThreadList));
            KeLowerIrql(OldIrql);

            if( PostBlock->NotifyType != PostSynchronous ) {

                // add to the deref list and clean the post block
                CmpAddToDelayedDeref(PostBlock,DelayedDeref);

                //
                // Front-end routine will do self cleanup for syncrounous notifications
                CmpFreePostBlock(PostBlock);
            }

            CMLOG(CML_MAJOR, CMS_NOTIFY) {
#if DBG
                if(PostBlock->TraceIntoDebugger) {
                    KdPrint(("[CM]\tCmpPostNotify: PostBlock:%08lx is a slave block,and notify is CLEANUP==> just cleanning\n", PostBlock));
                }
#endif
            }

            continue; //try the next one
        }

        //
        // Simulate that this block is the master one, so we can free the others
        // Doing that will ensure the right memory dealocation when the master
        // (from now on this block) will be freed.
        //
        if(!IsMasterPostBlock(PostBlock)) {
            //
            // oops.,this is not the master block, we have some more work to do
            //
            SlavePostBlock = PostBlock;
            do {
                SlavePostBlock = (PCM_POST_BLOCK)SlavePostBlock->CancelPostList.Flink;
                SlavePostBlock = CONTAINING_RECORD(SlavePostBlock,
                                                   CM_POST_BLOCK,
                                                   CancelPostList);
                //
                // reset the "master flag" if set
                //
                ClearMasterPostBlockFlag(SlavePostBlock);
            } while (SlavePostBlock != PostBlock);

            //
            // Make this post block the master one
            //
            SetMasterPostBlockFlag(PostBlock);
        }

        CMLOG(CML_MAJOR, CMS_NOTIFY) {
#if DBG
            if(PostBlock->TraceIntoDebugger) {
                KdPrint(("[CM]\tCmpPostNotify: Master block switched to :%08lx\n", PostBlock));
            }
#endif
        }

        //
        // Cancel all slave Post requests that may be linked to self
        //

        if( PostBlockType(PostBlock) != PostSynchronous ) {
            //
            // Front-end routine will do self cleanup for syncrounous notifications
            CmpCancelSlavePost(PostBlock,DelayedDeref);
            //
            // Do the same for the master (in case master and slave got switched)
            // This will avoid dereferencing the keybody from CmpPostApc
            CmpAddToDelayedDeref(PostBlock,DelayedDeref);
        }

        switch (PostBlockType(PostBlock)) {
            case PostSynchronous:
                //
                // This is a SYNC notify call.  There will be no user event,
                // and no user apc routine.  Quick exit here, just fill in
                // the Status and poke the event.
                //
                // Holder of the systemevent will wake up and free the
                // postblock.  If we free it here, we get a race & bugcheck.
                //
                // Set the flink to NULL so that the front side can tell this
                // has been removed if its wait aborts.
                //
                PostBlock->NotifyList.Flink = NULL;
                PostBlock->u->Sync.Status = Status;
                KeSetEvent(PostBlock->u->Sync.SystemEvent,
                           0,
                           FALSE);
                break;

            case PostAsyncUser:
                //
                // Insert the APC into the queue
                //
                KeInsertQueueApc(PostBlock->u->AsyncUser.Apc,
                                 (PVOID)ULongToPtr(Status),
                                 (PVOID)PostBlock,
                                 0);
                break;

            case PostAsyncKernel:
                //
                // Queue the work item, then free the post block.
                //
                if (PostBlock->u->AsyncKernel.WorkItem != NULL) {
                    ExQueueWorkItem(PostBlock->u->AsyncKernel.WorkItem,
                                    PostBlock->u->AsyncKernel.QueueType);
                }
                //
                // Signal Event if present, and deref it.
                //
                if (PostBlock->u->AsyncKernel.Event != NULL) {
                    KeSetEvent(PostBlock->u->AsyncKernel.Event,
                               0,
                               FALSE);
                    ObDereferenceObject(PostBlock->u->AsyncKernel.Event);
                }

				//
				// Multiple async kernel notification are not allowed
				//
				ASSERT(IsListEmpty(&(PostBlock->CancelPostList)) == TRUE);
				//
                // remove the post block from the thread list, and free it
                //
                // Use Cmp variant to protect for multiple deletion of the same object
                KeRaiseIrql(APC_LEVEL, &OldIrql);
                CmpRemoveEntryList(&(PostBlock->ThreadList));
                KeLowerIrql(OldIrql);
                
                // it was already added to delayed deref.
                CmpFreePostBlock(PostBlock);
                break;
        }
    }

    UNLOCK_POST_LIST();

    //
    // At this point we have a list of keybody elements that have to be dereferenciated
    // and the associated storage for the covering objects freed. The keybodies in this 
    // list have only one reference count on them (they were referenced only in 
    // NtNotifyChangeMultipleKeys), dereferencing them here should free the object
    //

    if( ARGUMENT_PRESENT(ExternalKeyDeref) ) {
        // do nothing; the caller wants to handle the dereferenciation by himself!
    } else {
        // dereferenciate all keybodies in the delayed list
        CmpDelayedDerefKeys(DelayedDeref);
    }
   
    return;
}


VOID
CmpPostApc(
    struct _KAPC *Apc,
    PKNORMAL_ROUTINE *NormalRoutine,
    PVOID *NormalContext,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
    )
/*++

Routine Description:

    This is the kernel apc routine.  It is called for all notifies,
    regardless of what form of notification the caller requested.

    We compute the postblock address from the apc object address.
    IoStatus is set.  SystemEvent and UserEvent will be signalled
    as appropriate.  If the user requested an APC, then NormalRoutine
    will be set at entry and executed when we exit.  The PostBlock
    is freed here.

Arguments:

    Apc - pointer to apc object

    NormalRoutine - Will be called when we return

    NormalContext - will be 1st argument to normal routine, ApcContext
                    passed in when NtNotifyChangeKey was called

    SystemArgument1 -  IN: Status value for IoStatusBlock
                      OUT: Ptr to IoStatusBlock (2nd arg to user apc routine)

    SystemArgument2 - Pointer to the PostBlock

Return Value:

    NONE.

--*/
{
    PCM_POST_BLOCK  PostBlock;

    PAGED_CODE();
    CMLOG(CML_MAJOR, CMS_NOTIFY) {
        KdPrint(("CmpPostApc:\n"));
        KdPrint(("\tApc:%08lx ", Apc));
        KdPrint(("NormalRoutine:%08lx\n", NormalRoutine));
        KdPrint(("\tNormalContext:%08lx", NormalContext));
        KdPrint(("\tSystemArgument1=IoStatusBlock:%08lx\n", SystemArgument1));
    }


    PostBlock = *(PCM_POST_BLOCK *)SystemArgument2;

    CMLOG(CML_MAJOR, CMS_NOTIFY) {
#if DBG
        if(PostBlock->TraceIntoDebugger) {
            KdPrint(("[CM]CmpPostApc: PostBlock:%08lx\n", PostBlock));
        }
#endif
    }
    //
    // Fill in IO Status Block
    //
    // IMPLEMENTATION NOTE:
    //      If we ever want to actually implement the code that returns
    //      names of things that changed, this is the place to copy the
    //      buffer into the caller's buffer.
    //
    //  Sundown only: Use a 32bit IO_STATUS_BLOCK if the caller is 32bit.

    try {
        CmpSetIoStatus(PostBlock->u->AsyncUser.IoStatusBlock, 
                       *((ULONG *)SystemArgument1), 
                       0L,
                       PsGetCurrentProcess()->Wow64Process != NULL);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }
    *SystemArgument1 = PostBlock->u->AsyncUser.IoStatusBlock;

    //
    // This is an Async notify, do all work here, including
    // cleaning up the post block
    //

    //
    // Signal UserEvent if present, and deref it.
    //
    if (PostBlock->u->AsyncUser.UserEvent != NULL) {
        KeSetEvent(PostBlock->u->AsyncUser.UserEvent,
                   0,
                   FALSE);
        ObDereferenceObject(PostBlock->u->AsyncUser.UserEvent);
    }

    //
    // remove the post block from the thread list, and free it
    //
    // Use Cmp variant to protect for multiple deletion of the same object
    CmpRemoveEntryList(&(PostBlock->ThreadList));

    // debug only checks
    CmpCheckPostBlock(PostBlock);
    //
	// Free the slave post block to avoid "dangling" postblocks
	//
	CmpFreeSlavePost(PostBlock);
    //
	// free this post block
	// 
	CmpFreePostBlock(PostBlock);

    return;
}


VOID
CmpPostApcRunDown(
    struct _KAPC *Apc
    )
/*++

Routine Description:

    This routine is called to clear away apcs in the apc queue
    of a thread that has been terminated.

    Since the apc is in the apc queue, we know that it is NOT in
    any NotifyBlock's post list.  It is, however, in the threads's
    PostBlockList.

    Therefore, poke any user events so that waiters are not stuck,
    drop the references so the event can be cleaned up, delist the
    PostBlock and free it.

    Since we are cleaning up the thread, SystemEvents are not interesting.

    Since the apc is in the apc queue, we know that if there were any other
    notifications related to this one, they are cleaned up by the
    CmPostNotify routine

Arguments:

    Apc - pointer to apc object

Return Value:

    NONE.

--*/
{
    PCM_POST_BLOCK  PostBlock;
    KIRQL           OldIrql;

    PAGED_CODE();
    CMLOG(CML_MAJOR, CMS_NOTIFY) {
        KdPrint(("CmpApcRunDown:"));
        KdPrint(("\tApc:%08lx \n", Apc));
    }

    KeRaiseIrql(APC_LEVEL, &OldIrql);

    PostBlock = (PCM_POST_BLOCK)Apc->SystemArgument2;

    CMLOG(CML_MAJOR, CMS_NOTIFY) {
#if DBG
        if(PostBlock->TraceIntoDebugger) {
            KdPrint(("[CM]CmpPostApcRunDown: PostBlock:%08lx\n", PostBlock));
        }
#endif
    }

    //
    // report status and wake up any threads that might otherwise
    // be stuck.  also drop any event references we hold
    //
    //  Sundown only: Use a 32bit IO_STATUS_BLOCK if the caller is 32bit. 

    try {
        CmpSetIoStatus(PostBlock->u->AsyncUser.IoStatusBlock, 
                       STATUS_NOTIFY_CLEANUP, 
                       0L, 
                       PsGetCurrentProcess()->Wow64Process != NULL);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

    if (PostBlock->u->AsyncUser.UserEvent != NULL) {
        KeSetEvent(
            PostBlock->u->AsyncUser.UserEvent,
            0,
            FALSE
            );
        ObDereferenceObject(PostBlock->u->AsyncUser.UserEvent);
    }

    //
    // delist the post block
    //
    // Use Cmp variant to protect for multiple deletion of the same object
    CmpRemoveEntryList(&(PostBlock->ThreadList));

	//
	// Free the slave post block to avoid "dangling" postblocks
	//
	CmpFreeSlavePost(PostBlock);
    //
    // Free the post block.  Use Ex call because PostBlocks are NOT
    // part of the global registry pool computation, but are instead
    // part of NonPagedPool with Quota.
    //
    CmpFreePostBlock(PostBlock);

    KeLowerIrql(OldIrql);

    return;
}


//
// Cleanup procedure
//
VOID
CmNotifyRunDown(
    PETHREAD    Thread
    )
/*++

Routine Description:

    This routine is called from PspExitThread to clean up any pending
    notify requests.

    It will traverse the thread's PostBlockList, for each PostBlock it
    finds, it will:

        1.  Remove it from the relevent NotifyBlock.  This requires
            that we hold the Registry mutex.

        2.  Remove it from the thread's PostBlockList.  This requires
            that we run at APC level.

        3.  By the time this procedure runs, user apcs are not interesting
            and neither are SystemEvents, so do not bother processing
            them.

            UserEvents and IoStatusBlocks could be refered to by other
            threads in the same process, or even a different process,
            so process them so those threads know what happened, use
            status code of STATUS_NOTIFY_CLEANUP.

            If the notify is a master one, cancel all slave notifications.
            Else only remove this notification from the master CancelPortList

        4.  Free the post block.

Arguments:

    Thread - pointer to the executive thread object for the thread
             we wish to do rundown on.

Return Value:

    NONE.

--*/
{
    PCM_POST_BLOCK      PostBlock;
    PCM_NOTIFY_BLOCK    NotifyBlock;
    KIRQL               OldIrql;

    PAGED_CODE();

    if ( IsListEmpty(&(Thread->PostBlockList)) == TRUE ) {
        return;
    }

    CMLOG(CML_API, CMS_NTAPI) {
        KdPrint(("CmNotifyRunDown: ethread:%08lx\n", Thread));
    }

    CmpLockRegistryExclusive();

	//
    // Aquire exclusive access over the postlist(s)
    //
    // This is not needed (see the rule above)
    //LOCK_POST_LIST(); 

    KeRaiseIrql(APC_LEVEL, &OldIrql);
    while (IsListEmpty(&(Thread->PostBlockList)) == FALSE) {

        //
        // remove from thread list
        //
        PostBlock = (PCM_POST_BLOCK)RemoveHeadList(&(Thread->PostBlockList));
        PostBlock = CONTAINING_RECORD(
                        PostBlock,
                        CM_POST_BLOCK,
                        ThreadList
                        );

        // Protect for multiple deletion of the same object
        CmpClearListEntry(&(PostBlock->ThreadList));

        CMLOG(CML_API, CMS_NTAPI) {
#if DBG
            if(PostBlock->TraceIntoDebugger) {
                KdPrint(("[CM]CmpNotifyRunDown: ethread:%08lx, PostBlock:%08lx\n", Thread,PostBlock));
            }
#endif
        }

        //
        // Canceling a master notification implies canceling all the slave notifications
        // from the CancelPostList
        //
        if(IsMasterPostBlock(PostBlock)) {
            CMLOG(CML_API, CMS_NTAPI) {
#if DBG
                if(PostBlock->TraceIntoDebugger) {
                        KdPrint(("[CM]\tCmpNotifyRunDown: PostBlock:%08lx is a master block\n", PostBlock));
                }
#endif
            }
            //
            // at this point, CmpReportNotify and friends will no longer
            // attempt to post this post block.
            //
            if (PostBlockType(PostBlock) == PostAsyncUser) {
                //
                // report status and wake up any threads that might otherwise
                // be stuck.  also drop any event references we hold
                //
                //  Sundown only: Use a 32bit IO_STATUS_BLOCK if the caller is 32bit. 

                try {
                    CmpSetIoStatus(PostBlock->u->AsyncUser.IoStatusBlock, 
                                   STATUS_NOTIFY_CLEANUP, 
                                   0L, 
                                   PsGetCurrentProcess()->Wow64Process != NULL);
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    CMLOG(CML_API, CMS_EXCEPTION) {
                        KdPrint(("!!CmNotifyRundown: code:%08lx\n", GetExceptionCode()));
                    }
                    NOTHING;
                }

                if (PostBlock->u->AsyncUser.UserEvent != NULL) {
                    KeSetEvent(
                        PostBlock->u->AsyncUser.UserEvent,
                        0,
                        FALSE
                        );
                    ObDereferenceObject(PostBlock->u->AsyncUser.UserEvent);
                }

                //
                // Cancel the APC. Otherwise the rundown routine will also
                // free the post block if the APC happens to be queued at
                // this point. If the APC is queued, then the post block has
                // already been removed from the notify list, so don't remove
                // it again.
                //
                if (!KeRemoveQueueApc(PostBlock->u->AsyncUser.Apc)) {

                    //
                    // remove from notify block's list
                    //
                    // Use Cmp variant to protect for multiple deletion of the same object
                    CmpRemoveEntryList(&(PostBlock->NotifyList));
                }
            } else {
                //
                // remove from notify block's list
                //
                // Use Cmp variant to protect for multiple deletion of the same object
                CmpRemoveEntryList(&(PostBlock->NotifyList));
            }

            //
            // Cancel all slave Post requests that may be linked to self
            //
            CmpCancelSlavePost(PostBlock,NULL); // we do not want delayed deref
			//
			// Free the slave Post blocks too
			//
			CmpFreeSlavePost(PostBlock);
        } else {

            CMLOG(CML_API, CMS_NTAPI) {
#if DBG
                if(PostBlock->TraceIntoDebugger) {
                    KdPrint(("[CM]\tCmpNotifyRunDown: PostBlock:%08lx is a slave block\n", PostBlock));
                }
#endif
            }
            //
            // Is a slave PostBlock, just remove self from the Notify PostList
            //
            // Use Cmp variant to protect for multiple deletion of the same object
            CmpRemoveEntryList(&(PostBlock->NotifyList));

            //
            // and unchain from the Master CancelPostList
            //
            // Use Cmp variant to protect for multiple deletion of the same object
            CmpRemoveEntryList(&(PostBlock->CancelPostList));

        }

        //
        // Free the post block.  Use Ex call because PostBlocks are NOT
        // part of the global registry pool computation, but are instead
        // part of NonPagedPool with Quota.
        //
        CmpFreePostBlock(PostBlock);
    }

    KeLowerIrql(OldIrql);

    // This is not needed (see the rule above)
    //UNLOCK_POST_LIST();

    CmpUnlockRegistry();
    return;
}


VOID
CmpFlushNotify(
    PCM_KEY_BODY        KeyBody
    )
/*++

Routine Description:

    Clean up notifyblock when a handle is closed or the key it refers
    to is deleted.

Arguments:

    KeyBody - supplies pointer to key object body for handle we
                are cleaning up.

Return Value:

    NONE

--*/
{
    PCM_NOTIFY_BLOCK    NotifyBlock;
    PCMHIVE             Hive;

    PAGED_CODE();
    ASSERT_CM_LOCK_OWNED();

    if (KeyBody->NotifyBlock == NULL) {
        return;
    }

#ifdef KCB_TO_KEYBODY_LINK
    ASSERT( KeyBody->KeyControlBlock->Delete == FALSE );
#endif

    CMLOG(CML_MAJOR, CMS_NOTIFY) {
#if DBG
        WCHAR                   *NameBuffer = NULL;
        UNICODE_STRING          KeyName;

        KdPrint(("[CM]CmpFlushNotify: NotifyBlock = %08lx\n",KeyBody->NotifyBlock));
        NameBuffer = ExAllocatePool(PagedPool, MAX_KEY_NAME_LENGTH);
        if(NameBuffer && KeyBody->KeyControlBlock->KeyNode) {
           CmpInitializeKeyNameString(KeyBody->KeyControlBlock->KeyNode,&KeyName,NameBuffer);
           KdPrint(("\t[CM]CmpFlushNotify: Key = %.*S\n",KeyName.Length / sizeof(WCHAR),KeyName.Buffer));
           ExFreePool(NameBuffer);
        }
#endif
    }

    //
    // Lock the hive exclusively to prevent multiple threads from whacking
    // on the list.
    //
    Hive = CONTAINING_RECORD(KeyBody->KeyControlBlock->KeyHive,
                             CMHIVE,
                             Hive);
    CmLockHive(Hive);
    //
    // Reread the notify block in case it has already been freed.
    //
    NotifyBlock = KeyBody->NotifyBlock;
    if (NotifyBlock == NULL) {
        CmUnlockHive(Hive);
        return;
    }

    //
    // Clean up all PostBlocks waiting on the NotifyBlock
    //
    if (IsListEmpty(&(NotifyBlock->PostList)) == FALSE) {
        CmpPostNotify(
            NotifyBlock,
            NULL,
            0,
            STATUS_NOTIFY_CLEANUP,
            NULL
            );
    }

    //
    // Release the subject context
    //
    SeReleaseSubjectContext(&NotifyBlock->SubjectContext);

    //
    // IMPLEMENTATION NOTE:
    //      If we ever do code to report names and types of events,
    //      this is the place to free the buffer.
    //

    //
    // Remove the NotifyBlock from the hive chain
    //
    NotifyBlock->HiveList.Blink->Flink = NotifyBlock->HiveList.Flink;
    if (NotifyBlock->HiveList.Flink != NULL) {
        NotifyBlock->HiveList.Flink->Blink = NotifyBlock->HiveList.Blink;
    }

    // Protect for multiple deletion of the same object
    CmpClearListEntry(&(NotifyBlock->HiveList));

    KeyBody->NotifyBlock = NULL;

#ifdef _CM_ENTRYLIST_MANIPULATION
    if (IsListEmpty(&(NotifyBlock->PostList)) == FALSE) {
        DbgPrint("CmpFlushNotify: NotifyBlock %08lx\n",NotifyBlock);
        DbgBreakPoint();
    }
    //check is the notify has been deleted from the hive notify list
    {
        PCM_NOTIFY_BLOCK ValidNotifyBlock;
        PLIST_ENTRY NotifyPtr;

        NotifyPtr = &(Hive->NotifyList);

        while (NotifyPtr->Flink != NULL) {
            NotifyPtr = NotifyPtr->Flink;

            ValidNotifyBlock = CONTAINING_RECORD(NotifyPtr, CM_NOTIFY_BLOCK, HiveList);
            if( ValidNotifyBlock == NotifyBlock ) {
                DbgPrint("CmpFlushNotify: NotifyBlock %08lx is about to be deleted but is still in the hive notify list\n",NotifyBlock);
                DbgBreakPoint();
            }
        }
    }
    RtlZeroMemory((PVOID)NotifyBlock, sizeof(CM_NOTIFY_BLOCK));
#endif
    
    CmUnlockHive(Hive);

    //
    // Free the block, clean up the KeyBody
    //
    ExFreePool(NotifyBlock);
    return;
}


//
// "Front Side" of notify.  See also Ntapi.c: ntnotifychangekey
//
NTSTATUS
CmpNotifyChangeKey(
    IN PCM_KEY_BODY     KeyBody,
    IN PCM_POST_BLOCK   PostBlock,
    IN ULONG            CompletionFilter,
    IN BOOLEAN          WatchTree,
    IN PVOID            Buffer,
    IN ULONG            BufferSize,
    IN PCM_POST_BLOCK   MasterPostBlock
    )
/*++

Routine Description:

    This routine sets up the NotifyBlock, and attaches the PostBlock
    to it.  When it returns, the Notify is visible to the system,
    and will receive event reports.

    If there is already an event report pending, then the notify
    call will be satisified at once.

Arguments:

    KeyBody - pointer to key object that handle refers to, allows access
              to key control block, notify block, etc.

    PostBlock - pointer to structure that describes how/where the caller
                is to be notified.

                WARNING:    PostBlock must come from Pool, THIS routine
                            will keep it, back side will free it.  This
                            routine WILL free it in case of error.

    CompletionFilter - what types of events the caller wants to see

    WatchTree - TRUE to watch whole subtree, FALSE to watch only immediate
                key the notify is applied to

    Buffer - pointer to area to recieve notify data

    BufferSize - size of buffer, also size user would like to allocate
                 for internal buffer

    MasterPostBlock - the post block of the master notification. Used to
                      insert the PostBlock into the CancelPostList list.

Return Value:

    Status.

--*/
{
    PCM_NOTIFY_BLOCK    NotifyBlock;
    PCM_NOTIFY_BLOCK    node;
    PLIST_ENTRY         ptr;
    PCMHIVE             Hive;
    KIRQL               OldIrql;

    PAGED_CODE();
    CMLOG(CML_WORKER, CMS_NOTIFY) {
        KdPrint(("CmpNotifyChangeKey:\n"));
        KdPrint(("\tKeyBody:%08lx PostBlock:%08lx ", KeyBody, PostBlock));
        KdPrint(("Filter:%08lx WatchTree:%08lx\n", CompletionFilter, WatchTree));
#if DBG
        if(PostBlock->TraceIntoDebugger) {
            WCHAR                   *NameBuffer = NULL;
            UNICODE_STRING          KeyName;

            KdPrint(("[CM]CmpNotifyChangeKey: PostBlock:%08lx\tMasterBlock: %08lx\n", PostBlock,MasterPostBlock));
            NameBuffer = ExAllocatePool(PagedPool, MAX_KEY_NAME_LENGTH);
            if(NameBuffer&&KeyBody->KeyControlBlock->KeyNode) {
               CmpInitializeKeyNameString(KeyBody->KeyControlBlock->KeyNode,&KeyName,NameBuffer);
               KdPrint(("\t[CM]CmpNotifyChangeKey: Key = %.*S\n",KeyName.Length / sizeof(WCHAR),KeyName.Buffer));
               ExFreePool(NameBuffer);
            }
        }
#endif
    }

    //
    // The registry lock should be aquired exclusively by the caller !!!
    //
    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    if (KeyBody->KeyControlBlock->Delete) {
#ifdef KCB_TO_KEYBODY_LINK
        ASSERT( KeyBody->NotifyBlock == NULL );
#endif
        CmpFreePostBlock(PostBlock);
        return STATUS_KEY_DELETED;
    }

    Hive = (PCMHIVE)KeyBody->KeyControlBlock->KeyHive;
    Hive = CONTAINING_RECORD(Hive, CMHIVE, Hive);
    NotifyBlock = KeyBody->NotifyBlock;

    if (NotifyBlock == NULL) {
        //
        // Set up new notify session
        //
        NotifyBlock = ExAllocatePoolWithQuotaTag(PagedPool|POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,sizeof(CM_NOTIFY_BLOCK),CM_NOTIFYBLOCK_TAG);
        CMLOG(CML_MINOR, CMS_POOL) {
            KdPrint(("**CmpNotifyChangeKey: allocate:%08lx, ", sizeof(CM_NOTIFY_BLOCK)));
            KdPrint(("type:%d, at:%08lx\n", PagedPool, NotifyBlock));
        }

        if (NotifyBlock == NULL) {
            CmpFreePostBlock(PostBlock);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        NotifyBlock->KeyControlBlock = KeyBody->KeyControlBlock;
        NotifyBlock->Filter = CompletionFilter;
        NotifyBlock->WatchTree = WatchTree;
        NotifyBlock->NotifyPending = FALSE;
        InitializeListHead(&(NotifyBlock->PostList));
        KeyBody->NotifyBlock = NotifyBlock;
        NotifyBlock->KeyBody = KeyBody;
        ASSERT( KeyBody->KeyControlBlock->Delete == FALSE );

        CMLOG(CML_WORKER, CMS_NOTIFY) {
#if DBG
            if(PostBlock->TraceIntoDebugger) {
                WCHAR                   *NameBuffer = NULL;
                UNICODE_STRING          KeyName;

                NameBuffer = ExAllocatePool(PagedPool, MAX_KEY_NAME_LENGTH);
                if(NameBuffer) {
                   CmpInitializeKeyNameString(KeyBody->KeyControlBlock->KeyNode,&KeyName,NameBuffer);
                   KdPrint(("[CM]\tCmpNotifyChangeKey: New NotifyBlock at:%08lx was allocated for Key = %.*S\n",NotifyBlock,KeyName.Length / sizeof(WCHAR),KeyName.Buffer));
                   ExFreePool(NameBuffer);
                }
            }
#endif
        }

        //
        // IMPLEMENTATION NOTE:
        //      If we ever want to actually return the buffers full of
        //      data, the buffer should be allocated and its address
        //      stored in the notify block here.
        //

        //
        // Capture the subject context so we can do checking once the
        // notify goes off.
        //
        SeCaptureSubjectContext(&NotifyBlock->SubjectContext);

        //
        // Attach notify block to hive in properly sorted order
        //
        ptr = &(Hive->NotifyList);
        while (TRUE) {
            if (ptr->Flink == NULL) {
                //
                // End of list, add self after ptr.
                //
                ptr->Flink = &(NotifyBlock->HiveList);
                NotifyBlock->HiveList.Flink = NULL;
                NotifyBlock->HiveList.Blink = ptr;
                break;
            }

            ptr = ptr->Flink;

            node = CONTAINING_RECORD(ptr, CM_NOTIFY_BLOCK, HiveList);

            if (node->KeyControlBlock->TotalLevels >
                KeyBody->KeyControlBlock->TotalLevels)
            {
                //
                // ptr -> notify with longer name than us, insert in FRONT
                //
                NotifyBlock->HiveList.Flink = ptr;
                ptr->Blink->Flink = &(NotifyBlock->HiveList);
                NotifyBlock->HiveList.Blink = ptr->Blink;
                ptr->Blink = &(NotifyBlock->HiveList);
                break;
            }
        }
    }


    //
    // Add post block to front of notify block's list, and add it to thread list.
    //
    InsertHeadList(
        &(NotifyBlock->PostList),
        &(PostBlock->NotifyList)
        );



    if( IsMasterPostBlock(PostBlock) ) {
        //
        // Protect against outrageous calls
        //
        ASSERT(PostBlock == MasterPostBlock);

        //
        // When the notification is a master one, initialize the CancelPostList list
        //
        InitializeListHead(&(PostBlock->CancelPostList));
    } else {
        //
        // Add PostBlock at the end of the CancelPostList list from the master post
        //
        InsertTailList(
            &(MasterPostBlock->CancelPostList),
            &(PostBlock->CancelPostList)
            );
    }


    KeRaiseIrql(APC_LEVEL, &OldIrql);
    InsertHeadList(
        &(PsGetCurrentThread()->PostBlockList),
        &(PostBlock->ThreadList)
        );

    CMLOG(CML_WORKER, CMS_NOTIFY) {
#if DBG
        if(PostBlock->TraceIntoDebugger) {
            KdPrint(("[CM]\tCmpNotifyChangeKey: Attaching the post:%08lx\t to thread:%08lx\n",PostBlock,PsGetCurrentThread()));
        }
#endif
    }

    KeLowerIrql(OldIrql);

    //
    // If there is a notify pending (will not be if we just created
    // the notify block) then post it at once.  Note that this call
    // ALWAYS returns STATUS_PENDING unless it fails.  Caller must
    // ALWAYS look in IoStatusBlock to see what happened.
    //
    if (NotifyBlock->NotifyPending == TRUE) {
        CmpPostNotify(
            NotifyBlock,
            NULL,
            0,
            STATUS_NOTIFY_ENUM_DIR,
            NULL
            );
        //
        // return STATUS_SUCCESS to signal to the caller the the notify already been triggered
        //
        return STATUS_SUCCESS;
    }

    //
    // return STATUS_PENDING to signal to the caller the the notify has not been triggered yet
    //
    return STATUS_PENDING;
}

VOID
CmpFreeSlavePost(
    PCM_POST_BLOCK  MasterPostBlock
    )
/*++

Routine Description:

	Free the slave post block related to this master post block

Arguments:

    MasterPostBlock - pointer to structure that describes the post requests.
                It should be a master post!!
Return Value:

    NONE.

--*/
{
    PCM_POST_BLOCK  SlavePostBlock;

    PAGED_CODE();
    CMLOG(CML_MAJOR, CMS_NOTIFY) {
        KdPrint(("CmpCancelSlavePost:\t"));
        KdPrint(("MasterPostBlock:%08lx\n", MasterPostBlock));
    }

    ASSERT(IsMasterPostBlock(MasterPostBlock));

    CMLOG(CML_MAJOR, CMS_NOTIFY) {
#if DBG
        if(MasterPostBlock->TraceIntoDebugger) {
            KdPrint(("[CM]CmCancelSlavePost: MasterPostBlock:%08lx\n", MasterPostBlock));
        }
#endif
    }

    if (IsListEmpty(&(MasterPostBlock->CancelPostList)) == TRUE) {
        //
        // Nothing to cancel, just return
        //
        CMLOG(CML_MAJOR, CMS_NOTIFY) {
#if DBG
            if(MasterPostBlock->TraceIntoDebugger) {
                KdPrint(("[CM]CmCancelSlavePost: MasterPostBlock:%08lx has no slaves\n", MasterPostBlock));
            }
#endif
        }

        return;
    }


    //
    // Pull all the entries in the cancel post list and unlink them (when they are slave requests)
    // We base here on the assumption that there is only one slave.
    //
    //     NOTE!!!
    //       When more than slave allowed, here to modify
    //


    SlavePostBlock = (PCM_POST_BLOCK)MasterPostBlock->CancelPostList.Flink;
    SlavePostBlock = CONTAINING_RECORD(SlavePostBlock,
                                       CM_POST_BLOCK,
                                       CancelPostList);

    CMLOG(CML_MAJOR, CMS_NOTIFY) {
#if DBG
        if(MasterPostBlock->TraceIntoDebugger) {
            KdPrint(("[CM]CmCancelSlavePost: Cleaning SlavePostBlock:%08lx\n", SlavePostBlock));
        }
#endif
    }

    //
    // This should be true !
    //
    ASSERT( !IsMasterPostBlock(SlavePostBlock) );

    //
    // Unchain from the Master CancelPostList
    //
    // Use Cmp variant to protect for multiple deletion of the same object
    CmpRemoveEntryList(&(SlavePostBlock->CancelPostList));

    //
    // delist the post block from the thread postblocklist
    //
    // Use Cmp variant to protect for multiple deletion of the same object
    CmpRemoveEntryList(&(SlavePostBlock->ThreadList));

    //
    // Free the post block.
    //
    CmpFreePostBlock(SlavePostBlock);

    //
    // Result validation. was it the only slave?
    //
    ASSERT(IsListEmpty(&(MasterPostBlock->CancelPostList)));
}

VOID
CmpCancelSlavePost(
    PCM_POST_BLOCK  MasterPostBlock,
    PLIST_ENTRY     DelayedDeref
    )
/*++

Routine Description:

	Unlink the slave postblock from its notify list and dereferences (or adds to the delayed deref list)
	the keybody related to this thread. This should disable the slave post block. 
	It will be cleared later in CmpPostApc.

Arguments:

    MasterPostBlock - pointer to structure that describes the post requests.
                It should be a master post!!
    DelayedDeref - pointer to list of delayed deref keybodies. If this parameter is not NULL,
                the keybody for the slave is not cleared before calling CmpFreePostBlock, 
                and instead is added to the list


Return Value:

    NONE.

--*/
{
    PCM_POST_BLOCK  SlavePostBlock;

    PAGED_CODE();
    CMLOG(CML_MAJOR, CMS_NOTIFY) {
        KdPrint(("CmpCancelSlavePost:\t"));
        KdPrint(("MasterPostBlock:%08lx\n", MasterPostBlock));
    }

    ASSERT_CM_LOCK_OWNED();

    ASSERT(IsMasterPostBlock(MasterPostBlock));

    CMLOG(CML_MAJOR, CMS_NOTIFY) {
#if DBG
        if(MasterPostBlock->TraceIntoDebugger) {
            KdPrint(("[CM]CmCancelSlavePost: MasterPostBlock:%08lx\n", MasterPostBlock));
        }
#endif
    }

    if (IsListEmpty(&(MasterPostBlock->CancelPostList)) == TRUE) {
        //
        // Nothing to cancel, just return
        //
        CMLOG(CML_MAJOR, CMS_NOTIFY) {
#if DBG
            if(MasterPostBlock->TraceIntoDebugger) {
                KdPrint(("[CM]CmCancelSlavePost: MasterPostBlock:%08lx has no slaves\n", MasterPostBlock));
            }
#endif
        }

        return;
    }


    //
    // Pull all the entries in the cancel post list and unlink them (when they are slave requests)
    // We base here on the assumption that there is only one slave.
    //
    //     NOTE!!!
    //       When more than slave allowed, here to modify
    //


    SlavePostBlock = (PCM_POST_BLOCK)MasterPostBlock->CancelPostList.Flink;
    SlavePostBlock = CONTAINING_RECORD(SlavePostBlock,
                                       CM_POST_BLOCK,
                                       CancelPostList);

    CMLOG(CML_MAJOR, CMS_NOTIFY) {
#if DBG
        if(MasterPostBlock->TraceIntoDebugger) {
            KdPrint(("[CM]CmCancelSlavePost: Cleaning SlavePostBlock:%08lx\n", SlavePostBlock));
        }
#endif
    }

    //
    // This should be true !
    //
    ASSERT( !IsMasterPostBlock(SlavePostBlock) );

    //
    // Remove it from notify block's list
    //
    // Use Cmp variant to protect for multiple deletion of the same object
	// This will disable the notifications that might come on the slave key
	//
    CmpRemoveEntryList(&(SlavePostBlock->NotifyList));

    if( DelayedDeref ) {
        // 
        // the caller wants to handle key body dereferenciation by himself
        //
        CmpAddToDelayedDeref(SlavePostBlock,DelayedDeref);
    }
}

VOID
CmpAddToDelayedDeref(
    PCM_POST_BLOCK  PostBlock,
    PLIST_ENTRY     DelayedDeref
    )
/*++

Routine Description:

    Add the key body attached to the post block to the delayed deref list.
    Cleans the post block KeyBody member, so it will not be dereferenced 
    when the post block is freed.

Arguments:

    PostBlock - pointer to structure that describes the post requests.

    DelayedDeref - the delayed deref list

Return Value:

    NONE.

--*/

{
    PAGED_CODE();

    // common sense
    ASSERT( PostBlock != NULL );

    if( PostBlock->PostKeyBody ) {
        //
        // If the post block has a keybody attached, add it to delayed deref list and 
        // clear the post block member. The key body will be deref'd prior after 
        // postblock lock is released.
        //
    
        // extra validation
        ASSERT(PostBlock->PostKeyBody->KeyBody != NULL);
        ASSERT(DelayedDeref);

        // add it to the end of the list
        InsertTailList(
            DelayedDeref,
            &(PostBlock->PostKeyBody->KeyBodyList)
            );
    
        // make sure we don't deref it in CmpFreePostBlock
        PostBlock->PostKeyBody = NULL;
    }

    return;
}

VOID
CmpDelayedDerefKeys(
                    PLIST_ENTRY DelayedDeref
                    )
/*++

Routine Description:

    Walk through the entire list, dereference each keybody and free storage for the 
    CM_POST_KEY_BODY allocated for this purpose.

Arguments:

    DelayedDeref - the delayed deref list

Return Value:

    NONE.

--*/
{
    PCM_POST_KEY_BODY   PostKeyBody;

    PAGED_CODE();

    // common sense
    ASSERT( DelayedDeref != NULL );

    while(IsListEmpty(DelayedDeref) == FALSE) {
        //
        // Remove from the delayed deref list and deref the coresponding keybody
        // free the storage associated with CM_POST_KEY_BODY
        //
        PostKeyBody = (PCM_POST_KEY_BODY)RemoveHeadList(DelayedDeref);
        PostKeyBody = CONTAINING_RECORD(PostKeyBody,
                                      CM_POST_KEY_BODY,
                                      KeyBodyList);

        // extra validation
        ASSERT(PostKeyBody->KeyBody != NULL);
        // this should be a valid key body
        ASSERT(PostKeyBody->KeyBody->Type == KEY_BODY_TYPE);
        
        // at last ..... dereference the key object
        ObDereferenceObject(PostKeyBody->KeyBody);

        // Free the storage for the CM_POST_KEY_BODY object (allocated by CmpAllocatePostBlock)
        ExFreePool(PostKeyBody);
    }
}

