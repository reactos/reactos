/* $Id: comm.c,v 1.1 2001/03/31 01:17:29 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/comm.c
 * PURPOSE:         Comm functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  modified from WINE [ Onno Hovers, (onno@stack.urc.tue.nl) ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <ddk/ntddk.h>
#include <kernel32/atom.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <wchar.h>
#include <string.h>
//#include <stdlib.h>

WINBOOL
STDCALL
BuildCommDCBA (
	LPCSTR	lpDef,
	LPDCB	lpDCB
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
BuildCommDCBW (
	LPCWSTR	lpDef,
	LPDCB	lpDCB
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
BuildCommDCBAndTimeoutsA (
	LPCSTR		lpDef,
	LPDCB		lpDCB,
	LPCOMMTIMEOUTS	lpCommTimeouts
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
BuildCommDCBAndTimeoutsW (
	LPCWSTR		lpDef,
	LPDCB		lpDCB,
	LPCOMMTIMEOUTS	lpCommTimeouts
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
ClearCommBreak (
	HANDLE	hFile
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
ClearCommError (
	HANDLE		hFile,
	LPDWORD		lpErrors,
	LPCOMSTAT	lpStat
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
CommConfigDialogA (
	LPCSTR		lpszName,
	HWND		hWnd,
	LPCOMMCONFIG	lpCC
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
CommConfigDialogW (
	LPCWSTR		lpszName,
	HWND		hWnd,
	LPCOMMCONFIG	lpCC
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
EscapeCommFunction (
	HANDLE	hFile,
	DWORD	dwFunc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
GetCommConfig (
	HANDLE		hCommDev,
	LPCOMMCONFIG	lpCC,
	LPDWORD		lpdwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetCommMask (
	HANDLE	hFile,
	LPDWORD	lpEvtMask
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



WINBOOL
STDCALL
GetCommModemStatus (
	HANDLE	hFile,
	LPDWORD	lpModemStat
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetCommProperties (
	HANDLE		hFile,
	LPCOMMPROP	lpCommProp
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetCommState (
	HANDLE hFile,
	LPDCB lpDCB
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetCommTimeouts (
	HANDLE		hFile,
	LPCOMMTIMEOUTS	lpCommTimeouts
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
GetDefaultCommConfigW (
	LPCWSTR		lpszName,
	LPCOMMCONFIG	lpCC,
	LPDWORD		lpdwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetDefaultCommConfigA (
	LPCSTR		lpszName,
	LPCOMMCONFIG	lpCC,
	LPDWORD		lpdwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
PurgeComm (
	HANDLE	hFile,
	DWORD	dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
SetCommBreak (
	HANDLE	hFile
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetCommConfig (
	HANDLE		hCommDev,
	LPCOMMCONFIG	lpCC,
	DWORD		dwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetCommMask (
	HANDLE	hFile,
	DWORD	dwEvtMask
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetCommState (
	HANDLE	hFile,
	LPDCB	lpDCB
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetCommTimeouts (
	HANDLE		hFile,
	LPCOMMTIMEOUTS	lpCommTimeouts
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
SetDefaultCommConfigA (
	LPCSTR		lpszName,
	LPCOMMCONFIG	lpCC,
	DWORD		dwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetDefaultCommConfigW (
	LPCWSTR		lpszName,
	LPCOMMCONFIG	lpCC,
	DWORD		dwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
SetupComm (
	HANDLE	hFile,
	DWORD	dwInQueue,
	DWORD	dwOutQueue
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
TransmitCommChar (
	HANDLE	hFile,
	char	cChar
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
WaitCommEvent (
	HANDLE		hFile,
	LPDWORD		lpEvtMask,
	LPOVERLAPPED	lpOverlapped
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/* EOF */
