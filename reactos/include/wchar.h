/*
 * Adapted from linux for the reactos kernel, march 1998 -- David Welch
 * Added wide character string functions, june 1998 -- Boudewijn Dekker
 * Removed extern specifier from ___wcstok, june 1998 -- Boudewijn Dekker
 * Added wcsicmp and wcsnicmp -- Boudewijn Dekker
 */

#include <internal/types.h>

#ifndef _LINUX_WSTRING_H_
#define _LINUX_WSTRING_H_



#ifndef _WCHAR_T_
#define _WCHAR_T_
#define _WCHAR_T
	typedef unsigned short wchar_t;
#endif
#define Aa_Difference (L'A'-L'a')

#define towupper(c) (((c>=L'a') && (c<=L'z')) ? c+Aa_Difference : c)
#define towlower(c) (((c>=L'A') && (c<=L'Z')) ? c-Aa_Difference : c)

//obsolete
wchar_t wtolower(wchar_t c );
wchar_t wtoupper(wchar_t c );


#define iswlower(c)  ((c) >= L'a' && (c) <= L'z')
#define iswupper(c)  ((c) >= L'A' && (c) <= L'Z')

#define iswdigit(c)	((c) >= L'0' && (c) <= L'9')
#define iswxdigit(c)	(((c) >= L'0' && (c) <= L'9') || ((c) >= L'A' && (c) <= L'F') || ((c) >= L'a' && (c) <= L'f') )

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

//wchar_t * ___wcstok = NULL;
wchar_t * wcscpy(wchar_t *,const wchar_t *);
wchar_t * wcsncpy(wchar_t *,const wchar_t *, size_t);
wchar_t * wcscat(wchar_t *, const wchar_t *);
wchar_t * wcsncat(wchar_t *, const wchar_t *, size_t);
int wcscmp(const wchar_t *,const wchar_t *);
int wcsncmp(const wchar_t *,const wchar_t *,size_t);
wchar_t* wcschr(const wchar_t* str, wchar_t ch);
wchar_t * wcsrchr(const wchar_t *,wchar_t);
wchar_t * wcspbrk(const wchar_t *,const wchar_t *);
wchar_t * wcstok(wchar_t *,const wchar_t *);
wchar_t * wcsstr(const wchar_t *,const wchar_t *);
size_t wcslen(const wchar_t * s);
size_t wcsnlen(const wchar_t * s, size_t count);
int wcsicmp(const wchar_t* cs,const wchar_t * ct);
int wcsnicmp(const wchar_t* cs,const wchar_t * ct, size_t count);
size_t wcsspn(const wchar_t *str,const wchar_t *accept);
size_t wcscspn(const wchar_t *str,const wchar_t *reject);
wchar_t *wcsrev(wchar_t *s);
wchar_t *wcsstr(const wchar_t *s,const wchar_t *b);
wchar_t *wcsdup(const wchar_t *ptr);
wchar_t *wcsupr(wchar_t *x);
wchar_t * wcslwr(wchar_t *x);

//obsolete
size_t wstrlen(const wchar_t * s);
int wcscmpi (const wchar_t* ws1, const wchar_t* ws2);



   
   
#ifdef __cplusplus
}
#endif

#endif 

