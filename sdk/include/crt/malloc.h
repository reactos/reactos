/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_MALLOC
#define _INC_MALLOC

#include <corecrt.h>

#pragma pack(push,_CRT_PACKING)

#ifndef _MM_MALLOC_H_INCLUDED
#define _MM_MALLOC_H_INCLUDED
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN64
#define _HEAP_MAXREQ 0xFFFFFFFFFFFFFFE0
#else
#define _HEAP_MAXREQ 0xFFFFFFE0
#endif

/* Return codes for _heapwalk()  */
#define _HEAPEMPTY (-1)
#define _HEAPOK (-2)
#define _HEAPBADBEGIN (-3)
#define _HEAPBADNODE (-4)
#define _HEAPEND (-5)
#define _HEAPBADPTR (-6)

/* Values for _heapinfo.useflag */
#define _FREEENTRY 0
#define _USEDENTRY 1

#ifndef _HEAPINFO_DEFINED
#define _HEAPINFO_DEFINED
 /* The structure used to walk through the heap with _heapwalk.  */
  typedef struct _heapinfo {
    int *_pentry;
    size_t _size;
    int _useflag;
  } _HEAPINFO;
#endif

  extern unsigned int _amblksiz;

/* Make sure that X86intrin.h doesn't produce here collisions.  */
#if (!defined (_XMMINTRIN_H_INCLUDED) && !defined (_MM_MALLOC_H_INCLUDED)) || defined(_aligned_malloc)
#define __DO_ALIGN_DEFINES
#endif

#ifdef __DO_ALIGN_DEFINES
#pragma push_macro("_aligned_free")
#pragma push_macro("_aligned_malloc")
#undef _aligned_free
#undef _aligned_malloc
#endif

#define _mm_free(a) _aligned_free(a)
#define _mm_malloc(a,b) _aligned_malloc(a,b)

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

  _Check_return_
  _Ret_maybenull_
  _Post_writable_byte_size_(_NewSize)
  void*
  __cdecl
  realloc(
    _Pre_maybenull_ _Post_invalid_ void *_Memory,
    _In_ size_t _NewSize);

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

#ifdef __DO_ALIGN_DEFINES

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

#endif /* __DO_ALIGN_DEFINES */

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

#ifdef __DO_ALIGN_DEFINES
#undef __DO_ALIGN_DEFINES

#pragma pop_macro("_aligned_malloc")
#pragma pop_macro("_aligned_free")

#endif

#define _MAX_WAIT_MALLOC_CRT 60000

  _CRTIMP int __cdecl _resetstkoflw (void);

  _CRTIMP
  unsigned long
  __cdecl
  _set_malloc_crt_max_wait(
    _In_ unsigned long _NewValue);

  _Check_return_
  _Ret_maybenull_
  _Post_writable_byte_size_(_NewSize)
  _CRTIMP
  void*
  __cdecl
  _expand(
    _In_opt_ void *_Memory,
    _In_ size_t _NewSize);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _msize(
    _In_ void *_Memory);

#ifdef __GNUC__
#undef _alloca
#define _alloca(x) __builtin_alloca((x))
#else
  _Ret_notnull_
  _Post_writable_byte_size_(_Size)
  void*
  __cdecl
  _alloca(
    _In_ size_t _Size);
#endif

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _get_sbh_threshold(void);

  _CRTIMP
  int
  __cdecl
  _set_sbh_threshold(
    _In_ size_t _NewValue);

  _CRTIMP
  errno_t
  __cdecl
  _set_amblksiz(
    _In_ size_t _Value);

  _CRTIMP
  errno_t
  __cdecl
  _get_amblksiz(
    _Out_ size_t *_Value);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _heapadd(
    _In_ void *_Memory,
    _In_ size_t _Size);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _heapchk(void);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _heapmin(void);

  _CRTIMP
  int
  __cdecl
  _heapset(
    _In_ unsigned int _Fill);

  _CRTIMP
  int
  __cdecl
  _heapwalk(
    _Inout_ _HEAPINFO *_EntryInfo);

  _CRTIMP
  size_t
  __cdecl
  _heapused(
    size_t *_Used,
    size_t *_Commit);

  _CRTIMP
  intptr_t
  __cdecl
  _get_heap_handle(void);

#define _ALLOCA_S_THRESHOLD 1024
#define _ALLOCA_S_STACK_MARKER 0xCCCC
#define _ALLOCA_S_HEAP_MARKER 0xDDDD

#if(defined(_X86_) && !defined(__x86_64))
#define _ALLOCA_S_MARKER_SIZE 8
#elif defined(__ia64__) || defined(__x86_64) || defined(__arm64__)
#define _ALLOCA_S_MARKER_SIZE 16
#elif defined(__arm__)
#define _ALLOCA_S_MARKER_SIZE 8
#endif

#if !defined(RC_INVOKED)
  static __inline void *_MarkAllocaS(void *_Ptr,unsigned int _Marker) {
    if(_Ptr) {
      *((unsigned int*)_Ptr) = _Marker;
      _Ptr = (char*)_Ptr + _ALLOCA_S_MARKER_SIZE;
    }
    return _Ptr;
  }
#endif

#undef _malloca
#define _malloca(size) \
  ((((size) + _ALLOCA_S_MARKER_SIZE) <= _ALLOCA_S_THRESHOLD) ? \
    _MarkAllocaS(_alloca((size) + _ALLOCA_S_MARKER_SIZE),_ALLOCA_S_STACK_MARKER) : \
    _MarkAllocaS(malloc((size) + _ALLOCA_S_MARKER_SIZE),_ALLOCA_S_HEAP_MARKER))
#undef _FREEA_INLINE
#define _FREEA_INLINE

#ifndef RC_INVOKED
#undef _freea
  static __inline void __cdecl _freea(void *_Memory) {
    unsigned int _Marker;
    if(_Memory) {
      _Memory = (char*)_Memory - _ALLOCA_S_MARKER_SIZE;
      _Marker = *(unsigned int *)_Memory;
      if(_Marker==_ALLOCA_S_HEAP_MARKER) {
	free(_Memory);
      }
#ifdef _ASSERTE
      else if(_Marker!=_ALLOCA_S_STACK_MARKER) {
	_ASSERTE(("Corrupted pointer passed to _freea",0));
      }
#endif
    }
  }
#endif /* RC_INVOKED */

#ifndef	NO_OLDNAMES
#define alloca _alloca
#endif

#ifdef HEAPHOOK
#ifndef _HEAPHOOK_DEFINED
#define _HEAPHOOK_DEFINED
  typedef int (__cdecl *_HEAPHOOK)(int,size_t,void *,void **);
#endif

  _CRTIMP
  _HEAPHOOK
  __cdecl
  _setheaphook(
    _In_opt_ _HEAPHOOK _NewHook);

#define _HEAP_MALLOC  1
#define _HEAP_CALLOC  2
#define _HEAP_FREE    3
#define _HEAP_REALLOC 4
#define _HEAP_MSIZE   5
#define _HEAP_EXPAND  6
#endif

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif /* _INC_MALLOC */
