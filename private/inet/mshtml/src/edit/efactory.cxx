//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  Class:      CMshtmlEdFactory
//
//  Contents:   Implementation of Class Factory for Mshtmled
//
//  History:    7-Jan-98   raminh  Created
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_EFACTORY_HXX_
#define X_EFACTORY_HXX_
#include "efactory.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

extern long       g_cServerLocks;    // Count of locks

MtDefine(CMshtmlEdFactory, Utilities, "CMshtmlEdFactory")

//+----------------------------------------------------------------------------
//
// Class Factory Implementation
//
//+----------------------------------------------------------------------------
STDMETHODIMP
CMshtmlEdFactory::QueryInterface(const IID& iid, void** ppv)
{    
	if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
	{
		*ppv = static_cast<IClassFactory*>(this) ; 
	}
	else
	{
		*ppv = NULL ;
		return E_NOINTERFACE ;
	}
	reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
	return S_OK ;
}

STDMETHODIMP_(ULONG)
CMshtmlEdFactory::AddRef()
{
	return InterlockedIncrement(&_ulRefs) ;
}

STDMETHODIMP_(ULONG)
CMshtmlEdFactory::Release() 
{
	if (InterlockedDecrement(&_ulRefs) == 0)
	{
		delete this ;
		return 0 ;
	}
	return _ulRefs ;
}

STDMETHODIMP
CMshtmlEdFactory::CreateInstance(IUnknown* pUnknownOuter,
                                           const IID& iid,
                                           void** ppv) 
{
	// Cannot aggregate.
	if (pUnknownOuter != NULL)
	{
		return CLASS_E_NOAGGREGATION ;
	}

	// Create component.
	CMshtmlEd* pMshtmled = new CMshtmlEd ;
	if (pMshtmled == NULL)
	{
		return E_OUTOFMEMORY ;
	}

	// Get the requested interface.
	HRESULT hr = pMshtmled->QueryInterface(iid, ppv) ;

	// Release the IUnknown pointer.
	pMshtmled->Release() ;
	return hr ;
}

STDMETHODIMP
CMshtmlEdFactory::LockServer(BOOL bLock) 
{
	if (bLock)
	{
		InterlockedIncrement(&g_cServerLocks) ; 
	}
	else
	{
		InterlockedDecrement(&g_cServerLocks) ;
	}
	return S_OK ;
}
