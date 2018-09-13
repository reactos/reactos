/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmplock.h

Abstract:

    Macros that hide the system calls used to do locking.  Allows
    cm and reg code to run in a variety of environments.

    Note that there is a single lock (in particular, a mutex) which
    protects the entire registry.

Author:

    Bryan M. Willman (bryanwi) 30-Oct-91

Environment:

Revision History:

--*/

//
// Macros for kernel mode environment
//

extern  KMUTEX  CmpRegistryMutex;
#if DBG
extern  LONG    CmpRegistryLockLocked;
#endif

//
// Test macro
//
#if DBG
#define ASSERT_CM_LOCK_OWNED() \
    if ( (CmpRegistryMutex.OwnerThread != KeGetCurrentThread())  ||   \
         (CmpRegistryMutex.Header.SignalState >= 1) )                 \
    {                                                                 \
        ASSERT(FALSE);                                                \
    }
#else
#define ASSERT_CM_LOCK_OWNED()
#endif

//
// This set of macros serializes all access to the registry via
// a single Mutex.
//

//
// CMP_LOCK_REGISTRY(
//      NTSTATUS        *pstatus,
//      PLARGE_INTEGER  timeout
//      );
//
//  Routine Description:
//
//      Acquires the CmpRegistryMutex, with specified timeout, and
//      returns status.
//
//  Arguments:
//
//      pstatus - pointer to variable to receive status from wait call
//
//      timeout - pointer to timeout value
//

#if DBG
#define CMP_LOCK_REGISTRY(status, timeout)                      \
{                                                               \
    status = KeWaitForSingleObject(                             \
                &CmpRegistryMutex,                              \
                Executive,                                      \
                KernelMode,                                     \
                FALSE,                                          \
                timeout                                         \
                );                                              \
    CmpRegistryLockLocked++;                                    \
}
#else
#define CMP_LOCK_REGISTRY(status, timeout)                      \
{                                                               \
    status = KeWaitForSingleObject(                             \
                &CmpRegistryMutex,                              \
                Executive,                                      \
                KernelMode,                                     \
                FALSE,                                          \
                timeout                                         \
                );                                              \
}
#endif

//
// CMP_UNLOCK_REGISTRY(
//      );
//
//  Routine Description:
//
//      Releases the CmpRegistryMutex.
//
//

#if DBG
#define CMP_UNLOCK_REGISTRY()                               \
{                                                           \
    ASSERT(CmpRegistryLockLocked > 0);                      \
    KeReleaseMutex(&CmpRegistryMutex, FALSE);               \
    CmpRegistryLockLocked--;                                \
}
#else
#define CMP_UNLOCK_REGISTRY()                               \
{                                                           \
    KeReleaseMutex(&CmpRegistryMutex, FALSE);               \
}
#endif


//
// Debugging asserts
//

#if DBG
#define ASSERT_REGISTRY_LOCKED()    ASSERT(CmpRegistryLockLocked > 0)
#else
#define ASSERT_REGISTRY_LOCKED()
#endif
