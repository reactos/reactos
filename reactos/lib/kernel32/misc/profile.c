/* $Id: profile.c,v 1.2 2001/05/27 15:40:31 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/profile.c
 * PURPOSE:         Profiles functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  modified from WINE [ Onno Hovers, (onno@stack.urc.tue.nl) ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <ddk/ntddk.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <wchar.h>
#include <string.h>
//#include <stdlib.h>

BOOL STDCALL
CloseProfileUserMapping (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

UINT
STDCALL
GetPrivateProfileIntW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpKeyName,
	INT	nDefault,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
STDCALL
GetPrivateProfileIntA (
	LPCSTR	lpAppName,
	LPCSTR	lpKeyName,
	INT	nDefault,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileSectionW (
	LPCWSTR	lpAppName,
	LPWSTR	lpReturnedString,
	DWORD	nSize,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileSectionA (
	LPCSTR	lpAppName,
	LPSTR	lpReturnedString,
	DWORD	nSize,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileSectionNamesW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileSectionNamesA (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileStringW (
	LPCWSTR lpAppName,
	LPCWSTR lpKeyName,
	LPCWSTR lpDefault,
	LPWSTR	lpReturnedString,
	DWORD	nSize,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileStringA (
	LPCSTR	lpAppName,
	LPCSTR	lpKeyName,
	LPCSTR	lpDefault,
	LPSTR	lpReturnedString,
	DWORD	nSize,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileStructW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileStructA (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

UINT
STDCALL
GetProfileIntW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpKeyName,
	INT	nDefault
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
STDCALL
GetProfileIntA (
	LPCSTR	lpAppName,
	LPCSTR	lpKeyName,
	INT	nDefault
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetProfileSectionW (
	LPCWSTR	lpAppName,
	LPWSTR	lpReturnedString,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetProfileSectionA (
	LPCSTR	lpAppName,
	LPSTR	lpReturnedString,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetProfileStringW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpKeyName,
	LPCWSTR	lpDefault,
	LPWSTR	lpReturnedString,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetProfileStringA (
	LPCSTR	lpAppName,
	LPCSTR	lpKeyName,
	LPCSTR	lpDefault,
	LPSTR	lpReturnedString,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

DWORD
STDCALL
OpenProfileUserMapping (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

WINBOOL
STDCALL
QueryWin31IniFilesMappedToRegistry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
WritePrivateProfileSectionA (
	LPCSTR	lpAppName,
	LPCSTR	lpString,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WritePrivateProfileSectionW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpString,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WritePrivateProfileStringA (
	LPCSTR	lpAppName,
	LPCSTR	lpKeyName,
	LPCSTR	lpString,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WritePrivateProfileStringW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpKeyName,
	LPCWSTR	lpString,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WritePrivateProfileStructA (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WritePrivateProfileStructW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}




WINBOOL
STDCALL
WriteProfileSectionA (
	LPCSTR	lpAppName,
	LPCSTR	lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WriteProfileSectionW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WriteProfileStringA (
	LPCSTR	lpAppName,
	LPCSTR	lpKeyName,
	LPCSTR	lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WriteProfileStringW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpKeyName,
	LPCWSTR	lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



/* EOF */
