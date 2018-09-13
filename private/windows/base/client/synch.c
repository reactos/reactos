/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    synch.c

Abstract:

    This module implements all Win32 syncronization
    objects.

Author:

    Mark Lucovsky (markl) 19-Sep-1990

Revision History:

--*/

#include "basedll.h"

//
// Critical Section Services
//

VOID
InitializeCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
    )

/*++

Routine Description:

    A critical section object is initialized using
    Win32InitializeCriticalSection.

    Once a critical section object has been initialized threads within a
    single process may enter and exit critical sections using the
    critical section object.  A critical section object may not me moved
    or copied.  The application must also not modify the insides of the
    object, but must treat it as logically opaque.
    description-of-function.

Arguments:

    lpCriticalSection - Supplies the address of a critical section object
        to be initialized.  It is the callers resposibility to allocate
        the storage used by a critical section object.

Return Value:

    None.

--*/

{
    NTSTATUS Status;

    Status = RtlInitializeCriticalSection(lpCriticalSection);
    if ( !NT_SUCCESS(Status) ){
        RtlRaiseStatus(Status);
        }
}

BOOL
InitializeCriticalSectionAndSpinCount(
    LPCRITICAL_SECTION lpCriticalSection,
    DWORD dwSpinCount
    )

/*++

Routine Description:

    A critical section object is initialized using
    Win32InitializeCriticalSection.

    Once a critical section object has been initialized threads within a
    single process may enter and exit critical sections using the
    critical section object.  A critical section object may not me moved
    or copied.  The application must also not modify the insides of the
    object, but must treat it as logically opaque.
    description-of-function.

Arguments:

    lpCriticalSection - Supplies the address of a critical section object
        to be initialized.  It is the callers resposibility to allocate
        the storage used by a critical section object.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    BOOL rv;

    rv = TRUE;
    Status = RtlInitializeCriticalSectionAndSpinCount(lpCriticalSection,dwSpinCount);
    if ( !NT_SUCCESS(Status) ){
        BaseSetLastNTError(Status);
        rv = FALSE;
        }
    return rv;
}



//
// Event Services
//
HANDLE
APIENTRY
CreateEventA(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to CreateEventW


--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    LPCWSTR NameBuffer;

    NameBuffer = NULL;
    if ( ARGUMENT_PRESENT(lpName) ) {
        Unicode = &NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString,lpName);
        Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                }
            else {
                BaseSetLastNTError(Status);
                }
            return NULL;
            }
        NameBuffer = (LPCWSTR)Unicode->Buffer;
        }

    return CreateEventW(
                lpEventAttributes,
                bManualReset,
                bInitialState,
                NameBuffer
                );
}


HANDLE
APIENTRY
CreateEventW(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    )

/*++

Routine Description:

    An event object is created and a handle opened for access to the
    object with the CreateEvent function.

    The CreateEvent function creates an event object with the specified
    initial state.  If an event is in the Signaled state (TRUE), a wait
    operation on the event does not block.  If the event is in the Not-
    Signaled state (FALSE), a wait operation on the event blocks until
    the specified event attains a state of Signaled, or the timeout
    value is exceeded.

    In addition to the STANDARD_RIGHTS_REQUIRED access flags, the following
    object type specific access flags are valid for event objects:

        - EVENT_MODIFY_STATE - Modify state access (set and reset) to
          the event is desired.

        - SYNCHRONIZE - Synchronization access (wait) to the event is
          desired.

        - EVENT_ALL_ACCESS - This set of access flags specifies all of
          the possible access flags for an event object.


Arguments:

    lpEventAttributes - An optional parameter that may be used to
        specify the attributes of the new event.  If the parameter is
        not specified, then the event is created without a security
        descriptor, and the resulting handle is not inherited on process
        creation.

    bManualReset - Supplies a flag which if TRUE specifies that the
        event must be manually reset.  If the value is FALSE, then after
        releasing a single waiter, the system automaticaly resets the
        event.

    bInitialState - The initial state of the event object, one of TRUE
        or FALSE.  If the InitialState is specified as TRUE, the event's
        current state value is set to one, otherwise it is set to zero.

    lpName - Optional unicode name of event

Return Value:

    NON-NULL - Returns a handle to the new event.  The handle has full
        access to the new event and may be used in any API that requires
        a handle to an event object.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    POBJECT_ATTRIBUTES pObja;
    HANDLE Handle;
    UNICODE_STRING ObjectName;
    PWCHAR pstrNewObjName = NULL;

    if ( ARGUMENT_PRESENT(lpName) ) {

        if (gpTermsrvFormatObjectName && 
            (pstrNewObjName = gpTermsrvFormatObjectName(lpName))) {

            RtlInitUnicodeString(&ObjectName,pstrNewObjName);

        } else {

            RtlInitUnicodeString(&ObjectName,lpName);
        }

        pObja = BaseFormatObjectAttributes(&Obja,lpEventAttributes,&ObjectName);
        }
    else {
        pObja = BaseFormatObjectAttributes(&Obja,lpEventAttributes,NULL);
        }

    Status = NtCreateEvent(
                &Handle,
                EVENT_ALL_ACCESS,
                pObja,
                bManualReset ? NotificationEvent : SynchronizationEvent,
                (BOOLEAN)bInitialState
                );

    if (pstrNewObjName) {
        RtlFreeHeap(RtlProcessHeap(), 0, pstrNewObjName);
    }

    if ( NT_SUCCESS(Status) ) {
        if ( Status == STATUS_OBJECT_NAME_EXISTS ) {
            SetLastError(ERROR_ALREADY_EXISTS);
            }
        else {
            SetLastError(0);
            }
        return Handle;
        }
    else {
        BaseSetLastNTError(Status);
        return NULL;
        }
}

HANDLE
APIENTRY
OpenEventA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to OpenNamedEventW

--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    if ( ARGUMENT_PRESENT(lpName) ) {
        Unicode = &NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString,lpName);
        Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                }
            else {
                BaseSetLastNTError(Status);
                }
            return NULL;
            }
        }
    else {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return NULL;
        }

    return OpenEventW(
                dwDesiredAccess,
                bInheritHandle,
                (LPCWSTR)Unicode->Buffer
                );
}

HANDLE
APIENTRY
OpenEventW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING ObjectName;
    NTSTATUS Status;
    HANDLE Object;
    PWCHAR pstrNewObjName = NULL;

    if ( !lpName ) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return NULL;
        }
    
    if (gpTermsrvFormatObjectName && 
        (pstrNewObjName = gpTermsrvFormatObjectName(lpName))) {

        RtlInitUnicodeString(&ObjectName,pstrNewObjName);

    } else {

        RtlInitUnicodeString(&ObjectName,lpName);
    }


    InitializeObjectAttributes(
        &Obja,
        &ObjectName,
        (bInheritHandle ? OBJ_INHERIT : 0),
        BaseGetNamedObjectDirectory(),
        NULL
        );

    Status = NtOpenEvent(
                &Object,
                dwDesiredAccess,
                &Obja
                );
    
    if (pstrNewObjName) {
        RtlFreeHeap(RtlProcessHeap(), 0, pstrNewObjName);
    }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return NULL;
        }
    return Object;
}

BOOL
SetEvent(
    HANDLE hEvent
    )

/*++

Routine Description:

    An event can be set to the signaled state (TRUE) with the SetEvent
    function.

    Setting the event causes the event to attain a state of Signaled,
    which releases all currently waiting threads (for manual reset
    events), or a single waiting thread (for automatic reset events).

Arguments:

    hEvent - Supplies an open handle to an event object.  The
        handle must have EVENT_MODIFY_STATE access to the event.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;

    Status = NtSetEvent(hEvent,NULL);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
ResetEvent(
    HANDLE hEvent
    )

/*++

Routine Description:

    The state of an event is set to the Not-Signaled state (FALSE) using
    the ClearEvent function.

    Once the event attains a state of Not-Signaled, any threads which
    wait on the event block, awaiting the event to become Signaled.  The
    reset event service sets the event count to zero for the state of
    the event.

Arguments:

    hEvent - Supplies an open handle to an event object.  The
        handle must have EVENT_MODIFY_STATE access to the event.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;

    Status = NtClearEvent(hEvent);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
PulseEvent(
    HANDLE hEvent
    )

/*++

Routine Description:

    An event can be set to the Signaled state and reset to the Not-
    Signaled state atomically with the PulseEvent function.

    Pulsing the event causes the event to attain a state of Signaled,
    release appropriate threads, and then reset the event.  When no
    waiters are currently waiting on the event, pulsing an event causes
    the event to release no threads and end up in the Not-Signaled
    state.  With waiters waiting on an event, pulsing an event has a
    different effect for manual reset events that it does for automatic
    reset events.  For manual reset events, pulsing releases all waiters
    and then leaves the event in the Not-Signaled state.  For automatic
    reset events, pulsing the event releases a single waiter and then
    leaves the event in the Not-Signaled state.

Arguments:

    hEvent - Supplies an open handle to an event object.  The
        handle must have EVENT_MODIFY_STATE access to the event.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;

    Status = NtPulseEvent(hEvent,NULL);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}


//
// Semaphore Services
//

HANDLE
APIENTRY
CreateSemaphoreA(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    LONG lInitialCount,
    LONG lMaximumCount,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to CreateSemaphoreW


--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    LPCWSTR NameBuffer;

    NameBuffer = NULL;
    if ( ARGUMENT_PRESENT(lpName) ) {
        Unicode = &NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString,lpName);
        Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                }
            else {
                BaseSetLastNTError(Status);
                }
            return NULL;
            }
        NameBuffer = (LPCWSTR)Unicode->Buffer;
        }

    return CreateSemaphoreW(
                lpSemaphoreAttributes,
                lInitialCount,
                lMaximumCount,
                NameBuffer
                );
}


HANDLE
APIENTRY
CreateSemaphoreW(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    LONG lInitialCount,
    LONG lMaximumCount,
    LPCWSTR lpName
    )

/*++

Routine Description:

    A semaphore object is created and a handle opened for access to the
    object with the CreateSemaphore function.

    The CreateSemaphore function causes a semaphore object to be created
    which contains the specified initial and maximum counts.

    In addition to the STANDARD_RIGHTS_REQUIRED access flags, the
    following object type specific access flags are valid for semaphore
    objects:

        - SEMAPHORE_MODIFY_STATE - Modify state access (release) to the
            semaphore is desired.

        - SYNCHRONIZE - Synchronization access (wait) to the semaphore
            is desired.

        - SEMAPHORE_ALL_ACCESS - This set of access flags specifies all
            of the possible access flags for a semaphore object.


Arguments:

    lpSemaphoreAttributes - An optional parameter that may be used to
        specify the attributes of the new semaphore.  If the parameter
        is not specified, then the semaphore is created without a
        security descriptor, , and the resulting handle is not inherited
        on process creation.

    lInitialCount - The initial count for the semaphore, this value
        must be positive and less than or equal to the maximum count.

    lMaximumCount - The maximum count for the semaphore, this value
        must be greater than zero..

    lpName - Supplies an optional unicode name for the object.

Return Value:

    NON-NULL - Returns a handle to the new semaphore.  The handle has
        full access to the new semaphore and may be used in any API that
        requires a handle to a semaphore object.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    POBJECT_ATTRIBUTES pObja;
    HANDLE Handle;
    UNICODE_STRING ObjectName;
    PWCHAR pstrNewObjName = NULL;

    if ( ARGUMENT_PRESENT(lpName) ) {

        if (gpTermsrvFormatObjectName && 
            (pstrNewObjName = gpTermsrvFormatObjectName(lpName))) {

            RtlInitUnicodeString(&ObjectName,pstrNewObjName);

        } else {

            RtlInitUnicodeString(&ObjectName,lpName);
        }
        pObja = BaseFormatObjectAttributes(&Obja,lpSemaphoreAttributes,&ObjectName);
        }
    else {
        pObja = BaseFormatObjectAttributes(&Obja,lpSemaphoreAttributes,NULL);
        }

    Status = NtCreateSemaphore(
                &Handle,
                SEMAPHORE_ALL_ACCESS,
                pObja,
                lInitialCount,
                lMaximumCount
                );
    
    if (pstrNewObjName) {
        RtlFreeHeap(RtlProcessHeap(), 0, pstrNewObjName);
    }

    if ( NT_SUCCESS(Status) ) {
        if ( Status == STATUS_OBJECT_NAME_EXISTS ) {
            SetLastError(ERROR_ALREADY_EXISTS);
            }
        else {
            SetLastError(0);
            }
        return Handle;
        }
    else {
        BaseSetLastNTError(Status);
        return NULL;
        }
}


HANDLE
APIENTRY
OpenSemaphoreA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to OpenSemaphoreW

--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    if ( ARGUMENT_PRESENT(lpName) ) {
        Unicode = &NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString,lpName);
        Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                }
            else {
                BaseSetLastNTError(Status);
                }
            return NULL;
            }
        }
    else {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return NULL;
        }

    return OpenSemaphoreW(
                dwDesiredAccess,
                bInheritHandle,
                (LPCWSTR)Unicode->Buffer
                );
}

HANDLE
APIENTRY
OpenSemaphoreW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING ObjectName;
    NTSTATUS Status;
    HANDLE Object;
    PWCHAR pstrNewObjName = NULL;

    if ( !lpName ) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return NULL;
        }

    if (gpTermsrvFormatObjectName && 
        (pstrNewObjName = gpTermsrvFormatObjectName(lpName))) {

        RtlInitUnicodeString(&ObjectName,pstrNewObjName);

    } else {

        RtlInitUnicodeString(&ObjectName,lpName);
    }

    InitializeObjectAttributes(
        &Obja,
        &ObjectName,
        (bInheritHandle ? OBJ_INHERIT : 0),
        BaseGetNamedObjectDirectory(),
        NULL
        );

    Status = NtOpenSemaphore(
                &Object,
                dwDesiredAccess,
                &Obja
                );

    if (pstrNewObjName) {
        RtlFreeHeap(RtlProcessHeap(), 0, pstrNewObjName);
    }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return NULL;
        }
    return Object;
}


BOOL
ReleaseSemaphore(
    HANDLE hSemaphore,
    LONG lReleaseCount,
    LPLONG lpPreviousCount
    )

/*++

Routine Description:

    A semaphore object can be released with the ReleaseSemaphore
    function.

    When the semaphore is released, the current count of the semaphore
    is incremented by the ReleaseCount.  Any threads that are waiting
    for the semaphore are examined to see if the current semaphore value
    is sufficient to satisfy their wait.

    If the value specified by ReleaseCount would cause the maximum count
    for the semaphore to be exceeded, then the count for the semaphore
    is not affected and an error status is returned.


Arguments:

    hSemaphore - Supplies an open handle to a semaphore object.  The
        handle must have SEMAPHORE_MODIFY_STATE access to the semaphore.

    lReleaseCount - The release count for the semaphore.  The count
        must be greater than zero and less than the maximum value
        specified for the semaphore.

    lpPreviousCount - An optional pointer to a variable that receives
        the previous count for the semaphore.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;

    Status = NtReleaseSemaphore(hSemaphore,lReleaseCount,lpPreviousCount);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}



//
// Mutex Services
//

HANDLE
APIENTRY
CreateMutexA(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    BOOL bInitialOwner,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to CreateMutexW

--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    LPCWSTR NameBuffer;

    NameBuffer = NULL;
    if ( ARGUMENT_PRESENT(lpName) ) {
        Unicode = &NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString,lpName);
        Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                }
            else {
                BaseSetLastNTError(Status);
                }
            return NULL;
            }

        NameBuffer = (LPCWSTR)Unicode->Buffer;
        }

    return CreateMutexW(
                lpMutexAttributes,
                bInitialOwner,
                NameBuffer
                );
}

HANDLE
APIENTRY
CreateMutexW(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    BOOL bInitialOwner,
    LPCWSTR lpName
    )

/*++

Routine Description:

    A mutex object can be created and a handle opened for access to the
    object with the CreateMutex function.

    A new mutex object is created and a handle opened to the object with
    ownership as determined by the InitialOwner parameter.  The status
    of the newly created mutex object is set to not abandoned.

    In addition to the STANDARD_RIGHTS_REQUIRED access flags, the
    following object type specific access flags are valid for mutex
    objects:

        - MUTEX_MODIFY_STATE - Modify access to the mutex is desired.
          This allows a process to release a mutex.

        - SYNCHRONIZE - Synchronization access (wait or release) to the
          mutex object is desired.

        - MUTEX_ALL_ACCESS - All possible types of access to the mutex
          object are desired.


Arguments:

    lpMutexAttributes - An optional parameter that may be used to specify
        the attributes of the new mutex.  If the parameter is not
        specified, then the mutex is created without a security
        descriptor, and the resulting handle is not inherited on process
        creation.

    bInitialOwner - A boolean value that determines whether the creator
        of the object desires immediate ownership of the mutex object.


    lpName - Supplies an optional unicode name for the mutex.

Return Value:

    NON-NULL - Returns a handle to the new mutex.  The handle has full
        access to the new mutex and may be used in any API that
        requires a handle to a mutex object.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    POBJECT_ATTRIBUTES pObja;
    HANDLE Handle;
    UNICODE_STRING ObjectName;
    PWCHAR pstrNewObjName = NULL;

    if ( ARGUMENT_PRESENT(lpName) ) {

        if (gpTermsrvFormatObjectName && 
            (pstrNewObjName = gpTermsrvFormatObjectName(lpName))) {

            RtlInitUnicodeString(&ObjectName,pstrNewObjName);

        } else {

            RtlInitUnicodeString(&ObjectName,lpName);
        }
        pObja = BaseFormatObjectAttributes(&Obja,lpMutexAttributes,&ObjectName);
        }
    else {
        pObja = BaseFormatObjectAttributes(&Obja,lpMutexAttributes,NULL);
        }


    Status = NtCreateMutant(
                &Handle,
                MUTANT_ALL_ACCESS,
                pObja,
                (BOOLEAN)bInitialOwner
                );

    if (pstrNewObjName) {
        RtlFreeHeap(RtlProcessHeap(), 0, pstrNewObjName);
    }
    
    if ( NT_SUCCESS(Status) ) {
        if ( Status == STATUS_OBJECT_NAME_EXISTS ) {
            SetLastError(ERROR_ALREADY_EXISTS);
            }
        else {
            SetLastError(0);
            }
        return Handle;
        }
    else {
        BaseSetLastNTError(Status);
        return NULL;
        }
}

HANDLE
APIENTRY
OpenMutexA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to OpenMutexW

--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    if ( ARGUMENT_PRESENT(lpName) ) {
        Unicode = &NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString,lpName);
        Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                }
            else {
                BaseSetLastNTError(Status);
                }
            return NULL;
            }
        }
    else {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return NULL;
        }

    return OpenMutexW(
                dwDesiredAccess,
                bInheritHandle,
                (LPCWSTR)Unicode->Buffer
                );
}

HANDLE
APIENTRY
OpenMutexW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING ObjectName;
    NTSTATUS Status;
    HANDLE Object;
    PWCHAR pstrNewObjName = NULL;

    if ( !lpName ) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return NULL;
        }

    if (gpTermsrvFormatObjectName && 
        (pstrNewObjName = gpTermsrvFormatObjectName(lpName))) {

        RtlInitUnicodeString(&ObjectName,pstrNewObjName);

    } else {

        RtlInitUnicodeString(&ObjectName,lpName);
    }

    InitializeObjectAttributes(
        &Obja,
        &ObjectName,
        (bInheritHandle ? OBJ_INHERIT : 0),
        BaseGetNamedObjectDirectory(),
        NULL
        );

    Status = NtOpenMutant(
                &Object,
                dwDesiredAccess,
                &Obja
                );

    if (pstrNewObjName) {
        RtlFreeHeap(RtlProcessHeap(), 0, pstrNewObjName);
    }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return NULL;
        }
    return Object;
}

BOOL
ReleaseMutex(
    HANDLE hMutex
    )

/*++

Routine Description:

    Ownership of a mutex object can be released with the ReleaseMutex
    function.

    A mutex object can only be released by a thread that currently owns
    the mutex object.  When the mutex is released, the current count of
    the mutex object is incremented by one.  If the resultant count is
    one, then the mutex object is no longer owned.  Any threads that are
    waiting for the mutex object are examined to see if their wait can
    be satisfied.

Arguments:

    hMutex - An open handle to a mutex object.  The handle must
        have MUTEX_MODIFY_STATE access to the mutex.

Return Value:

    TRUE - The operation was successful

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;

    Status = NtReleaseMutant(hMutex,NULL);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}


//
// Wait Services
//

DWORD
WaitForSingleObject(
    HANDLE hHandle,
    DWORD dwMilliseconds
    )

/*++

Routine Description:

    A wait operation on a waitable object is accomplished with the
    WaitForSingleObject function.

    Waiting on an object checks the current state of the object.  If the
    current state of the object allows continued execution, any
    adjustments to the object state are made (for example, decrementing
    the semaphore count for a semaphore object) and the thread continues
    execution.  If the current state of the object does not allow
    continued execution, the thread is placed into the wait state
    pending the change of the object's state or time-out.

Arguments:

    hHandle - An open handle to a waitable object. The handle must have
        SYNCHRONIZE access to the object.

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of -1
        specifies an infinite timeout period.

Return Value:

    WAIT_TIME_OUT - Indicates that the wait was terminated due to the
        TimeOut conditions.

    0 - indicates the specified object attained a Signaled
        state thus completing the wait.

    WAIT_ABANDONED - indicates the specified object attained a Signaled
        state but was abandoned.

--*/

{
    return WaitForSingleObjectEx(hHandle,dwMilliseconds,FALSE);
}

DWORD
APIENTRY
WaitForSingleObjectEx(
    HANDLE hHandle,
    DWORD dwMilliseconds,
    BOOL bAlertable
    )

/*++

Routine Description:

    A wait operation on a waitable object is accomplished with the
    WaitForSingleObjectEx function.

    Waiting on an object checks the current state of the object.  If the
    current state of the object allows continued execution, any
    adjustments to the object state are made (for example, decrementing
    the semaphore count for a semaphore object) and the thread continues
    execution.  If the current state of the object does not allow
    continued execution, the thread is placed into the wait state
    pending the change of the object's state or time-out.

    If the bAlertable parameter is FALSE, the only way the wait
    terminates is because the specified timeout period expires, or
    because the specified object entered the signaled state.  If the
    bAlertable parameter is TRUE, then the wait can return due to any
    one of the above wait termination conditions, or because an I/O
    completion callback terminated the wait early (return value of
    WAIT_IO_COMPLETION).

Arguments:

    hHandle - An open handle to a waitable object. The handle must have
        SYNCHRONIZE access to the object.

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of
        0xffffffff specifies an infinite timeout period.

    bAlertable - Supplies a flag that controls whether or not the
        wait may terminate early due to an I/O completion callback.
        A value of TRUE allows this API to complete early due to an I/O
        completion callback.  A value of FALSE will not allow I/O
        completion callbacks to terminate this call early.

Return Value:

    WAIT_TIME_OUT - Indicates that the wait was terminated due to the
        TimeOut conditions.

    0 - indicates the specified object attained a Signaled
        state thus completing the wait.

    0xffffffff - The wait terminated due to an error. GetLastError may be
        used to get additional error information.

    WAIT_ABANDONED - indicates the specified object attained a Signaled
        state but was abandoned.

    WAIT_IO_COMPLETION - The wait terminated due to one or more I/O
        completion callbacks.

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    PPEB Peb;

    Peb = NtCurrentPeb();
    switch( HandleToUlong(hHandle) ) {
        case STD_INPUT_HANDLE:  hHandle = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hHandle = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hHandle = Peb->ProcessParameters->StandardError;
                                break;
        }

    if (CONSOLE_HANDLE(hHandle) && VerifyConsoleIoHandle(hHandle)) {
        hHandle = GetConsoleInputWaitHandle();
        }

    pTimeOut = BaseFormatTimeOut(&TimeOut,dwMilliseconds);
rewait:
    Status = NtWaitForSingleObject(hHandle,(BOOLEAN)bAlertable,pTimeOut);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        Status = (NTSTATUS)0xffffffff;
        }
    else {
        if ( bAlertable && Status == STATUS_ALERTED ) {
            goto rewait;
            }
        }
    return (DWORD)Status;
}


DWORD
WINAPI
SignalObjectAndWait(
    HANDLE hObjectToSignal,
    HANDLE hObjectToWaitOn,
    DWORD dwMilliseconds,
    BOOL bAlertable
    )
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    PPEB Peb;

    Peb = NtCurrentPeb();
    switch( HandleToUlong(hObjectToWaitOn) ) {
        case STD_INPUT_HANDLE:  hObjectToWaitOn = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hObjectToWaitOn = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hObjectToWaitOn = Peb->ProcessParameters->StandardError;
                                break;
        }

    if (CONSOLE_HANDLE(hObjectToWaitOn) && VerifyConsoleIoHandle(hObjectToWaitOn)) {
        hObjectToWaitOn = GetConsoleInputWaitHandle();
        }

    pTimeOut = BaseFormatTimeOut(&TimeOut,dwMilliseconds);
rewait:
    Status = NtSignalAndWaitForSingleObject(
                hObjectToSignal,
                hObjectToWaitOn,
                (BOOLEAN)bAlertable,
                pTimeOut
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        Status = (NTSTATUS)0xffffffff;
        }
    else {
        if ( bAlertable && Status == STATUS_ALERTED ) {
            goto rewait;
            }
        }
    return (DWORD)Status;
}



DWORD
WaitForMultipleObjects(
    DWORD nCount,
    CONST HANDLE *lpHandles,
    BOOL bWaitAll,
    DWORD dwMilliseconds
    )

/*++

Routine Description:

A wait operation on multiple waitable objects (up to
MAXIMUM_WAIT_OBJECTS) is accomplished with the WaitForMultipleObjects
function.

Arguments:

    nCount - A count of the number of objects that are to be waited on.

    lpHandles - An array of object handles.  Each handle must have
        SYNCHRONIZE access to the associated object.

    bWaitAll - A flag that supplies the wait type.  A value of TRUE
        indicates a "wait all".  A value of false indicates a "wait
        any".

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of -1
        specifies an infinite timeout period.

Return Value:

    WAIT_TIME_OUT - indicates that the wait was terminated due to the
        TimeOut conditions.

    0 to MAXIMUM_WAIT_OBJECTS-1, indicates, in the case of wait for any
        object, the object number which satisfied the wait.  In the case
        of wait for all objects, the value only indicates that the wait
        was completed successfully.

    WAIT_ABANDONED_0 to (WAIT_ABANDONED_0)+(MAXIMUM_WAIT_OBJECTS - 1),
        indicates, in the case of wait for any object, the object number
        which satisfied the event, and that the object which satisfied
        the event was abandoned.  In the case of wait for all objects,
        the value indicates that the wait was completed successfully and
        at least one of the objects was abandoned.

--*/

{
    return WaitForMultipleObjectsEx(nCount,lpHandles,bWaitAll,dwMilliseconds,FALSE);
}

DWORD
APIENTRY
WaitForMultipleObjectsEx(
    DWORD nCount,
    CONST HANDLE *lpHandles,
    BOOL bWaitAll,
    DWORD dwMilliseconds,
    BOOL bAlertable
    )

/*++

Routine Description:

    A wait operation on multiple waitable objects (up to
    MAXIMUM_WAIT_OBJECTS) is accomplished with the
    WaitForMultipleObjects function.

    This API can be used to wait on any of the specified objects to
    enter the signaled state, or all of the objects to enter the
    signaled state.

    If the bAlertable parameter is FALSE, the only way the wait
    terminates is because the specified timeout period expires, or
    because the specified objects entered the signaled state.  If the
    bAlertable parameter is TRUE, then the wait can return due to any one of
    the above wait termination conditions, or because an I/O completion
    callback terminated the wait early (return value of
    WAIT_IO_COMPLETION).

Arguments:

    nCount - A count of the number of objects that are to be waited on.

    lpHandles - An array of object handles.  Each handle must have
        SYNCHRONIZE access to the associated object.

    bWaitAll - A flag that supplies the wait type.  A value of TRUE
        indicates a "wait all".  A value of false indicates a "wait
        any".

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of
        0xffffffff specifies an infinite timeout period.

    bAlertable - Supplies a flag that controls whether or not the
        wait may terminate early due to an I/O completion callback.
        A value of TRUE allows this API to complete early due to an I/O
        completion callback.  A value of FALSE will not allow I/O
        completion callbacks to terminate this call early.

Return Value:

    WAIT_TIME_OUT - indicates that the wait was terminated due to the
        TimeOut conditions.

    0 to MAXIMUM_WAIT_OBJECTS-1, indicates, in the case of wait for any
        object, the object number which satisfied the wait.  In the case
        of wait for all objects, the value only indicates that the wait
        was completed successfully.

    0xffffffff - The wait terminated due to an error. GetLastError may be
        used to get additional error information.

    WAIT_ABANDONED_0 to (WAIT_ABANDONED_0)+(MAXIMUM_WAIT_OBJECTS - 1),
        indicates, in the case of wait for any object, the object number
        which satisfied the event, and that the object which satisfied
        the event was abandoned.  In the case of wait for all objects,
        the value indicates that the wait was completed successfully and
        at least one of the objects was abandoned.

    WAIT_IO_COMPLETION - The wait terminated due to one or more I/O
        completion callbacks.

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    DWORD i;
    LPHANDLE HandleArray;
    HANDLE Handles[ 8 ];
    PPEB Peb;

    if (nCount > 8) {
        HandleArray = (LPHANDLE) RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), nCount*sizeof(HANDLE));
        if (HandleArray == NULL) {
            BaseSetLastNTError(STATUS_NO_MEMORY);
            return 0xffffffff;
        }
    } else {
        HandleArray = Handles;
    }
    RtlCopyMemory(HandleArray,(LPVOID)lpHandles,nCount*sizeof(HANDLE));

    Peb = NtCurrentPeb();
    for (i=0;i<nCount;i++) {
        switch( HandleToUlong(HandleArray[i]) ) {
            case STD_INPUT_HANDLE:  HandleArray[i] = Peb->ProcessParameters->StandardInput;
                                    break;
            case STD_OUTPUT_HANDLE: HandleArray[i] = Peb->ProcessParameters->StandardOutput;
                                    break;
            case STD_ERROR_HANDLE:  HandleArray[i] = Peb->ProcessParameters->StandardError;
                                    break;
            }

        if (CONSOLE_HANDLE(HandleArray[i]) && VerifyConsoleIoHandle(HandleArray[i])) {
            HandleArray[i] = GetConsoleInputWaitHandle();
            }
        }

    pTimeOut = BaseFormatTimeOut(&TimeOut,dwMilliseconds);
rewait:
    Status = NtWaitForMultipleObjects(
                 (CHAR)nCount,
                 HandleArray,
                 bWaitAll ? WaitAll : WaitAny,
                 (BOOLEAN)bAlertable,
                 pTimeOut
                 );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        Status = (NTSTATUS)0xffffffff;
        }
    else {
        if ( bAlertable && Status == STATUS_ALERTED ) {
            goto rewait;
            }
        }

    if (HandleArray != Handles) {
        RtlFreeHeap(RtlProcessHeap(), 0, HandleArray);
    }

    return (DWORD)Status;
}

VOID
Sleep(
    DWORD dwMilliseconds
    )

/*++

Routine Description:

    The execution of the current thread can be delayed for a specified
    interval of time with the Sleep function.

    The Sleep function causes the current thread to enter a
    waiting state until the specified interval of time has passed.

Arguments:

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  This allows an application to test an object to
        determine if it is in the signaled state.  A timeout value of -1
        specifies an infinite timeout period.

Return Value:

    None.

--*/

{
    SleepEx(dwMilliseconds,FALSE);
}

DWORD
APIENTRY
SleepEx(
    DWORD dwMilliseconds,
    BOOL bAlertable
    )

/*++

Routine Description:

    The execution of the current thread can be delayed for a specified
    interval of time with the SleepEx function.

    The SleepEx function causes the current thread to enter a waiting
    state until the specified interval of time has passed.

    If the bAlertable parameter is FALSE, the only way the SleepEx
    returns is when the specified time interval has passed.  If the
    bAlertable parameter is TRUE, then the SleepEx can return due to the
    expiration of the time interval (return value of 0), or because an
    I/O completion callback terminated the SleepEx early (return value
    of WAIT_IO_COMPLETION).

Arguments:

    dwMilliseconds - A time-out value that specifies the relative time,
        in milliseconds, over which the wait is to be completed.  A
        timeout value of 0 specified that the wait is to timeout
        immediately.  A timeout value of -1 specifies an infinite
        timeout period.

    bAlertable - Supplies a flag that controls whether or not the
        SleepEx may terminate early due to an I/O completion callback.
        A value of TRUE allows this API to complete early due to an I/O
        completion callback.  A value of FALSE will not allow I/O
        completion callbacks to terminate this call early.

Return Value:

    0 - The SleepEx terminated due to expiration of the time interval.

    WAIT_IO_COMPLETION - The SleepEx terminated due to one or more I/O
        completion callbacks.

--*/
{
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    NTSTATUS Status;

    pTimeOut = BaseFormatTimeOut(&TimeOut,dwMilliseconds);
    if (pTimeOut == NULL) {
        //
        // If Sleep( -1 ) then delay for the longest possible integer
        // relative to now.
        //

        TimeOut.LowPart = 0x0;
        TimeOut.HighPart = 0x80000000;
        pTimeOut = &TimeOut;
        }

rewait:
    Status = NtDelayExecution(
                (BOOLEAN)bAlertable,
                pTimeOut
                );
    if ( bAlertable && Status == STATUS_ALERTED ) {
        goto rewait;
        }
    return Status == STATUS_USER_APC ? WAIT_IO_COMPLETION : 0;
}

HANDLE
WINAPI
CreateWaitableTimerA(
    LPSECURITY_ATTRIBUTES lpTimerAttributes,
    BOOL bManualReset,
    LPCSTR lpTimerName
    )

/*++

Routine Description:

    ANSI thunk to CreateWaitableTimerW


--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    LPCWSTR NameBuffer;

    NameBuffer = NULL;
    if ( ARGUMENT_PRESENT(lpTimerName) ) {
        Unicode = &NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString,lpTimerName);
        Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                }
            else {
                BaseSetLastNTError(Status);
                }
            return NULL;
            }
        NameBuffer = (LPCWSTR)Unicode->Buffer;
        }

    return CreateWaitableTimerW(
                lpTimerAttributes,
                bManualReset,
                NameBuffer
                );
}

HANDLE
WINAPI
CreateWaitableTimerW(
    LPSECURITY_ATTRIBUTES lpTimerAttributes,
    BOOL bManualReset,
    LPCWSTR lpTimerName
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    POBJECT_ATTRIBUTES pObja;
    HANDLE Handle;
    UNICODE_STRING ObjectName;

    if ( ARGUMENT_PRESENT(lpTimerName) ) {
        RtlInitUnicodeString(&ObjectName,lpTimerName);
        pObja = BaseFormatObjectAttributes(&Obja,lpTimerAttributes,&ObjectName);
        }
    else {
        pObja = BaseFormatObjectAttributes(&Obja,lpTimerAttributes,NULL);
        }

    Status = NtCreateTimer(
                &Handle,
                TIMER_ALL_ACCESS,
                pObja,
                bManualReset ? NotificationTimer : SynchronizationTimer
                );

    if ( NT_SUCCESS(Status) ) {
        if ( Status == STATUS_OBJECT_NAME_EXISTS ) {
            SetLastError(ERROR_ALREADY_EXISTS);
            }
        else {
            SetLastError(0);
            }
        return Handle;
        }
    else {
        BaseSetLastNTError(Status);
        return NULL;
        }
}

HANDLE
WINAPI
OpenWaitableTimerA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpTimerName
    )
{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    if ( ARGUMENT_PRESENT(lpTimerName) ) {
        Unicode = &NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString,lpTimerName);
        Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                }
            else {
                BaseSetLastNTError(Status);
                }
            return NULL;
            }
        }
    else {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return NULL;
        }

    return OpenWaitableTimerW(
                dwDesiredAccess,
                bInheritHandle,
                (LPCWSTR)Unicode->Buffer
                );
}


HANDLE
WINAPI
OpenWaitableTimerW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpTimerName
    )
{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING ObjectName;
    NTSTATUS Status;
    HANDLE Object;

    if ( !lpTimerName ) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return NULL;
        }
    RtlInitUnicodeString(&ObjectName,lpTimerName);

    InitializeObjectAttributes(
        &Obja,
        &ObjectName,
        (bInheritHandle ? OBJ_INHERIT : 0),
        BaseGetNamedObjectDirectory(),
        NULL
        );

    Status = NtOpenTimer(
                &Object,
                dwDesiredAccess,
                &Obja
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return NULL;
        }
    return Object;
}

BOOL
WINAPI
SetWaitableTimer(
    HANDLE hTimer,
    const LARGE_INTEGER *lpDueTime,
    LONG lPeriod,
    PTIMERAPCROUTINE pfnCompletionRoutine,
    LPVOID lpArgToCompletionRoutine,
    BOOL fResume
    )
{
    NTSTATUS Status;

    Status = NtSetTimer(
                hTimer,
                (PLARGE_INTEGER)lpDueTime,
                (PTIMER_APC_ROUTINE)pfnCompletionRoutine,
                lpArgToCompletionRoutine,
                (BOOLEAN) fResume,
                lPeriod,
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    else {
        if ( Status == STATUS_TIMER_RESUME_IGNORED ) {
            SetLastError(ERROR_NOT_SUPPORTED);
            }
        else {
            SetLastError(0);
            }
        return TRUE;
        }
}

BOOL
WINAPI
CancelWaitableTimer(
    HANDLE hTimer
    )
{
    NTSTATUS Status;

    Status = NtCancelTimer(hTimer,NULL);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    else {
        return TRUE;
        }
}
