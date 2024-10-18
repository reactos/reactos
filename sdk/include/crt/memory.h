/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_MEMORY
#define _INC_MEMORY

#include <corecrt.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CONST_RETURN
#define _CONST_RETURN
#endif

#define _WConst_return _CONST_RETURN

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

  _Check_return_
  _CONST_RETURN
  void*
  __cdecl
  memchr(
    _In_reads_bytes_opt_(_MaxCount) const void *_Buf,
    _In_ int _Val,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _memicmp(
    _In_reads_bytes_opt_(_Size) const void *_Buf1,
    _In_reads_bytes_opt_(_Size) const void *_Buf2,
    _In_ size_t _Size);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _memicmp_l(
    _In_reads_bytes_opt_(_Size) const void *_Buf1,
    _In_reads_bytes_opt_(_Size) const void *_Buf2,
    _In_ size_t _Size,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  int
  __cdecl
  memcmp(
    _In_reads_bytes_opt_(_Size) const void *_Buf1,
    _In_reads_bytes_opt_(_Size) const void *_Buf2,
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
  void*
  __cdecl
  memccpy(
    _Out_writes_bytes_opt_(_Size) void *_Dst,
    _In_reads_bytes_opt_(_Size) const void *_Src,
    _In_ int _Val,
    _In_ size_t _Size);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  memicmp(
    _In_reads_bytes_opt_(_Size) const void *_Buf1,
    _In_reads_bytes_opt_(_Size) const void *_Buf2,
    _In_ size_t _Size);

#endif /* NO_OLDNAMES */

#endif /* _CRT_MEMORY_DEFINED */

#ifdef __cplusplus
}
#endif

#endif /* _INC_MEMORY */
