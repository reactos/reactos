/*******************************************************************************
        nocrt.h - C Runtime headers for those who are lazy...

        Owner:          Mikel
        Created:        5 Dec 94
 *******************************************************************************/

#ifndef _NOCRT_H_
#define _NOCRT_H_

#define _INC_STDLIB                                     // force stdlib.h not to be included
#define _INC_STRING                                     // same with string.h
#define _CTYPE_DISABLE_MACROS           // same with ctype macros
#define _CTYPE_DEFINED
#define _INC_ERRNO
#define _INC_STDDEF

#define ERANGE                  34                      // used in errno for overflow

/* Redefined C runtime calls.  Couldn't do it for FillBuf though
 */
#define isalpha(c)              IsCharAlpha(c)
#define isalnum(c)              IsCharAlphaNumeric(c)
#define isdigit(c)              (IsCharAlphaNumeric(c) && !IsCharAlpha(c))
#define isupper(c)              IsCharUpper(c)
#define memmove(m1, m2, n)      MoveMemory(m1, m2, n)
#define strcat(s1, s2)          lstrcat(s1, s2)
#define strcpy(d, s)            lstrcpy(d, s)
#define strcmp(s1, s2)          lstrcmp(s1, s2)
#define stricmp(s1, s2)         lstrcmpi(s1, s2)
#define strlen(s)               lstrlen(s)
#define strncpy(s1, s2, n)      StrCpyN(s1, s2, n)
#define tolower(c)              ((TCHAR) CharLower((LPTSTR)MAKELONG(c, 0)))
#define toupper(c)              ((TCHAR) CharUpper((LPTSTR)MAKELONG(c, 0)))
#define strncmp(s1, s2, n)      StrCmpN(s1, s2, n)
#define atoi(s1)                StrToInt(s1)


#ifndef __cplusplus
/* These are defined in nocrt2.h for C++.  Weird.
 */
#define MsoIsEqualGuid(g1, g2) \
        (!StrCmpNA((const CHAR *)g1, (const CHAR *)g2, sizeof(GUID)))
#define MsoIsEqualIid(i1, i2)   \
        MsoIsEqualGuid(i1, i2)
#define MsoIsEqualClsid(c1, c2) \
        MsoIsEqualGuid(c1, c2)
#endif

/* Runtimes we have to write ourselves, can't use Windows */
#include <ctype.h>                  // get wchar_t defined
int  isspace(int);
#ifdef UNICODE
long strtol(const wchar_t *, wchar_t **, int);
#else
long strtol(const char *, char **, int);
#endif


/* Use this function instead of a bunch of strtok()s */
#ifdef UNICODE
int  ScanDateNums(wchar_t *, wchar_t *, unsigned int [], int, int);
#else
int  ScanDateNums(char *, char *, unsigned int [], int, int);
#endif

/* Needed to fake out IsEqualGUID() macro */
#include <memory.h>
#ifndef WINNT
#pragma intrinsic(memcmp)
#endif

extern int errno;
#endif // _NOCRT_H_
