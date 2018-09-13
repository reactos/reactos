// DHTMLEd.cpp : Implementation of DLL Exports.
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f DHTMLEdps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "DHTMLEd.h"
#include <TRIEDIID.h>
#include "DHTMLEd_i.c"
#include "DHTMLEdit.h"
#include "DEInsTab.h"
#include "DENames.h"


CComModule _Module;

static void SpikeSharedFileCount ();

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_DHTMLEdit, CDHTMLEdit)
	OBJECT_ENTRY(CLSID_DHTMLSafe, CDHTMLSafe)
	OBJECT_ENTRY(CLSID_DEInsertTableParam, CDEInsertTableParam)
	OBJECT_ENTRY(CLSID_DEGetBlockFmtNamesParam, CDEGetBlockFmtNamesParam)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//	Array of CLSIDs as text to be DELETED when registering the control.
//	These represent no-longer supported GUIDs for interfaces of the past.
//	All GUIDs in this array will be deleted from the HKCR\CLSID section.
//	MAINTENANCE NOTE:
//	When interfaces get new GUIDs (and the old ones are to be invalidated)
//	add the old GUIDs here with appropriate comments.
//
static TCHAR* s_rtszOldClsids [] =
{
	TEXT("{683364AF-B37D-11D1-ADC5-006008A5848C}"),	// Original Edit control GUID
	TEXT("{711054E0-CA70-11D1-8CD2-00A0C959BC0A}"),	// Original Safe for Scripting GUID
	TEXT("{F8A79F00-DA38-11D1-8CD6-00A0C959BC0A}"),	// Intermediate Edit control GUID
	TEXT("{F8A79F01-DA38-11D1-8CD6-00A0C959BC0A}")	// Intermediate Safe for Scripting GUID
};


/////////////////////////////////////////////////////////////////////////////
//
//	Array of CURRENT Interface GUIDS.
//	Note that IIDs and CLSIDs are not equivalent!
//	ATL fails to unregister these when unregisterin the control.
//	MAINTENANCE NOTE:
//	When interface GUIDs are changed, update this array.
//
static TCHAR* s_rtszCurrentInterfaces [] =
{
	TEXT("{CE04B590-2B1F-11d2-8D1E-00A0C959BC0A}"),	// IDHTMLSafe
	TEXT("{CE04B591-2B1F-11d2-8D1E-00A0C959BC0A}"),	// IDHTMLEdit
	TEXT("{47B0DFC6-B7A3-11D1-ADC5-006008A5848C}"),	// IDEInsertTableParam
	TEXT("{8D91090D-B955-11D1-ADC5-006008A5848C}"),	// IDEGetBlockFmtNamesParam
	TEXT("{588D5040-CF28-11d1-8CD3-00A0C959BC0A}"),	// _DHTMLEditEvents
	TEXT("{D1FC78E8-B380-11d1-ADC5-006008A5848C}"),	// _DHTMLSafeEvents
};


//	MAINTENANCE NOTE:
//	If the GUID of the type library changes, update here:
//
static TCHAR* s_tszTypeLibGUID = TEXT("{683364A1-B37D-11D1-ADC5-006008A5848C}");


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
//
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
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


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE
//
STDAPI DllCanUnloadNow(void)
{
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type
//
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry
//
STDAPI DllRegisterServer(void)
{
	HRESULT hr = S_OK;
	CRegKey keyClassID;

	SpikeSharedFileCount ();

	// Unregister old CLSIDs, just in case the user is upgrading without unregistering first.
	hr = keyClassID.Open ( HKEY_CLASSES_ROOT, TEXT("CLSID") );
	_ASSERTE ( SUCCEEDED ( hr ) );
	if ( ERROR_SUCCESS == hr )
	{
		int ctszOldClsids = sizeof ( s_rtszOldClsids ) / sizeof ( TCHAR* );
		for ( int iOldIntf = 0; iOldIntf < ctszOldClsids; iOldIntf++ )
		{
			hr = keyClassID.RecurseDeleteKey ( s_rtszOldClsids [ iOldIntf ] );
		}
		hr = keyClassID.Close ();
	}
	// hr is NOT returned.  Any failure deleting possibly non-existant keys is OK.

	// registers object, typelib and all interfaces in typelib
	return _Module.RegisterServer(TRUE);
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry
//
STDAPI DllUnregisterServer(void)
{
	HRESULT	hr		= S_OK;
	HRESULT hrMod	= _Module.UnregisterServer();

	// Since ATL does not unregister the TypeLib, do it manually.
	CRegKey	keyTypeLib;
	hr = keyTypeLib.Open ( HKEY_CLASSES_ROOT, TEXT("TypeLib") );
	if ( ERROR_SUCCESS == hr )
	{
		hr = keyTypeLib.RecurseDeleteKey ( s_tszTypeLibGUID );
		keyTypeLib.Close ();
	}

	// Delete all current GUIDs from the Interfaces section.  ATL fails to do this, too.
	CRegKey keyInterface;
	hr = keyInterface.Open ( HKEY_CLASSES_ROOT, TEXT("Interface") );
	if ( ERROR_SUCCESS == hr )
	{
		int ctszCurIntf = sizeof ( s_rtszCurrentInterfaces ) / sizeof ( TCHAR* );
		for ( int iCurIntf = 0; iCurIntf < ctszCurIntf; iCurIntf++ )
		{
			hr = keyInterface.RecurseDeleteKey ( s_rtszCurrentInterfaces [ iCurIntf ] );
		}
		hr = keyInterface.Close ();
	}
	// DO NOT RETURN the hr from above! It's OK to fail.

	return hrMod;
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


// End of DHTMLEd.cpp
