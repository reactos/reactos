/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/synch/event.c
 * PURPOSE:         Wrappers for the NT Event Implementation
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

HANDLE
WINAPI
CreateEventExA(IN LPSECURITY_ATTRIBUTES lpEventAttributes  OPTIONAL,
               IN LPCSTR lpName  OPTIONAL,
               IN DWORD dwFlags,
               IN DWORD dwDesiredAccess)
{
    NTSTATUS Status;
    ANSI_STRING AnsiName;
    PUNICODE_STRING UnicodeCache;
    LPCWSTR UnicodeName = NULL;

    /* Check for a name */
    if (lpName)
    {
        /* Use TEB Cache */
        UnicodeCache = &NtCurrentTeb()->StaticUnicodeString;

        /* Convert to unicode */
        RtlInitAnsiString(&AnsiName, lpName);
        Status = RtlAnsiStringToUnicodeString(UnicodeCache, &AnsiName, FALSE);
        if (!NT_SUCCESS(Status))
        {
            /* Conversion failed */
            SetLastErrorByStatus(Status);
            return NULL;
        }

        /* Otherwise, save the buffer */
        UnicodeName = (LPCWSTR)UnicodeCache->Buffer;
    }

    /* Call the Unicode API */
    return CreateEventExW(lpEventAttributes,
                          UnicodeName,
                          dwFlags,
                          dwDesiredAccess);

}

HANDLE
WINAPI
CreateEventExW(IN LPSECURITY_ATTRIBUTES lpEventAttributes  OPTIONAL,
               IN LPCWSTR lpName  OPTIONAL,
               IN DWORD dwFlags,
               IN DWORD dwDesiredAccess)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES LocalAttributes;
    POBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    UNICODE_STRING ObjectName;
    BOOLEAN InitialState;
    EVENT_TYPE EventType;

    /* Now check if we got a name */
    if (lpName) RtlInitUnicodeString(&ObjectName, lpName);

	/* Check for invalid flags */
    if (dwFlags & ~(CREATE_EVENT_INITIAL_SET | CREATE_EVENT_MANUAL_RESET))
    {
	    /* Fail */
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Set initial state and event type */
    InitialState = (dwFlags & CREATE_EVENT_INITIAL_SET) ? TRUE : FALSE;
    EventType = (dwFlags & CREATE_EVENT_MANUAL_RESET) ? NotificationEvent : SynchronizationEvent;

    /* Now convert the object attributes */
    ObjectAttributes = BasepConvertObjectAttributes(&LocalAttributes,
                                                    lpEventAttributes,
                                                    lpName ? &ObjectName : NULL);

    /* Create the event */
    Status = NtCreateEvent(&Handle,
                           (ACCESS_MASK)dwDesiredAccess,
                           ObjectAttributes,
                           EventType,
                           InitialState);
    if (NT_SUCCESS(Status))
    {
        /* Check if the object already existed */
        if (Status == STATUS_OBJECT_NAME_EXISTS)
        {
            /* Set distinguished Win32 error code */
            SetLastError(ERROR_ALREADY_EXISTS);
        }
        else
        {
            /* Otherwise, set success */
            SetLastError(ERROR_SUCCESS);
        }

        /* Return the handle */
        return Handle;
    }
    else
    {
        /* Convert the NT Status and fail */
        SetLastErrorByStatus(Status);
        return NULL;
    }

}

HANDLE
WINAPI
CreateEventA(IN LPSECURITY_ATTRIBUTES lpEventAttributes  OPTIONAL,
             IN BOOL bManualReset,
             IN BOOL bInitialState,
             IN LPCSTR lpName  OPTIONAL)
{
    DWORD dwFlags = 0;

    /* Set new flags */
    if (bManualReset) dwFlags |= CREATE_EVENT_MANUAL_RESET;
    if (bInitialState) dwFlags |= CREATE_EVENT_INITIAL_SET;

    /* Call the newer API */
    return CreateEventExA(lpEventAttributes,
                          lpName,
                          dwFlags,
                          EVENT_ALL_ACCESS);
}

HANDLE
WINAPI
CreateEventW(IN LPSECURITY_ATTRIBUTES lpEventAttributes  OPTIONAL,
             IN BOOL bManualReset,
             IN BOOL bInitialState,
             IN LPCWSTR lpName  OPTIONAL)
{
    DWORD dwFlags = 0;

    /* Set new flags */
    if (bManualReset) dwFlags |= CREATE_EVENT_MANUAL_RESET;
    if (bInitialState) dwFlags |= CREATE_EVENT_INITIAL_SET;

    /* Call the newer API */
    return CreateEventExW(lpEventAttributes,
                          lpName,
                          dwFlags,
                          EVENT_ALL_ACCESS);
}

HANDLE
WINAPI
OpenEventA(IN DWORD dwDesiredAccess,
           IN BOOL bInheritHandle,
           IN LPCSTR lpName)
{
    NTSTATUS Status;
    ANSI_STRING AnsiName;
    PUNICODE_STRING UnicodeCache;

    /* Check for a name */
    if (lpName)
    {
        /* Use TEB Cache */
        UnicodeCache = &NtCurrentTeb()->StaticUnicodeString;

        /* Convert to unicode */
        RtlInitAnsiString(&AnsiName, lpName);
        Status = RtlAnsiStringToUnicodeString(UnicodeCache, &AnsiName, FALSE);
        if (!NT_SUCCESS(Status))
        {
            /* Conversion failed */
            SetLastErrorByStatus(Status);
            return NULL;
        }
    }
    else
    {
        /* We need a name */
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Call the Unicode API */
    return OpenEventW(dwDesiredAccess,
                      bInheritHandle,
                      (LPCWSTR)UnicodeCache->Buffer);
}

HANDLE
WINAPI
OpenEventW(IN DWORD dwDesiredAccess,
           IN BOOL bInheritHandle,
           IN LPCWSTR lpName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ObjectName;
    NTSTATUS Status;
    HANDLE Handle;

    /* Make sure we got a name */
    if (!lpName)
    {
        /* Fail without one */
        SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
        return NULL;
    }

    /* Initialize the object name and attributes */
    RtlInitUnicodeString(&ObjectName, lpName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &ObjectName,
                               bInheritHandle ? OBJ_INHERIT : 0,
                               hBaseDir,
                               NULL);

    /* Open the event */
    Status = NtOpenEvent(&Handle, dwDesiredAccess, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /* Convert the status and fail */
        SetLastErrorByStatus(Status);
        return NULL;
    }

    /* Return the handle */
    return Handle;
}

/*
 * @implemented
 */
BOOL
WINAPI
PulseEvent(IN HANDLE hEvent)
{
    NTSTATUS Status;

    /* Pulse the event */
    Status = NtPulseEvent(hEvent, NULL);
    if (NT_SUCCESS(Status)) return TRUE;

    /* If we got here, then we failed */
    SetLastErrorByStatus(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
ResetEvent(IN HANDLE hEvent)
{
    NTSTATUS Status;

    /* Clear the event */
    Status = NtResetEvent(hEvent, NULL);
    if (NT_SUCCESS(Status)) return TRUE;

    /* If we got here, then we failed */
    SetLastErrorByStatus(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetEvent(IN HANDLE hEvent)
{
    NTSTATUS Status;

    /* Set the event */
    Status = NtSetEvent(hEvent, NULL);
    if (NT_SUCCESS(Status)) return TRUE;

    /* If we got here, then we failed */
    SetLastErrorByStatus(Status);
    return FALSE;
}

/* EOF */
