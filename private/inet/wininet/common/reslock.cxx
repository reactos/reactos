/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    reslock.cxx

Abstract:

    Contains methods for RESOURCE_LOCK class

    Contents:
        RESOURCE_LOCK::Acquire()
        RESOURCE_LOCK::Release()

Author:

    Richard L Firth (rfirth) 18-Jun-1996

Revision History:

    18-Jun-1996 rfirth
        Created

--*/

#include <wininetp.h>

//
// class members
//

#ifdef OLD_VERSION


BOOL
RESOURCE_LOCK::Acquire(
    IN BOOL bExclusiveMode
    )

/*++

Routine Description:

    Acquires the resource protected by this lock. Acquires for non-exclusive
    (read) or exclusive (write) ownership

Arguments:

    bExclusiveMode  - TRUE if we are acquiring the resource for exclusive
                      (write) ownership

Return Value:

    BOOL
        TRUE    - resource is acquired

        FALSE   - failed to acquire resource (timeout?)

--*/

{
    DEBUG_ENTER((DBG_RESLOCK,
                Bool,
                "RESOURCE_LOCK::Acquire",
                "%B",
                bExclusiveMode
                ));

    INET_ASSERT(this != NULL);
    //INET_ASSERT(IsInitialized());
    //INET_ASSERT(IsValid());

    if (!IsInitialized()) {

        DEBUG_LEAVE(FALSE);

        return FALSE;
    }

    BOOL acquired = TRUE;

    //EnterCriticalSection(&_CritSect);
    if (bExclusiveMode) {

        //
        // acquired for exclusive ownership (write access). Set the owning
        // thread id and wait for the last current reader to release. Note
        // that if we're being re-entered, EnterCriticalSection() has already
        // done the work of checking the thread id and updating re-entrancy
        // counts, so if its already not zero, we know it must be us
        //

        ++_WriteCount;
        if (_ThreadId == 0) {
            _ThreadId = GetCurrentThreadId();
#if INET_DEBUG
            INET_ASSERT(_ThreadId != _ThreadIdReader);
#endif
            acquired = Wait(_hWriteEvent);
            EnterCriticalSection(&_CritSect);
        } else {

            INET_ASSERT(_ThreadId == GetCurrentThreadId());

        }
    } else {

        //
        // don't allow re-entry if already held for exclusive access
        //

        INET_ASSERT(_ThreadId == 0);

        //
        // acquired for non-exclusive ownership (read access). Just increase
        // the number of active readers. If this is the first then inhibit the
        // writer
        //

        if (InterlockedIncrement(&_Readers) == 0) {
#if INET_DEBUG
            if (_ThreadIdReader == 0) {
                _ThreadIdReader = GetCurrentThreadId();
            }
#endif
            ResetEvent(_hWriteEvent);
        }

        //
        // reader doesn't need to keep hold of critical section
        //

        //LeaveCriticalSection(&_CritSect);
    }

    DEBUG_LEAVE(acquired);

    return acquired;
}


VOID
RESOURCE_LOCK::Release(
    VOID
    )

/*++

Routine Description:

    Releases a resource previously acquired by RESOURCE_LOCK::Acquire()

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_RESLOCK,
                None,
                "RESOURCE_LOCK::Release",
                NULL
                ));

    INET_ASSERT(this != NULL);
    //INET_ASSERT(IsInitialized());
    //INET_ASSERT(IsValid());

    if (!IsInitialized()) {

        DEBUG_LEAVE(0);

        return;
    }

    if ((_ThreadId != 0) && (_ThreadId == GetCurrentThreadId())) {

        INET_ASSERT(_WriteCount > 0);

        if (--_WriteCount == 0) {

            //
            // we acquired _hWriteEvent; signal it to allow next writer to continue
            //

            SetEvent(_hWriteEvent);

            //
            // this resource no longer owned for exclusive access
            //

            _ThreadId = 0;
        }
        LeaveCriticalSection(&_CritSect);
    } else if (InterlockedDecrement(&_Readers) < 0) {

        INET_ASSERT(_Readers >= -1);

        //
        // we are last currently active reader; allow waiting writer to continue
        //

#if INET_DEBUG
        if (_ThreadIdReader == GetCurrentThreadId()) {
            _ThreadIdReader = 0;
        }
#endif
        SetEvent(_hWriteEvent);
    }

    DEBUG_LEAVE(0);
}

#else

BOOL
RESOURCE_LOCK::Acquire(
    IN BOOL bExclusiveMode
    )
{
    DEBUG_ENTER((DBG_RESLOCK,
                Bool,
                "RESOURCE_LOCK::Acquire",
                "%B",
                bExclusiveMode
                ));

    if (!IsInitialized()) {

        DEBUG_LEAVE(FALSE);

        return FALSE;
    }

    if (bExclusiveMode) {
        do {

            DEBUG_PRINT(RESLOCK,
                        INFO,
                        ("Waiting on WriteEvent\n")
                        );

            if (_ThreadId != GetCurrentThreadId()) {
                Wait(_hWriteEvent);
            }
            EnterCriticalSection(&_CritSect);

            INET_ASSERT((_ThreadId == 0) || (_ThreadId == GetCurrentThreadId()));

            if ((_Readers == -1)
                && ((_ThreadId == 0) || (_ThreadId == GetCurrentThreadId()))) {
                _ThreadId = GetCurrentThreadId();
                if (++_WriteCount == 1) {
                    ResetEvent(_hWriteEvent);
                }
                break;
            }

            DEBUG_PRINT(RESLOCK,
                        INFO,
                        ("trying again\n")
                        );

            LeaveCriticalSection(&_CritSect);
        } while ( 1 );
    } else {
        EnterCriticalSection(&_CritSect);
        if (++_Readers == 0) {

            DEBUG_PRINT(RESLOCK,
                        INFO,
                        ("Resetting WriteEvent\n")
                        );

            ResetEvent(_hWriteEvent);
        }
        LeaveCriticalSection(&_CritSect);
    }

    DEBUG_LEAVE(TRUE);

    return TRUE;
}

VOID
RESOURCE_LOCK::Release(
    VOID
    )
{
    DEBUG_ENTER((DBG_RESLOCK,
                None,
                "RESOURCE_LOCK::Release",
                NULL
                ));

    if (IsInitialized()) {
        if (_ThreadId == GetCurrentThreadId()) {

            DEBUG_PRINT(RESLOCK,
                        INFO,
                        ("Clearing writer\n")
                        );

            if (--_WriteCount == 0) {
                _ThreadId = 0;
                SetEvent(_hWriteEvent);
            }
            LeaveCriticalSection(&_CritSect);
        } else {
            EnterCriticalSection(&_CritSect);
            if (--_Readers == -1) {

                DEBUG_PRINT(RESLOCK,
                            INFO,
                            ("Setting WriteEvent\n")
                            );

                SetEvent(_hWriteEvent);
            }
            LeaveCriticalSection(&_CritSect);
        }
    }

    DEBUG_LEAVE(0);
}

#endif // OLD_VERSION
