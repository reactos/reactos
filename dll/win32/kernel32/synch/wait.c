/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/synch/wait.c
 * PURPOSE:         Wrappers for the NT Wait Implementation
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
    switch ((ULONG)hHandle)
    {
        /* Input handle */
        case STD_INPUT_HANDLE:

            /* Read it from the PEB */
            hHandle = NtCurrentPeb()->ProcessParameters->StandardInput;
            break;

        /* Output handle */
        case STD_OUTPUT_HANDLE:

            /* Read it from the PEB */
            hHandle = NtCurrentPeb()->ProcessParameters->StandardOutput;
            break;

        /* Error handle */
        case STD_ERROR_HANDLE:

            /* Read it from the PEB */
            hHandle = NtCurrentPeb()->ProcessParameters->StandardError;
            break;
    }

    /* Check for console handle */
    if ((IsConsoleHandle(hHandle)) && (!VerifyConsoleIoHandle(hHandle)))
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
        switch ((ULONG)HandleBuffer[i])
        {
            /* Input handle */
            case STD_INPUT_HANDLE:
                HandleBuffer[i] = NtCurrentPeb()->
                                  ProcessParameters->StandardInput;
                break;

            /* Output handle */
            case STD_OUTPUT_HANDLE:
                HandleBuffer[i] = NtCurrentPeb()->
                                  ProcessParameters->StandardOutput;
                break;

            /* Error handle */
            case STD_ERROR_HANDLE:
                HandleBuffer[i] = NtCurrentPeb()->
                                  ProcessParameters->StandardError;
                break;
        }

        /* Check for console handle */
        if ((IsConsoleHandle(HandleBuffer[i])) &&
            (!VerifyConsoleIoHandle(HandleBuffer[i])))
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
    switch ((ULONG)hObjectToWaitOn)
    {
        /* Input handle */
        case STD_INPUT_HANDLE:

            /* Read it from the PEB */
            hObjectToWaitOn = NtCurrentPeb()->
                              ProcessParameters->StandardInput;
            break;

        /* Output handle */
        case STD_OUTPUT_HANDLE:

            /* Read it from the PEB */
            hObjectToWaitOn = NtCurrentPeb()->
                              ProcessParameters->StandardOutput;
            break;

        /* Error handle */
        case STD_ERROR_HANDLE:

            /* Read it from the PEB */
            hObjectToWaitOn = NtCurrentPeb()->
                              ProcessParameters->StandardError;
            break;
    }

    /* Check for console handle */
    if ((IsConsoleHandle(hObjectToWaitOn)) &&
        (!VerifyConsoleIoHandle(hObjectToWaitOn)))
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

/* EOF */
