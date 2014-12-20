#include "precomp.h"

class CNetshellClassFactory :
    public IClassFactory
{
    public:
        CNetshellClassFactory(REFCLSID rclsid);
        
        /* IUnknown */
        virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID *ppvOut);
        virtual ULONG WINAPI AddRef();
        virtual ULONG WINAPI Release();
        
        /* IClassFactory */
        virtual HRESULT WINAPI CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject);
        virtual HRESULT WINAPI LockServer(BOOL fLock);
    
    private:
        LONG ref;
        CLSID clsid;
};

CNetshellClassFactory::CNetshellClassFactory(REFCLSID rclsid)
{
    ref = 0;
    clsid = rclsid;
}

HRESULT
WINAPI
CNetshellClassFactory::QueryInterface(
    REFIID riid,
    LPVOID *ppvObj)
{
    *ppvObj = NULL;
    if(IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppvObj = (IClassFactory*)this;
        InterlockedIncrement(&ref);
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG
WINAPI
CNetshellClassFactory::AddRef()
{
    ULONG refCount = InterlockedIncrement(&ref);

    return refCount;
}

ULONG
WINAPI
CNetshellClassFactory::Release()
{
    ULONG refCount = InterlockedDecrement(&ref);

    if (!refCount)
    {
        CoTaskMemFree(this);
        return 0;
    }
    return refCount;
}

HRESULT
WINAPI
CNetshellClassFactory::CreateInstance(
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualCLSID(clsid, CLSID_NetworkConnections))
        return ISF_NetConnect_Constructor(pUnkOuter, riid, ppvObject);
    else if (IsEqualCLSID(clsid, CLSID_ConnectionManager))
        return INetConnectionManager_Constructor(pUnkOuter, riid, ppvObject);
    else if (IsEqualCLSID(clsid, CLSID_LANConnectUI))
        return LanConnectUI_Constructor(pUnkOuter, riid, ppvObject);
    else if (IsEqualCLSID(clsid, CLSID_LanConnectStatusUI))
        return LanConnectStatusUI_Constructor(pUnkOuter, riid, ppvObject);

    return E_NOINTERFACE;
}

HRESULT
WINAPI
CNetshellClassFactory::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}

HRESULT IClassFactory_fnConstructor(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
    CNetshellClassFactory *pClsFactory = new CNetshellClassFactory(rclsid);
    if (!pClsFactory)
        return E_OUTOFMEMORY;

    pClsFactory->AddRef();
    HRESULT hr = pClsFactory->QueryInterface(riid, ppvOut);
    pClsFactory->Release();

    return hr;
}
