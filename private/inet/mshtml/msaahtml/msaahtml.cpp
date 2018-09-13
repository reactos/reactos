//============================================================================
//      File:   MSAAHTML.CPP
//      Date:   5/23/97
//      Desc:   The in-process server DLL husk for the MSAA registered handler
//              for MSHTML.DLL used by IE 4.0. Includes the standard DLL public 
//              entry points along with the class factory for the MSHTML main 
//              window.
//
//      You will need the NT SUR Beta 2 SDK or VC 4.2 or higher in order to build this 
//      project.  This is because you will need MIDL 3.00.15 or higher and new
//      headers and libs.  If you have VC 4.2 or higher installed, then everything should
//      already be configured correctly.
//
//      NOTE: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f MSAAHTMLps.mk in the project directory.
//
//      Modifications:
// 
//      Date        By      Description
//      -----------------------------------------------------------------
//      5/23/97     Jayc    Created
//
//============================================================================

//============================================================================
// Includes
//============================================================================

#include "stdafx.h"
#include "resource.h"
#include "prxymgr.h"
#include "oleacapi.h"

//============================================================================
//  Constants
//============================================================================

    //---------------------------------------------
    // BUGBUG - Move these into a string table
    //---------------------------------------------

static const TCHAR szClsidKeyDesc[] = TEXT("MSHTML Registered Handler");
static const TCHAR szAccClassName[] = TEXT("Internet Explorer_Server");
static const TCHAR szMSAAHandlers[] = TEXT("SOFTWARE\\Microsoft\\Active Accessibility\\Handlers");
static const TCHAR szOleAccDll[]    = TEXT("OLEACC.DLL");

EXTERN_C const GUID CLSID_MSAAHTML         ={0xe9975030, 0xd326, 0x11d0, {0xbd, 0xe6, 0x00, 0xaa, 0x00, 0x1a, 0x19, 0x53} };

//============================================================================
//  Private Class Declarations
//============================================================================

class CImplHTMLClassFactory : public IClassFactory
{
public:

    STDMETHODIMP            QueryInterface( REFIID riid, void **ppv );
    STDMETHODIMP_(ULONG)    AddRef( void );
    STDMETHODIMP_(ULONG)    Release( void );

    STDMETHODIMP            CreateInstance( IUnknown *pUnkOuter, REFIID riid, void **ppv );
    STDMETHODIMP            LockServer( BOOL fLock );
};

//============================================================================
//  Globals
//============================================================================

HINSTANCE               g_hInstance = NULL;
LONG                    g_cLocks    = 0;
CImplHTMLClassFactory   g_HTMLClassFactory;

    //-----------------------------------------------
    // OLEACC.DLL is dynamically linked.
    //-----------------------------------------------

HMODULE g_hOleAcc = NULL;
INIT_OLEACCAPI_STRUCTURE()

    //-----------------------------------------------
    // The g_pProxyMgr is created once and manages
    //  all instances of running Trident proxies
    //  throughout the lifetime of MSAAHTML.DLL.
    //-----------------------------------------------

CProxyManager   *g_pProxyMgr = NULL;

    //-----------------------------------------------
    // Synchronization objects.
    //-----------------------------------------------

static  CRITICAL_SECTION        s_CritSectCreatePrxyMgr;

    //-----------------------------------------------
    // Thread local storage slot.
    //-----------------------------------------------

#ifdef MSAAHTML_USE_TLS
DWORD   g_dwTLSSlot;
#endif


//============================================================================
//  Static Methods
//============================================================================

//-----------------------------------------------------------------------
//  LoadMSAADlls
//
//  DESCRIPTION:
//
//      Dynamically links to MSAA DLLs and establishes all function
//      pointers to MSAA API used by MSAAHTML.DLL.
//
//  PARAMETERS:
//
//      None.
//
//  RETURNS:
//
//      BOOL.
//
//-----------------------------------------------------------------------

static BOOL LoadMSAADlls( void )
{
    BOOL    bLoadSuccess = TRUE;


    g_hOleAcc = LoadLibrary( szOleAccDll );

    if ( g_hOleAcc )
    {
        for ( int i = 0; i < SIZE_OLEACCAPI; i++ )
        {
            OLEACCAPI_LPFN(i) = GetProcAddress( g_hOleAcc, OLEACCAPI_NAME(i) );

            if ( OLEACCAPI_LPFN(i) == NULL )
            {
                bLoadSuccess = FALSE;
                break;
            }
        }
    }
    else
        bLoadSuccess = FALSE;


    if ( !bLoadSuccess && g_hOleAcc )
    {
        FreeLibrary( g_hOleAcc );
        g_hOleAcc = NULL;
    }


    return( bLoadSuccess );
}

//-----------------------------------------------------------------------
//  UnloadMSAADlls
//
//  DESCRIPTION:
//
//      Frees any loaded MSAA DLLs.
//
//  PARAMETERS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//-----------------------------------------------------------------------

static void UnloadMSAADlls( void )
{
    if ( g_hOleAcc )
        FreeLibrary( g_hOleAcc );
}

//-----------------------------------------------------------------------
//  FreeAllMemory
//
//  DESCRIPTION:
//
//      Frees global memory allocated within these module.
//
//  PARAMETERS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//-----------------------------------------------------------------------

static void FreeAllMemory( void )
{
    ULONG   cRef;

    //------------------------------------------------
    // Since we only create the proxy mgr once, 
    //  our single Release() should destroy it.  A
    //  little debug code to see if this is true, or
    //  anyone else is holding onto it.
    //-------------------------------------------------

    if ( g_pProxyMgr )
    {
        g_pProxyMgr->Zombify();

        cRef = g_pProxyMgr->Release();

#ifdef _DEBUG
        assert( cRef == 0 );

        if ( cRef == 0 )
            g_pProxyMgr = NULL;

#ifndef NDEBUG
        if(g_uObjCount != 0)
        {
            TCHAR szBuf[140];
            wsprintf(szBuf, _T("Object count is %lu. These are not necessarily leaks. Do you want to break?"), g_uObjCount);
            int nRet = MessageBox(NULL, szBuf, _T("MEMORY LEAKS"), MB_YESNO | MB_ICONSTOP);
            if(nRet == IDYES)
            {
                #ifdef _M_IX86
                    __asm int 3;
                #else
                    DebugBreak();
                #endif // _M_IX86
            }
        }
#endif // NDEBUG
#endif
    }

}


#ifdef MSAAHTML_USE_TLS

//-----------------------------------------------------------------------
//  InitializeTLS
//
//  DESCRIPTION:
//
//      Initializes thread local storage slot.
//      An ITypeInfo* is stored in the TLS slot.
//
//  PARAMETERS:
//
//      bProcAttach     Boolean flag indicating whether a
//                      PROCESS_ATTACH or a THREAD_ATTACH is
//                      being handled.
//
//  RETURNS:
//
//      BOOL            TRUE if TLS successfully allocated and set;
//                      FALSE otherwise.
//
//-----------------------------------------------------------------------

static BOOL InitializeTLS( BOOL bProcAttach )
{
    LPVOID  lpvData = NULL;

    if ( bProcAttach )
    {
        g_dwTLSSlot = TlsAlloc();
        if ( g_dwTLSSlot == 0xFFFFFFFF )
            return FALSE;
    }

    //
    // BUGBUG if the TLS is anything more than a typeinfo, we need a struct
    //
    lpvData = (LPVOID) GlobalAlloc( GPTR, sizeof( ITypeInfo* ) );

    if ( lpvData )
    {
        if ( TlsSetValue( g_dwTLSSlot, lpvData ) )
            return TRUE;
        else
            GlobalFree( (HGLOBAL) lpvData );
    }

    if ( bProcAttach )
        TlsFree( g_dwTLSSlot );

    return FALSE;
}


//-----------------------------------------------------------------------
//  ReleaseTLS
//
//  DESCRIPTION:
//
//      Releases thread local storage slot.
//      An ITypeInfo* is stored in the TLS slot.
//
//  PARAMETERS:
//
//      bProcDetach     Boolean flag indicating whether a
//                      PROCESS_DETACH or a THREAD_DETACH is
//                      being handled.
//
//  RETURNS:
//
//      BOOL            TRUE if TLS successfully freed;
//                      FALSE otherwise.
//
//-----------------------------------------------------------------------

static BOOL ReleaseTLS( BOOL bProcDetach )
{
    ITypeInfo**     ppITypeInfo;

    ppITypeInfo = (ITypeInfo**) TlsGetValue( g_dwTLSSlot );

    if ( ppITypeInfo )
    {
        if ( *ppITypeInfo )
            (*ppITypeInfo)->Release();

        GlobalFree( (HGLOBAL) ppITypeInfo );
    }

    if ( bProcDetach )
        TlsFree( g_dwTLSSlot );

    return TRUE;
}

#endif // MSAAHTML_USE_TLS


//============================================================================
//  Public Methods
//============================================================================

//-----------------------------------------------------------------------
//  DllMain 
//
//  DESCRIPTION:
//
//      Main DLL entry point called by COM when the DLL is loaded or unloaded
//      by processes and threads.
//
//  PARAMETERS:d
//
//      See online help.
//
//  RETURNS:
//
//      See online help.
//
// ----------------------------------------------------------------------

extern "C"
BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/ )
{
    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:
#ifdef MSAAHTML_USE_TLS
        if ( !InitializeTLS( TRUE ) )
            return FALSE;
#endif

        LoadMSAADlls();
        
        g_hInstance = hInstance;

        InitializeCriticalSection( &s_CritSectCreatePrxyMgr );

#ifdef _DEBUG
        OutputDebugString( _T("MSAAHTML.DLL registered handler loaded.\n") );
#endif
        break;

    case DLL_THREAD_ATTACH:
#ifdef MSAAHTML_USE_TLS
        if ( !InitializeTLS( FALSE ) )
            return FALSE;
#endif
        break;

    case DLL_THREAD_DETACH:
        if ( g_pProxyMgr )
            g_pProxyMgr->DetachReadyTrees();
#ifdef MSAAHTML_USE_TLS
        ReleaseTLS( FALSE );
#endif
        break;

    case DLL_PROCESS_DETACH:
#ifdef MSAAHTML_USE_TLS
        ReleaseTLS( TRUE );
#endif
        DeleteCriticalSection( &s_CritSectCreatePrxyMgr );
        UnloadMSAADlls();
        FreeAllMemory();
        break;

    default:
        return FALSE;
    }


    return TRUE;
}


//-----------------------------------------------------------------------
//  DllCanUnloadNow
//
//  DESCRIPTION:
//
//      Called by COM periodically to determine whether the DLL can be 
//      unloaded. True when no more objects are being managed, or when
//      no more lock counts exist on the DLL.
//
//  PARAMETERS:
//
//      See online help.
//
//  RETURNS:
//
//      See online help.
//
// ----------------------------------------------------------------------

STDAPI DllCanUnloadNow( void )
{
    return ((g_cLocks == 0) ? S_OK : S_FALSE);
}


//-----------------------------------------------------------------------
//  DllGetClassObject
//
//  DESCRIPTION:
//
//      Called by COM to return a class factory to create an object of the 
//      requested type. We only support a factory for the root Window 
//      accessible object that proxies the main MSHTML.DLL client area.
//
//  PARAMETERS:
//
//      See online help.
//
//  RETURNS:
//
//      See online help.
//
// ----------------------------------------------------------------------

STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID *ppv )
{
    if ( !ppv )
        return( E_INVALIDARG );

    if ( rclsid == CLSID_MSAAHTML )
        return( g_HTMLClassFactory.QueryInterface( riid, ppv ) );

    *ppv = NULL;

    return( CLASS_E_CLASSNOTAVAILABLE );
}

//-----------------------------------------------------------------------
//  DllRegisterServer
//
//  DESCRIPTION:
//
//      Allows the DLL to be self-registering by adding entries to the
//      system registery.
//
//  PARAMETERS:
//
//      See online help.
//
//  RETURNS:
//
//      See online help.
//
//  NOTES:
//
//      No type library support is registered.
//
//-----------------------------------------------------------------------

STDAPI DllRegisterServer( void )
{
    HRESULT     hr;
    LONG        lError;
    OLECHAR     *lpwszClsid;
    TCHAR       szClsid[MAX_PATH],  szClsidKey[MAX_PATH], szThreadingModel[15],
                szBuffer[MAX_PATH], szInprocServer[MAX_PATH];


    //---------------------------------------------------
    // Clean out the registry for MSAAHTML.DLL
    //---------------------------------------------------

	DllUnregisterServer();

    //---------------------------------------------------
    // BUGBUG: move all string literals into string table
    //---------------------------------------------------

    //---------------------------------------------------
    // Register CLSID information for the proxy
    //---------------------------------------------------

    if ( (hr = StringFromCLSID( CLSID_MSAAHTML, &lpwszClsid )) != S_OK )
        return hr;

#ifndef _UNICODE 

    if ( !WideCharToMultiByte(CP_ACP, 0, lpwszClsid, -1, szClsid, (sizeof(szClsid)/sizeof(TCHAR)), NULL, NULL) )
        goto ERROR_EXIT_GETLASTERROR;

#else

    // TODO: this copy is redundant, but gets us to compile under _UNICODE for now
    if ( !_tcscpy( szClsid, lpwszClsid ) )
        goto ERROR_EXIT_GETLASTERROR;

#endif // _UNICODE

    //---------------------------------------------------
    // Add HKEY_CLASSES_ROOT\CLSID key
    //---------------------------------------------------

    if ( !_tcscpy( szClsidKey, TEXT("CLSID\\") ) )
        goto ERROR_EXIT_GETLASTERROR;

    if ( !_tcscat( szClsidKey, szClsid ) )
        goto ERROR_EXIT_GETLASTERROR;

    lError = RegSetValue( HKEY_CLASSES_ROOT, szClsidKey, REG_SZ, szClsidKeyDesc, sizeof(szClsidKeyDesc) );
    if ( lError != ERROR_SUCCESS ) goto ERROR_EXIT;

    //---------------------------------------------------
    // Add HKEY_CLASSES_ROOT\CLSID\InprocServer32 subkey
    //---------------------------------------------------

    if ( !GetModuleFileName( g_hInstance, szInprocServer, MAX_PATH ) )
        goto ERROR_EXIT_GETLASTERROR;

    if ( !_tcscpy( szBuffer, szClsidKey ) )
        goto ERROR_EXIT_GETLASTERROR;

    if ( !_tcscat( szBuffer, TEXT("\\InprocServer32") ) )
        goto ERROR_EXIT_GETLASTERROR;

    lError = RegSetValue( HKEY_CLASSES_ROOT, szBuffer, REG_SZ, szInprocServer, sizeof(szInprocServer) );
    if ( lError != ERROR_SUCCESS ) goto ERROR_EXIT;

    //---------------------------------------------------
    // Add ThreadingModel value
    //---------------------------------------------------

    HKEY pKey;

    lError = RegOpenKey( HKEY_CLASSES_ROOT, szBuffer, &pKey );
    if ( lError != ERROR_SUCCESS ) goto ERROR_EXIT;

    if ( !_tcscpy( szThreadingModel, TEXT("Apartment") ) )
        goto ERROR_EXIT_GETLASTERROR;

    lError = RegSetValueEx( pKey, TEXT("ThreadingModel"), 0, REG_SZ, (BYTE *)szThreadingModel, sizeof(szThreadingModel) );
    if ( lError != ERROR_SUCCESS ) goto ERROR_EXIT;

    lError = RegCloseKey( pKey );

ERROR_EXIT:

    return( lError == ERROR_SUCCESS ? S_OK : MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, lError ) );

ERROR_EXIT_GETLASTERROR:

    return( MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() ) );
}

//-----------------------------------------------------------------------
//  DllUnregisterServer
//
//  DESCRIPTION:
//
//      Allows the DLL to be self-unregistering by removing associated entries 
//      from the system registery.
//
//  PARAMETERS:
//
//      See online help.
//
//  RETURNS:
//
//      See online help.
//
// ----------------------------------------------------------------------

STDAPI DllUnregisterServer( void )
{
    HRESULT     hr;
    LONG        lError;
    HKEY        hKey;
    HKEY        hAccClassNameKey;
    LPOLESTR    lpwszClsid;
    TCHAR       szClsid[MAX_PATH], szClsidKey[MAX_PATH], szBuffer[MAX_PATH];

    //---------------------------------------------------
    // Unregister CLSID information for the proxy
    //---------------------------------------------------

    if ( (hr = StringFromCLSID( CLSID_MSAAHTML, &lpwszClsid )) != S_OK )
        return hr;

#ifndef _UNICODE

    if ( !WideCharToMultiByte(CP_ACP, 0, lpwszClsid, -1, szClsid, (sizeof(szClsid)/sizeof(TCHAR)), NULL, NULL) )
        goto ERROR_EXIT_GETLASTERROR;

#else

    // TODO: this copy is redundant, but gets us to compile under _UNICODE for now
    if ( !_tcscpy( szClsid, lpwszClsid ) )
        goto ERROR_EXIT_GETLASTERROR;

#endif // _UNICODE

    if ( !_tcscpy( szClsidKey, TEXT("CLSID\\") ) )
        goto ERROR_EXIT_GETLASTERROR;

    if ( !_tcscat( szClsidKey, szClsid ) )
        goto ERROR_EXIT_GETLASTERROR;

    lError = RegOpenKey( HKEY_CLASSES_ROOT, szClsidKey, &hKey  );
    if ( lError != ERROR_SUCCESS ) goto ERROR_EXIT;

    lError = RegDeleteKey( hKey, TEXT("InprocServer32") );
    if ( lError != ERROR_SUCCESS ) goto ERROR_EXIT;


    lError = RegOpenKey( hKey, TEXT("AccClassName"), &hAccClassNameKey  );
    if ( lError == ERROR_SUCCESS )
    {
        lError = RegCloseKey( hAccClassNameKey );
        if ( lError != ERROR_SUCCESS ) goto ERROR_EXIT;

        lError = RegDeleteKey( hKey, TEXT("AccClassName") );
        if ( lError != ERROR_SUCCESS ) goto ERROR_EXIT;
    }

    lError = RegCloseKey( hKey );
    if ( lError != ERROR_SUCCESS ) goto ERROR_EXIT;

    lError = RegDeleteKey( HKEY_CLASSES_ROOT, szClsidKey );
    if ( lError != ERROR_SUCCESS ) goto ERROR_EXIT;

    //---------------------------------------------------
    // Unregister handler from MSAA-specific section
    //---------------------------------------------------

    if ( !_tcscpy( szBuffer, szMSAAHandlers ) )
        goto ERROR_EXIT_GETLASTERROR;

    if ( !_tcscat( szBuffer, TEXT("\\")) )
        goto ERROR_EXIT_GETLASTERROR;

    if ( !_tcscat( szBuffer, szClsid ) )
        goto ERROR_EXIT_GETLASTERROR;

    hKey = NULL;
    lError = RegOpenKey( HKEY_LOCAL_MACHINE, szBuffer, &hKey );
    if ( lError == ERROR_SUCCESS )
    {
        lError = RegCloseKey( hKey );
        if ( lError != ERROR_SUCCESS ) goto ERROR_EXIT;

        lError = RegDeleteKey( HKEY_LOCAL_MACHINE, szBuffer );
    }
    else
        lError = ERROR_SUCCESS;

ERROR_EXIT:

    return( lError == ERROR_SUCCESS ? S_OK : MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, lError ) );

ERROR_EXIT_GETLASTERROR:

    return( MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() ) );
}

//============================================================================
//  Private Classes - CImplHTMLClassFactory
//============================================================================

//-----------------------------------------------------------------------
//  CImplHTMLClassFactory::QueryInterface()
//
//  DESCRIPTION:
//
//      Standard QI() implementation for the class factory.
//
//  PARAMETERS:
//
//      See online help.
//
//  RETURNS:
//
//      See online help.
//
// ----------------------------------------------------------------------

STDMETHODIMP CImplHTMLClassFactory::QueryInterface( REFIID riid, void **ppv )
{
    if ( !ppv )
        return( E_INVALIDARG );

    if ( riid == IID_IUnknown || riid == IID_IClassFactory )
        *ppv = (IClassFactory *)this;
    else
    {
        *ppv = NULL;
        return( E_NOINTERFACE );
    }
    
    ((IUnknown *)*ppv)->AddRef();

    return( S_OK );
}

//-----------------------------------------------------------------------
//  CImplHTMLClassFactory::AddRef()
//
//  DESCRIPTION:
//
//      Standard AddRef() implementation
//
//  PARAMETERS:
//
//      See online help.
//
//  RETURNS:
//
//      See online help.
//
// ----------------------------------------------------------------------

STDMETHODIMP_(ULONG) CImplHTMLClassFactory::AddRef( void ) 
{ 
    return( InterlockedIncrement( &g_cLocks ) ); 
}

//-----------------------------------------------------------------------
//  CImplHTMLClassFactory::Release()
//
//  DESCRIPTION:
//
//      Standard Release() implementation
//
//  PARAMETERS:
//
//      See online help.
//
//  RETURNS:
//
//      See online help.
//
// ----------------------------------------------------------------------

STDMETHODIMP_(ULONG) CImplHTMLClassFactory::Release( void ) 
{ 
    return( InterlockedDecrement( &g_cLocks ) ); 
}

//-----------------------------------------------------------------------
//  CImplHTMLClassFactory::CreateInstance()
//
//  DESCRIPTION:
//
//      Standard CreateInstance() implementation
//
//  PARAMETERS:
//
//      See online help.
//
//  RETURNS:
//
//      See online help.
//
// ----------------------------------------------------------------------

STDMETHODIMP CImplHTMLClassFactory::CreateInstance( IUnknown *pUnkOuter, REFIID riid, void **ppv )
{
    HRESULT     hr;


    if ( pUnkOuter != NULL )
        return( CLASS_E_NOAGGREGATION );

    if ( !ppv )
        return( E_INVALIDARG );

    *ppv = NULL;

    //---------------------------------------------------
    // Create one instance of Trident proxy manager. 
    //  TODO: Do we need to AddRef() on it to ensure that
    //    it stays around for the lifetime of the handler,
    //    or at least while one instance of Trident exits?
    //---------------------------------------------------

    hr = S_OK;

    EnterCriticalSection( &s_CritSectCreatePrxyMgr );

    if ( !g_pProxyMgr )
        if ( (g_pProxyMgr = new CProxyManager) == NULL )
            hr = E_OUTOFMEMORY;
    
    LeaveCriticalSection( &s_CritSectCreatePrxyMgr );

    if ( hr != S_OK )
        return hr;


    return g_pProxyMgr->QueryInterface( riid, ppv );
}

//-----------------------------------------------------------------------
//  CImplHTMLClassFactory::LockServer()
//
//  DESCRIPTION:
//
//      Standard LockServer() implementation
//
//  PARAMETERS:
//
//      See online help.
//
//  RETURNS:
//
//      See online help.
//
// ----------------------------------------------------------------------

STDMETHODIMP CImplHTMLClassFactory::LockServer( BOOL bLock )
{
    return( bLock ? InterlockedIncrement( &g_cLocks ) : InterlockedDecrement( &g_cLocks ));
}


//----  End of MSAAHTML.CPP  ----
