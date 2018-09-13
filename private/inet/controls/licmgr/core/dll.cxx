//+----------------------------------------------------------------------------
//  File:       dll.cxx
//
//  Synopsis:   This file contains the core routines and globals for creating
//              DLLs
//
//-----------------------------------------------------------------------------


// Includes -------------------------------------------------------------------
#include <core.hxx>


// Globals --------------------------------------------------------------------
static THREADSTATE *    g_pts = NULL;
HANDLE                  g_hinst = NULL;
HANDLE                  g_heap = NULL;
DWORD                   g_tlsThreadState = NULL_TLS;
LONG                    g_cUsage = 0;
GINFO                   g_ginfo = { 0 };

DECLARE_LOCK(DLL);


// Prototypes -----------------------------------------------------------------
class CClassFactory : public CComponent,
                      public IClassFactory2
{
    typedef CComponent parent;

public:
    CClassFactory(CLASSFACTORY * pcf);

    // IUnknown methods
    DEFINE_IUNKNOWN_METHODS;

    // IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID riid, void ** ppvObj);
    STDMETHOD(LockServer)(BOOL fLock);

    // IClassFactory2 methods
	STDMETHOD(GetLicInfo)(LICINFO * pLicInfo);
	STDMETHOD(RequestLicKey)(DWORD dwReserved, BSTR * pbstrKey);
	STDMETHOD(CreateInstanceLic)(IUnknown * pUnkOuter,
	                             IUnknown * pUnkReserved,
	                             REFIID riid, BSTR bstrKey,
	                             void ** ppvObj);

private:
    CLASSFACTORY *  _pcf;

    HRESULT PrivateQueryInterface(REFIID riid, void ** ppvObj);
};

static HRESULT DllProcessAttach();
static void    DllProcessDetach();
static HRESULT DllThreadAttach();
static void    DllThreadDetach(THREADSTATE * pts);

static void    DllProcessPassivate();
static void    DllThreadPassivate();


//+----------------------------------------------------------------------------
//  Function:   DllMain
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
extern "C" BOOL WINAPI
DllMain(
    HINSTANCE   hinst,
    DWORD       nReason,
    void *      )           // pvReserved - Unused
{
    HRESULT hr = S_OK;

    g_hinst = hinst;

    switch (nReason)
    {
    case DLL_PROCESS_ATTACH:
        hr = DllProcessAttach();
        break;

    case DLL_PROCESS_DETACH:
        DllProcessDetach();
        break;

    case DLL_THREAD_DETACH:
        {
            THREADSTATE *   pts = (THREADSTATE *)TlsGetValue(g_tlsThreadState);
            DllThreadDetach(pts);
        }
        break;
    }

    return !hr;
}


//+----------------------------------------------------------------------------
//  Function:   DllGetClassObject
//
//  Synopsis:
//
//  NOTE: This code limits class objects to supporting IUnknown and IClassFactory
//
//-----------------------------------------------------------------------------
STDAPI
DllGetClassObject(
    REFCLSID    rclsid,
    REFIID      riid,
    void **     ppv)
{
    CLASSFACTORY *  pcf;
    HRESULT         hr;

    hr = EnsureThreadState();
    if (hr)
        return hr;

    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;

    if (riid != IID_IClassFactory &&
        riid != IID_IClassFactory2)
        return E_NOINTERFACE;

    for (pcf=g_acf; pcf->pclsid; pcf++)
    {
        if (*(pcf->pclsid) == rclsid)
            break;
    }
    if (!pcf)
        return CLASS_E_CLASSNOTAVAILABLE;

    if (riid == IID_IClassFactory2 && !pcf->pfnLicense)
        return E_NOINTERFACE;

    CClassFactory * pCF = new CClassFactory(pcf);
    if (!pCF)
        return E_OUTOFMEMORY;

    *ppv = (void *)(IClassFactory2 *)pCF;
    return S_OK;
}


//+----------------------------------------------------------------------------
//  Function:   DllCanUnloadNow
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDAPI
DllCanUnloadNow()
{
    return ((g_cUsage==0)
                ? S_OK
                : S_FALSE);
}


//+----------------------------------------------------------------------------
//  Function:   DllProcessAttach
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT
DllProcessAttach()
{
    PFN_PATTACH *   ppfnPAttach;
    HRESULT         hr = S_OK;

    g_tlsThreadState = TlsAlloc();
    if (g_tlsThreadState == NULL_TLS)
    {
        return GetWin32Hresult();
    }

    INIT_LOCK(DLL);

    g_heap = GetProcessHeap();

    for (ppfnPAttach=g_apfnPAttach; *ppfnPAttach; ppfnPAttach++)
    {
        hr = (**ppfnPAttach)();
        if (hr)
            goto Error;
    }

Cleanup:
    return hr;

Error:
    DllProcessDetach();
    goto Cleanup;
}


//+----------------------------------------------------------------------------
//  Function:   DllProcessDetach
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
void
DllProcessDetach()
{
    THREADSTATE *   pts;
    PFN_PDETACH *   ppfnPDetach;

    Implies(g_pts, g_tlsThreadState != NULL_TLS);

    while (g_pts)
    {
        pts = g_pts;
        Verify(TlsSetValue(g_tlsThreadState, pts));
        DllThreadDetach(pts);

        Assert(!TlsGetValue(g_tlsThreadState));
        Assert(g_pts != pts);
    }

    for (ppfnPDetach=g_apfnPDetach; *ppfnPDetach; ppfnPDetach++)
        (**ppfnPDetach)();

    DEINIT_LOCK(DLL);

    if (g_tlsThreadState != NULL_TLS)
    {
        TlsFree(g_tlsThreadState);
    }
}


//+----------------------------------------------------------------------------
//  Function:   DllThreadAttach
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT
DllThreadAttach()
{
    THREADSTATE *   pts;
    PFN_TATTACH *   ppfnTAttach;
    HRESULT         hr;

    LOCK(DLL);

    Assert(g_tlsThreadState != NULL_TLS);
    Assert(!::TlsGetValue(g_tlsThreadState));
    hr = AllocateThreadState(&pts);
    if (hr)
        goto Error;

    Assert(pts);
    pts->dll.idThread = GetCurrentThreadId();
    Verify(TlsSetValue(g_tlsThreadState, pts));

    Verify(SUCCEEDED(::CoGetMalloc(1, &pts->dll.pmalloc)));

    for (ppfnTAttach=g_apfnTAttach; *ppfnTAttach; ppfnTAttach++)
    {
        hr = (**ppfnTAttach)(pts);
        if (hr)
            goto Error;
    }

    pts->ptsNext = g_pts;
    g_pts = pts;

Cleanup:
    return hr;

Error:
    DllThreadDetach(pts);
    goto Cleanup;
}


//+----------------------------------------------------------------------------
//  Function:   DllThreadDetach
//
//  Synopsis:
//
//  NOTE: Under Win95, DllThreadDetach may be called to clear memory on a
//        thread which did not allocate the memory.
//
//-----------------------------------------------------------------------------
void
DllThreadDetach(
    THREADSTATE * pts)
{
    THREADSTATE **  ppts;
    PFN_TDETACH *   ppfnTDetach;

    LOCK(DLL);

    if (!pts)
        return;

    Assert(!pts->dll.cUsage);
    Assert(pts == (THREADSTATE *)TlsGetValue(g_tlsThreadState));

    for (ppfnTDetach=g_apfnTDetach; *ppfnTDetach; ppfnTDetach++)
        (**ppfnTDetach)(pts);

    ::SRelease(pts->dll.pmalloc);

    ::TlsSetValue(g_tlsThreadState, NULL);

    for (ppts=&g_pts; *ppts && *ppts != pts; ppts=&((*ppts)->ptsNext));
    if (*ppts)
    {
        *ppts = pts->ptsNext;
    }
    delete pts;
}


//+----------------------------------------------------------------------------
//  Function:   DllProcessPassivate
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
void
DllProcessPassivate()
{
    PFN_PPASSIVATE *    ppfnPPassivate;

    LOCK(DLL);

    Assert(!g_cUsage);

    // BUGBUG: What are the respective roles of process/thread passivation?
    // BUGBUG: This is an unsafe add into g_cUsage...fix this!
    g_cUsage += REF_GUARD;
    for (ppfnPPassivate=g_apfnPPassivate; *ppfnPPassivate; ppfnPPassivate++)
        (**ppfnPPassivate)();
    g_cUsage -= REF_GUARD;
}


//+----------------------------------------------------------------------------
//  Function:   DllThreadPassivate
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
void
DllThreadPassivate()
{
    THREADSTATE *       pts = GetThreadState();
    PFN_TPASSIVATE *    ppfnTPassivate;

    Assert(!pts->dll.cUsage);
    pts->dll.cUsage += REF_GUARD;
    for (ppfnTPassivate=g_apfnTPassivate; *ppfnTPassivate; ppfnTPassivate++)
        (**ppfnTPassivate)(pts);
    pts->dll.cUsage -= REF_GUARD;
}


//+----------------------------------------------------------------------------
//  Function:   CClassFactory
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
CClassFactory::CClassFactory(
    CLASSFACTORY *  pcf)
    : CComponent(NULL)
{
    Assert(pcf);
    Assert(pcf->pfnFactory);
    _pcf = pcf;
}


//+----------------------------------------------------------------------------
//  Function:   CreateInstance
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CClassFactory::CreateInstance(
    IUnknown *  pUnkOuter,
    REFIID      riid,
    void **     ppvObj)
{
    if (!ppvObj)
        return E_INVALIDARG;
    *ppvObj = NULL;

    // BUGBUG: What error should be returned?
    if (pUnkOuter && riid != IID_IUnknown)
        return E_INVALIDARG;

    // BUGBUG: Should the factory just create the object and let this
    //         code perform the appropriate QI?
    // BUGBUG: This code should automatically handle aggregation
    Assert(_pcf);
    Assert(_pcf->pfnFactory);
    return _pcf->pfnFactory(pUnkOuter, riid, ppvObj);
}


//+----------------------------------------------------------------------------
//  Function:   LockServer
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CClassFactory::LockServer(
    BOOL    fLock)
{
    if (fLock)
    {
        AddRef();
        IncrementThreadUsage();
    }
    else
    {
        DecrementThreadUsage();
        Release();
    }
    return S_OK;
}


//+----------------------------------------------------------------------------
//  Function:   GetLicInfo
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CClassFactory::GetLicInfo(
    LICINFO *   pLicInfo)

{
    Assert(_pcf->pfnLicense);
    return _pcf->pfnLicense(LICREQUEST_INFO, pLicInfo);
}


//+----------------------------------------------------------------------------
//  Function:   RequestLicKey
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CClassFactory::RequestLicKey(
    DWORD   ,   // dwReserved
    BSTR *  pbstrKey)
{
    Assert(_pcf->pfnLicense);
    return _pcf->pfnLicense(LICREQUEST_OBTAIN, pbstrKey);
}


//+----------------------------------------------------------------------------
//  Function:   CreateInstanceLic
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CClassFactory::CreateInstanceLic(
    IUnknown *  pUnkOuter,
    IUnknown *  ,           // pUnkReserved
    REFIID      riid,
    BSTR        bstrKey,
    void **     ppvObj)
{
    Assert(_pcf->pfnLicense);

    if (!ppvObj)
        return E_INVALIDARG;
    *ppvObj = NULL;

    if (_pcf->pfnLicense(LICREQUEST_VALIDATE, bstrKey) != S_OK)
    {
        return CLASS_E_NOTLICENSED;
    }

    return CreateInstance(pUnkOuter, riid, ppvObj);
}


//+----------------------------------------------------------------------------
//  Function:   PrivateQueryInterface
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT
CClassFactory::PrivateQueryInterface(
    REFIID  riid,
    void ** ppvObj)
{
    if (riid == IID_IClassFactory)
    {
        *ppvObj = (void *)(IClassFactory *)this;
    }
    else if (riid == IID_IClassFactory2)
    {
        if (_pcf->pfnLicense)
        {
            *ppvObj = (void *)(IClassFactory2 *)this;
        }
        else
        {
            return E_NOINTERFACE;
        }
    }
    else
    {
        return parent::PrivateQueryInterface(riid, ppvObj);
    }
    return S_OK;
}


//+----------------------------------------------------------------------------
//  Function:   GetWin32Hresult
//
//  Synopsis:   Return an HRESULT derived from the current Win32 error
//
//-----------------------------------------------------------------------------
HRESULT
GetWin32Hresult()
{
    return HRESULT_FROM_WIN32(GetLastError());
}


//+----------------------------------------------------------------------------
//  Function:   EnsureThreadState
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT
EnsureThreadState()
{
    extern DWORD g_tlsThreadState;
    Assert(g_tlsThreadState != NULL_TLS);
    if (!TlsGetValue(g_tlsThreadState))
        return DllThreadAttach();
    return S_OK;
}


//+----------------------------------------------------------------------------
//  Function:   IncrementProcessUsage
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
void
IncrementProcessUsage()
{
#ifdef _DEBUG
    Verify(InterlockedIncrement(&g_cUsage) > 0);
#else
    InterlockedIncrement(&g_cUsage);
#endif
}


//+----------------------------------------------------------------------------
//  Function:   DecrementProcessUsage
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
void
DecrementProcessUsage()
{
    if (!InterlockedDecrement(&g_cUsage))
    {
        DllProcessPassivate();
    }
}



//+----------------------------------------------------------------------------
//  Function:   IncrementThreadUsage
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
void
IncrementThreadUsage()
{
#ifdef _DEBUG
    Verify(++TLS(dll.cUsage) > 0);
#else
    ++TLS(dll.cUsage);
#endif
    IncrementProcessUsage();
}


//+----------------------------------------------------------------------------
//  Function:   DecrementThreadUsage
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
void
DecrementThreadUsage()
{
    THREADSTATE *   pts = GetThreadState();

    pts->dll.cUsage--;
    Assert(pts->dll.cUsage >= 0);
    if (!pts->dll.cUsage)
    {
        DllThreadPassivate();
    }
    DecrementProcessUsage();
}
