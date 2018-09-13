//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       URLOSTRM.CXX
//
//  Contents:   Public interface and implementation of the URL
//              Open Stream APIs
//
//  Classes:    many (see below)
//
//  Functions:
//
//  History:    04-22-96    VictorS         Created
//              05-16-96    Jobi            Modified
//              06-06-96    Jobi            Modified
//              07-05-96    Ramesh          Modified
//----------------------------------------------------------------------------
#ifndef unix
// For some reaon on unix this causes NOERROR compile errors
#include "winerror.h"
#endif /* unix */
#include "ocidl.h"
#include "servprov.h"
#include "tchar.h"
#include "wininet.h"
#include "urlmki.h"
#include "urlhlink.h"
#include <shlwapi.h>
#include <shlwapip.h>

#define URLOSTRM_DONOT_NOTIFY_ONDATA    0xFF
#define URLOSTRM_NOTIFY_ONDATA            0x00

//----------------------------------------------------------//
//                                                          //
//  This file can never be compiled with the _UNICODE or    //
//  UNICODE macros defined.                                 //
//                                                          //
//----------------------------------------------------------//

//----------------------------------------------------------//
//  MACROS
//----------------------------------------------------------//

    // These macros can go away when macros and implementation of
    // the InetSDK has settled down...

#define IS_E_PENDING(x)  (x == E_PENDING)
#define LPUOSCALLBACK LPBINDSTATUSCALLBACK
#define PUMPREAD(strm) \
            { \
                DWORD dwSize = 0; \
                char * x = new char[20]; \
                hr = strm->Read(x, 20, &dwSize ); \
                if( !IS_E_PENDING(hr) && (dwSize != 0) ) \
                { \
                DPRINTF( ("Data on the over read! %d\n", dwSize) ); \
                } \
                delete x; \
            }

#define HANDLE_ABORT(hr) \
            { if( hr == E_ABORT) \
              { m_bInAbort = 1; \
                if( m_binding ) \
                    m_binding->Abort(); \
                 return(E_ABORT); \
               } \
            }

#define CHECK_MEMORY(ptr) \
            { if( !ptr ) \
              { DPRINTF( ("Failed to alloc memory") ); \
                m_bInAbort = 1; \
                if( m_binding ) \
                    m_binding->Abort(); \
                 return(E_OUTOFMEMORY); \
               } \
            }



    //
    //  Refcount helper
    //

    // Standardized COM Ref counting. ASSUMES that class has a ULONG
    // data member called 'm_ref'.

#define IMPLEMENT_REFCOUNT(clsname) \
        STDMETHOD_(ULONG, AddRef)() \
        {   CHECK_INTERFACE(this); \
            DPRINTF( ("(%#08x) " #clsname "::Addref %d\n", this, m_ref+1) );\
            return(++m_ref); }\
        STDMETHOD_(ULONG, Release)()\
            { CHECK_INTERFACE(this); \
            DPRINTF( ( "(%#08x) " #clsname "::Release : %d\n", this, m_ref-1) );\
            if( !--m_ref )\
            { delete this; return 0; }\
          return m_ref;\
        }



//----------------------------------------------------------
//
//  DEBUG MACROS
//
//----------------------------------------------------------

#ifdef _DEBUG

    // Check the validity of a pointer - use this for all allocated memory

#define CHECK_POINTER(val) \
            if (!(val) || IsBadWritePtr((void *)(val), sizeof(void *))) \
             { DPRINTF( ("BAD POINTER: %s!\n", #val ) ); \
              return E_POINTER; }

    // Check the validity of an interface pointer. Use this for all pointers
    // to C++ objects that are supposed to have vtables.

#define CHECK_INTERFACE(val) \
            if (!(val) || IsBadWritePtr((void *)(val), sizeof(void *)) \
                  || IsBadCodePtr( FARPROC( *((DWORD **)val)) ) ) \
            { DPRINTF( ("BAD INTERFACE: %s!\n", #val ) ); \
               return E_POINTER; }

    // Simple assert. Map this to whatever general
    // framework assert you want.

#define UOSASSERT(x) { if(!(x)) dprintf( "Assert in URLOSTRM API: %s\n", #x ); }

#define DUOS            OutputDebugString( "URLOSTRM API: " );
#define DPRINTF(x)          DUOS dprintf x ;

#ifndef CHECK_METHOD
#define CHECK_METHOD( m, args ) DUOS dprintf( "(%#08x) %s(", this, #m ); dprintf args ; dprintf(")\n");
#endif

#ifndef MEMPRINTF
#define MEMPRINTF(x) DPRINTF(x)
#endif

void dprintf( char * format, ... )
{
    char out[1024];
    va_list marker;
    va_start(marker, format);
    wvsprintf(out, format, marker);
    va_end(marker);
    OutputDebugString( out );
}


#else
#define CHECK_POINTER(x)
#define CHECK_INTERFACE(x)
#define CHECK_METHOD( m, args )
#define UOSASSERT(x)
#define DPRINTF(x)
#define MEMPRINTF(x)
#endif

//----------------------------------------------------------
//
//      Local heap stuff
//
//----------------------------------------------------------

    // Keeping this here makes this code portable to any .dll
static HANDLE   g_hHeap;

#ifdef _DEBUG
    // Uncomment the line below for Debug spew of memory stuff
//#define MONITER_MEMALLOC 1
#endif

#ifdef _DEBUG
static void * _cdecl
operator new( size_t size )
{
    if( !g_hHeap )
        g_hHeap = ::GetProcessHeap();

    // Heap alloc is the fastest gun in the west
    // for the type of allocations we do here.
    void * p = HeapAlloc(g_hHeap, 0, size);

    MEMPRINTF( ("operator new(%d) returns(%#08X)\n",size, DWORD(p)) );

    return(p);
}

static void _cdecl
operator delete ( void *ptr)
{
    MEMPRINTF( ("operator delete(%#08X)\n", DWORD(ptr) ) );

    HeapFree(g_hHeap, 0, ptr);
}
#endif


//----------------------------------------------------------
//
//      class CBuffer
//
//----------------------------------------------------------


//  Generic CBuffer class for quick and dirty mem allocs.
//  Caller must check return results.

class CBuffer
{
  public:
    CBuffer(ULONG cBytes);
    ~CBuffer();

    void *GetBuffer();

  private:
    void *      m_pBuf;
    // we'll use this temp buffer for small cases.
    //
    char        m_szTmpBuf[120];
    unsigned    m_fHeapAlloc:1;
};

inline
CBuffer::CBuffer(ULONG cBytes)
{
   if( !g_hHeap )
        g_hHeap = ::GetProcessHeap();

   m_pBuf = (cBytes <= 120) ? m_szTmpBuf : HeapAlloc(g_hHeap, 0, cBytes);
   m_fHeapAlloc = (cBytes > 120);
}

inline
CBuffer::~CBuffer()
{
    if (m_pBuf && m_fHeapAlloc) HeapFree(g_hHeap, 0, m_pBuf);
}

inline
void * CBuffer::GetBuffer()
{
    return m_pBuf;
}

//=--------------------------------------------------------------------------=
//
//  String ANSI <-> WIDE helper macros
//
//  This stuff stolen from marcwan...
//
// allocates a temporary buffer that will disappear when it goes out of scope
// NOTE: be careful of that -- make sure you use the string in the same or
// nested scope in which you created this buffer. people should not use this
// class directly.  use the macro(s) below.
//
//=--------------------------------------------------------------------------=

#define MAKE_WIDE(ptrname) \
    long __l##ptrname = (lstrlen(ptrname) + 1) * sizeof(WCHAR); \
    CBuffer __CBuffer##ptrname(__l##ptrname); \
    CHECK_POINTER(__CBuffer##ptrname.GetBuffer()); \
    if( !__CBuffer##ptrname.GetBuffer()) \
        return( E_OUTOFMEMORY ); \
    MultiByteToWideChar(CP_ACP, 0, ptrname, -1, \
        (LPWSTR)__CBuffer##ptrname.GetBuffer(), __l##ptrname); \
    LPWSTR __w##ptrname = (LPWSTR)__CBuffer##ptrname.GetBuffer()

#define WIDE_NAME(ptrname) __w##ptrname

#define MAKE_ANSI(ptrname) \
    long __l##ptrname = (lstrlenW(ptrname)*2 + 1) * sizeof(char); \
    CBuffer __CBuffer##ptrname(__l##ptrname); \
    CHECK_POINTER(__CBuffer##ptrname.GetBuffer()); \
    if( !__CBuffer##ptrname.GetBuffer()) \
        return( E_OUTOFMEMORY ); \
    WideCharToMultiByte(CP_ACP, 0, ptrname, -1, \
        (LPSTR)__CBuffer##ptrname.GetBuffer(), __l##ptrname, NULL, NULL); \
    LPSTR __a##ptrname = (LPSTR)__CBuffer##ptrname.GetBuffer()

#define ANSI_NAME(ptrname) __a##ptrname


//----------------------------------------------------------
//
//      Misc helper functions
//
//----------------------------------------------------------

// These registry functions are here for support of backdoor
// flags and screamer features.

static HRESULT
GetRegDword( HKEY mainkey, LPCTSTR subkey, LPCTSTR valueName, DWORD * result )
{
    HKEY    hkey = 0;
    DWORD       dwDisposition;

    LONG dwResult = RegCreateKeyEx(
                         mainkey, subkey,
                        0, // DWORD  Reserved,  // reserved
                        0, // LPTSTR  lpClass,  // address of class string
                        REG_OPTION_NON_VOLATILE, // DWORD  dwOptions,   // special options flag
                        KEY_ALL_ACCESS, // REGSAM  samDesired,  // desired security access
                        0, // LPSECURITY_ATTRIBUTES  lpSecurityAttributes,      // address of key security structure
                        &hkey, // PHKEY  phkResult,     // address of buffer for opened handle
                        &dwDisposition // LPDWORD  lpdwDisposition      // address of disposition value buffer
                       );

    HRESULT hr = dwResult == ERROR_SUCCESS ? NOERROR : E_FAIL;

    if( SUCCEEDED(hr) )
    {
        DWORD dwType;
        DWORD dwSize = sizeof(DWORD);
        DWORD dwSavedResult = *result;

        dwResult = RegQueryValueEx(
                        hkey,   // handle of key to query
                        valueName,
                        0, // LPDWORD  lpReserved,      // reserved
                        &dwType, // LPDWORD  lpType,    // address of buffer for value type
                        (LPBYTE)result, // LPBYTE  lpData,      // address of data buffer
                        &dwSize // LPDWORD  lpcbData    // address of data buffer size
                        );

        hr = dwResult == ERROR_SUCCESS ? NOERROR : E_FAIL;

        if( FAILED(hr) )
            *result = dwSavedResult;
    }

    if( hkey )
        RegCloseKey(hkey);

    return(hr);
}


static HRESULT
GetDLMRegDWord( LPCTSTR valueName, DWORD * result  )
{
    return(GetRegDword( HKEY_LOCAL_MACHINE,
                        _TEXT("Software\\Microsoft\\DownloadManager"),
                        valueName,
                        result ) );
}


static HRESULT
MyCreateFile( LPCWSTR filename, HANDLE & hfile )
{
    // BUGBUG: in retrospect this should be a ansi function
    // not a wide string one.

    HRESULT hr = NOERROR;

/**********
    MAKE_ANSI( filename );

    hfile = ::CreateFileA(
                          ANSI_NAME(filename), // LPCTSTR  lpFileName,    // pointer to name of the file
                          GENERIC_WRITE, // DWORD  dwDesiredAccess,       // access (read-write) mode
                          0, // DWORD  dwShareMode,       // share mode
                          0, // LPSECURITY_ATTRIBUTES  lpSecurityAttributes,      // pointer to security descriptor
                          CREATE_ALWAYS, // DWORD  dwCreationDistribution,        // how to create
                          FILE_ATTRIBUTE_NORMAL, //DWORD  dwFlagsAndAttributes,   // file attributes
                          0  // HANDLE  hTemplateFile   // handle to file with attributes to copy
                         );
*************/
    hfile = CreateFileWrapW(
                filename, 
                GENERIC_WRITE, 
                0, 
                0, 
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                0
        );

    // Our code likes HRESULT style error handling

    if( hfile == INVALID_HANDLE_VALUE )
        hr = MK_E_CANTOPENFILE;

    return(hr);
}



//----------------------------------------------------------
//
//      BindStatusCallback base class
//
//----------------------------------------------------------

//
//  This is the base class for the download objects. It implements
// the url mon callback interface (IBindStatusCallback) and
// IServiceProvider -- which it delegates to the caller's IBSCB.
//


    // State flags

class CBaseBSCB :   public IBindStatusCallbackMsg,
                    public IServiceProvider
{
public:

    CBaseBSCB( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback );
    virtual ~CBaseBSCB();

    STDMETHOD(KickOffDownload)( LPCWSTR szURL );

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);

    IMPLEMENT_REFCOUNT(CBaseBSCB);

    // IBindStatusCallback

    STDMETHODIMP OnStartBinding(
         DWORD grfBSCOption,
         IBinding  *pib);

    STDMETHODIMP GetPriority(
         LONG  *pnPriority);

    STDMETHODIMP OnLowResource(
         DWORD reserved);

    STDMETHODIMP OnProgress(
         ULONG ulProgress,
         ULONG ulProgressMax,
         ULONG ulStatusCode,
         LPCWSTR szStatusText);

    STDMETHODIMP OnDataAvailable(
         DWORD       grfBSCF,
         DWORD       dwSize,
         FORMATETC  *pformatetc,
         STGMEDIUM  *pstgmed);

    STDMETHODIMP OnStopBinding(
         HRESULT hresult,
         LPCWSTR szError);

    STDMETHODIMP GetBindInfo(
         DWORD  *grfBINDF,
         BINDINFO  *pbindinfo);

    STDMETHODIMP OnObjectAvailable(
         REFIID riid,
         IUnknown  *punk);

    STDMETHODIMP MessagePending(
        DWORD  dwPendingType,
        DWORD  dwPendingRecursion,
        DWORD  dwReserved);

    // IServiceProvider

    STDMETHODIMP QueryService(
            REFGUID rsid,
            REFIID iid,
            void **ppvObj);


    //  Local methods

    void    Abort();
    BOOL    IsAborted();
    BOOL    DownloadDone();
    HRESULT FinalResult();
    void    SetEncodingFlags( ULONG flags );

    IUnknown * Caller();

    // I guess at one point I thought it would be cool
    // to make all of these inlines and isolated from
    // the core functionality.

    HRESULT SignalOnData( DWORD flags, ULONG size, FORMATETC  *pformatetc);
    HRESULT SignalOnProgress( ULONG status, ULONG size, ULONG maxSize, LPCWSTR msg );
    HRESULT SignalOnStopBinding( HRESULT hr, LPCWSTR msg );

    HRESULT SignalOnStartBinding( DWORD grfBSCOption, IBinding  *pib);
    HRESULT SignalOnGetPriority(LONG*);
    HRESULT SignalOnLowResource(DWORD);
    HRESULT SignalGetBindInfo(DWORD  *grfBINDF,BINDINFO  *pbindinfo);

    virtual void    Neutralize();

    ULONG               m_ref;
    LPUOSCALLBACK       m_callback;
    IUnknown *          m_caller;
    IBinding *          m_binding;
    IServiceProvider *  m_callbackServiceProvider;
    IBindStatusCallbackMsg *_pBSCBMsg;
    BOOL                m_bInAbort : 1;
    BOOL                m_bInCache : 1;
    BOOL                m_bCheckedForServiceProvider : 1;
    DWORD               m_readSoFar;
    HRESULT             m_finalResult;

    // See notes above about IStream usage

    IStream *           m_UserStream;

    ULONG               m_maxSize;
    DWORD               m_bscoFlags;
    UINT                m_encoding;
    char                m_szCacheFileName[MAX_PATH];
};


    /*---------------------*/
    /*  INLINES            */
    /*---------------------*/

inline void     CBaseBSCB::Abort()      { m_bInAbort = 1; }
inline BOOL     CBaseBSCB::IsAborted()  { return(m_bInAbort); }
inline HRESULT  CBaseBSCB::FinalResult(){ return( m_finalResult ); }
inline IUnknown*CBaseBSCB::Caller()     { return( m_caller ); }
inline void     CBaseBSCB::SetEncodingFlags( ULONG flags ) { m_encoding = flags; }

inline HRESULT
CBaseBSCB::SignalOnData( DWORD flags, ULONG size, FORMATETC  *pformatetc )
{
        HRESULT hr=NOERROR;

        if(m_bscoFlags!=URLOSTRM_NOTIFY_ONDATA)
        return(hr);

        STGMEDIUM stg;

        stg.tymed = TYMED_ISTREAM;
        stg.pstm  = m_UserStream;
        stg.pUnkForRelease = NULL;

        if(m_callback)
                hr=m_callback->OnDataAvailable(flags,size,pformatetc,&stg);

        return(hr);
}

inline HRESULT
CBaseBSCB::SignalOnProgress( ULONG status, ULONG size, ULONG maxSize, LPCWSTR msg )
{
    if( !m_callback )
        return(NOERROR);

    if( size && !maxSize )
        maxSize = size;

    if( maxSize > m_maxSize )
        m_maxSize = maxSize;

    HRESULT hr = m_callback->OnProgress( size, m_maxSize, status, msg );

    return(hr);
}


inline HRESULT
CBaseBSCB::SignalOnStopBinding( HRESULT hres, LPCWSTR msg )
{
    if( !m_callback )
        return(NOERROR);

    HRESULT hr = m_callback->OnStopBinding( hres, msg );

    return(hr);
}



inline HRESULT
CBaseBSCB::SignalOnStartBinding( DWORD grfBSCOption, IBinding  *pib)
{
    if( !m_callback )
        return(NOERROR);
    return( m_callback->OnStartBinding(grfBSCOption,pib) );
}


inline HRESULT
CBaseBSCB:: SignalOnGetPriority(LONG* lng)
{
    if( !m_callback )
        return(E_NOTIMPL);
    return(m_callback->GetPriority(lng));
}

inline HRESULT
CBaseBSCB:: SignalOnLowResource(DWORD dw)
{
    if( !m_callback )
        return( NOERROR );
    return( m_callback->OnLowResource(dw) );
}

inline HRESULT
CBaseBSCB::SignalGetBindInfo(DWORD *grfBINDF, BINDINFO * pbindinfo)
{
    if( !m_callback )
        return(E_NOTIMPL);
    return( m_callback->GetBindInfo(grfBINDF, pbindinfo) );
}


    /*---------------------*/
    /*  OUT-OF-LINES       */
    /*---------------------*/


// Do nothing CTOR
CBaseBSCB::CBaseBSCB
(
    IUnknown *      caller,
    DWORD               bscof,
    LPUOSCALLBACK       callback
)
{
    m_binding       = 0;
    m_ref               = 0;
    m_bInAbort      = 0;
    m_bCheckedForServiceProvider = 0;
    m_bInCache      = 0;
    m_readSoFar     = 0;
    m_UserStream    = 0;
    m_encoding      = 0;
    m_bscoFlags    = bscof;
    m_callbackServiceProvider = 0;
    m_szCacheFileName[0] = NULL;
        m_finalResult   = S_OK;

    _pBSCBMsg = 0;

    if( (m_callback = callback) != 0 )
        m_callback->AddRef();

    if( (m_caller = caller) != 0 )
        caller->AddRef();
}

// Cleanup just call Neutralize();
CBaseBSCB::~CBaseBSCB()
{
    Neutralize();
}

void
CBaseBSCB::Neutralize()
{
    if( m_binding )
    {
        m_binding->Release();
        m_binding = 0;
    }
    if( m_caller )
    {
        m_caller->Release();
        m_caller = 0;
    }
    if( m_callback )
    {
        m_callback->Release();
        m_callback = 0;
    }
    if( m_callbackServiceProvider )
    {
        m_callbackServiceProvider->Release();
        m_callbackServiceProvider = 0;
    }
    if( m_UserStream )
    {
        m_UserStream->Release();
        m_UserStream = 0;
    }
    if (_pBSCBMsg)
    {
        _pBSCBMsg->Release();
    }
}

// IUnknown::QueryInterface
STDMETHODIMP
CBaseBSCB::QueryInterface
(
    const GUID &iid,
    void **     ppv
)
{
    CHECK_METHOD(CBaseBSCB::QueryInterface, ("") );

    if (iid==IID_IUnknown || iid==IID_IBindStatusCallback)
    {
        *ppv =(IBindStatusCallback*)this;
        AddRef();
        return(NOERROR);
    }


    if( iid==IID_IServiceProvider)
    {
        *ppv =(IServiceProvider*)this;
        AddRef();
        return(NOERROR);
    }

    if (iid==IID_IBindStatusCallbackMsg)
    {
        *ppv =(IBindStatusCallbackMsg*)this;
        AddRef();
        return(NOERROR);
    }


    return( E_NOINTERFACE );
}

// IServiceProvider::QueryService
STDMETHODIMP
CBaseBSCB::QueryService
(
    REFGUID rsid,
    REFIID iid,
    void **ppvObj
)
{
    CHECK_METHOD(CBaseBSCB::QueryService, ("") );

    HRESULT hr = E_NOINTERFACE;

    if (iid==IID_IBindStatusCallback)
    {
        *ppvObj =(IBindStatusCallbackMsg*)this;
        AddRef();
        return(NOERROR);
    }


    if( m_callback )
        hr = m_callback->QueryInterface( iid, ppvObj );

    if( FAILED(hr) && !m_callbackServiceProvider && !m_bCheckedForServiceProvider )
    {
       m_bCheckedForServiceProvider = 1;

       if( m_callback )
       {
            hr = m_callback->QueryInterface
                                (
                                  IID_IServiceProvider,
                                  (void**)&m_callbackServiceProvider
                                );
       }

        if( SUCCEEDED(hr) && m_callbackServiceProvider )
            hr = m_callbackServiceProvider->QueryService(rsid,iid,ppvObj);
        else
            hr = E_NOINTERFACE; // BUGBUG: what's that error code again?
    }

    HANDLE_ABORT(hr);

    return( hr );
}


// IBindStatusCallback::OnStartBinding
STDMETHODIMP
CBaseBSCB::OnStartBinding
(
    DWORD       grfBSCOption,
    IBinding   *pib
)
{
    CHECK_METHOD(CBaseBSCB::OnStartBinding, ("flags: %#08x, IBinding: %#08x",grfBSCOption,pib) );

    CHECK_INTERFACE(pib);

    HRESULT hr = SignalOnStartBinding(grfBSCOption,pib);

    // smooth over user's e_not_implemented for when we
    // return to urlmon

    if( hr == E_NOTIMPL )
        hr = NOERROR;
    else
        HANDLE_ABORT(hr);

    if( SUCCEEDED(hr) )
    {
        pib->AddRef();
        m_binding = pib;
    }

    return( hr );
}


// IBindStatusCallback::GetPriority
STDMETHODIMP
CBaseBSCB::GetPriority
(
    LONG  *pnPriority
)
{
    CHECK_METHOD(CBaseBSCB::GetPriority, ("pnPriority: %#08x", pnPriority) );
    CHECK_POINTER(pnPriority);

    if (!pnPriority)
        return E_POINTER;

    HRESULT hr = SignalOnGetPriority(pnPriority);

    if( hr == E_NOTIMPL )
    {
        // only override if caller doesn't implement.
        *pnPriority = NORMAL_PRIORITY_CLASS;
        hr = NOERROR;
    }
    else
    {
        HANDLE_ABORT(hr);
    }

    return( hr );

}


// IBindStatusCallback::OnLowResource
STDMETHODIMP
CBaseBSCB::OnLowResource( DWORD rsv)
{
    CHECK_METHOD(CBaseBSCB::OnLowResource, ("resv: %#08x",rsv) );

    HRESULT hr = SignalOnLowResource(rsv);

    // Keep downloading...

    if( hr == E_NOTIMPL )
        hr = NOERROR;
    else
        HANDLE_ABORT(hr);

    return( hr );
}


// IBindStatusCallback::OnStopBinding
STDMETHODIMP
CBaseBSCB::OnStopBinding
(
    HRESULT hresult,
    LPCWSTR szError
)
{
    CHECK_METHOD(CBaseBSCB::OnStopBinding, ("%#08X %ws", hresult, szError ? szError : L"[no error]" )  );

    // Store the hresult so we can return it to caller in the
    // blocking/sync case.

    HRESULT hr = SignalOnStopBinding( m_finalResult = hresult, szError );

    if( m_binding )
    {
        m_binding->Release();
        m_binding = 0;
    }

    if( hr == E_NOTIMPL )
        hr = NOERROR;
    else
        HANDLE_ABORT(hr);

    return( hr );
}


// IBindStatusCallback::GetBindInfo

STDMETHODIMP
CBaseBSCB::GetBindInfo
(
    DWORD  *    grfBINDF,
    BINDINFO*   pbindinfo
)
{
    CHECK_METHOD(CBaseBSCB::GetBindInfo, ("grfBINDF: %#08x, pbinfinfo ",grfBINDF) );

    CHECK_POINTER(grfBINDF);
    CHECK_POINTER(pbindinfo);

    *grfBINDF = 0;

    HRESULT hr = SignalGetBindInfo(grfBINDF,pbindinfo);

    if( SUCCEEDED(hr) || (hr == E_NOTIMPL) )
    {
        // Let the derived class choose the bind flags

        if(m_encoding)
        {
            *grfBINDF |= m_encoding;
            pbindinfo->grfBindInfoF |= m_encoding;
        }

        hr = NOERROR;
    }

    HANDLE_ABORT(hr);

    return( hr );
}


// IBindStatusCallback::OnObjectAvailable
STDMETHODIMP
CBaseBSCB::OnObjectAvailable
(
    REFIID riid,
    IUnknown  *punk
)
{
    // This should never be called
    CHECK_METHOD(CBaseBSCB::OnObjectAvailable, ("!") );
    UOSASSERT(0 && "This should never be called");
    return(NOERROR);
}

STDMETHODIMP
CBaseBSCB::OnProgress
(
    ULONG ulProgress,
    ULONG ulProgressMax,
    ULONG ulStatusCode,
    LPCWSTR szStatusText
)
{
    CHECK_METHOD(CBaseBSCB::OnProgress, ("!") );

    // URL moniker has a habit of passing ZERO
    // into ulProgressMax. So.. let's at least
    // pass in the amount we have so far...

    m_maxSize = ulProgressMax ? ulProgressMax : ulProgress;

    // This is useful information for the IStream implementation


    if( ulStatusCode == BINDSTATUS_USINGCACHEDCOPY )
        m_bInCache = TRUE;

    HRESULT hr;

    hr = SignalOnProgress( ulStatusCode, ulProgress, ulProgressMax, szStatusText );

    if( hr == E_NOTIMPL )
        hr = NOERROR;
    else
        HANDLE_ABORT(hr);

    return( hr );
}

// IBindStatusCallback::OnDataAvailable.

STDMETHODIMP
CBaseBSCB::OnDataAvailable
(
    DWORD           grfBSCF,
    DWORD           dwSize,
    FORMATETC  *pformatetc,
    STGMEDIUM  *pstgmed
)
{
    CHECK_METHOD(CBaseBSCB::OnDataAvailable,
                                ("Flags: %x, dwSize: %d", grfBSCF, dwSize) );

    HRESULT hr = NOERROR;

    // N.B Assumption here is that the pstgmed->pstm will always be the same

     if( !m_UserStream )
     {
            // We need to bump the refcount every time we
            // copy and store the pointer.

        m_UserStream = pstgmed->pstm;
        m_UserStream->AddRef();
     }

     if (*m_szCacheFileName == NULL)
     {
         STATSTG statstg;
         DWORD dwVal = 0;

         if (m_UserStream->Stat(&statstg,dwVal) == S_OK)
         {
             if (0==WideCharToMultiByte(  CP_ACP, 0, statstg.pwcsName, lstrlenW(statstg.pwcsName)+1, m_szCacheFileName,
                         MAX_PATH, NULL, NULL))
             {
                   m_szCacheFileName[0] = NULL;
             }
             if (statstg.pwcsName)
             {
                 CoTaskMemFree(statstg.pwcsName);
                 statstg.pwcsName = NULL;
             }
         }
         else
            m_szCacheFileName[0] = NULL;
     }

    hr = SignalOnData( grfBSCF, dwSize, pformatetc );

    // Tell the blocking state machine we are have data.
    // ClearState( WAITING_FOR_DATA );

    if( hr == E_NOTIMPL )
        hr = NOERROR;
    else
        HANDLE_ABORT(hr);

    return( hr );
}

// IBindStatusCallback::MessagePending.

STDMETHODIMP CBaseBSCB::MessagePending(DWORD  dwPendingType, DWORD  dwPendingRecursion, DWORD  dwReserved)
{
    MSG msg;
    HRESULT hr = NOERROR;

    if (m_callback && !_pBSCBMsg)
    {
        hr = m_callback->QueryInterface(IID_IBindStatusCallbackMsg, (void **) &_pBSCBMsg);
    }

    if (_pBSCBMsg && hr == NOERROR )
    {
        hr = _pBSCBMsg->MessagePending(dwPendingType, dwPendingRecursion, dwReserved );
    }

    return hr;
}

HRESULT
CBaseBSCB::KickOffDownload( LPCWSTR szURL )
{
    HRESULT                 hr;
    IOleObject *        pOleObject = 0;
    IServiceProvider *  pServiceProvider = 0;
    BOOL                bUseCaller = (Caller() != 0);
    IMoniker *          pmkr = 0;
    IBindCtx *          pBndCtx = 0;

    CHECK_POINTER(szURL);
    UOSASSERT(*szURL);


    IStream * pstrm = 0;

    // Don't bother if we don't have a caller...

    if( bUseCaller )
    {
        // By convention the we give the caller first crack at service
        // provider. The assumption here is that if they implement it
        // they have the decency to forward QS's to their container.

        hr = Caller()->QueryInterface( IID_IServiceProvider,
                                        (void**)&pServiceProvider );

        if( FAILED(hr) )
        {
            // Ok, now try the 'slow way' : maybe the object is an 'OLE' object
            // that knows about it's client site:

            hr = Caller()->QueryInterface( IID_IOleObject, (void**)&pOleObject );

            if( SUCCEEDED(hr) )
            {
                IOleClientSite * pClientSite = 0;

                hr = pOleObject->GetClientSite(&pClientSite);

                if( SUCCEEDED(hr) )
                {
                    // Now see if we have a service provider at that site
                    hr = pClientSite->QueryInterface
                                            ( IID_IServiceProvider,
                                            (void**)&pServiceProvider );
                }

                if( pClientSite )
                    pClientSite->Release();
            }
            else
            {
                // Ok, it's not an OLE object, maybe it's one of these
                // new fangled 'ObjectWithSites':

                IObjectWithSite * pObjWithSite = 0;

                hr = Caller()->QueryInterface( IID_IObjectWithSite,
                                                    (void**)&pObjWithSite );

                if( SUCCEEDED(hr) )
                {
                    // Now see if we have a service provider at that site

                    hr = pObjWithSite->GetSite(IID_IServiceProvider,
                                                (void**)&pServiceProvider);
                }

                if( pObjWithSite )
                    pObjWithSite->Release();

            }
            if( pOleObject )
                pOleObject->Release();

        }

        // BUGBUG: In the code above we stop looking at one level up --
        //  this may be too harsh and we should loop on client sites
        // until we get to the top...

        if( !pServiceProvider )
            hr = E_UNEXPECTED;

        IBindHost * pBindHost = 0;

        // Ok, we have a service provider, let's see if BindHost is
        // available. (Here there is some upward delegation going on
        // via service provider).

        if( SUCCEEDED(hr) )
            hr = pServiceProvider->QueryService( SID_SBindHost, IID_IBindHost,
                                                        (void**)&pBindHost );

        if( pServiceProvider )
            pServiceProvider->Release();

        pmkr = 0;

        if( pBindHost )
        {
            // This allows the container to actually drive the download
            // by creating it's own moniker.

            hr = pBindHost->CreateMoniker( LPOLESTR(szURL),NULL, &pmkr,0 );



            if( SUCCEEDED(hr) )
            {
                // This allows containers to hook the download for
                // doing progress and aborting

                hr = pBindHost->MonikerBindToStorage(pmkr, NULL, this, IID_IStream,(void**)&pstrm);
            }

            pBindHost->Release();
        }
        else
        {
            bUseCaller = 0;
        }
    }

    if( !bUseCaller )
    {
        // If you are here, then either the caller didn't pass
        // a 'caller' pointer or the caller is not in a BindHost
        // friendly environment.

        hr = ::CreateURLMoniker( 0, szURL, &pmkr );

        if( SUCCEEDED(hr) )
            hr = ::CreateBindCtx( 0, &pBndCtx );

                if( SUCCEEDED(hr) )
                {
                // Register US (not the caller) as the callback. This allows
        // us to hook all notfiications from URL moniker and filter
        // and manipulate to our satifisfaction.
                         hr = ::RegisterBindStatusCallback( pBndCtx, this, 0, 0L );
                }

            if( SUCCEEDED(hr) )
                {
                        hr = pmkr->BindToStorage( pBndCtx, NULL, IID_IStream, (void**)&pstrm );

                        // Smooth out the error code
                if( IS_E_PENDING(hr) )
                            hr = S_OK;
                }

    }

    if( pstrm )
        pstrm->Release();

    if( pmkr )
        pmkr->Release();

    if( pBndCtx )
        pBndCtx->Release();

    return(hr);
}



//----------------------------------------------------------
//
//      CPullDownload
//
//----------------------------------------------------------

// placeholder for covering the URL moniker anomolies

class CPullDownload : public CBaseBSCB
{
public:
    CPullDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback );
    STDMETHODIMP GetBindInfo(
         DWORD  *grfBINDF,
         BINDINFO  *pbindinfo);
};

inline
CPullDownload::CPullDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback )
    : CBaseBSCB(caller,bscof,callback)
{
}

STDMETHODIMP
CPullDownload::GetBindInfo
(
        DWORD  *        grfBINDF,
    BINDINFO  * pbindinfo
)
{
    // pointers are validated in base class

        HRESULT hr = CBaseBSCB::GetBindInfo( grfBINDF, pbindinfo );

        if( SUCCEEDED(hr))
                *grfBINDF = BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_ASYNCHRONOUS;

        return(hr);
}
//----------------------------------------------------------
//
//      Push Stream API
//
//----------------------------------------------------------

//
// Class used for implementing push model downloading when used
// in combination with the CStream object.
//
//  The general design for is this class pumps a
//  CBitBucket object with bits and the CStream object makes
//  those bits available to the caller for reading.
//

class CPushDownload : public CBaseBSCB
{
public:
    CPushDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback );
    ~CPushDownload();
    STDMETHODIMP GetBindInfo(
         DWORD  *grfBINDF,
         BINDINFO  *pbindinfo);
protected:

    // CBaseBSCB

    virtual void  Neutralize();


    // IBindStatusCallback

    STDMETHODIMP OnDataAvailable
    (
         DWORD          grfBSCF,
         DWORD          dwSize,
         FORMATETC *    pFmtetc,
         STGMEDIUM *    pstgmed
    ) ;

private:
    HRESULT CleanupPush();
};


CPushDownload::CPushDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback )
    : CBaseBSCB(caller,bscof, callback)
{
}

CPushDownload::~CPushDownload()
{
    CleanupPush();
}

void
CPushDownload::Neutralize()
{
    // We have to do special cleanup.

    CleanupPush();

    CBaseBSCB::Neutralize();
}

HRESULT
CPushDownload::CleanupPush()
{
    return(NOERROR);
}

STDMETHODIMP
CPushDownload::OnDataAvailable
(
     DWORD              grfBSCF,
     DWORD              dwSize,
     FORMATETC *    pFmtetc,
     STGMEDIUM *    pstgmed
)
{
    HRESULT hr = NOERROR;

    if( SUCCEEDED(hr) &&  pstgmed->pstm )
    {

        m_UserStream = pstgmed->pstm;

        // Add ref again because we are copying and storing the ptr

        m_UserStream->AddRef();

    }

    if( SUCCEEDED(hr) || IS_E_PENDING(hr) )
        hr = CBaseBSCB::OnDataAvailable(grfBSCF,dwSize,pFmtetc,pstgmed);

    return(hr);
}

STDMETHODIMP
CPushDownload::GetBindInfo
(
        DWORD  *        grfBINDF,
    BINDINFO  * pbindinfo
)
{

        HRESULT hr = CBaseBSCB::GetBindInfo( grfBINDF, pbindinfo );
        
        // PushDownload can not be ASYNC
        *grfBINDF = 0;

        return(hr);
}
//----------------------------------------------------------
//
//      Block Stream API
//
//----------------------------------------------------------

class CBlockDownload : public CPushDownload
{
public:
    CBlockDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback );
    ~CBlockDownload();
    STDMETHODIMP GetBindInfo(
         DWORD  *grfBINDF,
         BINDINFO  *pbindinfo);
    HRESULT GetStream( IStream ** ppStream );
};


inline
CBlockDownload::CBlockDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback )
    : CPushDownload(caller,bscof, callback)
{

}

template <class T> inline
HRESULT CheckThis( T * AThisPtr )
{
    CHECK_INTERFACE(AThisPtr);
    return(NOERROR);
}

CBlockDownload::~CBlockDownload()
{
    CheckThis(this);
}

HRESULT
CBlockDownload::GetStream( IStream ** ppStream )
{
    // REMEMBER: If you get this pointer and return it
    // to caller YOU MUST add ref it before handing
    // it back via an API

    HRESULT hr = E_FAIL;

    if( m_UserStream )
    {
        *ppStream = m_UserStream;
        hr = S_OK;
    }

    return( hr );
}

STDMETHODIMP
CBlockDownload::GetBindInfo
(
    DWORD      * grfBINDF,
    BINDINFO  * pbindinfo
)
{
        HRESULT hr = CBaseBSCB::GetBindInfo( grfBINDF, pbindinfo );

        return(hr);
}
//----------------------------------------------------------
//
//      Download to file
//
//----------------------------------------------------------

//
// This class implements the File downloading code. It reads from the
// stream from urlmon and writes every buffer directly to disk.
//

class CFileDownload : public CBaseBSCB
{
public:
        CFileDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback, LPCWSTR szFileName=0);
        ~CFileDownload();
    void SetFileName(LPCWSTR);

    STDMETHODIMP OnDataAvailable(
             DWORD grfBSCF,
             DWORD dwSize,
             FORMATETC  *pformatetc,
             STGMEDIUM  *pstgmed);

    STDMETHODIMP GetBindInfo(
             DWORD  *grfBINDF,
             BINDINFO  *pbindinfo);

    STDMETHOD(KickOffDownload)( LPCWSTR szURL );

    virtual void Neutralize();

private:
    HRESULT Cleanup();

    unsigned char * m_buffer;
    unsigned long   m_bufsize;
    HANDLE          m_file;
    LPCWSTR         m_filename;
    ULONG           m_okFromCache;

};

inline void
CFileDownload::SetFileName(LPCWSTR newFileName)
{
    // ASSUMES Calls to this class are synchronous

    m_filename = newFileName;
}


CFileDownload::CFileDownload
(
        IUnknown *              caller,
        DWORD                   bscof,
        LPUOSCALLBACK            callback,
        LPCWSTR                 szFileName
)
        : CBaseBSCB(caller, bscof, callback)
{
        m_buffer   = 0;
        m_bufsize  = 0;
        m_file     = INVALID_HANDLE_VALUE;
        m_filename = szFileName;

        m_okFromCache = 0;
}

CFileDownload::~CFileDownload()
{
        Cleanup();
}

STDMETHODIMP
CFileDownload::KickOffDownload( LPCWSTR szURL )
{
    // MAGIC: registry flag determines whether we
    // nuke this guy from the cache or not

    GetDLMRegDWord( _TEXT("CacheOk"), &m_okFromCache );
    return( CBaseBSCB::KickOffDownload(szURL) );
}

HRESULT CFileDownload::Cleanup()
{
        if( m_buffer )
        {
                delete m_buffer;
                m_buffer = 0;
        }

        if( m_file != INVALID_HANDLE_VALUE )
        {
                CloseHandle(m_file);
                m_file = INVALID_HANDLE_VALUE;
        }

        return(NOERROR);
}

STDMETHODIMP
CFileDownload::GetBindInfo
(
        DWORD     * grfBINDF,
        BINDINFO  * pbindinfo
)
{

        HRESULT hr = CBaseBSCB::GetBindInfo( grfBINDF, pbindinfo );

        if( SUCCEEDED(hr) && !m_okFromCache )
                *grfBINDF =  BINDF_PULLDATA;

        return(hr);
}


STDMETHODIMP
CFileDownload::OnDataAvailable
(
    DWORD       grfBSCF,
    DWORD       dwSize,
    FORMATETC  *pformatetc,
    STGMEDIUM  *pstgmed
)
{
    // Pointers are validated in base class

    HRESULT hr = CBaseBSCB::OnDataAvailable(grfBSCF,dwSize,pformatetc,pstgmed);

    if( FAILED(hr) || (dwSize == m_readSoFar) )
        return(hr);

    if( (m_file == INVALID_HANDLE_VALUE)  && dwSize )
    {
        CHECK_POINTER(m_filename);

        hr = MyCreateFile(m_filename,m_file);

        if( FAILED(hr) )
            hr = E_ABORT;
    }

    HANDLE_ABORT(hr);

    UOSASSERT( (m_file != INVALID_HANDLE_VALUE) );

    // Only allocate a read buffer if the one we have is not
    // big enough.

    if( m_buffer && (m_bufsize < (dwSize- m_readSoFar+1)) )
    {
        delete m_buffer;
        m_buffer = 0;
        m_bufsize=0;
    }

    if( !m_buffer )
    {
        m_bufsize=dwSize- m_readSoFar+1;
        DPRINTF( ("Allocating read buffer %d\n",m_bufsize) );
        m_buffer = new unsigned char [(dwSize- m_readSoFar+1)];
    }

    CHECK_MEMORY(m_buffer);

    DWORD dwReadThisMuch = dwSize - m_readSoFar;
    DWORD dwActual       = 0;
    DWORD dwCurrentRead  = 0;

    unsigned char * temp = m_buffer;
    do
    {
        hr = m_UserStream->Read(temp,dwReadThisMuch,&dwActual);
        dwCurrentRead += dwActual;
        dwReadThisMuch -= dwActual;
        temp += dwActual;

    }
    while (!(hr == S_FALSE || hr == E_PENDING) && SUCCEEDED(hr));

    if( dwCurrentRead )
    {
        m_readSoFar += (dwReadThisMuch = dwCurrentRead);

        BOOL bWriteOk = ::WriteFile(
                            m_file, // HANDLE  hFile,    // handle to file to write to
                            m_buffer, // LPCVOID  lpBuffer,    // pointer to data to write to file
                            dwReadThisMuch,        // DWORD  nNumberOfBytesToWrite,    // number of bytes to write
                            &dwActual,    // pointer to number of bytes written
                            0 ); // LPOVERLAPPED  lpOverlapped     // addr. of structure needed for overlapped

        if( !bWriteOk )
            hr = E_FAIL;
    }

    // PUMPREAD(pstgmed->pstm);

    if (grfBSCF & BSCF_LASTDATANOTIFICATION)
    {
        CloseHandle(m_file);
        m_file = INVALID_HANDLE_VALUE;
    }

    return( hr );
}
void
CFileDownload::Neutralize()
{
    // We have to do special cleanup.

    Cleanup();

    CBaseBSCB::Neutralize();
}


//----------------------------------------------------------
//
//      Download to Cache file
//  Implementation of the CCacheFileDownload
//
//----------------------------------------------------------
// This class downloads the file to the cache and returns the cache file name
class CCacheFileDownload : public CBaseBSCB
{
public:
        CCacheFileDownload( IUnknown * caller, DWORD bscof, LPUOSCALLBACK callback, LPCWSTR szFileName=0);
        ~CCacheFileDownload();

    STDMETHODIMP OnDataAvailable(
             DWORD grfBSCF,
             DWORD dwSize,
             FORMATETC  *pformatetc,
             STGMEDIUM  *pstgmed);

    STDMETHODIMP GetBindInfo(
             DWORD  *grfBINDF,
             BINDINFO  *pbindinfo);

    STDMETHOD(KickOffDownload)( LPCWSTR szURL );
private:
        DWORD m_readSoFar;
};

CCacheFileDownload::CCacheFileDownload
(
        IUnknown *              caller,
        DWORD                   bscof,
        LPUOSCALLBACK   callback,
        LPCWSTR                 szFileName
)
        : CBaseBSCB(caller, bscof, callback)
{
        m_readSoFar=0;
}

CCacheFileDownload::~CCacheFileDownload()
{
//      Cleanup();
}

STDMETHODIMP
CCacheFileDownload::KickOffDownload( LPCWSTR szURL )
{
    return( CBaseBSCB::KickOffDownload(szURL) );
}

STDMETHODIMP
CCacheFileDownload::GetBindInfo
(
    DWORD     * grfBINDF,
    BINDINFO  * pbindinfo
)
{

        HRESULT hr = CBaseBSCB::GetBindInfo( grfBINDF, pbindinfo );
        return(hr);
}


STDMETHODIMP
CCacheFileDownload::OnDataAvailable
(
    DWORD       grfBSCF,
    DWORD       dwSize,
    FORMATETC  *pformatetc,
    STGMEDIUM  *pstgmed
)
{
    // Pointers are validated in base class
        HRESULT hr = CBaseBSCB::OnDataAvailable(grfBSCF,dwSize,pformatetc,pstgmed);

        if( FAILED(hr) || (dwSize == m_readSoFar) )
                return(hr);

    IStream *       pstm=pstgmed->pstm;
    if ( pstm && dwSize > m_readSoFar)
        {
        DWORD dwRead = dwSize - m_readSoFar;
        DWORD dwActuallyRead;

        char * lp = new char[dwRead + 1];
            CHECK_MEMORY(lp);
        hr=pstm->Read(lp,dwRead,&dwActuallyRead);

                // Might not be needed
        if(hr!=S_OK)  // If Read Fails then return Error
             if(hr!=E_PENDING)
                 {
                 delete lp;
                 return(hr);
                 }

        delete lp;
        m_readSoFar += (dwRead = dwActuallyRead);

        }
        return( hr );
}



STDAPI
URLOpenPullStreamW
(
    LPUNKNOWN                   caller,
    LPCWSTR                     szURL,
    DWORD                       dwReserved,
    LPUOSCALLBACK               callback
)
{
    CHECK_POINTER(szURL);

    HRESULT hr;

    CPullDownload * download = new CPullDownload(caller,URLOSTRM_NOTIFY_ONDATA,callback);

    CHECK_POINTER(download);

    if( !download )
        hr = E_OUTOFMEMORY;
    else
    {
        download->SetEncodingFlags(dwReserved);
        hr = download->KickOffDownload(szURL);
    }

    return(hr);
}

STDAPI
URLDownloadToFileW
(
    LPUNKNOWN            caller,
    LPCWSTR             szURL,
    LPCWSTR             szFileName,
    DWORD               dwReserved,
    LPUOSCALLBACK       callback
)
{
    CHECK_POINTER(szURL);
        HRESULT hr;


    CFileDownload * strm = new CFileDownload(caller,URLOSTRM_DONOT_NOTIFY_ONDATA,callback,szFileName);

    CHECK_POINTER(strm);


    if( !strm )
        hr = E_OUTOFMEMORY;
    else
        {
        strm->AddRef(); // So that we have valid handle even after OnStopBinding()
        strm->SetEncodingFlags(dwReserved);
        hr = strm->KickOffDownload(szURL);
        }

    if (strm)
        strm->Release();
    return(hr);
}


extern void DoThreadCleanup(BOOL bInThreadDetach);
extern BOOL  g_bNT5OrGreater;

STDAPI
URLDownloadToCacheFileW
(
    LPUNKNOWN            caller,
    LPCWSTR             szURL,
    LPWSTR              szFileName,
    DWORD               dwBufLength,
    DWORD               dwReserved,
    LPUOSCALLBACK       callback
)
{
        CHECK_POINTER(szURL);

        HRESULT hr=S_OK;

        BOOL fFileURL = FALSE;

        if (dwBufLength <= 0)
        {
            hr = E_OUTOFMEMORY; // Buffer length invalid
            return hr;
        }

        //
        // 1,2, and 3 are reserved for the constants
        // URLOSTRM_USECACHE, CACHEONLY and GETNEWESTVERSION constants
        // For Compatibility with previous versions
        //
        dwBufLength = (dwBufLength < 4) ? MAX_PATH : dwBufLength;
        szFileName[0] = NULL;
        // For Cache calls
        MAKE_ANSI(szURL);


        CCacheFileDownload * strm = new CCacheFileDownload(caller,dwBufLength,callback,szFileName);

        CHECK_POINTER(strm);

        if( !strm )
        {
             hr = E_OUTOFMEMORY;
        }
        else
        {
             strm->AddRef(); // So that we have valid handle even after OnStopBinding()
             strm->SetEncodingFlags(dwReserved);
             hr = strm->KickOffDownload(szURL);
        }

        if(SUCCEEDED(hr))
        {
            if (*strm->m_szCacheFileName)
            {
                // If it is a file URL we have to convert it to WIN32 file
                CHAR szPath[MAX_PATH];
                DWORD cchPath = MAX_PATH;

                if(SUCCEEDED(PathCreateFromUrl(strm->m_szCacheFileName, szPath, &cchPath, 0)))
                {
                    fFileURL = TRUE;
                    //  url should now look like a DOS path
                    if (0==MultiByteToWideChar(CP_ACP, 0, 
                                    szPath,-1,
                                    szFileName, dwBufLength))
                    {
                        hr = E_OUTOFMEMORY;
                        szFileName[0] = NULL;
                    }
                    else
                        hr=S_OK;
                }
                if (!fFileURL)
                {
                    if (0==MultiByteToWideChar(CP_ACP, 0, strm->m_szCacheFileName,
                               lstrlen(strm->m_szCacheFileName)+1,
                               szFileName, dwBufLength))
                    {
                         hr = E_OUTOFMEMORY;
                         szFileName[0] = NULL;
                    }
                    else
                         hr=S_OK;
                }
            }
            else
                hr = E_FAIL;
        }

        if (strm)
            strm->Release();

        // WinSE QFE #3411
        // At a minimum, we may need to cleanup the notification hwnd created
        // because this thread can become unresponsive if a broadcast message
        // is sent and the client app isn't pumping messages on this thread.
        // We'll go ahead an do a full tls cleanup.

        // If this is NOT NT5 or greater, then our notification window is not a message window.
        // Clean up our thread data if no other activity on this thread.
        //
        if (!g_bNT5OrGreater)
        {
            DoThreadCleanup(FALSE);
        }

        return(hr);
}


STDAPI
URLOpenBlockingStreamW
(
    LPUNKNOWN            caller,
    LPCWSTR             szURL,
    LPSTREAM*            ppStream,
    DWORD               dwReserved,
    LPUOSCALLBACK       callback
)
{
    CHECK_POINTER(szURL);
    CHECK_POINTER(ppStream);
        HRESULT hr;


    CBlockDownload * strm = new CBlockDownload(caller,URLOSTRM_DONOT_NOTIFY_ONDATA,callback);

    CHECK_POINTER(strm);
    if( !strm )
        hr = E_OUTOFMEMORY;
    else
    {
        strm->AddRef();
        strm->SetEncodingFlags(dwReserved);
        hr = strm->KickOffDownload(szURL);
    }


    if( SUCCEEDED(hr) )
    {
        hr = strm->GetStream(ppStream);

        // We add ref this pointer because we are handing
        // it back to the user

        if( SUCCEEDED(hr) )
           (*ppStream)->AddRef();
    }

    if (strm)
        strm->Release();

    return(hr);
}


STDAPI
URLOpenStreamW
(
    LPUNKNOWN            caller,
    LPCWSTR             szURL,
    DWORD               dwReserved,
    LPUOSCALLBACK       callback
)
{
    CHECK_POINTER(szURL);
        HRESULT hr;


    CPushDownload * strm = new CPushDownload(caller,URLOSTRM_NOTIFY_ONDATA,callback);

    CHECK_POINTER(strm);

    if( !strm )
        hr = E_OUTOFMEMORY;
    else
    {
        strm->SetEncodingFlags(dwReserved);
        hr = strm->KickOffDownload(szURL);
    }

    return(hr);
}


//
//   ANSI VERSION OF PUBLIC API
//

STDAPI
URLOpenPullStreamA
(
    LPUNKNOWN                    caller,
    LPCSTR                      szURL,
    DWORD                       dwReserved,
    LPUOSCALLBACK               callback
)
{
    MAKE_WIDE(szURL);

    return( URLOpenPullStreamW(caller, WIDE_NAME(szURL), dwReserved, callback) );
}

STDAPI
URLDownloadToFileA
(
    LPUNKNOWN            caller,
    LPCSTR              szURL,
    LPCSTR              szFileName,
    DWORD               dwReserved,
    LPUOSCALLBACK       callback
)
{
    MAKE_WIDE(szURL);
    MAKE_WIDE(szFileName);

    return( URLDownloadToFileW( caller, WIDE_NAME(szURL), WIDE_NAME(szFileName),dwReserved, callback ) );
}

STDAPI
URLDownloadToCacheFileA
(
    LPUNKNOWN            caller,
    LPCSTR              szURL,
    LPSTR               szFileName,
    DWORD               dwBufLength,
    DWORD               dwReserved,
    LPUOSCALLBACK       callback
)
{
        HRESULT hr=E_OUTOFMEMORY;

        if (dwBufLength <= 0)
            return hr;

        MAKE_WIDE(szURL);
        LPWSTR lpwszfilename= new WCHAR[MAX_PATH];

        if (lpwszfilename!=NULL)
        {
                hr=URLDownloadToCacheFileW( caller, WIDE_NAME(szURL), lpwszfilename, MAX_PATH, dwReserved, callback );

                if (SUCCEEDED(hr))
                {
                        // Convert to ANSI.
                        dwBufLength = (dwBufLength < 4) ? MAX_PATH : dwBufLength;
                        if (0==WideCharToMultiByte(     CP_ACP, 0, lpwszfilename, lstrlenW(lpwszfilename)+1,szFileName,
                                                                                dwBufLength, NULL, NULL))
                        {
                                hr = E_OUTOFMEMORY;
                                szFileName[0] = NULL;
                        }
                }
                else
                    szFileName[0] = NULL;
        delete[] lpwszfilename;
        }
        return(hr);
}

STDAPI
URLOpenBlockingStreamA
(
    LPUNKNOWN            caller,
    LPCSTR              szURL,
    LPSTREAM*            ppStream,
    DWORD               dwReserved,
    LPUOSCALLBACK       callback
)
{
    MAKE_WIDE(szURL);

    return( URLOpenBlockingStreamW(caller,WIDE_NAME(szURL),ppStream,dwReserved,callback) );
}


STDAPI
URLOpenStreamA
(
    LPUNKNOWN            caller,
    LPCSTR              szURL,
    DWORD               dwReserved,
    LPUOSCALLBACK       callback
)
{
    MAKE_WIDE(szURL);

    return( URLOpenStreamW(caller,WIDE_NAME(szURL),dwReserved,callback) );
}



