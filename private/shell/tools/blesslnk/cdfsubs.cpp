#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <memory.h>
#include <subsmgr.h>
#include <mstask.h>
#include <wininet.h>
#include "cdfsubs.hpp"

// CDFAutoSubscribe
//
// Create the cache entry and subscribe to the channel.
//
// BUGBUG: Right now, if you create the cache entry for the CDF, subscribing
// to the channel doesn't work properly. Updating the channel will fail.
//

BOOL WINAPI CDFAutoSubscribe(HWND hwnd, LPSTR szFriendlyName, LPSTR szCDFUrl,
                             LPSTR szCDFFilePath, BOOL bSilent)
{
    BOOL                   bRet = FALSE;

    if (CreateCDFCacheEntry(szCDFUrl, szCDFFilePath)) {
        if (SubscribeChannel(hwnd, szFriendlyName, szCDFUrl, bSilent)) {
            bRet = TRUE;
        }
    }

    return bRet;
}

//
// CreateCDFCacheEntry
//

BOOL CreateCDFCacheEntry(LPSTR szCDFUrl, LPSTR szLocalFileName)
{
    BOOL                           bRet;
    FILETIME                       ftExpireTime;
    FILETIME                       ftLastMod;
    TCHAR                          achFileName[MAX_PATH];

    bRet = FALSE;
    GetSystemTimeAsFileTime(&ftLastMod);
    ftExpireTime.dwLowDateTime = (DWORD)0;
    ftExpireTime.dwHighDateTime = (DWORD)0;

    if (CreateUrlCacheEntry(szCDFUrl, 0, NULL, achFileName, 0)) {
        CopyFile(szLocalFileName, achFileName, FALSE);
        if (CommitUrlCacheEntry(szCDFUrl, achFileName, ftExpireTime,
                                ftLastMod, 0, NULL, 0, NULL, 0)) {
            bRet = TRUE;
        }
    }

    return bRet;
}

//
// SubscribeChannel
//

BOOL SubscribeChannel(HWND hwnd, LPSTR szName, LPSTR szURL, BOOL bSilent)
{
    ISubscriptionMgr              *pISubscriptionMgr = NULL;
    DWORD                          dwFlags = 0;
    BOOL                           bRet = FALSE;
    HRESULT                        hr = S_OK;
    SUBSCRIPTIONTYPE               subsType = SUBSTYPE_CHANNEL;
    SUBSCRIPTIONINFO               subsInfo;
    LPWSTR                         wzName = NULL;
    LPWSTR                         wzURL = NULL;

    if (FAILED(Ansi2Unicode(szName, &wzName))) {
        goto Exit;
    }

    if (FAILED(Ansi2Unicode(szURL, &wzURL))) {
        goto Exit;
    }

    hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER,
                          IID_ISubscriptionMgr, (void **)&pISubscriptionMgr);
    if (FAILED(hr)) {
        goto Exit;
    }
                          
    ZeroMemory(&subsInfo, sizeof(SUBSCRIPTIONINFO));
    subsInfo.cbSize = sizeof(SUBSCRIPTIONINFO);
    subsInfo.fUpdateFlags = SUBSINFO_CHANNELFLAGS | SUBSINFO_TASKFLAGS | SUBSINFO_GLEAM;
    subsInfo.bGleam = TRUE;

    subsInfo.fChannelFlags |= CHANNEL_AGENT_PRECACHE_ALL |
                              CHANNEL_AGENT_DYNAMIC_SCHEDULE;

    subsInfo.fTaskFlags = TASK_FLAG_START_ONLY_IF_IDLE | 
                          TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET;
    
    dwFlags = CREATESUBS_ADDTOFAVORITES | CREATESUBS_SOFTWAREUPDATE;
    if (bSilent) {
        dwFlags |= CREATESUBS_NOUI;
    }

    hr = pISubscriptionMgr->CreateSubscription(hwnd, wzURL, wzName,
                                               dwFlags, subsType, &subsInfo);

    if (hr == S_OK) {
        bRet = TRUE;
    }

    pISubscriptionMgr->Release();

Exit:

    if (wzName) {
        delete wzName;
    }

    if (wzURL) {
        delete wzURL;
    }

    return bRet;
}

// Ansi2Unicode: Convert ANSI strings to Unicode
// Ripped from urlmon\download\helpers.cxx

HRESULT Ansi2Unicode(const char * src, wchar_t **dest)
{
    if ((src == NULL) || (dest == NULL))
        return E_INVALIDARG;

    // find out required buffer size and allocate it
    int len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, NULL, 0);
    *dest = new WCHAR [len*sizeof(WCHAR)];
    if (!*dest)
        return E_OUTOFMEMORY;

    // Do the actual conversion.
    if ((MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, *dest, 
                                                    len*sizeof(wchar_t))) != 0)
        return S_OK; 
    else
        return E_FAIL;
}

