/*****************************************************************************\
    FILE: security.h
\*****************************************************************************/

#include "priv.h"
#include "util.h"
#include <imm.h>
#include <mshtml.h>

// /*
// Declared in \shell\lib\security.cpp and I should create a header for it.
STDAPI GetHTMLDoc2(IUnknown *punk, IHTMLDocument2 **ppHtmlDoc);

BOOL ProcessUrlAction(IUnknown * punkSite, LPCTSTR pszUrl, DWORD dwAction, DWORD dwFlags)
{
    BOOL fAllowed = FALSE;

    if (pszUrl) 
    {
        IInternetSecurityManager *pSecMgr;
        if (SUCCEEDED(CoCreateInstance(CLSID_InternetSecurityManager, 
                                       NULL, CLSCTX_INPROC_SERVER,
                                       IID_IInternetSecurityManager, 
                                       (void **)&pSecMgr))) 
        {
            WCHAR wzUrl[MAX_URL_STRING];
            DWORD dwZoneID = URLZONE_UNTRUSTED;
            DWORD dwPolicy = 0;
            DWORD dwContext = 0;

            IUnknown_SetSite(pSecMgr, punkSite);
            SHTCharToUnicode(pszUrl, wzUrl, ARRAYSIZE(wzUrl));
            if (S_OK == pSecMgr->ProcessUrlAction(wzUrl, dwAction, (BYTE *)&dwPolicy, sizeof(dwPolicy), (BYTE *)&dwContext, sizeof(dwContext), dwFlags, 0))
            {
                if (GetUrlPolicyPermissions(dwPolicy) == URLPOLICY_ALLOW)
                    fAllowed = TRUE;
            }
            IUnknown_SetSite(pSecMgr, NULL);
            pSecMgr->Release();
        }
    } 

    return fAllowed;
}


/*****************************************************************************\
    FUNCTION: SecurityZoneCheck
    
    PARAMETERS:
        punkSite: Site for QS, and enabling modal if UI needed.
        dwAction: verb to check. normally URLACTION_SHELL_VERB
        pidl: FTP URL that we need to verify
        pszUrl: FTP URL that we need to verify
        dwFlags: normally PUAF_DEFAULT | PUAF_WARN_IF_DENIED

    DESCRIPTION:
        Only pidl or pszUrl is passed.  This function will check if the verb
    (dwAction) is allowed in this zone.  Our first job is to find the zone which
    can be any of the following:
    1. Third party app that supports IInternetHostSecurityManager have a chance to disallow the action.
    2. Hosted in DefView w/WebView.  Zone of WebView can fail the action.
    3. Hosted in HTML FRAME.  Zone comes from trident can fail the action
    4. Hosted in DefView w/o WebView.  Zone comes from pidl or pszUrl and that can fail the action.
\*****************************************************************************/
BOOL ZoneCheckUrlAction(IUnknown * punkSite, DWORD dwAction, LPCTSTR pszUrl, DWORD dwFlags)
{
    BOOL IsSafe = TRUE; // Assume we will allow this.
    IInternetHostSecurityManager * pihsm;

    // What we want to do is allow this to happen only if the author of the HTML that hosts
    // the DefView is safe.  It's OK if they point to something unsafe, because they are
    // trusted.
    // 1. Third party app that supports IInternetHostSecurityManager have a chance to disallow the action.
    if (SUCCEEDED(IUnknown_QueryService(punkSite, IID_IInternetHostSecurityManager, IID_IInternetHostSecurityManager, (void**)&pihsm)))
    {
        if (S_OK != ZoneCheckHost(pihsm, dwAction, dwFlags))
        {
            // This zone is not OK or the user choose to not allow this to happen,
            // so cancel the operation.
            IsSafe = FALSE;    // Turn off functionality.
        }

        pihsm->Release();
    }

    // 1. Hosted in DefView w/WebView.  Zone of WebView can fail the action.
    if (IsSafe)
    {
        IOleCommandTarget * pct;

        if (SUCCEEDED(IUnknown_QueryService(punkSite, SID_DefView, IID_IOleCommandTarget, (void **)&pct)))
        {
            VARIANT vTemplatePath;
            vTemplatePath.vt = VT_EMPTY;
            if (pct->Exec(&CGID_DefView, DVCMDID_GETTEMPLATEDIRNAME, 0, NULL, &vTemplatePath) == S_OK)
            {
                if ((vTemplatePath.vt == VT_BSTR) && (S_OK != LocalZoneCheckPath(vTemplatePath.bstrVal)))
                    IsSafe = FALSE;

                // We were able to talk to the browser, so don't fall back on Trident because they may be
                // less secure.
                VariantClear(&vTemplatePath);
            }
            pct->Release();
        }
    }
    
    // 3. Hosted in HTML FRAME.  Zone comes from trident can fail the action
    if (IsSafe)
    {
        // Try to use the URL from the document to zone check 
        IHTMLDocument2 *pHtmlDoc;
        if (punkSite && SUCCEEDED(GetHTMLDoc2(punkSite, &pHtmlDoc)))
        {
            BSTR bstrPath;
            if (SUCCEEDED(pHtmlDoc->get_URL(&bstrPath)))
            {
                if (S_OK != ZoneCheckHost(pihsm, dwAction, dwFlags))
                {
                    // This zone is not OK or the user choose to not allow this to happen,
                    // so cancel the operation.
                    IsSafe = FALSE;    // Turn off functionality.
                }
                SysFreeString(bstrPath);
            }
            pHtmlDoc->Release();
        }
    }

    // 4. Hosted in DefView w/o WebView.  Zone comes from pidl or pszUrl and that can fail the action.
    if (IsSafe)
    {
        IsSafe = ProcessUrlAction(punkSite, pszUrl, dwAction, dwFlags);
    }

    return IsSafe;
}

//*/
BOOL ZoneCheckPidlAction(IUnknown * punkSite, DWORD dwAction, LPCITEMIDLIST pidl, DWORD dwFlags)
{
    TCHAR szUrl[MAX_URL_STRING];

    if (FAILED(UrlCreateFromPidl(pidl, SHGDN_FORPARSING, szUrl, ARRAYSIZE(szUrl), (ICU_ESCAPE | ICU_USERNAME), FALSE)))
        return FALSE;

    return ZoneCheckUrlAction(punkSite, dwAction, szUrl, dwFlags);
}