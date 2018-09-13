/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    reslock.hxx

Abstract:

    Contains RESOURCE_LOCK class

Author:

    Richard L Firth (rfirth) 18-Jun-1996

Revision History:

    18-Jun-1996 rfirth
        Created

--*/

//
// manifests
//

#if INET_DEBUG

#define DEFAULT_RESOURCE_LOCK_TIMEOUT   30000   // 30 seconds

#else

#define DEFAULT_RESOURCE_LOCK_TIMEOUT   INFINITE

#endif

//
// class definition
//

class RESOURCE_LOCK {

private:

    //
    // _Initialized - TRUE when the critical section & event have been created
    //

    BOOL _Initialized;

    //
    // _WriteCount - number of times writer has re-entered
    //

    LONG _WriteCount;

    //
    // _ThreadId - ID of the owning writer thread
    //

    DWORD _ThreadId;

    //
    // _Readers - number of reader threads. Used as a switch in concert with
    // InterlockedIncrement/InterlockedDecrement: -1 => 0 means this is the
    // first reader, and we reset the writer wait event; 0 => -1 means this is
    // the last reader and we set the writer wait event
    //

    LONG _Readers;

    //
    // _Timeout - number of milliseconds to wait for one of the event handles
    //

    DWORD _Timeout;

    //
    // _hWriteEvent - signalled when the last reader thread exits. At this time
    // the writer thread has exclusive access
    //

    HANDLE _hWriteEvent;

    //
    // _CritSect - used to serialize access. Writer keeps this until Releas()ed.
    // Readers just keep it long enough to update _Readers variable
    //

    CRITICAL_SECTION _CritSect;

    //
    // Wait - wait for an event to become signalled
    //

    BOOL Wait(HANDLE hEvent) {

        DWORD error = WaitForSingleObject(hEvent, _Timeout);

        if (error == WAIT_OBJECT_0) {
            return TRUE;
        } else {

            DEBUG_PRINT(UTIL,
                        ERROR,
                        ("RESOURCE_LOCK::Wait(): WaitForSingleObject() returns %d\n",
                        error
                        ));

            if (error == WAIT_TIMEOUT) {
                error = ERROR_INTERNET_TIMEOUT;
            } else {
                error = ERROR_INTERNET_INTERNAL_ERROR;
            }
            SetLastError(error);
            return FALSE;
        }
    }

    VOID SetTimeout(DWORD Timeout) {
        _Timeout = Timeout;
    }

    DWORD GetTimeout(VOID) const {
        return _Timeout;
    }

public:

    RESOURCE_LOCK() {
        _WriteCount = 0;
        _ThreadId = 0;
        _Readers = -1;
        _Timeout = DEFAULT_RESOURCE_LOCK_TIMEOUT;
        _hWriteEvent = NULL;
        _Initialized = FALSE;
    }

    ~RESOURCE_LOCK() {

        INET_ASSERT(_WriteCount == 0);
        INET_ASSERT(_ThreadId == 0);
        INET_ASSERT(_Readers == -1);

        if (_Initialized) {
            DeleteCriticalSection(&_CritSect);
            if (_hWriteEvent != NULL) {
                CloseHandle(_hWriteEvent);
            }
        }
    }

    VOID Initialize(VOID) {
        _WriteCount = 0;
        _ThreadId = 0;
        _Readers = -1;
        _Timeout = DEFAULT_RESOURCE_LOCK_TIMEOUT;
        InitializeCriticalSection(&_CritSect);

        //
        // _hWriteEvent is an auto-reset, initially-signalled event
        //

        _hWriteEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
        _Initialized = TRUE;
    }

    BOOL IsInitialized(VOID) const {
        return _Initialized;
    }

    BOOL IsValid(VOID) const {
        return (_hWriteEvent != NULL) ? TRUE : FALSE;
    }

    BOOL
    Acquire(
        IN BOOL bExclusiveMode = FALSE
        );

    BOOL AcquireExclusive(VOID) {
        return Acquire(TRUE);
    }

    VOID
    Release(
        VOID
        );
};
