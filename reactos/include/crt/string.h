/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_STRING
#define _INC_STRING

#include <crtdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _NLSCMP_DEFINED
#define _NLSCMP_DEFINED
#define _NLSCMPERROR 2147483647
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

#ifndef _CRT_MEMORY_DEFINED
#define _CRT_MEMORY_DEFINED
  _CRTIMP void *__cdecl _memccpy(void *_Dst,const void *_Src,int _Val,size_t _MaxCount);
  _CRTIMP _CONST_RETURN void *__cdecl memchr(const void *_Buf ,int _Val,size_t _MaxCount);
  _CRTIMP int __cdecl _memicmp(const void *_Buf1,const void *_Buf2,size_t _Size);
  _CRTIMP int __cdecl _memicmp_l(const void *_Buf1,const void *_Buf2,size_t _Size,_locale_t _Locale);
  int __cdecl memcmp(const void *_Buf1,const void *_Buf2,size_t _Size);
  _CRT_INSECURE_DEPRECATE_MEMORY(memcpy_s) void *__cdecl memcpy(void *_Dst,const void *_Src,size_t _Size);
  void *__cdecl memset(void *_Dst,int _Val,size_t _Size);
#ifndef	NO_OLDNAMES
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_memccpy) void *__cdecl memccpy(void *_Dst,const void *_Src,int _Val,size_t _Size);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_memicmp) int __cdecl memicmp(const void *_Buf1,const void *_Buf2,size_t _Size);
#endif
#endif

  char *__cdecl _strset(char *_Str,int _Val);
  char *__cdecl strcpy(char *_Dest,const char *_Source);
  char *__cdecl strcat(char *_Dest,const char *_Source);
  int __cdecl strcmp(const char *_Str1,const char *_Str2);
  size_t __cdecl strlen(const char *_Str);
  _CRTIMP size_t __cdecl strnlen(const char *_Str,size_t _MaxCount);

  _CRT_INSECURE_DEPRECATE_MEMORY(memmove_s) void *__cdecl memmove(void *_Dst,const void *_Src,size_t _Size);
  _CRTIMP char *__cdecl _strdup(const char *_Src);
  _CRTIMP _CONST_RETURN char *__cdecl strchr(const char *_Str,int _Val);
  _CRTIMP int __cdecl _stricmp(const char *_Str1,const char *_Str2);
  _CRTIMP int __cdecl _strcmpi(const char *_Str1,const char *_Str2);
  _CRTIMP int __cdecl _stricmp_l(const char *_Str1,const char *_Str2,_locale_t _Locale);
  _CRTIMP int __cdecl strcoll(const char *_Str1,const char *_Str2);
  _CRTIMP int __cdecl _strcoll_l(const char *_Str1,const char *_Str2,_locale_t _Locale);
  _CRTIMP int __cdecl _stricoll(const char *_Str1,const char *_Str2);
  _CRTIMP int __cdecl _stricoll_l(const char *_Str1,const char *_Str2,_locale_t _Locale);
  _CRTIMP int __cdecl _strncoll (const char *_Str1,const char *_Str2,size_t _MaxCount);
  _CRTIMP int __cdecl _strncoll_l(const char *_Str1,const char *_Str2,size_t _MaxCount,_locale_t _Locale);
  _CRTIMP int __cdecl _strnicoll (const char *_Str1,const char *_Str2,size_t _MaxCount);
  _CRTIMP int __cdecl _strnicoll_l(const char *_Str1,const char *_Str2,size_t _MaxCount,_locale_t _Locale);
  _CRTIMP size_t __cdecl strcspn(const char *_Str,const char *_Control);
  _CRTIMP _CRT_INSECURE_DEPRECATE(_strerror_s) char *__cdecl _strerror(const char *_ErrMsg);
  _CRTIMP _CRT_INSECURE_DEPRECATE(strerror_s) char *__cdecl strerror(int);
  _CRTIMP char *__cdecl _strlwr(char *_String);
  char *strlwr_l(char *_String,_locale_t _Locale);
  char *__cdecl strncat(char *_Dest,const char *_Source,size_t _Count);
  int __cdecl strncmp(const char *_Str1,const char *_Str2,size_t _MaxCount);
  _CRTIMP int __cdecl _strnicmp(const char *_Str1,const char *_Str2,size_t _MaxCount);
  _CRTIMP int __cdecl _strnicmp_l(const char *_Str1,const char *_Str2,size_t _MaxCount,_locale_t _Locale);
  char *strncpy(char *_Dest,const char *_Source,size_t _Count);
  _CRTIMP _CRT_INSECURE_DEPRECATE_CORE(_strnset_s) char *__cdecl _strnset(char *_Str,int _Val,size_t _MaxCount);
  _CRTIMP _CONST_RETURN char *__cdecl strpbrk(const char *_Str,const char *_Control);
  _CRTIMP _CONST_RETURN char *__cdecl strrchr(const char *_Str,int _Ch);
  _CRTIMP char *__cdecl _strrev(char *_Str);
  _CRTIMP size_t __cdecl strspn(const char *_Str,const char *_Control);
  _CRTIMP _CONST_RETURN char *__cdecl strstr(const char *_Str,const char *_SubStr);
  _CRTIMP _CRT_INSECURE_DEPRECATE_CORE(strtok_s) char *__cdecl strtok(char *_Str,const char *_Delim);
  _CRTIMP char *__cdecl _strupr(char *_String);
  _CRTIMP char *_strupr_l(char *_String,_locale_t _Locale);
  _CRTIMP size_t __cdecl strxfrm(char *_Dst,const char *_Src,size_t _MaxCount);
  _CRTIMP size_t __cdecl _strxfrm_l(char *_Dst,const char *_Src,size_t _MaxCount,_locale_t _Locale);

#ifndef	NO_OLDNAMES
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_strdup) char *__cdecl strdup(const char *_Src);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_strcmpi) int __cdecl strcmpi(const char *_Str1,const char *_Str2);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_stricmp) int __cdecl stricmp(const char *_Str1,const char *_Str2);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_strlwr) char *__cdecl strlwr(char *_Str);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_strnicmp) int __cdecl strnicmp(const char *_Str1,const char *_Str,size_t _MaxCount);
//  __CRT_INLINE int __cdecl strncasecmp (const char *__sz1, const char *__sz2, size_t __sizeMaxCompare) { return _strnicmp (__sz1, __sz2, __sizeMaxCompare); }
//  __CRT_INLINE int __cdecl strcasecmp (const char *__sz1, const char *__sz2) { return _stricmp (__sz1, __sz2); }
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_strnset) char *__cdecl strnset(char *_Str,int _Val,size_t _MaxCount);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_strrev) char *__cdecl strrev(char *_Str);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_strset) char *__cdecl strset(char *_Str,int _Val);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_strupr) char *__cdecl strupr(char *_Str);
#endif

#ifndef _WSTRING_DEFINED
#define _WSTRING_DEFINED

  _CRTIMP wchar_t *__cdecl _wcsdup(const wchar_t *_Str);
  _CRTIMP wchar_t *__cdecl wcscat(wchar_t *_Dest,const wchar_t *_Source);
  _CRTIMP _CONST_RETURN wchar_t *__cdecl wcschr(const wchar_t *_Str,wchar_t _Ch);
  _CRTIMP int __cdecl wcscmp(const wchar_t *_Str1,const wchar_t *_Str2);
  _CRTIMP wchar_t *__cdecl wcscpy(wchar_t *_Dest,const wchar_t *_Source);
  _CRTIMP size_t __cdecl wcscspn(const wchar_t *_Str,const wchar_t *_Control);
  _CRTIMP size_t __cdecl wcslen(const wchar_t *_Str);
  _CRTIMP size_t __cdecl wcsnlen(const wchar_t *_Src,size_t _MaxCount);
  _CRTIMP wchar_t *wcsncat(wchar_t *_Dest,const wchar_t *_Source,size_t _Count);
  _CRTIMP int __cdecl wcsncmp(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount);
  _CRTIMP wchar_t *wcsncpy(wchar_t *_Dest,const wchar_t *_Source,size_t _Count);
  _CRTIMP _CONST_RETURN wchar_t *__cdecl wcspbrk(const wchar_t *_Str,const wchar_t *_Control);
  _CRTIMP _CONST_RETURN wchar_t *__cdecl wcsrchr(const wchar_t *_Str,wchar_t _Ch);
  _CRTIMP size_t __cdecl wcsspn(const wchar_t *_Str,const wchar_t *_Control);
  _CRTIMP _CONST_RETURN wchar_t *__cdecl wcsstr(const wchar_t *_Str,const wchar_t *_SubStr);
  _CRTIMP _CRT_INSECURE_DEPRECATE_CORE(wcstok_s) wchar_t *__cdecl wcstok(wchar_t *_Str,const wchar_t *_Delim);
  _CRTIMP _CRT_INSECURE_DEPRECATE(_wcserror_s) wchar_t *__cdecl _wcserror(int _ErrNum);
  _CRTIMP _CRT_INSECURE_DEPRECATE(__wcserror_s) wchar_t *__cdecl __wcserror(const wchar_t *_Str);
  _CRTIMP int __cdecl _wcsicmp(const wchar_t *_Str1,const wchar_t *_Str2);
  _CRTIMP int __cdecl _wcsicmp_l(const wchar_t *_Str1,const wchar_t *_Str2,_locale_t _Locale);
  _CRTIMP int __cdecl _wcsnicmp(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount);
  _CRTIMP int __cdecl _wcsnicmp_l(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount,_locale_t _Locale);
  _CRTIMP _CRT_INSECURE_DEPRECATE_CORE(_wcsnset_s) wchar_t *__cdecl _wcsnset(wchar_t *_Str,wchar_t _Val,size_t _MaxCount);
  _CRTIMP wchar_t *__cdecl _wcsrev(wchar_t *_Str);
  _CRTIMP _CRT_INSECURE_DEPRECATE_CORE(_wcsset_s) wchar_t *__cdecl _wcsset(wchar_t *_Str,wchar_t _Val);
  _CRTIMP wchar_t *__cdecl _wcslwr(wchar_t *_String);
  _CRTIMP wchar_t *_wcslwr_l(wchar_t *_String,_locale_t _Locale);
  _CRTIMP wchar_t *__cdecl _wcsupr(wchar_t *_String);
  _CRTIMP wchar_t *_wcsupr_l(wchar_t *_String,_locale_t _Locale);
  _CRTIMP size_t __cdecl wcsxfrm(wchar_t *_Dst,const wchar_t *_Src,size_t _MaxCount);
  _CRTIMP size_t __cdecl _wcsxfrm_l(wchar_t *_Dst,const wchar_t *_Src,size_t _MaxCount,_locale_t _Locale);
  _CRTIMP int __cdecl wcscoll(const wchar_t *_Str1,const wchar_t *_Str2);
  _CRTIMP int __cdecl _wcscoll_l(const wchar_t *_Str1,const wchar_t *_Str2,_locale_t _Locale);
  _CRTIMP int __cdecl _wcsicoll(const wchar_t *_Str1,const wchar_t *_Str2);
  _CRTIMP int __cdecl _wcsicoll_l(const wchar_t *_Str1,const wchar_t *_Str2,_locale_t _Locale);
  _CRTIMP int __cdecl _wcsncoll(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount);
  _CRTIMP int __cdecl _wcsncoll_l(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount,_locale_t _Locale);
  _CRTIMP int __cdecl _wcsnicoll(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount);
  _CRTIMP int __cdecl _wcsnicoll_l(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount,_locale_t _Locale);

#ifndef	NO_OLDNAMES
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_wcsdup) wchar_t *__cdecl wcsdup(const wchar_t *_Str);
#define wcswcs wcsstr
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_wcsicmp) int __cdecl wcsicmp(const wchar_t *_Str1,const wchar_t *_Str2);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_wcsnicmp) int __cdecl wcsnicmp(const wchar_t *_Str1,const wchar_t *_Str2,size_t _MaxCount);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_wcsnset) wchar_t *__cdecl wcsnset(wchar_t *_Str,wchar_t _Val,size_t _MaxCount);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_wcsrev) wchar_t *__cdecl wcsrev(wchar_t *_Str);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_wcsset) wchar_t *__cdecl wcsset(wchar_t *_Str,wchar_t _Val);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_wcslwr) wchar_t *__cdecl wcslwr(wchar_t *_Str);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_wcsupr) wchar_t *__cdecl wcsupr(wchar_t *_Str);
  _CRTIMP _CRT_NONSTDC_DEPRECATE(_wcsicoll) int __cdecl wcsicoll(const wchar_t *_Str1,const wchar_t *_Str2);
#endif

#endif /* !_WSTRING_DEFINED */

#ifdef __cplusplus
}
#endif

#include <sec_api/string_s.h>
#endif
