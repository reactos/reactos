/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/lstring.c
 * PURPOSE:         Local string functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>


/*
 * @implemented
 */
int
STDCALL
lstrcmpA(
	 LPCSTR lpString1,
	 LPCSTR lpString2
	 )
{
	return strcmp(lpString1,lpString2);
}


/*
 * @implemented
 */
int
STDCALL
lstrcmpiA(
	  LPCSTR lpString1,
	  LPCSTR lpString2
	  )
{
        return _stricmp(lpString1,lpString2); 
}


/*
 * @implemented
 */
LPSTR
STDCALL
lstrcpynA(
	  LPSTR lpString1,
	  LPCSTR lpString2,
	  int iMaxLength
	  )
{
	return strncpy(lpString1,lpString2,iMaxLength);
}


/*
 * @implemented
 */
LPSTR
STDCALL
lstrcpyA(
	 LPSTR lpString1,
	 LPCSTR lpString2
	 )
{
	return strcpy(lpString1,lpString2);
}


/*
 * @implemented
 */
LPSTR
STDCALL
lstrcatA(
	 LPSTR lpString1,
	 LPCSTR lpString2
	 )
{
	return strcat(lpString1,lpString2);
}


/*
 * @implemented
 */
int
STDCALL
lstrlenA(
	 LPCSTR lpString
	 )
{
	return strlen(lpString);
}


/*
 * @implemented
 */
int
STDCALL
lstrcmpW(
	 LPCWSTR lpString1,
	 LPCWSTR lpString2
	 )
{
	return wcscmp(lpString1,lpString2);
}


/*
 * @implemented
 */
int
STDCALL
lstrcmpiW(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    )
{
        return _wcsicmp(lpString1,lpString2);
}


/*
 * @implemented
 */
LPWSTR
STDCALL
lstrcpynW(
    LPWSTR lpString1,
    LPCWSTR lpString2,
    int iMaxLength
    )
{
	return wcsncpy(lpString1,lpString2,iMaxLength);
}


/*
 * @implemented
 */
LPWSTR
STDCALL
lstrcpyW(
    LPWSTR lpString1,
    LPCWSTR lpString2
    )
{
	return wcscpy(lpString1,lpString2);	
}


/*
 * @implemented
 */
LPWSTR
STDCALL
lstrcatW(
    LPWSTR lpString1,
    LPCWSTR lpString2
    )
{
	return wcscat(lpString1,lpString2);
}


/*
 * @implemented
 */
int
STDCALL
lstrlenW(
    LPCWSTR lpString
    )
{
	return wcslen(lpString);
}
