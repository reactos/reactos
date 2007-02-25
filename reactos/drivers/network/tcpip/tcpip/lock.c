/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/lock.c
 * PURPOSE:     Locking and unlocking
 * PROGRAMMERS: Art Yerkes
 * REVISIONS:
 */
#include "precomp.h"

KIRQL TcpipGetCurrentIrql() { return KeGetCurrentIrql(); }

VOID TcpipInitializeSpinLock( PKSPIN_LOCK SpinLock ) {
    KeInitializeSpinLock( SpinLock );
}

VOID TcpipAcquireSpinLock( PKSPIN_LOCK SpinLock, PKIRQL Irql ) {
    KeAcquireSpinLock( SpinLock, Irql );
}

VOID TcpipAcquireSpinLockAtDpcLevel( PKSPIN_LOCK SpinLock ) {
    KeAcquireSpinLockAtDpcLevel( SpinLock );
}

VOID TcpipReleaseSpinLock( PKSPIN_LOCK SpinLock, KIRQL Irql ) {
    KeReleaseSpinLock( SpinLock, Irql );
}

VOID TcpipReleaseSpinLockFromDpcLevel( PKSPIN_LOCK SpinLock ) {
    KeReleaseSpinLockFromDpcLevel( SpinLock );
}

VOID TcpipInterlockedInsertTailList( PLIST_ENTRY ListHead,
				     PLIST_ENTRY Item,
				     PKSPIN_LOCK Lock ) {
    ExInterlockedInsertTailList( ListHead, Item, Lock );
}

VOID TcpipAcquireFastMutex( PFAST_MUTEX Mutex ) {
    ExAcquireFastMutex( Mutex );
}

VOID TcpipReleaseFastMutex( PFAST_MUTEX Mutex ) {
    ExReleaseFastMutex( Mutex );
}

VOID TcpipRecursiveMutexInit( PRECURSIVE_MUTEX RecMutex ) {
    RecursiveMutexInit( RecMutex );
}

UINT TcpipRecursiveMutexEnter( PRECURSIVE_MUTEX RecMutex, BOOL ToWrite ) {
    UINT Ret;
    TI_DbgPrint(DEBUG_LOCK,("Locking\n"));
    Ret = RecursiveMutexEnter( RecMutex, ToWrite );
    TI_DbgPrint(DEBUG_LOCK,("Locked\n"));
    return Ret;
}

VOID TcpipRecursiveMutexLeave( PRECURSIVE_MUTEX RecMutex ) {
    TI_DbgPrint(DEBUG_LOCK,("Unlocking\n"));
    RecursiveMutexLeave( RecMutex );
}
