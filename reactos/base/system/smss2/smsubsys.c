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

RTL_CRITICAL_SECTION SmpKnownSubSysLock;
LIST_ENTRY SmpKnownSubSysHead;
HANDLE SmpWindowsSubSysProcess;
HANDLE SmpWindowsSubSysProcessId;
BOOLEAN RegPosixSingleInstance;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
SmpLoadSubSystemsForMuSession(IN PULONG MuSessionId,
                              OUT PHANDLE ProcessId,
                              IN PUNICODE_STRING InitialCommand)
{
    DPRINT1("Should start subsystems for Session: %lx\n", *MuSessionId);
    return STATUS_SUCCESS;
}

