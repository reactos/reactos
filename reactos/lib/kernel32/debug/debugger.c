/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/debug/debugger.c
 * PURPOSE:         Win32 Debugger API
 * PROGRAMMER:      ???
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL WINAPI
CheckRemoteDebuggerPresent (
    HANDLE hProcess,
    PBOOL pbDebuggerPresent
    )
{
  HANDLE DebugPort;
  NTSTATUS Status;

  if (pbDebuggerPresent == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  Status = NtQueryInformationProcess(hProcess,
                                     ProcessDebugPort,
                                     (PVOID)&DebugPort,
                                     sizeof(HANDLE),
                                     NULL);
  if (NT_SUCCESS(Status))
  {
    *pbDebuggerPresent = ((DebugPort != NULL) ? TRUE : FALSE);
    return TRUE;
  }

  SetLastErrorByStatus(Status);
  return FALSE;
}


/*
 * @implemented
 */
BOOL WINAPI
ContinueDebugEvent (
    DWORD dwProcessId,
    DWORD dwThreadId,
    DWORD dwContinueStatus
    )
{
  CLIENT_ID ClientId;
  NTSTATUS Status;

  ClientId.UniqueProcess = (HANDLE)dwProcessId;
  ClientId.UniqueThread = (HANDLE)dwThreadId;

  Status = DbgUiContinue(&ClientId, dwContinueStatus);
  if (!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  return TRUE;
}


/*
 * NOTE: I'm not sure if the function is complete.
 *
 * @unmplemented
 */
BOOL WINAPI
DebugActiveProcess(DWORD dwProcessId)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  HANDLE hProcess;
  NTSTATUS Status;

  hProcess = OpenProcess(PROCESS_ALL_ACCESS,
			 FALSE,
			 dwProcessId);
  if (hProcess == NULL)
    return FALSE;

  /* Notify CSRSS */
  Request.Type = CSRSS_DEBUG_PROCESS;
  Request.Data.DebugProcessRequest.DebuggeeProcessId = dwProcessId;
  Request.Data.DebugProcessRequest.DebuggerProcessId = GetCurrentProcessId();
  Request.Data.DebugProcessRequest.DebuggerThreadId = GetCurrentThreadId();
  Status = CsrClientCallServer(&Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
  }

  CloseHandle(hProcess);

  /* Connect the current process (debugger) to the debug subsystem */
  Status = DbgUiConnectToDbg();
  if (!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  return TRUE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
DebugActiveProcessStop (
    DWORD dwProcessId
    )
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
DebugSetProcessKillOnExit (
    BOOL KillOnExit
    )
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
IsDebuggerPresent (VOID)
{
  return (BOOL)NtCurrentPeb()->BeingDebugged;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
WaitForDebugEvent (
    LPDEBUG_EVENT lpDebugEvent,
    DWORD dwMilliseconds
    )
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/* EOF */
