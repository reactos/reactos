//
// corecrt_malloc.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The memory allocation library.  These pieces of the allocation library are
// shared by both <stdlib.h> and <malloc.h>.
//
#pragma once

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



#if defined _DEBUG && defined _CRTDBG_MAP_ALLOC
    #pragma push_macro("_aligned_free")
    #pragma push_macro("_aligned_malloc")
    #pragma push_macro("_aligned_msize")
    #pragma push_macro("_aligned_offset_malloc")
    #pragma push_macro("_aligned_offset_realloc")
    #pragma push_macro("_aligned_offset_recalloc")
    #pragma push_macro("_aligned_realloc")
    #pragma push_macro("_aligned_recalloc")
    #pragma push_macro("_expand")
    #pragma push_macro("_freea")
    #pragma push_macro("_msize")
    #pragma push_macro("_recalloc")
    #pragma push_macro("calloc")
    #pragma push_macro("free")
    #pragma push_macro("malloc")
    #pragma push_macro("realloc")

    #undef _aligned_free
    #undef _aligned_malloc
    #undef _aligned_msize
    #undef _aligned_offset_malloc
    #undef _aligned_offset_realloc
    #undef _aligned_offset_recalloc
    #undef _aligned_realloc
    #undef _aligned_recalloc
    #undef _expand
    #undef _freea
    #undef _msize
    #undef _recalloc
    #undef calloc
    #undef free
    #undef malloc
    #undef realloc
#endif

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count * _Size)
_ACRTIMP _CRTALLOCATOR _CRTRESTRICT
void* __cdecl _calloc_base(
    _In_ size_t _Count,
    _In_ size_t _Size
    );

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count * _Size)
_ACRTIMP _CRT_JIT_INTRINSIC _CRTALLOCATOR _CRTRESTRICT _CRT_HYBRIDPATCHABLE
void* __cdecl calloc(
    _In_ _CRT_GUARDOVERFLOW size_t _Count,
    _In_ _CRT_GUARDOVERFLOW size_t _Size
    );

_Check_return_
_ACRTIMP int __cdecl _callnewh(
    _In_ size_t _Size
    );

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
_ACRTIMP _CRTALLOCATOR _CRT_HYBRIDPATCHABLE
void* __cdecl _expand(
    _Pre_notnull_           void*  _Block,
    _In_ _CRT_GUARDOVERFLOW size_t _Size
    );

_ACRTIMP
void __cdecl _free_base(
    _Pre_maybenull_ _Post_invalid_ void* _Block
    );

_ACRTIMP _CRT_HYBRIDPATCHABLE
void __cdecl free(
    _Pre_maybenull_ _Post_invalid_ void* _Block
    );

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
_ACRTIMP _CRTALLOCATOR _CRTRESTRICT
void* __cdecl _malloc_base(
    _In_ size_t _Size
    );

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
_ACRTIMP _CRTALLOCATOR _CRT_JIT_INTRINSIC _CRTRESTRICT _CRT_HYBRIDPATCHABLE
void* __cdecl malloc(
    _In_ _CRT_GUARDOVERFLOW size_t _Size
    );

_Check_return_
_ACRTIMP
size_t __cdecl _msize_base(
    _Pre_notnull_ void* _Block
    ) _CRT_NOEXCEPT;

_Check_return_
_ACRTIMP _CRT_HYBRIDPATCHABLE
size_t __cdecl _msize(
    _Pre_notnull_ void* _Block
    );

_Success_(return != 0) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
_ACRTIMP _CRTALLOCATOR _CRTRESTRICT
void* __cdecl _realloc_base(
    _Pre_maybenull_ _Post_invalid_  void*  _Block,
    _In_                            size_t _Size
    );

_Success_(return != 0) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
_ACRTIMP _CRTALLOCATOR _CRTRESTRICT _CRT_HYBRIDPATCHABLE
void* __cdecl realloc(
    _Pre_maybenull_ _Post_invalid_ void*  _Block,
    _In_ _CRT_GUARDOVERFLOW        size_t _Size
    );

_Success_(return != 0) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count * _Size)
_ACRTIMP _CRTALLOCATOR _CRTRESTRICT
void* __cdecl _recalloc_base(
    _Pre_maybenull_ _Post_invalid_ void*  _Block,
    _In_                           size_t _Count,
    _In_                           size_t _Size
    );

_Success_(return != 0) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count * _Size)
_ACRTIMP _CRTALLOCATOR _CRTRESTRICT
void* __cdecl _recalloc(
    _Pre_maybenull_ _Post_invalid_ void*  _Block,
    _In_ _CRT_GUARDOVERFLOW        size_t _Count,
    _In_ _CRT_GUARDOVERFLOW        size_t _Size
    );

_ACRTIMP
void __cdecl _aligned_free(
    _Pre_maybenull_ _Post_invalid_ void* _Block
    );

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
_ACRTIMP _CRTALLOCATOR _CRTRESTRICT
void* __cdecl _aligned_malloc(
    _In_ _CRT_GUARDOVERFLOW size_t _Size,
    _In_                    size_t _Alignment
    );

_Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
_ACRTIMP _CRTALLOCATOR _CRTRESTRICT
void* __cdecl _aligned_offset_malloc(
    _In_ _CRT_GUARDOVERFLOW size_t _Size,
    _In_                    size_t _Alignment,
    _In_                    size_t _Offset
    );

_Check_return_
_ACRTIMP
size_t __cdecl _aligned_msize(
    _Pre_notnull_ void*  _Block,
    _In_          size_t _Alignment,
    _In_          size_t _Offset
    );

_Success_(return != 0) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
_ACRTIMP _CRTALLOCATOR _CRTRESTRICT
void* __cdecl _aligned_offset_realloc(
    _Pre_maybenull_ _Post_invalid_ void*  _Block,
    _In_ _CRT_GUARDOVERFLOW        size_t _Size,
    _In_                           size_t _Alignment,
    _In_                           size_t _Offset
    );

_Success_(return != 0) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count * _Size)
_ACRTIMP _CRTALLOCATOR _CRTRESTRICT
void* __cdecl _aligned_offset_recalloc(
    _Pre_maybenull_ _Post_invalid_ void*  _Block,
    _In_ _CRT_GUARDOVERFLOW        size_t _Count,
    _In_ _CRT_GUARDOVERFLOW        size_t _Size,
    _In_                           size_t _Alignment,
    _In_                           size_t _Offset
    );

_Success_(return != 0) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
_ACRTIMP _CRTALLOCATOR _CRTRESTRICT
void* __cdecl _aligned_realloc(
    _Pre_maybenull_ _Post_invalid_ void*  _Block,
    _In_ _CRT_GUARDOVERFLOW        size_t _Size,
    _In_                           size_t _Alignment
    );

_Success_(return != 0) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count * _Size)
_ACRTIMP _CRTALLOCATOR _CRTRESTRICT
void* __cdecl _aligned_recalloc(
    _Pre_maybenull_ _Post_invalid_ void*  _Block,
    _In_ _CRT_GUARDOVERFLOW        size_t _Count,
    _In_ _CRT_GUARDOVERFLOW        size_t _Size,
    _In_                           size_t _Alignment
    );

#if defined _DEBUG && defined _CRTDBG_MAP_ALLOC
    #pragma pop_macro("realloc")
    #pragma pop_macro("malloc")
    #pragma pop_macro("free")
    #pragma pop_macro("calloc")
    #pragma pop_macro("_recalloc")
    #pragma pop_macro("_msize")
    #pragma pop_macro("_freea")
    #pragma pop_macro("_expand")
    #pragma pop_macro("_aligned_recalloc")
    #pragma pop_macro("_aligned_realloc")
    #pragma pop_macro("_aligned_offset_recalloc")
    #pragma pop_macro("_aligned_offset_realloc")
    #pragma pop_macro("_aligned_offset_malloc")
    #pragma pop_macro("_aligned_msize")
    #pragma pop_macro("_aligned_malloc")
    #pragma pop_macro("_aligned_free")
#endif



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
