/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxVerifierLock.cpp

Abstract:

    This module contains the implementation of the verifier lock

Author:




Environment:

    Both kernel and user mode

Revision History:




--*/

#include "fxobjectpch.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "FxVerifierLock.tmh"
#endif
}

//
// Mapping table structure between Fx object types and lock orders
//
FxVerifierOrderMapping FxVerifierOrderTable[] = {

    // Table is defined in fx\inc\FxVerifierLock.hpp
    FX_VERIFIER_LOCK_ENTRIES()
};

FxVerifierOrderMapping FxVerifierCallbackOrderTable[] = {

    // Table is defined in fx\inc\FxVerifierLock.hpp
    FX_VERIFIER_CALLBACKLOCK_ENTRIES()
};

//
// Organization of verifier lock structures
//
// As locks are acquired, they are added to the head of a list of locks held
// by the current thread if equal, or higher than the lock
// at the current list head.
//
// The hierachy of locks acquired by a thread is seperate for dispatch level
// (spinlock) and passive level (mutex) locks. These locks can not be mixed
// since holding a mutex lock does not prevent a DPC from interrupting the
// thread and properly acquiring a spinlock, which could appear to be
// of an improper level, giving a false report.
//
// In order to prevent memory allocations (which can fail) when locks are
// acquired, each FxVerifierLock structure contains members required to chain
// locks that are held on a per thread basis. (m_OwnedLink)
//
// Since we have no way of knowing the number of unique PETHREADS that may
// be holding locks at any time, a fixed size hash table is allocated
// and PETHREAD values are hashed into it, with chaining allowed on
// each entry due to overflows and collisions. The storage required for
// the hash table entry and chain is also allocated as member fields
// of the FxVerifierLock class. (m_ThreadTableEntry)
//
// The hash table and its chained entries (m_ThreadTableEntry) is protected
// by a global spinlock, FxDriverGlobals->ThreadTableLock.
//
// When a lock is acquired, the current threads address is used to look up
// a FxVerifierThreadTableEntry for it in the hash table.
//
// If one does not exist, this is the start of a new chain of locks for this
// ETHREAD, and the m_ThreadTableEntry of the current lock being acquired
// is inserted into the hash table, and the new lock chain is started using
// either the
// FxVerifierThreadTableEntry::PerThreadPassiveLockList, or
// FxVerifierThreadTableEntry::PerThreadDispatchLockList.
//
// If an entry does exist, the current lock is added to the head of
// the list in the existing entry.
//
// On lock release, the lock is identified in the list of held locks,
// which may not be the head if release was out of order (this is allowed),
// and removed from the list.
//
// If the list of locks held by the current thread is NULL for both
// the passive and dispatch lists, the threads entry is removed from
// the hash table since the thread no longer holds any locks.
//
// If either of these lists is not NULL, then the entries in the
// current locks FxVerifierThreadTableEntry is copied into one
// of the other locks on the new list head and the hash table
// is updated to point to the new entry.
//
// This is because the released FxVerifierLock entry may be freed
// as a result of its containing data structure being run down.
//

//
// Method Definitions
//

//
// Called at Driver Frameworks init time
//
extern "C"
void
FxVerifierLockInitialize(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    if( FxDriverGlobals->FxVerifierLock ) {

        FxDriverGlobals->ThreadTableLock.Initialize();

        FxVerifierLock::AllocateThreadTable(FxDriverGlobals);
    }

    return;
}

//
// Called at Driver Frameworks is unloading
//
extern "C"
void
FxVerifierLockDestroy(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    if( FxDriverGlobals->FxVerifierLock ) {
        FxVerifierLock::FreeThreadTable(FxDriverGlobals);

        FxDriverGlobals->ThreadTableLock.Uninitialize();
    }

    return;
}

VOID
FxVerifierLock::Lock(
    __out PKIRQL PreviousIrql,
    __in BOOLEAN AtDpc
    )
{
    MxThread curThread;
    FxVerifierLock* head;
    pFxVerifierThreadTableEntry perThreadList;
    KIRQL oldIrql = PASSIVE_LEVEL;

    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

    curThread = Mx::MxGetCurrentThread();

    //
    // First check to see if the lock is already
    // owned.
    //
    // This check is race free since this can only be set
    // to our thread from under the spinlock, and is cleared
    // before release.
    //

    if( m_OwningThread == curThread ) {

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_FATAL, TRACINGDEVICE,
            "Thread 0x%p already owns lock 0x%p for object 0x%p, WDFOBJECT 0x%p",
            curThread, this, m_ParentObject, m_ParentObject->GetObjectHandle());

        FxVerifierBugCheck( FxDriverGlobals,
                            WDF_RECURSIVE_LOCK,
                            (ULONG_PTR) m_ParentObject->GetObjectHandle(),
                            (ULONG_PTR) this);
    }

    if( m_UseMutex ) {
        // Get the Mutex
        Mx::MxEnterCriticalRegion();
        m_Mutex.AcquireUnsafe();

        *PreviousIrql = Mx::MxGetCurrentIrql();
    }
    else if (AtDpc) {
        // Get the spinlock
        m_Lock.AcquireAtDpcLevel();
        m_OldIrql = DISPATCH_LEVEL;

        *PreviousIrql = m_OldIrql;
    }
    else {
        // Try to force a thread switch to catch synchronization errors
        if (Mx::MxGetCurrentIrql() == PASSIVE_LEVEL) {
            LARGE_INTEGER sleepTime;

            sleepTime.QuadPart = 0;
            Mx::MxDelayExecutionThread(KernelMode, TRUE, &sleepTime);
        }

        // Get the spinlock using a local since KeAcquireSpinLock might use this
        // var as temp space while spinning and we would then corrupt it if
        // the owning thread used it in the middle of the spin.
        //
        m_Lock.Acquire(PreviousIrql);
        m_OldIrql = *PreviousIrql;
    }

    // Lock the verifier lists
    if (m_UseMutex) {
        FxDriverGlobals->ThreadTableLock.Acquire(&oldIrql);
    }
    else {
        FxDriverGlobals->ThreadTableLock.AcquireAtDpcLevel();
    }

    m_OwningThread = curThread;

    // Get our per thread list from the thread table
    perThreadList = FxVerifierLock::GetThreadTableEntry(curThread, this, FALSE);
    if( perThreadList == NULL ) {

        if (m_UseMutex) {
            FxDriverGlobals->ThreadTableLock.Release(oldIrql);
        }
        else {
            FxDriverGlobals->ThreadTableLock.ReleaseFromDpcLevel();
        }

        // Can't get an entry, so return
        return;
    }

    //
    // There are seperately sorted lists for passive and dispatch
    // level locks since dispatch level locks of a lower level can interrupt a
    // higher passive level lock, giving a false report.
    //
    if( m_UseMutex ) {
        head = perThreadList->PerThreadPassiveLockList;
    }
    else {
        head = perThreadList->PerThreadDispatchLockList;
    }

    if( head != NULL ) {

        // Validate current is > head
        if( m_Order > head->m_Order ) {
            m_OwnedLink = head;
            //perThreadList->PerThreadLockList = this;
        }
        else if( m_Order == head->m_Order ) {

            // Place at head if the same lock level as current head
            m_OwnedLink = head;
            //perThreadList->PerThreadLockList = this;
        }
        else {

            // Lock violation, m_Order < head->m_Order
            FxVerifierLock::DumpDetails(this, curThread, head);

            //
            // Caller is feeling lucky and resumed so place it at the head
            // to keep our lists intact
            //
            m_OwnedLink = head;
            //perThreadList->PerThreadLockList = this;
        }
    }
    else {
        this->m_OwnedLink = NULL;
        //perThreadList->PerThreadLockList = this;
    }

    //
    // Update to the next list head
    //
    if (m_UseMutex) {
        perThreadList->PerThreadPassiveLockList = this;
        FxDriverGlobals->ThreadTableLock.Release(oldIrql);
    }
    else {
        perThreadList->PerThreadDispatchLockList = this;
        FxDriverGlobals->ThreadTableLock.ReleaseFromDpcLevel();
    }

    return;
}

void
FxVerifierLock::Unlock(
    __in KIRQL PreviousIrql,
    __in BOOLEAN AtDpc
    )
{
    MxThread curThread;
    pFxVerifierThreadTableEntry perThreadList;
    KIRQL oldIrql;

    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

    // Only from DISPATCH_LEVEL or below
    curThread = Mx::MxGetCurrentThread();

    if( curThread != m_OwningThread ) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Thread 0x%p Is Attempting to release a Lock 0x%p "
                            "for Object 0x%p it does not own!",
                            curThread,this,m_ParentObject);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        return;
    }

    // Lock the verifier lists
    FxDriverGlobals->ThreadTableLock.Acquire(&oldIrql);

    // Get our per thread list from the thread table
    perThreadList = FxVerifierLock::GetThreadTableEntry(m_OwningThread, this, TRUE);
    if( perThreadList == NULL ) {

        // Can't get our entry, so release the spinlock and return
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Unlock:  Can't get per thread entry for thread %p",
                            curThread);

        m_OwningThread = NULL;

        FxDriverGlobals->ThreadTableLock.Release(oldIrql);

        if (m_UseMutex) {
            m_Mutex.ReleaseUnsafe();
            Mx::MxLeaveCriticalRegion();
        }
        else if (AtDpc) {
            m_Lock.ReleaseFromDpcLevel();
        }
        else {
            m_Lock.Release(PreviousIrql);

            // Try to force a thread switch to catch synchronization errors
            if (Mx::MxGetCurrentIrql() == PASSIVE_LEVEL) {
                LARGE_INTEGER sleepTime;

                sleepTime.QuadPart = 0;
                Mx::MxDelayExecutionThread(KernelMode, TRUE, &sleepTime);
            }
        }
        return;
    }

    if( m_UseMutex ) {
        if( perThreadList->PerThreadPassiveLockList == NULL ) {
            // Problem with verifier
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Thread has entry, but no locks recorded as "
                                "held for passive!");
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "this 0x%p, perThreadList 0x%p",
                                this, perThreadList);
            FxVerifierDbgBreakPoint(FxDriverGlobals);

            m_OwningThread = NULL;

            FxDriverGlobals->ThreadTableLock.Release(oldIrql);

            m_Mutex.ReleaseUnsafe();
            Mx::MxLeaveCriticalRegion();

            return;
        }
    }
    else {
        if( perThreadList->PerThreadDispatchLockList == NULL ) {
            // Problem with verifier
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Thread has entry, but no locks recorded as held "
                                "for dispatch!");
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "this 0x%p, perThreadList 0x%p",
                                this, perThreadList);
            FxVerifierDbgBreakPoint(FxDriverGlobals);

            m_OwningThread = NULL;

            FxDriverGlobals->ThreadTableLock.Release(oldIrql);

            if (AtDpc) {
                m_Lock.ReleaseFromDpcLevel();
            }
            else {
                m_Lock.Release(PreviousIrql);

                // Try to force a thread switch to catch synchronization errors
                if (Mx::MxGetCurrentIrql() == PASSIVE_LEVEL) {
                    LARGE_INTEGER sleepTime;

                    sleepTime.QuadPart = 0;
                    Mx::MxDelayExecutionThread(KernelMode, TRUE, &sleepTime);
                }
            }

            return;
        }
    }

    if( m_UseMutex ) {

        // Common case is a nested release
        if( perThreadList->PerThreadPassiveLockList == this ) {

            perThreadList->PerThreadPassiveLockList = this->m_OwnedLink;
            m_OwnedLink = NULL;

            ReleaseOrReplaceThreadTableEntry(curThread, this);






















        }
        else {
            //
            // Releasing out of order does not deadlock as
            // long as all acquires are in order, but its
            // not commmon
            //
            FxVerifierLock* next;
            FxVerifierLock* prev = NULL;

            // Skip the first entry checked by the above if
            prev = perThreadList->PerThreadPassiveLockList;
            next = perThreadList->PerThreadPassiveLockList->m_OwnedLink;

            while( next != NULL ) {

                if( next == this ) {
                    prev->m_OwnedLink = m_OwnedLink;
                    m_OwnedLink = NULL;

                    //
                    // The perThreadList entry may, or may not be the
                    // data structure in this lock. Sinse we are releasing
                    // the lock, this entry can no longer be referenced if
                    // so.
                    //
                    ReleaseOrReplaceThreadTableEntry(curThread, this);

//                    FxVerifierLock::ReplaceThreadTableEntry(curThread, this, perThreadList->PerThreadPassiveLockList);

                    break;
                }

                prev = next;
                next = next->m_OwnedLink;
            }

            if( next == NULL ) {
                // Somehow the entry is gone
                DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                    "Record entry for VerifierLock 0x%p is missing "
                                    "on list 0x%p for Thread 0x%p",
                                    this,perThreadList,m_OwningThread);
                FxVerifierDbgBreakPoint(FxDriverGlobals);
            }
        }
    }
    else {

        // Common case is a nested release
        if( perThreadList->PerThreadDispatchLockList == this ) {

            perThreadList->PerThreadDispatchLockList = this->m_OwnedLink;
            m_OwnedLink = NULL;

            ReleaseOrReplaceThreadTableEntry(curThread, this);






















        }
        else {
            //
            // Releasing out of order does not deadlock as
            // long as all acquires are in order, but its
            // not commmon
            //
            FxVerifierLock* next;
            FxVerifierLock* prev = NULL;

            // Skip the first entry checked by the above if
            prev = perThreadList->PerThreadDispatchLockList;
            next = perThreadList->PerThreadDispatchLockList->m_OwnedLink;

            while( next != NULL ) {

                if( next == this ) {
                    prev->m_OwnedLink = m_OwnedLink;
                    m_OwnedLink = NULL;

                    //
                    // The perThreadList entry may, or may not be the
                    // data structure in this lock. Sinse we are releasing
                    // the lock, this entry can no longer be referenced if
                    // so.
                    //
                    ReleaseOrReplaceThreadTableEntry(curThread, this);

//                    FxVerifierLock::ReplaceThreadTableEntry(curThread, this, perThreadList->PerThreadDispatchLockList);

                    break;
                }

                prev = next;
                next = next->m_OwnedLink;
            }

            if( next == NULL ) {
                // Somehow the entry is gone
                DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                    "Record entry for VerifierLock 0x%p is missing "
                                    "on list 0x%p for Thread 0x%p",
                                    this,perThreadList,m_OwningThread);
                FxVerifierDbgBreakPoint(FxDriverGlobals);
            }
        }

    }

    m_OwningThread = NULL;

    FxDriverGlobals->ThreadTableLock.Release(oldIrql);

    if (m_UseMutex) {
        m_Mutex.ReleaseUnsafe();
        Mx::MxLeaveCriticalRegion();
    }
    else if (AtDpc) {
        m_Lock.ReleaseFromDpcLevel();
    }
    else {
        m_Lock.Release(PreviousIrql);

        // Try to force a thread switch to catch synchronization errors
        if (Mx::MxGetCurrentIrql() == PASSIVE_LEVEL) {
            LARGE_INTEGER sleepTime;

            sleepTime.QuadPart = 0;
            Mx::MxDelayExecutionThread(KernelMode, TRUE, &sleepTime);
        }
    }

    return;
}

KIRQL
FxVerifierLock::GetLockPreviousIrql()
{
    return m_OldIrql;
}

void
FxVerifierLock::InitializeLockOrder()
{
    USHORT ObjectType;
    pFxVerifierOrderMapping p;

    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

    ObjectType = m_ParentObject->GetType();

    if( m_CallbackLock ) {
        p = FxVerifierCallbackOrderTable;
    }
    else {
        p = FxVerifierOrderTable;
    }

    while( p->ObjectType != 0 ) {

        if( p->ObjectType == ObjectType ) {
            m_Order = p->ObjectLockOrder;
            return;
        }

        p++;
    }

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                        "Object Type 0x%x does not have a lock order "
                        "defined in fx\\inc\\FxVerifierLock.hpp",
                        ObjectType);

    m_Order = FX_LOCK_ORDER_UNKNOWN;

    return;
}

//
// This looks up the supplied thread in the table, and if
// found returns it.
//
// If no entry for the thread is found, create one using the
// m_ThreadTableEntry for the lock.
//
pFxVerifierThreadTableEntry
FxVerifierLock::GetThreadTableEntry(
    __in MxThread        curThread,
    __in FxVerifierLock* pLock,
    __in BOOLEAN         LookupOnly
    )
{
    ULONG Hash, Index;
    PLIST_ENTRY head, next;
    FxVerifierLock* entry;

    PFX_DRIVER_GLOBALS FxDriverGlobals = pLock->GetDriverGlobals();

    // Verifier is off, or an early memory allocation failure
    if( FxDriverGlobals->ThreadTable == NULL ) {
        return NULL;
    }

    //
    // Hash the KeCurrentThread() information into an table
    // index.
    //
    // Hash takes into account that NT pool items don't use
    // the lower 4 bits (16 byte boundries)
    //

    Hash = (ULONG)((ULONG_PTR)curThread >> 4);
    Hash = ((Hash >> 16) & 0x0000FFFF) ^ (Hash & 0x0000FFFF);

    //
    // Hash table is maintained as a power of two
    //
    Index = Hash & (VERIFIER_THREAD_HASHTABLE_SIZE-1);

    head = &FxDriverGlobals->ThreadTable[Index];

    //
    // Walk the list to see if our thread has an entry
    //
    next = head->Flink;
    while( next != head ) {
        entry = CONTAINING_RECORD(next, FxVerifierLock, m_ThreadTableEntry.HashChain);

        if( entry->m_ThreadTableEntry.Thread == curThread ) {

            //DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            //                    "Returning existing Entry 0x%p for Thread 0x%p, "
            //                    "Lock 0x%p",
            //                    &entry->m_ThreadTableEntry, curThread, pLock);

            return &entry->m_ThreadTableEntry;
        }

        next = next->Flink;
    }

    if( LookupOnly ) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Thread 0x%p does not have an entry",curThread);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        return NULL;
    }

    //
    // The current ETHREAD has no locks it currently holds, so it has
    // no entry in the hash table.
    //
    // Use the supplied locks m_ThreadTableEntry to create an entry
    // for this ETHREAD and the start of a new list of held locks.
    //
    pLock->m_ThreadTableEntry.Thread = curThread;
    pLock->m_ThreadTableEntry.PerThreadPassiveLockList = NULL;
    pLock->m_ThreadTableEntry.PerThreadDispatchLockList = NULL;

    InsertTailList(head, &pLock->m_ThreadTableEntry.HashChain);

//    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
//                        "Returning new Entry 0x%p for Thread 0x%p, Lock 0x%p",
//                        &pLock->m_ThreadTableEntry, curThread, pLock);

    return &pLock->m_ThreadTableEntry;
}

























































































void
FxVerifierLock::ReleaseOrReplaceThreadTableEntry(
    __in MxThread        curThread,
    __in FxVerifierLock* pLock
    )

/*++

Routine Description:

    Removes the use of the supplied locks m_ThreadTableEntry by
    either releasing it if the ETHREAD is holding no more locks,
    or by copying it to the m_ThreadTableEntry of another lock
    held by the thread.

Arguments:

    curThread - Thread who is holding lock

    pLock - Lock whose m_ThreadTableEntry is to be released

Returns:


Comments:

   This is called with the verifier hash table lock held
   FxDriverGlobals->ThreadTableLock.

   The pLock has already been removed from the held locks chain,
   so the lock at the head of the list can be used for the new hash
   table entry.

--*/

{
    ULONG Hash, Index;
    PLIST_ENTRY head;
    FxVerifierLock* pNewLock = NULL;

    PFX_DRIVER_GLOBALS FxDriverGlobals = pLock->GetDriverGlobals();

    if( pLock->m_ThreadTableEntry.Thread == NULL ) {

        // This locks entry is not used for hash chaining
        //DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
        //                    "Entry 0x%p Not currently part of the hash chain",
        //                     pLock);

        return;
    }

    // It should be the current thread
    if( pLock->m_ThreadTableEntry.Thread != curThread ) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "OldEntry Thread 0x%p not Current! 0x%p",
                            pLock,curThread);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }

    Hash = (ULONG)((ULONG_PTR)curThread >> 4);
    Hash = ((Hash >> 16) & 0x0000FFFF) ^ (Hash & 0x0000FFFF);

    Index = Hash & (VERIFIER_THREAD_HASHTABLE_SIZE-1);

    head = &FxDriverGlobals->ThreadTable[Index];

    // Remove old entry
    RemoveEntryList(&pLock->m_ThreadTableEntry.HashChain);

    //
    // If both lock lists are NULL, we can just release the entry
    //
    if( (pLock->m_ThreadTableEntry.PerThreadPassiveLockList == NULL) &&
        (pLock->m_ThreadTableEntry.PerThreadDispatchLockList == NULL) ) {

        //DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
        //                    "Releasing entry  for lock 0x%p for Thread 0x%p",
        //                    pLock, curThread);

        // This is now an unused entry
        pLock->m_ThreadTableEntry.Thread = NULL;
        pLock->m_ThreadTableEntry.PerThreadPassiveLockList = NULL;
        pLock->m_ThreadTableEntry.PerThreadDispatchLockList = NULL;

        return;
    }

    //DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
    //                    "Replacing Lock 0x%p, for Thread 0x%p",
    //                    pLock, curThread);
    if( pLock->m_ThreadTableEntry.PerThreadPassiveLockList != NULL ) {
        pNewLock = pLock->m_ThreadTableEntry.PerThreadPassiveLockList;
    }
    else {
        pNewLock = pLock->m_ThreadTableEntry.PerThreadDispatchLockList;
    }

    ASSERT(pNewLock != NULL);
    ASSERT(pNewLock->m_ThreadTableEntry.Thread == NULL);
    ASSERT(pNewLock->m_ThreadTableEntry.PerThreadPassiveLockList == NULL);
    ASSERT(pNewLock->m_ThreadTableEntry.PerThreadDispatchLockList == NULL);

    // Copy the old lock structures table to the next one
    pNewLock->m_ThreadTableEntry.Thread = pLock->m_ThreadTableEntry.Thread;
    pNewLock->m_ThreadTableEntry.PerThreadPassiveLockList = pLock->m_ThreadTableEntry.PerThreadPassiveLockList;
    pNewLock->m_ThreadTableEntry.PerThreadDispatchLockList = pLock->m_ThreadTableEntry.PerThreadDispatchLockList;

    // Insert new entry at the end of thelist
    InsertTailList(head, &pNewLock->m_ThreadTableEntry.HashChain);

    return;
}

void
FxVerifierLock::AllocateThreadTable(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    KIRQL       oldIrql;
    ULONG       newEntries;
    PLIST_ENTRY newTable;

    FxDriverGlobals->ThreadTableLock.Acquire(&oldIrql);

    if( FxDriverGlobals->ThreadTable != NULL ) {
        FxDriverGlobals->ThreadTableLock.Release(oldIrql);
        return;
    }

    // Table must be kept as a power of 2 for hash algorithm
    newEntries = VERIFIER_THREAD_HASHTABLE_SIZE;

    newTable = (PLIST_ENTRY) FxPoolAllocateWithTag(
        FxDriverGlobals,
        NonPagedPool,
        sizeof(LIST_ENTRY) * newEntries,
        FxDriverGlobals->Tag);

    if( newTable == NULL ) {
        FxDriverGlobals->ThreadTableLock.Release(oldIrql);
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "No Memory to allocate thread table");
        return;
    }

    for(ULONG index=0; index < newEntries; index++ ) {
        InitializeListHead(&newTable[index]);
    }

    FxDriverGlobals->ThreadTable     = newTable;

    FxDriverGlobals->ThreadTableLock.Release(oldIrql);

    return;
}

void
FxVerifierLock::FreeThreadTable(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(FxDriverGlobals);

    FxDriverGlobals->ThreadTableLock.Acquire(&oldIrql);

    if( FxDriverGlobals->ThreadTable == NULL ) {
        FxDriverGlobals->ThreadTableLock.Release(oldIrql);
        return;
    }

    FxPoolFree(FxDriverGlobals->ThreadTable);

    FxDriverGlobals->ThreadTable = NULL;

    FxDriverGlobals->ThreadTableLock.Release(oldIrql);

    return;
}

void
FxVerifierLock::DumpDetails(
    __in FxVerifierLock* Lock,
    __in MxThread        curThread,
    __in FxVerifierLock* PerThreadList
    )
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = Lock->GetDriverGlobals();

    FxVerifierLock* next;

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                        "Thread 0x%p Attempted to acquire lock on Object 0x%p, "
                        "ObjectType 0x%x, at Level 0x%x out of sequence.",
                        curThread,Lock->m_ParentObject,
                        Lock->m_ParentObject->GetType(),
                        Lock->m_Order);

    next = PerThreadList;

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                        "Highest Lock Currently held is level 0x%x for "
                        "Object 0x%p, ObjectType 0x%x",
                        next->m_Order,
                        next->m_ParentObject,
                        next->m_ParentObject->GetType());

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                        "List of Already Acquired Locks and Objects:");

    while( next != NULL ) {

        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Object 0x%p, ObjectType 0x%x, LockLevel 0x%x",
                            next->m_ParentObject,
                            next->m_ParentObject->GetType(),
                            next->m_Order);

        next = next->m_OwnedLink;
    }

    FxVerifierDbgBreakPoint(FxDriverGlobals);

    return;
}


