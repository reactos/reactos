/*****************************************************************************
 *
 *	ftp.cpp - FTP folder bookkeeping
 *
 *****************************************************************************/

#include "priv.h"
//#include "ftpinet.h"
//#include "ftpsite.h"
//#include "ftplist.h"
//#include "msieftp.h"
//#include "cookie.h"

extern struct GLOBALTIMEOUTINFO g_gti;
extern DWORD g_dwOpenConnections;

/*****************************************************************************
 *
 *	Dynamic Globals.  There should be as few of these as possible.
 *
 *	All access to dynamic globals must be thread-safe.
 *
 *****************************************************************************/

ULONG g_cRef = 0;			/* Global reference count */
CRITICAL_SECTION g_csDll;	/* The shared critical section */


#ifdef DEBUG
DWORD g_TlsMem = 0xffffffff;

#endif // DEBUG

ULONG g_cRef_CFtpView = 0;  // Needed to determine when to purge cache.

/*****************************************************************************
 *
 *	DllAddRef / DllRelease
 *
 *	Maintain the DLL reference count.
 *
 *****************************************************************************/

void DllAddRef(void)
{
    InterlockedIncrement((LPLONG)&g_cRef);
}

void DllRelease(void)
{
    InterlockedDecrement((LPLONG)&g_cRef);
}

/*****************************************************************************
 *
 *	DllGetClassObject
 *
 *	OLE entry point.  Produces an IClassFactory for the indicated GUID.
 *
 *	The artificial refcount inside DllGetClassObject helps to
 *	avoid the race condition described in DllCanUnloadNow.  It's
 *	not perfect, but it makes the race window much smaller.
 *
 *****************************************************************************/

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;

    DllAddRef();
//    if (IsEqualIID(rclsid, CLSID_FtpFolder) ||
//        IsEqualIID(rclsid, CLSID_FtpWebView))
    {
	    hres = CFtpFactory_Create(rclsid, riid, ppvObj);
    }
//    else
//    {
//	    *ppvObj = NULL;
//	    hres = CLASS_E_CLASSNOTAVAILABLE;
//    }

    DllRelease();
    return hres;
}

/*****************************************************************************
 *
 *	DllCanUnloadNow
 *
 *	OLE entry point.  Fail iff there are outstanding refs.
 *
 *	There is an unavoidable race condition between DllCanUnloadNow
 *	and the creation of a new IClassFactory:  Between the time we
 *	return from DllCanUnloadNow() and the caller inspects the value,
 *	another thread in the same process may decide to call
 *	DllGetClassObject, thus suddenly creating an object in this DLL
 *	when there previously was none.
 *
 *	It is the caller's responsibility to prepare for this possibility;
 *	there is nothing we can do about it.
 *
 *****************************************************************************/

STDMETHODIMP DllCanUnloadNow(void)
{
    HRESULT hres;

    ENTERCRITICAL;

    hres = g_cRef ? S_FALSE : S_OK;

    TraceMsg(TF_FTP_DLLLOADING, "DllCanUnloadNow() returning hres=%#08lx. (S_OK means yes)", hres);

    LEAVECRITICAL;

    return hres;
}


/*****************************************************************************
 *
 *	Entry32
 *
 *	DLL entry point.
 *
 *	BUGBUG -- On a thread detach, must check if the thread owns any
 *	global timeouts.  If so, we must transfer the timeout to another
 *	thread or something.
 *
 *****************************************************************************/
STDAPI_(BOOL) DllEntry(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    static s_hresOle = E_FAIL;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        InitializeCriticalSection(&g_csDll);
	g_hinst = hinst;
	DisableThreadLibraryCalls(hinst);
        break;

    case DLL_PROCESS_DETACH:
        DeleteCriticalSection(&g_csDll);
        break;
    }
    return 1;
}




const CLSID CLSID_FtpFolder = {0x63da6ec0, 0x2e98, 0x11cf, 0x8d,0x82,0x44,0x45,0x53,0x54,0,0};
