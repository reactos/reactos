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
#include <debug.h>

typedef struct _DBGSS_THREAD_DATA
{
    struct _DBGSS_THREAD_DATA *Next;
    HANDLE ThreadHandle;
    HANDLE ProcessHandle;
    DWORD ProcessId;
    DWORD ThreadId;
    BOOLEAN HandleMarked;
} DBGSS_THREAD_DATA, *PDBGSS_THREAD_DATA;

#define DbgSsSetThreadData(d) \
    NtCurrentTeb()->DbgSsReserved[0] = d

#define DbgSsGetThreadData() \
    ((PDBGSS_THREAD_DATA)NtCurrentTeb()->DbgSsReserved[0])

/* PRIVATE FUNCTIONS *********************************************************/

VOID
WINAPI
SaveThreadHandle(IN DWORD dwProcessId,
                 IN DWORD dwThreadId,
                 IN HANDLE hThread)
{
    PDBGSS_THREAD_DATA ThreadData;

    /* Allocate a thread structure */
    ThreadData = RtlAllocateHeap(RtlGetProcessHeap(),
                                 0,
                                 sizeof(DBGSS_THREAD_DATA));
    if (!ThreadData) return;

    /* Fill it out */
    ThreadData->ThreadHandle = hThread;
    ThreadData->ProcessId = dwProcessId;
    ThreadData->ThreadId = dwThreadId;
    ThreadData->ProcessHandle = NULL;
    ThreadData->HandleMarked = FALSE;

    /* Link it */
    ThreadData->Next = DbgSsGetThreadData();
    DbgSsSetThreadData(ThreadData);
}

VOID
WINAPI
SaveProcessHandle(IN DWORD dwProcessId,
                  IN HANDLE hProcess)
{
    PDBGSS_THREAD_DATA ThreadData;

    /* Allocate a thread structure */
    ThreadData = RtlAllocateHeap(RtlGetProcessHeap(),
                                 0,
                                 sizeof(DBGSS_THREAD_DATA));
    if (!ThreadData) return;

    /* Fill it out */
    ThreadData->ProcessHandle = hProcess;
    ThreadData->ProcessId = dwProcessId;
    ThreadData->ThreadId = 0;
    ThreadData->ThreadHandle = NULL;
    ThreadData->HandleMarked = FALSE;

    /* Link it */
    ThreadData->Next = DbgSsGetThreadData();
    DbgSsSetThreadData(ThreadData);
}

VOID
WINAPI
MarkThreadHandle(IN DWORD dwThreadId)
{
    PDBGSS_THREAD_DATA ThreadData;

    /* Loop all thread data events */
    ThreadData = DbgSsGetThreadData();
    while (ThreadData)
    {
        /* Check if this one matches */
        if (ThreadData->ThreadId == dwThreadId)
        {
            /* Mark the structure and break out */
            ThreadData->HandleMarked = TRUE;
            break;
        }

        /* Move to the next one */
        ThreadData = ThreadData->Next;
    }
}

VOID
WINAPI
MarkProcessHandle(IN DWORD dwProcessId)
{
    PDBGSS_THREAD_DATA ThreadData;

    /* Loop all thread data events */
    ThreadData = DbgSsGetThreadData();
    while (ThreadData)
    {
        /* Check if this one matches */
        if (ThreadData->ProcessId == dwProcessId)
        {
            /* Make sure the thread ID is empty */
            if (!ThreadData->ThreadId)
            {
                /* Mark the structure and break out */
                ThreadData->HandleMarked = TRUE;
                break;
            }
        }

        /* Move to the next one */
        ThreadData = ThreadData->Next;
    }
}

VOID
WINAPI
RemoveHandles(IN DWORD dwProcessId,
              IN DWORD dwThreadId)
{
    PDBGSS_THREAD_DATA ThreadData;

    /* Loop all thread data events */
    ThreadData = DbgSsGetThreadData();
    while (ThreadData)
    {
        /* Check if this one matches */
        if (ThreadData->ProcessId == dwProcessId)
        {
            /* Make sure the thread ID matches too */
            if (ThreadData->ThreadId == dwThreadId)
            {
                /* Check if we have a thread handle */
                if (ThreadData->ThreadHandle)
                {
                    /* Close it */
                    CloseHandle(ThreadData->ThreadHandle);
                }

                /* Check if we have a process handle */
                if (ThreadData->ProcessHandle)
                {
                    /* Close it */
                    CloseHandle(ThreadData->ProcessHandle);
                }

                /* Unlink the thread data */
                DbgSsSetThreadData(ThreadData->Next);

                /* Free it*/
                RtlFreeHeap(RtlGetProcessHeap(), 0, ThreadData);

                /* Move to the next structure */
                ThreadData = DbgSsGetThreadData();
                continue;
            }
        }

        /* Move to the next one */
        ThreadData = ThreadData->Next;
    }
}

VOID
WINAPI
CloseAllProcessHandles(IN DWORD dwProcessId)
{
    PDBGSS_THREAD_DATA ThreadData;

    /* Loop all thread data events */
    ThreadData = DbgSsGetThreadData();
    while (ThreadData)
    {
        /* Check if this one matches */
        if (ThreadData->ProcessId == dwProcessId)
        {
            /* Check if we have a thread handle */
            if (ThreadData->ThreadHandle)
            {
                /* Close it */
                CloseHandle(ThreadData->ThreadHandle);
            }

            /* Check if we have a process handle */
            if (ThreadData->ProcessHandle)
            {
                /* Close it */
                CloseHandle(ThreadData->ProcessHandle);
            }

            /* Unlink the thread data */
            DbgSsSetThreadData(ThreadData->Next);

            /* Free it*/
            RtlFreeHeap(RtlGetProcessHeap(), 0, ThreadData);

            /* Move to the next structure */
            ThreadData = DbgSsGetThreadData();
            continue;
        }

        /* Move to the next one */
        ThreadData = ThreadData->Next;
    }
}

HANDLE
WINAPI
ProcessIdToHandle(IN DWORD dwProcessId)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    CLIENT_ID ClientId;

    /* If we don't have a PID, look it up */
    if (dwProcessId == -1) dwProcessId = (DWORD)CsrGetProcessId();

    /* Open a handle to the process */
    ClientId.UniqueThread = NULL;
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

/* PUBLIC FUNCTIONS **********************************************************/

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

    /* Remove the process/thread handles */
    RemoveHandles(dwProcessId, dwThreadId);

    /* Success */
    return TRUE;
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
    NtClose(Handle);

    /* Check if debugging worked */
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

    /* Close all the process handles */
    CloseAllProcessHandles(dwProcessId);

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
IsDebuggerPresent(VOID)
{
    return (BOOL)NtCurrentPeb()->BeingDebugged;
}

/*
 * @implemented
 */
BOOL
WINAPI
WaitForDebugEvent(IN LPDEBUG_EVENT lpDebugEvent,
                  IN DWORD dwMilliseconds)
{
    LARGE_INTEGER WaitTime;
    PLARGE_INTEGER Timeout;
    DBGUI_WAIT_STATE_CHANGE WaitStateChange;
    NTSTATUS Status;

    /* Check if this is an infinite wait */
    if (dwMilliseconds == INFINITE)
    {
        /* Under NT, this means no timer argument */
        Timeout = NULL;
    }
    else
    {
        /* Otherwise, convert the time to NT Format */
        WaitTime.QuadPart = UInt32x32To64(-10000, dwMilliseconds);
        Timeout = &WaitTime;
    }

    /* Loop while we keep getting interrupted */
    do
    {
        /* Call the native API */
        Status = DbgUiWaitStateChange(&WaitStateChange, Timeout);
    } while ((Status == STATUS_ALERTED) || (Status == STATUS_USER_APC));

    /* Check if the wait failed */
    if (!(NT_SUCCESS(Status)) || (Status == DBG_UNABLE_TO_PROVIDE_HANDLE))
    {
        /* Set the error code and quit */
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Check if we timed out */
    if (Status == STATUS_TIMEOUT)
    {
        /* Fail with a timeout error */
        SetLastError(ERROR_SEM_TIMEOUT);
        return FALSE;
    }

    /* Convert the structure */
    Status = DbgUiConvertStateChangeStructure(&WaitStateChange, lpDebugEvent);
    if (!NT_SUCCESS(Status))
    {
        /* Set the error code and quit */
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Check what kind of event this was */
    switch (lpDebugEvent->dwDebugEventCode)
    {
        /* New thread was created */
        case CREATE_THREAD_DEBUG_EVENT:

            /* Setup the thread data */
            SaveThreadHandle(lpDebugEvent->dwProcessId,
                             lpDebugEvent->dwThreadId,
                             lpDebugEvent->u.CreateThread.hThread);
            break;

        /* New process was created */
        case CREATE_PROCESS_DEBUG_EVENT:

            /* Setup the process data */
            SaveProcessHandle(lpDebugEvent->dwProcessId,
                              lpDebugEvent->u.CreateProcessInfo.hProcess);

            /* Setup the thread data */
            SaveThreadHandle(lpDebugEvent->dwProcessId,
                             lpDebugEvent->dwThreadId,
                             lpDebugEvent->u.CreateThread.hThread);
            break;

        /* Process was exited */
        case EXIT_PROCESS_DEBUG_EVENT:

            /* Mark the thread data as such */
            MarkProcessHandle(lpDebugEvent->dwProcessId);
            break;

        /* Thread was exited */
        case EXIT_THREAD_DEBUG_EVENT:

            /* Mark the thread data */
            MarkThreadHandle(lpDebugEvent->dwThreadId);
            break;

        /* Nothing to do for anything else */
        default:
            break;
    }

    /* Return success */
    return TRUE;
}

/* EOF */
