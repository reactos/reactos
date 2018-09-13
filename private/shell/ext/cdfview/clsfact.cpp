//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// clsfact.cpp 
//
//   Class factory for ineticon.
//
//   History:
//
//       3/16/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"
#include "clsfact.h"
#include "cdfidl.h"
#include "persist.h"
#include "cdfview.h"
#include "iconhand.h"
#include "chanmgrp.h"
#include "chanmgri.h"
#include "chanmenu.h"
#include "proppgs.h"
#include "dll.h"        // DllAddRef, DllRelease.


//
// Constructor and destructor.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfClassFactory::CCdfClassFactory ***
//
//    Class factory constructor.
//
////////////////////////////////////////////////////////////////////////////////
CCdfClassFactory::CCdfClassFactory (
	CREATEPROC pfn
)
: m_cRef(1),
  m_Create(pfn)
{
    ASSERT(m_Create != NULL);
    //
    // As long as this class is around the dll should stay loaded.
    //

    TraceMsg(TF_OBJECTS, "+ IClassFactory");

    DllAddRef();

	return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfClassFactory::~CCdfClassFactory ***
//
//    Class factory destructor.
//
////////////////////////////////////////////////////////////////////////////////
CCdfClassFactory::~CCdfClassFactory (
	void
)
{
    //
    // Matching Release for the constructor Addref.
    //

    TraceMsg(TF_OBJECTS, "- IClassFactory");

    DllRelease();

	return;
}


//
// IUnknown methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfClassFactory::QueryInterface ***
//
//    Class factory QI.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfClassFactory::QueryInterface (
    REFIID riid,
    void **ppv
)
{
    ASSERT(ppv);

    HRESULT hr;

    if (IID_IUnknown == riid || IID_IClassFactory == riid)
    {
        AddRef();
        *ppv = (IClassFactory*)this;
        hr = S_OK;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    ASSERT((SUCCEEDED(hr) && *ppv) || (FAILED(hr) && NULL == *ppv));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfClassFactory::AddRef ***
//
//    Class factory AddRef.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCdfClassFactory::AddRef (
    void
)
{
    ASSERT(m_cRef != 0);
    ASSERT(m_cRef < (ULONG)-1);

    return ++m_cRef;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfClassFactory::Release ***
//
//    Class factory Release.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCdfClassFactory::Release (
    void
)
{
    ASSERT (m_cRef != 0);

    ULONG cRef = --m_cRef;
    
    if (0 == cRef)
        delete this;

    return cRef;
}


//
// IClassFactory methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfClassFactory::CreateInstance ***
//
//    Creates a cdf view object.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfClassFactory::CreateInstance (
    IUnknown* pOuterUnknown,
    REFIID riid,
    void **ppvObj
)
{
    ASSERT(ppvObj);

    HRESULT hr;

    *ppvObj = NULL;

    if (NULL == pOuterUnknown)
    {
        IUnknown* pIUnknown;
            
        hr = m_Create(&pIUnknown);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIUnknown)

            hr = pIUnknown->QueryInterface(riid, ppvObj);

            pIUnknown->Release();
        }
    }
    else
    {
        //
        // This object doesn't support aggregation.
        //

        hr = CLASS_E_NOAGGREGATION;
    }

    ASSERT((SUCCEEDED(hr) && *ppvObj) || (FAILED(hr) && NULL == *ppvObj));

    return hr;
}


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** LockServer ***
//
//    Increments/decrements the class factory ref count.     
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfClassFactory::LockServer (
    BOOL bLock
)
{
    if (bLock)
        AddRef();
    else
        Release();

    return S_OK;
}
