#include "stdafx.h"
#include "cfact.h"
#include "stobject.h"

/************************************************************************************
IUnknown Implementation

************************************************************************************/

HRESULT CSysTrayFactory::QueryInterface(REFIID iid, void** ppvObject)
{
    HRESULT hr = S_OK;

    if ((iid == IID_IClassFactory) || (iid == IID_IUnknown))
    {
        *ppvObject = (IClassFactory*) this;
    }
    else
    {
        *ppvObject = NULL;
        hr = E_NOINTERFACE;
    }

    if (hr == S_OK)
    {
        ((IUnknown*) (*ppvObject))->AddRef();
    }

    return hr;
}

ULONG CSysTrayFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CSysTrayFactory::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    
    return m_cRef;
}

/************************************************************************************
IClassFactory Implementation

************************************************************************************/
HRESULT CSysTrayFactory::CreateInstance(IUnknown* pUnkOuter, REFIID iid, void** ppvObject)
{
    HRESULT hr = S_OK;
    
    if (pUnkOuter != NULL)
    {
        hr = CLASS_E_NOAGGREGATION;
    }
    else
    {
        CSysTray* ptray = new CSysTray(m_fRunTrayOnConstruct);

        if (ptray != NULL)
        {
            hr = ptray->QueryInterface(iid, ppvObject);
            ptray->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

HRESULT CSysTrayFactory::LockServer(BOOL fLock)
{
    if (fLock) 
        InterlockedIncrement(&g_cLocks);
    else
        InterlockedDecrement(&g_cLocks);

    return S_OK;
}

/************************************************************************************
Constructor/Destructor Implementation

************************************************************************************/

CSysTrayFactory::CSysTrayFactory(BOOL fRunTrayOnConstruct)
{
    m_fRunTrayOnConstruct = fRunTrayOnConstruct;
    m_cRef = 1;
    InterlockedIncrement(&g_cLocks);
}

CSysTrayFactory::~CSysTrayFactory()
{
    InterlockedDecrement(&g_cLocks);
}