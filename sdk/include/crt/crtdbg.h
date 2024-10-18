/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#include <corecrt.h>

#ifndef _INC_CRTDBG
#define _INC_CRTDBG

#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif

  typedef void *_HFILE;

#define _CRT_WARN 0
#define _CRT_ERROR 1
#define _CRT_ASSERT 2
#define _CRT_ERRCNT 3

#define _CRTDBG_MODE_FILE 0x1
#define _CRTDBG_MODE_DEBUG 0x2
#define _CRTDBG_MODE_WNDW 0x4
#define _CRTDBG_REPORT_MODE -1

#define _CRTDBG_INVALID_HFILE ((_HFILE)-1)
#define _CRTDBG_HFILE_ERROR ((_HFILE)-2)
#define _CRTDBG_FILE_STDOUT ((_HFILE)-4)
#define _CRTDBG_FILE_STDERR ((_HFILE)-5)
#define _CRTDBG_REPORT_FILE ((_HFILE)-6)

  typedef int (__cdecl *_CRT_REPORT_HOOK)(int,char *,int *);
  typedef int (__cdecl *_CRT_REPORT_HOOKW)(int,wchar_t *,int *);

#define _CRT_RPTHOOK_INSTALL 0
#define _CRT_RPTHOOK_REMOVE 1

#define _HOOK_ALLOC 1
#define _HOOK_REALLOC 2
#define _HOOK_FREE 3

  typedef int (__cdecl *_CRT_ALLOC_HOOK)(int,void *,size_t,int,long,const unsigned char *,int);

#define _CRTDBG_ALLOC_MEM_DF 0x01
#define _CRTDBG_DELAY_FREE_MEM_DF 0x02
#define _CRTDBG_CHECK_ALWAYS_DF 0x04
#define _CRTDBG_RESERVED_DF 0x08
#define _CRTDBG_CHECK_CRT_DF 0x10
#define _CRTDBG_LEAK_CHECK_DF 0x20

#define _CRTDBG_CHECK_EVERY_16_DF 0x00100000
#define _CRTDBG_CHECK_EVERY_128_DF 0x00800000
#define _CRTDBG_CHECK_EVERY_1024_DF 0x04000000

#define _CRTDBG_CHECK_DEFAULT_DF 0

#define _CRTDBG_REPORT_FLAG -1

#define _BLOCK_TYPE(block) (block & 0xFFFF)
#define _BLOCK_SUBTYPE(block) (block >> 16 & 0xFFFF)

#define _FREE_BLOCK 0
#define _NORMAL_BLOCK 1
#define _CRT_BLOCK 2
#define _IGNORE_BLOCK 3
#define _CLIENT_BLOCK 4
#define _MAX_BLOCKS 5

  typedef void (__cdecl *_CRT_DUMP_CLIENT)(void *,size_t);

  struct _CrtMemBlockHeader;

  typedef struct _CrtMemState {
    struct _CrtMemBlockHeader *pBlockHeader;
    size_t lCounts[_MAX_BLOCKS];
    size_t lSizes[_MAX_BLOCKS];
    size_t lHighWaterCount;
    size_t lTotalCount;
  } _CrtMemState;


// Debug reporting functions

#ifdef _DEBUG

    int __cdecl _CrtDbgReport(int reportType, const char *filename, int linenumber, const char *moduleName, const char *format, ...);
    int __cdecl _CrtDbgReportW(int reportType, const wchar_t *filename, int linenumber, const wchar_t *moduleName, const wchar_t *format, ...);

    int __cdecl _CrtDbgReportV(int reportType, const char *filename, int linenumber, const char *moduleName, const char *format, va_list arglist);
    int __cdecl _CrtDbgReportWV(int reportType, const wchar_t *filename, int linenumber, const wchar_t *moduleName, const wchar_t *format, va_list arglist);

#endif


// Assertion and error reporting

#ifndef _DEBUG

    #define _CrtDbgBreak() ((void)0)

    #ifndef _ASSERT_EXPR
        #define _ASSERT_EXPR(expr,expr_str) ((void)0)
    #endif

    #ifndef _ASSERT
        #define _ASSERT(expr) ((void)0)
    #endif

    #ifndef _ASSERTE
        #define _ASSERTE(expr) ((void)0)
    #endif


    #define _RPT0(rptno,msg)
    #define _RPTN(rptno,msg,...)

    #define _RPTW0(rptno,msg)
    #define _RPTWN(rptno,msg,...)

    #define _RPTF0(rptno,msg)
    #define _RPTFN(rptno,msg,...)

    #define _RPTFW0(rptno,msg)
    #define _RPTFWN(rptno,msg,...)

    #define _CrtSetReportMode(t,f) ((int)0)
    #define _CrtSetReportFile(t,f) ((_HFILE)0)

#else // _DEBUG

    #define _CrtDbgBreak() __debugbreak()

    #ifndef _ASSERT_EXPR
        #define _ASSERT_EXPR(expr,expr_str) \
            (void)((!!(expr)) || (_CrtDbgReportW(_CRT_ASSERT, _CRT_WIDE(__FILE__), __LINE__, NULL, L"%ls", expr_str) != 1) || (_CrtDbgBreak(), 0))
    #endif

    #ifndef _ASSERT
        #define _ASSERT(expr) _ASSERT_EXPR((expr), NULL)
    #endif

    #ifndef _ASSERTE
        #define _ASSERTE(expr) _ASSERT_EXPR((expr), _CRT_WIDE(#expr))
    #endif

    #define _RPT_BASE(...) \
        (void)((_CrtDbgReport(__VA_ARGS__) != 1) || (_CrtDbgBreak(), 0))

    #define _RPT_BASEW(...) \
        (void)((_CrtDbgReportW(__VA_ARGS__) != 1) || (_CrtDbgBreak(), 0))


    #define _RPT0(rptno,msg)        _RPT_BASE(rptno, NULL, 0, NULL, "%s", msg)
    #define _RPTN(rptno,msg,...)    _RPT_BASE(rptno, NULL, 0, NULL, msg, __VA_ARGS__)

    #define _RPTW0(rptno,msg)       _RPT_BASEW(rptno, NULL, 0, NULL, L"%s", msg)
    #define _RPTWN(rptno,msg,...)   _RPT_BASEW(rptno, NULL, 0, NULL, msg, __VA_ARGS__)

    #define _RPTF0(rptno,msg)       _RPT_BASE(rptno, __FILE__, __LINE__, NULL, "%s", msg)
    #define _RPTFN(rptno,msg,...)   _RPT_BASE(rptno, __FILE__, __LINE__, NULL, msg, __VA_ARGS__)

    #define _RPTFW0(rptno,msg)      _RPT_BASEW(rptno, _CRT_WIDE(__FILE__), __LINE__, NULL, L"%s", msg)
    #define _RPTFWN(rptno,msg,...)  _RPT_BASEW(rptno, _CRT_WIDE(__FILE__), __LINE__, NULL, msg, __VA_ARGS__)

    int __cdecl _CrtSetReportMode(int reportType, int reportMode);
    _HFILE __cdecl _CrtSetReportFile(int reportType, _HFILE reportFile);

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



#define _malloc_dbg(s,t,f,l) malloc(s)
#define _calloc_dbg(c,s,t,f,l) calloc(c,s)
#define _realloc_dbg(p,s,t,f,l) realloc(p,s)
#define _recalloc_dbg(p,c,s,t,f,l) _recalloc(p,c,s)
#define _expand_dbg(p,s,t,f,l) _expand(p,s)
#define _free_dbg(p,t) free(p)
#define _msize_dbg(p,t) _msize(p)

#define _aligned_malloc_dbg(s,a,f,l) _aligned_malloc(s,a)
#define _aligned_realloc_dbg(p,s,a,f,l) _aligned_realloc(p,s,a)
#define _aligned_recalloc_dbg(p,c,s,a,f,l) _aligned_realloc(p,c,s,a)
#define _aligned_free_dbg(p) _aligned_free(p)
#define _aligned_offset_malloc_dbg(s,a,o,f,l) _aligned_offset_malloc(s,a,o)
#define _aligned_offset_realloc_dbg(p,s,a,o,f,l) _aligned_offset_realloc(p,s,a,o)
#define _aligned_offset_recalloc_dbg(p,c,s,a,o,f,l) _aligned_offset_recalloc(p,c,s,a,o)

#define _malloca_dbg(s,t,f,l) _malloca(s)
#define _freea_dbg(p,t) _freea(p)

#define _strdup_dbg(s,t,f,l) _strdup(s)
#define _wcsdup_dbg(s,t,f,l) _wcsdup(s)
#define _mbsdup_dbg(s,t,f,l) _mbsdup(s)
#define _tempnam_dbg(s1,s2,t,f,l) _tempnam(s1,s2)
#define _wtempnam_dbg(s1,s2,t,f,l) _wtempnam(s1,s2)
#define _fullpath_dbg(s1,s2,le,t,f,l) _fullpath(s1,s2,le)
#define _wfullpath_dbg(s1,s2,le,t,f,l) _wfullpath(s1,s2,le)
#define _getcwd_dbg(s,le,t,f,l) _getcwd(s,le)
#define _wgetcwd_dbg(s,le,t,f,l) _wgetcwd(s,le)
#define _getdcwd_dbg(d,s,le,t,f,l) _getdcwd(d,s,le)
#define _wgetdcwd_dbg(d,s,le,t,f,l) _wgetdcwd(d,s,le)
#define _getdcwd_lk_dbg(d,s,le,t,f,l) _getdcwd_nolock(d,s,le)
#define _wgetdcwd_lk_dbg(d,s,le,t,f,l) _wgetdcwd_nolock(d,s,le)

#define _CrtSetReportHook(f) ((_CRT_REPORT_HOOK)0)
#define _CrtGetReportHook() ((_CRT_REPORT_HOOK)0)
#define _CrtSetReportHook2(t,f) ((int)0)
#define _CrtSetReportHookW2(t,f) ((int)0)

#define _CrtSetBreakAlloc(a) ((long)0)
#define _CrtSetAllocHook(f) ((_CRT_ALLOC_HOOK)0)
#define _CrtGetAllocHook() ((_CRT_ALLOC_HOOK)0)
#define _CrtCheckMemory() ((int)1)
#define _CrtSetDbgFlag(f) ((int)0)
#define _CrtDoForAllClientObjects(f,c) ((void)0)
#define _CrtIsValidPointer(p,n,r) ((int)1)
#define _CrtIsValidHeapPointer(p) ((int)1)
#define _CrtIsMemoryBlock(p,t,r,f,l) ((int)1)
#define _CrtReportBlockType(p) ((int)-1)
#define _CrtSetDumpClient(f) ((_CRT_DUMP_CLIENT)0)
#define _CrtGetDumpClient() ((_CRT_DUMP_CLIENT)0)
#define _CrtMemCheckpoint(s) ((void)0)
#define _CrtMemDifference(s1,s2,s3) ((int)0)
#define _CrtMemDumpAllObjectsSince(s) ((void)0)
#define _CrtMemDumpStatistics(s) ((void)0)
#define _CrtDumpMemoryLeaks() ((int)0)
#define _CrtSetDebugFillThreshold(t) ((size_t)0)
#define _CrtSetCheckCount(f) ((int)0)
#define _CrtGetCheckCount() ((int)0)

#ifdef __cplusplus
}

  void *__cdecl operator new[](size_t _Size);
  inline void *__cdecl operator new(size_t _Size,int,const char *,int) { return ::operator new(_Size); }
  inline void *__cdecl operator new[](size_t _Size,int,const char *,int) { return ::operator new[](_Size); }
  void __cdecl operator delete[](void *) throw();
  inline void __cdecl operator delete(void *_P,int,const char *,int) throw() { ::operator delete(_P); }
  inline void __cdecl operator delete[](void *_P,int,const char *,int) throw() { ::operator delete[](_P); }
#endif

#pragma pack(pop)

#include <sec_api/crtdbg_s.h>

#endif
