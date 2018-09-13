//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft, 1997
//
//  File:       PrevBand.cpp
//
//  Contents:   Implementation of DLL Exports
//
//  History:    7-24-97  Davepl  Created
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "PrevBand.h"
#include "PrevBand_i.c"
#include "PreviewBand.h"

// Proxy/Stub Information:
// To build a separate proxy/stub DLL, 
// run nmake -f PrevBandps.mk in the project directory.

// The one and only CComModule object

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_PreviewBand, CPreviewBand)
END_OBJECT_MAP()

// DllMain
//
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

// DllCanUnloadNow 
//
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

// DllGetClassObject
//
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}

// DllRegisterServer 
//
// Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // Registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

// DllUnregisterServer 
//
// Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
	return S_OK;
}


