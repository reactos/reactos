#include "priv.h"

/*****************************************************************************
 *
 *    CSrvWizFactory
 *
 *
 *****************************************************************************/

class CSrvWizFactory : public IClassFactory
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IClassFactory ***
    virtual STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
    virtual STDMETHODIMP LockServer(BOOL fLock);

public:
    CSrvWizFactory(REFCLSID rclsid);
    ~CSrvWizFactory(void);

    // Friend Functions
    friend HRESULT CSrvWizFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);

protected:
    int                     m_cRef;
    CLSID                   m_rclsid;
};



/*****************************************************************************
 *    IClassFactory::CreateInstance
 *****************************************************************************/

HRESULT CSrvWizFactory::CreateInstance(IUnknown *punkOuter, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres = E_FAIL;

    if (!punkOuter)
    {
        if (IsEqualIID(m_rclsid, CLSID_SrvWiz))
            hres = CSrvWiz_CreateInstance(riid, ppvObj);
    }
    else
    {
        hres = ResultFromScode(CLASS_E_NOAGGREGATION);
    }

    return hres;
}

/*****************************************************************************
 *
 *    IClassFactory::LockServer
 *
 *****************************************************************************/

HRESULT CSrvWizFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();

    return S_OK;
}

/*****************************************************************************
 *
 *    CSrvWizFactory_Create
 *
 *****************************************************************************/

HRESULT CSrvWizFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;

    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (LPVOID) new CSrvWizFactory(rclsid);
        hres = (*ppvObj) ? S_OK : E_OUTOFMEMORY;
    }
    else
        hres = ResultFromScode(E_NOINTERFACE);

    return hres;
}



/****************************************************\
    Constructor
\****************************************************/
CSrvWizFactory::CSrvWizFactory(REFCLSID rclsid) : m_cRef(1)
{
    m_rclsid = rclsid;
    DllAddRef();
}


/****************************************************\
    Destructor
\****************************************************/
CSrvWizFactory::~CSrvWizFactory()
{
    DllRelease();
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CSrvWizFactory::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CSrvWizFactory::Release()
{
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CSrvWizFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppvObj = SAFECAST(this, IClassFactory *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

