/* $Id: profile.c,v 1.7 2003/07/10 18:50:51 chorns Exp $
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

#include <k32.h>


/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
BOOL STDCALL
CloseProfileUserMapping(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
UINT STDCALL
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


/*
 * @unimplemented
 */
UINT STDCALL
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


/*
 * @unimplemented
 */
DWORD STDCALL
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


/*
 * @unimplemented
 */
DWORD STDCALL
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


/*
 * @unimplemented
 */
DWORD STDCALL
GetPrivateProfileSectionNamesW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetPrivateProfileSectionNamesA (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
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


/*
 * @unimplemented
 */
DWORD STDCALL
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


/*
 * @unimplemented
 */
DWORD STDCALL
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


/*
 * @unimplemented
 */
DWORD STDCALL
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


/*
 * @unimplemented
 */
UINT STDCALL
GetProfileIntW(LPCWSTR lpAppName,
	       LPCWSTR lpKeyName,
	       INT nDefault)
{
   return GetPrivateProfileIntW(lpAppName,
				lpKeyName,
				nDefault,
				NULL);
}


/*
 * @unimplemented
 */
UINT STDCALL
GetProfileIntA(LPCSTR lpAppName,
	       LPCSTR lpKeyName,
	       INT nDefault)
{
   return GetPrivateProfileIntA(lpAppName,
				lpKeyName,
				nDefault,
				NULL);
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetProfileSectionW(LPCWSTR lpAppName,
		   LPWSTR lpReturnedString,
		   DWORD nSize)
{
   return GetPrivateProfileSectionW(lpAppName,
				    lpReturnedString,
				    nSize,
				    NULL);
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetProfileSectionA(LPCSTR lpAppName,
		   LPSTR lpReturnedString,
		   DWORD nSize)
{
   return GetPrivateProfileSectionA(lpAppName,
				    lpReturnedString,
				    nSize,
				    NULL);
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetProfileStringW(LPCWSTR lpAppName,
		  LPCWSTR lpKeyName,
		  LPCWSTR lpDefault,
		  LPWSTR lpReturnedString,
		  DWORD nSize)
{
   return GetPrivateProfileStringW(lpAppName,
				   lpKeyName,
				   lpDefault,
				   lpReturnedString,
				   nSize,
				   NULL);
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetProfileStringA(LPCSTR lpAppName,
		  LPCSTR lpKeyName,
		  LPCSTR lpDefault,
		  LPSTR lpReturnedString,
		  DWORD nSize)
{
   return GetPrivateProfileStringA(lpAppName,
				   lpKeyName,
				   lpDefault,
				   lpReturnedString,
				   nSize,
				   NULL);
}


/*
 * @unimplemented
 */
BOOL STDCALL
OpenProfileUserMapping (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
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


/*
 * @unimplemented
 */
WINBOOL STDCALL
WritePrivateProfileSectionA (
	LPCSTR	lpAppName,
	LPCSTR	lpString,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
WritePrivateProfileSectionW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpString,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
WritePrivateProfileStringA(LPCSTR lpAppName,
			   LPCSTR lpKeyName,
			   LPCSTR lpString,
			   LPCSTR lpFileName)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
WritePrivateProfileStringW(LPCWSTR lpAppName,
			   LPCWSTR lpKeyName,
			   LPCWSTR lpString,
			   LPCWSTR lpFileName)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
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


/*
 * @unimplemented
 */
WINBOOL STDCALL
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


/*
 * @unimplemented
 */
WINBOOL STDCALL
WriteProfileSectionA(LPCSTR lpAppName,
		     LPCSTR lpString)
{
   return WritePrivateProfileSectionA(lpAppName,
				      lpString,
				      NULL);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
WriteProfileSectionW(LPCWSTR lpAppName,
		     LPCWSTR lpString)
{
   return WritePrivateProfileSectionW(lpAppName,
				      lpString,
				      NULL);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
WriteProfileStringA(LPCSTR lpAppName,
		    LPCSTR lpKeyName,
		    LPCSTR lpString)
{
   return WritePrivateProfileStringA(lpAppName,
				     lpKeyName,
				     lpString,
				     NULL);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
WriteProfileStringW(LPCWSTR lpAppName,
		    LPCWSTR lpKeyName,
		    LPCWSTR lpString)
{
   return WritePrivateProfileStringW(lpAppName,
				     lpKeyName,
				     lpString,
				     NULL);
}

/* EOF */
