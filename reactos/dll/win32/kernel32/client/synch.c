/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/synch/wait.c
 * PURPOSE:         Wrappers for the NT Wait Implementation
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x600
#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
DWORD
WINAPI
WaitForSingleObject(IN HANDLE hHandle,
                    IN DWORD dwMilliseconds)
{
    /* Call the extended API */
    return WaitForSingleObjectEx(hHandle, dwMilliseconds, FALSE);
}

/*
 * @implemented
 */
DWORD
WINAPI
WaitForSingleObjectEx(IN HANDLE hHandle,
                      IN DWORD dwMilliseconds,
                      IN BOOL bAlertable)
{
    PLARGE_INTEGER TimePtr;
    LARGE_INTEGER Time;
    NTSTATUS Status;

    /* Get real handle */
    hHandle = TranslateStdHandle(hHandle);

    /* Check for console handle */
    if ((IsConsoleHandle(hHandle)) && (VerifyConsoleIoHandle(hHandle)))
    {
        /* Get the real wait handle */
        hHandle = GetConsoleInputWaitHandle();
    }

    /* Check if this is an infinite wait */
    if (dwMilliseconds == INFINITE)
    {
        /* Under NT, this means no timer argument */
        TimePtr = NULL;
    }
    else
    {
        /* Otherwise, convert the time to NT Format */
        Time.QuadPart = UInt32x32To64(-10000, dwMilliseconds);
        TimePtr = &Time;
    }

    /* Start wait loop */
    do
    {
        /* Do the wait */
        Status = NtWaitForSingleObject(hHandle, (BOOLEAN)bAlertable, TimePtr);
        if (!NT_SUCCESS(Status))
        {
            /* The wait failed */
            SetLastErrorByStatus (Status);
            return WAIT_FAILED;
        }
    } while ((Status == STATUS_ALERTED) && (bAlertable));

    /* Return wait status */
    return Status;
}

/*
 * @implemented
 */
DWORD
WINAPI
WaitForMultipleObjects(IN DWORD nCount,
                       IN CONST HANDLE *lpHandles,
                       IN BOOL bWaitAll,
                       IN DWORD dwMilliseconds)
{
    /* Call the extended API */
    return WaitForMultipleObjectsEx(nCount,
                                    lpHandles,
                                    bWaitAll,
                                    dwMilliseconds,
                                    FALSE);
}

/*
 * @implemented
 */
DWORD
WINAPI
WaitForMultipleObjectsEx(IN DWORD nCount,
                         IN CONST HANDLE *lpHandles,
                         IN BOOL bWaitAll,
                         IN DWORD dwMilliseconds,
                         IN BOOL bAlertable)
{
    PLARGE_INTEGER TimePtr;
    LARGE_INTEGER Time;
    PHANDLE HandleBuffer;
    HANDLE Handle[8];
    DWORD i;
    NTSTATUS Status;

    /* Check if we have more handles then we locally optimize */
    if (nCount > 8)
    {
        /* Allocate a buffer for them */
        HandleBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                       0,
                                       nCount * sizeof(HANDLE));
        if (!HandleBuffer)
        {
            /* No buffer, fail the wait */
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return WAIT_FAILED;
        }
    }
    else
    {
        /* Otherwise, use our local buffer */
        HandleBuffer = Handle;
    }

    /* Copy the handles into our buffer and loop them all */
    RtlCopyMemory(HandleBuffer, (LPVOID)lpHandles, nCount * sizeof(HANDLE));
    for (i = 0; i < nCount; i++)
    {
        /* Check what kind of handle this is */
        HandleBuffer[i] = TranslateStdHandle(HandleBuffer[i]);

        /* Check for console handle */
        if ((IsConsoleHandle(HandleBuffer[i])) &&
            (VerifyConsoleIoHandle(HandleBuffer[i])))
        {
            /* Get the real wait handle */
            HandleBuffer[i] = GetConsoleInputWaitHandle();
        }
    }

    /* Check if this is an infinite wait */
    if (dwMilliseconds == INFINITE)
    {
        /* Under NT, this means no timer argument */
        TimePtr = NULL;
    }
    else
    {
        /* Otherwise, convert the time to NT Format */
        Time.QuadPart = UInt32x32To64(-10000, dwMilliseconds);
        TimePtr = &Time;
    }

    /* Start wait loop */
    do
    {
        /* Do the wait */
        Status = NtWaitForMultipleObjects(nCount,
                                          HandleBuffer,
                                          bWaitAll ? WaitAll : WaitAny,
                                          (BOOLEAN)bAlertable,
                                          TimePtr);
        if (!NT_SUCCESS(Status))
        {
            /* Wait failed */
            SetLastErrorByStatus (Status);
            return WAIT_FAILED;
        }
    } while ((Status == STATUS_ALERTED) && (bAlertable));

    /* Check if we didn't use our local buffer */
    if (HandleBuffer != Handle)
    {
        /* Free the allocated one */
        RtlFreeHeap(RtlGetProcessHeap(), 0, HandleBuffer);
    }

    /* Return wait status */
    return Status;
}

/*
 * @implemented
 */
DWORD
WINAPI
SignalObjectAndWait(IN HANDLE hObjectToSignal,
                    IN HANDLE hObjectToWaitOn,
                    IN DWORD dwMilliseconds,
                    IN BOOL bAlertable)
{
    PLARGE_INTEGER TimePtr;
    LARGE_INTEGER Time;
    NTSTATUS Status;

    /* Get real handle */
    hObjectToWaitOn = TranslateStdHandle(hObjectToWaitOn);

    /* Check for console handle */
    if ((IsConsoleHandle(hObjectToWaitOn)) &&
        (VerifyConsoleIoHandle(hObjectToWaitOn)))
    {
        /* Get the real wait handle */
        hObjectToWaitOn = GetConsoleInputWaitHandle();
    }

    /* Check if this is an infinite wait */
    if (dwMilliseconds == INFINITE)
    {
        /* Under NT, this means no timer argument */
        TimePtr = NULL;
    }
    else
    {
        /* Otherwise, convert the time to NT Format */
        Time.QuadPart = UInt32x32To64(-10000, dwMilliseconds);
        TimePtr = &Time;
    }

    /* Start wait loop */
    do
    {
        /* Do the wait */
        Status = NtSignalAndWaitForSingleObject(hObjectToSignal,
                                                hObjectToWaitOn,
                                                (BOOLEAN)bAlertable,
                                                TimePtr);
        if (!NT_SUCCESS(Status))
        {
            /* The wait failed */
            SetLastErrorByStatus (Status);
            return WAIT_FAILED;
        }
    } while ((Status == STATUS_ALERTED) && (bAlertable));

    /* Return wait status */
    return Status;
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateWaitableTimerExA(IN LPSECURITY_ATTRIBUTES lpTimerAttributes  OPTIONAL,
                       IN LPCSTR lpTimerName  OPTIONAL,
                       IN DWORD dwFlags,
                       IN DWORD dwDesiredAccess)
{
    NTSTATUS Status;
    ANSI_STRING AnsiName;
    PUNICODE_STRING UnicodeCache;
    LPCWSTR UnicodeName = NULL;

    /* Check for a name */
    if (lpTimerName)
    {
        /* Use TEB Cache */
        UnicodeCache = &NtCurrentTeb()->StaticUnicodeString;

        /* Convert to unicode */
        RtlInitAnsiString(&AnsiName, lpTimerName);
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
    return CreateWaitableTimerExW(lpTimerAttributes,
                                  UnicodeName,
                                  dwFlags,
                                  dwDesiredAccess);
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateWaitableTimerExW(IN LPSECURITY_ATTRIBUTES lpTimerAttributes  OPTIONAL,
                       IN LPCWSTR lpTimerName  OPTIONAL,
                       IN DWORD dwFlags,
                       IN DWORD dwDesiredAccess)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES LocalAttributes;
    POBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    UNICODE_STRING ObjectName;
    TIMER_TYPE TimerType;

    /* Now check if we got a name */
    if (lpTimerName) RtlInitUnicodeString(&ObjectName, lpTimerName);

    if (dwFlags & ~(CREATE_WAITABLE_TIMER_MANUAL_RESET))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    TimerType = (dwFlags & CREATE_WAITABLE_TIMER_MANUAL_RESET) ? NotificationTimer : SynchronizationTimer;

    /* Now convert the object attributes */
    ObjectAttributes = BasepConvertObjectAttributes(&LocalAttributes,
                                                    lpTimerAttributes,
                                                    lpTimerName ? &ObjectName : NULL);

    /* Create the timer */
    Status = NtCreateTimer(&Handle,
                           (ACCESS_MASK)dwDesiredAccess,
                           ObjectAttributes,
                           TimerType);
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
CreateWaitableTimerW(IN LPSECURITY_ATTRIBUTES lpTimerAttributes OPTIONAL,
                     IN BOOL bManualReset,
                     IN LPCWSTR lpTimerName OPTIONAL)
{
    DWORD dwFlags = 0;

    if (bManualReset)
        dwFlags |= CREATE_WAITABLE_TIMER_MANUAL_RESET;

    return CreateWaitableTimerExW(lpTimerAttributes,
                                  lpTimerName,
                                  dwFlags,
                                  TIMER_ALL_ACCESS);
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateWaitableTimerA(IN LPSECURITY_ATTRIBUTES lpTimerAttributes OPTIONAL,
                     IN BOOL bManualReset,
                     IN LPCSTR lpTimerName OPTIONAL)
{
    DWORD dwFlags = 0;

    if (bManualReset)
        dwFlags |= CREATE_WAITABLE_TIMER_MANUAL_RESET;

    return CreateWaitableTimerExA(lpTimerAttributes,
                                  lpTimerName,
                                  dwFlags,
                                  TIMER_ALL_ACCESS);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenWaitableTimerW(IN DWORD dwDesiredAccess,
                   IN BOOL bInheritHandle,
                   IN LPCWSTR lpTimerName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ObjectName;
    NTSTATUS Status;
    HANDLE Handle;

    /* Make sure we got a name */
    if (!lpTimerName)
    {
        /* Fail without one */
        SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
        return NULL;
    }

    /* Initialize the object name and attributes */
    RtlInitUnicodeString(&ObjectName, lpTimerName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &ObjectName,
                               bInheritHandle ? OBJ_INHERIT : 0,
                               hBaseDir,
                               NULL);

    /* Open the timer */
    Status = NtOpenTimer(&Handle, dwDesiredAccess, &ObjectAttributes);
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
HANDLE
WINAPI
OpenWaitableTimerA(IN DWORD dwDesiredAccess,
                   IN BOOL bInheritHandle,
                   IN LPCSTR lpTimerName)
{
    NTSTATUS Status;
    ANSI_STRING AnsiName;
    PUNICODE_STRING UnicodeCache;

    /* Check for a name */
    if (lpTimerName)
    {
        /* Use TEB Cache */
        UnicodeCache = &NtCurrentTeb()->StaticUnicodeString;

        /* Convert to unicode */
        RtlInitAnsiString(&AnsiName, lpTimerName);
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
   return OpenWaitableTimerW(dwDesiredAccess,
                             bInheritHandle,
                             (LPCWSTR)UnicodeCache->Buffer);
}

/*
 * @implemented
 */
BOOL
WINAPI
SetWaitableTimer(IN HANDLE hTimer,
                 IN const LARGE_INTEGER *pDueTime,
                 IN LONG lPeriod,
                 IN PTIMERAPCROUTINE pfnCompletionRoutine OPTIONAL,
                 IN OPTIONAL LPVOID lpArgToCompletionRoutine,
                 IN BOOL fResume)
{
    NTSTATUS Status;

    /* Set the timer */
    Status = NtSetTimer(hTimer,
                        (PLARGE_INTEGER)pDueTime,
                        (PTIMER_APC_ROUTINE)pfnCompletionRoutine,
                        lpArgToCompletionRoutine,
                        (BOOLEAN)fResume,
                        lPeriod,
                        NULL);
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
CancelWaitableTimer(IN HANDLE hTimer)
{
    NTSTATUS Status;

    /* Cancel the timer */
    Status = NtCancelTimer(hTimer, NULL);
    if (NT_SUCCESS(Status)) return TRUE;

    /* If we got here, then we failed */
    SetLastErrorByStatus(Status);
    return FALSE;
}

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

/*
 * @implemented
 */
VOID
WINAPI
InitializeCriticalSection(OUT LPCRITICAL_SECTION lpCriticalSection)
{
    NTSTATUS Status;

    /* Initialize the critical section and raise an exception if we failed */
    Status = RtlInitializeCriticalSection(
        (PRTL_CRITICAL_SECTION)lpCriticalSection);
    if (!NT_SUCCESS(Status)) RtlRaiseStatus(Status);
}

/*
 * @implemented
 */
BOOL
WINAPI
InitializeCriticalSectionAndSpinCount(OUT LPCRITICAL_SECTION lpCriticalSection,
                                      IN DWORD dwSpinCount)
{
    NTSTATUS Status;

    /* Initialize the critical section */
    Status = RtlInitializeCriticalSectionAndSpinCount(
        (PRTL_CRITICAL_SECTION)lpCriticalSection,
        dwSpinCount);
    if (!NT_SUCCESS(Status))
    {
        /* Set failure code */
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Success */
    return TRUE;
}


/*
 * @implemented
 */
VOID WINAPI
Sleep(DWORD dwMilliseconds)
{
  SleepEx(dwMilliseconds, FALSE);
  return;
}


/*
 * @implemented
 */
DWORD WINAPI
SleepEx(DWORD dwMilliseconds,
	BOOL bAlertable)
{
  LARGE_INTEGER Interval;
  NTSTATUS errCode;

  if (dwMilliseconds != INFINITE)
    {
      /*
       * System time units are 100 nanoseconds (a nanosecond is a billionth of
       * a second).
       */
      Interval.QuadPart = -((LONGLONG)dwMilliseconds * 10000);
    }
  else
    {
      /* Approximately 292000 years hence */
      Interval.QuadPart = -0x7FFFFFFFFFFFFFFFLL;
    }

dowait:
  errCode = NtDelayExecution ((BOOLEAN)bAlertable, &Interval);
  if ((bAlertable) && (errCode == STATUS_ALERTED)) goto dowait;
  return (errCode == STATUS_USER_APC) ? WAIT_IO_COMPLETION : 0;
}


/*
 * @implemented
 */
BOOL
WINAPI
RegisterWaitForSingleObject(
    PHANDLE phNewWaitObject,
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
{
    NTSTATUS Status = RtlRegisterWait(phNewWaitObject,
                                      hObject,
                                      Callback,
                                      Context,
                                      dwMilliseconds,
                                      dwFlags);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }
    return TRUE;
}


/*
 * @implemented
 */
HANDLE
WINAPI
RegisterWaitForSingleObjectEx(
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
{
    NTSTATUS Status;
    HANDLE hNewWaitObject;

    Status = RtlRegisterWait(&hNewWaitObject,
                             hObject,
                             Callback,
                             Context,
                             dwMilliseconds,
                             dwFlags);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return NULL;
    }

    return hNewWaitObject;
}


/*
 * @implemented
 */
BOOL
WINAPI
UnregisterWait(
    HANDLE WaitHandle
    )
{
    NTSTATUS Status = RtlDeregisterWaitEx(WaitHandle, NULL);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
UnregisterWaitEx(
    HANDLE WaitHandle,
    HANDLE CompletionEvent
    )
{
    NTSTATUS Status = RtlDeregisterWaitEx(WaitHandle, CompletionEvent);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}

/* EOF */
