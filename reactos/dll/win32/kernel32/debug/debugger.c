/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/debug/debugger.c
 * PURPOSE:         Wrappers for the NT Debug Implementation
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
BOOL
WINAPI
CheckRemoteDebuggerPresent(IN HANDLE hProcess,
                           OUT PBOOL pbDebuggerPresent)
{
    HANDLE DebugPort;
    NTSTATUS Status;

    /* Make sure we have an output and process*/
    if (!(pbDebuggerPresent) || !(hProcess))
    {
        /* Fail */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Check if the process has a debug object/port */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessDebugPort,
                                       (PVOID)&DebugPort,
                                       sizeof(HANDLE),
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Return the current state */
        *pbDebuggerPresent = (DebugPort) ? TRUE : FALSE;
        return TRUE;
    }

    /* Otherwise, fail */
    SetLastErrorByStatus(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
ContinueDebugEvent(IN DWORD dwProcessId,
                   IN DWORD dwThreadId,
                   IN DWORD dwContinueStatus)
{
    CLIENT_ID ClientId;
    NTSTATUS Status;

    /* Set the Client ID */
    ClientId.UniqueProcess = (HANDLE)dwProcessId;
    ClientId.UniqueThread = (HANDLE)dwThreadId;

    /* Continue debugging */
    Status = DbgUiContinue(&ClientId, dwContinueStatus);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Succes */
    return TRUE;
}

HANDLE
ProcessIdToHandle(IN DWORD dwProcessId)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    CLIENT_ID ClientId;

    /* If we don't have a PID, look it up */
    if (dwProcessId == 0xFFFFFFFF) dwProcessId = (DWORD)CsrGetProcessId();

    /* Open a handle to the process */
    ClientId.UniqueProcess = (HANDLE)dwProcessId;
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = NtOpenProcess(&Handle,
                           PROCESS_ALL_ACCESS,
                           &ObjectAttributes,
                           &ClientId);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        SetLastErrorByStatus(Status);
        return 0;
    }

    /* Return the handle */
    return Handle;
}

/*
 * @implemented
 */
BOOL
WINAPI
DebugActiveProcess(IN DWORD dwProcessId)
{
    NTSTATUS Status;
    HANDLE Handle;

    /* Connect to the debugger */
    Status = DbgUiConnectToDbg();
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Get the process handle */
    Handle = ProcessIdToHandle(dwProcessId);
    if (!Handle) return FALSE;

    /* Now debug the process */
    Status = DbgUiDebugActiveProcess(Handle);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Success */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DebugActiveProcessStop(IN DWORD dwProcessId)
{
    NTSTATUS Status;
    HANDLE Handle;

    /* Get the process handle */
    Handle = ProcessIdToHandle(dwProcessId);
    if (!Handle) return FALSE;

    /* Now stop debgging the process */
    Status = DbgUiStopDebugging(Handle);
    NtClose(Handle);

    /* Check for failure */
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    /* Success */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DebugBreakProcess(IN HANDLE Process)
{
    NTSTATUS Status;

    /* Send the breakin request */
    Status = DbgUiIssueRemoteBreakin(Process);
    if(!NT_SUCCESS(Status))
    {
        /* Failure */
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Success */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DebugSetProcessKillOnExit(IN BOOL KillOnExit)
{
    HANDLE Handle;
    NTSTATUS Status;
    ULONG State;

    /* Get the debug object */
    Handle = DbgUiGetThreadDebugObject();
    if (!Handle)
    {
        /* Fail */
        SetLastErrorByStatus(STATUS_INVALID_HANDLE);
        return FALSE;
    }

    /* Now set the kill-on-exit state */
    State = KillOnExit;
    Status = NtSetInformationDebugObject(Handle,
                                         DebugObjectKillProcessOnExitInformation,
                                         &State,
                                         sizeof(State),
                                         NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        SetLastError(Status);
        return FALSE;
    }

    /* Success */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsDebuggerPresent (VOID)
{
    return (BOOL)NtCurrentPeb()->BeingDebugged;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
WaitForDebugEvent(IN LPDEBUG_EVENT lpDebugEvent,
                  DWORD dwMilliseconds)
{
    /* FIXME: TODO */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/* EOF */
