/* $Id: session.c,v 1.6 2004/09/23 21:01:23 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/session.c
 * PURPOSE:         Win32 session (TS) functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *     2001-12-07 created
 */
#include <k32.h>

DWORD ActiveConsoleSessionId = 0;


/*
 * @unimplemented
 */
DWORD STDCALL
DosPathToSessionPathW (DWORD SessionID, LPWSTR InPath, LPWSTR * OutPath)
{
	return 0;
}

/*
 * From: ActiveVB.DE
 *
 * Declare Function DosPathToSessionPath _
 * Lib "kernel32.dll" _
 * Alias "DosPathToSessionPathA" ( _
 *     ByVal SessionId As Long, _
 *     ByVal pInPath As String, _
 *     ByVal ppOutPath As String ) _
 * As Long
 * 
 * @unimplemented
 */
DWORD STDCALL
DosPathToSessionPathA (DWORD SessionId, LPSTR InPath, LPSTR * OutPath)
{
	//DosPathToSessionPathW (SessionId,InPathW,OutPathW);
	return 0;
}

/*
 * @implemented
 */
BOOL STDCALL ProcessIdToSessionId (IN  DWORD dwProcessId,
				   OUT DWORD* pSessionId)
{
	NTSTATUS           Status = STATUS_SUCCESS;
	HANDLE             ProcessHandle = INVALID_HANDLE_VALUE;
	OBJECT_ATTRIBUTES  Oa = { sizeof (OBJECT_ATTRIBUTES), 0, };
	DWORD              SessionId = 0;

	if (IsBadWritePtr(pSessionId, sizeof (DWORD)))
	{
		SetLastError (ERROR_INVALID_DATA); //FIXME
		goto ProcessIdToSessionId_FAIL_EXIT;
	}
	Status = NtOpenProcess (
			& ProcessHandle,
			PROCESS_QUERY_INFORMATION,
			& Oa,
			NULL);
	if (!NT_SUCCESS(Status))
	{
		goto ProcessIdToSessionId_FAIL;
	}
	Status = NtQueryInformationProcess (
			ProcessHandle,
			ProcessSessionInformation,
			& SessionId,
			sizeof SessionId,
			NULL);
	if (!NT_SUCCESS(Status))
	{
		NtClose (ProcessHandle);
		goto ProcessIdToSessionId_FAIL;
	}
	NtClose (ProcessHandle);
	*pSessionId = SessionId;
	return TRUE;

ProcessIdToSessionId_FAIL:
	SetLastErrorByStatus(Status);
ProcessIdToSessionId_FAIL_EXIT:
	return FALSE;
}

/*
 * @implemented
 */
DWORD STDCALL
WTSGetActiveConsoleSessionId (VOID)
{
	return ActiveConsoleSessionId;
}

/* EOF */
