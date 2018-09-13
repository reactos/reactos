#include "priv.h"
#include "caggunk.h"
#include "srchasst.h"
#include "dhuihand.h"
#include "mlang.h"  // fo char conversion
#include "..\browseui\itbar.h"  // for CITIDM_GETFOLDERSEARCHES

// from wininet.dll
extern "C"
INTERNETAPI
BOOL
WINAPI
InternetInitializeAutoProxyDll(
    DWORD dwReserved
    );

/////////////////////////////////////////////////////////////////////////////
// CSearchAssistantOC

//  If you change this, change browseui also.
const WCHAR c_wszThisBandIsYourBand[] = L"$$SearchBand$$";

//  HKLM values
#define REG_SZ_SEARCH       L"Software\\Microsoft\\Internet Explorer\\Search"
#define REG_SZ_SEARCHASSIST L"SearchAssistant"
#define REG_SZ_SEARCHCFG    L"CustomizeSearch"

//  HKCU values
#define REG_SZ_IE_MAIN      L"Software\\Microsoft\\Internet Explorer\\Main" 
#define REG_SZ_IE_SEARCURL  L"Software\\Microsoft\\Internet Explorer\\SearchURL"
#define REG_SZ_SEARCHBAR    L"Search Bar"
#define REG_SZ_USECUSTOM    L"Use Custom Search URL"
#define REG_SZ_AUTOSEARCH   L"AutoSearch"
#define REG_SZ_PROVIDER     L"Provider"

#define SAOC_VERSION        2

const WCHAR c_wszSearchProps[] = REG_SZ_SEARCH L"\\SearchProperties";

STDAPI_(VARIANT_BOOL) UseCustomInternetSearch()
{
    VARIANT_BOOL bRet;
    DWORD dwVal;
    DWORD cbVal = sizeof(dwVal);
    
    if ((SHGetValueW(HKEY_CURRENT_USER, 
                    REG_SZ_IE_MAIN, 
                    REG_SZ_USECUSTOM, 
                    NULL, 
                    &dwVal, 
                    &cbVal) == ERROR_SUCCESS) &&
        (FALSE != dwVal))
    {
        bRet = VARIANT_TRUE;
    }
    else
    {
        bRet = VARIANT_FALSE;
    }

    return bRet;
}

STDAPI_(BOOL) GetSearchAssistantUrlW(LPWSTR pwszUrl, int cchUrl, BOOL bSubstitute, BOOL bCustomize)
{
    BOOL bResult;
    WCHAR wszUrlTmp[MAX_URL_STRING];
    WCHAR *pwszUrlRead;
    DWORD cb;

    ASSERT(pwszUrl);
    *pwszUrl = 0;

    if (bSubstitute)
    {
        cb = sizeof(wszUrlTmp);
        pwszUrlRead = wszUrlTmp;
    }
    else
    {
        cb = cchUrl * sizeof(WCHAR);
        pwszUrlRead = pwszUrl;
    }
    
    bResult = SHGetValueW(HKEY_LOCAL_MACHINE, 
                          REG_SZ_SEARCH, 
                          bCustomize ? REG_SZ_SEARCHCFG : REG_SZ_SEARCHASSIST,
                          NULL, (BYTE *)pwszUrlRead, &cb) == ERROR_SUCCESS;
    if (bResult && bSubstitute)
    {
        bResult = SUCCEEDED(URLSubstitution(wszUrlTmp, pwszUrl, cchUrl, URLSUB_ALL));
    }

    return bResult;
}

STDAPI_(BOOL) GetDefaultInternetSearchUrlW(LPWSTR pwszUrl, int cchUrl, BOOL bSubstitute)
{
    BOOL bResult = FALSE;
    DWORD cb;

    ASSERT(pwszUrl);
    *pwszUrl = 0;

    if (UseCustomInternetSearch())
    {
        //  First try the user specific value
        cb = cchUrl * sizeof(TCHAR);
        bResult = SHGetValueW(HKEY_CURRENT_USER, REG_SZ_IE_MAIN, REG_SZ_SEARCHBAR, 
                             NULL, (BYTE *)pwszUrl, &cb) == ERROR_SUCCESS;
    }
    
    if (!bResult)
    {
        bResult = GetSearchAssistantUrlW(pwszUrl, cchUrl, bSubstitute, FALSE);
    }

    return bResult;
}

STDAPI_(BOOL) GetSearchAssistantUrlA(LPSTR pszUrl, int cchUrl, BOOL bSubstitute, BOOL bCustomize)
{
    WCHAR wszUrl[INTERNET_MAX_URL_LENGTH];

    BOOL bResult = GetSearchAssistantUrlW(wszUrl, ARRAYSIZE(wszUrl), bSubstitute, bCustomize);

    SHUnicodeToAnsi(wszUrl, pszUrl, cchUrl);

    return bResult;
}

STDAPI_(BOOL) GetDefaultInternetSearchUrlA(LPSTR pszUrl, int cchUrl, BOOL bSubstitute)
{
    WCHAR wszUrl[INTERNET_MAX_URL_LENGTH];

    BOOL bResult = GetDefaultInternetSearchUrlW(wszUrl, ARRAYSIZE(wszUrl), bSubstitute);

    SHUnicodeToAnsi(wszUrl, pszUrl, cchUrl);

    return bResult;
}

void SetDefaultInternetSearchUrlW(LPCWSTR pwszUrl)
{
    DWORD dwUseCustom = FALSE;
    DWORD cb;
    
    if ((NULL != pwszUrl) && (0 != *pwszUrl))
    {
        cb = (lstrlenW(pwszUrl) + 1) * sizeof(WCHAR);
        if (SHSetValueW(HKEY_CURRENT_USER, REG_SZ_IE_MAIN, REG_SZ_SEARCHBAR, REG_SZ,
                        pwszUrl, cb) == ERROR_SUCCESS)
        {
            dwUseCustom = TRUE;
        }
    }

    cb = sizeof(dwUseCustom);

    SHSetValueW(HKEY_CURRENT_USER, REG_SZ_IE_MAIN, REG_SZ_USECUSTOM, REG_DWORD, 
                &dwUseCustom, cb);
}

//  GUID string helper:
HRESULT _BstrVariantFromGUID( REFGUID refguid, VARIANT* pvarGuid )
{
    WCHAR wszGuid[GUIDSTR_MAX];
    HRESULT hr = SHStringFromGUIDW( refguid, wszGuid, ARRAYSIZE(wszGuid) );
    if (FAILED(hr))
        return hr;

    pvarGuid->vt = VT_EMPTY;
    if (NULL == (pvarGuid->bstrVal = SysAllocString( wszGuid )))
        return E_OUTOFMEMORY;

    pvarGuid->vt = VT_BSTR;
    return S_OK;
}

HRESULT CSearch_Create(GUID *pguid, BSTR bstrTitle, BSTR bstrUrl, ISearch **ppSearch)
{
    HRESULT hres = E_INVALIDARG;

    ASSERT(ppSearch);
    *ppSearch = NULL;

    if (bstrTitle && bstrUrl && pguid)
    {
        BSTR _bstrTitle = SysAllocString(bstrTitle);
        BSTR _bstrUrl   = SysAllocString(bstrUrl);

        if (_bstrTitle && _bstrUrl)
        {
            CSearch *ps = new CSearch(pguid, _bstrTitle, _bstrUrl);

            if (ps)
            {
                hres = ps->QueryInterface(IID_ISearch, (void **)ppSearch);
                ps->Release();
            }
        }
        else
        {
            if (_bstrTitle)
                SysFreeString(_bstrTitle);

            if (_bstrUrl)
                SysFreeString(_bstrUrl);

            hres = E_OUTOFMEMORY;
        }
    }
    return hres;
}

CSearch::CSearch(GUID *pguid, BSTR bstrTitle, BSTR bstrUrl) : _cRef(1), _bstrTitle(bstrTitle), _bstrUrl(bstrUrl),
                                                 CImpIDispatch(&IID_ISearch)
{
    SHStringFromGUID(*pguid, _szId, ARRAYSIZE(_szId));
}

CSearch::~CSearch()
{
    if (_bstrTitle)
        SysFreeString(_bstrTitle);

    if (_bstrUrl)
        SysFreeString(_bstrUrl);
}

STDMETHODIMP CSearch::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSearch, ISearch),
        QITABENTMULTI(CSearch, IDispatch, ISearch),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSearch::AddRef()
{
    InterlockedIncrement(&_cRef);
    return _cRef;
}

STDMETHODIMP_(ULONG) CSearch::Release()
{
    if (InterlockedDecrement(&_cRef) > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CSearch::get_Title(BSTR *pbstrTitle)
{
    HRESULT hres = NOERROR;
    
    *pbstrTitle = SysAllocString(_bstrTitle);
    if (!*pbstrTitle)
        hres = E_OUTOFMEMORY;

    return hres;
}

HRESULT CSearch::get_Id(BSTR  *pbstrId)
{
    HRESULT hres = NOERROR;

    *pbstrId = SysAllocString(_szId);
    if (!*pbstrId)
        hres = E_OUTOFMEMORY;

    return hres;
}

HRESULT CSearch::get_Url(BSTR *pbstrUrl)
{
    HRESULT hres = NOERROR;
    
    *pbstrUrl = SysAllocString(_bstrUrl);
    if (!*pbstrUrl)
        hres = E_OUTOFMEMORY;

    return hres;
}

HRESULT CSearchCollection_Create(IFolderSearches *pfs, ISearches **ppSearches)
{
    HRESULT hres = E_INVALIDARG;
    
    ASSERT(ppSearches);
    *ppSearches = NULL;

    if (pfs)
    {
        CSearchCollection *psc = new CSearchCollection(pfs);

        if (psc)
        {
            hres = psc->QueryInterface(IID_ISearches, (void **)ppSearches);
            psc->Release();
        }
        else
            hres = E_OUTOFMEMORY;
    }
    
    return hres;
}

CSearchCollection::CSearchCollection(IFolderSearches *pfs) : _cRef(1), CImpIDispatch(&IID_ISearches)
{
    GUID guid;

    if (SUCCEEDED(pfs->DefaultSearch(&guid)))
        SHStringFromGUID(guid, _szDefault, ARRAYSIZE(_szDefault));
    
    _hdsaItems = DSA_Create(SIZEOF(URLSEARCH), 4);
    if (_hdsaItems)
    {
        IEnumUrlSearch *penum;

        if (SUCCEEDED(pfs->EnumSearches(&penum)))
        {
            URLSEARCH    us;
            ULONG        cElt;

            penum->Reset();
            while (S_OK == penum->Next(1, &us, &cElt) && 1 == cElt)
                DSA_AppendItem(_hdsaItems, &us);

            penum->Release();
        }
    }
}

CSearchCollection::~CSearchCollection()
{
    DSA_Destroy(_hdsaItems);
}

STDMETHODIMP CSearchCollection::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSearchCollection, ISearches),
        QITABENTMULTI(CSearchCollection, IDispatch, ISearches),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSearchCollection::AddRef()
{
    InterlockedIncrement(&_cRef);
    return _cRef;
}

STDMETHODIMP_(ULONG) CSearchCollection::Release()
{
    if (InterlockedDecrement(&_cRef) > 0)
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CSearchCollection::get_Count(long *plCount)
{
    *plCount = 0;
    
    if (_hdsaItems)
    {
        *plCount =  DSA_GetItemCount(_hdsaItems);
    }
    return S_OK;
}

STDMETHODIMP CSearchCollection::get_Default(BSTR *pbstrDefault)
{
    HRESULT hres = E_OUTOFMEMORY;
    
    *pbstrDefault = SysAllocString(_szDefault);
    if (*pbstrDefault)
        hres = S_OK;
    
    return hres;
}

STDMETHODIMP CSearchCollection::Item(VARIANT index, ISearch **ppid)
{
    HRESULT hres = E_NOTIMPL;
    
    *ppid = NULL;
    
    switch (index.vt)
    {
        case VT_I2:
            index.lVal = (long)index.iVal;
            // And fall through...

        case VT_I4:
            if ((index.lVal >= 0) && (index.lVal < DSA_GetItemCount(_hdsaItems)))
            {
                LPURLSEARCH pus;

                pus = (LPURLSEARCH)DSA_GetItemPtr(_hdsaItems, index.lVal);
                ASSERT(pus);
                
                hres = CSearch_Create(&pus->guid, pus->wszName, pus->wszUrl, ppid);
            }

            break;
#if 0
        // BUGBUG: should we worry about this one?
        case VT_BSTR:
#endif
    }

    return hres;
}

STDMETHODIMP CSearchCollection::_NewEnum(IUnknown **ppunk)
{
    *ppunk = NULL;
    return E_NOTIMPL;
}


CSearchAssistantOC::CSearchAssistantOC()
    :   m_punkSite(NULL)
{
#ifdef UNIX
    m_dwSafety = 0;
#endif
}

CSearchAssistantOC::~CSearchAssistantOC()
{
    ATOMICRELEASE(m_pSearchBandTBHelper);
    ATOMICRELEASE(m_punkSite);
}

HRESULT CSearchAssistantOC::OnDraw(ATL_DRAWINFO& di)
{
    return S_OK;
}

STDMETHODIMP CSearchAssistantOC::SetClientSite(IOleClientSite *pClientSite)
{
    if (NULL != pClientSite)
    {
        HRESULT hr;       
        IWebBrowser2 *pWebBrowser2;

        hr = IUnknown_QueryService(pClientSite, SID_SWebBrowserApp, IID_IWebBrowser2, 
                                   (void **)&pWebBrowser2);
        if (SUCCEEDED(hr))
        {
            BSTR bstrProp = SysAllocString(c_wszThisBandIsYourBand);

            if (bstrProp)
            {
                VARIANT var;
                var.vt = VT_EMPTY;

                hr = pWebBrowser2->GetProperty(bstrProp, &var);

                if (SUCCEEDED(hr) && (var.vt == VT_UNKNOWN))
                {
                    ATOMICRELEASE(m_pSearchBandTBHelper);

                    hr = var.punkVal->QueryInterface(IID_ISearchBandTBHelper, 
                                                     (void **)&m_pSearchBandTBHelper);
                    ASSERT(SUCCEEDED(hr));

                    if (NULL != m_pSearchBandTBHelper)
                    {
                        m_pSearchBandTBHelper->SetOCCallback(this);
                    }
                   
                    var.punkVal->Release();
                }

                SysFreeString(bstrProp);
            }
            
            pWebBrowser2->Release();
        }
    }
    else
    {
        if (NULL != m_pSearchBandTBHelper)
        {

            m_pSearchBandTBHelper->SetOCCallback(NULL);
            ATOMICRELEASE(m_pSearchBandTBHelper);
        }
    }
    return IOleObjectImpl<CSearchAssistantOC>::SetClientSite(pClientSite);
}

STDMETHODIMP CSearchAssistantOC::QueryStatus(const GUID *pguidCmdGroup,
                                             ULONG cCmds, 
                                             OLECMD prgCmds[],
                                             OLECMDTEXT *pCmdText)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP CSearchAssistantOC::Exec(const GUID *pguidCmdGroup,
                                      DWORD nCmdID, 
                                      DWORD nCmdexecopt,
                                      VARIANT *pvaIn,
                                      VARIANT *pvaOut)
{
    HRESULT hr = E_UNEXPECTED;
    
    if (NULL == pguidCmdGroup)
    {
        switch (nCmdID)
        {
            case SBID_SEARCH_NEXT:
                if ((NULL != pvaIn) && (pvaIn->vt == VT_I4))
                {
                    Fire_OnNextMenuSelect(pvaIn->lVal);

                    hr = S_OK;
                }
                else
                {
                    hr = E_INVALIDARG;
                }
                break;

            case SBID_SEARCH_NEW:
                if (NULL != pvaOut)
                {
                    m_bEventHandled = VARIANT_FALSE;

                    Fire_OnNewSearch();

                    pvaOut->vt = VT_BOOL;
                    pvaOut->boolVal = m_bEventHandled;

                    hr = S_OK;
                }
                else
                {
                    hr = E_INVALIDARG;
                }
                break;
        }
    }

    return hr;
}

STDMETHODIMP CSearchAssistantOC::AddNextMenuItem(BSTR bstrText, long idItem)
{
    HRESULT hr;

    if (IsTrustedSite())
    {
        if (NULL != m_pSearchBandTBHelper)
        {
            hr = m_pSearchBandTBHelper->AddNextMenuItem(bstrText, idItem);

            ASSERT(SUCCEEDED(hr));

        }
        hr = S_OK;
    }
    else
    {
        hr = E_ACCESSDENIED;
    }
    return hr;
}

STDMETHODIMP CSearchAssistantOC::ResetNextMenu()
{
    HRESULT hr;

    if (IsTrustedSite())
    {
        if (NULL != m_pSearchBandTBHelper)
        {
            hr = m_pSearchBandTBHelper->ResetNextMenu();
            
            ASSERT(SUCCEEDED(hr));
        }
        hr = S_OK;
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}


STDMETHODIMP CSearchAssistantOC::SetDefaultSearchUrl(BSTR bstrUrl)
{
    HRESULT hr;
    
    if (IsTrustedSite())
    {
        SetDefaultInternetSearchUrlW(bstrUrl);
        hr = S_OK;
    }
    else
    {
        hr = E_ACCESSDENIED;
    }
    
    return hr;
}

STDMETHODIMP CSearchAssistantOC::NavigateToDefaultSearch()
{
    HRESULT hr;
    

    IWebBrowser2 *pWebBrowser2;

    hr = IUnknown_QueryService(m_spClientSite, SID_SWebBrowserApp, IID_IWebBrowser2, 
                               (void **)&pWebBrowser2);
    if (SUCCEEDED(hr))
    {
        WCHAR wszUrl[INTERNET_MAX_URL_LENGTH];

        if (GetDefaultInternetSearchUrlW(wszUrl, ARRAYSIZE(wszUrl), TRUE))
        {
            BSTR bstrUrl = SysAllocString(wszUrl);


            if (NULL != bstrUrl) 
            {
                VARIANT varFrame;
                varFrame.vt = VT_BSTR;
                varFrame.bstrVal = SysAllocString(L"_search");
                if (NULL != varFrame.bstrVal)
                {
                    hr = pWebBrowser2->Navigate(bstrUrl, NULL, &varFrame, NULL, NULL);

                    ASSERT(SUCCEEDED(hr));

                    SysFreeString(varFrame.bstrVal);
                }

                SysFreeString(bstrUrl);
            }
        }
        pWebBrowser2->Release();
    }
 
    return S_OK;
}

typedef struct _GUIDREST
{
    const GUID *  pguid;
    RESTRICTIONS  rest;
} GUIDREST;

HRESULT CSearchAssistantOC::IsRestricted(BSTR bstrGuid, VARIANT_BOOL *pVal)
{
    HRESULT hr;
    GUID guid;

    if (IsTrustedSite())
    {
        *pVal = VARIANT_FALSE; // default to not restricted
        if (SUCCEEDED(SHCLSIDFromString(bstrGuid, &guid)))
        {
            // find computer is special because if it restricted then we show
            // it else don't show it (restriction name is HASFINDCOMPUTER
            if (IsEqualGUID(guid, SRCID_SFindComputer))
            {
                if (!SHRestricted(REST_HASFINDCOMPUTERS))
                    *pVal = VARIANT_TRUE;
            }
            else
            {
                static GUIDREST agr[] = 
                {
                    {&SRCID_SFileSearch, REST_NOFIND},
                    // rest_nofindprinter does not exist yet
                    //{&SRCID_SFindPrinter, REST_NOFINDPRINTER},
                };

                for (int i=0; i < ARRAYSIZE(agr); i++)
                {
                    if (IsEqualGUID(guid, *agr[i].pguid))
                    {
                        if (SHRestricted(agr[i].rest))
                            *pVal = VARIANT_TRUE;
                        break;
                    }
                }
            }
        }
        hr = S_OK;
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}

HRESULT CSearchAssistantOC::get_ShellFeaturesEnabled(VARIANT_BOOL *pVal)
{
    HRESULT hr;
    
    if (IsTrustedSite())
    {
        if (pVal)
        {
            *pVal = g_bRunOnNT5 ? VARIANT_TRUE : VARIANT_FALSE;
            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
    }
    
    return hr;
}

HRESULT CSearchAssistantOC::get_SearchAssistantDefault(VARIANT_BOOL *pVal)
{
    HRESULT hr;

    if (IsTrustedSite())
    {
        if (pVal)
        {
            *pVal = !UseCustomInternetSearch();
            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
    }
    
    return hr;
}

STDMETHODIMP CSearchAssistantOC::get_Searches(ISearches **ppid)
{
    HRESULT hr;
    *ppid = NULL;

    if (IsTrustedSite())
    {
        IServiceProvider *psp;
        hr = IUnknown_QueryService(m_spClientSite, SID_STopLevelBrowser, IID_IServiceProvider, (void**)&psp);
        if (SUCCEEDED(hr))
        {
            IOleCommandTarget *pct;

            hr = psp->QueryService(SID_SExplorerToolbar, IID_IOleCommandTarget, (void **)&pct);
            if (SUCCEEDED(hr))
            {
                VARIANTARG var = {0};
                
                hr = pct->Exec(&CGID_PrivCITCommands, CITIDM_GETFOLDERSEARCHES, 0, NULL, &var);
                if (SUCCEEDED(hr))
                {
                    IFolderSearches *pfs;

                    ASSERT(var.vt == VT_UNKNOWN && var.punkVal);
                    hr = var.punkVal->QueryInterface(IID_IFolderSearches, (void **)&pfs);
                    if (SUCCEEDED(hr))
                    {
                        hr = CSearchCollection_Create(pfs, ppid);
                        pfs->Release();
                    }
                    var.punkVal->Release();
                }
                pct->Release();
                
                hr = S_OK;
            }

            psp->Release();
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}

STDMETHODIMP CSearchAssistantOC::get_InWebFolder(VARIANT_BOOL *pVal)
{
    HRESULT hr;

    if (IsTrustedSite())
    {
        ASSERT(pVal);
        *pVal = VARIANT_FALSE;

        IBrowserService2 *pbs;
        
        hr = IUnknown_QueryService(m_spClientSite, SID_STopLevelBrowser, IID_IBrowserService2, (void **)&pbs);

        if (SUCCEEDED(hr))
        {
            ITEMIDLIST *pidl;

            hr = pbs->GetPidl(&pidl);

            if (SUCCEEDED(hr))
            {
                // BUGBUG: Don't use ILIsWeb().  We should use IShellFolder2::GetDefaultSearchGUID() and
                //   test for SRCID_SWebSearch vs. SRCID_SFileSearch/SRCID_SFindComputer/SRCID_SFindPrinter.
                //   This is because Shell Extensions need a way to indicate what kind of search they want
                //   and ILIsWeb() doesn't provide that.  An example of this is "Web Folders" won't return
                //   TRUE from ILIsWeb().  The use of ILIsWeb() should be limited.
                if (ILIsWeb(pidl))
                {
                    *pVal = VARIANT_TRUE;
                }
                
                ILFree(pidl);
            }
            pbs->Release();
        }
        hr = S_OK;
    }
    else
    {
        hr = E_ACCESSDENIED;
    }
    return hr;
}

void GetPerLocalePath(WCHAR *pwszKeyName, int cchKeyName)
{
    ASSERT(cchKeyName >= (ARRAYSIZE(c_wszSearchProps) + 1));
    
    StrCpyNW(pwszKeyName, c_wszSearchProps, cchKeyName);
    *(pwszKeyName + (ARRAYSIZE(c_wszSearchProps) - 1)) = L'\\';

    GetWebLocaleAsRFC1766(pwszKeyName + ARRAYSIZE(c_wszSearchProps), 
                          cchKeyName - (ARRAYSIZE(c_wszSearchProps)));
}

STDMETHODIMP CSearchAssistantOC::PutProperty(VARIANT_BOOL bPerLocale, BSTR bstrName, BSTR bstrValue)
{
    HRESULT hr;

    if (IsTrustedSite())
    {
        HKEY hkey;
        LPCWSTR pwszKeyName;
        WCHAR wszKeyName[MAX_PATH];
        DWORD dwDisposition;

        if (bPerLocale)
        {
            GetPerLocalePath(wszKeyName, ARRAYSIZE(wszKeyName));
            pwszKeyName = wszKeyName;
        }
        else
        {
            pwszKeyName = c_wszSearchProps;
        }

        if (RegCreateKeyExW(HKEY_CURRENT_USER, pwszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, 
                           KEY_WRITE, NULL, &hkey, &dwDisposition) == ERROR_SUCCESS)
        {
            if ((NULL != bstrValue) && (bstrValue[0] != 0))
            {
                RegSetValueExW(hkey, bstrName, 0, REG_BINARY, (LPBYTE)bstrValue, 
                               SysStringByteLen(bstrValue));
            }
            else
            {
                //  Empty or NULL string means remove the property
                RegDeleteValue(hkey, bstrName);
            }
            RegCloseKey(hkey);
        }
        hr = S_OK;
    }
    else
    {
        hr = E_ACCESSDENIED;
    }
    return hr;
}

STDMETHODIMP CSearchAssistantOC::GetProperty(VARIANT_BOOL bPerLocale, BSTR bstrName, BSTR *pbstrValue)
{
    HRESULT hr;

    if (NULL != pbstrValue)
    {
        *pbstrValue = NULL;
        
        if (IsTrustedSite())
        {
            HKEY hkey;
            LPCWSTR pwszKeyName;
            WCHAR wszKeyName[MAX_PATH];

            if (bPerLocale)
            {
                GetPerLocalePath(wszKeyName, ARRAYSIZE(wszKeyName));
                pwszKeyName = wszKeyName;
            }
            else
            {
                pwszKeyName = c_wszSearchProps;
            }

            if (RegOpenKeyExW(HKEY_CURRENT_USER, pwszKeyName, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
            {
                DWORD dwType;
                DWORD cbSize;
                
                if ((RegQueryValueExW(hkey, bstrName, NULL, &dwType, NULL, &cbSize) == ERROR_SUCCESS) &&
                    (dwType == REG_BINARY))
                {
                    BSTR bstrValue = SysAllocStringByteLen(NULL, cbSize);

                    if (NULL != bstrValue)
                    {
                        if (RegQueryValueExW(hkey, bstrName, NULL, &dwType, (LPBYTE)bstrValue, &cbSize) == ERROR_SUCCESS)
                        {
                            *pbstrValue = bstrValue;
                        }
                        else
                        {
                            SysFreeString(bstrValue);
                        }
                    }
                }
                
                RegCloseKey(hkey);
            }
            hr = S_OK;
        }
        else
        {
            hr = E_ACCESSDENIED;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

STDMETHODIMP CSearchAssistantOC::put_EventHandled(VARIANT_BOOL bHandled)
{
    HRESULT hr;
    
    if (IsTrustedSite())
    {
        m_bEventHandled = bHandled;
        hr = S_OK;
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}

STDMETHODIMP CSearchAssistantOC::GetSearchAssistantURL(VARIANT_BOOL bSubstitute, VARIANT_BOOL bCustomize, BSTR *pbstrValue)
{
    HRESULT hr;
    
    if (IsTrustedSite())
    {
        if (NULL != pbstrValue)
        {
            WCHAR wszUrl[INTERNET_MAX_URL_LENGTH];

            if (GetSearchAssistantUrlW(wszUrl, ARRAYSIZE(wszUrl), bSubstitute, bCustomize))
            {
                *pbstrValue = SysAllocString(wszUrl);
            }
            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}

STDMETHODIMP CSearchAssistantOC::NotifySearchSettingsChanged()
{
    HRESULT hr;
    
    if (IsTrustedSite())
    {
        SendShellIEBroadcastMessage(WM_WININICHANGE, 0, (LPARAM)SEARCH_SETTINGS_CHANGED, 3000);
        hr = S_OK;
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}

STDMETHODIMP CSearchAssistantOC::put_ASProvider(BSTR Provider)
{
    HRESULT hr;
    
    if (IsTrustedSite())
    {
        if (Provider)
        {
            DWORD dwRet = SHSetValueW(HKEY_CURRENT_USER, REG_SZ_IE_SEARCURL, REG_SZ_PROVIDER, REG_SZ,
                                      Provider, (lstrlenW(Provider) + 1) * sizeof(WCHAR));

            ASSERT(ERROR_SUCCESS == dwRet);
        }
        hr = S_OK;
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}

STDMETHODIMP CSearchAssistantOC::get_ASProvider(BSTR *pProvider)
{
    HRESULT hr;
    
    if (IsTrustedSite())
    {
        if (NULL != pProvider)
        {
            HKEY hkey;

            if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_CURRENT_USER, REG_SZ_IE_SEARCURL, 0, KEY_READ, &hkey))
            {
                DWORD dwType;
                DWORD dwSize;
                
                if ((ERROR_SUCCESS == RegQueryValueExW(hkey, REG_SZ_PROVIDER, NULL, 
                                                       &dwType, NULL, &dwSize)) && 
                                                       (REG_SZ == dwType))
                {
                    *pProvider = SysAllocStringByteLen(NULL, dwSize);
                    if (NULL != *pProvider)
                    {
                        if (ERROR_SUCCESS != RegQueryValueExW(hkey, REG_SZ_PROVIDER, NULL, 
                                                              &dwType, (LPBYTE)*pProvider, &dwSize))
                        {
                            *pProvider = 0;
                        }
                    }
                }
                RegCloseKey(hkey);
            }
            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}

STDMETHODIMP CSearchAssistantOC::put_ASSetting(int Setting)
{
    HRESULT hr;
    
    if (IsTrustedSite())
    {
        DWORD dwRet = SHSetValueW(HKEY_CURRENT_USER, REG_SZ_IE_MAIN, REG_SZ_AUTOSEARCH, REG_DWORD,
                                  &Setting, sizeof(DWORD));

        ASSERT(ERROR_SUCCESS == dwRet);
        hr = S_OK;
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}

STDMETHODIMP CSearchAssistantOC::get_ASSetting(int *pSetting)
{
    HRESULT hr;
    
    if (IsTrustedSite())
    {
        if (NULL != pSetting)
        {
            DWORD dwSize = sizeof(int);

            *pSetting = -1;
            
            DWORD dwRet = SHGetValueW(HKEY_CURRENT_USER, REG_SZ_IE_MAIN, REG_SZ_AUTOSEARCH, NULL,
                                      pSetting, &dwSize);

            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}

BOOL CSearchAssistantOC::IsTrustedSite()
{
    if (!m_bSafetyInited && m_spClientSite)
    {
        m_bSafetyInited = TRUE;

        IHTMLDocument2 *pHTMLDocument2;
      
        HRESULT hr = GetHTMLDoc2(m_spClientSite, &pHTMLDocument2);

        if (SUCCEEDED(hr))
        {
            ASSERT(NULL != pHTMLDocument2);

            IHTMLLocation *pHTMLLocation;

            hr = pHTMLDocument2->get_location(&pHTMLLocation);

            if (SUCCEEDED(hr) && (NULL != pHTMLLocation))
            {           
                BSTR bstrUrl;

                pHTMLLocation->get_href(&bstrUrl);

                if (SUCCEEDED(hr) && (NULL != bstrUrl))
                {
                    HKEY hkey;
                    
                    //  BUGBUG (tnoonan)
                    //  This code is duped with CSearchBand::_IsSafeUrl in browseui
                    
                    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\SafeSites", 0, KEY_READ, &hkey) == ERROR_SUCCESS)
                    {        
                        WCHAR wszValue[MAX_PATH];
                        WCHAR wszData[MAX_URL_STRING];
                        WCHAR wszExpandedUrl[MAX_URL_STRING];
                        DWORD cbData = SIZEOF(wszData);
                        DWORD cchValue = ARRAYSIZE(wszValue);

                        for (int i=0; RegEnumValueW(hkey, i, wszValue, &cchValue, NULL, NULL, (LPBYTE)wszData, &cbData) == ERROR_SUCCESS; i++)
                        {
                            if (SHExpandEnvironmentStringsW(wszData, wszExpandedUrl, ARRAYSIZE(wszExpandedUrl)) > 0)
                            {
                                cchValue = ARRAYSIZE(wszExpandedUrl);
                                if (SUCCEEDED(UrlCanonicalizeW(wszExpandedUrl, wszExpandedUrl, &cchValue, 0)))
                                {
                                    if (cchValue > 0)
                                    {
                                        BOOL bRet;
                                        if (wszExpandedUrl[cchValue-1] == L'*')
                                        {
                                            bRet = StrCmpNIW(bstrUrl, wszExpandedUrl, cchValue - 1) == 0;
                                        }
                                        else
                                        {
                                            bRet = StrCmpIW(bstrUrl, wszExpandedUrl) == 0;
                                        }

                                        m_bIsTrustedSite = bRet ? TRUE : FALSE;
                                        
                                        if (m_bIsTrustedSite)
                                            break;
                                    }
                                }
                                cbData = SIZEOF(wszData);
                                cchValue = ARRAYSIZE(wszValue);
                            }
                        }
                        RegCloseKey(hkey);
                    }

                    SysFreeString(bstrUrl);
                }
                
                pHTMLLocation->Release();
            }
            
            pHTMLDocument2->Release();
        }
    }

    return m_bIsTrustedSite;
}

HRESULT CSearchAssistantOC::UpdateRegistry(BOOL bRegister)
{
    //this control uses selfreg.inx, not the ATL registry goo
    return S_OK;
}

STDMETHODIMP CSearchAssistantOC::FindOnWeb()
{
    if (!IsTrustedSite() && m_punkSite==NULL)
        return E_ACCESSDENIED ;

    return ShowSearchBand( SRCID_SWebSearch ) ;
}

STDMETHODIMP CSearchAssistantOC::FindFilesOrFolders()
{
    if (!IsTrustedSite() && m_punkSite==NULL)
        return E_ACCESSDENIED ;

    return ShowSearchBand( SRCID_SFileSearch ) ;
}

STDMETHODIMP CSearchAssistantOC::FindComputer()
{
    if (!IsTrustedSite() && m_punkSite==NULL)
        return E_ACCESSDENIED ;

    return ShowSearchBand( SRCID_SFindComputer ) ;
}

STDMETHODIMP CSearchAssistantOC::FindPrinter()
{
    if (!IsTrustedSite() && m_punkSite==NULL)
        return E_ACCESSDENIED ;

    HRESULT hr = E_FAIL;
    IShellDispatch2* psd2;
    if( SUCCEEDED( (hr = CoCreateInstance( CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
                                           IID_IShellDispatch2, (void**)&psd2 )) ) )
    {
        hr = psd2->FindPrinter( NULL, NULL, NULL ) ;
        psd2->Release();
    }
    return hr ;
}

STDMETHODIMP CSearchAssistantOC::FindPeople()
{
    if (!IsTrustedSite() && m_punkSite==NULL)
        return E_ACCESSDENIED ;

    //  Odd-ball shellexecute...

    HRESULT      hr;
    LPITEMIDLIST pidl = NULL;

    if (SUCCEEDED( (hr = SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAM_FILES, &pidl )) ))
    {
        TCHAR szDir[MAX_PATH];
        if (SUCCEEDED( (hr = SHGetPathFromIDList(pidl, szDir)) ))
        {
            ULONG_PTR dwErr = (ULONG_PTR) ShellExecute(HWND_DESKTOP, TEXT("open"), TEXT("wab.exe"), 
                                                       TEXT("/find"), szDir, SW_SHOWNORMAL);
            if (dwErr <= 32)
            {
                switch (dwErr)
                {
                    case ERROR_FILE_NOT_FOUND:
                    case ERROR_PATH_NOT_FOUND:
                    case ERROR_BAD_FORMAT:
                        hr = HRESULT_FROM_WIN32(dwErr) ;
                        break ;

                    case SE_ERR_ASSOCINCOMPLETE:
                    case SE_ERR_NOASSOC:
                        hr = HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION) ;
                        break ;

                    case SE_ERR_DLLNOTFOUND:
                        hr = HRESULT_FROM_WIN32(ERROR_DLL_NOT_FOUND) ;
                        break ;

                    case SE_ERR_ACCESSDENIED:
                        hr = E_ACCESSDENIED ;
                        break ;

                    case SE_ERR_OOM:
                        hr = E_OUTOFMEMORY ;
                        break ;

                    case SE_ERR_SHARE:
                        hr = HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);
                        break ;

                    case SE_ERR_DDEBUSY:
                    case SE_ERR_DDEFAIL:
                    case SE_ERR_DDETIMEOUT:
                        hr = HRESULT_FROM_WIN32(ERROR_DDE_FAIL) ;
                        break ;
                }
            }
            else
                hr = S_OK ;
        }
        ILFree(pidl) ;
    }
    return hr ;
}

// Wininet helper method to retry autodetection

// check to make sure that the hosting page is on
// the local computer.
// stolen from the zones code by joshco
//
STDMETHODIMP CSearchAssistantOC::LocalZoneCheck( )
{
    HRESULT hr = E_ACCESSDENIED;

    //  Return S_FALSE if we don't have a host site since we have no way of doing a 
    //  security check.  This is as far as VB 5.0 apps get.
    if (!m_spClientSite)
        return S_FALSE;

    //  1)  Get an IHTMLDocument2 pointer
    //  2)  Get URL from doc
    //  3)  Create security manager
    //  4)  Check if doc URL zone is local, if so everything's S_OK
    //  5)  Otherwise, get and compare doc URL SID to requested URL SID

    IHTMLDocument2 *pHtmlDoc;
    if (SUCCEEDED(GetHTMLDoc2(m_spClientSite, &pHtmlDoc)))
    {
        ASSERT(pHtmlDoc);
        BSTR bstrDocUrl;
        if (SUCCEEDED(pHtmlDoc->get_URL(&bstrDocUrl)))
        {
            ASSERT(bstrDocUrl);
            IInternetSecurityManager *pSecMgr;

            if (SUCCEEDED(CoCreateInstance(CLSID_InternetSecurityManager, 
                                           NULL, 
                                           CLSCTX_INPROC_SERVER,
                                           IID_IInternetSecurityManager, 
                                           (void **)&pSecMgr)))
            {
                ASSERT(pSecMgr);
                DWORD dwZoneID = URLZONE_UNTRUSTED;
                if (SUCCEEDED(pSecMgr->MapUrlToZone(bstrDocUrl, &dwZoneID, 0)))
                {
                    if (dwZoneID == URLZONE_LOCAL_MACHINE)
                        hr = S_OK;
                }
               
                pSecMgr->Release();
            }
            SysFreeString(bstrDocUrl);
        }
        pHtmlDoc->Release();
    }
    else
    {
        //  If we don't have an IHTMLDocument2 we aren't running in a browser that supports
        //  our OM.  We shouldn't block in this case since we could potentially
        //  get here from other hosts (VB, WHS, etc.).
        hr = S_FALSE;
    }

    return hr;
}

// set flags so that the next navigate will cause 
// a proxy autodetection cycle
// used in dnserr.htm along with location.reload.
// added by joshco
//
STDMETHODIMP CSearchAssistantOC::NETDetectNextNavigate()
{
    HRESULT hr = S_FALSE;

 CHAR  szConnectionName[100];
 DWORD dwBufLen;
 DWORD dwFlags;
 BOOL fResult;

 if (  LocalZoneCheck() != S_OK ) {
     // some security problem.. time to bail.
    hr=E_ACCESSDENIED;
    goto error;
    }

 dwBufLen = sizeof(szConnectionName);

       // find the connection name via internetconnected state
       
 fResult = InternetGetConnectedStateExA(&dwFlags,  szConnectionName,dwBufLen, 0 );

 INTERNET_PER_CONN_OPTION_LISTA list;
 INTERNET_PER_CONN_OPTIONA option;
       
 list.dwSize = sizeof(INTERNET_PER_CONN_OPTION_LISTA);
 if(dwFlags & INTERNET_CONNECTION_LAN)
    {
        list.pszConnection = NULL;
    }
    else
    {
        list.pszConnection =  szConnectionName;
    }

 list.dwOptionCount = 1;
 list.pOptions = &option;
 option.dwOption = INTERNET_PER_CONN_FLAGS;
 dwBufLen= sizeof(list);

   // now call internetsetoption to do it..
   // first set this connectoid to enable autodetect
 if ( ! InternetQueryOptionA(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION ,
         &list,&dwBufLen) ) 
    {
           goto error;
    }
               
 option.Value.dwValue |= PROXY_TYPE_AUTO_DETECT ;

 if ( ! InternetSetOptionA(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION ,
        &list,sizeof(list))) 
   {
         goto error;
   }

 if ( ! InternetInitializeAutoProxyDll(0) ) {
         goto error;
   }

 //  Now set the autodetect flags for this connectoid to
 //  do a passive detect and shut itself off if it doesnt work
 option.dwOption = INTERNET_PER_CONN_AUTODISCOVERY_FLAGS;
 
 if ( ! InternetQueryOptionA(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION ,
         &list,&dwBufLen) ) 
    {
           goto error;
    }
               
 option.Value.dwValue &= ~(AUTO_PROXY_FLAG_DETECTION_RUN) ;

 if ( ! InternetSetOptionA(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION ,
        &list,sizeof(list))) 
   {
         goto error;
   }


 
 if ( ! InternetSetOptionA(NULL, INTERNET_OPTION_SETTINGS_CHANGED,NULL, 0) ) {
         goto error;
   }



 hr=S_OK;
 error: ;
     
 return hr;
}

STDMETHODIMP CSearchAssistantOC::PutFindText(BSTR FindText)
{
    HRESULT hr;
    
    if (IsTrustedSite())
    {
        IServiceProvider *pServiceProvider;
        
        hr = IUnknown_QueryService(m_pSearchBandTBHelper, 
                                   SID_SProxyBrowser, 
                                   IID_IServiceProvider, 
                                   (void **)&pServiceProvider);
        if (SUCCEEDED(hr))
        {
            IWebBrowser2 *pWebBrowser2;
            hr = pServiceProvider->QueryService(SID_SWebBrowserApp, 
                                                IID_IWebBrowser2, 
                                                (void **)&pWebBrowser2);
            if (SUCCEEDED(hr))
            {
                ::PutFindText(pWebBrowser2, FindText);
                pWebBrowser2->Release();
            }
            pServiceProvider->Release();
        }
        hr = S_OK;
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}

STDMETHODIMP CSearchAssistantOC::get_Version(int *pVersion)
{
    if (NULL != pVersion)
    {
        *pVersion = SAOC_VERSION;
    }
    return S_OK;
}

// x_hex_digit and URLEncode were stolen from trident

inline int x_hex_digit(int c)
{
    if (c >= 0 && c <= 9)
    {
        return c + '0';
    }
    if (c >= 10 && c <= 15)
    {
        return c - 10 + 'A';
    }
    return '0';
}

/*
   The following array was copied directly from NCSA Mosaic 2.2
 */
static const unsigned char isAcceptable[96] =
/*   0 1 2 3 4 5 6 7 8 9 A B C D E F */
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0,    /* 2x   !"#$%&'()*+,-./  */
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,    /* 3x  0123456789:;<=>?  */
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 4x  @ABCDEFGHIJKLMNO  */
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,    /* 5x  PQRSTUVWXYZ[\]^_  */
 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 6x  `abcdefghijklmno  */
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};   /* 7x  pqrstuvwxyz{\}~  
DEL */

// Performs URL-encoding of null-terminated strings. Pass NULL in pbOut
// to find buffer length required. Note that '\0' is not written out.

int URLEncode(char * pbOut, const char * pchIn)
{
    int     lenOut = 0;
    char *  pchOut = pbOut;

    ASSERT(pchIn);

    for (; *pchIn; pchIn++, lenOut++)
    {
        if (*pchIn == ' ')
        {
            if (pchOut)
                *pchOut++ = '+';
        }
        else if (*pchIn >= 32 && *pchIn <= 127 && isAcceptable[*pchIn - 32])
        {
            if (pchOut)
                *pchOut++ = *pchIn;
        }
        else
        {
            if (pchOut)
                *pchOut++ = '%';
            lenOut++;

            if (pchOut)
                *pchOut++ = (char)x_hex_digit((*pchIn >> 4) & 0xf);
            lenOut++;

            if (pchOut)
                *pchOut++ = (char)x_hex_digit(*pchIn & 0xf);
        }
    }
    return lenOut;
}

STDMETHODIMP CSearchAssistantOC::EncodeString(BSTR bstrValue, BSTR bstrCharSet, VARIANT_BOOL bUseUTF8, BSTR *pbstrResult)
{

    if ((NULL != bstrValue) && (NULL != pbstrResult))
    {
        HRESULT hr;
        IMultiLanguage2 *pMultiLanguage2;

        *pbstrResult = NULL;
        
        hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                              IID_IMultiLanguage2, (void**)&pMultiLanguage2);

        if (SUCCEEDED(hr))
        {
            UINT uiCodePage = CP_BOGUS;
            UINT cchVal = SysStringLen(bstrValue);
            DWORD dwMode = 0;

            if (!bUseUTF8)
            {
                //  We're not using UTF so try and get the code page from the 
                //  charset.
                
                MIMECSETINFO info;
                
                hr = pMultiLanguage2->GetCharsetInfo(bstrCharSet, &info);

                if (SUCCEEDED(hr))
                {                   
                    hr = pMultiLanguage2->ConvertStringFromUnicodeEx(&dwMode,
                                                                     info.uiCodePage,
                                                                     bstrValue,
                                                                     &cchVal,
                                                                     NULL,
                                                                     NULL,
                                                                     MLCONVCHARF_NOBESTFITCHARS,
                                                                     NULL);
                    if (S_OK == hr)
                    {
                        uiCodePage = info.uiCodePage;
                    }
                }
            }
            else
            {
                uiCodePage = CP_UTF_8;
            }

            if (uiCodePage == CP_BOGUS)
            {
                //  Crap, we have characters which don't work in the charset or the charset
                //  is unknown to MLang, maybe MLang can figure out a code page to use.
                
                IMLangCodePages *pMLangCodePages;

                //  When all else fails...
                uiCodePage = CP_ACP;

                hr = pMultiLanguage2->QueryInterface(IID_IMLangCodePages,
                                                     (void **)&pMLangCodePages);
                if (SUCCEEDED(hr))
                {
                    DWORD dwCodePages = 0;
                    long cchProcessed = 0;
                    UINT uiTmpCP = 0;
                    
                    if (SUCCEEDED(pMLangCodePages->GetStrCodePages(bstrValue, cchVal, 
                                                                   0, &dwCodePages,
                                                                   &cchProcessed)) 

                        &&

                        SUCCEEDED(pMLangCodePages->CodePagesToCodePage(dwCodePages,
                                                                       0,
                                                                       &uiTmpCP)))
                    {
                        uiCodePage = uiTmpCP;
                    }

                    pMLangCodePages->Release();
                }
            }

            dwMode = 0;

            UINT cbVal = 0;

            //  Ask MLang how big of a buffer we need
            hr = pMultiLanguage2->ConvertStringFromUnicode(&dwMode,
                                                           uiCodePage,
                                                           bstrValue,
                                                           &cchVal,
                                                           NULL,
                                                           &cbVal);

            if (SUCCEEDED(hr))
            {
                CHAR *pszValue = new CHAR[cbVal + 1];

                if (NULL != pszValue)
                {
                    //  Really convert the string
                    hr = pMultiLanguage2->ConvertStringFromUnicode(&dwMode,
                                                                   uiCodePage,
                                                                   bstrValue,
                                                                   &cchVal,
                                                                   pszValue,
                                                                   &cbVal);
                    if (SUCCEEDED(hr))
                    {
                        pszValue[cbVal] = 0;
                        
                        int cbEncVal = URLEncode(NULL, pszValue);
                        CHAR *pszEncVal = new CHAR[cbEncVal];

                        if (NULL != pszEncVal)
                        {
                            URLEncode(pszEncVal, pszValue);

                            int cchResult = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                                                pszEncVal, cbEncVal,
                                                                NULL, 0);

                            *pbstrResult = SysAllocStringLen(NULL, cchResult);

                            if (NULL != *pbstrResult)
                            {
                                MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                                    pszEncVal, cbEncVal,
                                                    *pbstrResult, cchResult);
                            }

                            delete [] pszEncVal;
                        }
                    }
                    delete [] pszValue;
                }
            }
            pMultiLanguage2->Release();
        }
    }


    return S_OK;
}

STDMETHODIMP CSearchAssistantOC::get_ShowFindPrinter(VARIANT_BOOL *pbShowFindPrinter)
{
    HRESULT hr;

    if (IsTrustedSite())
    {
        if (NULL != pbShowFindPrinter)
        {
            IShellDispatch2* psd;

            *pbShowFindPrinter = VARIANT_FALSE;

            if (SUCCEEDED(CoCreateInstance(CLSID_Shell, 0, CLSCTX_INPROC_SERVER, 
                                          IID_IShellDispatch2, (void**)&psd)))
            {
                BSTR bstrName = SysAllocString( L"DirectoryServiceAvailable");

                if (bstrName)
                {
                    VARIANT varRet = {0};
                    
                    if (SUCCEEDED(psd->GetSystemInformation(bstrName, &varRet)))
                    {
                        ASSERT(VT_BOOL == varRet.vt);
                        *pbShowFindPrinter = varRet.boolVal;
                    }
                    SysFreeString(bstrName);
                }
                psd->Release();
            }
            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}


#ifdef ENABLE_THIS_FOR_IE5X

STDMETHODIMP CSearchAssistantOC::RefreshLocation(IDispatch *pLocation)
{
    HRESULT hr;
    
    if (IsTrustedSite())
    {
        if (NULL != pLocation)
        {
            IHTMLLocation *pHTMLLocation;

            IUnknown_QueryService(pLocation, IID_IHTMLLocation, IID_IHTMLLocation, (void **)&pHTMLLocation);

            if (pHTMLLocation)
            {
                pHTMLLocation->reload(VARIANT_TRUE);
                pHTMLLocation->Release();
            }
        }
        hr = S_OK;
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    return hr;

}

#endif
//-------------------------------------------------------------------------//
#define REG_SZ_SHELL_SEARCH TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\") \
                            TEXT("Explorer\\FindExtensions\\Static\\ShellSearch")
//-------------------------------------------------------------------------//
HRESULT GetSearchURLs( 
    IN REFGUID guidSearch, 
    OUT LPTSTR pszUrl, 
    IN DWORD cch, 
    OUT OPTIONAL LPTSTR pszUrlNavNew, 
    OUT DWORD cchNavNew, 
    OUT BOOL *pfRunInProcess )
{
    HRESULT hr = E_FAIL ;
    DWORD   cb ;
    DWORD   dwType ;
    DWORD   dwErr ;

    *pfRunInProcess = FALSE ;
    if( pszUrlNavNew && cchNavNew )
        *pszUrlNavNew = 0 ;

    if( IsEqualGUID( guidSearch, SRCID_SWebSearch ) )
    {
        if( GetDefaultInternetSearchUrlW( pszUrl, cch, TRUE ) )
            hr = S_OK ;
    }
    else
    {
        //  The shell search URL-sucking stuff was adapted from
        //  CShellSearchExt::_GetSearchUrls() in browseui\browband.cpp, 
        //  and should be kept in sync.

        TCHAR szSubKey[32];
        HKEY  hkey, hkeySub;
        if( (dwErr = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_SZ_SHELL_SEARCH, 
                                   0, KEY_READ, &hkey )) != ERROR_SUCCESS )
            return HRESULT_FROM_WIN32( dwErr ) ;

        hr = E_FAIL ;

        for( int i = 0; 
             wnsprintf( szSubKey, ARRAYSIZE(szSubKey), TEXT("%d"), i ), 
                RegOpenKey(hkey, szSubKey, &hkeySub) == ERROR_SUCCESS ;
             i++ )
        {
            TCHAR szSearchGuid[MAX_PATH];

            cb = SIZEOF(szSearchGuid);
            
            if( SHGetValue( hkeySub, TEXT("SearchGUID"), NULL, &dwType, (BYTE*)szSearchGuid, &cb ) == ERROR_SUCCESS )
            {
                GUID guid;
                SHCLSIDFromString(szSearchGuid, &guid);

                if( IsEqualGUID( guid, guidSearch ) )
                {
                    cb = cch * sizeof(TCHAR);
                    if( SHGetValue( hkeySub, TEXT("SearchGUID\\Url"), NULL, 
                                    &dwType, (BYTE*)pszUrl, &cb ) == ERROR_SUCCESS )
                    {
                        if( pszUrlNavNew && cchNavNew )
                        {
                            // See if there is a secondary URL that we should navigate to
                            cb = cchNavNew * sizeof(TCHAR);
                            SHGetValue(hkeySub, TEXT("SearchGUID\\UrlNavNew"), NULL, &dwType, (BYTE*)pszUrlNavNew, &cb);
                        }

                        // try to grab the RunInProcess flag
                        *pfRunInProcess = (BOOL)SHRegGetIntW( hkeySub, L"RunInProcess", 0 );

                        RegCloseKey(hkeySub);
                        hr = S_OK ;
                        break;
                    }
                }
            }
            RegCloseKey(hkeySub);
        }
        RegCloseKey( hkey ) ;
    }
    return hr ;
}

STDMETHODIMP _IsShellSearchBand( REFGUID guidSearch )
{
    if (IsEqualGUID( guidSearch, SRCID_SFileSearch ) ||
        IsEqualGUID( guidSearch, SRCID_SFindComputer ) ||
        IsEqualGUID( guidSearch, SRCID_SFindPrinter ) )
        return S_OK;
    return S_FALSE;
}

//-------------------------------------------------------------------------//
//  Establishes the correct shell search dialog, etc.
STDMETHODIMP _ShowShellSearchBand( IWebBrowser2* pwb2, REFGUID guidSearch )
{
    ASSERT( pwb2 );
    ASSERT( S_OK == _IsShellSearchBand( guidSearch ) );

    HRESULT hr;
    VARIANT varBand;
    if (SUCCEEDED( (hr = _BstrVariantFromGUID( CLSID_FileSearchBand, &varBand )) ))
    {
        //  Retrieve the FileSearchBand's unknown from the browser frame as a VT_UNKNOWN property;
        //  (FileSearchBand initialized and this when he was created and hosted.)
        VARIANT varFsb;
        VariantInit( &varFsb );    
        if (SUCCEEDED( (hr = pwb2->GetProperty( varBand.bstrVal, &varFsb )) ))
        {
            if (VT_UNKNOWN == varFsb.vt && varFsb.punkVal != NULL )
            {
                //  Retrieve the IFileSearchBand interface
                IFileSearchBand* pfsb;
                if (SUCCEEDED( (hr = varFsb.punkVal->QueryInterface( IID_IFileSearchBand, (LPVOID*)&pfsb )) ))
                {
                    //  Assign the correct search type to the band
                    VARIANT varSearchID;
                    if (SUCCEEDED( (hr = _BstrVariantFromGUID( guidSearch, &varSearchID )) ))
                    {
                        VARIANT      varNil;
                        VARIANT_BOOL bNavToResults = VARIANT_FALSE ; 
                            // Note [scotthan]: we only navigate to results when we create a 
                            // new frame for the search, which we never do from srchasst.
                        VariantInit( &varNil );
                        pfsb->SetSearchParameters( &varSearchID.bstrVal, bNavToResults, &varNil, &varNil );
                        VariantClear( &varSearchID );
                    }
                    pfsb->Release();
                }
            }
            VariantClear( &varFsb );
        }
        VariantClear( &varBand );
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  The goop to show a search band in the current browser frame.
//  6/1
HRESULT CSearchAssistantOC::ShowSearchBand( REFGUID guidSearch ) 
{
    HRESULT           hr = E_FAIL;
    TCHAR             szUrl[MAX_URL_STRING];
    TCHAR             szUrlNavNew[MAX_URL_STRING];
    CLSID             clsidBand;
    BOOL              fShellSearchBand = FALSE;
    BOOL              fRunInProcess = FALSE;
    IUnknown*         punkSite = m_punkSite ? m_punkSite : (IUnknown*)m_spClientSite;
    IServiceProvider* psp  = NULL;
    IWebBrowser2*     pwb2 = NULL;

    if( !punkSite )
        return E_UNEXPECTED ;

    //  Determine band class and whether the band supports navigation
    if( (fShellSearchBand = (S_OK == _IsShellSearchBand( guidSearch ))) )
    {
        if (SHRestricted(REST_NOFIND) && IsEqualGUID(guidSearch, SRCID_SFileSearch))
            return E_ACCESSDENIED;

        clsidBand = CLSID_FileSearchBand;
    }
    else
    {
        clsidBand = CLSID_SearchBand;
        //  we need to navigate to a search URL, grope the registry for that special URL
        if( FAILED( (hr= GetSearchURLs( guidSearch, szUrl, ARRAYSIZE(szUrl), 
                                        szUrlNavNew, ARRAYSIZE(szUrlNavNew), 
                                        &fRunInProcess )) ) )
            return hr;
    }
        
    //  BUGBUG [scotthan]: this function will fail unless invoked from within a browser.
    //  This sits fine for now since SearchAsst is designed as a browser band.
    hr = IUnknown_QueryService( punkSite, SID_STopLevelBrowser, IID_IServiceProvider, (LPVOID*)&psp );
    if( SUCCEEDED( hr ) )
    {
        hr = psp->QueryService( SID_SWebBrowserApp, IID_IWebBrowser2, (LPVOID*)&pwb2 );
        if( SUCCEEDED( hr ) )
        {
            ASSERT(pwb2);
            WCHAR    *pwszBand ;
            if( SUCCEEDED( (hr = StringFromCLSID( clsidBand, &pwszBand )) ) )
            {
                VARIANT  var;
                VARIANT  varNil = {0};
                var.bstrVal = SysAllocString( pwszBand ) ;
                var.vt = VT_BSTR ;
                CoTaskMemFree( pwszBand ) ;
                
                // show a search bar
                hr = pwb2->ShowBrowserBar(&var, &varNil, &varNil);
                VariantClear( &var );
                
                if( SUCCEEDED( hr ) )
                {
                    VARIANT varFlags;
                    if( fShellSearchBand )
                    {
                        hr= _ShowShellSearchBand( pwb2, guidSearch );
                    }
                    else
                    {
                        varFlags.vt = VT_I4;
                        varFlags.lVal = navBrowserBar;
                        var.bstrVal = SysAllocString( T2W( szUrl ) ) ;
                        var.vt = VT_BSTR ;

                        // navigate the search bar to the correct url
                        hr = pwb2->Navigate2(&var, &varFlags, &varNil, &varNil, &varNil);
                        VariantClear( &var );
                        VariantClear( &varFlags );
                            
                        if( SUCCEEDED( hr ) )
                        {
                            hr = pwb2->put_Visible( TRUE ) ;
                        }
                    }
                }
            }
            pwb2->Release();
        }
        psp->Release();
    }
    return hr;
}

STDMETHODIMP CSearchAssistantOC::SetSite( IUnknown* punkSite )
{
    ATOMICRELEASE(m_punkSite);
    if ((m_punkSite = punkSite) != NULL)
        m_punkSite->AddRef() ;
    return S_OK ;
}

STDMETHODIMP CSearchAssistantOC::GetSite( REFIID riid, void** ppvSite )
{
    if( !m_punkSite )
        return E_FAIL ;
    return m_punkSite->QueryInterface( riid, ppvSite ) ;
}
