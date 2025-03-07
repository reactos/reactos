/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_WCHAR
#define _INC_WCHAR

#include <corecrt.h>

#define __need___va_list
#include <stdarg.h>

#define __CORRECT_ISO_CPP_WCHAR_H_PROTO // For stl

#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4820)
#endif

#ifndef WCHAR_MIN
#define WCHAR_MIN 0x0000
#endif
#ifndef WCHAR_MAX
#define WCHAR_MAX 0xffff /* UINT16_MAX */
#endif

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
#endif

#ifndef _FILE_DEFINED
  struct _iobuf {
    char *_ptr;
    int _cnt;
    char *_base;
    int _flag;
    int _file;
    int _charbuf;
    int _bufsiz;
    char *_tmpfname;
  };
  typedef struct _iobuf FILE;
#define _FILE_DEFINED
#endif

#ifndef _STDIO_DEFINED
  _CRTIMP FILE *__cdecl __iob_func(void);
  _CRTDATA(extern FILE _iob[];)
#ifdef _M_CEE_PURE
#define _iob __iob_func()
#endif
#endif

#ifndef _STDSTREAM_DEFINED
#define _STDSTREAM_DEFINED
#define stdin (&_iob[0])
#define stdout (&_iob[1])
#define stderr (&_iob[2])
#endif /* !_STDSTREAM_DEFINED */

#ifndef _FSIZE_T_DEFINED
  typedef unsigned long _fsize_t;
#define _FSIZE_T_DEFINED
#endif

#ifndef _WFINDDATA_T_DEFINED
  struct _wfinddata_t {
    unsigned attrib;
    time_t time_create;
    time_t time_access;
    time_t time_write;
    _fsize_t size;
    wchar_t name[260];
  };

  struct _wfinddata32_t {
    unsigned attrib;
    __time32_t time_create;
    __time32_t time_access;
    __time32_t time_write;
    _fsize_t size;
    wchar_t name[260];
  };

#if _INTEGRAL_MAX_BITS >= 64
  struct _wfinddatai64_t {
    unsigned attrib;
    time_t time_create;
    time_t time_access;
    time_t time_write;
    __MINGW_EXTENSION __int64 size;
    wchar_t name[260];
  };

  struct _wfinddata32i64_t {
    unsigned attrib;
    __time32_t time_create;
    __time32_t time_access;
    __time32_t time_write;
    __MINGW_EXTENSION __int64 size;
    wchar_t name[260];
  };

  struct _wfinddata64i32_t {
    unsigned attrib;
    __time64_t time_create;
    __time64_t time_access;
    __time64_t time_write;
    _fsize_t size;
    wchar_t name[260];
  };

  struct _wfinddata64_t {
    unsigned attrib;
    __time64_t time_create;
    __time64_t time_access;
    __time64_t time_write;
    __MINGW_EXTENSION __int64 size;
    wchar_t name[260];
  };
#endif

#define _WFINDDATA_T_DEFINED
#endif /* !_WFINDDATA_T_DEFINED */

#ifndef _CRT_CTYPEDATA_DEFINED
# define _CRT_CTYPEDATA_DEFINED
# ifndef _CTYPE_DISABLE_MACROS
#  ifndef __PCTYPE_FUNC
#   ifdef _DLL
#    define __PCTYPE_FUNC __pctype_func()
#   else
#    define __PCTYPE_FUNC _pctype
#   endif
#  endif /* !__PCTYPE_FUNC */
  _CRTIMP const unsigned short * __cdecl __pctype_func(void);
#  ifndef _M_CEE_PURE
  _CRTDATA(extern unsigned short *_pctype);
#  else
#   define _pctype (__pctype_func())
#  endif /* !_M_CEE_PURE */
# endif /* !_CTYPE_DISABLE_MACROS */
#endif /* !_CRT_CTYPEDATA_DEFINED */

#ifndef _CRT_WCTYPEDATA_DEFINED
#define _CRT_WCTYPEDATA_DEFINED
# ifndef _CTYPE_DISABLE_MACROS
  _CRTDATA(extern unsigned short *_wctype);
  _CRTIMP const wctype_t * __cdecl __pwctype_func(void);
#  ifndef _M_CEE_PURE
  _CRTDATA(extern const wctype_t *_pwctype);
#  else
#   define _pwctype (__pwctype_func())
#  endif /* !_M_CEE_PURE */
# endif /* !_CTYPE_DISABLE_MACROS */
#endif /* !_CRT_WCTYPEDATA_DEFINED */

#define _UPPER 0x1
#define _LOWER 0x2
#define _DIGIT 0x4
#define _SPACE 0x8

#define _PUNCT 0x10
#define _CONTROL 0x20
#define _BLANK 0x40
#define _HEX 0x80

#define _LEADBYTE 0x8000
#define _ALPHA (0x0100|_UPPER|_LOWER)

#ifndef _WCTYPE_DEFINED
#define _WCTYPE_DEFINED
  _Check_return_ _CRTIMP int __cdecl iswalpha(_In_ wint_t _C);
  _Check_return_ _CRTIMP int __cdecl _iswalpha_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP int __cdecl iswupper(_In_ wint_t _C);
  _Check_return_ _CRTIMP int __cdecl _iswupper_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP int __cdecl iswlower(_In_ wint_t _C);
  _Check_return_ _CRTIMP int __cdecl _iswlower_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP int __cdecl iswdigit(_In_ wint_t _C);
  _Check_return_ _CRTIMP int __cdecl _iswdigit_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP int __cdecl iswxdigit(_In_ wint_t _C);
  _Check_return_ _CRTIMP int __cdecl _iswxdigit_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP int __cdecl iswspace(_In_ wint_t _C);
  _Check_return_ _CRTIMP int __cdecl _iswspace_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP int __cdecl iswpunct(_In_ wint_t _C);
  _Check_return_ _CRTIMP int __cdecl _iswpunct_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP int __cdecl iswalnum(_In_ wint_t _C);
  _Check_return_ _CRTIMP int __cdecl _iswalnum_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP int __cdecl iswprint(_In_ wint_t _C);
  _Check_return_ _CRTIMP int __cdecl _iswprint_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP int __cdecl iswgraph(_In_ wint_t _C);
  _Check_return_ _CRTIMP int __cdecl _iswgraph_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP int __cdecl iswcntrl(_In_ wint_t _C);
  _Check_return_ _CRTIMP int __cdecl _iswcntrl_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP int __cdecl iswascii(_In_ wint_t _C);
  _Check_return_ _CRTIMP int __cdecl isleadbyte(_In_ int _C);
  _Check_return_ _CRTIMP int __cdecl _isleadbyte_l(_In_ int _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP wint_t __cdecl towupper(_In_ wint_t _C);
  _Check_return_ _CRTIMP wint_t __cdecl _towupper_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP wint_t __cdecl towlower(_In_ wint_t _C);
  _Check_return_ _CRTIMP wint_t __cdecl _towlower_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP int __cdecl iswctype(_In_ wint_t _C, _In_ wctype_t _Type);
  _Check_return_ _CRTIMP int __cdecl _iswctype_l(_In_ wint_t _C, _In_ wctype_t _Type, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP int __cdecl __iswcsymf(_In_ wint_t _C);
  _Check_return_ _CRTIMP int __cdecl _iswcsymf_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _Check_return_ _CRTIMP int __cdecl __iswcsym(_In_ wint_t _C);
  _Check_return_ _CRTIMP int __cdecl _iswcsym_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
  _CRTIMP int __cdecl is_wctype(_In_ wint_t _C, _In_ wctype_t _Type);

#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || !defined (NO_OLDNAMES)
_CRTIMP int __cdecl iswblank(wint_t _C);
#endif
#endif

#ifndef _WDIRECT_DEFINED
#define _WDIRECT_DEFINED

  _Check_return_
  _Ret_opt_z_
  _CRTIMP
  wchar_t*
  __cdecl
  _wgetcwd(
    _Out_writes_opt_(_SizeInWords) wchar_t *_DstBuf,
    _In_ int _SizeInWords);

  _Check_return_
  _Ret_opt_z_
  _CRTIMP
  wchar_t*
  __cdecl
  _wgetdcwd(
    _In_ int _Drive,
    _Out_writes_opt_(_SizeInWords) wchar_t *_DstBuf,
    _In_ int _SizeInWords);

  _Check_return_
  _Ret_opt_z_
  wchar_t*
  __cdecl
  _wgetdcwd_nolock(
    _In_ int _Drive,
    _Out_writes_opt_(_SizeInWords) wchar_t *_DstBuf,
    _In_ int _SizeInWords);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wchdir(
    _In_z_ const wchar_t *_Path);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wmkdir(
    _In_z_ const wchar_t *_Path);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wrmdir(
    _In_z_ const wchar_t *_Path);

#endif /* _WDIRECT_DEFINED */

#ifndef _WIO_DEFINED
#define _WIO_DEFINED

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _waccess(
    _In_z_ const wchar_t *_Filename,
    _In_ int _AccessMode);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wchmod(
    _In_z_ const wchar_t *_Filename,
    _In_ int _Mode);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wcreat(
    _In_z_ const wchar_t *_Filename,
    _In_ int _PermissionMode);

  _Check_return_
  _CRTIMP
  intptr_t
  __cdecl
  _wfindfirst32(
    _In_z_ const wchar_t *_Filename,
    _Out_ struct _wfinddata32_t *_FindData);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wfindnext32(
    _In_ intptr_t _FindHandle,
    _Out_ struct _wfinddata32_t *_FindData);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wunlink(
    _In_z_ const wchar_t *_Filename);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wrename(
    _In_z_ const wchar_t *_OldFilename,
    _In_z_ const wchar_t *_NewFilename);

  _CRTIMP
  wchar_t*
  __cdecl
  _wmktemp(
    _Inout_z_ wchar_t *_TemplateName);

#if _INTEGRAL_MAX_BITS >= 64

  _Check_return_
  _CRTIMP
  intptr_t
  __cdecl
  _wfindfirst32i64(
    _In_z_ const wchar_t *_Filename,
    _Out_ struct _wfinddata32i64_t *_FindData);

  _Check_return_
  _CRTIMP
  intptr_t
  __cdecl
  _wfindfirst64i32(
    _In_z_ const wchar_t *_Filename,
    _Out_ struct _wfinddata64i32_t *_FindData);

  _Check_return_
  _CRTIMP
  intptr_t
  __cdecl
  _wfindfirst64(
    _In_z_ const wchar_t *_Filename,
    _Out_ struct _wfinddata64_t *_FindData);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wfindnext32i64(
    _In_ intptr_t _FindHandle,
    _Out_ struct _wfinddata32i64_t *_FindData);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wfindnext64i32(
    _In_ intptr_t _FindHandle,
    _Out_ struct _wfinddata64i32_t *_FindData);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wfindnext64(
    _In_ intptr_t _FindHandle,
    _Out_ struct _wfinddata64_t *_FindData);

#endif /* _INTEGRAL_MAX_BITS >= 64 */

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wsopen_s(
    _Out_ int *_FileHandle,
    _In_z_ const wchar_t *_Filename,
    _In_ int _OpenFlag,
    _In_ int _ShareFlag,
    _In_ int _PermissionFlag);

#if !defined(__cplusplus) || !(defined(_X86_) && !defined(__x86_64))
  _CRTIMP int __cdecl _wopen(const wchar_t *_Filename,int _OpenFlag,...);
  _CRTIMP int __cdecl _wsopen(const wchar_t *_Filename,int _OpenFlag,int _ShareFlag,...);
#else
  extern "C++" _CRTIMP int __cdecl _wopen(const wchar_t *_Filename,int _OpenFlag,int _PermissionMode = 0);
  extern "C++" _CRTIMP int __cdecl _wsopen(const wchar_t *_Filename,int _OpenFlag,int _ShareFlag,int _PermissionMode = 0);
#endif

#endif /* !_WIO_DEFINED */

#ifndef _WLOCALE_DEFINED
#define _WLOCALE_DEFINED
  _Check_return_opt_
  _CRTIMP
  wchar_t*
  __cdecl
  _wsetlocale(
    _In_ int _Category,
    _In_opt_z_ const wchar_t *_Locale);
#endif

#ifndef _WPROCESS_DEFINED
#define _WPROCESS_DEFINED

  _CRTIMP
  intptr_t
  __cdecl
  _wexecl(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wexecle(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wexeclp(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wexeclpe(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wexecv(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList);

  _CRTIMP
  intptr_t
  __cdecl
  _wexecve(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList,
    _In_opt_z_ const wchar_t *const *_Env);

  _CRTIMP
  intptr_t
  __cdecl
  _wexecvp(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList);

  _CRTIMP
  intptr_t
  __cdecl
  _wexecvpe(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList,
    _In_opt_z_ const wchar_t *const *_Env);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnl(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnle(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnlp(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnlpe(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnv(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnve(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList,
    _In_opt_z_ const wchar_t *const *_Env);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnvp(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnvpe(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList,
    _In_opt_z_ const wchar_t *const *_Env);

#ifndef _CRT_WSYSTEM_DEFINED
#define _CRT_WSYSTEM_DEFINED
  _CRTIMP
  int
  __cdecl
  _wsystem(
    _In_opt_z_ const wchar_t *_Command);
#endif /* !_CRT_WSYSTEM_DEFINED */

#endif /* !_WPROCESS_DEFINED */

#ifndef _WCTYPE_INLINE_DEFINED
#undef _CRT_WCTYPE_NOINLINE
#if !defined(__cplusplus) || defined(_CRT_WCTYPE_NOINLINE)
#define iswalpha(_c) (iswctype(_c,_ALPHA))
#define iswupper(_c) (iswctype(_c,_UPPER))
#define iswlower(_c) (iswctype(_c,_LOWER))
#define iswdigit(_c) (iswctype(_c,_DIGIT))
#define iswxdigit(_c) (iswctype(_c,_HEX))
#define iswspace(_c) (iswctype(_c,_SPACE))
#define iswpunct(_c) (iswctype(_c,_PUNCT))
#define iswalnum(_c) (iswctype(_c,_ALPHA|_DIGIT))
#define iswprint(_c) (iswctype(_c,_BLANK|_PUNCT|_ALPHA|_DIGIT))
#define iswgraph(_c) (iswctype(_c,_PUNCT|_ALPHA|_DIGIT))
#define iswcntrl(_c) (iswctype(_c,_CONTROL))
#define iswascii(_c) ((unsigned)(_c) < 0x80)

#define _iswalpha_l(_c,_p) (_iswctype_l(_c,_ALPHA,_p))
#define _iswupper_l(_c,_p) (_iswctype_l(_c,_UPPER,_p))
#define _iswlower_l(_c,_p) (_iswctype_l(_c,_LOWER,_p))
#define _iswdigit_l(_c,_p) (_iswctype_l(_c,_DIGIT,_p))
#define _iswxdigit_l(_c,_p) (_iswctype_l(_c,_HEX,_p))
#define _iswspace_l(_c,_p) (_iswctype_l(_c,_SPACE,_p))
#define _iswpunct_l(_c,_p) (_iswctype_l(_c,_PUNCT,_p))
#define _iswalnum_l(_c,_p) (_iswctype_l(_c,_ALPHA|_DIGIT,_p))
#define _iswprint_l(_c,_p) (_iswctype_l(_c,_BLANK|_PUNCT|_ALPHA|_DIGIT,_p))
#define _iswgraph_l(_c,_p) (_iswctype_l(_c,_PUNCT|_ALPHA|_DIGIT,_p))
#define _iswcntrl_l(_c,_p) (_iswctype_l(_c,_CONTROL,_p))
#ifndef _CTYPE_DISABLE_MACROS
#define isleadbyte(_c) (__PCTYPE_FUNC[(unsigned char)(_c)] & _LEADBYTE)
#endif
#endif
#define _WCTYPE_INLINE_DEFINED
#endif

#if !defined(_POSIX_) || defined(__GNUC__)
#ifndef _INO_T_DEFINED
#define _INO_T_DEFINED
  typedef unsigned short _ino_t;
#ifndef NO_OLDNAMES
  typedef unsigned short ino_t;
#endif
#endif

#ifndef _DEV_T_DEFINED
#define _DEV_T_DEFINED
  typedef unsigned int _dev_t;
#ifndef NO_OLDNAMES
  typedef unsigned int dev_t;
#endif
#endif

#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
  typedef long _off_t;
#ifndef NO_OLDNAMES
  typedef long off_t;
#endif
#endif

#ifndef _OFF64_T_DEFINED
#define _OFF64_T_DEFINED
  __MINGW_EXTENSION typedef long long _off64_t;
#ifndef NO_OLDNAMES
  __MINGW_EXTENSION typedef long long off64_t;
#endif
#endif

#ifndef _STAT_DEFINED
#define _STAT_DEFINED

  struct _stat32 {
    _dev_t st_dev;
    _ino_t st_ino;
    unsigned short st_mode;
    short st_nlink;
    short st_uid;
    short st_gid;
    _dev_t st_rdev;
    _off_t st_size;
    __time32_t st_atime;
    __time32_t st_mtime;
    __time32_t st_ctime;
  };

  struct _stat {
    _dev_t st_dev;
    _ino_t st_ino;
    unsigned short st_mode;
    short st_nlink;
    short st_uid;
    short st_gid;
    _dev_t st_rdev;
    _off_t st_size;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
  };

#ifndef	NO_OLDNAMES
  struct stat {
    _dev_t st_dev;
    _ino_t st_ino;
    unsigned short st_mode;
    short st_nlink;
    short st_uid;
    short st_gid;
    _dev_t st_rdev;
    _off_t st_size;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
  };
#endif

#if _INTEGRAL_MAX_BITS >= 64

  struct _stat32i64 {
    _dev_t st_dev;
    _ino_t st_ino;
    unsigned short st_mode;
    short st_nlink;
    short st_uid;
    short st_gid;
    _dev_t st_rdev;
    __MINGW_EXTENSION __int64 st_size;
    __time32_t st_atime;
    __time32_t st_mtime;
    __time32_t st_ctime;
  };

  struct _stat64i32 {
    _dev_t st_dev;
    _ino_t st_ino;
    unsigned short st_mode;
    short st_nlink;
    short st_uid;
    short st_gid;
    _dev_t st_rdev;
    _off_t st_size;
    __time64_t st_atime;
    __time64_t st_mtime;
    __time64_t st_ctime;
  };

  struct _stat64 {
    _dev_t st_dev;
    _ino_t st_ino;
    unsigned short st_mode;
    short st_nlink;
    short st_uid;
    short st_gid;
    _dev_t st_rdev;
    __MINGW_EXTENSION __int64 st_size;
    __time64_t st_atime;
    __time64_t st_mtime;
    __time64_t st_ctime;
  };
#endif

#define __stat64 _stat64

#endif

#ifndef _WSTAT_DEFINED
#define _WSTAT_DEFINED

  _CRTIMP
  int
  __cdecl
  _wstat(
    _In_z_ const wchar_t *_Name,
    _Out_ struct _stat *_Stat);

  _CRTIMP
  int
  __cdecl
  _wstat32(
    _In_z_ const wchar_t *_Name,
    _Out_ struct _stat32 *_Stat);

#if _INTEGRAL_MAX_BITS >= 64

  _CRTIMP
  int
  __cdecl
  _wstat32i64(
    _In_z_ const wchar_t *_Name,
    _Out_ struct _stat32i64 *_Stat);

  _CRTIMP
  int
  __cdecl
  _wstat64i32(
    _In_z_ const wchar_t *_Name,
    _Out_ struct _stat64i32 *_Stat);

  _CRTIMP
  int
  __cdecl
  _wstat64(
    _In_z_ const wchar_t *_Name,
    _Out_ struct _stat64 *_Stat);

#endif /* _INTEGRAL_MAX_BITS >= 64 */

#endif /* _WSTAT_DEFINED */

#endif /* !defined(_POSIX_) || defined(__GNUC__) */

#ifndef _WCONIO_DEFINED
#define _WCONIO_DEFINED

  _CRTIMP
  wchar_t*
  _cgetws(
    _Inout_z_ wchar_t *_Buffer);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  _getwch(void);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  _getwche(void);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  _putwch(
    wchar_t _WCh);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  _ungetwch(
    wint_t _WCh);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cputws(
    _In_z_ const wchar_t *_String);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cwprintf(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cwscanf(
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cwscanf_l(
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vcwprintf(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cwprintf_p(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vcwprintf_p(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _CRTIMP
  int
  __cdecl
  _cwprintf_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _CRTIMP
  int
  __cdecl
  _vcwprintf_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _CRTIMP
  int
  __cdecl
  _cwprintf_p_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _CRTIMP
  int
  __cdecl
  _vcwprintf_p_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  wint_t
  __cdecl
  _putwch_nolock(
    wchar_t _WCh);

  _Check_return_
  wint_t
  __cdecl
  _getwch_nolock(void);

  _Check_return_
  wint_t
  __cdecl
  _getwche_nolock(void);

  _Check_return_opt_
  wint_t
  __cdecl
  _ungetwch_nolock(
    wint_t _WCh);

#endif /* _WCONIO_DEFINED */

#ifndef _WSTDIO_DEFINED
#define _WSTDIO_DEFINED

#ifdef _POSIX_
  _CRTIMP FILE *__cdecl _wfsopen(const wchar_t *_Filename,const wchar_t *_Mode);
#else
  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  _wfsopen(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_Mode,
    _In_ int _ShFlag);
#endif

  _Check_return_opt_
  _CRTIMP_ALT
  wint_t
  __cdecl
  fgetwc(
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  _fgetwchar(void);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  fputwc(
    _In_ wchar_t _Ch,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  _fputwchar(
    _In_ wchar_t _Ch);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  getwc(
    _Inout_ FILE *_File);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  getwchar(void);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  putwc(
    _In_ wchar_t _Ch,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  putwchar(
    _In_ wchar_t _Ch);

  _Check_return_opt_
  _CRTIMP_ALT
  wint_t
  __cdecl
  ungetwc(
    _In_ wint_t _Ch,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  wchar_t*
  __cdecl
  fgetws(
    _Out_writes_z_(_SizeInWords) wchar_t *_Dst,
    _In_ int _SizeInWords,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fputws(
    _In_z_ const wchar_t *_Str,
    _Inout_ FILE *_File);

  _CRTIMP
  wchar_t*
  __cdecl
  _getws(
    _Pre_notnull_ _Post_z_ wchar_t *_String);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _putws(
    _In_z_ const wchar_t *_Str);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fwprintf(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  wprintf(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _scwprintf(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  vfwprintf(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  vwprintf(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

#if defined __cplusplus || defined _CRT_NON_CONFORMING_SWPRINTFS
  _CRTIMP
  int
  __cdecl
  swprintf(
    _Out_ wchar_t*,
    const wchar_t*,
    ...);

  _CRTIMP
  int
  __cdecl
  vswprintf(
    _Out_ wchar_t*,
    const wchar_t*,
    va_list);
#endif

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _swprintf_c(
    _Out_writes_z_(_SizeInWords) wchar_t *_DstBuf,
    _In_ size_t _SizeInWords,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vswprintf_c(
    _Out_writes_z_(_SizeInWords) wchar_t *_DstBuf,
    _In_ size_t _SizeInWords,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _CRTIMP int __cdecl _snwprintf(wchar_t *_Dest,size_t _Count,const wchar_t *_Format,...);
  _CRTIMP int __cdecl _vsnwprintf(wchar_t *_Dest,size_t _Count,const wchar_t *_Format,va_list _Args);

#ifndef __NO_ISOCEXT  /* externs in libmingwex.a */
  _CRTIMP int __cdecl snwprintf (wchar_t *s, size_t n, const wchar_t * format, ...);
  __CRT_INLINE int __cdecl vsnwprintf (wchar_t *s, size_t n, const wchar_t *format, va_list arg) { return _vsnwprintf(s,n,format,arg); }
  _CRTIMP int __cdecl vwscanf (const wchar_t *, va_list);
  _CRTIMP int __cdecl vfwscanf (FILE *,const wchar_t *,va_list);
  _CRTIMP int __cdecl vswscanf (const wchar_t *,const wchar_t *,va_list);
#endif

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fwprintf_p(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _wprintf_p(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vfwprintf_p(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vwprintf_p(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _swprintf_p(
    _Out_writes_z_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vswprintf_p(
    _Out_writes_z_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _scwprintf_p(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _vscwprintf_p(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _wprintf_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _wprintf_p_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vwprintf_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vwprintf_p_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fwprintf_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fwprintf_p_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vfwprintf_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vfwprintf_p_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _swprintf_c_l(
    _Out_writes_z_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _swprintf_p_l(
    _Out_writes_z_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vswprintf_c_l(
    _Out_writes_z_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vswprintf_p_l(
    _Out_writes_z_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _scwprintf_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _scwprintf_p_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _vscwprintf_p_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snwprintf_l(
    _Out_writes_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vsnwprintf_l(
    _Out_writes_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _CRTIMP
  int
  __cdecl
  _swprintf(
    _Pre_notnull_ _Post_z_ wchar_t *_Dest,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _CRTIMP
  int
  __cdecl
  _vswprintf(
    _Pre_notnull_ _Post_z_ wchar_t *_Dest,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _Args);


  _CRTIMP int __cdecl __swprintf_l(wchar_t *_Dest,const wchar_t *_Format,_locale_t _Plocinfo,...);
  _CRTIMP int __cdecl __vswprintf_l(wchar_t *_Dest,const wchar_t *_Format,_locale_t _Plocinfo,va_list _Args);

#ifndef _CRT_NON_CONFORMING_SWPRINTFS
  _Check_return_opt_
  static inline
  int
  __cdecl
  swprintf(
      _Out_writes_z_(_SizeInWords) wchar_t* _DstBuf,
      _In_ size_t _SizeInWords,
      _In_z_ _Printf_format_string_ const wchar_t* _Format,
      ...)
  {
      int ret;
      va_list args;

      va_start(args, _Format);
      ret = _vsnwprintf(_DstBuf, _SizeInWords, _Format, args);
      va_end(args);
      return ret;
  }

  _Check_return_opt_
  static inline
  int
  __cdecl
  vswprintf(
      _Out_writes_z_(_SizeInWords) wchar_t* _DstBuf,
      _In_ size_t _SizeInWords,
      _In_z_ _Printf_format_string_ const wchar_t* _Format,
      va_list _ArgList)
  {
      return _vsnwprintf(_DstBuf, _SizeInWords, _Format, _ArgList);
  }
#endif

  _Check_return_
  _CRTIMP
  wchar_t*
  __cdecl
  _wtempnam(
    _In_opt_z_ const wchar_t *_Directory,
    _In_opt_z_ const wchar_t *_FilePrefix);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _vscwprintf(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _vscwprintf_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_
  int
  __cdecl
  fwscanf(
    _Inout_ FILE *_File,
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fwscanf_l(
    _Inout_ FILE *_File,
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_
  int
  __cdecl
  swscanf(
    _In_z_ const wchar_t *_Src,
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _swscanf_l(
    _In_z_ const wchar_t *_Src,
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snwscanf(
    _In_reads_(_MaxCount) _Pre_z_ const wchar_t *_Src,
    _In_ size_t _MaxCount,
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snwscanf_l(
    _In_reads_(_MaxCount) _Pre_z_ const wchar_t *_Src,
    _In_ size_t _MaxCount,
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_
  int
  __cdecl
  wscanf(
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _wscanf_l(
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  _wfdopen(
    _In_ int _FileHandle,
    _In_z_ const wchar_t *_Mode);

  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  _wfopen(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_Mode);

  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  _wfreopen(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_Mode,
    _Inout_ FILE *_OldFile);

#ifndef _CRT_WPERROR_DEFINED
#define _CRT_WPERROR_DEFINED
  _CRTIMP
  void
  __cdecl
  _wperror(
    _In_opt_z_ const wchar_t *_ErrMsg);
#endif

  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  _wpopen(
    _In_z_ const wchar_t *_Command,
    _In_z_ const wchar_t *_Mode);

#if !defined(NO_OLDNAMES) && !defined(wpopen)
#define wpopen _wpopen
#endif

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wremove(
    _In_z_ const wchar_t *_Filename);

  _CRTIMP
  wchar_t*
  __cdecl
  _wtmpnam(
    _Pre_maybenull_ _Post_z_ wchar_t *_Buffer);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  _fgetwc_nolock(
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  _fputwc_nolock(
    _In_ wchar_t _Ch,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  _ungetwc_nolock(
    _In_ wint_t _Ch,
    _Inout_ FILE *_File);

#undef _CRT_GETPUTWCHAR_NOINLINE

#if !defined(__cplusplus) || defined(_CRT_GETPUTWCHAR_NOINLINE)
#define getwchar() fgetwc(stdin)
#define putwchar(_c) fputwc((_c),stdout)
#else
  _Check_return_ __CRT_INLINE wint_t __cdecl getwchar() {return (fgetwc(stdin)); }
  _Check_return_opt_ __CRT_INLINE wint_t __cdecl putwchar(_In_ wchar_t _C) {return (fputwc(_C,stdout)); }
#endif

#define getwc(_stm) fgetwc(_stm)
#define putwc(_c,_stm) fputwc(_c,_stm)
#define _putwc_nolock(_c,_stm) _fputwc_nolock(_c,_stm)
#define _getwc_nolock(_c) _fgetwc_nolock(_c)

#endif /* _WSTDIO_DEFINED */

#ifndef _WSTDLIB_DEFINED
#define _WSTDLIB_DEFINED

  _CRTIMP
  wchar_t*
  __cdecl
  _itow(
    _In_ int _Value,
    _Pre_notnull_ _Post_z_ wchar_t *_Dest,
    _In_ int _Radix);

  _CRTIMP
  wchar_t*
  __cdecl
  _ltow(
    _In_ long _Value,
    _Pre_notnull_ _Post_z_ wchar_t *_Dest,
    _In_ int _Radix);

  _CRTIMP
  wchar_t*
  __cdecl
  _ultow(
    _In_ unsigned long _Value,
    _Pre_notnull_ _Post_z_ wchar_t *_Dest,
    _In_ int _Radix);

  _Check_return_
  double
  __cdecl
  wcstod(
    _In_z_ const wchar_t *_Str,
    _Out_opt_ _Deref_post_z_ wchar_t **_EndPtr);

  _Check_return_
  _CRTIMP
  double
  __cdecl
  _wcstod_l(
    _In_z_ const wchar_t *_Str,
    _Out_opt_ _Deref_post_z_ wchar_t **_EndPtr,
    _In_opt_ _locale_t _Locale);

  float __cdecl wcstof( const wchar_t *nptr, wchar_t **endptr);

#if !defined __NO_ISOCEXT /* in libmingwex.a */
  float __cdecl wcstof (const wchar_t * __restrict__, wchar_t ** __restrict__);
  long double __cdecl wcstold (const wchar_t * __restrict__, wchar_t ** __restrict__);
#endif /* __NO_ISOCEXT */

  _Check_return_
  long
  __cdecl
  wcstol(
    _In_z_ const wchar_t *_Str,
    _Out_opt_ _Deref_post_z_ wchar_t **_EndPtr,
    _In_ int _Radix);

  _Check_return_
  _CRTIMP
  long
  __cdecl
  _wcstol_l(
    _In_z_ const wchar_t *_Str,
    _Out_opt_ _Deref_post_z_ wchar_t **_EndPtr,
    _In_ int _Radix,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  unsigned long
  __cdecl
  wcstoul(
    _In_z_ const wchar_t *_Str,
    _Out_opt_ _Deref_post_z_ wchar_t **_EndPtr,
    _In_ int _Radix);

  _Check_return_
  _CRTIMP
  unsigned long
  __cdecl
  _wcstoul_l(
    _In_z_ const wchar_t *_Str,
    _Out_opt_ _Deref_post_z_ wchar_t **_EndPtr,
    _In_ int _Radix,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  wchar_t*
  __cdecl
  _wgetenv(
    _In_z_ const wchar_t *_VarName);

#ifndef _CRT_WSYSTEM_DEFINED
#define _CRT_WSYSTEM_DEFINED
  _CRTIMP
  int
  __cdecl
  _wsystem(
    _In_opt_z_ const wchar_t *_Command);
#endif

  _Check_return_
  _CRTIMP
  double
  __cdecl
  _wtof(
    _In_z_ const wchar_t *_Str);

  _Check_return_
  _CRTIMP
  double
  __cdecl
  _wtof_l(
    _In_z_ const wchar_t *_Str,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wtoi(
    _In_z_ const wchar_t *_Str);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wtoi_l(
    _In_z_ const wchar_t *_Str,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  long
  __cdecl
  _wtol(
    _In_z_ const wchar_t *_Str);

  _Check_return_
  _CRTIMP
  long
  __cdecl
  _wtol_l(
    _In_z_ const wchar_t *_Str,
    _In_opt_ _locale_t _Locale);

#if _INTEGRAL_MAX_BITS >= 64

  __MINGW_EXTENSION
  _CRTIMP
  wchar_t*
  __cdecl
  _i64tow(
    _In_ __int64 _Val,
    _Pre_notnull_ _Post_z_ wchar_t *_DstBuf,
    _In_ int _Radix);

  __MINGW_EXTENSION
  _CRTIMP
  wchar_t*
  __cdecl
  _ui64tow(
    _In_ unsigned __int64 _Val,
    _Pre_notnull_ _Post_z_ wchar_t *_DstBuf,
    _In_ int _Radix);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  __int64
  __cdecl
  _wtoi64(
    _In_z_ const wchar_t *_Str);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  __int64
  __cdecl
  _wtoi64_l(
    _In_z_ const wchar_t *_Str,
    _In_opt_ _locale_t _Locale);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  __int64
  __cdecl
  _wcstoi64(
    _In_z_ const wchar_t *_Str,
    _Out_opt_ _Deref_post_z_ wchar_t **_EndPtr,
    _In_ int _Radix);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  __int64
  __cdecl
  _wcstoi64_l(
    _In_z_ const wchar_t *_Str,
    _Out_opt_ _Deref_post_z_ wchar_t **_EndPtr,
    _In_ int _Radix,
    _In_opt_ _locale_t _Locale);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  unsigned __int64
  __cdecl
  _wcstoui64(
    _In_z_ const wchar_t *_Str,
    _Out_opt_ _Deref_post_z_ wchar_t **_EndPtr,
    _In_ int _Radix);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  unsigned __int64
  __cdecl
  _wcstoui64_l(
    _In_z_ const wchar_t *_Str,
    _Out_opt_ _Deref_post_z_ wchar_t **_EndPtr,
    _In_ int _Radix,
    _In_opt_ _locale_t _Locale);

#endif /* _INTEGRAL_MAX_BITS >= 64 */

#endif /* _WSTDLIB_DEFINED */

#ifndef _POSIX_

#ifndef _WSTDLIBP_DEFINED
#define _WSTDLIBP_DEFINED

  _Check_return_
  _CRTIMP
  wchar_t*
  __cdecl
  _wfullpath(
    _Out_writes_opt_z_(_SizeInWords) wchar_t *_FullPath,
    _In_z_ const wchar_t *_Path,
    _In_ size_t _SizeInWords);

  _CRTIMP
  void
  __cdecl
  _wmakepath(
    _Pre_notnull_ _Post_z_ wchar_t *_ResultPath,
    _In_opt_z_ const wchar_t *_Drive,
    _In_opt_z_ const wchar_t *_Dir,
    _In_opt_z_ const wchar_t *_Filename,
    _In_opt_z_ const wchar_t *_Ext);

#ifndef _CRT_WPERROR_DEFINED
#define _CRT_WPERROR_DEFINED
  _CRTIMP
  void
  __cdecl
  _wperror(
    _In_opt_z_ const wchar_t *_ErrMsg);
#endif

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wputenv(
    _In_z_ const wchar_t *_EnvString);

  _CRTIMP
  void
  __cdecl
  _wsearchenv(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_EnvVar,
    _Pre_notnull_ _Post_z_ wchar_t *_ResultPath);

  _CRTIMP
  void
  __cdecl
  _wsplitpath(
    _In_z_ const wchar_t *_FullPath,
    _Pre_maybenull_ _Post_z_ wchar_t *_Drive,
    _Pre_maybenull_ _Post_z_ wchar_t *_Dir,
    _Pre_maybenull_ _Post_z_ wchar_t *_Filename,
    _Pre_maybenull_ _Post_z_ wchar_t *_Ext);

#endif /* _WSTDLIBP_DEFINED */

#endif /* _POSIX_ */

#ifndef _WSTRING_DEFINED
#define _WSTRING_DEFINED

  _Check_return_
  _CRTIMP
  wchar_t*
  __cdecl
  _wcsdup(
    _In_z_ const wchar_t *_Str);

  wchar_t*
  __cdecl
  wcscat(
    _Inout_updates_z_(_String_length_(_Dest) + _String_length_(_Source) + 1) wchar_t *_Dest,
    _In_z_ const wchar_t *_Source);

  _CONST_RETURN
  wchar_t*
  __cdecl
  wcschr(
    _In_z_ const wchar_t *_Str,
    wchar_t _Ch);

#ifdef __cplusplus
    extern "C++"
    _Check_return_
    _When_(return != NULL, _Ret_range_(_String, _String + _String_length_(_String) - 1))
    inline wchar_t* __cdecl wcschr(_In_z_ wchar_t *_String, wchar_t _C)
    {
        return const_cast<wchar_t*>(wcschr(static_cast<const wchar_t*>(_String), _C));
    }
#endif // __cplusplus

  _Check_return_
  int
  __cdecl
  wcscmp(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2);

  wchar_t*
  __cdecl
  wcscpy(
    _Out_writes_z_(_String_length_(_Source) + 1) wchar_t *_Dest,
    _In_z_ const wchar_t *_Source);

  _Check_return_
  size_t
  __cdecl
  wcscspn(
    _In_z_ const wchar_t *_Str,
    _In_z_ const wchar_t *_Control);

  _CRTIMP
  size_t
  __cdecl
  wcslen(
    _In_z_ const wchar_t *_Str);

  _When_(_MaxCount > _String_length_(_Src),
         _Post_satisfies_(return == _String_length_(_Src)))
  _When_(_MaxCount <= _String_length_(_Src),
         _Post_satisfies_(return == _MaxCount))
  size_t
  __cdecl
  wcsnlen(
    _In_reads_or_z_(_MaxCount) const wchar_t *_Src,
    _In_ size_t _MaxCount);

  wchar_t*
  __cdecl
  wcsncat(
    wchar_t *_Dest,
    const wchar_t *_Source,
    size_t _Count);

  _Check_return_
  int
  __cdecl
  wcsncmp(
    _In_reads_or_z_(_MaxCount) const wchar_t *_Str1,
    _In_reads_or_z_(_MaxCount) const wchar_t *_Str2,
    _In_ size_t _MaxCount);

  wchar_t*
  __cdecl
  wcsncpy(
    wchar_t *_Dest,
    const wchar_t *_Source,
    size_t _Count);

  _Check_return_
  _CONST_RETURN
  wchar_t*
  __cdecl
  wcspbrk(
    _In_z_ const wchar_t *_Str,
    _In_z_ const wchar_t *_Control);

#ifdef __cplusplus
    extern "C++"
    _Check_return_
    inline wchar_t* __cdecl wcspbrk(_In_z_ wchar_t *_Str, _In_z_ const wchar_t *_Control)
    {
        return const_cast<wchar_t*>(wcspbrk(static_cast<const wchar_t*>(_Str), _Control));
    }
#endif // __cplusplus

  _Check_return_
  _CONST_RETURN
  wchar_t*
  __cdecl
  wcsrchr(
    _In_z_ const wchar_t *_Str,
    _In_ wchar_t _Ch);

#ifdef __cplusplus
    extern "C++"
    _Check_return_
    inline wchar_t* __cdecl wcsrchr(_In_z_ wchar_t *_Str, _In_ wchar_t _Ch)
    {
        return const_cast<wchar_t*>(wcsrchr(static_cast<const wchar_t*>(_Str), _Ch));
    }
#endif // __cplusplus

  _Check_return_
  size_t
  __cdecl
  wcsspn(
    _In_z_ const wchar_t *_Str,
    _In_z_ const wchar_t *_Control);

  _CONST_RETURN
  wchar_t*
  __cdecl
  wcsstr(
    _In_z_ const wchar_t *_Str,
    _In_z_ const wchar_t *_SubStr);

#ifdef __cplusplus
    extern "C++"
    _Check_return_ _Ret_maybenull_
    _When_(return != NULL, _Ret_range_(_String, _String + _String_length_(_String) - 1))
    inline wchar_t* __cdecl wcsstr(_In_z_ wchar_t *_String, _In_z_ const wchar_t *_SubStr)
    {
        return const_cast<wchar_t*>(wcsstr(static_cast<const wchar_t*>(_String), _SubStr));
    }
#endif // __cplusplus

  _Check_return_
  wchar_t*
  __cdecl
  wcstok(
    _Inout_opt_z_ wchar_t *_Str,
    _In_z_ const wchar_t *_Delim);

  _Check_return_
  _CRTIMP
  wchar_t*
  __cdecl
  _wcserror(
    _In_ int _ErrNum);

  _Check_return_
  _CRTIMP
  wchar_t*
  __cdecl
  __wcserror(
    _In_opt_z_ const wchar_t *_Str);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wcsicmp(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wcsicmp_l(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wcsnicmp(
    _In_reads_or_z_(_MaxCount) const wchar_t *_Str1,
    _In_reads_or_z_(_MaxCount) const wchar_t *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wcsnicmp_l(
    _In_reads_or_z_(_MaxCount) const wchar_t *_Str1,
    _In_reads_or_z_(_MaxCount) const wchar_t *_Str2,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  wchar_t*
  __cdecl
  _wcsnset(
    wchar_t *_Str,
    wchar_t _Val,
    size_t _MaxCount);

  _CRTIMP
  wchar_t*
  __cdecl
  _wcsrev(
    _Inout_z_ wchar_t *_Str);

  _CRTIMP
  wchar_t*
  __cdecl
  _wcsset(
    wchar_t *_Str,
    wchar_t _Val);

  _CRTIMP
  wchar_t*
  __cdecl
  _wcslwr(
    _Inout_z_ wchar_t *_String);

  _CRTIMP
  wchar_t*
  _wcslwr_l(
    wchar_t *_String,
    _locale_t _Locale);

  _CRTIMP
  wchar_t*
  __cdecl
  _wcsupr(
    _Inout_z_ wchar_t *_String);

  _CRTIMP
  wchar_t*
  _wcsupr_l(
    wchar_t *_String,
    _locale_t _Locale);

  _Check_return_opt_
  size_t
  __cdecl
  wcsxfrm(
    _Out_writes_opt_(_MaxCount) _Post_maybez_ wchar_t *_Dst,
    _In_z_ const wchar_t *_Src,
    _In_ size_t _MaxCount);

  _Check_return_opt_
  _CRTIMP
  size_t
  __cdecl
  _wcsxfrm_l(
    _Out_writes_opt_(_MaxCount) _Post_maybez_ wchar_t *_Dst,
    _In_z_ const wchar_t *_Src,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  int
  __cdecl
  wcscoll(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wcscoll_l(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wcsicoll(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wcsicoll_l(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wcsncoll(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wcsncoll_l(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wcsnicoll(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wcsnicoll_l(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

#ifndef NO_OLDNAMES

  _Check_return_
  _CRTIMP
  wchar_t*
  __cdecl
  wcsdup(
    _In_z_ const wchar_t *_Str);

#define wcswcs wcsstr

  _Check_return_
  _CRTIMP
  int
  __cdecl
  wcsicmp(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  wcsnicmp(
    _In_reads_or_z_(_MaxCount) const wchar_t *_Str1,
    _In_reads_or_z_(_MaxCount) const wchar_t *_Str2,
    _In_ size_t _MaxCount);

  _CRTIMP
  wchar_t*
  __cdecl
  wcsnset(
    _Inout_updates_z_(_MaxCount) wchar_t *_Str,
    _In_ wchar_t _Val,
    _In_ size_t _MaxCount);

  _CRTIMP
  wchar_t*
  __cdecl
  wcsrev(
    _Inout_z_ wchar_t *_Str);

  _CRTIMP
  wchar_t*
  __cdecl
  wcsset(
    _Inout_z_ wchar_t *_Str,
    wchar_t _Val);

  _CRTIMP
  wchar_t*
  __cdecl
  wcslwr(
    _Inout_z_ wchar_t *_Str);

  _CRTIMP
  wchar_t*
  __cdecl
  wcsupr(
    _Inout_z_ wchar_t *_Str);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  wcsicoll(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2);

#endif /* NO_OLDNAMES */

#endif /* _WSTRING_DEFINED */

#ifndef _TM_DEFINED
#define _TM_DEFINED
  struct tm {
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
#endif

#ifndef _WTIME_DEFINED
#define _WTIME_DEFINED

  _CRTIMP
  wchar_t*
  __cdecl
  _wasctime(
    _In_ const struct tm *_Tm);

  _CRTIMP
  wchar_t*
  __cdecl
  _wctime32(
    _In_ const __time32_t *_Time);

  _Success_(return > 0)
  size_t
  __cdecl
  wcsftime(
    _Out_writes_z_(_SizeInWords) wchar_t *_Buf,
    _In_ size_t _SizeInWords,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_ const struct tm *_Tm);

  _Success_(return > 0)
  _CRTIMP
  size_t
  __cdecl
  _wcsftime_l(
    _Out_writes_z_(_SizeInWords) wchar_t *_Buf,
    _In_ size_t _SizeInWords,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_ const struct tm *_Tm,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  wchar_t*
  __cdecl
  _wstrdate(
    _Out_writes_z_(9) wchar_t *_Buffer);

  _CRTIMP
  wchar_t*
  __cdecl
  _wstrtime(
    _Out_writes_z_(9) wchar_t *_Buffer);

  _CRTIMP
  errno_t
  __cdecl
  _wasctime_s(
    _Out_writes_(_SizeInWords) _Post_readable_size_(26) wchar_t *_Buf,
    _In_ size_t _SizeInWords,
    _In_ const struct tm *_Tm);

  _CRTIMP
  errno_t
  __cdecl
  _wctime32_s(
    _Out_writes_(_SizeInWords) _Post_readable_size_(26) wchar_t *_Buf,
    _In_ size_t _SizeInWords,
    _In_ const __time32_t *_Time);

  _CRTIMP
  errno_t
  __cdecl
  _wstrdate_s(
    _Out_writes_(_SizeInWords) _Post_readable_size_(9) wchar_t *_Buf,
    _In_range_(>=, 9) size_t _SizeInWords);

  _CRTIMP
  errno_t
  __cdecl
  _wstrtime_s(
    _Out_writes_(_SizeInWords) _Post_readable_size_(9) wchar_t *_Buf,
    _In_ size_t _SizeInWords);

#if _INTEGRAL_MAX_BITS >= 64

  _CRTIMP
  wchar_t*
  __cdecl
  _wctime64(
    _In_ const __time64_t *_Time);

  _CRTIMP
  errno_t
  __cdecl
  _wctime64_s(
    _Out_writes_(_SizeInWords) _Post_readable_size_(26) wchar_t *_Buf,
    _In_ size_t _SizeInWords,
    _In_ const __time64_t *_Time);

#endif /* _INTEGRAL_MAX_BITS >= 64 */

#if !defined (RC_INVOKED) && !defined (_INC_WTIME_INL)
#define _INC_WTIME_INL
#ifdef _USE_32BIT_TIME_T
__CRT_INLINE wchar_t *__cdecl _wctime(const time_t *_Time) { return _wctime32(_Time); }
#else /* !_USE_32BIT_TIME_T */
__CRT_INLINE wchar_t *__cdecl _wctime(const time_t *_Time) { return _wctime64(_Time); }
#endif /* !_USE_32BIT_TIME_T */
#endif /* !defined (RC_INVOKED) && !defined (_INC_WTIME_INL) */

#endif /* _WTIME_DEFINED */

  typedef int mbstate_t;
  typedef wchar_t _Wint_t;

  wint_t
  __cdecl
  btowc(
    int);

  size_t
  __cdecl
  mbrlen(
    _In_reads_bytes_opt_(_SizeInBytes) _Pre_opt_z_ const char *_Ch,
    _In_ size_t _SizeInBytes,
    _Out_opt_ mbstate_t *_State);

  size_t
  __cdecl
  mbrtowc(
    _Pre_maybenull_ _Post_z_ wchar_t *_DstCh,
    _In_reads_bytes_opt_(_SizeInBytes) _Pre_opt_z_ const char *_SrcCh,
    _In_ size_t _SizeInBytes,
    _Out_opt_ mbstate_t *_State);

  size_t
  __cdecl
  mbsrtowcs(
    _Pre_notnull_ _Post_z_ wchar_t *_Dest,
    _Inout_ _Deref_prepost_opt_valid_ const char **_PSrc,
    _In_ size_t _Count,
    _Inout_opt_ mbstate_t *_State);

  size_t
  __cdecl
  wcrtomb(
    _Pre_maybenull_ _Post_z_ char *_Dest,
    _In_ wchar_t _Source,
    _Out_opt_ mbstate_t *_State);

  size_t
  __cdecl
  wcsrtombs(
    _Pre_maybenull_ _Post_z_ char *_Dest,
    _Inout_ _Deref_prepost_z_ const wchar_t **_PSource,
    _In_ size_t _Count,
    _Out_opt_ mbstate_t *_State);

  int
  __cdecl
  wctob(
    _In_ wint_t _WCh);

#ifndef __NO_ISOCEXT /* these need static lib libmingwex.a */

  wchar_t*
  __cdecl
  wmemset(
    _Out_writes_all_(_N) wchar_t *_S,
    _In_ wchar_t _C,
    _In_ size_t _N);

  _CONST_RETURN
  wchar_t*
  __cdecl
  wmemchr(
    _In_reads_(_N) const wchar_t *_S,
    _In_ wchar_t _C,
    _In_ size_t _N);

#ifdef __cplusplus
    extern "C++"
    inline wchar_t* __cdecl wmemchr(
        _In_reads_(_N) wchar_t *_S,
        _In_ wchar_t _C,
        _In_ size_t _N)
    {
        const wchar_t *_SC = _S;
        return const_cast<wchar_t*>(wmemchr(_SC, _C, _N));
    }
#endif // __cplusplus

  int
  __cdecl
  wmemcmp(
    _In_reads_(_N) const wchar_t *_S1,
    _In_reads_(_N) const wchar_t *_S2,
    _In_ size_t _N);

  _Post_equal_to_(_S1)
  _At_buffer_(_S1, _Iter_, _N, _Post_satisfies_(_S1[_Iter_] == _S2[_Iter_]))
  wchar_t*
  __cdecl
  wmemcpy(
    _Out_writes_all_(_N) wchar_t *_S1,
    _In_reads_(_N) const wchar_t *_S2,
    _In_ size_t _N);

  wchar_t*
  __cdecl
  wmemmove(
    _Out_writes_all_opt_(_N) wchar_t *_S1,
    _In_reads_opt_(_N) const wchar_t *_S2,
    _In_ size_t _N);

  __MINGW_EXTENSION
  long long
  __cdecl
  wcstoll(
    const wchar_t *nptr,
    wchar_t **endptr,
    int base);

  __MINGW_EXTENSION
  unsigned long long
  __cdecl
  wcstoull(
    const wchar_t *nptr,
    wchar_t **endptr,
    int base);

#endif /* __NO_ISOCEXT */

  void*
  __cdecl
  memmove(
    _Out_writes_bytes_all_opt_(_MaxCount) void *_Dst,
    _In_reads_bytes_opt_(_MaxCount) const void *_Src,
    _In_ size_t _MaxCount);

  _Post_equal_to_(_Dst)
  _At_buffer_((unsigned char*)_Dst,
              _Iter_,
              _MaxCount,
              _Post_satisfies_(((unsigned char*)_Dst)[_Iter_] == ((unsigned char*)_Src)[_Iter_]))
  void*
  __cdecl
  memcpy(
    _Out_writes_bytes_all_(_MaxCount) void *_Dst,
    _In_reads_bytes_(_MaxCount) const void *_Src,
    _In_ size_t _MaxCount);

  __CRT_INLINE
  int
  __cdecl
  fwide(
    _In_opt_ FILE *_F,
    int _M)
  {
    (void)_F;
    return (_M);
  }

  __CRT_INLINE
  int
  __cdecl
  mbsinit(
    _In_opt_ const mbstate_t *_P)
  {
    return (!_P || *_P==0);
  }

  __CRT_INLINE
  _CONST_RETURN
  wchar_t*
  __cdecl
  wmemchr(
    _In_reads_(_N) const wchar_t *_S,
    _In_ wchar_t _C,
    _In_ size_t _N)
  {
    for (;0<_N;++_S,--_N)
    {
      if (*_S==_C) return (_CONST_RETURN wchar_t *)(_S);
    }
    return (0);
  }

  __CRT_INLINE
  int
  __cdecl
  wmemcmp(
    _In_reads_(_N) const wchar_t *_S1,
    _In_reads_(_N) const wchar_t *_S2,
    _In_ size_t _N)
  {
    for (; 0 < _N; ++_S1,++_S2,--_N)
    {
      if (*_S1!=*_S2) return (*_S1 < *_S2 ? -1 : +1);
    }
    return (0);
  }

  _Post_equal_to_(_S1)
  _At_buffer_(_S1,
              _Iter_,
              _N,
              _Post_satisfies_(_S1[_Iter_] == _S2[_Iter_]))
  __CRT_INLINE
  wchar_t*
  __cdecl
  wmemcpy(
    _Out_writes_all_(_N) wchar_t *_S1,
    _In_reads_(_N) const wchar_t *_S2,
    _In_ size_t _N)
  {
    return (wchar_t *)memcpy(_S1,_S2,_N*sizeof(wchar_t));
  }

  __CRT_INLINE
  wchar_t*
  __cdecl
  wmemmove(
    _Out_writes_all_opt_(_N) wchar_t *_S1,
    _In_reads_opt_(_N) const wchar_t *_S2,
    _In_ size_t _N)
  {
    return (wchar_t *)memmove(_S1,_S2,_N*sizeof(wchar_t));
  }

  __CRT_INLINE
  wchar_t*
  __cdecl
  wmemset(
    _Out_writes_all_(_N) wchar_t *_S,
    _In_ wchar_t _C,
    _In_ size_t _N)
  {
    wchar_t *_Su = _S;
    for (;0<_N;++_Su,--_N) {
      *_Su = _C;
    }
    return (_S);
  }

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#include <sec_api/wchar_s.h>
#endif
