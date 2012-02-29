/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/notify.c
 * PURPOSE:         Change Notifications and Sync for File System Drivers
 * PROGRAMMERS:     Pierre Schweitzer
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
FsRtlNotifyCompleteIrpList(IN PNOTIFY_CHANGE NotifyChange,
                           IN NTSTATUS Status);

BOOLEAN
FsRtlNotifySetCancelRoutine(IN PIRP Irp,
                            IN PNOTIFY_CHANGE NotifyChange OPTIONAL);

VOID
NTAPI
FsRtlCancelNotify(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)
{
    IoReleaseCancelSpinLock(Irp->CancelIrql);
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
FsRtlCheckNotifyForDelete(IN PLIST_ENTRY NotifyList,
                          IN PVOID FsContext)
{
    PLIST_ENTRY NextEntry;
    PNOTIFY_CHANGE NotifyChange;

    if (!IsListEmpty(NotifyList))
    {
        /* Browse the notifications list to find the matching entry */
        for (NextEntry = NotifyList->Flink;
             NextEntry != NotifyList;
             NextEntry = NextEntry->Flink)
        {
            NotifyChange = CONTAINING_RECORD(NextEntry, NOTIFY_CHANGE, NotifyList);
            /* If the current record matches with the given context, it's the good one */
            if (NotifyChange->FsContext == FsContext && !IsListEmpty(&(NotifyChange->NotifyIrps)))
            {
                FsRtlNotifyCompleteIrpList(NotifyChange, STATUS_DELETE_PENDING);
            }
        }
    }
}

PNOTIFY_CHANGE
FsRtlIsNotifyOnList(IN PLIST_ENTRY NotifyList,
                    IN PVOID FsContext)
{
    PLIST_ENTRY NextEntry;
    PNOTIFY_CHANGE NotifyChange;

    if (!IsListEmpty(NotifyList))
    {
        /* Browse the notifications list to find the matching entry */
        for (NextEntry = NotifyList->Flink;
             NextEntry != NotifyList;
             NextEntry = NextEntry->Flink)
        {
            NotifyChange = CONTAINING_RECORD(NextEntry, NOTIFY_CHANGE, NotifyList);
            /* If the current record matches with the given context, it's the good one */
            if (NotifyChange->FsContext == FsContext)
            {
                return NotifyChange;
            }
        }
    }
    return NULL;
}

VOID
FORCEINLINE
FsRtlNotifyAcquireFastMutex(IN PREAL_NOTIFY_SYNC RealNotifySync)
{
    ULONG_PTR CurrentThread = (ULONG_PTR)KeGetCurrentThread();

    /* Only acquire fast mutex if it's not already acquired by the current thread */
    if (RealNotifySync->OwningThread != CurrentThread)
    {
        ExAcquireFastMutexUnsafe(&(RealNotifySync->FastMutex));
        RealNotifySync->OwningThread = CurrentThread;
    }
    /* Whatever the case, keep trace of the attempt to acquire fast mutex */
    RealNotifySync->OwnerCount++;
}

/*
 * @implemented
 */
VOID
FsRtlNotifyCompleteIrp(IN PIRP Irp,
                       IN PNOTIFY_CHANGE NotifyChange,
                       IN ULONG DataLength,
                       IN NTSTATUS Status,
                       IN BOOLEAN SkipCompletion)
{
    PVOID Buffer;
    PIO_STACK_LOCATION Stack;

    PAGED_CODE();

    /* Check if we need to complete */
    if (!FsRtlNotifySetCancelRoutine(Irp, NotifyChange) && SkipCompletion)
    {
        return;
    }

    /* No succes => no data to return just complete */
    if (Status != STATUS_SUCCESS)
    {
        goto Completion;
    }

    /* Ensure there's something to return */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    if (!DataLength || Stack->Parameters.NotifyDirectory.Length < DataLength)
    {
        Status = STATUS_NOTIFY_ENUM_DIR;
        goto Completion;
    }

    /* Ensture there's a buffer where to find data */
    if (!NotifyChange->AllocatedBuffer)
    {
        Irp->IoStatus.Information = DataLength;
        NotifyChange->Buffer = NULL;
        goto Completion;
    }

    /* Now, browse all the way to return data
     * and find the one that will work. We will
     * return data whatever happens
     */
    if (Irp->AssociatedIrp.SystemBuffer)
    {
        Buffer = Irp->AssociatedIrp.SystemBuffer;
        goto CopyAndComplete;
    }

    if (Irp->MdlAddress)
    {
        Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
        goto CopyAndComplete;
    }

    if (!(Stack->Control & SL_PENDING_RETURNED))
    {
        Buffer = Irp->UserBuffer;
        goto CopyAndComplete;
    }

    Irp->Flags |= (IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_SYNCHRONOUS_PAGING_IO);
    Irp->AssociatedIrp.SystemBuffer = NotifyChange->AllocatedBuffer;
    /* Nothing to copy */
    goto ReleaseAndComplete;

CopyAndComplete:
    _SEH2_TRY
    {
        RtlCopyMemory(Buffer, NotifyChange->AllocatedBuffer, DataLength);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Do nothing */
    }
    _SEH2_END;

ReleaseAndComplete:
    PsReturnProcessPagedPoolQuota(NotifyChange->OwningProcess, NotifyChange->ThisBufferLength);

    /* Release buffer UNLESS it's used */
    if (NotifyChange->AllocatedBuffer != Irp->AssociatedIrp.SystemBuffer &&
        NotifyChange->AllocatedBuffer)
    {
        ExFreePoolWithTag(NotifyChange->AllocatedBuffer, 0);
    }

    /* Prepare for return */
    NotifyChange->AllocatedBuffer = 0;
    NotifyChange->ThisBufferLength = 0;
    Irp->IoStatus.Information = DataLength;
    NotifyChange->Buffer = NULL;

    /* Finally complete */
Completion:
    IoMarkIrpPending(Irp);
    Irp->IoStatus.Status = Status;
    IofCompleteRequest(Irp, EVENT_INCREMENT);
}

/*
 * @implemented
 */
VOID
FsRtlNotifyCompleteIrpList(IN PNOTIFY_CHANGE NotifyChange,
                           IN NTSTATUS Status)
{
    PIRP Irp;
    ULONG DataLength;
    PLIST_ENTRY NextEntry;

    DataLength = NotifyChange->DataLength;

    NotifyChange->Flags &= (INVALIDATE_BUFFERS | WATCH_TREE);
    NotifyChange->DataLength = 0;
    NotifyChange->LastEntry = 0;

    while (!IsListEmpty(&(NotifyChange->NotifyIrps)))
    {
        /* We take the first entry */
        NextEntry = RemoveHeadList(&(NotifyChange->NotifyIrps));
        Irp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);
        /* We complete it */
        FsRtlNotifyCompleteIrp(Irp, NotifyChange, DataLength, Status, TRUE);
        /* If we're notifying success, just notify first one */
        if (Status == STATUS_SUCCESS)
            break;
    }
}

VOID
FORCEINLINE
FsRtlNotifyReleaseFastMutex(IN PREAL_NOTIFY_SYNC RealNotifySync)
{
    RealNotifySync->OwnerCount--;
    /* Release the fast mutex only if no other instance needs it */
    if (!RealNotifySync->OwnerCount)
    {
        ExReleaseFastMutexUnsafe(&(RealNotifySync->FastMutex));
        RealNotifySync->OwningThread = (ULONG_PTR)0;
    }
}

/*
 * @implemented
 */
BOOLEAN
FsRtlNotifySetCancelRoutine(IN PIRP Irp,
                            IN PNOTIFY_CHANGE NotifyChange OPTIONAL)
{
    PDRIVER_CANCEL CancelRoutine;

    /* Acquire cancel lock */
    IoAcquireCancelSpinLock(&Irp->CancelIrql);

    /* If NotifyChange was given */
    if (NotifyChange)
    {
        /* First get cancel routine */
        CancelRoutine = IoSetCancelRoutine(Irp, NULL);
        Irp->IoStatus.Information = 0;
        /* Release cancel lock */
        IoReleaseCancelSpinLock(Irp->CancelIrql);
        /* If there was a cancel routine */
        if (CancelRoutine)
        {
            /* Decrease reference count */
            InterlockedDecrement((PLONG)&NotifyChange->ReferenceCount);
            /* Notify that we removed cancel routine */
            return TRUE;
        }
    }
    else
    {
        /* If IRP is cancel, call FsRtl cancel routine */
        if (Irp->Cancel)
        {
             FsRtlCancelNotify(NULL, Irp);
        }
        else
        {
            /* Otherwise, define FsRtl cancel routine as IRP cancel routine */
            IoSetCancelRoutine(Irp, FsRtlCancelNotify);
            /* Release lock */
            IoReleaseCancelSpinLock(Irp->CancelIrql);
        }
    }

    /* Return that we didn't removed cancel routine */
    return FALSE;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlNotifyChangeDirectory
 * @implemented
 *
 * Lets FSD know if changes occures in the specified directory.
 * Directory will be reenumerated. 
 *
 * @param NotifySync
 *        Synchronization object pointer
 *
 * @param FsContext
 *        Used to identify the notify structure
 *
 * @param FullDirectoryName
 *        String (A or W) containing the full directory name 
 *
 * @param NotifyList
 *        Notify list pointer (to head)
 *
 * @param WatchTree
 *        True to notify changes in subdirectories too
 *
 * @param CompletionFilter
 *        Used to define types of changes to notify
 *
 * @param NotifyIrp
 *        IRP pointer to complete notify operation. It can be null
 *
 * @return None
 *
 * @remarks This function only redirects to FsRtlNotifyFilterChangeDirectory.
 *
 *--*/
VOID
NTAPI
FsRtlNotifyChangeDirectory(IN PNOTIFY_SYNC NotifySync,
                           IN PVOID FsContext,
                           IN PSTRING FullDirectoryName,
                           IN PLIST_ENTRY NotifyList,
                           IN BOOLEAN WatchTree,
                           IN ULONG CompletionFilter,
                           IN PIRP NotifyIrp)
{
    FsRtlNotifyFilterChangeDirectory(NotifySync,
                                     NotifyList,
                                     FsContext,
                                     FullDirectoryName,
                                     WatchTree,
                                     TRUE,
                                     CompletionFilter,
                                     NotifyIrp,
                                     NULL,
                                     NULL,
                                     NULL);
}

/*++
 * @name FsRtlNotifyCleanup
 * @implemented
 *
 * Called by FSD when all handles to FileObject (identified by FsContext) are closed
 *
 * @param NotifySync
 *        Synchronization object pointer
 *
 * @param NotifyList
 *        Notify list pointer (to head) 
 *
 * @param FsContext
 *        Used to identify the notify structure
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyCleanup(IN PNOTIFY_SYNC NotifySync,
                   IN PLIST_ENTRY NotifyList,
                   IN PVOID FsContext)
{
    PNOTIFY_CHANGE NotifyChange;
    PREAL_NOTIFY_SYNC RealNotifySync;
    PSECURITY_SUBJECT_CONTEXT SubjectContext = NULL;

    /* Get real structure hidden behind the opaque pointer */
    RealNotifySync = (PREAL_NOTIFY_SYNC)NotifySync;

    /* Acquire the fast mutex */
    FsRtlNotifyAcquireFastMutex(RealNotifySync);

    _SEH2_TRY
    {
        /* Find if there's a matching notification with the FsContext */
        NotifyChange = FsRtlIsNotifyOnList(NotifyList, FsContext);
        if (NotifyChange)
        {
            /* Mark it as to know that cleanup is in process */
            NotifyChange->Flags |= CLEANUP_IN_PROCESS;

            /* If there are pending IRPs, complete them using the STATUS_NOTIFY_CLEANUP status */
            if (!IsListEmpty(&NotifyChange->NotifyIrps))
            {
                FsRtlNotifyCompleteIrpList(NotifyChange, STATUS_NOTIFY_CLEANUP);
            }

            /* Decrease reference number and if 0 is reached, it's time to do complete cleanup */
            if (!InterlockedDecrement((PLONG)&(NotifyChange->ReferenceCount)))
            {
                /* Remove it from the notifications list */
                RemoveEntryList(&NotifyChange->NotifyList);

                /* In case there was an allocated buffer, free it */
                if (NotifyChange->AllocatedBuffer)
                {
                    PsReturnProcessPagedPoolQuota(NotifyChange->OwningProcess, NotifyChange->ThisBufferLength);
                    ExFreePool(NotifyChange->AllocatedBuffer);
                }

                /* In case there the string was set, get the captured subject security context */
                if (NotifyChange->FullDirectoryName)
                {
                    SubjectContext = NotifyChange->SubjectContext;
                }

                /* Finally, free the notification, as it's not needed anymore */
                ExFreePool(NotifyChange);
            }
        }
    }
    _SEH2_FINALLY
    {
        /* Release fast mutex */
        FsRtlNotifyReleaseFastMutex(RealNotifySync);

        /* If the subject security context was captured, release and free it */
        if (SubjectContext)
        {
            SeReleaseSubjectContext(SubjectContext);
            ExFreePool(SubjectContext);
        }
    }
    _SEH2_END;
}

/*++
 * @name FsRtlNotifyFilterChangeDirectory
 * @unimplemented
 *
 * FILLME
 *
 * @param NotifySync
 *        FILLME
 *
 * @param NotifyList
 *        FILLME
 *
 * @param FsContext
 *        FILLME
 *
 * @param FullDirectoryName
 *        FILLME
 *
 * @param WatchTree
 *        FILLME
 *
 * @param IgnoreBuffer
 *        FILLME
 *
 * @param CompletionFilter
 *        FILLME
 *
 * @param NotifyIrp
 *        FILLME
 *
 * @param TraverseCallback
 *        FILLME
 *
 * @param SubjectContext
 *        FILLME
 *
 * @param FilterCallback
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyFilterChangeDirectory(IN PNOTIFY_SYNC NotifySync,
                                 IN PLIST_ENTRY NotifyList,
                                 IN PVOID FsContext,
                                 IN PSTRING FullDirectoryName,
                                 IN BOOLEAN WatchTree,
                                 IN BOOLEAN IgnoreBuffer,
                                 IN ULONG CompletionFilter,
                                 IN PIRP NotifyIrp,
                                 IN PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback OPTIONAL,
                                 IN PSECURITY_SUBJECT_CONTEXT SubjectContext OPTIONAL,
                                 IN PFILTER_REPORT_CHANGE FilterCallback OPTIONAL)
{
    KeBugCheck(FILE_SYSTEM);
}

/*++
 * @name FsRtlNotifyFilterReportChange
 * @unimplemented
 *
 * FILLME
 *
 * @param NotifySync
 *        FILLME
 *
 * @param NotifyList
 *        FILLME
 *
 * @param FullTargetName
 *        FILLME
 *
 * @param TargetNameOffset
 *        FILLME
 *
 * @param StreamName
 *        FILLME
 *
 * @param NormalizedParentName
 *        FILLME
 *
 * @param FilterMatch
 *        FILLME
 *
 * @param Action
 *        FILLME
 *
 * @param TargetContext
 *        FILLME
 *
 * @param FilterContext
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyFilterReportChange(IN PNOTIFY_SYNC NotifySync,
                              IN PLIST_ENTRY NotifyList,
                              IN PSTRING FullTargetName,
                              IN USHORT TargetNameOffset,
                              IN PSTRING StreamName OPTIONAL,
                              IN PSTRING NormalizedParentName OPTIONAL,
                              IN ULONG FilterMatch,
                              IN ULONG Action,
                              IN PVOID TargetContext,
                              IN PVOID FilterContext)
{
    KeBugCheck(FILE_SYSTEM);
}

/*++
 * @name FsRtlNotifyFullChangeDirectory
 * @implemented
 *
 * Lets FSD know if changes occures in the specified directory. 
 *
 * @param NotifySync
 *        Synchronization object pointer
 *
 * @param NotifyList
 *        Notify list pointer (to head) 
 *
 * @param FsContext
 *        Used to identify the notify structure
 *
 * @param FullDirectoryName
 *        String (A or W) containing the full directory name 
 *
 * @param WatchTree
 *        True to notify changes in subdirectories too
 *
 * @param IgnoreBuffer
 *        True to reenumerate directory. It's ignored it NotifyIrp is null
 *
 * @param CompletionFilter
 *        Used to define types of changes to notify
 *
 * @param NotifyIrp
 *        IRP pointer to complete notify operation. It can be null
 *
 * @param TraverseCallback
 *        Pointer to a callback function. It's called each time a change is
 *        done in a subdirectory of the main directory. It's ignored it NotifyIrp
 *        is null
 *
 * @param SubjectContext
 *        Pointer to pass to SubjectContext member of TraverseCallback.
 *        It's freed after use. It's ignored it NotifyIrp is null
 *
 * @return None
 *
 * @remarks This function only redirects to FsRtlNotifyFilterChangeDirectory.
 *
 *--*/
VOID
NTAPI
FsRtlNotifyFullChangeDirectory(IN PNOTIFY_SYNC NotifySync,
                               IN PLIST_ENTRY NotifyList,
                               IN PVOID FsContext,
                               IN PSTRING FullDirectoryName,
                               IN BOOLEAN WatchTree,
                               IN BOOLEAN IgnoreBuffer,
                               IN ULONG CompletionFilter,
                               IN PIRP NotifyIrp,
                               IN PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback OPTIONAL,
                               IN PSECURITY_SUBJECT_CONTEXT SubjectContext OPTIONAL)
{
    FsRtlNotifyFilterChangeDirectory(NotifySync,
                                     NotifyList,
                                     FsContext,
                                     FullDirectoryName,
                                     WatchTree,
                                     IgnoreBuffer,
                                     CompletionFilter,
                                     NotifyIrp,
                                     TraverseCallback,
                                     SubjectContext,
                                     NULL);
}

/*++
 * @name FsRtlNotifyFullReportChange
 * @implemented
 *
 * Complets the pending notify IRPs.
 *
 * @param NotifySync
 *        Synchronization object pointer
 *
 * @param NotifyList
 *        Notify list pointer (to head) 
 *
 * @param FullTargetName
 *        String (A or W) containing the full directory name that changed
 *
 * @param TargetNameOffset
 *        Offset, in FullTargetName, of the final component that is in the changed directory 
 *
 * @param StreamName
 *        String (A or W) containing a stream name
 *
 * @param NormalizedParentName
 *        String (A or W) containing the full directory name that changed with long names
 *
 * @param FilterMatch
 *        Flags that will be compared to the completion filter
 *
 * @param Action
 *        Action code to store in user's buffer
 *
 * @param TargetContext
 *        Pointer to a callback function. It's called each time a change is
 *        done in a subdirectory of the main directory.
 *
 * @return None
 *
 * @remarks This function only redirects to FsRtlNotifyFilterReportChange.
 *
 *--*/
VOID
NTAPI
FsRtlNotifyFullReportChange(IN PNOTIFY_SYNC NotifySync,
                            IN PLIST_ENTRY NotifyList,
                            IN PSTRING FullTargetName,
                            IN USHORT TargetNameOffset,
                            IN PSTRING StreamName OPTIONAL,
                            IN PSTRING NormalizedParentName OPTIONAL,
                            IN ULONG FilterMatch,
                            IN ULONG Action,
                            IN PVOID TargetContext)
{
    FsRtlNotifyFilterReportChange(NotifySync,
                                  NotifyList,
                                  FullTargetName,
                                  TargetNameOffset,
                                  StreamName,
                                  NormalizedParentName,
                                  FilterMatch,
                                  Action,
                                  TargetContext,
                                  NULL);
}

/*++
 * @name FsRtlNotifyInitializeSync
 * @implemented
 *
 * Allocates the internal structure associated with notifications.
 *
 * @param NotifySync
 *        Opaque pointer. It will receive the address of the allocated internal structure.
 *
 * @return None
 *
 * @remarks This function raise an exception in case of a failure.
 *
 *--*/
VOID
NTAPI
FsRtlNotifyInitializeSync(IN PNOTIFY_SYNC *NotifySync)
{
    PREAL_NOTIFY_SYNC RealNotifySync;

    *NotifySync = NULL;
    
    RealNotifySync = ExAllocatePoolWithTag(NonPagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                           sizeof(REAL_NOTIFY_SYNC), 'FSNS');
    ExInitializeFastMutex(&(RealNotifySync->FastMutex));
    RealNotifySync->OwningThread = 0;
    RealNotifySync->OwnerCount = 0;

    *NotifySync = RealNotifySync;
}

/*++
 * @name FsRtlNotifyReportChange
 * @implemented
 *
 * Complets the pending notify IRPs.
 *
 * @param NotifySync
 *        Synchronization object pointer
 *
 * @param NotifyList
 *        Notify list pointer (to head) 
 *
 * @param FullTargetName
 *        String (A or W) containing the full directory name that changed
 *
 * @param FileNamePartLength
 *        Length of the final component that is in the changed directory
 *
 * @param FilterMatch
 *        Flags that will be compared to the completion filter
 *
 * @return None
 *
 * @remarks This function only redirects to FsRtlNotifyFilterReportChange.
 *
 *--*/
VOID
NTAPI
FsRtlNotifyReportChange(IN PNOTIFY_SYNC NotifySync,
                        IN PLIST_ENTRY NotifyList,
                        IN PSTRING FullTargetName,
                        IN PUSHORT FileNamePartLength,
                        IN ULONG FilterMatch)
{
      FsRtlNotifyFilterReportChange(NotifySync,
                                    NotifyList,
                                    FullTargetName,
                                    FullTargetName->Length - *FileNamePartLength,
                                    NULL,
                                    NULL,
                                    FilterMatch,
                                    0,
                                    NULL,
                                    NULL); 
}

/*++
 * @name FsRtlNotifyUninitializeSync
 * @implemented
 *
 * Uninitialize a NOTIFY_SYNC object
 *
 * @param NotifySync
 *        Address of a pointer to a PNOTIFY_SYNC object previously
 *        initialized by FsRtlNotifyInitializeSync()
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyUninitializeSync(IN PNOTIFY_SYNC *NotifySync)
{
    if (*NotifySync)
    {
        ExFreePoolWithTag(*NotifySync, 'FSNS');
        *NotifySync = NULL;
    }
}

