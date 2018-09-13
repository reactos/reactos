/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Notify.c

Abstract:

    The Notify package provides support to filesystems which implement
    NotifyChangeDirectory.  This package will manage a queue of notify
    blocks which are attached to some filesystem structure (i.e. Vcb
    in Fat, HPFS).  The filesystems will allocate a fast mutex to be used
    by this package to synchronize access to the notify queue.

    The following routines are provided by this package:

        o  FsRtlNotifyInitializeSync - Create and initializes the
           synchronization object.

        o  FsrtlNotifyUninitializeSync - Deallocates the synchronization
           object.

        o  FsRtlNotifyChangeDirectory - This routine is called whenever the
           filesystems receive a NotifyChangeDirectoryFile call.  This
           routine allocates any neccessary structures and places the
           Irp in the NotifyQueue (or possibly completes or cancels it
           immediately).

        o  FsRtlNotifyFullChangeDirectory - This routine is called whenever the
           filesystems receive a NotifyChangeDirectoryFile call.  This differs
           from the FsRtlNotifyChangeDirectory in that it expects to return
           the notify information in the user's buffer.

        o  FsRtlNotifyReportChange - This routine is called by the
           filesystems whenever they perform some operation that could
           cause the completion of a notify operation.  This routine will
           walk through the notify queue to see if any Irps are affected
           by the indicated operation.

        o  FsRtlNotifyFullReportChange - This routine is called by the
           filesystems whenever they perform some operation that could
           cause the completion of a notify operation.  This routine differs
           from the FsRtlNotifyReportChange call in that it returns more
           detailed information in the caller's buffer if present.

        o  FsRtlNotifyCleanup - This routine is called to remove any
           references to a particular FsContext structure from the notify
           queue.  If the matching FsContext structure is found in the
           queue, then all associated Irps are completed.

Author:

    Brian Andrew    [BrianAn]   9-19-1991

Revision History:

--*/

#include "FsRtlP.h"

//
//  Trace level for the module
//

#define Dbg                              (0x04000000)

//
//  This is the synchronization object for the notify package.  The caller
//  given a pointer to this structure.
//

typedef struct _REAL_NOTIFY_SYNC {

    FAST_MUTEX FastMutex;
    ERESOURCE_THREAD OwningThread;
    ULONG OwnerCount;

} REAL_NOTIFY_SYNC, *PREAL_NOTIFY_SYNC;

//
//  A list of the following structures is used to store the NotifyChange
//  requests.  They are linked to a filesystem-defined list head.
//

typedef struct _NOTIFY_CHANGE {

    //
    //  Fast Mutex.  This fast mutex is used to access the list containing this
    //  structure.
    //

    PREAL_NOTIFY_SYNC NotifySync;

    //
    //  FsContext.  This value is given by the filesystems to uniquely
    //  identify this structure.  The identification is on a
    //  per-user file object basis.  The expected value is the Ccb address
    //  for this user file object.
    //

    PVOID FsContext;

    //
    //  StreamID.  This value matches the FsContext field in the file object for
    //  the directory being watched.  This is used to identify the directory stream
    //  when the directory is being deleted.
    //

    PVOID StreamID;

    //
    //  TraverseAccessCallback.  This is the filesystem-supplied routine used
    //  to call back into the filesystem to check whether the caller has traverse
    //  access when watching a sub-directory.  Only applies when watching a
    //  sub-directory.
    //

    PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback;

    //
    //  SubjectContext.  If the caller specifies a traverse callback routine
    //  we will need to pass the Security Context from the thread which
    //  originated this call.  The notify package will free this structure
    //  on tearing down the notify package.  We don't expect to need this
    //  structure often.
    //

    PSECURITY_SUBJECT_CONTEXT SubjectContext;

    //
    //  Full Directory Name.  The following string is the full directory
    //  name of the directory being watched.  It is used during watch tree
    //  operations to check whether this directory is an ancestor of
    //  the modified file.  The string could be in ANSI or UNICODE form.
    //

    PSTRING FullDirectoryName;

    //
    //  Notify List.  The following field links the notify structures for
    //  a particular volume.
    //

    LIST_ENTRY NotifyList;

    //
    //  Notify Irps.  The following field links the Irps associated with
    //
    //

    LIST_ENTRY NotifyIrps;

    //
    //  Flags.  State of the notify for this volume.
    //

    USHORT Flags;

    //
    //  Character size.  Larger size indicates unicode characters.
    //  unicode names.
    //

    UCHAR CharacterSize;

    //
    //  Completion Filter.  This field is used to mask the modification
    //  actions to determine whether to complete the notify irp.
    //

    ULONG CompletionFilter;

    //
    //  The following values are used to manage a buffer if there is no current
    //  Irp to complete. The fields have the following meaning:
    //
    //      AllocatedBuffer     - Buffer we need to allocate
    //      Buffer              - Buffer to store data in
    //      BufferLength        - Length of original user buffer
    //      ThisBufferLength    - Length of the buffer we are using
    //      DataLength          - Current length of the data in the buffer
    //      LastEntry           - Offset of previous entry in the buffer
    //

    PVOID AllocatedBuffer;
    PVOID Buffer;
    ULONG BufferLength;
    ULONG ThisBufferLength;
    ULONG DataLength;
    ULONG LastEntry;

    //
    //  Reference count which keeps the notify structure around.  Such references include
    // 
    //      - Lifetime reference.  Count set to one initially and removed on cleanup
    //      - Cancel reference.  Reference the notify struct when storing the cancel routine
    //          in the Irp.  The routine which actually clears the routine will decrement
    //          this value.
    //

    ULONG ReferenceCount;

    //
    //  This is the process on whose behalf the structure was allocated.  We
    //  charge any quota to this process.
    //

    PEPROCESS OwningProcess;

} NOTIFY_CHANGE, *PNOTIFY_CHANGE;

#define NOTIFY_WATCH_TREE               (0x0001)
#define NOTIFY_IMMEDIATE_NOTIFY         (0x0002)
#define NOTIFY_CLEANUP_CALLED           (0x0004)
#define NOTIFY_DEFER_NOTIFY             (0x0008)
#define NOTIFY_DIR_IS_ROOT              (0x0010)
#define NOTIFY_STREAM_IS_DELETED        (0x0020)

//
//      CAST
//      Add2Ptr (
//          IN PVOID Pointer,
//          IN ULONG Increment
//          IN (CAST)
//          );
//
//      ULONG
//      PtrOffset (
//          IN PVOID BasePtr,
//          IN PVOID OffsetPtr
//          );
//

#define Add2Ptr(PTR,INC,CAST) ((CAST)((PUCHAR)(PTR) + (INC)))

#define PtrOffset(BASE,OFFSET) ((ULONG)((PCHAR)(OFFSET) - (PCHAR)(BASE)))

//
//      VOID
//      SetFlag (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//
//      VOID
//      ClearFlag (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//

#define SetFlag(F,SF) {     \
    (F) |= (SF);            \
}

#define ClearFlag(F,SF) {   \
    (F) &= ~(SF);           \
}

//
//  VOID
//  AcquireNotifySync (
//      IN PREAL_NOTIFY_SYNC NotifySync
//      );
//
//  VOID
//  ReleaseNotifySync (
//      IN PREAL_NOTIFY_SYNC NotifySync
//      );
//

#define AcquireNotifySync(NS) {                                             \
    ERESOURCE_THREAD _CurrentThread;                                        \
    _CurrentThread = (ERESOURCE_THREAD) PsGetCurrentThread();               \
    if (_CurrentThread != ((PREAL_NOTIFY_SYNC) (NS))->OwningThread) {       \
        ExAcquireFastMutexUnsafe( &((PREAL_NOTIFY_SYNC) (NS))->FastMutex ); \
        ((PREAL_NOTIFY_SYNC) (NS))->OwningThread = _CurrentThread;          \
    }                                                                       \
    ((PREAL_NOTIFY_SYNC) (NS))->OwnerCount += 1;                            \
}

#define ReleaseNotifySync(NS) {                                             \
    ((PREAL_NOTIFY_SYNC) (NS))->OwnerCount -= 1;                            \
    if (((PREAL_NOTIFY_SYNC) (NS))->OwnerCount == 0) {                      \
        ((PREAL_NOTIFY_SYNC) (NS))->OwningThread = (ERESOURCE_THREAD) 0;    \
        ExReleaseFastMutexUnsafe(&((PREAL_NOTIFY_SYNC) (NS))->FastMutex);   \
    }                                                                       \
}

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('NrSF')


//
//  Local support routines
//

PNOTIFY_CHANGE
FsRtlIsNotifyOnList (
    IN PLIST_ENTRY NotifyListHead,
    IN PVOID FsContext
    );

VOID
FsRtlNotifyCompleteIrp (
    IN PIRP NotifyIrp,
    IN PNOTIFY_CHANGE Notify,
    IN ULONG DataLength,
    IN NTSTATUS Status,
    IN ULONG CheckCancel
    );

BOOLEAN
FsRtlNotifySetCancelRoutine (
    IN PIRP NotifyIrp,
    IN PNOTIFY_CHANGE Notify OPTIONAL
    );

BOOLEAN
FsRtlNotifyUpdateBuffer (
    IN PFILE_NOTIFY_INFORMATION NotifyInfo,
    IN ULONG FileAction,
    IN PSTRING ParentName,
    IN PSTRING TargetName,
    IN PSTRING StreamName OPTIONAL,
    IN BOOLEAN UnicodeName,
    IN ULONG SizeOfEntry
    );

VOID
FsRtlNotifyCompleteIrpList (
    IN PNOTIFY_CHANGE Notify,
    IN NTSTATUS Status
    );

VOID
FsRtlCancelNotify (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP ThisIrp
    );

VOID
FsRtlCheckNotifyForDelete (
    IN PLIST_ENTRY NotifyListHead,
    IN PVOID FsContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsRtlNotifyInitializeSync)
#pragma alloc_text(PAGE, FsRtlNotifyUninitializeSync)
#pragma alloc_text(PAGE, FsRtlNotifyFullChangeDirectory)
#pragma alloc_text(PAGE, FsRtlNotifyFullReportChange)
#pragma alloc_text(PAGE, FsRtlIsNotifyOnList)
#pragma alloc_text(PAGE, FsRtlNotifyChangeDirectory)
#pragma alloc_text(PAGE, FsRtlNotifyCleanup)
#pragma alloc_text(PAGE, FsRtlNotifyCompleteIrp)
#pragma alloc_text(PAGE, FsRtlNotifyReportChange)
#pragma alloc_text(PAGE, FsRtlNotifyUpdateBuffer)
#pragma alloc_text(PAGE, FsRtlCheckNotifyForDelete)
#endif


NTKERNELAPI
VOID
FsRtlNotifyInitializeSync (
    IN PNOTIFY_SYNC *NotifySync
    )

/*++

Routine Description:

    This routine is called to allocate and initialize the synchronization object
    for this notify list.

Arguments:

    NotifySync  -  This is the address to store the structure we allocate.

Return Value:

    None.

--*/

{
    PREAL_NOTIFY_SYNC RealSync;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlNotifyInitializeSync:  Entered\n", 0 );

    //
    //  Clear the pointer and then attempt to allocate a non-paged
    //  structure.
    //

    *NotifySync = NULL;

    RealSync = (PREAL_NOTIFY_SYNC) FsRtlpAllocatePool( NonPagedPool,
                                                       sizeof( REAL_NOTIFY_SYNC ));

    //
    //  Initialize the structure.
    //

    ExInitializeFastMutex( &RealSync->FastMutex );
    RealSync->OwningThread = (ERESOURCE_THREAD) 0;
    RealSync->OwnerCount = 0;

    *NotifySync = (PNOTIFY_SYNC) RealSync;

    DebugTrace( -1, Dbg, "FsRtlNotifyInitializeSync:  Exit\n", 0 );
    return;
}


NTKERNELAPI
VOID
FsRtlNotifyUninitializeSync (
    IN PNOTIFY_SYNC *NotifySync
    )

/*++

Routine Description:

    This routine is called to uninitialize the synchronization object
    for this notify list.

Arguments:

    NotifySync  -  This is the address containing the pointer to our synchronization
        object.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlNotifyUninitializeSync:  Entered\n", 0 );

    //
    //  Free the structure if present and clear the pointer.
    //

    if (*NotifySync != NULL) {

        ExFreePool( *NotifySync );
        *NotifySync = NULL;
    }

    DebugTrace( -1, Dbg, "FsRtlNotifyUninitializeSync:  Exit\n", 0 );
    return;
}


VOID
FsRtlNotifyChangeDirectory (
    IN PNOTIFY_SYNC NotifySync,
    IN PVOID FsContext,
    IN PSTRING FullDirectoryName,
    IN PLIST_ENTRY NotifyList,
    IN BOOLEAN WatchTree,
    IN ULONG CompletionFilter,
    IN PIRP NotifyIrp
    )

/*++

Routine Description:

    This routine is called by a file system which has received a NotifyChange
    request.  This routine checks if there is already a notify structure and
    inserts one if not present.  With a notify structure in hand, we check
    whether we already have a pending notify and report it if so.  If there
    is no pending notify, we check if this Irp has already been cancelled and
    completes it if so.  Otherwise we add this to the list of Irps waiting
    for notification.

Arguments:

    NotifySync  -  This is the controlling fast mutex for this notify list.
        It is stored here so that it can be found for an Irp which is being
        cancelled.

    FsContext  -  This is supplied by the file system so that this notify
                  structure can be uniquely identified.

    FullDirectoryName  -  Points to the full name for the directory associated
                          with this notify structure.

    NotifyList  -  This is the start of the notify list to add this
                   structure to.

    WatchTree  -  This indicates whether all subdirectories for this directory
                  should be watched, or just the directory itself.

    CompletionFilter  -  This provides the mask to determine which operations
                         will trigger the notify operations.

    NotifyIrp  -  This is the Irp to complete on notify change.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlNotifyChangeDirectory:  Entered\n", 0 );

    //
    //  We will simply call the full notify routine to do the real work.
    //

    FsRtlNotifyFullChangeDirectory( NotifySync,
                                    NotifyList,
                                    FsContext,
                                    FullDirectoryName,
                                    WatchTree,
                                    TRUE,
                                    CompletionFilter,
                                    NotifyIrp,
                                    NULL,
                                    NULL );

    DebugTrace( -1, Dbg, "FsRtlNotifyChangeDirectory:  Exit\n", 0 );

    return;
}


VOID
FsRtlNotifyFullChangeDirectory (
    IN PNOTIFY_SYNC NotifySync,
    IN PLIST_ENTRY NotifyList,
    IN PVOID FsContext,
    IN PSTRING FullDirectoryName,
    IN BOOLEAN WatchTree,
    IN BOOLEAN IgnoreBuffer,
    IN ULONG CompletionFilter,
    IN PIRP NotifyIrp,
    IN PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback OPTIONAL,
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext OPTIONAL
    )

/*++

Routine Description:

    This routine is called by a file system which has received a NotifyChange
    request.  This routine checks if there is already a notify structure and
    inserts one if not present.  With a notify structure in hand, we check
    whether we already have a pending notify and report it if so.  If there
    is no pending notify, we check if this Irp has already been cancelled and
    completes it if so.  Otherwise we add this to the list of Irps waiting
    for notification.

    This is the version of this routine which understands about the user's
    buffer and will fill it in on a reported change.

Arguments:

    NotifySync  -  This is the controlling fast mutex for this notify list.
        It is stored here so that it can be found for an Irp which is being
        cancelled.

    NotifyList  -  This is the start of the notify list to add this
        structure to.

    FsContext  -  This is supplied by the file system so that this notify
        structure can be uniquely identified.  If the NotifyIrp is not specified
        then this is used to identify the stream and it will match the FsContext
        field in the file object of a stream being deleted.

    FullDirectoryName  -  Points to the full name for the directory associated
        with this notify structure.  Ignored if the NotifyIrp is not specified.

    WatchTree  -  This indicates whether all subdirectories for this directory
        should be watched, or just the directory itself.  Ignored if the
        NotifyIrp is not specified.

    IgnoreBuffer  -  Indicates whether we will always ignore any user buffer
        and force the directory to be reenumerated.  This will speed up the
        operation.  Ignored if the NotifyIrp is not specified.

    CompletionFilter  -  This provides the mask to determine which operations
        will trigger the notify operations.  Ignored if the NotifyIrp is not
        specified.

    NotifyIrp  -  This is the Irp to complete on notify change.  If this irp is
        not specified it means that the stream represented by this file object
        is being deleted.

    TraverseCallback  -  If specified we must call this routine when a change
        has occurred in a subdirectory being watched in a tree.  This will
        let the filesystem check if the watcher has traverse access to that
        directory.  Ignored if the NotifyIrp is not specified.

    SubjectContext - If there is a traverse callback routine then we will
        pass this subject context as a parameter to the call.  We will release
        the context and free the structure when done with it.  Ignored if the
        NotifyIrp is not specified.

Return Value:

    None.

--*/

{
    PNOTIFY_CHANGE Notify;
    PIO_STACK_LOCATION IrpSp;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlNotifyFullChangeDirectory:  Entered\n", 0 );

    //
    //  Acquire exclusive access to the list by acquiring the mutex.
    //

    AcquireNotifySync( NotifySync );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If there is no Irp then find all of the pending Irps whose file objects
        //  refer to the same stream and complete them with STATUS_DELETE_PENDING.
        //

        if (NotifyIrp == NULL) {

            FsRtlCheckNotifyForDelete( NotifyList, FsContext );
            try_return( NOTHING );
        }

        //
        //  Get the current Stack location
        //

        IrpSp = IoGetCurrentIrpStackLocation( NotifyIrp );

        //
        //  Clear the Iosb in the Irp.
        //

        NotifyIrp->IoStatus.Status = STATUS_SUCCESS;
        NotifyIrp->IoStatus.Information = 0;

        //
        //  If the file object has already gone through cleanup, then complete
        //  the request immediately.
        //

        if (FlagOn( IrpSp->FileObject->Flags, FO_CLEANUP_COMPLETE )) {

            //
            //  Always mark this Irp as pending returned.
            //

            IoMarkIrpPending( NotifyIrp );

            FsRtlCompleteRequest( NotifyIrp, STATUS_NOTIFY_CLEANUP );
            try_return( NOTHING );
        }

        //
        //  If the notify structure is not already in the list, add it
        //  now.
        //

        Notify = FsRtlIsNotifyOnList( NotifyList, FsContext );

        if (Notify == NULL) {

            //
            //  Allocate and initialize the structure.
            //

            Notify = FsRtlpAllocatePool( PagedPool, sizeof( NOTIFY_CHANGE ));
            RtlZeroMemory( Notify, sizeof( NOTIFY_CHANGE ));

            Notify->NotifySync = (PREAL_NOTIFY_SYNC) NotifySync;
            Notify->FsContext = FsContext;
            Notify->StreamID = IrpSp->FileObject->FsContext;

            Notify->TraverseCallback = TraverseCallback;
            Notify->SubjectContext = SubjectContext;
            SubjectContext = NULL;

            Notify->FullDirectoryName = FullDirectoryName;

            InitializeListHead( &Notify->NotifyIrps );

            if (WatchTree) {

                SetFlag( Notify->Flags, NOTIFY_WATCH_TREE );
            }

            if (FullDirectoryName == NULL) {

                //
                //  In the view index we aren't using this buffer to hold a
                //  unicode string.
                //

                Notify->CharacterSize = sizeof( CHAR );

            } else {

                //
                //  We look at the directory name to decide if we have a unicode
                //  name.
                //

                if (FullDirectoryName->Length >= 2
                    && FullDirectoryName->Buffer[1] == '\0') {

                    Notify->CharacterSize = sizeof( WCHAR );

                } else {

                    Notify->CharacterSize = sizeof( CHAR );
                }

                if (FullDirectoryName->Length == Notify->CharacterSize) {

                    SetFlag( Notify->Flags, NOTIFY_DIR_IS_ROOT );
                }
            }

            Notify->CompletionFilter = CompletionFilter;

            //
            //  If we are to return data to the user then look for the length
            //  of the original buffer in the IrpSp.
            //

            if (!IgnoreBuffer) {

                Notify->BufferLength = IrpSp->Parameters.NotifyDirectory.Length;
            }

            Notify->OwningProcess = THREAD_TO_PROCESS( NotifyIrp->Tail.Overlay.Thread );
            InsertTailList( NotifyList, &Notify->NotifyList );

            Notify->ReferenceCount = 1;

        //
        //  If we have already been called with cleanup then complete
        //  the request immediately.
        //

        } else if (FlagOn( Notify->Flags, NOTIFY_CLEANUP_CALLED )) {

            //
            //  Always mark this Irp as pending returned.
            //

            IoMarkIrpPending( NotifyIrp );

            FsRtlCompleteRequest( NotifyIrp, STATUS_NOTIFY_CLEANUP );
            try_return( NOTHING );

        //
        //  If this file has been deleted then complete with STATUS_DELETE_PENDING.
        //

        } else if (FlagOn( Notify->Flags, NOTIFY_STREAM_IS_DELETED )) {

            //
            //  Always mark this Irp as pending returned.
            //

            IoMarkIrpPending( NotifyIrp );

            FsRtlCompleteRequest( NotifyIrp, STATUS_DELETE_PENDING );
            try_return( NOTHING );

        //
        //  If the notify pending flag is set or there is data in an internal buffer
        //  we complete this Irp immediately and exit.
        //

        } else if (FlagOn( Notify->Flags, NOTIFY_IMMEDIATE_NOTIFY )
                   && !FlagOn( Notify->Flags, NOTIFY_DEFER_NOTIFY )) {

            DebugTrace( 0, Dbg, "Notify has been pending\n", 0 );

            //
            //  Clear the flag in our notify structure before completing the
            //  Irp.  This will prevent a caller who reposts in his completion
            //  routine from looping in the completion routine.
            //

            ClearFlag( Notify->Flags, NOTIFY_IMMEDIATE_NOTIFY );

            //
            //  Always mark this Irp as pending returned.
            //

            IoMarkIrpPending( NotifyIrp );

            FsRtlCompleteRequest( NotifyIrp, STATUS_NOTIFY_ENUM_DIR );
            try_return( NOTHING );

        } else if (Notify->DataLength != 0
                   && !FlagOn( Notify->Flags, NOTIFY_DEFER_NOTIFY )) {

            ULONG ThisDataLength = Notify->DataLength;

            //
            //  Now set our buffer pointers back to indicate an empty buffer.
            //

            Notify->DataLength = 0;
            Notify->LastEntry = 0;

            FsRtlNotifyCompleteIrp( NotifyIrp,
                                    Notify,
                                    ThisDataLength,
                                    STATUS_SUCCESS,
                                    FALSE );

            try_return( NOTHING );
        }

        //
        //  Add the Irp to the tail of the notify queue.
        //

        NotifyIrp->IoStatus.Information = (ULONG_PTR) Notify;
        IoMarkIrpPending( NotifyIrp );
        InsertTailList( &Notify->NotifyIrps, &NotifyIrp->Tail.Overlay.ListEntry );

        //
        //  Increment the reference count to indicate that Irp might go through cancel.
        //

        InterlockedIncrement( &Notify->ReferenceCount );

        //
        //  Call the routine to set the cancel routine.
        //

        FsRtlNotifySetCancelRoutine( NotifyIrp, NULL );

    try_exit:  NOTHING;
    } finally {

        //
        //  Release the mutex.
        //

        ReleaseNotifySync( NotifySync );

        //
        //  If there is still a subject context then release it and deallocate
        //  the structure.  Remember that if FullDirectoryName is null, it means
        //  this is a view index, not a directory index, and the SubjectContext
        //  is really a piece of file system context information.
        //

        if ((SubjectContext != NULL) &&
            (Notify->FullDirectoryName != NULL)) {

            SeReleaseSubjectContext( SubjectContext );
            ExFreePool( SubjectContext );
        }

        DebugTrace( -1, Dbg, "FsRtlNotifyFullChangeDirectory:  Exit\n", 0 );
    }

    return;
}


VOID
FsRtlNotifyReportChange (
    IN PNOTIFY_SYNC NotifySync,
    IN PLIST_ENTRY NotifyList,
    IN PSTRING FullTargetName,
    IN PSTRING TargetName,
    IN ULONG FilterMatch
    )

/*++

Routine Description:

    This routine is called by a file system when a file has been modified in
    such a way that it will cause a notify change Irp to complete.  We walk
    through all the notify structures looking for those structures which
    would be associated with an ancestor directory of the target file name.

    We look for all the notify structures which have a filter match and
    then check that the directory name in the notify structure is a
    proper prefix of the full target name.

    If we find a notify structure which matches the above conditions, we
    complete all the Irps for the notify structure.  If the structure has
    no Irps, we mark the notify pending field.

Arguments:

    NotifySync  -  This is the controlling fast mutex for this notify list.
        It is stored here so that it can be found for an Irp which is being
        cancelled.

    NotifyList  -  This is the start of the notify list to add this
                   structure to.

    FullTargetName  -  This is the full name of the file which has been
                       changed.

    TargetName  -  This is the final component of the modified file.

    FilterMatch  -  This flag field is compared with the completion filter
                    in the notify structure.  If any of the corresponding
                    bits in the completion filter are set, then a notify
                    condition exists.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlNotifyReportChange:  Entered\n", 0 );

    //
    //  Call the full notify routine to do the actual work.
    //

    FsRtlNotifyFullReportChange( NotifySync,
                                 NotifyList,
                                 FullTargetName,
                                 (USHORT) (FullTargetName->Length - TargetName->Length),
                                 NULL,
                                 NULL,
                                 FilterMatch,
                                 0,
                                 NULL );

    DebugTrace( -1, Dbg, "FsRtlNotifyReportChange:  Exit\n", 0 );

    return;
}


VOID
FsRtlNotifyFullReportChange (
    IN PNOTIFY_SYNC NotifySync,
    IN PLIST_ENTRY NotifyList,
    IN PSTRING FullTargetName,
    IN USHORT TargetNameOffset,
    IN PSTRING StreamName OPTIONAL,
    IN PSTRING NormalizedParentName OPTIONAL,
    IN ULONG FilterMatch,
    IN ULONG Action,
    IN PVOID TargetContext
    )

/*++

Routine Description:

    This routine is called by a file system when a file has been modified in
    such a way that it will cause a notify change Irp to complete.  We walk
    through all the notify structures looking for those structures which
    would be associated with an ancestor directory of the target file name.

    We look for all the notify structures which have a filter match and
    then check that the directory name in the notify structure is a
    proper prefix of the full target name.

    If we find a notify structure which matches the above conditions, we
    complete all the Irps for the notify structure.  If the structure has
    no Irps, we mark the notify pending field.

Arguments:

    NotifySync  -  This is the controlling fast mutex for this notify list.
        It is stored here so that it can be found for an Irp which is being
        cancelled.

    NotifyList  -  This is the start of the notify list to add this
        structure to.

    FullTargetName - This is the full name of the file from the root of the volume.

    TargetNameOffset - This is the offset in the full name of the final component
        of the name.

    StreamName  -  If present then this is the stream name to store with
        the filename.

    NormalizedParentName  -  If present this is the same path as the parent name
        but the DOS-ONLY names have been replaced with the associated long name.

    FilterMatch  -  This flag field is compared with the completion filter
        in the notify structure.  If any of the corresponding bits in the
        completion filter are set, then a notify condition exists.

    Action  -  This is the action code to store in the user's buffer if
        present.

    TargetContext  -  This is one of the context pointers to pass to the file
        system if performing a traverse check in the case of a tree being
        watched.

Return Value:

    None.

--*/

{
    PLIST_ENTRY NotifyLinks;

    STRING NormalizedParent;
    STRING ParentName;
    STRING TargetName;

    PNOTIFY_CHANGE Notify;
    STRING TargetParent;
    PIRP NotifyIrp;

    BOOLEAN NotifyIsParent;
    BOOLEAN ViewIndex = FALSE;
    UCHAR ComponentCount;
    ULONG SizeOfEntry;
    ULONG CurrentOffset;
    ULONG NextEntryOffset;
    ULONG ExceptionCode;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlNotifyFullReportChange:  Entered\n", 0 );

    //
    //  If this is a change to the root directory then return immediately.
    //

    if ((TargetNameOffset == 0) && (FullTargetName != NULL)) {

        DebugTrace( -1, Dbg, "FsRtlNotifyFullReportChange:  Exit\n", 0 );
        return;
    }

    ParentName.Buffer = NULL;
    TargetName.Buffer = NULL;

    //
    //  Acquire exclusive access to the list by acquiring the mutex.
    //

    AcquireNotifySync( NotifySync );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Walk through all the notify blocks.
        //

        for (NotifyLinks = NotifyList->Flink;
             NotifyLinks != NotifyList;
             NotifyLinks = NotifyLinks->Flink) {

            //
            //  Obtain the Notify structure from the list entry.
            //

            Notify = CONTAINING_RECORD( NotifyLinks, NOTIFY_CHANGE, NotifyList );

            //
            //  The rules for deciding whether this notification applies are
            //  different for view indices versus file name indices (directories).
            //

            if (FullTargetName == NULL) {

                ASSERTMSG( "Directory notify handle in view index notify list!", Notify->FullDirectoryName == NULL);

                //
                //  Make sure this is the Fcb being watched.
                //

                if (TargetContext != Notify->SubjectContext) {

                    continue;
                }

                TargetParent.Buffer = NULL;
                TargetParent.Length = 0;

                ViewIndex = TRUE;

            //
            //  Handle the directory case.
            //

            } else {

                ASSERTMSG( "View index notify handle in directory notify list!", Notify->FullDirectoryName != NULL);

                //
                //  If the length of the name in the notify block is currently zero then
                //  someone is doing a rename and we can skip this block.
                //

                if (Notify->FullDirectoryName->Length == 0) {

                    continue;
                }

                //
                //  If this filter match is not part of the completion filter then continue.
                //

                if (!(FilterMatch & Notify->CompletionFilter)) {

                    continue;
                }

                //
                //  If there is no normalized name then set its value from the full
                //  file name.
                //

                if (!ARGUMENT_PRESENT( NormalizedParentName )) {
                    NormalizedParent.Buffer = FullTargetName->Buffer;
                    NormalizedParent.Length = TargetNameOffset;

                    if (NormalizedParent.Length != Notify->CharacterSize) {

                        NormalizedParent.Length -= Notify->CharacterSize;
                    }

                    NormalizedParent.MaximumLength = NormalizedParent.Length;

                    NormalizedParentName = &NormalizedParent;
                }

                //
                //  If the length of the directory being watched is longer than the
                //  parent of the modified file then it can't be an ancestor of the
                //  modified file.
                //

                if (Notify->FullDirectoryName->Length > NormalizedParentName->Length) {

                    continue;
                }

                //
                //  If the lengths match exactly then this can only be the parent of
                //  the modified file.
                //

                if (NormalizedParentName->Length == Notify->FullDirectoryName->Length) {

                    NotifyIsParent = TRUE;

                //
                //  If we are not watching the subtree of this directory then continue.
                //

                } else if (!FlagOn( Notify->Flags, NOTIFY_WATCH_TREE )) {

                    continue;

                //
                //  The watched directory can only be an ancestor of the modified
                //  file.  Make sure that there is legal pathname separator immediately
                //  after the end of the watched directory name within the normalized name.
                //  If the watched directory is the root then we know this condition is TRUE.
                //

                } else {

                    if (!FlagOn( Notify->Flags, NOTIFY_DIR_IS_ROOT )) {

                        //
                        //  Check for the character size.
                        //

                        if (Notify->CharacterSize == sizeof( CHAR )) {

                            if (*(Add2Ptr( NormalizedParentName->Buffer,
                                           Notify->FullDirectoryName->Length,
                                           PCHAR )) != '\\') {

                                continue;
                            }

                        } else if (*(Add2Ptr( NormalizedParentName->Buffer,
                                              Notify->FullDirectoryName->Length,
                                              PWCHAR )) != L'\\') {

                            continue;
                        }
                    }

                    NotifyIsParent = FALSE;
                }

                //
                //  We now have a correct match of the name lengths.  Now verify that the
                //  characters match exactly.
                //

                if (!RtlEqualMemory( Notify->FullDirectoryName->Buffer,
                                     NormalizedParentName->Buffer,
                                     Notify->FullDirectoryName->Length )) {

                    continue;
                }

                //
                //  The characters are correct.  Now check in the case of a non-parent
                //  notify that we have traverse callback.
                //

                if (!NotifyIsParent &&
                    Notify->TraverseCallback != NULL &&
                    !Notify->TraverseCallback( Notify->FsContext,
                                               TargetContext,
                                               Notify->SubjectContext )) {

                    continue;
                }
            }

            //
            //  If this entry is going into a buffer then check that
            //  it will fit.
            //

            if (!FlagOn( Notify->Flags, NOTIFY_IMMEDIATE_NOTIFY )
                && Notify->BufferLength != 0) {

                ULONG AllocationLength;

                AllocationLength = 0;
                NotifyIrp = NULL;

                //
                //  If we don't already have a buffer then check to see
                //  if we have any Irps in the list and use the buffer
                //  length in the Irp.
                //

                if (Notify->ThisBufferLength == 0) {

                    //
                    //  If there is an entry in the list then get the length.
                    //

                    if (!IsListEmpty( &Notify->NotifyIrps )) {

                        PIO_STACK_LOCATION IrpSp;

                        NotifyIrp = CONTAINING_RECORD( Notify->NotifyIrps.Flink,
                                                       IRP,
                                                       Tail.Overlay.ListEntry );

                        IrpSp = IoGetCurrentIrpStackLocation( NotifyIrp );

                        AllocationLength = IrpSp->Parameters.NotifyDirectory.Length;

                    //
                    //  Otherwise use the caller's last buffer size.
                    //

                    } else {

                        AllocationLength = Notify->BufferLength;
                    }

                //
                //  Otherwise use the length of the current buffer.
                //

                } else {

                    AllocationLength = Notify->ThisBufferLength;
                }

                //
                //  Build the strings for the relative name.  This includes
                //  the strings for the parent name, file name and stream
                //  name.
                //

                if (!NotifyIsParent) {

                    //
                    //  We need to find the string for the ancestor of this
                    //  file from the watched directory.  If the normalized parent
                    //  name is the same as the parent name then we can use
                    //  the tail of the parent directly.  Otherwise we need to
                    //  count the matching name components and capture the
                    //  final components.
                    //

                    if (!ViewIndex) {

                        //
                        //  If the watched directory is the root then we just use the full
                        //  parent name.
                        //

                        if (FlagOn( Notify->Flags, NOTIFY_DIR_IS_ROOT ) ||
                            NormalizedParentName->Buffer != FullTargetName->Buffer) {

                            //
                            //  If we don't have a string for the parent then construct
                            //  it now.
                            //

                            if (ParentName.Buffer == NULL) {

                                ParentName.Buffer = FullTargetName->Buffer;
                                ParentName.Length = TargetNameOffset;

                                if (ParentName.Length != Notify->CharacterSize) {

                                    ParentName.Length -= Notify->CharacterSize;
                                }

                                ParentName.MaximumLength = ParentName.Length;
                            }

                            //
                            //  Count through the components of the parent until we have
                            //  swallowed the same number of name components as in the
                            //  watched directory name.  We have the unicode version and
                            //  the Ansi version to watch for.
                            //

                            ComponentCount = 0;
                            CurrentOffset = 0;

                            //
                            //  If this is the root then there is no more to do.
                            //

                            if (FlagOn( Notify->Flags, NOTIFY_DIR_IS_ROOT )) {

                                NOTHING;

                            } else {

                                ULONG ParentComponentCount;
                                ULONG ParentOffset;

                                ParentComponentCount = 1;
                                ParentOffset = 0;

                                if (Notify->CharacterSize == sizeof( CHAR )) {

                                    //
                                    //  Find the number of components in the parent.  We
                                    //  have to do this for each one because this name and
                                    //  the number of components could have changed.
                                    //

                                    while (ParentOffset < Notify->FullDirectoryName->Length) {

                                        if (*((PCHAR) Notify->FullDirectoryName->Buffer + ParentOffset) == '\\') {

                                            ParentComponentCount += 1;
                                        }

                                        ParentOffset += 1;
                                    }

                                    while (TRUE) {

                                        if (*((PCHAR) ParentName.Buffer + CurrentOffset) == '\\') {

                                            ComponentCount += 1;

                                            if (ComponentCount == ParentComponentCount) {

                                                break;
                                            }

                                        }

                                        CurrentOffset += 1;
                                    }

                                } else {

                                    //
                                    //  Find the number of components in the parent.  We
                                    //  have to do this for each one because this name and
                                    //  the number of components could have changed.
                                    //

                                    while (ParentOffset < Notify->FullDirectoryName->Length / sizeof( WCHAR )) {

                                        if (*((PWCHAR) Notify->FullDirectoryName->Buffer + ParentOffset) == '\\') {

                                            ParentComponentCount += 1;
                                        }

                                        ParentOffset += 1;
                                    }

                                    while (TRUE) {

                                        if (*((PWCHAR) ParentName.Buffer + CurrentOffset) == L'\\') {

                                            ComponentCount += 1;

                                            if (ComponentCount == ParentComponentCount) {

                                                break;
                                            }
                                        }

                                        CurrentOffset += 1;
                                    }

                                    //
                                    //  Convert characters to bytes.
                                    //

                                    CurrentOffset *= Notify->CharacterSize;
                                }
                            }

                            //
                            //  We now know the offset into the parent name of the separator
                            //  immediately preceding the relative parent name.  Construct the
                            //  target parent name for the buffer.
                            //

                            CurrentOffset += Notify->CharacterSize;

                            TargetParent.Buffer = Add2Ptr( ParentName.Buffer,
                                                           CurrentOffset,
                                                           PCHAR );
                            TargetParent.MaximumLength =
                            TargetParent.Length = ParentName.Length - (USHORT) CurrentOffset;

                        //
                        //  If the normalized is the same as the parent name use the portion
                        //  after the match with the watched directory.
                        //

                        } else {

                            TargetParent.Buffer = Add2Ptr( NormalizedParentName->Buffer,
                                                           (Notify->FullDirectoryName->Length +
                                                            Notify->CharacterSize),
                                                           PCHAR );

                            TargetParent.MaximumLength =
                            TargetParent.Length = NormalizedParentName->Length -
                                                  Notify->FullDirectoryName->Length -
                                                  Notify->CharacterSize;

                        }
                    }

                } else {

                    //
                    //  The length of the target parent is zero.
                    //

                    TargetParent.Length = 0;
                }

                //
                //  Compute how much buffer space this report will take.
                //

                SizeOfEntry = FIELD_OFFSET( FILE_NOTIFY_INFORMATION, FileName );

                if (ViewIndex) {

                    //
                    //  In the view index case, the information to copy to the
                    //  buffer comes to us in the stream name, and that is all
                    //  the room we need to worry about having.
                    //

                    ASSERT(ARGUMENT_PRESENT( StreamName ));

                    SizeOfEntry += StreamName->Length;

                } else {

                    //
                    //  If there is a parent to report, find the size and include a separator
                    //  character.
                    //

                    if (!NotifyIsParent) {

                        if (Notify->CharacterSize == sizeof( CHAR )) {

                            SizeOfEntry += RtlOemStringToCountedUnicodeSize( &TargetParent );

                        } else {

                            SizeOfEntry += TargetParent.Length;
                        }

                        //
                        //  Include the separator.  This is always a unicode character.
                        //

                        SizeOfEntry += sizeof( WCHAR );
                    }

                    //
                    //  If we don't have the string for the target then construct it now.
                    //

                    if (TargetName.Buffer == NULL) {

                        TargetName.Buffer = Add2Ptr( FullTargetName->Buffer, TargetNameOffset, PCHAR );
                        TargetName.MaximumLength =
                        TargetName.Length = FullTargetName->Length - TargetNameOffset;
                    }

                    if (Notify->CharacterSize == sizeof( CHAR )) {

                        SizeOfEntry += RtlOemStringToCountedUnicodeSize( &TargetName );

                    } else {

                        SizeOfEntry += TargetName.Length;
                    }

                    //
                    //  If there is a stream name then add the bytes needed
                    //  for that.
                    //

                    if (ARGUMENT_PRESENT( StreamName )) {

                        //
                        //  Add the space needed for the ':' separator.
                        //

                        if (Notify->CharacterSize == sizeof( WCHAR )) {

                            SizeOfEntry += (StreamName->Length + sizeof( WCHAR ));

                        } else {

                            SizeOfEntry += (RtlOemStringToCountedUnicodeSize( StreamName )
                                            + sizeof( CHAR ));
                        }
                    }
                }

                //
                //  Remember if this report would overflow the buffer.
                //

                NextEntryOffset = (ULONG)LongAlign( Notify->DataLength );

                if (SizeOfEntry <= AllocationLength
                    && (NextEntryOffset + SizeOfEntry) <= AllocationLength) {

                    PFILE_NOTIFY_INFORMATION NotifyInfo = NULL;

                    //
                    //  If there is already a notify buffer, we append this
                    //  data to it.
                    //

                    if (Notify->Buffer != NULL) {

                        NotifyInfo = Add2Ptr( Notify->Buffer,
                                              Notify->LastEntry,
                                              PFILE_NOTIFY_INFORMATION );

                        NotifyInfo->NextEntryOffset = NextEntryOffset - Notify->LastEntry;

                        Notify->LastEntry = NextEntryOffset;

                        NotifyInfo = Add2Ptr( Notify->Buffer,
                                              Notify->LastEntry,
                                              PFILE_NOTIFY_INFORMATION );

                    //
                    //  If there is an Irp list we check whether we will need
                    //  to allocate a new buffer.
                    //

                    } else if (NotifyIrp != NULL) {

                        if (NotifyIrp->AssociatedIrp.SystemBuffer != NULL) {

                            Notify->Buffer =
                            NotifyInfo = NotifyIrp->AssociatedIrp.SystemBuffer;

                            Notify->ThisBufferLength = AllocationLength;

                        } else if (NotifyIrp->MdlAddress != NULL) {

                            Notify->Buffer =
                            NotifyInfo = MmGetSystemAddressForMdl( NotifyIrp->MdlAddress );

                            Notify->ThisBufferLength = AllocationLength;
                        }
                    }

                    //
                    //  If we need to allocate a buffer, we will charge quota
                    //  to the original process and allocate paged pool.
                    //

                    if (Notify->Buffer == NULL) {

                        BOOLEAN ChargedQuota = FALSE;

                        try {

                            PsChargePoolQuota( Notify->OwningProcess,
                                               PagedPool,
                                               AllocationLength );

                            ChargedQuota = TRUE;

                            Notify->AllocatedBuffer =
                            Notify->Buffer = FsRtlpAllocatePool( PagedPool,
                                                                 AllocationLength );

                            Notify->ThisBufferLength = AllocationLength;

                            NotifyInfo = Notify->Buffer;

                        } except(( ExceptionCode = GetExceptionCode(), FsRtlIsNtstatusExpected(ExceptionCode))
                                  ? EXCEPTION_EXECUTE_HANDLER
                                  : EXCEPTION_CONTINUE_SEARCH ) {

                            
                            ASSERT( (ExceptionCode == STATUS_INSUFFICIENT_RESOURCES) ||
                                    (ExceptionCode == STATUS_QUOTA_EXCEEDED) );

                            //
                            //  Return quota if we allocated the buffer.
                            //

                            if (ChargedQuota) {

                                PsReturnPoolQuota( Notify->OwningProcess,
                                                   PagedPool,
                                                   AllocationLength );

                            }

                            //
                            //  Forget any current buffer and resort to immediate
                            //  notify.
                            //

                            SetFlag( Notify->Flags, NOTIFY_IMMEDIATE_NOTIFY );
                        }
                    }

                    //
                    //  If we have a buffer then fill in the results
                    //  from this operation.  Otherwise we remember
                    //  to simply alert the caller.
                    //

                    if (NotifyInfo != NULL) {

                        //
                        //  Update the buffer with the current data.
                        //

                        if (FsRtlNotifyUpdateBuffer( NotifyInfo,
                                                     Action,
                                                     &TargetParent,
                                                     &TargetName,
                                                     StreamName,
                                                     (BOOLEAN) (Notify->CharacterSize == sizeof( WCHAR )),
                                                     SizeOfEntry )) {

                            //
                            //  Update the buffer data length.
                            //

                            Notify->DataLength = NextEntryOffset + SizeOfEntry;

                        //
                        //  We couldn't copy the data into the buffer.  Just
                        //  notify without any additional information.
                        //

                        } else {

                            SetFlag( Notify->Flags, NOTIFY_IMMEDIATE_NOTIFY );
                        }
                    }

                } else {

                    SetFlag( Notify->Flags, NOTIFY_IMMEDIATE_NOTIFY );
                }

                //
                //  If we have a buffer but can't use it then clear all of the
                //  buffer related fields.  Also deallocate any buffer allocated
                //  by us.
                //

                if (FlagOn( Notify->Flags, NOTIFY_IMMEDIATE_NOTIFY )
                    && Notify->Buffer != NULL) {

                    if (Notify->AllocatedBuffer != NULL) {

                        PsReturnPoolQuota( Notify->OwningProcess,
                                           PagedPool,
                                           Notify->ThisBufferLength );

                        ExFreePool( Notify->AllocatedBuffer );
                    }

                    Notify->AllocatedBuffer = Notify->Buffer = NULL;

                    Notify->ThisBufferLength = Notify->DataLength = Notify->LastEntry = 0;
                }
            }

            //
            //  Complete the next entry on the list if we aren't holding
            //  up notification.
            //

            if (Action == FILE_ACTION_RENAMED_OLD_NAME) {

                SetFlag( Notify->Flags, NOTIFY_DEFER_NOTIFY );

            } else {

                ClearFlag( Notify->Flags, NOTIFY_DEFER_NOTIFY );

                if (!IsListEmpty( &Notify->NotifyIrps )) {

                    FsRtlNotifyCompleteIrpList( Notify, STATUS_SUCCESS );
                }
            }
        }

    } finally {

        ReleaseNotifySync( NotifySync );

        DebugTrace( -1, Dbg, "FsRtlNotifyFullReportChange:  Exit\n", 0 );
    }

    return;
}


VOID
FsRtlNotifyCleanup (
    IN PNOTIFY_SYNC NotifySync,
    IN PLIST_ENTRY NotifyList,
    IN PVOID FsContext
    )

/*++

Routine Description:

    This routine is called for a cleanup of a user directory handle.  We
    walk through our notify structures looking for a matching context field.
    We complete all the pending notify Irps for this Notify structure, remove
    the notify structure and deallocate it.

Arguments:

    NotifySync  -  This is the controlling fast mutex for this notify list.
        It is stored here so that it can be found for an Irp which is being
        cancelled.

    NotifyList  -  This is the start of the notify list to add this
                   structure to.

    FsContext  -  This is a unique value assigned by the file system to
                  identify a particular notify structure.

Return Value:

    None.

--*/

{
    PNOTIFY_CHANGE Notify;
    PSECURITY_SUBJECT_CONTEXT SubjectContext = NULL;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlNotifyCleanup:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Mutex             -> %08lx\n", Mutex );
    DebugTrace(  0, Dbg, "Notify List       -> %08lx\n", NotifyList );
    DebugTrace(  0, Dbg, "FsContext         -> %08lx\n", FsContext );

    //
    //  Acquire exclusive access to the list by acquiring the mutex.
    //

    AcquireNotifySync( NotifySync );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Search for a match on the list.
        //

        Notify = FsRtlIsNotifyOnList( NotifyList, FsContext );

        //
        //  If found, then complete all the Irps with STATUS_NOTIFY_CLEANUP
        //

        if (Notify != NULL) {

            //
            //  Set the flag to indicate that we have been called with cleanup.
            //

            SetFlag( Notify->Flags, NOTIFY_CLEANUP_CALLED );

            if (!IsListEmpty( &Notify->NotifyIrps )) {

                FsRtlNotifyCompleteIrpList( Notify, STATUS_NOTIFY_CLEANUP );
            }

            RemoveEntryList( &Notify->NotifyList );

            InterlockedDecrement( &Notify->ReferenceCount );

            if (Notify->ReferenceCount == 0) {
                
                if (Notify->AllocatedBuffer != NULL) {

                    PsReturnPoolQuota( Notify->OwningProcess,
                                       PagedPool,
                                       Notify->ThisBufferLength );

                    ExFreePool( Notify->AllocatedBuffer );
                }

                if (Notify->FullDirectoryName != NULL) {

                    SubjectContext = Notify->SubjectContext;
                }

                ExFreePool( Notify );
            }
        }

    } finally {

        ReleaseNotifySync( NotifySync );

        if (SubjectContext != NULL) {

            SeReleaseSubjectContext( SubjectContext );
            ExFreePool( SubjectContext );
        }

        DebugTrace( -1, Dbg, "FsRtlNotifyCleanup:  Exit\n", 0 );
    }

    return;
}


//
//  Local support routine
//

PNOTIFY_CHANGE
FsRtlIsNotifyOnList (
    IN PLIST_ENTRY NotifyListHead,
    IN PVOID FsContext
    )

/*++

Routine Description:

    This routine is called to walk through a notify list searching for
    a member associated with the FsContext value.

Arguments:

    NotifyListHead  -  This is the start of the notify list.

    FsContext  -  This is supplied by the file system so that this notify
                  structure can be uniquely identified.

Return Value:

    PNOTIFY_CHANGE - A pointer to the matching structure is returned.  NULL
                     is returned if the structure isn't present.

--*/

{
    PLIST_ENTRY Link;

    PNOTIFY_CHANGE ThisNotify;
    PNOTIFY_CHANGE Notify;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlIsNotifyOnList:  Entered\n", 0 );

    //
    //  Assume we won't have a match.
    //

    Notify = NULL;

    //
    //  Walk through all the entries on the list looking for a match.
    //

    for (Link = NotifyListHead->Flink;
         Link != NotifyListHead;
         Link = Link->Flink) {

        //
        //  Obtain the notify structure from the link.
        //

        ThisNotify = CONTAINING_RECORD( Link, NOTIFY_CHANGE, NotifyList );

        //
        //  If the context field matches, remember this structure and
        //  exit.
        //

        if (ThisNotify->FsContext == FsContext) {

            Notify = ThisNotify;
            break;
        }
    }

    DebugTrace(  0, Dbg, "Notify Structure  -> %08lx\n", Notify );
    DebugTrace( -1, Dbg, "FsRtlIsNotifyOnList:  Exit\n", 0 );

    return Notify;
}


//
//  Local support routine
//

VOID
FsRtlNotifyCompleteIrp (
    IN PIRP NotifyIrp,
    IN PNOTIFY_CHANGE Notify,
    IN ULONG DataLength,
    IN NTSTATUS Status,
    IN ULONG CheckCancel
    )

/*++

Routine Description:

    This routine is called to complete an Irp with the data in the NOTIFY_CHANGE
    structure.

Arguments:

    NotifyIrp  -  Irp to complete.

    Notify  -  Notify structure which contains the data.

    DataLength  -  This is the length of the data in the buffer in the notify
        structure.  A value of zero indicates that we should complete the
        request with STATUS_NOTIFY_ENUM_DIR.

    Status  -  Indicates the status to complete the Irp with.

    CheckCancel - Indicates if we should only complete the irp if we clear the cancel routine
        ourselves.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlIsNotifyCompleteIrp:  Entered\n", 0 );

    //
    //  Attempt to clear the cancel routine.  If this routine owns the cancel
    //  routine then it can complete the irp.  Otherwise there is a cancel underway
    //  on this.
    //

    if (FsRtlNotifySetCancelRoutine( NotifyIrp, Notify ) || !CheckCancel) {

        //
        //  We only process the buffer if the status is STATUS_SUCCESS.
        //

        if (Status == STATUS_SUCCESS) {

            //
            //  Get the current Stack location
            //

            IrpSp = IoGetCurrentIrpStackLocation( NotifyIrp );

            //
            //  If the data won't fit in the user's buffer or there was already a
            //  buffer overflow then return the alternate status code.  If the data
            //  was already stored in the Irp buffer then we know that we won't
            //  take this path.  Otherwise we wouldn't be cleaning up the Irp
            //  correctly.
            //

            if (DataLength == 0
                || IrpSp->Parameters.NotifyDirectory.Length < DataLength) {

                Status = STATUS_NOTIFY_ENUM_DIR;

            //
            //  We have to carefully return the buffer to the user and handle all
            //  of the different buffer cases.  If there is no allocated buffer
            //  in the notify structure it means that we have already used the
            //  caller's buffer.
            //
            //  1 - If the system allocated an associated system buffer we
            //      can simply fill that in.
            //
            //  2 - If there is an Mdl then we get a system address for the Mdl
            //      and copy the data into it.
            //
            //  3 - If there is only a user's buffer and pending has not been
            //      returned, we can fill the user's buffer in directly.
            //
            //  4 - If there is only a user's buffer and pending has been returned
            //      then we are not in the user's address space.  We dress up
            //      the Irp with our system buffer and let the Io system
            //      copy the data in.
            //

            } else {

                if (Notify->AllocatedBuffer != NULL) {

                    //
                    //  Protect the copy with a try-except and ignore the buffer
                    //  if we have some error in copying it to the buffer.
                    //

                    try {

                        if (NotifyIrp->AssociatedIrp.SystemBuffer != NULL) {

                            RtlCopyMemory( NotifyIrp->AssociatedIrp.SystemBuffer,
                                           Notify->AllocatedBuffer,
                                           DataLength );

                        } else if (NotifyIrp->MdlAddress != NULL) {

                            RtlCopyMemory( MmGetSystemAddressForMdl( NotifyIrp->MdlAddress ),
                                           Notify->AllocatedBuffer,
                                           DataLength );

                        } else if (!FlagOn( IrpSp->Control, SL_PENDING_RETURNED )) {

                            RtlCopyMemory( NotifyIrp->UserBuffer,
                                           Notify->AllocatedBuffer,
                                           DataLength );

                        } else {

                            NotifyIrp->Flags |= (IRP_BUFFERED_IO | IRP_INPUT_OPERATION | IRP_DEALLOCATE_BUFFER);
                            NotifyIrp->AssociatedIrp.SystemBuffer = Notify->AllocatedBuffer;

                        }

                    } except( EXCEPTION_EXECUTE_HANDLER ) {

                        Status = STATUS_NOTIFY_ENUM_DIR;
                        DataLength = 0;
                    }

                    //
                    //  Return the quota and deallocate the buffer if we didn't pass it
                    //  back via the irp.
                    //

                    PsReturnPoolQuota( Notify->OwningProcess, PagedPool, Notify->ThisBufferLength );

                    if (Notify->AllocatedBuffer != NotifyIrp->AssociatedIrp.SystemBuffer
                        && Notify->AllocatedBuffer != NULL) {

                        ExFreePool( Notify->AllocatedBuffer );
                    }

                    Notify->AllocatedBuffer = NULL;
                    Notify->ThisBufferLength = 0;
                }

                //
                //  Update the data length in the Irp.
                //

                NotifyIrp->IoStatus.Information = DataLength;

                //
                //  Show that there is no buffer in the notify package
                //  anymore.
                //

                Notify->Buffer = NULL;
            }
        }

        //
        //  Make sure the Irp is marked as pending returned.
        //

        IoMarkIrpPending( NotifyIrp );

        //
        //  Now complete the request.
        //

        FsRtlCompleteRequest( NotifyIrp, Status );
    }

    DebugTrace( -1, Dbg, "FsRtlIsNotifyCompleteIrp:  Exit\n", 0 );

    return;
}


//
//  Local support routine
//

BOOLEAN
FsRtlNotifySetCancelRoutine (
    IN PIRP NotifyIrp,
    IN PNOTIFY_CHANGE Notify OPTIONAL
    )

/*++

Routine Description:

    This is a separate routine because it cannot be paged.

Arguments:

    NotifyIrp  -  Set the cancel routine in this Irp.

    Notify - If NULL then we are setting the cancel routine.  If not-NULL then we
        are clearing the cancel routine.  If the cancel routine is not-null then
        we need to decrement the reference count on this Notify structure

Return Value:

    BOOLEAN - Only meaningfull if Notify is specified.  It indicates if this
        routine cleared the cancel routine.  FALSE indicates that the cancel
        routine is processing the Irp.

--*/

{
    BOOLEAN ClearedCancel = FALSE;
    PDRIVER_CANCEL CurrentCancel;

    //
    //  Grab the cancel spinlock and set our cancel routine in the Irp.
    //

    IoAcquireCancelSpinLock( &NotifyIrp->CancelIrql );

    //
    //  If we are completing an Irp then clear the cancel routine and
    //  the information field.
    //

    if (ARGUMENT_PRESENT( Notify )) {

        CurrentCancel = IoSetCancelRoutine( NotifyIrp, NULL );
        NotifyIrp->IoStatus.Information = 0;

        IoReleaseCancelSpinLock( NotifyIrp->CancelIrql );

        //
        //  If the current cancel routine is non-NULL then decrement the reference count
        //  in the Notify.
        //

        if (CurrentCancel != NULL) {

            InterlockedDecrement( &Notify->ReferenceCount );
            ClearedCancel = TRUE;
        }

    //
    //  If the cancel flag is set, we complete the Irp with cancelled
    //  status and exit.
    //

    } else if (NotifyIrp->Cancel) {

            DebugTrace( 0, Dbg, "Irp has been cancelled\n", 0 );

            FsRtlCancelNotify( NULL, NotifyIrp );

    } else {

        //
        //  Set our cancel routine in the Irp.
        //

        IoSetCancelRoutine( NotifyIrp, FsRtlCancelNotify );

        IoReleaseCancelSpinLock( NotifyIrp->CancelIrql );
    }

    return ClearedCancel;
}


//
//  Local support routine
//

BOOLEAN
FsRtlNotifyUpdateBuffer (
    IN PFILE_NOTIFY_INFORMATION NotifyInfo,
    IN ULONG FileAction,
    IN PSTRING ParentName,
    IN PSTRING TargetName,
    IN PSTRING StreamName OPTIONAL,
    IN BOOLEAN UnicodeName,
    IN ULONG SizeOfEntry
    )

/*++

Routine Description:

    This routine is called to fill in a FILE_NOTIFY_INFORMATION structure for a
    notify change event.  The main work is in converting an OEM string to Unicode.

Arguments:

    NotifyInfo  -  Information structure to complete.

    FileAction  -  Action which triggered the notification event.

    ParentName  -  Relative path to the parent of the changed file from the
        directory being watched.  The length for this will be zero if the modified
        file is in the watched directory.

    TargetName  -  This is the name of the modified file.

    StreamName  -  If present there is a stream name to append to the filename.

    UnicodeName  -  Indicates if the above name is Unicode or Oem.

    SizeOfEntry  -  Indicates the number of bytes to be used in the buffer.

Return Value:

    BOOLEAN - TRUE if we were able to update the buffer, FALSE otherwise.

--*/

{
    BOOLEAN CopiedToBuffer;
    ULONG BufferOffset = 0;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlNotifyUpdateBuffer:  Entered\n", 0 );

    //
    //  Protect the entire call with a try-except.  If we had an error
    //  we will assume that we have a bad buffer and we won't return
    //  the data in the buffer.
    //

    try {

        //
        //  Update the common fields in the notify information.
        //

        NotifyInfo->NextEntryOffset = 0;
        NotifyInfo->Action = FileAction;

        NotifyInfo->FileNameLength = SizeOfEntry - FIELD_OFFSET( FILE_NOTIFY_INFORMATION, FileName );

        //
        //  If we have a unicode name, then copy the data directly into the output buffer.
        //

        if (UnicodeName) {

            if (ParentName->Length != 0) {

                RtlCopyMemory( NotifyInfo->FileName,
                               ParentName->Buffer,
                               ParentName->Length );

                *(Add2Ptr( NotifyInfo->FileName, ParentName->Length, PWCHAR )) = L'\\';
                BufferOffset = ParentName->Length + sizeof( WCHAR );
            }

            RtlCopyMemory( Add2Ptr( NotifyInfo->FileName,
                                    BufferOffset,
                                    PVOID ),
                           TargetName->Buffer,
                           TargetName->Length );

            if (ARGUMENT_PRESENT( StreamName )) {

                BufferOffset += TargetName->Length;

                *(Add2Ptr( NotifyInfo->FileName, BufferOffset, PWCHAR )) = L':';

                RtlCopyMemory( Add2Ptr( NotifyInfo->FileName,
                                        BufferOffset + sizeof( WCHAR ),
                                        PVOID ),
                               StreamName->Buffer,
                               StreamName->Length );
            }

        //
        //  For a non-unicode name, use the conversion routines.
        //

        } else {

            ULONG BufferLength;

            if (ParentName->Length != 0) {

                RtlOemToUnicodeN( NotifyInfo->FileName,
                                  NotifyInfo->FileNameLength,
                                  &BufferLength,
                                  ParentName->Buffer,
                                  ParentName->Length );

                *(Add2Ptr( NotifyInfo->FileName, BufferLength, PWCHAR )) = L'\\';

                BufferOffset = BufferLength + sizeof( WCHAR );
            }

            //
            //  For view indices, we do not have a parent name.
            //

            if (ParentName->Length == 0) {

                ASSERT(ARGUMENT_PRESENT( StreamName ));

                RtlCopyMemory( Add2Ptr( NotifyInfo->FileName,
                                           BufferOffset,
                                           PCHAR ),
                               StreamName->Buffer,
                               StreamName->Length );

            } else {

                RtlOemToUnicodeN( Add2Ptr( NotifyInfo->FileName,
                                           BufferOffset,
                                           PWCHAR ),
                                  NotifyInfo->FileNameLength,
                                  &BufferLength,
                                  TargetName->Buffer,
                                  TargetName->Length );

                if (ARGUMENT_PRESENT( StreamName )) {

                    BufferOffset += BufferLength;

                    *(Add2Ptr( NotifyInfo->FileName, BufferOffset, PWCHAR )) = L':';

                    RtlOemToUnicodeN( Add2Ptr( NotifyInfo->FileName,
                                               BufferOffset + sizeof( WCHAR ),
                                               PWCHAR ),
                                      NotifyInfo->FileNameLength,
                                      &BufferLength,
                                      StreamName->Buffer,
                                      StreamName->Length );
                }
            }
        }

        CopiedToBuffer = TRUE;

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        CopiedToBuffer = FALSE;
    }

    DebugTrace( -1, Dbg, "FsRtlNotifyUpdateBuffer:  Exit\n", 0 );

    return CopiedToBuffer;
}


//
//  Local support routine
//

VOID
FsRtlNotifyCompleteIrpList (
    IN OUT PNOTIFY_CHANGE Notify,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine walks through the Irps for a particular notify structure
    and completes the Irps with the indicated status.  If the status is
    STATUS_SUCCESS then we are completing an Irp because of a notification event.
    In that case we look at the notify structure to decide if we can return the
    data to the user.

Arguments:

    Notify  -  This is the notify change structure.

    Status  -  Indicates the status used to complete the request.  If this status
        is STATUS_SUCCESS then we only want to complete one Irp.  Otherwise we
        want complete all the Irps in the list.

Return Value:

    None.

--*/

{
    PIRP Irp;
    ULONG DataLength;

    DebugTrace( +1, Dbg, "FsRtlNotifyCompleteIrpList:  Entered\n", 0 );

    DataLength = Notify->DataLength;

    //
    //  Clear the fields to indicate that there is no more data to return.
    //

    ClearFlag( Notify->Flags, NOTIFY_IMMEDIATE_NOTIFY );
    Notify->DataLength = 0;
    Notify->LastEntry = 0;

    //
    //  Walk through all the Irps in the list.  We are never called unless
    //  there is at least one irp.
    //

    do {

        Irp = CONTAINING_RECORD( Notify->NotifyIrps.Flink, IRP, Tail.Overlay.ListEntry );

        RemoveHeadList( &Notify->NotifyIrps );

        //
        //  Call our completion routine to complete the request.
        //

        FsRtlNotifyCompleteIrp( Irp,
                                Notify,
                                DataLength,
                                Status,
                                TRUE );

        //
        //  If we were only to complete one Irp then break now.
        //

        if (Status == STATUS_SUCCESS) {

            break;
        }

    } while (!IsListEmpty( &Notify->NotifyIrps ));

    DebugTrace( -1, Dbg, "FsRtlNotifyCompleteIrpList:  Exit\n", 0 );

    return;
}


//
//  Local support routine
//

VOID
FsRtlCancelNotify (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP ThisIrp
    )

/*++

Routine Description:

    This routine is for an Irp which is being cancelled.  We Null the cancel
    routine and then walk through the Irps for this notify structure and
    complete all cancelled Irps.  It is possible there is pending notify
    stored in the buffer for this Irp.  In this case we want to copy the
    data to a system buffer if possible.

Arguments:

    DeviceObject - Ignored.

    ThisIrp  -  This is the Irp to cancel.

Return Value:

    None.

--*/

{
    PSECURITY_SUBJECT_CONTEXT SubjectContext = NULL;

    PNOTIFY_CHANGE Notify;
    PNOTIFY_SYNC NotifySync;
    LONG ExceptionCode;

    UNREFERENCED_PARAMETER( DeviceObject );

    DebugTrace( +1, Dbg, "FsRtlCancelNotify:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Irp   -> %08lx\n", Irp );

    //
    //  Capture the notify structure.
    //

    Notify = (PNOTIFY_CHANGE) ThisIrp->IoStatus.Information;

    //
    //  Void the cancel routine and release the cancel spinlock.
    //

    IoSetCancelRoutine( ThisIrp, NULL );
    ThisIrp->IoStatus.Information = 0;
    IoReleaseCancelSpinLock( ThisIrp->CancelIrql );

    FsRtlEnterFileSystem();

    //
    //  Grab the mutex for this structure.
    //

    NotifySync = Notify->NotifySync;
    AcquireNotifySync( NotifySync );

    //
    //  Use a try finally to faciltate cleanup.
    //

    try {

        //
        //  Remove the Irp from the queue.
        //

        RemoveEntryList( &ThisIrp->Tail.Overlay.ListEntry );

        IoMarkIrpPending( ThisIrp );

        //
        //  We now have the Irp.  Check to see if there is data stored
        //  in the buffer for this Irp.
        //

        if (Notify->Buffer != NULL
            && Notify->AllocatedBuffer == NULL

            && ((ThisIrp->MdlAddress != NULL
                 && MmGetSystemAddressForMdl( ThisIrp->MdlAddress ) == Notify->Buffer)

                || (Notify->Buffer == ThisIrp->AssociatedIrp.SystemBuffer))) {

            PIRP NextIrp;
            PVOID NewBuffer;
            ULONG NewBufferLength;
            PIO_STACK_LOCATION  IrpSp;

            //
            //  Initialize the above values.
            //

            NewBuffer = NULL;
            NewBufferLength = 0;

            //
            //  Remember the next Irp on the list.  Find the length of any
            //  buffer it might have.  Also keep a pointer to the buffer
            //  if present.
            //

            if (!IsListEmpty( &Notify->NotifyIrps )) {

                NextIrp = CONTAINING_RECORD( Notify->NotifyIrps.Flink,
                                             IRP,
                                             Tail.Overlay.ListEntry );

                IrpSp = IoGetCurrentIrpStackLocation( NextIrp );

                //
                //  If the buffer here is large enough to hold the data we
                //  can use that buffer.
                //

                if (IrpSp->Parameters.NotifyDirectory.Length >= Notify->DataLength) {

                    //
                    //  If there is a system buffer or Mdl then get a new
                    //  buffer there.
                    //

                    if (NextIrp->AssociatedIrp.SystemBuffer != NULL) {

                        NewBuffer = NextIrp->AssociatedIrp.SystemBuffer;

                    } else if (NextIrp->MdlAddress != NULL) {

                        NewBuffer = MmGetSystemAddressForMdl( NextIrp->MdlAddress );
                    }

                    NewBufferLength = IrpSp->Parameters.NotifyDirectory.Length;

                    if (NewBufferLength > Notify->BufferLength) {

                        NewBufferLength = Notify->BufferLength;
                    }
                }

            //
            //  Otherwise check if the user's original buffer is larger than
            //  the current buffer.
            //

            } else if (Notify->BufferLength >= Notify->DataLength) {

                NewBufferLength = Notify->BufferLength;
            }

            //
            //  If we have a new buffer length then we either have a new
            //  buffer or need to allocate one.  We will do this under
            //  the protection of a try-except in order to continue in the
            //  event of a failure.
            //

            if (NewBufferLength != 0) {

                BOOLEAN ChargedQuota;

                try {

                    ChargedQuota = FALSE;

                    if (NewBuffer == NULL) {

                        PsChargePoolQuota( Notify->OwningProcess,
                                           PagedPool,
                                           NewBufferLength );

                        ChargedQuota = TRUE;

                        //
                        //  If we didn't get an error then attempt to
                        //  allocate the pool.  If there is an error
                        //  don't forget to release the quota.
                        //

                        NewBuffer = FsRtlpAllocatePool( PagedPool,
                                                        NewBufferLength );

                        Notify->AllocatedBuffer = NewBuffer;
                    }

                    //
                    //  Now copy the data over to the new buffer.
                    //

                    RtlCopyMemory( NewBuffer,
                                   Notify->Buffer,
                                   Notify->DataLength );

                    //
                    //  It is possible that the buffer size changed.
                    //

                    Notify->ThisBufferLength = NewBufferLength;
                    Notify->Buffer = NewBuffer;

                } except( FsRtlIsNtstatusExpected( ExceptionCode = GetExceptionCode()) ?
                          EXCEPTION_EXECUTE_HANDLER :
                          EXCEPTION_CONTINUE_SEARCH ) {

                    ASSERT( (ExceptionCode == STATUS_INSUFFICIENT_RESOURCES) ||
                            (ExceptionCode == STATUS_QUOTA_EXCEEDED) );
                    
                    //
                    //  Return quota if we allocated the buffer.
                    //

                    if (ChargedQuota) {

                        PsReturnPoolQuota( Notify->OwningProcess,
                                           PagedPool,
                                           NewBufferLength );
                    }

                    //
                    //  Forget any current buffer and resort to immediate
                    //  notify.
                    //

                    SetFlag( Notify->Flags, NOTIFY_IMMEDIATE_NOTIFY );
                }

            //
            //  Otherwise set the immediate notify flag.
            //

            } else {

                SetFlag( Notify->Flags, NOTIFY_IMMEDIATE_NOTIFY );
            }

            //
            //  If the immediate notify flag is set then clear the other
            //  values in the notify structures.
            //

            if (FlagOn( Notify->Flags, NOTIFY_IMMEDIATE_NOTIFY )) {

                //
                //  Forget any current buffer and resort to immediate
                //  notify.
                //

                Notify->AllocatedBuffer = Notify->Buffer = NULL;

                Notify->ThisBufferLength =
                Notify->DataLength = Notify->LastEntry = 0;
            }
        }

        //
        //  Complete the Irp with status cancelled.
        //

        FsRtlCompleteRequest( ThisIrp, STATUS_CANCELLED );

        //
        //  Decrement the count of Irps that might go through the cancel path.
        //

        InterlockedDecrement( &Notify->ReferenceCount );

        if (Notify->ReferenceCount == 0) {

            if (Notify->AllocatedBuffer != NULL) {

                PsReturnPoolQuota( Notify->OwningProcess,
                                   PagedPool,
                                   Notify->ThisBufferLength );

                ExFreePool( Notify->AllocatedBuffer );
            }

            if (Notify->FullDirectoryName != NULL) {

                SubjectContext = Notify->SubjectContext;
            }

            ExFreePool( Notify );
            Notify = NULL;
        }

    } finally {

        //
        //  No matter how we exit, we release the mutex.
        //

        ReleaseNotifySync( NotifySync );

        if (SubjectContext != NULL) {

            SeReleaseSubjectContext( SubjectContext );
            ExFreePool( SubjectContext );
        }

        FsRtlExitFileSystem();

        DebugTrace( -1, Dbg, "FsRtlCancelNotify:  Exit\n", 0 );
    }

    return;
}


//
//  Local support routine
//

VOID
FsRtlCheckNotifyForDelete (
    IN PLIST_ENTRY NotifyListHead,
    IN PVOID StreamID
    )

/*++

Routine Description:

    This routine is called when a stream is being marked for delete.  We will
    walk through the notify structures looking for an Irp for the same stream.
    We will complete these Irps with STATUS_DELETE_PENDING.

Arguments:

    NotifyListHead  -  This is the start of the notify list.

    StreamID  -  This is the Context ID used to identify the stream.

Return Value:

    None.

--*/

{
    PLIST_ENTRY Link;

    PNOTIFY_CHANGE ThisNotify;

    PAGED_CODE();

    //
    //  Walk through all the entries on the list looking for a match.
    //

    for (Link = NotifyListHead->Flink;
         Link != NotifyListHead;
         Link = Link->Flink) {

        //
        //  Obtain the notify structure from the link.
        //

        ThisNotify = CONTAINING_RECORD( Link, NOTIFY_CHANGE, NotifyList );

        //
        //  If the context field matches, then complete any waiting Irps.
        //

        if (ThisNotify->StreamID == StreamID) {

            //
            //  Start by marking the notify structure as file deleted.
            //

            SetFlag( ThisNotify->Flags, NOTIFY_STREAM_IS_DELETED );

            //
            //  Now complete all of the Irps on this list.
            //

            if (!IsListEmpty( &ThisNotify->NotifyIrps )) {

                FsRtlNotifyCompleteIrpList( ThisNotify, STATUS_DELETE_PENDING );
            }
        }
    }

    return;
}
