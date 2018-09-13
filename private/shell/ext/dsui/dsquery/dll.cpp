#include "pch.h"
#pragma hdrstop

#define INITGUID
#include <initguid.h>
#include "iids.h"


/*----------------------------------------------------------------------------
/ Globals
/----------------------------------------------------------------------------*/

HINSTANCE g_hInstance = 0;
DWORD     g_tls = 0;

HRESULT _OpenSavedDsQuery(LPTSTR pSavedQuery);



/*-----------------------------------------------------------------------------
/ DllMain
/ -------
/   Main entry point.  We are passed reason codes and assored other
/   information when loaded or closed down.
/
/ In:
/   hInstance = our instance handle
/   dwReason = reason code
/   pReserved = depends on the reason code.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
EXTERN_C BOOL DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved)
{
    if ( DLL_PROCESS_ATTACH == dwReason )
    {
        TraceSetMaskFromCLSID(CLSID_DsQuery);
    
        GLOBAL_HINSTANCE = hInstance;
        DisableThreadLibraryCalls(GLOBAL_HINSTANCE);
    }

    return TRUE;
}


/*-----------------------------------------------------------------------------
/ DllCanUnloadNow
/ ---------------
/   Called by the outside world to determine if our DLL can be unloaded. If we
/   have any objects in existance then we must not unload.
/
/ In:
/   -
/ Out:
/   BOOL inidicate unload state.
/----------------------------------------------------------------------------*/
STDAPI DllCanUnloadNow(VOID)
{
    return GLOBAL_REFCOUNT ? S_FALSE : S_OK;
}


/*-----------------------------------------------------------------------------
/ DllGetClassObject
/ -----------------
/   Given a class ID and an interface ID, return the relevant object.  This used
/   by the outside world to access the objects contained here in.
/
/ In:
/   rCLISD = class ID required
/   riid = interface within that class required
/   ppvObject -> receives the newly created object.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

CF_TABLE_BEGIN(g_ObjectInfo)

    // core query handler stuff    
    CF_TABLE_ENTRY( &CLSID_DsQuery, CDsQuery_CreateInstance, COCREATEONLY),

    // start/find and context menu entries
    CF_TABLE_ENTRY( &CLSID_DsFind, CDsFind_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_DsStartFind, CDsFind_CreateInstance, COCREATEONLY),

    // column handler for object class and adspath
    CF_TABLE_ENTRY( &CLSID_PublishedAtCH, CQueryThreadCH_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_ObjectClassCH, CQueryThreadCH_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_MachineRoleCH, CQueryThreadCH_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_MachineOwnerCH, CQueryThreadCH_CreateInstance, COCREATEONLY),

    // domain query form specific column handlers                                                    
    CF_TABLE_ENTRY( &CLSID_PathElement1CH, CDomainCH_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_PathElement3CH, CDomainCH_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_PathElementDomainCH, CDomainCH_CreateInstance, COCREATEONLY),

CF_TABLE_END(g_ObjectInfo)

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        for (LPCOBJECTINFO pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if (IsEqualGUID(rclsid, *(pcls->pclsid)))
            {
                *ppv = (void*)pcls;
                InterlockedIncrement(&GLOBAL_REFCOUNT);
                return NOERROR;
            }
        }
    }

    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}


/*-----------------------------------------------------------------------------
/ DllRegisterServer // DllUnregisterServer
/ ----------------------------------------
/   Called to allow us to setup the registry entries that we use, this
/   takes advantage of the ADVPACK APIs and loads our .inf data from
/   our resource block.
/
/ In:
/   -
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI DllRegisterServer(VOID)
{
    HRESULT hr;

    hr = CallRegInstall(GLOBAL_HINSTANCE, "RegDll");

    if ( SUCCEEDED(hr) )
    {
#if DOWNLEVEL_SHELL
        hr = CallRegInstall(GLOBAL_HINSTANCE, "RegDllWin95");
#else
        hr = CallRegInstall(GLOBAL_HINSTANCE, "RegDllWinNT");
#endif
    }

    return hr;
}

STDAPI DllUnregisterServer(VOID)
{
    return CallRegInstall(GLOBAL_HINSTANCE, "UnRegDll");
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK;
}


/*----------------------------------------------------------------------------
/ Exported (RunDll32) APIs
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ OpenQueryWindow
/ ----------------
/   Opens the query window, parsing the specified CLSID for the form to 
/   select, in the same way invoking Start/Search/<bla>.   
/
/ In:
/   hInstanec, hPrevInstance = instance information
/   pCmdLine = .dsq File to be opened
/   nCmdShow = display flags for our window
/
/ Out:
/   INT
/----------------------------------------------------------------------------*/
INT WINAPI OpenQueryWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, INT nCmdShow)
{
    HRESULT hr, hrCoInit;
    CLSID clsidForm;
    OPENQUERYWINDOW oqw = { 0 };
    DSQUERYINITPARAMS dqip = { 0 };
    ICommonQuery* pCommonQuery = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_CORE, "OpenQueryWindow");

    //
    // get the ICommonQuery object we are going to use
    //

    hr = hrCoInit = CoInitialize(NULL);
    FailGracefully(hr, "Failed to CoInitialize");
   
    hr = CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (LPVOID*)&pCommonQuery);
    FailGracefully(hr, "Failed in CoCreateInstance of CLSID_CommonQuery");

    dqip.cbStruct = SIZEOF(dqip);
    dqip.dwFlags = 0;
    dqip.pDefaultScope = NULL;
    
    oqw.cbStruct = SIZEOF(oqw);
    oqw.dwFlags = 0;
    oqw.clsidHandler = CLSID_DsQuery;
    oqw.pHandlerParameters = &dqip;

    //
    // can we parse the form CLSID from the command line?
    //

    if ( GetGUIDFromString(A2T(pCmdLine), &oqw.clsidDefaultForm) )
    {
        TraceMsg("Parsed out the form CLSID, so specifying the def form/remove forms");
        oqw.dwFlags |= OQWF_DEFAULTFORM|OQWF_REMOVEFORMS;
    }
    
    hr = pCommonQuery->OpenQueryWindow(NULL, &oqw, NULL);
    FailGracefully(hr, "OpenQueryWindow failed");

exit_gracefully:

    DoRelease(pCommonQuery);

    if ( SUCCEEDED(hrCoInit) )
        CoUninitialize();

    TraceLeaveValue(0);
}


/*-----------------------------------------------------------------------------
/ OpenSavedDsQuery
/ ----------------
/   Open a saved DS query and display the query UI with that query.
/
/ In:
/   hInstanec, hPrevInstance = instance information
/   pCmdLine = .dsq File to be opened
/   nCmdShow = display flags for our window
/
/ Out:
/   INT
/----------------------------------------------------------------------------*/

// UNICODE platforms export the W export as the prefered way of invoking the DLL
// on a .QDS, we provide the ANSI version as a thunk to ensure compatibility.

#ifdef UNICODE

INT WINAPI OpenSavedDsQueryW(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLineW, INT nCmdShow)
{
    HRESULT hr;

    TraceEnter(TRACE_CORE, "OpenSavedDsQueryW");
    Trace(TEXT("pCmdLine: %s, nCmdShow %d"), pCmdLineW, nCmdShow);

    hr = _OpenSavedDsQuery(pCmdLineW);
    FailGracefully(hr, "Failed when calling _OpenSavedDsQuery");

    // hr = S_OK;                  // success

exit_gracefully:

    TraceLeaveResult(hr);
}

#endif

INT WINAPI OpenSavedDsQuery(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, INT nCmdShow)
{
    HRESULT hr;
    USES_CONVERSION;
    
    TraceEnter(TRACE_CORE, "OpenSavedDsQuery");
    Trace(TEXT("pCmdLine: %s, nCmdShow %d"), A2T(pCmdLine), nCmdShow);

    hr = _OpenSavedDsQuery(A2T(pCmdLine));
    FailGracefully(hr, "Failed when calling _OpenSavedDsQuery");

    // hr = S_OK;                  // success
        
exit_gracefully:

    TraceLeaveResult(hr);
}

HRESULT _OpenSavedDsQuery(LPTSTR pSavedQuery)
{
    HRESULT hr, hrCoInit;
    ICommonQuery* pCommonQuery = NULL;
    CDsPersistQuery* pDsPersistQuery = NULL;
    OPENQUERYWINDOW oqw;
    DSQUERYINITPARAMS dqip;
    USES_CONVERSION;

    TraceEnter(TRACE_CORE, "OpenSavedQueryW");
    Trace(TEXT("Filename is: "), pSavedQuery);

    hr = hrCoInit = CoInitialize(NULL);
    FailGracefully(hr, "Failed to CoInitialize");

    // Construct the persistance object so that we can load objects from the given file
    // assuming that pSavedQuery is a valid filename.

    pDsPersistQuery = new CDsPersistQuery(pSavedQuery);

    if ( !pDsPersistQuery )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to construct IPersistQuery object");

    // Now lets get the ICommonQuery and get it to open itself based on the 
    // IPersistQuery stream that are giving it.

    hr =CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (LPVOID*)&pCommonQuery);
    FailGracefully(hr, "Failed in CoCreateInstance of CLSID_CommonQuery");

    dqip.cbStruct = SIZEOF(dqip);
    dqip.dwFlags = 0;
    dqip.pDefaultScope = NULL;

    oqw.cbStruct = SIZEOF(oqw);
    oqw.dwFlags = OQWF_LOADQUERY|OQWF_ISSUEONOPEN|OQWF_REMOVEFORMS;
    oqw.clsidHandler = CLSID_DsQuery;
    oqw.pHandlerParameters = &dqip;
    oqw.pPersistQuery = pDsPersistQuery;

    hr = pCommonQuery->OpenQueryWindow(NULL, &oqw, NULL);
    FailGracefully(hr, "OpenQueryWindow failed");

exit_gracefully:

    // Failed so report that this was a bogus query file, however the user may have
    // already been prompted with nothing.

    if ( FAILED(hr) )
    {
        WIN32_FIND_DATA fd;
        HANDLE handle;

        Trace(TEXT("FindFirstFile on: %s"), pSavedQuery);
        handle = FindFirstFile(pSavedQuery, &fd);

        if ( INVALID_HANDLE_VALUE != handle )
        {
            Trace(TEXT("Resulting 'long' name is: "), fd.cFileName);
            pSavedQuery = fd.cFileName;
            FindClose(handle);
        }

        FormatMsgBox(NULL, 
                     GLOBAL_HINSTANCE, IDS_WINDOWTITLE, IDS_ERR_BADDSQ, 
                     MB_OK|MB_ICONERROR, 
                     pSavedQuery);
    }

    DoRelease(pDsPersistQuery);

    if ( SUCCEEDED(hrCoInit) )
        CoUninitialize();

    TraceLeaveValue(0);
}


/*-----------------------------------------------------------------------------
/ Compile external stub functions for:
/   - multi monitor support
/   - delay loading
/----------------------------------------------------------------------------*/

#define COMPILE_MULTIMON_STUBS
#include "multimon.h"

#if 0 
#define COMPILE_DELAYLOAD_STUBS
#include "shdload.h"
#endif

