#include "private.h"
#include "subsmgrp.h"
#include "offl_cpp.h"
#include "helper.h"
#include "propshts.h"

#include <mluisupp.h>

#undef TF_THISMODULE
#define TF_THISMODULE TF_WEBCHECKCORE

extern HRESULT LoadSubscription(LPCTSTR url, LPMYPIDL *);
extern TCHAR szInternetSettings[];

extern void PropagateGeneralProp(HWND, POOEBuf);
extern INT_PTR CALLBACK EnableScreenSaverDlgProc(HWND, UINT, WPARAM, LPARAM);
extern HRESULT CreateSubscriptionFromOOEBuf(POOEBuf, LPMYPIDL *);

extern BOOL CALLBACK _AddOnePropSheetPage(HPROPSHEETPAGE, LPARAM);

extern TCHAR g_szDontAskScreenSaver[];    // from WIZARDS.CPP

#define MAX_STR_LENGTH 200

extern DWORD  aHelpIDs[];
extern TCHAR  c_szHelpFile[];

typedef struct
{
    CSubscriptionMgr* pMgr;
    LPCWSTR pwszName;
    LPCWSTR pwszUrl;
    SUBSCRIPTIONINFO* pSubsInfo;
    SUBSCRIPTIONTYPE subsType;
    DWORD dwFlags;
} SUBSCRIBE_ADI_INFO;

static const TCHAR SUBSCRIBEADIPROP[] = TEXT("SADIP");

INT_PTR CALLBACK SummarizeDesktopSubscriptionDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

////////////////////////////////////////////////////////////////////////////////
// Private imports from shdocvw (AddToFavorites API)
//
STDAPI SHAddSubscribeFavorite (HWND hwnd, LPCWSTR pwszURL, LPCWSTR pwszName, DWORD dwFlags,
                               SUBSCRIPTIONTYPE subsType, SUBSCRIPTIONINFO* pInfo);
////////////////////////////////////////////////////////////////////////////////


void UpdateSubsInfoFromOOE (SUBSCRIPTIONINFO* pInfo, POOEBuf pooe);

HRESULT CreateBSTRFromTSTR(BSTR * pBstr, LPCTSTR sz)
{
    int i = lstrlen(sz) + 1;
    *pBstr = SysAllocStringLen(NULL, i);
    if(NULL == *pBstr)
        return E_OUTOFMEMORY;

    MyStrToOleStrN(*pBstr, i, sz);
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
//                        Subsctiption Manager
//
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// constructor / destructor
//
CSubscriptionMgr::CSubscriptionMgr(void)
{
    ASSERT(NULL == _pidl);

    m_cRef = 1;
    m_eInitSrc = _INIT_FROM_URL;    //  Default.
    m_oldType = SUBSTYPE_URL;       //  Default.
    DllAddRef();
}

CSubscriptionMgr::~CSubscriptionMgr()
{
    if (_pidl)
    {
        COfflineFolderEnum::FreePidl(_pidl);
        _pidl = NULL;
    }
    SAFELOCALFREE(m_pBuf);
    SAFERELEASE(m_pUIHelper);
    DllRelease();
}

//
// IUnknown members
//
STDMETHODIMP CSubscriptionMgr::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv=NULL;

    // Validate requested interface
    if ((IID_IUnknown == riid) ||
        (IID_ISubscriptionMgr == riid) ||
        (IID_ISubscriptionMgr2 == riid))
    {
        *ppv=(ISubscriptionMgr2 *)this;
    } 
    else if(IID_ISubscriptionMgrPriv == riid)
    {
        *ppv=(ISubscriptionMgrPriv *)this;
    }
    else if(IID_IShellExtInit == riid)
    {
        *ppv=(IShellExtInit *)this;
    }
    else if(IID_IShellPropSheetExt == riid)
    {
        *ppv=(IShellPropSheetExt *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    ((LPUNKNOWN)*ppv)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CSubscriptionMgr::AddRef(void)
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CSubscriptionMgr::Release(void)
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}


HRESULT CSubscriptionMgr::RemovePages(HWND hDlg)
{
    HRESULT hr;

    ASSERT(NULL != m_pUIHelper);

    if (NULL == m_pUIHelper)
    {
        return E_UNEXPECTED;
    }

    ISubscriptionAgentShellExt *psase;

    hr = m_pUIHelper->QueryInterface(IID_ISubscriptionAgentShellExt, (void **)&psase);

    if (SUCCEEDED(hr))
    {
        hr = psase->RemovePages(hDlg);
        psase->Release();
    }

    return hr;
}

HRESULT CSubscriptionMgr::SaveSubscription()
{
    HRESULT hr;

    ASSERT(NULL != m_pUIHelper);

    if (NULL == m_pUIHelper)
    {
        return E_UNEXPECTED;
    }

    ISubscriptionAgentShellExt *psase;

    hr = m_pUIHelper->QueryInterface(IID_ISubscriptionAgentShellExt, (void **)&psase);

    if (SUCCEEDED(hr))
    {
        hr = psase->SaveSubscription();
        psase->Release();
    }

    return hr;
}

HRESULT CSubscriptionMgr::URLChange(LPCWSTR pwszNewURL)
{
    return E_NOTIMPL;
}

HRESULT GetInfoFromDataObject(IDataObject *pido,
                              TCHAR *pszPath, DWORD cchPath,
                              TCHAR *pszFriendlyName, DWORD cchFriendlyName,
                              TCHAR *pszURL, DWORD cchURL,
                              INIT_SRC_ENUM *peInitSrc)
{
    STGMEDIUM stgmed;
    FORMATETC fmtetc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    HRESULT hr = pido->GetData(&fmtetc, &stgmed);

    if (hr == S_OK)
    {
        TCHAR szTempURL[INTERNET_MAX_URL_LENGTH];
        TCHAR *pszSection;
        TCHAR *pszEntry;
        TCHAR szTempPath[MAX_PATH];
        TCHAR szIniPath[MAX_PATH];

        if (DragQueryFile((HDROP)stgmed.hGlobal, 0, szTempPath, ARRAYSIZE(szTempPath)))
        {
            // save path
            if (NULL != pszPath)
            {
                StrCpyN(pszPath, szTempPath, cchPath);
            }

            StrCpyN(szIniPath, szTempPath, ARRAYSIZE(szIniPath));
            
            // make friendly name from path
            if (NULL != pszFriendlyName)
            {
                PathStripPath(szTempPath);
                PathRemoveExtension(szTempPath);
                StrCpyN(pszFriendlyName, szTempPath, cchFriendlyName);
            }

            if ((NULL != pszURL) || (NULL != peInitSrc))
            {

                if (PathIsDirectory(szIniPath))
                {
                    PathAppend(szIniPath, TEXT("desktop.ini"));
                    pszSection = TEXT("Channel");
                    pszEntry = TEXT("CDFURL");

                    if (NULL != peInitSrc)
                        *peInitSrc = _INIT_FROM_CHANNEL;
                }
                else
                {
                    pszSection = TEXT("InternetShortcut");
                    pszEntry = TEXT("URL");
                    
                    if (NULL != peInitSrc)
                        *peInitSrc = _INIT_FROM_INTSHCUT;
                }

                if (NULL != pszURL)
                {
                    // canonicalize url
                    if (SHGetIniString(pszSection, pszEntry,
                                                szTempURL, 
                                                INTERNET_MAX_URL_LENGTH, 
                                                szIniPath))
                    {
                        if(!InternetCanonicalizeUrl(szTempURL, pszURL, &cchURL, ICU_NO_ENCODE))
                        {
                            // failed - use non-canonical version
                            StrCpyN(pszURL, szTempURL, cchURL);
                        }
                    }
                    else
                    {
                        hr = E_FAIL;
                    }
                }
            }
        }
        else 
        {
            hr = E_FAIL;
        }

        ReleaseStgMedium(&stgmed);
    }

    return hr;
}

//
// IShellExtInit / IShellPropSheetExt members
//
STDMETHODIMP CSubscriptionMgr::Initialize(LPCITEMIDLIST pcidlFolder, IDataObject * pido, HKEY hkeyProgID)
{
    HRESULT hr;
    ISubscriptionItem *psi;
    CLSID clsid;
    SUBSCRIPTIONCOOKIE cookie;

    hr = GetInfoFromDataObject(pido, m_pszPath, ARRAYSIZE(m_pszPath),
                               m_pszFriendly, ARRAYSIZE(m_pszFriendly),
                               m_pszURL, ARRAYSIZE(m_pszURL),
                               &m_eInitSrc);

    if (SUCCEEDED(hr))
    {
        hr = DoGetItemFromURL(m_pszURL, &psi);
        
        if (SUCCEEDED(hr))
        {
            SUBSCRIPTIONITEMINFO sii;

            sii.cbSize = sizeof(SUBSCRIPTIONITEMINFO);

            hr = psi->GetSubscriptionItemInfo(&sii);

            if (SUCCEEDED(hr))
            {
                clsid = sii.clsidAgent;
            }

            psi->GetCookie(&cookie);

            psi->Release();
        }

        if (FAILED(hr))
        {
            //  New subscription       
            hr = S_OK;
            CreateCookie(&cookie);

            switch (m_eInitSrc)
            {
                case _INIT_FROM_INTSHCUT:
                    clsid = CLSID_WebCrawlerAgent;
                    break;
                    
                case _INIT_FROM_CHANNEL:
                    clsid = CLSID_ChannelAgent;
                    break;

                default:
                    hr = E_FAIL;
                    break;
            }
        }

        if (SUCCEEDED(hr))
        {
            //  HACKHACK:
            //  We call coinit and uninit and keep an object pointer around.
            //  This ain't cool but will work as long as the agents are in
            //  webcheck.  Need to fix this for multi-cast handler.
            hr = CoInitialize(NULL);

            if (SUCCEEDED(hr))
            {
                hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER,
                                      IID_IUnknown, (void**)&m_pUIHelper);

                if (SUCCEEDED(hr))
                {
                    ISubscriptionAgentShellExt *psase;

                    hr = m_pUIHelper->QueryInterface(IID_ISubscriptionAgentShellExt, (void **)&psase);
                    if (SUCCEEDED(hr))
                    {
                        WCHAR wszURL[ARRAYSIZE(m_pszURL)];
                        WCHAR wszName[MAX_NAME + 1];

                        MyStrToOleStrN(wszURL, ARRAYSIZE(wszURL), m_pszURL);
                        MyStrToOleStrN(wszName, ARRAYSIZE(wszName), m_pszFriendly);
                        
                        hr = psase->Initialize(&cookie, wszURL, wszName,
                                               (clsid == CLSID_ChannelAgent) ?
                                               SUBSTYPE_CHANNEL : SUBSTYPE_URL);
                        psase->Release();
                    }
                }
                CoUninitialize();
            }
        }
    }

    return hr;
}

STDMETHODIMP CSubscriptionMgr::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    HRESULT hr;
    if (SHRestricted2W(REST_NoEditingSubscriptions, NULL, 0))
        return E_FAIL;

    ASSERT(NULL != m_pUIHelper);

    if (NULL == m_pUIHelper)
    {
        return E_UNEXPECTED;
    }

    IShellPropSheetExt *pspse;

    hr = m_pUIHelper->QueryInterface(IID_IShellPropSheetExt, (void **)&pspse);

    if (SUCCEEDED(hr))
    {
        hr = pspse->AddPages(lpfnAddPage, lParam);
        pspse->Release();
    }

    return hr;
}

STDMETHODIMP CSubscriptionMgr::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplacePage, LPARAM lParam)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// ISubscriptionMgr members
//

STDMETHODIMP CSubscriptionMgr::IsSubscribed(LPCWSTR pURL, BOOL * pFSub)
{
    HRESULT hr;

    ASSERT (pURL && pFSub);
    MyOleStrToStrN(m_pszURL, INTERNET_MAX_URL_LENGTH, pURL);
 
    * pFSub = FALSE;

    if (!_pidl)
    {
        hr = LoadSubscription(m_pszURL, &_pidl);
        if ((hr != S_OK) || (!_pidl))
            return S_OK;
    }
    else if (UrlCompare(URL(&(_pidl->ooe)), m_pszURL, TRUE))
    {
        COfflineFolderEnum::FreePidl(_pidl);
        _pidl = NULL;
        hr = LoadSubscription(m_pszURL, &_pidl);
        if ((hr != S_OK) || (!_pidl))
            return S_OK;
    }

    * pFSub = TRUE;
    return S_OK;
}

STDMETHODIMP CSubscriptionMgr::DeleteSubscription(LPCWSTR pURL, HWND hwnd)
{
    ASSERT(pURL);
    MyOleStrToStrN(m_pszURL, INTERNET_MAX_URL_LENGTH, pURL);

    if (!_pidl)
    {
        HRESULT hr;

        hr = LoadSubscription(m_pszURL, &_pidl);
        if ((hr != S_OK) || (!_pidl))
            return E_FAIL;
    }

    // This is a restricted action.  The restriction
    // is checked in ConfirmDelete.  If you remove this call,
    // you must add the restriction check here.
    if (!ConfirmDelete(hwnd, 1, &_pidl))
        return E_FAIL;

    HRESULT hr = DoDeleteSubscription(&(_pidl->ooe));
    if (SUCCEEDED(hr))
    {
        TraceMsg(TF_ALWAYS, "%s(URL:%s) deleted", NAME(&(_pidl->ooe)), URL(&(_pidl->ooe)));

        _GenerateEvent(SHCNE_DELETE, (LPITEMIDLIST)_pidl, NULL);

        COfflineFolderEnum::FreePidl(_pidl);
        _pidl = NULL;
    }
    return hr;
}

STDMETHODIMP CSubscriptionMgr::ShowSubscriptionProperties(LPCWSTR pURL, HWND hwnd)
{
    HRESULT hr = S_OK;
    LPMYPIDL oldPidl = NULL, newPidl = NULL;

    ASSERT(pURL);
    MyOleStrToStrN(m_pszURL, INTERNET_MAX_URL_LENGTH, pURL);

    if (!m_pBuf)
    {
        m_pBuf = (OOEBuf *)MemAlloc(LPTR, sizeof(OOEBuf));
        if (NULL == m_pBuf)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        GetDefaultOOEBuf(m_pBuf, SUBSTYPE_URL);

        if(SUCCEEDED(LoadSubscription(m_pszURL, &oldPidl)))
        {
            POOEntry pooe = &(oldPidl->ooe);

            StrCpyN(m_pszFriendly, NAME(&(oldPidl->ooe)), ARRAYSIZE(m_pszFriendly));

            CopyToOOEBuf(&(oldPidl->ooe), m_pBuf);

            m_pBuf->m_dwPropSheetFlags = PSF_IS_ALREADY_SUBSCRIBED;
        }
        else
        {
            CreateCookie(&m_pBuf->m_Cookie);
            StrCpyN(m_pszFriendly, m_pszURL, ARRAYSIZE(m_pszFriendly));
            StrCpyN(m_pBuf->m_URL, m_pszURL, ARRAYSIZE(m_pBuf->m_URL));
            StrCpyN(m_pBuf->m_Name, m_pszURL, ARRAYSIZE(m_pBuf->m_Name));
        }

        hr = CoCreateInstance(*(&m_pBuf->clsidDest), NULL, CLSCTX_INPROC_SERVER, 
                              IID_IUnknown, (void **)&m_pUIHelper);

        if (SUCCEEDED(hr))
        {
            ISubscriptionAgentShellExt *psase;
            
            hr = m_pUIHelper->QueryInterface(IID_ISubscriptionAgentShellExt, (void **)&psase);

            if (SUCCEEDED(hr))
            {
                WCHAR wszURL[MAX_URL + 1];
                WCHAR wszName[MAX_NAME + 1];

                MyStrToOleStrN(wszURL, ARRAYSIZE(wszURL), m_pBuf->m_URL);
                MyStrToOleStrN(wszName, ARRAYSIZE(wszName), m_pBuf->m_Name);

                hr = psase->Initialize(&m_pBuf->m_Cookie, wszURL, wszName, (SUBSCRIPTIONTYPE)-1);
                psase->Release();
            }

            if (SUCCEEDED(hr))
            {
                PROPSHEETHEADER psh = { 0 } ;
                HPROPSHEETPAGE hPropPage[MAX_PROP_PAGES];

                // initialize propsheet header.
                psh.dwSize      = sizeof(PROPSHEETHEADER);
                psh.dwFlags     = PSH_PROPTITLE;
                psh.hwndParent  = hwnd;
                psh.pszCaption  = m_pszFriendly;
                psh.hInstance   = g_hInst;
                psh.nPages      = 0;
                psh.nStartPage  = 0;
                psh.phpage      = hPropPage;

                PROPSHEETPAGE psp;
                psp.dwSize          = sizeof(PROPSHEETPAGE);
                psp.dwFlags         = PSP_DEFAULT;
                psp.hInstance       = MLGetHinst();
                psp.pszIcon         = NULL;
                psp.pszTitle        = NULL;
                psp.lParam          = (LPARAM)this;

                psp.pszTemplate     = MAKEINTRESOURCE(IDD_SUBSPROPS_SUMMARY);
                psp.pfnDlgProc      = SummaryPropDlgProc;

                psh.phpage[psh.nPages++] = CreatePropertySheetPage(&psp);

                if (NULL != hPropPage[0])
                {
                    if (m_pBuf->m_dwPropSheetFlags & PSF_IS_ALREADY_SUBSCRIBED)
                    {
                        hr = AddPages(_AddOnePropSheetPage, (LPARAM)&psh);
                    }

                    if (SUCCEEDED(hr))
                    {
                        INT_PTR iRet = PropertySheet(&psh);

                        if (iRet < 0)
                        {
                            hr = E_FAIL;
                        }
                        else
                        {
                            hr = LoadSubscription(m_pszURL, &newPidl);
                            if (SUCCEEDED(hr))
                            {
                                if (_pidl)
                                {
                                    COfflineFolderEnum::FreePidl(_pidl);
                                }

                                _pidl = newPidl;
                            }
                        }
                    }
                }
            }
        }

        if (NULL != oldPidl)
        {
            COfflineFolderEnum::FreePidl(oldPidl);
        }
    }

    return hr;
}

//
//
//

void
CSubscriptionMgr::ChangeSubscriptionValues (
    OOEBuf *pCurrent,
    SUBSCRIPTIONINFO *pNew
)
{
    //
    // Channel flags
    //

    if (SUBSINFO_CHANNELFLAGS & pNew->fUpdateFlags)
    {
        pCurrent->fChannelFlags = pNew->fChannelFlags;
    }


    //
    // The subscription schedule.
    //

    if (SUBSINFO_SCHEDULE & pNew->fUpdateFlags)
    {

        switch (pNew->schedule)
        {

            case SUBSSCHED_DAILY:
            case SUBSSCHED_MANUAL:
            case SUBSSCHED_WEEKLY:
                LoadGroupCookie(&pCurrent->groupCookie, pNew->schedule);
                break;

            case SUBSSCHED_CUSTOM:
                pCurrent->groupCookie = pNew->customGroupCookie;
                break;

            case SUBSSCHED_AUTO:
                {
                    //  BUGBUG. We should look at subType;
                    memset(&pCurrent->groupCookie, 0, sizeof(pCurrent->groupCookie));  //t-mattgi so it will look at the trigger
                    PTASK_TRIGGER pNewTrigger = ((PTASK_TRIGGER)pNew->pTrigger);
                    if (pNewTrigger && pNewTrigger->cbTriggerSize == sizeof(TASK_TRIGGER))
                    {
                        pCurrent->m_Trigger = *pNewTrigger;
                    }
                    else    //bad trigger; use daily as default
                    {
                        pCurrent->m_Trigger.cbTriggerSize = 0;
                        pCurrent->groupCookie = NOTFCOOKIE_SCHEDULE_GROUP_DAILY;
                    }
                }
                pCurrent->fChannelFlags |= CHANNEL_AGENT_DYNAMIC_SCHEDULE;
                break;

            default:
                ASSERT(FALSE);
                break;
        }
    }

    //
    // Recurse levels.
    //

    if (SUBSINFO_RECURSE & pNew->fUpdateFlags)
       pCurrent->m_RecurseLevels = pNew->dwRecurseLevels;

    //
    // Webcrawler flags.  Note:  The flags are not or'd with the current flags.
    // The caller must set all of the webcrawler flags they want to use.
    //

    if (SUBSINFO_WEBCRAWL & pNew->fUpdateFlags)
        pCurrent->m_RecurseFlags = pNew->fWebcrawlerFlags;

    //
    // Mail notification.
    //

    if (SUBSINFO_MAILNOT & pNew->fUpdateFlags)
        pCurrent->bMail = pNew->bMailNotification;
    else
        pCurrent->bMail = FALSE;

    //
    // Need password.
    //

    if (SUBSINFO_NEEDPASSWORD & pNew->fUpdateFlags)
        pCurrent->bNeedPassword = pNew->bNeedPassword;
    else
        pCurrent->bNeedPassword = FALSE;
    
    //
    // User name.
    //

    if (SUBSINFO_USER & pNew->fUpdateFlags)
    {
        if (pNew->bstrUserName)
        {
            MyOleStrToStrN(pCurrent->username, MAX_USERNAME, pNew->bstrUserName);
        }
        pCurrent->bNeedPassword = pNew->bNeedPassword;
    }
    
    //
    // Password.
    //

    if (SUBSINFO_PASSWORD & pNew->fUpdateFlags)
    {
        if (pNew->bstrPassword)
        {
            MyOleStrToStrN(pCurrent->password, MAX_PASSWORD, pNew->bstrPassword);
        }
        pCurrent->bNeedPassword = pNew->bNeedPassword;
    }

    //
    // Friendly Name.
    //

    if (SUBSINFO_FRIENDLYNAME & pNew->fUpdateFlags)
    {
        if (pNew->bstrFriendlyName)
        {
            MyOleStrToStrN(pCurrent->m_Name, MAX_NAME, pNew->bstrFriendlyName);
        }
    }

    //
    // Gleam
    //

    if (SUBSINFO_GLEAM & pNew->fUpdateFlags)
    {
        pCurrent->bGleam = pNew->bGleam;
    }

    //
    // Changes only (notification only)
    //

    if (SUBSINFO_CHANGESONLY & pNew->fUpdateFlags)
    {
        pCurrent->bChangesOnly = pNew->bChangesOnly;
    }

    //
    // dwMaxSizeKB
    //
    if (SUBSINFO_MAXSIZEKB & pNew->fUpdateFlags)
    {
        pCurrent->m_SizeLimit = pNew->dwMaxSizeKB;
    }

    //
    // Task flags
    //
    if (SUBSINFO_TASKFLAGS & pNew->fUpdateFlags)
    {
        pCurrent->grfTaskTrigger = pNew->fTaskFlags;
    }

    return;
}

//
// CSubscriptionMgr::CountSubscriptions
// BUGBUG: We could make this public if other people need it.  An enumerator
// would be more useful though...
//
HRESULT CSubscriptionMgr::CountSubscriptions(SUBSCRIPTIONTYPE subType, PDWORD pdwCount)
{
    HRESULT hr;
    IEnumSubscription *pes;

    ASSERT(NULL != pdwCount);

    *pdwCount = 0;

    hr = EnumSubscriptions(0, &pes);

    if (SUCCEEDED(hr))
    {
        SUBSCRIPTIONCOOKIE cookie;

        while (S_OK == pes->Next(1, &cookie, NULL))
        {
            ISubscriptionItem *psi;
            DWORD dwRet;

            if (SUCCEEDED(SubscriptionItemFromCookie(FALSE, &cookie, &psi)))
            {
                if (SUCCEEDED(ReadDWORD(psi, c_szPropChannel, &dwRet)) && dwRet)
                {
                    if (SUBSTYPE_CHANNEL == subType)
                        (*pdwCount)++;
                }
                else if (SUCCEEDED(ReadDWORD(psi, c_szPropDesktopComponent, &dwRet)) && dwRet)
                {
                    if (SUBSTYPE_DESKTOPURL == subType || SUBSTYPE_DESKTOPCHANNEL == subType)
                        (*pdwCount)++;
                }
                else
                {
                    if (SUBSTYPE_URL == subType)
                        (*pdwCount)++;
                }
                psi->Release();
            }
        }
    }

    return hr;
}

//
// CSubscriptionMgr::IsValidSubscriptionInfo
//

#define SUBSCRIPTIONSCHEDULE_MAX 4

BOOL CSubscriptionMgr::IsValidSubscriptionInfo(SUBSCRIPTIONTYPE subType, SUBSCRIPTIONINFO *pSI)
{
    if (pSI->cbSize != sizeof(SUBSCRIPTIONINFO))
    {
        return FALSE;
    }
    else if (pSI->fUpdateFlags & ~SUBSINFO_ALLFLAGS)
    {
        return FALSE;
    }
    else if (pSI->pTrigger && ((TASK_TRIGGER*)(pSI->pTrigger))->cbTriggerSize &&
        (subType == SUBSTYPE_URL || subType == SUBSTYPE_DESKTOPURL)) // || pSI->schedule != SUBSSCHED_AUTO))
    {
        return FALSE;
    }
    else if (pSI->fUpdateFlags & SUBSINFO_SCHEDULE)
    {
        if (pSI->schedule > SUBSCRIPTIONSCHEDULE_MAX)
        {
            return FALSE;
        }
        if (pSI->schedule == SUBSSCHED_CUSTOM && pSI->customGroupCookie == CLSID_NULL)
        {
            return FALSE;
        }
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
// *** RemoveURLMapping ***
//
// Description:
//     Removes the cache and registry settings that wininet uses to map the 
//     given url to a local file.
//
////////////////////////////////////////////////////////////////////////////////

#define PRELOAD_REG_KEY \
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Cache\\Preload")

void RemoveURLMapping(LPCWSTR pszURL)
{
    BYTE cei[MY_MAX_CACHE_ENTRY_INFO];
    DWORD cbcei = sizeof(cei);
    LPINTERNET_CACHE_ENTRY_INFOW pcei = (LPINTERNET_CACHE_ENTRY_INFOW)cei;
    
    //
    // Look up the url in the cache
    //
    if (GetUrlCacheEntryInfoExW(pszURL, pcei, &cbcei, NULL, 0, NULL, 0))
    {
        // 
        // see if it has a mapping because it is a preinstalled cache entry
        //
        if (pcei->CacheEntryType & INSTALLED_CACHE_ENTRY)
        {
            //
            // Clear the flag
            //
            pcei->CacheEntryType &= ~INSTALLED_CACHE_ENTRY;
            SetUrlCacheEntryInfoW(pszURL, pcei, CACHE_ENTRY_ATTRIBUTE_FC);

            //
            // Now remove the mapping from the registry
            //
            HKEY hk;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, PRELOAD_REG_KEY, 0, KEY_WRITE, &hk) == ERROR_SUCCESS) 
            {
                RegDeleteValueW(hk, pszURL);
                RegCloseKey(hk);
            }
        }
    }
}

//
// CSubscriptionMgr::CreateSubscription
// entry point for creating a subscription
// flags:
//    CREATESUBS_FROMFAVORITES -- already exists in favorites, use alternate summary dialog
//                         that doesn't do AddToFavorites.  Valid only for channel or url.
//    CREATESUBS_INACTIVEPLATINUM -- when creating a channel subscription, show Activate Channel dialog
//                         valid only for channel subscriptions with CREATESUBS_FROMFAVORITES
//    CREATESUBS_ADDTOFAVORITES -- display summary dialog before wizard
//        default summary: for channel or url, use AddToFavorites from shdocvw
//                         for desktop item, just a confirmation dialog
//                         for other, no summary -- straight to wizard
//    CREATESUBS_NOUI -- totally silent
//    CREATESUBS_NOSAVE -- update subscription in memory buffer, not on disk (pInfo must be non-NULL)
//
STDMETHODIMP
CSubscriptionMgr::CreateSubscription (
    HWND hwnd,
    LPCWSTR pwszURL,
    LPCWSTR pwszFriendlyName,
    DWORD dwFlags,
    SUBSCRIPTIONTYPE subsType,
    SUBSCRIPTIONINFO *pInfo
)
{
    HRESULT hr = E_INVALIDARG;
    BOOL bAlready;

    if (IsFlagSet(dwFlags, CREATESUBS_NOUI) || !IsFlagSet (dwFlags, CREATESUBS_ADDTOFAVORITES))
    {    //no UI, so skip ATF dialog
        hr = CreateSubscriptionNoSummary (hwnd, pwszURL, pwszFriendlyName, dwFlags,
            subsType, pInfo);

        if (hr == S_OK)
        {
            //
            // The user successfully subscribed to this URL so remove
            // mappings used for preinstalled content as this URL is now
            // "Activated"
            //
            RemoveURLMapping(pwszURL);
        }
    }
    else
    {
        switch (subsType)
        {
        case SUBSTYPE_URL:
        case SUBSTYPE_CHANNEL:
            hr = SHAddSubscribeFavorite (hwnd, pwszURL, pwszFriendlyName,
                                         dwFlags, subsType, pInfo);
            break;

        case SUBSTYPE_DESKTOPCHANNEL:
        case SUBSTYPE_DESKTOPURL:
            hr = IsSubscribed (pwszURL, &bAlready);
            if (SUCCEEDED (hr) && bAlready)
                break;  //don't display summary dialog since it has nothing useful for this case
            hr = CreateDesktopSubscription (hwnd, pwszURL, pwszFriendlyName,
                                            dwFlags, subsType, pInfo);
            break;

        default:    //SUBSTYPE_EXTERNAL -- don't know what kind of summary to show
            hr = CreateSubscriptionNoSummary (hwnd, pwszURL, pwszFriendlyName, dwFlags,
                subsType, pInfo);
            break;
        }
    }

    return hr;
}


//
// CSubscriptionMgr::CreateSubscriptionNoSummary
// modify a SUBSCRIPTIONINFO interactively, using the wizard
// persists info to Subscriptions folder, unless SUBSINFO_NOSAVE passed
//
STDMETHODIMP
CSubscriptionMgr::CreateSubscriptionNoSummary (
    HWND hwnd,
    LPCWSTR pwszURL,
    LPCWSTR pwszFriendlyName,
    DWORD dwFlags,
    SUBSCRIPTIONTYPE subType,
    SUBSCRIPTIONINFO *pInfo
)
{
    HRESULT hr = S_OK;
    
    //
    // Validate the parameters.
    //
    if (!IS_VALID_SUBSCRIPTIONTYPE(subType)
        || !pwszURL
        || !pwszFriendlyName
        || (!pInfo && (dwFlags & CREATESUBS_NOSAVE))
        || (pInfo && !IsValidSubscriptionInfo(subType, pInfo)))
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    //
    // Fail if already subscribed and we aren't in no save or no UI mode.
    // Caller is responsible for UI.
    //
    BOOL fAlreadySubscribed;
    if ((FAILED(IsSubscribed(pwszURL, &fAlreadySubscribed)) || fAlreadySubscribed) &&
        (!(dwFlags & (CREATESUBS_NOSAVE | CREATESUBS_NOUI))))
    {
        return E_FAIL;
    }

    //
    // Fail if restrictions are in place.  
    // BUGBUG: Currently cdfview is handling channel restrictions for 
    // but we should probably do it here.
    // BUGBUG: Should we have a flag parameter to override this?
    //
    if (SUBSTYPE_URL == subType)
    {
        DWORD dwMaxCount = SHRestricted2W(REST_MaxSubscriptionCount, NULL, 0);
        DWORD dwCount;
        if (SHRestricted2W(REST_NoAddingSubscriptions, pwszURL, 0)
            || ((dwMaxCount > 0)
                && SUCCEEDED(CountSubscriptions(subType, &dwCount))
                && (dwCount >= dwMaxCount)))
        {
            if (!IsFlagSet(dwFlags, CREATESUBS_NOUI))
                SGMessageBox(hwnd, IDS_RESTRICTED, MB_OK);
            return E_ACCESSDENIED;
        }
    }
    
    //
    // Get the subscription defaults and merge in the caller's info
    //
    OOEBuf subProps;
    GetDefaultOOEBuf(&subProps, subType);
    
    //this is (intentionally) duplicated below... it needs to be after the ChangeSubscriptionValues()
    //call, but we need to grab the url first to make sure it's subscribable.
    MyOleStrToStrN(subProps.m_URL, INTERNET_MAX_URL_LENGTH, pwszURL);

    // BUGBUG: Does this mean we can't support plugin protocols?
    if (/*(subType != SUBSTYPE_EXTERNAL) &&*/ !IsHTTPPrefixed(subProps.m_URL))
    {
        return E_INVALIDARG;
    }

    if (pInfo)
    {
        ChangeSubscriptionValues(&subProps, pInfo);
        if (fAlreadySubscribed)
        {
            ReadCookieFromInetDB(subProps.m_URL, &subProps.m_Cookie);
            subProps.m_dwPropSheetFlags |= PSF_IS_ALREADY_SUBSCRIBED;
        }
    }

    // Disallow password caching if restriction is in place.  This both
    // skips the wizard page and prevents the caller's password from
    // being saved.
    if (SHRestricted2W(REST_NoSubscriptionPasswords, NULL, 0))
    {
        subProps.bNeedPassword = FALSE;
        subProps.username[0] = 0;
        subProps.password[0] = 0;
        subProps.dwFlags &= ~(PROP_WEBCRAWL_UNAME | PROP_WEBCRAWL_PSWD);
    }

    //the passed-in name and url override whatever's in the info buffer
    MyOleStrToStrN(subProps.m_URL, INTERNET_MAX_URL_LENGTH, pwszURL);
    MyOleStrToStrN(subProps.m_Name, MAX_NAME_QUICKLINK, pwszFriendlyName);

    //
    // If we're in UI mode, initialize the wizard
    //
    if (!IsFlagSet(dwFlags, CREATESUBS_NOUI))
    {
        hr = CreateWizard(hwnd, subType, &subProps);

    } // !NOUI

    //
    // If we're not in NOSAVE mode, then create/save the subscription
    //
    if (SUCCEEDED(hr))
    {
        if (!IsFlagSet(dwFlags, CREATESUBS_NOSAVE))
        {
            //hack to let AddToFavorites dialog display the screen-saver-activate dialog -- it
            //needs to pass NOUI, which disables all UI, but if you also pass FROMFAVORITES and not
            //ADDTOFAVORITES then we still let this dialog slip through.  (Shouldn't affect anyone
            //else because FROMFAVORITES is always used with ADDTOFAVORITES.)
            if (!IsFlagSet(dwFlags, CREATESUBS_NOUI) ||
                (IsFlagSet(dwFlags, CREATESUBS_FROMFAVORITES) && !IsFlagSet (dwFlags, CREATESUBS_ADDTOFAVORITES)))
            {
                // See if the user wants to be asked about enabling the screen saver.
                DWORD dwValue = BST_UNCHECKED;
                ReadRegValue(   HKEY_CURRENT_USER,
                                WEBCHECK_REGKEY,
                                g_szDontAskScreenSaver,
                                &dwValue,
                                SIZEOF(dwValue));

                if  (
                    (subProps.fChannelFlags & CHANNEL_AGENT_PRECACHE_SCRNSAVER)
                    &&
                    (dwValue == BST_UNCHECKED)
                    &&
                    !IsADScreenSaverActive()
                    )
                {
                    DialogBox( MLGetHinst(), 
                             MAKEINTRESOURCE(IDD_SUBSCRIPTION_ENABLECHANNELSAVER),
                             hwnd,
                             EnableScreenSaverDlgProc);
                }
            }

            //
            // Create a new pidl with the user specified properties.
            //
            if (_pidl)
            {
                COfflineFolderEnum::FreePidl(_pidl);
                _pidl = NULL;
                SAFERELEASE(m_pUIHelper);
            }
            hr = CreateSubscriptionFromOOEBuf(&subProps, &_pidl);
            if (SUCCEEDED(hr))
            {
                ASSERT(_pidl);
                //
                // Send a notification that a subscription has changed.
                //
                _GenerateEvent(SHCNE_CREATE, (LPITEMIDLIST)_pidl, NULL);
            }
        } //!NOSAVE
        else if (S_OK == hr)
        {
            //in NOSAVE mode, so don't actually create subscription -- save it back
            //to passed-in buffer
            ASSERT (pInfo);
            pInfo->fUpdateFlags = SUBSINFO_ALLFLAGS;    //fill in all possible fields
            UpdateSubsInfoFromOOE (pInfo, &subProps);
        }
    }
    
    return hr;
}


STDMETHODIMP
CSubscriptionMgr::CreateDesktopSubscription (HWND hwnd, LPCWSTR pwszURL, LPCWSTR pwszFriendlyName,
                        DWORD dwFlags, SUBSCRIPTIONTYPE subsType, SUBSCRIPTIONINFO *pInfo)
{
    HRESULT hr;
    SUBSCRIPTIONINFO siTemp = { sizeof(SUBSCRIPTIONINFO), 0 };
    if (!pInfo)
        pInfo = &siTemp;    //make sure we have a valid buffer if caller doesn't give us one

    //make sure adminrestrictions allow this

    if (SHRestricted2W(REST_NoAddingChannels, pwszURL, 0))
        return E_FAIL;

    SUBSCRIBE_ADI_INFO parms = { this, pwszFriendlyName, pwszURL, pInfo, subsType, dwFlags };

    //make sure this url is subscribable; if not, show error dialog
    {
        TCHAR sz[MAX_URL];
        MyOleStrToStrN (sz, ARRAYSIZE(sz), pwszURL);

        if (!IsHTTPPrefixed (sz))
        {
            SGMessageBox(hwnd, IDS_HTTPONLY, MB_ICONINFORMATION | MB_OK);
            return E_INVALIDARG;
        }
    }

    INT_PTR iDlgResult = DialogBoxParam (MLGetHinst(), MAKEINTRESOURCE(IDD_DESKTOP_SUBSCRIPTION_SUMMARY),
                        hwnd, SummarizeDesktopSubscriptionDlgProc, (LPARAM)&parms);

    switch (iDlgResult)
    {
    case -1:
        hr = E_FAIL;
        break;
    case IDCANCEL:
        hr = S_FALSE;
        break;
    default:
        hr = CreateSubscriptionNoSummary (hwnd, pwszURL, pwszFriendlyName,
                CREATESUBS_NOUI | dwFlags, subsType, pInfo);
        break;
    }

    return hr;
}


STDMETHODIMP
CSubscriptionMgr::GetDefaultInfo(
    SUBSCRIPTIONTYPE    subType,
    SUBSCRIPTIONINFO *pInfo
)
{
    //
    // Validate the parameters.
    //
    if (!IS_VALID_SUBSCRIPTIONTYPE(subType)
        || !pInfo 
        || (pInfo->cbSize != sizeof(SUBSCRIPTIONINFO)))
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    memset((void *)pInfo, 0, sizeof(SUBSCRIPTIONINFO));
    pInfo->cbSize = sizeof(SUBSCRIPTIONINFO);

    // Fill in default structure.  Note that lines are commented out
    // below to indicate the field is initialized to 0 without wasting
    // code (memset above already cleared structure out.)
    
    pInfo->fUpdateFlags = SUBSINFO_RECURSE | SUBSINFO_MAILNOT 
                        | SUBSINFO_WEBCRAWL 
                        /*| SUBSINFO_SCHEDULE */ | SUBSINFO_CHANGESONLY
                        | SUBSINFO_CHANNELFLAGS;
    pInfo->dwRecurseLevels = DEFAULTLEVEL;
    pInfo->schedule = SUBSSCHED_AUTO;
    
    switch (subType)
    {
        case SUBSTYPE_URL:
//            pInfo->bChangesOnly = FALSE;
//            pInfo->bMailNotification = FALSE;
//            pInfo->bPasswordNeeded = FALSE;
            pInfo->fWebcrawlerFlags = DEFAULTFLAGS;
            break;

        case SUBSTYPE_CHANNEL:
//            pInfo->bChangesOnly = FALSE;
//            pInfo->bMailNotification = FALSE;
            pInfo->fChannelFlags = CHANNEL_AGENT_PRECACHE_ALL | CHANNEL_AGENT_DYNAMIC_SCHEDULE;
            break;

        case SUBSTYPE_DESKTOPCHANNEL:
//          pInfo->bChangesOnly = FALSE;
//          pInfo->bMailNotification = FALSE;
            pInfo->fChannelFlags = CHANNEL_AGENT_PRECACHE_ALL | CHANNEL_AGENT_DYNAMIC_SCHEDULE;
            break;
            
        case SUBSTYPE_DESKTOPURL:
//          pInfo->bChangesOnly = FALSE;
//          pInfo->bMailNotification = FALSE;
            pInfo->fWebcrawlerFlags = DEFAULTFLAGS;
            break;
            
        default:
            return E_NOTIMPL;
    }
    
    return S_OK;
}

STDMETHODIMP
CSubscriptionMgr::GetSubscriptionInfo(
    LPCWSTR pwszURL,
    SUBSCRIPTIONINFO *pInfo
)
{
    HRESULT hr;

    //
    // Validate the parameters.
    //
    if (!pInfo 
        || !pwszURL
        || (pInfo->cbSize != sizeof(SUBSCRIPTIONINFO)))
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    BOOL    bSubscribe;
    hr = IsSubscribed(pwszURL, &bSubscribe);

    RETURN_ON_FAILURE(hr);
    if (!bSubscribe)
    {
        return E_FAIL;
    }


    // We cannot rely on the caller passing us a clean SUBSCRIPTIONINFO
    // structure.  We need to clean it ourselves.
    DWORD dwFlags = pInfo->fUpdateFlags;
    ZeroMemory(pInfo, sizeof(SUBSCRIPTIONINFO));
    pInfo->cbSize = sizeof(SUBSCRIPTIONINFO);
    pInfo->fUpdateFlags = dwFlags;

    OOEBuf ooeb;    //BUGBUG this is kind of lame -- but we need the code in UpdateSubsInfoFromOOE
    CopyToOOEBuf (&(_pidl->ooe), &ooeb);    //to work once for a buf and once for an entry, and it's
    UpdateSubsInfoFromOOE (pInfo, &ooeb);   //easier to convert entry->buf so we do that here.

    return S_OK;
}


void UpdateSubsInfoFromOOE (SUBSCRIPTIONINFO* pInfo, POOEBuf pooe)
{
    DWORD   dwFlags = pInfo->fUpdateFlags & SUBSINFO_ALLFLAGS;
    SUBSCRIPTIONTYPE subType = GetItemCategory(pooe);

    if (dwFlags & SUBSINFO_USER)
    {
        SAFEFREEBSTR (pInfo->bstrUserName);
        CreateBSTRFromTSTR(&(pInfo->bstrUserName), pooe->username);
    }
    if (dwFlags & SUBSINFO_PASSWORD)
    {
        SAFEFREEBSTR (pInfo->bstrPassword);
        CreateBSTRFromTSTR(&(pInfo->bstrPassword), pooe->password);
    }
    if (dwFlags & SUBSINFO_FRIENDLYNAME)
    {
        SAFEFREEBSTR (pInfo->bstrFriendlyName);
        CreateBSTRFromTSTR(&(pInfo->bstrFriendlyName), pooe->m_Name);
    }
    
    pInfo->fUpdateFlags = dwFlags;
    if (dwFlags & SUBSINFO_SCHEDULE)
    {
        pInfo->schedule = GetGroup(pooe);
        if (pInfo->schedule == SUBSSCHED_CUSTOM)
        {
            if (pooe->groupCookie != GUID_NULL)
            {
                pInfo->customGroupCookie = pooe->groupCookie;
            }
            else
            {
                GetItemSchedule(&pooe->m_Cookie, &pInfo->customGroupCookie);
                if (pInfo->customGroupCookie == GUID_NULL)
                {
                    pInfo->schedule = SUBSSCHED_MANUAL;
                }
            }
        }
    }
    
    if (PTASK_TRIGGER pInfoTrigger = (PTASK_TRIGGER)pInfo->pTrigger)
    {
        if (pInfoTrigger->cbTriggerSize == pooe->m_Trigger.cbTriggerSize)
            *(pInfoTrigger) = pooe->m_Trigger;
        else
            pInfoTrigger->cbTriggerSize = 0;
    }
    //otherwise, it's already null and we can't do anything about it... luckily, we'll never
    //have a trigger that we need to write back to a SUBSCRIPTIONINFO that didn't already have
    //one.

    if (dwFlags & SUBSINFO_RECURSE)
        pInfo->dwRecurseLevels = pooe->m_RecurseLevels;
    if (dwFlags & SUBSINFO_WEBCRAWL)
        pInfo->fWebcrawlerFlags = pooe->m_RecurseFlags;
    if (dwFlags & SUBSINFO_MAILNOT)
        pInfo->bMailNotification = pooe->bMail;
    if (dwFlags & SUBSINFO_GLEAM)
        pInfo->bGleam = pooe->bGleam;
    if (dwFlags & SUBSINFO_CHANGESONLY)
        pInfo->bChangesOnly = pooe->bChangesOnly;
    if (dwFlags & SUBSINFO_NEEDPASSWORD)
        pInfo->bNeedPassword = pooe->bNeedPassword;
    if (dwFlags & SUBSINFO_CHANNELFLAGS)
    {
        if ((subType==SUBSTYPE_CHANNEL)||(subType==SUBSTYPE_DESKTOPCHANNEL))
        {
            pInfo->fChannelFlags = pooe->fChannelFlags;
        }
        else
        {
            pInfo->fChannelFlags = 0;
            pInfo->fUpdateFlags &= (~SUBSINFO_CHANNELFLAGS);
        }
    }
    if (dwFlags & SUBSINFO_MAXSIZEKB)
        pInfo->dwMaxSizeKB = pooe->m_SizeLimit;


    if (dwFlags & SUBSINFO_TYPE)
    {
        pInfo->subType = GetItemCategory(pooe);
        ASSERT(IS_VALID_SUBSCRIPTIONTYPE(pInfo->subType));
    }

    if (dwFlags & SUBSINFO_TASKFLAGS)
    {
        pInfo->fTaskFlags = pooe->grfTaskTrigger;
    }

}


STDMETHODIMP
CSubscriptionMgr::UpdateSubscription(LPCWSTR pwszURL)
{
    ASSERT(pwszURL);
    BOOL    bSubscribe = FALSE;
    HRESULT hr = IsSubscribed(pwszURL, &bSubscribe);

    RETURN_ON_FAILURE(hr);
    if (!bSubscribe)
    {
        return E_INVALIDARG;
    }

    //
    // Fail if restrictions are in place.  
    // BUGBUG: Should we have a flag parameter to override this?
    //
    if (SHRestricted2W(REST_NoManualUpdates, NULL, 0))
    {
        SGMessageBox(NULL, IDS_RESTRICTED, MB_OK);
        return E_ACCESSDENIED;
    }

    return SendUpdateRequests(NULL, &(_pidl->ooe.m_Cookie), 1);
}

STDMETHODIMP
CSubscriptionMgr::UpdateAll()
{
    //
    // Fail if restrictions are in place.  
    // BUGBUG: Should we have a flag parameter to override this?
    //
    if (SHRestricted2W(REST_NoManualUpdates, NULL, 0))
    {
        SGMessageBox(NULL, IDS_RESTRICTED, MB_OK);
        return E_ACCESSDENIED;
    }

    return SendUpdateRequests(NULL, NULL, 0);
}

HRESULT MergeOOEBuf(POOEBuf p1, POOEBuf p2, DWORD fMask)
{
    ASSERT(p1 && p2);
    DWORD   dwMask = p2->dwFlags & fMask;

    if (dwMask == 0)
        return S_OK;

    if (p1->clsidDest != p2->clsidDest)
        return E_INVALIDARG;

    if (dwMask & PROP_WEBCRAWL_COOKIE)
    {
        //  We shouldn't merge cookies.
    }

    if (dwMask & PROP_WEBCRAWL_SIZE)
    {
        p1->m_SizeLimit = p2->m_SizeLimit;
    }
    if (dwMask & PROP_WEBCRAWL_FLAGS)
    {
        p1->m_RecurseFlags = p2->m_RecurseFlags;
    }
    if (dwMask & PROP_WEBCRAWL_LEVEL)
    {
        p1->m_RecurseLevels = p2->m_RecurseLevels;
    }
    if (dwMask & PROP_WEBCRAWL_URL)
    {
        StrCpyN(p1->m_URL, p2->m_URL, MAX_URL);
    }
    if (dwMask & PROP_WEBCRAWL_NAME)
    {
        StrCpyN(p1->m_Name, p2->m_Name, MAX_NAME);
    }

    if (dwMask & PROP_WEBCRAWL_PSWD)
    {
        StrCpyN(p1->password, p2->password, MAX_PASSWORD);
    }
    if (dwMask & PROP_WEBCRAWL_UNAME)
    {
        StrCpyN(p1->username, p2->username, MAX_USERNAME);
    }
    if (dwMask & PROP_WEBCRAWL_DESKTOP)
    {
        p1->bDesktop = p2->bDesktop;
    }
    if (dwMask & PROP_WEBCRAWL_CHANNEL)
    {
        p1->bChannel = p2->bChannel;
    }
    if (dwMask & PROP_WEBCRAWL_EMAILNOTF)
    {
        p1->bMail = p2->bMail;
    }
    if (dwMask & PROP_WEBCRAWL_RESCH)
    {
        p1->grfTaskTrigger = p2->grfTaskTrigger;
        p1->groupCookie = p2->groupCookie;
        p1->fChannelFlags |= (p2->fChannelFlags & CHANNEL_AGENT_DYNAMIC_SCHEDULE);
    }
    if (dwMask & PROP_WEBCRAWL_LAST)
    {
        p1->m_LastUpdated = p2->m_LastUpdated;
    }
    if (dwMask & PROP_WEBCRAWL_STATUS)
    {
        p1->status = p2->status;
    }
    if (dwMask & PROP_WEBCRAWL_PRIORITY)
    {
        p1->m_Priority = p2->m_Priority;
    }
    if (dwMask & PROP_WEBCRAWL_GLEAM)
    {
        p1->bGleam = p2->bGleam;
    }
    if (dwMask & PROP_WEBCRAWL_CHANGESONLY)
    {
        p1->bChangesOnly = p2->bChangesOnly;
    }
    if (dwMask & PROP_WEBCRAWL_CHANNELFLAGS)
    {
        p1->fChannelFlags = p2->fChannelFlags;
    }

    p1->dwFlags |= (p2->dwFlags & fMask & (~PROP_WEBCRAWL_COOKIE));

    return S_OK;
}


INT_PTR CALLBACK SummarizeDesktopSubscriptionDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SUBSCRIBE_ADI_INFO* pInfo = (SUBSCRIBE_ADI_INFO*)GetProp(hDlg,SUBSCRIBEADIPROP);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pInfo = (SUBSCRIBE_ADI_INFO*)lParam;
        ASSERT (pInfo);
        SetProp (hDlg, SUBSCRIBEADIPROP, (HANDLE)pInfo);
        {   //block to declare vars to update captions
            TCHAR sz[MAX_URL];

            if (pInfo->subsType == SUBSTYPE_DESKTOPCHANNEL)
            {
                if(MLLoadString(
                    (pInfo->pSubsInfo->bNeedPassword ? IDS_DESKTOPCHANNEL_SUMMARY_TEXT : IDS_DESKTOPCHANNEL_SUMMARY_NOPW),
                    sz, ARRAYSIZE(sz)))
                {
                    SetDlgItemText(hDlg, IDC_DESKTOP_SUMMARY_TEXT, sz);
                }
            }

            MyOleStrToStrN (sz, ARRAYSIZE(sz), pInfo->pwszName);
            SetListViewToString(GetDlgItem(hDlg, IDC_SUBSCRIBE_ADI_NAME), sz);
            MyOleStrToStrN (sz, ARRAYSIZE(sz), pInfo->pwszUrl);
            SetListViewToString (GetDlgItem (hDlg, IDC_SUBSCRIBE_ADI_URL), sz);
        }
        break;

    case WM_COMMAND:
        ASSERT (pInfo);
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;

        case IDOK:
            //subscription happens in calling function when we return IDOK
            EndDialog(hDlg, IDOK);
            break;

        case IDC_SUBSCRIBE_CUSTOMIZE:
            //run through wizard in NOSAVE mode
            if (S_OK == pInfo->pMgr->CreateSubscriptionNoSummary (hDlg, pInfo->pwszUrl,
                                        pInfo->pwszName, pInfo->dwFlags | CREATESUBS_NOSAVE,
                                        pInfo->subsType, pInfo->pSubsInfo))
            {
                SendMessage (hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDOK), TRUE);
            }
            break;
        }
        break;

        case WM_NOTIFY:
            if (LOWORD(wParam) == IDC_SUBSCRIBE_ADI_URL)
            {
                NM_LISTVIEW * pnmlv = (NM_LISTVIEW *)lParam;
                if (pnmlv->hdr.code == LVN_GETINFOTIP)
                {
                    TCHAR szURL[MAX_URL];
                    LV_ITEM lvi = {0};
                    lvi.mask = LVIF_TEXT;
                    lvi.pszText = szURL;
                    lvi.cchTextMax = ARRAYSIZE(szURL);
                    if (!ListView_GetItem (GetDlgItem (hDlg, IDC_SUBSCRIBE_ADI_URL), &lvi))
                        return FALSE;

                    NMLVGETINFOTIP  * pTip = (NMLVGETINFOTIP *)pnmlv;
                    ASSERT(pTip);
                    StrCpyN(pTip->pszText, szURL, pTip->cchTextMax);
                    return TRUE;
                }
            }
        break;

    case WM_DESTROY:
        RemoveProp (hDlg, SUBSCRIBEADIPROP);
        break;
    }

    return FALSE;
}
