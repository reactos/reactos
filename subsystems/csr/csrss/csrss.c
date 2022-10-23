/*
 * LICENSE:         BSD - See COPYING.ARM in root directory
 * PROJECT:         ReactOS Client/Server Runtime SubSystem
 * FILE:            subsystems/win32/csrss/csrss.c
 * PURPOSE:         CSRSS Process Main Executable Code
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#define NTOS_MODE_USER
#include <ndk/exfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>

#include <csr/csrsrv.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
CsrpSetDefaultProcessHardErrorMode(VOID)
{
    ULONG DefaultHardErrorMode = 0;

    /* Disable hard errors */
    NtSetInformationProcess(NtCurrentProcess(),
                            ProcessDefaultHardErrorMode,
                            &DefaultHardErrorMode,
                            sizeof(DefaultHardErrorMode));
}

int
_cdecl
_main(int argc,
      char *argv[],
      char *envp[],
      int DebugFlag)
{
    KPRIORITY BasePriority = PROCESS_PRIORITY_NORMAL_FOREGROUND + 4;
    NTSTATUS Status;
#if defined (_X86_)
    ULONG Response;
#endif
    UNREFERENCED_PARAMETER(envp);
    UNREFERENCED_PARAMETER(DebugFlag);

    /* Set the base priority */
    NtSetInformationProcess(NtCurrentProcess(),
                            ProcessBasePriority,
                            &BasePriority,
                            sizeof(BasePriority));

#if defined (_X86_)
    /* Give us IOPL so that we can access the VGA registers */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessUserModeIOPL,
                                     NULL,
                                     0);
    if (!NT_SUCCESS(Status))
    {
        /* Raise a hard error */
        DPRINT1("CSRSS: Could not raise IOPL, Status: 0x%08lx\n", Status);
        Status = NtRaiseHardError(STATUS_IO_PRIVILEGE_FAILED,
                                  0,
                                  0,
                                  NULL,
                                  OptionOk,
                                  &Response);
    }
#endif

    /* Initialize CSR through CSRSRV */
    Status = CsrServerInitialization(argc, argv);
    if (!NT_SUCCESS(Status))
    {
        /* Kill us */
        DPRINT1("CSRSS: Unable to initialize server, Status: 0x%08lx\n", Status);
        NtTerminateProcess(NtCurrentProcess(), Status);
    }

    /* Disable errors */
    CsrpSetDefaultProcessHardErrorMode();

    /* If this is Session 0, make sure killing us bugchecks the system */
    if (NtCurrentPeb()->SessionId == 0) RtlSetProcessIsCritical(TRUE, NULL, FALSE);

    /* Kill this thread. CSRSRV keeps us going */
    NtTerminateThread(NtCurrentThread(), Status);
    return 0;
}

/* EOF */
