// triedit.cpp : Implementation of DLL Exports.
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f trieditps.mk in the project directory.

#include "stdafx.h"

#include <initguid.h>

#include "resource.h"
#include "triedit.h"
#include "triedcid.h"       //IOleCommandTarget CIDs for TriEdit
#include "htmparse.h"
#include "Document.h"
#include "undo.h"
#include "triedit_i.c"

CComModule _Module;

static void SpikeSharedFileCount ();

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_TriEditDocument, CTriEditDocument)
	OBJECT_ENTRY(CLSID_TriEditParse, CTriEditParse)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
		_Module.Term();
	return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	SpikeSharedFileCount ();

	// registers object, typelib and all interfaces in typelib
	return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;
    ITypeLib *  pTypeLib;
    TLIBATTR *  pTypeLibAttr;

	_Module.UnregisterServer();

	// Ideally, we want to fix this using ::GetModuleFileName()
	// but at this point, we want to keep changes to minimal. (1/14/99)
#ifdef _DEBUG
    hr = LoadTypeLib(L"triedit.dll", &pTypeLib);
#else
    hr = LoadTypeLib(L"triedit.dll", &pTypeLib);
#endif

    _ASSERTE(hr == S_OK);
    
    if (hr == S_OK)
    {
        if (pTypeLib->GetLibAttr(&pTypeLibAttr) == S_OK)
        {
            hr = UnRegisterTypeLib(pTypeLibAttr->guid, pTypeLibAttr->wMajorVerNum,
                    pTypeLibAttr->wMinorVerNum, pTypeLibAttr->lcid,
                    pTypeLibAttr->syskind);
            _ASSERTE(hr == S_OK);
    
            pTypeLib->ReleaseTLibAttr(pTypeLibAttr);
        }
        pTypeLib->Release();
    }

    return S_OK;
}

//	Because we've changed from a shared component to a system component, and we're now
//	installed by IE using RollBack rather than reference counting, a serious bug
//	occurs if we're installed once under IE4, IE5 is installed, and the original
//	product is uninstalled.  (We're deleted.  Bug 23681.)
//	This crude but effective routine spikes our reference count to 10000.
//	It doesn't matter so much where we're installed NOW, it matters where the shared
//	component was, or might be, installed.  Even if it's a different copy, the
//	DLL will be unregistered when its reference count is decremented to zero.
//
static void SpikeSharedFileCount ()
{
	CRegKey	keyShared;
	CRegKey	keyCurVer;
	HRESULT	hr = S_OK;

	hr = keyCurVer.Open ( HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion" ) );
	_ASSERTE ( SUCCEEDED ( hr ) );

	if ( FAILED ( hr ) )
	{
		return;	// There's nothing we can do.
	}

	hr = keyShared.Open ( HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls") );
	
	// We expect there to be a SharedDLLs key, but it's possible that there is none.
	if ( FAILED ( hr ) )
	{
		hr = keyShared.Create ( keyCurVer, TEXT("SharedDlls") );
	}

	_ASSERT ( SUCCEEDED ( hr ) );
	if ( SUCCEEDED ( hr ) )
	{
		TCHAR	tszPath[_MAX_PATH];
		TCHAR	tszMod[_MAX_PATH];
		DWORD	cchPath	= _MAX_PATH;
		
		// Build the string X:\Program Files\Common Files\Microsoft Shared\Triedit\dhtmled.ocx
		hr = keyCurVer.QueryValue ( tszPath, TEXT("CommonFilesDir"), &cchPath );
		if ( SUCCEEDED ( hr ) )
		{
			_tcscat ( tszPath, TEXT("\\Microsoft Shared\\Triedit\\") );
			
			// This routine gets the full path name of this DLL.  It SHOULD be the same
			// as the path we're constructing, but that could change in the future, so
			// truncate all but the bare file name.
			if ( 0 != GetModuleFileName ( _Module.GetModuleInstance(), tszMod, _MAX_PATH ) )
			{
				_tcsrev ( tszMod );				// Reverse the string
				_tcstok ( tszMod, TEXT("\\") );	// This replaces the first backslash with a \0.
				_tcsrev ( tszMod );
				_tcscat ( tszPath, tszMod );

				hr = keyShared.SetValue ( 10000, tszPath );
			}
		}
		hr = keyShared.Close ();
	}
	keyCurVer.Close ();
}


#ifdef _ATL_STATIC_REGISTRY
#pragma warning(disable: 4100 4189)	// Necessary for ia64 build
#include <statreg.h>
#include <statreg.cpp>
#pragma warning(default: 4100 4189)	// Necessary for ia64 build
#endif

#include <atlimpl.cpp>
