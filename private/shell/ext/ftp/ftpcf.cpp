/*****************************************************************************
 *
 *    ftpcf.cpp - IClassFactory interface
 *
 *****************************************************************************/

#include "priv.h"
#include "ftpwebvw.h"
#include "msieftp.h"


/*****************************************************************************
 *
 *    CFtpFactory
 *
 *
 *****************************************************************************/

class CFtpFactory       : public IClassFactory
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IClassFactory ***
    virtual STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
    virtual STDMETHODIMP LockServer(BOOL fLock);

public:
    CFtpFactory(REFCLSID rclsid);
    ~CFtpFactory(void);

    // Friend Functions
    friend HRESULT CFtpFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);

protected:
    int                     m_cRef;
    CLSID                   m_rclsid;
};



/*****************************************************************************
 *    IClassFactory::CreateInstance
 *****************************************************************************/

HRESULT CFtpFactory::CreateInstance(IUnknown * punkOuter, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres = ResultFromScode(REGDB_E_CLASSNOTREG);

    if (!punkOuter)
    {
        if (IsEqualIID(m_rclsid, CLSID_FtpFolder))
            hres = CFtpFolder_Create(riid, ppvObj);
        else if (IsEqualIID(m_rclsid, CLSID_FtpWebView))
            hres = CFtpWebView_Create(riid, ppvObj);
        else if (IsEqualIID(m_rclsid, CLSID_FtpInstaller))
            hres = CFtpInstaller_Create(riid, ppvObj);
        else if (IsEqualIID(m_rclsid, CLSID_FtpDataObject))
            hres = CFtpObj_Create(riid, ppvObj);
        else
            ASSERT(0);  // What are you looking for?
    }
    else
    {        // Does anybody support aggregation any more?
        hres = ResultFromScode(CLASS_E_NOAGGREGATION);
    }

    return hres;
}

/*****************************************************************************
 *
 *    IClassFactory::LockServer
 *
 *    What a stupid function.  Locking the server is identical to
 *    creating an object and not releasing it until you want to unlock
 *    the server.
 *
 *****************************************************************************/

HRESULT CFtpFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();

    return S_OK;
}

/*****************************************************************************
 *
 *    CFtpFactory_Create
 *
 *****************************************************************************/

HRESULT CFtpFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;

    if (GetShdocvwVersion() < 5)
    {
        // Check if we are running under older IE's and fail so that
        // side by side IE4, IE5 can work
        hres = ResultFromScode(E_NOINTERFACE);
    }
    else if (IsEqualIID(riid, IID_IClassFactory))
    {
        *ppvObj = (LPVOID) new CFtpFactory(rclsid);
        hres = (*ppvObj) ? S_OK : E_OUTOFMEMORY;
    }
    else
        hres = ResultFromScode(E_NOINTERFACE);

    return hres;
}





/****************************************************\
    Constructor
\****************************************************/
CFtpFactory::CFtpFactory(REFCLSID rclsid) : m_cRef(1)
{
    m_rclsid = rclsid;
    DllAddRef();
    LEAK_ADDREF(LEAK_CFtpFactory);
}


/****************************************************\
    Destructor
\****************************************************/
CFtpFactory::~CFtpFactory()
{
    DllRelease();
    LEAK_DELREF(LEAK_CFtpFactory);
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CFtpFactory::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CFtpFactory::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CFtpFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppvObj = SAFECAST(this, IClassFactory *);
    }
    else
    {
        TraceMsg(TF_FTPQI, "CFtpFactory::QueryInterface() failed.");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}
