#include "private.h"
#include "offl_cpp.h"
#include "subsmgrp.h"
#include "helper.h"

#include <mluisupp.h>

#ifdef DEBUG
void    DumpTaskTrigger(TASK_TRIGGER * pTaskTrigger);
#endif  // DEBUG

// {D994B6F0-DA3C-11d1-857D-00C04FA35C89}
const GUID NOOP_SCHEDULE_COOKIE = 
{ 0xd994b6f0, 0xda3c, 0x11d1, { 0x85, 0x7d, 0x0, 0xc0, 0x4f, 0xa3, 0x5c, 0x89 
} };

#ifndef TASK_FLAG_RUN_ONLY_IF_LOGGED_ON
#define TASK_FLAG_RUN_ONLY_IF_LOGGED_ON        (0x2000)
#endif

const PROPSPEC c_rgPropRead[] = {
    { PRSPEC_PROPID, PID_INTSITE_SUBSCRIPTION},
    { PRSPEC_PROPID, PID_INTSITE_FLAGS},
    { PRSPEC_PROPID, PID_INTSITE_TRACKING},
    { PRSPEC_PROPID, PID_INTSITE_CODEPAGE},
};

void UpdateTimeFormat(LPTSTR tszTimeFormat, ULONG cchTimeFormat);

HRESULT WriteProperties(POOEntry pooe);
HRESULT ReadProperties(POOEBuf pBuf);

const TCHAR c_szLoadWC[] = TEXT("LoadWC");

void FixupRandomTrigger(TASK_TRIGGER *pTrigger)
{
    if (pTrigger->wRandomMinutesInterval > 0)
    {
        //  We have a random interval so we need to add a random number of minutes to it.
        //  Given the fact that all of the fields need to carry over to the next, the
        //  simplest way to do this is to flatten the start time into FILETIME, add the
        //  random minutes, and then convert back to a TASK_TRIGGER.  This let's us use
        //  Win32 APIs instead of doing all of the calendar and carry over stuff ourselves.

        SYSTEMTIME st;
        CFileTime ft;

        memset(&st, 0, sizeof(SYSTEMTIME));
        st.wYear = pTrigger->wBeginYear;
        st.wMonth = pTrigger->wBeginMonth;
        st.wDay = pTrigger->wBeginDay;
        st.wHour = pTrigger->wStartHour;
        st.wMinute = pTrigger->wStartMinute;

        SystemTimeToFileTime(&st, &ft);

        ft += ONE_MINUTE_IN_FILETIME * (__int64)Random(pTrigger->wRandomMinutesInterval);

        FileTimeToSystemTime(&ft, &st);

        pTrigger->wBeginYear = st.wYear;
        pTrigger->wBeginMonth = st.wMonth;
        pTrigger->wBeginDay = st.wDay;
        pTrigger->wStartHour = st.wHour;
        pTrigger->wStartMinute = st.wMinute;

        pTrigger->wRandomMinutesInterval = 0;
    }
}

//  Come up with a name like "MSN Recommended Schedule"
void CreatePublisherScheduleNameW(WCHAR *pwszSchedName, int cchSchedName, 
                                  const TCHAR *pszName, const WCHAR *pwszName)
{
    WCHAR wszFormat[MAX_PATH];
    WCHAR wszPubName[MAX_PATH];
    const WCHAR *pwszPubName;

    ASSERT((NULL != pszName) || (NULL != pwszName));
    ASSERT((NULL != pwszSchedName) && (cchSchedName > 0));

    if (NULL == pwszName)
    {
        ASSERT(NULL != pszName);
        MyStrToOleStrN(wszPubName, ARRAYSIZE(wszPubName), pszName);
        pwszPubName = wszPubName;
    }
    else
    {
        pwszPubName = pwszName;
    }

#ifdef UNICODE
    MLLoadStringW(IDS_RECOMMENDED_SCHEDULE_FORMAT, wszFormat, ARRAYSIZE(wszFormat));
#else
    CHAR szFormat[MAX_PATH];
    MLLoadStringA(IDS_RECOMMENDED_SCHEDULE_FORMAT, szFormat, ARRAYSIZE(szFormat));
    MultiByteToWideChar(CP_ACP, 0, szFormat, -1, wszFormat, ARRAYSIZE(wszFormat));
#endif

    wnsprintfW(pwszSchedName, cchSchedName, wszFormat, pwszPubName);
}

void CreatePublisherScheduleName(TCHAR *pszSchedName, int cchSchedName, 
                                 const TCHAR *pszName, const WCHAR *pwszName)
{
    WCHAR wszSchedName[MAX_PATH];

    CreatePublisherScheduleNameW(wszSchedName, ARRAYSIZE(wszSchedName),
                                 pszName, pwszName);

    MyOleStrToStrN(pszSchedName, cchSchedName, wszSchedName);
}

HICON LoadItemIcon(ISubscriptionItem *psi, BOOL bLarge)
{
    HICON hIcon = NULL;
    SUBSCRIPTIONITEMINFO sii;
    SUBSCRIPTIONCOOKIE cookie;
    HRESULT hr;

    sii.cbSize = sizeof(SUBSCRIPTIONITEMINFO);

    psi->GetCookie(&cookie);
    
    hr = psi->GetSubscriptionItemInfo(&sii);

    if (SUCCEEDED(hr))
    {
        ISubscriptionAgentShellExt *pSubscriptionAgentShellExt;
        
        hr = CoCreateInstance(sii.clsidAgent, NULL, CLSCTX_INPROC_SERVER, 
                              IID_ISubscriptionAgentShellExt, 
                              (void **)&pSubscriptionAgentShellExt);
        if (SUCCEEDED(hr))
        {
            hr = pSubscriptionAgentShellExt->Initialize(&cookie, L"", L"", (SUBSCRIPTIONTYPE)-1);

            if (SUCCEEDED(hr))
            {
                IExtractIcon *pExtractIcon;
                hr = pSubscriptionAgentShellExt->QueryInterface(IID_IExtractIcon, 
                                                                (void **)&pExtractIcon);

                if (SUCCEEDED(hr))
                {
                    TCHAR szIconFile[INTERNET_MAX_URL_LENGTH];
                    int iIndex;
                    UINT wFlags;
                    HICON hIconScrap = NULL;
                    HICON *phIconLarge = bLarge ? &hIcon : &hIconScrap;
                    HICON *phIconSmall = bLarge ? &hIconScrap : &hIcon;
                    
                    hr = pExtractIcon->GetIconLocation(0, szIconFile, ARRAYSIZE(szIconFile), &iIndex, &wFlags);

                    if (SUCCEEDED(hr))
                    {
                        hr = pExtractIcon->Extract(szIconFile, iIndex, phIconLarge, phIconSmall, 
                                                   MAKELONG(GetSystemMetrics(SM_CXICON), 
                                                            GetSystemMetrics(SM_CXSMICON)));

                        if (S_FALSE == hr)
                        {
                            hIcon = ExtractIcon(g_hInst, szIconFile, iIndex);

                            if (NULL == hIcon)
                            {
                                hr = E_FAIL;
                            }
                        }
                        else if ((NULL != hIconScrap) && (hIcon != hIconScrap))
                        {
                            DestroyIcon(hIconScrap);
                        }
                    }
                    pExtractIcon->Release();
                }
            }
            
            pSubscriptionAgentShellExt->Release();
        }
    }

    if (FAILED(hr))
    {
        DWORD dwChannel = 0;
        DWORD dwDesktop = 0;
        int iSize = bLarge ? GetSystemMetrics(SM_CXICON) : GetSystemMetrics(SM_CXSMICON);
        int id;
        HINSTANCE   hinstSrc;

        ReadDWORD(psi, c_szPropChannel, &dwChannel);
        ReadDWORD(psi, c_szPropDesktopComponent, &dwDesktop);

        if (dwDesktop == 1)
        {
            id = IDI_DESKTOPITEM;
            hinstSrc = MLGetHinst();
        }
        else if (dwChannel == 1)
        {
            id = IDI_CHANNEL;
            hinstSrc = g_hInst;
        }
        else
        {
            id = IDI_WEBDOC;
            hinstSrc = g_hInst;
        }

        hIcon = (HICON)LoadImage(hinstSrc, MAKEINTRESOURCE(id), IMAGE_ICON, 
                                 iSize, iSize, LR_DEFAULTCOLOR);
    
    }

    return hIcon;
}

BOOL ScheduleCookieExists(SYNCSCHEDULECOOKIE *pSchedCookie)
{
    HRESULT hr;
    ISyncScheduleMgr *pSyncScheduleMgr;

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_SyncMgr, NULL, CLSCTX_ALL, 
                              IID_ISyncScheduleMgr, (void **)&pSyncScheduleMgr);

        if (SUCCEEDED(hr))
        {
            ISyncSchedule *pSyncSchedule = NULL;

            hr = pSyncScheduleMgr->OpenSchedule(pSchedCookie, 0, &pSyncSchedule);
            if (SUCCEEDED(hr))
            {
                pSyncSchedule->Release();
            }
            pSyncScheduleMgr->Release();
        }
        CoUninitialize();
    }

    return hr == S_OK;
}

BOOL IsScheduleNameInUse(TCHAR *pszSchedName)
{
    HRESULT hr;
    ISyncScheduleMgr *pSyncScheduleMgr;

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_SyncMgr, NULL, CLSCTX_ALL, 
                              IID_ISyncScheduleMgr, (void **)&pSyncScheduleMgr);

        if (SUCCEEDED(hr))
        {
            CWaitCursor waitCursor;
            ISyncSchedule *pSyncSchedule = NULL;
            SYNCSCHEDULECOOKIE schedCookie = GUID_NULL;
            WCHAR wszSchedName[MAX_PATH];

            MyStrToOleStrN(wszSchedName, ARRAYSIZE(wszSchedName), pszSchedName);

            hr = pSyncScheduleMgr->CreateSchedule(wszSchedName, 0, 
                                                  &schedCookie, &pSyncSchedule);
            if (SUCCEEDED(hr))
            {
                pSyncSchedule->Release();
            }
            pSyncScheduleMgr->Release();
        }
        CoUninitialize();
    }

    return hr == SYNCMGR_E_NAME_IN_USE;
}

struct CONFLICT_DATA
{
    TCHAR szSchedName[MAX_PATH];
    TCHAR szFriendlyTrigger[MAX_PATH];
};

INT_PTR CALLBACK SchedConflictDlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            TCHAR szConflictFormat[MAX_PATH];
            TCHAR szConflictMsg[MAX_PATH * 2];
            CONFLICT_DATA *pcd = (CONFLICT_DATA *)lParam;

            ASSERT(NULL != pcd);

            MLLoadString(IDS_SCHED_CONFLICT_FORMAT, 
                       szConflictFormat, ARRAYSIZE(szConflictFormat));

            wnsprintf(szConflictMsg, ARRAYSIZE(szConflictMsg),
                      szConflictFormat, pcd->szSchedName);

            SetDlgItemText(hdlg, IDC_SCHEDULE_MESSAGE, szConflictMsg);
            SetDlgItemText(hdlg, IDC_FRIENDLY_SCHEDULE_TEXT, pcd->szFriendlyTrigger);
            
            bResult = TRUE;
            break;
        }

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                EndDialog(hdlg, LOWORD(wParam));
                bResult = TRUE;
            }
            break;
    }
    
    return bResult;
}

BOOL CompareTaskTrigger(TASK_TRIGGER *pTrigA, TASK_TRIGGER *pTrigB)
{
    BOOL bDontMatch;    //  TRUE if any elements don't match

    //  Simple memcmp won't work since the start date will be different
    //  when in fact they are effectively the same schedule - at least
    //  from a user perspective.

    //  BUGBUG - this is far from complete - we only check for values
    //  which can be set by our wizard.

    bDontMatch  = pTrigA->wStartHour != pTrigB->wStartHour;
    bDontMatch |= pTrigA->wStartMinute != pTrigB->wStartMinute;
    bDontMatch |= pTrigA->TriggerType != pTrigB->TriggerType;
    bDontMatch |= pTrigA->Type.Daily.DaysInterval != pTrigB->Type.Daily.DaysInterval;
    bDontMatch |= pTrigA->MinutesDuration != pTrigB->MinutesDuration;
    bDontMatch |= pTrigA->MinutesInterval != pTrigB->MinutesInterval;
    bDontMatch |= pTrigA->wRandomMinutesInterval != pTrigB->wRandomMinutesInterval;

    return !bDontMatch;
}


//  HandleScheduleNameConflict
//
//  Return values:
//  CONFLICT_NONE               - pSchedCookie will be GUID_NULL and the caller is 
//                                free to create a new schedule
//  CONFLICT_RESOLVED_USE_NEW   - pSchedCookie will be the cookie of an existing
//                                schedule which the caller should update with
//                                it's new TASK_TRIGGER
//  CONFLICT_RESOLVED_USE_OLD   - pSchedCookie will be the cookie of an existing
//                                schedule which the caller should use without
//                                modifying anything
//  CONFLICT_UNRESOLVED         - pSchedCookie will be GUID_NULL and the caller
//                                shouldn't do anything until the user has made
//                                up his/her mind
//
int HandleScheduleNameConflict(/* in  */ TCHAR *pszSchedName, 
                               /* in  */ TASK_TRIGGER *pTrigger,
                               /* in  */ HWND hwndParent,
                               /* out */ SYNCSCHEDULECOOKIE *pSchedCookie)
{
    HRESULT hr;
    ISyncScheduleMgr *pSyncScheduleMgr;
    int iResult = CONFLICT_NONE;

    *pSchedCookie = GUID_NULL;

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_SyncMgr, NULL, CLSCTX_ALL, 
                              IID_ISyncScheduleMgr, (void **)&pSyncScheduleMgr);

        if (SUCCEEDED(hr))
        {
            ISyncSchedule *pSyncSchedule = NULL;
            SYNCSCHEDULECOOKIE schedCookie = GUID_NULL;
            WCHAR wszSchedName[MAX_PATH];

            MyStrToOleStrN(wszSchedName, ARRAYSIZE(wszSchedName), pszSchedName);

            {
                CWaitCursor waitCursor;

                hr = pSyncScheduleMgr->CreateSchedule(wszSchedName, 0, 
                                                      &schedCookie, &pSyncSchedule);
            }
            if (SUCCEEDED(hr))
            {
                pSyncSchedule->Release();
            }
            else if (SYNCMGR_E_NAME_IN_USE == hr)
            {
                //  schedCookie will have the cookie of the conflicting schedule
                hr = pSyncScheduleMgr->OpenSchedule(&schedCookie, 0, &pSyncSchedule);

                if (SUCCEEDED(hr))
                {
                    ITaskTrigger *pITaskTrigger;
                    
                    hr = pSyncSchedule->GetTrigger(&pITaskTrigger);

                    if (SUCCEEDED(hr))
                    {
                        TASK_TRIGGER existTrigger = { sizeof(TASK_TRIGGER) };

                        hr = pITaskTrigger->GetTrigger(&existTrigger);

                        if (SUCCEEDED(hr))
                        {
                            if (!CompareTaskTrigger(&existTrigger, pTrigger))
                            {
                                CONFLICT_DATA cd;
                                LPWSTR pwszFriendlyTrigger;

                                StrCpyN(cd.szSchedName, pszSchedName, ARRAYSIZE(cd.szSchedName));
                                if (SUCCEEDED(pITaskTrigger->GetTriggerString(&pwszFriendlyTrigger)))
                                {
                                    MyOleStrToStrN(cd.szFriendlyTrigger,
                                                   ARRAYSIZE(cd.szFriendlyTrigger),
                                                   pwszFriendlyTrigger);
                                    CoTaskMemFree(pwszFriendlyTrigger);
                                }
                                else
                                {
                                    cd.szFriendlyTrigger[0] = TEXT('\0');
                                }

                                INT_PTR iRet = DialogBoxParam(MLGetHinst(),
                                                              MAKEINTRESOURCE(IDD_DUPLICATE_SCHEDULE),
                                                              hwndParent,
                                                              SchedConflictDlgProc,
                                                              (LPARAM)&cd);
                                switch (iRet)
                                {
                                    case IDC_NEW_SETTINGS:
                                        iResult = CONFLICT_RESOLVED_USE_NEW;
                                        *pSchedCookie = schedCookie;
                                        break;
                                        
                                    case IDC_OLD_SETTINGS:
                                        iResult = CONFLICT_RESOLVED_USE_OLD;
                                        *pSchedCookie = schedCookie;
                                        break;

                                    default:
                                        iResult = CONFLICT_UNRESOLVED;
                                        break;
                                }
                            }
                        }
                        pITaskTrigger->Release();
                    }
                    pSyncSchedule->Release();
                }
            }

            pSyncScheduleMgr->Release();
        }
        CoUninitialize();
    }

    return iResult;
}

HRESULT UpdateScheduleTrigger(SYNCSCHEDULECOOKIE *pSchedCookie, TASK_TRIGGER *pTrigger)
{
    HRESULT hr;
    ISyncScheduleMgr *pSyncScheduleMgr;

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        CWaitCursor waitCursor;
        hr = CoCreateInstance(CLSID_SyncMgr, NULL, CLSCTX_ALL, 
                              IID_ISyncScheduleMgr, (void **)&pSyncScheduleMgr);

        if (SUCCEEDED(hr))
        {
            ISyncSchedule *pSyncSchedule;

            hr = pSyncScheduleMgr->OpenSchedule(pSchedCookie, 0, &pSyncSchedule);
            if (SUCCEEDED(hr))
            {
                ITaskTrigger *pITaskTrigger;
                
                hr = pSyncSchedule->GetTrigger(&pITaskTrigger);
                if (SUCCEEDED(hr))
                {
                    FixupRandomTrigger(pTrigger);

                    hr = pITaskTrigger->SetTrigger(pTrigger);

                    if (SUCCEEDED(hr))
                    {
                        hr = pSyncSchedule->Save();
                    }

                    pITaskTrigger->Release();
                }
                pSyncSchedule->Release();
            }
            pSyncScheduleMgr->Release();
        }
        CoUninitialize();
    }

    return hr;
}

HRESULT RemoveItemFromAllSchedules(SUBSCRIPTIONCOOKIE *pCookie)
{
    HRESULT hr;
    ISyncScheduleMgr *pSyncScheduleMgr;

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        CWaitCursor waitCursor;
        hr = CoCreateInstance(CLSID_SyncMgr, NULL, CLSCTX_ALL, 
                              IID_ISyncScheduleMgr, (void **)&pSyncScheduleMgr);

        if (SUCCEEDED(hr))
        {
            IEnumSyncSchedules *pEnumSyncSchedules;
            
            hr = pSyncScheduleMgr->EnumSyncSchedules(&pEnumSyncSchedules);

            if (SUCCEEDED(hr))
            {
                SYNCSCHEDULECOOKIE schedCookie;
                ULONG ulFetched;

                while (S_OK == pEnumSyncSchedules->Next(1, &schedCookie, &ulFetched) &&
                       (0 != ulFetched))    //  BUGBUG - this shouldn't be necessary
                {
                    ISyncSchedule *pSyncSchedule;

                    //  If this fails, there ain't much we can do about
                    //  it so just plod along anyhow

                    if (SUCCEEDED(pSyncScheduleMgr->OpenSchedule(&schedCookie, 0, &pSyncSchedule)))
                    {
                        //  Don't care about the return value, it's cheaper
                        //  for us to just delete than to ask if it's there
                        //  and then delete.
                        pSyncSchedule->SetItemCheck(CLSID_WebCheckOfflineSync,
                                                    pCookie,
                                                    SYNCMGRITEMSTATE_UNCHECKED);
                        pSyncSchedule->Save();
                        pSyncSchedule->Release();
                    }
                }
                pEnumSyncSchedules->Release();
            }
            pSyncScheduleMgr->Release();
        }
        CoUninitialize();
    }

    return hr;
}

HRESULT AddRemoveScheduledItem(SYNC_HANDLER_ITEM_INFO *pSyncHandlerItemInfo, // For Add
                               SUBSCRIPTIONCOOKIE *pCookie,                  // For Remove
                               SYNCSCHEDULECOOKIE *pSchedCookie, BOOL bAdd)
{
    HRESULT hr;
    ISyncScheduleMgr *pSyncScheduleMgr;

    ASSERT((bAdd && (NULL != pSyncHandlerItemInfo)) || 
            (!bAdd && (NULL != pCookie)));

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_SyncMgr, NULL, CLSCTX_ALL, 
                              IID_ISyncScheduleMgr, (void **)&pSyncScheduleMgr);

        if (SUCCEEDED(hr))
        {
            ISyncSchedule *pSyncSchedule;

            hr = pSyncScheduleMgr->OpenSchedule(pSchedCookie, 0, &pSyncSchedule);
            if (SUCCEEDED(hr))
            {
                if (bAdd)
                {

                    hr = pSyncSchedule->AddItem(pSyncHandlerItemInfo);
                    hr = pSyncSchedule->SetItemCheck(CLSID_WebCheckOfflineSync,
                                                     &pSyncHandlerItemInfo->itemID,
                                                     SYNCMGRITEMSTATE_CHECKED);
                }
                else
                {

                    hr = pSyncSchedule->SetItemCheck(CLSID_WebCheckOfflineSync,
                                                     pCookie,
                                                     SYNCMGRITEMSTATE_UNCHECKED);
                }
                hr = pSyncSchedule->Save();
                pSyncSchedule->Release();
            }
            pSyncScheduleMgr->Release();
        }
        CoUninitialize();
    }

    return hr;
}

HRESULT SetScheduleValues(ISyncSchedule *pSyncSchedule, 
                          TASK_TRIGGER *pTrigger,
                          DWORD dwSyncScheduleFlags)
{
    HRESULT hr;
    ITaskTrigger *pITaskTrigger;

    ASSERT(NULL != pSyncSchedule);
    ASSERT(NULL != pTrigger);

    hr = pSyncSchedule->GetTrigger(&pITaskTrigger);

    ASSERT(SUCCEEDED(hr));

    if (SUCCEEDED(hr))
    {
        FixupRandomTrigger(pTrigger);
        hr = pITaskTrigger->SetTrigger(pTrigger);
        pITaskTrigger->Release();

        ASSERT(SUCCEEDED(hr));

        if (SUCCEEDED(hr))
        {
            DWORD dwFlags;
            DWORD dwConnectionType = SYNCSCHEDINFO_FLAGS_CONNECTION_LAN;
            WCHAR wszConnectionName[MAX_PATH];

            //  Return code doesn't help us.  This returns the best guess
            //  at connection:
            //      1) LAN
            //      2) currently connected connectoid
            //      3) auto-dial connectoid
            //  This is according to darrenmi, if this changes - kill him.
            InternetGetConnectedStateExW(&dwFlags, wszConnectionName,
                                         ARRAYSIZE(wszConnectionName), 0);

            if (dwFlags & INTERNET_CONNECTION_MODEM)
            {
                dwConnectionType = SYNCSCHEDINFO_FLAGS_CONNECTION_WAN;
            }

            hr = pSyncSchedule->SetConnection(
                (dwConnectionType == SYNCSCHEDINFO_FLAGS_CONNECTION_WAN) ?
                    wszConnectionName : NULL, 
                dwConnectionType);

            ASSERT(SUCCEEDED(hr));

            if (SUCCEEDED(hr))
            {
                hr = pSyncSchedule->SetFlags(dwSyncScheduleFlags);

                ASSERT(SUCCEEDED(hr));

                if (SUCCEEDED(hr))
                {
                    hr = pSyncSchedule->Save();

                    ASSERT(SUCCEEDED(hr));
                }
            }
        }
    }

    return hr;
}

HRESULT CreateSchedule(LPWSTR pwszScheduleName, DWORD dwSyncScheduleFlags, 
                       SYNCSCHEDULECOOKIE *pSchedCookie, TASK_TRIGGER *pTrigger,
                       BOOL fDupCookieOK)
{
    HRESULT hr;
    ISyncScheduleMgr *pSyncScheduleMgr;
    ISyncSchedule *pSyncSchedule = NULL;

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        CWaitCursor waitCursor;
        hr = CoCreateInstance(CLSID_SyncMgr, NULL, CLSCTX_ALL, 
                              IID_ISyncScheduleMgr, (void **)&pSyncScheduleMgr);

        if (SUCCEEDED(hr))
        {
            hr = pSyncScheduleMgr->CreateSchedule(pwszScheduleName, 0, 
                                      pSchedCookie, &pSyncSchedule);

            ASSERT((FAILED(hr) && (NULL == pSyncSchedule)) ||
                   (SUCCEEDED(hr) && (NULL != pSyncSchedule)));

            switch (hr)
            {
                case S_OK:
                    hr = SetScheduleValues(pSyncSchedule, pTrigger, dwSyncScheduleFlags);

                #ifdef DEBUG
                    if (FAILED(hr))
                    {
                        TraceMsg(TF_ALWAYS, "SetScheduleValues failed - hr=0x%08x", hr);
                    }
                #endif
                
                    break;

                case HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS):
                    if (fDupCookieOK)
                    {
                        hr = S_OK;
                    }
                    break;
            }

            SAFERELEASE(pSyncSchedule);

            pSyncScheduleMgr->Release();
        }
        CoUninitialize();
    }

    return hr;
}

BOOL IsCookieOnSchedule(ISyncSchedule *pSyncSchedule, SUBSCRIPTIONCOOKIE *pCookie)
{
    HRESULT hr;
    DWORD dwCheckState = SYNCMGRITEMSTATE_UNCHECKED;

    hr = pSyncSchedule->GetItemCheck(CLSID_WebCheckOfflineSync,
                                     pCookie,
                                     &dwCheckState);

    return SUCCEEDED(hr) && (SYNCMGRITEMSTATE_CHECKED & dwCheckState);
}

struct GIS_DATA
{
    SUBSCRIPTIONCOOKIE *pSubsCookie;
    SYNCSCHEDULECOOKIE *pSchedCookie;
};

BOOL GetItemScheduleCallback(ISyncSchedule *pSyncSchedule, 
                             SYNCSCHEDULECOOKIE *pSchedCookie, 
                             LPARAM lParam)
{
    BOOL bContinue = TRUE;
    GIS_DATA *pgd = (GIS_DATA *)lParam;
    
    if (IsCookieOnSchedule(pSyncSchedule, pgd->pSubsCookie))
    {
        *pgd->pSchedCookie = *pSchedCookie;
        bContinue = FALSE;
    }

    return bContinue;
}


HRESULT GetItemSchedule(SUBSCRIPTIONCOOKIE *pSubsCookie, SYNCSCHEDULECOOKIE *pSchedCookie)
{
    HRESULT hr;
    ISyncScheduleMgr *pSyncScheduleMgr;

    *pSchedCookie = GUID_NULL;
    
    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_SyncMgr, NULL, CLSCTX_ALL, 
                              IID_ISyncScheduleMgr, (void **)&pSyncScheduleMgr);
        if (SUCCEEDED(hr))
        {
            ISubscriptionItem *psi;

            //  First let's chech to see if it has a custom schedule
            hr = SubscriptionItemFromCookie(FALSE, pSubsCookie, &psi);

            if (SUCCEEDED(hr))
            {
                SUBSCRIPTIONITEMINFO sii;

                sii.cbSize = sizeof(SUBSCRIPTIONITEMINFO);
                
                hr = psi->GetSubscriptionItemInfo(&sii);

                psi->Release();

                if (SUCCEEDED(hr) && (sii.ScheduleGroup != GUID_NULL))
                {
                    *pSchedCookie = sii.ScheduleGroup;
                }
                else
                {
                    GIS_DATA gd;

                    gd.pSubsCookie = pSubsCookie;
                    gd.pSchedCookie = pSchedCookie;
                    EnumSchedules(GetItemScheduleCallback, (LPARAM)&gd);
                }
            }


            pSyncScheduleMgr->Release();
        }
        CoUninitialize();
    }

    return hr;
}

HRESULT EnumSchedules(SCHEDULEENUMCALLBACK pCallback, LPARAM lParam)
{
    HRESULT hr;
    ISyncScheduleMgr *pSyncScheduleMgr;

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        CWaitCursor waitCursor;
        hr = CoCreateInstance(CLSID_SyncMgr, NULL, CLSCTX_ALL, 
                              IID_ISyncScheduleMgr, (void **)&pSyncScheduleMgr);

        if (SUCCEEDED(hr))
        {
            IEnumSyncSchedules *pEnumSyncSchedules;
            
            hr = pSyncScheduleMgr->EnumSyncSchedules(&pEnumSyncSchedules);

            if (SUCCEEDED(hr))
            {
                SYNCSCHEDULECOOKIE schedCookie;
                ULONG ulFetched;

                while (S_OK == pEnumSyncSchedules->Next(1, &schedCookie, &ulFetched)&&
                       (0 != ulFetched))    //  BUGBUG - this shouldn't be necessary
                {
                    ISyncSchedule *pSyncSchedule;

                    HRESULT hrTemp = pSyncScheduleMgr->OpenSchedule(&schedCookie, 0, &pSyncSchedule);
                    if (SUCCEEDED(hrTemp) && pSyncSchedule)
                    {
                        BOOL bContinue = pCallback(pSyncSchedule, &schedCookie, lParam);
                        pSyncSchedule->Release();

                        if (!bContinue)
                        {
                            hr = S_FALSE;
                            break;
                        }
                    }
                }
                pEnumSyncSchedules->Release();
            }
            pSyncScheduleMgr->Release();
        }
        CoUninitialize();
    }

    return hr;
}

SUBSCRIPTIONTYPE GetItemCategory(BOOL bDesktop, const CLSID& clsidDest)
{
    SUBSCRIPTIONTYPE st;
    
    if (clsidDest == CLSID_WebCrawlerAgent)
    {
        st = (!bDesktop) ? SUBSTYPE_URL : SUBSTYPE_DESKTOPURL;
    }
    else if (clsidDest == CLSID_ChannelAgent)
    {
        st = (!bDesktop) ? SUBSTYPE_CHANNEL : SUBSTYPE_DESKTOPCHANNEL;
    }
    else
    {
        st = SUBSTYPE_EXTERNAL;
    }

    return st;
}


SUBSCRIPTIONSCHEDULE GetGroup(BOOL bDesktop, const CLSID& clsidDest, 
                              DWORD fChannelFlags, const NOTIFICATIONCOOKIE& groupCookie)
{


    SUBSCRIPTIONTYPE category = GetItemCategory(bDesktop, clsidDest);
    if (category == SUBSTYPE_CHANNEL || category == SUBSTYPE_DESKTOPCHANNEL) {
        if ((fChannelFlags & CHANNEL_AGENT_DYNAMIC_SCHEDULE) && 
            (GUID_NULL == groupCookie))
            return SUBSSCHED_AUTO;
    }

    //  We have no idea about the AUTO schedule stuff of unknown types.

    if (groupCookie == NOTFCOOKIE_SCHEDULE_GROUP_DAILY)
            return SUBSSCHED_DAILY;
    else if (groupCookie == NOTFCOOKIE_SCHEDULE_GROUP_WEEKLY)
        return SUBSSCHED_WEEKLY;
    else if (groupCookie == NOTFCOOKIE_SCHEDULE_GROUP_MANUAL)
        return SUBSSCHED_MANUAL;
    else
        return SUBSSCHED_CUSTOM;
}

HRESULT LoadGroupCookie(NOTIFICATIONCOOKIE * pCookie, SUBSCRIPTIONSCHEDULE subGroup)
{
    if (pCookie)    {
        switch (subGroup)   {
        case SUBSSCHED_DAILY:
            *pCookie = NOTFCOOKIE_SCHEDULE_GROUP_DAILY;
            break;
        case SUBSSCHED_WEEKLY:
            *pCookie = NOTFCOOKIE_SCHEDULE_GROUP_WEEKLY;
            break;
        case SUBSSCHED_MANUAL:
            *pCookie = NOTFCOOKIE_SCHEDULE_GROUP_MANUAL;
            break;
        default:
            *pCookie = CLSID_NULL;
            ASSERT(0);
            break;
        }
        return S_OK;
    }

    return E_INVALIDARG;
}


/////////////////////////////////////////////////////////////////////
//  Subscription helper functions.


HRESULT TSTR2BSTR(VARIANT * pvarBSTR, LPCTSTR srcTSTR)
{
    ASSERT(pvarBSTR);
    ASSERT(srcTSTR);

    BSTR    bstrBuf = NULL;
    LONG    lTSTRLen = 0;

    lTSTRLen = lstrlen(srcTSTR) + 1;
    bstrBuf = SysAllocStringLen(NULL, lTSTRLen);
    if (!bstrBuf)
        return E_OUTOFMEMORY;
    MyStrToOleStrN(bstrBuf, lTSTRLen, srcTSTR);
    pvarBSTR->vt = VT_BSTR;
    pvarBSTR->bstrVal = bstrBuf;
    return S_OK; 
}

HRESULT WriteCookieToInetDB(LPCTSTR pszURL, SUBSCRIPTIONCOOKIE *pCookie, BOOL bRemove)
{
    PROPVARIANT propCookie;
    LPOLESTR pclsid = NULL; // init to keep compiler happy

    ASSERT(pszURL);
    if (bRemove)
    {
        propCookie.vt = VT_EMPTY;
    } 
    else 
    {
        ASSERT(pCookie);

        if (FAILED(StringFromCLSID(*pCookie, &pclsid)))
            return E_FAIL;

        propCookie.vt = VT_LPWSTR;
        propCookie.pwszVal = pclsid;
    }

    HRESULT hr = IntSiteHelper(pszURL, &c_rgPropRead[PROP_SUBSCRIPTION], &propCookie, 1, TRUE);

    if (!bRemove)
        CoTaskMemFree(pclsid);

    return hr;
}

HRESULT WritePropertiesToItem(POOEntry pooe, ISubscriptionItem *psi)
{
    HRESULT hr = S_OK;
    VARIANT var;
    BOOL bHasUNAME = TRUE;

    ASSERT(NULL != psi);
    
    VariantInit(&var);
    if (pooe->dwFlags & PROP_WEBCRAWL_URL)
    {
        if (FAILED(TSTR2BSTR(&var, URL(pooe))))
            return E_FAIL;
        WriteVariant(psi, c_szPropURL, &var);
        VariantClear(&var);
    }

    if (pooe->dwFlags & PROP_WEBCRAWL_NAME)
    {
        if (FAILED(TSTR2BSTR(&var, NAME(pooe))))
            return E_FAIL;
        WriteVariant(psi, c_szPropName, &var);
        VariantClear(&var);
    }

    if (pooe->dwFlags & PROP_WEBCRAWL_UNAME)
    {
        if(*(UNAME(pooe)))
        {
            if (FAILED(TSTR2BSTR(&var, UNAME(pooe))))
                return E_FAIL;
            WriteVariant(psi, c_szPropCrawlUsername, &var);
            VariantClear(&var);
        }
        else
        {
            WriteEMPTY(psi, c_szPropCrawlUsername);
            bHasUNAME = FALSE;
        }
    }

    if (pooe->dwFlags & PROP_WEBCRAWL_DESKTOP)
    {
        if (pooe->bDesktop)
        {
            WriteDWORD(psi, c_szPropDesktopComponent, 1);
        }
        else
        {
            WriteEMPTY(psi, c_szPropDesktopComponent);
        }
    }

    if (pooe->dwFlags & PROP_WEBCRAWL_CHANNEL)
    {
        if (pooe->bChannel)
        {
            WriteDWORD(psi, c_szPropChannel, 1);
        }
        else
        {
            WriteEMPTY(psi, c_szPropChannel);
        }
    }

    if (pooe->dwFlags & PROP_WEBCRAWL_GLEAM)
    {
        if (pooe->bGleam)
        {
            WriteDWORD(psi, c_szPropEnableShortcutGleam, 1);
        }
        else
        {
            WriteEMPTY(psi, c_szPropEnableShortcutGleam);
        }
    }

    if (pooe->dwFlags & PROP_WEBCRAWL_CHANGESONLY)
    {
        if (pooe->bChangesOnly)
        {
            WriteDWORD(psi, c_szPropCrawlChangesOnly, 1);
        }
        else
        {
            WriteEMPTY(psi, c_szPropCrawlChangesOnly);
        }
    }

    if (pooe->dwFlags & PROP_WEBCRAWL_CHANNELFLAGS)
    {
        if (pooe->fChannelFlags)
        {
            WriteDWORD(psi, c_szPropChannelFlags, pooe->fChannelFlags);
        }
        else
        {
            WriteEMPTY(psi, c_szPropChannelFlags);
        }
    }

    if (pooe->dwFlags & PROP_WEBCRAWL_EMAILNOTF)
    {
        if (pooe->bMail)
        {
            WriteDWORD(psi, c_szPropEmailNotf, 1);
        }
        else
        {
            WriteEMPTY(psi, c_szPropEmailNotf);
        }
    }

    if (pooe->dwFlags & PROP_WEBCRAWL_PSWD)
    {
        if (*(PASSWD(pooe)) && bHasUNAME)
        {
            if (FAILED(TSTR2BSTR(&var, PASSWD(pooe))))
                return E_FAIL;
            WritePassword(psi, var.bstrVal);
            VariantClear(&var);
        }
        else
        {
            WritePassword(psi, NULL);
        }
    }

    if (pooe->dwFlags & PROP_WEBCRAWL_LEVEL)
    {
        if(pooe->m_RecurseLevels)
        {
            WriteDWORD(psi, c_szPropCrawlLevels, pooe->m_RecurseLevels);
        }
        else
        {
            // top page only was specified, empty out levels
            WriteEMPTY(psi, c_szPropCrawlLevels);
        }
    }

    if (pooe->dwFlags & PROP_WEBCRAWL_FLAGS)
    {
        WriteDWORD(psi, c_szPropCrawlFlags, pooe->m_RecurseFlags);
    }

    if (pooe->dwFlags & PROP_WEBCRAWL_ACTUALSIZE)
    {
        WriteDWORD(psi, c_szPropCrawlActualSize, pooe->m_ActualSize);
    }

    if (pooe->dwFlags & PROP_WEBCRAWL_SIZE)
    {
        if(pooe->m_SizeLimit)
        {
            // limit was specified
            WriteDWORD(psi, c_szPropCrawlMaxSize, pooe->m_SizeLimit);
        }
        else
        {
            // no limit was specified, empty out limit prop
            WriteEMPTY(psi, c_szPropCrawlMaxSize);
        }
    }

    SUBSCRIPTIONITEMINFO sii;
    sii.cbSize = sizeof(SUBSCRIPTIONITEMINFO);
    if (SUCCEEDED(psi->GetSubscriptionItemInfo(&sii)))
    {
        sii.dwFlags = pooe->grfTaskTrigger;
        psi->SetSubscriptionItemInfo(&sii);
    }

    //  We don't write Status/Last update.

    // BUGBUG: We should fail the subscription if we were unable to write
    // any of the properties into the notification for some reason.


    return hr;
}

HRESULT WriteProperties(POOEntry pooe)
{
    HRESULT hr;
    ISubscriptionItem *psi = NULL;

    ASSERT(NULL != pooe);

    hr = SubscriptionItemFromCookie(FALSE, &pooe->m_Cookie, &psi);

    if (SUCCEEDED(hr))
    {
        hr = WritePropertiesToItem(pooe, psi);
        psi->Release();
    }
    return hr;
}

HRESULT CreatePublisherSchedule()
{
    return S_OK;
}

#define RANDOM_TIME_START       0       // 12am (in minutes)
#define RANDOM_TIME_END         300     // 5am (in minutes)
#define RANDOM_TIME_INC         30      // 30min increment

DWORD GetRandomTime(DWORD StartMins, DWORD EndMins, DWORD Inc)
{
    DWORD Range;
    DWORD nIncrements;

    if (StartMins > EndMins)
    {
        Range = ((1440 - StartMins) + EndMins);
    }
    else
    {
        Range = (EndMins - StartMins);
    }

    nIncrements = ((Range / Inc) + 1);

    return (StartMins + (Random(nIncrements) * Inc));
}

HRESULT CreateDefaultSchedule(SUBSCRIPTIONSCHEDULE subsSchedule,
                              SYNCSCHEDULECOOKIE *pSchedCookie)
{
    HRESULT hr = S_OK;
    TASK_TRIGGER trig;
    int resID;

    *pSchedCookie = GUID_NULL;
    
    switch (subsSchedule)
    {
        case SUBSSCHED_DAILY:
            trig.TriggerType = TASK_TIME_TRIGGER_DAILY;
            trig.Type.Daily.DaysInterval = 1;
            resID = IDS_DAILY_GRO;
            *pSchedCookie = NOTFCOOKIE_SCHEDULE_GROUP_DAILY;
            break;

        case SUBSSCHED_WEEKLY:
            trig.TriggerType = TASK_TIME_TRIGGER_WEEKLY;
            trig.Type.Weekly.WeeksInterval = 1;
            trig.Type.Weekly.rgfDaysOfTheWeek = TASK_MONDAY;
            resID = IDS_WEEKLY_GRO;
            *pSchedCookie = NOTFCOOKIE_SCHEDULE_GROUP_WEEKLY;
            break;

        case SUBSSCHED_AUTO:
        case SUBSSCHED_CUSTOM:
        case SUBSSCHED_MANUAL:
        default:
            resID = 0;
            hr = E_FAIL;
            break;
    }

    if (SUCCEEDED(hr))
    {
        if (!ScheduleCookieExists(pSchedCookie))
        {
            WCHAR wszSchedName[MAX_PATH];
            DWORD dwRandTime = GetRandomTime(RANDOM_TIME_START, 
                                             RANDOM_TIME_END, 
                                             RANDOM_TIME_INC);

            trig.cbTriggerSize = sizeof(TASK_TRIGGER);
            trig.wRandomMinutesInterval = RANDOM_TIME_INC;
            trig.wStartHour = (UINT)(dwRandTime / 60);
            trig.wStartMinute = (UINT)(dwRandTime % 60);
            trig.rgFlags = 0;

            MLLoadStringW(resID, wszSchedName, ARRAYSIZE(wszSchedName));

            hr = CreateSchedule(wszSchedName, 0, pSchedCookie, &trig, TRUE);

            if (hr == SYNCMGR_E_NAME_IN_USE)
            {
                hr = S_OK;
            }
        }
    }

    return hr;
}

HRESULT AddIt(ISubscriptionItem *psi, POOEntry pooe, SUBSCRIPTIONSCHEDULE subGroup)
{
    SYNCSCHEDULECOOKIE schedCookie = GUID_NULL;
    HRESULT hr = E_FAIL;

    switch (subGroup)
    {
        case SUBSSCHED_DAILY:
        case SUBSSCHED_WEEKLY:
            hr = CreateDefaultSchedule(subGroup, &schedCookie);
            break;
            
        case SUBSSCHED_CUSTOM:
            schedCookie = pooe->groupCookie;
            hr = S_OK;
            break;
        
        case SUBSSCHED_MANUAL:
            RemoveItemFromAllSchedules(&pooe->m_Cookie);
            hr = S_FALSE;
            break;

        case SUBSSCHED_AUTO:
            //  BUGBUG - for now, until pub schedules are wired in
            hr = CreateDefaultSchedule(SUBSSCHED_DAILY, &schedCookie);
            break;
    }

    if (hr == S_OK)
    {
        ASSERT(GUID_NULL != schedCookie);

        if (NOOP_SCHEDULE_COOKIE == schedCookie)
        {
            hr = S_FALSE;
        }
        if (GUID_NULL != schedCookie)
        {
            SYNC_HANDLER_ITEM_INFO shii;

            shii.handlerID = CLSID_WebCheckOfflineSync;
            shii.itemID = pooe->m_Cookie;
            shii.hIcon = NULL;
            MyStrToOleStrN(shii.wszItemName, ARRAYSIZE(shii.wszItemName), NAME(pooe));
            shii.dwCheckState = SYNCMGRITEMSTATE_CHECKED;

            hr = AddScheduledItem(&shii, &schedCookie);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return S_OK;
//  return hr;
}

HRESULT ScheduleIt(ISubscriptionItem *psi, TCHAR *pszName, TASK_TRIGGER *pTrigger)
{
    HRESULT hr;
    SUBSCRIPTIONITEMINFO subscriptionItemInfo;

    ASSERT(pTrigger->cbTriggerSize == sizeof(TASK_TRIGGER));

    subscriptionItemInfo.cbSize = sizeof(SUBSCRIPTIONITEMINFO);

#ifdef DEBUG
    DumpTaskTrigger(pTrigger);
#endif

    hr = psi->GetSubscriptionItemInfo(&subscriptionItemInfo);

    if (SUCCEEDED(hr))
    {
        if (GUID_NULL != subscriptionItemInfo.ScheduleGroup)
        {
            hr = UpdateScheduleTrigger(&subscriptionItemInfo.ScheduleGroup, pTrigger);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    if (FAILED(hr))
    {
        WCHAR wszSchedName[MAX_PATH];

        CreatePublisherScheduleNameW(wszSchedName, ARRAYSIZE(wszSchedName),
                                     pszName, NULL);

        hr = CreateSchedule(wszSchedName, SYNCSCHEDINFO_FLAGS_READONLY,
                            &subscriptionItemInfo.ScheduleGroup, pTrigger, TRUE);

        if (SUCCEEDED(hr) || (hr == SYNCMGR_E_NAME_IN_USE))
        {
            psi->SetSubscriptionItemInfo(&subscriptionItemInfo);
            hr = S_OK;
        }
        else
        {
            TraceMsg(TF_ALWAYS, "Error creating schedule - hr=0x%08x", hr);
        }
    }

    if (SUCCEEDED(hr))
    {
        SYNC_HANDLER_ITEM_INFO shii;

        shii.handlerID = CLSID_WebCheckOfflineSync;
        psi->GetCookie(&shii.itemID);
        shii.hIcon = NULL;
        MyStrToOleStrN(shii.wszItemName, ARRAYSIZE(shii.wszItemName), pszName);
        
        hr = AddScheduledItem(&shii, &subscriptionItemInfo.ScheduleGroup); 
    }

    return S_OK;
//    return hr;
}

HRESULT ReadProperties(POOEBuf pBuf)
{
    VARIANT var;
    HRESULT hr;
    ASSERT(pBuf);
    BOOL    bHasUNAME = TRUE;
    ISubscriptionItem *psi = NULL;

    ASSERT(NULL != pBuf);

    hr = SubscriptionItemFromCookie(FALSE, &pBuf->m_Cookie, &psi);

    if (SUCCEEDED(hr))
    {
        VariantInit(&var);
        if (pBuf->dwFlags & PROP_WEBCRAWL_URL)
        {
            hr = ReadVariant(psi, c_szPropURL, &var);
            if (SUCCEEDED(hr) && (var.vt == VT_BSTR))
            {
                MyOleStrToStrN(pBuf->m_URL, MAX_URL, var.bstrVal);
            }
            else
            {
                pBuf->m_URL[0] = (TCHAR)0;
            }
            VariantClear(&var);
        }

        if (pBuf->dwFlags & PROP_WEBCRAWL_NAME) 
        {
            hr = ReadVariant(psi, c_szPropName, &var);
            if (SUCCEEDED(hr) && (var.vt == VT_BSTR)) 
            {
                MyOleStrToStrN(pBuf->m_Name, MAX_NAME, var.bstrVal);
            }
            else
            {
                pBuf->m_Name[0] = (TCHAR)0;
            }
            VariantClear(&var);
        }

        if (pBuf->dwFlags & PROP_WEBCRAWL_UNAME)
        {
            hr = ReadVariant(psi, c_szPropCrawlUsername, &var);
            if (SUCCEEDED(hr) && (var.vt == VT_BSTR))
            {
                MyOleStrToStrN(pBuf->username, MAX_USERNAME, var.bstrVal);
            }
            else
            {
                pBuf->username[0] = (TCHAR)0;
                bHasUNAME = FALSE;
            }
            VariantClear(&var);
        }

        if (pBuf->dwFlags & PROP_WEBCRAWL_DESKTOP)
        {
            hr = ReadVariant(psi, c_szPropDesktopComponent, &var);
            if (SUCCEEDED(hr) && (var.vt == VT_I4) && (var.lVal == 1))
            {
                pBuf->bDesktop = TRUE;
            }
            else
            {
                pBuf->bDesktop = FALSE;
            }
            VariantClear(&var);
        }

        if (pBuf->dwFlags & PROP_WEBCRAWL_CHANNEL)
        {
            hr = ReadVariant(psi, c_szPropChannel, &var);
            if (SUCCEEDED(hr) && (var.vt == VT_I4) && (var.lVal == 1))
            {
                pBuf->bChannel = TRUE;
            }
            else
            {
                pBuf->bChannel = FALSE;
            }
            VariantClear(&var);
        }

        if (pBuf->dwFlags & PROP_WEBCRAWL_GLEAM)
        {
            hr = ReadVariant(psi, c_szPropEnableShortcutGleam, &var);
            if (SUCCEEDED(hr) && (var.vt == VT_I4) && (var.lVal == 1))
            {
                pBuf->bGleam = TRUE;
            }
            else
            {
                pBuf->bGleam = FALSE;
            }
            VariantClear(&var);
        }

        if (pBuf->dwFlags & PROP_WEBCRAWL_CHANGESONLY)
        {
            hr = ReadVariant(psi, c_szPropCrawlChangesOnly, &var);
            if (SUCCEEDED(hr) && (var.vt == VT_I4) && (var.lVal == 1))
            {
                pBuf->bChangesOnly = TRUE;
            }
            else 
            {
                pBuf->bChangesOnly = FALSE;
            }
            VariantClear(&var);
        }

        if (pBuf->dwFlags & PROP_WEBCRAWL_CHANNELFLAGS)
        {
            hr = ReadVariant(psi, c_szPropChannelFlags, &var);
            if (SUCCEEDED(hr) && (var.vt == VT_I4))
            {
                pBuf->fChannelFlags = var.lVal;
            }
            else 
            {
                pBuf->fChannelFlags = 0;
            }
            VariantClear(&var);
        }

        if (pBuf->dwFlags & PROP_WEBCRAWL_EMAILNOTF)
        {
            hr = ReadVariant(psi, c_szPropEmailNotf, &var);
            if (SUCCEEDED(hr) && (var.vt == VT_I4) && (var.lVal == 1))
            {
                pBuf->bMail = TRUE;
            }
            else
            {
                pBuf->bMail = FALSE;
            }
            VariantClear(&var);
        }

        if ((pBuf->dwFlags & PROP_WEBCRAWL_PSWD) && bHasUNAME)
        {
            BSTR bstrVal = NULL;
            hr = ReadPassword(psi, &bstrVal);
            if (SUCCEEDED(hr) && bstrVal)
            {
                MyOleStrToStrN(pBuf->password, MAX_PASSWORD, bstrVal);
            }
            else
            {
                pBuf->password[0] = (TCHAR)0;
            }
            SAFEFREEBSTR(bstrVal);
        }

        if ((pBuf->dwFlags & PROP_WEBCRAWL_PSWD) || (pBuf->dwFlags & PROP_WEBCRAWL_UNAME)) {
            //bNeedPassword isn't stored in the property map... calculate it from the presence
            //of username/password.
            pBuf->bNeedPassword = pBuf->password[0] || pBuf->username[0];
        }

        if (pBuf->dwFlags & PROP_WEBCRAWL_STATUS)
        {
            hr = ReadVariant(psi, c_szPropStatusString, &var);
            if (SUCCEEDED(hr) && (var.vt == VT_BSTR))
            {
                MyOleStrToStrN(pBuf->statusStr, MAX_STATUS, var.bstrVal);
            }
            else
            {
                pBuf->statusStr[0] = (TCHAR)0;
            }
            VariantClear(&var);

            hr = ReadSCODE(psi, c_szPropStatusCode, &(pBuf->status));
            //  BUGBUG! What should we put here if we don't have last status?
            if (FAILED(hr))
            {
                pBuf->status = S_OK;
            }
            VariantClear(&var);
        }

        //
        // Use the CompletionTime property if present and it is greater than
        // value in the NOTIFICATIONITEM structure.
        //

        if (pBuf->dwFlags & PROP_WEBCRAWL_LAST) 
        {
            CFileTime ft;
            hr = ReadVariant(psi, c_szPropCompletionTime, &var);
            if (SUCCEEDED(hr) && (var.vt == VT_DATE))
            {
                VariantTimeToFileTime(var.date, ft);

                if (ft > pBuf->m_LastUpdated)
                {
                    pBuf->m_LastUpdated = ft;
                }
            }
        }

        if (pBuf->dwFlags & PROP_WEBCRAWL_LEVEL)
        {
            hr = ReadDWORD(psi, c_szPropCrawlLevels, (DWORD *)&(pBuf->m_RecurseLevels));
            if (FAILED(hr))
            {
                pBuf->m_RecurseLevels = 0;
            }
        }

        if (pBuf->dwFlags & PROP_WEBCRAWL_FLAGS)
        {
            hr = ReadDWORD(psi, c_szPropCrawlFlags, (DWORD *)&(pBuf->m_RecurseFlags));
            if (FAILED(hr))
            {
                pBuf->m_RecurseFlags = 0;   //  Minimal memory usage.
            }
        }

        if (pBuf->dwFlags & PROP_WEBCRAWL_SIZE)
        {
            hr = ReadDWORD(psi, c_szPropCrawlMaxSize, (DWORD *)&(pBuf->m_SizeLimit));
            if (FAILED(hr))
            {
                pBuf->m_SizeLimit = 0;
            }
        }

        if (pBuf->dwFlags & PROP_WEBCRAWL_ACTUALSIZE)
        {
            hr = ReadDWORD(psi, c_szPropCrawlActualSize, (DWORD *)&(pBuf->m_ActualSize));
            if (FAILED(hr))
            {
                pBuf->m_ActualSize = 0;
            }
        }

        SUBSCRIPTIONITEMINFO sii;
        sii.cbSize = sizeof(SUBSCRIPTIONITEMINFO);
        if (SUCCEEDED(psi->GetSubscriptionItemInfo(&sii)))
        {
            pBuf->grfTaskTrigger = sii.dwFlags;
        }

        psi->Release();
    }

    // BUGBUG: Need to support c_szPropEnableShortcutGleam here.
    // BUGBUG: Need to support c_szPropEnableShortcutGleam here.
    
    return S_OK;
}

// If url is NULL, no need to compare.
HRESULT LoadWithCookie(LPCTSTR pszURL, POOEBuf pBuf, DWORD *pdwBufferSize, SUBSCRIPTIONCOOKIE *pCookie)
{
    HRESULT hr = LoadOOEntryInfo(pBuf, pCookie, pdwBufferSize);

    if (SUCCEEDED(hr) && pszURL)
    {
        if (UrlCompare(pBuf->m_URL, pszURL, TRUE))
        {
            TraceMsg(TF_ALWAYS, "Mismatched cookie/URL in LoadWithCookie");
            hr = E_FAIL;      //  Mismatched cookie!
        }
    }

    return hr;
}

HRESULT ReadCookieFromInetDB(LPCTSTR pszURL, SUBSCRIPTIONCOOKIE *pCookie)
{
    ASSERT(pszURL && pCookie);
    PROPVARIANT propCookie;

    PropVariantInit(&propCookie);

    HRESULT hr = FindURLProps(pszURL, &propCookie);

    if (SUCCEEDED(hr) && (propCookie.vt == VT_LPWSTR))
    {
        hr = CLSIDFromString(propCookie.pwszVal, pCookie);
    }
    PropVariantClear(&propCookie);

    //  If we couldn't find it, use a brute force approach
    if (S_OK != hr)
    {
        CEnumSubscription *pes = new CEnumSubscription;

        if (NULL != pes)
        {
            if (SUCCEEDED(pes->Initialize(0)))
            {
                SUBSCRIPTIONCOOKIE cookie;
                BOOL bFound = FALSE;
                
                while (!bFound && (S_OK == pes->Next(1, &cookie, NULL)))
                {
                    ISubscriptionItem *psi;
                    
                    if (SUCCEEDED(SubscriptionItemFromCookie(FALSE, &cookie, &psi)))
                    {
                        LPTSTR pszCurURL;

                        if (SUCCEEDED(ReadTSTR(psi, c_szPropURL, &pszCurURL)))
                        {
                            bFound = (StrCmpI(pszCurURL, pszURL) == 0);
                            CoTaskMemFree(pszCurURL);
                        }
                        psi->Release();
                    }
                }

                if (bFound)
                {
                    WriteCookieToInetDB(pszURL, &cookie, FALSE);
                    *pCookie = cookie;
                    hr = S_OK;
                }
            }
            pes->Release();
        }
    }

    return hr;
}

HRESULT LoadSubscription(LPCTSTR url, LPMYPIDL *ppidl)
{
    HRESULT hr;

    POOEntry pooe = NULL;
    OOEBuf  ooeBuf;
    DWORD   dwBufferSize;
    SUBSCRIPTIONCOOKIE cookie;

    hr = ReadCookieFromInetDB(url, &cookie);
    if (S_OK == hr)
    {
        hr = LoadWithCookie(url, &ooeBuf, &dwBufferSize, &cookie);
        if (hr == S_OK)
        {
            *ppidl = COfflineFolderEnum::NewPidl(dwBufferSize);
            if (!(*ppidl))
            {
                return E_OUTOFMEMORY;
            }
            pooe = &((*ppidl)->ooe);
            CopyToMyPooe(&ooeBuf, pooe);
        }
        else 
        {
            WriteCookieToInetDB(url, NULL, TRUE);
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}
 
// KENSY: This needs to work like GetDefaultInfo

HRESULT GetDefaultOOEBuf(OOEBuf * pBuf, SUBSCRIPTIONTYPE subType)
{
    ASSERT(pBuf);
    ASSERT(IS_VALID_SUBSCRIPTIONTYPE(subType));

    memset((void *)pBuf, 0, sizeof(OOEBuf));
    pBuf->dwFlags = PROP_WEBCRAWL_ALL;
    pBuf->m_RecurseLevels = DEFAULTLEVEL;
    pBuf->m_RecurseFlags = DEFAULTFLAGS;
    pBuf->m_Priority = AGENT_PRIORITY_NORMAL;
    if (subType == SUBSTYPE_CHANNEL || subType == SUBSTYPE_DESKTOPCHANNEL)    
    {
        pBuf->clsidDest = CLSID_ChannelAgent;
        pBuf->fChannelFlags = CHANNEL_AGENT_PRECACHE_ALL | CHANNEL_AGENT_DYNAMIC_SCHEDULE;
    } 
    else
    {
        pBuf->clsidDest = CLSID_WebCrawlerAgent;
    }
    pBuf->bDesktop = (subType == SUBSTYPE_DESKTOPCHANNEL || subType == SUBSTYPE_DESKTOPURL);
    pBuf->bChannel = (subType == SUBSTYPE_CHANNEL || subType == SUBSTYPE_DESKTOPCHANNEL);
    pBuf->bGleam = !(pBuf->bDesktop);
    pBuf->m_LastUpdated = 0;
    pBuf->m_NextUpdate = 0;

    //  BUGBUG: Is this what we want?  IE 4 was DAILY.
    //  Default to not changing the schedule settings -- if it's already subscribed
    //  we won't blast anything and if it's not already subscribed then it will
    //  just be manual.
    pBuf->groupCookie = NOOP_SCHEDULE_COOKIE;       

    pBuf->grfTaskTrigger = TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET |     // Default to not autodial
                           TASK_FLAG_START_ONLY_IF_IDLE;               // and to idle time

    return S_OK;
}

HRESULT CreateSubscriptionFromOOEBuf(OOEBuf *pBuf, LPMYPIDL *ppidl)
{
    HRESULT hr;
    DWORD dwBufferSize = BufferSize(pBuf);

    *ppidl = COfflineFolderEnum::NewPidl(dwBufferSize);
    if (!(*ppidl))
    {
        return E_OUTOFMEMORY;
    }
    POOEntry pooe = &((*ppidl)->ooe);
    CopyToMyPooe(pBuf, pooe);

    //  See if the caller has already given us a cookie
    if (GUID_NULL == pooe->m_Cookie)
    {
        //  Nope, see if we have one already
        ReadCookieFromInetDB(URL(pooe), &(pooe->m_Cookie));

        if (GUID_NULL == pooe->m_Cookie)
        {
            //  Nope, so create one
            CreateCookie(&pooe->m_Cookie);
        }
    }

    WriteCookieToInetDB(URL(pooe), &(pooe->m_Cookie), FALSE);

    WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
    SUBSCRIPTIONITEMINFO sii;
    sii.cbSize = sizeof(SUBSCRIPTIONITEMINFO);
    sii.dwFlags = 0;
    sii.dwPriority = 0;
    sii.ScheduleGroup = CLSID_NULL;
    sii.clsidAgent = pooe->clsidDest;
    MyStrToOleStrN(wszURL, ARRAYSIZE(wszURL), URL(pooe));
    hr = AddUpdateSubscription(&(pooe->m_Cookie), &sii, wszURL, 0, NULL, NULL);

    if (SUCCEEDED(hr))
    {
        hr = WriteProperties(pooe);

        if (SUCCEEDED(hr))
        {
            ISubscriptionItem *psi;

            hr = SubscriptionItemFromCookie(TRUE, &pooe->m_Cookie, &psi);

            if (SUCCEEDED(hr))
            {
                SUBSCRIPTIONSCHEDULE subGroup = GetGroup(pooe);
                SUBSCRIPTIONTYPE   subType = GetItemCategory(pooe);

                if (subGroup == SUBSSCHED_AUTO)
                {
                    if (subType != SUBSTYPE_CHANNEL && subType != SUBSTYPE_DESKTOPCHANNEL)
                    {
                        hr = AddIt(psi, pooe, SUBSSCHED_DAILY);
                    }
                    else
                    {
                        if (pooe->m_Trigger.cbTriggerSize == sizeof(TASK_TRIGGER))
                        {
                            hr = ScheduleIt(psi, NAME(pooe), &pooe->m_Trigger);
                            pooe->groupCookie = CLSID_NULL;
                        }
                        else
                        {
                            hr = AddIt(psi, pooe, SUBSSCHED_DAILY);
                        }
                    }
                }
                else 
                {
                    hr = AddIt(psi, pooe, subGroup);
                }

                psi->Release();
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        FireSubscriptionEvent(SUBSNOTF_CREATE, &pooe->m_Cookie);
    }
    else 
    {
        TraceMsg(TF_ALWAYS, "Failed to add new subscription");
        TraceMsg(TF_ALWAYS, "\thr = 0x%x", hr);
        COfflineFolderEnum::FreePidl(*ppidl);
        *ppidl = NULL;
    }

    return hr;
}

HRESULT SendUpdateRequests(HWND hwnd, CLSID * arrClsid, UINT count)
{
    ISubscriptionMgr2 *pSubsMgr2;
    HRESULT hr;

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, 
                              IID_ISubscriptionMgr2, (void**)&pSubsMgr2);
        if (SUCCEEDED(hr))
        {
            SUBSCRIPTIONCOOKIE *pCookies = NULL;
            ULONG nItemsToRun = count;

            if (NULL == arrClsid)
            {
                IEnumSubscription *pes;

                hr = pSubsMgr2->EnumSubscriptions(0, &pes);

                if (SUCCEEDED(hr))
                {
                    ASSERT(NULL != pes);

                    pes->GetCount(&nItemsToRun);
                    if (nItemsToRun > 0)
                    {
                        pCookies = new SUBSCRIPTIONCOOKIE[nItemsToRun];

                        if (NULL != pCookies)
                        {
                            hr = pes->Next(nItemsToRun, pCookies, &nItemsToRun);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    pes->Release();
                }
            }
            else
            {
                pCookies = arrClsid;
            }

            if (SUCCEEDED(hr))
            {
                hr = pSubsMgr2->UpdateItems(0, nItemsToRun, pCookies);
            }

            if ((NULL == arrClsid) && (NULL != pCookies))
            {
                delete [] pCookies;
            }

            pSubsMgr2->Release();
        }
        CoUninitialize();
    }

    return hr;
}

HRESULT DoDeleteSubscription(POOEntry pooe)
{
    HRESULT hr;
    ISubscriptionItem *psi;

    ASSERT(NULL != pooe);

    hr = SubscriptionItemFromCookie(FALSE, &pooe->m_Cookie, &psi);

    if (SUCCEEDED(hr))
    {
        WritePassword(psi, NULL);

        hr = DoDeleteSubscriptionItem(&pooe->m_Cookie, TRUE);

        if (SUCCEEDED(hr) && (GetItemCategory(pooe) != SUBSTYPE_EXTERNAL))
        {
            WriteCookieToInetDB(URL(pooe), NULL, TRUE);
        }
    }

    return hr;
}

HRESULT PersistUpdate(POOEntry pooe, BOOL bCreate)
{
    HRESULT hr;
    ISubscriptionItem *psi;
    
    hr = SubscriptionItemFromCookie(bCreate, &(pooe->m_Cookie), &psi);

    if (SUCCEEDED(hr))
    {
        SUBSCRIPTIONITEMINFO sii = { sizeof(SUBSCRIPTIONITEMINFO) };

        hr = psi->GetSubscriptionItemInfo(&sii);

        if (SUCCEEDED(hr) || bCreate)
        {
            sii.clsidAgent = pooe->clsidDest;
            hr = psi->SetSubscriptionItemInfo(&sii);

            if (SUCCEEDED(hr))
            {
                hr = WritePropertiesToItem(pooe, psi);
                
                if (SUCCEEDED(hr) && IsNativeAgent(pooe->clsidDest))
                {
                    WriteCookieToInetDB(URL(pooe), &(pooe->m_Cookie), FALSE);
                }
            }
        }

        // REVIEW: should we delete on failure here?
        psi->Release();
    }

    return hr;
}

#ifdef NEWSCHED_AUTONAME
void NewSched_AutoNameHelper(HWND hDlg)
{
    TCHAR szDays[16];
    TCHAR szTime[128];
    TCHAR szFormat[MAX_PATH];
    TCHAR szSchedName[MAX_PATH];
    LPTSTR lpArguments[2];
    BOOL bTranslate;
    int nDays = GetDlgItemInt(hDlg, IDC_SCHEDULE_DAYS, &bTranslate, FALSE);

    if (MLLoadString((nDays == 1) ? IDS_SCHED_FORMAT_DAILY : IDS_SCHED_FORMAT, 
        szFormat, ARRAYSIZE(szFormat)))
    {
        TCHAR szTimeFormat[32];
        SYSTEMTIME st;

        DateTime_GetSystemtime(GetDlgItem(hDlg, IDC_SCHEDULE_TIME), &st);

        UpdateTimeFormat(szTimeFormat, ARRAYSIZE(szTimeFormat));
        GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st,
                      szTimeFormat, szTime, ARRAYSIZE(szTime));
        GetDlgItemText(hDlg, IDC_SCHEDULE_DAYS, szDays, ARRAYSIZE(szDays));

        lpArguments[0] = szDays;
        lpArguments[1] = szTime;

        if (FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          szFormat, 0, 0, szSchedName, ARRAYSIZE(szSchedName),
                          (va_list *)&lpArguments[0]))
        {
            SetDlgItemText(hDlg, IDC_SCHEDULE_NAME, szSchedName);
        }
    }
}
#endif

BOOL NewSched_ResolveNameConflictHelper(HWND hDlg, TASK_TRIGGER *pTrig, 
                                        SYNCSCHEDULECOOKIE *pSchedCookie)
{
    BOOL bResult;

    SYSTEMTIME st;
    TCHAR szSchedName[MAX_PATH];

    GetDlgItemText(hDlg, IDC_SCHEDULE_NAME, szSchedName, ARRAYSIZE(szSchedName));

    TrimWhiteSpace(szSchedName);

    if (szSchedName[0] != 0)
    {
        bResult = TRUE;
            
        memset(pTrig, 0, sizeof(TASK_TRIGGER));
        pTrig->cbTriggerSize = sizeof(TASK_TRIGGER);

        GetLocalTime(&st);
        pTrig->wBeginYear = st.wYear;
        pTrig->wBeginMonth = st.wMonth;
        pTrig->wBeginDay = st.wDay;

        DateTime_GetSystemtime(GetDlgItem(hDlg, IDC_SCHEDULE_TIME), &st);
        pTrig->wStartHour = st.wHour;
        pTrig->wStartMinute = st.wMinute;

        pTrig->TriggerType = TASK_TIME_TRIGGER_DAILY;

        BOOL bTranslated;
        pTrig->Type.Daily.DaysInterval = (WORD)GetDlgItemInt(hDlg, IDC_SCHEDULE_DAYS, &bTranslated, FALSE);

        int iConflictResult = HandleScheduleNameConflict(szSchedName, 
                                                         pTrig,
                                                         hDlg,
                                                         pSchedCookie);
        switch (iConflictResult)
        {
            case CONFLICT_NONE:
                ASSERT(GUID_NULL == *pSchedCookie);
                break;

            case CONFLICT_RESOLVED_USE_NEW:
                ASSERT(GUID_NULL != *pSchedCookie);
                break;

            case CONFLICT_RESOLVED_USE_OLD:
                ASSERT(GUID_NULL == *pSchedCookie);
                pTrig->cbTriggerSize = 0;
                break;

            case CONFLICT_UNRESOLVED:
                bResult = FALSE;
                break;
        }
    }
    else
    {
        SGMessageBox(hDlg, IDS_EMPTY_SCHEDULE_NAME, MB_OK | MB_ICONWARNING);
        bResult = FALSE;
    }

    return bResult;
}

void NewSched_CreateScheduleHelper(HWND hDlg, TASK_TRIGGER *pTrig,
                                   SYNCSCHEDULECOOKIE *pSchedCookie)
{
    HRESULT hr;
    
    if (GUID_NULL == *pSchedCookie)
    {
        //  Create new schedule
        TCHAR szSchedName[MAX_PATH];
        WCHAR wszSchedName[MAX_PATH];

        DWORD dwSyncScheduleFlags = 
            (IsDlgButtonChecked(hDlg, IDC_WIZ_SCHEDULE_AUTOCONNECT) == BST_CHECKED) 
                 ? SYNCSCHEDINFO_FLAGS_AUTOCONNECT : 0;

        GetDlgItemText(hDlg, IDC_SCHEDULE_NAME, szSchedName, ARRAYSIZE(szSchedName));
        MyStrToOleStrN(wszSchedName, ARRAYSIZE(wszSchedName), szSchedName);
        hr = CreateSchedule(wszSchedName, dwSyncScheduleFlags, pSchedCookie, pTrig, FALSE);

        ASSERT(SUCCEEDED(hr));
    }
    else if (sizeof(TASK_TRIGGER) == pTrig->cbTriggerSize)
    {
        //  Update existing schedule with new task trigger
        hr = UpdateScheduleTrigger(pSchedCookie, pTrig);
        ASSERT(SUCCEEDED(hr));
    }
    else
    {
        //  Use existing schedule without munging it
    }
}

void NewSched_SetDefaultScheduleName(HWND hDlg)
{
    if (SUCCEEDED(CoInitialize(NULL)))
    {   
        ISyncScheduleMgr *pSyncScheduleMgr;
        
        if (SUCCEEDED(CoCreateInstance(CLSID_SyncMgr, NULL, CLSCTX_ALL, 
                                       IID_ISyncScheduleMgr, 
                                       (void **)&pSyncScheduleMgr)))
        {
            SYNCSCHEDULECOOKIE schedCookie;
            ISyncSchedule *pSyncSchedule;

            if (SUCCEEDED(pSyncScheduleMgr->CreateSchedule(L"", 0, &schedCookie, &pSyncSchedule)))
            {
                WCHAR wszSchedName[MAX_PATH];
                DWORD cchSchedName = ARRAYSIZE(wszSchedName);

                if (SUCCEEDED(pSyncSchedule->GetScheduleName(&cchSchedName, wszSchedName)))
                {
                    TCHAR szSchedName[MAX_PATH];

                    MyOleStrToStrN(szSchedName, ARRAYSIZE(szSchedName), wszSchedName);
                    SetDlgItemText(hDlg, IDC_SCHEDULE_NAME, szSchedName);
                }
                pSyncSchedule->Release();
            }
            pSyncScheduleMgr->Release();
        }
        CoUninitialize();
    }
}

void NewSched_OnInitDialogHelper(HWND hDlg)
{
    SYSTEMTIME st;

    GetLocalTime(&st);

    Edit_LimitText(GetDlgItem(hDlg, IDC_SCHEDULE_NAME), MAX_PATH - 1);
    Edit_LimitText(GetDlgItem(hDlg, IDC_SCHEDULE_DAYS), 2);
    SendMessage(GetDlgItem(hDlg, IDC_SCHEDULE_DAYS_SPIN), 
                UDM_SETRANGE, 0, MAKELONG(99, 1));
    SendMessage(GetDlgItem(hDlg, IDC_SCHEDULE_DAYS_SPIN), UDM_SETPOS, 0, 1);

    HWND hwndTimePicker = GetDlgItem(hDlg, IDC_SCHEDULE_TIME);
    TCHAR szTimeFormat[32];

    UpdateTimeFormat(szTimeFormat, ARRAYSIZE(szTimeFormat));
    DateTime_SetSystemtime(hwndTimePicker, GDT_VALID, &st);
    DateTime_SetFormat(hwndTimePicker, szTimeFormat);

    NewSched_SetDefaultScheduleName(hDlg);
}

int KeepSpinNumberInRange(HWND hdlg, int idEdit, int idSpin, int minVal, int maxVal)
{
    BOOL bTranslate;
    int val = GetDlgItemInt(hdlg, idEdit, &bTranslate, FALSE);
    if (!bTranslate || (val  < minVal) || (val > maxVal))
    {   
        //  We have a problem, query the spin control
        val = LOWORD(SendDlgItemMessage(hdlg, idSpin, UDM_GETPOS, 0, 0));
        val = max(minVal, min(maxVal, val));
        SetDlgItemInt(hdlg, idEdit, val, FALSE);
    }

    return val;
}


void SetPropSheetFlags(POOEBuf pBuf, BOOL bSet, DWORD dwPropSheetFlags)
{
    if (bSet)
    {
        pBuf->m_dwPropSheetFlags |= dwPropSheetFlags;
    }
    else
    {
        pBuf->m_dwPropSheetFlags &= ~dwPropSheetFlags;
    }
}


HRESULT FindURLProps(LPCTSTR m_URL, PROPVARIANT * pVarInfo)
{
    HRESULT hr; 

    hr = IntSiteHelper(m_URL, &c_rgPropRead[PROP_SUBSCRIPTION], pVarInfo, 1, FALSE);
    return hr;
}

HRESULT LoadOOEntryInfo(POOEBuf pBuf, SUBSCRIPTIONCOOKIE *pCookie, DWORD *pdwSize)
{
    HRESULT hr;

    if (!pBuf || !pCookie || !pdwSize)
    {
        TraceMsg(TF_ALWAYS, "Invalid ARG (1/2/3) %x %x", pBuf, pCookie, pdwSize);
        return E_INVALIDARG;
    }

    ISubscriptionItem *psi;
    hr = SubscriptionItemFromCookie(FALSE, pCookie, &psi);

    if (SUCCEEDED(hr))
    {
        SUBSCRIPTIONITEMINFO sii;
        
        sii.cbSize = sizeof(SUBSCRIPTIONITEMINFO);

        hr = psi->GetSubscriptionItemInfo(&sii);

        if (SUCCEEDED(hr))
        {       
            ZeroMemory((void *)pBuf, sizeof(OOEBuf));
            pBuf->m_Cookie = *pCookie;

        //  TODO: resolve scheduling goo!

        //    pBuf->groupCookie = pItem->groupCookie;
        //    pBuf->grfTaskTrigger = pItem->TaskData.dwTaskFlags;
        /*    if (pItem->groupCookie == CLSID_NULL)   {
                pBuf->m_Trigger = pItem->TaskTrigger;
                if (pBuf->m_Trigger.cbTriggerSize != sizeof(TASK_TRIGGER))  {
                    ASSERT(0);
                    return E_INVALIDARG;
                }
            } else  {
                pBuf->m_Trigger.cbTriggerSize = 0;  //  Invalid
            }
        */

            pBuf->clsidDest = sii.clsidAgent;

            if (!IsNativeAgent(sii.clsidAgent))
            {
                pBuf->dwFlags = PROP_WEBCRAWL_EXTERNAL;
            }
            else
            {
                pBuf->dwFlags = PROP_WEBCRAWL_ALL; 
            }

            hr = ReadProperties(pBuf);
            *pdwSize = BufferSize(pBuf);
        }
        psi->Release();
    }

    return hr;
}

/////////////////////////////////////////////////
//
//  SaveBufferChange
//      newBuf: [in/out]
/////////////////////////////////////////////////

HRESULT SaveBufferChange(POOEBuf newBuf, BOOL bCreate)
{
    HRESULT hr;
    DWORD   dwSize;
    POOEntry pooe;
    LPMYPIDL newPidl;

    ASSERT (newBuf);
    if (newBuf->dwFlags == 0)
        return S_OK;

    dwSize = BufferSize(newBuf);
    newPidl = COfflineFolderEnum::NewPidl(dwSize);
    if (!newPidl)
        return E_OUTOFMEMORY;

    pooe = &(newPidl->ooe);                
    CopyToMyPooe(newBuf, pooe);
    newBuf->dwFlags = 0;
    hr = PersistUpdate(pooe, bCreate);
    if (SUCCEEDED(hr))  {
        DWORD dwPropSheetFlags = newBuf->m_dwPropSheetFlags; //  Preserve prop sheet flags
        
        hr = LoadWithCookie(URL(pooe), newBuf, &dwSize, &(pooe->m_Cookie));

        newBuf->m_dwPropSheetFlags = dwPropSheetFlags;  //  restore 
        newBuf->dwFlags = 0;
        if (hr == S_OK)  {
            COfflineFolderEnum::FreePidl(newPidl);
            newPidl = COfflineFolderEnum::NewPidl(dwSize);
            if (!(newPidl))  {
                return E_OUTOFMEMORY;
            }
            pooe = &(newPidl->ooe);
            CopyToMyPooe(newBuf, pooe);
        }
        _GenerateEvent(SHCNE_UPDATEITEM, (LPITEMIDLIST)newPidl, NULL);
    }
    COfflineFolderEnum::FreePidl(newPidl);
    return hr;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// IntSiteHelper
//
// pszURL               url to read/write props for
// pPropspec            properties to read or write
// pReadPropvar         where to store or get properties
// uPropVarArraySize    number of properties
// fWrite               read/write flag
//

HRESULT IntSiteHelper(LPCTSTR pszURL, const PROPSPEC *pPropspec,
        PROPVARIANT *pPropvar, UINT uPropVarArraySize, BOOL fWrite)
{
    HRESULT                     hr;
    IUniformResourceLocator *   purl = NULL;
    IPropertySetStorage *       ppropsetstg = NULL; // init to keep compiler happy
    IPropertyStorage *          ppropstg = NULL; // init to keep compiler happy

    hr = SHCoCreateInstance(NULL, &CLSID_InternetShortcut, NULL, 
            IID_IUniformResourceLocator, (LPVOID*)&purl);

    if(SUCCEEDED(hr)) {
        hr = purl->SetURL(pszURL, 0);
    }

    if(SUCCEEDED(hr)) {
        hr = purl->QueryInterface(IID_IPropertySetStorage,
                (LPVOID *)&ppropsetstg);
    }

    if(SUCCEEDED(hr)) {
        hr = ppropsetstg->Open(FMTID_InternetSite, STGM_READWRITE, &ppropstg);
        ppropsetstg->Release();
    }

    if(SUCCEEDED(hr)) {
        if(fWrite) {
            hr = ppropstg->WriteMultiple(uPropVarArraySize, pPropspec,
                            pPropvar, 0);
            ppropstg->Commit(STGC_DEFAULT);
        } else {
            hr = ppropstg->ReadMultiple(uPropVarArraySize, pPropspec,
                            pPropvar);
        }
        ppropstg->Release();
    }

    if(purl)
        purl->Release();

    return hr;
}

// CODE FROM SYNCMGR SOURCES.

//
// Local constants
// 
// DEFAULT_TIME_FORMAT - what to use if there's a problem getting format
//                       from system.
//
#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))
#define DEFAULT_TIME_FORMAT         TEXT("hh:mm tt")
#define GET_LOCALE_INFO(lcid)                           \
        {                                               \
            cch = GetLocaleInfo(LOCALE_USER_DEFAULT,    \
                                (lcid),                 \
                                tszScratch,             \
                                ARRAYLEN(tszScratch));  \
            if (!cch)                                   \
            {                                           \
                break;                                  \
            }                                           \
        }
//+--------------------------------------------------------------------------
//
//  Function:   UpdateTimeFormat
//
//  Synopsis:   Construct a time format containing hour and minute for use
//              with the date picker control.
//
//  Arguments:  [tszTimeFormat] - buffer to fill with time format
//              [cchTimeFormat] - size in chars of buffer
//
//  Modifies:   *[tszTimeFormat]
//
//  History:    11-18-1996   DavidMun   Created
//
//  Notes:      This is called on initialization and for wininichange
//              processing.
//
//---------------------------------------------------------------------------
void 
UpdateTimeFormat(
        LPTSTR tszTimeFormat, 
        ULONG  cchTimeFormat)
{
    ULONG cch;
    TCHAR tszScratch[80];
    BOOL  fAmPm = FALSE;
    BOOL  fAmPmPrefixes = FALSE;
    BOOL  fLeadingZero = FALSE;

    do
    {
        GET_LOCALE_INFO(LOCALE_ITIME);
        fAmPm = (*tszScratch == TEXT('0'));

        if (fAmPm)
        {
            GET_LOCALE_INFO(LOCALE_ITIMEMARKPOSN);
            fAmPmPrefixes = (*tszScratch == TEXT('1'));
        }

        GET_LOCALE_INFO(LOCALE_ITLZERO);
        fLeadingZero = (*tszScratch == TEXT('1'));

        GET_LOCALE_INFO(LOCALE_STIME);

        //
        // See if there's enough room in destination string
        //

        cch = 1                     +  // terminating nul
              1                     +  // first hour digit specifier "h"
              2                     +  // minutes specifier "mm"
              (fLeadingZero != 0)   +  // leading hour digit specifier "h"
              lstrlen(tszScratch)   +  // separator string
              (fAmPm ? 3 : 0);         // space and "tt" for AM/PM

        if (cch > cchTimeFormat)
        {
            cch = 0; // signal error
        }
    } while (0);

    //
    // If there was a problem in getting locale info for building time string
    // just use the default and bail.
    //

    if (!cch)
    {
        StrCpy(tszTimeFormat, DEFAULT_TIME_FORMAT);
        return;
    }

    //
    // Build a time string that has hours and minutes but no seconds.
    //

    tszTimeFormat[0] = TEXT('\0');

    if (fAmPm)
    {
        if (fAmPmPrefixes)
        {
            StrCpy(tszTimeFormat, TEXT("tt "));
        }

        StrCat(tszTimeFormat, TEXT("h"));
 
        if (fLeadingZero)
        {
            StrCat(tszTimeFormat, TEXT("h"));
        }
    }
    else 
    {
        StrCat(tszTimeFormat, TEXT("H"));
 
        if (fLeadingZero)
        {
            StrCat(tszTimeFormat, TEXT("H"));
        }
    }

    StrCat(tszTimeFormat, tszScratch); // separator
    StrCat(tszTimeFormat, TEXT("mm"));

    if (fAmPm && !fAmPmPrefixes)
    {
        StrCat(tszTimeFormat, TEXT(" tt"));
    }
}
