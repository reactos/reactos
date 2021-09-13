/*
 * PROJECT:         ReactOS NT Layer/System API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/ntdll/dbg/dbgui.c
 * PURPOSE:         Native Wrappers for the NT Debug Implementation
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>

#include <ndk/dbgkfuncs.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
DbgUiConnectToDbg(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* Don't connect twice */
    if (NtCurrentTeb()->DbgSsReserved[1]) return STATUS_SUCCESS;

    /* Setup the Attributes */
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, 0);

    /* Create the object */
    return ZwCreateDebugObject(&NtCurrentTeb()->DbgSsReserved[1],
                               DEBUG_OBJECT_ALL_ACCESS,
                               &ObjectAttributes,
                               DBGK_KILL_PROCESS_ON_EXIT);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DbgUiContinue(IN PCLIENT_ID ClientId,
              IN NTSTATUS ContinueStatus)
{
    /* Tell the kernel object to continue */
    return ZwDebugContinue(NtCurrentTeb()->DbgSsReserved[1],
                           ClientId,
                           ContinueStatus);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DbgUiConvertStateChangeStructure(IN PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
                                 OUT PVOID Win32DebugEvent)
{
    NTSTATUS Status;
    THREAD_BASIC_INFORMATION ThreadBasicInfo;
    LPDEBUG_EVENT DebugEvent = Win32DebugEvent;

    /* Write common data */
    DebugEvent->dwProcessId = PtrToUlong(WaitStateChange->AppClientId.UniqueProcess);
    DebugEvent->dwThreadId = PtrToUlong(WaitStateChange->AppClientId.UniqueThread);

    /* Check what kind of even this is */
    switch (WaitStateChange->NewState)
    {
        /* New thread */
        case DbgCreateThreadStateChange:
        {
            /* Setup Win32 code */
            DebugEvent->dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;

            /* Copy data over */
            DebugEvent->u.CreateThread.hThread =
                WaitStateChange->StateInfo.CreateThread.HandleToThread;
            DebugEvent->u.CreateThread.lpStartAddress =
                WaitStateChange->StateInfo.CreateThread.NewThread.StartAddress;

            /* Query the TEB */
            Status = NtQueryInformationThread(WaitStateChange->StateInfo.
                                              CreateThread.HandleToThread,
                                              ThreadBasicInformation,
                                              &ThreadBasicInfo,
                                              sizeof(ThreadBasicInfo),
                                              NULL);
            if (!NT_SUCCESS(Status))
            {
                /* Failed to get PEB address */
                DebugEvent->u.CreateThread.lpThreadLocalBase = NULL;
            }
            else
            {
                /* Write PEB Address */
                DebugEvent->u.CreateThread.lpThreadLocalBase =
                    ThreadBasicInfo.TebBaseAddress;
            }
            break;
        }

        /* New process */
        case DbgCreateProcessStateChange:
        {
            /* Write Win32 debug code */
            DebugEvent->dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;

            /* Copy data over */
            DebugEvent->u.CreateProcessInfo.hProcess =
                WaitStateChange->StateInfo.CreateProcessInfo.HandleToProcess;
            DebugEvent->u.CreateProcessInfo.hThread =
                WaitStateChange->StateInfo.CreateProcessInfo.HandleToThread;
            DebugEvent->u.CreateProcessInfo.hFile =
                WaitStateChange->StateInfo.CreateProcessInfo.NewProcess.
                FileHandle;
            DebugEvent->u.CreateProcessInfo.lpBaseOfImage =
                WaitStateChange->StateInfo.CreateProcessInfo.NewProcess.
                BaseOfImage;
            DebugEvent->u.CreateProcessInfo.dwDebugInfoFileOffset =
                WaitStateChange->StateInfo.CreateProcessInfo.NewProcess.
                DebugInfoFileOffset;
            DebugEvent->u.CreateProcessInfo.nDebugInfoSize =
                WaitStateChange->StateInfo.CreateProcessInfo.NewProcess.
                DebugInfoSize;
            DebugEvent->u.CreateProcessInfo.lpStartAddress =
                WaitStateChange->StateInfo.CreateProcessInfo.NewProcess.
                InitialThread.StartAddress;

            /* Query TEB address */
            Status = NtQueryInformationThread(WaitStateChange->StateInfo.
                                              CreateProcessInfo.HandleToThread,
                                              ThreadBasicInformation,
                                              &ThreadBasicInfo,
                                              sizeof(ThreadBasicInfo),
                                              NULL);
            if (!NT_SUCCESS(Status))
            {
                /* Failed to get PEB address */
                DebugEvent->u.CreateProcessInfo.lpThreadLocalBase = NULL;
            }
            else
            {
                /* Write PEB Address */
                DebugEvent->u.CreateProcessInfo.lpThreadLocalBase =
                    ThreadBasicInfo.TebBaseAddress;
            }

            /* Clear image name */
            DebugEvent->u.CreateProcessInfo.lpImageName = NULL;
            DebugEvent->u.CreateProcessInfo.fUnicode = TRUE;
            break;
        }

        /* Thread exited */
        case DbgExitThreadStateChange:
        {
            /* Write the Win32 debug code and the exit status */
            DebugEvent->dwDebugEventCode = EXIT_THREAD_DEBUG_EVENT;
            DebugEvent->u.ExitThread.dwExitCode =
                WaitStateChange->StateInfo.ExitThread.ExitStatus;
            break;
        }

        /* Process exited */
        case DbgExitProcessStateChange:
        {
            /* Write the Win32 debug code and the exit status */
            DebugEvent->dwDebugEventCode = EXIT_PROCESS_DEBUG_EVENT;
            DebugEvent->u.ExitProcess.dwExitCode =
                WaitStateChange->StateInfo.ExitProcess.ExitStatus;
            break;
        }

        /* Any sort of exception */
        case DbgExceptionStateChange:
        case DbgBreakpointStateChange:
        case DbgSingleStepStateChange:
        {
            /* Check if this was a debug print */
            if (WaitStateChange->StateInfo.Exception.ExceptionRecord.
                ExceptionCode == DBG_PRINTEXCEPTION_C)
            {
                /* Set the Win32 code */
                DebugEvent->dwDebugEventCode = OUTPUT_DEBUG_STRING_EVENT;

                /* Copy debug string information */
                DebugEvent->u.DebugString.lpDebugStringData =
                    (PVOID)WaitStateChange->
                           StateInfo.Exception.ExceptionRecord.
                           ExceptionInformation[1];
                DebugEvent->u.DebugString.nDebugStringLength =
                    WaitStateChange->StateInfo.Exception.ExceptionRecord.
                    ExceptionInformation[0];
                DebugEvent->u.DebugString.fUnicode = FALSE;
            }
            else if (WaitStateChange->StateInfo.Exception.ExceptionRecord.
                     ExceptionCode == DBG_RIPEXCEPTION)
            {
                /* Set the Win32 code */
                DebugEvent->dwDebugEventCode = RIP_EVENT;

                /* Set exception information */
                DebugEvent->u.RipInfo.dwType =
                    WaitStateChange->StateInfo.Exception.ExceptionRecord.
                    ExceptionInformation[1];
                DebugEvent->u.RipInfo.dwError =
                    WaitStateChange->StateInfo.Exception.ExceptionRecord.
                    ExceptionInformation[0];
            }
            else
            {
                /* Otherwise, this is a debug event, copy info over */
                DebugEvent->dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
                DebugEvent->u.Exception.ExceptionRecord =
                    WaitStateChange->StateInfo.Exception.ExceptionRecord;
                DebugEvent->u.Exception.dwFirstChance =
                    WaitStateChange->StateInfo.Exception.FirstChance;
            }
            break;
        }

        /* DLL Load */
        case DbgLoadDllStateChange:
        {
            /* Set the Win32 debug code */
            DebugEvent->dwDebugEventCode = LOAD_DLL_DEBUG_EVENT;

            /* Copy the rest of the data */
            DebugEvent->u.LoadDll.hFile =
                WaitStateChange->StateInfo.LoadDll.FileHandle;
            DebugEvent->u.LoadDll.lpBaseOfDll =
                WaitStateChange->StateInfo.LoadDll.BaseOfDll;
            DebugEvent->u.LoadDll.dwDebugInfoFileOffset =
                WaitStateChange->StateInfo.LoadDll.DebugInfoFileOffset;
            DebugEvent->u.LoadDll.nDebugInfoSize =
                WaitStateChange->StateInfo.LoadDll.DebugInfoSize;
            DebugEvent->u.LoadDll.lpImageName =
                WaitStateChange->StateInfo.LoadDll.NamePointer;

            /* It's Unicode */
            DebugEvent->u.LoadDll.fUnicode = TRUE;
            break;
        }

        /* DLL Unload */
        case DbgUnloadDllStateChange:
        {
            /* Set Win32 code and DLL Base */
            DebugEvent->dwDebugEventCode = UNLOAD_DLL_DEBUG_EVENT;
            DebugEvent->u.UnloadDll.lpBaseOfDll =
                WaitStateChange->StateInfo.UnloadDll.BaseAddress;
            break;
        }

        /* Anything else, fail */
        default: return STATUS_UNSUCCESSFUL;
    }

    /* Return success */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DbgUiWaitStateChange(OUT PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
                     IN PLARGE_INTEGER TimeOut OPTIONAL)
{
    /* Tell the kernel to wait */
    return NtWaitForDebugEvent(NtCurrentTeb()->DbgSsReserved[1],
                               TRUE,
                               TimeOut,
                               WaitStateChange);
}

/*
 * @implemented
 */
VOID
NTAPI
DbgUiRemoteBreakin(VOID)
{
    /* Make sure a debugger is enabled; if so, breakpoint */
    if (NtCurrentPeb()->BeingDebugged) DbgBreakPoint();

    /* Exit the thread */
    RtlExitUserThread(STATUS_SUCCESS);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DbgUiIssueRemoteBreakin(IN HANDLE Process)
{
    HANDLE hThread;
    CLIENT_ID ClientId;
    NTSTATUS Status;

    /* Create the thread that will do the breakin */
    Status = RtlCreateUserThread(Process,
                                 NULL,
                                 FALSE,
                                 0,
                                 0,
                                 PAGE_SIZE,
                                 (PVOID)DbgUiRemoteBreakin,
                                 NULL,
                                 &hThread,
                                 &ClientId);

    /* Close the handle on success */
    if(NT_SUCCESS(Status)) NtClose(hThread);

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
HANDLE
NTAPI
DbgUiGetThreadDebugObject(VOID)
{
    /* Just return the handle from the TEB */
    return NtCurrentTeb()->DbgSsReserved[1];
}

/*
* @implemented
*/
VOID
NTAPI
DbgUiSetThreadDebugObject(HANDLE DebugObject)
{
    /* Just set the handle in the TEB */
    NtCurrentTeb()->DbgSsReserved[1] = DebugObject;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DbgUiDebugActiveProcess(IN HANDLE Process)
{
    NTSTATUS Status;

    /* Tell the kernel to start debugging */
    Status = NtDebugActiveProcess(Process, NtCurrentTeb()->DbgSsReserved[1]);
    if (NT_SUCCESS(Status))
    {
        /* Now break-in the process */
        Status = DbgUiIssueRemoteBreakin(Process);
        if (!NT_SUCCESS(Status))
        {
            /* We couldn't break-in, cancel debugging */
            DbgUiStopDebugging(Process);
        }
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DbgUiStopDebugging(IN HANDLE Process)
{
    /* Call the kernel to remove the debug object */
    return NtRemoveProcessDebug(Process, NtCurrentTeb()->DbgSsReserved[1]);
}

/* EOF */
