/* $Id: debugger.c,v 1.2 2003/04/02 00:06:00 hyperion Exp $
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

BOOL WINAPI CheckRemoteDebuggerPresent(HANDLE hProcess, PBOOL pbDebuggerPresent)
{
 SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
 return FALSE;
}

BOOL WINAPI ContinueDebugEvent
(
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

 if(!NT_SUCCESS(Status))
 {
  SetLastErrorByStatus(Status);
  return FALSE;
 }

 return TRUE;
}

BOOL WINAPI DebugActiveProcess(DWORD dwProcessId)
{
 SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
 return FALSE;
}

BOOL WINAPI DebugActiveProcessStop(DWORD dwProcessId)
{
 SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
 return FALSE;
}

BOOL WINAPI DebugSetProcessKillOnExit(BOOL KillOnExit)
{
 SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
 return FALSE;
}

BOOL WINAPI IsDebuggerPresent(VOID)
{
 return (WINBOOL)NtCurrentPeb()->BeingDebugged;
}

BOOL WINAPI WaitForDebugEvent(LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds)
{
 SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
 return FALSE;
}

/* EOF */
