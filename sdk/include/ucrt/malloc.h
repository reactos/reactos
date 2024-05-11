//
// malloc.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The memory allocation library.
//
#pragma once
#ifndef _INC_MALLOC // include guard for 3rd party interop
#define _INC_MALLOC

#include <corecrt.h>
#include <corecrt_malloc.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



// Maximum heap request the heap manager will attempt
#ifdef _WIN64
    #define _HEAP_MAXREQ 0xFFFFFFFFFFFFFFE0
#else
    #define _HEAP_MAXREQ 0xFFFFFFE0
#endif



// Constants for _heapchk and _heapwalk routines
#define _HEAPEMPTY    (-1)
#define _HEAPOK       (-2)
#define _HEAPBADBEGIN (-3)
#define _HEAPBADNODE  (-4)
#define _HEAPEND      (-5)
#define _HEAPBADPTR   (-6)
#define _FREEENTRY      0
#define _USEDENTRY      1

typedef struct _heapinfo
{
    int* _pentry;
    size_t _size;
    int _useflag;
} _HEAPINFO;

#define _mm_free(a)      _aligned_free(a)
#define _mm_malloc(a, b) _aligned_malloc(a, b)



_Ret_notnull_ _Post_writable_byte_size_(_Size)
void* __cdecl _alloca(_In_ size_t _Size);



#if !defined __midl && !defined RC_INVOKED

    _ACRTIMP intptr_t __cdecl _get_heap_handle(void);

    _Check_return_
    _DCRTIMP int __cdecl _heapmin(void);

    #if defined _DEBUG || defined _CRT_USE_WINAPI_FAMILY_DESKTOP_APP || defined _CORECRT_BUILD
        _ACRTIMP int __cdecl _heapwalk(_Inout_ _HEAPINFO* _EntryInfo);
    #endif

    #if defined _CRT_USE_WINAPI_FAMILY_DESKTOP_APP || defined _CRT_USE_WINAPI_FAMILY_GAMES
        _Check_return_ _DCRTIMP int __cdecl _heapchk(void);
    #endif

    _DCRTIMP int __cdecl _resetstkoflw(void);

    #define _ALLOCA_S_THRESHOLD     1024
    #define _ALLOCA_S_STACK_MARKER  0xCCCC
    #define _ALLOCA_S_HEAP_MARKER   0xDDDD

    #ifdef _WIN64
        #define _ALLOCA_S_MARKER_SIZE 16
    #else
        #define _ALLOCA_S_MARKER_SIZE 8
    #endif

    _STATIC_ASSERT(sizeof(unsigned int) <= _ALLOCA_S_MARKER_SIZE);


    #pragma warning(push)
    #pragma warning(disable: 6540) // C6540: attribute annotations on this function will invalidate all
                                   // of its existing __declspec annotations

    __inline void* _MarkAllocaS(_Out_opt_ __crt_typefix(unsigned int*) void* _Ptr, unsigned int _Marker)
    {
        if (_Ptr)
        {
            *((unsigned int*)_Ptr) = _Marker;
            _Ptr = (char*)_Ptr + _ALLOCA_S_MARKER_SIZE;
        }
        return _Ptr;
    }

    __inline size_t _MallocaComputeSize(size_t _Size)
    {
        size_t _MarkedSize = _Size + _ALLOCA_S_MARKER_SIZE;
        return _MarkedSize > _Size ? _MarkedSize : 0;
    }

    #pragma warning(pop)

#endif



#ifdef _DEBUG
// C6255: _alloca indicates failure by raising a stack overflow exception
// C6386: buffer overrun
    #ifndef _CRTDBG_MAP_ALLOC
        #undef _malloca
        #define _malloca(size)                                                           \
            __pragma(warning(suppress: 6255 6386))                                       \
            (_MallocaComputeSize(size) != 0                                              \
                ? _MarkAllocaS(malloc(_MallocaComputeSize(size)), _ALLOCA_S_HEAP_MARKER) \
                : NULL)
    #endif

#else

    #undef _malloca
    #define _malloca(size)                                                                 \
        __pragma(warning(suppress: 6255 6386))                                             \
        (_MallocaComputeSize(size) != 0                                                    \
            ? (((_MallocaComputeSize(size) <= _ALLOCA_S_THRESHOLD)                         \
                ? _MarkAllocaS(_alloca(_MallocaComputeSize(size)), _ALLOCA_S_STACK_MARKER) \
                : _MarkAllocaS(malloc(_MallocaComputeSize(size)), _ALLOCA_S_HEAP_MARKER))) \
            : NULL)

#endif



#if defined __midl && !defined RC_INVOKED
#elif defined _DEBUG && defined _CRTDBG_MAP_ALLOC
#else

    #undef _freea

    #pragma warning(push)
    #pragma warning(disable: 6014) // leaking memory
    __inline void __CRTDECL _freea(_Pre_maybenull_ _Post_invalid_ void* _Memory)
    {
        unsigned int _Marker;
        if (_Memory)
        {
            _Memory = (char*)_Memory - _ALLOCA_S_MARKER_SIZE;
            _Marker = *(unsigned int*)_Memory;
            if (_Marker == _ALLOCA_S_HEAP_MARKER)
            {
                free(_Memory);
            }
            #ifdef _ASSERTE
            else if (_Marker != _ALLOCA_S_STACK_MARKER)
            {
                _ASSERTE(("Corrupted pointer passed to _freea" && 0));
            }
            #endif
        }
    }
    #pragma warning(pop)

#endif



#if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES
    #define alloca _alloca
#endif



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_MALLOC
