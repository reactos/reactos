/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dbgkp.h

Abstract:

    This header file describes private data structures and functions
    that make up the kernel mode portion of the Dbg subsystem.

Author:

    Mark Lucovsky (markl) 19-Jan-1990

[Environment:]

    optional-environment-info (e.g. kernel mode only...)

[Notes:]

    optional-notes

Revision History:

--*/

#ifndef _DBGKP_
#define _DBGKP_

#include "ntos.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include <zwapi.h>
#include <string.h>

NTSTATUS
DbgkpSendApiMessage(
    IN OUT PDBGKM_APIMSG ApiMsg,
    IN PVOID Port,
    IN BOOLEAN SuspendProcess
    );

VOID
DbgkpSuspendProcess(
    IN BOOLEAN CreateDeleteLockHeld
    );

VOID
DbgkpResumeProcess(
    IN BOOLEAN CreateDeleteLockHeld
    );

HANDLE
DbgkpSectionHandleToFileHandle(
    IN HANDLE SectionHandle
    );

#endif // _DBGKP_
