/*****************************************************************************
 *
 *	ftpcf.cpp - IClassFactory interface
 *
 *****************************************************************************/

#include "priv.h"
#include <initguid.h>
#include "guids.h"
#include "sccls.h"


/*****************************************************************************
 *
 *	CFtpFactory
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
 *	IClassFactory::CreateInstance
 *****************************************************************************/

HRESULT CFtpFactory::CreateInstance(IUnknown * punkOuter, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;

    if (!punkOuter)
    {
        if (IsEqualCLSID(m_rclsid, CLSID_ExeDllColumnProvider))
			hres = CExeDllColumnProvider_CreateInstance(punkOuter, riid, ppvObj);
        else
            if (IsEqualCLSID(m_rclsid, CLSID_FolderShortcutMenu))
                hres= CFShortcutMenu_CreateInstance(punkOuter, riid, ppvObj,1);
            else
                if (IsEqualCLSID(m_rclsid, CLSID_ShortcutMenu))
                    hres= CFShortcutMenu_CreateInstance(punkOuter, riid, ppvObj,0);
                else
					if (IsEqualCLSID(m_rclsid, CLSID_FolderInfoTip))
						hres = CFolderInfoTip_CreateInstance(punkOuter, riid, ppvObj);
					else
						ASSERT(0);  // What are you looking for?
    }
    else
    {		/* Does anybody support aggregation any more? */
	    hres = ResultFromScode(CLASS_E_NOAGGREGATION);
    }

    return hres;
}

/*****************************************************************************
 *
 *	IClassFactory::LockServer
 *
 *	What a stupid function.  Locking the server is identical to
 *	creating an object and not releasing it until you want to unlock
 *	the server.
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
 *	CFtpFactory_Create
 *
 *****************************************************************************/

HRESULT CFtpFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;

    if (IsEqualCLSID(riid, IID_IClassFactory))
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
}


/****************************************************\
    Destructor
\****************************************************/
CFtpFactory::~CFtpFactory()
{
    DllRelease();
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
    if (IsEqualCLSID(riid, IID_IUnknown) || IsEqualCLSID(riid, IID_IClassFactory))
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
