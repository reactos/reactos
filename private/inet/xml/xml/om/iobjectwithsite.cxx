/*
 * @(#)IObjectWithSite.cxx 1.0 11/18/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "xml/om/iobjectwithsite.hxx"


HRESULT STDMETHODCALLTYPE IObjectWithSiteWrapper::SetSite( 
            /* [in] */ IUnknown __RPC_FAR *pUnkSite)
{        
    STACK_ENTRY_WRAPPED;
    // BUGBUG - 50332 - This lock causes a deadlock when the site is set from the 
    // mimeviewer. We need to have some sort of lock here.  Fix for RTM.
    // MutexLock lock(_pMutex);
    HRESULT hr = S_OK;

    TRY
    {
        getWrapped()->setSite(pUnkSite);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT STDMETHODCALLTYPE IObjectWithSiteWrapper::GetSite( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvSite)
{        
    if (!ppvSite)
        return E_INVALIDARG;

    STACK_ENTRY_WRAPPED;
    MutexLock lock(_pMutex);
    HRESULT hr = S_OK;

    TRY
    {
        getWrapped()->getSite(riid, ppvSite);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


