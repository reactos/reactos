/* $Id: stubs.c,v 1.1 1999/08/11 19:56:48 ea Exp $
 *
 * version.dll stubs: remove from this file if
 * you implement one of these functions.
 */
#include <windows.h>

#ifndef HAVE_DLL_FORWARD
DWORD
STDCALL
GetFileVersionInfoSizeA (
	LPSTR	lptstrFilename,
	LPDWORD	lpdwHandle
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
GetFileVersionInfoA (
        LPSTR lptstrFilename,
        DWORD dwHandle,
        DWORD dwLen,
        LPVOID lpData
        )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
GetFileVersionInfoSizeW (
	LPWSTR	lptstrFilename,
	LPDWORD	lpdwHandle
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetFileVersionInfoW (
	LPWSTR	lptstrFilename,
	DWORD	dwHandle,
	DWORD	dwLen,
	LPVOID	lpData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

#endif /* ndef HAVE_DLL_FORWARD */


DWORD
STDCALL
VerFindFileA (
        DWORD uFlags,
        LPSTR szFileName,
        LPSTR szWinDir,
        LPSTR szAppDir,
        LPSTR szCurDir,
        PUINT lpuCurDirLen,
        LPSTR szDestDir,
        PUINT lpuDestDirLen
        )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VerFindFileW (
	DWORD	uFlags,
	LPWSTR	szFileName,
	LPWSTR	szWinDir,
	LPWSTR	szAppDir,
	LPWSTR	szCurDir,
	PUINT	lpuCurDirLen,
	LPWSTR	szDestDir,
	PUINT	lpuDestDirLen
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VerInstallFileA (
        DWORD uFlags,
        LPSTR szSrcFileName,
        LPSTR szDestFileName,
        LPSTR szSrcDir,
        LPSTR szDestDir,
        LPSTR szCurDir,
        LPSTR szTmpFile,
        PUINT lpuTmpFileLen
        )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VerInstallFileW (
	DWORD	uFlags,
	LPWSTR	szSrcFileName,
	LPWSTR	szDestFileName,
	LPWSTR	szSrcDir,
	LPWSTR	szDestDir,
	LPWSTR	szCurDir,
	LPWSTR	szTmpFile,
	PUINT	lpuTmpFileLen
        )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD
STDCALL
VerLanguageNameA (
	DWORD	wLang,
	LPSTR	szLang,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VerLanguageNameW (
	DWORD	wLang,
	LPWSTR	szLang,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

WINBOOL
STDCALL
VerQueryValueA (
	const LPVOID	pBlock,
	LPSTR		lpSubBlock,
	LPVOID		* lplpBuffer,
	PUINT		puLen
        )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
VerQueryValueW (
	const LPVOID	pBlock,
	LPWSTR		lpSubBlock,
	LPVOID		* lplpBuffer,
	PUINT		puLen
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* VerQueryValueIndex seems undocumented */

DWORD
STDCALL
VerQueryValueIndexA (
	DWORD Unknown0,
	DWORD Unknown1,
	DWORD Unknown2,
	DWORD Unknown3,
	DWORD Unknown4,
	DWORD Unknown5
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VerQueryValueIndexW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/* EOF */
