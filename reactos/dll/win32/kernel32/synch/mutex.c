/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/synch/mutex.c
 * PURPOSE:         Wrappers for the NT Mutant Implementation
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include "debug.h"

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
HANDLE
WINAPI
CreateMutexExA(IN LPSECURITY_ATTRIBUTES lpMutexAttributes  OPTIONAL,
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
    return CreateMutexExW(lpMutexAttributes,
                          UnicodeName,
                          dwFlags,
                          dwDesiredAccess);
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateMutexExW(IN LPSECURITY_ATTRIBUTES lpMutexAttributes  OPTIONAL,
               IN LPCWSTR lpName  OPTIONAL,
               IN DWORD dwFlags,
               IN DWORD dwDesiredAccess)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES LocalAttributes;
    POBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    UNICODE_STRING ObjectName;
    BOOLEAN InitialOwner;

    /* Now check if we got a name */
    if (lpName) RtlInitUnicodeString(&ObjectName, lpName);

    if (dwFlags & ~(CREATE_MUTEX_INITIAL_OWNER))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    InitialOwner = (dwFlags & CREATE_MUTEX_INITIAL_OWNER) != 0;

    /* Now convert the object attributes */
    ObjectAttributes = BasepConvertObjectAttributes(&LocalAttributes,
                                                    lpMutexAttributes,
                                                    lpName ? &ObjectName : NULL);

    /* Create the mutant */
    Status = NtCreateMutant(&Handle,
                            (ACCESS_MASK)dwDesiredAccess,
                            ObjectAttributes,
                            InitialOwner);
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
CreateMutexA(IN LPSECURITY_ATTRIBUTES lpMutexAttributes  OPTIONAL,
             IN BOOL bInitialOwner,
             IN LPCSTR lpName  OPTIONAL)
{
    DWORD dwFlags = 0;

    if (bInitialOwner)
        dwFlags |= CREATE_MUTEX_INITIAL_OWNER;

    return CreateMutexExA(lpMutexAttributes,
                          lpName,
                          dwFlags,
                          MUTANT_ALL_ACCESS);
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateMutexW(IN LPSECURITY_ATTRIBUTES lpMutexAttributes  OPTIONAL,
             IN BOOL bInitialOwner,
             IN LPCWSTR lpName  OPTIONAL)
{
    DWORD dwFlags = 0;

    if (bInitialOwner)
        dwFlags |= CREATE_MUTEX_INITIAL_OWNER;

    return CreateMutexExW(lpMutexAttributes,
                          lpName,
                          dwFlags,
                          MUTANT_ALL_ACCESS);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenMutexA(IN DWORD dwDesiredAccess,
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
    return OpenMutexW(dwDesiredAccess,
                      bInheritHandle,
                      (LPCWSTR)UnicodeCache->Buffer);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenMutexW(IN DWORD dwDesiredAccess,
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

    /* Open the mutant */
    Status = NtOpenMutant(&Handle, dwDesiredAccess, &ObjectAttributes);
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
ReleaseMutex(IN HANDLE hMutex)
{
    NTSTATUS Status;

    /* Release the mutant */
    Status = NtReleaseMutant(hMutex, NULL);
    if (NT_SUCCESS(Status)) return TRUE;

    /* If we got here, then we failed */
    SetLastErrorByStatus(Status);
    return FALSE;
}


/* EOF */
