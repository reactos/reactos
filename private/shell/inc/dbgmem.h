/* Copyright 1996 Microsoft */

#ifndef _DBGMEMORY_
#define _DBGMEMORY_

#ifdef DEBUG

#define DBGMEM_MEMORY   0x00
#define DBGMEM_STRDUP   0x01
#define DBGMEM_OBJECT   0x02
#define DBGMEM_TRACED   0x04
#define DBGMEM_UNKNOBJ  0x08

#endif // DEBUG


#define     DML_TYPE_MAIN            0
#define     DML_TYPE_THREAD          1
#define     DML_TYPE_FRAME           2
#define     DML_TYPE_DOWNLOAD        3
#define     DML_TYPE_NAVIGATE        4
#define     DML_TYPE_MASK   0x0000000f
#define     DML_BEGIN       0x00000000
#define     DML_END         0x80000000

#ifdef DEBUG

EXTERN_C BOOL g_bUseNewLeakDetection;

STDAPI_(void) MemLeakInit(UINT wFlags, LPCTSTR pszFile, UINT iLine);
#define DebugMemLeak(wFlags)  MemLeakInit(wFlags, TEXT(__FILE__), __LINE__)

// shdocvw memory leak tracker...
// void _DumpMemLeak( DWORD dwFlags ); // call DebugMemLeak instead!

// stuff used to move memory ownership from one thread to another
STDAPI_(void) remove_from_thread_memlist( DWORD dwThreadId, LPVOID pv );
STDAPI_(void) transfer_to_thread_memlist( DWORD dwThread, LPVOID pv );
STDAPI_(UINT) mem_thread_message( void );
STDAPI_(void) received_for_thread_memlist( DWORD dwFlags, void * pData );
#else
#define DebugMemLeak(wFlags)  (0)
#endif

typedef struct _leakmeminfo {
    HMODULE hmod;               // which dll was it allocated from...
    void*   pv;
    UINT    cb;
    UINT    nType;
    // DWORD   adwCaller[4];   // for future
    LPCSTR  pszFile;            // file where memory block was allocced
    INT_PTR iLine;              // line where memory block was allocced
} LEAKMEMINFO, *PLEAKMEMINFO;

typedef struct _IntelliLeakDumpCBFunctions
{
    void (STDMETHODCALLTYPE *pfnDumpLeakedMemory)(PLEAKMEMINFO pmeminfo);
    LPWSTR (STDMETHODCALLTYPE *pfnGetLeakSymbolicName)(PLEAKMEMINFO pmeminfo, LPWSTR pwszBuf, int cchBuf);
} INTELLILEAKDUMPCBFUNCTIONS;

typedef struct _LeakDetectFunctions
{

    HLOCAL (STDMETHODCALLTYPE * pfnTrcLocalAlloc)(UINT uFlags,
                               UINT uBytes,  
                               LPCSTR pszFile,  
                               const int iLine );

    HLOCAL (STDMETHODCALLTYPE * pfnTrcLocalFree )(HLOCAL hMem );

    LPTSTR (STDMETHODCALLTYPE * pfnTrcStrDup )(LPTSTR lpSrch,
                            LPCSTR pszFile,
                            const int iLine);

    void  (STDMETHODCALLTYPE * pfnDumpMemLeak )(DWORD wFlags);
    void  (STDMETHODCALLTYPE * pfnDebugMemLeak)(UINT wFlags, LPCTSTR pszFile, UINT iLine);
    void  (STDMETHODCALLTYPE * pfnreceived_for_thread_memlist)( DWORD dwFlags, void * pData );
    void  (STDMETHODCALLTYPE * pfnremove_from_thread_memlist )( DWORD dwThreadId, LPVOID pv );
    UINT  (STDMETHODCALLTYPE * pfnmem_thread_message )();
    void  (STDMETHODCALLTYPE * pfnremove_from_memlist )(void *pv);
    void  (STDMETHODCALLTYPE * pfnadd_to_memlist )(HMODULE hmod, void *pv, unsigned int nSize, UINT nType, LPCSTR pszFile, const INT_PTR iLine );
    void  (STDMETHODCALLTYPE * pfnregister_hmod_intelli_dump)(HMODULE hmod, const INTELLILEAKDUMPCBFUNCTIONS*pildf);

} LEAKDETECTFUNCS;

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif

STDAPI_(BOOL) GetLeakDetectionFunctionTable(LEAKDETECTFUNCS *pTable);
#ifdef __cplusplus
}

#endif  /* __cplusplus */

#ifdef DEBUG

// use the macros below
STDAPI_(HLOCAL) _TrcLocalAlloc(
    UINT uFlags,                            // flags used in LocalAlloc
    UINT uBytes,                            // number of bytes to be allocated
    LPCSTR pszFile,                         // file which allocced memory
    const int iLine                         // line which allocced memory
    );

STDAPI_(LPTSTR)  _TrcStrDup(
    LPTSTR lpSrch,                          // pointer to string to StrDup
    LPCSTR pszFile,                         // file which allocced memory
    const int   iLine                       // line which allocced memory
    );

STDAPI_(HLOCAL) _TrcLocalFree(
    HLOCAL hMem                             // memory to be freed
    );

STDAPI_(void) add_to_memlist(
    HMODULE hmod,                           // for secondary memory allocators...
    void *pv,                               // pointer to memory block
    unsigned int nSize,                     // size of memory block
    unsigned int nType,                     // what the possible use of the memory is
    LPCSTR pszFile,                         // file name
    const INT_PTR iLine                     // line number
    );

STDAPI_(void) remove_from_memlist(
    void *pv                                // pointer to memory block
    );

STDAPI_(void) register_intelli_dump(
    HMODULE hmod, 
    const INTELLILEAKDUMPCBFUNCTIONS *pfns
    );

#ifndef _NO_DBGMEMORY_REDEFINITION_

#ifdef __cplusplus
extern void* __cdecl operator new( size_t nSize, LPCSTR pszFile, const int iLine );
#define new new( __FILE__, __LINE__ )
#endif

#endif // _NO_DBGMEMORY_REDEFINITIONS

// use these macros
#define TrcLocalAlloc( _uFlags, _uBytes )   \
    _TrcLocalAlloc( _uFlags, _uBytes, __FILE__, __LINE__ )

#define TrcStrDup( _pSrc )   \
    _TrcStrDup( (LPTSTR)_pSrc, (LPCSTR)__FILE__, (const int)__LINE__ )

#define TrcLocalFree( _hMem )   \
    _TrcLocalFree( _hMem )

#define DbgAddToMemList( _pv ) \
{                              \
    if ( _pv )                 \
        add_to_memlist( 0, _pv, (UINT)LocalSize( _pv ), DBGMEM_TRACED, __FILE__, __LINE__ ); \
}

#define DbgRemoveFromMemList( _pv ) \
    remove_from_memlist( _pv )

#else // DEBUG

#define add_to_memlist(p)       (0)
#define remove_from_memlist(p)  (0)
#define remove_from_thread_memlist(p,v) (0)

#define TrcLocalAlloc( _uFlags, _uBytes )   \
    LocalAlloc( _uFlags, _uBytes )

#define TrcStrDup( _pSrc )   \
    StrDup( _pSrc )

#define TrcLocalFree( _hMem )   \
    LocalFree( _hMem )

#define DbgAddToMemList( _pv )      // NOP
#define DbgRemoveFromMemList( _pv ) // NOP

#endif // DEBUG

#endif // _DBGMEMORY_
