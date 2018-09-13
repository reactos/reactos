/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    tstr.h

Abstract:

    This include file contains manifests and macros to be used to integrate
    the TCHAR and LPTSTR definitions

    Note that our naming convention is that a "size" indicates a number of
    bytes whereas a "length" indicates a number of characters.

Author:

    Richard Firth (rfirth) 02-Apr-1991

Environment:

    Portable (Win/32).
    Requires ANSI C extensions: slash-slash comments, long external names,
    _ultoa() routine.

Revision History:

    22-May-1991 Danl
        Added STRSIZE macro
    19-May-1991 JohnRo
        Changed some parm names to make things easier to read.
    15-May-1991 rfirth
        Added TCHAR_SPACE and MAKE_TCHAR() macro
    15-Jul-1991 RFirth
        Added STRING_SPACE_REQD() and DOWN_LEVEL_STRSIZE
    05-Aug-1991 JohnRo
        Added MEMCPY macro.
    19-Aug-1991 JohnRo
        Added character type stuff: ISDIGIT(), TOUPPER(), etc.
    20-Aug-1991 JohnRo
        Changed strnicmp to _strnicmp to keep PC-LINT happy.  Ditto stricmp.
    13-Sep-1991 JohnRo
        Need UNICODE STRSIZE() too.
    13-Sep-1991 JohnRo
        Added UNICODE STRCMP() and various others.
    18-Oct-1991 JohnRo
        Added NetpCopy routines and WCSSIZE().
    26-Nov-1991 JohnRo
        Added NetpNCopy routines (like strncpy but do conversions as well).
    09-Dec-1991 rfirth
        Added STRREV
    03-Jan-1992 JohnRo
        Added NetpAlloc{type}From{type} routines and macros.
    09-Jan-1992 JohnRo
        Added ATOL() macro and wtol() routine.
        Ditto ULTOA() macro and ultow() routine.
    16-Jan-1992 Danl
        Cut this info from \net\inc\tstring.h
    30-Jan-1992 JohnRo
        Added STRSTR().
        Use _wcsupr() instead of wcsupr() to keep PC-LINT happy.
        Added STRCMPI() and STRNCMPI().
        Fixed a few definitions which were missing MAKE_STR_FUNCTION etc.
    14-Mar-1992 JohnRo
        Avoid compiler warnings using WCSSIZE(), MEMCPY(), etc.
        Added TCHAR_TAB.
    09-Apr-1992 JohnRo
        Prepare for WCHAR.H (_wcsicmp vs _wcscmpi, etc).

--*/

#ifndef _TSTR_H_INCLUDED
#define _TSTR_H_INCLUDED

#include <ctype.h>              // isdigit(), iswdigit() eventually, etc.
#include <stdlib.h>             // atol(), _ultoa().
#include <string.h>             // memcpy(), strlen(), etc.
#include <wchar.h>


LPWSTR
ultow (
    IN DWORD Value,
    OUT LPWSTR Area,
    IN DWORD Radix
    );

LONG
wtol (
    IN LPWSTR Src
    );


#ifdef LM20_COMPATIBLE
#define MAKE_STR_FUNCTION(s)    s##f
#else
#define MAKE_STR_FUNCTION(s)    s
#endif


#if defined(UNICODE)

//
// function macro prototypes
//

#define ATOL(Src)           (LONG)MAKE_STR_FUNCTION(wtol)(Src)

#define ISALNUM(tchar)      iswalnum(tchar)   // locale-dependent.
#define ISALPHA(tchar)      iswalpha(tchar)   // locale-dependent.
#define ISCNTRL(tchar)      iswcntrl(tchar)   // locale-dependent.
#define ISDIGIT(tchar)      iswdigit(tchar)
#define ISGRAPH(tchar)      iswgraph(tchar)   // locale-dependent.
#define ISLOWER(tchar)      iswlower(tchar)   // locale-dependent.
#define ISPRINT(tchar)      iswprint(tchar)   // locale-dependent.
#define ISPUNCT(tchar)      iswpunct(tchar)   // locale-dependent.
#define ISSPACE(tchar)      iswspace(tchar)   // locale-dependent.
#define ISUPPER(tchar)      iswupper(tchar)   // locale-dependent.
#define ISXDIGIT(tchar)     iswxdigit(tchar)

#define STRCAT(dest, src)   (LPTSTR)MAKE_STR_FUNCTION(wcscat)((dest), (src))
#define STRCHR(s1, c)       (LPTSTR)MAKE_STR_FUNCTION(wcschr)((s1), (c))
#define STRCPY(dest, src)   (LPTSTR)MAKE_STR_FUNCTION(wcscpy)((dest), (src))
#define STRCSPN(s, c)       (DWORD)MAKE_STR_FUNCTION(wcscspn)((s), (c))
// STRLEN: Get character count of s.
#define STRLEN(s)           (DWORD)MAKE_STR_FUNCTION(wcslen)(s)
#define STRNCAT(dest, src, n) \
            (LPTSTR)MAKE_STR_FUNCTION(wcsncat)((dest), (src), (n))
#define STRNCPY(dest, src, n) \
            (LPTSTR)MAKE_STR_FUNCTION(wcsncpy)((dest), (src), (n))
#define STRSPN(s1, s2)      (DWORD)MAKE_STR_FUNCTION(wcsspn)((s1), (s2))
#define STRRCHR             (LPTSTR)MAKE_STR_FUNCTION(wcsrchr)
#define STRSTR              (LPTSTR)MAKE_STR_FUNCTION(wcswcs)
#define STRUPR(s)           (LPTSTR)MAKE_STR_FUNCTION(_wcsupr)(s)

// these don't have formal parameters because we want to take the address of
// the mapped function in certain cases.  Modify as appropriate.
// Note that for these functions, lengths are in characters.

// compare functions: len is maximum number of characters being compared.
#define STRCMP              (LONG)MAKE_STR_FUNCTION(wcscmp)
#define STRCMPI             (LONG)MAKE_STR_FUNCTION(_wcsicmp)
#define STRICMP             (LONG)MAKE_STR_FUNCTION(_wcsicmp)
#define STRNCMP             (LONG)MAKE_STR_FUNCTION(wcsncmp)
#define STRNCMPI            (LONG)MAKE_STR_FUNCTION(_wcsnicmp)
#define STRNICMP            (LONG)MAKE_STR_FUNCTION(_wcsnicmp)

#define TOLOWER(tchar)      towlower(tchar)   // locale-dependent.
#define TOUPPER(tchar)      towupper(tchar)   // locale-dependent.

#define ULTOA(Value,Result,Radix) \
            (LPTSTR)MAKE_STR_FUNCTION(ultow)( (Value), (Result), (Radix) )

//
// manifests
//

#define _CHAR_TYPE  WCHAR

#else   // not UNICODE

//
// function macro prototypes
//

#define ATOL(Src)           (LONG)MAKE_STR_FUNCTION(atol)(Src)

#define ISALNUM(tchar)      isalnum(tchar)   // locale-dependent.
#define ISALPHA(tchar)      isalpha(tchar)   // locale-dependent.
#define ISCNTRL(tchar)      iscntrl(tchar)   // locale-dependent.
#define ISDIGIT(tchar)      isdigit(tchar)
#define ISGRAPH(tchar)      isgraph(tchar)   // locale-dependent.
#define ISLOWER(tchar)      islower(tchar)   // locale-dependent.
#define ISPRINT(tchar)      isprint(tchar)   // locale-dependent.
#define ISPUNCT(tchar)      ispunct(tchar)   // locale-dependent.
#define ISSPACE(tchar)      isspace(tchar)   // locale-dependent.
#define ISUPPER(tchar)      isupper(tchar)   // locale-dependent.
#define ISXDIGIT(tchar)     isxdigit(tchar)

#define STRCAT(dest, src)   (LPTSTR)MAKE_STR_FUNCTION(strcat)((dest), (src))
#define STRNCAT(dest, src, n) \
            (LPTSTR)MAKE_STR_FUNCTION(strncat)((dest), (src), (n))
// STRLEN: Get character count of s.
#define STRLEN(s)           (DWORD)MAKE_STR_FUNCTION(strlen)(s)
#define STRSPN(s1, s2)      (DWORD)MAKE_STR_FUNCTION(strspn)((s1), (s2))
#define STRCSPN(s, c)       (DWORD)MAKE_STR_FUNCTION(strcspn)((s), (c))
#define STRCPY(dest, src)   (LPTSTR)MAKE_STR_FUNCTION(strcpy)((dest), (src))
#define STRNCPY(dest, src, n) \
            (LPTSTR)MAKE_STR_FUNCTION(strncpy)((dest), (src), (n))
#define STRCHR(s1, c)       (LPTSTR)MAKE_STR_FUNCTION(strchr)((s1), (c))
#define STRRCHR             (LPTSTR)MAKE_STR_FUNCTION(strrchr)
#define STRSTR              (LPTSTR)MAKE_STR_FUNCTION(strstr)
#define STRUPR(s)           (LPTSTR)MAKE_STR_FUNCTION(strupr)(s)
#define STRREV(s)           (LPTSTR)MAKE_STR_FUNCTION(strrev)(s)

// these don't have formal parameters because we want to take the address of
// the mapped function in certain cases.  Modify as appropriate.
// Note that for these functions, lengths are in characters.

// compare functions: len is maximum number of characters being compared.
#define STRCMP              (LONG)MAKE_STR_FUNCTION(lstrcmp)
#define STRCMPI             (LONG)MAKE_STR_FUNCTION(lstrcmpi)
#define STRICMP             (LONG)MAKE_STR_FUNCTION(lstrcmpi)
#define STRNCMP             (LONG)MAKE_STR_FUNCTION(strncmp)
#define STRNCMPI            (LONG)MAKE_STR_FUNCTION(_strnicmp)
#define STRNICMP            (LONG)MAKE_STR_FUNCTION(_strnicmp)

#define TOLOWER(tchar)      tolower(tchar)   // locale-dependent.
#define TOUPPER(tchar)      toupper(tchar)   // locale-dependent.

#define ULTOA(Value,Result,Radix) \
            (LPTSTR)MAKE_STR_FUNCTION(_ultoa)( (Value), (Result), (Radix) )

//
// manifests
//

#define _CHAR_TYPE  TCHAR

#endif // not UNICODE


//
// For the memory routines, the counts are always BYTE counts.
//
#define MEMCPY                  MAKE_STR_FUNCTION(memcpy)
#define MEMMOVE                 MAKE_STR_FUNCTION(memmove)

//
// These are used to determine the number of bytes (including the NUL
// terminator) in a string.  This will generally be used when
// calculating the size of a string for allocation purposes.
//

#define STRSIZE(p)      ((STRLEN(p)+1) * sizeof(TCHAR))
#define WCSSIZE(s)      ((MAKE_STR_FUNCTION(wcslen)(s)+1) * sizeof(WCHAR))


//
// character literals (both types)
//

#define TCHAR_EOS       ((_CHAR_TYPE)'\0')
#define TCHAR_STAR      ((_CHAR_TYPE)'*')
#define TCHAR_BACKSLASH ((_CHAR_TYPE)'\\')
#define TCHAR_FWDSLASH  ((_CHAR_TYPE)'/')
#define TCHAR_COLON     ((_CHAR_TYPE)':')
#define TCHAR_DOT       ((_CHAR_TYPE)'.')
#define TCHAR_SPACE     ((_CHAR_TYPE)' ')
#define TCHAR_TAB       ((_CHAR_TYPE)'\t')


//
// General purpose macro for casting character types to whatever type in vogue
// (as defined in this file)
//

#define MAKE_TCHAR(c)   ((_CHAR_TYPE)(c))

//
// IS_PATH_SEPARATOR
//
// lifted from curdir.c and changed to use TCHAR_ character literals, checks
// if a character is a path separator i.e. is a member of the set [\/]
//

#define IS_PATH_SEPARATOR(ch) ((ch == TCHAR_BACKSLASH) || (ch == TCHAR_FWDSLASH))

//
// The following 2 macros lifted from I_Net canonicalization files
//

#define IS_DRIVE(c)             ISALPHA(c)
#define IS_NON_ZERO_DIGIT(c)    (((c) >= MAKE_TCHAR('1')) && ((c) <= MAKE_TCHAR('9')))

//
// STRING_SPACE_REQD returns a number (of bytes) corresponding to the space
// required in which (n) characters can be accomodated
//

#define STRING_SPACE_REQD(n)    ((n) * sizeof(_CHAR_TYPE))

//
// DOWN_LEVEL_STRLEN returns the number of single-byte characters necessary to
// store a converted _CHAR_TYPE string. This will be WCHAR (or wchar_t) if
// UNICODE is defined or CHAR (or char) otherwise
//

#define DOWN_LEVEL_STRSIZE(n)   ((n) / sizeof(_CHAR_TYPE))

#endif  // _TSTR_H_INCLUDED
