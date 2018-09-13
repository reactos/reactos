//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dll.cpp
//
//  Core entry points for the DLL
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#define INITGUID
#include <initguid.h>
#include "iids.h"

/*----------------------------------------------------------------------------
/ Globals
/----------------------------------------------------------------------------*/

HINSTANCE g_hInstance = NULL;
HINSTANCE g_hAclEditDll = NULL;
DWORD     g_tls = 0xffffffffL;


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
STDAPI_(BOOL)
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*pReserved*/)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hInstance;
        g_tls = TlsAlloc();
        DebugProcessAttach();
        TraceSetMaskFromCLSID(CLSID_DsSecurity);
#ifndef DEBUG
        DisableThreadLibraryCalls(hInstance);
#endif
        break;

    case DLL_PROCESS_DETACH:
        SchemaCache_Destroy();
        if (g_hAclEditDll)
            FreeLibrary(g_hAclEditDll);
        TlsFree(g_tls);
        DebugProcessDetach();
        break;

    case DLL_THREAD_DETACH:
        DebugThreadDetach();
        break;
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
STDAPI
DllCanUnloadNow(void)
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
/   ppv -> receives the newly created object.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
STDAPI
DllGetClassObject(REFCLSID rCLSID, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;
    CDsSecurityClassFactory *pClassFactory;

    TraceEnter(TRACE_CORE, "DllGetClassObject");
    TraceGUID("Object requested", rCLSID);
    TraceGUID("Interface requested", riid);

    *ppv = NULL;

    if (!IsEqualIID(rCLSID, CLSID_DsSecurity))
        ExitGracefully(hr, CLASS_E_CLASSNOTAVAILABLE, "CLSID not supported");

    pClassFactory = new CDsSecurityClassFactory;

    if (!pClassFactory)
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create class factory");

    hr = pClassFactory->QueryInterface(riid, ppv);

    if (FAILED(hr))
        delete pClassFactory;           

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ WaitOnThread
/ -----------------
/   If a thread is running (if the handle is non-NULL) wait for it to complete.
/   Then set the handle to NULL.
/
/ In:
/   phThread = address of thread handle
/
/ Out:
/   Result of WaitForSingleObject, or zero.
/----------------------------------------------------------------------------*/
DWORD
WaitOnThread(HANDLE *phThread)
{
    DWORD dwResult = 0;

    if (phThread != NULL && *phThread != NULL)
    {
        HCURSOR hcurPrevious = SetCursor(LoadCursor(NULL, IDC_WAIT));

        SetThreadPriority(*phThread, THREAD_PRIORITY_HIGHEST);

        dwResult = WaitForSingleObject(*phThread, INFINITE);

        CloseHandle(*phThread);
        *phThread = NULL;

        SetCursor(hcurPrevious);
    }

    return dwResult;
}


/*-----------------------------------------------------------------------------
/ Thread Local Storage helpers
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ ThreadCoInitialize
/ ------------------
/   There is some thread local storage that indicates if we have called
/   CoInitialize.  If CoInitialize has not yet been called, call it now.
/   Otherwise, do nothing.
/
/ In:
/   -
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

HRESULT
ThreadCoInitialize(void)
{
    HRESULT hr = S_OK;

    TraceEnter(TRACE_CORE, "ThreadCoInitialize");

    if (!TlsGetValue(g_tls))
    {
        TraceMsg("Calling CoInitialize");
        hr = CoInitialize(NULL);
        TlsSetValue(g_tls, (LPVOID)SUCCEEDED(hr));
    }

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ ThreadCoUninitialize
/ ------------------
/   There is some thread local storage that indicates if we have called
/   CoInitialize.  If CoInitialize has been called, call CoUninitialize now.
/   Otherwise, do nothing.
/
/ In:
/   -
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

void
ThreadCoUninitialize(void)
{
    TraceEnter(TRACE_CORE, "ThreadCoUninitialize");

    if (TlsGetValue(g_tls))
    {
        TraceMsg("Calling CoUninitialize");
        CoUninitialize();
        TlsSetValue(g_tls, NULL);
    }

    TraceLeaveVoid();
}


//
// Wrappers for delay-loading aclui.dll
//
char const c_szCreateSecurityPage[] = "CreateSecurityPage";
char const c_szEditSecurity[] = "EditSecurity";
typedef HPROPSHEETPAGE (WINAPI *PFN_CREATESECPAGE)(LPSECURITYINFO);
typedef BOOL (WINAPI *PFN_EDITSECURITY)(HWND, LPSECURITYINFO);

HRESULT
_CreateSecurityPage(LPSECURITYINFO pSI, HPROPSHEETPAGE *phPage)
{
    HRESULT hr = E_FAIL;

    if (NULL == g_hAclEditDll)
        g_hAclEditDll = LoadLibrary(c_szAclUI);

    if (g_hAclEditDll)
    {
        static PFN_CREATESECPAGE s_pfnCreateSecPage = NULL;

        if (NULL == s_pfnCreateSecPage)
            s_pfnCreateSecPage = (PFN_CREATESECPAGE)GetProcAddress(g_hAclEditDll, c_szCreateSecurityPage);

        if (s_pfnCreateSecPage)
        {
            hr = S_OK;

            *phPage = (*s_pfnCreateSecPage)(pSI);

            if (NULL == *phPage)
                hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

HRESULT
_EditSecurity(HWND hwndOwner, LPSECURITYINFO pSI)
{
    HRESULT hr = E_FAIL;

    if (NULL == g_hAclEditDll)
        g_hAclEditDll = LoadLibrary(c_szAclUI);

    if (g_hAclEditDll)
    {
        static PFN_EDITSECURITY s_pfnEditSecurity = NULL;

        if (NULL == s_pfnEditSecurity)
            s_pfnEditSecurity = (PFN_EDITSECURITY)GetProcAddress(g_hAclEditDll, c_szEditSecurity);

        if (s_pfnEditSecurity)
        {
            hr = S_OK;
            (*s_pfnEditSecurity)(hwndOwner, pSI);
        }
    }
    return hr;
}

