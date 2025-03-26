//
// crtdbg.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Public debugging facilities for the CRT
//
#pragma once
#ifndef _INC_CRTDBG // include guard for 3rd party interop
#define _INC_CRTDBG

#include <corecrt.h>
#include <vcruntime_new_debug.h>
#include <intrin.h> // for __debugbreak() on GCC

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



typedef void* _HFILE; // file handle pointer

#define _CRT_WARN           0
#define _CRT_ERROR          1
#define _CRT_ASSERT         2
#define _CRT_ERRCNT         3

#define _CRTDBG_MODE_FILE      0x1
#define _CRTDBG_MODE_DEBUG     0x2
#define _CRTDBG_MODE_WNDW      0x4
#define _CRTDBG_REPORT_MODE    -1

#define _CRTDBG_INVALID_HFILE ((_HFILE)(intptr_t)-1)
#define _CRTDBG_HFILE_ERROR   ((_HFILE)(intptr_t)-2)
#define _CRTDBG_FILE_STDOUT   ((_HFILE)(intptr_t)-4)
#define _CRTDBG_FILE_STDERR   ((_HFILE)(intptr_t)-5)
#define _CRTDBG_REPORT_FILE   ((_HFILE)(intptr_t)-6)



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Client-defined reporting and allocation hooks
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

typedef int (__CRTDECL* _CRT_REPORT_HOOK )(int, char*,    int*);
typedef int (__CRTDECL* _CRT_REPORT_HOOKW)(int, wchar_t*, int*);

#define _CRT_RPTHOOK_INSTALL  0
#define _CRT_RPTHOOK_REMOVE   1


typedef int (__CRTDECL* _CRT_ALLOC_HOOK)(int, void*, size_t, int, long, unsigned char const*, int);

#ifdef _M_CEE
    typedef int (__clrcall* _CRT_ALLOC_HOOK_M)(int, void*, size_t, int, long, unsigned char const*, int);
#endif

#define _HOOK_ALLOC     1
#define _HOOK_REALLOC   2
#define _HOOK_FREE      3



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Memory Management and State Tracking
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// Bit values for _crtDbgFlag flag. These bitflags control debug heap behavior.
#define _CRTDBG_ALLOC_MEM_DF        0x01  // Turn on debug allocation
#define _CRTDBG_DELAY_FREE_MEM_DF   0x02  // Don't actually free memory
#define _CRTDBG_CHECK_ALWAYS_DF     0x04  // Check heap every alloc/dealloc
#define _CRTDBG_RESERVED_DF         0x08  // Reserved - do not use
#define _CRTDBG_CHECK_CRT_DF        0x10  // Leak check/diff CRT blocks
#define _CRTDBG_LEAK_CHECK_DF       0x20  // Leak check at program exit

// Some bit values for _crtDbgFlag which correspond to frequencies for checking
// the heap.
#define _CRTDBG_CHECK_EVERY_16_DF   0x00100000 // Check heap every 16 heap ops
#define _CRTDBG_CHECK_EVERY_128_DF  0x00800000 // Check heap every 128 heap ops
#define _CRTDBG_CHECK_EVERY_1024_DF 0x04000000 // Check heap every 1024 heap ops

// We do not check the heap by default at this point because the cost was too
// high for some applications. You can still turn this feature on manually.
#define _CRTDBG_CHECK_DEFAULT_DF    0

#define _CRTDBG_REPORT_FLAG         -1 // Query bitflag status

#define _BLOCK_TYPE(block)          (block & 0xFFFF)
#define _BLOCK_SUBTYPE(block)       (block >> 16 & 0xFFFF)

// Memory block identification
#define _FREE_BLOCK      0
#define _NORMAL_BLOCK    1
#define _CRT_BLOCK       2
#define _IGNORE_BLOCK    3
#define _CLIENT_BLOCK    4
#define _MAX_BLOCKS      5

// _UNKNOWN_BLOCK is a sentinel value that may be passed to some functions that
// expect a block type as an argument.  If this value is passed, those functions
// will use the block type specified in the block header instead.  This is used
// in cases where the heap lock cannot be acquired to compute the block type
// before calling the function (e.g. when the caller is outside of the CoreCRT).
#define _UNKNOWN_BLOCK (-1)

typedef void (__CRTDECL* _CRT_DUMP_CLIENT)(void*, size_t);

#ifdef _M_CEE
    typedef void (__clrcall* _CRT_DUMP_CLIENT_M)(void*, size_t);
#endif

struct _CrtMemBlockHeader;

typedef struct _CrtMemState
{
    struct _CrtMemBlockHeader* pBlockHeader;
    size_t lCounts[_MAX_BLOCKS];
    size_t lSizes[_MAX_BLOCKS];
    size_t lHighWaterCount;
    size_t lTotalCount;
} _CrtMemState;

#ifndef _DEBUG

    #define _CrtGetAllocHook()                  ((_CRT_ALLOC_HOOK)0)
    #define _CrtSetAllocHook(f)                 ((_CRT_ALLOC_HOOK)0)

    #define _CrtGetDumpClient()                 ((_CRT_DUMP_CLIENT)0)
    #define _CrtSetDumpClient(f)                ((_CRT_DUMP_CLIENT)0)

    #define _CrtCheckMemory()                   ((int)1)
    #define _CrtDoForAllClientObjects(f, c)     ((void)0)
    #define _CrtDumpMemoryLeaks()               ((int)0)
    #define _CrtIsMemoryBlock(p, t, r, f, l)    ((int)1)
    #define _CrtIsValidHeapPointer(p)           ((int)1)
    #define _CrtIsValidPointer(p, n, r)         ((int)1)
    #define _CrtMemCheckpoint(s)                ((void)0)
    #define _CrtMemDifference(s1, s2, s3)       ((int)0)
    #define _CrtMemDumpAllObjectsSince(s)       ((void)0)
    #define _CrtMemDumpStatistics(s)            ((void)0)
    #define _CrtReportBlockType(p)              ((int)-1)
    #define _CrtSetBreakAlloc(a)                ((long)0)
    #define _CrtSetDbgFlag(f)                   ((int)0)


#else // ^^^ !_DEBUG ^^^ // vvv _DEBUG vvv //

    #ifndef _M_CEE_PURE

        _ACRTIMP int*  __cdecl __p__crtDbgFlag(void);
        _ACRTIMP long* __cdecl __p__crtBreakAlloc(void);

        #define _crtDbgFlag    (*__p__crtDbgFlag())
        #define _crtBreakAlloc (*__p__crtBreakAlloc())

        _ACRTIMP _CRT_ALLOC_HOOK __cdecl _CrtGetAllocHook(void);

        _ACRTIMP _CRT_ALLOC_HOOK __cdecl _CrtSetAllocHook(
            _In_opt_ _CRT_ALLOC_HOOK _PfnNewHook
            );

        _ACRTIMP _CRT_DUMP_CLIENT __cdecl _CrtGetDumpClient(void);

        _ACRTIMP _CRT_DUMP_CLIENT __cdecl _CrtSetDumpClient(
            _In_opt_ _CRT_DUMP_CLIENT _PFnNewDump
            );

    #endif // _M_CEE_PURE

    _ACRTIMP int __cdecl _CrtCheckMemory(void);

    typedef void (__cdecl* _CrtDoForAllClientObjectsCallback)(void*, void*);

    _ACRTIMP void __cdecl _CrtDoForAllClientObjects(
        _In_ _CrtDoForAllClientObjectsCallback _Callback,
        _In_ void*                             _Context
        );

    _ACRTIMP int __cdecl _CrtDumpMemoryLeaks(void);

    _ACRTIMP int __cdecl _CrtIsMemoryBlock(
        _In_opt_  void const*  _Block,
        _In_      unsigned int _Size,
        _Out_opt_ long*        _RequestNumber,
        _Out_opt_ char**       _FileName,
        _Out_opt_ int*         _LineNumber
        );

    _Check_return_
    _ACRTIMP int __cdecl _CrtIsValidHeapPointer(
        _In_opt_ void const* _Pointer
        );

    _Check_return_
    _ACRTIMP int __cdecl _CrtIsValidPointer(
        _In_opt_ void const*  _Pointer,
        _In_     unsigned int _Size,
        _In_     int          _ReadWrite
        );

    _ACRTIMP void __cdecl _CrtMemCheckpoint(
        _Out_ _CrtMemState* _State
        );

    _ACRTIMP int __cdecl _CrtMemDifference(
        _Out_ _CrtMemState*       _State,
        _In_  _CrtMemState const* _OldState,
        _In_  _CrtMemState const* _NewState
        );

    _ACRTIMP void __cdecl _CrtMemDumpAllObjectsSince(
        _In_opt_ _CrtMemState const* _State
        );

    _ACRTIMP void __cdecl _CrtMemDumpStatistics(
        _In_ _CrtMemState const* _State
        );

    _Check_return_
    _ACRTIMP int __cdecl _CrtReportBlockType(
        _In_opt_ void const* _Block
        );

    _ACRTIMP long __cdecl _CrtSetBreakAlloc(
        _In_ long _NewValue
        );

    _ACRTIMP int __cdecl _CrtSetDbgFlag(
        _In_ int _NewFlag
        );

#endif // _DEBUG



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Debug Heap Routines
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#ifndef _DEBUG

    #define _calloc_dbg(c, s, t, f, l)      calloc(c, s)
    #define _expand_dbg(p, s, t, f, l)      _expand(p, s)
    #define _free_dbg(p, t)                 free(p)
    #define _malloc_dbg(s, t, f, l)         malloc(s)
    #define _msize_dbg(p, t)                _msize(p)
    #define _realloc_dbg(p, s, t, f, l)     realloc(p, s)
    #define _recalloc_dbg(p, c, s, t, f, l) _recalloc(p, c, s)

    #define _aligned_free_dbg(p)                              _aligned_free(p)
    #define _aligned_malloc_dbg(s, a, f, l)                   _aligned_malloc(s, a)
    #define _aligned_msize_dbg(p, a, o)                       _aligned_msize(p, a, o)
    #define _aligned_offset_malloc_dbg(s, a, o, f, l)         _aligned_offset_malloc(s, a, o)
    #define _aligned_offset_realloc_dbg(p, s, a, o, f, l)     _aligned_offset_realloc(p, s, a, o)
    #define _aligned_offset_recalloc_dbg(p, c, s, a, o, f, l) _aligned_offset_recalloc(p, c, s, a, o)
    #define _aligned_realloc_dbg(p, s, a, f, l)               _aligned_realloc(p, s, a)
    #define _aligned_recalloc_dbg(p, c, s, a, f, l)           _aligned_recalloc(p, c, s, a)

    #define _freea_dbg(p, t)         _freea(p)
    #define _malloca_dbg(s, t, f, l) _malloca(s)

    #define _dupenv_s_dbg(ps1, size, s2, t, f, l)  _dupenv_s(ps1, size, s2)
    #define _fullpath_dbg(s1, s2, le, t, f, l)     _fullpath(s1, s2, le)
    #define _getcwd_dbg(s, le, t, f, l)            _getcwd(s, le)
    #define _getdcwd_dbg(d, s, le, t, f, l)        _getdcwd(d, s, le)
    #define _getdcwd_lk_dbg(d, s, le, t, f, l)     _getdcwd(d, s, le)
    #define _mbsdup_dbg(s, t, f, l)                _mbsdup(s)
    #define _strdup_dbg(s, t, f, l)                _strdup(s)
    #define _tempnam_dbg(s1, s2, t, f, l)          _tempnam(s1, s2)
    #define _wcsdup_dbg(s, t, f, l)                _wcsdup(s)
    #define _wdupenv_s_dbg(ps1, size, s2, t, f, l) _wdupenv_s(ps1, size, s2)
    #define _wfullpath_dbg(s1, s2, le, t, f, l)    _wfullpath(s1, s2, le)
    #define _wgetcwd_dbg(s, le, t, f, l)           _wgetcwd(s, le)
    #define _wgetdcwd_dbg(d, s, le, t, f, l)       _wgetdcwd(d, s, le)
    #define _wgetdcwd_lk_dbg(d, s, le, t, f, l)    _wgetdcwd(d, s, le)
    #define _wtempnam_dbg(s1, s2, t, f, l)         _wtempnam(s1, s2)

#else // ^^^ !_DEBUG ^^^ // vvv _DEBUG vvv //

    #ifdef _CRTDBG_MAP_ALLOC

        #define calloc(c, s)       _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _expand(p, s)      _expand_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define free(p)            _free_dbg(p, _NORMAL_BLOCK)
        #define malloc(s)          _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _msize(p)          _msize_dbg(p, _NORMAL_BLOCK)
        #define realloc(p, s)      _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _recalloc(p, c, s) _recalloc_dbg(p, c, s, _NORMAL_BLOCK, __FILE__, __LINE__)

        #define _aligned_free(p)                        _aligned_free_dbg(p)
        #define _aligned_malloc(s, a)                   _aligned_malloc_dbg(s, a, __FILE__, __LINE__)
        #define _aligned_msize(p, a, o)                 _aligned_msize_dbg(p, a, o)
        #define _aligned_offset_malloc(s, a, o)         _aligned_offset_malloc_dbg(s, a, o, __FILE__, __LINE__)
        #define _aligned_offset_realloc(p, s, a, o)     _aligned_offset_realloc_dbg(p, s, a, o, __FILE__, __LINE__)
        #define _aligned_offset_recalloc(p, c, s, a, o) _aligned_offset_recalloc_dbg(p, c, s, a, o, __FILE__, __LINE__)
        #define _aligned_realloc(p, s, a)               _aligned_realloc_dbg(p, s, a, __FILE__, __LINE__)
        #define _aligned_recalloc(p, c, s, a)           _aligned_recalloc_dbg(p, c, s, a, __FILE__, __LINE__)

        #define _freea(p)   _freea_dbg(p, _NORMAL_BLOCK)
        #define _malloca(s) _malloca_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)

        #define _dupenv_s(ps1, size, s2)  _dupenv_s_dbg(ps1, size, s2, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _fullpath(s1, s2, le)     _fullpath_dbg(s1, s2, le, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _getcwd(s, le)            _getcwd_dbg(s, le, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _getdcwd(d, s, le)        _getdcwd_dbg(d, s, le, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _mbsdup(s)                _strdup_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _strdup(s)                _strdup_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _tempnam(s1, s2)          _tempnam_dbg(s1, s2, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _wcsdup(s)                _wcsdup_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _wdupenv_s(ps1, size, s2) _wdupenv_s_dbg(ps1, size, s2, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _wfullpath(s1, s2, le)    _wfullpath_dbg(s1, s2, le, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _wgetcwd(s, le)           _wgetcwd_dbg(s, le, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _wgetdcwd(d, s, le)       _wgetdcwd_dbg(d, s, le, _NORMAL_BLOCK, __FILE__, __LINE__)
        #define _wtempnam(s1, s2)         _wtempnam_dbg(s1, s2, _NORMAL_BLOCK, __FILE__, __LINE__)

        #if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES
            #define   strdup(s)          _strdup_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
            #define   wcsdup(s)          _wcsdup_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
            #define   tempnam(s1, s2)    _tempnam_dbg(s1, s2, _NORMAL_BLOCK, __FILE__, __LINE__)
            #define   getcwd(s, le)      _getcwd_dbg(s, le, _NORMAL_BLOCK, __FILE__, __LINE__)
        #endif

    #endif // _CRTDBG_MAP_ALLOC

    _ACRTIMP void __cdecl _aligned_free_dbg(
        _Pre_maybenull_ _Post_invalid_ void* _Block
        );

    _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
    _ACRTIMP _CRTALLOCATOR void* __cdecl _aligned_malloc_dbg(
        _In_       size_t      _Size,
        _In_       size_t      _Alignment,
        _In_opt_z_ char const* _FileName,
        _In_       int         _LineNumber
        );

    _ACRTIMP size_t __cdecl _aligned_msize_dbg(
        _Pre_notnull_ void*  _Block,
        _In_          size_t _Alignment,
        _In_          size_t _Offset
        );

    _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
    _ACRTIMP _CRTALLOCATOR void* __cdecl _aligned_offset_malloc_dbg(
        _In_       size_t      _Size,
        _In_       size_t      _Alignment,
        _In_       size_t      _Offset,
        _In_opt_z_ char const* _FileName,
        _In_       int         _LineNumber
        );

    _Success_(return != 0) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
    _ACRTIMP _CRTALLOCATOR void* __cdecl _aligned_offset_realloc_dbg(
        _Pre_maybenull_ _Post_invalid_ void*       _Block,
        _In_                           size_t      _Size,
        _In_                           size_t      _Alignment,
        _In_                           size_t      _Offset,
        _In_opt_z_                     char const* _FileName,
        _In_                           int         _LineNumber
        );

    _Success_(return != 0) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count * _Size)
    _ACRTIMP _CRTALLOCATOR void* __cdecl _aligned_offset_recalloc_dbg(
        _Pre_maybenull_ _Post_invalid_ void*       _Block,
        _In_                           size_t      _Count,
        _In_                           size_t      _Size,
        _In_                           size_t      _Alignment,
        _In_                           size_t      _Offset,
        _In_opt_z_                     char const* _FileName,
        _In_                           int         _LineNumber
        );

    _Success_(return != 0) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
    _ACRTIMP _CRTALLOCATOR void* __cdecl _aligned_realloc_dbg(
        _Pre_maybenull_ _Post_invalid_ void*       _Block,
        _In_                           size_t      _Size,
        _In_                           size_t      _Alignment,
        _In_opt_z_                     char const* _FileName,
        _In_                           int         _LineNumber
        );

    _Success_(return != 0) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count * _Size)
    _ACRTIMP _CRTALLOCATOR void* __cdecl _aligned_recalloc_dbg(
        _Pre_maybenull_ _Post_invalid_ void*       _Block,
        _In_                           size_t      _Count,
        _In_                           size_t      _Size,
        _In_                           size_t      _Alignment,
        _In_opt_z_                     char const* _FileName,
        _In_                           int         _LineNumber
        );

    _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count * _Size)
    _ACRTIMP _CRTALLOCATOR void* __cdecl _calloc_dbg(
        _In_       size_t      _Count,
        _In_       size_t      _Size,
        _In_       int         _BlockUse,
        _In_opt_z_ char const* _FileName,
        _In_       int         _LineNumber
        );

    _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
    _ACRTIMP _CRTALLOCATOR void* __cdecl _expand_dbg(
        _Pre_notnull_ void*       _Block,
        _In_          size_t      _Size,
        _In_          int         _BlockUse,
        _In_opt_z_    char const* _FileName,
        _In_          int         _LineNumber
        );

    _ACRTIMP void __cdecl _free_dbg(
        _Pre_maybenull_ _Post_invalid_ void* _Block,
        _In_                           int   _BlockUse
        );

    _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
    _ACRTIMP _CRTALLOCATOR void* __cdecl _malloc_dbg(
        _In_       size_t      _Size,
        _In_       int         _BlockUse,
        _In_opt_z_ char const* _FileName,
        _In_       int         _LineNumber
        );

    _ACRTIMP size_t __cdecl _msize_dbg(
        _Pre_notnull_ void* _Block,
        _In_          int   _BlockUse
        );

    _Success_(return != 0) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Size)
    _ACRTIMP _CRTALLOCATOR void* __cdecl _realloc_dbg(
        _Pre_maybenull_ _Post_invalid_ void*       _Block,
        _In_                           size_t      _Size,
        _In_                           int         _BlockUse,
        _In_opt_z_                     char const* _FileName,
        _In_                           int         _LineNumber
        );

    _Success_(return != 0) _Check_return_ _Ret_maybenull_ _Post_writable_byte_size_(_Count * _Size)
    _ACRTIMP _CRTALLOCATOR void* __cdecl _recalloc_dbg(
        _Pre_maybenull_ _Post_invalid_ void*       _Block,
        _In_                           size_t      _Count,
        _In_                           size_t      _Size,
        _In_                           int         _BlockUse,
        _In_opt_z_                     char const* _FileName,
        _In_                           int         _LineNumber
        );

    _Success_(return == 0)
    _Check_return_wat_
    _DCRTIMP errno_t __cdecl _dupenv_s_dbg(
        _Outptr_result_buffer_maybenull_(*_PBufferSizeInBytes) char** _PBuffer,
        _Out_opt_                      size_t*     _PBufferSizeInBytes,
        _In_z_                         char const* _VarName,
        _In_                           int          _BlockType,
        _In_opt_z_                     char const* _FileName,
        _In_                           int          _LineNumber
        );

    _Success_(return != 0)
    _Check_return_ _Ret_maybenull_z_
    _ACRTIMP _CRTALLOCATOR char* __cdecl _fullpath_dbg(
        _Out_writes_opt_z_(_SizeInBytes) char*       _FullPath,
        _In_z_                           char const* _Path,
        _In_                             size_t      _SizeInBytes,
        _In_                             int         _BlockType,
        _In_opt_z_                       char const* _FileName,
        _In_                             int         _LineNumber
        );

    _Success_(return != 0)
    _Check_return_ _Ret_maybenull_z_
    _DCRTIMP _CRTALLOCATOR char* __cdecl _getcwd_dbg(
        _Out_writes_opt_z_(_SizeInBytes) char*       _DstBuf,
        _In_                             int         _SizeInBytes,
        _In_                             int         _BlockType,
        _In_opt_z_                       char const* _FileName,
        _In_                             int         _LineNumber
        );


    _Success_(return != 0)
    _Check_return_ _Ret_maybenull_z_
    _DCRTIMP _CRTALLOCATOR char* __cdecl _getdcwd_dbg(
        _In_                             int         _Drive,
        _Out_writes_opt_z_(_SizeInBytes) char*       _DstBuf,
        _In_                             int         _SizeInBytes,
        _In_                             int         _BlockType,
        _In_opt_z_                       char const* _FileName,
        _In_                             int         _LineNumber
        );

    _Check_return_ _Ret_maybenull_z_
    _ACRTIMP _CRTALLOCATOR char* __cdecl _strdup_dbg(
        _In_opt_z_ char const* _String,
        _In_       int         _BlockUse,
        _In_opt_z_ char const* _FileName,
        _In_       int         _LineNumber
        );

    _Check_return_ _Ret_maybenull_z_
    _ACRTIMP _CRTALLOCATOR char* __cdecl _tempnam_dbg(
        _In_opt_z_ char const* _DirName,
        _In_opt_z_ char const* _FilePrefix,
        _In_       int         _BlockType,
        _In_opt_z_ char const* _FileName,
        _In_       int         _LineNumber
        );

    _Success_(return != 0)
    _Check_return_ _Ret_maybenull_z_
    _ACRTIMP _CRTALLOCATOR wchar_t* __cdecl _wcsdup_dbg(
        _In_opt_z_ wchar_t const* _String,
        _In_       int            _BlockUse,
        _In_opt_z_ char const*    _FileName,
        _In_       int            _LineNumber
        );

    _Success_(return == 0)
    _Check_return_wat_
    _DCRTIMP errno_t __cdecl _wdupenv_s_dbg(
        _Outptr_result_buffer_maybenull_(*_PBufferSizeInWords) wchar_t** _PBuffer,
        _Out_opt_                        size_t*         _PBufferSizeInWords,
        _In_z_                           wchar_t const* _VarName,
        _In_                             int             _BlockType,
        _In_opt_z_                       char const*    _FileName,
        _In_                             int             _LineNumber
        );

    _Success_(return != 0)
    _Check_return_ _Ret_maybenull_z_
    _ACRTIMP _CRTALLOCATOR wchar_t* __cdecl _wfullpath_dbg(
        _Out_writes_opt_z_(_SizeInWords) wchar_t*       _FullPath,
        _In_z_                           wchar_t const* _Path,
        _In_                             size_t         _SizeInWords,
        _In_                             int            _BlockType,
        _In_opt_z_                       char const*    _FileName,
        _In_                             int            _LineNumber
        );

    _Success_(return != 0)
    _Check_return_ _Ret_maybenull_z_
    _DCRTIMP _CRTALLOCATOR wchar_t* __cdecl _wgetcwd_dbg(
        _Out_writes_opt_z_(_SizeInWords) wchar_t*    _DstBuf,
        _In_                             int         _SizeInWords,
        _In_                             int         _BlockType,
        _In_opt_z_                       char const* _FileName,
        _In_                             int         _LineNumber
        );

    _Success_(return != 0)
    _Check_return_ _Ret_maybenull_z_
    _DCRTIMP _CRTALLOCATOR wchar_t* __cdecl _wgetdcwd_dbg(
        _In_                             int         _Drive,
        _Out_writes_opt_z_(_SizeInWords) wchar_t*    _DstBuf,
        _In_                             int         _SizeInWords,
        _In_                             int         _BlockType,
        _In_opt_z_                       char const* _FileName,
        _In_                             int         _LineNumber
        );

    _Check_return_ _Ret_maybenull_z_
    _ACRTIMP _CRTALLOCATOR wchar_t* __cdecl _wtempnam_dbg(
        _In_opt_z_ wchar_t const* _DirName,
        _In_opt_z_ wchar_t const* _FilePrefix,
        _In_       int            _BlockType,
        _In_opt_z_ char const*    _FileName,
        _In_       int            _LineNumber
        );

    #define _malloca_dbg(s, t, f, l) _malloc_dbg(s, t, f, l)
    #define _freea_dbg(p, t)         _free_dbg(p, t)

    #if defined __cplusplus && defined _CRTDBG_MAP_ALLOC
    namespace std
    {
        using ::_calloc_dbg;
        using ::_free_dbg;
        using ::_malloc_dbg;
        using ::_realloc_dbg;
    }
    #endif

#endif // _DEBUG



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Debug Reporting
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

#ifndef _DEBUG

    #define _CrtSetDebugFillThreshold(t)        ((size_t)0)
    #define _CrtSetReportFile(t, f)             ((_HFILE)0)
    #define _CrtSetReportMode(t, f)             ((int)0)
    #define _CrtGetReportHook()                 ((_CRT_REPORT_HOOK)0)
    #define _CrtSetReportHook(f)                ((_CRT_REPORT_HOOK)0)
    #define _CrtSetReportHook2(t, f)            ((int)0)
    #define _CrtSetReportHookW2(t, f)           ((int)0)

#else // ^^^ !_DEBUG ^^^ // vvv _DEBUG vvv //

    _ACRTIMP int __cdecl _CrtDbgReport(
        _In_       int         _ReportType,
        _In_opt_z_ char const* _FileName,
        _In_       int         _Linenumber,
        _In_opt_z_ char const* _ModuleName,
        _In_opt_z_ char const* _Format,
        ...);

    _ACRTIMP int __cdecl _CrtDbgReportW(
        _In_       int            _ReportType,
        _In_opt_z_ wchar_t const* _FileName,
        _In_       int            _LineNumber,
        _In_opt_z_ wchar_t const* _ModuleName,
        _In_opt_z_ wchar_t const* _Format,
        ...);


    _ACRTIMP int __cdecl _VCrtDbgReportA(
        _In_       int         _ReportType,
        _In_opt_   void*       _ReturnAddress,
        _In_opt_z_ char const* _FileName,
        _In_       int         _LineNumber,
        _In_opt_z_ char const* _ModuleName,
        _In_opt_z_ char const* _Format,
                   va_list     _ArgList
        );

    _ACRTIMP int __cdecl _VCrtDbgReportW(
        _In_       int            _ReportType,
        _In_opt_   void*          _ReturnAddress,
        _In_opt_z_ wchar_t const* _FileName,
        _In_       int            _LineNumber,
        _In_opt_z_ wchar_t const* _ModuleName,
        _In_opt_z_ wchar_t const* _Format,
                   va_list        _ArgList
        );

    _ACRTIMP size_t __cdecl _CrtSetDebugFillThreshold(
        _In_ size_t _NewDebugFillThreshold
        );

    _ACRTIMP size_t __cdecl _CrtGetDebugFillThreshold(void);

    _ACRTIMP _HFILE __cdecl _CrtSetReportFile(
        _In_     int    _ReportType,
        _In_opt_ _HFILE _ReportFile
        );

    _ACRTIMP int __cdecl _CrtSetReportMode(
        _In_ int _ReportType,
        _In_ int _ReportMode
        );

    #ifndef _M_CEE_PURE

        extern long _crtAssertBusy;

        _ACRTIMP _CRT_REPORT_HOOK __cdecl _CrtGetReportHook(void);

        // _CrtSetReportHook[[W]2]:
        // For IJW, we need two versions:  one for clrcall and one for cdecl.
        // For pure and native, we just need clrcall and cdecl, respectively.
        _ACRTIMP _CRT_REPORT_HOOK __cdecl _CrtSetReportHook(
            _In_opt_ _CRT_REPORT_HOOK _PFnNewHook
            );

        _ACRTIMP int __cdecl _CrtSetReportHook2(
            _In_     int              _Mode,
            _In_opt_ _CRT_REPORT_HOOK _PFnNewHook
            );

        _ACRTIMP int __cdecl _CrtSetReportHookW2(
            _In_     int               _Mode,
            _In_opt_ _CRT_REPORT_HOOKW _PFnNewHook
            );

    #endif // !_M_CEE_PURE

#endif // _DEBUG




//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Assertions and Error Reporting Macros
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#ifndef _DEBUG

    #define _CrtDbgBreak() ((void)0)

    #ifndef _ASSERT_EXPR
        #define _ASSERT_EXPR(expr, msg) ((void)0)
    #endif

    #ifndef _ASSERT
        #define _ASSERT(expr) ((void)0)
    #endif

    #ifndef _ASSERTE
        #define _ASSERTE(expr) ((void)0)
    #endif

    #define _RPT0(rptno, msg)
    #define _RPTN(rptno, msg, ...)

    #define _RPTW0(rptno, msg)
    #define _RPTWN(rptno, msg, ...)

    #define _RPTF0(rptno, msg)
    #define _RPTFN(rptno, msg, ...)

    #define _RPTFW0(rptno, msg)
    #define _RPTFWN(rptno, msg, ...)

#else // ^^^ !_DEBUG ^^^ // vvv _DEBUG vvv //

    #define _CrtDbgBreak() __debugbreak()

    // !! is used to ensure that any overloaded operators used to evaluate expr
    // do not end up at &&.
    #ifndef _ASSERT_EXPR
        #define _ASSERT_EXPR(expr, msg) \
            (void)(                                                                                     \
                (!!(expr)) ||                                                                           \
                (1 != _CrtDbgReportW(_CRT_ASSERT, _CRT_WIDE(__FILE__), __LINE__, NULL, L"%ls", msg)) || \
                (_CrtDbgBreak(), 0)                                                                     \
            )
    #endif

    #ifndef _ASSERT
        #define _ASSERT(expr) _ASSERT_EXPR((expr), NULL)
    #endif

    #ifndef _ASSERTE
        #define _ASSERTE(expr) _ASSERT_EXPR((expr), _CRT_WIDE(#expr))
    #endif

    #define _RPT_BASE(...)                           \
        (void) ((1 != _CrtDbgReport(__VA_ARGS__)) || \
                (_CrtDbgBreak(), 0))

    #define _RPT_BASE_W(...)                          \
        (void) ((1 != _CrtDbgReportW(__VA_ARGS__)) || \
                (_CrtDbgBreak(), 0))

    #define _RPT0(rptno, msg)      _RPT_BASE(rptno, NULL, 0, NULL, "%s", msg)
    #define _RPTN(rptno, msg, ...) _RPT_BASE(rptno, NULL, 0, NULL, msg, __VA_ARGS__)

    #define _RPTW0(rptno, msg)      _RPT_BASE_W(rptno, NULL, 0, NULL, L"%ls", msg)
    #define _RPTWN(rptno, msg, ...) _RPT_BASE_W(rptno, NULL, 0, NULL, msg, __VA_ARGS__)

    #define _RPTF0(rptno, msg)      _RPT_BASE(rptno, __FILE__, __LINE__, NULL, "%s", msg)
    #define _RPTFN(rptno, msg, ...) _RPT_BASE(rptno, __FILE__, __LINE__, NULL, msg, __VA_ARGS__)

    #define _RPTFW0(rptno, msg)      _RPT_BASE_W(rptno, _CRT_WIDE(__FILE__), __LINE__, NULL, L"%ls", msg)
    #define _RPTFWN(rptno, msg, ...) _RPT_BASE_W(rptno, _CRT_WIDE(__FILE__), __LINE__, NULL, msg, __VA_ARGS__)

#endif // _DEBUG

// Asserts in debug.  Invokes Watson in both debug and release
#define _ASSERT_AND_INVOKE_WATSON(expr)                                              \
    {                                                                                \
        _ASSERTE((expr));                                                            \
        if (!(expr))                                                                 \
        {                                                                            \
            _invoke_watson(_CRT_WIDE(#expr), __FUNCTIONW__, __FILEW__, __LINE__, 0); \
        }                                                                            \
    }

// _ASSERT_BASE is provided only for backwards compatibility.
#ifndef _ASSERT_BASE
    #define _ASSERT_BASE _ASSERT_EXPR
#endif

#define _RPT1 _RPTN
#define _RPT2 _RPTN
#define _RPT3 _RPTN
#define _RPT4 _RPTN
#define _RPT5 _RPTN

#define _RPTW1 _RPTWN
#define _RPTW2 _RPTWN
#define _RPTW3 _RPTWN
#define _RPTW4 _RPTWN
#define _RPTW5 _RPTWN

#define _RPTF1 _RPTFN
#define _RPTF2 _RPTFN
#define _RPTF3 _RPTFN
#define _RPTF4 _RPTFN
#define _RPTF5 _RPTFN

#define _RPTFW1 _RPTFWN
#define _RPTFW2 _RPTFWN
#define _RPTFW3 _RPTFWN
#define _RPTFW4 _RPTFWN
#define _RPTFW5 _RPTFWN



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_CRTDBG
