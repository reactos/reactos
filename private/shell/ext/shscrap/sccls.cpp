#include "shole.h"
#include "ids.h"
#include "scguid.h"

LONG g_cRefThisDll = 0;         // per-instance

STDAPI_(void) DllAddRef(void)
{
    InterlockedIncrement(&g_cRefThisDll);
}

STDAPI_(void) DllRelease(void)
{
    InterlockedDecrement(&g_cRefThisDll);
}

class CMyClassFactory : public IClassFactory
{
public:
    CMyClassFactory(REFCLSID rclsid);
    ~CMyClassFactory() { DllRelease(); }

    // IUnKnown
    virtual HRESULT __stdcall QueryInterface(REFIID,void **);
    virtual ULONG __stdcall AddRef(void);
    virtual ULONG __stdcall Release(void);

    // IClassFactory
    virtual HRESULT __stdcall CreateInstance(
            IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
    virtual HRESULT __stdcall LockServer(BOOL fLock);

protected:
    UINT   _cRef;
    CLSID  _clsid;
};

CMyClassFactory::CMyClassFactory(REFCLSID rclsid) : _cRef(1), _clsid(rclsid)
{
    DllAddRef();
}

HRESULT CMyClassFactory::QueryInterface(REFIID riid, void **ppvObject)
{
    HRESULT hres;
    if (IsEqualGUID(riid, IID_IClassFactory) || IsEqualGUID(riid, IID_IUnknown)) {
        _cRef++;
        *ppvObject = (LPCLASSFACTORY)this;
        hres = NOERROR;
    }
    else
    {
        *ppvObject = NULL;
        hres = ResultFromScode(E_NOINTERFACE);
    }

    return hres;
}

ULONG CMyClassFactory::AddRef(void)
{
    return ++_cRef;
}

ULONG CMyClassFactory::Release(void)
{
    if (--_cRef>0) {
        return _cRef;
    }

    delete this;
    return 0;
}

HRESULT CMyClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    LPUNKNOWN punk;
    HRESULT hres;
    if (IsEqualGUID(_clsid, CLSID_CScrapData))
    {
        hres = CScrapData_CreateInstance(&punk);
    }
#ifdef FEATURE_SHELLEXTENSION
    else if (IsEqualGUID(_clsid, CLSID_CTemplateFolder))
    {
	hres = CTemplateFolder_CreateInstance(&punk);
    }
    else if (IsEqualGUID(_clsid, CLSID_CScrapExt))
    {
	hres = CScrapExt_CreateInstance(&punk);
    }
#endif
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
    return S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppvOut)
{
    if (IsEqualGUID(rclsid,CLSID_CScrapData)
#ifdef FEATURE_SHELLEXTENSION
	|| IsEqualGUID(rclsid,CLSID_CTemplateFolder)
	|| IsEqualGUID(rclsid,CLSID_CScrapExt)
#endif
	)
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
    if (g_cRefThisDll)
    {
        return S_FALSE;
    }

    DebugMsg(DM_TRACE, TEXT("sc TR - DllCanUnloadNow returning S_OK (bye, bye...)"));

    return S_OK;
}
