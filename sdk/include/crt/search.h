/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _INC_SEARCH
#define _INC_SEARCH

#include <corecrt.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CRT_ALGO_DEFINED
#define _CRT_ALGO_DEFINED

  _Check_return_
  void *
  __cdecl
  bsearch(
    _In_ const void *_Key,
    _In_reads_bytes_(_NumOfElements * _SizeOfElements) const void *_Base,
    _In_ size_t _NumOfElements,
    _In_ size_t _SizeOfElements,
    _In_ int(__cdecl *_PtFuncCompare)(const void *, const void *));

  void
  __cdecl
  qsort(
    _Inout_updates_bytes_(_NumOfElements * _SizeOfElements) void *_Base,
    _In_ size_t _NumOfElements,
    _In_ size_t _SizeOfElements,
    _In_ int(__cdecl *_PtFuncCompare)(const void *, const void *));

#endif

  _Check_return_
  _CRTIMP
  void *
  __cdecl
  _lfind(
    _In_ const void *_Key,
    _In_reads_bytes_((*_NumOfElements) * _SizeOfElements) const void *_Base,
    _Inout_ unsigned int *_NumOfElements,
    _In_ unsigned int _SizeOfElements,
    _In_ int(__cdecl *_PtFuncCompare)(const void *, const void *));

  _Check_return_
  _CRTIMP
  void *
  __cdecl
  _lsearch(
    _In_ const void *_Key,
    _Inout_updates_bytes_((*_NumOfElements) * _SizeOfElements) void *_Base,
    _Inout_ unsigned int *_NumOfElements,
    _In_ unsigned int _SizeOfElements,
    _In_ int(__cdecl *_PtFuncCompare)(const void *, const void *));

#ifndef NO_OLDNAMES

  _Check_return_
  _CRTIMP
  void *
  __cdecl
  lfind(
    _In_ const void *_Key,
    _In_reads_bytes_((*_NumOfElements) * _SizeOfElements) const void *_Base,
    _Inout_ unsigned int *_NumOfElements,
    _In_ unsigned int _SizeOfElements,
    _In_ int(__cdecl *_PtFuncCompare)(const void *, const void *));

  _Check_return_
  _CRTIMP
  void *
  __cdecl
  lsearch(
    _In_ const void *_Key,
    _Inout_updates_bytes_((*_NumOfElements) * _SizeOfElements) void *_Base,
    _Inout_ unsigned int *_NumOfElements,
    _In_ unsigned int _SizeOfElements,
    _In_ int(__cdecl *_PtFuncCompare)(const void *, const void *));

#endif

#ifdef __cplusplus
}
#endif

#include <sec_api/search_s.h>

#endif /*_INC_SEARCH */
