/***
*tchar.h - definitions for generic international text functions
*
*       Copyright (c) 1991-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Definitions for generic international functions, mostly defines
*       which map string/formatted-io/ctype functions to char, wchar_t, or
*       MBCS versions.  To be used for compatibility between single-byte,
*       multi-byte and Unicode text models.
*
*       [Public]
*
****/

#if _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_TCHAR
#define _INC_TCHAR

#ifdef  _MSC_VER
#pragma warning(disable:4514)       /* disable unwanted C++ /W4 warning */
/* #pragma warning(default:4514) */ /* use this to reenable, if necessary */
#endif  /* _MSC_VER */

#ifdef  __cplusplus
extern "C" {
#endif


/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  _NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else   /* ndef _NTSDK */
/* current definition */
#ifdef  CRTDLL
#define _CRTIMP __declspec(dllexport)
#else   /* ndef CRTDLL */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* CRTDLL */
#endif  /* _NTSDK */
#endif  /* _CRTIMP */


#define _ftcscat    _tcscat
#define _ftcschr    _tcschr
#define _ftcscpy    _tcscpy
#define _ftcscspn   _tcscspn
#define _ftcslen    _tcslen
#define _ftcsncat   _tcsncat
#define _ftcsncpy   _tcsncpy
#define _ftcspbrk   _tcspbrk
#define _ftcsrchr   _tcsrchr
#define _ftcsspn    _tcsspn
#define _ftcsstr    _tcsstr
#define _ftcstok    _tcstok

#define _ftcsdup    _tcsdup
#define _ftcsnset   _tcsnset
#define _ftcsrev    _tcsrev
#define _ftcsset    _tcsset

#define _ftcscmp      _tcscmp
#define _ftcsicmp     _tcsicmp
#define _ftcsnccmp    _tcsnccmp
#define _ftcsncmp     _tcsncmp
#define _ftcsncicmp   _tcsncicmp
#define _ftcsnicmp    _tcsnicmp

#define _ftcscoll     _tcscoll
#define _ftcsicoll    _tcsicoll
#define _ftcsnccoll   _tcsnccoll
#define _ftcsncoll    _tcsncoll
#define _ftcsncicoll  _tcsncicoll
#define _ftcsnicoll   _tcsnicoll

/* Redundant "logical-character" mappings */

#define _ftcsclen   _tcsclen
#define _ftcsnccat  _tcsnccat
#define _ftcsnccpy  _tcsnccpy
#define _ftcsncset  _tcsncset

#define _ftcsdec    _tcsdec
#define _ftcsinc    _tcsinc
#define _ftcsnbcnt  _tcsnbcnt
#define _ftcsnccnt  _tcsnccnt
#define _ftcsnextc  _tcsnextc
#define _ftcsninc   _tcsninc
#define _ftcsspnp   _tcsspnp

#define _ftcslwr    _tcslwr
#define _ftcsupr    _tcsupr

#define _ftclen     _tclen
#define _ftccpy     _tccpy
#define _ftccmp     _tccmp


#ifdef  _UNICODE

#ifdef __cplusplus
}   /* ... extern "C" */
#endif

/* ++++++++++++++++++++ UNICODE ++++++++++++++++++++ */

#include <wchar.h>

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef _WCTYPE_T_DEFINED
typedef wchar_t wint_t;
typedef wchar_t wctype_t;
#define _WCTYPE_T_DEFINED
#endif

#ifndef __TCHAR_DEFINED
typedef wchar_t     _TCHAR;
typedef wchar_t     _TSCHAR;
typedef wchar_t     _TUCHAR;
typedef wchar_t     _TXCHAR;
typedef wint_t      _TINT;
#define __TCHAR_DEFINED
#endif

#ifndef _TCHAR_DEFINED
#if     !__STDC__
typedef wchar_t     TCHAR;
#endif
#define _TCHAR_DEFINED
#endif

#define _TEOF       WEOF

#define __T(x)      L ## x


/* Program */

#define _tmain      wmain
#define _tWinMain   wWinMain
#define _tenviron   _wenviron
#define __targv     __wargv

/* Formatted i/o */

#define _tprintf    wprintf
#define _ftprintf   fwprintf
#define _stprintf   swprintf
#define _sntprintf  _snwprintf
#define _vtprintf   vwprintf
#define _vftprintf  vfwprintf
#define _vstprintf  vswprintf
#define _vsntprintf _vsnwprintf
#define _tscanf     wscanf
#define _ftscanf    fwscanf
#define _stscanf    swscanf


/* Unformatted i/o */

#define _fgettc     fgetwc
#define _fgettchar  _fgetwchar
#define _fgetts     fgetws
#define _fputtc     fputwc
#define _fputtchar  _fputwchar
#define _fputts     fputws
#define _gettc      getwc
#define _gettchar   getwchar
#define _getts      _getws
#define _puttc      putwc
#define _puttchar   putwchar
#define _putts      _putws
#define _ungettc    ungetwc


/* String conversion functions */

#define _tcstod     wcstod
#define _tcstol     wcstol
#define _tcstoul    wcstoul

#define _itot       _itow
#define _ltot       _ltow
#define _ultot      _ultow
#define _ttoi       _wtoi
#define _ttol       _wtol

#define _ttoi64     _wtoi64
#define _i64tot     _i64tow
#define _ui64tot    _ui64tow

/* String functions */

#define _tcscat     wcscat
#define _tcschr     wcschr
#define _tcscpy     wcscpy
#define _tcscspn    wcscspn
#define _tcslen     wcslen
#define _tcsncat    wcsncat
#define _tcsncpy    wcsncpy
#define _tcspbrk    wcspbrk
#define _tcsrchr    wcsrchr
#define _tcsspn     wcsspn
#define _tcsstr     wcsstr
#define _tcstok     wcstok

#define _tcsdup     _wcsdup
#define _tcsnset    _wcsnset
#define _tcsrev     _wcsrev
#define _tcsset     _wcsset

#define _tcscmp     wcscmp
#define _tcsicmp    _wcsicmp
#define _tcsnccmp   wcsncmp
#define _tcsncmp    wcsncmp
#define _tcsncicmp  _wcsnicmp
#define _tcsnicmp   _wcsnicmp

#define _tcscoll    wcscoll
#define _tcsicoll   _wcsicoll
#define _tcsnccoll  _wcsncoll
#define _tcsncoll   _wcsncoll
#define _tcsncicoll _wcsnicoll
#define _tcsnicoll  _wcsnicoll


/* Execute functions */

#define _texecl     _wexecl
#define _texecle    _wexecle
#define _texeclp    _wexeclp
#define _texeclpe   _wexeclpe
#define _texecv     _wexecv
#define _texecve    _wexecve
#define _texecvp    _wexecvp
#define _texecvpe   _wexecvpe

#define _tspawnl    _wspawnl
#define _tspawnle   _wspawnle
#define _tspawnlp   _wspawnlp
#define _tspawnlpe  _wspawnlpe
#define _tspawnv    _wspawnv
#define _tspawnve   _wspawnve
#define _tspawnvp   _wspawnvp
#define _tspawnvp   _wspawnvp
#define _tspawnvpe  _wspawnvpe

#define _tsystem    _wsystem


/* Time functions */

#define _tasctime   _wasctime
#define _tctime     _wctime
#define _tstrdate   _wstrdate
#define _tstrtime   _wstrtime
#define _tutime     _wutime
#define _tcsftime   wcsftime


/* Directory functions */

#define _tchdir     _wchdir
#define _tgetcwd    _wgetcwd
#define _tgetdcwd   _wgetdcwd
#define _tmkdir     _wmkdir
#define _trmdir     _wrmdir


/* Environment/Path functions */

#define _tfullpath  _wfullpath
#define _tgetenv    _wgetenv
#define _tmakepath  _wmakepath
#define _tputenv    _wputenv
#define _tsearchenv _wsearchenv
#define _tsplitpath _wsplitpath


/* Stdio functions */

#define _tfdopen    _wfdopen
#define _tfsopen    _wfsopen
#define _tfopen     _wfopen
#define _tfreopen   _wfreopen
#define _tperror    _wperror
#define _tpopen     _wpopen
#define _ttempnam   _wtempnam
#define _ttmpnam    _wtmpnam


/* Io functions */

#define _taccess    _waccess
#define _tchmod     _wchmod
#define _tcreat     _wcreat
#define _tfindfirst _wfindfirst
#define _tfindfirsti64  _wfindfirsti64
#define _tfindnext  _wfindnext
#define _tfindnexti64   _wfindnexti64
#define _tmktemp    _wmktemp
#define _topen      _wopen
#define _tremove    _wremove
#define _trename    _wrename
#define _tsopen     _wsopen
#define _tunlink    _wunlink

#define _tfinddata_t    _wfinddata_t
#define _tfinddatai64_t _wfinddatai64_t


/* Stat functions */

#define _tstat      _wstat
#define _tstati64   _wstati64


/* Setlocale functions */

#define _tsetlocale _wsetlocale


/* Redundant "logical-character" mappings */

#define _tcsclen    wcslen
#define _tcsnccat   wcsncat
#define _tcsnccpy   wcsncpy
#define _tcsncset   _wcsnset

#define _tcsdec     _wcsdec
#define _tcsinc     _wcsinc
#define _tcsnbcnt   _wcsncnt
#define _tcsnccnt   _wcsncnt
#define _tcsnextc   _wcsnextc
#define _tcsninc    _wcsninc
#define _tcsspnp    _wcsspnp

#define _tcslwr     _wcslwr
#define _tcsupr     _wcsupr
#define _tcsxfrm    wcsxfrm


#if     (__STDC__ || defined(_NO_INLINING)) && !defined(_M_M68K)
#define _tclen(_pc) (1)
#define _tccpy(_pc1,_cpc2) ((*(_pc1) = *(_cpc2)))
#define _tccmp(_cpc1,_cpc2) ((*(_cpc1))-(*(_cpc2)))
#else   /* __STDC__ */
__inline size_t __cdecl _tclen(const wchar_t *_cpc) { return (_cpc,1); }
__inline void __cdecl _tccpy(wchar_t *_pc1, const wchar_t *_cpc2) { *_pc1 = (wchar_t)*_cpc2; }
__inline int __cdecl _tccmp(const wchar_t *_cpc1, const wchar_t *_cpc2) { return (int) ((*_cpc1)-(*_cpc2)); }
#endif  /* __STDC__ */


/* ctype functions */

#define _istalnum   iswalnum
#define _istalpha   iswalpha
#define _istascii   iswascii
#define _istcntrl   iswcntrl
#define _istdigit   iswdigit
#define _istgraph   iswgraph
#define _istlower   iswlower
#define _istprint   iswprint
#define _istpunct   iswpunct
#define _istspace   iswspace
#define _istupper   iswupper
#define _istxdigit  iswxdigit

#define _totupper   towupper
#define _totlower   towlower

#define _istlegal(_c)   (1)
#define _istlead(_c)    (0)
#define _istleadbyte(_c)    (0)


#if     (__STDC__ || defined(_NO_INLINING)) && !defined(_M_M68K)
#define _wcsdec(_cpc1, _cpc2) ((_cpc2)-1)
#define _wcsinc(_pc)    ((_pc)+1)
#define _wcsnextc(_cpc) ((unsigned int) *(_cpc))
#define _wcsninc(_pc, _sz) (((_pc)+(_sz)))
#define _wcsncnt(_cpc, _sz) ((wcslen(_cpc)>_sz) ? _sz : wcslen(_cpc))
#define _wcsspnp(_cpc1, _cpc2) ((*((_cpc1)+wcsspn(_cpc1,_cpc2))) ? ((_cpc1)+wcsspn(_cpc1,_cpc2)) : NULL)
#else   /* __STDC__ */
__inline wchar_t * __cdecl _wcsdec(const wchar_t * _cpc1, const wchar_t * _cpc2) { return (wchar_t *)(_cpc1,(_cpc2-1)); }
__inline wchar_t * __cdecl _wcsinc(const wchar_t * _pc) { return (wchar_t *)(_pc+1); }
__inline unsigned int __cdecl _wcsnextc(const wchar_t * _cpc) { return (unsigned int)*_cpc; }
__inline wchar_t * __cdecl _wcsninc(const wchar_t * _pc, size_t _sz) { return (wchar_t *)(_pc+_sz); }
__inline size_t __cdecl _wcsncnt( const wchar_t * _cpc, size_t _sz) { size_t len; len = wcslen(_cpc); return (len>_sz) ? _sz : len; }
__inline wchar_t * __cdecl _wcsspnp( const wchar_t * _cpc1, const wchar_t * _cpc2) { return (*(_cpc1 += wcsspn(_cpc1,_cpc2))!='\0') ? (wchar_t*)_cpc1 : NULL; }
#endif  /* __STDC__ */


#else   /* ndef _UNICODE */

/* ++++++++++++++++++++ SBCS and MBCS ++++++++++++++++++++ */

#ifdef __cplusplus
}   /* ... extern "C" */
#endif

#include <string.h>

#ifdef  __cplusplus
extern "C" {
#endif


#define _TEOF       EOF

#define __T(x)      x


/* Program */

#define _tmain      main
#define _tWinMain   WinMain
#ifdef  _POSIX_
#define _tenviron   environ
#else
#define _tenviron  _environ
#endif
#define __targv     __argv


/* Formatted i/o */

#define _tprintf    printf
#define _ftprintf   fprintf
#define _stprintf   sprintf
#define _sntprintf  _snprintf
#define _vtprintf   vprintf
#define _vftprintf  vfprintf
#define _vstprintf  vsprintf
#define _vsntprintf _vsnprintf
#define _tscanf     scanf
#define _ftscanf    fscanf
#define _stscanf    sscanf


/* Unformatted i/o */

#define _fgettc     fgetc
#define _fgettchar  _fgetchar
#define _fgetts     fgets
#define _fputtc     fputc
#define _fputtchar  _fputchar
#define _fputts     fputs
#define _gettc      getc
#define _gettchar   getchar
#define _getts      gets
#define _puttc      putc
#define _puttchar   putchar
#define _putts      puts
#define _ungettc    ungetc


/* String conversion functions */

#define _tcstod     strtod
#define _tcstol     strtol
#define _tcstoul    strtoul

#define _itot       _itoa
#define _ltot       _ltoa
#define _ultot      _ultoa
#define _ttoi       atoi
#define _ttol       atol

#define _ttoi64     atoi64
#define _i64tot     _i64toa
#define _ui64tot    _ui64toa

/* String functions */

#define _tcscat     strcat
#define _tcscpy     strcpy
#define _tcslen     strlen
#define _tcsxfrm    strxfrm
#define _tcsdup     _strdup


/* Execute functions */

#define _texecl     _execl
#define _texecle    _execle
#define _texeclp    _execlp
#define _texeclpe   _execlpe
#define _texecv     _execv
#define _texecve    _execve
#define _texecvp    _execvp
#define _texecvpe   _execvpe

#define _tspawnl    _spawnl
#define _tspawnle   _spawnle
#define _tspawnlp   _spawnlp
#define _tspawnlpe  _spawnlpe
#define _tspawnv    _spawnv
#define _tspawnve   _spawnve
#define _tspawnvp   _spawnvp
#define _tspawnvpe  _spawnvpe

#define _tsystem    system


/* Time functions */

#define _tasctime   asctime
#define _tctime     ctime
#define _tstrdate   _strdate
#define _tstrtime   _strtime
#define _tutime     _utime
#define _tcsftime   strftime


/* Directory functions */

#define _tchdir     _chdir
#define _tgetcwd    _getcwd
#define _tgetdcwd   _getdcwd
#define _tmkdir     _mkdir
#define _trmdir     _rmdir


/* Environment/Path functions */

#define _tfullpath  _fullpath
#define _tgetenv    getenv
#define _tmakepath  _makepath
#define _tputenv    _putenv
#define _tsearchenv _searchenv
#define _tsplitpath _splitpath


/* Stdio functions */

#ifdef  _POSIX_
#define _tfdopen    fdopen
#else
#define _tfdopen    _fdopen
#endif
#define _tfsopen    _fsopen
#define _tfopen     fopen
#define _tfreopen   freopen
#define _tperror    perror
#define _tpopen     _popen
#define _ttempnam   _tempnam
#define _ttmpnam    tmpnam


/* Io functions */

#define _tchmod     _chmod
#define _tcreat     _creat
#define _tfindfirst _findfirst
#define _tfindfirsti64  _findfirsti64
#define _tfindnext  _findnext
#define _tfindnexti64   _findnexti64
#define _tmktemp    _mktemp

#ifdef _POSIX_
#define _topen      open
#define _taccess    access
#else
#define _topen      _open
#define _taccess    _access
#endif

#define _tremove    remove
#define _trename    rename
#define _tsopen     _sopen
#define _tunlink    _unlink

#define _tfinddata_t    _finddata_t
#define _tfinddatai64_t _finddatai64_t


/* ctype functions */

#define _istascii   isascii
#define _istcntrl   iscntrl
#define _istxdigit  isxdigit


/* Stat functions */

#define _tstat      _stat
#define _tstati64   _stati64


/* Setlocale functions */

#define _tsetlocale setlocale


#ifdef _MBCS

/* ++++++++++++++++++++ MBCS ++++++++++++++++++++ */

#ifdef __cplusplus
}   /* ... extern "C" */
#endif

#include <mbstring.h>

#ifdef  __cplusplus
extern "C" {
#endif


#ifndef __TCHAR_DEFINED
typedef char            _TCHAR;
typedef signed char     _TSCHAR;
typedef unsigned char   _TUCHAR;
typedef unsigned char   _TXCHAR;
typedef unsigned int    _TINT;
#define __TCHAR_DEFINED
#endif

#ifndef _TCHAR_DEFINED
#if     !__STDC__
typedef char            TCHAR;
#endif
#define _TCHAR_DEFINED
#endif


#ifdef _MB_MAP_DIRECT

/* use mb functions directly - types must match */

/* String functions */

#define _tcschr     _mbschr
#define _tcscspn    _mbscspn
#define _tcsncat    _mbsnbcat
#define _tcsncpy    _mbsnbcpy
#define _tcspbrk    _mbspbrk
#define _tcsrchr    _mbsrchr
#define _tcsspn     _mbsspn
#define _tcsstr     _mbsstr
#define _tcstok     _mbstok

#define _tcsnset    _mbsnbset
#define _tcsrev     _mbsrev
#define _tcsset     _mbsset

#define _tcscmp     _mbscmp
#define _tcsicmp    _mbsicmp
#define _tcsnccmp   _mbsncmp
#define _tcsncmp    _mbsnbcmp
#define _tcsncicmp  _mbsnicmp
#define _tcsnicmp   _mbsnbicmp

#define _tcscoll    _mbscoll
#define _tcsicoll   _mbsicoll
#define _tcsnccoll  _mbsncoll
#define _tcsncoll   _mbsnbcoll
#define _tcsncicoll _mbsnicoll
#define _tcsnicoll  _mbsnbicoll


/* "logical-character" mappings */

#define _tcsclen    _mbslen
#define _tcsnccat   _mbsncat
#define _tcsnccpy   _mbsncpy
#define _tcsncset   _mbsnset


/* MBCS-specific mappings */

#define _tcsdec     _mbsdec
#define _tcsinc     _mbsinc
#define _tcsnbcnt   _mbsnbcnt
#define _tcsnccnt   _mbsnccnt
#define _tcsnextc   _mbsnextc
#define _tcsninc    _mbsninc
#define _tcsspnp    _mbsspnp

#define _tcslwr     _mbslwr
#define _tcsupr     _mbsupr

#define _tclen      _mbclen
#define _tccpy      _mbccpy

#define _tccmp(_cpuc1,_cpuc2)   _tcsnccmp(_cpuc1,_cpuc2,1)


#else /* _MB_MAP_DIRECT */

#if     (__STDC__ || defined(_NO_INLINING)) && !defined(_M_M68K)

/* use type-safe linked-in function thunks */

/* String functions */

_CRTIMP char * __cdecl _tcschr(const char *, unsigned int);
_CRTIMP size_t __cdecl _tcscspn(const char *, const char *);
_CRTIMP char * __cdecl _tcsncat(char *, const char *, size_t);
_CRTIMP char * __cdecl _tcsncpy(char *, const char *, size_t);
_CRTIMP char * __cdecl _tcspbrk(const char *, const char *);
_CRTIMP char * __cdecl _tcsrchr(const char *, int);
_CRTIMP size_t __cdecl _tcsspn(const char *, const char *);
_CRTIMP char * __cdecl _tcsstr(const char *, const char *);
_CRTIMP char * __cdecl _tcstok(char *, const char *);

_CRTIMP char * __cdecl _tcsnset(char *, unsigned int, size_t);
_CRTIMP char * __cdecl _tcsrev(char *);
_CRTIMP char * __cdecl _tcsset(char *, unsigned int);

_CRTIMP int __cdecl _tcscmp(const char *, const char *);
_CRTIMP int __cdecl _tcsicmp(const char *, const char *);
_CRTIMP int __cdecl _tcsnccmp(const char *, const char *, size_t);
_CRTIMP int __cdecl _tcsncmp(const char *, const char *, size_t);
_CRTIMP int __cdecl _tcsncicmp(const char *, const char *, size_t);
_CRTIMP int __cdecl _tcsnicmp(const char *, const char *, size_t);

_CRTIMP int __cdecl _tcscoll(const char *, const char *);
_CRTIMP int __cdecl _tcsicoll(const char *, const char *);
_CRTIMP int __cdecl _tcsnccoll(const char *, const char *, size_t);
_CRTIMP int __cdecl _tcsncoll(const char *, const char *, size_t);
_CRTIMP int __cdecl _tcsncicoll(const char *, const char *, size_t);
_CRTIMP int __cdecl _tcsnicoll(const char *, const char *, size_t);


/* "logical-character" mappings */

_CRTIMP size_t __cdecl _tcsclen(const char *);
_CRTIMP char * __cdecl _tcsnccat(char *, const char *, size_t);
_CRTIMP char * __cdecl _tcsnccpy(char *, const char *, size_t);
_CRTIMP char * __cdecl _tcsncset(char *, unsigned int, size_t);


/* MBCS-specific mappings */

_CRTIMP char * __cdecl _tcsdec(const char *, const char *);
_CRTIMP char * __cdecl _tcsinc(const char *);
_CRTIMP size_t __cdecl _tcsnbcnt(const char *, size_t);
_CRTIMP size_t __cdecl _tcsnccnt(const char *, size_t);
_CRTIMP unsigned int __cdecl _tcsnextc (const char *);
_CRTIMP char * __cdecl _tcsninc(const char *, size_t);
_CRTIMP char * __cdecl _tcsspnp(const char *, const char *);

_CRTIMP char * __cdecl _tcslwr(char *);
_CRTIMP char * __cdecl _tcsupr(char *);

_CRTIMP size_t __cdecl _tclen(const char *);
_CRTIMP void __cdecl _tccpy(char *, const char *);


#else   /* __STDC__ */

/* the default: use type-safe inline function thunks */

#define _PUC    unsigned char *
#define _CPUC   const unsigned char *
#define _PC     char *
#define _CPC    const char *
#define _UI     unsigned int


/* String functions */

__inline _PC _tcschr(_CPC _s1,_UI _c) {return (_PC)_mbschr((_CPUC)_s1,_c);}
__inline size_t _tcscspn(_CPC _s1,_CPC _s2) {return _mbscspn((_CPUC)_s1,(_CPUC)_s2);}
__inline _PC _tcsncat(_PC _s1,_CPC _s2,size_t _n) {return (_PC)_mbsnbcat((_PUC)_s1,(_CPUC)_s2,_n);}
__inline _PC _tcsncpy(_PC _s1,_CPC _s2,size_t _n) {return (_PC)_mbsnbcpy((_PUC)_s1,(_CPUC)_s2,_n);}
__inline _PC _tcspbrk(_CPC _s1,_CPC _s2) {return (_PC)_mbspbrk((_CPUC)_s1,(_CPUC)_s2);}
__inline _PC _tcsrchr(_CPC _s1,_UI _c) {return (_PC)_mbsrchr((_CPUC)_s1,_c);}
__inline size_t _tcsspn(_CPC _s1,_CPC _s2) {return _mbsspn((_CPUC)_s1,(_CPUC)_s2);}
__inline _PC _tcsstr(_CPC _s1,_CPC _s2) {return (_PC)_mbsstr((_CPUC)_s1,(_CPUC)_s2);}
__inline _PC _tcstok(_PC _s1,_CPC _s2) {return (_PC)_mbstok((_PUC)_s1,(_CPUC)_s2);}

__inline _PC _tcsnset(_PC _s1,_UI _c,size_t _n) {return (_PC)_mbsnbset((_PUC)_s1,_c,_n);}
__inline _PC _tcsrev(_PC _s1) {return (_PC)_mbsrev((_PUC)_s1);}
__inline _PC _tcsset(_PC _s1,_UI _c) {return (_PC)_mbsset((_PUC)_s1,_c);}

__inline int _tcscmp(_CPC _s1,_CPC _s2) {return _mbscmp((_CPUC)_s1,(_CPUC)_s2);}
__inline int _tcsicmp(_CPC _s1,_CPC _s2) {return _mbsicmp((_CPUC)_s1,(_CPUC)_s2);}
__inline int _tcsnccmp(_CPC _s1,_CPC _s2,size_t _n) {return _mbsncmp((_CPUC)_s1,(_CPUC)_s2,_n);}
__inline int _tcsncmp(_CPC _s1,_CPC _s2,size_t _n) {return _mbsnbcmp((_CPUC)_s1,(_CPUC)_s2,_n);}
__inline int _tcsncicmp(_CPC _s1,_CPC _s2,size_t _n) {return _mbsnicmp((_CPUC)_s1,(_CPUC)_s2,_n);}
__inline int _tcsnicmp(_CPC _s1,_CPC _s2,size_t _n) {return _mbsnbicmp((_CPUC)_s1,(_CPUC)_s2,_n);}

__inline int _tcscoll(_CPC _s1,_CPC _s2) {return _mbscoll((_CPUC)_s1,(_CPUC)_s2);}
__inline int _tcsicoll(_CPC _s1,_CPC _s2) {return _mbsicoll((_CPUC)_s1,(_CPUC)_s2);}
__inline int _tcsnccoll(_CPC _s1,_CPC _s2,size_t _n) {return _mbsncoll((_CPUC)_s1,(_CPUC)_s2,_n);}
__inline int _tcsncoll(_CPC _s1,_CPC _s2,size_t _n) {return _mbsnbcoll((_CPUC)_s1,(_CPUC)_s2,_n);}
__inline int _tcsncicoll(_CPC _s1,_CPC _s2,size_t _n) {return _mbsnicoll((_CPUC)_s1,(_CPUC)_s2,_n);}
__inline int _tcsnicoll(_CPC _s1,_CPC _s2,size_t _n) {return _mbsnbicoll((_CPUC)_s1,(_CPUC)_s2,_n);}


/* "logical-character" mappings */

__inline size_t _tcsclen(_CPC _s1) {return _mbslen((_CPUC)_s1);}
__inline _PC _tcsnccat(_PC _s1,_CPC _s2,size_t _n) {return (_PC)_mbsncat((_PUC)_s1,(_CPUC)_s2,_n);}
__inline _PC _tcsnccpy(_PC _s1,_CPC _s2,size_t _n) {return (_PC)_mbsncpy((_PUC)_s1,(_CPUC)_s2,_n);}
__inline _PC _tcsncset(_PC _s1,_UI _c,size_t _n) {return (_PC)_mbsnset((_PUC)_s1,_c,_n);}


/* MBCS-specific mappings */

__inline _PC _tcsdec(_CPC _s1,_CPC _s2) {return (_PC)_mbsdec((_CPUC)_s1,(_CPUC)_s2);}
__inline _PC _tcsinc(_CPC _s1) {return (_PC)_mbsinc((_CPUC)_s1);}
__inline size_t _tcsnbcnt(_CPC _s1,size_t _n) {return _mbsnbcnt((_CPUC)_s1,_n);}
__inline size_t _tcsnccnt(_CPC _s1,size_t _n) {return _mbsnccnt((_CPUC)_s1,_n);}
__inline _PC _tcsninc(_CPC _s1,size_t _n) {return (_PC)_mbsninc((_CPUC)_s1,_n);}
__inline _PC _tcsspnp(_CPC _s1,_CPC _s2) {return (_PC)_mbsspnp((_CPUC)_s1,(_CPUC)_s2);}
__inline _PC _tcslwr(_PC _s1) {return (_PC)_mbslwr((_PUC)_s1);}
__inline _PC _tcsupr(_PC _s1) {return (_PC)_mbsupr((_PUC)_s1);}

__inline size_t _tclen(_CPC _s1) {return _mbclen((_CPUC)_s1);}
__inline void _tccpy(_PC _s1,_CPC _s2) {_mbccpy((_PUC)_s1,(_CPUC)_s2); return;}


/* inline helper */
__inline _UI _tcsnextc(_CPC _s1) {_UI _n=0; if (_ismbblead((_UI)*(_PUC)_s1)) _n=((_UI)*_s1++)<<8; _n+=(_UI)*_s1; return(_n);}


#endif /* __STDC__ */

#endif /* _MB_MAP_DIRECT */


/* MBCS-specific mappings */

#define _tccmp(_cp1,_cp2)   _tcsnccmp(_cp1,_cp2,1)


/* ctype functions */

#define _istalnum   _ismbcalnum
#define _istalpha   _ismbcalpha
#define _istdigit   _ismbcdigit
#define _istgraph   _ismbcgraph
#define _istlegal   _ismbclegal
#define _istlower   _ismbclower
#define _istprint   _ismbcprint
#define _istpunct   _ismbcpunct
#define _istspace   _ismbcspace
#define _istupper   _ismbcupper

#define _totupper   _mbctoupper
#define _totlower   _mbctolower

#define _istlead    _ismbblead
#define _istleadbyte    isleadbyte

#else   /* !_MBCS */

/* ++++++++++++++++++++ SBCS ++++++++++++++++++++ */


#ifndef __TCHAR_DEFINED
typedef char            _TCHAR;
typedef signed char     _TSCHAR;
typedef unsigned char   _TUCHAR;
typedef char            _TXCHAR;
typedef int             _TINT;
#define __TCHAR_DEFINED
#endif

#ifndef _TCHAR_DEFINED
#if     !__STDC__
typedef char            TCHAR;
#endif
#define _TCHAR_DEFINED
#endif


/* String functions */

#define _tcschr     strchr
#define _tcscspn    strcspn
#define _tcsncat    strncat
#define _tcsncpy    strncpy
#define _tcspbrk    strpbrk
#define _tcsrchr    strrchr
#define _tcsspn     strspn
#define _tcsstr     strstr
#define _tcstok     strtok

#define _tcsnset    _strnset
#define _tcsrev     _strrev
#define _tcsset     _strset

#define _tcscmp     strcmp
#define _tcsicmp    _stricmp
#define _tcsnccmp   strncmp
#define _tcsncmp    strncmp
#define _tcsncicmp  _strnicmp
#define _tcsnicmp   _strnicmp

#define _tcscoll    strcoll
#define _tcsicoll   _stricoll
#define _tcsnccoll  _strncoll
#define _tcsncoll   _strncoll
#define _tcsncicoll _strnicoll
#define _tcsnicoll  _strnicoll


/* "logical-character" mappings */

#define _tcsclen    strlen
#define _tcsnccat   strncat
#define _tcsnccpy   strncpy
#define _tcsncset   _strnset


/* MBCS-specific functions */

#define _tcsdec     _strdec
#define _tcsinc     _strinc
#define _tcsnbcnt   _strncnt
#define _tcsnccnt   _strncnt
#define _tcsnextc   _strnextc
#define _tcsninc    _strninc
#define _tcsspnp    _strspnp

#define _tcslwr     _strlwr
#define _tcsupr     _strupr
#define _tcsxfrm    strxfrm

#define _istlead(_c)    (0)
#define _istleadbyte(_c)    (0)

#if     (__STDC__ || defined(_NO_INLINING)) && !defined(_M_M68K)
#define _tclen(_pc) (1)
#define _tccpy(_pc1,_cpc2) (*(_pc1) = *(_cpc2))
#define _tccmp(_cpc1,_cpc2) (((unsigned char)*(_cpc1))-((unsigned char)*(_cpc2)))
#else   /* __STDC__ */
__inline size_t __cdecl _tclen(const char *_cpc) { return (_cpc,1); }
__inline void __cdecl _tccpy(char *_pc1, const char *_cpc2) { *_pc1 = *_cpc2; }
__inline int __cdecl _tccmp(const char *_cpc1, const char *_cpc2) { return (int) (((unsigned char)*_cpc1)-((unsigned char)*_cpc2)); }
#endif  /* __STDC__ */


/* ctype-functions */

#define _istalnum   isalnum
#define _istalpha   isalpha
#define _istdigit   isdigit
#define _istgraph   isgraph
#define _istlower   islower
#define _istprint   isprint
#define _istpunct   ispunct
#define _istspace   isspace
#define _istupper   isupper

#define _totupper   toupper
#define _totlower   tolower

#define _istlegal(_c)   (1)


/* the following is optional if functional versions are available */

/* define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif


#if     (__STDC__ || defined(_NO_INLINING)) && !defined(_M_M68K)
#define _strdec(_cpc1, _cpc2) ((_cpc2)-1)
#define _strinc(_pc)    ((_pc)+1)
#define _strnextc(_cpc) ((unsigned int) *(_cpc))
#define _strninc(_pc, _sz) (((_pc)+(_sz)))
#define _strncnt(_cpc, _sz) ((strlen(_cpc)>_sz) ? _sz : strlen(_cpc))
#define _strspnp(_cpc1, _cpc2) ((*((_cpc1)+strspn(_cpc1,_cpc2))) ? ((_cpc1)+strspn(_cpc1,_cpc2)) : NULL)
#else   /* __STDC__ */
__inline char * __cdecl _strdec(const char * _cpc1, const char * _cpc2) { return (char *)(_cpc1,(_cpc2-1)); }
__inline char * __cdecl _strinc(const char * _pc) { return (char *)(_pc+1); }
__inline unsigned int __cdecl _strnextc(const char * _cpc) { return (unsigned int)*_cpc; }
__inline char * __cdecl _strninc(const char * _pc, size_t _sz) { return (char *)(_pc+_sz); }
__inline size_t __cdecl _strncnt( const char * _cpc, size_t _sz) { size_t len; len = strlen(_cpc); return (len>_sz) ? _sz : len; }
__inline char * __cdecl _strspnp( const char * _cpc1, const char * _cpc2) { return (*(_cpc1 += strspn(_cpc1,_cpc2))!='\0') ? (char*)_cpc1 : NULL; }
#endif  /* __STDC__ */


#endif  /* _MBCS */

#endif  /* _UNICODE */


/* Generic text macros to be used with string literals and character constants.
   Will also allow symbolic constants that resolve to same. */

#define _T(x)       __T(x)
#define _TEXT(x)    __T(x)


#ifdef __cplusplus
}   /* ... extern "C" */
#endif

#endif  /* _INC_TCHAR */
