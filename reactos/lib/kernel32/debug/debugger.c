/* $Id: debugger.c,v 1.3 2003/07/10 18:50:50 chorns Exp $
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
 * @unimplemented
 */
BOOL WINAPI CheckRemoteDebuggerPresent(HANDLE hProcess, PBOOL pbDebuggerPresent)
{
 SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
 return FALSE;
}


/*
 * @implemented
 */
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


/*
 * @unimplemented
 */
BOOL WINAPI DebugActiveProcess(DWORD dwProcessId)
{
 SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
 return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI DebugActiveProcessStop(DWORD dwProcessId)
{
 SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
 return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI DebugSetProcessKillOnExit(BOOL KillOnExit)
{
 SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
 return FALSE;
}


/*
 * @implemented
 */
BOOL WINAPI IsDebuggerPresent(VOID)
{
 return (WINBOOL)NtCurrentPeb()->BeingDebugged;
}


/*
 * @unimplemented
 */
BOOL WINAPI WaitForDebugEvent(LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds)
{
 SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
 return FALSE;
}

/* EOF */
