/*
 * Unicode definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_WCHAR_H
#define __WINE_WCHAR_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#include <stdarg.h>

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MSVCRT_WCHAR_T_DEFINED
#define MSVCRT_WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short MSVCRT(wchar_t);
#endif
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL  0
#else
#define NULL  ((void *)0)
#endif
#endif

#define WCHAR_MIN 0
#define WCHAR_MAX ((MSVCRT(wchar_t))-1)

typedef int MSVCRT(mbstate_t);

#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif

#ifndef MSVCRT_WCTYPE_T_DEFINED
typedef MSVCRT(wchar_t) MSVCRT(wint_t);
typedef MSVCRT(wchar_t) MSVCRT(wctype_t);
#define MSVCRT_WCTYPE_T_DEFINED
#endif

#ifndef _MSC_VER
# ifndef __int64
#  define __int64 long long
# endif
#endif

#ifndef USE_MSVCRT_PREFIX
# ifndef WEOF
#  define WEOF        (wint_t)(0xFFFF)
# endif
#else
# ifndef MSVCRT_WEOF
#  define MSVCRT_WEOF (MSVCRT_wint_t)(0xFFFF)
# endif
#endif /* USE_MSVCRT_PREFIX */

#ifndef MSVCRT_FSIZE_T_DEFINED
typedef unsigned long _fsize_t;
#define MSVCRT_FSIZE_T_DEFINED
#endif

#ifndef MSVCRT_DEV_T_DEFINED
typedef unsigned int   _dev_t;
#define MSVCRT_DEV_T_DEFINED
#endif

#ifndef MSVCRT_INO_T_DEFINED
typedef unsigned short _ino_t;
#define MSVCRT_INO_T_DEFINED
#endif

#ifndef MSVCRT_OFF_T_DEFINED
typedef int MSVCRT(_off_t);
#define MSVCRT_OFF_T_DEFINED
#endif

#ifndef MSVCRT_TIME_T_DEFINED
typedef long MSVCRT(time_t);
#define MSVCRT_TIME_T_DEFINED
#endif

#ifndef MSVCRT_TM_DEFINED
#define MSVCRT_TM_DEFINED
struct MSVCRT(tm) {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};
#endif /* MSVCRT_TM_DEFINED */

#ifndef MSVCRT_FILE_DEFINED
#define MSVCRT_FILE_DEFINED
typedef struct MSVCRT(_iobuf)
{
  char* _ptr;
  int   _cnt;
  char* _base;
  int   _flag;
  int   _file;
  int   _charbuf;
  int   _bufsiz;
  char* _tmpfname;
} MSVCRT(FILE);
#endif  /* MSVCRT_FILE_DEFINED */

#ifndef MSVCRT_WFINDDATA_T_DEFINED
#define MSVCRT_WFINDDATA_T_DEFINED

struct _wfinddata_t {
  unsigned attrib;
  MSVCRT(time_t) time_create;
  MSVCRT(time_t) time_access;
  MSVCRT(time_t) time_write;
  _fsize_t       size;
  MSVCRT(wchar_t) name[260];
};

struct _wfinddatai64_t {
  unsigned attrib;
  MSVCRT(time_t) time_create;
  MSVCRT(time_t) time_access;
  MSVCRT(time_t) time_write;
  __int64        size;
  MSVCRT(wchar_t) name[260];
};

#endif /* MSVCRT_WFINDDATA_T_DEFINED */

#ifndef MSVCRT_STAT_DEFINED
#define MSVCRT_STAT_DEFINED

struct _stat {
  _dev_t         st_dev;
  _ino_t         st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t         st_rdev;
  MSVCRT(_off_t) st_size;
  MSVCRT(time_t) st_atime;
  MSVCRT(time_t) st_mtime;
  MSVCRT(time_t) st_ctime;
};

struct _stati64 {
  _dev_t         st_dev;
  _ino_t         st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t         st_rdev;
  __int64        st_size;
  MSVCRT(time_t) st_atime;
  MSVCRT(time_t) st_mtime;
  MSVCRT(time_t) st_ctime;
};
#endif /* MSVCRT_STAT_DEFINED */

/* ASCII char classification table - binary compatible */
#define _UPPER        0x0001  /* C1_UPPER */
#define _LOWER        0x0002  /* C1_LOWER */
#define _DIGIT        0x0004  /* C1_DIGIT */
#define _SPACE        0x0008  /* C1_SPACE */
#define _PUNCT        0x0010  /* C1_PUNCT */
#define _CONTROL      0x0020  /* C1_CNTRL */
#define _BLANK        0x0040  /* C1_BLANK */
#define _HEX          0x0080  /* C1_XDIGIT */
#define _LEADBYTE     0x8000
#define _ALPHA       (0x0100|_UPPER|_LOWER)  /* (C1_ALPHA|_UPPER|_LOWER) */

#ifndef MSVCRT_WCTYPE_DEFINED
#define MSVCRT_WCTYPE_DEFINED
int MSVCRT(is_wctype)(MSVCRT(wint_t),MSVCRT(wctype_t));
int MSVCRT(isleadbyte)(int);
int MSVCRT(iswalnum)(MSVCRT(wint_t));
int MSVCRT(iswalpha)(MSVCRT(wint_t));
int MSVCRT(iswascii)(MSVCRT(wint_t));
int MSVCRT(iswcntrl)(MSVCRT(wint_t));
int MSVCRT(iswctype)(MSVCRT(wint_t),MSVCRT(wctype_t));
int MSVCRT(iswdigit)(MSVCRT(wint_t));
int MSVCRT(iswgraph)(MSVCRT(wint_t));
int MSVCRT(iswlower)(MSVCRT(wint_t));
int MSVCRT(iswprint)(MSVCRT(wint_t));
int MSVCRT(iswpunct)(MSVCRT(wint_t));
int MSVCRT(iswspace)(MSVCRT(wint_t));
int MSVCRT(iswupper)(MSVCRT(wint_t));
int MSVCRT(iswxdigit)(MSVCRT(wint_t));
MSVCRT(wchar_t) MSVCRT(towlower)(MSVCRT(wchar_t));
MSVCRT(wchar_t) MSVCRT(towupper)(MSVCRT(wchar_t));
#endif /* MSVCRT_WCTYPE_DEFINED */

#ifndef MSVCRT_WDIRECT_DEFINED
#define MSVCRT_WDIRECT_DEFINED
int              _wchdir(const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)* _wgetcwd(MSVCRT(wchar_t)*,int);
MSVCRT(wchar_t)* _wgetdcwd(int,MSVCRT(wchar_t)*,int);
int              _wmkdir(const MSVCRT(wchar_t)*);
int              _wrmdir(const MSVCRT(wchar_t)*);
#endif /* MSVCRT_WDIRECT_DEFINED */

#ifndef MSVCRT_WIO_DEFINED
#define MSVCRT_WIO_DEFINED
int         _waccess(const MSVCRT(wchar_t)*,int);
int         _wchmod(const MSVCRT(wchar_t)*,int);
int         _wcreat(const MSVCRT(wchar_t)*,int);
long        _wfindfirst(const MSVCRT(wchar_t)*,struct _wfinddata_t*);
long        _wfindfirsti64(const MSVCRT(wchar_t)*, struct _wfinddatai64_t*);
int         _wfindnext(long,struct _wfinddata_t*);
int         _wfindnexti64(long, struct _wfinddatai64_t*);
MSVCRT(wchar_t)*_wmktemp(MSVCRT(wchar_t)*);
int         _wopen(const MSVCRT(wchar_t)*,int,...);
int         _wrename(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
int         _wsopen(const MSVCRT(wchar_t)*,int,int,...);
int         _wunlink(const MSVCRT(wchar_t)*);
#endif /* MSVCRT_WIO_DEFINED */

#ifndef MSVCRT_WLOCALE_DEFINED
#define MSVCRT_WLOCALE_DEFINED
MSVCRT(wchar_t)* _wsetlocale(int,const MSVCRT(wchar_t)*);
#endif /* MSVCRT_WLOCALE_DEFINED */

#ifndef MSVCRT_WPROCESS_DEFINED
#define MSVCRT_WPROCESS_DEFINED
int         _wexecl(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wexecle(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wexeclp(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wexeclpe(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wexecv(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *);
int         _wexecve(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *,const MSVCRT(wchar_t)* const *);
int         _wexecvp(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *);
int         _wexecvpe(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *,const MSVCRT(wchar_t)* const *);
int         _wspawnl(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wspawnle(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wspawnlp(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wspawnlpe(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wspawnv(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *);
int         _wspawnve(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *,const MSVCRT(wchar_t)* const *);
int         _wspawnvp(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *);
int         _wspawnvpe(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *,const MSVCRT(wchar_t)* const *);
int         _wsystem(const MSVCRT(wchar_t)*);
#endif /* MSVCRT_WPROCESS_DEFINED */

#ifndef MSVCRT_WSTAT_DEFINED
#define MSVCRT_WSTAT_DEFINED
int _wstat(const MSVCRT(wchar_t)*,struct _stat*);
int _wstati64(const MSVCRT(wchar_t)*,struct _stati64*);
#endif /* MSVCRT_WSTAT_DEFINED */

#ifndef MSVCRT_WSTDIO_DEFINED
#define MSVCRT_WSTDIO_DEFINED
MSVCRT(wint_t)  _fgetwchar(void);
MSVCRT(wint_t)  _fputwchar(MSVCRT(wint_t));
MSVCRT(wchar_t)*_getws(MSVCRT(wchar_t)*);
int             _putws(const MSVCRT(wchar_t)*);
int             _snwprintf(MSVCRT(wchar_t)*,MSVCRT(size_t),const MSVCRT(wchar_t)*,...);
int             _vsnwprintf(MSVCRT(wchar_t)*,MSVCRT(size_t),const MSVCRT(wchar_t)*,va_list);
MSVCRT(FILE)*   _wfdopen(int,const MSVCRT(wchar_t)*);
MSVCRT(FILE)*   _wfopen(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(FILE)*   _wfreopen(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(FILE)*);
MSVCRT(FILE)*   _wfsopen(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,int);
void            _wperror(const MSVCRT(wchar_t)*);
MSVCRT(FILE)*   _wpopen(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
int             _wremove(const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*_wtempnam(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*_wtmpnam(MSVCRT(wchar_t)*);

MSVCRT(wint_t)  MSVCRT(fgetwc)(MSVCRT(FILE)*);
MSVCRT(wchar_t)*MSVCRT(fgetws)(MSVCRT(wchar_t)*,int,MSVCRT(FILE)*);
MSVCRT(wint_t)  MSVCRT(fputwc)(MSVCRT(wint_t),MSVCRT(FILE)*);
int             MSVCRT(fputws)(const MSVCRT(wchar_t)*,MSVCRT(FILE)*);
int             MSVCRT(fwprintf)(MSVCRT(FILE)*,const MSVCRT(wchar_t)*,...);
int             MSVCRT(fputws)(const MSVCRT(wchar_t)*,MSVCRT(FILE)*);
int             MSVCRT(fwscanf)(MSVCRT(FILE)*,const MSVCRT(wchar_t)*,...);
MSVCRT(wint_t)  MSVCRT(getwc)(MSVCRT(FILE)*);
MSVCRT(wint_t)  MSVCRT(getwchar)(void);
MSVCRT(wchar_t)*MSVCRT(getws)(MSVCRT(wchar_t)*);
MSVCRT(wint_t)  MSVCRT(putwc)(MSVCRT(wint_t),MSVCRT(FILE)*);
MSVCRT(wint_t)  MSVCRT(putwchar)(MSVCRT(wint_t));
int             MSVCRT(putws)(const MSVCRT(wchar_t)*);
int             MSVCRT(swprintf)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int             MSVCRT(swscanf)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
MSVCRT(wint_t)  MSVCRT(ungetwc)(MSVCRT(wint_t),MSVCRT(FILE)*);
int             MSVCRT(vfwprintf)(MSVCRT(FILE)*,const MSVCRT(wchar_t)*,va_list);
int             MSVCRT(vswprintf)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,va_list);
int             MSVCRT(vwprintf)(const MSVCRT(wchar_t)*,va_list);
int             MSVCRT(wprintf)(const MSVCRT(wchar_t)*,...);
int             MSVCRT(wscanf)(const MSVCRT(wchar_t)*,...);
#endif /* MSVCRT_WSTDIO_DEFINED */

#ifndef MSVCRT_WSTDLIB_DEFINED
#define MSVCRT_WSTDLIB_DEFINED
MSVCRT(wchar_t)*_itow(int,MSVCRT(wchar_t)*,int);
MSVCRT(wchar_t)*_i64tow(__int64,MSVCRT(wchar_t)*,int);
MSVCRT(wchar_t)*_ltow(long,MSVCRT(wchar_t)*,int);
MSVCRT(wchar_t)*_ui64tow(unsigned __int64,MSVCRT(wchar_t)*,int);
MSVCRT(wchar_t)*_ultow(unsigned long,MSVCRT(wchar_t)*,int);
MSVCRT(wchar_t)*_wfullpath(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,size_t);
MSVCRT(wchar_t)*_wgetenv(const MSVCRT(wchar_t)*);
void            _wmakepath(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
void            _wperror(const MSVCRT(wchar_t)*);
int             _wputenv(const MSVCRT(wchar_t)*);
void            _wsearchenv(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(wchar_t)*);
void            _wsplitpath(const MSVCRT(wchar_t)*,MSVCRT(wchar_t)*,MSVCRT(wchar_t)*,MSVCRT(wchar_t)*,MSVCRT(wchar_t)*);
int             _wsystem(const MSVCRT(wchar_t)*);
int             _wtoi(const MSVCRT(wchar_t)*);
__int64         _wtoi64(const MSVCRT(wchar_t)*);
long            _wtol(const MSVCRT(wchar_t)*);

MSVCRT(size_t) MSVCRT(mbstowcs)(MSVCRT(wchar_t)*,const char*,MSVCRT(size_t));
int            MSVCRT(mbtowc)(MSVCRT(wchar_t)*,const char*,MSVCRT(size_t));
double         MSVCRT(wcstod)(const MSVCRT(wchar_t)*,MSVCRT(wchar_t)**);
long           MSVCRT(wcstol)(const MSVCRT(wchar_t)*,MSVCRT(wchar_t)**,int);
MSVCRT(size_t) MSVCRT(wcstombs)(char*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
unsigned long  MSVCRT(wcstoul)(const MSVCRT(wchar_t)*,MSVCRT(wchar_t)**,int);
int            MSVCRT(wctomb)(char*,MSVCRT(wchar_t));
#endif /* MSVCRT_WSTDLIB_DEFINED */

#ifndef MSVCRT_WSTRING_DEFINED
#define MSVCRT_WSTRING_DEFINED
MSVCRT(wchar_t)*_wcsdup(const MSVCRT(wchar_t)*);
int             _wcsicmp(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
int             _wcsicoll(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*_wcslwr(MSVCRT(wchar_t)*);
int             _wcsnicmp(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
MSVCRT(wchar_t)*_wcsnset(MSVCRT(wchar_t)*,MSVCRT(wchar_t),MSVCRT(size_t));
MSVCRT(wchar_t)*_wcsrev(MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*_wcsset(MSVCRT(wchar_t)*,MSVCRT(wchar_t));
MSVCRT(wchar_t)*_wcsupr(MSVCRT(wchar_t)*);

MSVCRT(wchar_t)*MSVCRT(wcscat)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcschr)(const MSVCRT(wchar_t)*,MSVCRT(wchar_t));
int             MSVCRT(wcscmp)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
int             MSVCRT(wcscoll)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcscpy)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(size_t)  MSVCRT(wcscspn)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(size_t)  MSVCRT(wcslen)(const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcsncat)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
int             MSVCRT(wcsncmp)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
MSVCRT(wchar_t)*MSVCRT(wcsncpy)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
MSVCRT(wchar_t)*MSVCRT(wcspbrk)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcsrchr)(const MSVCRT(wchar_t)*,MSVCRT(wchar_t) wcFor);
MSVCRT(size_t)  MSVCRT(wcsspn)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcsstr)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcstok)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(size_t)  MSVCRT(wcsxfrm)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
#endif /* MSVCRT_WSTRING_DEFINED */

#ifndef MSVCRT_WTIME_DEFINED
#define MSVCRT_WTIME_DEFINED
MSVCRT(wchar_t)*_wasctime(const struct MSVCRT(tm)*);
MSVCRT(size_t)  wcsftime(MSVCRT(wchar_t)*,MSVCRT(size_t),const MSVCRT(wchar_t)*,const struct MSVCRT(tm)*);
MSVCRT(wchar_t)*_wctime(const MSVCRT(time_t)*);
MSVCRT(wchar_t)*_wstrdate(MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*_wstrtime(MSVCRT(wchar_t)*);
#endif /* MSVCRT_WTIME_DEFINED */

MSVCRT(wchar_t) btowc(int);
MSVCRT(size_t)  mbrlen(const char *,MSVCRT(size_t),MSVCRT(mbstate_t)*);
MSVCRT(size_t)  mbrtowc(MSVCRT(wchar_t)*,const char*,MSVCRT(size_t),MSVCRT(mbstate_t)*);
MSVCRT(size_t)  mbsrtowcs(MSVCRT(wchar_t)*,const char**,MSVCRT(size_t),MSVCRT(mbstate_t)*);
MSVCRT(size_t)  wcrtomb(char*,MSVCRT(wchar_t),MSVCRT(mbstate_t)*);
MSVCRT(size_t)  wcsrtombs(char*,const MSVCRT(wchar_t)**,MSVCRT(size_t),MSVCRT(mbstate_t)*);
int             wctob(MSVCRT(wint_t));

#ifdef __cplusplus
}
#endif

#endif /* __WINE_WCHAR_H */
