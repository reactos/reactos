
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
    // SpinLock to guard queue access
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


//////////////////////////////////////////////////////////////////////////

//
//  The oplock state is now just a ULONG.
//

typedef ULONG OPLOCK_STATE;

//
//  The non-opaque definition of an OPLOCK is a pointer to a privately
//  defined structure.
//

typedef struct _NONOPAQUE_OPLOCK {

    //
    //  This is the Irp used to successfully request a level I oplock or
    //  batch oplock.  It is completed to initiate the Oplock I break
    //  procedure.
    //

    PIRP IrpExclusiveOplock;

    //
    //  This is a pointer to the original file object used when granting
    //  an Oplock I or batch oplock.
    //

    PFILE_OBJECT FileObject;

    //
    //  The start of a linked list of Irps used to successfully request
    //  a level II oplock.
    //

    LIST_ENTRY IrpOplocksII;

    //
    //  The following links the Irps waiting to be completed on a queue
    //  of Irps.
    //

    LIST_ENTRY WaitingIrps;

    //
    //  Oplock state.  This indicates the current oplock state.
    //

    OPLOCK_STATE OplockState;

    //
    //  This FastMutex is used to control access to this structure.
    //

    PFAST_MUTEX FastMutex;

    //
    //  This is the timer package to process an OpFilter break.
    //

    struct _OPFILTER_TIMER *OpFilter;

    //
    //  Count of outstanding timers.
    //

    ULONG TimerCount;

} NONOPAQUE_OPLOCK, *PNONOPAQUE_OPLOCK;

//
//  Each Waiting Irp record corresponds to an Irp that is waiting for an
//  oplock break to be acknowledged and is maintained in a queue off of the
//  Oplock's WaitingIrps list.
//

typedef struct _WAITING_IRP {

    //
    //  The link structures for the list of waiting irps.
    //

    LIST_ENTRY Links;

    //
    //  This is the Irp attached to this structure.
    //

    PIRP Irp;

    //
    //  This is the routine to call when we are done with an Irp we
    //  held on a waiting queue.  (We originally returned STATUS_PENDING).
    //

    POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine;

    //
    //  The context field to use when we are done with the Irp.
    //

    PVOID Context;

    //
    //  This points to an event object used when we do not want to
    //  give up this thread.
    //

    PKEVENT Event;

    //
    //  This field contains a copy of the Irp Iosb.Information field.
    //  We copy it here so that we can store the Oplock address in the
    //  Irp.
    //

    ULONG Information;

} WAITING_IRP, *PWAITING_IRP;

//
//  The following structure is used when timing out the OpFilter oplocks.
//

typedef struct _OPFILTER_TIMER {

    KDPC OpFilterDpc;
    KTIMER OpFilterTimer;
    WORK_QUEUE_ITEM OpFilterItem;
    PNONOPAQUE_OPLOCK Oplock;

} OPFILTER_TIMER, *POPFILTER_TIMER;

