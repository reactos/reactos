/* $Id: wchar.h,v 1.4 2002/10/29 04:45:26 rex Exp $
 */
/*
 * wchar.h
 *
 * wide-character types. Conforming to the Single UNIX(r) Specification
 * Version 2, System Interface & Headers Issue 5
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __WCHAR_H_INCLUDED__
#define __WCHAR_H_INCLUDED__

/* INCLUDES */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>

/* OBJECTS */

/* TYPES */
typedef wchar_t wint_t; /* An integral type capable of storing any valid
                           value of wchar_t, or WEOF */
typedef long int wctype_t; /* A scalar type of a data object that can hold
                              values which represent locale-specific
                              character classification. */
typedef void * mbstate_t; /* An object type other than an array type that
                             can hold the conversion state information
                             necessary to convert between sequences of
                             (possibly multibyte) characters and
                             wide-characters */

/* CONSTANTS */

/* PROTOTYPES */
wint_t            btowc(int);
int               fwprintf(FILE *, const wchar_t *, ...);
int               fwscanf(FILE *, const wchar_t *, ...);
int               iswalnum(wint_t);
int               iswalpha(wint_t);
int               iswcntrl(wint_t);
int               iswdigit(wint_t);
int               iswgraph(wint_t);
int               iswlower(wint_t);
int               iswprint(wint_t);
int               iswpunct(wint_t);
int               iswspace(wint_t);
int               iswupper(wint_t);
int               iswxdigit(wint_t);
int               iswctype(wint_t, wctype_t);
wint_t            fgetwc(FILE *);
wchar_t          *fgetws(wchar_t *, int, FILE *);
wint_t            fputwc(wchar_t, FILE *);
int               fputws(const wchar_t *, FILE *);
int               fwide(FILE *, int);
wint_t            getwc(FILE *);
wint_t            getwchar(void);
int               mbsinit(const mbstate_t *);
size_t            mbrlen(const char *, size_t, mbstate_t *);
size_t            mbrtowc(wchar_t *, const char *, size_t,
                      mbstate_t *);
size_t            mbsrtowcs(wchar_t *, const char **, size_t,
                      mbstate_t *);
wint_t            putwc(wchar_t, FILE *);
wint_t            putwchar(wchar_t);
int               swprintf(wchar_t *, size_t, const wchar_t *, ...);
int               swscanf(const wchar_t *, const wchar_t *, ...);
wint_t            towlower(wint_t);
wint_t            towupper(wint_t);
wint_t            ungetwc(wint_t, FILE *);
int               vfwprintf(FILE *, const wchar_t *, va_list);
int               vwprintf(const wchar_t *, va_list);
int               vswprintf(wchar_t *, size_t, const wchar_t *,
                      va_list);
size_t            wcrtomb(char *, wchar_t, mbstate_t *);
wchar_t          *wcscat(wchar_t *, const wchar_t *);
wchar_t          *wcschr(const wchar_t *, wchar_t);
int               wcscmp(const wchar_t *, const wchar_t *);
int               wcscoll(const wchar_t *, const wchar_t *);
wchar_t          *wcscpy(wchar_t *, const wchar_t *);
size_t            wcscspn(const wchar_t *, const wchar_t *);
size_t            wcsftime(wchar_t *, size_t, const wchar_t *,
                      const struct tm *);
size_t            wcslen(const wchar_t *);
wchar_t          *wcsncat(wchar_t *, const wchar_t *, size_t);
int               wcsncmp(const wchar_t *, const wchar_t *, size_t);
wchar_t          *wcsncpy(wchar_t *, const wchar_t *, size_t);
wchar_t          *wcspbrk(const wchar_t *, const wchar_t *);
wchar_t          *wcsrchr(const wchar_t *, wchar_t);
size_t            wcsrtombs(char *, const wchar_t **, size_t,
                      mbstate_t *);
size_t            wcsspn(const wchar_t *, const wchar_t *);
wchar_t          *wcsstr(const wchar_t *, const wchar_t *);
double            wcstod(const wchar_t *, wchar_t **);
wchar_t          *wcstok(wchar_t *, const wchar_t *, wchar_t **);
long int          wcstol(const wchar_t *, wchar_t **, int);
unsigned long int wcstoul(const wchar_t *, wchar_t **, int);
wchar_t          *wcswcs(const wchar_t *, const wchar_t *);
int               wcswidth(const wchar_t *, size_t);
size_t            wcsxfrm(wchar_t *, const wchar_t *, size_t);
int               wctob(wint_t);
wctype_t          wctype(const char *);
int               wcwidth(wchar_t);
wchar_t          *wmemchr(const wchar_t *, wchar_t, size_t);
int               wmemcmp(const wchar_t *, const wchar_t *, size_t);
wchar_t          *wmemcpy(wchar_t *, const wchar_t *, size_t);
wchar_t          *wmemmove(wchar_t *, const wchar_t *, size_t);
wchar_t          *wmemset(wchar_t *, wchar_t, size_t);
int               wprintf(const wchar_t *, ...);
int               wscanf(const wchar_t *, ...);

/* MACROS */
#define WCHAR_MAX (0xFFFF)
#define WCHAR_MIN (0x0000)

/* FIXME? */
#define WEOF      (0xFFFF)

#endif /* __WCHAR_H_INCLUDED__ */

/* EOF */

