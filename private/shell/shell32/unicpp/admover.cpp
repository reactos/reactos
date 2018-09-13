// ADMover.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f ADMoverps.mk in the project directory.

#include "stdafx.h"
#pragma hdrstop

#include "deskmovr.h"

#ifdef POSTSPLIT

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_DeskMovr, CDeskMovr)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point


// BUGBUG/TODO: Rename this to "atlcreate.cpp" or something more descriptive.

STDAPI_(BOOL) DeskMovr_DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
    }
    return TRUE;    // ok
}

STDAPI CDeskMovr_CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void **ppunk)
{
    return CComCreator< CComPolyObject< CDeskMovr > >::CreateInstance( (LPVOID)pUnkOuter, IID_IUnknown, (LPVOID*)ppunk );
}

STDAPI CWebViewFolderContents_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, LPVOID * ppvOut)
{
    return CComCreator< CComPolyObject< CWebViewFolderContents > >::CreateInstance((LPVOID) punkOuter, riid, ppvOut);
}

STDAPI CShellFolderViewOC_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, LPVOID * ppvOut)
{
    return CComCreator< CComPolyObject< CShellFolderViewOC > >::CreateInstance((LPVOID) punkOuter, riid, ppvOut);
}

#endif
