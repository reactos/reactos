/*
 * Adapted from linux for the reactos kernel, march 1998 -- David Welch
 * Added wide character string functions, june 1998 -- Boudewijn Dekker
 * Removed extern specifier from ___wcstok, june 1998 -- Boudewijn Dekker
 * Added wcsicmp and wcsnicmp -- Boudewijn Dekker
 */

#ifndef _LINUX_WSTRING_H_
#define _LINUX_WSTRING_H_

#include <types.h>        /* for size_t */

typedef unsigned short wchar_t;

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern wchar_t * ___wcstok;
extern wchar_t * wcscpy(wchar_t *,const wchar_t *);
extern wchar_t * wcsncpy(wchar_t *,const wchar_t *, __kernel_size_t);
extern wchar_t * wcscat(wchar_t *, const wchar_t *);
extern wchar_t * wcsncat(wchar_t *, const wchar_t *, __kernel_size_t);
extern int wcscmp(const wchar_t *,const wchar_t *);
extern int wcsncmp(const wchar_t *,const wchar_t *,__kernel_size_t);
wchar_t* wcschr(const wchar_t* str, wchar_t ch);
extern wchar_t * wcsrchr(const wchar_t *,wchar_t);
extern wchar_t * wcspbrk(const wchar_t *,const wchar_t *);
extern wchar_t * wcstok(wchar_t *,const wchar_t *);
extern wchar_t * wcsstr(const wchar_t *,const wchar_t *);
extern size_t wcsnlen(const wchar_t * s, size_t count);
extern int wcsicmp(const wchar_t* cs,const wchar_t * ct);
extern int wcsnicmp(const wchar_t* cs,const wchar_t * ct, size_t count);
extern size_t wcscspn(const wchar_t *, const wchar_t *);
extern size_t wcslen(const wchar_t *);
extern size_t wcsspn(const wchar_t *, const wchar_t *);

extern unsigned long wstrlen(PWSTR);
WCHAR wtoupper(WCHAR c);
WCHAR wtolower(WCHAR c);
   
#ifdef __cplusplus
}
#endif

#endif 

