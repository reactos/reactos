/*
 * @(#)IObjectSafety.cxx 1.0 11/18/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "xml/om/iobjectsafety.hxx"

HRESULT STDMETHODCALLTYPE IObjectSafetyWrapper::GetInterfaceSafetyOptions(
                                        REFIID riid, DWORD *pdwSupportedOptions,
                                        DWORD *pdwEnabledOptions)
{
    if (!pdwSupportedOptions || !pdwEnabledOptions)
        return E_POINTER;   // BUGBUG - what about E_INVALIDARG?

    STACK_ENTRY_WRAPPED;
    MutexLock lock(_pMutex);
    HRESULT hr = S_OK;

    TRY
    {
        getWrapped()->getInterfaceSafetyOptions(riid, pdwSupportedOptions, pdwEnabledOptions);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

HRESULT STDMETHODCALLTYPE IObjectSafetyWrapper::SetInterfaceSafetyOptions(
                                        REFIID riid, DWORD dwOptionSetMask,
                                        DWORD dwEnabledOptions)
{
    STACK_ENTRY_WRAPPED;

    // BUGBUG - 50332 - This lock causes a deadlock when the site is set from the 
    // mimeviewer. We need to have some sort of lock here.  Fix for RTM.
    //  MutexLock lock(_pMutex);

    HRESULT hr = S_OK;

    TRY
    {
        getWrapped()->setInterfaceSafetyOptions(riid, dwOptionSetMask, dwEnabledOptions);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}

