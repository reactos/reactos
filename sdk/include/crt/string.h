/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_STRING
#define _INC_STRING

#include <corecrt.h>

#define __CORRECT_ISO_CPP_STRING_H_PROTO // For stl

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _NLSCMP_DEFINED
#define _NLSCMP_DEFINED
#define _NLSCMPERROR 2147483647
#endif

#ifndef _CRT_MEMORY_DEFINED
#define _CRT_MEMORY_DEFINED

  _CRTIMP
  void*
  __cdecl
  _memccpy(
    _Out_writes_bytes_opt_(_MaxCount) void *_Dst,
    _In_ const void *_Src,
    _In_ int _Val,
    _In_ size_t _MaxCount);

_CRT_DISABLE_GCC_WARNINGS
  _Must_inspect_result_
  _CRTIMP
  _CONST_RETURN
  void*
  __cdecl
  memchr(
    _In_reads_bytes_opt_(_MaxCount) const void *_Buf,
    _In_ int _Val,
    _In_ size_t _MaxCount);
_CRT_RESTORE_GCC_WARNINGS

#if defined __cplusplus
    extern "C++" _Must_inspect_result_
    inline void* __CRTDECL memchr(
        _In_reads_bytes_opt_(_MaxCount) void *_Buf,
        _In_ int _Val,
        _In_ size_t _MaxCount)
    {
        const void *_Bufc = _Buf;
        return const_cast<void*>(memchr(_Bufc, _Val, _MaxCount));
    }
#endif

  _Must_inspect_result_
  _CRTIMP
  int
  __cdecl
  _memicmp(
    _In_reads_bytes_opt_(_Size) const void *_Buf1,
    _In_reads_bytes_opt_(_Size) const void *_Buf2,
    _In_ size_t _Size);

  _Must_inspect_result_
  _CRTIMP
  int
  __cdecl
  _memicmp_l(
    _In_reads_bytes_opt_(_Size) const void *_Buf1,
    _In_reads_bytes_opt_(_Size) const void *_Buf2,
    _In_ size_t _Size,
    _In_opt_ _locale_t _Locale);

  _Must_inspect_result_
  int
  __cdecl
  memcmp(
    _In_reads_bytes_(_Size) const void *_Buf1,
    _In_reads_bytes_(_Size) const void *_Buf2,
    _In_ size_t _Size);

  _Post_equal_to_(_Dst)
  _At_buffer_((unsigned char*)_Dst,
              _Iter_,
              _MaxCount,
              _Post_satisfies_(((unsigned char*)_Dst)[_Iter_] == ((unsigned char*)_Src)[_Iter_]))
  _CRT_INSECURE_DEPRECATE_MEMORY(memcpy_s)
  void*
  __cdecl
  memcpy(
    _Out_writes_bytes_all_(_MaxCount) void *_Dst,
    _In_reads_bytes_(_MaxCount) const void *_Src,
    _In_ size_t _MaxCount);

  _Post_equal_to_(_Dst)
  _At_buffer_((unsigned char*)_Dst,
              _Iter_,
              _Size,
              _Post_satisfies_(((unsigned char*)_Dst)[_Iter_] == _Val))
  void*
  __cdecl
  memset(
    _Out_writes_bytes_all_(_Size) void *_Dst,
    _In_ int _Val,
    _In_ size_t _Size);

#ifndef NO_OLDNAMES

  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_memccpy)
  void*
  __cdecl
  memccpy(
    _Out_writes_bytes_opt_(_Size) void *_Dst,
    _In_reads_bytes_opt_(_Size) const void *_Src,
    _In_ int _Val,
    _In_ size_t _Size);

  _Check_return_
  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_memicmp)
  int
  __cdecl
  memicmp(
    _In_reads_bytes_opt_(_Size) const void *_Buf1,
    _In_reads_bytes_opt_(_Size) const void *_Buf2,
    _In_ size_t _Size);

#endif /* NO_OLDNAMES */

#endif /* _CRT_MEMORY_DEFINED */

  char*
  __cdecl
  _strset(
    _Inout_z_ char *_Str,
    _In_ int _Val);

  char*
  __cdecl
  strcpy(
    _Out_writes_z_(_String_length_(_Source) + 1) char *_Dest,
    _In_z_ const char *_Source);

  char*
  __cdecl
  strcat(
    _Inout_updates_z_(_String_length_(_Dest) + _String_length_(_Source) + 1) char *_Dest,
    _In_z_ const char *_Source);

  _Check_return_
  int
  __cdecl
  strcmp(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str2);

  _CRTIMP
  size_t
  __cdecl
  strlen(
    _In_z_ const char *_Str);

  _When_(_MaxCount > _String_length_(_Str),
         _Post_satisfies_(return == _String_length_(_Str)))
  _When_(_MaxCount <= _String_length_(_Str),
         _Post_satisfies_(return == _MaxCount))
  _CRTIMP
  size_t
  __cdecl
  strnlen(
    _In_reads_or_z_(_MaxCount) const char *_Str,
    _In_ size_t _MaxCount);

  _CRT_INSECURE_DEPRECATE_MEMORY(memmove_s)
  void*
  __cdecl
  memmove(
    _Out_writes_bytes_all_opt_(_MaxCount) void *_Dst,
    _In_reads_bytes_opt_(_MaxCount) const void *_Src,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  char*
  __cdecl
  _strdup(
    _In_opt_z_ const char *_Src);

_CRT_DISABLE_GCC_WARNINGS
  _Check_return_
  _CRTIMP
  _CONST_RETURN
  char*
  __cdecl
  strchr(
    _In_z_ const char *_Str,
    _In_ int _Val);
_CRT_RESTORE_GCC_WARNINGS

#ifdef __cplusplus
    extern "C++"
    _Check_return_
    inline char* __CRTDECL strchr(_In_z_ char *_String, _In_ int _Ch)
    {
        return const_cast<char*>(strchr(static_cast<const char*>(_String), _Ch));
    }
#endif // __cplusplus

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _stricmp(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str2);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _strcmpi(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str2);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _stricmp_l(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str2,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  strcoll(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str2);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _strcoll_l(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str2,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _stricoll(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str2);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _stricoll_l(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str2,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _strncoll(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _strncoll_l(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str2,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _strnicoll(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _strnicoll_l(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str2,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  strcspn(
    _In_z_ const char *_Str,
    _In_z_ const char *_Control);

  _Check_return_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE(_strerror_s)
  char*
  __cdecl
  _strerror(
    _In_opt_z_ const char *_ErrMsg);

  _Check_return_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE(strerror_s)
  char*
  __cdecl
  strerror(
    _In_ int);

  _CRTIMP
  char*
  __cdecl
  _strlwr(
    _Inout_z_ char *_String);

  char*
  strlwr_l(
    char *_String,
    _locale_t _Locale);

  char*
  __cdecl
  strncat(
    char *_Dest,
    const char *_Source,
    size_t _Count);

  _Check_return_
  int
  __cdecl
  strncmp(
    _In_reads_or_z_(_MaxCount) const char *_Str1,
    _In_reads_or_z_(_MaxCount) const char *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _strnicmp(
    _In_reads_or_z_(_MaxCount) const char *_Str1,
    _In_reads_or_z_(_MaxCount) const char *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _strnicmp_l(
    _In_reads_or_z_(_MaxCount) const char *_Str1,
    _In_reads_or_z_(_MaxCount) const char *_Str2,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  char*
  __cdecl
  strncpy(
    char *_Dest,
    const char *_Source,
    size_t _Count);

  _CRTIMP
  _CRT_INSECURE_DEPRECATE_CORE(_strnset_s)
  char*
  __cdecl
  _strnset(
    char *_Str,
    int _Val,
    size_t _MaxCount);

_CRT_DISABLE_GCC_WARNINGS
  _Check_return_
  _CRTIMP
  _CONST_RETURN
  char*
  __cdecl
  strpbrk(
    _In_z_ const char *_Str,
    _In_z_ const char *_Control);
_CRT_RESTORE_GCC_WARNINGS

#ifdef __cplusplus
    extern "C++"
    _Check_return_
    inline char* __CRTDECL strpbrk(_In_z_ char *_String, _In_z_ const char *_Control)
    {
        return const_cast<char*>(strpbrk(static_cast<const char*>(_String), _Control));
    }
#endif // __cplusplus

_CRT_DISABLE_GCC_WARNINGS
  _Check_return_
  _CRTIMP
  _CONST_RETURN
  char*
  __cdecl
  strrchr(
    _In_z_ const char *_Str,
    _In_ int _Ch);
_CRT_RESTORE_GCC_WARNINGS

#ifdef __cplusplus
    extern "C++"
    _Check_return_
    inline char* __CRTDECL strrchr(_In_z_ char *_String, _In_ int _Ch)
    {
        return const_cast<char*>(strrchr(static_cast<const char*>(_String), _Ch));
    }
#endif // __cplusplus

  _CRTIMP
  char*
  __cdecl
  _strrev(
    _Inout_z_ char *_Str);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  strspn(
    _In_z_ const char *_Str,
    _In_z_ const char *_Control);

_CRT_DISABLE_GCC_WARNINGS
  _Check_return_
  _CRTIMP
  _CONST_RETURN
  char*
  __cdecl
  strstr(
    _In_z_ const char *_Str,
    _In_z_ const char *_SubStr);
_CRT_RESTORE_GCC_WARNINGS

#ifdef __cplusplus
    extern "C++"
    _Check_return_ _Ret_maybenull_
    inline char* __CRTDECL strstr(_In_z_ char *_String, _In_z_ const char *_SubString)
    {
        return const_cast<char*>(strstr(static_cast<const char*>(_String), _SubString));
    }
#endif // __cplusplus

  _Check_return_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE_CORE(strtok_s)
  char*
  __cdecl
  strtok(
    _Inout_opt_z_ char *_Str,
    _In_z_ const char *_Delim);

  _CRTIMP
  char*
  __cdecl
  _strupr(
    _Inout_z_ char *_String);

  _CRTIMP
  char*
  _strupr_l(
    char *_String,
    _locale_t _Locale);

  _Check_return_opt_
  _CRTIMP
  size_t
  __cdecl
  strxfrm(
    _Out_writes_opt_(_MaxCount) _Post_maybez_ char *_Dst,
    _In_z_ const char *_Src,
    _In_ size_t _MaxCount);

  _Check_return_opt_
  _CRTIMP
  size_t
  __cdecl
  _strxfrm_l(
    _Out_writes_opt_(_MaxCount) _Post_maybez_ char *_Dst,
    _In_z_ const char *_Src,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

#if __STDC_WANT_SECURE_LIB__

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strset_s(
    _Inout_updates_z_(_DstSize) char *_Dst,
    _In_ size_t _DstSize,
    _In_ int _Value);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strerror_s(
    _Out_writes_z_(_SizeInBytes) char *_Buf,
    _In_ size_t _SizeInBytes,
    _In_opt_z_ const char *_ErrMsg);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strlwr_s(
    _Inout_updates_z_(_Size) char *_Str,
    _In_ size_t _Size);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strlwr_s_l(
    _Inout_updates_z_(_Size) char *_Str,
    _In_ size_t _Size,
    _In_opt_ _locale_t _Locale);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strnset_s(
    _Inout_updates_z_(_Size) char *_Str,
    _In_ size_t _Size,
    _In_ int _Val,
    _In_ size_t _MaxCount);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strupr_s(
    _Inout_updates_z_(_Size) char *_Str,
    _In_ size_t _Size);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strupr_s_l(
    _Inout_updates_z_(_Size) char *_Str,
    _In_ size_t _Size,
    _In_opt_ _locale_t _Locale);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  strcpy_s(
    _Out_writes_z_(_Size) char *_Dst,
    _In_ size_t _Size,
    _In_z_ const char *_Src);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  strcat_s(
    _Inout_updates_z_(_Size) char *_Dst,
    _In_ size_t _Size,
    _In_z_ const char *_Src);

#endif /* __STDC_WANT_SECURE_LIB__ */

#ifndef NO_OLDNAMES

  _Check_return_
  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_strdup)
  char*
  __cdecl
  strdup(
    _In_opt_z_ const char *_Src);

  _Check_return_
  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_strcmpi)
  int
  __cdecl
  strcmpi(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str2);

  _Check_return_
  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_stricmp)
  int
  __cdecl
  stricmp(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str2);

  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_strlwr)
  char*
  __cdecl
  strlwr(
    _Inout_z_ char *_Str);

  _Check_return_
  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_strnicmp)
  int
  __cdecl
  strnicmp(
    _In_z_ const char *_Str1,
    _In_z_ const char *_Str,
    _In_ size_t _MaxCount);

  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_strnset)
  char*
  __cdecl
  strnset(
    _Inout_updates_z_(_MaxCount) char *_Str,
    _In_ int _Val,
    _In_ size_t _MaxCount);

  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_strrev)
  char*
  __cdecl
  strrev(
    _Inout_z_ char *_Str);

  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_strset)
  char*
  __cdecl
  strset(
    _Inout_z_ char *_Str,
    _In_ int _Val);

  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_strupr)
  char*
  __cdecl
  strupr(
    _Inout_z_ char *_Str);

#endif /* NO_OLDNAMES */

#ifndef _WSTRING_DEFINED
#define _WSTRING_DEFINED

  _Check_return_
  _CRTIMP
  wchar_t*
  __cdecl
  _wcsdup(
    _In_z_ const wchar_t *_Str);

  _CRTIMP
  _CRT_INSECURE_DEPRECATE(wcscat_s)
  wchar_t*
  __cdecl
  wcscat(
    _Inout_updates_z_(_String_length_(_Dest) + _String_length_(_Source) + 1) wchar_t *_Dest,
    _In_z_ const wchar_t *_Source);

  _Check_return_
  _When_(return != 0, _Ret_range_(_Str, _Str + _String_length_(_Str) - 1))
  _CRTIMP
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
    inline wchar_t* __CRTDECL wcschr(_In_z_ wchar_t *_String, wchar_t _C)
    {
        return const_cast<wchar_t*>(wcschr(static_cast<const wchar_t*>(_String), _C));
    }
#endif // __cplusplus

  _Check_return_
  _CRTIMP
  int
  __cdecl
  wcscmp(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2);

  _CRTIMP
  _CRT_INSECURE_DEPRECATE(wcscpy_s)
  wchar_t*
  __cdecl
  wcscpy(
    _Out_writes_z_(_String_length_(_Source) + 1) wchar_t *_Dest,
    _In_z_ const wchar_t *_Source);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  wcscspn(
    _In_z_ const wchar_t *_Str,
    _In_z_ const wchar_t *_Control);

  _CRTIMP
  wchar_t*
  __cdecl
  wcsncat(
    wchar_t *_Dest,
    const wchar_t *_Source,
    size_t _Count);

  _Ret_range_(==,_String_length_(_Str))
  _CRTIMP
  size_t
  __cdecl
  wcslen(
    _In_z_ const wchar_t *_Str);

  _When_(_MaxCount > _String_length_(_Src),
         _Post_satisfies_(return == _String_length_(_Src)))
  _When_(_MaxCount <= _String_length_(_Src),
         _Post_satisfies_(return == _MaxCount))
  _CRTIMP
  _CRT_INSECURE_DEPRECATE(wcsnlen_s)
  size_t
  __cdecl
  wcsnlen(
    _In_reads_or_z_(_MaxCount) const wchar_t *_Src,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  wcsncmp(
    _In_reads_or_z_(_MaxCount) const wchar_t *_Str1,
    _In_reads_or_z_(_MaxCount) const wchar_t *_Str2,
    _In_ size_t _MaxCount);

  _CRTIMP
  _CRT_INSECURE_DEPRECATE(wcsncpy_s)
  wchar_t*
  __cdecl
  wcsncpy(
    wchar_t *_Dest,
    const wchar_t *_Source,
    size_t _Count);

  _Check_return_
  _CRTIMP
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
  _CRTIMP
  _CONST_RETURN
  wchar_t*
  __cdecl
  wcsrchr(
    _In_z_ const wchar_t *_Str,
    _In_ wchar_t _Ch);

#ifdef __cplusplus
    extern "C++"
    _Check_return_
    inline wchar_t* __CRTDECL wcsrchr(_In_z_ wchar_t *_String, _In_ wchar_t _C)
    {
        return const_cast<wchar_t*>(wcsrchr(static_cast<const wchar_t*>(_String), _C));
    }
#endif // __cplusplus

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  wcsspn(
    _In_z_ const wchar_t *_Str,
    _In_z_ const wchar_t *_Control);

_CRT_DISABLE_GCC_WARNINGS
  _When_(return != 0,
         _Ret_range_(_Str, _Str + _String_length_(_Str) - 1))
  _CRTIMP
  _CONST_RETURN
  wchar_t*
  __cdecl
  wcsstr(
    _In_z_ const wchar_t *_Str,
    _In_z_ const wchar_t *_SubStr);
_CRT_RESTORE_GCC_WARNINGS

#ifdef __cplusplus
    extern "C++"
    _Check_return_ _Ret_maybenull_
    _When_(return != NULL, _Ret_range_(_String, _String + _String_length_(_String) - 1))
    inline wchar_t* __CRTDECL wcsstr(_In_z_ wchar_t *_String, _In_z_ const wchar_t *_SubStr)
    {
        return const_cast<wchar_t*>(wcsstr(static_cast<const wchar_t*>(_String), _SubStr));
    }
#endif // __cplusplus

  _Check_return_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE_CORE(wcstok_s)
  wchar_t*
  __cdecl
  wcstok(
    _Inout_opt_z_ wchar_t *_Str,
    _In_z_ const wchar_t *_Delim);

  _Check_return_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE(_wcserror_s)
  wchar_t*
  __cdecl
  _wcserror(
    _In_ int _ErrNum);

  _Check_return_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE(__wcserror_s)
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
  _CRT_INSECURE_DEPRECATE_CORE(_wcsnset_s)
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
  _CRT_INSECURE_DEPRECATE_CORE(_wcsset_s)
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
  _CRTIMP
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
  _CRTIMP
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

#if __STDC_WANT_SECURE_LIB__

  _CRTIMP
  errno_t
  __cdecl
  wcscat_s(
    wchar_t *Dest,
    size_t SizeInWords,
    const wchar_t *_Source);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  wcscpy_s(
    _Out_writes_z_(SizeInWords) wchar_t *Dest,
    _In_ size_t SizeInWords,
    _In_z_ const wchar_t *_Source);

  _CRTIMP
  errno_t
  __cdecl
  wcsnlen_s(
    wchar_t **_Src,
    size_t _MaxCount);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  wcsncpy_s(
    _Out_writes_z_(_DstSizeInChars) wchar_t *_Dst,
    _In_ size_t _DstSizeInChars,
    _In_z_ const wchar_t *_Src,
    _In_ size_t _MaxCount);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcserror_s(
    _Out_writes_opt_z_(_SizeInWords) wchar_t *_Buf,
    _In_ size_t _SizeInWords,
    _In_ int _ErrNum);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  __wcserror_s(
    _Out_writes_opt_z_(_SizeInWords) wchar_t *_Buffer,
    _In_ size_t _SizeInWords,
    _In_z_ const wchar_t *_ErrMsg);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcsnset_s(
    _Inout_updates_z_(_DstSizeInWords) wchar_t *_Dst,
    _In_ size_t _DstSizeInWords,
    _In_ wchar_t _Val,
    _In_ size_t _MaxCount);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcsset_s(
    _Inout_updates_z_(_SizeInWords) wchar_t *_Str,
    _In_ size_t _SizeInWords,
    _In_ wchar_t _Val);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcslwr_s(
    _Inout_updates_z_(_SizeInWords) wchar_t *_Str,
    _In_ size_t _SizeInWords);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcslwr_s_l(
    _Inout_updates_z_(_SizeInWords) wchar_t *_Str,
    _In_ size_t _SizeInWords,
    _In_opt_ _locale_t _Locale);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcsupr_s(
    _Inout_updates_z_(_Size) wchar_t *_Str,
    _In_ size_t _Size);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcsupr_s_l(
    _Inout_updates_z_(_Size) wchar_t *_Str,
    _In_ size_t _Size,
    _In_opt_ _locale_t _Locale);

#endif /* __STDC_WANT_SECURE_LIB__ */

#ifndef NO_OLDNAMES

  _Check_return_
  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_wcsdup)
  wchar_t*
  __cdecl
  wcsdup(
    _In_z_ const wchar_t *_Str);

#define wcswcs wcsstr

  _Check_return_
  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_wcsicmp)
  int
  __cdecl
  wcsicmp(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2);

  _Check_return_
  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_wcsnicmp)
  int
  __cdecl
  wcsnicmp(
    _In_reads_or_z_(_MaxCount) const wchar_t *_Str1,
    _In_reads_or_z_(_MaxCount) const wchar_t *_Str2,
    _In_ size_t _MaxCount);

  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_wcsnset)
  wchar_t*
  __cdecl
  wcsnset(
    _Inout_updates_z_(_MaxCount) wchar_t *_Str,
    _In_ wchar_t _Val,
    _In_ size_t _MaxCount);

  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_wcsrev)
  wchar_t*
  __cdecl
  wcsrev(
    _Inout_z_ wchar_t *_Str);

  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_wcsset)
  wchar_t*
  __cdecl
  wcsset(
    _Inout_z_ wchar_t *_Str,
    wchar_t _Val);

  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_wcslwr)
  wchar_t*
  __cdecl
  wcslwr(
    _Inout_z_ wchar_t *_Str);

  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_wcsupr)
  wchar_t*
  __cdecl
  wcsupr(
    _Inout_z_ wchar_t *_Str);

  _Check_return_
  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_wcsicoll)
  int
  __cdecl
  wcsicoll(
    _In_z_ const wchar_t *_Str1,
    _In_z_ const wchar_t *_Str2);

#endif /* NO_OLDNAMES */

#endif /* !_WSTRING_DEFINED */

#ifdef __cplusplus
}
#endif

#include <sec_api/string_s.h>
#endif
