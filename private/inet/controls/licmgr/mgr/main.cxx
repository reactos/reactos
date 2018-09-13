//+----------------------------------------------------------------------------
//  File:       main.cxx
//
//  Synopsis:   This file contains the core routines and globals for creating
//              LICMGR.DLL
//
//-----------------------------------------------------------------------------


// Includes -------------------------------------------------------------------
#include <mgr.hxx>
#include <factory.hxx>


// Globals --------------------------------------------------------------------
BEGIN_PROCESS_ATTACH
    ATTACH_METHOD(ProcessAttachMIME64)
END_PROCESS_ATTACH

BEGIN_PROCESS_DETACH
END_PROCESS_DETACH

BEGIN_THREAD_ATTACH
END_THREAD_ATTACH

BEGIN_THREAD_DETACH
END_THREAD_DETACH

BEGIN_PROCESS_PASSIVATE
END_PROCESS_PASSIVATE

BEGIN_THREAD_PASSIVATE
END_THREAD_PASSIVATE

BEGIN_CLASS_FACTORIES
    FACTORY(CLSID_LicenseManager, LicenseManagerFactory, NULL)
END_CLASS_FACTORIES

DEFINE_REGISTRY_SECKEY(LicenseManagerCLSID, CLSID, {5220cb21-c88d-11cf-b347-00aa00a28331})
    DEFAULT_VALUE("Microsoft Licensed Class Manager 1.0")
    BEGIN_SUBKEY(Implemented Categories)
        BEGIN_SUBKEY({7DD95801-9882-11CF-9FA9-00AA006C42C4})
        END_SUBKEY
        BEGIN_SUBKEY({7DD95802-9882-11CF-9FA9-00AA006C42C4})
        END_SUBKEY
    END_SUBKEY
    BEGIN_SUBKEY(InprocServer32)
        DEFAULT_VALUE(<m>)
        BEGIN_NAMED_VALUES
            NAMED_VALUE(ThreadingModel, Apartment)
        END_NAMED_VALUES
    END_SUBKEY
    BEGIN_SUBKEY(MiscStatus)
        DEFAULT_VALUE(0)
    END_SUBKEY
    BEGIN_SUBKEY(ProgID)
        DEFAULT_VALUE(License.Manager.1)
    END_SUBKEY
    BEGIN_SUBKEY(Version)
        DEFAULT_VALUE(1.0)
    END_SUBKEY
    BEGIN_SUBKEY(VersionIndependentProgID)
        DEFAULT_VALUE(License.Manager)
    END_SUBKEY
END_REGISTRY_KEY

DEFINE_REGISTRY_KEY(LicenseManagerProgID, License.Manager.1)
    DEFAULT_VALUE("Microsoft Licensed Class Manager 1.0")
    BEGIN_SUBKEY(CLSID)
        DEFAULT_VALUE({5220cb21-c88d-11cf-b347-00aa00a28331})
    END_SUBKEY
END_REGISTRY_KEY

DEFINE_REGISTRY_KEY(LicenseManagerVProgID, License.Manager)
    DEFAULT_VALUE("Microsoft Licensed Class Manager")
    BEGIN_SUBKEY(CurVer)
        DEFAULT_VALUE(License.Manager.1)
    END_SUBKEY
END_REGISTRY_KEY

BEGIN_REGISTRY_KEYS
    REGISTRY_KEY(LicenseManagerCLSID)
    REGISTRY_KEY(LicenseManagerProgID)
    REGISTRY_KEY(LicenseManagerVProgID)
END_REGISTRY_KEYS


//+----------------------------------------------------------------------------
//  Function:   AllocateThreadState
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT
AllocateThreadState(
    THREADSTATE **  ppts)
{
    Assert(ppts);

    *ppts = new THREADSTATE;
    if (!*ppts)
    {
        return E_OUTOFMEMORY;
    }

    memset(*ppts, 0, sizeof(THREADSTATE));
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Function:   LicensedClassManagerFactory
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
HRESULT
LicenseManagerFactory(
    IUnknown *  pUnkOuter,
    REFIID      riid,
    void **     ppvObj)
{
    CLicenseManager *  plcmgr = new CLicenseManager(pUnkOuter);

    if (!plcmgr)
    {
        *ppvObj = NULL;
        return E_OUTOFMEMORY;
    }

    return plcmgr->PrivateQueryInterface(riid, ppvObj);
}


//+----------------------------------------------------------------------------
//
//  Member:     CLicenseManager
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
CLicenseManager::CLicenseManager(
    IUnknown *  pUnkOuter)
 : CComponent(pUnkOuter)
{
    _pUnkSite = NULL;

    _fDirty  = FALSE;
    _fLoaded = FALSE;
    _fPersistPBag   = FALSE;
    _fPersistStream = FALSE;

    _guidLPK = GUID_NULL;
}


//+----------------------------------------------------------------------------
//
//  Member:     ~CLicenseManager
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
CLicenseManager::~CLicenseManager()
{
    int i;

    for (i = _aryLic.Size()-1; i >= 0; i--)
    {
        ::SysFreeString(_aryLic[i].bstrLic);
        ::SRelease(_aryLic[i].pcf2);
    }

    ::SRelease(_pUnkSite);
}


//+----------------------------------------------------------------------------
//
//  Member:     SetSite
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::SetSite(
    IUnknown *  pUnkSite)
{
    ::SClear(&_pUnkSite);

    _pUnkSite = pUnkSite;
    if (_pUnkSite)
    {
        _pUnkSite->AddRef();
    }
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetSite
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::GetSite(
    REFIID  riid,
    void ** ppvSite)
{
    HRESULT hr;

    if (!ppvSite)
        return E_INVALIDARG;

    if (_pUnkSite)
    {
        hr = _pUnkSite->QueryInterface(riid, ppvSite);
    }
    else
    {
        *ppvSite = NULL;
        hr = E_FAIL;
    }
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     SetClientSite
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::SetClientSite(
    IOleClientSite *    pClientSite)
{
    return SetSite(pClientSite);
}


//+----------------------------------------------------------------------------
//
//  Member:     GetClientSite
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::GetClientSite(
    IOleClientSite **   ppClientSite)
{
    return GetSite(IID_IOleClientSite, (void **)ppClientSite);
}


//+----------------------------------------------------------------------------
//
//  Member:     PrivateQueryInterface
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
HRESULT
CLicenseManager::PrivateQueryInterface(
    REFIID  riid,
    void ** ppvObj)
{
    if (riid == IID_IObjectWithSite)
        *ppvObj = (IObjectWithSite *)this;

    else if (riid == IID_IOleObject)
        *ppvObj = (IOleObject *)this;

    else if (riid == IID_ILicensedClassManager)
        *ppvObj = (ILicensedClassManager *)this;

    else if (riid == IID_ILocalRegistry)
        *ppvObj = (ILocalRegistry *)this;

    else if (riid == IID_IRequireClasses)
        *ppvObj = (IRequireClasses *)this;

    else if (riid == IID_IPersistStream && !_fPersistPBag)
    {
        _fPersistStream = TRUE;
        *ppvObj = (IPersistStream *)this;
    }

    else if (riid == IID_IPersistStreamInit && !_fPersistPBag)
    {
        _fPersistStream = TRUE;
        *ppvObj = (IPersistStreamInit *)this;
    }

    else if (riid == IID_IPersistPropertyBag && !_fPersistStream)
    {
        _fPersistPBag = TRUE;
        *ppvObj = (IPersistPropertyBag *)this;
    }

    if (*ppvObj)
        return S_OK;
    else
        return parent::PrivateQueryInterface(riid, ppvObj);
}
