// ARPCtl.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f ARPCtlps.mk in the project directory.

#include "priv.h"

// Define GUIDs

const IID IID_IARPCtl = {0x0CB57B2B,0xD652,0x11D1,{0xB1,0xDE,0x00,0xC0,0x4F,0xC2,0xA1,0x18}};
const IID LIBID_ARPCTLLib = {0x0CB57B1E,0xD652,0x11D1,{0xB1,0xDE,0x00,0xC0,0x4F,0xC2,0xA1,0x18}};
const IID DIID__ARPCtlEvents = {0x0E5B1C7E,0xD87D,0x11d1,{0xB1,0xDE,0x00,0xC0,0x4F,0xC2,0xA1,0x18}};
const CLSID CLSID_CARPCtl = {0x0CB57B2C,0xD652,0x11D1,{0xB1,0xDE,0x00,0xC0,0x4F,0xC2,0xA1,0x18}};

// Globals
    HINSTANCE g_hInst;

// Root ATL stuff
CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CARPCtl, CARPCtl)
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
        g_hInst = hInstance;

#ifdef DEBUG
    CcshellGetDebugFlags();
       
    if (g_dwBreakFlags & BF_ONDLLLOAD)
        DebugBreak();
#endif
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
	// registers object, typelib and all interfaces in typelib
	return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
	return S_OK;
}


