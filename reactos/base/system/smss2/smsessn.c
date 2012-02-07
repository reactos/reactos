/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smss.c
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS ********************************************************************/

RTL_CRITICAL_SECTION SmpSessionListLock;
LIST_ENTRY SmpSessionListHead;
ULONG SmpNextSessionId;
ULONG SmpNextSessionIdScanMode;
BOOLEAN SmpDbgSsLoaded;
HANDLE SmpSessionsObjectDirectory;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
SmpGetProcessMuSessionId(IN HANDLE ProcessHandle,
                         OUT PULONG SessionId)
{
    NTSTATUS Status;
    ULONG ProcessSession;

    /* Query the kernel for the session ID */
    Status = NtQueryInformationProcess(ProcessHandle,
                                       ProcessSessionInformation,
                                       &ProcessSession,
                                       sizeof(ProcessSession),
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Copy it back into the buffer */
        *SessionId = ProcessSession;
    }
    else
    {
        /* Failure -- assume session zero */
        DPRINT1("SMSS: GetProcessMuSessionId, Process=%x, Status=%x\n",
                ProcessHandle, Status);
        *SessionId = 0;
    }
    
    /* Return result */
    return Status;
}

NTSTATUS
NTAPI
SmpSetProcessMuSessionId(IN HANDLE ProcessHandle,
                         IN ULONG SessionId)
{
    NTSTATUS Status;

    /* Tell the kernel about the session ID */
    Status = NtSetInformationProcess(ProcessHandle,
                                     ProcessSessionInformation,
                                     &SessionId,
                                     sizeof(SessionId));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: SetProcessMuSessionId, Process=%x, Status=%x\n",
                ProcessHandle, Status);
    }

    /* Return */
    return Status;
}
