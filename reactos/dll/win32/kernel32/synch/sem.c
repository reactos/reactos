/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/synch/sem.c
 * PURPOSE:         Wrappers for the NT Semaphore Implementation
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include "debug.h"

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
HANDLE
WINAPI
CreateSemaphoreExA(IN LPSECURITY_ATTRIBUTES lpSemaphoreAttributes  OPTIONAL,
                   IN LONG lInitialCount,
                   IN LONG lMaximumCount,
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
    return CreateSemaphoreExW(lpSemaphoreAttributes,
                              lInitialCount,
                              lMaximumCount,
                              UnicodeName,
                              dwFlags,
                              dwDesiredAccess);
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateSemaphoreExW(IN LPSECURITY_ATTRIBUTES lpSemaphoreAttributes  OPTIONAL,
                   IN LONG lInitialCount,
                   IN LONG lMaximumCount,
                   IN LPCWSTR lpName  OPTIONAL,
                   IN DWORD dwFlags,
                   IN DWORD dwDesiredAccess)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES LocalAttributes;
    POBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    UNICODE_STRING ObjectName;

    /* Now check if we got a name */
    if (lpName) RtlInitUnicodeString(&ObjectName, lpName);

    if (dwFlags != 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Now convert the object attributes */
    ObjectAttributes = BasepConvertObjectAttributes(&LocalAttributes,
                                                    lpSemaphoreAttributes,
                                                    lpName ? &ObjectName : NULL);

    /* Create the semaphore */
   Status = NtCreateSemaphore(&Handle,
                              (ACCESS_MASK)dwDesiredAccess,
                              ObjectAttributes,
                              lInitialCount,
                              lMaximumCount);
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


/*
 * @implemented
 */
HANDLE
WINAPI
CreateSemaphoreA(IN LPSECURITY_ATTRIBUTES lpSemaphoreAttributes  OPTIONAL,
                 IN LONG lInitialCount,
                 IN LONG lMaximumCount,
                 IN LPCSTR lpName  OPTIONAL)
{
    return CreateSemaphoreExA(lpSemaphoreAttributes,
                              lInitialCount,
                              lMaximumCount,
                              lpName,
                              0,
                              SEMAPHORE_ALL_ACCESS);
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateSemaphoreW(IN LPSECURITY_ATTRIBUTES lpSemaphoreAttributes  OPTIONAL,
                 IN LONG lInitialCount,
                 IN LONG lMaximumCount,
                 IN LPCWSTR lpName  OPTIONAL)
{
    return CreateSemaphoreExW(lpSemaphoreAttributes,
                              lInitialCount,
                              lMaximumCount,
                              lpName,
                              0,
                              SEMAPHORE_ALL_ACCESS);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenSemaphoreA(IN DWORD dwDesiredAccess,
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
    return OpenSemaphoreW(dwDesiredAccess,
                          bInheritHandle,
                          (LPCWSTR)UnicodeCache->Buffer);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenSemaphoreW(IN DWORD dwDesiredAccess,
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

    /* Open the semaphore */
    Status = NtOpenSemaphore(&Handle, dwDesiredAccess, &ObjectAttributes);
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
ReleaseSemaphore(IN HANDLE hSemaphore,
                 IN LONG lReleaseCount,
                 IN LPLONG lpPreviousCount)
{
    NTSTATUS Status;

    /* Release the semaphore */
    Status = NtReleaseSemaphore(hSemaphore, lReleaseCount, lpPreviousCount);
    if (NT_SUCCESS(Status)) return TRUE;

    /* If we got here, then we failed */
    SetLastErrorByStatus(Status);
    return FALSE;
}

/* EOF */
