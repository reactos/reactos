#include "..\\inc\\urlint.h"
#include "..\\inc\\wcheckcb.h"
#include "..\\inc\\debug.h"

// createion method
STDMETHODIMP CreateCallbackClassFactory(IClassFactory** ppCF)
{
    *ppCF = (IClassFactory*)new CCallbackObjFactory;
    
    if (*ppCF == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

// constructor
CCallbackObjFactory::CCallbackObjFactory()
{
    DllAddRef();
    m_cRef = 1;
    m_cLocks = 0;
}

// destructor
CCallbackObjFactory::~CCallbackObjFactory()
{
    Assert(m_cRef == 0 && m_cLocks == 0);
    DllRelease();
}


/******************************************************************************
    IUnknown methods
******************************************************************************/

STDMETHODIMP CCallbackObjFactory::QueryInterface(REFIID iid, void** ppvObject)
{
    *ppvObject = NULL;

    if (iid == IID_IUnknown || iid == IID_IClassFactory)
    {
        *ppvObject = (void*)this;
        ((LPUNKNOWN)*ppvObject)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCallbackObjFactory::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CCallbackObjFactory::Release(void)
{
    if (--m_cRef)
        return m_cRef;

    delete this;
    return 0;
}


/******************************************************************************
    IClassFactory methods
******************************************************************************/

STDMETHODIMP CCallbackObjFactory::CreateInstance(
                                          LPUNKNOWN pUnkOuter, 
                                          REFIID riid, 
                                          LPVOID* ppv)
{
    *ppv = NULL;
    HRESULT hr = S_OK;

    if (pUnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    CControlRefreshCallback *pCRC = new CControlRefreshCallback;
    if (pCRC == NULL)
        return E_OUTOFMEMORY;

    hr = pCRC->QueryInterface(riid, ppv);
    pCRC->Release();

    return hr;
}

STDMETHODIMP CCallbackObjFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();

    return S_OK;
}

