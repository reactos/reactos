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
  /* Can't use strncpy, because strncpy will fill unused bytes in
     lpString1 with NUL bytes while lstrcpynA doesn't. Also lstrcpynA
     guarantees NUL termination while strncpy doesn't */

  if (1 < iMaxLength)
    {
      char *d = lpString1;
      const char *s = lpString2;

      do
        {
          if ('\0' == (*d++ = *s++))
            {
              break;
            }
        }
      while(1 != --iMaxLength);
      *d = '\0';
    }
  else if (1 == iMaxLength)
    {
      /* Only space for the terminator */
      *lpString1 = '\0';
    }

  return lpString1;
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
  /* Can't use wcsncpy, because wcsncpy will fill unused bytes in
     lpString1 with NUL bytes while lstrcpynW doesn't Also lstrcpynW
     guarantees NUL termination while wcsncpy doesn't */

  if (1 < iMaxLength)
    {
      WCHAR *d = lpString1;
      const WCHAR *s = lpString2;

      do
        {
          if (L'\0' == (*d++ = *s++))
            {
              break;
            }
        }
      while(1 != --iMaxLength);
      *d = L'\0';
    }
  else if (1 == iMaxLength)
    {
      /* Only space for the terminator */
      *lpString1 = L'\0';
    }

  return lpString1;
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
