// homepg.cxx : Implementation of CHomePage
#include "headers.h"
#include "iextag.h"
#include "homepg.h"
#include "shlwapi.h"
#include "inetreg.h"
#include "wininet.h"
#include "urlmon.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CHomePage

STDMETHODIMP CHomePage::Init(IElementBehaviorSite *pSite)
{
    HRESULT hr = E_INVALIDARG;

    if (pSite != NULL)
    {
        m_pSite = pSite;
        m_pSite->AddRef();
        hr = S_OK;
    }

    return hr;  
}

STDMETHODIMP CHomePage::Notify(LONG lNotify, VARIANT * pVarNotify)
{
    return S_OK;
}

// Helper functions for internal use only
STDMETHODIMP CHomePage::GetWindow(HWND *phWnd)
{
    HRESULT hr = E_FAIL;
    IWindowForBindingUI *pWindowForBindingUI = NULL;

    if (m_pSite != NULL) {

        // Get IWindowForBindingUI ptr
        hr = m_pSite->QueryInterface(IID_IWindowForBindingUI,
                (LPVOID *)&pWindowForBindingUI);

        if (FAILED(hr)) {
            IServiceProvider *pServProv;
            hr = m_pSite->QueryInterface(IID_IServiceProvider, (LPVOID *)&pServProv);

            if (hr == NOERROR) {
                pServProv->QueryService(IID_IWindowForBindingUI,IID_IWindowForBindingUI,
                    (LPVOID *)&pWindowForBindingUI);
                pServProv->Release();
            }
        }

        if (pWindowForBindingUI) {
            pWindowForBindingUI->GetWindow(IID_IWindowForBindingUI, phWnd);
            pWindowForBindingUI->Release();
        }
    }

    return hr;
}

STDMETHODIMP CHomePage::GetHTMLDocument(IHTMLDocument2 **ppDoc)
{
    HRESULT hr = E_FAIL;

    if (m_pSite != NULL)
    {
        IHTMLElement *pElement = NULL;
        hr = m_pSite->GetElement(&pElement);
        if (SUCCEEDED(hr))
        {
            IDispatch * pDispDoc = NULL;
            hr = pElement->get_document(&pDispDoc);
            if (SUCCEEDED(hr))
            {
                hr = pDispDoc->QueryInterface(IID_IHTMLDocument2, (void **)ppDoc);
                pDispDoc->Release();
            }
            pElement->Release();
        }
    }

    return hr;
}


STDMETHODIMP CHomePage::GetHTMLWindow(IHTMLWindow2 **ppWindow)
{
    HRESULT hr = E_FAIL;
    IHTMLDocument2 *pDoc = NULL;

    hr = GetHTMLDocument(&pDoc);

    if (SUCCEEDED(hr))
    {
        hr = pDoc->get_parentWindow(ppWindow);
        pDoc->Release();
    }

    return hr;
}

STDMETHODIMP CHomePage::IsSameSecurityID(IInternetSecurityManager *pISM, BSTR bstrURL, BSTR bstrDocBase)
{
    HRESULT hrRet = S_FALSE;
    HRESULT hr = S_OK;
    BYTE pbSecIdURL[INTERNET_MAX_URL_LENGTH];
    BYTE pbSecIdDocBase[INTERNET_MAX_URL_LENGTH];
    DWORD dwSecIdURL = INTERNET_MAX_URL_LENGTH, dwSecIdDocBase = INTERNET_MAX_URL_LENGTH;

    hr = pISM->GetSecurityId(bstrDocBase, pbSecIdDocBase, &dwSecIdDocBase, 0);

    _ASSERTE(hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    if (SUCCEEDED(hr)) {

        hr = pISM->GetSecurityId(bstrURL, pbSecIdURL, &dwSecIdURL, 0);
        _ASSERTE(hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

        if (SUCCEEDED(hr)) {

            if ((dwSecIdURL == dwSecIdDocBase) &&
                (memcmp(pbSecIdURL, pbSecIdDocBase, dwSecIdURL) == 0)) {

                hrRet = S_OK;
            }
        }
    }

    return hrRet;
}

STDMETHODIMP CHomePage::IsAuthorized(BSTR bstrURL)
{
    HRESULT hr = E_FAIL;
    IHTMLDocument2 *pDoc = NULL;
    BSTR bstrDocBase = NULL;
    IInternetSecurityManager *pISM = NULL;

    hr = GetHTMLDocument(&pDoc);

    if (SUCCEEDED(hr))
    {
        hr = pDoc->get_URL(&bstrDocBase);

        if (SUCCEEDED(hr)) {
            if (SUCCEEDED(hr = CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER,
                IID_IInternetSecurityManager, (void **)&pISM))) {


                hr = IsSameSecurityID(pISM, bstrURL, bstrDocBase);
                pISM->Release();
            }

            SysFreeString(bstrDocBase);
        }

        pDoc->Release();
    }

    return SUCCEEDED(hr)?hr:S_FALSE;
}


STDMETHODIMP CHomePage::SetUserHomePage(LPCSTR szURL)
{
    HRESULT hr = S_OK;
    LONG    lResult = ERROR_SUCCESS;
    HKEY    hKey    = NULL;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_MAIN, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
    {
        if (((lResult = RegSetValueExA (hKey, REGSTRA_VAL_STARTPAGE, 0, REG_SZ,
                (LPCBYTE)szURL, lstrlenA(szURL)+1))) != ERROR_SUCCESS) {

            hr = HRESULT_FROM_WIN32(lResult);
        }
    }

    if (hKey)
        RegCloseKey(hKey);

    return FAILED(hr)?S_FALSE:hr;
}

STDMETHODIMP CHomePage::GetHomePage(BSTR& bstrURL, BSTR& bstrName)
{
    LPWSTR pwszName = L"_top";
    HRESULT hr = E_FAIL;
    WCHAR wszHomePage[INTERNET_MAX_URL_LENGTH];
    DWORD dwType;
    DWORD dwSize = INTERNET_MAX_URL_LENGTH;

    if (SHRegGetUSValueW(REGSTR_PATH_MAIN, 
                        REGSTR_VAL_STARTPAGE, 
                        &dwType, 
                        wszHomePage, 
                        &dwSize, 
                        0, NULL, 0) == ERROR_SUCCESS) 
    {
        bstrURL = SysAllocString(wszHomePage);
    }

    bstrName = SysAllocString(pwszName);

    return (bstrName && bstrURL) ? S_OK : E_FAIL;
}

STDMETHODIMP CHomePage::setHomePage(BSTR bstrURL)
{
    HRESULT hr = S_FALSE;
    char szMsgTitle[MAX_PATH];
    char szMsgFormat[INTERNET_MAX_URL_LENGTH];
    char szURL[INTERNET_MAX_URL_LENGTH];
    char szPageTitle[INTERNET_MAX_URL_LENGTH];
    char szMsg[2*INTERNET_MAX_URL_LENGTH];
    HWND hWnd = NULL;
    DWORD dwValue = 0;
    DWORD dwLen = sizeof(DWORD);
    HKEY hkeyRest = 0;

    // Check if setting home page is restricted

    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_SET_HOMEPAGE_RESTRICTION, 0,
                     KEY_READ, &hkeyRest) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hkeyRest, REGVAL_HOMEPAGE_RESTRICTION, NULL, NULL,
                            (LPBYTE)&dwValue, &dwLen) == ERROR_SUCCESS
            && dwValue)
        {
            hr = E_ACCESSDENIED;
        }

        RegCloseKey(hkeyRest);

        if (FAILED(hr))
        {
            goto Exit;
        }
    }

    if ( bstrURL && WideCharToMultiByte(CP_ACP, 0, bstrURL, -1, szURL,
            sizeof(szURL), NULL, NULL) &&
         LoadStringA(g_hInst, IDS_SETHOMEPAGE_TITLE, szMsgTitle, sizeof(szMsgTitle)) &&
         LoadStringA(g_hInst, IDS_SETHOMEPAGE_MSG, szMsgFormat, sizeof(szMsgFormat))) {

         if (wnsprintfA(szMsg, sizeof(szMsg), szMsgFormat, szURL) && GetWindow(&hWnd) == S_OK) {
            MSGBOXPARAMS      mbp;
            WCHAR             wzMsg[MAX_HOMEPAGE_MESSAGE_LEN];
            WCHAR             wzTitle[MAX_HOMEPAGE_TITLE_LEN];

            if (!MultiByteToWideChar(CP_ACP, 0, szMsg, -1, wzMsg,
                                     MAX_HOMEPAGE_MESSAGE_LEN))
            {
                goto Exit;
            }

            if (!MultiByteToWideChar(CP_ACP, 0, szMsgTitle, -1, wzTitle,
                                     MAX_HOMEPAGE_TITLE_LEN))
            {
                goto Exit;
            }
                                                    
            mbp.cbSize = sizeof (MSGBOXPARAMS);
            mbp.hwndOwner = hWnd;
            mbp.hInstance = g_hInst;
            mbp.dwStyle = MB_YESNO | MB_USERICON;
            mbp.lpszCaption = wzTitle;
            mbp.lpszIcon = MAKEINTRESOURCE(IDI_HOMEPAGE);
            mbp.dwContextHelpId = 0;
            mbp.lpfnMsgBoxCallback = NULL;
            mbp.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT);
            mbp.lpszText = wzMsg;
        
            if (MessageBoxIndirect(&mbp) == IDYES) {

                // got approval, now stomp the reg key
                hr = SetUserHomePage(szURL);
            }
         }
    }

Exit:

    return hr;
}

STDMETHODIMP CHomePage::isHomePage(BSTR bstrURL, VARIANT_BOOL *pVal)
{
    HRESULT hr = S_OK;
    BSTR bstrHome = NULL;
    BSTR bstrName = NULL;

    if (pVal == NULL)
    {
        return E_POINTER;
    }


    hr = GetHomePage(bstrHome, bstrName);
    if (SUCCEEDED(hr)) {

        if (StrCmpIW(bstrHome, bstrURL) == 0) {

            // matches and is the home page
            // now verify that the doc base URL is indeed
            // in the same domain (same security ID and is 
            // therefore authorized to inspect the home page

            hr = IsAuthorized(bstrHome);

        } else {
            hr = S_FALSE;
        }
    }

    if (bstrName)
        SysFreeString(bstrName);

    if (bstrHome)
        SysFreeString(bstrHome);

    hr =  SUCCEEDED(hr)?hr:S_FALSE;

    if (hr == S_FALSE)
        *pVal = FALSE;
    else
        *pVal = TRUE;

    return hr;
}

STDMETHODIMP CHomePage::navigateHomePage()
{
    IHTMLWindow2 *pWindow = NULL;
    IHTMLWindow2 *pWindowNew = NULL;
    HRESULT hr = S_OK;
    BSTR bstrURL = NULL;
    BSTR bstrName = NULL;

    hr = GetHTMLWindow(&pWindow);

    if (SUCCEEDED(hr)) {

        hr = GetHomePage(bstrURL, bstrName);
        if (SUCCEEDED(hr)) {

            hr = pWindow->open(bstrURL, bstrName, NULL, NULL, &pWindowNew);
        }
    }

    if (FAILED(hr))
        hr = S_FALSE;       // script friendly

    if (pWindow)
        pWindow->Release();

    if (pWindowNew)
        pWindowNew->Release();

    if (bstrName)
        SysFreeString(bstrName);

    if (bstrURL)
        SysFreeString(bstrURL);

    return hr;
}
