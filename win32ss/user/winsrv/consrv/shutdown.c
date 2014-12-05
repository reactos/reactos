/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/shutdown.c
 * PURPOSE:         Processes Shutdown
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

// NOTE: See http://blogs.msdn.com/b/ntdebugging/archive/2007/06/09/how-windows-shuts-down.aspx
ULONG
NTAPI
ConsoleClientShutdown(IN PCSR_PROCESS CsrProcess,
                      IN ULONG Flags,
                      IN BOOLEAN FirstPhase)
{
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrProcess);

    UNIMPLEMENTED;

    if ( ProcessData->ConsoleHandle != NULL ||
         ProcessData->HandleTable   != NULL )
    {
        DPRINT1("ConsoleClientShutdown(0x%p, 0x%x, %s) - Console process [0x%x, 0x%x]\n",
                CsrProcess, Flags, FirstPhase ? "FirstPhase" : "LastPhase",
                CsrProcess->ClientId.UniqueProcess, CsrProcess->ClientId.UniqueThread);

        /* We are done with the process itself */
        CsrDereferenceProcess(CsrProcess);
        return CsrShutdownCsrProcess;
    }
    else
    {
        DPRINT1("ConsoleClientShutdown(0x%p, 0x%x, %s) - Non-console process [0x%x, 0x%x]\n",
                CsrProcess, Flags, FirstPhase ? "FirstPhase" : "LastPhase",
                CsrProcess->ClientId.UniqueProcess, CsrProcess->ClientId.UniqueThread);

        /* On first pass, ignore the process since the GUI server should take it... */
        if (FirstPhase) return CsrShutdownNonCsrProcess;

        /* ... otherwise, call the generic handler */
        // FIXME: Should call a generic shutdown handler!!
        CsrDereferenceProcess(CsrProcess);
        return CsrShutdownCsrProcess;
    }

    return CsrShutdownNonCsrProcess;
}

/* EOF */
