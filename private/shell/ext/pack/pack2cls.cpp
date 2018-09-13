#include "priv.h"
#include "privcpp.h"
#include "ids.h"
#include <advpub.h>

#define INITGUID
#include <initguid.h>
#include "packguid.h"

UINT g_cRefThisDll = 0;
HINSTANCE g_hinst = NULL;

STDAPI_(BOOL) DllMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved)
{
    switch(dwReason) {
    case DLL_PROCESS_ATTACH:
        g_hinst = hDll;
#ifdef DEBUG
        CcshellGetDebugFlags();
#endif // DEBUG
    	break;

    default:
    	break;
    }

    return TRUE;
}

void *  __cdecl operator new(unsigned int nSize)
{
	return((LPVOID)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, nSize));
}


void  __cdecl operator delete(void *pv)
{
	LocalFree((HLOCAL)pv);
}


extern "C" int __cdecl _purecall()
{
	return(0);
}

class CMyClassFactory : public IClassFactory
{
public:
    CMyClassFactory(REFCLSID rclsid);
    ~CMyClassFactory() { g_cRefThisDll--; }

    // IUnKnown
    STDMETHODIMP         QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IClassFactory
    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppvObject);
    STDMETHODIMP LockServer(BOOL fLock);

protected:
    UINT   _cRef;
    CLSID  _clsid;
};

CMyClassFactory::CMyClassFactory(REFCLSID rclsid) : _cRef(1), _clsid(rclsid)
{
    g_cRefThisDll++;
}

HRESULT CMyClassFactory::QueryInterface(REFIID riid, void **ppvObject)
{
    HRESULT hres;
    if (IsEqualGUID(riid, IID_IClassFactory) || IsEqualGUID(riid, IID_IUnknown)) 
    {
        DebugMsg(DM_TRACE, "pack cf - QueryInterface called");

	_cRef++;
    	*ppvObject = (IClassFactory *)this;
	hres = NOERROR;
    }
    else
    {
	*ppvObject = NULL;
	hres = E_NOINTERFACE;
    }

    return hres;
}

ULONG CMyClassFactory::AddRef(void)
{
    DebugMsg(DM_TRACE, "pack cf - AddRef called");
    return ++_cRef;
}

ULONG CMyClassFactory::Release(void)
{
    DebugMsg(DM_TRACE, "pack cf - Release called");

    if (--_cRef>0) {
    	return _cRef;
    }

    delete this;
    return 0;
}

HRESULT CMyClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppvObject)
{
    DebugMsg(DM_TRACE, "CMyClassFactory::CreateInstance called");

    if (pUnkOuter)
	return CLASS_E_NOAGGREGATION;

    IUnknown* punk;
    HRESULT hres;
    if (IsEqualGUID(_clsid, CLSID_CPackage))
    {
	hres = CPackage_CreateInstnace(&punk);
    }
    else
    {
	return E_UNEXPECTED;
    }

    if (SUCCEEDED(hres))
    {
	hres = punk->QueryInterface(riid, ppvObject);
	punk->Release();
    }
    return hres;
}

HRESULT CMyClassFactory::LockServer(BOOL fLock)
{
    DebugMsg(DM_TRACE, "pack cf - LockServer called");
    return S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppvOut)
{
    DebugMsg(DM_TRACE, "pack - DllGetClassObject called");

    if (IsEqualGUID(rclsid,CLSID_CPackage))
    {
    	CMyClassFactory *pmycls = new CMyClassFactory(rclsid);
	if (pmycls)
	{
	    HRESULT hres = pmycls->QueryInterface(riid, ppvOut);
	    pmycls->Release();
	    return hres;
	}
	return E_OUTOFMEMORY;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow(void)
{
    DebugMsg(DM_TRACE, "pack - DllCanUnloadNow called");
    if (g_cRefThisDll)
    {
	return S_FALSE;
    }

    DebugMsg(DM_TRACE, "pack - DllCanUnloadNow returning S_OK (bye, bye...)");

    return S_OK;
}

// Call ADVPACK for the given section of our resource based INF>
//   hInstance = resource instance to get REGINST section from
//   szSection = section name to invoke
HRESULT CallRegInstall(HINSTANCE hInstance, LPCSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");
        if ( pfnri )
        {
#ifdef WINNT
            STRENTRY seReg[] =
            {
                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };
            hr = pfnri(hInstance, szSection, &stReg);
#else
            hr = pfnri(hInstance, szSection, NULL);
#endif
        }
        FreeLibrary(hinstAdvPack);
    }
    return hr;
}

STDAPI DllRegisterServer(void)
{
    CallRegInstall(g_hinst, "RegDll");
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    CallRegInstall(g_hinst, "UnregDll");
    return S_OK;
}

