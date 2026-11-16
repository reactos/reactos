/*
 * COPYRIGHT:       See COPYING in the top level directory
 *                  Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/synchansi.c
 * PURPOSE:         Synchronization functions (ANSI)
 * PROGRAMMERS:     Ryan Wilson <ryan@verosnt.com>
 */


/* INCLUDES ******************************************************************/

#include <k32.h>

/* CreateWaitableTimerExW lives in client/synch.c; PSDK hides it below Vista. */
HANDLE WINAPI CreateWaitableTimerExW(LPSECURITY_ATTRIBUTES, LPCWSTR, DWORD, DWORD);

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
HANDLE
WINAPI
CreateWaitableTimerA(IN LPSECURITY_ATTRIBUTES lpTimerAttributes OPTIONAL,
                     IN BOOL bManualReset,
                     IN LPCSTR lpTimerName OPTIONAL)
{
    ConvertWin32AnsiObjectApiToUnicodeApi(WaitableTimer, lpTimerName, lpTimerAttributes, bManualReset);
}

/*
 * @implemented
 *
 * CreateWaitableTimerExA wraps CreateWaitableTimerExW (client/synch.c).
 * Firefox PE import: KERNEL32.dll!CreateWaitableTimerExA (mozglue.dll).
 */
HANDLE
WINAPI
CreateWaitableTimerExA(IN LPSECURITY_ATTRIBUTES lpTimerAttributes OPTIONAL,
                       IN LPCSTR lpTimerName OPTIONAL,
                       IN DWORD dwFlags,
                       IN DWORD dwDesiredAccess)
{
    ConvertAnsiToUnicodePrologue

    if (!lpTimerName)
    {
        return CreateWaitableTimerExW(lpTimerAttributes,
                                      NULL,
                                      dwFlags,
                                      dwDesiredAccess);
    }

    ConvertAnsiToUnicodeBody(lpTimerName)

    if (NT_SUCCESS(Status))
    {
        return CreateWaitableTimerExW(lpTimerAttributes,
                                      UnicodeCache->Buffer,
                                      dwFlags,
                                      dwDesiredAccess);
    }

    ConvertAnsiToUnicodeEpilogue
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
    ConvertOpenWin32AnsiObjectApiToUnicodeApi(WaitableTimer, dwDesiredAccess, bInheritHandle, lpTimerName);
}

/*
 * @implemented
 */
HANDLE
WINAPI
DECLSPEC_HOTPATCH
CreateSemaphoreA(IN LPSECURITY_ATTRIBUTES lpSemaphoreAttributes  OPTIONAL,
                 IN LONG lInitialCount,
                 IN LONG lMaximumCount,
                 IN LPCSTR lpName  OPTIONAL)
{
    ConvertWin32AnsiObjectApiToUnicodeApi(Semaphore, lpName, lpSemaphoreAttributes, lInitialCount, lMaximumCount);
}

/*
 * @implemented
 */
HANDLE
WINAPI
DECLSPEC_HOTPATCH
OpenSemaphoreA(IN DWORD dwDesiredAccess,
               IN BOOL bInheritHandle,
               IN LPCSTR lpName)
{
    ConvertOpenWin32AnsiObjectApiToUnicodeApi(Semaphore, dwDesiredAccess, bInheritHandle, lpName);
}

/*
 * @implemented
 */
HANDLE
WINAPI
DECLSPEC_HOTPATCH
OpenMutexA(IN DWORD dwDesiredAccess,
           IN BOOL bInheritHandle,
           IN LPCSTR lpName)
{
    ConvertOpenWin32AnsiObjectApiToUnicodeApi(Mutex, dwDesiredAccess, bInheritHandle, lpName);
}

/* EOF */
