//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       LIBMAIN.CXX
//
//  Contents:   DLL implementation for MSHTMLED
//
//  History:    7-Jan-98   raminh  Created
//             12-Mar-98   raminh  Converted over to use ATL
//-------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef X_INITGUID_H_
#define X_INITGUID_H_
#include "initguid.h"
#endif

#ifndef X_OptsHold_H_
#define X_OptsHold_H_
#include "optshold.h"
#endif

#include "optshold_i.c"

#ifndef X_DLGHELPR_H_
#define X_DLGHELPR_H_
#include "dlghelpr.h"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef _X_EDTRACK_HXX_
#define _X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"
#endif

#ifndef X_EDUTIL_H_
#define X_EDUTIL_H_
#include "edutil.hxx"
#endif

#ifndef X_EDCMD_H_
#define X_EDCMD_H_
#include "edcmd.hxx"
#endif

#ifndef X_BLOCKCMD_H_
#define X_BLOCKCMD_H_
#include "blockcmd.hxx"
#endif

#ifndef _X_MISCCMD_HXX_
#define _X_MISCCMD_HXX_
#include "misccmd.hxx"
#endif

#ifndef NO_IME
#ifndef X_DIMM_H_
#define X_DIMM_H_
#include "dimm.h"
#endif
#endif

MtDefine(OpNewATL, Mem, "operator new (mshtmled ATL)")

//
// Misc stuff to keep the linker happy
//
DWORD               g_dwFALSE = 0;          // Used for assert to fool the compiler
EXTERN_C HANDLE     g_hProcessHeap = NULL;  // g_hProcessHeap is set by the CRT in dllcrt0.c
HINSTANCE           g_hInstance = NULL;
HINSTANCE           g_hEditLibInstance = NULL;

// _purecall() is here to avoid linking unwanted CRT code such as assert etc.
extern "C" {
int __cdecl  _purecall(void)
{
    return(FALSE);
}
};

// Below is the trick used to make ATL use our memory allocator
_CRTIMP void    __cdecl x_free(void * pv) { MemFree(pv); }
_CRTIMP void *  __cdecl x_malloc(size_t cb) { return(MemAlloc(Mt(OpNewATL), cb)); }
_CRTIMP void *  __cdecl x_realloc(void * pv, size_t cb)
{
    void * pvNew = pv;
    HRESULT hr = MemRealloc(Mt(OpNewATL), &pvNew, cb);
    return(hr == S_OK ? pvNew : NULL);
}

//
// For the Active IMM (aka DIMM)
//
// BUGBUG (cthrash) This object is cocreated inside mshtml.  We need to get the
// pointer from mshtml because DIMM needs to intercept IMM32 calls. Currently
// DIMM functionality is crippled because we never set g_pActiveIMM.
//

#ifndef NO_IME
CRITICAL_SECTION g_csActiveIMM ; // Protect access to IMM
int g_cRefActiveIMM = 0;    // Local Ref Count for acess to IMM
IActiveIMMApp * g_pActiveIMM = NULL;
BOOL HasActiveIMM() { return g_pActiveIMM != NULL; }
IActiveIMMApp * GetActiveIMM() { return g_pActiveIMM; }
#endif

#if defined(QUILL) || defined(TREE_SYNC)
const CLSID CLSID_QHTMLEditor = {0x3FCCC081,0xAA1E,0x11d1,0x8E,0x02,0x00,0xA0,0xC9,0x1B,0xC8,0xEC};
#endif

//
// CComModule and Object map for ATL
//

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_HtmlDlgHelper, CHtmlDlgHelper)
    OBJECT_ENTRY(CLSID_HTMLEditor, CHTMLEditor)
#if defined(QUILL) || defined(TREE_SYNC)
    OBJECT_ENTRY(CLSID_QHTMLEditor, CHTMLEditor)
#endif
END_OBJECT_MAP()

#ifdef UNIX
void DeinitDynamicLibraries();
#endif

//+----------------------------------------------------------------------------
//
// Function: DllMain
//
//+----------------------------------------------------------------------------
#ifdef UNIX
extern "C"
#endif
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        OSVERSIONINFOA ovi;
        
#ifdef UNIX // Unix setup doesn't invoke COM. Need to do it here.
        CoInitialize(NULL);
#endif
        _Module.Init(ObjectMap, hInstance);
        CSelectTracker::InitMetrics();
        DisableThreadLibraryCalls(hInstance);
        g_hInstance = hInstance;
        CGetBlockFmtCommand::Init();
        CComposeSettingsCommand::Init();

        // Init platform information
        ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        Verify(GetVersionExA(&ovi));

        g_dwPlatformVersion = (ovi.dwMajorVersion << 16) + ovi.dwMinorVersion;
        g_dwPlatformID      = ovi.dwPlatformId;

#if DBG==1

    //  Tags for the .dll should be registered before
    //  calling DbgExRestoreDefaultDebugState().  Do this by
    //  declaring each global tag object or by explicitly calling
    //  DbgExTagRegisterTrace.

    DbgExRestoreDefaultDebugState();

#endif // DBG==1

#ifdef UNIX
        CoUninitialize();
#endif
#ifndef NO_IME
        InitializeCriticalSection(&g_csActiveIMM);
#endif
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
#ifndef UNIX    // because of the extern "C" change above
        extern void DeinitDynamicLibraries();
#endif
        _Module.Term();
        CGetBlockFmtCommand::Deinit();
        CComposeSettingsCommand::Deinit();
        DeinitDynamicLibraries();

#ifndef NO_IME
        DeleteCriticalSection( & g_csActiveIMM );
#endif        
    }
    return TRUE;    
}

//+----------------------------------------------------------------------------
//
// Function: DllCanUnloadNow
//
// Used to determine whether the DLL can be unloaded by OLE
//+----------------------------------------------------------------------------
STDAPI 
DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

//+----------------------------------------------------------------------------
//
// Function: DllClassObject
//
// Returns a class factory to create an object of the requested type
//+----------------------------------------------------------------------------
STDAPI 
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

//+----------------------------------------------------------------------------
//
// Function: DllRegisterServer
//
// Adds entries to the system registry
//+----------------------------------------------------------------------------
STDAPI 
DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    DbgMemoryTrackDisable(TRUE);
    HRESULT hr = _Module.RegisterServer(TRUE);
    DbgMemoryTrackDisable(FALSE);
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// Function: DllUnRegisterServer
//
// Removes entries from the system registry
//+----------------------------------------------------------------------------
STDAPI 
DllUnregisterServer(void)
{
    DbgMemoryTrackDisable(TRUE);
    THR_NOTRACE( _Module.UnregisterServer() );
    DbgMemoryTrackDisable(FALSE);
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Function:   DllEnumClassObjects
//
//  Synopsis:   Given an index in class object map, returns the
//              corresponding CLSID and IUnknown
//              This function is used by MsHtmpad to CoRegister
//              local class ids. 
//
//----------------------------------------------------------------
STDAPI
DllEnumClassObjects(int i, CLSID *pclsid, IUnknown **ppUnk)
{
    HRESULT             hr      = S_FALSE;
    _ATL_OBJMAP_ENTRY * pEntry  = _Module.m_pObjMap;
    
    if (!pEntry)
        goto Cleanup;

    pEntry += i;

    if (pEntry->pclsid == NULL)
        goto Cleanup;

    memcpy(pclsid, pEntry->pclsid, sizeof( CLSID ) );
    hr = THR( DllGetClassObject( *pclsid, IID_IUnknown, (void **)ppUnk) );

Cleanup:
    return hr;
}

