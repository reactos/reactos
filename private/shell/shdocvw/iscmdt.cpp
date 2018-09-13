#include "priv.h"
#include "ishcut.h"
#include "assocurl.h"
#include "shlwapi.h"
#include "resource.h"
#include "shlguid.h"

STDMETHODIMP Intshcut::QueryStatus(
    const GUID *pguidCmdGroup,
    ULONG cCmds,
    MSOCMD rgCmds[],
    MSOCMDTEXT *pcmdtext
)
{
    return E_NOTIMPL;
}

struct SHORTCUT_ICON_PARAMS
{
    WCHAR *pwszFileName;
    WCHAR *pwszShortcutUrl;
    BSTR   bstrIconUrl;

    ~SHORTCUT_ICON_PARAMS()
    {
        if(pwszFileName)
            LocalFree(pwszFileName);

        if(bstrIconUrl)
            SysFreeString(bstrIconUrl);

        if(pwszShortcutUrl)
            SHFree(pwszShortcutUrl);
    }
};


const WCHAR wszDefaultShortcutIconName[] = ISHCUT_DEFAULT_FAVICONW;
const WCHAR wszDefaultShortcutIconNameAtRoot[] = ISHCUT_DEFAULT_FAVICONATROOTW;
extern const LARGE_INTEGER c_li0 ;

VOID
GetIconUrlFromLinkTag(
    IHTMLDocument2* pHTMLDocument,
    BSTR *pbstrIconUrl
)
{
    HRESULT hres;
    IHTMLLinkElement *pLink = NULL;
    hres = SearchForElementInHead(pHTMLDocument, OLESTR("REL"), OLESTR("SHORTCUT ICON"), IID_IHTMLLinkElement, (LPUNKNOWN *)&pLink);
    if(S_OK == hres)
    {
        hres = pLink->get_href(pbstrIconUrl);
        pLink->Release();
    }

}


BOOL SetIconForShortcut(
    WCHAR *pwszIconUrl,
    INamedPropertyBag *pNamedBag
)
{
 // Do it synchronously on this thread
    BOOL fRet = FALSE;
    WCHAR wszCacheFileName[MAX_PATH];
    HRESULT hr;

    ASSERT(pNamedBag);

    hr = URLDownloadToCacheFileW(NULL, pwszIconUrl, wszCacheFileName, sizeof(wszCacheFileName), NULL, NULL);
    if(S_OK == hr)
    {
        // 77657 security bug: we must not call LoadImage because the Win9x version can
        // crash with buffer overrun if given a corrupt icon.  ExtractIcon helps validate the file
        // to prevent that specific crash.

        HICON hIcon = ExtractIcon(g_hinst, wszCacheFileName, 0);

        if(hIcon) // It is really an Icon
        {
            // Make this icon sticky in cache
            SetUrlCacheEntryGroupW(pwszIconUrl, INTERNET_CACHE_GROUP_ADD,
                                            CACHEGROUP_ID_BUILTIN_STICKY, NULL, 0, NULL);


            DestroyIcon(hIcon);
            // get the file - set the icon and return
            fRet = TRUE; // We Got the icon file - even if we are unable set it
            // Store this url away in the shortcut file
            PROPSPEC rgpropspec[2];
            PROPVARIANT rgpropvar[2];
            PROPVARIANT var;
            SA_BSTR     bstr;


            StrCpyNW(bstr.wsz, pwszIconUrl, ARRAYSIZE(bstr.wsz));
            bstr.cb = lstrlenW(bstr.wsz) * sizeof(WCHAR);

            var.vt = VT_BSTR;
            var.bstrVal = bstr.wsz;


            hr = pNamedBag->WritePropertyNPB(ISHCUT_INISTRING_SECTIONW, ISHCUT_INISTRING_ICONFILEW,
                                                &var);


            if(S_OK == hr)
            {
                LPCWSTR pwszTemp1 = L"1";
                StrCpyNW(bstr.wsz, pwszTemp1, ARRAYSIZE(bstr.wsz));
                bstr.cb = (ARRAYSIZE(pwszTemp1) - 1)*sizeof(WCHAR);

                hr = pNamedBag->WritePropertyNPB(ISHCUT_INISTRING_SECTIONW, ISHCUT_INISTRING_ICONINDEXW,
                                                &var);

            }


            // Update the intsite database - whether or not the
            // shortcut file was updated. This is because we need to
            // ensure that the intsite db is updated even if the shortcut file name is not known

            IPropertySetStorage *ppropsetstg;
            IPropertyStorage *ppropstg;

            rgpropspec[0].ulKind = PRSPEC_PROPID;
            rgpropspec[0].propid = PID_INTSITE_ICONINDEX;
            rgpropspec[1].ulKind = PRSPEC_PROPID;
            rgpropspec[1].propid = PID_INTSITE_ICONFILE;




            rgpropvar[0].vt = VT_I4;
            rgpropvar[0].lVal = 1;
            rgpropvar[1].vt = VT_LPWSTR;
            rgpropvar[1].pwszVal = pwszIconUrl;



            hr = pNamedBag->QueryInterface(IID_IPropertySetStorage,(LPVOID *)&ppropsetstg);


            if(SUCCEEDED(hr))
            {
                hr = ppropsetstg->Open(FMTID_InternetSite, STGM_READWRITE, &ppropstg);
                ppropsetstg->Release();
            }

            if(SUCCEEDED(hr))
            {
                hr = ppropstg->WriteMultiple(2, rgpropspec, rgpropvar, 0);
                ppropstg->Commit(STGC_DEFAULT);
                ppropstg->Release();
            }
       }
    }

    return fRet;
}

HRESULT PreUpdateShortcutIcon(IUniformResourceLocatorW *purlW, LPTSTR pszHashItem, int* piIndex,
                              UINT* puFlags, int* piImageIndex, LPWSTR *ppwszURL)
{
    ASSERT(pszHashItem);
    ASSERT(piIndex);
    ASSERT(puFlags);
    ASSERT(piImageIndex);
    IExtractIconA *pEIA = NULL;
    HRESULT hr;


    ASSERT(purlW);

    if(purlW)
    {
        hr = purlW->GetURL(ppwszURL);

        if(S_OK == hr)
        {
            hr = GetGenericURLIcon(pszHashItem, MAX_PATH, piIndex);

            if (SUCCEEDED(hr))
            {
                SHFILEINFO fi = {0};

                if (SHGetFileInfo(pszHashItem, 0, &fi, sizeof(SHFILEINFO),
                                  SHGFI_SYSICONINDEX))
                {
                    *piImageIndex = fi.iIcon;
                }
                else
                {
                    *piImageIndex = -1;
                }
            }
        }
    }

    return hr;
}


DWORD
DownloadAndSetIconForShortCutThreadProc(
    LPVOID pIn
)
{
    HINSTANCE hShdocvw = LoadLibrary(TEXT("shdocvw.dll"));
    SHORTCUT_ICON_PARAMS *pParams = (SHORTCUT_ICON_PARAMS *)pIn;
    WCHAR *pwszShortcutFilePath = pParams->pwszFileName;
    WCHAR *pwszIconUrl = pParams->bstrIconUrl;
    WCHAR wszFullUrl[MAX_URL_STRING];
    LPWSTR pwszBaseUrl = NULL;
    DWORD cchFullUrlSize = ARRAYSIZE(wszFullUrl);
    TCHAR  szHash[MAX_PATH];
    IPersistFile *   ppf = NULL;
    BOOL fRet = FALSE;
    INT iImageIndex;
    INT iIconIndex;
    UINT uFlags = 0;
    HRESULT hr;
    IUniformResourceLocatorW *purlW = NULL;
    HRESULT hresCoInit = E_FAIL;

    hresCoInit = CoInitialize(NULL);
    ASSERT(hShdocvw);
    hr = CoCreateInstance(CLSID_InternetShortcut, NULL,
                CLSCTX_INPROC_SERVER,
                IID_IUniformResourceLocatorW, (LPVOID *)&purlW);

    ASSERT(purlW);
    DbgAddToMemList(pIn);
    if((S_OK == hr) && purlW)
    {

        if(S_OK == hr)
        {
            if(pwszShortcutFilePath)
            {
                hr = purlW->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
                if(S_OK == hr)
                {
                    ASSERT(ppf);
                    hr = ppf->Load(pwszShortcutFilePath, STGM_READWRITE);
                }
            }
            else if(pParams->pwszShortcutUrl)
            {
                // Use the URL to init the shortcut
                hr = purlW->SetURL(pParams->pwszShortcutUrl, IURL_SETURL_FL_GUESS_PROTOCOL);
            }
            else
            {
                hr = E_FAIL;
                // Can't create an object and init it
            }
        }
    }



    if((S_OK == hr) && (purlW))
    {
        hr = PreUpdateShortcutIcon(purlW, szHash, &iIconIndex, &uFlags, &iImageIndex, (LPWSTR *)&pwszBaseUrl);

        INamedPropertyBag   *pNamedBag = NULL;
        hr = purlW->QueryInterface(IID_INamedPropertyBag,(LPVOID *)&pNamedBag);
        if((S_OK == hr) && (pNamedBag))
        {
            if(pwszIconUrl)
            {
                WCHAR *pwszIconFullUrl;
                if(pwszBaseUrl)
                {
                    hr = UrlCombineW(pwszBaseUrl, pwszIconUrl, wszFullUrl, &cchFullUrlSize, 0);
                    ASSERT(S_OK == hr);
                    if(SUCCEEDED(hr))
                    {
                        pwszIconFullUrl = wszFullUrl;
                    }
                 }
                 else
                 {
                    pwszIconFullUrl = pwszIconUrl; // try it as it is
                 }
                 fRet = SetIconForShortcut( pwszIconFullUrl, pNamedBag);

            }

            if((FALSE == fRet) && (pwszBaseUrl))
            {
                 
                hr = UrlCombineW(pwszBaseUrl, wszDefaultShortcutIconNameAtRoot, wszFullUrl, &cchFullUrlSize, 0);
                fRet = SetIconForShortcut(wszFullUrl, pNamedBag);
            }

            pNamedBag->Release();
        }
    }



    if(fRet)
    {
        _SHUpdateImage(PathFindFileName(szHash), iIconIndex, uFlags, iImageIndex);
    }

    if(ppf)
    {
        ppf->Save(NULL, FALSE); // Save off Icon related stuff
        ppf->Release();
    }

    if(purlW)
        purlW->Release();

    if(pParams)
        delete pParams;

    if(pwszBaseUrl)
        SHFree(pwszBaseUrl);

    if(SUCCEEDED(hresCoInit))
        CoUninitialize();


    //FreeLibraryAndExitThread(hShdocvw); -- Need a FreeLibraryAndExitThread for thread pools
    return fRet;
}






STDMETHODIMP Intshcut::_DoIconDownload()
{
    SHORTCUT_ICON_PARAMS *pIconParams;
    BOOL fThreadStarted = FALSE;
    HRESULT hr = S_OK;


    pIconParams = new SHORTCUT_ICON_PARAMS;
    if(pIconParams)
    {
        if(_punkSite)
        {
            IServiceProvider *psp;
            hr = _punkSite->QueryInterface(IID_IServiceProvider, (LPVOID *)&psp);

            if(SUCCEEDED(hr))
            {
                IWebBrowser2 *pwb=NULL;

                hr = psp->QueryService(SID_SHlinkFrame, IID_IWebBrowser2, (LPVOID *)&pwb);
                if(SUCCEEDED(hr))
                {
                    IDispatch *pdisp = NULL;
                    ASSERT(pwb);
                    hr = pwb->get_Document(&pdisp);
                    if(pdisp)
                    {
                        IHTMLDocument2 *pHTMLDocument;
                        ASSERT(SUCCEEDED(hr));
                        hr = pdisp->QueryInterface(IID_IHTMLDocument2, (void **)(&pHTMLDocument));
                        if(SUCCEEDED(hr))
                        {
                            ASSERT(pHTMLDocument);
                            GetIconUrlFromLinkTag(pHTMLDocument, &(pIconParams->bstrIconUrl));
                            pHTMLDocument->Release();
                        }
                        pdisp->Release();
                    }
                    pwb->Release();
                }
                psp->Release();
            }

        }


        if(m_pszFile)
        {
            pIconParams->pwszFileName = StrDupW(m_pszFile);

        }

        // Now fill in the URL of the shortcut
        hr = GetURLW(&(pIconParams->pwszShortcutUrl));

        ASSERT(SUCCEEDED(hr));
        if(S_OK == hr)
        {
            fThreadStarted = SHQueueUserWorkItem(DownloadAndSetIconForShortCutThreadProc,
                                                 (LPVOID)(pIconParams),
                                                 0,
                                                 (DWORD_PTR)NULL,
                                                 (DWORD_PTR *)NULL,
                                                 "shdocvw.dll",
                                                 TPS_LONGEXECTIME
                                                 );
            DbgRemoveFromMemList((LPVOID)(pIconParams)); // We do not free this guy
        }


    }

    if(FALSE == fThreadStarted)
    {
        if(pIconParams)
        {
            delete pIconParams;
        }
    }

    return fThreadStarted ? S_OK : E_FAIL;
}



STDMETHODIMP Intshcut::Exec(
    const GUID *pguidCmdGroup,
    DWORD nCmdID,
    DWORD nCmdexecopt,
    VARIANTARG *pvarargIn,
    VARIANTARG *pvarargOut
)
{

    HRESULT hres = S_OK;

    if (pguidCmdGroup && IsEqualGUID(CGID_ShortCut, *pguidCmdGroup))
    {
        switch(nCmdID)
        {
            case ISHCUTCMDID_DOWNLOADICON:
            {
                DWORD dwFlags = 0;
                BOOL fFetch = TRUE;
                WCHAR *pwszUrl;
                // Don't do it for FTP shortcuts

                if(SUCCEEDED(GetURLW(&pwszUrl))) 
                {
                    if((URL_SCHEME_FTP == GetUrlSchemeW(pwszUrl)))
                        fFetch = FALSE;
                    SHFree(pwszUrl);
                }
                
                if(fFetch && (InternetGetConnectedState(&dwFlags, 0)))
                    hres = _DoIconDownload();
            }
                break;

            default:
                break;

        }
    }
    return hres;
}
