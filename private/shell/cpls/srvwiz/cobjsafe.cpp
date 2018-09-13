#include "priv.h"
#include "comcat.h"
#include <hliface.h>
#include <imm.h>
#include <mshtml.h>
#include "cobjsafe.h"

// a default isafetyobject that we generally would use...  marks 
// deals with IDispatch 


HRESULT CObjectSafety::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    if (IsEqualIID(riid, IID_IDispatch))
    {
        *pdwEnabledOptions = _dwSafetyOptions;
    }
    else
    {
        ::DefaultGetSafetyOptions(riid, pdwSupportedOptions, pdwEnabledOptions);
    }

    return S_OK;
}


HRESULT CObjectSafety::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    if (dwOptionSetMask & ~(INTERFACESAFE_FOR_UNTRUSTED_CALLER |
                            INTERFACESAFE_FOR_UNTRUSTED_DATA))
    {
        return E_INVALIDARG;
    }

    if (IsEqualIID(riid, IID_IDispatch))
    {
        _dwSafetyOptions = (_dwSafetyOptions & ~dwOptionSetMask) |
                           (dwEnabledOptions & dwOptionSetMask);
        return S_OK;
    }
    else
    {
        return ::DefaultSetSafetyOptions(riid, dwOptionSetMask, dwEnabledOptions);
    }
}



// *** IObjectSafety
//
// A couple static functions called by sitemap (and webbrowser).
// These are static so anyone else in this dll who has an OC
// that's always safe can just call them.
//
// These functions say we are safe for these three interfaces we implement
//  IID_IDispatch
//  IID_IPersistStream
//  IID_IPersistPropertyBag
//
// The WebBrowser OC handles IDispatch differently.
//
HRESULT DefaultGetSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    *pdwSupportedOptions = 0;
    *pdwEnabledOptions = 0;

    if (IsEqualIID(riid, IID_IDispatch) ||
        IsEqualIID(riid, IID_IPersistStream) ||
        IsEqualIID(riid, IID_IPersistStreamInit) ||
//        IsEqualIID(riid, IID_IPersistHistory) ||
        IsEqualIID(riid, IID_IPersistPropertyBag))
    {
        *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
        *pdwEnabledOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
    }

    return S_OK;
}

HRESULT DefaultSetSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    if (dwOptionSetMask & ~(INTERFACESAFE_FOR_UNTRUSTED_CALLER |
                            INTERFACESAFE_FOR_UNTRUSTED_DATA))
    {
        return E_INVALIDARG;
    }

    if (IsEqualIID(riid, IID_IDispatch) ||
        IsEqualIID(riid, IID_IPersistStream) ||
        IsEqualIID(riid, IID_IPersistStreamInit) ||
//        IsEqualIID(riid, IID_IPersistHistory) ||
        IsEqualIID(riid, IID_IPersistPropertyBag))
    {
        return S_OK;
    }

    return E_FAIL;
}


// When CWebBrowserOC is in the safe for scripting mode, we can't give out
// anyone else's IDispatch that is not also safe for scripting.
// This function encapsulates the basic functionality needed by both
// MakeSafeScripting and MakeSafeForInitializing (which we don't use)
BOOL MakeSafeFor(
IUnknown *punk,                 // object to test for safety
REFCATID catid,                 // category of safety
REFIID riid,                    // interface on which safety is desired
DWORD dwXSetMask,               // options to set
DWORD dwXOptions                // options to make safe for
                                    // (either INTERFACESAFE_FOR_UNTRUSTED_CALLER or
                                    //  INTERFACESAFE_FOR_UNTRUSTED_DATA)
)
{
    HRESULT hres;

    // first try IObjectSafety
    IObjectSafety *posafe;
    if (SUCCEEDED(punk->QueryInterface(IID_IObjectSafety, (LPVOID*) &posafe)))
    {
        hres = posafe->SetInterfaceSafetyOptions(riid, dwXSetMask, dwXOptions);
        posafe->Release();

        if (SUCCEEDED(hres))
            return TRUE;
    }

    // check the registry for "safe for scripting" component category

    // we need the classid -- get it thru IPersist
    CLSID clsid;
    IPersist *ppersist;
    hres = punk->QueryInterface(IID_IPersist, (LPVOID*) &ppersist);
    if (SUCCEEDED(hres))
    {
        hres = ppersist->GetClassID(&clsid);
        ppersist->Release();
    }
    if (FAILED(hres))
    {
        return FALSE;
    }

    // Create the category manager
    ICatInformation *pcatinfo;
    hres = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
                            NULL, CLSCTX_INPROC_SERVER,
                            IID_ICatInformation, (LPVOID*) &pcatinfo);
    if (FAILED(hres))
        return FALSE;

    // Ask if the object belongs to the specified category
    CATID rgcatid[1];
    rgcatid[0] = catid;

    hres = pcatinfo->IsClassOfCategories(clsid, 1, rgcatid, 0, NULL);
    pcatinfo->Release();

    return (hres==S_OK) ? TRUE : FALSE;
}

HRESULT MakeSafeForScripting(IUnknown** ppDisp)
{
    HRESULT hres = S_OK;

    if (!MakeSafeFor(*ppDisp, CATID_SafeForScripting, IID_IDispatch,
                       INTERFACESAFE_FOR_UNTRUSTED_CALLER,
                       INTERFACESAFE_FOR_UNTRUSTED_CALLER))
    {
        (*ppDisp)->Release();
        *ppDisp = NULL;
        hres = E_FAIL;
    }

    return hres;
}

