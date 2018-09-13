/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    crtsubst.h

Abstract:

    Maps some CRT functions to Win32 calls

Author:

    Rajeev Dujari (rajeevd) 04-Apr-1996

Revision History:

    04-Apr-1996 rajeevd
        Created
--*/
#include "iert.h"
#ifndef unix
/*
   On NT, kernel32 forwards RtlMoveMemory to ntdll.
   On 95, kernel32 has RtlMoveMemory but ntdll doesn't.
   Override the NT headers forwarding at compile time.
*/
#ifdef RtlMoveMemory
#undef RtlMoveMemory
extern "C" void RtlMoveMemory (void *, const void *, unsigned long);
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
#undef _strstr
#undef strstr
#undef _strchr
#undef strchr
#undef strrchr
#undef __atoi
#undef _atoi
#undef atoi
#undef _strncat
#undef strncat
#undef _strncpy
#undef strncpy
#undef _strnicmp
#undef strnicmp
#undef _strncmp
#undef strncmp
#undef StrChr


#define free(ptr)         FREE_MEMORY((HLOCAL) ptr)
#define malloc(size)      ((PVOID)ALLOCATE_MEMORY(LMEM_FIXED, size))
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
#define _strstr           StrStr
#define strstr            StrStr
#define StrChr            PrivateStrChr
#define _strchr           StrChr
#define strchr            StrChr
#define strrchr(s, c)     StrRChr(s, NULL, c)
#define __atoi            StrToInt
#define _atoi             StrToInt
#define atoi              StrToInt
#define strncat           StrNCat
#define _strncat          StrNCat
#define strncpy           StrNCpy
#define _strncpy          StrNCpy
#define strnicmp          StrCmpNIC
#define _strnicmp         StrCmpNIC
#define strncmp           StrCmpNC
#define _strncmp          StrCmpNC

#undef itoa
#undef ultoa

//#define itoa(val,s,n)     _itoa(val,s,n)
//#define ultoa(val,s,n)    _ultoa(val,s,n)

 
#endif /* unix */

