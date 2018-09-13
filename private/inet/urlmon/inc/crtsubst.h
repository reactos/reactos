/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    crtsubst.h

Abstract:

    Maps some CRT functions to Win32 calls

Author:

    Rajeev Dujari (rajeevd) 04-Apr-1996

Revision History:

    04-Apr-1997 vincentr
        Copied from wininet
    04-Apr-1996 rajeevd
        Created
--*/

#ifndef _CRTSUBSTR_H
#define _CRTSUBSTR_H

#include <shlwapi.h>

#ifndef unix 
/*
   On NT, kernel32 forwards RtlMoveMemory to ntdll.
   On 95, kernel32 has RtlMoveMemory but ntdll doesn't.
   Override the NT headers forwarding at compile time.
*/
#ifdef RtlMoveMemory
#undef RtlMoveMemory
#ifdef __cplusplus
extern "C" {
#endif
void RtlMoveMemory (void *, const void *, unsigned long);
#ifdef __cplusplus
}
#endif
#endif

/* WARNING: Be careful mapping CRT strncpy to Win32 lstrcpyn.

   strncpy  (dst, "bar", 2);  // dst will get 'b', 'a'
   lstrcpyn (dst, "bar" 2);   // dst will get 'b',  0

   strncpy  (dst, "bar", 6);  // dst will get 'b', 'a', 'r', 0, 0, 0
   lstrcpyn (dst, "bar", 6);  // dst will get 'b', 'a', 'r', 0
*/

#undef free
#undef malloc
#undef memmove
#undef strdup
#undef stricmp
#undef _stricmp
#undef strlwr
#undef _strlwr
#undef strupr
#undef tolower
#undef toupper
#undef wcslen
#undef wcscmp
#undef wcscpy
#undef wcsncpy
#undef wcscat
#undef wcschr
#undef wcsrchr
#undef wcsstr
#undef _wcsicmp
#undef _wcsnicmp
#undef _strstr
#undef strstr
#undef _strchr
#undef strchr
#undef _strrchr
#undef strrchr
#undef __atoi
#undef _atoi
#undef atoi
#undef atol
#undef _strncat
#undef strncat
#undef _strncpy
#undef strncpy
#undef _strnicmp
#undef strnicmp
#undef _strncmp
#undef strncmp
#undef sprintf
#undef vsprintf
#undef wvsprintf

#define free(ptr)         LocalFree((HLOCAL) ptr)
#define malloc(size)      ((PVOID)LocalAlloc(LMEM_FIXED, size))
#define memmove(m1,m2,n)  RtlMoveMemory (m1,m2,n)
#define strdup(s)         NewString(s)
#define stricmp(s1,s2)    lstrcmpi(s1,s2)
#define _stricmp(s1,s2)   lstrcmpi(s1,s2)
#define strlwr(s)         CharLower(s)
#define _strlwr(s)        CharLower(s)
#define strupr(s)         CharUpper(s)
#define tolower(c)        ((BYTE) CharLower((LPSTR) ((DWORD)((BYTE)(c) & 0xff))))
#define toupper(c)        ((BYTE) CharUpper((LPSTR) ((DWORD)((BYTE)(c) & 0xff))))
#define wcslen(s)         lstrlenW(s)
#define wcscmp            StrCmpW
#define wcscpy            StrCpyW
#define wcsncpy(s1, s2, n) StrCpyNW(s1, s2, n)
#define wcscat            StrCatW
#define wcschr            StrChrW
#define wcsrchr(s, c)     StrRChrW(s, NULL, c)
#define wcsstr            StrStrW
#define _wcsicmp          StrCmpIW
#define _wcsnicmp         StrCmpNIW
#define _strstr           StrStr
#define strstr            StrStr
#define _strchr           StrChr
#define strchr            StrChr
#define _strrchr(s, c)    StrRChr(s, NULL, c)
#define strrchr(s, c)     StrRChr(s, NULL, c)
#define __atoi            StrToInt
#define _atoi             StrToInt
#define atoi              StrToInt
#define atol              StrToInt
#define strncat           StrNCat
#define _strncat          StrNCat
#define strncpy           StrNCpy
#define _strncpy          StrNCpy
#define strnicmp          StrCmpNI
#define _strnicmp         StrCmpNI
#define strncmp           StrCmpN
#define _strncmp          StrCmpN
#define sprintf           w4sprintf
#define vsprintf          w4vsprintf
#define wvsprintf         w4vsprintf

#undef itoa
#undef ultoa

#define itoa(val,s,n)     _itoa(val,s,n)
#define ultoa(val,s,n)    _ultoa(val,s,n)

#endif /* unix */

#endif // _CRTSUBSTR_H


