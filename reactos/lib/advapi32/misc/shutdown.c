/* $Id: shutdown.c,v 1.1 1999/05/19 16:43:30 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/misc/shutdown.c
 * PURPOSE:         System shutdown functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 * 	19990413 EA	created
 * 	19990515 EA
 */

#include <windows.h>
#include <ddk/ntddk.h>

#define USZ {0,0,0}

/**********************************************************************
 *	AbortSystemShutdownW
 */
BOOL
STDCALL
AbortSystemShutdownW(
	LPWSTR	lpMachineName
	)
{
	NTSTATUS Status;
	
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	AbortSystemShutdownA
 */
BOOL
STDCALL
AbortSystemShutdownA(
	LPSTR	lpMachineName
	)
{
	UNICODE_STRING	MachineNameW = USZ;
	NTSTATUS	Status;
	BOOL		rv;

	Status = RtlAnsiStringToUnicodeString(
			& MachineNameW,
			lpMachineName,
			TRUE
			);
	if (STATUS_SUCCESS != Status) 
	{
		SetLastError(RtlNtStatusToDosError(Status));
		return FALSE;
	}
	rv = AbortSystemShutdownW(
			MachineNameW.Buffer
			);
	RtlFreeUnicodeString(
			& MachineNameW
			);
	SetLastError(ERROR_SUCCESS);
	return rv;
}


/**********************************************************************
 *	InitiateSystemShutdownW
 */
BOOL
STDCALL
InitiateSystemShutdownW(
	LPWSTR	lpMachineName,
	LPWSTR	lpMessage,
	DWORD	dwTimeout,
	BOOL	bForceAppsClosed,
	BOOL	bRebootAfterShutdown
	)
{
	SHUTDOWN_ACTION	Action = ShutdownNoReboot;
	NTSTATUS	Status;

	if (lpMachineName)
	{
		/* remote machine shutdown not supported yet */
		SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		return FALSE;
	}
	if (dwTimeout)
	{
	}
	Status = NtShutdownSystem(Action);
	SetLastError(RtlNtStatusToDosError(Status));
	return FALSE;
}


/**********************************************************************
 *	InitiateSystemShutdownA
 */
BOOL
STDCALL
InitiateSystemShutdownA(
	LPSTR	lpMachineName,
	LPSTR	lpMessage,
	DWORD	dwTimeout,
	BOOL	bForceAppsClosed,
	BOOL	bRebootAfterShutdown
	)
{
	UNICODE_STRING	MachineNameW = USZ;
	UNICODE_STRING	MessageW = USZ;
	NTSTATUS	Status;
	INT		LastError;
	BOOL		rv;

	if (lpMachineName)
	{
		Status = RtlAnsiStringToUnicodeString(
				& MachineNameW,
				lpMachineName,
				TRUE
				);
		if (STATUS_SUCCESS != Status)
		{
			SetLastError(RtlNtStatusToDosError(Status));
			return FALSE;
		}
	}
	if (lpMessage)
	{
		Status = RtlAnsiStringToUnicodeString(
				& MessageW,
				lpMessage,
				TRUE
				);
		if (STATUS_SUCCESS != Status)
		{
			if (MachineNameW.Length)
			{
				RtlFreeUnicodeString(&MachineNameW);
			}
			SetLastError(RtlNtStatusToDosError(Status));
			return FALSE;
		}
	}
	rv = InitiateSystemShutdownW(
			MachineNameW.Buffer,
			MessageW.Buffer,
			dwTimeout,
			bForceAppsClosed,
			bRebootAfterShutdown
			);
	LastError = GetLastError();
	if (MachineNameW.Length) RtlFreeUnicodeString(&MachineNameW);
	if (MessageW.Length) RtlFreeUnicodeString(&MessageW);
	SetLastError(LastError);
	return rv;
}


/* EOF */

