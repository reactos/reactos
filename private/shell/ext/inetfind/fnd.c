/*****************************************************************************
 *
 *	fnd.c - Find ... On the Internet
 *
 *****************************************************************************/

#include "fnd.h"

/*****************************************************************************
 *
 *	The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflDll

/*****************************************************************************
 *
 *	DllGetClassObject
 *
 *	OLE entry point.  Produces an IClassFactory for the indicated GUID.
 *
 *****************************************************************************/

STDAPI
DllGetClassObject(REFCLSID rclsid, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProc(DllGetClassObject, (_ "G", rclsid));
    if (IsEqualIID(rclsid, &CLSID_Fnd)) {
	hres = CFndFactory_New(riid, ppvObj);
    } else {
	*ppvObj = 0;
	hres = CLASS_E_CLASSNOTAVAILABLE;
    }
    ExitOleProcPpv(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *	DllCanUnloadNow
 *
 *	OLE entry point.  Fail iff there are outstanding refs.
 *
;begin_internal
 *	There is an unavoidable race condition between DllCanUnloadNow
 *	and the creation of a new reference:  Between the time we
 *	return from DllCanUnloadNow() and the caller inspects the value,
 *	another thread in the same process may decide to call
 *	DllGetClassObject, thus suddenly creating an object in this DLL
 *	when there previously was none.
 *
 *	It is the caller's responsibility to prepare for this possibility;
 *	there is nothing we can do about it.
;end_internal
 *
 *****************************************************************************/

STDMETHODIMP
DllCanUnloadNow(void)
{
    SquirtSqflPtszV(sqfl, TEXT("DllCanUnloadNow() - g_cRef = %d"), g_cRef);
    return g_cRef ? S_FALSE : S_OK;
}

/*****************************************************************************
 *
 *	Entry32
 *
 *	DLL entry point.
 *
 *****************************************************************************/

BOOL APIENTRY
Entry32(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
	g_hinst = hinst;
	DisableThreadLibraryCalls(hinst);
#ifdef	DEBUG
	sqflCur = GetProfileInt(TEXT("DEBUG"), TEXT("InetFind"), 0);
	SquirtSqflPtszV(sqfl, TEXT("LoadDll - InetFind"));
#endif
    }
    return 1;
}

/*****************************************************************************
 *
 *	The long-awaited CLSID
 *
 *****************************************************************************/

#include <initguid.h>

DEFINE_GUID(CLSID_Fnd, 0x37865980, 0x75d1, 0x11cf,
		       0xbf,0xc7,0x44,0x45,0x53,0x54,0,0);
