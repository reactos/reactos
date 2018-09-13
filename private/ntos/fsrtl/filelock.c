/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FileLock.c

Abstract:

    The file lock package provides a set of routines that allow the
    caller to handle byte range file lock requests.  A variable of
    type FILE_LOCK is needed for every file with byte range locking.
    The package provides routines to set and clear locks, and to
    test for read or write access to a file with byte range locks.

    The main idea of the package is to have the file system initialize
    a FILE_LOCK variable for every data file as its opened, and then
    to simply call a file lock processing routine to handle all IRP's
    with a major function code of LOCK_CONTROL.  The package is responsible
    for keeping track of locks and for completing the LOCK_CONTROL IRPS.
    When processing a read or write request the file system can then call
    two query routines to check for access.

    Most of the code for processing IRPS and checking for access use
    paged pool and can encounter a page fault, therefore the check routines
    cannot be called at DPC level.  To help servers that do call the file
    system to do read/write operations at DPC level there is a additional
    routine that simply checks for the existence of a lock on a file and
    can be run at DPC level.

    Concurrent access to the FILE_LOCK variable must be controlled by the
    caller.

    The functions provided in this package are as follows:

      o  FsRtlInitializeFileLock - Initialize a new FILE_LOCK structure.

      o  FsRtlUninitializeFileLock - Uninitialize an existing FILE_LOCK
         structure.

      o  FsRtlProcessFileLock - Process an IRP whose major function code
         is LOCK_CONTROL.

      o  FsRtlCheckLockForReadAccess - Check for read access to a range
         of bytes in a file given an IRP.

      o  FsRtlCheckLockForWriteAccess - Check for write access to a range
         of bytes in a file given an IRP.

      o  FsRtlAreThereCurrentFileLocks - Check if there are any locks
         currently assigned to a file.

      o  FsRtlGetNextFileLock - This procedure enumerates the current locks
         of a file lock variable.

      o  FsRtlFastCheckLockForRead - Check for read access to a range of
         bytes in a file given separate parameters.

      o  FsRtlFastCheckLockForWrite - Check for write access to a range of
         bytes in a file given separate parameters.

      o  FsRtlFastLock - A fast non-Irp based way to get a lock

      o  FsRtlFastUnlockSingle - A fast non-Irp based way to release a single
         lock

      o  FsRtlFastUnlockAll - A fast non-Irp based way to release all locks
         held by a file object.

      o  FsRtlFastUnlockAllByKey - A fast non-Irp based way to release all
         locks held by a file object that match a key.


Authors:

    Gary Kimura     [GaryKi]    24-Apr-1990
    Dan Lovinger    [DanLo]     22-Sep-1995

Revision History:

--*/

#include "FsRtlP.h"

//
//  Local constants
//

//
//  Local debug trace level
//

#define Dbg                 (0x20000000)

//
//  YA definition of INLINE
//

#ifndef INLINE
#define INLINE __inline
#endif

#define TAG_EXCLUSIVE_LOCK  'xeLF'
#define TAG_FILE_LOCK       'lfLF'
#define TAG_LOCK_INFO       'ilLF'
#define TAG_LOCKTREE_NODE   'nlLF'
#define TAG_SHARED_LOCK     'hsLF'
#define TAG_WAITING_LOCK    'lwLF'

//
//  Globals
//

//
//  This mutex synchronizes threads competing to initialize file lock structures.
//

FAST_MUTEX FsRtlCreateLockInfo;

//
//  Lookaside lists
//
//  Here is a good place to note why this is still nonpaged.  We need to be able
//  to cancel lock IRPs at DPC, and the ripple effects of this (esp. granting waiting
//  locks and synchronizing the waiting list) implies some unfortunate realities.
//
//  This should be reinvestigated post NT 5.0.
//

NPAGED_LOOKASIDE_LIST FsRtlSharedLockLookasideList;
NPAGED_LOOKASIDE_LIST FsRtlExclusiveLockLookasideList;
NPAGED_LOOKASIDE_LIST FsRtlWaitingLockLookasideList;
NPAGED_LOOKASIDE_LIST FsRtlLockTreeNodeLookasideList;
NPAGED_LOOKASIDE_LIST FsRtlLockInfoLookasideList;

PAGED_LOOKASIDE_LIST FsRtlFileLockLookasideList;


//
//  Local structures
//

/*++

    Some of the decisions made regarding the internal datastructres may not be clear,
    so I should discuss the evolution of this design.

    The original file lock implementation was a single linked list, extended in the MP
    case to a set of linked lists which each held locks in page-aligned segments of the
    file. If locks spilled over these page-aligned segments the code fell back to the
    UP single linked list. There are clearly peformance implications with substantial
    usage of file locks, since these are mandatory locks.

    This implementation goes for O(lgn) search performance by using splay trees. In order to
    apply simple trees to this problem no node of the tree can overlap, so since shared
    locks can in fact overlap something must be done. The solution used here is to have
    a meta-structure contain all locks which do overlap and have the tree operations
    split and merge these nodes of (potentially) multiple locks. This is the LOCKTREE_NODE.
    It should be noted that the worst case add/delete lock times are still linear.

    Exclusive locks pose a problem because of an asymmetry in the semantics of applying
    locks to a file. If a process applies a shared lock to a section of a file, no application
    of an exclusive lock to bytes in that section can succeed. However, if a process
    applies an exclusive lock, that same process can get a shared lock as well. This
    behavior conflicts with the mergeable node since by applying locks in a given order
    we can get a node to have many shared locks and "rogue" exclusive locks which are
    hidden except to a linear search, which is what we're designing out. So exclusive locks
    must be seperated from the shared locks. This is the reason we have two lock trees.

    Since we have two lock trees, the average case search is now O(lgm + lgn) for m exlcusive
    and n shared. Also, since no exclusive locks can ever overlap each other it is now
    unreasonable to have them use LOCKTREE_NODES - this would impose a memory penalty on code
    which was weighted toward exclusive locks. This means that the exclusive locks should
    be wired into the splay tree directly. So we need an RTL_SPLAY_LINKS, but this is 64 bits
    bigger than the SINGLE_LIST_ENTRY which shared locks need (to be threaded off of a
    LOCKTREE_NODE), which dictates seperate shared and exclusive lock structures to avoid
    penalizing code which was weighted toward shared locks by having that wasted 64 bits per
    lock. Hence EX_LOCK and SH_LOCK (they actually occupy different pool block sizes).

    Zero length locks are a bizzare creation, and there is some errata relating to them. It
    used to be the case that zero length locks would be granted without exception. This is
    flat out bogus, and has been changed (NT 4.0). They are now subject to failure if they
    occupy a point interior to a lock of a type that can cause an access failure. A particular
    case that was previously allowed was a zero length exclusive lock interior to another
    exclusive lock.

    Zero length locks cannot conflict with zero length locks. This is the subject of some
    special code throughout the module. Note especially that zero length exclusive locks can
    "overlap". Zero length locks also cannot conflict at the starting byte and ending byte of a
    range - they are points on the line.

--*/

typedef struct _LOCKTREE_NODE {

    //
    //  List of locks under this node
    //

    SINGLE_LIST_ENTRY Locks;

    //
    //  Flag whether this node is holey as a result of a failed allocation
    //  during a node split.  During deletion of shared locks, we may
    //  discover that the locks in the node no longer have total overlap
    //  but cannot allocate resources to create the new nodes in the tree.
    //
    //  Any insert into the region occupied by a holey node will finish by
    //  trying to split a holey node up.  Any split or access check in a
    //  holey node must completely traverse the locks at the node.
    //

    BOOLEAN HoleyNode;

    //
    //  Maximum byte offset affected by locks in this node.
    //  Note: minimum offset is the starting offset of the
    //  first lock at this node.
    //

    ULONGLONG Extent;

    //
    //  Splay tree links to parent, lock groups strictly less than
    //  and lock groups strictly greater than locks in this node.
    //

    RTL_SPLAY_LINKS Links;

    //
    //  Last lock in the list (useful for node collapse under insert)
    //

    SINGLE_LIST_ENTRY Tail;

} LOCKTREE_NODE, *PLOCKTREE_NODE;

//
//  Define the threading wrappers for lock information
//

//
//  Each shared lock record corresponds to a current granted lock and is
//  maintained in a queue off of a LOCKTREE_NODE's Locks list.  The list
//  of current locks is ordered according to the starting byte of the lock.
//

typedef struct _SH_LOCK {

    //
    //  The link structures for the list of shared locks.
    //

    SINGLE_LIST_ENTRY   Link;

    //
    //  The actual locked range
    //

    FILE_LOCK_INFO LockInfo;

} SH_LOCK, *PSH_LOCK;

//
//  Each exclusive lock record corresponds to a current granted lock and is
//  threaded into the exclusive lock tree.
//

typedef struct _EX_LOCK {

    //
    //  The link structures for the list of current locks.
    //

    RTL_SPLAY_LINKS     Links;

    //
    //  The actual locked range
    //

    FILE_LOCK_INFO LockInfo;

} EX_LOCK, *PEX_LOCK;

//
//  Each Waiting lock record corresponds to a IRP that is waiting for a
//  lock to be granted and is maintained in a queue off of the FILE_LOCK's
//  WaitingLockQueue list.
//

typedef struct _WAITING_LOCK {

    //
    //  The link structures for the list of waiting locks
    //

    SINGLE_LIST_ENTRY   Link;

    //
    //  The context field to use when completing the irp via the alternate
    //  routine
    //

    PVOID Context;

    //
    //  A pointer to the IRP that is waiting for a lock
    //

    PIRP Irp;

} WAITING_LOCK, *PWAITING_LOCK;


//
//  Each lock or waiting onto some lock queue.
//

typedef struct _LOCK_QUEUE {

    //
    // Sync to guard queue access.
    //

    KSPIN_LOCK  QueueSpinLock;

    //
    //  The items contain locktrees of the current granted
    //  locks and a list of the waiting locks
    //

    PRTL_SPLAY_LINKS SharedLockTree;
    PRTL_SPLAY_LINKS ExclusiveLockTree;
    SINGLE_LIST_ENTRY WaitingLocks;
    SINGLE_LIST_ENTRY WaitingLocksTail;

} LOCK_QUEUE, *PLOCK_QUEUE;


//
//  Any file_lock which has had a lock applied gets a non-paged pool
//  structure which tracks the current locks applied to the file
//

typedef struct _LOCK_INFO {

    //
    //  LowestLockOffset retains the offset of the lowest existing
    //  lock.  This facilitates a quick check to see if a read or
    //  write can proceed without locking the lock database.  This is
    //  helpful for applications that use mirrored locks -- all locks
    //  are higher than file data.
    //
    //  If the lowest lock has an offset > 0xffffffff, LowestLockOffset
    //  is set to 0xffffffff.
    //

    ULONG LowestLockOffset;

    //
    //  The optional procedure to call to complete a request
    //

    PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine;

    //
    //  The optional procedure to call when unlocking a byte range
    //

    PUNLOCK_ROUTINE UnlockRoutine;

    //
    // The locked ranges
    //

    LOCK_QUEUE  LockQueue;

} LOCK_INFO, *PLOCK_INFO;

//
//  Local Macros
//

//
//  The following macros sort out the allocation of internal structures.
//

INLINE
PSH_LOCK
FsRtlAllocateSharedLock (
    VOID
    )
{
    return (PSH_LOCK) ExAllocateFromNPagedLookasideList( &FsRtlSharedLockLookasideList );
}

INLINE
PEX_LOCK
FsRtlAllocateExclusiveLock (
    VOID
    )
{
    return (PEX_LOCK) ExAllocateFromNPagedLookasideList( &FsRtlExclusiveLockLookasideList );
}

INLINE
PWAITING_LOCK
FsRtlAllocateWaitingLock (
    VOID
    )
{
    return (PWAITING_LOCK) ExAllocateFromNPagedLookasideList( &FsRtlWaitingLockLookasideList );
}

INLINE
PLOCKTREE_NODE
FsRtlAllocateLockTreeNode (
    VOID
    )
{
    return (PLOCKTREE_NODE) ExAllocateFromNPagedLookasideList( &FsRtlLockTreeNodeLookasideList );
}

INLINE
PLOCK_INFO
FsRtlAllocateLockInfo (
    VOID
    )
{
    return (PLOCK_INFO) ExAllocateFromNPagedLookasideList( &FsRtlLockInfoLookasideList );
}


INLINE
VOID
FsRtlFreeSharedLock (
    IN PSH_LOCK C
    )
{
    ExFreeToNPagedLookasideList( &FsRtlSharedLockLookasideList, (PVOID)C );
}

INLINE
VOID
FsRtlFreeExclusiveLock (
    IN PEX_LOCK C
    )
{
    ExFreeToNPagedLookasideList( &FsRtlExclusiveLockLookasideList, (PVOID)C );
}

INLINE
VOID
FsRtlFreeWaitingLock (
    IN PWAITING_LOCK C
    )
{
    ExFreeToNPagedLookasideList( &FsRtlWaitingLockLookasideList, (PVOID)C );
}

INLINE
VOID
FsRtlFreeLockTreeNode (
    IN PLOCKTREE_NODE C
    )
{
    ExFreeToNPagedLookasideList( &FsRtlLockTreeNodeLookasideList, (PVOID)C );
}

INLINE
VOID
FsRtlFreeLockInfo (
    IN PLOCK_INFO C
    )
{
    ExFreeToNPagedLookasideList( &FsRtlLockInfoLookasideList, (PVOID)C );
}


#define FsRtlAcquireLockQueue(a,b)                  \
        ExAcquireSpinLock(&(a)->QueueSpinLock, b);

#define FsRtlReacquireLockQueue(a,b,c)              \
        ExAcquireSpinLock(&(b)->QueueSpinLock, c);

#define FsRtlReleaseLockQueue(a,b)                  \
        ExReleaseSpinLock(&(a)->QueueSpinLock, b);


//
//  Generic way to complete a lock IRP.  We like to treat this as an overloaded
//  function so it can be used with LOCK_INFO and FILE_LOCK structures, as
//  appropriate using paged/nonpaged pool to discover the completion routine.
//

#define FsRtlCompleteLockIrp( A, B, C, D, E, F )                \
        FsRtlCompleteLockIrpReal( (A)->CompleteLockIrpRoutine,  \
                                  B,                            \
                                  C,                            \
                                  D,                            \
                                  E,                            \
                                  F )

INLINE
VOID
FsRtlCompleteLockIrpReal (
    IN PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine,
    IN PVOID Context,
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN PNTSTATUS NewStatus,
    IN PFILE_OBJECT FileObject
    )
{
    //
    //  This fools the compiler into generating the Status only once
    //  if it is calculated from an expression.
    //

    NTSTATUS LocalStatus = Status;

    if (CompleteLockIrpRoutine != NULL) {

        if (FileObject != NULL) {

            FileObject->LastLock = NULL;
        }

        Irp->IoStatus.Status = LocalStatus;
        *NewStatus = CompleteLockIrpRoutine( Context, Irp );

    } else {

        FsRtlCompleteRequest( Irp, LocalStatus );
        *NewStatus = LocalStatus;
    }
}

//
//  Define USERTEST to get a version which compiles into a usermode test rig
//

#ifdef USERTEST
#include <stdio.h>
#include <stdlib.h>
#undef FsRtlAllocateSharedLock
#undef FsRtlAllocateExclusiveLock
#undef FsRtlAllocateLockTreeNode
#undef FsRtlAllocateWaitingLock
#undef FsRtlFreeSharedLock
#undef FsRtlFreeExclusiveLock
#undef FsRtlFreeLockTreeNode
#undef FsRtlFreeWaitingLock
#undef FsRtlAcquireLockQueue
#undef FsRtlReacquireLockQueue
#undef FsRtlReleaseLockQueue
#undef FsRtlCompleteLockIrp
#undef IoCompleteRequest

#define FsRtlAllocateSharedLock( C )        (PSH_LOCK)malloc(sizeof(SH_LOCK))
#define FsRtlAllocateExclusiveLock( C )     (PEX_LOCK)malloc(sizeof(EX_LOCK))
#define FsRtlAllocateLockTreeNode( C )      (PLOCKTREE_NODE)malloc(sizeof(LOCKTREE_NODE))
#define FsRtlAllocateWaitingLock( C )       (PWAITING_LOCK)malloc(sizeof(WAITING_LOCK))
#define FsRtlAllocateLockInfo( C )          (PLOCK_INFO)malloc(sizeof(LOCK_INFO))
#define FsRtlFreeSharedLock( C )            free(C)
#define FsRtlFreeExclusiveLock( C )         free(C)
#define FsRtlFreeLockTreeNode( C )          free(C)
#define FsRtlFreeWaitingLock( C )           free(C)
#define FsRtlFreeLockInfo( C )              free(C)
#define FsRtlAcquireLockQueue(a,b)          (*(b) = '\0')
#define FsRtlReacquireLockQueue(a,b,c)      (*(c) = '\0')
#define FsRtlReleaseLockQueue(a,b)
#define FsRtlCompleteLockIrp(_FileLock, _Context, _Irp, _Status, _NewStatus, _FileObject)   \
    {                                                                                       \
        DbgBreakPoint();                                                                    \
        *_NewStatus = STATUS_SUCCESS;                                                       \
    }

#define ExReleaseFastMutex(M)
#define ExAcquireFastMutex(M)
#define KeInitializeSpinLock(L)
#define KfRaiseIrql(L)                      ('\0')
#define KfLowerIrql(I)
#define IoAcquireCancelSpinLock(I)
#define IoReleaseCancelSpinLock(I)
#define IoCompleteRequest(I, S)
#endif

//
//  The following routines are private to this module
//

VOID
FsRtlSplitLocks (
    IN PLOCKTREE_NODE ParentNode,
    IN PSINGLE_LIST_ENTRY *pStartLink,
    IN PLARGE_INTEGER LastShadowedByte,
    IN PLARGE_INTEGER GlueOffset
    );

PRTL_SPLAY_LINKS
FsRtlFindFirstOverlappingSharedNode (
    IN PRTL_SPLAY_LINKS        Tree,
    IN PLARGE_INTEGER          StartingByte,
    IN PLARGE_INTEGER          EndingByte,
    IN OUT PRTL_SPLAY_LINKS    *LastEdgeNode,
    IN OUT PBOOLEAN            GreaterThan
    );

PRTL_SPLAY_LINKS
FsRtlFindFirstOverlappingExclusiveNode (
    IN PRTL_SPLAY_LINKS        Tree,
    IN PLARGE_INTEGER          StartingByte,
    IN PLARGE_INTEGER          EndingByte,
    IN OUT PRTL_SPLAY_LINKS    *LastEdgeNode,
    IN OUT PBOOLEAN            GreaterThan
    );

PSH_LOCK
FsRtlFindFirstOverlapInNode (
    IN PLOCKTREE_NODE Node,
    IN PLARGE_INTEGER StartingByte,
    IN PLARGE_INTEGER EndingByte
    );

BOOLEAN
FsRtlPrivateInsertLock (
    IN PLOCK_INFO LockInfo,
    IN PFILE_OBJECT FileObject,
    IN PFILE_LOCK_INFO FileLockInfo
    );

BOOLEAN
FsRtlPrivateInsertSharedLock (
    IN PLOCK_QUEUE LockQueue,
    IN PSH_LOCK NewLock
    );

VOID
FsRtlPrivateInsertExclusiveLock (
    IN PLOCK_QUEUE LockQueue,
    IN PEX_LOCK NewLock
    );

VOID
FsRtlPrivateCheckWaitingLocks (
    IN PLOCK_INFO   LockInfo,
    IN PLOCK_QUEUE  LockQueue,
    IN KIRQL        OldIrql
    );

VOID
FsRtlPrivateCancelFileLockIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
FsRtlPrivateCheckForExclusiveLockAccess (
    IN PLOCK_QUEUE LockInfo,
    IN PFILE_LOCK_INFO FileLockInfo
    );

BOOLEAN
FsRtlPrivateCheckForSharedLockAccess (
    IN PLOCK_QUEUE LockInfo,
    IN PFILE_LOCK_INFO FileLockInfo
    );

NTSTATUS
FsRtlPrivateFastUnlockAll (
    IN PFILE_LOCK FileLock,
    IN PFILE_OBJECT FileObject,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN BOOLEAN MatchKey,
    IN PVOID Context OPTIONAL
    );

BOOLEAN
FsRtlPrivateInitializeFileLock (
    IN PFILE_LOCK   FileLock,
    IN BOOLEAN ViaFastCall
    );

VOID
FsRtlPrivateRemoveLock (
    IN PLOCK_INFO LockInfo,
    IN PFILE_LOCK_INFO,
    IN BOOLEAN CheckForWaiters
    );

BOOLEAN
FsRtlCheckNoSharedConflict (
   IN PLOCK_QUEUE LockQueue,
   IN PLARGE_INTEGER Starting,
   IN PLARGE_INTEGER Ending
   );

BOOLEAN
FsRtlCheckNoExclusiveConflict (
    IN PLOCK_QUEUE LockQueue,
    IN PLARGE_INTEGER Starting,
    IN PLARGE_INTEGER Ending,
    IN ULONG Key,
    IN PFILE_OBJECT FileObject,
    IN PVOID ProcessId
    );

VOID
FsRtlPrivateResetLowestLockOffset (
    PLOCK_INFO LockInfo
    );

NTSTATUS
FsRtlFastUnlockSingleShared (
    IN PLOCK_INFO LockInfo,
    IN PFILE_OBJECT FileObject,
    IN LARGE_INTEGER UNALIGNED *FileOffset,
    IN PLARGE_INTEGER Length,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN PVOID Context OPTIONAL,
    IN BOOLEAN IgnoreUnlockRoutine,
    IN BOOLEAN CheckForWaiters
    );

NTSTATUS
FsRtlFastUnlockSingleExclusive (
    IN PLOCK_INFO LockInfo,
    IN PFILE_OBJECT FileObject,
    IN LARGE_INTEGER UNALIGNED *FileOffset,
    IN PLARGE_INTEGER Length,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN PVOID Context OPTIONAL,
    IN BOOLEAN IgnoreUnlockRoutine,
    IN BOOLEAN CheckForWaiters
    );


VOID
FsRtlInitializeFileLocks (
    VOID
    )
/*++

Routine Description:

    Initializes the global portion of the filelock package.

Arguments:

    None

Return Value:

    None.

--*/
{
#ifndef USERTEST

    //
    //  Build the lookaside lists for our internal structures.
    //

    ExInitializeNPagedLookasideList( &FsRtlSharedLockLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(SH_LOCK),
                                     TAG_SHARED_LOCK,
                                     16 );

    ExInitializeNPagedLookasideList( &FsRtlExclusiveLockLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(EX_LOCK),
                                     TAG_EXCLUSIVE_LOCK,
                                     16 );

    ExInitializeNPagedLookasideList( &FsRtlWaitingLockLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(WAITING_LOCK),
                                     TAG_WAITING_LOCK,
                                     16 );

    ExInitializeNPagedLookasideList( &FsRtlLockTreeNodeLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(LOCKTREE_NODE),
                                     TAG_LOCKTREE_NODE,
                                     16 );

    ExInitializeNPagedLookasideList( &FsRtlLockInfoLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(LOCK_INFO),
                                     TAG_LOCK_INFO,
                                     8 );

    ExInitializePagedLookasideList( &FsRtlFileLockLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(FILE_LOCK),
                                    TAG_FILE_LOCK,
                                    8 );

    //
    //  Initialize the LockInfo creation mutex
    //

    ExInitializeFastMutex(&FsRtlCreateLockInfo);


#endif
}


VOID
FsRtlInitializeFileLock (
    IN PFILE_LOCK FileLock,
    IN PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine OPTIONAL,
    IN PUNLOCK_ROUTINE UnlockRoutine OPTIONAL
    )

/*++

Routine Description:

    This routine initializes a new FILE_LOCK structure.  The caller must
    supply the memory for the structure.  This call must precede all other
    calls that utilize the FILE_LOCK variable.

Arguments:

    FileLock - Supplies a pointer to the FILE_LOCK structure to
        initialize.

    CompleteLockIrpRoutine - Optionally supplies an alternate routine to
        call for completing IRPs.  FsRtlProcessFileLock by default will
        call IoCompleteRequest to finish up an IRP; however if the caller
        want to process the completion itself then it needs to specify
        a completion routine here.  This routine will then be called in
        place of IoCompleteRequest.

    UnlockRoutine - Optionally supplies a routine to call when removing
        a lock.

Return Value:

    None.

--*/

{
    DebugTrace(+1, Dbg, "FsRtlInitializeFileLock, FileLock = %08lx\n", FileLock);

    //
    // Clear non-paged pool pointer
    //

    FileLock->LockInformation = NULL;
    FileLock->CompleteLockIrpRoutine = CompleteLockIrpRoutine;
    FileLock->UnlockRoutine = UnlockRoutine;

    FileLock->FastIoIsQuestionable = FALSE;

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "FsRtlInitializeFileLock -> VOID\n", 0 );

    return;
}


BOOLEAN
FsRtlPrivateInitializeFileLock (
    IN PFILE_LOCK   FileLock,
    IN BOOLEAN ViaFastCall
    )
/*++

Routine Description:

    This routine initializes a new LOCK_INFO structure in non-paged
    pool for the FILE_LOCK.  This routines only occurs once for a given
    FILE_LOCK and it only occurs if any locks are applied to that file.

Arguments:

    FileLock - Supplies a pointer to the FILE_LOCK structure to
        initialize.

    ViaFastCall - Indicates if we are being invoked via a fast call or
        via the slow irp based method.

Return Value:

    TRUE - If LockInfo structure was allocated and initialized

--*/
{
    PLOCK_INFO  LockInfo;
    BOOLEAN     Results = FALSE;

    ExAcquireFastMutex( &FsRtlCreateLockInfo );

    try {

        if (FileLock->LockInformation != NULL) {

            //
            // Structure is already allocated, just return
            //

            try_return( Results = TRUE );
        }

        //
        //  Allocate pool for lock structures.  If we fail then we will either return false or
        //  raise based on if we know the caller has an try-except to handle a raise.
        //

        LockInfo = FsRtlAllocateLockInfo();

        if (LockInfo == NULL) {

            if (ViaFastCall) {

                try_return( Results = FALSE );

            } else {

                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }
        }

        //
        //  Allocate and initialize the waiting lock queue
        //  spinlock, and initialize the queues
        //

        LockInfo->LowestLockOffset = 0xffffffff;

        KeInitializeSpinLock( &LockInfo->LockQueue.QueueSpinLock );
        LockInfo->LockQueue.SharedLockTree = NULL;
        LockInfo->LockQueue.ExclusiveLockTree = NULL;
        LockInfo->LockQueue.WaitingLocks.Next = NULL;
        LockInfo->LockQueue.WaitingLocksTail.Next = NULL;

        //
        // Copy Irp & Unlock routines from pagable FileLock structure
        // to non-pagable LockInfo structure
        //

        LockInfo->CompleteLockIrpRoutine = FileLock->CompleteLockIrpRoutine;
        LockInfo->UnlockRoutine = FileLock->UnlockRoutine;

        //
        // Clear continuation info for enum routine
        //

        FileLock->LastReturnedLockInfo.FileObject = NULL;
        FileLock->LastReturnedLock = NULL;

        //
        // Link LockInfo into FileLock
        //

        FileLock->LockInformation = (PVOID) LockInfo;
        Results = TRUE;

    try_exit: NOTHING;
    } finally {

        ExReleaseFastMutex( &FsRtlCreateLockInfo );
    }

    return Results;
}


VOID
FsRtlUninitializeFileLock (
    IN PFILE_LOCK FileLock
    )

/*++

Routine Description:

    This routine uninitializes a FILE_LOCK structure.  After calling this
    routine the File lock must be reinitialized before being used again.

    This routine will free all files locks and completes any outstanding
    lock requests as a result of cleaning itself up.

Arguments:

    FileLock - Supplies a pointer to the FILE_LOCK struture being
        decommissioned.

Return Value:

    None.

--*/

{
    PLOCK_INFO          LockInfo;
    PSH_LOCK            ShLock;
    PEX_LOCK            ExLock;
    PSINGLE_LIST_ENTRY  Link;
    PWAITING_LOCK       WaitingLock;
    PLOCKTREE_NODE      LockTreeNode;
    PIRP                Irp;
    NTSTATUS            NewStatus;
    KIRQL               OldIrql;
    PKPRCB              Prcb;

    DebugTrace(+1, Dbg, "FsRtlUninitializeFileLock, FileLock = %08lx\n", FileLock);

    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {
        return ;
    }

    //
    //  Lock the queue
    //

    FsRtlAcquireLockQueue(&LockInfo->LockQueue, &OldIrql);

    //
    //  Free lock trees
    //

    while (LockInfo->LockQueue.SharedLockTree != NULL) {

        LockTreeNode = CONTAINING_RECORD(LockInfo->LockQueue.SharedLockTree, LOCKTREE_NODE, Links);

        //
        //  Remove all locks associated with the root node
        //

        while (LockTreeNode->Locks.Next != NULL) {
            Link = PopEntryList (&LockTreeNode->Locks);
            ShLock = CONTAINING_RECORD( Link, SH_LOCK, Link );

            FsRtlFreeSharedLock(ShLock);
        }

        //
        //  Slice off the root node of the tree
        //

        RtlDeleteNoSplay(&LockTreeNode->Links, &LockInfo->LockQueue.SharedLockTree);

        FsRtlFreeLockTreeNode(LockTreeNode);
    }

    while (LockInfo->LockQueue.ExclusiveLockTree != NULL) {

        ExLock = CONTAINING_RECORD(LockInfo->LockQueue.ExclusiveLockTree, EX_LOCK, Links);

        RtlDeleteNoSplay(&ExLock->Links, &LockInfo->LockQueue.ExclusiveLockTree);

        FsRtlFreeExclusiveLock(ExLock);
    }

    //
    //  Free WaitingLockQueue
    //

    while (LockInfo->LockQueue.WaitingLocks.Next != NULL) {

        Link = PopEntryList( &LockInfo->LockQueue.WaitingLocks );
        WaitingLock = CONTAINING_RECORD( Link, WAITING_LOCK, Link );

        Irp = WaitingLock->Irp;

        //
        //  To complete an irp in the waiting queue we need to
        //  void the cancel routine (protected by a spinlock) before
        //  we can complete the irp
        //

        FsRtlReleaseLockQueue (&LockInfo->LockQueue, OldIrql);

        IoAcquireCancelSpinLock( &Irp->CancelIrql );
        IoSetCancelRoutine( Irp, NULL );
        IoReleaseCancelSpinLock( Irp->CancelIrql );

        Irp->IoStatus.Information = 0;

        FsRtlCompleteLockIrp(
             LockInfo,
             WaitingLock->Context,
             Irp,
             STATUS_RANGE_NOT_LOCKED,
             &NewStatus,
             NULL );

        FsRtlAcquireLockQueue(&LockInfo->LockQueue, &OldIrql);
        FsRtlFreeWaitingLock( WaitingLock );
    }

    //
    // Free pool used to track the lock info on this file
    //

    FsRtlReleaseLockQueue( &LockInfo->LockQueue, OldIrql );
    FsRtlFreeLockInfo( LockInfo );

    //
    // Unlink LockInfo from FileLock
    //

    FileLock->LockInformation = NULL;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FsRtlUninitializeFileLock -> VOID\n", 0 );
    return;
}


PFILE_LOCK
FsRtlAllocateFileLock (
    IN PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine OPTIONAL,
    IN PUNLOCK_ROUTINE UnlockRoutine OPTIONAL

    )
{
    PFILE_LOCK FileLock;

    FileLock = ExAllocateFromPagedLookasideList( &FsRtlFileLockLookasideList );

    if (FileLock != NULL) {

        FsRtlInitializeFileLock( FileLock,
                                 CompleteLockIrpRoutine,
                                 UnlockRoutine );
    }

    return FileLock;
}

VOID
FsRtlFreeFileLock (
    IN PFILE_LOCK FileLock
    )
{
    FsRtlUninitializeFileLock( FileLock );

    ExFreeToPagedLookasideList( &FsRtlFileLockLookasideList, FileLock );
}


NTSTATUS
FsRtlProcessFileLock (
    IN PFILE_LOCK FileLock,
    IN PIRP Irp,
    IN PVOID Context OPTIONAL
    )

/*++

Routine Description:

    This routine processes a file lock IRP it does either a lock request,
    or an unlock request.  It also completes the IRP.  Once called the user
    (i.e., File System) has relinquished control of the input IRP.

    If pool is not available to store the information this routine will raise a
    status value indicating insufficient resources.

Arguments:

    FileLock - Supplies the File lock being modified/queried.

    Irp - Supplies the Irp being processed.

    Context - Optionally supplies a context to use when calling the user
        alternate IRP completion routine.

Return Value:

    NTSTATUS - The return status for the operation.

--*/

{
    PIO_STACK_LOCATION IrpSp;

    IO_STATUS_BLOCK Iosb;
    NTSTATUS        Status;
    LARGE_INTEGER   ByteOffset;

    DebugTrace(+1, Dbg, "FsRtlProcessFileLock, FileLock = %08lx\n", FileLock);

    Iosb.Information = 0;

    //
    //  Get a pointer to the current Irp stack location and assert that
    //  the major function code is for a lock operation
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    ASSERT( IrpSp->MajorFunction == IRP_MJ_LOCK_CONTROL );

    //
    //  Now process the different minor lock operations
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_LOCK:

        ByteOffset = IrpSp->Parameters.LockControl.ByteOffset;

        (VOID) FsRtlPrivateLock( FileLock,
                                 IrpSp->FileObject,
                                 &ByteOffset,
                                 IrpSp->Parameters.LockControl.Length,
                                 IoGetRequestorProcess(Irp),
                                 IrpSp->Parameters.LockControl.Key,
                                 BooleanFlagOn(IrpSp->Flags, SL_FAIL_IMMEDIATELY),
                                 BooleanFlagOn(IrpSp->Flags, SL_EXCLUSIVE_LOCK),
                                 &Iosb,
                                 Irp,
                                 Context,
                                 FALSE );

        break;

    case IRP_MN_UNLOCK_SINGLE:

        ByteOffset = IrpSp->Parameters.LockControl.ByteOffset;

        Iosb.Status = FsRtlFastUnlockSingle( FileLock,
                                             IrpSp->FileObject,
                                             &ByteOffset,
                                             IrpSp->Parameters.LockControl.Length,
                                             IoGetRequestorProcess(Irp),
                                             IrpSp->Parameters.LockControl.Key,
                                             Context,
                                             FALSE );

        FsRtlCompleteLockIrp( FileLock, Context, Irp, Iosb.Status, &Status, NULL );
        break;

    case IRP_MN_UNLOCK_ALL:

        Iosb.Status = FsRtlFastUnlockAll( FileLock,
                                          IrpSp->FileObject,
                                          IoGetRequestorProcess(Irp),
                                          Context );

        FsRtlCompleteLockIrp( FileLock, Context, Irp, Iosb.Status, &Status, NULL );
        break;

    case IRP_MN_UNLOCK_ALL_BY_KEY:

        Iosb.Status = FsRtlFastUnlockAllByKey( FileLock,
                                               IrpSp->FileObject,
                                               IoGetRequestorProcess(Irp),
                                               IrpSp->Parameters.LockControl.Key,
                                               Context );

        FsRtlCompleteLockIrp( FileLock, Context, Irp, Iosb.Status, &Status, NULL );
        break;

    default:

        //
        //  For all other minor function codes we say they're invalid and
        //  complete the request.  Note that the IRP has not been marked
        //  pending so this error will be returned directly to the caller.
        //

        DebugTrace(0, 1, "Invalid LockFile Minor Function Code %08lx\n", IrpSp->MinorFunction);


        FsRtlCompleteRequest( Irp, STATUS_INVALID_DEVICE_REQUEST );

        Iosb.Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FsRtlProcessFileLock -> %08lx\n", Iosb.Status);

    return Iosb.Status;
}


BOOLEAN
FsRtlCheckLockForReadAccess (
    IN PFILE_LOCK FileLock,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine checks to see if the caller has read access to the
    range indicated in the IRP due to file locks.  This call does not
    complete the Irp it only uses it to get the lock information and read
    information.  The IRP must be for a read operation.

Arguments:

    FileLock - Supplies the File Lock to check.

    Irp - Supplies the Irp being processed.

Return Value:

    BOOLEAN - TRUE if the indicated user/request has read access to the
        entire specified byte range, and FALSE otherwise

--*/

{
    BOOLEAN Result;

    PIO_STACK_LOCATION IrpSp;

    PLOCK_INFO     LockInfo;
    LARGE_INTEGER  StartingByte;
    LARGE_INTEGER  Length;
    ULONG          Key;
    PFILE_OBJECT   FileObject;
    PVOID          ProcessId;
    LARGE_INTEGER  BeyondLastByte;

    DebugTrace(+1, Dbg, "FsRtlCheckLockForReadAccess, FileLock = %08lx\n", FileLock);

    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {
        DebugTrace(-1, Dbg, "FsRtlCheckLockForReadAccess (No current lock info) -> TRUE\n", 0);
        return TRUE;
    }

    //
    //  Do a really fast test to see if there are any exclusive locks to start with
    //

    if (LockInfo->LockQueue.ExclusiveLockTree == NULL) {
        DebugTrace(-1, Dbg, "FsRtlCheckLockForReadAccess (No current locks) -> TRUE\n", 0);
        return TRUE;
    }

    //
    //  Get the read offset and compare it to the lowest existing lock.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    StartingByte  = IrpSp->Parameters.Read.ByteOffset;
    (ULONGLONG)Length.QuadPart = (ULONGLONG)IrpSp->Parameters.Read.Length;

    (ULONGLONG)BeyondLastByte.QuadPart = (ULONGLONG)StartingByte.QuadPart + Length.LowPart;
    if ( (ULONGLONG)BeyondLastByte.QuadPart <= (ULONGLONG)LockInfo->LowestLockOffset ) {
        DebugTrace(-1, Dbg, "FsRtlCheckLockForReadAccess (Below lowest lock) -> TRUE\n", 0);
        return TRUE;
    }

    //
    //  Get remaining parameters.
    //

    Key           = IrpSp->Parameters.Read.Key;
    FileObject    = IrpSp->FileObject;
    ProcessId     = IoGetRequestorProcess( Irp );

    //
    //  Call our private work routine to do the real check
    //

    Result = FsRtlFastCheckLockForRead( FileLock,
                                        &StartingByte,
                                        &Length,
                                        Key,
                                        FileObject,
                                        ProcessId );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FsRtlCheckLockForReadAccess -> %08lx\n", Result);

    return Result;
}


BOOLEAN
FsRtlCheckLockForWriteAccess (
    IN PFILE_LOCK FileLock,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine checks to see if the caller has write access to the
    indicated range due to file locks.  This call does not complete the
    Irp it only uses it to get the lock information and write information.
    The IRP must be for a write operation.

Arguments:

    FileLock - Supplies the File Lock to check.

    Irp - Supplies the Irp being processed.

Return Value:

    BOOLEAN - TRUE if the indicated user/request has write access to the
        entire specified byte range, and FALSE otherwise

--*/

{
    BOOLEAN Result;

    PIO_STACK_LOCATION IrpSp;

    PLOCK_INFO      LockInfo;
    LARGE_INTEGER   StartingByte;
    LARGE_INTEGER   Length;
    ULONG           Key;
    PFILE_OBJECT    FileObject;
    PVOID           ProcessId;
    LARGE_INTEGER   BeyondLastByte;

    DebugTrace(+1, Dbg, "FsRtlCheckLockForWriteAccess, FileLock = %08lx\n", FileLock);

    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {
        DebugTrace(-1, Dbg, "FsRtlCheckLockForWriteAccess (No current lock info) -> TRUE\n", 0);
        return TRUE;
    }

    //
    //  Do a really fast test to see if there are any locks to start with
    //

    if (LockInfo->LockQueue.ExclusiveLockTree == NULL && LockInfo->LockQueue.SharedLockTree == NULL) {
        DebugTrace(-1, Dbg, "FsRtlCheckLockForWriteAccess (No current locks) -> TRUE\n", 0);
        return TRUE;
    }

    //
    //  Get the write offset and compare it to the lowest existing lock.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    StartingByte  = IrpSp->Parameters.Write.ByteOffset;
    (ULONGLONG)Length.QuadPart = (ULONGLONG)IrpSp->Parameters.Write.Length;

    (ULONGLONG)BeyondLastByte.QuadPart = (ULONGLONG)StartingByte.QuadPart + Length.LowPart;
    if ( (ULONGLONG)BeyondLastByte.QuadPart <= (ULONGLONG)LockInfo->LowestLockOffset ) {
        DebugTrace(-1, Dbg, "FsRtlCheckLockForWriteAccess (Below lowest lock) -> TRUE\n", 0);
        return TRUE;
    }

    //
    //  Get remaining parameters.
    //

    Key           = IrpSp->Parameters.Write.Key;
    FileObject    = IrpSp->FileObject;
    ProcessId     = IoGetRequestorProcess( Irp );

    //
    //  Call our private work routine to do the real work
    //

    Result = FsRtlFastCheckLockForWrite( FileLock,
                                         &StartingByte,
                                         &Length,
                                         Key,
                                         FileObject,
                                         ProcessId );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FsRtlCheckLockForWriteAccess -> %08lx\n", Result);

    return Result;
}


PRTL_SPLAY_LINKS
FsRtlFindFirstOverlappingSharedNode (
    IN PRTL_SPLAY_LINKS         Tree,
    IN PLARGE_INTEGER           StartingByte,
    IN PLARGE_INTEGER           EndingByte,
    IN OUT PRTL_SPLAY_LINKS     *LastEdgeNode,
    IN OUT PBOOLEAN             GreaterThan
    )
/*++

Routine Description:

    This routine returns the first node in the shared lock tree which
    overlaps with the range given. No nodes given by RtlRealPredecessor()
    on the result overlap the range.

Arguments:

    Tree - supplies the splay links of the root node of the shared tree
        to search

    StartingByte - supplies the first byte offset of the range to check

    EndingByte - supplies the last byte offset of the range to check

    LastEdgeNode - optional, will be set to the last node searched in the
        not including returned node (presumeably where a new node will
        be inserted if return is NULL).

    GreaterThan - optional, set according to whether LastEdgeNode is covering
        a range greater than the queried range. !GreaterThan == LessThan, since
        we would have returned this node in the "Equals" (overlap) case.

Return Value:

    The splay links of the node, if such a node exists, NULL otherwise

--*/
{
    PLOCKTREE_NODE        Node, LastOverlapNode;
    PRTL_SPLAY_LINKS      SplayLinks;
    PSH_LOCK              Lock;

    if (LastEdgeNode) *LastEdgeNode = NULL;
    if (GreaterThan) *GreaterThan = FALSE;

    LastOverlapNode = NULL;
    SplayLinks = Tree;

    while (SplayLinks) {

        Node = CONTAINING_RECORD( SplayLinks, LOCKTREE_NODE, Links );

        //
        //  Pull up the first lock on the chain at this node to check
        //  the starting byte offset of locks at this node
        //

        Lock = CONTAINING_RECORD( Node->Locks.Next, SH_LOCK, Link );

        //
        //  We may have to go right in the tree if this lock covers a range before the start of this
        //  range we are looking for overlap on or this lock is [0, 0).  This is important since a lock
        //  on [0, 0) will look like the extent is from [0, ~0], which is the only case where the zero
        //  length lock relation of End < Start does not hold.
        //

        if (Node->Extent < (ULONGLONG)StartingByte->QuadPart ||
            (Lock->LockInfo.StartingByte.QuadPart == 0 && Lock->LockInfo.Length.QuadPart == 0)) {

            if ((ULONGLONG)Lock->LockInfo.EndingByte.QuadPart == (ULONGLONG)EndingByte->QuadPart &&
                (ULONGLONG)Lock->LockInfo.StartingByte.QuadPart == (ULONGLONG)StartingByte->QuadPart) {

                //
                //  The extent of the node is less than the starting position of the
                //  range we are checking and the first lock on this node is equal to
                //  the range, which implies that the range and the lock are zero
                //  length.
                //
                //  This is a zero length lock node and we are searching for zero
                //  length overlap. This makes multiple zero length shared locks
                //  occupy the same node, which is a win, but makes application of
                //  zero length exclusive locks check the length of the overlapping
                //  lock to see if they really conflict.
                //

                break;
            }

            //
            //  All locks at this node are strictly less than this
            //  byterange, so go right in the tree.
            //

            if (LastEdgeNode) *LastEdgeNode = SplayLinks;
            if (GreaterThan) *GreaterThan = FALSE;

            SplayLinks = RtlRightChild(SplayLinks);
            continue;
        }

        if ((ULONGLONG)Lock->LockInfo.StartingByte.QuadPart <= (ULONGLONG)EndingByte->QuadPart) {

            //
            //  We have an overlap, but we need to see if the byterange starts
            //  before this node so that there is the guarantee that we start
            //  the search at the correct point. There may be still be predecessor
            //  nodes covering the byterange.
            //

            if ((ULONGLONG)Lock->LockInfo.StartingByte.QuadPart <= (ULONGLONG)StartingByte->QuadPart) {

                //
                //  This node begins at a byte offset prior to the byterange we
                //  are checking, so it must be the correct starting position.
                //

                break;
            }

            //
            //  Drop a marker at this node so that we can come back if it turns out
            //  that the left subtree does not cover the range of bytes before this
            //  node in the byterange.
            //

            LastOverlapNode = Node;
        }

        //
        //  It must now be the case that all locks at this node are strictly greater
        //  than the byterange, or we have the candidate overlap case above,
        //  so go left in the tree.
        //

        if (LastEdgeNode) *LastEdgeNode = SplayLinks;
        if (GreaterThan) *GreaterThan = TRUE;

        SplayLinks = RtlLeftChild(SplayLinks);
    }

    if (SplayLinks == NULL) {

        //
        //  We hit the edge of the tree. If the LastOverlapNode is set, it means that
        //  we had kept searching left in the tree for a node that covered the starting
        //  byte of the byterange, but didn't find it. If it isn't set, we'll do the
        //  right thing anyway since Node <- NULL.
        //

        Node = LastOverlapNode;
    }

    if (Node == NULL) {

        //
        // No overlapping node existed
        //

        return NULL;
    }

    //
    // Return the splay links of the first overlapping node
    //

    return &Node->Links;
}


PRTL_SPLAY_LINKS
FsRtlFindFirstOverlappingExclusiveNode (
    IN PRTL_SPLAY_LINKS        Tree,
    IN PLARGE_INTEGER          StartingByte,
    IN PLARGE_INTEGER          EndingByte,
    IN OUT PRTL_SPLAY_LINKS    *LastEdgeNode,
    IN OUT PBOOLEAN            GreaterThan
    )
/*++

Routine Description:

    This routine returns the first node in the exclusive lock tree which
    overlaps with the range given. No nodes given by RtlRealPredecessor()
    on the result overlap the range.

Arguments:

    Tree - supplies the splay links of the root node of the exclusive tree
        to search

    StartingByte - supplies the first byte offset of the range to check

    EndingByte - supplies the last byte offset of the range to check

    LastEdgeNode - optional, will be set to the last node searched
        not including returned node (presumeably where a new node will
        be inserted if return is NULL).

    GreaterThan - optional, set according to whether LastEdgeNode is covering
        a range greater than the queried range. !GreaterThan == LessThan, since
        we would have returned this node in the "Equals" (overlap) case.

Return Value:

    The splay links of the node, if such a node exists, NULL otherwise

--*/
{
    PRTL_SPLAY_LINKS    SplayLinks;
    PEX_LOCK            Lock, LastOverlapNode;

    if (LastEdgeNode) *LastEdgeNode = NULL;
    if (GreaterThan) *GreaterThan = FALSE;

    LastOverlapNode = NULL;
    SplayLinks = Tree;

    while (SplayLinks) {

        Lock = CONTAINING_RECORD( SplayLinks, EX_LOCK, Links );

        //
        //  We may have to go right in the tree if this lock covers a range before the start of this
        //  range we are looking for overlap on or this lock is [0, 0).  This is important since a lock
        //  on [0, 0) will look like the extent is from [0, ~0], which is the only case where the zero
        //  length lock relation of End < Start does not hold.
        //

        if ((ULONGLONG)Lock->LockInfo.EndingByte.QuadPart < (ULONGLONG)StartingByte->QuadPart ||
            (Lock->LockInfo.StartingByte.QuadPart == 0 && Lock->LockInfo.Length.QuadPart == 0)) {

            if ((ULONGLONG)Lock->LockInfo.EndingByte.QuadPart == (ULONGLONG)EndingByte->QuadPart &&
                (ULONGLONG)Lock->LockInfo.StartingByte.QuadPart == (ULONGLONG)StartingByte->QuadPart) {

                //
                //  The extent of the lock is less than the starting position of the
                //  range we are checking and the lock is equal to the range, which
                //  implies that the range and the lock are zero length.
                //
                //  This is a zero length lock node and we are searching for zero
                //  length overlap. Since the exclusive tree is one lock per node,
                //  we are in the potential middle of a run of zero length locks in
                //  the tree. Go left to find the first zero length lock.
                //
                //  This is actually the same logic we'd use for equivalent locks,
                //  but the only time that can happen in this tree is for zero length
                //  locks.
                //

                LastOverlapNode = Lock;

                if (LastEdgeNode) *LastEdgeNode = SplayLinks;
                if (GreaterThan) *GreaterThan = FALSE;

                SplayLinks = RtlLeftChild(SplayLinks);
                continue;
            }

            //
            //  This lock is strictly less than this byterange, so go
            //  right in the tree.
            //

            if (LastEdgeNode) *LastEdgeNode = SplayLinks;
            if (GreaterThan) *GreaterThan = FALSE;

            SplayLinks = RtlRightChild(SplayLinks);
            continue;
        }

        if ((ULONGLONG)Lock->LockInfo.StartingByte.QuadPart <= (ULONGLONG)EndingByte->QuadPart) {

            //
            //  We have an overlap, but we need to see if the byterange starts
            //  before this node so that there is the guarantee that we start
            //  the search at the correct point. There may be still be predecessor
            //  nodes covering the byterange.
            //

            if ((ULONGLONG)Lock->LockInfo.StartingByte.QuadPart <= (ULONGLONG)StartingByte->QuadPart) {

                //
                //  This node begins at a byte offset prior to the byterange we
                //  are checking, so it must be the correct starting position.
                //

                break;
            }

            //
            //  Drop a marker at this node so that we can come back if it turns out
            //  that the left subtree does not cover the range of bytes before this
            //  node in the byterange.
            //

            LastOverlapNode = Lock;
        }

        //
        //  It must now be the case this lock is strictly greater than the byterange,
        //  or we have the candidate overlap case above, so go left in the tree.
        //

        if (LastEdgeNode) *LastEdgeNode = SplayLinks;
        if (GreaterThan) *GreaterThan = TRUE;

        SplayLinks = RtlLeftChild(SplayLinks);
    }

    if (SplayLinks == NULL) {

        //
        //  We hit the edge of the tree. If the LastOverlapNode is set, it means that
        //  we had kept searching left in the tree for a node that covered the starting
        //  byte of the byterange, but didn't find it. If it isn't set, we'll do the
        //  right thing anyway since Node <- NULL.
        //

        Lock = LastOverlapNode;
    }

    if (Lock == NULL) {

        //
        // No overlapping lock existed
        //

        return NULL;
    }

    //
    // Return the splay links of the first overlapping lock
    //

    return &Lock->Links;
}


PSH_LOCK
FsRtlFindFirstOverlapInNode (
    IN PLOCKTREE_NODE Node,
    IN PLARGE_INTEGER StartingByte,
    IN PLARGE_INTEGER EndingByte
    )

/*++

Routine Description:

    This routine examines a shared lock node, usually a node which is known to be composed
    of several non-overlapping lock segments (holey), for true overlap with the indicated
    range.  This is not handled in the normal overlap check (..FindFirstOverlappingSharedLock)
    since the needs for holey checks are rather different than the full node check.

Arguments:

    Node - the lock tree node to be examined for overlap

    StartingByte - supplies the first byte offset of the range to check

    EndingByte - supplies the last byte offset of the range to check

Return Value:

    PSH_LOCK - the first lock which overlaps with the specified range.

--*/
{
    PSH_LOCK Lock;
    PSINGLE_LIST_ENTRY Link;

    for (Link = Node->Locks.Next;
         Link;
         Link = Link->Next) {

        Lock = CONTAINING_RECORD( Link, SH_LOCK, Link );

        //
        //  Logic is the same as above checkers.  If the ending byte of the lock is less than the
        //  starting byte of the range, OR we have the weird [0, 0) case, then the lock is almost
        //  certainly less than the range.
        //

        if ((ULONGLONG)Lock->LockInfo.EndingByte.QuadPart < (ULONGLONG)StartingByte->QuadPart ||
            (Lock->LockInfo.StartingByte.QuadPart == 0 && Lock->LockInfo.Length.QuadPart == 0)) {

            //
            //  ... except if the lock and range are equivalent, in which case we have discovered
            //  zero lock/range overlap.
            //

            if ((ULONGLONG)Lock->LockInfo.EndingByte.QuadPart == (ULONGLONG)EndingByte->QuadPart &&
                (ULONGLONG)Lock->LockInfo.StartingByte.QuadPart == (ULONGLONG)StartingByte->QuadPart) {

                return Lock;
            }

            //
            //  Look forward in the node.
            //

            continue;
        }

        //
        //  No overlap at all if the lock begins at a higher byte than the last of the range.
        //  We already covered zero length locks (where this is true, and overlap could still
        //  occur).
        //

        if ((ULONGLONG)Lock->LockInfo.StartingByte.QuadPart > (ULONGLONG)EndingByte->QuadPart) {

            return NULL;
        }

        //
        //  Regular overlap has occured.  Return this lock.
        //

        return Lock;
    }

    //
    //  If we invoke this check and wander off the end of the node without determining what is
    //  going on, something is terribly wrong.
    //

    ASSERT( FALSE );

    return NULL;
}


PFILE_LOCK_INFO
FsRtlGetNextFileLock (
    IN PFILE_LOCK FileLock,
    IN BOOLEAN Restart
    )

/*++

Routine Description:

    This routine enumerates the individual file locks denoted by the input file lock
    variable. It returns a pointer to the file lock information stored for each lock.
    The caller is responsible for synchronizing call to this procedure and for not
    altering any of the data returned by this procedure. If the caller does not
    synchronize the enumeration will not be reliably complete.

    The way a programmer will use this procedure to enumerate all of the locks
    is as follows:

    for (p = FsRtlGetNextFileLock( FileLock, TRUE );
         p != NULL;
         p = FsRtlGetNextFileLock( FileLock, FALSE )) {

            // Process the lock information referenced by p
    }

    Order is *not* guaranteed.

Arguments:

    FileLock - Supplies the File Lock to enumerate.  The current
        enumeration state is stored in the file lock variable so if multiple
        threads are enumerating the lock at the same time the results will
        be unpredictable.

    Restart - Indicates if the enumeration is to start at the beginning of the
        file lock tree or if we are continuing from a previous call.

Return Value:

    PFILE_LOCK_INFO - Either it returns a pointer to the next file lock
        record for the input file lock or it returns NULL if there
        are not more locks.

--*/

{
    FILE_LOCK_INFO      FileLockInfo;
    PVOID               ContinuationPointer;
    PLOCK_INFO          LockInfo;
    PLOCKTREE_NODE      Node;
    PSINGLE_LIST_ENTRY  Link;
    PRTL_SPLAY_LINKS    SplayLinks, LastSplayLinks;
    PSH_LOCK            ShLock;
    PEX_LOCK            ExLock;
    BOOLEAN             FoundReturnable, GreaterThan;
    KIRQL               OldIrql;

    DebugTrace(+1, Dbg, "FsRtlGetNextFileLock, FileLock = %08lx\n", FileLock);

    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {
        //
        //  No lock information on this FileLock
        //

        return NULL;
    }

    FoundReturnable = FALSE;

    //
    //  Before getting the spinlock, copy pagable info onto stack
    //

    FileLockInfo = FileLock->LastReturnedLockInfo;
    ContinuationPointer = FileLock->LastReturnedLock;

    FsRtlAcquireLockQueue (&LockInfo->LockQueue, &OldIrql);

    if (!Restart) {
        //
        //  Given the last returned lock, find its current successor in the tree.
        //  Previous implementations would reset the enumeration if the last returned
        //  lock had been removed from the tree but I think we can be better in that
        //  case since every other structure modifying event (add new locks, delete
        //  other locks) would *not* have caused the reset. Possible minor performance
        //  enhancement.
        //

        //
        //  Find the node which could contain the last returned lock. We enumerate the
        //  exclusive lock tree, then the shared lock tree. Find the one we're enumerating.
        //

        if (FileLockInfo.ExclusiveLock) {

            //
            //  Continue enumeration in the exclusive lock tree
            //

            ExLock = NULL;

            SplayLinks = FsRtlFindFirstOverlappingExclusiveNode( LockInfo->LockQueue.ExclusiveLockTree,
                                                                 &FileLockInfo.StartingByte,
                                                                 &FileLockInfo.EndingByte,
                                                                 &LastSplayLinks,
                                                                 &GreaterThan );

            if (SplayLinks == NULL) {

                //
                //  No overlapping nodes were found, try to find successor
                //

                if (GreaterThan) {

                    //
                    //  Last node looked at was greater than the lock so it is
                    //  the place to pick up the enumeration
                    //

                    SplayLinks = LastSplayLinks;

                } else {

                    //
                    // Last node looked at was less than the lock so grab its successor
                    //

                    if (LastSplayLinks) {

                        SplayLinks = RtlRealSuccessor(LastSplayLinks);
                    }
                }

            } else {

                //
                //  Found an overlapping lock, see if it is the last returned
                //

                for (;
                    SplayLinks;
                    SplayLinks = RtlRealSuccessor(SplayLinks)) {

                    ExLock = CONTAINING_RECORD( SplayLinks, EX_LOCK, Links );

                    if (ContinuationPointer == ExLock &&
                        (ULONGLONG)FileLockInfo.StartingByte.QuadPart == (ULONGLONG)ExLock->LockInfo.StartingByte.QuadPart &&
                        (ULONGLONG)FileLockInfo.Length.QuadPart == (ULONGLONG)ExLock->LockInfo.Length.QuadPart &&
                        FileLockInfo.Key == ExLock->LockInfo.Key &&
                        FileLockInfo.FileObject == ExLock->LockInfo.FileObject &&
                        FileLockInfo.ProcessId == ExLock->LockInfo.ProcessId) {

                        //
                        //  Found last returned, dig up its successor
                        //

                        SplayLinks = RtlRealSuccessor(SplayLinks);

                        //
                        //  Got the node cold, so we're done
                        //

                        break;
                    }

                    //
                    //  This lock overlapped and was not the last returned. In fact, since this lock would
                    //  have conflicted with the last returned we know it could not have been returned
                    //  before, so this should be returned to the caller.
                    //
                    //  However, if it is a zero length lock we are looking for and a zero length lock we hit,
                    //  we are at the beginning of a run we need to inspect. If we cannot find the last lock
                    //  we returned, resume the enumeration at the beginning of the run.
                    //

                    if (ExLock->LockInfo.Length.QuadPart != 0 || FileLockInfo.Length.QuadPart != 0) {

                        break;
                    }

                    //
                    //  Keep wandering down the run
                    //
                }
            }

            //
            //  Were we able to find a lock to return?
            //

            if (SplayLinks == NULL) {

                //
                //  There aren't any more exclusive locks, fall over to the shared tree
                //

                SplayLinks = LockInfo->LockQueue.SharedLockTree;

                if (SplayLinks) {

                    while (RtlLeftChild(SplayLinks)) {

                        SplayLinks = RtlLeftChild(SplayLinks);
                    }

                    Node = CONTAINING_RECORD(SplayLinks, LOCKTREE_NODE, Links);
                    ShLock = CONTAINING_RECORD(Node->Locks.Next, SH_LOCK, Link);

                    FileLockInfo = ShLock->LockInfo;
                    ContinuationPointer = ShLock;
                    FoundReturnable = TRUE;
                }

            } else {

                //
                //  This is the lock to return
                //

                ExLock = CONTAINING_RECORD( SplayLinks, EX_LOCK, Links );

                FileLockInfo = ExLock->LockInfo;
                ContinuationPointer = ExLock;
                FoundReturnable = TRUE;
            }

        } else {

            //
            //  Continue enumeration in the shared lock tree
            //

            Node = NULL;

            SplayLinks = FsRtlFindFirstOverlappingSharedNode( LockInfo->LockQueue.SharedLockTree,
                                                              &FileLockInfo.StartingByte,
                                                              &FileLockInfo.EndingByte,
                                                              &LastSplayLinks,
                                                              &GreaterThan );

            if (SplayLinks == NULL) {

                //
                //  No overlapping nodes were found
                //

                if (GreaterThan) {

                    //
                    //  Last node looked at was greater than the lock so it is
                    //  the place to pick up the enumeration
                    //

                    if (LastSplayLinks) {

                        SplayLinks = LastSplayLinks;
                        Node = CONTAINING_RECORD( LastSplayLinks, LOCKTREE_NODE, Links );
                    }

                } else {

                    //
                    // Last node looked at was less than the lock so grab its successor
                    //

                    if (LastSplayLinks) {

                        SplayLinks = RtlRealSuccessor(LastSplayLinks);

                        if (SplayLinks) {

                            Node = CONTAINING_RECORD( SplayLinks, LOCKTREE_NODE, Links );
                        }
                    }
                }

            } else {

                //
                //  Grab the node we found
                //

                Node = CONTAINING_RECORD( SplayLinks, LOCKTREE_NODE, Links );
            }

            //
            //  If we have a node to look at, it may still not contain the the last returned lock
            //  if this isn't synchronized.
            //

            if (Node != NULL) {

                //
                //    Walk down the locks at this node looking for the last returned lock
                //

                for (Link = Node->Locks.Next;
                     Link;
                     Link = Link->Next) {

                    //
                    //  Get a pointer to the current lock record
                    //

                    ShLock = CONTAINING_RECORD( Link, SH_LOCK, Link );

                    //
                    // See if it's a match
                    //

                    if (ContinuationPointer == ShLock &&
                        (ULONGLONG)FileLockInfo.StartingByte.QuadPart == (ULONGLONG)ShLock->LockInfo.StartingByte.QuadPart &&
                        (ULONGLONG)FileLockInfo.Length.QuadPart == (ULONGLONG)ShLock->LockInfo.Length.QuadPart &&
                        FileLockInfo.Key == ShLock->LockInfo.Key &&
                        FileLockInfo.FileObject == ShLock->LockInfo.FileObject &&
                        FileLockInfo.ProcessId == ShLock->LockInfo.ProcessId) {

                        Link = Link->Next;
                        break;
                    }

                    //
                    // See if we passed by its slot
                    //

                    if ((ULONGLONG)FileLockInfo.StartingByte.QuadPart < (ULONGLONG)ShLock->LockInfo.StartingByte.QuadPart) {

                        break;
                    }
                }

                if (Link == NULL) {

                    //
                    //  This node doesn't contain the successor, so move
                    //  up to the successor node in the tree and return the
                    //  first lock. If we're actually at the end of the tree
                    //  we just fall off the end correctly.
                    //

                    SplayLinks = RtlRealSuccessor(SplayLinks);

                    if (SplayLinks) {

                        Node = CONTAINING_RECORD( SplayLinks, LOCKTREE_NODE, Links );

                        Link = Node->Locks.Next;
                    }
                }

                if (Link) {

                    //
                    //  Found a Lock to return, copy it to the stack
                    //

                    ShLock = CONTAINING_RECORD( Link, SH_LOCK, Link );

                    FileLockInfo = ShLock->LockInfo;
                    ContinuationPointer = ShLock;
                    FoundReturnable = TRUE;
                }

            }
        }

    } else {

        //
        //  Restarting the enumeration. Find leftmost node in the exclusive tree and hand back
        //  the first lock, falling over to the shared if no exlcusive locks are applied
        //

        if (LockInfo->LockQueue.ExclusiveLockTree) {

            SplayLinks = LockInfo->LockQueue.ExclusiveLockTree;

            while (RtlLeftChild(SplayLinks) != NULL) {

                SplayLinks = RtlLeftChild(SplayLinks);
            }

            ExLock = CONTAINING_RECORD( SplayLinks, EX_LOCK, Links );

            FileLockInfo = ExLock->LockInfo;
            ContinuationPointer = ExLock;
            FoundReturnable = TRUE;

        } else {

            if (LockInfo->LockQueue.SharedLockTree) {

                SplayLinks = LockInfo->LockQueue.SharedLockTree;

                while (RtlLeftChild(SplayLinks) != NULL) {

                    SplayLinks = RtlLeftChild(SplayLinks);
                }

                Node = CONTAINING_RECORD( SplayLinks, LOCKTREE_NODE, Links );
                ShLock = CONTAINING_RECORD( Node->Locks.Next, SH_LOCK, Link );

                FileLockInfo = ShLock->LockInfo;
                ContinuationPointer = ShLock;
                FoundReturnable = TRUE;
            }
        }
    }

    //
    //  Release all the lock queues
    //

    FsRtlReleaseLockQueue (&LockInfo->LockQueue, OldIrql);

    if (!FoundReturnable) {

        //
        //  No returnable lock was found, end of list
        //

        return NULL;
    }

    //
    // Update current enum location information
    //

    FileLock->LastReturnedLockInfo = FileLockInfo;
    FileLock->LastReturnedLock = ContinuationPointer;

    //
    // Return lock record to caller
    //

    return &FileLock->LastReturnedLockInfo;
}


BOOLEAN
FsRtlCheckNoSharedConflict (
   IN PLOCK_QUEUE LockQueue,
   IN PLARGE_INTEGER Starting,
   IN PLARGE_INTEGER Ending
   )
/*++

Routine Description:

    This routine checks to see if there is overlap in the shared locks with
    the given range. It is intended for use in the write access check path
    so that a rebalance will occur.

Arguments:

    FileLock - Supplies the File Lock to check

    StartingByte - Supplies the first byte (zero based) to check

    Length - Supplies the length, in bytes, to check

    Key - Supplies the key to use in the check

    FileObject - Supplies the file object to use in the check

    ProcessId - Supplies the Process Id to use in the check

Return Value:

    BOOLEAN - TRUE if the indicated user/request doesn't conflict in
        entire specified byte range, and FALSE otherwise

--*/
{
    PRTL_SPLAY_LINKS SplayLinks, BeginLinks;
    PLOCKTREE_NODE Node;

    SplayLinks = FsRtlFindFirstOverlappingSharedNode( LockQueue->SharedLockTree,
                                                      Starting,
                                                      Ending,
                                                      &BeginLinks,
                                                      NULL);

    if (BeginLinks) {

        LockQueue->SharedLockTree = RtlSplay(BeginLinks);
    }

    //
    //  If this node is holey, we'll have to walk the whole thing.
    //

    if (SplayLinks) {

        Node = CONTAINING_RECORD( SplayLinks, LOCKTREE_NODE, Links );

        if (Node->HoleyNode) {

            return (BOOLEAN)(FsRtlFindFirstOverlapInNode( Node, Starting, Ending ) == NULL);
        }

        //
        //  Overlapping non-holey node, so we do have shared lock conflict.
        //

        return FALSE;
    }

    //
    //  No node overlaps.
    //

    return TRUE;
}


BOOLEAN
FsRtlCheckNoExclusiveConflict (
    IN PLOCK_QUEUE LockQueue,
    IN PLARGE_INTEGER Starting,
    IN PLARGE_INTEGER Ending,
    IN ULONG Key,
    IN PFILE_OBJECT FileObject,
    IN PVOID ProcessId
    )
/*++

Routine Description:

    This routine checks to see if there is conflict in the exclusive locks with
    a given range and identifying tuple of key, fileobject and process. This is
    for part of the read access path.

Arguments:

    FileLock - Supplies the File Lock to check

    StartingByte - Supplies the first byte (zero based) to check

    Length - Supplies the length, in bytes, to check

    Key - Supplies the key to use in the check

    FileObject - Supplies the file object to use in the check

    ProcessId - Supplies the Process Id to use in the check

Return Value:

    BOOLEAN - TRUE if the indicated user/request doesn't conflict in
        entire specified byte range, and FALSE otherwise

--*/
{
    PRTL_SPLAY_LINKS SplayLinks, BeginLinks;
    PEX_LOCK Lock;
    BOOLEAN Status = TRUE;

    //
    //  Find the node to begin the search at and go
    //

    for (SplayLinks = FsRtlFindFirstOverlappingExclusiveNode( LockQueue->ExclusiveLockTree,
                                                              Starting,
                                                              Ending,
                                                              &BeginLinks,
                                                              NULL);
         SplayLinks;
         SplayLinks = RtlRealSuccessor(SplayLinks)) {

        Lock = CONTAINING_RECORD( SplayLinks, EX_LOCK, Links );

        //
        //  If the current lock is greater than the end of the range we're
        //  looking for then the the user doesn't conflict
        //
        //  if (Ending < Lock->StartingByte) ...
        //

        if ((ULONGLONG)Ending->QuadPart < (ULONGLONG)Lock->LockInfo.StartingByte.QuadPart) {

            DebugTrace(0, Dbg, "FsRtlCheckForExclusiveConflict, Ending < Lock->StartingByte\n", 0);

            break;
        }

        //
        //  Check for any overlap with the request. The test for
        //  overlap is that starting byte is less than or equal to the locks
        //  ending byte, and the ending byte is greater than or equal to the
        //  locks starting byte.  We already tested for this latter case in
        //  the preceding statement.
        //
        //  if (Starting <= Lock->StartingByte + Lock->Length - 1) ...
        //

        if ((ULONGLONG)Starting->QuadPart <= (ULONGLONG)Lock->LockInfo.EndingByte.QuadPart) {

            //
            //  This request overlaps the lock. We cannot grant the request
            //  if the file object, process id, and key do not match. Otherwise
            //  we'll continue looping looking at locks
            //

            if ((Lock->LockInfo.FileObject != FileObject) ||
                (Lock->LockInfo.ProcessId != ProcessId) ||
                (Lock->LockInfo.Key != Key)) {

                DebugTrace(0, Dbg, "FsRtlCheckForExclusiveConflict, Range locked already\n", 0);

                Status = FALSE;
                break;
            }
        }
    }

    if (BeginLinks) {

        LockQueue->ExclusiveLockTree = RtlSplay(BeginLinks);
    }

    //
    //  We searched the entire range without a conflict so we'll note no conflict
    //

    return Status;
}


BOOLEAN
FsRtlFastCheckLockForRead (
    IN PFILE_LOCK FileLock,
    IN PLARGE_INTEGER StartingByte,
    IN PLARGE_INTEGER Length,
    IN ULONG Key,
    IN PFILE_OBJECT FileObject,
    IN PVOID ProcessId
    )

/*++

Routine Description:

    This routine checks to see if the caller has read access to the
    indicated range due to file locks.

Arguments:

    FileLock - Supplies the File Lock to check

    StartingByte - Supplies the first byte (zero based) to check

    Length - Supplies the length, in bytes, to check

    Key - Supplies the to use in the check

    FileObject - Supplies the file object to use in the check

    ProcessId - Supplies the Process Id to use in the check

Return Value:

    BOOLEAN - TRUE if the indicated user/request has read access to the
        entire specified byte range, and FALSE otherwise

--*/

{
    LARGE_INTEGER Starting;
    LARGE_INTEGER Ending;

    PLOCK_INFO            LockInfo;
    PLOCK_QUEUE           LockQueue;
    KIRQL                 OldIrql;
    PFILE_LOCK_INFO       LastLock;
    BOOLEAN               Status;

    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {

        //
        // No lock information on this FileLock
        //

        DebugTrace(0, Dbg, "FsRtlFastCheckLockForRead, No lock info\n", 0);
        return TRUE;
    }

    //
    // If there isn't an exclusive lock then we can immediately grant access
    //

    if (LockInfo->LockQueue.ExclusiveLockTree == NULL) {
        DebugTrace(0, Dbg, "FsRtlFastCheckLockForRead, No exlocks present\n", 0);
        return TRUE;
    }

    //
    // If length is zero then automatically give grant access
    //

    if ((ULONGLONG)Length->QuadPart == 0) {

        DebugTrace(0, Dbg, "FsRtlFastCheckLockForRead, Length == 0\n", 0);
        return TRUE;
    }

    //
    //  Get our starting and ending byte position
    //

    Starting = *StartingByte;
    (ULONGLONG)Ending.QuadPart = (ULONGLONG)Starting.QuadPart + (ULONGLONG)Length->QuadPart - 1;

    //
    // Now check lock queue
    //

    LockQueue = &LockInfo->LockQueue;

    //
    //  Grab the waiting lock queue spinlock to exclude anyone from messing
    //  with the queue while we're using it
    //

    FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

    //
    //  If the range ends below the lowest existing lock, this read is OK.
    //

    if ( ((ULONGLONG)Ending.QuadPart < (ULONGLONG)LockInfo->LowestLockOffset) ) {
        DebugTrace(0, Dbg, "FsRtlFastCheckLockForRead (below lowest lock)\n", 0);

        FsRtlReleaseLockQueue(LockQueue, OldIrql);
        return TRUE;
    }

    //
    //  If the caller just locked this range, he can read it.
    //

    LastLock = (PFILE_LOCK_INFO)FileObject->LastLock;
    if ((LastLock != NULL) &&
        ((ULONGLONG)Starting.QuadPart >= (ULONGLONG)LastLock->StartingByte.QuadPart) &&
        ((ULONGLONG)Ending.QuadPart <= (ULONGLONG)LastLock->EndingByte.QuadPart) &&
        (LastLock->Key == Key) &&
        (LastLock->ProcessId == ProcessId)) {

        FsRtlReleaseLockQueue(LockQueue, OldIrql);
        return TRUE;
    }

    //
    //  Check the exclusive locks for a conflict. It is impossible to have
    //  a read conflict with any shared lock.
    //

    Status = FsRtlCheckNoExclusiveConflict(LockQueue, &Starting, &Ending, Key, FileObject, ProcessId);

    FsRtlReleaseLockQueue(LockQueue, OldIrql);

    return Status;
}


BOOLEAN
FsRtlFastCheckLockForWrite (
    IN PFILE_LOCK FileLock,
    IN PLARGE_INTEGER StartingByte,
    IN PLARGE_INTEGER Length,
    IN ULONG Key,
    IN PVOID FileObject,
    IN PVOID ProcessId
    )

/*++

Routine Description:

    This routine checks to see if the caller has write access to the
    indicated range due to file locks

Arguments:

    FileLock - Supplies the File Lock to check

    StartingByte - Supplies the first byte (zero based) to check

    Length - Supplies the length, in bytes, to check

    Key - Supplies the to use in the check

    FileObject - Supplies the file object to use in the check

    ProcessId - Supplies the Process Id to use in the check

Return Value:

    BOOLEAN - TRUE if the indicated user/request has write access to the
        entire specified byte range, and FALSE otherwise

--*/

{
    LARGE_INTEGER Starting;
    LARGE_INTEGER Ending;

    PLOCK_INFO              LockInfo;
    PLOCK_QUEUE             LockQueue;
    KIRQL                   OldIrql;
    PFILE_LOCK_INFO         LastLock;
    BOOLEAN                 Status;

    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {

        //
        // No lock information on this FileLock
        //

        DebugTrace(0, Dbg, "FsRtlFastCheckLockForRead, No lock info\n", 0);
        return TRUE;
    }

    //
    //  If there isn't a lock then we can immediately grant access
    //

    if (LockInfo->LockQueue.SharedLockTree == NULL && LockInfo->LockQueue.ExclusiveLockTree == NULL) {

        DebugTrace(0, Dbg, "FsRtlFastCheckLockForWrite, No locks present\n", 0);
        return TRUE;
    }

    //
    //  If length is zero then automatically grant access
    //

    if ((ULONGLONG)Length->QuadPart == 0) {

        DebugTrace(0, Dbg, "FsRtlFastCheckLockForWrite, Length == 0\n", 0);
        return TRUE;
    }

    //
    //  Get our starting and ending byte position
    //

    Starting = *StartingByte;
    (ULONGLONG)Ending.QuadPart = (ULONGLONG)Starting.QuadPart + (ULONGLONG)Length->QuadPart - 1;

    //
    //  Now check lock queue
    //

    LockQueue = &LockInfo->LockQueue;

    //
    //  Grab the waiting lock queue spinlock to exclude anyone from messing
    //  with the queue while we're using it
    //

    FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

    //
    //  If the range ends below the lowest existing lock, this write is OK.
    //

    if ( ((ULONGLONG)Ending.QuadPart < (ULONGLONG)LockInfo->LowestLockOffset) ) {

        DebugTrace(0, Dbg, "FsRtlFastCheckLockForWrite (below lowest lock)\n", 0);

        FsRtlReleaseLockQueue(LockQueue, OldIrql);
        return TRUE;
    }

    //
    //  If the caller just locked this range exclusively, he can write it.
    //

    LastLock = (PFILE_LOCK_INFO)((PFILE_OBJECT)FileObject)->LastLock;
    if ((LastLock != NULL) &&
        ((ULONGLONG)Starting.QuadPart >= (ULONGLONG)LastLock->StartingByte.QuadPart) &&
        ((ULONGLONG)Ending.QuadPart <= (ULONGLONG)LastLock->EndingByte.QuadPart) &&
        (LastLock->Key == Key) &&
        (LastLock->ProcessId == ProcessId) &&
        LastLock->ExclusiveLock) {

        FsRtlReleaseLockQueue(LockQueue, OldIrql);
        return TRUE;
    }

    //
    //  Check the shared locks for overlap. Any overlap in the shared locks is fatal.
    //

    Status = FsRtlCheckNoSharedConflict(LockQueue, &Starting, &Ending);

    if (Status == TRUE) {

        //
        //  No overlap in the shared locks, so check the exclusive locks for overlap.
        //

        Status = FsRtlCheckNoExclusiveConflict(LockQueue, &Starting, &Ending, Key, FileObject, ProcessId);
    }

    FsRtlReleaseLockQueue(LockQueue, OldIrql);

    return Status;
}


VOID
FsRtlSplitLocks (
    IN PLOCKTREE_NODE ParentNode,
    IN PSINGLE_LIST_ENTRY *pStartLink OPTIONAL,
    IN PLARGE_INTEGER LastShadowedByte OPTIONAL,
    IN PLARGE_INTEGER GlueOffset OPTIONAL
    )
/*++

Routine Description:

    This routine examines and possibly splits off shared locks associated
    with a node into new nodes of the lock tree. Called from routines that
    have just deleted locks.

    The arguments that supply the initial conditions for the operation are
    optional if the node is known to be holey.

Arguments:

    ParentNode- Supplies the node the locks are coming from

    pStartLink - Supplies the pointer to the link address of the start of the
        range of locks in the ParentNode's locklist that need to be checked

    LastShadowedByte - Supplies the last byte offset that needs to be checked

    GlueOffset - Supplies the maximum offset affected by locks prior to this
        point in the list

Return Value:

    BOOLEAN - True if the split was successful, False otherwise.  The node will
        be marked as Holey if the split could not occur.

--*/
{
    PSH_LOCK                Lock;
    PLOCKTREE_NODE          NewNode;
    PSINGLE_LIST_ENTRY      Link, *pLink, *NextpLink;
    LARGE_INTEGER           MaxOffset, StartOffset, HaltOffset;

    BOOLEAN                 ExtentValid;
    BOOLEAN                 FailedHoleySplit = FALSE;

    //
    //  There are two cases: the node is holey or not.  If the node is holey, at some
    //  point we failed to get resources to complete a split, so despite our caller's
    //  good intentions we need to go over the entire node.
    //

    if (ParentNode->HoleyNode) {

        //
        //  Just move the starting link back to the front.  The maximum offset and
        //  starting offset of the node will be initialized in the loop.  We also turn
        //  off the holey flag, which will be turned on again as appropriate.
        //

        pStartLink = &ParentNode->Locks.Next;
        ParentNode->HoleyNode = FALSE;

        HaltOffset.QuadPart = ParentNode->Extent;

    } else {

        HaltOffset = *LastShadowedByte;
        MaxOffset = *GlueOffset;
        StartOffset.QuadPart = 0;

        if (!ParentNode->Locks.Next ||
            (ULONGLONG)HaltOffset.QuadPart <= (ULONGLONG)MaxOffset.QuadPart) {

            //
            //  The parent node is not there, doesn't have links associated, or the
            //  last possible byte that is affected by the operation our caller made
            //  is interior to the max extent of all locks still in this node - in
            //  which case there is nothing that needs to be done.
            //

            return;
        }
    }

    //
    //  If the extent of the node is past the last byte affected by whatever
    //  operations were done to this node, we can avoid the linear scan of
    //  the list past that last affected byte since we already know the
    //  extent of the entire list! If it is not (note that it would have to
    //  be equal - by defintion) then we need to recalculate the extents of
    //  all nodes we touch in this operation.
    //

    ExtentValid = (ParentNode->Extent > (ULONGLONG)HaltOffset.QuadPart);

    for (pLink = pStartLink;
         (Link = *pLink) != NULL;
         pLink = NextpLink) {

        NextpLink = &Link->Next;

        Lock = CONTAINING_RECORD( Link, SH_LOCK, Link );

        if (ParentNode->Locks.Next == *pLink) {

            //
            //  We're at the first lock in the node, and we know that we're going to leave
            //  at least one lock here. Skip over that lock. We also know that the max
            //  offset must be that locks's ending byte - make sure it is. Note that this
            //  code is *exactly* the same as the update MaxOffset code at the bottom of
            //  the loop.
            //

            MaxOffset.QuadPart = Lock->LockInfo.EndingByte.QuadPart;

            //
            //  Set the starting offset of the node.  This is only an issue for zero length
            //  locks, so that we can figure out what is going on if we split a node and wind
            //  up with some number of "overlapped" zero length locks at the front of the new
            //  node.  We must be able to notice this case, and not think that each needs to
            //  be in a seperate node.
            //

            StartOffset.QuadPart = Lock->LockInfo.StartingByte.QuadPart;

            //
            //  If extents are invalid we also need to set it in case this turns out to
            //  be the only lock at this node.
            //

            if (!ExtentValid) {

                ParentNode->Extent = (ULONGLONG)MaxOffset.QuadPart;
            }

            continue;
        }

        //
        //  If the lock begins at a byte offset greater than the maximum offset seen to this
        //  point, AND this is not a zero length node starting at the beginning of this node,
        //  break the node.  The second half of the test keeps co-incident zero length locks
        //  in the same node. (zero length lock ---> starting = ending + 1).
        //

        if ((ULONGLONG)Lock->LockInfo.StartingByte.QuadPart > (ULONGLONG)MaxOffset.QuadPart &&
            !(Lock->LockInfo.Length.QuadPart == 0 &&
              Lock->LockInfo.StartingByte.QuadPart == StartOffset.QuadPart)) {

            //
            //  Break the node up here
            //

            NewNode = FsRtlAllocateLockTreeNode();

            if (NewNode == NULL) {

                //
                //  If we are out of resources, this node is now holey - we know that the locks at
                //  this node do not completely cover the indicated range.  Keep splitting for two
                //  reasons: more resources may become avaliable, and we must keep updating the
                //  node's extent if it is known to be invalid.
                //

                //
                //  Now if this node was already holey it is not possible to state that, if we
                //  manage to split if further as we keep walking, that the resulting "left" node
                //  is not holey.  See below.
                //

                if (ParentNode->HoleyNode) {

                    FailedHoleySplit = TRUE;
                }

                ParentNode->HoleyNode = TRUE;

            } else {

                //
                //  Initialize the node.
                //

                RtlInitializeSplayLinks(&NewNode->Links);
                NewNode->HoleyNode = FALSE;

                //
                //  Find the spot in the tree to take the new node(s). If the current node has
                //  a free right child, we use it, else find the successor node and use its
                //  left child. One of these cases must be avaliable since we know there are
                //  no nodes between this node and its successor.
                //

                if (RtlRightChild(&ParentNode->Links) == NULL) {

                    RtlInsertAsRightChild(&ParentNode->Links, &NewNode->Links);

                } else {

                    ASSERT(RtlLeftChild(RtlRealSuccessor(&ParentNode->Links)) == NULL);
                    RtlInsertAsLeftChild(RtlRealSuccessor(&ParentNode->Links), &NewNode->Links);
                }

                //
                //  Move the remaining locks over to the new node and fix up extents
                //

                NewNode->Locks.Next = *pLink;
                *pLink = NULL;

                NewNode->Tail.Next = ParentNode->Tail.Next;
                ParentNode->Tail.Next = CONTAINING_RECORD( pLink, SINGLE_LIST_ENTRY, Next );

                //
                //  This will cause us to fall into the first-lock clause above on the next pass
                //

                NextpLink = &NewNode->Locks.Next;

                //
                // The new node's extent is now copied from the parent. The old node's extent must be
                // the maximum offset we have seen to this point.
                //
                // Note that if ExtentValid is true, that must mean that the lock ending at that extent
                // is in the new node since if it was in the old node we wouldn't have been able to split.
                //

                NewNode->Extent = ParentNode->Extent;
                ParentNode->Extent = (ULONGLONG)MaxOffset.QuadPart;

                //
                //  The parent node can no longer be holey if we have not failed a split in this node.
                //

                if (!FailedHoleySplit) {

                    ParentNode->HoleyNode = FALSE;

                } else {

                    //
                    //  So reset the failure flag for the new node.
                    //

                    FailedHoleySplit = FALSE;
                }

                //
                //  Move over to the new node.
                //

                ParentNode = NewNode;

                continue;
            }
        }

        if (ExtentValid &&
            (ULONGLONG)Lock->LockInfo.StartingByte.QuadPart > (ULONGLONG)HaltOffset.QuadPart) {

            //
            //  Our extents are good and this lock is past the shadow, so we can stop
            //

            return;
        }

        if ((ULONGLONG)MaxOffset.QuadPart < (ULONGLONG)Lock->LockInfo.EndingByte.QuadPart) {

            //
            //  Update maximum offset
            //

            MaxOffset.QuadPart = Lock->LockInfo.EndingByte.QuadPart;

            if (!ExtentValid) {

                //
                //  Extents are not good so we must update the extent
                //

                ParentNode->Extent = (ULONGLONG)MaxOffset.QuadPart;
            }
        }
    }

    //
    //  Reached the end of the list, so update the extent (case of all subsequent locks
    //  having been interior to GlueOffset)
    //

    ParentNode->Extent = (ULONGLONG)MaxOffset.QuadPart;

    return;
}


VOID
FsRtlPrivateRemoveLock (
    IN PLOCK_INFO LockInfo,
    IN PFILE_LOCK_INFO FileLockInfo,
    IN BOOLEAN CheckForWaiters
    )

/*++

Routine Description:

    General purpose cleanup routine.  Finds the given lock structure
    and removes it from the file lock list. Differs from UnlockSingle
    only in that it disables the UnlockRoutine of the FileLock and
    optionalizes walking the waiting locks list.

Arguments:

    FileLock - Supplies the file's lock structure supposedly containing a stale lock

    FileLockInfo - Supplies file lock data being freed

    CheckForWaiters - If true check for possible waiting locks, caused
        by freeing the locked range

Return Value:

    None.

--*/

{
    NTSTATUS Status;

    if (FileLockInfo->ExclusiveLock) {

        //
        //  We must find it in the exclusive lock tree
        //

        Status = FsRtlFastUnlockSingleExclusive( LockInfo,

                                                 FileLockInfo->FileObject,
                                                 &FileLockInfo->StartingByte,
                                                 &FileLockInfo->Length,
                                                 FileLockInfo->ProcessId,
                                                 FileLockInfo->Key,

                                                 NULL,
                                                 TRUE,
                                                 CheckForWaiters );

        ASSERT( Status == STATUS_SUCCESS);

    } else {

        //
        //  We must find it in the shared lock tree
        //

        Status = FsRtlFastUnlockSingleShared( LockInfo,

                                              FileLockInfo->FileObject,
                                              &FileLockInfo->StartingByte,
                                              &FileLockInfo->Length,
                                              FileLockInfo->ProcessId,
                                              FileLockInfo->Key,

                                              NULL,
                                              TRUE,
                                              CheckForWaiters );

        ASSERT( Status == STATUS_SUCCESS);
    }

    return;
}


NTSTATUS
FsRtlFastUnlockSingle (
    IN PFILE_LOCK FileLock,
    IN PFILE_OBJECT FileObject,
    IN LARGE_INTEGER UNALIGNED *FileOffset,
    IN PLARGE_INTEGER Length,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN PVOID Context OPTIONAL,
    IN BOOLEAN AlreadySynchronized
    )

/*++

Routine Description:

    This routine performs an Unlock Single operation on the current locks
    associated with the specified file lock.  Only the lock with a matching
    file object, process id, key, and range is freed.

Arguments:

    FileLock - Supplies the file lock being freed.

    FileObject - Supplies the file object holding the locks

    FileOffset - Supplies the offset to be unlocked

    Length - Supplies the length in bytes to be unlocked

    ProcessId - Supplies the process Id to use in this operation

    Key - Supplies the key to use in this operation

    Context - Optionally supplies context to use when completing Irps

    AlreadySynchronized - Indicates that the caller has already synchronized
        access to the file lock so the fields in the file lock and
        be updated without further locking, but not the queues.

Return Value:

    NTSTATUS - The completion status for this operation

--*/

{
    NTSTATUS Status;

    //
    //  XXX AlreadySynchronized is obsolete. It was apparently added for the dead
    //  XXX SoloLock code.
    //

    if (FileLock->LockInformation == NULL) {

        //
        //  Fast exit - no locks are applied
        //

        return STATUS_RANGE_NOT_LOCKED;
    }

    Status = FsRtlFastUnlockSingleExclusive( FileLock->LockInformation,
                                             FileObject,
                                             FileOffset,
                                             Length,
                                             ProcessId,
                                             Key,
                                             Context,
                                             FALSE,
                                             TRUE );

    if (Status == STATUS_SUCCESS) {

        //
        //  Found and unlocked in the exclusive tree, so we're done
        //

        return Status;
    }

    Status = FsRtlFastUnlockSingleShared( FileLock->LockInformation,
                                          FileObject,
                                          FileOffset,
                                          Length,
                                          ProcessId,
                                          Key,
                                          Context,
                                          FALSE,
                                          TRUE );

    return Status;
}


NTSTATUS
FsRtlFastUnlockSingleShared (
    IN PLOCK_INFO LockInfo,
    IN PFILE_OBJECT FileObject,
    IN LARGE_INTEGER UNALIGNED *FileOffset,
    IN PLARGE_INTEGER Length,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN PVOID Context OPTIONAL,
    IN BOOLEAN IgnoreUnlockRoutine,
    IN BOOLEAN CheckForWaiters
    )

/*++

Routine Description:

    This routine performs an Unlock Single operation on the current locks
    associated with the specified file lock.  Only the lock with a matching
    file object, process id, key, and range is freed.

Arguments:

    LockInfo - Supplies the lock data being operated on

    FileObject - Supplies the file object holding the locks

    FileOffset - Supplies the offset to be unlocked

    Length - Supplies the length in bytes to be unlocked

    ProcessId - Supplies the process Id to use in this operation

    Key - Supplies the key to use in this operation

    Context - Optionally supplies context to use when completing Irps

    IgnoreUnlockRoutine - inidicates that the filelock's unlock routine
        should not be called on lock removal (for removal of aborted
        locks)

    CheckForWaiters - If true check for possible waiting locks, caused
        by freeing the locked range

Return Value:

    NTSTATUS - The completion status for this operation

--*/

{
    PSINGLE_LIST_ENTRY      *pLink, Link;
    KIRQL                   OldIrql;

    PLOCK_QUEUE             LockQueue;
    PRTL_SPLAY_LINKS        SplayLinks;
    LARGE_INTEGER           EndingOffset, MaxOffset;
    PLOCKTREE_NODE          Node;
    LARGE_INTEGER           AlignedFileOffset;

    //
    //  General case - search the outstanding lock queue for this lock
    //

    AlignedFileOffset = *FileOffset;

    LockQueue = &LockInfo->LockQueue;

    FsRtlAcquireLockQueue(LockQueue, &OldIrql);

    //
    //  Check for the no locks currently held
    //

    if (LockQueue->SharedLockTree == NULL) {

        FsRtlReleaseLockQueue( LockQueue, OldIrql );

        return STATUS_RANGE_NOT_LOCKED;
    }

    //
    //  Find the overlapping node, if it exists, to search. Note that
    //  we don't have to go through more than one node in the tree
    //  since we are assuming this is an existing lock.
    //

    EndingOffset.QuadPart = (ULONGLONG)AlignedFileOffset.QuadPart + (ULONGLONG)Length->QuadPart - 1;

    SplayLinks = FsRtlFindFirstOverlappingSharedNode( LockQueue->SharedLockTree,
                                                      &AlignedFileOffset,
                                                      &EndingOffset,
                                                      NULL,
                                                      NULL );

    if (SplayLinks == NULL) {

        //
        //  No node in the tree overlaps this range, so we're done
        //

        FsRtlReleaseLockQueue(LockQueue, OldIrql);

        return STATUS_RANGE_NOT_LOCKED;
    }

    Node = CONTAINING_RECORD( SplayLinks, LOCKTREE_NODE, Links );
    MaxOffset.QuadPart = 0;

    for (pLink = &Node->Locks.Next;
         (Link = *pLink) != NULL;
         pLink = &Link->Next) {

        PSH_LOCK Lock;

        Lock = CONTAINING_RECORD( Link, SH_LOCK, Link );

        DebugTrace(0, Dbg, "Sh Top of Loop, Lock = %08lx\n", Lock );

        if ((Lock->LockInfo.FileObject == FileObject) &&
            (Lock->LockInfo.ProcessId == ProcessId) &&
            (Lock->LockInfo.Key == Key) &&
            ((ULONGLONG)Lock->LockInfo.StartingByte.QuadPart == (ULONGLONG)AlignedFileOffset.QuadPart) &&
            ((ULONGLONG)Lock->LockInfo.Length.QuadPart == (ULONGLONG)Length->QuadPart)) {

            DebugTrace(0, Dbg, "Sh Found one to unlock\n", 0);

            //
            //  We have an exact match so now is the time to delete this
            //  lock.  Remove the lock from the list, then call the
            //  optional unlock routine, then delete the lock.
            //

            if (FileObject->LastLock == &Lock->LockInfo) {

                FileObject->LastLock = NULL;
            }

            if (*pLink == Node->Tail.Next) {

                //
                //  Deleting the tail node of the list. Safe even if deleting the
                //  first node since this implies we're also deleting the last node
                //  in the node which means we'll delete the node ...
                //

                Node->Tail.Next = CONTAINING_RECORD( pLink, SINGLE_LIST_ENTRY, Next );
            }

            //
            //  Snip the deleted lock
            //

            *pLink = Link->Next;

            if (pLink == &Node->Locks.Next) {

                //
                //  Deleted first lock in node
                //

                if (Node->Locks.Next == NULL) {

                    //
                    // Just deleted last lock on this node, so free it
                    //

                    LockQueue->SharedLockTree = RtlDelete(SplayLinks);

                    FsRtlFreeLockTreeNode(Node);

                    Node = NULL;
                }

                if (LockInfo->LowestLockOffset != 0xffffffff &&
                    LockInfo->LowestLockOffset == Lock->LockInfo.StartingByte.LowPart) {

                    //
                    //  This was the lowest lock in the trees, reset the lowest lock offset
                    //

                    FsRtlPrivateResetLowestLockOffset(LockInfo);
                }
            }

            //
            //  Now the fun begins. It may be the case that the lock just snipped from
            //  the chain was gluing locks at this node together, so we need to
            //  inspect the chain.
            //

            if (Node) {

                FsRtlSplitLocks(Node, pLink, &Lock->LockInfo.EndingByte, &MaxOffset);
            }

            if (!IgnoreUnlockRoutine && LockInfo->UnlockRoutine != NULL) {

                FsRtlReleaseLockQueue( LockQueue, OldIrql );

                LockInfo->UnlockRoutine( Context, &Lock->LockInfo );

                FsRtlReacquireLockQueue( LockInfo, LockQueue, &OldIrql );

            }

            FsRtlFreeSharedLock( Lock );

            //
            //  See if there are additional waiting locks that we can
            //  now release.
            //

            if (CheckForWaiters && LockQueue->WaitingLocks.Next) {

                FsRtlPrivateCheckWaitingLocks( LockInfo, LockQueue, OldIrql );
            }

            FsRtlReleaseLockQueue( LockQueue, OldIrql );

            return STATUS_SUCCESS;
        }

        if ((ULONGLONG)Lock->LockInfo.StartingByte.QuadPart > (ULONGLONG)AlignedFileOffset.QuadPart) {

            //
            //  The current lock begins at a byte offset greater than the range we are seeking
            //  to unlock. This range must therefore not be locked.
            //

            break;
        }

        if ((ULONGLONG)MaxOffset.QuadPart < (ULONGLONG)Lock->LockInfo.EndingByte.QuadPart) {

            //
            // Maintain the maximum offset affected by locks up to this point.
            //

            MaxOffset.QuadPart = Lock->LockInfo.EndingByte.QuadPart;
        }
    }

    //
    //  Lock was not found, return to our caller
    //

    FsRtlReleaseLockQueue(LockQueue, OldIrql);
    return STATUS_RANGE_NOT_LOCKED;
}


NTSTATUS
FsRtlFastUnlockSingleExclusive (
    IN PLOCK_INFO LockInfo,
    IN PFILE_OBJECT FileObject,
    IN LARGE_INTEGER UNALIGNED *FileOffset,
    IN PLARGE_INTEGER Length,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN PVOID Context OPTIONAL,
    IN BOOLEAN IgnoreUnlockRoutine,
    IN BOOLEAN CheckForWaiters
    )

/*++

Routine Description:

    This routine performs an Unlock Single operation on the exclusive locks
    associated with the specified lock data.  Only the lock with a matching
    file object, process id, key, and range is freed.

Arguments:

    LockInfo - Supplies the lock data being operated on

    FileObject - Supplies the file object holding the locks

    FileOffset - Supplies the offset to be unlocked

    Length - Supplies the length in bytes to be unlocked

    ProcessId - Supplies the process Id to use in this operation

    Key - Supplies the key to use in this operation

    Context - Optionally supplies context to use when completing Irps

    IgnoreUnlockRoutine - inidicates that the filelock's unlock routine
        should not be called on lock removal (for removal of aborted
        locks)

    CheckForWaiters - If true check for possible waiting locks, caused
        by freeing the locked range

Return Value:

    NTSTATUS - The completion status for this operation

--*/

{
    KIRQL                   OldIrql;
    PLOCK_QUEUE             LockQueue;
    PRTL_SPLAY_LINKS        SplayLinks;
    LARGE_INTEGER           EndingOffset;
    PEX_LOCK                Lock;
    LARGE_INTEGER           AlignedFileOffset;

    //
    //  General case - search the outstanding lock queue for this lock
    //

    AlignedFileOffset = *FileOffset;

    LockQueue = &LockInfo->LockQueue;

    FsRtlAcquireLockQueue(LockQueue, &OldIrql);

    //
    //  Check for the no locks currently held
    //

    if (LockQueue->ExclusiveLockTree == NULL) {

        FsRtlReleaseLockQueue( LockQueue, OldIrql );

        return STATUS_RANGE_NOT_LOCKED;
    }

    //
    //  Find the overlapping lock, if it exists. Note that this is usually
    //  the only lock we need to check since we are assuming this is an
    //  existing lock. However, if the lock is a zero length lock we will
    //  have a run of locks to check.
    //

    EndingOffset.QuadPart = (ULONGLONG)AlignedFileOffset.QuadPart + (ULONGLONG)Length->QuadPart - 1;

    for (SplayLinks = FsRtlFindFirstOverlappingExclusiveNode( LockQueue->ExclusiveLockTree,
                                                              &AlignedFileOffset,
                                                              &EndingOffset,
                                                              NULL,
                                                              NULL );
         SplayLinks;
         SplayLinks = RtlRealSuccessor(SplayLinks)) {

        Lock = CONTAINING_RECORD( SplayLinks, EX_LOCK, Links );

        if ((Lock->LockInfo.FileObject == FileObject) &&
            (Lock->LockInfo.ProcessId == ProcessId) &&
            (Lock->LockInfo.Key == Key) &&
            ((ULONGLONG)Lock->LockInfo.StartingByte.QuadPart == (ULONGLONG)AlignedFileOffset.QuadPart) &&
            ((ULONGLONG)Lock->LockInfo.Length.QuadPart == (ULONGLONG)Length->QuadPart)) {

            DebugTrace(0, Dbg, "Ex Found one to unlock\n", 0);

            //
            //  We have an exact match so now is the time to delete this
            //  lock.  Remove the lock from the list, then call the
            //  optional unlock routine, then delete the lock.
            //

            if (FileObject->LastLock == &Lock->LockInfo) {

                FileObject->LastLock = NULL;
            }

            //
            //  Snip the deleted lock
            //

            LockQueue->ExclusiveLockTree = RtlDelete(&Lock->Links);

            if (LockInfo->LowestLockOffset != 0xffffffff &&
                LockInfo->LowestLockOffset == Lock->LockInfo.StartingByte.LowPart) {

                //
                //  This was the lowest lock in the tree, so reset the lowest lock
                //  offset
                //

                FsRtlPrivateResetLowestLockOffset(LockInfo);
            }

            if (!IgnoreUnlockRoutine && LockInfo->UnlockRoutine != NULL) {

                FsRtlReleaseLockQueue( LockQueue, OldIrql );

                LockInfo->UnlockRoutine( Context, &Lock->LockInfo );

                FsRtlReacquireLockQueue( LockInfo, LockQueue, &OldIrql );

            }

            FsRtlFreeExclusiveLock( Lock );

            //
            //  See if there are additional waiting locks that we can
            //  now release.
            //

            if (CheckForWaiters && LockQueue->WaitingLocks.Next) {

                FsRtlPrivateCheckWaitingLocks( LockInfo, LockQueue, OldIrql );
            }

            FsRtlReleaseLockQueue( LockQueue, OldIrql );

            return STATUS_SUCCESS;
        }

        if ((ULONGLONG)Lock->LockInfo.StartingByte.QuadPart > (ULONGLONG)AlignedFileOffset.QuadPart) {

            //
            //  The current lock begins at a byte offset greater than the range we are seeking
            //  to unlock. This range must therefore not be locked.
            //

            break;
        }
    }

    //
    //  Lock was not found, return to our caller
    //

    FsRtlReleaseLockQueue(LockQueue, OldIrql);
    return STATUS_RANGE_NOT_LOCKED;
}


NTSTATUS
FsRtlFastUnlockAll (
    IN PFILE_LOCK FileLock,
    IN PFILE_OBJECT FileObject,
    IN PEPROCESS ProcessId,
    IN PVOID Context OPTIONAL
    )

/*++

Routine Description:

    This routine performs an Unlock all operation on the current locks
    associated with the specified file lock.  Only those locks with
    a matching file object and process id are freed.

Arguments:

    FileLock - Supplies the file lock being freed.

    FileObject - Supplies the file object associated with the file lock

    ProcessId - Supplies the Process Id assoicated with the locks to be
        freed

    Context - Supplies an optional context to use when completing waiting
        lock irps.

Return Value:

    None

--*/

{
    return FsRtlPrivateFastUnlockAll(
                FileLock,
                FileObject,
                ProcessId,
                0, FALSE,           // No Key
                Context );
}


NTSTATUS
FsRtlFastUnlockAllByKey (
    IN PFILE_LOCK FileLock,
    IN PFILE_OBJECT FileObject,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN PVOID Context OPTIONAL
    )

/*++

Routine Description:

    This routine performs an Unlock All by Key operation on the current locks
    associated with the specified file lock.  Only those locks with
    a matching file object, process id, and key are freed.  The input Irp
    is completed by this procedure

Arguments:

    FileLock - Supplies the file lock being freed.

    FileObject - Supplies the file object associated with the file lock

    ProcessId - Supplies the Process Id assoicated with the locks to be
        freed

    Key - Supplies the Key to use in this operation

    Context - Supplies an optional context to use when completing waiting
        lock irps.

Return Value:

    NTSTATUS - The return status for the operation.

--*/

{
    return FsRtlPrivateFastUnlockAll(
                FileLock,
                FileObject,
                ProcessId,
                Key, TRUE,
                Context );

}


//
//  Local Support Routine
//

BOOLEAN
FsRtlPrivateLock (
    IN PFILE_LOCK FileLock,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN BOOLEAN FailImmediately,
    IN BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK Iosb,
    IN PIRP Irp OPTIONAL,
    IN PVOID Context,
    IN BOOLEAN AlreadySynchronized
    )

/*++

Routine Description:

    This routine preforms a lock operation request.  This handles both the fast
    get lock and the Irp based get lock.  If the Irp is supplied then
    this routine will either complete the Irp or enqueue it as a waiting
    lock request.

Arguments:

    FileLock - Supplies the File Lock to work against

    FileObject - Supplies the file object used in this operation

    FileOffset - Supplies the file offset used in this operation

    Length - Supplies the length used in this operation

    ProcessId - Supplies the process ID used in this operation

    Key - Supplies the key used in this operation

    FailImmediately - Indicates if the request should fail immediately
        if the lock cannot be granted.

    ExclusiveLock - Indicates if this is a request for an exclusive or
        shared lock

    Iosb - Receives the Status if this operation is successful

    Context - Supplies the context with which to complete Irp with

    AlreadySynchronized - Indicates that the caller has already synchronized
        access to the file lock so the fields in the file lock and
        be updated without further locking, but not the queues.

Return Value:

    BOOLEAN - TRUE if this operation completed and FALSE otherwise.

--*/

{
    BOOLEAN Results;
    BOOLEAN AccessGranted;
    BOOLEAN ViaFastCall;
    BOOLEAN ReleaseQueue = FALSE;

    PLOCK_INFO  LockInfo;
    PLOCK_QUEUE LockQueue;
    KIRQL       OldIrql;
    FILE_LOCK_INFO FileLockInfo;

    DebugTrace(+1, Dbg, "FsRtlPrivateLock, FileLock = %08lx\n", FileLock);

    //
    //  If the irp is null then this is being called via the fast call method.
    //

    ViaFastCall = (BOOLEAN) !ARGUMENT_PRESENT( Irp );

    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {
        DebugTrace(+2, Dbg, "FsRtlPrivateLock, New LockInfo required\n", 0);

        //
        // No lock information on this FileLock, create the structure.
        //
        //

        if (!FsRtlPrivateInitializeFileLock (FileLock, ViaFastCall)) {

            return FALSE;
        }

        //
        // Set flag so file locks will be checked on the fast io
        // code paths
        //

        FileLock->FastIoIsQuestionable = TRUE;

        //
        // Pickup allocated lockinfo structure
        //

        LockInfo = (PLOCK_INFO) FileLock->LockInformation;
    }

    //
    // Assume success and build LockData structure prior to acquiring
    // the lock queue spinlock.  (mp perf enhancement)
    //

    FileLockInfo.StartingByte = *FileOffset;
    FileLockInfo.Length = *Length;
    (ULONGLONG)FileLockInfo.EndingByte.QuadPart =
            (ULONGLONG)FileLockInfo.StartingByte.QuadPart + (ULONGLONG)FileLockInfo.Length.QuadPart - 1;

    FileLockInfo.Key = Key;
    FileLockInfo.FileObject = FileObject;
    FileLockInfo.ProcessId = ProcessId;
    FileLockInfo.ExclusiveLock = ExclusiveLock;

    LockQueue = &LockInfo->LockQueue;

    //
    //  Now we need to actually run through our current lock queue.
    //

    FsRtlAcquireLockQueue(LockQueue, &OldIrql);
    ReleaseQueue = TRUE;

    try {

        //
        //  Case on whether we're trying to take out an exclusive lock or
        //  a shared lock.  And in both cases try to get appropriate access.
        //

        if (ExclusiveLock) {

            DebugTrace(0, Dbg, "Check for write access\n", 0);

            AccessGranted = FsRtlPrivateCheckForExclusiveLockAccess(
                                LockQueue,
                                &FileLockInfo );

        } else {

            DebugTrace(0, Dbg, "Check for read access\n", 0);

            AccessGranted = FsRtlPrivateCheckForSharedLockAccess(
                                LockQueue,
                                &FileLockInfo );
        }

        //
        //  Now AccessGranted tells us whether we can really get the access
        //  for the range we want
        //

        if (!AccessGranted) {

            DebugTrace(0, Dbg, "We do not have access\n", 0);

            //
            //  We cannot read/write to the range, so we cannot take out
            //  the lock.  Now if the user wanted to fail immediately then
            //  we'll complete the Irp, otherwise we'll enqueue this Irp
            //  to the waiting lock queue
            //

            if (FailImmediately) {

                //
                //  Set our status and return, the finally clause will
                //  complete the request
                //

                DebugTrace(0, Dbg, "And we fail immediately\n", 0);

                Iosb->Status = STATUS_LOCK_NOT_GRANTED;
                try_return( Results = TRUE );

            } else if (ARGUMENT_PRESENT(Irp)) {

                PWAITING_LOCK WaitingLock;

                DebugTrace(0, Dbg, "And we enqueue the Irp for later\n", 0);

                //
                //  Allocate a new waiting record, set it to point to the
                //  waiting Irp, and insert it in the tail of the waiting
                //  locks queue
                //

                WaitingLock = FsRtlAllocateWaitingLock();

                //
                //  Simply raise out if we can't allocate.
                //

                if (WaitingLock == NULL) {

                    ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                }

                WaitingLock->Irp = Irp;
                WaitingLock->Context = Context;
                IoMarkIrpPending( Irp );

                //
                // Add WaitingLock WaitingLockQueue
                //

                WaitingLock->Link.Next = NULL;
                if (LockQueue->WaitingLocks.Next == NULL) {

                    //
                    // Create new list
                    //

                    LockQueue->WaitingLocks.Next = &WaitingLock->Link;
                    LockQueue->WaitingLocksTail.Next = &WaitingLock->Link;

                } else {

                    //
                    // Add waiter to tail of list
                    //

                    LockQueue->WaitingLocksTail.Next->Next = &WaitingLock->Link;
                    LockQueue->WaitingLocksTail.Next = &WaitingLock->Link;
                }


                //
                //  Setup IRP in case it's canceled - then set the
                //  IRP's cancel routine
                //

                Irp->IoStatus.Information = (ULONG_PTR)LockInfo;
                IoSetCancelRoutine( Irp, FsRtlPrivateCancelFileLockIrp );

                if (Irp->Cancel) {

                    //
                    // Pull the cancel routine off of the IRP - if it is not
                    // NULL, this means we won the race with IoCancelIrp and
                    // will be responsible for cancelling the IRP synchronously.
                    // If NULL, we lost and our cancel routine is already being
                    // called for us.
                    //
                    // This must be done while holding the lock queue down since
                    // this is how we synchronize with the cancel.
                    //

                    if (IoSetCancelRoutine( Irp, NULL )) {

                        //
                        // Irp's cancel routine was not called, do it ourselves.
                        // Indicate to the cancel routine that he does not need
                        // to release the cancel spinlock by passing a NULL DO.
                        //
                        // The queue will be dropped in order to complete the Irp.
                        // We communicate the previous IRQL through the Irp itself.
                        //

                        Irp->CancelIrql = OldIrql;
                        FsRtlPrivateCancelFileLockIrp( NULL, Irp );
                        ReleaseQueue = FALSE;
                    }
                }

                Iosb->Status = STATUS_PENDING;
                try_return( Results = TRUE );

            } else {

                try_return( Results = FALSE );
            }
        }

        DebugTrace(0, Dbg, "We have access\n", 0);

        if (!FsRtlPrivateInsertLock( LockInfo, FileObject, &FileLockInfo )) {

            //
            //  Resource exhaustion will cause us to fail here.  Via the fast call, indicate
            //  that it may be worthwhile to go around again via the Irp based path.  If we
            //  are already there, simply raise out.
            //

            if (ViaFastCall) {

                try_return( Results = FALSE );

            } else {

                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

        } else {

            Iosb->Status = STATUS_SUCCESS;
        }

        //
        //  At long last, we're done.
        //

        Results = TRUE;

    try_exit: NOTHING;
    } finally {

        if (ReleaseQueue) {
            
            FsRtlReleaseLockQueue(LockQueue, OldIrql);
        }

        //
        //  Complete the request provided we were given one and it is not a pending status
        //

        if (!AbnormalTermination() && ARGUMENT_PRESENT(Irp) && (Iosb->Status != STATUS_PENDING)) {

            NTSTATUS NewStatus;

            //
            //  We must reference the fileobject for the case that the IRP completion
            //  fails and we need to lift the lock.  Although the only reason we have
            //  to touch the fileobject in the remove case is to unset the LastLock field,
            //  we have no way of knowing if we will race with a reference count drop
            //  and lose.
            //

            ObReferenceObject( FileObject );

            //
            //  Complete the request, if the don't get back success then
            //  we need to possibly remove the lock that we just
            //  inserted.
            //

            FsRtlCompleteLockIrp(
                LockInfo,
                Context,
                Irp,
                Iosb->Status,
                &NewStatus,
                FileObject );

            if (!NT_SUCCESS(NewStatus) && NT_SUCCESS(Iosb->Status) ) {

                //
                // Irp failed, remove the lock which was added
                //

                FsRtlPrivateRemoveLock (
                    LockInfo,
                    &FileLockInfo,
                    TRUE );
            }

            //
            //  Lift our private reference to the fileobject. This may induce deletion.
            //

            ObDereferenceObject( FileObject );

            Iosb->Status = NewStatus;
        }

        DebugTrace(-1, Dbg, "FsRtlPrivateLock -> %08lx\n", Results);
    }

    //
    //  and return to our caller
    //

    return Results;
}


//
//  Internal Support Routine
//

BOOLEAN
FsRtlPrivateInsertLock (
    IN PLOCK_INFO LockInfo,
    IN PFILE_OBJECT FileObject,
    IN PFILE_LOCK_INFO FileLockInfo
    )

/*++

Routine Description:

    This routine fills in a new lock record of the appropriate type and inserts
    it into the lock information.

Arguments:

    LockInfo - Supplies the lock being modified

    FileObject - The associated file object to update hints in

    FileLockInfo - Supplies the new lock data to add to the lock queue

Return Value:

    BOOLEAN - True if the insert was successful, False if no resources were avaliable
        to complete the operation.

--*/

{
    //
    //  Now add the lock to the appropriate tree.
    //

    if (FileLockInfo->ExclusiveLock) {

        PEX_LOCK ExLock;

        ExLock = FsRtlAllocateExclusiveLock();

        if (ExLock == NULL) {

            return FALSE;
        }

        ExLock->LockInfo = *FileLockInfo;

        FsRtlPrivateInsertExclusiveLock( &LockInfo->LockQueue, ExLock );

        FileObject->LastLock = &ExLock->LockInfo;

    } else {

        PSH_LOCK ShLock;

        ShLock = FsRtlAllocateSharedLock();

        if (ShLock == NULL) {

            return FALSE;
        }

        ShLock->LockInfo = *FileLockInfo;

        if (!FsRtlPrivateInsertSharedLock( &LockInfo->LockQueue, ShLock )) {

            return FALSE;
        }

        FileObject->LastLock = &ShLock->LockInfo;
    }

    //
    //  Fix up the lowest lock offset if need be
    //

    if ((ULONGLONG)FileLockInfo->StartingByte.QuadPart < (ULONGLONG)LockInfo->LowestLockOffset) {

        ASSERT( FileLockInfo->StartingByte.HighPart == 0 );
        LockInfo->LowestLockOffset = FileLockInfo->StartingByte.LowPart;
    }

    return TRUE;
}


//
//  Internal Support Routine
//

BOOLEAN
FsRtlPrivateInsertSharedLock (
    IN PLOCK_QUEUE LockQueue,
    IN PSH_LOCK NewLock
    )

/*++

Routine Description:

    This routine adds a new shared lock record to the File lock's current
    lock queue. Locks are inserted into nodes ordered by their starting byte.

Arguments:

    LockQueue - Supplies the lock queue being modified

    NewLock - Supplies the new shared lock to add to the lock queue

Return Value:

    BOOLEAN - True if the insert was successful, False if no resources were avaliable
        to complete the operation.

--*/
{
    PSINGLE_LIST_ENTRY pLink, Link;
    PRTL_SPLAY_LINKS OverlappedSplayLinks, ParentSplayLinks;
    PLOCKTREE_NODE Node, NextNode;
    PSH_LOCK NextLock;
    BOOLEAN GreaterThan;

    OverlappedSplayLinks = FsRtlFindFirstOverlappingSharedNode( LockQueue->SharedLockTree,
                                                                &NewLock->LockInfo.StartingByte,
                                                                &NewLock->LockInfo.EndingByte,
                                                                &ParentSplayLinks,
                                                                &GreaterThan );

    if (OverlappedSplayLinks == NULL) {

        //
        //  Simple insert case, build a new node
        //

        NextNode = FsRtlAllocateLockTreeNode();

        //
        //  If no resources are avaliable, simply fail now.
        //

        if (NextNode == NULL) {

            return FALSE;
        }

        RtlInitializeSplayLinks(&NextNode->Links);
        NextNode->HoleyNode = FALSE;

        NextNode->Locks.Next = NextNode->Tail.Next = &NewLock->Link;
        NextNode->Extent = (ULONGLONG)NewLock->LockInfo.EndingByte.QuadPart;
        NewLock->Link.Next = NULL;

        if (ParentSplayLinks) {

            //
            //  We have a real parent node in the tree
            //

            if (GreaterThan) {

                ASSERT(RtlLeftChild(ParentSplayLinks) == NULL);
                RtlInsertAsLeftChild(ParentSplayLinks, &NextNode->Links);

            } else {

                ASSERT(RtlRightChild(ParentSplayLinks) == NULL);
                RtlInsertAsRightChild(ParentSplayLinks, &NextNode->Links);
            }

            //
            //  Splay all new nodes in the tree
            //

            LockQueue->SharedLockTree = RtlSplay(&NextNode->Links);

        } else {

            //
            //  First node in the tree
            //

            LockQueue->SharedLockTree = &NextNode->Links;
        }

        return TRUE;
    }

    //
    //  Now we examine the node to see if it is holey as a result of a resource-failed split.
    //  If it is, we must complete the split before adding the new lock.
    //

    Node = CONTAINING_RECORD( OverlappedSplayLinks, LOCKTREE_NODE, Links );

    //
    //  Search down the overlapped node finding the position for the new lock
    //

    for (pLink = &Node->Locks;
         (Link = pLink->Next) != NULL;
         pLink = Link) {

        PSH_LOCK Lock;

        Lock = CONTAINING_RECORD( Link, SH_LOCK, Link );

        //
        //  We sort locks on this list first by starting byte, then by whether the length is zero or not.
        //  This is important so that zero length locks appear prior to non-zero length locks, so that
        //  they are split out of nodes into the tree in the correct order.
        //
        //  if (NewLock->StartingByte <= Lock->StartingByte) ...
        //

        if (((ULONGLONG)NewLock->LockInfo.StartingByte.QuadPart < (ULONGLONG)Lock->LockInfo.StartingByte.QuadPart) ||

            ((ULONGLONG)NewLock->LockInfo.StartingByte.QuadPart == (ULONGLONG)Lock->LockInfo.StartingByte.QuadPart &&
             (NewLock->LockInfo.Length.QuadPart == 0 || Lock->LockInfo.Length.QuadPart != 0))) {

            break;
        }
    }

    //
    //  At this point pLink points to the record that comes right after
    //  the new lock that we're inserting so we can simply push the
    //  newlock into the entrylist
    //

    DebugTrace(0, Dbg, "InsertSharedLock, Insert Before = %08lx\n", Link);

    if (pLink->Next == NULL) {

        //
        //    Adding onto the tail of the list
        //

        Node->Tail.Next = &NewLock->Link;
    }

    NewLock->Link.Next = pLink->Next;
    pLink->Next = &NewLock->Link;

    //
    //  And splay the node we inserted into
    //

    LockQueue->SharedLockTree = RtlSplay(OverlappedSplayLinks);

    if ((ULONGLONG)NewLock->LockInfo.EndingByte.QuadPart > Node->Extent) {

        //
        //  The new lock extends the range of this node, so fix up the extent
        //

        Node->Extent = NewLock->LockInfo.EndingByte.QuadPart;

        //
        //  Walk across the remainder of the tree integrating newly overlapping
        //  nodes into the node we just inserted the new lock into.  Note that
        //  this isn't so much a walk as a repeated examination of our successor's
        //  until one does not overlap (or we hit the end).
        //

        ParentSplayLinks = OverlappedSplayLinks;

        for (OverlappedSplayLinks = RtlRealSuccessor(ParentSplayLinks);
             OverlappedSplayLinks;
             OverlappedSplayLinks = RtlRealSuccessor(ParentSplayLinks)) {

            NextNode = CONTAINING_RECORD( OverlappedSplayLinks, LOCKTREE_NODE, Links );
            NextLock = CONTAINING_RECORD( NextNode->Locks.Next, SH_LOCK, Link );

            if ((ULONGLONG)NextLock->LockInfo.StartingByte.QuadPart > Node->Extent) {

                //
                //  This node is not overlapped, so stop
                //

                break;
            }

            //
            //  If we are intergrating a holey node into a non-holey node, try to split
            //  the node first.  It will be better to get this done with a smaller node
            //  than a big, fully integrated one.  Note that we are guaranteed that the
            //  node will remain a candidate for integration since the first lock on the
            //  node will still be there, and overlaps.
            //

            if (!Node->HoleyNode && NextNode->HoleyNode) {

                FsRtlSplitLocks( NextNode, NULL, NULL, NULL );
            }

            //
            //  Integrate the locks in this node into our list
            //

            Node->Tail.Next->Next = NextNode->Locks.Next;
            Node->Tail.Next = NextNode->Tail.Next;

            if (NextNode->Extent > Node->Extent) {

                //
                //  If the node we just swallowed was (still!) holey, we perhaps made this
                //  node holey too.  The resolution of this is left to the lock split we will
                //  perform after integration is complete.
                //
                //  Note that if the extent of the node we are swallowing is interior
                //  to the current node, we just covered whatever holes it contained.
                //

                if (NextNode->HoleyNode) {

                    Node->HoleyNode = TRUE;
                }

                Node->Extent = NextNode->Extent;
            }

            //
            //  Free the now empty node.
            //

            RtlDeleteNoSplay( OverlappedSplayLinks, &LockQueue->SharedLockTree );
            FsRtlFreeLockTreeNode( NextNode );
        }
    }

    //
    //  Now, perhaps this node is still holey.  For grins lets try one more time to split
    //  this thing apart.
    //

    if (Node->HoleyNode) {

        FsRtlSplitLocks( Node, NULL, NULL, NULL );
    }

    //
    //  And return to our caller
    //

    return TRUE;
}


//
//  Internal Support Routine
//

VOID
FsRtlPrivateInsertExclusiveLock (
    IN PLOCK_QUEUE LockQueue,
    IN PEX_LOCK NewLock
    )

/*++

Routine Description:

    This routine adds a new exclusive lock record to the File lock's current
    lock queue.

Arguments:

    LockQueue - Supplies the lock queue being modified

    NewLock - Supplies the new exclusive lock to add to the lock queue

Return Value:

    None.

--*/

{
    PRTL_SPLAY_LINKS OverlappedSplayLinks, ParentSplayLinks;
    BOOLEAN GreaterThan;

    OverlappedSplayLinks = FsRtlFindFirstOverlappingExclusiveNode( LockQueue->ExclusiveLockTree,
                                                                   &NewLock->LockInfo.StartingByte,
                                                                   &NewLock->LockInfo.EndingByte,
                                                                   &ParentSplayLinks,
                                                                   &GreaterThan );

    //
    //  This is the exclusive tree. Nothing can overlap (caller is supposed to insure this) unless
    //  the lock is a zero length lock, in which case we just insert it - still.
    //

    ASSERT(!OverlappedSplayLinks || NewLock->LockInfo.Length.QuadPart == 0);

    //
    //  Simple insert ...
    //

    RtlInitializeSplayLinks(&NewLock->Links);

    if (OverlappedSplayLinks) {

        //
        //  With zero length locks we have OverlappedSplayLinks at the starting point
        //  of a run of zero length locks, so we have to e flexible about where the new
        //  node is inserted.
        //

        if (RtlRightChild(OverlappedSplayLinks)) {

            //
            //  Right slot taken. We can use the left slot or go to the sucessor's left slot
            //

            if (RtlLeftChild(OverlappedSplayLinks)) {

                ASSERT(RtlLeftChild(RtlRealSuccessor(OverlappedSplayLinks)) == NULL);
                RtlInsertAsLeftChild(RtlRealSuccessor(OverlappedSplayLinks), &NewLock->Links);

            } else {

                RtlInsertAsLeftChild(OverlappedSplayLinks, &NewLock->Links);
            }


        } else {

            RtlInsertAsRightChild(OverlappedSplayLinks, &NewLock->Links);
        }

    } else if (ParentSplayLinks) {

        //
        //  We have a real parent node in the tree, and must be at a leaf since
        //  there was no overlap
        //

        if (GreaterThan) {

            ASSERT(RtlLeftChild(ParentSplayLinks) == NULL);
            RtlInsertAsLeftChild(ParentSplayLinks, &NewLock->Links);

        } else {

            ASSERT(RtlRightChild(ParentSplayLinks) == NULL);
            RtlInsertAsRightChild(ParentSplayLinks, &NewLock->Links);
        }

    } else {

        //
        //  First node in the tree
        //

        LockQueue->ExclusiveLockTree = &NewLock->Links;
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Internal Support Routine
//

VOID
FsRtlPrivateCheckWaitingLocks (
    IN PLOCK_INFO   LockInfo,
    IN PLOCK_QUEUE  LockQueue,
    IN KIRQL        OldIrql
    )

/*++

Routine Description:

    This routine checks to see if any of the current waiting locks are now
    be satisfied, and if so it completes their IRPs.

Arguments:

    LockInfo - LockInfo which LockQueue is member of

    LockQueue - Supplies queue which needs to be checked

    OldIrql - Irql to restore when LockQueue is released

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY *pLink, Link;
    NTSTATUS NewStatus;
    BOOLEAN Result;

    pLink = &LockQueue->WaitingLocks.Next;
    while ((Link = *pLink) != NULL) {

        PWAITING_LOCK WaitingLock;

        PIRP Irp;
        PIO_STACK_LOCATION IrpSp;

        BOOLEAN AccessGranted;

        FILE_LOCK_INFO FileLockInfo;

        //
        //  Get a pointer to the waiting lock record
        //

        WaitingLock = CONTAINING_RECORD( Link, WAITING_LOCK, Link );

        DebugTrace(0, Dbg, "FsRtlCheckWaitingLocks, Loop top, WaitingLock = %08lx\n", WaitingLock);

        //
        //  Get a local copy of the necessary fields we'll need to use
        //

        Irp = WaitingLock->Irp;
        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        FileLockInfo.StartingByte  = IrpSp->Parameters.LockControl.ByteOffset;
        FileLockInfo.Length        = *IrpSp->Parameters.LockControl.Length;
        (ULONGLONG)FileLockInfo.EndingByte.QuadPart =
            (ULONGLONG)FileLockInfo.StartingByte.QuadPart + (ULONGLONG)FileLockInfo.Length.QuadPart - 1;

        FileLockInfo.FileObject    = IrpSp->FileObject;
        FileLockInfo.ProcessId     = IoGetRequestorProcess( Irp );
        FileLockInfo.Key           = IrpSp->Parameters.LockControl.Key;
        FileLockInfo.ExclusiveLock = BooleanFlagOn(IrpSp->Flags, SL_EXCLUSIVE_LOCK);

        //
        //  Now case on whether we're trying to take out an exclusive lock or
        //  a shared lock.  And in both cases try to get the appropriate access
        //  For the exclusive case we send in a NULL file object and process
        //  id, this will ensure that the lookup does not give us write
        //  access through an exclusive lock.
        //

        if (FileLockInfo.ExclusiveLock) {

            DebugTrace(0, Dbg, "FsRtlCheckWaitingLocks do we have write access?\n", 0);

            AccessGranted = FsRtlPrivateCheckForExclusiveLockAccess(
                                LockQueue,
                                &FileLockInfo );
        } else {

            DebugTrace(0, Dbg, "FsRtlCheckWaitingLocks do we have read access?\n", 0);

            AccessGranted = FsRtlPrivateCheckForSharedLockAccess(
                                LockQueue,
                                &FileLockInfo );

        }

        //
        //  Now AccessGranted tells us whether we can really get the access for
        //  the range we want.
        //
        //  No matter what happens, this Irp must be completed now - even if we
        //  are resource starved.  User mode deadlock could be induced since there
        //  may no longer be a pending unlock to cause a rescan of the waiting
        //  list.
        //

        if (AccessGranted) {

            DebugTrace(0, Dbg, "FsRtlCheckWaitingLocks now has access\n", 0);

            //
            //  Clear the cancel routine
            //

            IoSetCancelRoutine( Irp, NULL );

            Result = FsRtlPrivateInsertLock( LockInfo, IrpSp->FileObject, &FileLockInfo );

            //
            //  Now we need to remove this granted waiter and complete
            //  it's irp.
            //

            *pLink = Link->Next;
            if (Link == LockQueue->WaitingLocksTail.Next) {
                LockQueue->WaitingLocksTail.Next = (PSINGLE_LIST_ENTRY) pLink;
            }

            //
            // Release LockQueue and complete this waiter
            //

            FsRtlReleaseLockQueue(LockQueue, OldIrql);

            //
            //  Reference the fileobject over the completion attempt so we can have a
            //  chance to cleanup safely if we fail
            //

            ObReferenceObject( FileLockInfo.FileObject );

            //
            //  Now we can complete the IRP, if we don't get back success
            //  from the completion routine then we remove the lock we just
            //  inserted.
            //

            FsRtlCompleteLockIrp( LockInfo,
                                  WaitingLock->Context,
                                  Irp,
                                  (Result? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES),
                                  &NewStatus,
                                  FileLockInfo.FileObject );

            if (Result && !NT_SUCCESS(NewStatus)) {

                //
                // Irp was not sucessfull, remove lock if it was added.
                //

                FsRtlPrivateRemoveLock (
                    LockInfo,
                    &FileLockInfo,
                    FALSE );
            }

            //
            //  Drop our private reference to the fileobject
            //

            ObDereferenceObject( FileLockInfo.FileObject );

            //
            // Re-acquire queue lock
            //

            FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

            //
            // Start scan over from begining
            //

            pLink = &LockQueue->WaitingLocks.Next;


            //
            //  Free up pool
            //

            FsRtlFreeWaitingLock( WaitingLock );


        } else {

            DebugTrace( 0, Dbg, "FsRtlCheckWaitingLocks still no access\n", 0);

            //
            // Move to next lock
            //

            pLink = &Link->Next;
        }

    }

    //
    //  And return to our caller
    //

    return;
}


BOOLEAN
FsRtlPrivateCheckForExclusiveLockAccess (
    IN PLOCK_QUEUE LockQueue,
    IN PFILE_LOCK_INFO FileLockInfo
    )
/*++

Routine Description:

    This routine checks to see if the caller can get an exclusive lock on
    the indicated range due to file locks in the passed in lock queue.

    Assumes Lock queue is held by caller

Arguments:

    LockQueue - Queue which needs to be checked for collision

    FileLockInfo - Lock which is being checked


Return Value:

    BOOLEAN - TRUE if the indicated user can place the exclusive lock over the
        entire specified byte range, and FALSE otherwise

--*/

{
    PRTL_SPLAY_LINKS SplayLinks, LastSplayLinks = NULL;
    PLOCKTREE_NODE Node;
    PSH_LOCK ShLock;
    PEX_LOCK ExLock;

    if (LockQueue->SharedLockTree &&
        (SplayLinks = FsRtlFindFirstOverlappingSharedNode( LockQueue->SharedLockTree,
                                                           &FileLockInfo->StartingByte,
                                                           &FileLockInfo->EndingByte,
                                                           &LastSplayLinks, NULL))) {

        Node = CONTAINING_RECORD(SplayLinks, LOCKTREE_NODE, Links);

        //
        //  If this node is holey, we'll have to walk the whole thing.
        //

        if (Node->HoleyNode) {

            ShLock = FsRtlFindFirstOverlapInNode( Node,
                                                  &FileLockInfo->StartingByte,
                                                  &FileLockInfo->EndingByte );

        } else {

            ShLock = CONTAINING_RECORD(Node->Locks.Next, SH_LOCK, Link);
        }

        //
        //  Look for overlap that we care about.  Perhaps no overlap existed in the holey case.
        //

        if (ShLock &&
            (FileLockInfo->Length.QuadPart || ShLock->LockInfo.Length.QuadPart)) {

            //
            //  If we are checking a nonzero extent and overlapped, it is fatal. If we
            //  are checking a zero extent and overlapped a nonzero extent, it is fatal.
            //

            return FALSE;
        }
    }

    if (LastSplayLinks) {

        LockQueue->SharedLockTree = RtlSplay(LastSplayLinks);
        LastSplayLinks = NULL;
    }

    if (LockQueue->ExclusiveLockTree &&
        (SplayLinks = FsRtlFindFirstOverlappingExclusiveNode( LockQueue->ExclusiveLockTree,
                                                              &FileLockInfo->StartingByte,
                                                              &FileLockInfo->EndingByte,
                                                              &LastSplayLinks, NULL))) {

        ExLock = CONTAINING_RECORD(SplayLinks, EX_LOCK, Links);

        if (FileLockInfo->Length.QuadPart || ExLock->LockInfo.Length.QuadPart) {

            //
            //  If we are checking a nonzero extent and overlapped, it is fatal. If we
            //  are checking a zero extent and overlapped a nonzero extent, it is fatal.
            //

            return FALSE;
        }
    }

    if (LastSplayLinks) {

        LockQueue->ExclusiveLockTree = RtlSplay(LastSplayLinks);
    }

    //
    //  We searched the entire range without a conflict so we can grant
    //  the exclusive lock
    //

    return TRUE;
}


BOOLEAN
FsRtlPrivateCheckForSharedLockAccess (
    IN PLOCK_QUEUE LockQueue,
    IN PFILE_LOCK_INFO FileLockInfo
    )
/*++

Routine Description:

    This routine checks to see if the caller can get a shared lock on
    the indicated range due to file locks in the passed in lock queue.

    Assumes Lock queue is held by caller

Arguments:

    LockQueue - Queue which needs to be checked for collision

    FileLockInfo - Lock which is being checked

Arguments:

Return Value:

    BOOLEAN - TRUE if the indicated user can place the shared lock over
        entire specified byte range, and FALSE otherwise

--*/

{
    PEX_LOCK Lock;
    PRTL_SPLAY_LINKS SplayLinks, LastSplayLinks;
    BOOLEAN Status = TRUE;

    //
    // If there are no exclusive locks, this is quick ...
    //

    if (LockQueue->ExclusiveLockTree == NULL) {

        return TRUE;
    }

    //
    //  No lock in the shared lock tree can prevent access, so just search the exclusive
    //  tree for conflict.
    //

    for (SplayLinks = FsRtlFindFirstOverlappingExclusiveNode( LockQueue->ExclusiveLockTree,
                                                              &FileLockInfo->StartingByte,
                                                              &FileLockInfo->EndingByte,
                                                              &LastSplayLinks, NULL);
         SplayLinks;
         SplayLinks = RtlRealSuccessor(SplayLinks)) {

        Lock = CONTAINING_RECORD( SplayLinks, EX_LOCK, Links );

        if ((ULONGLONG)Lock->LockInfo.StartingByte.QuadPart > (ULONGLONG)FileLockInfo->EndingByte.QuadPart) {

            //
            //  This node is covering a range greater than the range we care about,
            //  so we're done
            //

            break;
        }

        //
        //  We may not be able to grant the request if the fileobject, processid,
        //  and key do not match.
        //

        if ((Lock->LockInfo.FileObject != FileLockInfo->FileObject) ||
             (Lock->LockInfo.ProcessId != FileLockInfo->ProcessId) ||
             (Lock->LockInfo.Key != FileLockInfo->Key)) {

            //
            //  We have a mismatch between caller and owner. It is ok not to conflict
            //  if the caller and owner will have/have zero length locks (zero length
            //  locks cannot conflict).
            //

            if (FileLockInfo->Length.QuadPart || Lock->LockInfo.Length.QuadPart) {

                Status = FALSE;
                break;
            }
        }
    }

    if (LastSplayLinks) {

        LockQueue->ExclusiveLockTree = RtlSplay(LastSplayLinks);
    }

    //
    //  We searched the entire range without a conflict so we can grant
    //  the shared lock
    //

    return Status;
}


VOID
FsRtlPrivateResetLowestLockOffset (
    PLOCK_INFO LockInfo
    )

/*++

Routine Description:

    This routine resets the lowest lock offset hint in a LOCK_INFO to
    the lowest lock offset currently held by a lock inside of the LOCK_INFO.

Arguments:

    LockInfo - the lock data to operate on

Return Value:

    None

--*/

{
    PEX_LOCK ExLock = NULL;
    PSH_LOCK ShLock = NULL;
    PFILE_LOCK_INFO LowestLockInfo = NULL;
    PRTL_SPLAY_LINKS SplayLinks;
    PLOCKTREE_NODE Node;

    //
    //  Fix up the lowest lock offset if we have non-empty trees and there was
    //  a lock in the low 32 bit region
    //

    if (LockInfo->LowestLockOffset != 0xffffffff &&
        (LockInfo->LockQueue.SharedLockTree != NULL ||
         LockInfo->LockQueue.ExclusiveLockTree != NULL)) {

        //
        //  Grab the lowest nodes in the trees
        //

        if (LockInfo->LockQueue.SharedLockTree) {

            SplayLinks = LockInfo->LockQueue.SharedLockTree;

            while (RtlLeftChild(SplayLinks) != NULL) {

                SplayLinks = RtlLeftChild(SplayLinks);
            }

            Node = CONTAINING_RECORD( SplayLinks, LOCKTREE_NODE, Links );
            ShLock = CONTAINING_RECORD( Node->Locks.Next, SH_LOCK, Link );
        }

        if (LockInfo->LockQueue.ExclusiveLockTree) {

            SplayLinks = LockInfo->LockQueue.ExclusiveLockTree;

            while (RtlLeftChild(SplayLinks) != NULL) {

                SplayLinks = RtlLeftChild(SplayLinks);
            }

            ExLock = CONTAINING_RECORD( SplayLinks, EX_LOCK, Links );
        }

        //
        //  Figure out which of the lowest locks is actually lowest. We know that one of the lock
        //  trees at least has a lock, so if we have don't have exclusive locks then we do know
        //  we have shared locks ...
        //

        if (ExLock &&
            (!ShLock ||
             (ULONGLONG)ExLock->LockInfo.StartingByte.QuadPart < (ULONGLONG)ShLock->LockInfo.StartingByte.QuadPart)) {

            LowestLockInfo = &ExLock->LockInfo;

        } else {

            LowestLockInfo = &ShLock->LockInfo;
        }

        if (LowestLockInfo->StartingByte.HighPart == 0) {

            LockInfo->LowestLockOffset = LowestLockInfo->StartingByte.LowPart;

        } else {

            LockInfo->LowestLockOffset = 0xffffffff;
        }

    } else {

        //
        //  If there are no locks, set the lock offset high
        //

        LockInfo->LowestLockOffset = 0xffffffff;
    }
}


NTSTATUS
FsRtlPrivateFastUnlockAll (
    IN PFILE_LOCK FileLock,
    IN PFILE_OBJECT FileObject,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN BOOLEAN MatchKey,
    IN PVOID Context OPTIONAL
    )

/*++

Routine Description:

    This routine performs an Unlock all operation on the current locks
    associated with the specified file lock.  Only those locks with
    a matching file object and process id are freed.  Additionally,
    it is possible to free only those locks which also match a given
    key.

Arguments:

    FileLock - Supplies the file lock being freed.

    FileObject - Supplies the file object associated with the file lock

    ProcessId - Supplies the Process Id assoicated with the locks to be
        freed

    Key - Supplies the Key to use in this operation

    MatchKey - Whether or not the Key must also match for lock to be freed.

    Context - Supplies an optional context to use when completing waiting
        lock irps.

Return Value:

    None

--*/

{
    PLOCK_INFO              LockInfo;
    PLOCK_QUEUE             LockQueue;
    PSINGLE_LIST_ENTRY      *pLink, *SavepLink, Link;
    NTSTATUS                NewStatus;
    KIRQL                   OldIrql;
    LARGE_INTEGER           MaxOffset, SaveEndingByte;
    BOOLEAN                 UnlockRoutine;
    PSH_LOCK                ShLock;
    PEX_LOCK                ExLock;
    PRTL_SPLAY_LINKS        SplayLinks, SuccessorLinks;
    PLOCKTREE_NODE          Node;


    DebugTrace(+1, Dbg, "FsRtlPrivateFastUnlockAll, FileLock = %08lx\n", FileLock);

    if ((LockInfo = FileLock->LockInformation) == NULL) {

        //
        // No lock information on this FileLock
        //

        DebugTrace(+1, Dbg, "FsRtlPrivateFastUnlockAll, No LockInfo\n", FileLock);
        return STATUS_RANGE_NOT_LOCKED;
    }

    FileObject->LastLock = NULL;

    LockQueue = &LockInfo->LockQueue;

    //
    //  Grab the waiting lock queue spinlock to exclude anyone from messing
    //  with the queue while we're using it
    //

    FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

    if (LockQueue->SharedLockTree == NULL && LockQueue->ExclusiveLockTree == NULL) {

        //
        // No locks on this FileLock
        //

        DebugTrace(+1, Dbg, "FsRtlPrivateFastUnlockAll, No LockTrees\n", FileLock);
        FsRtlReleaseLockQueue(LockQueue, OldIrql);

        return STATUS_RANGE_NOT_LOCKED;
    }

    //
    //  Remove all matching locks in the shared lock tree
    //

    if (LockQueue->SharedLockTree != NULL) {

        //
        //  Grab the lowest node in the tree
        //

        SplayLinks = LockQueue->SharedLockTree;

        while (RtlLeftChild(SplayLinks) != NULL) {

            SplayLinks = RtlLeftChild(SplayLinks);
        }

        //
        //  Walk all nodes in the tree
        //

        UnlockRoutine = FALSE;

        for (;
             SplayLinks;
             SplayLinks = SuccessorLinks) {

            Node = CONTAINING_RECORD(SplayLinks, LOCKTREE_NODE, Links );

            //
            //  Save the next node because we may split this node apart in the process
            //  of deleting locks. It would be a waste of time to traverse those split
            //  nodes. The only case in which we will not have traversed the entire list
            //  before doing the split will be if there is an unlock routine attached
            //  to this FileLock in which case we will be restarting the entire scan
            //  anyway.
            //

            SuccessorLinks = RtlRealSuccessor(SplayLinks);

            //
            //  Search down the current lock queue looking for a match on
            //  the file object and process id
            //

            SavepLink = NULL;
            SaveEndingByte.QuadPart = 0;

            pLink = &Node->Locks.Next;
            while ((Link = *pLink) != NULL) {

                ShLock = CONTAINING_RECORD( Link, SH_LOCK, Link );

                DebugTrace(0, Dbg, "Top of ShLock Loop, Lock = %08lx\n", ShLock );

                if ((ShLock->LockInfo.FileObject == FileObject) &&
                    (ShLock->LockInfo.ProcessId == ProcessId) &&
                    (!MatchKey || ShLock->LockInfo.Key == Key)) {

                    DebugTrace(0, Dbg, "Found one to unlock\n", 0);

                    //
                    //  We have a match so now is the time to delete this lock.
                    //  Save the neccesary information to do the split node check.
                    //  Remove the lock from the list, then call the
                    //  optional unlock routine, then delete the lock.
                    //

                    if (SavepLink == NULL) {

                        //
                        //  Need to remember where the first lock was deleted
                        //

                        SavepLink = pLink;
                    }

                    if ((ULONGLONG)ShLock->LockInfo.EndingByte.QuadPart > (ULONGLONG)SaveEndingByte.QuadPart) {

                        //
                        //  Need to remember where the last offset affected by deleted locks is
                        //

                        SaveEndingByte.QuadPart = ShLock->LockInfo.EndingByte.QuadPart;
                    }

                    if (*pLink == Node->Tail.Next) {

                        //
                        //  Deleting the tail node of the list. Safe even if deleting the
                        //  first node since this implies we're also deleting the last node
                        //  in the node which means we'll delete the node ...
                        //

                        Node->Tail.Next = CONTAINING_RECORD( pLink, SINGLE_LIST_ENTRY, Next );
                    }

                    *pLink = Link->Next;

                    if (LockInfo->UnlockRoutine != NULL) {

                        //
                        //  Signal a lock that needs to have a special unlock routine
                        //  called on it. This is complex to deal with since we'll have
                        //  to release the queue, call it, and reacquire - meaning we
                        //  also have to restart. But we still need to reorder the node
                        //  first ...
                        //

                        UnlockRoutine = TRUE;

                        break;
                    }

                    FsRtlFreeSharedLock( ShLock );

                } else {

                    //
                    // Move to next lock
                    //

                    pLink = &Link->Next;
                }

                if (SavepLink == NULL && (ULONGLONG)ShLock->LockInfo.EndingByte.QuadPart > (ULONGLONG)MaxOffset.QuadPart) {

                    //
                    //  Save the max offset until we have deleted our first node
                    //

                    MaxOffset.QuadPart = ShLock->LockInfo.EndingByte.QuadPart;
                }
            }

            if (SavepLink) {

                //
                //  Locks were actually deleted here, so we have to check the state of the node
                //

                if (Node->Locks.Next == NULL) {

                    //
                    //  We have just deleted everything at this node
                    //

                    LockQueue->SharedLockTree = RtlDelete(SplayLinks);

                    FsRtlFreeLockTreeNode(Node);

                } else {

                    //
                    //  Now that we have deleted all matching locks in this node, we do the
                    //  check on the node to split out any now non-overlapping locks. Conceptually,
                    //  we have deleted just one big lock that starts at the starting byte of the
                    //  first deleted lock and extends to the last byte of the last deleted lock.
                    //

                    FsRtlSplitLocks(Node, SavepLink, &SaveEndingByte, &MaxOffset);
                }
            }

            if (UnlockRoutine) {

                //
                //  We dropped out of the node scan because we had a lock that needs extra
                //  processing during unlock. Do it.
                //

                FsRtlReleaseLockQueue(LockQueue, OldIrql);

                LockInfo->UnlockRoutine(Context, &ShLock->LockInfo);

                FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

                FsRtlFreeSharedLock(ShLock);

                UnlockRoutine = FALSE;

                //
                //  We have to restart the scan, because the list may have changed while
                //  we were in the unlock routine. Careful, because the tree may be empty.
                //

                if (SuccessorLinks = LockQueue->SharedLockTree) {

                    while (RtlLeftChild(SuccessorLinks) != NULL) {

                        SuccessorLinks = RtlLeftChild(SuccessorLinks);
                    }
                }
            }
        }
    }

    //
    //  Remove all matching locks in the exclusive lock tree
    //

    if (LockQueue->ExclusiveLockTree != NULL) {

        SplayLinks = LockQueue->ExclusiveLockTree;

        while (RtlLeftChild(SplayLinks) != NULL) {

            SplayLinks = RtlLeftChild(SplayLinks);
        }

        //
        //  Walk all nodes in the tree
        //

        UnlockRoutine = FALSE;

        for (; SplayLinks;
               SplayLinks = SuccessorLinks ) {

            SuccessorLinks = RtlRealSuccessor(SplayLinks);

            ExLock = CONTAINING_RECORD( SplayLinks, EX_LOCK, Links );

            DebugTrace(0, Dbg, "Top of ExLock Loop, Lock = %08lx\n", ExLock );

            if ((ExLock->LockInfo.FileObject == FileObject) &&
                (ExLock->LockInfo.ProcessId == ProcessId) &&
                (!MatchKey || ExLock->LockInfo.Key == Key)) {

                LockQueue->ExclusiveLockTree = RtlDelete(&ExLock->Links);

                if (LockInfo->UnlockRoutine != NULL) {

                    //
                    //  We're dropping out of the node scan because we have a lock
                    //  that needs extra processing during unlock. Do it.
                    //

                    FsRtlReleaseLockQueue(LockQueue, OldIrql);

                    LockInfo->UnlockRoutine(Context, &ExLock->LockInfo);

                    FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

                    //
                    //  We have to restart the scan, because the list may have changed while
                    //  we were in the unlock routine. Careful, because the tree may be empty.
                    //

                    if (SuccessorLinks = LockQueue->ExclusiveLockTree) {

                        while (RtlLeftChild(SuccessorLinks) != NULL) {

                            SuccessorLinks = RtlLeftChild(SuccessorLinks);
                        }
                    }
                }

                FsRtlFreeExclusiveLock(ExLock);
            }
        }
    }

    //
    //  Search down the waiting lock queue looking for a match on the
    //  file object and process id.
    //

    pLink = &LockQueue->WaitingLocks.Next;
    while ((Link = *pLink) != NULL) {

        PWAITING_LOCK WaitingLock;
        PIRP WaitingIrp;
        PIO_STACK_LOCATION WaitingIrpSp;

        WaitingLock = CONTAINING_RECORD( Link, WAITING_LOCK, Link );

        DebugTrace(0, Dbg, "Top of Waiting Loop, WaitingLock = %08lx\n", WaitingLock);

        //
        //  Get a copy of the necessary fields we'll need to use
        //

        WaitingIrp = WaitingLock->Irp;
        WaitingIrpSp = IoGetCurrentIrpStackLocation( WaitingIrp );

        if ((FileObject == WaitingIrpSp->FileObject) &&
            (ProcessId == IoGetRequestorProcess( WaitingIrp )) &&
            (!MatchKey || Key == WaitingIrpSp->Parameters.LockControl.Key)) {

            DebugTrace(0, Dbg, "Found a waiting lock to abort\n", 0);

            //
            //  We now void the cancel routine in the irp
            //

            IoSetCancelRoutine( WaitingIrp, NULL );
            WaitingIrp->IoStatus.Information = 0;

            //
            //  We have a match so now is the time to delete this waiter
            //  But we must not mess up our link iteration variable.  We
            //  do this by simply starting the iteration over again,
            //  after we delete ourselves.  We also will deallocate the
            //  lock after we delete it.
            //

            *pLink = Link->Next;
            if (Link == LockQueue->WaitingLocksTail.Next) {
                LockQueue->WaitingLocksTail.Next = (PSINGLE_LIST_ENTRY) pLink;
            }

            FsRtlReleaseLockQueue(LockQueue, OldIrql);

            //
            //  And complete this lock request Irp
            //

            FsRtlCompleteLockIrp( LockInfo,
                                    WaitingLock->Context,
                                    WaitingIrp,
                                    STATUS_SUCCESS,
                                    &NewStatus,
                                    NULL );

            //
            // Reaqcuire lock queue spinlock and start over
            //

            FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

            //
            // Start over
            //

            pLink = &LockQueue->WaitingLocks.Next;

            //
            // Put memory onto free list
            //

            FsRtlFreeWaitingLock( WaitingLock );
            continue;
        }

        //
        // Move to next lock
        //

        pLink = &Link->Next;
    }

    //
    //  At this point we've gone through unlocking everything. So
    //  now try and release any waiting locks.
    //

    FsRtlPrivateCheckWaitingLocks( LockInfo, LockQueue, OldIrql );

    //
    //  We deleted a (possible) bunch of locks, go repair the lowest lock offset
    //

    FsRtlPrivateResetLowestLockOffset( LockInfo );

    FsRtlReleaseLockQueue( LockQueue, OldIrql );

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "FsRtlFastUnlockAll -> VOID\n", 0);
    return STATUS_SUCCESS;
}


VOID
FsRtlPrivateCancelFileLockIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the cancel function for an irp saved in a
    waiting lock queue

Arguments:

    DeviceObject - Ignored

    Irp - Supplies the Irp being cancelled.  A pointer to the FileLock
        structure for the lock is stored in the information field of the
        irp's iosb.

Return Value:

    none.

--*/

{
    PSINGLE_LIST_ENTRY *pLink, Link;
    PLOCK_INFO  LockInfo;
    PLOCK_QUEUE LockQueue;
    KIRQL       OldIrql;
    NTSTATUS    NewStatus;


    UNREFERENCED_PARAMETER( DeviceObject );

    //
    //  The information field is used to store a pointer to the file lock
    //  containing the irp
    //

    LockInfo = (PLOCK_INFO) (Irp->IoStatus.Information);

    //
    // Iterate through the lock queue.
    //

    LockQueue = &LockInfo->LockQueue;

    //
    // Release the cancel spinlock and lock the waiting queue if this
    // is initiated by Io. We already have the lock queue if this is
    // the race fixup.
    //
    // If this is from ourselves, we have the cancel Irql in the Irp.
    //

    if (DeviceObject) {
        
        IoReleaseCancelSpinLock( Irp->CancelIrql );
        FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);
    
    } else {

        OldIrql = Irp->CancelIrql;
    }

    //
    // Iterate through all of the waiting locks looking for a canceled one.
    
    pLink = &LockQueue->WaitingLocks.Next;
    while ((Link = *pLink) != NULL) {

        PWAITING_LOCK WaitingLock;

        //
        //  Get a pointer to the waiting lock record
        //

        WaitingLock = CONTAINING_RECORD( Link, WAITING_LOCK, Link );

        DebugTrace(0, Dbg, "FsRtlPrivateCancelFileLockIrp, Loop top, WaitingLock = %08lx\n", WaitingLock);

        if( WaitingLock->Irp != Irp ) {

            pLink = &Link->Next;
            continue;
        }

        //
        //  We've found it -- remove it from the list
        //

        *pLink = Link->Next;
        if (Link == LockQueue->WaitingLocksTail.Next) {

            LockQueue->WaitingLocksTail.Next = (PSINGLE_LIST_ENTRY) pLink;
        }

        Irp->IoStatus.Information = 0;

        //
        // Release LockQueue and complete this waiter
        //

        FsRtlReleaseLockQueue(LockQueue, OldIrql);

        //
        // Complete this waiter
        //

        FsRtlCompleteLockIrp( LockInfo,
                              WaitingLock->Context,
                              Irp,
                              STATUS_CANCELLED,
                              &NewStatus,
                              NULL );

        //
        //  Free up pool
        //

        FsRtlFreeWaitingLock( WaitingLock );

        //
        // Our job is done!
        //

        return;
    }

    //
    // Release lock queue
    //

    FsRtlReleaseLockQueue(LockQueue, OldIrql);

    return;
}

