/* $Id: mailslot.c,v 1.1 2001/03/31 01:17:29 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/mailslot.c
 * PURPOSE:         Mailslot functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>
#include <wchar.h>
#include <string.h>
#include <ntdll/rtl.h>

#include <kernel32/kernel32.h>
#include <kernel32/error.h>

/* FUNCTIONS ****************************************************************/

HANDLE
STDCALL
CreateMailslotA (
	LPCSTR			lpName,
	DWORD			nMaxMessageSize,
	DWORD			lReadTimeout,
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HANDLE
STDCALL
CreateMailslotW (
	LPCWSTR			lpName,
	DWORD			nMaxMessageSize,
	DWORD			lReadTimeout,
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}

WINBOOL
STDCALL
GetMailslotInfo (
	HANDLE	hMailslot,
	LPDWORD	lpMaxMessageSize,
	LPDWORD	lpNextSize,
	LPDWORD	lpMessageCount,
	LPDWORD	lpReadTimeout
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
SetMailslotInfo (
	HANDLE	hMailslot,
	DWORD	lReadTimeout
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* EOF */
