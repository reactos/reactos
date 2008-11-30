/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/exit.c
 * PURPOSE:         Shutdown related functions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

#include <user32.h>

#include <wine/debug.h>

/*
 * Sequence of events:
 *
 * - App (usually explorer) calls ExitWindowsEx()
 * - ExitWindowsEx() sends a message to CSRSS
 * - CSRSS impersonates the caller and sends a message to a hidden WinLogon window
 * - WinLogon checks if the caller has the required privileges
 * - WinLogon enters pending log-out state
 * - WinLogon impersonates the interactive user and calls ExitWindowsEx() again,
 *   passing some special internal flags
 * - CSRSS loops over all processes of the interactive user (sorted by their
 *   SetProcessShutdownParameters() level), sending WM_QUERYENDSESSION and
 *   WM_ENDSESSION messages to its top-level windows. If the messages aren't
 *   processed within the timeout period (registry key HKCU\Control Panel\Desktop\HungAppTimeout)
 *   CSRSS will put up a dialog box asking if the process should be terminated.
 *   Using the registry key HKCU\Control Panel\Desktop\AutoEndTask you can
 *   specify that the dialog box shouldn't be shown and CSRSS should just
 *   terminate the thread. If the the WM_ENDSESSION message is processed
 *   but the thread doesn't terminate within the timeout specified by
 *   HKCU\Control Panel\Desktop\WaitToKillAppTimeout CSRSS will terminate
 *   the thread. When all the top-level windows have been destroyed CSRSS
 *   will terminate the process.
 *   If the process is a console process, CSRSS will send a CTRL_LOGOFF_EVENT
 *   to the console control handler on logoff. No event is sent on shutdown.
 *   If the handler doesn't respond in time the same activities as for GUI
 *   apps (i.e. display dialog box etc) take place. This also happens if
 *   the handler returns TRUE.
 * - This ends the processing for the first ExitWindowsEx() call from WinLogon.
 *   Execution continues in WinLogon, which calls ExitWindowsEx() again to
 *   terminate COM processes in the interactive user's session.
 * - WinLogon stops impersonating the interactive user (whos processes are
 *   all dead by now). and enters log-out state
 * - If the ExitWindowsEx() request was for a logoff, WinLogon sends a SAS
 *   event (to display the "press ctrl+alt+del") to the GINA. WinLogon then
 *   waits for the GINA to send a SAS event to login.
 * - If the ExitWindowsEx() request was for shutdown/restart, WinLogon calls
 *   ExitWindowsEx() again in the system process context.
 * - CSRSS goes through the motions of sending WM_QUERYENDSESSION/WM_ENDSESSION
 *   to GUI processes running in the system process context but won't display
 *   dialog boxes or kill threads/processes. Same for console processes,
 *   using the CTRL_SHUTDOWN_EVENT. The Service Control Manager is one of
 *   these console processes and has a special timeout value WaitToKillServiceTimeout.
 * - WinLogon issues a "InitiateSystemShutdown" request to the SM (SMSS API # 1)
 * - the SM propagates the shutdown request to every environment subsystem it
 *   started since bootstrap time (still active ones, of course)
 * - each environment subsystem, on shutdown request, releases every resource
 *   it aquired during its life (processes, memory etc), then dies
 * - when every environment subsystem has gone to bed, the SM actually initiates
 *   the kernel and executive shutdown by calling NtShutdownSystem.
 */
/*
 * @implemented
 */
BOOL WINAPI
ExitWindowsEx(UINT uFlags,
	      DWORD dwReserved)
{
  CSR_API_MESSAGE Request;
  ULONG CsrRequest;
  NTSTATUS Status;

  CsrRequest = MAKE_CSR_API(EXIT_REACTOS, CSR_GUI);
  Request.Data.ExitReactosRequest.Flags = uFlags;
  Request.Data.ExitReactosRequest.Reserved = dwReserved;

  Status = CsrClientCallServer(&Request,
			       NULL,
                   CsrRequest,
			       sizeof(CSR_API_MESSAGE));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }

  return(TRUE);
}


/*
 * @implemented
 */
BOOL WINAPI
RegisterServicesProcess(DWORD ServicesProcessId)
{
  CSR_API_MESSAGE Request;
  ULONG CsrRequest;
  NTSTATUS Status;

  CsrRequest = MAKE_CSR_API(REGISTER_SERVICES_PROCESS, CSR_GUI);
  Request.Data.RegisterServicesProcessRequest.ProcessId = (HANDLE)ServicesProcessId;

  Status = CsrClientCallServer(&Request,
                   NULL,
			       CsrRequest,
			       sizeof(CSR_API_MESSAGE));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }

  return(TRUE);
}

/* EOF */
