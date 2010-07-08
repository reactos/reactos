/*
 * PROJECT:         ReactOS Client Server Runtime SubSystem (CSRSS)
 * LICENSE:         BSD - See COPYING.ARM in root directory
 * FILE:            subsystems/win32/csrss/csrss.c
 * PURPOSE:         Main Executable Code
 * PROGRAMMERS:     Alex Ionescu
 *                  ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/ntndk.h>
#include <api.h>
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
    KPRIORITY BasePriority = (8 + 1) + 4;
    NTSTATUS Status;

    /* Set the Priority */
    NtSetInformationProcess(NtCurrentProcess(),
                            ProcessBasePriority,
                            &BasePriority,
                            sizeof(KPRIORITY));

    /* Initialize CSR through CSRSRV */
    Status = CsrServerInitialization(argc, argv);
    if (!NT_SUCCESS(Status))
    {
        /* Kill us */
        DPRINT1("CSRSS: CsrServerInitialization failed:% lx\n", Status);
        NtTerminateProcess(NtCurrentProcess(), Status);
    }

    /* Disable errors */
    CsrpSetDefaultProcessHardErrorMode();

    /* If this is Session 0, make sure killing us bugchecks the system */
    if (!NtCurrentPeb()->SessionId) RtlSetProcessIsCritical(TRUE, NULL, FALSE);

    /* Kill this thread. CSRSRV keeps us going */
    NtTerminateThread (NtCurrentThread(), Status);
    return 0;
}

/* EOF */
