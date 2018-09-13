//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       admin.cpp
//
//  Contents:   
//
//  Classes:
//
//  Functions:
//
//  History:    
//
//----------------------------------------------------------------------------

#include "private.h"
#include "shguidp.h"
#include "chanmgr.h"
#include "chanmgrp.h"
#include "winineti.h"

#include <mluisupp.h>

// Infodelivery Policies registry locations
#define INFODELIVERY_POLICIES TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Infodelivery")
// const TCHAR c_szRegKeyRestrictions[]    = INFODELIVERY_POLICIES TEXT("\\Restrictions");
const TCHAR c_szRegKeyModifications[]   = INFODELIVERY_POLICIES TEXT("\\Modifications");
const TCHAR c_szRegKeyCompletedMods[]   = INFODELIVERY_POLICIES TEXT("\\CompletedModifications");
const TCHAR c_szRegKeyIESetup[]         = TEXT("Software\\Microsoft\\IE4\\Setup");

// Wininet cache preload directory
const TCHAR c_szRegKeyCachePreload[]    = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Cache\\Preload");

// Registry key names of supported Modifications
const TCHAR c_szAddChannels[]               = TEXT("AddChannels");
const TCHAR c_szRemoveChannels[]            = TEXT("RemoveChannels");
const TCHAR c_szRemoveAllChannels[]         = TEXT("RemoveAllChannels");
const TCHAR c_szAddSubscriptions[]          = TEXT("AddSubscriptions");
const TCHAR c_szRemoveSubscriptions[]       = TEXT("RemoveSubscriptions");
const TCHAR c_szAddScheduleGroups[]         = TEXT("AddScheduleGroups");
const TCHAR c_szRemoveScheduleGroups[]      = TEXT("RemoveScheduleGroups");
const TCHAR c_szAddDesktopComponents[]      = TEXT("AddDesktopComponents");
const TCHAR c_szRemoveDesktopComponents[]   = TEXT("RemoveDesktopComponents");

// Registry value names of supported Modifications
const TCHAR c_szURL[]                   = TEXT("URL");
const TCHAR c_szTitle[]                 = TEXT("Title");
const TCHAR c_szLogo[]                  = TEXT("Logo");
const TCHAR c_szWideLogo[]              = TEXT("WideLogo");
const TCHAR c_szIcon[]                  = TEXT("Icon");
const TCHAR c_szCategory[]              = TEXT("Category");
const TCHAR c_szChannelGuide[]          = TEXT("ChannelGuide"); // DO NOTE CHANGE THIS STRING WITHOUT UPDATING CDFVIEW!!!
const TCHAR c_szPreloadURL[]            = TEXT("PreloadURL");
const TCHAR c_szLCID[]                  = TEXT("LangId");       // This must be an LCID despite its name
const TCHAR c_szSoftware[]              = TEXT("Software");
const TCHAR c_szSubscriptionType[]      = TEXT("SubscriptionType");
const TCHAR c_szScheduleGroup[]         = TEXT("ScheduleGroup");
const TCHAR c_szEarliestTime[]          = TEXT("EarliestTime");
const TCHAR c_szIntervalTime[]          = TEXT("IntervalTime");
const TCHAR c_szLatestTime[]            = TEXT("LatestTime");
const TCHAR c_szComponentType[]         = TEXT("DesktopComponentType");
const TCHAR c_szUsername[]              = TEXT("Username");
const TCHAR c_szPassword[]              = TEXT("Password");
const TCHAR c_szOldIEVersion[]          = TEXT("OldIEVersion");
const TCHAR c_szNonActive[]             = TEXT("NonActive");
const TCHAR c_szOffline[]               = TEXT("Offline");
const TCHAR c_szSynchronize[]           = TEXT("Synchronize");

// Names of reserved schedule groups that we support even in localized version
const WCHAR c_szScheduleAuto[]          = L"Auto";
const WCHAR c_szScheduleDaily[]         = L"Daily";
const WCHAR c_szScheduleWeekly[]        = L"Weekly";
const WCHAR c_szScheduleManual[]        = L"Manual";

// Function prototypes for Modification handlers
HRESULT ProcessAddChannels(HKEY hkey);
HRESULT ProcessRemoveChannels(HKEY hkey);
HRESULT ProcessRemoveAllChannels(HKEY hkey);
HRESULT ProcessAddSubscriptions(HKEY hkey);
HRESULT ProcessRemoveSubscriptions(HKEY hkey);
HRESULT ProcessAddScheduleGroups(HKEY hkey);
HRESULT ProcessRemoveScheduleGroups(HKEY hkey);
HRESULT ProcessAddDesktopComponents(HKEY hkey);
HRESULT ProcessRemoveDesktopComponents(HKEY hkey);

HRESULT Channel_GetBasePath(LPTSTR pszPath, int cch);

// Helper functions
void ShowChannelDirectories(BOOL fShow);

// Table of supported Actions and corresponding functions
// NOTE: The table must be ordered appropriately (RemoveAll must come before Add)
typedef HRESULT (*PFNACTION)(HKEY);
typedef struct { LPCTSTR szAction; PFNACTION pfnAction; } ACTIONTABLE;
ACTIONTABLE rgActionTable[] = {
    { c_szRemoveAllChannels,        &ProcessRemoveAllChannels },
    { c_szRemoveSubscriptions,      &ProcessRemoveSubscriptions },
    { c_szRemoveChannels,           &ProcessRemoveChannels },
    { c_szRemoveDesktopComponents,  &ProcessRemoveDesktopComponents },
    { c_szRemoveScheduleGroups,     &ProcessRemoveScheduleGroups },
    { c_szAddChannels,              &ProcessAddChannels },
    { c_szAddDesktopComponents,     &ProcessAddDesktopComponents },
    { c_szAddScheduleGroups,        &ProcessAddScheduleGroups },
    { c_szAddSubscriptions,         &ProcessAddSubscriptions }
};
#define ACTIONTABLECOUNT (sizeof(rgActionTable) / sizeof(ACTIONTABLE))
#define ACTIONTABLE_ADDCHANNELS 5

// Helper class to manipulate registry keys
class CRegKey
{
    HKEY m_hkey;
    DWORD dwIndex;
public:
    CRegKey(void)
    {
        m_hkey = NULL;
        dwIndex = 0;
    }
    ~CRegKey(void)
    {
        if (m_hkey)
        {
            LONG lRet = RegCloseKey(m_hkey);
            ASSERT(ERROR_SUCCESS == lRet);
            m_hkey = NULL;
        }
    }
    void SetKey(HKEY hkey)
    {
        m_hkey = hkey;
    }
    HKEY GetKey(void)
    {
        return m_hkey;
    }
    HRESULT OpenForRead(HKEY hkey, LPCTSTR szSubKey)
    {
        ASSERT(NULL == m_hkey);
        LONG lRet = RegOpenKeyEx(hkey, szSubKey, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &m_hkey);
        ASSERT((ERROR_SUCCESS == lRet) || !m_hkey);
        return (ERROR_SUCCESS == lRet)?(S_OK):(E_FAIL);
    }
    HRESULT CreateForWrite(HKEY hkey, LPCTSTR szSubKey)
    {
        ASSERT(NULL == m_hkey);
        DWORD dwDisp;
        LONG lRet = RegCreateKeyEx(hkey, szSubKey, 0, TEXT(""), 0, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &m_hkey, &dwDisp);
        ASSERT(ERROR_SUCCESS == lRet);
        return (ERROR_SUCCESS == lRet)?(S_OK):(E_FAIL);
    }
    HRESULT GetSubKeyCount(PDWORD pdwKeys)
    {
        ASSERT(NULL != m_hkey);
        LONG lRet = RegQueryInfoKey(m_hkey, NULL, NULL, NULL, pdwKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        ASSERT(ERROR_SUCCESS == lRet);
        return (ERROR_SUCCESS == lRet)?(S_OK):(E_FAIL);
    }
    HRESULT Next(LPTSTR szSubKey)
    {
        ASSERT(NULL != m_hkey);
        DWORD dwLen = MAX_PATH; // Assumes size of incoming buffer.
        LONG lRet = RegEnumKeyEx(m_hkey, dwIndex, szSubKey, &dwLen, NULL, NULL, NULL, NULL);
        dwIndex++;
        if (ERROR_SUCCESS == lRet)
            return S_OK;
        else if (ERROR_NO_MORE_ITEMS == lRet)
            return S_FALSE;
        else
        {
            ASSERT(FALSE);
            return E_FAIL;
        }
    }
    HRESULT Reset(void)
    {
        dwIndex = 0;
        return S_OK;
    }
    HRESULT SetValue(LPCTSTR szValueName, DWORD dwValue)
    {
        ASSERT(m_hkey);
        LONG lRet = RegSetValueEx(m_hkey, szValueName, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
        ASSERT(ERROR_SUCCESS == lRet);
        return (ERROR_SUCCESS == lRet)?(S_OK):(E_FAIL);
    }
    HRESULT GetValue(LPCTSTR szValueName, DWORD *pdwValue)
    {
        ASSERT(m_hkey);
        DWORD dwType = REG_DWORD;
        DWORD dwLen = sizeof(DWORD);
        LONG lRet = RegQueryValueEx(m_hkey, szValueName, 0, &dwType, (LPBYTE)pdwValue, &dwLen);
        return (ERROR_SUCCESS == lRet)?(S_OK):(E_FAIL);
    }
    HRESULT GetStringValue(LPCTSTR szValueName, LPTSTR szValue, DWORD dwLen)
    {
        ASSERT(m_hkey);
        DWORD dwType = REG_SZ;
        LONG lRet = RegQueryValueEx(m_hkey, szValueName, 0, &dwType, (LPBYTE)szValue, &dwLen);
        return (ERROR_SUCCESS == lRet)?(S_OK):(E_FAIL);
    }
    HRESULT SetBSTRValue(LPCTSTR szValueName, BSTR bstr)
    {
        ASSERT(m_hkey);
        TCHAR szValue[INTERNET_MAX_URL_LENGTH];
        MyOleStrToStrN(szValue, sizeof(szValue), bstr);
        LONG lRet = RegSetValueEx(m_hkey, szValueName, 0, REG_SZ, (LPBYTE)szValue, lstrlen(szValue) + 1);
        ASSERT(ERROR_SUCCESS == lRet);
        return (ERROR_SUCCESS == lRet)?(S_OK):(E_FAIL);
    }
    HRESULT GetBSTRValue(LPCTSTR szValueName, BSTR *pbstr)
    {
        ASSERT(m_hkey);
        *pbstr = NULL;
        TCHAR szValue[INTERNET_MAX_URL_LENGTH];
        DWORD dwType = REG_SZ;
        DWORD dwLen = sizeof(szValue);
        LONG lRet = RegQueryValueEx(m_hkey, szValueName, 0, &dwType, (LPBYTE)szValue, &dwLen);
        if (ERROR_SUCCESS == lRet)
        {
            *pbstr = SysAllocStringLen(NULL, dwLen);    // dwLen includes null terminator
            if (*pbstr)
            {
                MyStrToOleStrN(*pbstr, dwLen, szValue);
                return S_OK;
            }
        }
        return E_FAIL;
    }
};

// Helper class to manage Dynamic Pointer Arrays of HKEYs.
class CRegKeyDPA
{
    HDPA m_hdpa;
    int m_count;
public:
    CRegKeyDPA(void)
    {
        m_hdpa = NULL;
        m_count = 0;
    }
    ~CRegKeyDPA(void)
    {
        if (m_hdpa)
        {
            ASSERT(m_count);
            int i;
            for (i = 0; i < m_count; i++)
                RegCloseKey(GetKey(i));
            DPA_Destroy(m_hdpa);
        }
    }
    int GetCount(void)
    {
        return m_count;
    }
    HKEY GetKey(int i)
    {
        ASSERT(i >= 0 && i < m_count);
        return (HKEY)DPA_GetPtr(m_hdpa, i);
    }
    HRESULT Add(HKEY hkey, LPCTSTR szSubKey)
    {
        if (!m_hdpa)
        {
            m_hdpa = DPA_CreateEx(5, NULL); // Choose arbitrary growth value
            if (!m_hdpa)
                return E_FAIL;
        }
        HKEY hkeyNew;
        LONG lRet = RegOpenKeyEx(hkey, szSubKey, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hkeyNew);
        if (ERROR_SUCCESS != lRet)
            return E_FAIL;
        if (-1 == DPA_InsertPtr(m_hdpa, DPA_APPEND, hkeyNew))
        {
            RegCloseKey(hkeyNew);
            return E_FAIL;
        }
        m_count++;
        return S_OK;
    }
};

//
// 8/18/98 darrenmi
// Copied (and butchered) from shdocvw\util.cpp so we don't have to load it at startup
//
DWORD WCRestricted2W(BROWSER_RESTRICTIONS rest, LPCWSTR pwzUrl, DWORD dwReserved)
{
    DWORD dwType, dw = 0, dwSize = sizeof(DWORD);

    // we only handle NoChannelUI restriction
    if(rest != REST_NoChannelUI)
    {
        return 0;
    }

    // read registry setting
    SHGetValue(HKEY_CURRENT_USER,
            TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Infodelivery\\Restrictions"),
            TEXT("NoChannelUI"),
            &dwType, &dw, &dwSize);

    return dw;
}


// ProcessInfodeliveryPolicies
//
// This is the main Admin API for Infodelivery.  It returns E_FAIL for errors,
// S_FALSE for nothing to process, and S_OK for correctly processed items.
//
// Reg key organization     [Modifications]         - the key to process
//                              [GUID1]             - group of actions
//                                  [AddChannels]   - sample action
//                                      [Channel1]  - element of an action
//
HRESULT ProcessInfodeliveryPolicies(void)
{
    HRESULT hr;
    CRegKey regModifications;
    TCHAR   szGUID[MAX_PATH];

    // Check if channels should be hidden.
    if (WCRestricted2W(REST_NoChannelUI, NULL, 0))
    {
        ShowChannelDirectories(FALSE);
    }
    else
    {
        ShowChannelDirectories(TRUE);
    }
    
    // Bail out quickly if there are no Modifications to perform. (Return S_FALSE)
    hr = regModifications.OpenForRead(HKEY_CURRENT_USER, c_szRegKeyModifications);
    if (FAILED(hr))
        return S_FALSE;

    // Prepare to use the CompletedModifications key.
    CRegKey regCompletedMods;
    hr = regCompletedMods.CreateForWrite(HKEY_CURRENT_USER, c_szRegKeyCompletedMods);
    if (FAILED(hr))
        return hr;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;

    // Prepare queues of registry keys to actions
    CRegKeyDPA rgKeyQueue[ACTIONTABLECOUNT];

    // Enumerate the GUID keys, skipping the completed ones.
    // Enumerate the Actions beneath them and add them to queues.
    // BUGBUG: ignoring errors here too.
    while (S_OK == regModifications.Next(szGUID))
    {
        DWORD dwValue;
        if (FAILED(regCompletedMods.GetValue(szGUID, &dwValue)))
        {
            CRegKey regGUID;
            TCHAR   szAction[MAX_PATH];
            hr = regGUID.OpenForRead(regModifications.GetKey(), szGUID);
            while (S_OK == regGUID.Next(szAction))
            {
                // Search the table to see if it's a key we understand.
                // If so, add it to the queue.
                int i;
                for (i = 0; i < ACTIONTABLECOUNT; i++)
                {
                    if (!StrCmpI(rgActionTable[i].szAction, szAction))
                    {
                        rgKeyQueue[i].Add(regGUID.GetKey(), szAction);
                        break;
                    }
                }
            }
        }
    }

    // Process all the keys we've accumulated.  (Correct order is assumed.)
    int i;
    for (i = 0; i < ACTIONTABLECOUNT; i++)
    {
        if (rgKeyQueue[i].GetCount())
        {
            int iKey;
            for (iKey = 0; iKey < rgKeyQueue[i].GetCount(); iKey++)
            {
                (rgActionTable[i].pfnAction)(rgKeyQueue[i].GetKey(iKey));
            }
        }
    }

    // Walk the GUIDs we've processed and mark them completed with the time.
    // Updating ones we skipped as well will help with garbage collection.
    regModifications.Reset();
    while (S_OK == regModifications.Next(szGUID))
    {
        SYSTEMTIME st;
        FILETIME ft;
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &ft);
        regCompletedMods.SetValue(szGUID, ft.dwHighDateTime);
    }

    // Delete the Actions.  NOTE: NT's RegDeleteKey() doesn't delete sub-keys.
    // BUGBUG: This shlwapi API uses KEY_ALL_ACCESS.
    // BUGBUG: We probably have to close all the keys here.
    SHDeleteKey(HKEY_CURRENT_USER, c_szRegKeyModifications);

    // If any channels were processed, tell the cache to reload.
    // BUGBUG: We should only do this for default channels.
    if (rgKeyQueue[ACTIONTABLE_ADDCHANNELS].GetCount())
    {
        ASSERT(!StrCmpI(rgActionTable[ACTIONTABLE_ADDCHANNELS].szAction, c_szAddChannels));
        BOOL bRet = LoadUrlCacheContent();
        ASSERT(bRet);
    }

    CoUninitialize();

    return S_OK;
}

//
// ProcessAddChannels_SortCallback - sort in reverse order
//
int ProcessAddChannels_SortCallback(PVOID p1, PVOID p2, LPARAM lparam)
{
    return StrCmpI((LPTSTR)p2, (LPTSTR)p1);
}

//
// ProcessAddChannels
//
HRESULT ProcessAddChannels(HKEY hkey)
{
    // Enumerate the channels in the AddChannels key
    HRESULT hr;
    DWORD dwChannels;
    CRegKey regAdd;
    regAdd.SetKey(hkey);
    hr = regAdd.GetSubKeyCount(&dwChannels);
    if (SUCCEEDED(hr) && dwChannels)
    {
        // Check if the channels are the same code page as the system default.
        BOOL bCodePageMatch = TRUE;
        LCID lcidChannel = 0;
        if (SUCCEEDED(regAdd.GetValue(c_szLCID, &lcidChannel)))
        {
            TCHAR szCodePageSystem[8];
            TCHAR szCodePageChannel[8];
            szCodePageChannel[0] = 0;   // Init in case there's no locale info
            GetLocaleInfo(lcidChannel, LOCALE_IDEFAULTANSICODEPAGE, szCodePageChannel, ARRAYSIZE(szCodePageChannel));
            int iRet = GetLocaleInfo(GetSystemDefaultLCID(), LOCALE_IDEFAULTANSICODEPAGE, szCodePageSystem, ARRAYSIZE(szCodePageSystem));
            ASSERT(iRet);
            if (StrCmpI(szCodePageSystem, szCodePageChannel))
                bCodePageMatch = FALSE;
        }
    
        hr = E_FAIL;
        TCHAR *pch = (TCHAR *)MemAlloc(LMEM_FIXED, dwChannels * MAX_PATH * sizeof(TCHAR));
        if (pch)
        {
            HDPA hdpa = DPA_Create(dwChannels);
            if (hdpa)
            {
                DWORD i;
                TCHAR *pchCur = pch;
                for (i = 0; i < dwChannels; i++)
                {
                    if ((S_OK != regAdd.Next(pchCur)) || (-1 == DPA_InsertPtr(hdpa, DPA_APPEND, pchCur)))
                        break;
                    pchCur += MAX_PATH;
                }
                if (i >= dwChannels)
                {
                    // Sort channels by registry key name,
                    DPA_Sort(hdpa, ProcessAddChannels_SortCallback, 0);
                    // Now create them.
                    for (i = 0; i < dwChannels; i++)
                    {
                        BSTR bstrURL = NULL;
                        BSTR bstrTitle = NULL;
                        BSTR bstrLogo = NULL;
                        BSTR bstrWideLogo = NULL;
                        BSTR bstrIcon = NULL;
                        BSTR bstrPreloadURL = NULL;
                        DWORD dwCategory = 0;       // default to channel
                        DWORD dwChannelGuide = 0;   // default to not a guide
                        DWORD dwSoftware = 0;       // default to non-software channel
                        DWORD dwOffline = 0;
                        DWORD dwSynchronize = 0;
                        CRegKey regChannel;
                        regChannel.OpenForRead(hkey, (LPCTSTR)DPA_GetPtr(hdpa, i));
                        hr = regChannel.GetBSTRValue(c_szURL, &bstrURL);
                        hr = regChannel.GetBSTRValue(c_szTitle, &bstrTitle);
                        hr = regChannel.GetBSTRValue(c_szLogo, &bstrLogo);
                        hr = regChannel.GetBSTRValue(c_szWideLogo, &bstrWideLogo);
                        hr = regChannel.GetBSTRValue(c_szIcon, &bstrIcon);
                        hr = regChannel.GetBSTRValue(c_szPreloadURL, &bstrPreloadURL);
                        hr = regChannel.GetValue(c_szCategory, &dwCategory);
                        hr = regChannel.GetValue(c_szChannelGuide, &dwChannelGuide);
                        hr = regChannel.GetValue(c_szSoftware, &dwSoftware);
                        hr = regChannel.GetValue(c_szOffline, &dwOffline);
                        hr = regChannel.GetValue(c_szSynchronize, &dwSynchronize);
                        if (bstrTitle)
                        {
                            IChannelMgr *pChannelMgr = NULL;
                            hr = CoCreateInstance(CLSID_ChannelMgr, NULL, CLSCTX_INPROC_SERVER, IID_IChannelMgr, (void**)&pChannelMgr);
                            if (SUCCEEDED(hr))
                            {
                                // See if channel already exists - do nothing if it does (62976)
                                IEnumChannels *pEnumChannels = NULL;
                                if (SUCCEEDED(pChannelMgr->EnumChannels(CHANENUM_ALLFOLDERS, bstrURL, &pEnumChannels)))
                                {
                                    CHANNELENUMINFO Bogus={0};
                                    ULONG cFetched=0;

                                    if ((S_OK == pEnumChannels->Next(1, &Bogus, &cFetched)) && cFetched)
                                    {
                                        // Oops. It exists. Skip all this goo.
                                        hr = E_FAIL;
                                    }
                                }
                                SAFERELEASE(pEnumChannels);
                            }
                            if (SUCCEEDED(hr))
                            {
                                if (dwCategory && bCodePageMatch)
                                {
                                    // create a category (useless if code page doesn't match)
                                    CHANNELCATEGORYINFO csi = {0};
                                    csi.cbSize   = sizeof(csi);
                                    csi.pszURL   = bstrURL;
                                    csi.pszTitle = bstrTitle;
                                    csi.pszLogo  = bstrLogo;
                                    csi.pszIcon  = bstrIcon;
                                    csi.pszWideLogo = bstrWideLogo;
                                    hr = pChannelMgr->AddCategory(&csi);
                                }
                                else if (!dwCategory && bstrURL)
                                {
                                    // update the registry if it's a channel guide
                                    if (dwChannelGuide)
                                    {
                                        CRegKey reg;
                                        hr = reg.CreateForWrite(HKEY_CURRENT_USER, c_szRegKey);
                                        if (SUCCEEDED(hr))
                                            reg.SetBSTRValue(c_szChannelGuide, bstrTitle);
                                    }
                                    // tell wininet if there's preload content
                                    if (bstrPreloadURL)
                                    {
                                        CRegKey reg;
                                        hr = reg.CreateForWrite(HKEY_CURRENT_USER, c_szRegKeyCachePreload);
                                        if (SUCCEEDED(hr))
                                        {
                                            TCHAR szURL[INTERNET_MAX_URL_LENGTH];
                                            MyOleStrToStrN(szURL, sizeof(szURL), bstrURL);
                                            reg.SetBSTRValue(szURL, bstrPreloadURL);
                                        }
                                    }
                                    // create a channel (use URL instead of Title if code page doesn't match)
                                    CHANNELSHORTCUTINFO csi = {0};
                                    csi.cbSize   = sizeof(csi);
                                    csi.pszURL   = bstrURL;
                                    if (bCodePageMatch)
                                        csi.pszTitle = bstrTitle;
                                    else
                                        csi.pszTitle = bstrURL;
                                    csi.pszLogo  = bstrLogo;
                                    csi.pszIcon  = bstrIcon;
                                    csi.pszWideLogo = bstrWideLogo;
                                    if (dwSoftware)
                                        csi.bIsSoftware = TRUE;
                                    hr = pChannelMgr->AddChannelShortcut(&csi);
                                }
                            }
                            SAFERELEASE(pChannelMgr);

                            if (dwOffline)
                            {
                                ISubscriptionMgr2 *pSubMgr2 = NULL;
                                hr = CoCreateInstance(CLSID_SubscriptionMgr, 
                                                      NULL, 
                                                      CLSCTX_INPROC_SERVER, 
                                                      IID_ISubscriptionMgr2, 
                                                      (void**)&pSubMgr2);
                                if (SUCCEEDED(hr))
                                {
                                    hr = pSubMgr2->CreateSubscription(NULL, 
                                                                      bstrURL, 
                                                                      bstrTitle, 
                                                                      CREATESUBS_NOUI,
                                                                      SUBSTYPE_CHANNEL, 
                                                                      NULL);

                                    if (dwSynchronize)
                                    {
                                        BOOL bIsSubscribed;
                                        SUBSCRIPTIONCOOKIE cookie;

                                        if (SUCCEEDED(pSubMgr2->IsSubscribed(bstrURL, &bIsSubscribed))
                                            && bIsSubscribed &&
                                            SUCCEEDED(ReadCookieFromInetDB(bstrURL, &cookie)))
                                        {
                                            pSubMgr2->UpdateItems(SUBSMGRUPDATE_MINIMIZE, 1, &cookie);
                                        }
                                    }
                                    pSubMgr2->Release();
                                }
                            }
                        }
                        SAFEFREEBSTR(bstrURL);
                        SAFEFREEBSTR(bstrTitle);
                        SAFEFREEBSTR(bstrLogo);
                        SAFEFREEBSTR(bstrWideLogo);
                        SAFEFREEBSTR(bstrIcon);
                        SAFEFREEBSTR(bstrPreloadURL);
                    }
                }
                DPA_Destroy(hdpa);
            }
            MemFree(pch);
        }
    }
    regAdd.SetKey(NULL);
    return S_OK;
}

//
// ProcessRemoveChannels
//
HRESULT ProcessRemoveChannels(HKEY hkey)
{
    // Enumerate the channel keys in the RemoveChannels key
    HRESULT hr;
    CRegKey reg;
    reg.SetKey(hkey);
    TCHAR szChannel[MAX_PATH];
    while (S_OK == reg.Next(szChannel))
    {
        CRegKey regChannel;
        DWORD dwNonActive = 0;  // default to deleting Active & NonActive channels
        TCHAR szURL[INTERNET_MAX_URL_LENGTH];
        regChannel.OpenForRead(hkey, szChannel);
        regChannel.GetValue(c_szNonActive, &dwNonActive);
        if (SUCCEEDED(regChannel.GetStringValue(c_szURL, szURL, sizeof(szURL))))
        {
            // Check if the channel is Active to determine if we can delete it
            if (dwNonActive)
            {
                CRegKey regPreload;
                if (SUCCEEDED(regPreload.OpenForRead(HKEY_CURRENT_USER, c_szRegKeyCachePreload)))
                {
                    if (SUCCEEDED(regPreload.GetStringValue(szURL, NULL, 0)))
                    {
                        dwNonActive = 0;
                    }
                }
            }

            // Now delete the channel if appropriate
            if (!dwNonActive)
            {
                IChannelMgr *pChannelMgr = NULL;
                hr = CoCreateInstance(CLSID_ChannelMgr, NULL, CLSCTX_INPROC_SERVER, IID_IChannelMgr, (void**)&pChannelMgr);
                if (SUCCEEDED(hr))
                {
                    BSTR bstrURL;
                    if (SUCCEEDED(regChannel.GetBSTRValue(c_szURL, &bstrURL)))
                    {
                        IEnumChannels *pEnum;
                        hr = pChannelMgr->EnumChannels(CHANENUM_ALLFOLDERS | CHANENUM_PATH, bstrURL, &pEnum);
                        if (SUCCEEDED(hr))
                        {
                            CHANNELENUMINFO info;
                            while (S_OK == pEnum->Next(1, &info, NULL))
                            {
                                hr = pChannelMgr->DeleteChannelShortcut(info.pszPath);
                                ASSERT(SUCCEEDED(hr));
                                CoTaskMemFree(info.pszPath);
                            }
                            pEnum->Release();
                        }
                        SysFreeString(bstrURL);
                    }
                    pChannelMgr->Release();
                }
            }    
        }
    }
    reg.SetKey(NULL);
    return S_OK;
}

//
// IsScheduleGroupReserved
//
HRESULT IsScheduleGroupReserved(LPCWSTR wzName, SUBSCRIPTIONSCHEDULE *pSched)
{
    HRESULT hr = S_OK;
    if (!StrCmpIW(wzName, c_szScheduleAuto))
        *pSched = SUBSSCHED_AUTO;
    else if (!StrCmpIW(wzName, c_szScheduleDaily))
        *pSched = SUBSSCHED_DAILY;
    else if (!StrCmpIW(wzName, c_szScheduleWeekly))
        *pSched = SUBSSCHED_WEEKLY;
    else if (!StrCmpIW(wzName, c_szScheduleManual))
        *pSched = SUBSSCHED_MANUAL;
    else
    {
        *pSched = SUBSSCHED_CUSTOM;
        hr = E_FAIL;
    }
    return hr;
}

//
// FindScheduleGroupFromName
//
HRESULT FindScheduleGroupFromName(LPCWSTR wzName, PNOTIFICATIONCOOKIE pCookie)
{
    // BUGBUG: Need to worry about localized names.
    HRESULT hrRet = E_FAIL;
    HRESULT hr;
    INotificationMgr *pNotMgr = NULL;
    hr = CoCreateInstance(CLSID_StdNotificationMgr, NULL, CLSCTX_INPROC_SERVER, IID_INotificationMgr, (void**)&pNotMgr);
    if (SUCCEEDED(hr))
    {
        // BUGBUG: what flags?
        IEnumScheduleGroup *pEnumScheduleGroup = NULL;
        hr = pNotMgr->GetEnumScheduleGroup(0, &pEnumScheduleGroup);
        if (SUCCEEDED(hr))
        {
            // Iterate through schedule groups
            for (;;)
            {
                IScheduleGroup *pScheduleGroup = NULL;
                ULONG uFetched;
                hr = pEnumScheduleGroup->Next(1, &pScheduleGroup, &uFetched);
                if (SUCCEEDED(hr) && (1 == uFetched))
                {
                    GROUPINFO info = { 0 };
                    hr = pScheduleGroup->GetAttributes(NULL, NULL, pCookie, &info, NULL, NULL);
                    if (SUCCEEDED(hr))
                    {
                        ASSERT(info.cbSize == SIZEOF(GROUPINFO));
                        ASSERT(info.pwzGroupname != NULL);
                        if (!StrCmpW(info.pwzGroupname, wzName))
                        {
                            // found it
                            hrRet = S_OK;
                            SAFEDELETE(info.pwzGroupname);
                            break;
                        }
                        SAFEDELETE(info.pwzGroupname);
                    }
                    pScheduleGroup->Release();
                }
                else
                {
                    break;
                }
            }
            pEnumScheduleGroup->Release();
        }
        pNotMgr->Release();
    }
    return hrRet;
}

//
// ProcessAddSubscriptions
//
HRESULT ProcessAddSubscriptions(HKEY hkey)
{
    // Enumerate the subscription keys in the AddSubscriptions key
    HRESULT hr;
    CRegKey reg;
    reg.SetKey(hkey);
    TCHAR szSubscription[MAX_PATH];
    while (S_OK == reg.Next(szSubscription))
    {
        // Create the subscription
        // BUGBUG: What if there is one already?
        CRegKey regSubscription;
        regSubscription.OpenForRead(hkey, szSubscription);
        BSTR bstrURL, bstrTitle, bstrGroup, bstrUsername, bstrPassword;
        DWORD dwSubType;
        DWORD dwSynchronize = 0;
        hr = regSubscription.GetBSTRValue(c_szURL, &bstrURL);
        hr = regSubscription.GetBSTRValue(c_szTitle, &bstrTitle);
        hr = regSubscription.GetBSTRValue(c_szScheduleGroup, &bstrGroup);
        hr = regSubscription.GetBSTRValue(c_szUsername, &bstrUsername);
        hr = regSubscription.GetBSTRValue(c_szPassword, &bstrPassword);
        hr = regSubscription.GetValue(c_szSynchronize, &dwSynchronize);
        if (bstrURL && bstrTitle && bstrGroup && SUCCEEDED(regSubscription.GetValue(c_szSubscriptionType, &dwSubType)))
        {
            SUBSCRIPTIONINFO si = {0};
            si.cbSize = sizeof(SUBSCRIPTIONINFO);
            si.fUpdateFlags = SUBSINFO_SCHEDULE;
            if (bstrUsername && bstrPassword)
            {
                si.fUpdateFlags |= (SUBSINFO_USER | SUBSINFO_PASSWORD);
                si.bstrUserName = bstrUsername;
                si.bstrPassword = bstrPassword;
            }
            if (dwSubType == SUBSTYPE_CHANNEL || dwSubType == SUBSTYPE_DESKTOPCHANNEL)
            {
                si.fUpdateFlags |= SUBSINFO_CHANNELFLAGS;
                si.fChannelFlags = 0;   //  Notify only.
            }
            hr = IsScheduleGroupReserved(bstrGroup, &si.schedule);
            if (FAILED(hr))
            {
                hr = FindScheduleGroupFromName(bstrGroup, &si.customGroupCookie);
            }

            if (SUCCEEDED(hr))
            {
                ISubscriptionMgr2 *pSubMgr2 = NULL;
                hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr2, (void**)&pSubMgr2);
                if (SUCCEEDED(hr))
                {
                    hr = pSubMgr2->CreateSubscription(NULL, bstrURL, bstrTitle, CREATESUBS_NOUI,
                                                     (SUBSCRIPTIONTYPE)dwSubType, &si);
                    if (dwSynchronize)
                    {
                        BOOL bIsSubscribed;
                        SUBSCRIPTIONCOOKIE cookie;

                        if (SUCCEEDED(pSubMgr2->IsSubscribed(bstrURL, &bIsSubscribed)) && 
                            bIsSubscribed &&
                            SUCCEEDED(ReadCookieFromInetDB(bstrURL, &cookie)))
                        {
                            pSubMgr2->UpdateItems(SUBSMGRUPDATE_MINIMIZE, 1, &cookie);
                        }
                    }

                    pSubMgr2->Release();
                }
            }
        }
        SAFEFREEBSTR(bstrURL);
        SAFEFREEBSTR(bstrTitle);
        SAFEFREEBSTR(bstrGroup);
        SAFEFREEBSTR(bstrUsername);
        SAFEFREEBSTR(bstrPassword);
    }
    reg.SetKey(NULL);
    return S_OK;
}

//
// ProcessRemoveSubscriptions
//
HRESULT ProcessRemoveSubscriptions(HKEY hkey)
{
    // Enumerate the subscription keys in the RemoveSubscriptions key
    HRESULT hr;
    CRegKey reg;
    reg.SetKey(hkey);
    TCHAR szSubscription[MAX_PATH];
    while (S_OK == reg.Next(szSubscription))
    {
        // Find the URL to delete
        CRegKey regSubscription;
        regSubscription.OpenForRead(hkey, szSubscription);
        BSTR bstrURL;
        if (SUCCEEDED(regSubscription.GetBSTRValue(c_szURL, &bstrURL)))
        {
            ISubscriptionMgr *pSubMgr = NULL;
            hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr, (void**)&pSubMgr);
            if (SUCCEEDED(hr))
            {
                hr = pSubMgr->DeleteSubscription(bstrURL, NULL);
                pSubMgr->Release();
            }
            SysFreeString(bstrURL);
        }
    }
    reg.SetKey(NULL);
    return S_OK;
}

//
// PRIVATE VERSION HANDLING CODE - REVIEW THIS CODE SHOULD HAVE BEEN STOLEN 
// FROM SETUP
//
struct MYVERSION
{
    DWORD dw1;  // most sig version number
    DWORD dw2;
    DWORD dw3;
    DWORD dw4;  // least sig version number
};

int CompareDW(DWORD dw1, DWORD dw2)
{
    if (dw1 > dw2)
        return 1;
    if (dw1 < dw2)
        return -1;

    return 0;
}

int CompareVersion(MYVERSION * pv1, MYVERSION * pv2)
{
    int rv;

    rv = CompareDW(pv1->dw1, pv2->dw1);

    if (rv == 0)
    {
        rv = CompareDW(pv1->dw2, pv2->dw2);

        if (rv == 0)
        {
            rv = CompareDW(pv1->dw3, pv2->dw3);

            if (rv == 0)
            {
                rv = CompareDW(pv1->dw4, pv2->dw4);
            }
        }
    }

    return rv;
}

//
// Returns TRUE if an INT was parsed and *pwsz is NOT NULL
// if a . was found
//
BOOL GetDWORDFromStringAndAdvancePtr(DWORD *pdw, LPWSTR *pwsz)
{
    if (!StrToIntExW(*pwsz, 0, (int *)pdw))
        return FALSE;

    *pwsz = StrChrW(*pwsz, L'.');

    if (*pwsz)
        *pwsz = *pwsz +1;

    return TRUE;
}

BOOL GetVersionFromString(MYVERSION *pver, LPWSTR pwsz)
{
    BOOL rv;

    rv = GetDWORDFromStringAndAdvancePtr(&pver->dw1, &pwsz);
    if (!rv || pwsz == NULL)
        return FALSE;

    rv = GetDWORDFromStringAndAdvancePtr(&pver->dw2, &pwsz);
    if (!rv || pwsz == NULL)
        return FALSE;

    rv = GetDWORDFromStringAndAdvancePtr(&pver->dw3, &pwsz);
    if (!rv || pwsz == NULL)
        return FALSE;

    rv = GetDWORDFromStringAndAdvancePtr(&pver->dw4, &pwsz);
    if (!rv)
        return FALSE;

    return TRUE;
}

//
// ProcessRemoveAllChannels
//
HRESULT ProcessRemoveAllChannels(HKEY hkey)
{
    HRESULT hr;
    HINSTANCE hAdvPack = NULL;
    DELNODE pfDELNODE = NULL;
    IChannelMgrPriv *pChannelMgrPriv = NULL;
    CRegKey regAdd;
    regAdd.SetKey(hkey);
    TCHAR szChannelFolder[MAX_PATH];

    hr = CoCreateInstance(CLSID_ChannelMgr, NULL, CLSCTX_INPROC_SERVER, IID_IChannelMgrPriv, (void**)&pChannelMgrPriv);
    if (FAILED(hr))
    {
        goto Exit;
    }

    if ((hAdvPack = LoadLibrary(TEXT("advpack.dll"))) != NULL) 
    {
        pfDELNODE = (DELNODE)GetProcAddress( hAdvPack, "DelNode");
        if (!pfDELNODE) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Loop Through Channel Folders to delete
    while (S_OK == regAdd.Next(szChannelFolder))
    {
        DWORD dwSoftware = 0, dwChannelGuide = 0;
        CRegKey regChannelFolder;
 
        CHAR szChannelPath[MAX_PATH];
        TCHAR szChannelPathT[MAX_PATH];
        TCHAR szFavsT[MAX_PATH]; //Retrieve Unicode data from registry

        BSTR bstrOldIEVersion = NULL;
        BOOL bVersion = TRUE;
        regChannelFolder.OpenForRead(hkey, szChannelFolder);
        
        // Check whether old IE version is correct.
        hr = regChannelFolder.GetBSTRValue(c_szOldIEVersion, &bstrOldIEVersion);
        if (SUCCEEDED(hr) && bstrOldIEVersion)
        {
            CRegKey regKeyIESetup;
            hr = regKeyIESetup.OpenForRead(HKEY_LOCAL_MACHINE, c_szRegKeyIESetup);
    
            if (SUCCEEDED(hr))
            {
                BSTR bstrRealOldIEVersion = NULL;
                hr = regKeyIESetup.GetBSTRValue(c_szOldIEVersion, &bstrRealOldIEVersion);
                if (SUCCEEDED(hr) && bstrRealOldIEVersion)
                {
                    MYVERSION verOldIEVersion, verRealOldIEVersion;

                    if (GetVersionFromString(&verOldIEVersion,     bstrOldIEVersion) &&
                        GetVersionFromString(&verRealOldIEVersion, bstrRealOldIEVersion))
                    {
                        //
                        // If the old version of IE that was on this machine (verRealOldIEVersion)
                        // is infact NEWER than the old version number in the CABs that we want to 
                        // delete (verOldIEVersion) then dont blow away old channel folder.
                        // Otherwise default to blow away channels.
                        //
                        if (CompareVersion(&verRealOldIEVersion, &verOldIEVersion) > 0)
                        {
                            bVersion = FALSE;
                        }
                    }

                    SAFEFREEBSTR(bstrRealOldIEVersion);
                }
            }
            SAFEFREEBSTR(bstrOldIEVersion);
        }

        if (!bVersion)
        {
            continue;
        }
        
        hr = regChannelFolder.GetValue(c_szChannelGuide, &dwChannelGuide);
        if (FAILED(hr) || (SUCCEEDED(hr) && !dwChannelGuide))
        {
            if (SUCCEEDED(pChannelMgrPriv->GetChannelFolderPath(szChannelPath, MAX_PATH, IChannelMgrPriv::CF_CHANNEL)))
            {
                // Retrieve Favorites Path from registry
                if (SUCCEEDED(Channel_GetBasePath((LPTSTR)szFavsT, ARRAYSIZE(szFavsT))))
                {   
                    // Convert from ANSI
                    SHAnsiToTChar(szChannelPath, szChannelPathT, ARRAYSIZE(szChannelPathT));
                    // If channel folder doesn't exist, then szChannelPath will contain the Favorites path.
                    // Don't delete the entries.
                    if (StrCmpI(szFavsT, szChannelPathT))
                       pfDELNODE(szChannelPath, ADN_DONT_DEL_DIR);
                }
            }
        }

        hr = regChannelFolder.GetValue(c_szSoftware, &dwSoftware);
        if (FAILED(hr) || (SUCCEEDED(hr) && !dwSoftware))
        {
            if (SUCCEEDED(pChannelMgrPriv->GetChannelFolderPath(szChannelPath, MAX_PATH, IChannelMgrPriv::CF_SOFTWAREUPDATE)))
            {
                pfDELNODE(szChannelPath, ADN_DONT_DEL_DIR);
            }
        }

        hr = S_OK;                        
    }
    regAdd.SetKey(NULL);

Exit:
    SAFERELEASE(pChannelMgrPriv);
    
    if (hAdvPack) {
        FreeLibrary(hAdvPack);
    }

    return hr;
}

//
// ProcessAddScheduleGroups
//
HRESULT ProcessAddScheduleGroups(HKEY hkey)
{
    // Enumerate the schedule group keys in the AddScheduleGroups key
    HRESULT hr;
    CRegKey reg;
    reg.SetKey(hkey);
    TCHAR szScheduleGroup[MAX_PATH];
    while (S_OK == reg.Next(szScheduleGroup))
    {
        // Read the Title and the Earliest, Latest, and Interval times
        // BUGBUG: Currently times must be in minutes.
        // BUGBUG: Currently we don't look for StartDate or EndDate.
        CRegKey regScheduleGroup;
        regScheduleGroup.OpenForRead(hkey, szScheduleGroup);
        BSTR bstrTitle;
        DWORD dwET, dwIT, dwLT;
        hr = regScheduleGroup.GetBSTRValue(c_szTitle, &bstrTitle);
        if (SUCCEEDED(hr)
            && SUCCEEDED(regScheduleGroup.GetValue(c_szEarliestTime, &dwET))
            && SUCCEEDED(regScheduleGroup.GetValue(c_szIntervalTime, &dwIT))
            && SUCCEEDED(regScheduleGroup.GetValue(c_szLatestTime, &dwLT)))
        {
            INotificationMgr *pNotMgr = NULL;
            hr = CoCreateInstance(CLSID_StdNotificationMgr, NULL, CLSCTX_INPROC_SERVER, IID_INotificationMgr, (void**)&pNotMgr);
            if (SUCCEEDED(hr))
            {
                // Create the schedule group
                IScheduleGroup *pScheduleGroup = NULL;
                NOTIFICATIONCOOKIE cookie;
                hr = pNotMgr->CreateScheduleGroup(0, &pScheduleGroup, &cookie, 0);
                if (SUCCEEDED(hr))
                {
                    TASK_TRIGGER tt;
                    GROUPINFO gi = { 0 };
                    SYSTEMTIME st;
                    GetLocalTime(&st);
                    tt.cbTriggerSize = sizeof(tt);
                    if (SUCCEEDED(ScheduleToTaskTrigger(&tt, &st, NULL, (long)dwIT, (long)dwET, (long)dwLT)))
                    {
                        gi.cbSize = sizeof(GROUPINFO);
                        gi.pwzGroupname = bstrTitle;
                    
                        hr = pScheduleGroup->SetAttributes(&tt, NULL, &cookie, &gi, 0);
                    }
                    pScheduleGroup->Release();
                }
                pNotMgr->Release();
            }
        }
        SAFEFREEBSTR(bstrTitle);
    }
    reg.SetKey(NULL);
    return S_OK;
}

//
// ProcessRemoveScheduleGroups
//
HRESULT ProcessRemoveScheduleGroups(HKEY hkey)
{
    // Enumerate the schedule group keys in the RemoveScheduleGroups key
    HRESULT hr;
    CRegKey reg;
    reg.SetKey(hkey);
    TCHAR szScheduleGroup[MAX_PATH];
    while (S_OK == reg.Next(szScheduleGroup))
    {
        // Find the title to delete
        CRegKey regScheduleGroup;
        regScheduleGroup.OpenForRead(hkey, szScheduleGroup);
        BSTR bstrTitle;
        if (SUCCEEDED(regScheduleGroup.GetBSTRValue(c_szTitle, &bstrTitle)))
        {
            GUID groupCookie;
            if (SUCCEEDED(FindScheduleGroupFromName(bstrTitle, &groupCookie)))
            {
                INotificationMgr *pNotMgr = NULL;
                hr = CoCreateInstance(CLSID_StdNotificationMgr, NULL, CLSCTX_INPROC_SERVER, IID_INotificationMgr, (void**)&pNotMgr);
                if (SUCCEEDED(hr))
                {
                    hr = pNotMgr->RevokeScheduleGroup(&groupCookie, NULL, 0);
                    pNotMgr->Release();
                }
            }
            SysFreeString(bstrTitle);
        }
    }
    reg.SetKey(NULL);
    return S_OK;
}

//
// ProcessAddDesktopComponents
//
HRESULT ProcessAddDesktopComponents(HKEY hkey)
{
    return S_OK;
}

//
// ProcessRemoveDesktopComponents
//
HRESULT ProcessRemoveDesktopComponents(HKEY hkey)
{
    // Enumerate the component keys in the ProcessRemoveDesktopComponents key
    // HRESULT hr;
    CRegKey reg;
    reg.SetKey(hkey);
    TCHAR szComponent[MAX_PATH];
    while (S_OK == reg.Next(szComponent))
    {
        // Find the URL to delete
        CRegKey regComponent;
        regComponent.OpenForRead(hkey, szComponent);
        BSTR bstrURL;
        if (SUCCEEDED(regComponent.GetBSTRValue(c_szURL, &bstrURL)))
        {
            SysFreeString(bstrURL);
        }
    }
    reg.SetKey(NULL);
    return S_OK;
}


//
// NoChannelUI processing.
//

#define SHELLFOLDERS \
   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")


typedef enum _tagXMLDOCTYPE {
    DOC_CHANNEL,
    DOC_SOFTWAREUPDATE
} XMLDOCTYPE;

//
// Get the path to the favorites directory.
//
HRESULT Channel_GetBasePath(LPTSTR pszPath, int cch)
{
    ASSERT(pszPath || 0 == cch);

    HRESULT hr = E_FAIL;

    HKEY hKey;
    DWORD dwLen = cch;

    if (RegOpenKey(HKEY_CURRENT_USER, SHELLFOLDERS, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hKey, TEXT("Favorites"), NULL, NULL,
                            (LPBYTE)pszPath, &dwLen) == ERROR_SUCCESS)
        {
            hr = S_OK;
        }
        RegCloseKey(hKey);
    }
    
    return hr;
}

HRESULT Channel_GetFolder(LPTSTR pszPath, XMLDOCTYPE xdt )
{
    TCHAR   szFavs[MAX_PATH];
    TCHAR   szChannel[MAX_PATH];
    HRESULT hr = E_FAIL;

    if (SUCCEEDED(Channel_GetBasePath(szFavs, ARRAYSIZE(szFavs))))
    {
        //
        // Get the potentially localized name of the Channel folder from
        // tack this on the Favorites path 
        //
        MLLoadString(
                   ((xdt == DOC_CHANNEL)? IDS_CHANNEL_FOLDER : IDS_SOFTWAREUPDATE_FOLDER),
                   szChannel, MAX_PATH);
        PathCombine(pszPath, szFavs, szChannel);

        hr = S_OK;
    }
    return hr;
}

//
// Set/Clear the "hidden" attribute of a channel directory.
//

void ShowChannelDirectory(BOOL fShow, XMLDOCTYPE xdt)
{
    TCHAR szPath[MAX_PATH];
    DWORD dwAttributes;

    if (SUCCEEDED(Channel_GetFolder(szPath, xdt)))
    {
        dwAttributes = GetFileAttributes(szPath);
        
        if (0xffffffff != dwAttributes)
        {
            if (fShow && (dwAttributes & FILE_ATTRIBUTE_HIDDEN))
            {
                SetFileAttributes(szPath, dwAttributes & ~FILE_ATTRIBUTE_HIDDEN);
            }
            else if (!fShow && !(dwAttributes & FILE_ATTRIBUTE_HIDDEN))
            {
                SetFileAttributes(szPath, dwAttributes | FILE_ATTRIBUTE_HIDDEN);
            }
        }
    }

    return;
}

//
// Hide or show channel directories
//

void ShowChannelDirectories(BOOL fShow)
{
    ShowChannelDirectory(fShow, DOC_CHANNEL);
    ShowChannelDirectory(fShow, DOC_SOFTWAREUPDATE);

    return;
}
