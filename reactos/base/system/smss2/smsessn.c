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
SmpSetProcessMuSessionId(IN HANDLE ProcessHandle,
                         IN ULONG SessionId)
{
    NTSTATUS Status;

    /* Tell the kernel about our session ID */
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
