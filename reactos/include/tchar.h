/* 
 * tchar.h
 *
 * Unicode mapping layer for the standard C library. By including this
 * file and using the 't' names for string functions
 * (eg. _tprintf) you can make code which can be easily adapted to both
 * Unicode and non-unicode environments. In a unicode enabled compile define
 * _UNICODE before including tchar.h, otherwise the standard non-unicode
 * library functions will be used.
 *
 * Note that you still need to include string.h or stdlib.h etc. to define
 * the appropriate functions. Also note that there are several defines
 * included for non-ANSI functions which are commonly available (but using
 * the convention of prepending an underscore to non-ANSI library function
 * names).
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.7 $
 * $Author: hyperion $
 * $Date: 2003/02/09 21:15:29 $
 *
 */

#ifndef	_TCHAR_H_
#define _TCHAR_H_

/*
 * NOTE: This tests _UNICODE, which is different from the UNICODE define
 *       used to differentiate Win32 API calls.
 */
#ifdef	_UNICODE


/*
 * Use TCHAR instead of char or wchar_t. It will be appropriately translated
 * if _UNICODE is correctly defined (or not).
 */
#ifndef _TCHAR_DEFINED
#ifndef RC_INVOKED
typedef wchar_t _TCHAR;
typedef signed wchar_t _TSCHAR;
typedef unsigned wchar_t _TUCHAR;
typedef wchar_t _TXCHAR;
/* #if !__STDC__ */
typedef wchar_t TCHAR;
/* #endif */
#endif	/* Not RC_INVOKED */
#define _TCHAR_DEFINED
#endif


/*
 * Enclose constant strings and literal characters in the _TEXT and _T macro to make
 * them unicode constant strings when _UNICODE is defined.
 */
#ifndef _TEXT
#define	_TEXT(x)	L ## x
#endif
#ifndef _T
#define	_T(x)		L ## x
#endif

/*
 * Unicode functions
 */

#define _tmain _wmain

#define	_tprintf		wprintf
#define	_ftprintf		fwprintf
#define	_stprintf		swprintf
#define	_sntprintf	_snwprintf
#define	_vtprintf		vwprintf
#define	_vftprintf	vfwprintf
#define	_vstprintf	vswprintf
#define	_vsntprintf	_vsnwprintf
#define	_tscanf		wscanf
#define	_ftscanf		fwscanf
#define	_stscanf		swscanf
#define	_fgettc		fgetwc
#define	_fgettchar	_fgetwchar
#define	_fgetts		fgetws
#define	_fputtc		fputwc
#define	_fputtchar	_fputwchar
#define	_fputts		fputws
#define	_gettc		getwc
#define	_getts		getws
#define	_puttc		putwc
#define	_putts		putws
#define	_ungettc	ungetwc
#define	_tcstod		wcstod
#define	_tcstol		wcstol
#define	_tcstoul		wcstoul
#define	_tcscat		wcscat
#define	_tcschr		wcschr
#define	_tcscmp		wcscmp
#define	_tcscpy		wcscpy
#define	_tcscspn	wcscspn
#define	_tcslen		wcslen
#define	_tcsncat		wcsncat
#define	_tcsncmp	wcsncmp
#define	_tcsncpy	wcsncpy
#define	_tcspbrk		wcspbrk
#define	_tcsrchr		wcsrchr
#define	_tcsspn		wcsspn
#define	_tcsstr		wcsstr
#define	_tcstok		wcstok
#define	_tcsdup		_wcsdup
#define	_tcsicmp	_wcsicmp
#define	_tcsnicmp	_wcsnicmp
#define	_tcsnset		_wcsnset
#define	_tcsrev		_wcsrev
#define	_tcsset		_wcsset
#define	_tcslwr		_wcslwr
#define	_tcsupr		_wcsupr
#define	_tcsxfrm		wcsxfrm
#define	_tcscoll		wcscoll
#define	_tcsicoll		_wcsicoll
#define	_istalpha	iswalpha
#define	_istupper	iswupper
#define	_istlower	iswlower
#define	_istdigit		iswdigit
#define	_istxdigit	iswxdigit
#define	_istspace	iswspace
#define	_istpunct	iswpunct
#define	_istalnum	iswalnum
#define	_istprint		iswprint
#define	_istgraph	iswgraph
#define	_istcntrl		iswcntrl
#define	_istascii		iswascii
#define	_totupper	towupper
#define	_totlower	towlower
#define	_ttoi		_wtoi
#define	_tcsftime	wcsftime

#else	/* Not _UNICODE */

/*
 * TCHAR, the type you should use instead of char.
 */
#ifndef _TCHAR_DEFINED
#ifndef RC_INVOKED
typedef char _TCHAR;
typedef signed char _TSCHAR;
typedef unsigned char _TUCHAR;
typedef char _TXCHAR;

/*#if !__STDC__*/
typedef char TCHAR;
/*#endif*/

#endif
#define _TCHAR_DEFINED
#endif

/*
 * Enclose constant strings and characters in the _TEXT and _T macro.
 */
#ifndef _TEXT
#define	_TEXT(x)	x
#endif
#ifndef _T
#define	_T(x)		x
#endif


/*
 * Non-unicode (standard) functions
 */

#define _tmain main

#define	_tprintf	printf
#define _ftprintf	fprintf
#define	_stprintf	sprintf
#define	_sntprintf	_snprintf
#define	_vtprintf	vprintf
#define	_vftprintf	vfprintf
#define _vstprintf	vsprintf
#define	_vsntprintf	_vsnprintf
#define	_tscanf		scanf
#define	_ftscanf	fscanf
#define	_stscanf	sscanf
#define	_fgettc		fgetc
#define	_fgettchar	_fgetchar
#define	_fgetts		fgets
#define	_fputtc		fputc
#define	_fputtchar	_fputchar
#define	_fputts		fputs
#define	_gettc		getc
#define	_getts		gets
#define	_puttc		putc
#define	_putts		puts
#define	_ungettc	ungetc
#define	_tcstod		strtod
#define	_tcstol		strtol
#define _tcstoul	strtoul
#define	_tcscat		strcat
#define _tcschr		strchr
#define _tcscmp		strcmp
#define _tcscpy		strcpy
#define _tcscspn	strcspn
#define	_tcslen		strlen
#define	_tcsncat	strncat
#define	_tcsncmp	strncmp
#define	_tcsncpy	strncpy
#define	_tcspbrk	strpbrk
#define	_tcsrchr	strrchr
#define _tcsspn		strspn
#define	_tcsstr		strstr
#define _tcstok		strtok
#define	_tcsdup		_strdup
#define	_tcsicmp	_stricmp
#define	_tcsnicmp	_strnicmp
#define	_tcsnset	_strnset
#define	_tcsrev		_strrev
#define _tcsset		_strset
#define	_tcslwr		_strlwr
#define	_tcsupr		_strupr
#define	_tcsxfrm	strxfrm
#define	_tcscoll	strcoll
#define	_tcsicoll	_stricoll
#define	_istalpha	isalpha
#define	_istupper	isupper
#define	_istlower	islower
#define	_istdigit	isdigit
#define	_istxdigit	isxdigit
#define	_istspace	isspace
#define	_istpunct	ispunct
#define	_istalnum	isalnum
#define	_istprint	isprint
#define	_istgraph	isgraph
#define	_istcntrl	iscntrl
#define	_istascii	isascii
#define _totupper	toupper
#define	_totlower	tolower
#define	_ttoi		atoi
#define _tcsftime	strftime

#endif	/* Not _UNICODE */

#endif	/* Not _TCHAR_H_ */

