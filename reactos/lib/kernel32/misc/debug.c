/* $Id: debug.c,v 1.4 2002/09/07 15:12:27 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/debug.c
 * PURPOSE:         Application debugger support functions
 * PROGRAMMER:      ???
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>
#include <kernel32/error.h>
#include <stdarg.h>
#include <stdio.h>


/* FUNCTIONS *****************************************************************/

WINBOOL
STDCALL
ContinueDebugEvent (
	DWORD	dwProcessId,
	DWORD	dwThreadId,
	DWORD	dwContinueStatus
	)
{
	CLIENT_ID ClientId;
	NTSTATUS Status;

	ClientId.UniqueProcess = (HANDLE)dwProcessId;
	ClientId.UniqueThread = (HANDLE)dwThreadId;

	Status = DbgUiContinue (&ClientId,
	                        dwContinueStatus);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return FALSE;
	}
	return TRUE;
}


WINBOOL
STDCALL
DebugActiveProcess (
	DWORD	dwProcessId
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


VOID
STDCALL
DebugBreak (
	VOID
	)
{
	DbgBreakPoint ();
}


WINBOOL
STDCALL
IsDebuggerPresent (
	VOID
	)
{
	return (WINBOOL)NtCurrentPeb ()->BeingDebugged;
}


/*
 * NOTE: Don't call DbgService()!
 *       It's a ntdll internal function and is NOT exported!
 */

VOID STDCALL OutputDebugStringA(LPCSTR lpOutputString)
{
   DbgPrint( (PSTR)lpOutputString );
}

VOID STDCALL OutputDebugStringW(LPCWSTR lpOutputString)
{
   UNICODE_STRING UnicodeOutput;
   ANSI_STRING AnsiString;
   char buff[512];

   UnicodeOutput.Buffer = (WCHAR *)lpOutputString;
   UnicodeOutput.Length = lstrlenW(lpOutputString)*sizeof(WCHAR);
   UnicodeOutput.MaximumLength = UnicodeOutput.Length;
   AnsiString.Buffer = buff;
   AnsiString.MaximumLength = 512;
   AnsiString.Length = 0;
   if( UnicodeOutput.Length > 512 )
     UnicodeOutput.Length = 512;
   if( NT_SUCCESS( RtlUnicodeStringToAnsiString( &AnsiString, &UnicodeOutput, FALSE ) ) )
     DbgPrint( AnsiString.Buffer );
}


WINBOOL
STDCALL
WaitForDebugEvent (
	LPDEBUG_EVENT	lpDebugEvent,
	DWORD		dwMilliseconds
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/* EOF */
