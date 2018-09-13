//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       trayagnt.cpp
//
//  Contents:   tray notification agent
//
//  Classes:
//
//  Functions:
//
//  History:    01-14-1997  rayen (Raymond Endres)  Created
//
//----------------------------------------------------------------------------
#include "private.h"
#include "updateui.h"
#include "chanmgr.h"
#include "chanmgrp.h"
#include "shguidp.h"
#include "offline.h"
#include "offl_cpp.h"

//xnotfmgr - can probably nuke most of this file

#undef TF_THISMODULE
#define TF_THISMODULE   TF_TRAYAGENT

extern const TCHAR c_szTrayUI[] = TEXT("BogusClassName");
const TCHAR c_szTrayMenu[] = TEXT("TrayMenu");

#if WANT_REGISTRY_LOG
const TCHAR c_szLog[] = TEXT("\\Log");
const TCHAR c_szLogMaxIndex[] = TEXT("MaxIndex");
#endif

typedef struct _tagCHANNELIMAGEDATA
{
    CHAR             szPath[MAX_PATH];
    CHAR             szHash[MAX_PATH];
    int              iIndex;
    UINT             uFlags;
    int              iImageIndex;
} CHANNELIMAGEDATA, *PCHANNELIMAGEDATA;


CTrayUI *g_pTrayUI;     // TrayUI object (not COM), separate from TrayAgent for now

DWORD WINAPI UpdateRequest(UINT idCmd, INotification *);
HRESULT UpdateNotifyReboot(void);
extern BOOL OnConnectedNotification(void);
extern BOOL OnDisconnectedNotification(void);
extern HRESULT LoadWithCookie(LPCTSTR, OOEBuf *, DWORD *, SUBSCRIPTIONCOOKIE *);
void DoReboot(void);
void IdleBegin(HWND hwnd);
void IdleEnd(void);

// Private message sent by Channel Screen Saver to
// periodically initiate relaunch of the screen saver.
#define WM_LOADSCREENSAVER  (WM_USER+550)
extern BOOL ReloadChannelScreenSaver(); // from SSSEPROX.CPP

//----------------------------------------------------------------------------
// IEPlaySound - stolen from shdocvw
// Plays one of registered sounds asynchronously
//----------------------------------------------------------------------------
#if 0
void IEPlaySound(LPCTSTR pszSound)
{
    TCHAR szKey[MAX_PATH];

    // check the registry first
    // if there's nothing registered, we blow off the play,
    // but we don't set the MM_DONTLOAD flag so taht if they register
    // something we will play it
#error Potential Buffer overflow in wsprintf:
    wsprintf(szKey, TEXT("AppEvents\\Schemes\\Apps\\Explorer\\%s\\.current"), pszSound);

    TCHAR szFileName[MAX_PATH];
    LONG cbSize = SIZEOF(szFileName);

    if ((RegQueryValue(HKEY_CURRENT_USER, szKey, szFileName, &cbSize) == ERROR_SUCCESS) && cbSize)
    {
        //
        // Unlike SHPlaySound in shell32.dll, we get the registry value
        // above and pass it to PlaySound with SND_FILENAME instead of
        // SDN_APPLICATION, so that we play sound even if the application
        // is not Explroer.exe (such as IExplore.exe or WebBrowserOC).
        //
        PlaySound(szFileName, NULL, SND_FILENAME | SND_ASYNC);
    }
}
#endif

#ifdef DEBUG
extern INotificationSink * g_pOfflineTraySink;
HRESULT RunTest()
{
//xnotfmgr

    INotificationMgr * pMgr = NULL;
    INotification   * pNotf = NULL;
    HRESULT             hr;

    hr = GetNotificationMgr(&pMgr);
    if (FAILED(hr) || !pMgr)
        return E_FAIL;

    hr = pMgr->CreateNotification(
            NOTIFICATIONTYPE_AGENT_START,
            0, NULL, &pNotf, 0);

    if (SUCCEEDED(hr) && !pNotf)
        hr = E_FAIL;

    if (SUCCEEDED(hr))
        hr = pMgr->DeliverNotification(pNotf, CLSID_WebCrawlerAgent, 
                DM_DELIVER_DEFAULT_PROCESS |
                DM_THROTTLE_MODE |
                DM_NEED_COMPLETIONREPORT
                ,g_pOfflineTraySink,0,0);

    SAFERELEASE(pNotf);
    SAFERELEASE(pMgr);
    return hr;
}
#endif

HRESULT CancelAllDownloads()
{
//xnotfmgr
    HRESULT hr = S_OK;
    INotificationMgr    * pMgr = NULL;
    IEnumNotification   * pRunEnum = NULL;
    IEnumNotification   * pThrEnum = NULL;

    hr = GetNotificationMgr(&pMgr);
    if (FAILED(hr))  {
        return hr;
    }

    hr = pMgr->GetEnumNotification(EF_NOTIFICATION_INPROGRESS, &pRunEnum);
    if (SUCCEEDED(hr))  {
        hr = pMgr->GetEnumNotification(EF_NOTIFICATION_THROTTLED, &pThrEnum);
    }

    if (FAILED(hr))  {
        SAFERELEASE(pRunEnum);
        SAFERELEASE(pThrEnum);
        SAFERELEASE(pMgr);
        return hr;
    }

    INotification *pNotCancel = NULL;
    hr = pMgr->CreateNotification(NOTIFICATIONTYPE_TASKS_ABORT,
                               (NOTIFICATIONFLAGS)0,
                               NULL,
                               &pNotCancel,
                               0);

    if (FAILED(hr) || !pNotCancel){
        SAFERELEASE(pMgr);
        SAFERELEASE(pRunEnum);
        SAFERELEASE(pThrEnum);
        return E_FAIL;
    }

    NOTIFICATIONITEM    item = {0};
    ULONG               cItems = 0;
    item.cbSize = sizeof(NOTIFICATIONITEM);

    hr = pThrEnum->Next(1, &item, &cItems);
    while ( SUCCEEDED(hr) && cItems )
    {
        CLSID   cookie;
        BOOL    bAbort = FALSE;
        if (item.NotificationType == NOTIFICATIONTYPE_AGENT_START 
            //  REVIEW big hack.
            && item.clsidDest != CLSID_ConnectionAgent)
        {
            bAbort = TRUE;
            cookie = item.NotificationCookie;
        }
        SAFERELEASE(item.pNotification);
        item.cbSize = sizeof(NOTIFICATIONITEM);
        cItems = 0;
        hr = pThrEnum->Next(1, &item, &cItems);

        //  REVIEW. This sucks. If we put the following statement before
        //  the Next statement, we can delete the package and then break the
        //  enumerator.
        if (bAbort) {
            HRESULT hr1 = pMgr->DeliverReport(pNotCancel, &cookie, 0);
            ASSERT(SUCCEEDED(hr1));
        }
    }

    cItems = 0;
    item.cbSize = sizeof(NOTIFICATIONITEM);

    hr = pRunEnum->Next(1, &item, &cItems);
    while ( SUCCEEDED(hr) && cItems )
    {
        CLSID   cookie;
        BOOL    bAbort = FALSE;
        if (item.NotificationType == NOTIFICATIONTYPE_AGENT_START 
            //  REVIEW big hack.
            && item.clsidDest != CLSID_ConnectionAgent)
        {
            bAbort = TRUE;
            cookie = item.NotificationCookie;
        }
        SAFERELEASE(item.pNotification);
        item.cbSize = sizeof(NOTIFICATIONITEM);
        cItems = 0;
        hr = pRunEnum->Next(1, &item, &cItems);

        //  REVIEW. We don't seem to have the same problem. Just try to be
        //  cautious.
        if (bAbort) {
            HRESULT hr1 = pMgr->DeliverReport(pNotCancel, &cookie, 0);
            ASSERT(SUCCEEDED(hr1));
        }
    }

    SAFERELEASE(pMgr);
    SAFERELEASE(pNotCancel);
    SAFERELEASE(pRunEnum);
    SAFERELEASE(pThrEnum);
    return hr;
}

//
// Get the path of channel containing the given URL.
//

HRESULT GetChannelPath(LPCSTR pszURL, LPTSTR pszPath, int cch,
                       IChannelMgrPriv** ppIChannelMgrPriv)
{
    ASSERT(pszURL);
    ASSERT(pszPath || 0 == cch);
    ASSERT(ppIChannelMgrPriv);

    HRESULT hr;
    BOOL    bCoinit = FALSE;

    hr = CoCreateInstance(CLSID_ChannelMgr, NULL, CLSCTX_INPROC_SERVER,
                          IID_IChannelMgrPriv, (void**)ppIChannelMgrPriv);

    if ((hr == CO_E_NOTINITIALIZED || hr == REGDB_E_IIDNOTREG) &&
        SUCCEEDED(CoInitialize(NULL)))
    {
        bCoinit = TRUE;
        hr = CoCreateInstance(CLSID_ChannelMgr, NULL, CLSCTX_INPROC_SERVER,
                          IID_IChannelMgrPriv, (void**)ppIChannelMgrPriv);
    }

    if (SUCCEEDED(hr))
    {
        ASSERT(*ppIChannelMgrPriv);

        IChannelMgr* pIChannelMgr;

        hr = (*ppIChannelMgrPriv)->QueryInterface(IID_IChannelMgr,
                                                (void**)&pIChannelMgr);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIChannelMgr);

            WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
            MyStrToOleStrN(wszURL, ARRAYSIZE(wszURL), pszURL);

            IEnumChannels* pIEnumChannels;

            hr = pIChannelMgr->EnumChannels(CHANENUM_ALLFOLDERS | CHANENUM_PATH,
                                            wszURL, &pIEnumChannels);

            if (SUCCEEDED(hr))
            {
                ASSERT(pIEnumChannels);

                CHANNELENUMINFO ci;

                if (S_OK == pIEnumChannels->Next(1, &ci, NULL))
                {
                    MyOleStrToStrN(pszPath, cch, ci.pszPath);
                    
                    CoTaskMemFree(ci.pszPath);
                }
                else
                {
                    hr = E_FAIL;
                }

                pIEnumChannels->Release();
            }

            pIChannelMgr->Release();
        }

    }

    if (bCoinit)
        CoUninitialize();

    ASSERT((SUCCEEDED(hr) && *ppIChannelMgrPriv) || FAILED(hr));

    return hr;
}

//
// Update channel
//

HRESULT UpdateChannel(IChannelMgrPriv* pIChannelMgrPriv, LPCSTR pszURL)
{
    ASSERT(pIChannelMgrPriv);
    ASSERT(pszURL);

    HRESULT hr;

    IChannelMgr* pIChannelMgr;

    hr = pIChannelMgrPriv->QueryInterface(IID_IChannelMgr,
                                          (void**)&pIChannelMgr);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIChannelMgr);

        WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
        MyStrToOleStrN(wszURL, ARRAYSIZE(wszURL), pszURL);

        IEnumChannels* pIEnumChannels;

        hr = pIChannelMgr->EnumChannels(CHANENUM_ALLFOLDERS | CHANENUM_PATH,
                                        wszURL, &pIEnumChannels);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIEnumChannels);

            CHANNELENUMINFO ci;

            //
            // Update all instances of this channel.
            //

            while (S_OK == pIEnumChannels->Next(1, &ci, NULL))
            {
                char szPath[MAX_PATH];
                MyOleStrToStrN(szPath, ARRAYSIZE(szPath), ci.pszPath);

                // Removed to give UPDATEIMAGE (gleams) a better chance.
                //SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, (void*)szPath,
                //               NULL);

                CoTaskMemFree(ci.pszPath);
            }    

            pIEnumChannels->Release();
        }

        pIChannelMgr->Release();
    }    

    return hr;
}

//
// Gather data that will be used to update a channel image.
//

HRESULT PreUpdateChannelImage(IChannelMgrPriv* pIChannelMgrPriv,
                              CHANNELIMAGEDATA* pcid)
{
    ASSERT(pcid); 
    ASSERT(pIChannelMgrPriv);

    return (pIChannelMgrPriv)->PreUpdateChannelImage(pcid->szPath,
                                                     pcid->szHash,
                                                     &pcid->iIndex,
                                                     &pcid->uFlags,
                                                     &pcid->iImageIndex);
}

//
// Update a channel image.
//

HRESULT UpdateChannelImage(IChannelMgrPriv* pIChannelMgrPriv,
                          CHANNELIMAGEDATA* pcid)
{
    ASSERT(pcid);
    ASSERT(pIChannelMgrPriv);

    WCHAR wszHash[MAX_PATH];
    MyStrToOleStrN(wszHash, ARRAYSIZE(wszHash), pcid->szHash);

    return pIChannelMgrPriv->UpdateChannelImage(wszHash, pcid->iIndex,
                                                pcid->uFlags,
                                                pcid->iImageIndex);
}

//----------------------------------------------------------------------------
// Tray Agent object
//----------------------------------------------------------------------------

CTrayAgent::CTrayAgent()
{
    DBG("Creating CTrayAgent object");

    //
    // Maintain global count of objects in webcheck.dll
    //
    DllAddRef();

    //
    // Initialize object member variables
    //
    m_cRef = 1;
#ifdef DEBUG
    m_AptThreadId = GetCurrentThreadId();
#endif
}

CTrayAgent::~CTrayAgent()
{
    //
    // Maintain global count of objects
    //
    DllRelease();

    //
    // Release/delete any resources
    //
    DBG("Destroyed CTrayAgent object");
}

//
// IUnknown members
//

STDMETHODIMP_(ULONG) CTrayAgent::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CTrayAgent::Release(void)
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CTrayAgent::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv = NULL;
//xnotfmgr

    //
    // Currently just support INotificationSink
    //
    if ((IID_IUnknown == riid) ||
        (IID_INotificationSink == riid))
    {
        *ppv = (INotificationSink *)this;
    }

    //
    // Addref through the interface
    //
    if( NULL != *ppv )
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}


//
// INotificationSink member(s)
//
STDMETHODIMP CTrayAgent::OnNotification(
    LPNOTIFICATION         pNotification,
    LPNOTIFICATIONREPORT   pNotificationReport,
    DWORD                  dwReserved)
{
//xnotfmgr
    DBG("CTrayAgent::OnNotification called");
    ASSERT(pNotification);
    ASSERT(GetCurrentThreadId() == m_AptThreadId);

    //
    // Extract Notification Type.
    //
    HRESULT          hr;
    NOTIFICATIONTYPE notfType;
    hr = pNotification->GetNotificationInfo(&notfType, NULL, NULL, NULL, 0);
    ASSERT(SUCCEEDED(hr));

    if (notfType == NOTIFICATIONTYPE_CONFIG_CHANGED)
    {
        DBG("CTrayAgent::OnNotification - config changed");
        //
        // The global properties have changed and we must respond.
        //
        ASSERT(g_pTrayUI);  // Fault here means NotificationMgr calling me in wrong process
        if (g_pTrayUI)
            g_pTrayUI->ConfigChanged();
        return S_OK;
    }
    else if (notfType == NOTIFICATIONTYPE_BEGIN_REPORT) {
        DBG("CTrayAgent::OnNotification - begin report");
        ASSERT(g_pTrayUI);
        if (g_pTrayUI)
            g_pTrayUI->OnBeginReport(pNotification);
        return S_OK;
    }
    else if (notfType == NOTIFICATIONTYPE_AGENT_START)
    {
        DBG("CTrayAgent::OnNotification - agent start (update now)");
        //
        // The user chose Update Subscriptions Now.
        // BUGBUG: Is this the best notification to use for this?
        // BUGBUG: Are there any properties we care about?  Like a guid to trigger
        // a specific one?
        //
        ASSERT(g_pTrayUI);  // Fault here means NotificationMgr calling me in wrong process
        if (g_pTrayUI)  
            g_pTrayUI->UpdateNow(pNotification);
        return S_OK;
    }
    else if (notfType == NOTIFICATIONTYPE_END_REPORT)
    {
        DBG("CTrayAgent::OnNotification - end report");
        ASSERT(g_pTrayUI);
        if (g_pTrayUI)
            g_pTrayUI->OnEndReport(pNotification);

        #if WANT_REGISTRY_LOG
        //
        // Log all the End Reports
        //
        BSTR    bstrStatus = NULL;
        CLSID   cookie;

        if (g_pTrayUI &&
            SUCCEEDED(ReadBSTR(pNotification,NULL, c_szPropStatusString, &bstrStatus))
            && SUCCEEDED(ReadGUID(pNotification, NULL, c_szStartCookie, &cookie)))
        {
            g_pTrayUI->AddToLog(bstrStatus, CLSID_NULL, cookie);
        } else  {
            DBG_WARN("CTrayAgent::OnNotification/End Report - Not status str");
        }
        SAFEFREEBSTR(bstrStatus);
        #endif

        //
        // Read the status code from the end report.
        //
        SCODE scEndStatus;
        hr = ReadSCODE(pNotification, NULL, c_szPropStatusCode, &scEndStatus);
        if (SUCCEEDED(hr) && SUCCEEDED(scEndStatus))
        {
            //
            // Special feature for desktop HTML:
            // If we receive an end report with "DesktopComponent=1" in it,
            // let the desktop know that it needs to refresh itself.  We always
            // do this instead of only on "changes detected" because desktop
            // component authors don't want to change their CDFs.
            //
            DWORD dwRet;
            HRESULT hr2 = ReadDWORD(pNotification, NULL, c_szPropDesktopComponent, &dwRet);
            if (SUCCEEDED(hr2) && (dwRet == 1))
            {
                IActiveDesktop *pAD = NULL;
                hr2 = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IActiveDesktop, (void**)&pAD);
                DBGASSERT(SUCCEEDED(hr2), "Unable to create ActiveDesktop in order to refresh desktop component");
                if (SUCCEEDED(hr2))
                {
                    ASSERT(pAD);
                    pAD->ApplyChanges(AD_APPLY_FORCE | AD_APPLY_REFRESH | AD_APPLY_BUFFERED_REFRESH);
                    pAD->Release();
                }
            }
        }
        
        //
        // If the delivery agent succeeded and changes were detected
        // notify the appropriate code.
        //
        if (SUCCEEDED(hr) && SUCCEEDED(scEndStatus) && (S_FALSE != scEndStatus))
        {
            //
            // Gleam the Internet Shortcut for the URL if requested.  (EnableShortcutGleam=1)
            // NOTE: End Reports without changes (S_FALSE) were filtered above.
            //
            DWORD dwRet;
            hr = ReadDWORD(pNotification, NULL, c_szPropEnableShortcutGleam, &dwRet);
            if (SUCCEEDED(hr) && dwRet)
            {
                LPSTR strURL = NULL;
                hr = ReadAnsiSTR(pNotification, NULL, c_szPropURL, &strURL);
                if (SUCCEEDED(hr))
                {
                    PROPVARIANT propvar;
                    PropVariantInit(&propvar);
                    hr = IntSiteHelper(strURL, &c_rgPropRead[PROP_FLAGS], &propvar, 1, FALSE);
                    if (SUCCEEDED(hr) && (VT_UI4 == propvar.vt))
                    {
                        // Set our flag without disturbing the others.
                        propvar.ulVal |= PIDISF_RECENTLYCHANGED;  
                    }
                    else
                    {
                        // Be sure to clear the variant if it wasn't a DWORD.
                        PropVariantClear(&propvar);
                        propvar.vt = VT_UI4;
                        propvar.ulVal = PIDISF_RECENTLYCHANGED;  
                    }

                    //
                    // Update channels.
                    //

                    hr = ReadDWORD(pNotification, NULL, c_szPropChannel, &dwRet);
                    BOOL bChannel = SUCCEEDED(hr) && dwRet;

                    CHANNELIMAGEDATA   cid = {0};
                    IChannelMgrPriv*   pIChannelMgrPriv = NULL;
                    HRESULT            hr2 = E_FAIL;

                    if (bChannel)
                    {
                        hr2 = GetChannelPath(strURL, cid.szPath,
                                             ARRAYSIZE(cid.szPath),
                                             &pIChannelMgrPriv);
                        if (SUCCEEDED(hr2))
                            hr2 = PreUpdateChannelImage(pIChannelMgrPriv, &cid);
                    }

                    hr = IntSiteHelper(strURL, &c_rgPropRead[PROP_FLAGS], &propvar, 1, TRUE);
                    DBGASSERT(SUCCEEDED(hr), "CTrayAgent::OnNotification - failed to set gleam.");

                    if (bChannel && SUCCEEDED(hr2))
                    {
                        ASSERT(pIChannelMgrPriv);

                        pIChannelMgrPriv->InvalidateCdfCache();
                        UpdateChannelImage(pIChannelMgrPriv, &cid);
                        UpdateChannel(pIChannelMgrPriv, strURL);

                        pIChannelMgrPriv->Release();
                    }
                }
                MemFree(strURL); // Free the string allocated by ReadAnsiSTR().
            }

            //
            // Send Email to notify the user if requested (EmailNotification=1)
            // NOTE: End Reports without changes (S_FALSE) were filtered above.
            //
            hr = ReadDWORD(pNotification, NULL, c_szPropEmailNotf, &dwRet);
            if (SUCCEEDED(hr) && dwRet)
            {
                hr = SendEmailFromNotification(pNotification);
            }
        }
        else
        {
            DBG_WARN("CTrayAgent::OnNotification - unchanged or failed end report");
        }

        return S_OK;
    }
    else if (notfType == NOTIFICATIONTYPE_DISCONNECT_FROM_INTERNET)
    {
        OnDisconnectedNotification();
        return S_OK;
    }
    else if (notfType == NOTIFICATIONTYPE_CONNECT_TO_INTERNET )
    {
        OnConnectedNotification();
        return S_OK;
    }
    else
    {
        //
        // TrayAgent doesn't handle this notification
        //
        DBG("CTrayAgent::OnNotification - unknown notification");
        return S_OK;
    }
}

//----------------------------------------------------------------------------
// TrayUI object (not COM)
//----------------------------------------------------------------------------

LRESULT CALLBACK TrayUI_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
//xnotfmgr
    //
    // We need this annoying global to make single and double left click work nicely.
    // (USER should have a WM_LBUTTONSINGLECLK message.)
    //
    static int iLeftClick = 0;
    static POINT pt;

    switch (uMsg)
    {
        case WM_USER:   // Subscription icon was clicked
            ASSERT(g_pTrayUI);
            switch (lParam)
            {
#ifdef DEBUG
                case WM_RBUTTONUP:
                    if (iLeftClick) {
                        ASSERT(1 == iLeftClick);
                        KillTimer(hwnd, TIMER_ID_DBL_CLICK);
                        iLeftClick = 0;
                    }
                    GetCursorPos(&pt);
                    g_pTrayUI->OpenContextMenu(&pt);
                    break;
                case WM_LBUTTONUP:
                    if (0 == iLeftClick)    // first left click up
                    {
                        UINT uRet;
                        GetCursorPos(&pt);
                        uRet = SetTimer(hwnd, TIMER_ID_DBL_CLICK, GetDoubleClickTime(), NULL);
                        ASSERT(uRet);
                        iLeftClick = 1;
                    }
                    else    // second left click up
                    {
                        ASSERT(1 == iLeftClick);
                        KillTimer(hwnd, TIMER_ID_DBL_CLICK);
                        iLeftClick = 0;
                        g_pTrayUI->OpenSubscriptionFolder();
                    }
                    break;
#endif
                 case UM_NEEDREBOOT:
                    // forward reboot required flag to update agent
                    if (FAILED(UpdateNotifyReboot()))
                        DoReboot();
                    break;
           }
            return 0;

        case WM_TIMER:
            switch(wParam) {
            case TIMER_ID_DBL_CLICK:
                // Timer went off so it was a single click
                KillTimer(hwnd, TIMER_ID_DBL_CLICK);
                if (1 == iLeftClick)    {
                    iLeftClick = 0;
                    g_pTrayUI->OpenContextMenu(&pt);
                }
                break;
            } /* switch */
            break;

        // Process the Infodelivery Admin Policies on WM_WININICHANGE lParam="Policy"
        case WM_WININICHANGE:
            if (lParam && !lstrcmpi((LPCTSTR)lParam, TEXT("policy")))
            {
                ProcessInfodeliveryPolicies();
            }
            // BUGBUG: This should be done on Policy and another filter, not for
            // all changes.  (The other filter hasn't been defined yet.)

            //  TODO: handle this in the new architecture!
            //SetNotificationMgrRestrictions(NULL);
            break;

        case WM_LOADSCREENSAVER:
        {
            EVAL(ReloadChannelScreenSaver());
            break;
        }

        default:
            break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

CTrayUI::CTrayUI(void)
{
    // There should only be one of these objects
    ASSERT(!g_pTrayUI);
    // Assert that we're zero initialized.
    ASSERT(!m_hwnd);
#if WANT_REGISTRY_LOG
    ASSERT(!m_cLogs);
#endif
    ASSERT(!m_cUpdates);
    ASSERT(!m_fUpdatingTrayIcon);
#ifdef DEBUG
    m_AptThreadId = GetCurrentThreadId();
#endif
}

CTrayUI::~CTrayUI(void)
{
    //
    // Clean up any BSTRs we have around
    //

    //  ZDC Detect ongoing updates?
    ASSERT(m_cUpdates >= 0);
    
#if WANT_REGISTRY_LOG
    ASSERT(m_cLogs >= 0);
    ASSERT(m_cLogs < TRAYUI_CLOGS);
    for (int i = 0; i < m_cLogs; i++)
    {
        SAFEFREEBSTR(m_aLogEntry[i].bstrStatus);
    }
#endif
}

#if WANT_REGISTRY_LOG
#define TRAYLOGVERSION  716

STDMETHODIMP WriteSingleEntry(PLogEntry pLog, LPCTSTR szSubKey, int index)
{

    ASSERT(pLog && szSubKey);
    ASSERT(index >= 0);
    ASSERT(index < TRAYUI_CLOGS);

    //  Check to see if it's a valid log entry.
    if (!(pLog->bstrStatus))    {
        ASSERT(0);
        return E_INVALIDARG;
    }

    TCHAR       szLogName[16];
    wsprintf(szLogName, TEXT("%d"), index);
    
    CRegStream * prLog = new CRegStream(HKEY_CURRENT_USER, szSubKey, szLogName);
    if (prLog)  {
        CLSID       clsidAgent = pLog->clsidAgent;
        CLSID       cookie = pLog->startCookie;
        FILETIME    ftLog = pLog->ftLog;
        int         versionNumber = TRAYLOGVERSION;
        BSTR        bstrStatus = pLog->bstrStatus;
        DWORD       cbWritten;
        int         len;

        prLog->Write(&versionNumber, sizeof(int), &cbWritten);
        ASSERT(sizeof(int) == cbWritten);

        prLog->Write(&clsidAgent, sizeof(CLSID), &cbWritten);
        ASSERT(sizeof(CLSID) == cbWritten);

        prLog->Write(&cookie, sizeof(CLSID), &cbWritten);
        ASSERT(sizeof(CLSID) == cbWritten);

        prLog->Write(&ftLog, sizeof(FILETIME), &cbWritten);
        ASSERT(sizeof(FILETIME) == cbWritten);

        len = lstrlenW(bstrStatus);
        prLog->Write(&len, sizeof(int), &cbWritten);
        ASSERT(sizeof(int) == cbWritten);
        len *= sizeof(WCHAR);
        prLog->Write(bstrStatus, len, &cbWritten);
        ASSERT(len == (int)cbWritten);

        delete prLog;
        return S_OK;
    } else  {
        return E_FAIL;
    }
}

STDMETHODIMP ReadSingleEntry(PLogEntry pLog, LPCTSTR szSubKey, int index)
{
    ASSERT(pLog && szSubKey);
    ASSERT(index >= 0);
    ASSERT(index < TRAYUI_CLOGS);

    //  Check to see if it's a valid log entry.
    if (pLog->bstrStatus)    {
        ASSERT(0);
        SAFEFREEBSTR(pLog->bstrStatus);
    }

    TCHAR       szLogName[16];
    HRESULT     hrLog = E_INVALIDARG;
    wsprintf(szLogName, TEXT("%d"), index);
    
    CRegStream * prLog = new CRegStream(HKEY_CURRENT_USER, szSubKey, szLogName);
    if (prLog && (FALSE == prLog->m_fNewStream)) {
        CLSID       clsidAgent;
        CLSID       cookie;
        FILETIME    ftLog;
        int         versionNumber;
        BSTR        bstrStatus = NULL;
        DWORD       cbWritten;
        int         len;

        if (SUCCEEDED(prLog->Read(&versionNumber, sizeof(int), &cbWritten))
            && (sizeof(int) == cbWritten)
            && (TRAYLOGVERSION == versionNumber))
        {
            hrLog = ERROR_SUCCESS;
        }

        if (ERROR_SUCCESS == hrLog) {
            hrLog = E_INVALIDARG;
            if (SUCCEEDED(prLog->Read(&clsidAgent, sizeof(CLSID), &cbWritten))
                && (sizeof(CLSID) == cbWritten))
            {
                hrLog = ERROR_SUCCESS;
            }
        }

        if (ERROR_SUCCESS == hrLog) {
            hrLog = E_INVALIDARG;
            if (SUCCEEDED(prLog->Read(&cookie, sizeof(CLSID), &cbWritten))
                && (sizeof(CLSID) == cbWritten))
            {
                hrLog = ERROR_SUCCESS;
            }
        }

        if (ERROR_SUCCESS == hrLog) {
            hrLog = E_INVALIDARG;
            if (SUCCEEDED(prLog->Read(&ftLog, sizeof(FILETIME), &cbWritten))
                && (sizeof(FILETIME) == cbWritten))
            {
                hrLog = ERROR_SUCCESS;
            }
        }

        if (ERROR_SUCCESS == hrLog) {
            hrLog = E_INVALIDARG;
            if (SUCCEEDED(prLog->Read(&len, sizeof(int), &cbWritten))
                && (sizeof(int) == cbWritten)
                && (len < INTERNET_MAX_URL_LENGTH)
                && (bstrStatus = SysAllocStringLen(NULL, len))
                && SUCCEEDED(prLog->Read(bstrStatus, len * sizeof(WCHAR), &cbWritten))
                && (len * sizeof(WCHAR) == (int)cbWritten))
            {
                hrLog = ERROR_SUCCESS;
                bstrStatus[len] = 0;
            }
        }

        if (ERROR_SUCCESS == hrLog)  {
            pLog->clsidAgent = clsidAgent;
            pLog->startCookie = cookie;
            pLog->ftLog = ftLog;
            pLog->bstrStatus = bstrStatus;

#ifdef  DEBUG
            WCHAR wszCookie[GUIDSTR_MAX];
            TCHAR tmpTSTR[INTERNET_MAX_URL_LENGTH];

            StringFromGUID2(cookie, wszCookie, ARRAYSIZE(wszCookie));
            MyOleStrToStrN(tmpTSTR, ARRAYSIZE(wszCookie), wszCookie);
            TraceMsg(TF_THISMODULE, TEXT("TrayUI:LoadLog - %s(cookie)"), tmpTSTR);
            MyOleStrToStrN(tmpTSTR, ARRAYSIZE(tmpTSTR), bstrStatus);
            TraceMsg(TF_THISMODULE, TEXT("TrayUI:LoadLog - %s(status)"), tmpTSTR);

            SYSTEMTIME  stLog;
            FileTimeToSystemTime(&ftLog, &stLog);
            GetTimeFormat(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE,
                                &stLog, NULL, tmpTSTR, ARRAYSIZE(tmpTSTR));
            TraceMsg(TF_THISMODULE, TEXT("TrayUI:LoadLog - %s(time)"), tmpTSTR);
#endif
        } else  {
            SAFEFREEBSTR(bstrStatus);                        
        }
    }

    if (prLog)
        delete prLog;

    return hrLog;
}

STDMETHODIMP CTrayUI::SyncLogWithReg(int index, BOOL fWriteMax)
{
    if ((index < 0) || (index >= m_cLogs))
        return S_FALSE;

    TCHAR   szSubKey[1024];

    ASSERT((lstrlen(c_szRegKey) + lstrlen(c_szLog) + 2) < 1024);

#error Potential Buffer overflow:
    lstrcpy(szSubKey, c_szRegKey);
    lstrcat(szSubKey, c_szLog);

    HRESULT hr = E_FAIL;

    hr = WriteSingleEntry(&(m_aLogEntry[index]), szSubKey, index);
    if ((ERROR_SUCCESS == hr) && fWriteMax)    {
        CRegStream * prs = 
            new CRegStream(HKEY_CURRENT_USER, szSubKey, c_szLogMaxIndex);
        DWORD   cbWritten;

        if (prs)   {
            prs->Write(&m_cLogs, sizeof(int), &cbWritten);
            ASSERT(sizeof(int) == cbWritten);
            delete prs;
        } else  {
            hr = E_FAIL;
        }
    }

    return hr;
}

STDMETHODIMP CTrayUI::SaveLogToReg(void)
{
    HRESULT hr = S_OK;
    TCHAR   szSubKey[1024];

    ASSERT((lstrlen(c_szRegKey) + lstrlen(c_szLog) + 2) < 1024);

#error Potential Buffer overflow:    
    lstrcpy(szSubKey, c_szRegKey);
    lstrcat(szSubKey, c_szLog);

    int     index = 0;
    int     cIndex = 0;
    ULONG   cbWritten;
    int     maxIndex = m_cLogs;

    for (; index < maxIndex; index ++)  {
        if (SUCCEEDED(
                WriteSingleEntry(&(m_aLogEntry[index]), szSubKey, cIndex)
                     )
            )
        {
            cIndex ++;
        }
    } 

    CRegStream * prs = 
            new CRegStream(HKEY_CURRENT_USER, szSubKey, c_szLogMaxIndex);
    if (prs)   {
        prs->Write(&cIndex, sizeof(int), &cbWritten);
        ASSERT(sizeof(int) == cbWritten);
        delete prs;
    }

    return hr;
}

STDMETHODIMP CTrayUI::LoadLogFromReg(void)
{
    HRESULT hr = S_OK;
    TCHAR   szSubKey[1024];

    ASSERT((lstrlen(c_szRegKey) + lstrlen(c_szLog) + 2) < 1024);

#error Potential Buffer overflow:    
    lstrcpy(szSubKey, c_szRegKey);
    lstrcat(szSubKey, c_szLog);

    m_cLogs = 0;

    int     index = 0;
    ULONG   cbWritten;
    int     len = 0;
    int     maxIndex = 0;
    
    CRegStream * prs = new CRegStream(HKEY_CURRENT_USER, szSubKey, c_szLogMaxIndex);
    
    if (!prs)   {
        hr = E_FAIL;
    } else if (TRUE == prs->m_fNewStream)  {
        hr = E_FAIL;
    } else  {
        if (SUCCEEDED(prs->Read(&maxIndex, sizeof(int), &cbWritten))
                && (sizeof(int) == cbWritten) && (maxIndex >= 0))
        {
            FILETIME    ftFirst;
            SYSTEMTIME  stNow;
    
            if (TRAYUI_CLOGS < maxIndex)    {
                maxIndex = TRAYUI_CLOGS;
            }
    
            m_cLogPtr = -1;
            GetSystemTime(&stNow);
            SystemTimeToFileTime(&stNow, &ftFirst);
    
            for (; index < maxIndex; index ++)  
            {
                PLogEntry pLog = &(m_aLogEntry[m_cLogs]);
                if (SUCCEEDED(ReadSingleEntry(pLog, szSubKey, index)))  {
                   if (-1 == CompareFileTime(&(pLog->ftLog), &ftFirst)) {
                        ftFirst= pLog->ftLog;
                        m_cLogPtr = m_cLogs;
                    }
                    m_cLogs ++;
                }
            } 
    
            //  m_cLogPtr points to the oldest log. If we haven't used up
            //  all log space m_cLogPtr should point to 0. We then adjust
            //  it to the next available log slot.
            if (m_cLogs < TRAYUI_CLOGS) {
                ASSERT(m_cLogPtr == 0);
                m_cLogPtr = m_cLogs;
            }
        }
    }
    if (prs)
        delete prs;

    return hr;
}

STDMETHODIMP CTrayUI::AddToLog(BSTR bstrStatus, CLSID clsidAgent, CLSID startCookie)
{
    ASSERT(bstrStatus);
    BSTR    bstrTmp = NULL;
    bstrTmp = SysAllocString(bstrStatus);

    if (bstrTmp == NULL)
        return E_OUTOFMEMORY;

    PLogEntry   pLog = &(m_aLogEntry[m_cLogPtr]);

    pLog->bstrStatus = bstrTmp;
    pLog->clsidAgent = clsidAgent;
    pLog->startCookie = startCookie;

    SYSTEMTIME  stNow;
    GetSystemTime(&stNow);
    SystemTimeToFileTime(&stNow, &(pLog->ftLog));
    
    if (m_cLogPtr == m_cLogs)    {
        m_cLogs ++;
        SyncLogWithReg(m_cLogPtr, TRUE);
    } else  {
        SyncLogWithReg(m_cLogPtr, FALSE);
    }

    m_cLogPtr ++;
    m_cLogPtr %= TRAYUI_CLOGS;
    return S_OK;
}
#endif // WANT_REGISTRY_LOG

STDMETHODIMP CTrayUI::InitTrayUI(void)
{
    // shouldn't already be initialized
    ASSERT(NULL == m_hwnd);

    // create a hidden window
    WNDCLASS wndclass;

    wndclass.style         = 0;
    wndclass.lpfnWndProc   = TrayUI_WndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = g_hInst;
    wndclass.hIcon         = NULL;
    wndclass.hCursor       = NULL;
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = c_szTrayUI;

    RegisterClass (&wndclass) ;

    m_hwnd = CreateWindow(c_szTrayUI,
                c_szTrayUI,
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                NULL,
                NULL,
                g_hInst,
                NULL);

    DBGASSERT(m_hwnd, "failed to create TrayUI window");
    if (NULL == m_hwnd)
        return ERROR_NOT_ENOUGH_MEMORY;
    ShowWindow(m_hwnd, SW_HIDE);

    //
    // Add an icon to tray after seeing if it's enabled in the registry.
    //
    ASSERT(FALSE == m_fUpdatingTrayIcon);

    // turn on idle monitoring
    IdleBegin(m_hwnd);

    return S_OK;
}

STDMETHODIMP CTrayUI::DestroyTrayUI(void)
{
    // stop idle monitoring
    IdleEnd();

    if (m_hwnd)
    {
        BOOL bRet;
        bRet = DestroyWindow(m_hwnd);
        ASSERT(bRet);
        m_hwnd = NULL;
    }
    return S_OK;
}


STDMETHODIMP CTrayUI::OpenSubscriptionFolder(void)
{
#ifdef DEBUG
    // BUGBUG: This is copied from shdocvw and it might change post beta 1.
    TCHAR szSubPath[MAX_PATH];
    DWORD dwSize = SIZEOF(szSubPath);

    if (ReadRegValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_SUBSCRIPTION,
                       REGSTR_VAL_DIRECTORY, (LPBYTE)szSubPath, dwSize) == FALSE)
    {
        TCHAR szWindows[MAX_PATH];

        GetWindowsDirectory(szWindows, ARRAYSIZE(szWindows));
        PathCombine(szSubPath, szWindows, TEXT("Subscriptions"));
    }

    SHELLEXECUTEINFO shei;
    ZeroMemory(&shei, sizeof(shei));
    shei.cbSize     = sizeof(shei);
    shei.lpFile     = szSubPath;
    shei.nShow      = SW_SHOWNORMAL;
    ShellExecuteEx(&shei);

#endif
    return S_OK;
}

STDMETHODIMP CTrayUI::OpenContextMenu(POINT * pPoint)
{
#ifdef DEBUG
    int     iCmd;
    HMENU   hMenu;
    TCHAR   menuString[MAX_PATH];

    ASSERT(pPoint);
    // SetForegroundWindow(hwnd);
    hMenu = CreatePopupMenu();
    if (!hMenu)
        return E_FAIL;


//    MLLoadString(IDS_SHOWPROG, menuString, MAX_PATH);
//    AppendMenu(hMenu, MF_STRING, IDS_SHOWPROG, menuString);

    MLLoadString(IDS_CANCELDL, menuString, MAX_PATH);
    AppendMenu(hMenu, MF_STRING, IDS_CANCELDL, menuString);

    MLLoadString(IDS_VIEW_SUBS, menuString, MAX_PATH);
    AppendMenu(hMenu, MF_STRING, IDS_VIEW_SUBS, menuString);

    SetMenuDefaultItem(hMenu, IDS_CANCELDL, FALSE);

    if (hMenu)
    {
        //
        // Set the owner window to be foreground as a hack so the
        // popup menu disappears when the user clicks elsewhere.
        //
        BOOL bRet;
        ASSERT(m_hwnd);
        bRet = SetForegroundWindow(m_hwnd);
        ASSERT(bRet);
        iCmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
            pPoint->x, pPoint->y, 0, m_hwnd, NULL);
        DestroyMenu(hMenu);

        MSG msgTmp;
        while (PeekMessage(&msgTmp, m_hwnd, WM_LBUTTONDOWN, WM_LBUTTONUP, PM_REMOVE)) {
            DispatchMessage(&msgTmp);
        }
    }

    switch (iCmd)
    {
        case IDS_SHOWPROG:
            RunTest();
            break;
        case IDS_CANCELDL:
            CancelAllDownloads();
            break;
        case IDS_VIEW_SUBS:
            OpenSubscriptionFolder();
            break;
        default:
            break;
    }

#endif
    return S_OK;
}

STDMETHODIMP CTrayUI::UpdateNow(INotification * pNotification)
{
    //
    // Update Subscriptions Now
    // We really don't want to do this on the caller's thread.  We should
    // always get here through a notification on the appartment thread.
    // (No direct calls from other threads are allowed to avoid race
    // conditions at startup.)
    ASSERT(GetCurrentThreadId() == m_AptThreadId);

    // Essentially we do a PostThreadMessage here to the updating thread.
    return UpdateRequest(UM_ONREQUEST, pNotification);
}


STDMETHODIMP CTrayUI::SetTrayIcon(DWORD fUpdating)
{
#ifdef DEBUG
    if (fUpdating == m_fUpdatingTrayIcon)   {
        return S_OK;
    }

    if (fUpdating)
    {
        BOOL bRet;
        NOTIFYICONDATA NotifyIconData;
        NotifyIconData.cbSize = sizeof(NOTIFYICONDATA);
        NotifyIconData.hWnd = m_hwnd;
        NotifyIconData.uID = 0;
        NotifyIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        NotifyIconData.uCallbackMessage = WM_USER;
        NotifyIconData.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_TRAYICON));
        ASSERT(NotifyIconData.hIcon);
        bRet = MLLoadString(IDS_TRAY_TOOLTIP, NotifyIconData.szTip, ARRAYSIZE(NotifyIconData.szTip));
        ASSERT(bRet);
        bRet = Shell_NotifyIcon(NIM_ADD, &NotifyIconData);
        ASSERT(bRet);
    }
    else
    {
        // Remove the tray icon
        BOOL bRet;
        NOTIFYICONDATA NotifyIconData;
        NotifyIconData.cbSize = sizeof(NOTIFYICONDATA);
        NotifyIconData.hWnd = m_hwnd;
        NotifyIconData.uID = 0;
        NotifyIconData.uFlags = 0;
        bRet = Shell_NotifyIcon(NIM_DELETE, &NotifyIconData);
        ASSERT(bRet);
    }

    m_fUpdatingTrayIcon = fUpdating;
#endif
    return S_OK;
}

STDMETHODIMP CTrayUI::ConfigChanged(void)
{
    return S_OK;
}

STDMETHODIMP CTrayUI::OnEndReport(INotification * pNot)
{
//xnotfmgr
    ASSERT(pNot);
    CLSID   cookie;

    if (SUCCEEDED(ReadGUID(pNot, NULL, c_szStartCookie, &cookie)))
    {
        DBGIID("TrayAgent::OnEndReport - ", cookie);
        UpdateRequest(UM_ENDREPORT, pNot);
        LONG lTmp = InterlockedDecrement(&m_cUpdates);

        if (!lTmp)
            SetTrayIcon(FALSE);

        OOEBuf      ooeBuf;
        LPMYPIDL    newPidl = NULL;
        DWORD       dwSize = 0;

        ZeroMemory((void *)&ooeBuf, sizeof(OOEBuf));
        HRESULT hr = LoadWithCookie(NULL, &ooeBuf, &dwSize, &cookie);

        if (SUCCEEDED(hr))
        {
            newPidl = COfflineFolderEnum::NewPidl(dwSize);
            if (newPidl)   {
                CopyToMyPooe(&ooeBuf, &(newPidl->ooe));
                _GenerateEvent(SHCNE_UPDATEITEM, (LPITEMIDLIST)newPidl, NULL);
                COfflineFolderEnum::FreePidl(newPidl);
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CTrayUI::OnBeginReport(INotification * pNot)
{
//xnotfmgr
    ASSERT(pNot);
    CLSID   cookie;

    if (SUCCEEDED(ReadGUID(pNot, NULL, c_szStartCookie, &cookie)))
    {
        DBGIID("TrayAgent::OnBeginReport - ", cookie);
        UpdateRequest(UM_BEGINREPORT, pNot);
        InterlockedIncrement(&m_cUpdates);
        SetTrayIcon(TRUE);
    }

    return S_OK;
}

