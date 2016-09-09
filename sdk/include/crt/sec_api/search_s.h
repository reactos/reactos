/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _INC_SEARCH_S
#define _INC_SEARCH_S

#include <search.h>

#if defined(MINGW_HAS_SECURE_API)

#ifdef __cplusplus
extern "C" {
#endif

  _Check_return_
  _CRTIMP
  void *
  __cdecl
  _lfind_s(
    _In_ const void *_Key,
    _In_reads_bytes_((*_NumOfElements) * _SizeOfElements) const void *_Base,
    _Inout_ unsigned int *_NumOfElements,
    _In_ size_t _SizeOfElements,
    _In_ int(__cdecl *_PtFuncCompare)(void *, const void *, const void *),
    void *_Context);

  _Check_return_
  _CRTIMP
  void *
  __cdecl
  _lsearch_s(
    _In_ const void *_Key,
    _Inout_updates_bytes_((*_NumOfElements) * _SizeOfElements) void *_Base,
    _Inout_ unsigned int *_NumOfElements,
    _In_ size_t _SizeOfElements,
    _In_ int(__cdecl *_PtFuncCompare)(void *, const void *, const void *),
    void *_Context);

#ifdef __cplusplus
}
#endif

#endif

#endif /* _INC_SEARCH_S */
