/* $Id: stubs.c,v 1.4 2003/07/10 21:07:14 chorns Exp $
 *
 * version.dll stubs: remove from this file if
 * you implement one of these functions.
 */
#include <windows.h>

#ifndef HAVE_DLL_FORWARD

/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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



/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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
