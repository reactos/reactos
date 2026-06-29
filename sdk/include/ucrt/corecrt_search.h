//
// corecrt_search.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Declarations of functions for sorting and searching.  These declarations are
// split out so that they may be included by both <stdlib.h> and <search.h>.
// <stdlib.h> does not include <search.h> to avoid introducing conflicts with
// other user headers named <search.h>.
//
#pragma once

#include <corecrt.h>
#include <stddef.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER


    typedef int (__cdecl* _CoreCrtSecureSearchSortCompareFunction)(void*, void const*, void const*);
    typedef int (__cdecl* _CoreCrtNonSecureSearchSortCompareFunction)(void const*, void const*);


#if __STDC_WANT_SECURE_LIB__

    _Check_return_
    _ACRTIMP void* __cdecl bsearch_s(
        _In_                                               void const* _Key,
        _In_reads_bytes_(_NumOfElements * _SizeOfElements) void const* _Base,
        _In_                                               rsize_t     _NumOfElements,
        _In_                                               rsize_t     _SizeOfElements,
        _In_                   _CoreCrtSecureSearchSortCompareFunction _CompareFunction,
        _In_opt_                                           void*       _Context
        );

    _ACRTIMP void __cdecl qsort_s(
        _Inout_updates_bytes_(_NumOfElements * _SizeOfElements) void*   _Base,
        _In_                                                    rsize_t _NumOfElements,
        _In_                                                    rsize_t _SizeOfElements,
        _In_                    _CoreCrtSecureSearchSortCompareFunction _CompareFunction,
        _In_opt_                                                void*   _Context
        );

#endif // __STDC_WANT_SECURE_LIB__



_Check_return_
_ACRTIMP void* __cdecl bsearch(
    _In_                                               void const* _Key,
    _In_reads_bytes_(_NumOfElements * _SizeOfElements) void const* _Base,
    _In_                                               size_t      _NumOfElements,
    _In_                                               size_t      _SizeOfElements,
    _In_                _CoreCrtNonSecureSearchSortCompareFunction _CompareFunction
    );

_ACRTIMP void __cdecl qsort(
    _Inout_updates_bytes_(_NumOfElements * _SizeOfElements) void*  _Base,
    _In_                                                    size_t _NumOfElements,
    _In_                                                    size_t _SizeOfElements,
    _In_                _CoreCrtNonSecureSearchSortCompareFunction _CompareFunction
    );

_Check_return_
_ACRTIMP void* __cdecl _lfind_s(
    _In_                                                  void const*   _Key,
    _In_reads_bytes_((*_NumOfElements) * _SizeOfElements) void const*   _Base,
    _Inout_                                               unsigned int* _NumOfElements,
    _In_                                                  size_t        _SizeOfElements,
    _In_                        _CoreCrtSecureSearchSortCompareFunction _CompareFunction,
    _In_                                                  void*         _Context
    );

_Check_return_
_ACRTIMP void* __cdecl _lfind(
    _In_                                                  void const*   _Key,
    _In_reads_bytes_((*_NumOfElements) * _SizeOfElements) void const*   _Base,
    _Inout_                                               unsigned int* _NumOfElements,
    _In_                                                  unsigned int  _SizeOfElements,
    _In_                     _CoreCrtNonSecureSearchSortCompareFunction _CompareFunction
    );

_Check_return_
_ACRTIMP void* __cdecl _lsearch_s(
    _In_                                                        void const*   _Key,
    _Inout_updates_bytes_((*_NumOfElements ) * _SizeOfElements) void*         _Base,
    _Inout_                                                     unsigned int* _NumOfElements,
    _In_                                                        size_t        _SizeOfElements,
    _In_                              _CoreCrtSecureSearchSortCompareFunction _CompareFunction,
    _In_                                                        void*         _Context
    );

_Check_return_
_ACRTIMP void* __cdecl _lsearch(
    _In_                                                        void const*   _Key,
    _Inout_updates_bytes_((*_NumOfElements ) * _SizeOfElements) void*         _Base,
    _Inout_                                                     unsigned int* _NumOfElements,
    _In_                                                        unsigned int  _SizeOfElements,
    _In_                           _CoreCrtNonSecureSearchSortCompareFunction _CompareFunction
    );



// Managed search routines
#if defined __cplusplus && defined _M_CEE
extern "C++"
{
    typedef int (__clrcall* _CoreCrtMgdSecureSearchSortCompareFunction)(void*, void const*, void const*);
    typedef int (__clrcall* _CoreCrtMgdNonSecureSearchSortCompareFunction)(void const*, void const*);

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_
        void* __clrcall bsearch_s(
                _In_                                               void const* _Key,
                _In_reads_bytes_(_NumOfElements * _SizeOfElements) void const* _Base,
                _In_                                               rsize_t     _NumOfElements,
                _In_                                               rsize_t     _SizeOfElements,
                _In_                _CoreCrtMgdSecureSearchSortCompareFunction _CompareFunction,
                _In_                                               void*       _Context);

        void __clrcall qsort_s(
                _Inout_updates_bytes_(_NumOfElements * _SizeOfElements) void*   _Base,
                _In_                                                    rsize_t _NumOfElements,
                _In_                                                    rsize_t _SizeOfElements,
                _In_                 _CoreCrtMgdSecureSearchSortCompareFunction _CompareFunction,
                _In_                                                    void*   _Context);

    #endif // __STDC_WANT_SECURE_LIB__

    _Check_return_
    void* __clrcall bsearch(
        _In_                                               void const* _Key,
        _In_reads_bytes_(_NumOfElements * _SizeOfElements) void const* _Base,
        _In_                                               size_t _NumOfElements,
        _In_                                               size_t _SizeOfElements,
        _In_        _CoreCrtMgdNonSecureSearchSortCompareFunction _CompareFunction
        );

    _Check_return_
    void* __clrcall _lfind_s(
        _In_                                               void const*   _Key,
        _In_reads_bytes_(_NumOfElements * _SizeOfElements) void const*   _Base,
        _Inout_                                            unsigned int* _NumOfElements,
        _In_                                               size_t        _SizeOfElements,
        _In_                  _CoreCrtMgdSecureSearchSortCompareFunction _CompareFunction,
        _In_                                               void*         _Context
        );

    _Check_return_
    void* __clrcall _lfind(
        _In_                                                  void const*   _Key,
        _In_reads_bytes_((*_NumOfElements) * _SizeOfElements) void const*   _Base,
        _Inout_                                               unsigned int* _NumOfElements,
        _In_                                                  unsigned int  _SizeOfElements,
        _In_                  _CoreCrtMgdNonSecureSearchSortCompareFunction _CompareFunction
        );

    _Check_return_
    void* __clrcall _lsearch_s(
        _In_                                                  void const*   _Key,
        _In_reads_bytes_((*_NumOfElements) * _SizeOfElements) void*         _Base,
        _In_                                                  unsigned int* _NumOfElements,
        _In_                                                  size_t        _SizeOfElements,
        _In_                     _CoreCrtMgdSecureSearchSortCompareFunction _CompareFunction,
        _In_                                                  void*         _Context
        );

    _Check_return_
    void* __clrcall _lsearch(
        _In_                                                       void const*   _Key,
        _Inout_updates_bytes_((*_NumOfElements) * _SizeOfElements) void*         _Base,
        _Inout_                                                    unsigned int* _NumOfElements,
        _In_                                                       unsigned int  _SizeOfElements,
        _In_                       _CoreCrtMgdNonSecureSearchSortCompareFunction _CompareFunction
        );

    void __clrcall qsort(
        _Inout_updates_bytes_(_NumOfElements * _SizeOfElements) void*  _Base,
        _In_                                                    size_t _NumOfElements,
        _In_                                                    size_t _SizeOfElements,
        _In_             _CoreCrtMgdNonSecureSearchSortCompareFunction _CompareFunction
        );
}
#endif // defined __cplusplus && defined _M_CEE



#if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES

    _Check_return_ _CRT_NONSTDC_DEPRECATE(_lfind)
    _ACRTIMP void* __cdecl lfind(
        _In_                                                  void const*   _Key,
        _In_reads_bytes_((*_NumOfElements) * _SizeOfElements) void const*   _Base,
        _Inout_                                               unsigned int* _NumOfElements,
        _In_                                                  unsigned int  _SizeOfElements,
        _In_                     _CoreCrtNonSecureSearchSortCompareFunction _CompareFunction
        );

    _Check_return_ _CRT_NONSTDC_DEPRECATE(_lsearch)
    _ACRTIMP void* __cdecl lsearch(
        _In_                                                       void const*   _Key,
        _Inout_updates_bytes_((*_NumOfElements) * _SizeOfElements) void*         _Base,
        _Inout_                                                    unsigned int* _NumOfElements,
        _In_                                                       unsigned int  _SizeOfElements,
        _In_                          _CoreCrtNonSecureSearchSortCompareFunction _CompareFunction
        );

#endif // _CRT_INTERNAL_NONSTDC_NAMES



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
