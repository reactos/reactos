/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_STDLIB
#define _INC_STDLIB

#include <corecrt.h>
#include <limits.h>

#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MB_LEN_MAX
#define MB_LEN_MAX 5
#endif

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#ifndef _ONEXIT_T_DEFINED
#define _ONEXIT_T_DEFINED

  typedef int (__cdecl *_onexit_t)(void);

#ifndef	NO_OLDNAMES
#define onexit_t _onexit_t
#endif
#endif

#ifndef _DIV_T_DEFINED
#define _DIV_T_DEFINED

  typedef struct _div_t {
    int quot;
    int rem;
  } div_t;

  typedef struct _ldiv_t {
    long quot;
    long rem;
  } ldiv_t;
#endif

#ifndef _CRT_DOUBLE_DEC
#define _CRT_DOUBLE_DEC

#pragma pack(4)
  typedef struct {
    unsigned char ld[10];
  } _LDOUBLE;
#pragma pack()

#define _PTR_LD(x) ((unsigned char *)(&(x)->ld))

  typedef struct {
    double x;
  } _CRT_DOUBLE;

  typedef struct {
    float f;
  } _CRT_FLOAT;
#if __MINGW_GNUC_PREREQ(4,4)
#pragma push_macro("long")
#undef long
#endif

  typedef struct {
    long double x;
  } _LONGDOUBLE;

#if __MINGW_GNUC_PREREQ(4,4)
#pragma pop_macro("long")
#endif

#pragma pack(4)
  typedef struct {
    unsigned char ld12[12];
  } _LDBL12;
#pragma pack()
#endif

#define RAND_MAX 0x7fff

#ifndef MB_CUR_MAX
#define MB_CUR_MAX ___mb_cur_max_func()
#ifdef _M_CEE_PURE
  _CRTIMP int* __cdecl __p___mb_cur_max();
  #define __mb_cur_max (*__p___mb_cur_max())
#else /* !_M_CEE_PURE */
  _CRTIMP extern int __mb_cur_max;
#endif /* !_M_CEE_PURE */
  _CRTIMP int __cdecl ___mb_cur_max_func(void);
  _CRTIMP int __cdecl ___mb_cur_max_l_func(_locale_t);
#endif /* !MB_CUR_MAX */

#define __max(a,b) (((a) > (b)) ? (a) : (b))
#define __min(a,b) (((a) < (b)) ? (a) : (b))

#define _MAX_PATH 260
#define _MAX_DRIVE 3
#define _MAX_DIR 256
#define _MAX_FNAME 256
#define _MAX_EXT 256

#define _OUT_TO_DEFAULT 0
#define _OUT_TO_STDERR 1
#define _OUT_TO_MSGBOX 2
#define _REPORT_ERRMODE 3

#define _WRITE_ABORT_MSG 0x1
#define _CALL_REPORTFAULT 0x2

#define _MAX_ENV 32767

  typedef void (__cdecl *_purecall_handler)(void);

  _CRTIMP _purecall_handler __cdecl _set_purecall_handler(_In_opt_ _purecall_handler _Handler);
  _CRTIMP _purecall_handler __cdecl _get_purecall_handler(void);

  typedef void (__cdecl *_invalid_parameter_handler)(const wchar_t *,const wchar_t *,const wchar_t *,unsigned int,uintptr_t);
  _invalid_parameter_handler __cdecl _set_invalid_parameter_handler(_In_opt_ _invalid_parameter_handler _Handler);
  _invalid_parameter_handler __cdecl _get_invalid_parameter_handler(void);

#include <errno.h>
  _CRTIMP unsigned long *__cdecl __doserrno(void);
#define _doserrno (*__doserrno())
  errno_t __cdecl _set_doserrno(_In_ unsigned long _Value);
  errno_t __cdecl _get_doserrno(_Out_ unsigned long *_Value);

  _CRTIMP extern char *_sys_errlist[];
  _CRTIMP extern int _sys_nerr;

#if defined(_DLL) && defined(_M_IX86)
  _CRTIMP int *__cdecl __p___argc(void);
  _CRTIMP char ***__cdecl __p___argv(void);
  _CRTIMP wchar_t ***__cdecl __p___wargv(void);
  _CRTIMP char ***__cdecl __p__environ(void);
  _CRTIMP wchar_t ***__cdecl __p__wenviron(void);
  _CRTIMP char **__cdecl __p__pgmptr(void);
  _CRTIMP wchar_t **__cdecl __p__wpgmptr(void);
#endif

// FIXME: move inside _M_CEE_PURE section
  _CRTIMP int *__cdecl __p___argc();
  _CRTIMP char ***__cdecl __p___argv();
  _CRTIMP wchar_t ***__cdecl __p___wargv();
  _CRTIMP char ***__cdecl __p__environ();
  _CRTIMP wchar_t ***__cdecl __p__wenviron();
  _CRTIMP char **__cdecl __p__pgmptr();
  _CRTIMP wchar_t **__cdecl __p__wpgmptr();

#ifdef _M_CEE_PURE
  #define __argv (*__p___argv())
  #define __argc (*__p___argc())
  #define __wargv (*__p___wargv())
  #define _environ   (*__p__environ())
  #define _wenviron  (*__p__wenviron())
  #define _pgmptr    (*__p__pgmptr())
  #define _wpgmptr   (*__p__wpgmptr())
#else /* !_M_CEE_PURE */
  _CRTIMP extern int __argc;
  _CRTIMP extern char **__argv;
  _CRTIMP extern wchar_t **__wargv;
  _CRTIMP extern char **_environ;
  _CRTIMP extern wchar_t **_wenviron;
  _CRTIMP extern char *_pgmptr;
  _CRTIMP extern wchar_t *_wpgmptr;
#endif /* !_M_CEE_PURE */

  _CRTIMP errno_t __cdecl _get_environ(_Out_ char***);
  _CRTIMP errno_t __cdecl _get_wenviron(_Out_ wchar_t***);
  _CRTIMP errno_t __cdecl _get_pgmptr(_Outptr_result_z_ char **_Value);
  _CRTIMP errno_t __cdecl _get_wpgmptr(_Outptr_result_z_ wchar_t **_Value);

#ifdef _M_CEE_PURE
  _CRTIMP int* __cdecl __p__fmode();
  #define _fmode (*__p__fmode())
#else
  _CRTIMP extern int _fmode;
#endif /* !_M_CEE_PURE */
  _CRTIMP errno_t __cdecl _set_fmode(_In_ int _Mode);
  _CRTIMP errno_t __cdecl _get_fmode(_Out_ int *_PMode);

#ifdef _M_CEE_PURE
  _CRTIMP unsigned int* __cdecl __p__osplatform();
  _CRTIMP unsigned int* __cdecl __p__osver();
  _CRTIMP unsigned int* __cdecl __p__winver();
  _CRTIMP unsigned int* __cdecl __p__winmajor();
  _CRTIMP unsigned int* __cdecl __p__winminor();
#define _osplatform  (*__p__osplatform())
#define _osver       (*__p__osver())
#define _winver      (*__p__winver())
#define _winmajor    (*__p__winmajor())
#define _winminor    (*__p__winminor())
#else /* !_M_CEE_PURE */
  _CRTIMP extern unsigned int _osplatform;
  _CRTIMP extern unsigned int _osver;
  _CRTIMP extern unsigned int _winver;
  _CRTIMP extern unsigned int _winmajor;
  _CRTIMP extern unsigned int _winminor;
#endif /* !_M_CEE_PURE */

  errno_t __cdecl _get_osplatform(_Out_ unsigned int *_Value);
  errno_t __cdecl _get_osver(_Out_ unsigned int *_Value);
  errno_t __cdecl _get_winver(_Out_ unsigned int *_Value);
  errno_t __cdecl _get_winmajor(_Out_ unsigned int *_Value);
  errno_t __cdecl _get_winminor(_Out_ unsigned int *_Value);

#ifndef _countof
#ifndef __cplusplus
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#else
  extern "C++" {
    template <typename _CountofType,size_t _SizeOfArray>
       char (*__countof_helper(/*UNALIGNED*/ _CountofType (&_Array)[_SizeOfArray]))[_SizeOfArray];
#define _countof(_Array) sizeof(*__countof_helper(_Array))
  }
#endif
#endif

#ifndef _CRT_TERMINATE_DEFINED
#define _CRT_TERMINATE_DEFINED
  __declspec(noreturn) void __cdecl exit(_In_ int _Code);
  _CRTIMP __declspec(noreturn) void __cdecl _exit(_In_ int _Code);
#if !defined __NO_ISOCEXT /* extern stub in static libmingwex.a */
  /* C99 function name */
  __declspec(noreturn) void __cdecl _Exit(int); /* Declare to get noreturn attribute.  */
  __CRT_INLINE void __cdecl _Exit(int status)
  {  _exit(status); }
#endif
#if __MINGW_GNUC_PREREQ(4,4)
#pragma push_macro("abort")
#undef abort
#endif
  __declspec(noreturn) void __cdecl abort(void);
#if __MINGW_GNUC_PREREQ(4,4)
#pragma pop_macro("abort")
#endif
#endif

  _CRTIMP unsigned int __cdecl _set_abort_behavior(_In_ unsigned int _Flags, _In_ unsigned int _Mask);

#ifndef _CRT_ABS_DEFINED
#define _CRT_ABS_DEFINED
  int __cdecl abs(_In_ int _X);
  long __cdecl labs(_In_ long _X);
#endif

#if _INTEGRAL_MAX_BITS >= 64
  __MINGW_EXTENSION __int64 __cdecl _abs64(__int64);
#endif
  int __cdecl atexit(void (__cdecl *)(void));

#ifndef _CRT_ATOF_DEFINED
#define _CRT_ATOF_DEFINED

  _Check_return_
  double
  __cdecl
  atof(
    _In_z_ const char *_String);

  _Check_return_
  double
  __cdecl
  _atof_l(
    _In_z_ const char *_String,
    _In_opt_ _locale_t _Locale);

#endif /* _CRT_ATOF_DEFINED */

  _Check_return_
  int
  __cdecl
  atoi(
    _In_z_ const char *_Str);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _atoi_l(
    _In_z_ const char *_Str,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  long
  __cdecl
  atol(
    _In_z_ const char *_Str);

  _Check_return_
  _CRTIMP
  long
  __cdecl
  _atol_l(
    _In_z_ const char *_Str,
    _In_opt_ _locale_t _Locale);

#ifndef _CRT_ALGO_DEFINED
#define _CRT_ALGO_DEFINED

  _Check_return_
  void*
  __cdecl
  bsearch(
    _In_ const void *_Key,
    _In_reads_bytes_(_NumOfElements * _SizeOfElements) const void *_Base,
    _In_ size_t _NumOfElements,
    _In_ size_t _SizeOfElements,
    _In_ int (__cdecl *_PtFuncCompare)(const void *,const void *));

  void
  __cdecl
  qsort(
    _Inout_updates_bytes_(_NumOfElements * _SizeOfElements) _Post_readable_byte_size_(_NumOfElements * _SizeOfElements) void *_Base,
    _In_ size_t _NumOfElements,
    _In_ size_t _SizeOfElements,
    _In_ int (__cdecl *_PtFuncCompare)(const void *,const void *));

#endif /* _CRT_ALGO_DEFINED */

#if !defined(__GNUC__) && !defined(__clang__)

  _Check_return_
  unsigned short
  __cdecl
  _byteswap_ushort(
    _In_ unsigned short _Short);

  _Check_return_
  unsigned long
  __cdecl
  _byteswap_ulong(
    _In_ unsigned long _Long);

#if _INTEGRAL_MAX_BITS >= 64
  _Check_return_
  __MINGW_EXTENSION
  unsigned __int64
  __cdecl
  _byteswap_uint64(
    _In_ unsigned __int64 _Int64);
#endif

#endif /* !defined(__GNUC__) && !defined(__clang__) */

  _Check_return_
  div_t
  __cdecl
  div(
    _In_ int _Numerator,
    _In_ int _Denominator);

  _Check_return_
  char*
  __cdecl
  getenv(
    _In_z_ const char *_VarName);

  _CRTIMP
  char*
  __cdecl
  _itoa(
    _In_ int _Value,
    _Pre_notnull_ _Post_z_ char *_Dest,
    _In_ int _Radix);

#if _INTEGRAL_MAX_BITS >= 64

  __MINGW_EXTENSION
  _CRTIMP
  char*
  __cdecl
  _i64toa(
    _In_ __int64 _Val,
    _Pre_notnull_ _Post_z_ char *_DstBuf,
    _In_ int _Radix);

  __MINGW_EXTENSION
  _CRTIMP
  char*
  __cdecl
  _ui64toa(
    _In_ unsigned __int64 _Val,
    _Pre_notnull_ _Post_z_ char *_DstBuf,
    _In_ int _Radix);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  __int64
  __cdecl
  _atoi64(
    _In_z_ const char *_String);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  __int64
  __cdecl
  _atoi64_l(
    _In_z_ const char *_String,
    _In_opt_ _locale_t _Locale);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  __int64
  __cdecl
  _strtoi64(
    _In_z_ const char *_String,
    _Out_opt_ _Deref_post_z_ char **_EndPtr,
    _In_ int _Radix);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  __int64
  __cdecl
  _strtoi64_l(
    _In_z_ const char *_String,
    _Out_opt_ _Deref_post_z_ char **_EndPtr,
    _In_ int _Radix,
    _In_opt_ _locale_t _Locale);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  unsigned __int64
  __cdecl
  _strtoui64(
    _In_z_ const char *_String,
    _Out_opt_ _Deref_post_z_ char **_EndPtr,
    _In_ int _Radix);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  unsigned __int64
  __cdecl
  _strtoui64_l(
    _In_z_ const char *_String,
    _Out_opt_ _Deref_post_z_ char **_EndPtr,
    _In_ int _Radix,
    _In_opt_ _locale_t _Locale);

#endif /* _INTEGRAL_MAX_BITS >= 64 */

  _Check_return_
  ldiv_t
  __cdecl
  ldiv(
    _In_ long _Numerator,
    _In_ long _Denominator);

  _CRTIMP
  char*
  __cdecl
  _ltoa(
    _In_ long _Value,
    _Pre_notnull_ _Post_z_ char *_Dest,
    _In_ int _Radix);

  _Check_return_
  int
  __cdecl
  mblen(
    _In_reads_bytes_opt_(_MaxCount) _Pre_opt_z_ const char *_Ch,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mblen_l(
    _In_reads_bytes_opt_(_MaxCount) _Pre_opt_z_ const char *_Ch,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbstrlen(
    _In_z_ const char *_Str);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbstrlen_l(
    _In_z_ const char *_Str,
    _In_opt_ _locale_t _Locale);

  _Success_(return>0)
  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbstrnlen(
    _In_z_ const char *_Str,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbstrnlen_l(
    _In_z_ const char *_Str,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  int
  __cdecl
  mbtowc(
    _Pre_notnull_ _Post_z_ wchar_t *_DstCh,
    _In_reads_bytes_opt_(_SrcSizeInBytes) _Pre_opt_z_ const char *_SrcCh,
    _In_ size_t _SrcSizeInBytes);

  _CRTIMP
  int
  __cdecl
  _mbtowc_l(
    _Pre_notnull_ _Post_z_ wchar_t *_DstCh,
    _In_reads_bytes_opt_(_SrcSizeInBytes) _Pre_opt_z_ const char *_SrcCh,
    _In_ size_t _SrcSizeInBytes,
    _In_opt_ _locale_t _Locale);

  size_t
  __cdecl
  mbstowcs(
    _Out_writes_opt_z_(_MaxCount) wchar_t *_Dest,
    _In_z_ const char *_Source,
    _In_ size_t _MaxCount);

  _CRTIMP
  size_t
  __cdecl
  _mbstowcs_l(
    _Out_writes_opt_z_(_MaxCount) wchar_t *_Dest,
    _In_z_ const char *_Source,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  int
  __cdecl
  rand(void);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _set_error_mode(
    _In_ int _Mode);

  void
  __cdecl
  srand(
    _In_ unsigned int _Seed);

  _Check_return_
  double
  __cdecl
  strtod(
    _In_z_ const char *_Str,
    _Out_opt_ _Deref_post_z_ char **_EndPtr);

  float
  __cdecl
  strtof(
    const char *nptr,
    char **endptr);

#if !defined __NO_ISOCEXT  /* in libmingwex.a */
  float __cdecl strtof (const char * __restrict__, char ** __restrict__);
  long double __cdecl strtold(const char * __restrict__, char ** __restrict__);
#endif /* __NO_ISOCEXT */

  _Check_return_
  _CRTIMP
  double
  __cdecl
  _strtod_l(
    _In_z_ const char *_Str,
    _Out_opt_ _Deref_post_z_ char **_EndPtr,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  long
  __cdecl
  strtol(
    _In_z_ const char *_Str,
    _Out_opt_ _Deref_post_z_ char **_EndPtr,
    _In_ int _Radix);

  _Check_return_
  _CRTIMP
  long
  __cdecl
  _strtol_l(
    _In_z_ const char *_Str,
    _Out_opt_ _Deref_post_z_ char **_EndPtr,
    _In_ int _Radix,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  unsigned long
  __cdecl
  strtoul(
    _In_z_ const char *_Str,
    _Out_opt_ _Deref_post_z_ char **_EndPtr,
    _In_ int _Radix);

  _Check_return_
  _CRTIMP
  unsigned long
  __cdecl
  _strtoul_l(
    _In_z_ const char *_Str,
    _Out_opt_ _Deref_post_z_ char **_EndPtr,
    _In_ int _Radix,
    _In_opt_ _locale_t _Locale);

#ifndef _CRT_SYSTEM_DEFINED
#define _CRT_SYSTEM_DEFINED
  int
  __cdecl
  system(
    _In_opt_z_ const char *_Command);
#endif

  _CRTIMP
  char*
  __cdecl
  _ultoa(
    _In_ unsigned long _Value,
    _Pre_notnull_ _Post_z_ char *_Dest,
    _In_ int _Radix);

  int
  __cdecl
  wctomb(
    _Out_writes_opt_z_(MB_LEN_MAX) char *_MbCh,
    _In_ wchar_t _WCh);

  _CRTIMP
  int
  __cdecl
  _wctomb_l(
    _Pre_maybenull_ _Post_z_ char *_MbCh,
    _In_ wchar_t _WCh,
    _In_opt_ _locale_t _Locale);

  size_t
  __cdecl
  wcstombs(
    _Out_writes_opt_z_(_MaxCount) char *_Dest,
    _In_z_ const wchar_t *_Source,
    _In_ size_t _MaxCount);

  _CRTIMP
  size_t
  __cdecl
  _wcstombs_l(
    _Out_writes_opt_z_(_MaxCount) char *_Dest,
    _In_z_ const wchar_t *_Source,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

#ifndef _CRT_ALLOCATION_DEFINED
#define _CRT_ALLOCATION_DEFINED

  _Check_return_
  _Ret_maybenull_
  _Post_writable_byte_size_(_NumOfElements * _SizeOfElements)
  void*
  __cdecl
  calloc(
    _In_ size_t _NumOfElements,
    _In_ size_t _SizeOfElements);

  void
  __cdecl
  free(
    _Pre_maybenull_ _Post_invalid_ void *_Memory);

  _Check_return_
  _Ret_maybenull_
  _Post_writable_byte_size_(_Size)
  void*
  __cdecl
  malloc(
    _In_ size_t _Size);

  _Success_(return != 0)
  _Check_return_
  _Ret_maybenull_
  _Post_writable_byte_size_(_NewSize)
  void*
  __cdecl
  realloc(
    _Pre_maybenull_ _Post_invalid_ void *_Memory,
    _In_ size_t _NewSize);

  _Success_(return != 0)
  _Check_return_
  _Ret_maybenull_
  _Post_writable_byte_size_(_Count * _Size)
  _CRTIMP
  void*
  __cdecl
  _recalloc(
    _Pre_maybenull_ _Post_invalid_ void *_Memory,
    _In_ size_t _Count,
    _In_ size_t _Size);

/* Make sure that X86intrin.h doesn't produce here collisions.  */
#if (!defined (_XMMINTRIN_H_INCLUDED) && !defined (_MM_MALLOC_H_INCLUDED)) || defined(_aligned_malloc)
#pragma push_macro("_aligned_free")
#pragma push_macro("_aligned_malloc")
#undef _aligned_free
#undef _aligned_malloc

  _CRTIMP
  void
  __cdecl
  _aligned_free(
    _Pre_maybenull_ _Post_invalid_ void *_Memory);

  _Check_return_
  _Ret_maybenull_
  _Post_writable_byte_size_(_Size)
  _CRTIMP
  void*
  __cdecl
  _aligned_malloc(
    _In_ size_t _Size,
    _In_ size_t _Alignment);

#pragma pop_macro("_aligned_free")
#pragma pop_macro("_aligned_malloc")
#endif

  _Check_return_
  _Ret_maybenull_
  _Post_writable_byte_size_(_Size)
  _CRTIMP
  void*
  __cdecl
  _aligned_offset_malloc(
    _In_ size_t _Size,
    _In_ size_t _Alignment,
    _In_ size_t _Offset);

  _Success_(return != 0)
  _Check_return_
  _Ret_maybenull_
  _Post_writable_byte_size_(_Size)
  _CRTIMP
  void*
  __cdecl
  _aligned_realloc(
    _Pre_maybenull_ _Post_invalid_ void *_Memory,
    _In_ size_t _Size,
    _In_ size_t _Alignment);

  _Success_(return != 0)
  _Check_return_
  _Ret_maybenull_
  _Post_writable_byte_size_(_Count * _Size)
  _CRTIMP
  void*
  __cdecl
  _aligned_recalloc(
    _Pre_maybenull_ _Post_invalid_ void *_Memory,
    _In_ size_t _Count,
    _In_ size_t _Size,
    _In_ size_t _Alignment);

  _Success_(return != 0)
  _Check_return_
  _Ret_maybenull_
  _Post_writable_byte_size_(_Size)
  _CRTIMP
  void*
  __cdecl
  _aligned_offset_realloc(
    _Pre_maybenull_ _Post_invalid_ void *_Memory,
    _In_ size_t _Size,
    _In_ size_t _Alignment,
    _In_ size_t _Offset);

  _Check_return_
  _Ret_maybenull_
  _Post_writable_byte_size_(_Count * _Size)
  _CRTIMP
  void*
  __cdecl
  _aligned_offset_recalloc(
    _Pre_maybenull_ _Post_invalid_ void *_Memory,
    _In_ size_t _Count,
    _In_ size_t _Size,
    _In_ size_t _Alignment,
    _In_ size_t _Offset);

#endif /* _CRT_ALLOCATION_DEFINED */

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

  float
  __cdecl
  wcstof(
    const wchar_t *nptr,
    wchar_t **endptr);

#if !defined __NO_ISOCEXT /* in libmingwex.a */
  float __cdecl wcstof( const wchar_t * __restrict__, wchar_t ** __restrict__);
  long double __cdecl wcstold(const wchar_t * __restrict__, wchar_t ** __restrict__);
#endif /* __NO_ISOCEXT */

  _Check_return_
  _CRTIMP
  double
  __cdecl
  _wcstod_l(
    _In_z_ const wchar_t *_Str,
    _Out_opt_ _Deref_post_z_ wchar_t **_EndPtr,
    _In_opt_ _locale_t _Locale);

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
#define _CVTBUFSIZE (309+40)

  _Check_return_
  _CRTIMP
  char*
  __cdecl
  _fullpath(
    _Out_writes_opt_z_(_SizeInBytes) char *_FullPath,
    _In_z_ const char *_Path,
    _In_ size_t _SizeInBytes);

  _Check_return_
  _CRTIMP
  char*
  __cdecl
  _ecvt(
    _In_ double _Val,
    _In_ int _NumOfDigits,
    _Out_ int *_PtDec,
    _Out_ int *_PtSign);

  _Check_return_
  _CRTIMP
  char*
  __cdecl
  _fcvt(
    _In_ double _Val,
    _In_ int _NumOfDec,
    _Out_ int *_PtDec,
    _Out_ int *_PtSign);

  _CRTIMP
  char*
  __cdecl
  _gcvt(
    _In_ double _Val,
    _In_ int _NumOfDigits,
    _Pre_notnull_ _Post_z_ char *_DstBuf);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _atodbl(
    _Out_ _CRT_DOUBLE *_Result,
    _In_z_ char *_Str);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _atoldbl(
    _Out_ _LDOUBLE *_Result,
    _In_z_ char *_Str);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _atoflt(
    _Out_ _CRT_FLOAT *_Result,
    _In_z_ char *_Str);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _atodbl_l(
    _Out_ _CRT_DOUBLE *_Result,
    _In_z_ char *_Str,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _atoldbl_l(
    _Out_ _LDOUBLE *_Result,
    _In_z_ char *_Str,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _atoflt_l(
    _Out_ _CRT_FLOAT *_Result,
    _In_z_ char *_Str,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  unsigned long
  __cdecl
  _lrotl(
    _In_ unsigned long _Val,
    _In_ int _Shift);

  _Check_return_
  unsigned long
  __cdecl
  _lrotr(
    _In_ unsigned long _Val,
    _In_ int _Shift);

  _CRTIMP
  void
  __cdecl
  _makepath(
    _Pre_notnull_ _Post_z_ char *_Path,
    _In_opt_z_ const char *_Drive,
    _In_opt_z_ const char *_Dir,
    _In_opt_z_ const char *_Filename,
    _In_opt_z_ const char *_Ext);

  _onexit_t
  __cdecl
  _onexit(
    _In_opt_ _onexit_t _Func);

#ifndef _CRT_PERROR_DEFINED
#define _CRT_PERROR_DEFINED
  void
  __cdecl
  perror(
    _In_opt_z_ const char *_ErrMsg);
#endif

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _putenv(
    _In_z_ const char *_EnvString);

#if !defined(__clang__)

  _Check_return_
  unsigned int
  __cdecl
  _rotl(
    _In_ unsigned int _Val,
    _In_ int _Shift);

#if _INTEGRAL_MAX_BITS >= 64
  __MINGW_EXTENSION
  _Check_return_
  unsigned __int64
  __cdecl
  _rotl64(
    _In_ unsigned __int64 _Val,
    _In_ int _Shift);
#endif

  _Check_return_
  unsigned int
  __cdecl
  _rotr(
    _In_ unsigned int _Val,
    _In_ int _Shift);

#if _INTEGRAL_MAX_BITS >= 64
  __MINGW_EXTENSION
  _Check_return_
  unsigned __int64
  __cdecl
  _rotr64(
    _In_ unsigned __int64 _Val,
    _In_ int _Shift);
#endif

#endif /* !defined(__clang__) */

  _CRTIMP
  void
  __cdecl
  _searchenv(
    _In_z_ const char *_Filename,
    _In_z_ const char *_EnvVar,
    _Pre_notnull_ _Post_z_ char *_ResultPath);

  _CRTIMP
  void
  __cdecl
  _splitpath(
    _In_z_ const char *_FullPath,
    _Pre_maybenull_ _Post_z_ char *_Drive,
    _Pre_maybenull_ _Post_z_ char *_Dir,
    _Pre_maybenull_ _Post_z_ char *_Filename,
    _Pre_maybenull_ _Post_z_ char *_Ext);

  _CRTIMP
  void
  __cdecl
  _swab(
    _Inout_updates_(_SizeInBytes) _Post_readable_size_(_SizeInBytes) char *_Buf1,
    _Inout_updates_(_SizeInBytes) _Post_readable_size_(_SizeInBytes) char *_Buf2,
    int _SizeInBytes);

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

  _CRTIMP
  __MINGW_ATTRIB_DEPRECATED
  void
  __cdecl
  _beep(
    _In_ unsigned _Frequency,
    _In_ unsigned _Duration);

  /* Not to be confused with  _set_error_mode (int).  */
  _CRTIMP
  __MINGW_ATTRIB_DEPRECATED
  void
  __cdecl
  _seterrormode(
    _In_ int _Mode);

  _CRTIMP
  __MINGW_ATTRIB_DEPRECATED
  void
  __cdecl
  _sleep(
    _In_ unsigned long _Duration);

#endif /* _POSIX_ */

#ifndef NO_OLDNAMES
#ifndef _POSIX_
#if 0
#ifndef __cplusplus
#ifndef NOMINMAX
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#endif /* NOMINMAX */
#endif /* __cplusplus */
#endif

#define sys_errlist _sys_errlist
#define sys_nerr _sys_nerr
#define environ _environ

  _Check_return_
  _CRTIMP
  char*
  __cdecl
  ecvt(
    _In_ double _Val,
    _In_ int _NumOfDigits,
    _Out_ int *_PtDec,
    _Out_ int *_PtSign);

  _Check_return_
  _CRTIMP
  char*
  __cdecl
  fcvt(
    _In_ double _Val,
    _In_ int _NumOfDec,
    _Out_ int *_PtDec,
    _Out_ int *_PtSign);

  _CRTIMP
  char*
  __cdecl
  gcvt(
    _In_ double _Val,
    _In_ int _NumOfDigits,
    _Pre_notnull_ _Post_z_ char *_DstBuf);

  _CRTIMP
  char*
  __cdecl
  itoa(
    _In_ int _Val,
    _Pre_notnull_ _Post_z_ char *_DstBuf,
    _In_ int _Radix);

  _CRTIMP
  char*
  __cdecl
  ltoa(
    _In_ long _Val,
    _Pre_notnull_ _Post_z_ char *_DstBuf,
    _In_ int _Radix);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  putenv(
    _In_z_ const char *_EnvString);

  _CRTIMP
  void
  __cdecl
  swab(
    _Inout_updates_z_(_SizeInBytes) char *_Buf1,
    _Inout_updates_z_(_SizeInBytes) char *_Buf2,
    _In_ int _SizeInBytes);

  _CRTIMP
  char*
  __cdecl
  ultoa(
    _In_ unsigned long _Val,
    _Pre_notnull_ _Post_z_ char *_Dstbuf,
    _In_ int _Radix);

  onexit_t
  __cdecl
  onexit(
    _In_opt_ onexit_t _Func);

#endif /* _POSIX_ */
#endif /* NO_OLDNAMES */

#if !defined __NO_ISOCEXT /* externs in static libmingwex.a */

  __MINGW_EXTENSION typedef struct { long long quot, rem; } lldiv_t;

  __MINGW_EXTENSION _Check_return_ lldiv_t __cdecl lldiv(_In_ long long, _In_ long long);

#if defined(_MSC_VER) && !defined(__clang__)
  _Check_return_ long long __cdecl llabs(_In_ long long _j);
  #pragma function(llabs)
#endif
  __MINGW_EXTENSION _Check_return_ __CRT_INLINE long long __cdecl llabs(_In_ long long _j) { return (_j >= 0 ? _j : -_j); }

  __MINGW_EXTENSION long long  __cdecl strtoll(const char* __restrict__, char** __restrict, int);
  __MINGW_EXTENSION unsigned long long  __cdecl strtoull(const char* __restrict__, char** __restrict__, int);

  /* these are stubs for MS _i64 versions */
  __MINGW_EXTENSION long long  __cdecl atoll (const char *);

#ifndef __STRICT_ANSI__
  __MINGW_EXTENSION long long  __cdecl wtoll (const wchar_t *);
  __MINGW_EXTENSION char *__cdecl lltoa (long long, char *, int);
  __MINGW_EXTENSION char *__cdecl ulltoa (unsigned long long , char *, int);
  __MINGW_EXTENSION wchar_t *__cdecl lltow (long long, wchar_t *, int);
  __MINGW_EXTENSION wchar_t *__cdecl ulltow (unsigned long long, wchar_t *, int);

  /* __CRT_INLINE using non-ansi functions */
  __MINGW_EXTENSION __CRT_INLINE long long  __cdecl atoll (const char * _c) { return _atoi64 (_c); }
  __MINGW_EXTENSION __CRT_INLINE char *__cdecl lltoa (long long _n, char * _c, int _i) { return _i64toa (_n, _c, _i); }
  __MINGW_EXTENSION __CRT_INLINE char *__cdecl ulltoa (unsigned long long _n, char * _c, int _i) { return _ui64toa (_n, _c, _i); }
  __MINGW_EXTENSION __CRT_INLINE long long  __cdecl wtoll (const wchar_t * _w) { return _wtoi64 (_w); }
  __MINGW_EXTENSION __CRT_INLINE wchar_t *__cdecl lltow (long long _n, wchar_t * _w, int _i) { return _i64tow (_n, _w, _i); }
  __MINGW_EXTENSION __CRT_INLINE wchar_t *__cdecl ulltow (unsigned long long _n, wchar_t * _w, int _i) { return _ui64tow (_n, _w, _i); }
#endif /* (__STRICT_ANSI__)  */

#endif /* !__NO_ISOCEXT */

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#include <sec_api/stdlib_s.h>
#endif
