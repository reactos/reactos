/* $Id: privilege.c,v 1.3 2002/09/08 10:22:37 chorns Exp $ 
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/token/privilege.c
 * PURPOSE:         advapi32.dll token's privilege handling
 * PROGRAMMER:      E.Aliberti
 * UPDATE HISTORY:
 *	20010317 ea	stubs
 */
#include <windows.h>
#include <ddk/ntddk.h>


/**********************************************************************
 *	LookupPrivilegeValueA				EXPORTED
 *	LookupPrivilegeValueW				EXPORTED
 */
BOOL STDCALL LookupPrivilegeValueA (
	LPCSTR	lpSystemName, 
	LPCSTR	lpName, 
	PLUID	lpLuid 
	)
{
	BOOL		rv = FALSE;
	DWORD		le = ERROR_SUCCESS;

	ANSI_STRING	SystemNameA;
	UNICODE_STRING	SystemNameW;
	ANSI_STRING	NameA;
	UNICODE_STRING	NameW;

	HANDLE		ProcessHeap = GetProcessHeap ();


	/* Remote system? */
	if (NULL != lpSystemName)
	{
		RtlInitAnsiString (
			& SystemNameA,
			(LPSTR) lpSystemName
			);
		RtlAnsiStringToUnicodeString (
			& SystemNameW,
			& SystemNameA,
			TRUE
			);
	}
	/* Check the privilege name is not NULL */
	if (NULL != lpName)
	{
		RtlInitAnsiString (
			& NameA,
			(LPSTR) lpName
			);
		RtlAnsiStringToUnicodeString (
			& NameW,
			& NameA,
			TRUE
			);
	}
	else
	{
		SetLastError (ERROR_INVALID_PARAMETER);
		return (FALSE);
	}
	/* 
	 * Forward the call to the UNICODE version
	 * of this API.
	 */
	if (FALSE == (rv = LookupPrivilegeValueW (
				(lpSystemName ? SystemNameW.Buffer : NULL), 
				NameW.Buffer, 
				lpLuid 
				)
			)
		)
	{
		le = GetLastError ();
	}
	/* Remote system? */
	if (NULL != lpSystemName)
	{
		RtlFreeHeap (
			ProcessHeap,
			0,
			SystemNameW.Buffer
			);
	}
	/* Name */
	RtlFreeHeap (
		ProcessHeap,
		0,
	       NameW.Buffer
	       );
	/*
	 * Set the last error, if any reported by
	 * the UNICODE call.
	 */
	if (ERROR_SUCCESS != le)
	{
		SetLastError (le);
	}
	return (rv);
}
 

BOOL STDCALL LookupPrivilegeValueW (
	LPCWSTR	lpSystemName, 
	LPCWSTR	lpName, 
	PLUID	lpLuid 
	)
{
	SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
	return (FALSE);
}


/**********************************************************************
 *	LookupPrivilegeDisplayNameA			EXPORTED
 *	LookupPrivilegeDisplayNameW			EXPORTED
 */
BOOL STDCALL LookupPrivilegeDisplayNameA (
	LPCSTR	lpSystemName, 
	LPCSTR	lpName, 
	LPSTR	lpDisplayName, 
	LPDWORD	cbDisplayName, 
	LPDWORD	lpLanguageId 
	)
{
	SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
	return (FALSE);
}
	

BOOL STDCALL LookupPrivilegeDisplayNameW (
	LPCWSTR	lpSystemName, 
	LPCWSTR	lpName, 
	LPWSTR	lpDisplayName, 
	LPDWORD	cbDisplayName, 
	LPDWORD	lpLanguageId 
	)
{
	SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
	return (FALSE);
}


/**********************************************************************
 *	LookupPrivilegeNameA				EXPORTED
 *	LookupPrivilegeNameW				EXPORTED
 */
BOOL STDCALL LookupPrivilegeNameA (
	LPCSTR	lpSystemName, 
	PLUID	lpLuid, 
	LPSTR	lpName, 
	LPDWORD	cbName 
	) 
{
	SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
	return (FALSE);
}
 

BOOL STDCALL LookupPrivilegeNameW (
	LPCWSTR	lpSystemName, 
	PLUID	lpLuid, 
	LPWSTR	lpName, 
	LPDWORD	cbName 
	) 
{
	SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
	return (FALSE);
}
 

/* EOF */
