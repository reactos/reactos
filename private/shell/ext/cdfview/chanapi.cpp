
// API to install a channel by creating a system folder in the channel directory
//
// Julian Jiggins (julianj), 4th May, 1997
//

#include "stdinc.h"
#include "resource.h"
#include "cdfidl.h"
#include "xmlutil.h"
#include "persist.h"
#include "cdfview.h"
#include "chanapi.h"
#include "chanmgrp.h"
#include "chanmgri.h"
#include "chanenum.h"
#include "dll.h"
#include "shguidp.h"
#include "winineti.h"
#define _SHDOCVW_
#include <shdocvw.h>

#include <mluisupp.h>

#ifdef UNIX
#undef EVAL
#define EVAL(x) x

STDAPI SHAddSubscribeFavorite(HWND hwnd, LPCWSTR pwszURL, LPCWSTR pwszName, DWORD dwFlags, SUBSCRIPTIONTYPE subsType, SUBSCRIPTIONINFO* pInfo);
#endif /* UNIX */

#define SHELLFOLDERS \
   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")

// Wininet cache preload registry key in HKCU
const TCHAR c_szRegKeyCachePreload[]    = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Cache\\Preload");

BOOL  PathCombineCleanPath(LPTSTR pszCleanPath, LPCTSTR pszPath);
#ifndef UNICODE
int   MyPathCleanupSpec(LPCTSTR pszDir, LPTSTR pszSpec);
#endif
void Channel_OrderItem(LPCTSTR szPath);

// BUGBUG: This was copied from shdocvw\favorite.cpp
static const int CREATESUBS_ACTIVATE = 0x8000;      //hidden flag meaning channel is already on system

//
// Debugging code
//
#if 0
void DumpOrderList(HDPA hdpa)
{
    int i = 0;
    PORDERITEM poi = (PORDERITEM)DPA_GetPtr(hdpa, i);
    while (poi)
    {
        TCHAR szName[MAX_PATH];
        wnsprintf(szName, ARRAYSIZE(szName), "nOrder=%d, lParam=%d, pidl=", poi->nOrder, poi->lParam);
        OutputDebugString(szName);
        ASSERT(SHGetPathFromIDListA(poi->pidl, szName));
        OutputDebugString(szName);
        OutputDebugString("\n");
        i++;
        poi = (PORDERITEM)DPA_GetPtr(hdpa, i);
    }
}
void DumpPidl(LPITEMIDLIST pidl)
{
    TCHAR szName[MAX_PATH];
    ASSERT(SHGetPathFromIDListA(pidl, szName));
    OutputDebugString(szName);
    OutputDebugString("\n");
}
#endif

//
// Constructor and destructor.
//

////////////////////////////////////////////////////////////////////////////////
//
// *** CChannelMgr::CChannelMgr ***
//
//    Constructor.
//
////////////////////////////////////////////////////////////////////////////////
CChannelMgr::CChannelMgr (
    void
)
: m_cRef(1)
{
    TraceMsg(TF_OBJECTS, "+ IChannelMgr");

    DllAddRef();

    return;
}

////////////////////////////////////////////////////////////////////////////////
//
// *** CChannelMgr::~CChannelMgr ***
//
//    Destructor.
//
////////////////////////////////////////////////////////////////////////////////
CChannelMgr::~CChannelMgr (
    void
)
{
    ASSERT(0 == m_cRef);

    //
    // Matching Release for the constructor Addref.
    //

    TraceMsg(TF_OBJECTS, "- IChannelMgr");

    DllRelease();

    return;
}


//
// IUnknown methods.
//

////////////////////////////////////////////////////////////////////////////////
//
// *** CChannelMgr::QueryInterface ***
//
//    CChannelMgr QI.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CChannelMgr::QueryInterface (
    REFIID riid,
    void **ppv
)
{
    ASSERT(ppv);

    HRESULT hr;

    *ppv = NULL;

    if (IID_IUnknown == riid || IID_IChannelMgr == riid)
    {
        *ppv = (IChannelMgr*)this;
    }
    else if ((IID_IChannelMgrPriv2 == riid) || (IID_IChannelMgrPriv == riid))
    {
        *ppv = (IChannelMgrPriv2*)this;
    }
    else if (IID_IShellCopyHook == riid)
    {  
        *ppv = (ICopyHook*)this;
    }
#ifdef UNICODE
    else if (IID_IShellCopyHookA == riid)
    {  
        *ppv = (ICopyHookA*)this;
    }
#endif
    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        hr = S_OK;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    ASSERT((SUCCEEDED(hr) && *ppv) || (FAILED(hr) && NULL == *ppv));

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// *** CChannelMgr::AddRef ***
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CChannelMgr::AddRef (
    void
)
{
    ASSERT(m_cRef != 0);
    ASSERT(m_cRef < (ULONG)-1);

    return ++m_cRef;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CChannelMgr::Release ***
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CChannelMgr::Release (
    void
)
{
    ASSERT (m_cRef != 0);

    ULONG cRef = --m_cRef;
    
    if (0 == cRef)
        delete this;

    return cRef;
}

////////////////////////////////////////////////////////////////////////////////
//
// IChannelMgr member(s)
//
////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CChannelMgr::AddCategory(CHANNELCATEGORYINFO *pCategoryInfo)
{
    ASSERT(pCategoryInfo);
    if (!pCategoryInfo || pCategoryInfo->cbSize < sizeof(CHANNELCATEGORYINFO))
    {
        return E_INVALIDARG;
    }
        
    //
    // Convert all the wide str params to tstrs
    //
    LPWSTR pwszURL   = pCategoryInfo->pszURL;
    LPWSTR pwszTitle = pCategoryInfo->pszTitle;
    LPWSTR pwszLogo  = pCategoryInfo->pszLogo;
    LPWSTR pwszIcon  = pCategoryInfo->pszIcon;
    LPWSTR pwszWideLogo  = pCategoryInfo->pszWideLogo;

    //
    // REVIEW:is this too much to alloc on the stack?
    // 
    TCHAR szURL[INTERNET_MAX_URL_LENGTH];
    TCHAR szTitle[MAX_PATH];
    TCHAR szLogo[INTERNET_MAX_URL_LENGTH];
    TCHAR szIcon[INTERNET_MAX_URL_LENGTH];
    TCHAR szWideLogo[INTERNET_MAX_URL_LENGTH];

    LPTSTR pszURL   = NULL;
    LPTSTR pszTitle = NULL;
    LPTSTR pszLogo  = NULL;
    LPTSTR pszIcon  = NULL;
    LPTSTR pszWideLogo  = NULL;

    if (pwszTitle)
    {
        SHUnicodeToTChar(pwszTitle, szTitle, MAX_PATH);
        pszTitle = szTitle;
    }
    else
    {
        return E_INVALIDARG; // required option
    }

    if (pwszURL)
    {
        SHUnicodeToTChar(pwszURL, szURL, INTERNET_MAX_URL_LENGTH);
        pszURL = szURL;
    }

    if (pwszLogo)
    {
        SHUnicodeToTChar(pwszLogo, szLogo,INTERNET_MAX_URL_LENGTH);
        pszLogo = szLogo;
    }

    if (pwszIcon)
    {
        SHUnicodeToTChar(pwszIcon, szIcon,INTERNET_MAX_URL_LENGTH);
        pszIcon = szIcon;
    }

    if (pwszWideLogo)
    {
        SHUnicodeToTChar(pwszWideLogo, szWideLogo,INTERNET_MAX_URL_LENGTH);
        pszWideLogo = szWideLogo;
    }

    //
    // Find the Channel directory
    // Attempt to create folder if one doesn't exist
    //
    TCHAR szPath[MAX_PATH];
    if (FAILED(Channel_GetFolder(szPath, DOC_CHANNEL)))
    {
        return E_FAIL;  // couldn't find Channel Folder or create empty one
    }

    // Convert the title into a path component
    TCHAR szFileTitle[MAX_PATH];
    szFileTitle[0] = TEXT('\0');
    PathCombineCleanPath(szFileTitle, pszTitle);

    TraceMsg(TF_GENERAL, "AddCategory(): pszTitle = %s, szFileTitle = %s", pszTitle, szFileTitle);

    //
    // add title to channel folder path
    //
    PathCombine(szPath, szPath, szFileTitle);

    //
    // Create the logoized, iconized, webviewed special folder
    // REVIEW public ChanMgr api doesn't handle iconIndex yet. Should fix!
    //

    //
    // REVIEW REVIEW HACK HACK BUGBUG and TABS as well :-P
    // this is not clean or elegant
    // we only work if the incoming URL is infact a UNC
    // and we copy the file to the Category Folder
    //
    if (pszURL)
    {
        TCHAR szTargetPath[MAX_PATH];
        LPTSTR pszFilename = PathFindFileName(pszURL);

        //
        // Create folder, webview htm is just filename no path.
        //
        Channel_CreateSpecialFolder(szPath, pszFilename, pszLogo, pszWideLogo, pszIcon, 0);

        //
        // Now build target fully qualified path to use to copy html file
        //
        PathCombine(szTargetPath, szPath, pszFilename);
        
        //
        // Copy html into category folder and mark it hidden
        if (!CopyFile(pszURL, szTargetPath, FALSE))
        {
            // If the copy fails, try again after clearing the attributes.
            SetFileAttributes(szTargetPath, FILE_ATTRIBUTE_NORMAL);
            CopyFile(pszURL, szTargetPath, FALSE);
        }
        SetFileAttributes(szTargetPath, FILE_ATTRIBUTE_HIDDEN);
    }
    else
        Channel_CreateSpecialFolder(szPath, NULL, pszLogo, pszWideLogo, pszIcon, 0);

    //
    // Place the channel category in the appropriate "order".
    //
    Channel_OrderItem(szPath);

    //
    // Notify the system that a new item has been added.
    //
    SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, (void*)szPath, NULL);

    return S_OK;
}

//
// DeleteCategory 
//
STDMETHODIMP CChannelMgr::DeleteCategory(LPWSTR pwzTitle)
{  
    TCHAR szTitle[INTERNET_MAX_URL_LENGTH];

    SHUnicodeToTChar(pwzTitle, szTitle, INTERNET_MAX_URL_LENGTH);

    //
    // REVIEW: deletegate to just deleting the channel ok???
    //
    return ::DeleteChannel(szTitle);
}

STDMETHODIMP CChannelMgr::AddChannelShortcut(CHANNELSHORTCUTINFO *pChannelInfo)
{
    if (!pChannelInfo || 
        pChannelInfo->cbSize < sizeof(CHANNELSHORTCUTINFO) ||
        pChannelInfo->pszURL == NULL ||
        pChannelInfo->pszTitle == NULL)
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    TCHAR szURL[INTERNET_MAX_URL_LENGTH];
    TCHAR szTitle[INTERNET_MAX_URL_LENGTH];
    TCHAR szLogo[INTERNET_MAX_URL_LENGTH];
    TCHAR szIcon[INTERNET_MAX_URL_LENGTH];
    TCHAR szWideLogo[INTERNET_MAX_URL_LENGTH];

    LPTSTR pszLogo = NULL;
    LPTSTR pszIcon = NULL;
    LPTSTR pszWideLogo = NULL;

    //
    // Convert BSTRs to TSTRs
    //
    SHUnicodeToTChar(pChannelInfo->pszURL,   szURL,   INTERNET_MAX_URL_LENGTH);
    SHUnicodeToTChar(pChannelInfo->pszTitle, szTitle, INTERNET_MAX_URL_LENGTH);

    //
    // Now handle optional arguments
    //
    if (pChannelInfo->pszLogo != NULL)
    {
        SHUnicodeToTChar(pChannelInfo->pszLogo, szLogo, INTERNET_MAX_URL_LENGTH);
        pszLogo = szLogo;
    }

    if (pChannelInfo->pszWideLogo != NULL)
    {
        SHUnicodeToTChar(pChannelInfo->pszWideLogo, szWideLogo, INTERNET_MAX_URL_LENGTH);
        pszWideLogo = szWideLogo;
    }

    if (pChannelInfo->pszIcon != NULL)
    {   
        SHUnicodeToTChar(pChannelInfo->pszIcon, szIcon, INTERNET_MAX_URL_LENGTH);
        pszIcon = szIcon;
    }

    return ::AddChannel(szTitle, szURL, pszLogo, pszWideLogo, pszIcon,
        pChannelInfo->bIsSoftware ? DOC_SOFTWAREUPDATE : DOC_CHANNEL);
}

STDMETHODIMP CChannelMgr::DeleteChannelShortcut(LPWSTR pwzTitle)
{
    TCHAR szTitle[INTERNET_MAX_URL_LENGTH];

    SHUnicodeToTChar(pwzTitle, szTitle, INTERNET_MAX_URL_LENGTH);

    return ::DeleteChannel(szTitle);
}


STDMETHODIMP CChannelMgr::EnumChannels(DWORD dwEnumFlags, LPCWSTR pszURL,
                                       IEnumChannels** ppIEnumChannels)
{
    *ppIEnumChannels = (IEnumChannels*) new CChannelEnum(dwEnumFlags, pszURL);

    return *ppIEnumChannels ? S_OK : E_OUTOFMEMORY;
}


//
// IChannelMgrPriv
//

STDMETHODIMP CChannelMgr::GetBaseChannelPath(LPSTR pszPath, int cch)
{
    ASSERT(pszPath || 0 == cch);
    HRESULT	hr;

#ifdef UNICODE
    TCHAR	tszPath[MAX_PATH];

    hr = Channel_GetBasePath(tszPath, MAX_PATH);
    SHTCharToAnsi(tszPath, pszPath, ARRAYSIZE(tszPath));

#else
    hr = Channel_GetBasePath(pszPath, cch);
#endif
    return hr;
}

STDMETHODIMP CChannelMgr::GetChannelFolderPath (LPSTR pszPath, int cch,
                                                CHANNELFOLDERLOCATION cflChannel)
{
    ASSERT (pszPath || 0 == cch);

    XMLDOCTYPE xdt;
    switch (cflChannel)
    {
    case CF_CHANNEL:
        xdt = DOC_CHANNEL;
        break;
    case CF_SOFTWAREUPDATE:
        xdt = DOC_SOFTWAREUPDATE;
        break;
    default:
        return E_INVALIDARG;
    }

    TCHAR   tszPath[MAX_PATH];
    HRESULT hr = Channel_GetFolder(tszPath, xdt);
    if (cch <= StrLen(tszPath))
        return E_FAIL;

#ifdef UNICODE
    SHTCharToAnsi(tszPath, pszPath, ARRAYSIZE(tszPath));
#else
    StrCpy(pszPath, tszPath);
#endif
    return S_OK;
}

STDMETHODIMP CChannelMgr::GetChannelFolder (LPITEMIDLIST* ppidl,
                                            CHANNELFOLDERLOCATION cflChannel)
{
    if (ppidl == NULL)
        return E_FAIL;

    char szPath[MAX_PATH];
    HRESULT hr = GetChannelFolderPath (szPath, ARRAYSIZE(szPath), cflChannel);
    if (FAILED (hr))
        return hr;

#ifdef UNICODE
    TCHAR   tszPath[MAX_PATH];

    SHAnsiToTChar(szPath, tszPath, ARRAYSIZE(tszPath));

    return Channel_CreateILFromPath (tszPath, ppidl);
#else
    return Channel_CreateILFromPath (szPath, ppidl);
#endif
}

STDMETHODIMP CChannelMgr::InvalidateCdfCache(void)
{
    InterlockedIncrement((LONG*)&g_dwCacheCount);
    return S_OK;
}

STDMETHODIMP CChannelMgr::PreUpdateChannelImage(
    LPCSTR pszPath,
    LPSTR pszHashItem,
    int* piIndex,
    UINT* puFlags,
    int* piImageIndex
)
{
#ifdef UNICODE
    TCHAR   tszPath[MAX_PATH];
    TCHAR   tszHashItem[MAX_PATH];

    SHAnsiToTChar(pszPath, tszPath, ARRAYSIZE(tszPath));
    SHAnsiToTChar(pszHashItem, tszHashItem, ARRAYSIZE(tszHashItem));
    return ::PreUpdateChannelImage(tszPath, tszHashItem, piIndex, puFlags,
                                   piImageIndex);
#else
    return ::PreUpdateChannelImage(pszPath, pszHashItem, piIndex, puFlags,
                                   piImageIndex);
#endif
}

STDMETHODIMP CChannelMgr::UpdateChannelImage(
    LPCWSTR pszHashItem,
    int iIndex,
    UINT uFlags,
    int iImageIndex
)
{
    ::UpdateChannelImage(pszHashItem, iIndex, uFlags, iImageIndex);

    return S_OK;
}

STDMETHODIMP CChannelMgr::ShowChannel(
    IWebBrowser2 *pIWebBrowser2,
    LPWSTR pwszURL, 
    HWND hwnd
)
{
    HRESULT hr;
    
    if (!pwszURL || !pIWebBrowser2)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = ::NavigateBrowser(pIWebBrowser2, pwszURL, hwnd);
    }

    return hr;
}

STDMETHODIMP CChannelMgr::IsChannelInstalled(LPCWSTR pwszURL)
{
    return ::Channel_IsInstalled(pwszURL) ? S_OK : S_FALSE;
}

STDMETHODIMP CChannelMgr::AddAndSubscribe(HWND hwnd, LPCWSTR pwszURL, 
                                          ISubscriptionMgr *pSubscriptionMgr)
{
    return AddAndSubscribeEx2(hwnd, pwszURL, pSubscriptionMgr, FALSE);
}

STDMETHODIMP CChannelMgr::AddAndSubscribeEx2(HWND hwnd, LPCWSTR pwszURL, 
                                             ISubscriptionMgr *pSubscriptionMgr, 
                                             BOOL bAlwaysSubscribe)
{
    HRESULT hr;
    
    BSTR bstrPreinstalled = NULL;
    HRESULT hrPreinstalled = IsChannelPreinstalled(pwszURL, &bstrPreinstalled);

    if (hrPreinstalled == S_OK)
    {
        RemovePreinstalledMapping(pwszURL);
    }

    WCHAR wszTitle[MAX_PATH];
    TASK_TRIGGER     tt = {0};
    SUBSCRIPTIONINFO si = {0};
    BOOL fIsSoftware = FALSE;

    si.cbSize       = sizeof(SUBSCRIPTIONINFO);
    si.fUpdateFlags |= SUBSINFO_SCHEDULE;
    si.schedule     = SUBSSCHED_AUTO;
    si.pTrigger     = (LPVOID)&tt;

    hr = DownloadMinCDF(hwnd, pwszURL, wszTitle, ARRAYSIZE(wszTitle), &si, &fIsSoftware);
            
    if (hr == S_OK)
    {

        DWORD dwFlags = 0;
        BOOL bInstalled = Channel_IsInstalled(pwszURL);

        if (bInstalled)
        {
            dwFlags |= CREATESUBS_ACTIVATE | CREATESUBS_FROMFAVORITES;
        }
        else
        {
            dwFlags |= CREATESUBS_ADDTOFAVORITES;
        }

        if (bAlwaysSubscribe || !bInstalled || (hrPreinstalled == S_OK))
        {

            if (!pSubscriptionMgr)
            {
                hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                                      CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr,
                                      (void**)&pSubscriptionMgr);
            }
            else
            {
                pSubscriptionMgr->AddRef();
            }

            if (SUCCEEDED(hr))
            {
                hr = pSubscriptionMgr->CreateSubscription(hwnd, 
                                                          pwszURL, 
                                                          wszTitle,
                                                          dwFlags, 
                                                          SUBSTYPE_CHANNEL, 
                                                          &si);

                //  This will kill the one we may have CoCreated
                pSubscriptionMgr->Release();
            }
        }
    }

    if (hr != S_OK && hrPreinstalled == S_OK && bstrPreinstalled != NULL)
    {
        SetupPreinstalledMapping(pwszURL, bstrPreinstalled);
    }

    SysFreeString(bstrPreinstalled);

    return hr;
}

STDMETHODIMP CChannelMgr::WriteScreenSaverURL(LPCWSTR pwszURL, LPCWSTR pwszScreenSaverURL)
{
    return Channel_WriteScreenSaverURL(pwszURL, pwszScreenSaverURL);
}

STDMETHODIMP CChannelMgr::RefreshScreenSaverURLs()
{
    return Channel_RefreshScreenSaverURLs();
}

STDMETHODIMP CChannelMgr::DownloadMinCDF(HWND hwnd, LPCWSTR pwszURL, 
                                         LPWSTR pwszTitle, DWORD cchTitle, 
                                         SUBSCRIPTIONINFO *pSubInfo, 
                                         BOOL *pfIsSoftware)
{
    HRESULT hr;
    IXMLDocument* pIXMLDocument;
    IXMLElement* pIXMLElement;

    ASSERT(pSubInfo);
    ASSERT(pfIsSoftware);

    *pwszTitle = NULL;

    DLL_ForcePreloadDlls(PRELOAD_MSXML);
    
    hr = CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          IID_IXMLDocument, (void**)&pIXMLDocument);

    if (SUCCEEDED(hr))
    {
        BOOL fStartedOffLine = IsGlobalOffline();
        BOOL fInformUserOfDownloadProblem = FALSE;

        SetGlobalOffline(FALSE);
        if (InternetAutodial(INTERNET_AUTODIAL_FORCE_ONLINE, 0))
        {
            if (!DownloadCdfUI(hwnd, pwszURL, pIXMLDocument))
            {
                hr = E_FAIL;
                fInformUserOfDownloadProblem = TRUE;
            }
            else
            {
                Channel_SendUpdateNotifications(pwszURL);

                LONG lDontCare;
                hr = XML_GetFirstChannelElement(pIXMLDocument, &pIXMLElement, &lDontCare);
                if (SUCCEEDED(hr))
                {
                    *pfIsSoftware = XML_GetDocType(pIXMLDocument) == DOC_SOFTWAREUPDATE;
                    XML_GetSubscriptionInfo(pIXMLElement, pSubInfo);

                    BSTR bstrTitle = XML_GetAttribute(pIXMLElement, XML_TITLE);
                    if (bstrTitle)
                    {
                        if (bstrTitle[0])
                        {
                            StrCpyNW(pwszTitle, bstrTitle, cchTitle);
                        }
                        else
                        {
                            if (StrCpyNW(pwszTitle, PathFindFileNameW(pwszURL), cchTitle))
                            {
                                PathRemoveExtensionW(pwszTitle);
                            }
                            else
                            {
                                hr = S_FALSE;
                            }
                        }
                        SysFreeString(bstrTitle);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    pIXMLElement->Release();
                }
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            fInformUserOfDownloadProblem = TRUE;
        }

        pIXMLDocument->Release();

        if (fStartedOffLine)
        {
            SetGlobalOffline(TRUE);
        }

        if (fInformUserOfDownloadProblem)
        {
            ASSERT(FAILED(hr));

            CDFMessageBox(hwnd, IDS_INFO_MUST_CONNECT, IDS_INFO_DLG_TITLE,
                            MB_OK | MB_ICONINFORMATION);

            //  Set return val to S_FALSE so the caller can distinguish between
            //  errors which we've informed the user of and hard failures 
            //  such as out of memory, etc.

            hr = S_FALSE;
        }
    }
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// *** Channel_RemoveURLMapping ***
//
// Description:
//     Removes the cache and registry settings that wininet uses to map the
//     given url to a local file.
//
////////////////////////////////////////////////////////////////////////////////

#define PRELOAD_REG_KEY \
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Cache\\Preload")

void Channel_RemoveURLMapping(LPCTSTR pszURL)
{
    DWORD cbcei = MAX_CACHE_ENTRY_INFO_SIZE;
    BYTE cei[MAX_CACHE_ENTRY_INFO_SIZE];
    LPINTERNET_CACHE_ENTRY_INFO pcei = (LPINTERNET_CACHE_ENTRY_INFO)cei;

    //
    // Look up the url in the cache
    //
    if (GetUrlCacheEntryInfoEx(pszURL, pcei, &cbcei, NULL, 0, NULL, 0))
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
            SetUrlCacheEntryInfo(pszURL, pcei, CACHE_ENTRY_ATTRIBUTE_FC);

            //
            // Now remove the mapping from the registry
            //
            HKEY hk;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, PRELOAD_REG_KEY, 0, KEY_WRITE,
                             &hk) == ERROR_SUCCESS)
            {
                RegDeleteValue(hk, pszURL);
                RegCloseKey(hk);
            }
        }
    }
}
#ifndef UNICODE
//
// widechar version of above routine
//
void Channel_RemoveURLMapping(LPCWSTR wszURL)
{
    CHAR szURL[INTERNET_MAX_URL_LENGTH];

    if (SHUnicodeToTChar(wszURL, szURL, ARRAYSIZE(szURL)))
    {
        Channel_RemoveURLMapping(szURL);
    }
}
#endif

// cheap lookup function to see if we are dealing with a preinstalled URL
BOOL Channel_CheckURLMapping( LPCWSTR wszURL )
{
    TCHAR szURL[INTERNET_MAX_URL_LENGTH];
    if (SHUnicodeToTChar(wszURL, szURL, ARRAYSIZE(szURL)))
    {
        HKEY hkey;
        if ( RegOpenKeyEx(HKEY_CURRENT_USER, PRELOAD_REG_KEY, 0, KEY_READ, &hkey) == ERROR_SUCCESS )
        {
            // check to see if the value exists for the URL....
            TCHAR szPath[MAX_PATH];
            DWORD cbSize = sizeof(szPath);
            
            LONG lRes = RegQueryValueEx( hkey, szURL, NULL, NULL, (LPBYTE) szPath, &cbSize );
            RegCloseKey( hkey );
            
            return (lRes == ERROR_SUCCESS ); 
        }
    }
    return FALSE;
}


////////////////////////////////////////////////////////////////////////////////
//
// *** CChannelMgr::IsChannelPreinstalled ***
//
// Description:
//     Returns S_OK if the channel url has a preinstalled cache entry
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CChannelMgr::IsChannelPreinstalled(LPCWSTR pwszURL, BSTR * bstrFile)
{
    HRESULT hr = S_FALSE;

    TCHAR szURL[INTERNET_MAX_URL_LENGTH];
    if (SHUnicodeToTChar(pwszURL, szURL, ARRAYSIZE(szURL)))
    {
        DWORD cbcei = MAX_CACHE_ENTRY_INFO_SIZE;
        BYTE cei[MAX_CACHE_ENTRY_INFO_SIZE];
        LPINTERNET_CACHE_ENTRY_INFO pcei = (LPINTERNET_CACHE_ENTRY_INFO)cei;

        //
        // Look up the url in the cache
        //
        if (GetUrlCacheEntryInfoEx(szURL, pcei, &cbcei, NULL, 0, NULL, 0))
        {
            //
            // see if it has a mapping because it is a preinstalled cache entry
            //
            if (pcei->CacheEntryType & INSTALLED_CACHE_ENTRY)
            {
                //
                // Get a BSTR from the internet cache entry local file name
                //
                if (bstrFile)
                {
                    WCHAR wszFile[MAX_PATH];
                    SHTCharToUnicode(pcei->lpszLocalFileName, wszFile, ARRAYSIZE(wszFile));
                    *bstrFile = SysAllocString(wszFile);
                }

                hr = S_OK;
            }
        }
    }

    return hr;
}

STDMETHODIMP CChannelMgr::RemovePreinstalledMapping(LPCWSTR pwszURL)
{
    Channel_RemoveURLMapping(pwszURL);
    return S_OK;
}

STDMETHODIMP CChannelMgr::SetupPreinstalledMapping(LPCWSTR pwszURL, LPCWSTR pwszFile)
{
    FILETIME ftZero = {0};
    CommitUrlCacheEntryW(pwszURL, pwszFile, ftZero, ftZero, INSTALLED_CACHE_ENTRY, NULL, 0, NULL, 0);

    //
    // Make sure that there isn't an in memory cached old version of this URL
    //
#ifdef UNICODE
    Cache_RemoveItem(pwszURL);
#else
    char szURL[INTERNET_MAX_URL_LENGTH];
    SHUnicodeToAnsi(pwszURL, szURL, ARRAYSIZE(szURL));
    Cache_RemoveItem(szURL);
#endif
    return S_OK;
}

//
// Create the special folder that can have a webview associated with it, and an
// icon and a logo view.
//
HRESULT Channel_CreateSpecialFolder(
    LPCTSTR pszPath,    // path to folder to create
    LPCTSTR pszURL,     // url for webview
    LPCTSTR pszLogo,    // [optional] path to logo
    LPCTSTR pszWideLogo,    // [optional] path to wide logo
    LPCTSTR pszIcon,    // [optional] path to icon file
    int     nIconIndex  // index to icon in above file
    )
{
    //
    // First create the directory if it doesn't exist
    //
    if (!PathFileExists(pszPath))
    {
        if (Channel_CreateDirectory(pszPath) != 0)
        {
            return E_FAIL;
        }
    }

    //
    // Mark it as a SYSTEM folder
    //
    if (!SetFileAttributes(pszPath, FILE_ATTRIBUTE_SYSTEM))
        return E_FAIL;

    //
    // Make desktop.ini
    //
    TCHAR szDesktopIni[MAX_PATH];
    PathCombine(szDesktopIni, pszPath, TEXT("desktop.ini"));
    WritePrivateProfileString(NULL, NULL, NULL, szDesktopIni); 

    //
    // Write ConfirmFileOp=0 to turn off shell warnings during file operations
    //
    EVAL(WritePrivateProfileString( 
        TEXT(".ShellClassInfo"),
        TEXT("ConfirmFileOp"),  TEXT("0"), szDesktopIni));

    //
    // Write the URL for this category folders webview
    //
    if (pszURL)
    {
        EVAL(WritePrivateProfileString( 
            TEXT(".ShellClassInfo"),
            TEXT("URL"),  pszURL, szDesktopIni));
    }

    //
    // Write the Logo for this channel if present
    //
    if (pszLogo)
    {
        EVAL(WritePrivateProfileString( 
            TEXT(".ShellClassInfo"),
            TEXT("Logo"),  pszLogo, szDesktopIni));
    }

    //
    // Write the WideLogo for this channel if present
    //
    if (pszWideLogo)
    {
        EVAL(WritePrivateProfileString( 
            TEXT(".ShellClassInfo"),
            TEXT("WideLogo"),  pszWideLogo, szDesktopIni));
    }
    
    //
    // Write the Icon URL for this category folder if present
    //
    if (pszIcon)
    {
        TCHAR szIconIndex[8];                            // can handle 999999
        ASSERT(nIconIndex >= 0 && nIconIndex <= 999999); // sanity check
        wnsprintf(szIconIndex, ARRAYSIZE(szIconIndex), TEXT("%d"), nIconIndex);

        EVAL(WritePrivateProfileString( 
            TEXT(".ShellClassInfo"),
            TEXT("IconIndex"),  szIconIndex, szDesktopIni));

        EVAL(WritePrivateProfileString( 
            TEXT(".ShellClassInfo"),
            TEXT("IconFile"),  pszIcon, szDesktopIni));
    }
    
    //
    // Flush the buffers
    //
    WritePrivateProfileString(NULL, NULL, NULL, szDesktopIni);

    SetFileAttributes(szDesktopIni, FILE_ATTRIBUTE_HIDDEN);

    return S_OK;
}

//
// Create the special channels folder
//

//
// Channel folders are no longer created in IE5+
//
/*
HRESULT Channel_CreateChannelFolder( XMLDOCTYPE xdt )
{
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(Channel_GetFolder(szPath, xdt)) && szPath[0] != 0)
    {
        //
        // Create a special folder with an icon that lives in the cdfview.dll
        //
        return Channel_CreateSpecialFolder(
            szPath, NULL, NULL, NULL, g_szModuleName, -IDI_CHANNELFOLDER);
    }
    else
        return E_FAIL;
}
*/

HRESULT Channel_GetBasePath(LPTSTR pszPath, int cch)
{
    ASSERT(pszPath || 0 == cch);

    HRESULT hr = E_FAIL;

    HKEY hKey;
    DWORD dwLen = cch * sizeof(TCHAR);

    if (RegOpenKey(HKEY_CURRENT_USER, SHELLFOLDERS, &hKey) == ERROR_SUCCESS)
    {
#ifndef UNIX
        if (RegQueryValueEx(hKey, TEXT("Favorites"), NULL, NULL,
                            (LPBYTE)pszPath, &dwLen) == ERROR_SUCCESS)
        {
            hr = S_OK;
        }
#else
        TCHAR szUnexpandPath[MAX_PATH+1];
        dwLen = MAX_PATH;
        if (RegQueryValueEx(hKey, TEXT("Favorites"), NULL, NULL,
                            (LPBYTE)szUnexpandPath, &dwLen) == ERROR_SUCCESS)
        {
            hr = S_OK;
        }
        DWORD length = ExpandEnvironmentStrings((LPTSTR)szUnexpandPath,
                                                (LPTSTR)pszPath,
                                                cch);
        if (length == 0 || length > cch)
            hr = E_FAIL;
#endif /* UNIX */
        RegCloseKey(hKey);
    }
    
    return hr;
}

BSTR Channel_GetFullPath(LPCWSTR pwszName)
{
    ASSERT(pwszName);

    BSTR bstrRet = NULL;

    TCHAR szName[MAX_PATH];
    if (SHUnicodeToTChar(pwszName, szName, ARRAYSIZE(szName)))
    {
        TCHAR szPath[MAX_PATH];

        if (SUCCEEDED(Channel_GetFolder(szPath, DOC_CHANNEL)))
        {
            if (PathCombineCleanPath(szPath, szName))
            {
                WCHAR wszPath[MAX_PATH];

                if (SHTCharToUnicode(szPath, wszPath, ARRAYSIZE(wszPath)))
                {
                    bstrRet = SysAllocString(wszPath);
                }
            }
        }
    }

    return bstrRet;
}

    
HRESULT Channel_GetFolder(LPTSTR pszPath, XMLDOCTYPE xdt )
{
    TCHAR   szFavs[MAX_PATH];
    TCHAR   szChannel[MAX_PATH];
    ULONG    cbChannel = sizeof(szChannel);
    HRESULT hr = E_FAIL;

    if (SUCCEEDED(Channel_GetBasePath(szFavs, ARRAYSIZE(szFavs))))
    {
        switch (xdt)
        {
            case DOC_CHANNEL:
                //
                // Get the potentially localized name of the Channel folder from the
                // registry if it is there.  Otherwise just read it from the resource.
                // Then tack this on the favorites path.
                //

                if (ERROR_SUCCESS != SHRegGetUSValue(L"Software\\Microsoft\\Windows\\CurrentVersion",
                                                     L"ChannelFolderName", NULL, (void*)szChannel,
                                                     &cbChannel, TRUE, NULL, 0))
                {
                    MLLoadString(IDS_CHANNEL_FOLDER, szChannel, ARRAYSIZE(szChannel));
                }
                break;

            case DOC_SOFTWAREUPDATE:
                MLLoadString(IDS_SOFTWAREUPDATE_FOLDER, szChannel, ARRAYSIZE(szChannel));
                break;
        }

        PathCombine(pszPath, szFavs, szChannel);

        //
        // If the channels folder doesn't exist create it now
        // 
        if (!PathFileExists(pszPath))
        {
            //
            // In IE5+ use the favorites folder if the channels folder does not
            // exist.
            //

            StrCpy(pszPath, szFavs);

            //
            // Create special folder with an icon that lives in cdfview.dll
            //
            /*
            Channel_CreateSpecialFolder(
                pszPath, NULL, NULL, NULL, g_szModuleName, -IDI_CHANNELFOLDER);
            */
        }

        hr = S_OK;
    }
    return hr;
}

// NOTE: This registry location and name is used by webcheck as well so do not
// change it here without updating webcheck.
const TCHAR c_szRegKeyWebcheck[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Webcheck");
const TCHAR c_szRegValueChannelGuide[] = TEXT("ChannelGuide");

HRESULT Channel_GetChannelGuide(LPTSTR pszPath, int cch)
{
    ASSERT(pszPath || 0 == cch);

    HRESULT hr = E_FAIL;

    HKEY hKey;
    DWORD dwLen = cch * sizeof(TCHAR);

    if (RegOpenKey(HKEY_CURRENT_USER, c_szRegKeyWebcheck, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hKey, c_szRegValueChannelGuide, NULL, NULL,
                            (LPBYTE)pszPath, &dwLen) == ERROR_SUCCESS)
        {
            hr = S_OK;
        }

        RegCloseKey(hKey);
    }
    
    return hr;
}

//
// Channel_OrderChannel - Set the order of the new channel in the folder
//
void Channel_OrderItem(LPCTSTR szPath)
{
    HRESULT hr;
    BOOL bRet;
    LPITEMIDLIST pidlParent = NULL, pidlChild = NULL;
    IShellFolder *psfDesktop = NULL;
    IShellFolder *psfParent = NULL;
    IPersistFolder *pPF = NULL;
    IOrderList *pOL = NULL;
    HDPA hdpa = NULL;
    int iChannel = -1;  // Pick a negative index to see if we find it.
    int iInsert = 0;    // Assume we don't find the channel guide.
    PORDERITEM poi;

    // Get the full pidl of the new channel (ignore the pidlParent name)
    hr = Channel_CreateILFromPath(szPath, &pidlParent);
    if (FAILED(hr))
        goto cleanup;

    // Allocate a child pidl and make the parent pidl
    pidlChild = ILClone(ILFindLastID(pidlParent));
    if (!pidlChild)
        goto cleanup;
    bRet = ILRemoveLastID(pidlParent);
    ASSERT(bRet);

    // Get the IShellFolder of the parent by going through the
    // desktop folder.
    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED(hr))
        goto cleanup;
    hr = psfDesktop->BindToObject(pidlParent, NULL, IID_IShellFolder, (void**)&psfParent);
    if (FAILED(hr))
        goto cleanup;

    // Get the order list of the parent.
    hr = CoCreateInstance(CLSID_OrderListExport, NULL, CLSCTX_INPROC_SERVER, IID_IPersistFolder, (void**)&pPF);
    if (FAILED(hr))
        goto cleanup;
    hr = pPF->Initialize(pidlParent);
    if (FAILED(hr))
        goto cleanup;
    hr = pPF->QueryInterface(IID_IOrderList, (void**)&pOL);
    if (FAILED(hr))
        goto cleanup;
    hr = pOL->GetOrderList(&hdpa);

    // Create a DPA list if there wasn't one already.
    if (!hdpa)
    {
        hdpa = DPA_Create(2);
        if (!hdpa)
            goto cleanup;
    }
    else
    {
        // First, get the channel guide pidl.
        TCHAR szGuide[MAX_PATH];
        WCHAR wzGuide[MAX_PATH];
        LPITEMIDLIST pidlGuide = NULL;
        if (SUCCEEDED(Channel_GetChannelGuide(szGuide, sizeof(szGuide))))
        {
            if (SHTCharToUnicode(szGuide, wzGuide, ARRAYSIZE(wzGuide)))
            {
                ULONG ucch;
                hr = psfParent->ParseDisplayName(NULL, NULL, wzGuide, &ucch, &pidlGuide, NULL);
                ASSERT(!pidlGuide || SUCCEEDED(hr));
            }
        }

        // Now do the search.
        // Check to see if the channel is in the DPA list.
        // Check to see if the channel guide is there and first.
        int i = 0;
        poi = (PORDERITEM)DPA_GetPtr(hdpa, i);
        while (poi)
        {
            if (!psfParent->CompareIDs(0, pidlChild, poi->pidl))
                iChannel = poi->nOrder;
            if (pidlGuide && !psfParent->CompareIDs(0, pidlGuide, poi->pidl) && (poi->nOrder == 0))
                iInsert = 1;
            i++;
            poi = (PORDERITEM)DPA_GetPtr(hdpa, i);
        }
    }

    // If the channel pidl was not found, insert it at the end
    if (iChannel < 0)
    {
        // Allocate an order item to insert
        hr = pOL->AllocOrderItem(&poi, pidlChild);
        if (SUCCEEDED(hr))
        {
            iChannel = DPA_InsertPtr(hdpa, 0x7fffffff, poi);
            if (iChannel >= 0)
                poi->nOrder = iChannel;
        }
    }

    // Reorder the channels.  The new channel is in the list at
    // position iChannel.  We're moving it to position iInsert.
    if (iChannel >= 0)
    {
        int i = 0;
        poi = (PORDERITEM)DPA_GetPtr(hdpa, i);
        while (poi)
        {
            if (poi->nOrder == iChannel && iChannel >= iInsert)
                poi->nOrder = iInsert;
            else if (poi->nOrder >= iInsert && poi->nOrder < iChannel)
                poi->nOrder++;
            
            i++;
            poi = (PORDERITEM)DPA_GetPtr(hdpa, i);
        }

        // Finally, set the order.
        hr = pOL->SetOrderList(hdpa, psfParent);
        ASSERT(SUCCEEDED(hr));
    }

cleanup:
    if (hdpa)
        pOL->FreeOrderList(hdpa);
    if (pOL)
        pOL->Release();
    if (pPF)
        pPF->Release();
    if (psfParent)
        psfParent->Release();
    if (psfDesktop)
        psfDesktop->Release();
    ILFree(pidlParent);    // NULL is OK.
    ILFree(pidlChild);
}

//
// AddChannel - Add a channel
//
HRESULT AddChannel(
    LPCTSTR pszName, 
    LPCTSTR pszURL, 
    LPCTSTR pszLogo, 
    LPCTSTR pszWideLogo, 
    LPCTSTR pszIcon,
    XMLDOCTYPE xdt )
{
    HRESULT hr = S_OK;
    BOOL fDirectoryAlreadyExisted = FALSE;
    
    //
    // Find the Channel directory
    // Attempt to create folder if one doesn't exist
    //
    TCHAR szPath[MAX_PATH];
    if (FAILED(Channel_GetFolder(szPath, xdt)))
    {
        return E_FAIL;  // couldn't find Channel Folder or create empty one
    }

    // Cleanup each of the path components independently.
    PathCombineCleanPath(szPath, pszName);
    TraceMsg(TF_GENERAL, "Channel Path = %s", szPath);

    //
    // Make the new folder if it doesn't already exist
    //
    if (!PathFileExists(szPath))
    {
        if (Channel_CreateDirectory(szPath) != 0)
        {
            return E_FAIL;
        }
    }
    else
    {
        fDirectoryAlreadyExisted = TRUE;
    }

    //
    // Mark it as a SYSTEM folder
    //
    if (!SetFileAttributes(szPath, FILE_ATTRIBUTE_SYSTEM))
        return E_FAIL;

    //
    // Add the channel to the registry database of all channels.
    //

    //if (FAILED(Reg_WriteChannel(szPath, pszURL)))
    //    return E_FAIL;

    //
    // Place the channel folder in the appropriate "order".
    //
    Channel_OrderItem(szPath);
    
    // Build the CDFINI GUID string
    //
    TCHAR szCDFViewGUID[GUID_STR_LEN];
    SHStringFromGUID(CLSID_CDFINI, szCDFViewGUID, ARRAYSIZE(szCDFViewGUID));

    //
    // Make desktop.ini
    //
    PathCombine(szPath, szPath, TEXT("desktop.ini"));
    TraceMsg(TF_GENERAL, "INI path = %s", szPath);
    WritePrivateProfileString(NULL, NULL, NULL, szPath); // Create desktop.ini

    //
    // Write in the CDFViewer GUID
    //
    EVAL(WritePrivateProfileString( 
        TEXT(".ShellClassInfo"),
        TEXT("CLSID"),  szCDFViewGUID, szPath));

    //
    // Write ConfirmFileOp=0 to turn off shell warnings during file operations
    //
    EVAL(WritePrivateProfileString( 
        TEXT(".ShellClassInfo"),
        TEXT("ConfirmFileOp"),  TEXT("0"), szPath));

    //
    // Write the actual URL for this channel
    //
    EVAL(WritePrivateProfileString( 
        TEXT("Channel"),
        TEXT("CDFURL"),  pszURL, szPath));


    Channel_GetAndWriteScreenSaverURL(pszURL, szPath);

    //
    // Write the default Logo URL for this channel if present
    //
    if (pszLogo)
    {
        EVAL(WritePrivateProfileString( 
            TEXT("Channel"),
            TEXT("Logo"),  pszLogo, szPath));
    }

    //
    // Write the default WideLogo URL for this channel if present
    //
    if (pszWideLogo)
    {
        EVAL(WritePrivateProfileString( 
            TEXT("Channel"),
            TEXT("WideLogo"),  pszWideLogo, szPath));
    }

    //
    // Write the default Icon URL for this channel if present
    //
    if (pszIcon)
    {
        EVAL(WritePrivateProfileString( 
            TEXT("Channel"),
            TEXT("Icon"),  pszIcon, szPath));
    }
    
    //
    // Flush the buffers
    //
    WritePrivateProfileString(NULL, NULL, NULL, szPath); // Create desktop.ini

    EVAL(SetFileAttributes(szPath, FILE_ATTRIBUTE_HIDDEN));

    //
    // Notify the system that a new item has been added, or if the item 
    // already existed just send an UPDATEDIR notification
    //
    PathRemoveFileSpec(szPath);
    if (fDirectoryAlreadyExisted)
    {
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, (void*)szPath, NULL);
    }
    else
    {
        SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, (void*)szPath, NULL);
    }

    return S_OK;
}

//
// DeleteChannel - Deletes a channel by name
//
// Returns 
//     S_OK if channel existed and successfully deleted
//     S_FALSE if channel didn't exist.
//     E_FAIL else.
//
HRESULT DeleteChannel(LPTSTR szName)
{
    TCHAR szFolderPath[MAX_PATH];
    TCHAR szDesktopIniPath[MAX_PATH];
        
    if (PathIsRelative(szName))
    {
        //
        // Find the Channel directory
        // Note don't create if doesn't exist
        //
        // BUGBUG - this won't find app channels.
        if (FAILED(Channel_GetFolder(szFolderPath, DOC_CHANNEL)))
        {
            return S_FALSE;  // couldn't find Channel Folder so channel can't exist
        }

        // Cleanup each of the path components independently.
        PathCombineCleanPath(szFolderPath, szName);
        TraceMsg(TF_GENERAL, "Delete Channel Path = %s", szFolderPath);

    }
    else
    {
        // Assume absolute paths were retrieved by enumeration and
        // therefore don't need to be "cleaned".
        StrCpyN(szFolderPath, szName, ARRAYSIZE(szFolderPath));
    }

    // Create the desktop.ini path
    PathCombine(szDesktopIniPath, szFolderPath, TEXT("desktop.ini"));

    // Find URL to CDF from desktop.ini
    TCHAR szCDFURL[INTERNET_MAX_URL_LENGTH];
    GetPrivateProfileString(TEXT("Channel"), TEXT("CDFURL"), TEXT(""), szCDFURL, ARRAYSIZE(szCDFURL), szDesktopIniPath);

    // Remove the URL from the cache preload registry key
    HKEY hkeyPreload;
    LONG lRet = RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegKeyCachePreload, 0, KEY_WRITE, &hkeyPreload);
    if (ERROR_SUCCESS == lRet)
    {
        lRet = RegDeleteValue(hkeyPreload, szCDFURL);
        lRet = RegCloseKey(hkeyPreload);
        ASSERT(ERROR_SUCCESS == lRet);
    }

    if  (
        !DeleteFile(szDesktopIniPath)
        ||
        !SetFileAttributes(szFolderPath, FILE_ATTRIBUTE_NORMAL)
        ||
        //
        // REVIEW: should change this to delete all contents of channel folder
        // incase other files get stored there in the future.
        //
        !RemoveDirectory(szFolderPath)
        )
    {
        return S_FALSE;
    }

    return S_OK;
}

//
// CountChannels - Counts the number of channels
//
// Returns the count
//
DWORD CountChannels(void)
{
    DWORD cChannels = 0;
    IEnumChannels *pEnum = (IEnumChannels *) new CChannelEnum(CHANENUM_CHANNELFOLDER, NULL);
    if (pEnum)
    {
        CHANNELENUMINFO cei;
        while (S_OK == pEnum->Next(1, &cei, NULL))
        {
            cChannels++;
        }
        pEnum->Release();
    }

    return cChannels;
}


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** OpenChannel ***
//
//
// Description:
//     Opens a new browser and selects the given channel
//
// Parameters:
//     [In]  hwndParent - The owner hwnd.
//     [In]  hinst      - The hinstance for this process.
//     [In]  pszCmdLine - The local path to the cdf file.  This will be the path
//                        to the file in the cache if the cdf came from the net.
//     [In]  nShow      - ShowWindow parameter.
//
// Return:
//     None.
//
// Comments:
//     This is the implementation for the context menu "Open Channel" command.
//     It gets invoke via RunDll32.exe.
//
////////////////////////////////////////////////////////////////////////////////
EXTERN_C
STDAPI_(void)
OpenChannel(
    HWND hwndParent,
    HINSTANCE hinst,
    LPSTR pszCmdLine,
    int nShow
)
{
    WCHAR wszPath[INTERNET_MAX_URL_LENGTH];

    if (MultiByteToWideChar(CP_ACP, 0, pszCmdLine, -1, wszPath,
                            ARRAYSIZE(wszPath)))
    {
        OpenChannelHelper(wszPath, hwndParent);
    }

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Subscribe ***
//
//
// Description:
//     Takes the given cdf through the user subscription path.
//
// Parameters:
//     [In]  hwndParent - The owner hwnd.
//     [In]  hinst      - The hinstance for this process.
//     [In]  pszCmdLine - The local path to the cdf file.  This will be the path
//                        to the file in the cache if the cdf came from the net.
//     [In]  nShow      - ShowWindow parameter.
//
// Return:
//     None.
//
// Comments:
//     This is the implementation of the context menu "Subscribe" command.  It
//     gets invoke via rundll32.exe.
//
//     This function handles channel and desktop component cdfs.
//
////////////////////////////////////////////////////////////////////////////////
EXTERN_C
STDAPI_(void)
Subscribe(
    HWND hwndParent,
    HINSTANCE hinst,
    LPSTR pszCmdLine,
    int nShow
)
{
    WCHAR wszPath[INTERNET_MAX_URL_LENGTH];

    if (MultiByteToWideChar(CP_ACP, 0, pszCmdLine, -1, wszPath,
                            ARRAYSIZE(wszPath)))
    {
        SubscribeToCDF(hwndParent, wszPath, STC_ALL);
    }

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** ParseDesktopComponent ***
//
//
// Description:
//     Fills an information structure for a desktop component and optionally
//     allows the user to subscribe to the component.
//
// Parameters:
//     [In]  hwndOwner - The owner hwnd.  If NULL there is no attempt to
//                       subscribe to this URL.
//     [In]  wszURL    - The URL to the desktop component cdf.
//     [Out] pInfo     - A pointer that receives desktop component information
//                       that is read from the cdf.
//
// Return:
//     S_OK if the desktop info could be read.
//     S_FALSE if the user hits cancel
//     E_FAIL otherwise.
//
// Comments:
//     This function is used by the desktop component property pages.  It may
//     create a subscription to the component but unlike Subscribe, it will
//     not add a desktop component to the system.  Creating the component is
//     left to the desktop component property pages.
//
////////////////////////////////////////////////////////////////////////////////
EXTERN_C
STDAPI
ParseDesktopComponent(
    HWND hwndOwner,
    LPWSTR wszURL,
    COMPONENT*  pInfo
)
{
    HRESULT hr;

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        CCdfView* pCCdfView = new CCdfView;

        if (pCCdfView)
        {
            hr = pCCdfView->Load(wszURL, 0);

            if (SUCCEEDED(hr))
            {
                IXMLDocument* pIXMLDocument;

                TraceMsg(TF_CDFPARSE, "ParseDesktopComponent");

                TCHAR szFile[MAX_PATH];
                TCHAR szURL[INTERNET_MAX_URL_LENGTH];

                SHUnicodeToTChar(wszURL, szURL, ARRAYSIZE(szURL));

                hr = URLDownloadToCacheFile(NULL, szURL, szFile,
                                            ARRAYSIZE(szFile), 0, NULL);

                if (SUCCEEDED(hr))
                {
                    hr = pCCdfView->ParseCdf(NULL, &pIXMLDocument, PARSE_LOCAL);

                    if (SUCCEEDED(hr))
                    {
                        ASSERT(pIXMLDocument);

                        if (DOC_DESKTOPCOMPONENT == XML_GetDocType(pIXMLDocument))
                        {
                            BOOL fOk = FALSE;

                            if (hwndOwner)
                            {
                                fOk = SubscriptionHelper(pIXMLDocument, hwndOwner,
                                                   SUBSTYPE_DESKTOPCHANNEL,
                                                   SUBSACTION_SUBSCRIBEONLY,
                                                   wszURL, DOC_DESKTOPCOMPONENT, NULL);
                            }

                            if (fOk)
                            {
                                if(SUCCEEDED(hr = XML_GetDesktopComponentInfo(pIXMLDocument, pInfo)) &&
                                    !pInfo->wszSubscribedURL[0])
                                {
                                    // Since XML_GetDesktopComponentInfo did not fillout the SubscribedURL
                                    // field (because the CDF file didn't have a SELF tag), we need to 
                                    // fill it with what was really subscribed to (the URL for CDF file itself)
                                    StrCpyNW(pInfo->wszSubscribedURL, wszURL, ARRAYSIZE(pInfo->wszSubscribedURL));
                                }
                            }
                            else
                                hr = S_FALSE;
                        }
                        else
                        {
                            hr = E_FAIL;
                        }

                        pIXMLDocument->Release();
                    }
                }
            }

            pCCdfView->Release();
        }
    }
    
    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** SubscribeToCDF ***
//
//
// Description:
//     Subscribes to the given cdf.
//
// Parameters:
//     [In]  hwndOwner - The owner hwnd.  Used to display the subscription
//                       wizard.
//     [In]  wszURL    - An url to the cdf.
//     [In]  dwFlags   - The type of cdf the caller wants to subscribe to.
//                       STC_CHANNEL, STC_DESKTOPCOMPONENT or STC_ALL.
//
// Return:
//     S_OK if the subscription worked and the user subscribed.
//     S_FALSE if the subscription UI appeared but the user decided not to
//             subscribe.
//     E_INVALIDARG if the cdf couldn't be opened or parsed.
//     E_ACCESSDENIED if the cdf file doesn't match the type specified in
//                    dwFlags.
//     E_FAIL on any other errors.
//
// Comments:
//     Private API.  Currently called by desktop component drop handler.
//
////////////////////////////////////////////////////////////////////////////////
EXTERN_C
STDAPI
SubscribeToCDF(
    HWND hwndOwner,
    LPWSTR wszURL,
    DWORD dwFlags
)
{
    HRESULT hr;

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        CCdfView* pCCdfView = new CCdfView;

        if (pCCdfView)
        {
            hr = pCCdfView->Load(wszURL, 0);

            if (SUCCEEDED(hr))
            {
                IXMLDocument* pIXMLDocument;

                TraceMsg(TF_CDFPARSE, "SubscribeToCDF");

                hr = pCCdfView->ParseCdf(NULL, &pIXMLDocument, PARSE_LOCAL);

                if (SUCCEEDED(hr))
                {
                    ASSERT(pIXMLDocument);

                    XMLDOCTYPE xdt = XML_GetDocType(pIXMLDocument);

                    switch(xdt)
                    {
                    case DOC_CHANNEL:
                    case DOC_SOFTWAREUPDATE:
                        // Admins can disallow adding channels and limit
                        // the number of installed channels.
                        if ((dwFlags & STC_CHANNEL) &&
                            !SHRestricted2W(REST_NoAddingChannels, wszURL, 0) &&
                            (!SHRestricted2W(REST_MaxChannelCount, NULL, 0) ||
                            (CountChannels() < SHRestricted2W(REST_MaxChannelCount, NULL, 0))))
                        {
                            if (SubscriptionHelper(pIXMLDocument, hwndOwner,
                                            SUBSTYPE_CHANNEL,
                                            SUBSACTION_ADDADDITIONALCOMPONENTS,
                                            wszURL, xdt, NULL))
                            {
                                OpenChannelHelper(wszURL, hwndOwner);

                                hr = S_OK;
                            }
                            else
                            {
                                hr = S_FALSE;
                            }

                        }
                        else
                        {
                            hr = E_ACCESSDENIED;
                        }
                        break;

                    case DOC_DESKTOPCOMPONENT:
#ifndef UNIX
                        if (hwndOwner &&
                            (WhichPlatform() != PLATFORM_INTEGRATED))
#else
                        /* No Active Desktop on Unix */
                        if (0)
#endif /* UNIX */
                        {
                            TCHAR szText[MAX_PATH];
                            TCHAR szTitle[MAX_PATH];

                            MLLoadString(IDS_BROWSERONLY_DLG_TEXT, 
                                       szText, ARRAYSIZE(szText)); 
                            MLLoadString(IDS_BROWSERONLY_DLG_TITLE,
                                       szTitle, ARRAYSIZE(szTitle));

                            MessageBox(hwndOwner, szText, szTitle, MB_OK); 
                        }
                        else if (dwFlags & STC_DESKTOPCOMPONENT)
                        {
                            if (SubscriptionHelper(pIXMLDocument, hwndOwner,
                                            SUBSTYPE_DESKTOPCHANNEL,
                                            SUBSACTION_ADDADDITIONALCOMPONENTS,
                                            wszURL,
                                            DOC_DESKTOPCOMPONENT, NULL))
                            {
                                hr = S_OK;
                            }
                            else
                            {
                                hr = S_FALSE;
                            }
                        }
                        else
                        {
                            hr = E_ACCESSDENIED;
                        }
                        break;

                    case DOC_UNKNOWN:
                        //If it is NOT a cdfFile, then we get DOC_UNKNOWN. We must return error
                        // here sothat the caller knows that this is NOT a cdffile.
                        hr = E_INVALIDARG;
                        break;
                        
                    default:
                        break;
                    }

                    pIXMLDocument->Release();
                }
                else
                {
                    hr = E_INVALIDARG;
                }
            }

            pCCdfView->Release();
        }

        CoUninitialize();
    }

    return hr;
}

//
// Utility functions.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** OpenChannelHelper ***
//
//
// Description:
//     Opens the browser with the wszURL channel selected.
//
// Parameters:
//     [In]  wszURL    - The URL of the cdf file to display.
//     [In]  hwndOwner - The owning hwnd for error messages.  Can be NULL.
//
// Return:
//     S_OK if the browser was opened.
//     E_FAIL otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
OpenChannelHelper(
    LPWSTR wszURL,
    HWND hwndOwner
)
{
    //
    // REVIEW:  Handle non-channel cdfs.
    //

    HRESULT hr;

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        IWebBrowser2* pIWebBrowser2;

        hr = CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_LOCAL_SERVER,
                              IID_IWebBrowser2, (void**)&pIWebBrowser2);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIWebBrowser2);
            //
            // Navigate to the root url of the cdf ref'd in wszURL
            //
            hr = NavigateBrowser(pIWebBrowser2, wszURL, hwndOwner);

            pIWebBrowser2->Release();
        }

        CoUninitialize();
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// *** ShowChannelPane
//
// Description - shows the channel pane for the given web browser object
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
ShowChannelPane(
    IWebBrowser2* pIWebBrowser2
)
{
    HRESULT hr;
    VARIANT guid;
    VARIANT empty = {0};
    BSTR    bstrGuid;
    TCHAR szGuid[GUID_STR_LEN];
    WCHAR wszGuid[GUID_STR_LEN];

    if (!SHRestricted2W(REST_NoChannelUI, NULL, 0))
    {
#ifdef ENABLE_CHANNELPANE
        SHStringFromGUID(CLSID_ChannelBand, szGuid, ARRAYSIZE(szGuid));
#else
        SHStringFromGUID(CLSID_FavBand, szGuid, ARRAYSIZE(szGuid));
#endif
        SHTCharToUnicode(szGuid, wszGuid, ARRAYSIZE(wszGuid));

        if ((bstrGuid = SysAllocString(wszGuid)) == NULL)
            return E_OUTOFMEMORY;

        guid.vt = VT_BSTR;
        guid.bstrVal = bstrGuid;

        hr = pIWebBrowser2->ShowBrowserBar(&guid, &empty, &empty);

        SysFreeString(bstrGuid);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

//
// These routines are only needed if we want to pidl a variant pidl for
// navigating the ChannelPane to a specific pidl
//
SAFEARRAY * MakeSafeArrayFromData(const void * pData,DWORD cbData)
{
    SAFEARRAY * psa;

    if (!pData || 0 == cbData)
        return NULL;  // nothing to do

    // create a one-dimensional safe array
    psa = SafeArrayCreateVector(VT_UI1,0,cbData);
    ASSERT(psa);

    if (psa) {
        // copy data into the area in safe array reserved for data
        // Note we party directly on the pointer instead of using locking/
        // unlocking functions.  Since we just created this and no one
        // else could possibly know about it or be using it, this is OK.

        ASSERT(psa->pvData);
        memcpy(psa->pvData,pData,cbData);
    }

    return psa;
}

/************************************************************\
    FUNCTION: InitVARIANTFromPidl

    PARAMETER:
        pvar - Allocated by caller and filled in by this function.
        pidl - Allocated by caller and caller needs to free.

    DESCRIPTION:
        This function will take the PIDL parameter and COPY it
    into the Variant data structure.  This allows the pidl
    to be freed and the pvar to be used later, however, it
    is necessary to call VariantClear(pvar) to free memory
    that this function allocates.
\************************************************************/
BOOL InitVARIANTFromPidl(VARIANT* pvar, LPCITEMIDLIST pidl)
{
    UINT cb = ILGetSize(pidl);
    SAFEARRAY* psa = MakeSafeArrayFromData((const void *)pidl, cb);
    if (psa) {
        ASSERT(psa->cDims == 1);
        // ASSERT(psa->cbElements == cb);
        ASSERT(ILGetSize((LPCITEMIDLIST)psa->pvData)==cb);
        VariantInit(pvar);
        pvar->vt = VT_ARRAY|VT_UI1;
        pvar->parray = psa;
        return TRUE;
    }

    return FALSE;
}


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** NavigateBrowser ***
//
//
// Description:
//     Navigate the browser to the correct URL for the given cdf.
//
// Parameters:
//     [In]  IWebBrowser2 - The browser top naviagte.
//     [In]  szwURL       - The path to the cdf file.
//     [In]  hwnd         - The wner hwnd.
//
// Return:
//     S_OK if the cdf file was parsed and the browser naviagted to teh URL.
//     E_FAIL otherwise.
//
// Comments:
//     Read the href of the first CHANNEL tag in the cfd file and navigate
//     the browser to this href.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
NavigateBrowser(
    IWebBrowser2* pIWebBrowser2,
    LPWSTR wszURL,
    HWND hwnd
)
{
    ASSERT(pIWebBrowser2);
    ASSERT(wszURL);
    ASSERT(*wszURL != 0);

    HRESULT hr = E_FAIL;

    //
    // Try to navigate to the title page.
    //

    CCdfView* pCCdfView = new CCdfView;

    if (pCCdfView)
    {
        hr = pCCdfView->Load(wszURL, 0);

        if (SUCCEEDED(hr))
        {
            IXMLDocument* pIXMLDocument = NULL;

            TraceMsg(TF_CDFPARSE, "NavigateBrowser");

            BOOL fIsURLChannelShortcut = PathIsDirectoryW(wszURL);

            hr = pCCdfView->ParseCdf(NULL, &pIXMLDocument, PARSE_LOCAL | PARSE_REMOVEGLEAM);
            
            if (SUCCEEDED(hr))
            {
                ASSERT(pIXMLDocument);

                //
                // Iff the CDF parsed correctly then show the channel pane
                //
                // pIWebBrowser2->put_TheaterMode(VARIANT_TRUE);
                pIWebBrowser2->put_Visible(VARIANT_TRUE);



                IXMLElement*    pIXMLElement;
                LONG            nIndex;

                hr = XML_GetFirstChannelElement(pIXMLDocument,
                                                &pIXMLElement, &nIndex);

                if (SUCCEEDED(hr))
                {
                    ASSERT(pIXMLElement);

                    BSTR bstrURL = XML_GetAttribute(pIXMLElement, XML_HREF);

                    if (bstrURL && *bstrURL)
                    {
                        if (!fIsURLChannelShortcut)
                        {
                            LPOLESTR pszPath = Channel_GetChannelPanePath(wszURL);

                            if (pszPath)
                            {
                                if (SUCCEEDED(ShowChannelPane(pIWebBrowser2)))
                                    NavigateChannelPane(pIWebBrowser2, pszPath);

                                CoTaskMemFree(pszPath);
                            }
                        }
                        else
                        {
                            TCHAR szChanDir[MAX_PATH];

                            if (SUCCEEDED(Channel_GetFolder(szChanDir,
                                                            DOC_CHANNEL)))
                            {
                                WCHAR wszChanDir[MAX_PATH];

                                if (SHTCharToUnicode(szChanDir, wszChanDir,
                                                   ARRAYSIZE(wszChanDir)))
                                {

                                    if (PathIsPrefixW(wszChanDir, wszURL))
                                    {
                                        if (SUCCEEDED(ShowChannelPane(
                                                                pIWebBrowser2)))
                                        {
                                            NavigateChannelPane(pIWebBrowser2,
                                                                wszURL);
                                        }
                                    }
                                }
                            }
                        }

                        VARIANT vNull = {0};
                        VARIANT vTargetURL;

                        vTargetURL.vt      = VT_BSTR;
                        vTargetURL.bstrVal = bstrURL;
                    
                        //
                        // Nav the main browser pain to the target URL
                        //
                        hr = pIWebBrowser2->Navigate2(&vTargetURL, &vNull,
                                                      &vNull, &vNull, &vNull);
                    }

                    if (bstrURL)
                        SysFreeString(bstrURL);

                    pIXMLElement->Release();
                }
            }
            else if (OLE_E_NOCACHE == hr)
            {
                VARIANT vNull = {0};
                VARIANT vTargetURL;

                if (!fIsURLChannelShortcut)
                {                        
                    vTargetURL.bstrVal = SysAllocString(wszURL);
                }
                else
                {
                    vTargetURL.bstrVal = pCCdfView->ReadFromIni(TSTR_INI_URL);
                }

                if (vTargetURL.bstrVal)
                {
                    vTargetURL.vt = VT_BSTR;

                    //
                    // Nav the main browser pain to the target URL
                    //
                    hr = pIWebBrowser2->Navigate2(&vTargetURL, &vNull, &vNull,
                                                  &vNull, &vNull);

                    SysFreeString(vTargetURL.bstrVal);
                }

            }

            if (pIXMLDocument)
                pIXMLDocument->Release();
        }

        pCCdfView->Release();
    }

    return hr;
}

//
// Navigate the channel pane to the given channel.
//

HRESULT
NavigateChannelPane(
    IWebBrowser2* pIWebBrowser2,
    LPCWSTR pwszPath
)
{
    ASSERT(pIWebBrowser2);

    HRESULT hr = E_FAIL;

    if (pwszPath)
    {
        TCHAR szPath[MAX_PATH];

        if (SHUnicodeToTChar(pwszPath, szPath, ARRAYSIZE(szPath)))
        {
            LPITEMIDLIST pidl;

            if (SUCCEEDED(Channel_CreateILFromPath(szPath,  &pidl)))
            {
                ASSERT(pidl);

                VARIANT varPath;

                if (InitVARIANTFromPidl(&varPath, pidl))
                {
                    VARIANT varNull = {0};
                    VARIANT varFlags;

                    varFlags.vt   = VT_I4;
                    varFlags.lVal = navBrowserBar;

                    hr = pIWebBrowser2->Navigate2(&varPath, &varFlags,
                                                  &varNull, &varNull,
                                                  &varNull);

                    VariantClear(&varPath);
                }

                ILFree(pidl);
            }
        }

    }

    return hr;
}


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** SubscriptionHelper ***
//
//
// Description:
//     Gets subscription information from the given document and uses that info
//     to create a subscription
//
// Parameters:
//     [In]  pIXMLDocument - A pointer to the cdf document.
//     [In]  hwnd          - The ownser hwnd.  Used to display UI.
//     [In]  st            - The type of subscription.
//     [In]  sa            - Flag used to determine if additional steps should
//                           be taken if a user does create a subscription.
//
// Return:
//     TRUE if a subscition for this document exists when this function returns.
//     FALSE if the document doesn't have a subscription and one wasn't created.
//
// Comments:
//     If a subscription to a channel is created then a channel shortcut has to
//     be added to the favorites\channel folder.
//
//     If a subscription to a desktop component is created then the caller
//     determines if the desktop component gets added to the system.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
SubscriptionHelper(
    IXMLDocument *pIXMLDocument,
    HWND hwnd,
    SUBSCRIPTIONTYPE st,
    SUBSCRIPTIONACTION sa,
    LPCWSTR pszwURL,
    XMLDOCTYPE xdt,
    BSTR* pbstrSubscribedURL
)
{
    ASSERT(pIXMLDocument);

    BOOL bChannelInstalled = FALSE;

    HRESULT         hr;
    IXMLElement*    pIXMLElement;
    LONG            nIndex;

    hr = XML_GetFirstChannelElement(pIXMLDocument, &pIXMLElement, &nIndex);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIXMLElement);

        BSTR bstrURL = XML_GetAttribute(pIXMLElement, XML_SELF);

        if ((NULL == bstrURL || 0 == *bstrURL) && pszwURL)
        {
            if (bstrURL)
                SysFreeString(bstrURL);

            bstrURL = SysAllocString(pszwURL);
        }

        if (bstrURL)
        {
            ISubscriptionMgr* pISubscriptionMgr = NULL;

#ifndef UNIX
            hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                                  CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr,
                                  (void**)&pISubscriptionMgr);

            if (SUCCEEDED(hr))
            {
                ASSERT(pISubscriptionMgr);

                //hr = pISubscriptionMgr->IsSubscribed(bstrURL, &bSubscribed);
                bChannelInstalled = Channel_IsInstalled(bstrURL);

                if (SUBSTYPE_DESKTOPCHANNEL == st && hwnd)
                {
                    BOOL bSubscribed;

                    hr = pISubscriptionMgr->IsSubscribed(bstrURL, &bSubscribed);

                    if (bSubscribed)
                    {
                        TCHAR szText[MAX_PATH];
                        TCHAR szTitle[MAX_PATH];

                        MLLoadString(IDS_OVERWRITE_DLG_TEXT,  szText,
                                   ARRAYSIZE(szText)); 
                        MLLoadString(IDS_OVERWRITE_DLG_TITLE, szTitle,
                                   ARRAYSIZE(szTitle));

                        if (IDYES == MessageBox(hwnd, szText, szTitle,
                                                MB_YESNO | MB_ICONQUESTION))
                        {
                            pISubscriptionMgr->DeleteSubscription(bstrURL, NULL);
                            bChannelInstalled = FALSE;
                        }
                    }
                }
#else
                bChannelInstalled = Channel_IsInstalled(bstrURL);
#endif /* UNIX */

                if (!bChannelInstalled)
                {
                    BSTR bstrName;
                    
                    if (SUBSTYPE_DESKTOPCHANNEL != st)
                    {
                        bstrName = XML_GetAttribute(pIXMLElement, XML_TITLE);
                    }
                    else
                    {
                        IXMLElement* pDskCmpIXMLElement;

                        if (SUCCEEDED(XML_GetFirstDesktopComponentElement(
                                                            pIXMLDocument,
                                                            &pDskCmpIXMLElement,
                                                            &nIndex)))
                        {
                            ASSERT(pDskCmpIXMLElement);

                            bstrName = XML_GetAttribute(pDskCmpIXMLElement,
                                                        XML_TITLE);

                            pDskCmpIXMLElement->Release();
                        }
                        else
                        {
                            bstrName = NULL;
                        }
                    }

                    if ((NULL == bstrName || 0 == *bstrName) && pszwURL)
                    {
                        WCHAR szwFilename[MAX_PATH];

                        if (StrCpyNW(szwFilename, PathFindFileNameW(pszwURL),
                                     ARRAYSIZE(szwFilename)))
                        {
                            PathRemoveExtensionW(szwFilename);

                            if (bstrName)
                                SysFreeString(bstrName);

                            bstrName = SysAllocString(szwFilename);
                        }
                    }


                    if (bstrName)
                    {
                        TASK_TRIGGER     tt = {0};
                        SUBSCRIPTIONINFO si = {0};

                        si.cbSize       = sizeof(SUBSCRIPTIONINFO);
                        si.fUpdateFlags |= SUBSINFO_SCHEDULE;
                        si.schedule     = SUBSSCHED_AUTO;
                        si.pTrigger     = (LPVOID)&tt;

                        XML_GetSubscriptionInfo(pIXMLElement, &si);

                        bChannelInstalled = SubscribeToURL(pISubscriptionMgr,
                                                     bstrURL, bstrName, &si,
                                                     hwnd, st, (xdt==DOC_SOFTWAREUPDATE));
                        
#ifndef UNIX
                        if (bChannelInstalled &&
                            SUBSACTION_ADDADDITIONALCOMPONENTS == sa)
                        {
                            if (SUBSTYPE_CHANNEL == st)
                            {
                                // Update the subscription if the user has
                                // choosen to view the screen saver item.
                                if  (
                                    SUCCEEDED(pISubscriptionMgr->GetSubscriptionInfo(bstrURL,
                                                                                        &si))
                                    &&
                                    (si.fChannelFlags & CHANNEL_AGENT_PRECACHE_SCRNSAVER)
                                    )
                                {
                                    pISubscriptionMgr->UpdateSubscription(bstrURL);
                                }

                                //t-mattgi: moved this to AddToFav code in shdocvw
                                //because there, we know what folder to add it to
                                //AddChannel(szName, szURL, NULL, NULL, NULL, xdt);
                            }
                            else if (SUBSTYPE_DESKTOPCHANNEL == st)
                            {
                                COMPONENT Info;


                                if(SUCCEEDED(XML_GetDesktopComponentInfo(
                                                                  pIXMLDocument,
                                                                  &Info)))
                                {
                                    if(!Info.wszSubscribedURL[0])
                                    {
                                        // Since XML_GetDesktopComponentInfo did not fillout the SubscribedURL
                                        // field (because the CDF file didn't have a SELF tag), we need to 
                                        // fill it with what was really subscribed to (the URL for CDF file itself)
                                        StrCpyNW(Info.wszSubscribedURL, bstrURL, ARRAYSIZE(Info.wszSubscribedURL));                                    
                                    }
                                    pISubscriptionMgr->UpdateSubscription(bstrURL);
                                    
                                    AddDesktopComponent(&Info);
                                }
                            }
                        }
#endif /* !UNIX */
                    }
                }

#ifndef UNIX

                pISubscriptionMgr->Release();
            }
#endif /* !UNIX */

            if (pbstrSubscribedURL)
            {
                *pbstrSubscribedURL = bstrURL;
            }
            else
            {
                SysFreeString(bstrURL);
            }
        }

        pIXMLElement->Release();
    }

    return bChannelInstalled;
}
    
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** SubscribeToURL ***
//
//
// Description:
//     Takes the user to the subscription wizard for the given URL.
//
// Parameters:
//     [In]  ISubscriptionMgr - The subscription manager interface
//     [In]  bstrURL          - The URL to subscribe.
//     [In]  bstrName         - The name of the subscription.
//     [In]  psi              - A subscription information structure.
//     [In]  hwnd             - The owner hwnd.
//     [In]  st               - The type of subsciption. SUBSTYPE_CHANNEL or
//                              SUSBSTYPE_DESKTOPCOMPONENT.
//     [In]  bIsSoftare       - modifies SUBSTYPE_CHANNEL since software updates
//                              don't have their own subscriptiontype
//
// Return:
//     TRUE if the user subscribe to the URL.
//     FALSE if the user didn't subscribe to the URL.
//
// Comments:
//     The subscription manager CreateSubscription function takes the user
//     through the subscriptiopn wizard.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
SubscribeToURL(
    ISubscriptionMgr* pISubscriptionMgr,
    BSTR bstrURL,
    BSTR bstrName,
    SUBSCRIPTIONINFO* psi,
    HWND hwnd,
    SUBSCRIPTIONTYPE st,
    BOOL bIsSoftware
)
{
#ifndef UNIX
    ASSERT(pISubscriptionMgr);
#endif /* !UNIX */
    ASSERT(bstrURL);
    ASSERT(bstrName);

    BOOL bSubscribed = FALSE;

    DWORD dwFlags = 0;

    if (Channel_IsInstalled(bstrURL))
    {
        dwFlags |= CREATESUBS_ACTIVATE | CREATESUBS_FROMFAVORITES;
    }
    else
    {
        dwFlags |= CREATESUBS_ADDTOFAVORITES;
    }

    if (bIsSoftware)
    {
        dwFlags |= CREATESUBS_SOFTWAREUPDATE;
    }

#ifndef UNIX
    HRESULT hr = pISubscriptionMgr->CreateSubscription(hwnd, bstrURL, bstrName,
                                                       dwFlags, st, psi);
#else
    /* Unix does not have subscription support      */
    /* But, we want to add the channel to favorites */
    HRESULT hr = E_FAIL;

    if ((dwFlags & CREATESUBS_ADDTOFAVORITES) && (st == SUBSTYPE_CHANNEL || st == SUBSTYPE_URL))
       hr = SHAddSubscribeFavorite(hwnd, bstrURL, bstrName, dwFlags, st, psi);       
#endif /* UNIX */

#if 0
    if (SUCCEEDED(hr))
    {
        pISubscriptionMgr->IsSubscribed(bstrURL, &bSubscribed);

    }
#else
    //t-mattgi: REVIEW with edwardP
    //can't just check if subscribed -- they might choose to add to channel bar without
    //subscribing, then we still want to return true.  or do we?
    bSubscribed = (hr == S_OK); //case where they clicked OK
#endif

    return bSubscribed;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** AddDesktopComponent ***
//
//
// Description:
//     Calls the desktop component manager to add a new component.
//
// Parameters:
//     [In]  pInfo - Information about the new component to add.
//
// Return:
//     S_OK if the component was added.
//     E_FAIL otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
AddDesktopComponent(
    COMPONENT* pInfo
)
{
    ASSERT(pInfo);

    HRESULT hr = S_OK;

#ifndef UNIX
    /* No Active Desktop on Unix */

    IActiveDesktop* pIActiveDesktop;

    hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER,
                          IID_IActiveDesktop, (void**)&pIActiveDesktop);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIActiveDesktop);

        // Assign a default position to the component
        pInfo->cpPos.iLeft = COMPONENT_DEFAULT_LEFT;
        pInfo->cpPos.iTop = COMPONENT_DEFAULT_TOP;

        hr = pIActiveDesktop->AddDesktopItem(pInfo, 0);

        //
        // Apply all except refresh as this causes timing issues because the 
        // desktop is in offline mode but not in silent mode
        //
        if (SUCCEEDED(hr))
        {
            DWORD dwFlags = AD_APPLY_ALL;
            // If the desktop component url is already in cache, we want to
            // refresh right away - otherwise not
            if(!(CDFIDL_IsCachedURL(pInfo->wszSubscribedURL)))
            {
                //It is not in cache, we want to wait until the download
                // is done before refresh. So don't refresh right away
                dwFlags &= ~(AD_APPLY_REFRESH);
            }
            else
                dwFlags |= AD_APPLY_BUFFERED_REFRESH;

            hr = pIActiveDesktop->ApplyChanges(dwFlags);
        }

        pIActiveDesktop->Release();
    }

#endif /* UNIX */

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// *** Channel_CreateDirectory ***
//
// Description:
//     creates a directory including any intermediate
//     directories in the path.
//
// Parameters:
//     LPCTSTR pszPath - path of directory to create
//
// Returns:
//     0 if function succeeded, else returns GetLastError() 
//
// Comments:
//     Copied from SHCreateDirectory. Can't use SHCreateDirectory directly 
//     because that fires off an SHChangeNotify msg immediately and need to 
//     fire it off only after the desktop.ini has been created in AddChannel().
//     Also would have to do a runtime check for NT vs Win95 as this api doesn't
//     have A & W versions.
//
////////////////////////////////////////////////////////////////////////////////
int Channel_CreateDirectory(LPCTSTR pszPath)
{
    int ret = 0;

    if (!CreateDirectory(pszPath, NULL)) {
        TCHAR *pSlash, szTemp[MAX_PATH + 1];  // +1 for PathAddBackslash()
        TCHAR *pEnd;

        ret = GetLastError();

        // There are certain error codes that we should bail out here
        // before going through and walking up the tree...
        switch (ret)
        {
        case ERROR_FILENAME_EXCED_RANGE:
        case ERROR_FILE_EXISTS:
            return(ret);
        }

        StrCpyN(szTemp, pszPath, ARRAYSIZE(szTemp) - 1);
        pEnd = PathAddBackslash(szTemp); // for the loop below

        // assume we have 'X:\' to start this should even work
        // on UNC names because will will ignore the first error

#ifndef UNIX
        pSlash = szTemp + 3;
#else
        /* absolute paths on unix start with / */
        pSlash = szTemp + 1;
#endif /* UNIX */

        // create each part of the dir in order

        while (*pSlash) {
            while (*pSlash && *pSlash != TEXT(FILENAME_SEPARATOR))
                pSlash = CharNext(pSlash);

            if (*pSlash) {
                ASSERT(*pSlash == TEXT(FILENAME_SEPARATOR));

                *pSlash = 0;    // terminate path at seperator

                if (pSlash + 1 == pEnd)
                    ret = CreateDirectory(szTemp, NULL) ? 0 : GetLastError();
                else
                    ret = CreateDirectory(szTemp, NULL) ? 0 : GetLastError();

            }
            *pSlash++ = TEXT(FILENAME_SEPARATOR);     // put the seperator back
        }
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
// PathCombineCleanPath
////////////////////////////////////////////////////////////////////////////////
BOOL PathCombineCleanPath
(
    LPTSTR  pszCleanPath,
    LPCTSTR pszPath
)
{
    TCHAR   szComponent[MAX_PATH];
    TCHAR * pszComponent = szComponent;
    BOOL    bCleaned = FALSE;

    if (*pszPath == TEXT(FILENAME_SEPARATOR))
        pszPath++;

    *pszComponent = TEXT('\0');

    for (;;)
    {
        if  (
            (*pszPath == TEXT(FILENAME_SEPARATOR))
            ||
            (!*pszPath)
            )
        {
            *pszComponent = TEXT('\0');

#ifdef UNICODE
            PathCleanupSpec(NULL, szComponent);
#else
            MyPathCleanupSpec(NULL, szComponent);
#endif
            PathCombine(pszCleanPath, pszCleanPath, szComponent);

            bCleaned = TRUE;

            if (*pszPath)
            {
                // Reset for the next component
                pszComponent = szComponent;
                *pszComponent = TEXT('\0');
                pszPath = CharNext(pszPath);
            }
            else
                break;
        }
        else
        {
            LPTSTR pszNextChar = CharNext(pszPath);
            while (pszPath < pszNextChar)
            {
                *pszComponent++ = *pszPath++;
            }
        }
    }

    return bCleaned;
}
#ifndef UNICODE
////////////////////////////////////////////////////////////////////////////////
// MyPathCleanupPath
////////////////////////////////////////////////////////////////////////////////
int MyPathCleanupSpec(LPCTSTR pszDir, LPTSTR pszSpec)
{
    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    if ((VER_PLATFORM_WIN32_NT == osvi.dwPlatformId))
    {
        WCHAR wszDir[MAX_PATH];
        WCHAR wszSpec[MAX_PATH];
        LPWSTR pwszDir = wszDir;
        int iRet;

        if (pszDir)
            EVAL(MultiByteToWideChar(CP_ACP, 0, pszDir, -1, wszDir,  ARRAYSIZE(wszDir)) != 0);
        else
            pwszDir = NULL;

        EVAL(MultiByteToWideChar(CP_ACP, 0, pszSpec, -1, wszSpec,  ARRAYSIZE(wszSpec)) != 0);

        iRet = PathCleanupSpec((LPTSTR)pwszDir, (LPTSTR)wszSpec);

        EVAL(WideCharToMultiByte(CP_ACP, 0, wszSpec, -1, pszSpec, MAX_PATH, NULL, NULL) != 0);

        return iRet;
    }
    else
        return PathCleanupSpec(pszDir, pszSpec);
}
#endif
//
// Is the given path from the recycle bin?
//

BOOL IsRecycleBinPath(LPCTSTR pszPath)
{
    ASSERT(pszPath);

    //
    // "RECYCLED" is hard coded in bitbuck.c in shell32
    //

    return (0 == StrNCmpI(TEXT("RECYCLED"), PathSkipRoot(pszPath), 8));
}

////////////////////////////////////////////////////////////////////////////////
//
// ICopyHook::CopyCallback
//
// Either allows the shell to move, copy, delete, or rename a folder or printer
// object, or disallows the shell from carrying out the operation. The shell 
// calls each copy hook handler registered for a folder or printer object until
// either all the handlers have been called or one of them returns IDCANCEL.
//
// RETURNS:
//
//  IDYES    - Allows the operation.
//  IDNO     - Prevents the operation on this file, but continues with any other
//             operations (for example, a batch copy operation).
//  IDCANCEL - Prevents the current operation and cancels any pending operations
//
////////////////////////////////////////////////////////////////////////////////
UINT CChannelMgr::CopyCallback(
    HWND hwnd,          // Handle of the parent window for displaying UI objects
    UINT wFunc,         // Operation to perform. 
    UINT wFlags,        // Flags that control the operation 
    LPCTSTR pszSrcFile,  // Pointer to the source file 
    DWORD dwSrcAttribs, // Source file attributes 
    LPCTSTR pszDestFile, // Pointer to the destination file 
    DWORD dwDestAttribs // Destination file attributes 
)
{
    HRESULT hr;

    //
    // Return immediately if this isn't a delete of a system folder
    // Redundant check as this should be made in shdocvw
    //
    if (!(dwSrcAttribs & FILE_ATTRIBUTE_SYSTEM) ||
        !(dwSrcAttribs & FILE_ATTRIBUTE_DIRECTORY))
    {
        return IDYES;
    }

    //
    // Build a string containing the guid to check for in the desktop.ini
    //
    TCHAR szFolderGUID[GUID_STR_LEN];
    TCHAR szCDFViewGUID[GUID_STR_LEN];
    SHStringFromGUID(CLSID_CDFINI, szCDFViewGUID, ARRAYSIZE(szCDFViewGUID));

    //
    // Build path to desktop.ini in folder
    //
    TCHAR szPath[MAX_PATH]; 
    PathCombine(szPath, pszSrcFile, TEXT("desktop.ini"));

    //
    // Read CLSID from desktop.ini if present
    //
    GetPrivateProfileString(
        TEXT(".ShellClassInfo"),
        TEXT("CLSID"),
        TEXT(""), 
        szFolderGUID, 
        GUID_STR_LEN, 
        szPath);

    if (StrEql(szFolderGUID, szCDFViewGUID))
    {
        //
        // We are deleting/renaming a  folder that has the system bit set and 
        // desktop.ini that contains the CLSID for the CDFINI handler, so it
        // must be a channel
        //

        //
        // Find URL to CDF from desktop.ini
        //
        TCHAR szCDFURL[INTERNET_MAX_URL_LENGTH];
        WCHAR wszCDFURL[INTERNET_MAX_URL_LENGTH];
        GetPrivateProfileString(
            TEXT("Channel"),
            TEXT("CDFURL"),
            TEXT(""), 
            szCDFURL, 
            INTERNET_MAX_URL_LENGTH, 
            szPath);

        switch(wFunc)
        {
        case FO_RENAME:
            //Reg_RemoveChannel(pszSrcFile);
            //Reg_WriteChannel(pszDestFile, szCDFURL);
            break;

        case FO_COPY:
            //Reg_WriteChannel(pszDestFile, szCDFURL);
            break;

        case FO_MOVE:
            //Reg_RemoveChannel(pszSrcFile);
            //Reg_WriteChannel(pszDestFile, szCDFURL);
            break;

        case FO_DELETE:
            TraceMsg(TF_GENERAL, "Deleting a channel");

            //
            // Check if there is a shell restriction against remove channel(s)
            //
            if (SHRestricted2(REST_NoRemovingChannels, szCDFURL, 0) ||
                SHRestricted2(REST_NoEditingChannels,  szCDFURL, 0)    )
            {
                TraceMsg(TF_GENERAL, "Channel Delete blocked by shell restriction");
                return IDNO;
            }
            else if (!IsRecycleBinPath(pszSrcFile))
            {
                //
                // Delete the in memory cache entry for this url.
                //

                Cache_RemoveItem(szCDFURL);

                //
                // Delete the wininet cache entry for this cdf.
                //

                DeleteUrlCacheEntry(szCDFURL);

                //
                // Remove the channel from the registry.
                //

                //Reg_RemoveChannel(pszSrcFile);
                //
                // Convert UrlToCdf to wide str, and then to bstr
                //
                SHTCharToUnicode(szCDFURL, wszCDFURL, INTERNET_MAX_URL_LENGTH);
                BSTR bstrCDFURL = SysAllocString(wszCDFURL);

                if (bstrCDFURL)
                {
                    //
                    // Create the subscription manager
                    //
                    ISubscriptionMgr* pISubscriptionMgr = NULL;
                    hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                                      CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr,
                                      (void**)&pISubscriptionMgr);
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Delete the actual subscription to the CDF
                        //
                        hr = pISubscriptionMgr->DeleteSubscription(bstrCDFURL,NULL);

                        TraceMsg(TF_GENERAL, 
                                 SUCCEEDED(hr) ? 
                                 "DeleteSubscription Succeeded" :
                                 "DeleteSubscription Failed");

                        pISubscriptionMgr->Release();
                    }
                    SysFreeString(bstrCDFURL);
                }
            }
            break;

        default:
            break;
        }
    }
    return IDYES;
}

#ifdef UNICODE
UINT CChannelMgr::CopyCallback(
    HWND hwnd,          // Handle of the parent window for displaying UI objects
    UINT wFunc,         // Operation to perform. 
    UINT wFlags,        // Flags that control the operation 
    LPCSTR pszSrcFile,  // Pointer to the source file 
    DWORD dwSrcAttribs, // Source file attributes 
    LPCSTR	pszDestFile, // Pointer to the destination file 
    DWORD dwDestAttribs // Destination file attributes 
)
{
    WCHAR  wszSrcFile[MAX_PATH];
    WCHAR  wszDestFile[MAX_PATH];

    SHAnsiToUnicode(pszSrcFile, wszSrcFile, MAX_PATH);
    SHAnsiToUnicode(pszDestFile, wszDestFile, MAX_PATH);
    return CopyCallback(hwnd, wFunc, wFlags, wszSrcFile, dwSrcAttribs, wszDestFile, dwDestAttribs);
}
#endif

HRESULT Channel_CreateILFromPath(LPCTSTR pszPath, LPITEMIDLIST* ppidl)
{
    ASSERT(pszPath);
    ASSERT(ppidl);

    HRESULT hr;

    IShellFolder* pIShellFolder;

    hr = SHGetDesktopFolder(&pIShellFolder);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIShellFolder);

        WCHAR wszPath[MAX_PATH];

        if (SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath)))
        {
            ULONG ucch;

            hr = pIShellFolder->ParseDisplayName(NULL, NULL, wszPath, &ucch,
                                                 ppidl, NULL);
        }
        else
        {
            hr = E_FAIL;
        }

        pIShellFolder->Release();
    }

    return hr;
}


HRESULT GetChannelIconInfo(LPCTSTR pszPath, LPTSTR pszHashItem, int* piIndex,
                              UINT* puFlags)
{
    ASSERT(pszPath);
    ASSERT(pszHashItem);
    ASSERT(piIndex);
    ASSERT(puFlags);

    HRESULT hr;

    IShellFolder* pdsktopIShellFolder;

    hr = SHGetDesktopFolder(&pdsktopIShellFolder);

    if (SUCCEEDED(hr))
    {
        ASSERT(pdsktopIShellFolder);

        LPITEMIDLIST pidl; 

        hr = Channel_CreateILFromPath(pszPath, &pidl);

        if (SUCCEEDED(hr))
        {
            ASSERT(pidl);
            ASSERT(!ILIsEmpty(pidl));
            ASSERT(!ILIsEmpty(_ILNext(pidl)));

            LPITEMIDLIST pidlLast = ILClone(ILFindLastID(pidl));

            if (pidlLast)
            {
                if (ILRemoveLastID(pidl))
                {
                    IShellFolder* pIShellFolder;

                    hr = pdsktopIShellFolder->BindToObject(pidl, NULL,
                                                           IID_IShellFolder,
                                                        (void**)&pIShellFolder);

                    if (SUCCEEDED(hr))
                    {
                        ASSERT(pIShellFolder);

                        IExtractIcon* pIExtractIcon;

                        hr = pIShellFolder->GetUIObjectOf(NULL, 1,
                                                      (LPCITEMIDLIST*)&pidlLast,
                                                      IID_IExtractIcon,
                                                      NULL,
                                                      (void**)&pIExtractIcon);

                        if (SUCCEEDED(hr))
                        {
                            ASSERT(pIExtractIcon);

                            hr = pIExtractIcon->GetIconLocation(0, pszHashItem,
                                                                MAX_PATH,
                                                                piIndex,
                                                                puFlags);
                                                                

                            pIExtractIcon->Release();
                        }

                        pIShellFolder->Release();
                    }

                }

                ILFree(pidlLast);
            }

            ILFree(pidl);
        }

        pdsktopIShellFolder->Release();
    }

    return hr;
}

HRESULT PreUpdateChannelImage(LPCTSTR pszPath, LPTSTR pszHashItem, int* piIndex,
                              UINT* puFlags, int* piImageIndex)
{
    ASSERT(pszPath);
    ASSERT(pszHashItem);
    ASSERT(piIndex);
    ASSERT(puFlags);
    ASSERT(piImageIndex);

    HRESULT hr;

    TraceMsg(TF_GLEAM, "Pre     SHChangeNotify %s", pszPath);

    hr = GetChannelIconInfo(pszPath, pszHashItem, piIndex, puFlags);

    if (SUCCEEDED(hr))
    {
        SHFILEINFO fi = {0};

        if (SHGetFileInfo(pszPath, 0, &fi, sizeof(SHFILEINFO),
                          SHGFI_SYSICONINDEX))
        {
            *piImageIndex = fi.iIcon;
        }
        else
        {
            *piImageIndex = -1;
        }
    }

    return hr;
}

//
// UpdateChannelImage is a copy of _SHUpdateImageW found in shdocvw\util.cpp!
//

void UpdateChannelImage(LPCWSTR pszHashItem, int iIndex, UINT uFlags,
                        int iImageIndex)
{
    SHChangeUpdateImageIDList rgPidl;
    SHChangeDWORDAsIDList rgDWord;

    int cLen = MAX_PATH - (lstrlenW( pszHashItem ) + 1);
    cLen *= sizeof( WCHAR );

    if ( cLen < 0 )
        cLen = 0;

    // make sure we send a valid index
    if ( iImageIndex == -1 )
        iImageIndex = II_DOCUMENT;

    rgPidl.dwProcessID = GetCurrentProcessId();
    rgPidl.iIconIndex = iIndex;
    rgPidl.iCurIndex = iImageIndex;
    rgPidl.uFlags = uFlags;
    StrCpyNW( rgPidl.szName, pszHashItem, ARRAYSIZE(rgPidl.szName) );
    rgPidl.cb = (USHORT)(sizeof( rgPidl ) - cLen);
    _ILNext( (LPITEMIDLIST) &rgPidl )->mkid.cb = 0;

    rgDWord.cb = sizeof( rgDWord) - sizeof(USHORT);
    rgDWord.dwItem1 = (DWORD) iImageIndex;
    rgDWord.dwItem2 = 0;
    rgDWord.cbZero = 0;

    TraceMsg(TF_GLEAM, "Sending SHChangeNotify %S,%d (image index %d)",
             pszHashItem, iIndex, iImageIndex);

    // pump it as an extended event
    SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_IDLIST | SHCNF_FLUSH, &rgDWord,
                   &rgPidl);
    
    return;
}

HRESULT UpdateImage(LPCTSTR pszPath)
{
    ASSERT(pszPath);

    HRESULT hr;

    TCHAR  szHash[MAX_PATH];
    int   iIndex;
    UINT  uFlags;
    int   iImageIndex;
    
    hr = PreUpdateChannelImage(pszPath, szHash, &iIndex, &uFlags, &iImageIndex);

    if (SUCCEEDED(hr))
    {
        WCHAR wszHash[MAX_PATH];
        SHTCharToUnicode(szHash, wszHash, ARRAYSIZE(wszHash));

        UpdateChannelImage(wszHash, iIndex, uFlags, iImageIndex);
    }

    return hr;
}

//
// Determines if the channel is installed on the system.
//

BOOL
Channel_IsInstalled(
    LPCWSTR pszURL
)
{
    ASSERT(pszURL);

    BOOL fRet = FALSE;

    CChannelEnum* pCChannelEnum = new CChannelEnum(CHANENUM_CHANNELFOLDER |
                                                   CHANENUM_SOFTUPDATEFOLDER,
                                                   pszURL);

    if (pCChannelEnum)
    {
        CHANNELENUMINFO ci;

        fRet =  (S_OK == pCChannelEnum->Next(1, &ci, NULL));

        pCChannelEnum->Release();
    }

    return fRet;
}

HRESULT
Channel_WriteScreenSaverURL(
    LPCWSTR pszURL,
    LPCWSTR pszScreenSaverURL
)
{
    ASSERT(pszURL);

    HRESULT hr = S_OK;

#ifndef UNIX

    CChannelEnum* pCChannelEnum = new CChannelEnum(CHANENUM_CHANNELFOLDER |
                                                   CHANENUM_SOFTUPDATEFOLDER |
                                                   CHANENUM_PATH,
                                                   pszURL);
    if (pCChannelEnum)
    {
        CHANNELENUMINFO ci;

        if (S_OK == pCChannelEnum->Next(1, &ci, NULL))
        {
            TCHAR szDesktopINI[MAX_PATH];
            TCHAR szScreenSaverURL[INTERNET_MAX_URL_LENGTH];
            
            ASSERT(ci.pszPath);

            if (pszScreenSaverURL)
            {
                SHUnicodeToTChar(pszScreenSaverURL, szScreenSaverURL, ARRAYSIZE(szScreenSaverURL));
            }

            SHUnicodeToTChar(ci.pszPath, szDesktopINI, ARRAYSIZE(szDesktopINI));

            PathCombine(szDesktopINI, szDesktopINI, c_szDesktopINI);

            WritePrivateProfileString(c_szChannel, 
                                      c_szScreenSaverURL, 
                                      pszScreenSaverURL ? szScreenSaverURL : NULL, 
                                      szDesktopINI);

            CoTaskMemFree(ci.pszPath);
        }
        else
        {
            hr = E_FAIL;
        }

        pCChannelEnum->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

#endif /* !UNIX */

    return hr;
}

HRESULT Channel_GetAndWriteScreenSaverURL(LPCTSTR pszURL, LPCTSTR pszDesktopINI)
{
    HRESULT hr = S_OK;

#ifndef UNIX
    
    CCdfView* pCCdfView = new CCdfView;

    if (NULL != pCCdfView)
    {
        WCHAR wszURL[INTERNET_MAX_URL_LENGTH];

        SHTCharToUnicode(pszURL, wszURL, ARRAYSIZE(wszURL));
        
        if (SUCCEEDED(pCCdfView->Load(wszURL, 0)))
        {
            IXMLDocument* pIXMLDocument;

            if (SUCCEEDED(pCCdfView->ParseCdf(NULL, &pIXMLDocument, PARSE_LOCAL)))
            {
                BSTR bstrSSUrl;
                TCHAR szSSURL[INTERNET_MAX_URL_LENGTH];
                TCHAR *pszScreenSaverURL = NULL;
                
                ASSERT(NULL != pIXMLDocument);

                if (SUCCEEDED(XML_GetScreenSaverURL(pIXMLDocument, &bstrSSUrl)))
                {
                    SHUnicodeToTChar(bstrSSUrl, szSSURL, ARRAYSIZE(szSSURL));
                    pszScreenSaverURL = szSSURL;
                    SysFreeString(bstrSSUrl);
                    TraceMsg(TF_ALWAYS,  "CDFVIEW: %ws has screensaver URL=%ws", wszURL, bstrSSUrl);

                }
                else
                {
                    TraceMsg(TF_ALWAYS,  "CDFVIEW: %ws has no screensaver URL", wszURL);
                }

                WritePrivateProfileString(c_szChannel, 
                                          c_szScreenSaverURL, 
                                          pszScreenSaverURL, 
                                          pszDesktopINI);
                pIXMLDocument->Release();
            }
            else
            {
                TraceMsg(TF_ALWAYS,  "CDFVIEW: cdf parse failed %ws", wszURL);
            }
        }
        else
        {
            TraceMsg(TF_ALWAYS,  "CDFVIEW: Couldn't load cdf %ws", wszURL);
        }

        pCCdfView->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

#endif /* !UNIX */

    return hr;
}

HRESULT Channel_RefreshScreenSaverURLs()
{
    HRESULT hr = S_OK;

#ifndef UNIX

    CChannelEnum* pCChannelEnum = new CChannelEnum(CHANENUM_CHANNELFOLDER |
                                                   CHANENUM_SOFTUPDATEFOLDER |
                                                   CHANENUM_PATH | 
                                                   CHANENUM_URL,
                                                   NULL);
    if (pCChannelEnum)
    {
        CHANNELENUMINFO ci;

        while (S_OK == pCChannelEnum->Next(1, &ci, NULL))
        {
            TCHAR szDesktopINI[MAX_PATH];
            TCHAR szURL[INTERNET_MAX_URL_LENGTH];

            ASSERT(ci.pszPath);
            ASSERT(ci.pszURL);

            SHUnicodeToTChar(ci.pszPath, szDesktopINI, ARRAYSIZE(szDesktopINI));
            SHUnicodeToTChar(ci.pszURL, szURL, ARRAYSIZE(szURL));

            PathCombine(szDesktopINI, szDesktopINI, c_szDesktopINI);

            Channel_GetAndWriteScreenSaverURL(szURL, szDesktopINI);

            CoTaskMemFree(ci.pszPath);
            CoTaskMemFree(ci.pszURL);
        }

        pCChannelEnum->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

#endif /* !UNIX */

    return hr;
}
    
//
// Finds the path of the channel in the channels folder that has the given url.
//

LPOLESTR
Channel_GetChannelPanePath(
    LPCWSTR pszURL
)
{
    ASSERT(pszURL);

    LPOLESTR pstrRet = NULL;

    CChannelEnum* pCChannelEnum = new CChannelEnum(CHANENUM_CHANNELFOLDER |
                                                   CHANENUM_PATH, pszURL);

    if (pCChannelEnum)
    {
        CHANNELENUMINFO ci;

        if (S_OK == pCChannelEnum->Next(1, &ci, NULL))
            pstrRet = ci.pszPath;

        pCChannelEnum->Release();
    }

    return pstrRet;
}

//
// Send SHChangeNotify for agiven URL.
//

void Channel_SendUpdateNotifications(LPCWSTR pwszURL)
{
    CChannelEnum* pCChannelEnum = new CChannelEnum(
                                            CHANENUM_CHANNELFOLDER |
                                            CHANENUM_PATH,
                                            pwszURL);

    if (pCChannelEnum)
    {
        CHANNELENUMINFO ci;

        while (S_OK == pCChannelEnum->Next(1, &ci, NULL))
        {
            TCHAR szPath[MAX_PATH];

            if (SHUnicodeToTChar(ci.pszPath, szPath, ARRAYSIZE(szPath)))
            {
                //
                // Clearing the gleam flag will result in a SHCNE_UPDATEIMAGE
                // notification.
                //

                TCHAR szURL[INTERNET_MAX_URL_LENGTH];

                if (SHUnicodeToTChar(pwszURL, szURL, ARRAYSIZE(szURL)))
                    ClearGleamFlag(szURL, szPath);

                SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, (void*)szPath,
                               NULL);
            }

            CoTaskMemFree(ci.pszPath);
        }

        pCChannelEnum->Release();
    }

    return;
}

