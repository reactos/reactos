// ClientCaps.cpp : Implementation of CClientCaps
#include "headers.h"
#include "iextag.h"
#include "ccaps.h"

typedef HRESULT STDAPICALLTYPE SHOWHTMLDIALOGFN (HWND hwndParent, IMoniker *pmk, 
                                                 VARIANT *pvarArgIn, WCHAR* pchOptions, 
                                                 VARIANT *pvArgOut);
#define REGKEY_ACTIVESETUP   "Software\\Microsoft\\Active Setup"

/////////////////////////////////////////////////////////////////////////////
// CClientCaps


STDMETHODIMP CClientCaps::get_javaEnabled(VARIANT_BOOL * pVal)
{
    IOmNavigator *pClientInformation;
    HRESULT hr = GetClientInformation(&pClientInformation);

    if (SUCCEEDED(hr))
    {
        hr = pClientInformation->javaEnabled(pVal);
        pClientInformation->Release();
    }
        return hr;
}

STDMETHODIMP CClientCaps::get_cookieEnabled(VARIANT_BOOL * pVal)
{
    IOmNavigator *pClientInformation;
    HRESULT hr = GetClientInformation(&pClientInformation);

    if (SUCCEEDED(hr))
    {
        hr = pClientInformation->get_cookieEnabled(pVal);
        pClientInformation->Release();
    }
        return hr;

}

STDMETHODIMP CClientCaps::get_cpuClass(BSTR * p)
{
    IOmNavigator *pClientInformation;
    HRESULT hr = GetClientInformation(&pClientInformation);

    if (SUCCEEDED(hr))
    {
        hr = pClientInformation->get_cpuClass(p);
        pClientInformation->Release();
    }
        return hr;

}

STDMETHODIMP CClientCaps::get_systemLanguage(BSTR * p)
{
    IOmNavigator *pClientInformation;
    HRESULT hr = GetClientInformation(&pClientInformation);

    if (SUCCEEDED(hr))
    {
        hr = pClientInformation->get_systemLanguage(p);
        pClientInformation->Release();
    }
        return hr;

}

STDMETHODIMP CClientCaps::get_userLanguage(BSTR * p)
{
    IOmNavigator *pClientInformation;
    HRESULT hr = GetClientInformation(&pClientInformation);

    if (SUCCEEDED(hr))
    {
        hr = pClientInformation->get_userLanguage(p);
        pClientInformation->Release();
    }
        return hr;

}

STDMETHODIMP CClientCaps::get_platform(BSTR * p)
{
    IOmNavigator *pClientInformation;
    HRESULT hr = GetClientInformation(&pClientInformation);

    if (SUCCEEDED(hr))
    {
        hr = pClientInformation->get_platform(p);
        pClientInformation->Release();
    }
        return hr;

}

STDMETHODIMP CClientCaps::get_connectionSpeed(long * p)
{
    IOmNavigator *pClientInformation;
    HRESULT hr = GetClientInformation(&pClientInformation);

    if (SUCCEEDED(hr))
    {
        hr = pClientInformation->get_connectionSpeed(p);
        pClientInformation->Release();
    }
        return hr;

}

STDMETHODIMP CClientCaps::get_onLine(VARIANT_BOOL *p)
{
    IOmNavigator *pClientInformation;
    HRESULT hr = GetClientInformation(&pClientInformation);

    if (SUCCEEDED(hr))
    {
        hr = pClientInformation->get_onLine(p);
        pClientInformation->Release();
    }
        return hr;

}

STDMETHODIMP CClientCaps::get_colorDepth(long * p)
{
    IHTMLScreen *pScreen;
    HRESULT hr = GetScreen(&pScreen);

    if (SUCCEEDED(hr))
    {
        hr = pScreen->get_colorDepth(p);
        pScreen->Release();
    }
        return hr;

}

STDMETHODIMP CClientCaps::get_bufferDepth(long * p)
{
    IHTMLScreen *pScreen;
    HRESULT hr = GetScreen(&pScreen);

    if (SUCCEEDED(hr))
    {
        hr = pScreen->get_bufferDepth(p);
        pScreen->Release();
    }
        return hr;

}

STDMETHODIMP CClientCaps::get_width(long * p)
{
    IHTMLScreen *pScreen;
    HRESULT hr = GetScreen(&pScreen);

    if (SUCCEEDED(hr))
    {
        hr = pScreen->get_width(p);
        pScreen->Release();
    }
        return hr;

}

STDMETHODIMP CClientCaps::get_height(long * p)
{
    IHTMLScreen *pScreen;
    HRESULT hr = GetScreen(&pScreen);

    if (SUCCEEDED(hr))
    {
        hr = pScreen->get_height(p);
        pScreen->Release();
    }
        return hr;

}

STDMETHODIMP CClientCaps::get_availHeight(long * p)
{
    IHTMLScreen *pScreen;
    HRESULT hr = GetScreen(&pScreen);

    if (SUCCEEDED(hr))
    {
        hr = pScreen->get_availHeight(p);
        pScreen->Release();
    }
        return hr;

}

STDMETHODIMP CClientCaps::get_availWidth(long * p)
{
    IHTMLScreen *pScreen;
    HRESULT hr = GetScreen(&pScreen);

    if (SUCCEEDED(hr))
    {
        hr = pScreen->get_availWidth(p);
        pScreen->Release();
    }
        return hr;

}

STDMETHODIMP CClientCaps::get_connectionType(BSTR *pbstr)
{
    DWORD dwFlags = 0;
    BOOL bConnected;
    HRESULT hr = S_OK;
    DWORD dwState = 0;
    DWORD dwSize = sizeof(dwState);
    BOOL bGlobalOffline = FALSE ;  // Has the user gone into offline mode, inspite of having a connection.       

    if (InternetQueryOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState, &dwSize))
    {
        if (dwState &  INTERNET_STATE_DISCONNECTED_BY_USER)
            bGlobalOffline = TRUE;
    }

    bConnected = InternetGetConnectedStateEx(&dwFlags, NULL, 0, 0);


    // NOTE: We use literal strings in code and resources so these don't get localized.
    if (bConnected && !bGlobalOffline)
    {
        // If we are connected figure out how.
        if (dwFlags &  INTERNET_CONNECTION_MODEM )
        {
            *pbstr = SysAllocString(L"modem"); 
        }
        else if (dwFlags & INTERNET_CONNECTION_LAN )
        {
            *pbstr = SysAllocString(L"lan");
        }
        else 
        {
            // Don't know what to do here. 
            *pbstr = SysAllocString(L"");
            hr = S_FALSE;
        }
    }
    else 
    {
        *pbstr = SysAllocString(L"offline");
    }                    

    if (!*pbstr)
        hr = E_OUTOFMEMORY;
        
    return hr;
}
        
STDMETHODIMP CClientCaps::isComponentInstalled(BSTR bstrName, BSTR bstrType, BSTR bstrVersion, VARIANT_BOOL *pVal)
{

    if (pVal == NULL)
    {
        return E_POINTER;
    }

    DWORD dwVersionMS;
    DWORD dwVersionLS;
    HRESULT hr = GetVersion(bstrName, bstrType, &dwVersionMS, &dwVersionLS);
    HRESULT hrRet;

    if (hr == S_OK)
    {
        if (bstrVersion && bstrVersion[0] != L'\0')
        {
            // User wants us to check for minimum version number.
            DWORD dwVersionReqdMS;
            DWORD dwVersionReqdLS;
            hr = GetVersionFromString(bstrVersion, &dwVersionReqdMS, &dwVersionReqdLS);
            if (SUCCEEDED(hr))
            {
                if ( (dwVersionMS > dwVersionReqdMS) || 
                     (dwVersionMS == dwVersionReqdMS && dwVersionLS >= dwVersionReqdLS)
                   )
                {
                    *pVal = TRUE;
                }
                else 
                {
                    *pVal = FALSE;
                }
                hrRet = S_OK;
            }
            else
            {
                *pVal = FALSE;
                hrRet = S_FALSE; 
            }
        }
        else 
        {                      
            *pVal = TRUE;
            hrRet = S_OK;
        }
    }
    else if (hr == S_FALSE)
    {
        *pVal = FALSE;
        hrRet = S_OK;
    }
    else
    {
        // This is really an error, but to avoid script error dialogs we still return a success
        // code, but indicate that the component is not installed.
        *pVal = FALSE;
        hrRet = S_FALSE;
    }
                         
        return hrRet;
}

STDMETHODIMP CClientCaps::getComponentVersion(BSTR bstrName, BSTR bstrType, BSTR* pbstrVersion)
{
    if (pbstrVersion == NULL)
        return E_POINTER;

    DWORD dwVersionMS;
    DWORD dwVersionLS;
    HRESULT hr = GetVersion(bstrName, bstrType, &dwVersionMS, &dwVersionLS);

    if (hr == S_OK)
    {
        hr = GetStringFromVersion(dwVersionMS, dwVersionLS, pbstrVersion); 
    }
    else 
    {
        *pbstrVersion = SysAllocString(L"");
        hr = S_OK;
    }                      
         
        return hr;
}

STDMETHODIMP CClientCaps::compareVersions(BSTR bstrVer1, BSTR bstrVer2, long *p)
{
    if ( p == NULL)
        return E_POINTER;

    HRESULT hr = S_OK;
    if (bstrVer1 == NULL || bstrVer2 == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        DWORD dwMS1 = 0 , dwLS1 = 0, dwMS2 = 0, dwLS2 = 0;

        HRESULT hr1 = GetVersionFromString(bstrVer1, &dwMS1, &dwLS1);
        HRESULT hr2 = GetVersionFromString(bstrVer2, &dwMS2, &dwLS2);

        if (SUCCEEDED(hr1) && SUCCEEDED(hr2))
        {
            if (dwMS1 > dwMS2)
                *p = 1;
            else if (dwMS1 < dwMS2)
                *p = -1;
            else  /* dwMS1 == dwMS2 */
            {
                if (dwLS1 > dwLS2)
                    *p = 1;
                else if (dwLS1 < dwLS2)
                    *p = -1;
                else
                    *p = 0;
            }
            hr = S_OK;
        }
        else
        {
            *p = 1;         // BUGBUG: what is the right thing to do here.
            hr = S_FALSE;
        }
    }
    return hr;
}


STDMETHODIMP CClientCaps::addComponentRequest(BSTR bstrName, BSTR bstrType, BSTR bstrVer)
// Checks if the passed component is installed (optionally at or above the passed version)
// and if not, adds it to a list of components to be added at a call to DoComponentRequest
{
    HRESULT hr = 0;
    VARIANT_BOOL bInstalled;
    uCLSSPEC classspec;
    int iLength;
    int iRes;
    LPWSTR pwszComponentID = NULL;

    LPSTR pszComponentID = NULL;



    hr = isComponentInstalled(bstrName, bstrType, bstrVer, &bInstalled);

    // Unknown Error
    if(! SUCCEEDED(hr))
    {
        goto Exit;
    }
    // Component is already installed
    if(bInstalled)
    {
        hr = S_OK;
        goto Exit;
    }
    // otherwise, add the comnponent to the list of components to be installed


    // First figure out the type of the component and populate the CLSSPEC appropriately.
    if (0 == StrCmpICW(bstrType, L"mimetype"))
    {
        classspec.tyspec =  TYSPEC_MIMETYPE;
        classspec.tagged_union.pMimeType = bstrName;
    }
    else if (0 == StrCmpICW(bstrType, L"progid"))
    {
        classspec.tyspec = TYSPEC_PROGID;
        classspec.tagged_union.pProgId = bstrName;
    }
    else if (0 == StrCmpICW(bstrType, L"clsid"))
    {
        classspec.tyspec = TYSPEC_CLSID;
        // Convert class-id string to GUID.
        hr = CLSIDFromString(bstrName, &classspec.tagged_union.clsid);
        if (FAILED(hr))
            goto Exit;
    }
    else if (0 == StrCmpICW(bstrType, L"componentid")) //internally called a FeatureID
    {
        classspec.tyspec = TYSPEC_FILENAME;
        classspec.tagged_union.pFileName = bstrName;
    }
    else
    {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    // Get a ComponentID from the uCLSSPEC
    hr = GetComponentIDFromCLSSPEC(&classspec, &pszComponentID);
    if(FAILED(hr))
    {
        goto Exit;
    }


    // Convert the ComponentID to a wide character string (wide expected by ShowHTMLDialog)
    iLength = lstrlenA(pszComponentID) + 1;
    pwszComponentID = new WCHAR[iLength];
    if(pwszComponentID == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    iRes = MultiByteToWideChar(CP_ACP,0,pszComponentID, iLength,
                             pwszComponentID, iLength);
    if(iRes == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        if(pwszComponentID)
        {
            delete [] pwszComponentID;
            pwszComponentID = NULL;
        }
        goto Exit;
    }


    // initialize array for first time
    if(ppwszComponents == NULL)
    {
        // Hard coded initial size of 10; It's in the right order of magnitude
        // Maybe this should be in a constant, but that seems a lot of overhead
        // for such a small feature
        ppwszComponents = new LPWSTR[10];
        iComponentNum = 0;
        iComponentCap = 10;
    }

    // Resizing the array of Components
    if(iComponentNum >= iComponentCap)
    {
        iComponentCap *= 2;
        LPWSTR * ppwszOldComponents = ppwszComponents;
        ppwszComponents = NULL;
        ppwszComponents = new LPWSTR[iComponentCap];
        if(ppwszComponents == NULL)
        {
            hr = E_OUTOFMEMORY;
            ppwszComponents = ppwszOldComponents;
            if(pwszComponentID)
            {
                delete [] pwszComponentID;
                pwszComponentID = NULL;
            }
            goto Exit;
        }
        for(int i = 0; i < iComponentNum; i++)
        {
            ppwszComponents[i] = ppwszOldComponents[i];
        }
        delete [] ppwszOldComponents;
    }

    ppwszComponents[iComponentNum++] =  pwszComponentID;
    hr = S_OK;    

Exit:

    if(pszComponentID)
    {
        CoTaskMemFree(pszComponentID);
    }
    return hr;
}

STDMETHODIMP CClientCaps::doComponentRequest(VARIANT_BOOL * pVal)
// Uses the url in HKLM\Software\Microsoft\Active Setup\JITSetupPage 
// to add the list of components logged using AddComponentRequest
{
    SHOWHTMLDIALOGFN *pfnShowHTMLDialog = NULL;
    HINSTANCE hInst = 0;
    HRESULT hr = 0;
    LPWSTR pwszFeatures = NULL;
    IMoniker *pMk = NULL;
    VARIANT vtDialogArg;
    VariantInit(&vtDialogArg);
    VARIANT vtRetVal;
    VariantInit(&vtRetVal);
    int i,j,k,iFeatureArgLength;
    TCHAR *pszSETUPPAGE = _T("JITSetupPage");
    WCHAR wszDownLoadPage[MAX_PATH];
    WCHAR wszDownLoadPageReg[MAX_PATH];
//    LPWSTR pwszDownLoadPage = NULL;
    HKEY hkeyActiveSetup = 0;
    LONG lResult = 0;
    DWORD dwType;
    DWORD dwSize = INTERNET_MAX_URL_LENGTH;
    int iLength = 0;
    int iRes = 0;
    OSVERSIONINFO osvi;


    // NT5 should never show JIT dialog.

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
        osvi.dwMajorVersion >= 5) {
        hr = E_ACCESSDENIED;
        goto Exit;
    }

    // No Component Requests to process.  Return success and exit
    if(iComponentNum <= 0)
    {
        if(pVal)
            *pVal = TRUE;
        hr = S_OK;
        goto Exit;
    }


    // calculate space needed
    iFeatureArgLength = 0;
    // Length for the characters in the ComponentIDs
    for(k = 0; k < iComponentNum; k++)
    {
        iFeatureArgLength += lstrlenW(ppwszComponents[k]);
    }
    iFeatureArgLength += 9*iComponentNum - 1;  // "Feature="s and &'s
    iFeatureArgLength += 10;                 // breathing room
    pwszFeatures = new WCHAR[iFeatureArgLength];
    if(!pwszFeatures)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // copy the individual strings to one big string, seperated by "&feature="
    // i.e. "feature=JavaVM&feature=MOBILEPKx86"
    // i is the position in the full string
    // k is the current substring
    for(i = k = 0; k < iComponentNum; k++)
    {
        // "feature="
        StrCpyW(pwszFeatures + i, L"feature=");
        i += 8;
        iLength = lstrlenW(ppwszComponents[k]);
        // componentID
        StrCpyW(pwszFeatures + i, ppwszComponents[k]);
        i += iLength;
        // "&" || '\0'
        if(k + 1 < iComponentNum)
        {
            pwszFeatures[i] = L'&';
            i++;
        }
        else
        {
            pwszFeatures[i] = L'\0';
            i++;
        }
    }

    // Change string to variant
    vtDialogArg.vt = VT_BSTR;
    vtDialogArg.bstrVal = SysAllocString(pwszFeatures);
    if(! vtDialogArg.bstrVal)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // get the download dialog page from the registry
    if ((lResult = RegOpenKeyExA( HKEY_LOCAL_MACHINE, 
            REGKEY_ACTIVESETUP, 0, KEY_READ,
            &hkeyActiveSetup)) != ERROR_SUCCESS) 
    {
        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    if (lResult = SHQueryValueEx(hkeyActiveSetup, pszSETUPPAGE, NULL, 
        &dwType, wszDownLoadPageReg, &dwSize) != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    if (wszDownLoadPageReg) {
        WCHAR             *wszDownLoadPageFile = NULL;

        wszDownLoadPageFile = PathFindFileNameW(wszDownLoadPageReg);
        hr = SHGetWebFolderFilePathW(wszDownLoadPageFile, wszDownLoadPage, MAX_PATH);
        if (FAILED(hr)) {
            goto Exit;
        }
    }

/*    iLength = lstrlenA(szDownLoadPage) + 1;
    pwszDownLoadPage = new WCHAR[iLength];
    iRes = MultiByteToWideChar(CP_ACP,0,szDownLoadPage, iLength,
                             pwszDownLoadPage, iLength);
    if(iRes == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

*/
    // Get Moniker to The JIT Dialog page
    hr =  CreateURLMoniker(NULL, wszDownLoadPage, &pMk);
    if (FAILED(hr))
        goto Exit;


    // Get the ShowHTMLDialog function from the mshtml dll
    hInst = LoadLibraryEx(TEXT("MSHTML.DLL"), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hInst)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    pfnShowHTMLDialog = (SHOWHTMLDIALOGFN *)GetProcAddress(hInst, "ShowHTMLDialog");
    if (!pfnShowHTMLDialog) 
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }    

    
    // Show the JIT download page; the rest of the work is accomplished there
    hr = (*pfnShowHTMLDialog)(NULL, pMk, &vtDialogArg, NULL, &vtRetVal);

    if(FAILED(hr))
    {
        goto Exit;
    }


    // Process the return value
    if ((vtRetVal.vt == VT_I4) && vtRetVal.lVal != S_OK ) 
    {
        hr = S_FALSE;
        if(pVal)
            *pVal = FALSE;
    }
    else if(vtRetVal.vt != VT_I4)
    {
        hr = S_FALSE;
        if(pVal)
            *pVal = FALSE;
    }
    else
    {
        hr = S_OK;
        if(pVal)
           *pVal = TRUE;
    }

    // Since everything was successful, clear the component list
    clearComponentRequest();



Exit:    // Goto needed because these resources must be cleaned up on all errors and successes

    // clear string
    if(pwszFeatures)
        delete [] pwszFeatures;
    if(pMk)
    {
        pMk->Release();
    }
//    if(pwszDownLoadPage)
//    {
//        delete [] pwszDownLoadPage;
//        pwszDownLoadPage = NULL;
//    }
    VariantClear(&vtRetVal);
    VariantClear(&vtDialogArg);
    if(hInst)
    {
         FreeLibrary(hInst);
    }

    return hr;
}

STDMETHODIMP CClientCaps::clearComponentRequest()
// Clears the list of components logged using AddComponentRequest
{
    int i;
    for(i = 0; i < iComponentNum; i++)
    {
        if(ppwszComponents[i] != NULL)
        {
            // delete the wide string
            delete [] ppwszComponents[i];
            ppwszComponents[i] = NULL;
        }
    }
    iComponentNum = 0;
    return S_OK;
}

    
// IElementBehavior methods

STDMETHODIMP CClientCaps::Init(IElementBehaviorSite *pSite)
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

STDMETHODIMP CClientCaps::Notify(LONG lNotify, VARIANT * pVarNotify)
{
    return S_OK;
}

// Helper functions for internal use only

STDMETHODIMP CClientCaps::GetHTMLDocument(IHTMLDocument2 **ppDoc)
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


STDMETHODIMP CClientCaps::GetHTMLWindow(IHTMLWindow2 **ppWindow)
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

STDMETHODIMP CClientCaps::GetClientInformation(IOmNavigator **ppNav)
{
    HRESULT hr = E_FAIL;
    IHTMLWindow2 *pWindow = NULL;

    hr = GetHTMLWindow(&pWindow);

    if (SUCCEEDED(hr))
    {
        hr = pWindow->get_clientInformation(ppNav);
        pWindow->Release();
    }

    return hr ;
}

STDMETHODIMP CClientCaps::GetScreen(IHTMLScreen **ppScreen)
{
    HRESULT hr = E_FAIL;
    IHTMLWindow2 *pWindow = NULL;

    hr = GetHTMLWindow(&pWindow);

    if (SUCCEEDED(hr))
    {
        hr = pWindow->get_screen(ppScreen);
        pWindow->Release();
    }

    return hr ;
}

// Returns S_OK if component is installed and the version # in pbstrVersion.
// Returns S_FALSE if component is not installed.
// E_INVALIDARG if the input args are ill-formed or not an IE component.
// E_* if it encounters an error. 

STDMETHODIMP CClientCaps::GetVersion(BSTR bstrName, BSTR bstrType, LPDWORD pdwFileVersionMS, LPDWORD pdwFileVersionLS)
{
    if (bstrName == NULL || bstrType == NULL)
        return E_INVALIDARG;
        
    uCLSSPEC classspec;
    HRESULT hr = E_FAIL;
    QUERYCONTEXT qc = {0};

    // First figure out the type of the component and populate the CLSSPEC appropriately.
    if (0 == StrCmpICW(bstrType, L"mimetype"))
    {
        classspec.tyspec =  TYSPEC_MIMETYPE;
        classspec.tagged_union.pMimeType = bstrName;
    }
    else if (0 == StrCmpICW(bstrType, L"progid"))
    {
        classspec.tyspec = TYSPEC_PROGID;
        classspec.tagged_union.pProgId = bstrName;
    }
    else if (0 == StrCmpICW(bstrType, L"clsid"))
    {
        classspec.tyspec = TYSPEC_CLSID;
        // Convert class-id string to GUID.
        hr = CLSIDFromString(bstrName, &classspec.tagged_union.clsid);
        if (FAILED(hr))
            goto Exit;
    }
    else if (0 == StrCmpICW(bstrType, L"componentid"))
    {
        classspec.tyspec = TYSPEC_FILENAME;
        classspec.tagged_union.pFileName = bstrName;
    }
    else
    {
        hr = E_INVALIDARG;
        goto Exit;
    }                


    hr = FaultInIEFeature(NULL, &classspec, &qc, FIEF_FLAG_PEEK);

    if (hr == S_OK || (qc.dwVersionHi != 0 || qc.dwVersionLo != 0))
    {
        // Put the version #'s that we found in the out args.
        if (pdwFileVersionMS != NULL)
            *pdwFileVersionMS = qc.dwVersionHi;
        if (pdwFileVersionLS != NULL)
            *pdwFileVersionLS = qc.dwVersionLo;

        hr = S_OK;         
    }
    else if ( hr == S_FALSE)
    {
        // this implies the component is not recognized as an IE component.
        // The input argument must be incorrect in this case.
        hr = E_INVALIDARG;
    }
    else if ( hr == HRESULT_FROM_WIN32(ERROR_PRODUCT_UNINSTALLED))
    {
        hr = S_FALSE;
    }

Exit:
    return hr;
} 


// ---------------------------------------------------------------------------
// %%Function: GetVersionFromString
//
//    converts version in text format (a,b,c,d) into two dwords (a,b), (c,d)
//    The printed version number is of format a.b.d (but, we don't care)
// ---------------------------------------------------------------------------
HRESULT
CClientCaps::GetVersionFromString(LPCOLESTR szBuf, LPDWORD pdwFileVersionMS, LPDWORD pdwFileVersionLS)
{
    LPCOLESTR pch = szBuf;
    OLECHAR ch;

    *pdwFileVersionMS = 0;
    *pdwFileVersionLS = 0;

    if (!pch)            // default to zero if none provided
        return S_OK;

    if (StrCmpCW(pch, L"-1,-1,-1,-1") == 0) {
        *pdwFileVersionMS = 0xffffffff;
        *pdwFileVersionLS = 0xffffffff;
        return S_OK;
    }

    USHORT n = 0;

    USHORT a = 0;
    USHORT b = 0;
    USHORT c = 0;
    USHORT d = 0;

    enum HAVE { HAVE_NONE, HAVE_A, HAVE_B, HAVE_C, HAVE_D } have = HAVE_NONE;


    for (ch = *pch++;;ch = *pch++) {

        if ((ch == L',') || (ch == L'\0')) {

            switch (have) {

            case HAVE_NONE:
                a = n;
                have = HAVE_A;
                break;

            case HAVE_A:
                b = n;
                have = HAVE_B;
                break;

            case HAVE_B:
                c = n;
                have = HAVE_C;
                break;

            case HAVE_C:
                d = n;
                have = HAVE_D;
                break;

            case HAVE_D:
                return E_INVALIDARG; // invalid arg
            }

            if (ch == L'\0') {
                // all done convert a,b,c,d into two dwords of version

                *pdwFileVersionMS = ((a << 16)|b);
                *pdwFileVersionLS = ((c << 16)|d);

                return S_OK;
            }

            n = 0; // reset

        } else if ( (ch < L'0') || (ch > L'9'))
            return E_INVALIDARG;    // invalid arg
        else
            n = n*10 + (ch - L'0');


    } /* end forever */

    // NEVERREACHED
}


// ---------------------------------------------------------------------------
// %%Function: GetStringFromVersion
//
//    converts version from two DWORD's to the string format a,b,c,d
// ---------------------------------------------------------------------------

HRESULT CClientCaps::GetStringFromVersion(DWORD dwVersionMS, DWORD dwVersionLS, BSTR *pbstrVersion)
{
    if (pbstrVersion == NULL)
        return E_POINTER;

    // 16-bits is a max of 5 decimal digits * 4 + 3 ','s  + null terminator    
    const int maxStringSize = 5 * 4 + 3 * 1 + 1;                            
    OLECHAR rgch[maxStringSize];

    USHORT a = (USHORT)(dwVersionMS >> 16);
    USHORT b = (USHORT)(dwVersionMS & 0xffff);
    USHORT c = (USHORT)(dwVersionLS >> 16);
    USHORT d = (USHORT)(dwVersionLS & 0xffff);

    OLECHAR rgchFormat[] = L"%hu,%hu,%hu,%hu";

    int nRet = wnsprintfW(rgch, maxStringSize, rgchFormat, a, b, c, d);

    HRESULT hr;
    if (nRet < 7 ) // 0,0,0,0
    {
        hr = E_FAIL;
    }
    else
    {
        *pbstrVersion = SysAllocString(rgch);
        if (*pbstrVersion == NULL)
            hr = E_OUTOFMEMORY;
        else
            hr = S_OK;
    }
    
    return hr;
}                            
