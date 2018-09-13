// Functions stolen from shdocvw\util.cpp

#include "stdafx.h"
#pragma hdrstop
//#include "resource.h"
//#include "..\ids.h"
//#include "deskstat.h"
//#include "dutil.h"
//#include <webcheck.h>
//#include "sdspatch.h"
//#include "dsubscri.h"

#include <mluisupp.h>

// Helper function to convert Ansi string to allocated BSTR
#ifndef UNICODE
BSTR AllocBStrFromString(LPTSTR psz)
{
    OLECHAR wsz[INFOTIPSIZE];  // assumes INFOTIPSIZE number of chars max

    SHAnsiToUnicode(psz, wsz, ARRAYSIZE(wsz));
    return SysAllocString(wsz);

}
#endif


#ifdef POSTSPLIT

//+-----------------------------------------------------------------
//
// Helper function for getting the TopLeft point of an element from
//      mshtml.dll, and have the point reported in inside relative
//      coordinates (inside margins, borders and padding.)
//-----------------------------------------------------------------
HRESULT CSSOM_TopLeft(IHTMLElement * pIElem, POINT * ppt) 
{
    HRESULT       hr = E_FAIL;
    IHTMLStyle    *pistyle;

    if (SUCCEEDED(pIElem->get_style(&pistyle)) && pistyle) {
        VARIANT var = {0};

        if (SUCCEEDED(pistyle->get_top(&var)) && var.bstrVal) {
            ppt->y = StrToIntW(var.bstrVal);
            VariantClear(&var);

            if (SUCCEEDED(pistyle->get_left(&var)) && var.bstrVal) {
                ppt->x = StrToIntW(var.bstrVal);
                VariantClear(&var);
                hr = S_OK;
            }
        }

        pistyle->Release();
    }

    return hr;
}

HRESULT GetHTMLElementStrMember(IHTMLElement *pielem, LPTSTR pszName, DWORD cchSize, BSTR bstrMember)
{
    HRESULT hr;
    VARIANT var = {0};

    if (!pielem)
        hr = E_INVALIDARG;
    else if (SUCCEEDED(hr = pielem->getAttribute(bstrMember, TRUE, &var)))
    {
        if ((VT_BSTR == var.vt) && (var.bstrVal))
        {
#ifdef UNICODE          
            StrCpyNW(pszName, (LPCWSTR)var.bstrVal, cchSize);
#else // UNICODE
            SHUnicodeToAnsi((BSTR)var.bstrVal, pszName, cchSize);
#endif // UNICODE
        }
        else
            hr = E_FAIL; // Try VariantChangeType?????

        VariantClear(&var);
    }

    return hr;
}

/******************************************************************\
    FUNCTION: IElemCheckForExistingSubscription()

    RETURN VALUE:
    S_OK    - if the IHTMLElement points to a TAG that has a "subscribed_url" property
              that is subscribed. 
    S_FALSE - if the IHTMLElement points to a TAG that has a
              "subscribed_url" property but the URL is not subscribed.
    E_FAIL  - if the IHTMLElement points to a TAG that does not
              have a  "subscribed_url" property.
\******************************************************************/
HRESULT IElemCheckForExistingSubscription(IHTMLElement *pielem)
{
    HRESULT hr = E_FAIL;
    TCHAR szHTMLElementName[MAX_URL_STRING];

    if (!pielem)
        return E_INVALIDARG;

    if (SUCCEEDED(GetHTMLElementStrMember(pielem, szHTMLElementName, SIZECHARS(szHTMLElementName), (BSTR)(s_sstrSubSRCMember.wsz))))
        hr = (CheckForExistingSubscription(szHTMLElementName) ? S_OK : S_FALSE);

    return hr;
}

HRESULT IElemCloseDesktopComp(IHTMLElement *pielem)
{
    HRESULT hr;
    TCHAR szHTMLElementID[MAX_URL_STRING];

    ASSERT(pielem);
    if (pielem &&
        SUCCEEDED(hr = GetHTMLElementStrMember(pielem, szHTMLElementID, SIZECHARS(szHTMLElementID), (BSTR)(s_sstrIDMember.wsz))))
    {
        hr = UpdateComponentFlags(szHTMLElementID, COMP_CHECKED | COMP_UNCHECKED, COMP_UNCHECKED) ? S_OK : E_FAIL;
        ASSERT(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
            REFRESHACTIVEDESKTOP();
    }

    return hr;
}

HRESULT IElemGetSubscriptionsDialog(IHTMLElement *pielem, HWND hwnd)
{
    HRESULT hr;
    TCHAR szHTMLElementName[MAX_URL_STRING];

    ASSERT(pielem);
    if (SUCCEEDED(hr = GetHTMLElementStrMember(pielem, szHTMLElementName, SIZECHARS(szHTMLElementName), (BSTR)(s_sstrSubSRCMember.wsz))))
    {
        ASSERT(CheckForExistingSubscription(szHTMLElementName)); // We should not have gotten this far.
        hr = ShowSubscriptionProperties(szHTMLElementName, hwnd);
    }

    return hr;
}

HRESULT IElemSubscribeDialog(IHTMLElement *pielem, HWND hwnd)
{
    HRESULT hr;
    TCHAR szHTMLElementName[MAX_URL_STRING];

    ASSERT(pielem);
    hr = GetHTMLElementStrMember(pielem, szHTMLElementName, SIZECHARS(szHTMLElementName), (BSTR)(s_sstrSubSRCMember.wsz));
    if (SUCCEEDED(hr))
    {
        ASSERT(!CheckForExistingSubscription(szHTMLElementName)); // We should not have gotten this far.
        hr = CreateSubscriptionsWizard(SUBSTYPE_DESKTOPURL, szHTMLElementName, NULL, hwnd);
    }

    return hr;
}

HRESULT IElemUnsubscribe(IHTMLElement *pielem)
{
    HRESULT hr;
    TCHAR szHTMLElementName[MAX_URL_STRING];

    ASSERT(pielem);
    hr = GetHTMLElementStrMember(pielem, szHTMLElementName, SIZECHARS(szHTMLElementName), (BSTR)(s_sstrSubSRCMember.wsz));
    if (SUCCEEDED(hr))
    {
        ASSERT(CheckForExistingSubscription(szHTMLElementName)); // We should not have gotten this far.
        hr = DeleteFromSubscriptionList(szHTMLElementName) ? S_OK : S_FALSE;
    }

    return hr;
}

HRESULT IElemUpdate(IHTMLElement *pielem)
{
    HRESULT hr;
    TCHAR szHTMLElementName[MAX_URL_STRING];

    ASSERT(pielem);
    hr = GetHTMLElementStrMember(pielem, szHTMLElementName, SIZECHARS(szHTMLElementName), (BSTR)(s_sstrSubSRCMember.wsz));
    if (SUCCEEDED(hr))
    {
        ASSERT(CheckForExistingSubscription(szHTMLElementName)); // We should not have gotten this far.
        hr = UpdateSubscription(szHTMLElementName) ? S_OK : S_FALSE;
    }

    return hr;
}

HRESULT ShowSubscriptionProperties(LPCTSTR pszUrl, HWND hwnd)
{
    HRESULT hr;
    ISubscriptionMgr *psm;

    if (SUCCEEDED(hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                          CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr,
                          (void**)&psm)))
    {
        WCHAR wzURL[MAX_URL_STRING];
        LPCWSTR pwzURL = wzURL;

#ifndef UNICODE
        SHAnsiToUnicode(pszUrl, wzURL, ARRAYSIZE(wzURL));
#else // UNICODE
        pwzURL = pszUrl;
#endif // UNICODE

        hr = psm->ShowSubscriptionProperties(pwzURL, hwnd);
        psm->Release();
    }

    return hr;
}

HRESULT CreateSubscriptionsWizard(SUBSCRIPTIONTYPE subType, LPCTSTR pszUrl, SUBSCRIPTIONINFO *pInfo, HWND hwnd)
{
    HRESULT hr;
    ISubscriptionMgr *psm;

    if (SUCCEEDED(hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                          CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr,
                          (void**)&psm)))
    {
        WCHAR wzURL[MAX_URL_STRING];
        LPCWSTR pwzURL = wzURL;

#ifndef UNICODE
        SHAnsiToUnicode(pszUrl, wzURL, ARRAYSIZE(wzURL));
#else // UNICODE
        pwzURL = pszUrl;
#endif // UNICODE

        hr = psm->CreateSubscription(hwnd, pwzURL, pwzURL, CREATESUBS_ADDTOFAVORITES, subType, pInfo);
        psm->UpdateSubscription(pwzURL);
        psm->Release();
    }

    return hr;
}

HRESULT ShowComponentSettings(void)
{
    TCHAR szCompSettings[MAX_PATH];
    MLLoadString(IDS_DESKTOPWEBSETTINGS, szCompSettings, ARRAYSIZE(szCompSettings));
    SHRunControlPanel(szCompSettings, NULL);

    return S_OK;
}


// return an aliased pointer to the pidl in this variant (that is why it is const)
// see VariantToIDList

LPCITEMIDLIST VariantToConstIDList(const VARIANT *pv)
{
    if (pv == NULL)
        return NULL;

    LPCITEMIDLIST pidl = NULL;
    VARIANT v;

    if (pv->vt == (VT_BYREF | VT_VARIANT) && pv->pvarVal)
        v = *pv->pvarVal;
    else
        v = *pv;

    switch (v.vt)
    {
    case VT_UI4:
        // HACK in process case, avoid the use of this if possible
        // BUGBUG raymondc Sundown this is still wrong
        pidl = (LPCITEMIDLIST)(UINT_PTR)v.ullVal;
        ASSERT(pidl);
        break;

    case VT_ARRAY | VT_UI1:
        pidl = (LPCITEMIDLIST)v.parray->pvData;   // alias: PIDL encoded
        break;

    case VT_DISPATCH | VT_BYREF:
        if (v.ppdispVal == NULL)
            break;

        v.pdispVal = *v.ppdispVal;

        // fall through...

    case VT_DISPATCH:
        CSDFldrItem *pcfi;
        if (v.pdispVal && SUCCEEDED(v.pdispVal->QueryInterface(IID_ICSDFolderItem, (void **)&pcfi)))
        {
            pidl = pcfi->Pidl();    // return alias!
            pcfi->Release();
        }
        break;
    }
    return pidl;
}

// ALLOCATES pidl from a variant, client should free this
// see VariantToConstIDList

LPITEMIDLIST VariantToIDList(const VARIANT *pv)
{
    LPITEMIDLIST pidl = NULL;
    VARIANT v;
    
    if (!pv)
        return NULL;

    if (pv->vt == (VT_BYREF | VT_VARIANT) && pv->pvarVal)
        v = *pv->pvarVal;
    else
        v = *pv;

    switch (v.vt)
    {
    case VT_I2:
        v.lVal = (long)v.iVal;
        // Fall through

    case VT_I4:
        pidl = SHCloneSpecialIDList(HWND_DESKTOP, v.lVal, TRUE);
        break;

    case VT_BSTR:
        pidl = ILCreateFromPathW(v.bstrVal);
        break;

    default:
        // use above code
        LPCITEMIDLIST pidlToCopy = VariantToConstIDList(pv);
        if (pidlToCopy)
            pidl = ILClone(pidlToCopy);
        break;
    }
    return pidl;
}


BOOL GetInfoTip(IShellFolder* psf, LPCITEMIDLIST pidl, LPTSTR pszText, int cchTextMax)
{
    BOOL fRet = FALSE;

    *pszText = 0;   // empty for failure

    if (pidl)
    {
        IQueryInfo *pqi;
        if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, &pidl, IID_IQueryInfo, NULL, (void**)&pqi)))
        {
            WCHAR *pwszTip;
            pqi->GetInfoTip(0, &pwszTip);
            if (pwszTip)
            {
                fRet = TRUE;
                SHUnicodeToTChar(pwszTip, pszText, cchTextMax);
                SHFree(pwszTip);
            }
            pqi->Release();
        }
    }
    return fRet;
}

BSTR StrRetToBStr(LPCITEMIDLIST pidl, STRRET *pstr)
{
    OLECHAR wszPath[MAX_PATH];
    switch (pstr->uType)
    {
    case STRRET_WSTR:
        StrRetToUnicode(wszPath, ARRAYSIZE(wszPath), pidl, pstr);
        break;
    case STRRET_OFFSET:
        SHAnsiToUnicode((LPSTR)pidl + pstr->uOffset, wszPath, ARRAYSIZE(wszPath));
        break;
    case STRRET_CSTR:
        SHAnsiToUnicode(pstr->cStr, wszPath, ARRAYSIZE(wszPath));
        break;
    default:
        wszPath[0] = '\0';
    }

    return SysAllocString(wszPath);
}

DWORD StrRetToUnicode(LPWSTR wzPath, DWORD cbPathLen, LPCITEMIDLIST pidl, STRRET *pstr)
{
    DWORD cchResult = 0;

    if (cbPathLen)
    {
        switch (pstr->uType)
        {
        case STRRET_WSTR:
            {
                OLECHAR * pwsz = pstr->pOleStr;

                StrCpyNW(wzPath, pwsz, cbPathLen);
                cchResult = lstrlenW(wzPath);
                SHFree(pstr->pOleStr);
            }
            break;
        case STRRET_OFFSET:
            cchResult = SHAnsiToUnicode((LPSTR)pidl + pstr->uOffset, wzPath, cbPathLen);
            break;
        case STRRET_CSTR:
            cchResult = SHAnsiToUnicode(pstr->cStr, wzPath, cbPathLen);
            break;
        default:
            wzPath[0] = L'\0';
            cchResult = 1;
        }
    }
    return cchResult;
}


#ifndef UNICODE
BSTR SysAllocStringA(LPCSTR pszAnsiStr)
{
    OLECHAR FAR * bstrOut = NULL;
    LPWSTR pwzTemp;
    UINT cchSize = (lstrlenA(pszAnsiStr) + 2);  // Count of characters

    if (!pszAnsiStr)
        return NULL;    // What the hell do you expect?

    pwzTemp = (LPWSTR) LocalAlloc(LPTR, cchSize * sizeof(WCHAR));
    if (pwzTemp)
    {
        SHAnsiToUnicode(pszAnsiStr, pwzTemp, cchSize);
        bstrOut = SysAllocString(pwzTemp);
        LocalFree(pwzTemp);
    }

    return bstrOut;
}
#endif

#if 0 // BUGBUG - g_dfs.DefRevCount - raymondc - incomplete

void SaveDefaultFolderSettings()
{
    HKEY hkCabStreams;

    g_dfs.dwDefRevCount++;
    if (RegOpenKey(g_hkeyExplorer, c_szStreams, &hkCabStreams) == ERROR_SUCCESS)
    {
        RegSetValueEx(hkCabStreams, TEXT("Settings"), 0L, REG_BINARY, (LPBYTE)&g_dfs, SIZEOF(g_dfs));
        RegCloseKey(hkCabStreams);
    }
}

#else

void SaveDefaultFolderSettings() // BUGBUG raymondc
{
}

#endif

#endif // POSTSPLIT
