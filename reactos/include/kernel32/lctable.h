/*
 * nls/lctable.h
 */



#ifndef __NLS_LCTABLE_H
#define __NLS_LCTABLE_H


struct __CODEPAGE;

#ifdef WIN32_LEAN_AND_MEAN
	typedef DWORD LCID;
#endif

#define LOCALE_ARRAY 200
typedef struct __LOCALE
{
   LCID			Id;
   LPSTR		AbbrName;
   LPWSTR		*Info0;
   LPWSTR		*Info1;
   LPWSTR		*ShortDateFormat;
   LPWSTR		*LongDateFormat;
   LPWSTR		*TimeFormat;
   struct __CODEPAGE 	*AnsiCodePage;
   struct __CODEPAGE 	*OemCodePage;
} LOCALE, *PLOCALE, *LPLOCALE;

extern LOCALE  __Locale[LOCALE_ARRAY];
extern PLOCALE __UserLocale;

extern BOOL __LocaleInit(VOID);

#endif