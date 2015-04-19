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
        LONG m_ref;
        CLSID m_clsid;
};

CNetshellClassFactory::CNetshellClassFactory(REFCLSID rclsid) :
    m_ref(0),
    m_clsid(rclsid)
{
}

HRESULT
WINAPI
CNetshellClassFactory::QueryInterface(
    REFIID riid,
    LPVOID *ppvObj)
{
    *ppvObj = NULL;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppvObj = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG
WINAPI
CNetshellClassFactory::AddRef()
{
    ULONG refCount = InterlockedIncrement(&m_ref);

    return refCount;
}

ULONG
WINAPI
CNetshellClassFactory::Release()
{
    ULONG refCount = InterlockedDecrement(&m_ref);

    if (!refCount)
        CoTaskMemFree(this);

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

    if (IsEqualCLSID(m_clsid, CLSID_NetworkConnections))
        return ISF_NetConnect_Constructor(pUnkOuter, riid, ppvObject);
    else if (IsEqualCLSID(m_clsid, CLSID_ConnectionManager))
        return INetConnectionManager_Constructor(pUnkOuter, riid, ppvObject);
    else if (IsEqualCLSID(m_clsid, CLSID_LANConnectUI))
        return LanConnectUI_Constructor(pUnkOuter, riid, ppvObject);
    else if (IsEqualCLSID(m_clsid, CLSID_LanConnectStatusUI))
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
