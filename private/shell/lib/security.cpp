#include "proj.h"
#include <mshtml.h>

STDAPI GetHTMLDoc2(IUnknown *punk, IHTMLDocument2 **ppHtmlDoc)
{
    *ppHtmlDoc = NULL;

    if (!punk)
        return E_FAIL;
        
    *ppHtmlDoc = NULL;
    //  The window.external, jscript "new ActiveXObject" and the <OBJECT> tag
    //  don't take us down the same road.

    IOleClientSite *pClientSite;
    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IOleClientSite, &pClientSite));
    if (SUCCEEDED(hr))
    {
        //  <OBJECT> tag path

        IOleContainer *pContainer;
        hr = pClientSite->GetContainer(&pContainer);
        if (SUCCEEDED(hr))
        {
            hr = pContainer->QueryInterface(IID_PPV_ARG(IHTMLDocument2, ppHtmlDoc));
            pContainer->Release();
        }
    
        if (FAILED(hr))
        {
            //  window.external path
            IWebBrowser2 *pWebBrowser2;
            hr = IUnknown_QueryService(pClientSite, SID_SWebBrowserApp, IID_PPV_ARG(IWebBrowser2, &pWebBrowser2));
            if (SUCCEEDED(hr))
            {
                IDispatch *pDispatch;
                hr = pWebBrowser2->get_Document(&pDispatch);
                if (SUCCEEDED(hr))
                {
                    hr = pDispatch->QueryInterface(IID_PPV_ARG(IHTMLDocument2, ppHtmlDoc));
                    pDispatch->Release();
                }
                pWebBrowser2->Release();
            }
        }
        pClientSite->Release();
    }
    else
    {
        //  jscript path
        hr = IUnknown_QueryService(punk, SID_SContainerDispatch, IID_PPV_ARG(IHTMLDocument2, ppHtmlDoc));
    }

    ASSERT(FAILED(hr) || (*ppHtmlDoc));

    return hr;
}

STDAPI LocalZoneCheckPath(LPCWSTR pszUrl)
{
    HRESULT hr = E_ACCESSDENIED;
    if (pszUrl) 
    {
        IInternetSecurityManager *pSecMgr;
        if (SUCCEEDED(CoCreateInstance(CLSID_InternetSecurityManager, 
                                       NULL, CLSCTX_INPROC_SERVER,
                                       IID_IInternetSecurityManager, 
                                       (void **)&pSecMgr))) 
        {
            DWORD dwZoneID = URLZONE_UNTRUSTED;
            if (SUCCEEDED(pSecMgr->MapUrlToZone(pszUrl, &dwZoneID, 0))) 
            {
                if (dwZoneID == URLZONE_LOCAL_MACHINE)
                    hr = S_OK;
            }       
            pSecMgr->Release();
        }
    } 
    else 
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

STDAPI LocalZoneCheck(IUnknown *punkSite)
{
    //  Return S_FALSE if we don't have a host site since we have no way of doing a 
    //  security check.  This is as far as VB 5.0 apps get.
    if (!punkSite)
        return S_FALSE;

    HRESULT hr = E_ACCESSDENIED;
    BOOL fTriedBrowser = FALSE;

    // Try to find the original template path for zone checking
    IOleCommandTarget * pct;
    if (SUCCEEDED(IUnknown_QueryService(punkSite, SID_DefView, IID_IOleCommandTarget, (void **)&pct)))
    {
        VARIANT vTemplatePath;
        vTemplatePath.vt = VT_EMPTY;
        if (pct->Exec(&CGID_DefView, DVCMDID_GETTEMPLATEDIRNAME, 0, NULL, &vTemplatePath) == S_OK)
        {
            fTriedBrowser = TRUE;
            if (vTemplatePath.vt == VT_BSTR)
                hr = LocalZoneCheckPath(vTemplatePath.bstrVal);

            // We were able to talk to the browser, so don't fall back on Trident because they may be
            // less secure.
            fTriedBrowser = TRUE;
            VariantClear(&vTemplatePath);
        }
        pct->Release();
    }

    // If this is one of those cases where the browser doesn't exist (AOL, VB, ...) then
    // we will check the scripts security.  If we did ask the browser, don't ask trident
    // because the browser is often more restrictive in some cases.
    if (!fTriedBrowser && (hr != S_OK))
    {
        // Try to use the URL from the document to zone check 
        IHTMLDocument2 *pHtmlDoc;
        if (SUCCEEDED(GetHTMLDoc2(punkSite, &pHtmlDoc)))
        {
            BSTR bstrURL;
            if (SUCCEEDED(pHtmlDoc->get_URL(&bstrURL)))
            {
                // NOTE: the above URL is improperly escaped, this is
                // due to app compat. if you depend on this URL being valid
                // use another means to get this

                hr = LocalZoneCheckPath(bstrURL);
                SysFreeString(bstrURL);
            }
            pHtmlDoc->Release();
        }
    }
                            
    return hr;
}


