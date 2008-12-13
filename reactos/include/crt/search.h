/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_SEARCH
#define _INC_SEARCH

#include <_mingw.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CRT_ALGO_DEFINED
#define _CRT_ALGO_DEFINED
  void *__cdecl bsearch(const void *_Key,const void *_Base,size_t _NumOfElements,size_t _SizeOfElements,int (__cdecl *_PtFuncCompare)(const void *,const void *));
  void __cdecl qsort(void *_Base,size_t _NumOfElements,size_t _SizeOfElements,int (__cdecl *_PtFuncCompare)(const void *,const void *));
#endif
  _CRTIMP void *__cdecl _lfind(const void *_Key,const void *_Base,unsigned int *_NumOfElements,unsigned int _SizeOfElements,int (__cdecl *_PtFuncCompare)(const void *,const void *));
  _CRTIMP void *__cdecl _lsearch(const void *_Key,void *_Base,unsigned int *_NumOfElements,unsigned int _SizeOfElements,int (__cdecl *_PtFuncCompare)(const void *,const void *));

#ifndef	NO_OLDNAMES
  void *__cdecl lfind(const void *_Key,const void *_Base,unsigned int *_NumOfElements,unsigned int _SizeOfElements,int (__cdecl *_PtFuncCompare)(const void *,const void *));
  void *__cdecl lsearch(const void *_Key,void *_Base,unsigned int *_NumOfElements,unsigned int _SizeOfElements,int (__cdecl *_PtFuncCompare)(const void *,const void *));
#endif

#ifdef __cplusplus
}
#endif

#include <sec_api/search_s.h>

#endif
