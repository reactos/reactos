#include "stdafx.h"
#pragma hdrstop

// This isn't a typical delay load since it's called only if wininet
// is already loaded in memory. Otherwise the call is dropped on the floor.
// Defview did it this way I assume to keep WININET out of first boot time.
BOOL MyInternetSetOption(HANDLE h, DWORD dw1, LPVOID lpv, DWORD dw2)
{
    BOOL bRet = FALSE;
    HMODULE hmod = GetModuleHandle(TEXT("wininet.dll"));
    if (hmod)
    {
        typedef BOOL (*PFNINTERNETSETOPTIONA)(HANDLE h, DWORD dw1, LPVOID lpv, DWORD dw2);
        FARPROC fp = GetProcAddress(hmod, "InternetSetOptionA");
        if (fp)
        {
            bRet = ((PFNINTERNETSETOPTIONA)fp)(h, dw1, lpv, dw2);
        }
    }
    return bRet;
}

// REVIEW: maybe just check (hwnd == GetShellWindow())

STDAPI_(BOOL) IsDesktopWindow(HWND hwnd)
{
    TCHAR szName[80];

    GetClassName(hwnd, szName, ARRAYSIZE(szName));
    if (!lstrcmp(szName, TEXT(STR_DESKTOPCLASS)))
    {
        GetWindowText(hwnd, szName, ARRAYSIZE(szName));
        return !lstrcmp(szName, TEXT("Program Manager"));
    }
    return FALSE;
}


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


STDAPI LocalZoneCheckPath(LPCWSTR bstrPath)
{
    HRESULT hr = E_ACCESSDENIED;
    if (bstrPath) 
    {
        IInternetSecurityManager *pSecMgr;
        if (SUCCEEDED(CoCreateInstance(CLSID_InternetSecurityManager, 
                                       NULL, CLSCTX_INPROC_SERVER,
                                       IID_IInternetSecurityManager, 
                                       (void **)&pSecMgr))) 
        {
            DWORD dwZoneID = URLZONE_UNTRUSTED;
            if (SUCCEEDED(pSecMgr->MapUrlToZone(bstrPath, &dwZoneID, 0))) 
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
    HRESULT hr = E_ACCESSDENIED;
    IOleCommandTarget * pct;
    BOOL fTriedBrowser = FALSE;

    //  Return S_FALSE if we don't have a host site since we have no way of doing a 
    //  security check.  This is as far as VB 5.0 apps get.
    if (!punkSite)
        return S_FALSE;

    // Try to find the original template path for zone checking
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
            BSTR bstrPath;
            if (SUCCEEDED(pHtmlDoc->get_URL(&bstrPath)))
            {
                hr = LocalZoneCheckPath(bstrPath);
                SysFreeString(bstrPath);
            }
            pHtmlDoc->Release();
        }
    }
                            
    return hr;
}
