/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/dbg/dbgui.c
 * PURPOSE:         User-Mode DbgUI Support
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
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
    if (NtCurrentTeb()->DbgSsReserved[0]) return STATUS_SUCCESS;

    /* Setup the Attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               0,
                               0,
                               0,
                               0);

    /* Create the object */
    return ZwCreateDebugObject(&NtCurrentTeb()->DbgSsReserved[0],
                               DEBUG_OBJECT_ALL_ACCESS,
                               &ObjectAttributes,
                               TRUE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DbgUiContinue(PCLIENT_ID ClientId,
              ULONG ContinueStatus)
{
    /* Tell the kernel object to continue */
    return ZwDebugContinue(NtCurrentTeb()->DbgSsReserved[0],
                           ClientId,
                           ContinueStatus);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DbgUiWaitStateChange(PDBGUI_WAIT_STATE_CHANGE DbgUiWaitStateCange,
                     PLARGE_INTEGER TimeOut)
{
    /* Tell the kernel to wait */
    return NtWaitForDebugEvent(NtCurrentTeb()->DbgSsReserved[0],
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
DbgUiIssueRemoteBreakin(HANDLE Process)
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

/* EOF */
