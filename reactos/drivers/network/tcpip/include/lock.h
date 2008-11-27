#ifndef _LOCK_H
#define _LOCK_H

extern KIRQL TcpipGetCurrentIrql();
extern VOID TcpipInitializeSpinLock( PKSPIN_LOCK SpinLock );
extern VOID TcpipAcquireSpinLock( PKSPIN_LOCK SpinLock, PKIRQL Irql );
extern VOID TcpipReleaseSpinLock( PKSPIN_LOCK SpinLock, KIRQL Irql );
extern VOID TcpipAcquireSpinLockAtDpcLevel( PKSPIN_LOCK SpinLock );
extern VOID TcpipReleaseSpinLockFromDpcLevel( PKSPIN_LOCK SpinLock );
extern VOID TcpipInterlockedInsertTailList( PLIST_ENTRY ListHead,
					    PLIST_ENTRY Item,
					    PKSPIN_LOCK Lock );
extern VOID TcpipAcquireFastMutex( PFAST_MUTEX Mutex );
extern VOID TcpipReleaseFastMutex( PFAST_MUTEX Mutex );
extern VOID TcpipRecursiveMutexInit( PRECURSIVE_MUTEX RecMutex );
extern UINT TcpipRecursiveMutexEnter( PRECURSIVE_MUTEX RecMutex,
				      BOOLEAN ToWrite );
extern VOID TcpipRecursiveMutexLeave( PRECURSIVE_MUTEX RecMutex );

#endif/*_LOCK_H*/
