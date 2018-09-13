//-------------------------------------------------------------------------//
// ShDocProp.cpp : Implementation of DLL Exports.
//-------------------------------------------------------------------------//

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f ShDocPropps.mk in the project directory.

#include "pch.h"
#include "resource.h"
#include "initguid.h"

//  ActiveX control:
#include "PropTree.h"
#include "..\ctl\ctl.h"          // CPropertyTreeCtl
#include "..\cmn\PropTree_i.c"   // IID_/CLSID_PropertyTreeCtl

//  Default property server, shell column provider.
#include "PTsrv32.h"            
#include "..\srv\DefSrv32.h"    // CPTDefSrv32
#include "..\cmn\PTsrv32_i.c"   // CLSID_PTDefaultServer32
#include "..\cmn\PTserver_i.c"  // IID_IPropertyServer

//  Audio/video file property server, shell column provider.
#include "avprop.h"            
#include "..\avprop\srv.h"     //  CAVFilePropServer
#include "..\cmn\avprop_i.c"   // CLSID_AVFilePropServer

//  Shell extension
#include "shdocext.h"           
#include "..\cmn\shdocext_i.c"  // CLSID_ShDocPropExt
#include "ext.h"         	// CShellExt

//-------------------------------------------------------------------------//
CComModule _Module;

//-------------------------------------------------------------------------//
BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_ShDocPropExt, CShellExt)
    OBJECT_ENTRY(CLSID_PropertyTreeCtl, CPropertyTreeCtl)
    OBJECT_ENTRY(CLSID_PTDefaultServer32, CPTDefSrv32)
    OBJECT_ENTRY(CLSID_ExeVerColumnProvider, CExeVerColumnProvider)
    OBJECT_ENTRY(CLSID_AVFilePropServer, CAVFilePropServer)
    OBJECT_ENTRY(CLSID_AVColumnProvider, CAVColumnProvider)
END_OBJECT_MAP()

//-------------------------------------------------------------------------//
// DLL Entry Point
STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
    }
    return TRUE;
}

//-------------------------------------------------------------------------//
// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

//-------------------------------------------------------------------------//
// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv) ;
}

//-------------------------------------------------------------------------//
// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
    // register object, typelib and all interfaces in typelib
    HRESULT hr = _Module.RegisterServer(TRUE) ;
    
    if( SUCCEEDED( hr ) )
    {
        CAVFilePropServer::RegisterFileAssocs() ;
    }
    
    return hr ;
}

//-------------------------------------------------------------------------//
// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}


