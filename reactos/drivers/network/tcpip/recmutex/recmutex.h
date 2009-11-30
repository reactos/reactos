#ifndef _ROSRTL_RECMUTEX_H
#define _ROSRTL_RECMUTEX_H

typedef struct _RECURSIVE_MUTEX {
    /* Lock. */
    FAST_MUTEX Mutex;
    /* Number of times this object was locked */
    SIZE_T LockCount;
    /* CurrentThread */
    PVOID CurrentThread;
    /* Notification event which signals that another thread can take over */
    KEVENT StateLockedEvent;
} RECURSIVE_MUTEX, *PRECURSIVE_MUTEX;

extern VOID RecursiveMutexInit( PRECURSIVE_MUTEX RecMutex );
extern VOID RecursiveMutexEnter( PRECURSIVE_MUTEX RecMutex );
extern VOID RecursiveMutexLeave( PRECURSIVE_MUTEX RecMutex );

#define ASSERT_LOCKED(x)

#endif/*_ROSRTL_RECMUTEX_H*/
