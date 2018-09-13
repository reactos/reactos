//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       urlcf.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <eapp.h>

#ifdef EAPP_TEST
const GUID CLSID_ResProtocol =     {0x79eaca00, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b}};
const GUID CLSID_OhServNameSp =    {0x79eaca01, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b}};
const GUID CLSID_MimeHandlerTest1= {0x79eaca02, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b}};

const GUID CLSID_NotificaitonTest1  = {0xc733e501, 0x576e, 0x11d0, {0xb2, 0x8c, 0x00, 0xc0, 0x4f, 0xd7, 0xcd, 0x22}};
const GUID CLSID_NotificaitonTest2  = {0xc733e502, 0x576e, 0x11d0, {0xb2, 0x8c, 0x00, 0xc0, 0x4f, 0xd7, 0xcd, 0x22}};
const GUID CLSID_NotificaitonTest3  = {0xc733e503, 0x576e, 0x11d0, {0xb2, 0x8c, 0x00, 0xc0, 0x4f, 0xd7, 0xcd, 0x22}};
const GUID CLSID_NotificaitonTest4  = {0xc733e504, 0x576e, 0x11d0, {0xb2, 0x8c, 0x00, 0xc0, 0x4f, 0xd7, 0xcd, 0x22}};
#endif // EAPP_TEST

const GUID CLSID_StdEncodingFilterFac= {0x54c37cd0, 0xd944, 0x11d0, {0xa9, 0xf4, 0x00, 0x60, 0x97, 0x94, 0x23, 0x11}};
const GUID CLSID_CdlProtocol  =    {0x3dd53d40, 0x7b8b, 0x11d0, {0xb0, 0x13, 0x00, 0xaa, 0x00, 0x59, 0xce, 0x02}};
const GUID CLSID_DeCompMimeFilter   = {0x8f6b0360, 0xb80d, 0x11d0, {0xa9, 0xb3, 0x00, 0x60, 0x97, 0x94, 0x23, 0x11}};
const GUID CLSID_ClassInstallFilter = {0x32b533bb, 0xedae, 0x11d0, {0xbd, 0x5a, 0x0, 0xaa, 0x0, 0xb9, 0x2a, 0xf1}};

#ifdef EAPP_TEST
HRESULT CreateNotificationTest(DWORD dwId, REFCLSID rclsid, IUnknown *pUnkOuter, REFIID riid, IUnknown **ppUnk);
#endif

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::Create
//
//  Synopsis:
//
//  Arguments:  [clsid] --
//              [ppCF] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CUrlClsFact::Create(REFCLSID clsid, CUrlClsFact **ppCF)
{
    EProtDebugOut((DEB_PLUGPROT, "NULL _IN CUrlClsFact::Create\n"));
    HRESULT hr = NOERROR;
    CUrlClsFact * pCF =  NULL;

#ifdef EAPP_TEST
    if (   (clsid == CLSID_ResProtocol)
        || (clsid == CLSID_OhServNameSp)
       )
    {
        pCF = (CUrlClsFact *) new CUrlClsFact(clsid);
    }
    else if
        (
            (clsid == CLSID_NotificaitonTest1)
         || (clsid == CLSID_NotificaitonTest2)
         || (clsid == CLSID_NotificaitonTest3)
         || (clsid == CLSID_NotificaitonTest4)
         || (clsid == CLSID_DeCompMimeFilter )
         || (clsid == CLSID_MimeHandlerTest1)
         )
    {
        pCF = (CUrlClsFact *) new CUrlClsFact(clsid);
    }
#endif

    if(   (clsid == CLSID_DeCompMimeFilter )
       || (clsid == CLSID_StdEncodingFilterFac)
       || (clsid == CLSID_ClassInstallFilter)
       || (clsid == CLSID_CdlProtocol)
       )
    {
        pCF = (CUrlClsFact *) new CUrlClsFact(clsid);
    }


    if (pCF == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        *ppCF = pCF;
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CUrlClsFact::Create (hr:%lx\n", pCF,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::CUrlClsFact
//
//  Synopsis:   constructor
//
//  Arguments:  [clsid] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:      we need to keep a refcount on the dll if for each object given to
//              outside, including ClassFactories.
//              The corresponding DllRelease is in the destructor
//
//----------------------------------------------------------------------------
CUrlClsFact::CUrlClsFact(REFCLSID clsid) : _CRefs(), _CLocks(0)
{
    _ClsID =  clsid;
    DllAddRef();
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::~CUrlClsFact
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CUrlClsFact::~CUrlClsFact()
{
    DllRelease();
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::CreateInstance
//
//  Synopsis:   creates an instance of an Explode Object
//
//  Arguments:  [pUnkOuter] -- controlling unknown (must be NULL)
//              [riid] --      id of desired interface
//              [ppv] --       pointer to receive the interface
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:      S_OK - success
//              CLASS_E_NOAGGREATION - the caller tried to aggregate
//              CLASS_E_CLASSNOTAVAILABLE - couldn't initialize the class
//              E_OUTOFMEMORY - not enough memory to instantiate class
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlClsFact::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID * ppv)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CUrlClsFact::CreateInstance\n", this));
    HRESULT hr = NOERROR;
    // Class factory init time, the pointer to the creation function of
    // the object is given.  Use that to create the object

    //DumpIID(riid);
    //DumpIID(_rClsID);

    if (riid == IID_IClassFactory)
    {
        *ppv = (IClassFactory *)this;
        AddRef();
    }

    else if (   (_ClsID == CLSID_CdlProtocol)
        )
    {
        hr = CreateAPP(_ClsID, pUnkOuter, riid, (IUnknown **)ppv);
    }

#ifdef EAPP_TEST
    else if (   (_ClsID == CLSID_ResProtocol)
             || (_ClsID == CLSID_OhServNameSp)
             || (_ClsID == CLSID_MimeHandlerTest1)
            )

    {
        hr = CreateAPP(_ClsID, pUnkOuter, riid, (IUnknown **)ppv);
    }
    else if(   (_ClsID == CLSID_NotificaitonTest1)
            || (_ClsID == CLSID_NotificaitonTest2)
            || (_ClsID == CLSID_NotificaitonTest3)
            || (_ClsID == CLSID_NotificaitonTest4))
    {

     //   hr = CreateNotificationTest(
     //       0, _ClsID, pUnkOuter, riid, (IUnknown **)ppv);

        hr = E_FAIL;
    }
#endif
    else if (_ClsID == CLSID_ClassInstallFilter)
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
        if (pUnkOuter)
            hr = CLASS_E_NOAGGREGATION;
        else
        {
            CClassInstallFilter *pCIF = NULL;
            pCIF = new CClassInstallFilter();
            *ppv = (LPVOID)(IOInetProtocol *)pCIF;
            if (!*ppv)
                hr = E_OUTOFMEMORY;
            else
                hr = S_OK;
        }
    }
    else if( _ClsID == CLSID_DeCompMimeFilter)
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
        if( pUnkOuter)
            hr = CLASS_E_NOAGGREGATION;
        else
        {
            CMimeFt*    pMft = NULL;
            hr = CMimeFt::Create(&pMft);
            if( (hr == NOERROR) && pMft )
            {
                hr = pMft->QueryInterface(riid, ppv);
                pMft->Release();
            }
        }
    }
    else if( _ClsID == CLSID_StdEncodingFilterFac)
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
        if( pUnkOuter)
            hr = CLASS_E_NOAGGREGATION;
        else
        {
            *ppv = NULL;
            *ppv = new CEncodingFilterFactory;
            if( *ppv == NULL )
                hr = E_OUTOFMEMORY;
            else
                hr = ((IEncodingFilterFactory*)(*ppv))->AddRef();                
        }
    
    }


    EProtDebugOut((DEB_PLUGPROT, "%p OUT CUrlClsFact::CreateInstance (hr:%lx)\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::LockServer
//
//  Synopsis:   locks the server, preventing it from being unloaded
//
//  Arguments:  [fLock] -- TRUE to lock, FALSE to unlock
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlClsFact::LockServer(BOOL fLock)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CUrlClsFact::LockServer\n", this));
    HRESULT hr = NOERROR;
    if (fLock)
    {
        if (++_CLocks == 1)
        {
            DllAddRef();
        }
    }
    else
    {
        EProtAssert((_CLocks > 0));
        if (_CLocks > 0)
        {
            if (--_CLocks == 0)
            {
                DllRelease();
            }
        }
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CUrlClsFact::LockServer (hr:%lx)\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlClsFact::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    HRESULT hr = NOERROR;
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CUrlClsFact::QueryInterface\n", this));

    if (   riid == IID_IUnknown
        || riid == IID_IClassFactory)
    {
        *ppvObj = this;
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }
    if (hr == NOERROR)
    {
        AddRef();
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CUrlClsFact::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CUrlClsFact::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CUrlClsFact::AddRef(void)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CUrlClsFact::AddRef\n", this));

    LONG lRet = ++_CRefs;

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CUrlClsFact::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CUrlClsFact::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CUrlClsFact::Release(void)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CUrlClsFact::Release\n", this));

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        delete this;
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CUrlClsFact::Release (cRefs:%ld)\n", this,lRet));
    return lRet;
}

#if DBG==1
HRESULT DumpIID(REFIID riid)
{

    HRESULT hr;
    LPOLESTR pszStr = NULL;

#ifdef EAPP_TEST
    hr = StringFromCLSID(riid, &pszStr);
    EProtDebugOut((DEB_PLUGPROT, "API >>> DumpIID (riid:%ws) \n", pszStr));
#endif // EAPP_TEST

    if (pszStr)
    {
        delete pszStr;
    }
    return hr = NOERROR;
}
#endif




