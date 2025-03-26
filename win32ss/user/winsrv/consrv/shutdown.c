/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/shutdown.c
 * PURPOSE:         Processes Shutdown
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include <psapi.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

static void
NotifyConsoleProcessForShutdown(IN PCSR_PROCESS CsrProcess,
                                IN PCONSOLE_PROCESS_DATA ProcessData,
                                IN ULONG Flags)
{
    DPRINT1("ConsoleClientShutdown(0x%p, 0x%x) - Console process [0x%x, 0x%x]\n",
             CsrProcess, Flags, CsrProcess->ClientId.UniqueProcess, CsrProcess->ClientId.UniqueThread);

    /* Send a log-off event. In reality this should be way more complex */
    ConSrvConsoleCtrlEventTimeout(CTRL_LOGOFF_EVENT, ProcessData,
                                  ShutdownSettings.WaitToKillAppTimeout);
}

static BOOL
NotifyGenericProcessForShutdown(IN PCSR_PROCESS CsrProcess,
                                IN ULONG Flags)
{
    /* FIXME: Implement the generic process shutdown handler */
    UNIMPLEMENTED_ONCE;

    return TRUE;
}

ULONG
NTAPI
NonConsoleProcessShutdown(IN PCSR_PROCESS Process,
                          IN ULONG Flags)
{
    if (NotifyGenericProcessForShutdown(Process, Flags))
    {
        /* Terminate this process */
#if DBG
        WCHAR buffer[MAX_PATH];
        if (!GetProcessImageFileNameW(Process->ProcessHandle, buffer, ARRAYSIZE(buffer)))
        {
            DPRINT1("Terminating process %x\n", Process->ClientId.UniqueProcess);
        }
        else
        {
            DPRINT1("Terminating process %x (%S)\n", Process->ClientId.UniqueProcess, buffer);
        }
#endif
        NtTerminateProcess(Process->ProcessHandle, 0);
        WaitForSingleObject(Process->ProcessHandle, ShutdownSettings.ProcessTerminateTimeout);
    }

    CsrDereferenceProcess(Process);
    return CsrShutdownCsrProcess;
}

// NOTE: See https://web.archive.org/web/20150629001832/http://blogs.msdn.com/b/ntdebugging/archive/2007/06/09/how-windows-shuts-down.aspx
ULONG
NTAPI
ConsoleClientShutdown(IN PCSR_PROCESS CsrProcess,
                      IN ULONG Flags,
                      IN BOOLEAN FirstPhase)
{
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrProcess);

    /* Do not kill system processes when a user is logging off */
    if ((Flags & EWX_SHUTDOWN) == EWX_LOGOFF &&
        (CsrProcess->ShutdownFlags & (CsrShutdownSystem | CsrShutdownOther)))
    {
        DPRINT("Do not kill a system process in a logoff request!\n");
        return CsrShutdownNonCsrProcess;
    }

    /* Is it a console process? */
    if ( ProcessData->ConsoleHandle != NULL ||
         ProcessData->HandleTable   != NULL )
    {
        NotifyConsoleProcessForShutdown(CsrProcess, ProcessData, Flags);

        /* We are done with the process itself */
        CsrDereferenceProcess(CsrProcess);
        return CsrShutdownCsrProcess;
    }
    else
    {
        DPRINT("ConsoleClientShutdown(0x%p, 0x%x, %s) - Non-console process [0x%x, 0x%x]\n",
                CsrProcess, Flags, FirstPhase ? "FirstPhase" : "LastPhase",
                CsrProcess->ClientId.UniqueProcess, CsrProcess->ClientId.UniqueThread);

        /* On first pass, let the gui server terminate all the processes that it owns */
        if (FirstPhase) return CsrShutdownNonCsrProcess;

        /* Use the generic handler since this isn't a gui process */
        return NonConsoleProcessShutdown(CsrProcess, Flags);
    }

    return CsrShutdownNonCsrProcess;
}

/* EOF */
