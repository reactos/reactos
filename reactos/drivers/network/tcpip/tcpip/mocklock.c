/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/lock.c
 * PURPOSE:     Locking and unlocking
 * PROGRAMMERS: Art Yerkes
 * REVISIONS:
 */
#include "precomp.h"

KIRQL KernelIrql = PASSIVE_LEVEL;

KIRQL TcpipGetCurrentIrql() { return KernelIrql; }

VOID TcpipInitializeSpinLock( PKSPIN_LOCK SpinLock ) {
}

VOID TcpipAcquireSpinLock( PKSPIN_LOCK SpinLock, PKIRQL Irql ) {
    *Irql = KernelIrql;
    KernelIrql = DISPATCH_LEVEL;
}

VOID TcpipAcquireSpinLockAtDpcLevel( PKSPIN_LOCK SpinLock ) {
    ASSERT(KernelIrql == DISPATCH_LEVEL);
}

VOID TcpipReleaseSpinLock( PKSPIN_LOCK SpinLock, KIRQL Irql ) {
    ASSERT( Irql <= KernelIrql );
    KernelIrql = Irql;
}

VOID TcpipReleaseSpinLockFromDpcLevel( PKSPIN_LOCK SpinLock ) {
    ASSERT(KernelIrql == DISPATCH_LEVEL);
}

VOID TcpipInterlockedInsertTailList( PLIST_ENTRY ListHead,
				     PLIST_ENTRY Item,
				     PKSPIN_LOCK Lock ) {
    InsertTailList( ListHead, Item );
}

VOID TcpipAcquireFastMutex( PFAST_MUTEX Mutex ) {
}

VOID TcpipReleaseFastMutex( PFAST_MUTEX Mutex ) {
}

VOID TcpipRecursiveMutexInit( PRECURSIVE_MUTEX RecMutex ) {
}

UINT TcpipRecursiveMutexEnter( PRECURSIVE_MUTEX RecMutex, BOOL ToWrite ) {
    return 0;
}

VOID TcpipRecursiveMutexLeave( PRECURSIVE_MUTEX RecMutex ) {
}
