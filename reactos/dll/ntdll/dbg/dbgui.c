/*
 * PROJECT:         ReactOS NT Layer/System API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/ntdll/dbg/dbgui.c
 * PURPOSE:         Native Wrappers for the NT Debug Implementation
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
DebugService(IN ULONG Service,
             IN PVOID Buffer,
             IN ULONG Length,
             IN PVOID Argument1,
             IN PVOID Argument2);

NTSTATUS
NTAPI
DebugPrint(IN PANSI_STRING DebugString,
           IN ULONG ComponentId,
           IN ULONG Level)
{
    /* Call the INT2D Service */
    return DebugService(BREAKPOINT_PRINT,
                        DebugString->Buffer,
                        DebugString->Length,
                        UlongToPtr(ComponentId),
                        UlongToPtr(Level));
}

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
                               TRUE);
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
 * @unimplemented
 */
NTSTATUS
NTAPI
DbgUiConvertStateChangeStructure(IN PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
                                 IN LPDEBUG_EVENT DebugEvent)
{
    /* FIXME: UNIMPLEMENTED */
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DbgUiWaitStateChange(OUT PDBGUI_WAIT_STATE_CHANGE DbgUiWaitStateCange,
                     IN PLARGE_INTEGER TimeOut OPTIONAL)
{
    /* Tell the kernel to wait */
    return NtWaitForDebugEvent(NtCurrentTeb()->DbgSsReserved[1],
                               TRUE,
                               TimeOut,
                               DbgUiWaitStateCange);
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
