#include "private.h"
#include "offl_cpp.h"
#include "propshts.h"
#include "subsmgrp.h"
#include <iehelpid.h>

#include <mluisupp.h>

#define INITGUID
#include <initguid.h>
#include "imnact.h"
#include "helper.h"

void WriteDefaultEmail(LPTSTR szBuf);
void WriteDefaultSMTPServer(LPTSTR szBuf);

const TCHAR c_szDefEmail[] = TEXT("DefaultEmail");
const TCHAR c_szDefServer[] = TEXT("DefaultSMTPServer");
TCHAR c_szHelpFile[] = TEXT("iexplore.hlp");

struct NEWSCHED_DATA
{
    SYNCSCHEDULECOOKIE SchedCookie;
    TCHAR szSchedName[MAX_PATH];
};

#define MIN_DOWNLOAD_K  50
#define MAX_DOWNLOAD_K  UD_MAXVAL
#define DEF_DOWNLOAD_K  500

DWORD aHelpIDs[] = {

//  Schedule page
    IDC_SCHEDULE_TEXT,              IDH_GROUPBOX,
    IDC_MANUAL_SYNC,                IDH_SUBPROPS_SCHEDTAB_MANUAL_SCHEDULE,
    IDC_SCHEDULED_SYNC,             IDH_SUBPROPS_SCHEDTAB_CUSTOM_SCHEDULE,
    IDC_SCHEDULE_LIST,              IDH_SUBPROPS_SCHEDTAB_SCHEDDESC,
    IDC_SCHEDULE_NEW,               IDH_NEW_OFFLINE_SCHED,
    IDC_SCHEDULE_EDIT,              IDH_EDIT_OFFLINE_SCHED,
    IDC_SCHEDULE_REMOVE,            IDH_REMOVE_OFFLINE_SCHED,
//    IDC_IDLE_ONLY,                  IDH_SUBPROPS_SCHED_DONTUPDATE,

//  Download page
    IDC_CONTENT_GROUPBOX,           IDH_GROUPBOX,
    IDC_DOWNLOAD_PAGES_LABEL1,      IDH_SUBPROPS_RECTAB_ADVOPTS_DOWNLOAD_PAGES_DEEP,
    IDC_LEVELS,                     IDH_SUBPROPS_RECTAB_ADVOPTS_DOWNLOAD_PAGES_DEEP,
    IDC_LEVELS_SPIN,                IDH_SUBPROPS_RECTAB_ADVOPTS_DOWNLOAD_PAGES_DEEP,
    IDC_DOWNLOAD_PAGES_LABEL2,      IDH_SUBPROPS_RECTAB_ADVOPTS_DOWNLOAD_PAGES_DEEP,
    IDC_FOLLOW_LINKS,               IDH_SUBPROPS_RECTAB_ADVOPTS_FOLLOW_LINKS,
    IDC_LIMIT_SPACE_CHECK,          IDH_SUBPROPS_RECTAB_ADVOPTS_MAX_DOWNLOAD,
    IDC_LIMIT_SPACE_EDIT,           IDH_SUBPROPS_RECTAB_ADVOPTS_MAX_DOWNLOAD,
    IDC_LIMIT_SPACE_SPIN,           IDH_SUBPROPS_RECTAB_ADVOPTS_MAX_DOWNLOAD,
    IDC_LIMIT_SPACE_TEXT,           IDH_SUBPROPS_RECTAB_ADVOPTS_MAX_DOWNLOAD,   
    IDC_ADVANCED,                   IDH_SUBPROPS_RECTAB_ADVANCED,
    IDC_EMAIL_GROUPBOX,             IDH_SUBPROPS_RECTAB_EMAIL_NOTIFICATION,
    IDC_EMAIL_NOTIFY,               IDH_SUBPROPS_RECTAB_EMAIL_NOTIFICATION,
    IDC_EMAIL_ADDRESS_TEXT,         IDH_SUBPROPS_RECTAB_MAILOPTS_EMAIL_ADDRESS,
    IDC_EMAIL_ADDRESS,              IDH_SUBPROPS_RECTAB_MAILOPTS_EMAIL_ADDRESS,
    IDC_EMAIL_SERVER_TEXT,          IDH_SUBPROPS_RECTAB_MAILOPTS_EMAIL_SERVER,
    IDC_EMAIL_SERVER,               IDH_SUBPROPS_RECTAB_MAILOPTS_EMAIL_SERVER,
    IDC_LOGIN_LABEL,                IDH_SUBPROPS_RECTAB_CHANNEL_LOGIN,
    IDC_LOGIN,                      IDH_SUBPROPS_RECTAB_CHANNEL_LOGIN,
    IDC_DOWNLOAD_ALL,               IDH_CHANNEL_DOWNLOAD_ALL,
    IDC_DOWNLOAD_MIN,               IDH_CHANNEL_DOWNLOAD_COVER_N_TOC,

    //  Advanced popup
    IDC_ADVANCED_GROUPBOX,          IDH_GROUPBOX,
    IDC_DOWNLOAD_IMAGES,            IDH_SUBPROPS_RECTAB_ADVOPTS_DOWNLOAD_IMAGES,
    IDC_DOWNLOAD_MEDIA,             IDH_SUBPROPS_RECTAB_ADVOPTS_DOWNLOAD_SOUND,
    IDC_DOWNLOAD_APPLETS,           IDH_SUBPROPS_RECTAB_ADVOPTS_DOWNLOAD_ACTIVEX,
    IDC_DOWNLOAD_ONLY_HTML_LINKS,   IDH_SUBPROPS_RECTAB_ADVOPTS_ONLY_HTML_LINKS,

    //  Login popup
    IDC_USERNAME_LABEL,             IDH_SUBPROPS_RECTAB_LOGINOPTS_USER_ID,
    IDC_USERNAME,                   IDH_SUBPROPS_RECTAB_LOGINOPTS_USER_ID,
    IDC_PASSWORD_LABEL,             IDH_SUBPROPS_RECTAB_LOGINOPTS_PASSWORD,
    IDC_PASSWORD,                   IDH_SUBPROPS_RECTAB_LOGINOPTS_PASSWORD,

    //  New schedule popup
    IDC_SCHEDULE_LABEL1,            IDH_NEWSCHED_EVERY_AT_TIME,
    IDC_SCHEDULE_DAYS,              IDH_NEWSCHED_EVERY_AT_TIME,
    IDC_SCHEDULE_DAYS_SPIN,         IDH_NEWSCHED_EVERY_AT_TIME,
    IDC_SCHEDULE_LABEL2,            IDH_NEWSCHED_EVERY_AT_TIME,
    IDC_SCHEDULE_TIME,              IDH_NEWSCHED_EVERY_AT_TIME,
    IDC_SCHEDULE_NAME_TEXT,         IDH_NEWSCHED_NAME,
    IDC_SCHEDULE_NAME,              IDH_NEWSCHED_NAME,
    IDC_SCHEDULE_LABEL3,            IDH_NEWSCHED_EVERY_AT_TIME,

    //  Summary page
    IDC_NAME,                   IDH_SUBPROPS_SUBTAB_SUBSCRIBED_NAME,
    IDC_URL_TEXT,               IDH_SUBPROPS_SUBTAB_SUBSCRIBED_URL,
    IDC_URL,                    IDH_SUBPROPS_SUBTAB_SUBSCRIBED_URL,
    IDC_VISITS_TEXT,            IDH_WEBDOC_VISITS,
    IDC_VISITS,                 IDH_WEBDOC_VISITS,
    IDC_MAKE_OFFLINE,           IDH_MAKE_AVAIL_OFFLINE,
    IDC_SUMMARY,                IDH_GROUPBOX,
    IDC_LAST_SYNC_TEXT,         IDH_SUBPROPS_SUBTAB_LAST,
    IDC_LAST_SYNC,              IDH_SUBPROPS_SUBTAB_LAST,
    IDC_DOWNLOAD_SIZE_TEXT,     IDH_SUBPROPS_DLSIZE,
    IDC_DOWNLOAD_SIZE,          IDH_SUBPROPS_DLSIZE,
    IDC_DOWNLOAD_RESULT_TEXT,   IDH_SUBPROPS_SUBTAB_RESULT,
    IDC_DOWNLOAD_RESULT,        IDH_SUBPROPS_SUBTAB_RESULT,

    //  dah end
    0,                              0
};



/********************************************************************************
    Property sheet helpers
*********************************************************************************/

inline POOEBuf GetBuf(HWND hdlg)
{
    POOEBuf pBuf = (POOEBuf) GetWindowLongPtr(hdlg, DWLP_USER);

    return pBuf;
}

void EnableControls(HWND hdlg, const int *pIDs, int nIDs, BOOL bEnable)
{
    for (int i = 0; i < nIDs; i++)
    {
        EnableWindow(GetDlgItem(hdlg, *pIDs++), bEnable);
    }
}

/********************************************************************************
    Summary property sheet code
*********************************************************************************/
inline POOEBuf Summary_GetBuf(HWND hdlg)
{
    CSubscriptionMgr *pSubsMgr = (CSubscriptionMgr*) GetWindowLongPtr(hdlg, DWLP_USER);

    ASSERT(NULL != pSubsMgr);

    return (NULL != pSubsMgr) ? pSubsMgr->m_pBuf : NULL;
}

void Summary_ShowOfflineSummary(HWND hdlg, POOEBuf pBuf, BOOL bShow)
{
    static const int offSumIDs[] =
    {
        IDC_SUMMARY,
        IDC_LAST_SYNC_TEXT,
        IDC_LAST_SYNC,
        IDC_DOWNLOAD_SIZE_TEXT,
        IDC_DOWNLOAD_SIZE,
        IDC_DOWNLOAD_RESULT,
        IDC_DOWNLOAD_RESULT_TEXT,
        IDC_FREESPACE_TEXT
    };

    if (bShow)
    {
        TCHAR szLastSync[128];
        TCHAR szDownloadSize[128];
        TCHAR szDownloadResult[128];

        MLLoadString(IDS_VALUE_UNKNOWN, szLastSync, ARRAYSIZE(szLastSync));
        StrCpyN(szDownloadSize, szLastSync, ARRAYSIZE(szDownloadSize));
        StrCpyN(szDownloadResult, szLastSync, ARRAYSIZE(szDownloadResult));

        ISubscriptionItem *psi;
            
        if (SUCCEEDED(SubscriptionItemFromCookie(FALSE, &pBuf->m_Cookie, &psi)))
        {
            enum { spLastSync, spDownloadSize, spDownloadResult };

            static const LPCWSTR pProps[] =
            { 
                c_szPropCompletionTime,
                c_szPropCrawlActualSize,
                c_szPropStatusString
            };
            VARIANT vars[ARRAYSIZE(pProps)];

            if (SUCCEEDED(psi->ReadProperties(ARRAYSIZE(pProps), pProps, vars)))
            {
                if (VT_DATE == vars[spLastSync].vt)
                {
                    FILETIME ft, ft2;
                    DWORD dwFlags = FDTF_DEFAULT;
                    
                    VariantTimeToFileTime(vars[spLastSync].date, ft);
                    LocalFileTimeToFileTime(&ft, &ft2);
                    SHFormatDateTime(&ft2, &dwFlags, szLastSync, ARRAYSIZE(szLastSync));
                }

                if (VT_I4 == vars[spDownloadSize].vt)
                {
                    StrFormatByteSize(vars[spDownloadSize].lVal * 1024, 
                                      szDownloadSize, ARRAYSIZE(szDownloadSize));
                }

                if (VT_BSTR == vars[spDownloadResult].vt)
                {
                #ifdef UNICODE
                    wnsprintf(szDownloadResult, ARRAYSIZE(szDownloadResult),
                              TEXT("%s"), vars[spDownloadResult].bstrVal);
                #else
                    wnsprintf(szDownloadResult, ARRAYSIZE(szDownloadResult),
                              TEXT("%S"), vars[spDownloadResult].bstrVal);
                #endif
                }

                for (int i = 0; i < ARRAYSIZE(pProps); i++)
                {
                    VariantClear(&vars[i]);
                }
            }
            psi->Release();
        }

        SetDlgItemText(hdlg, IDC_LAST_SYNC, szLastSync);
        SetDlgItemText(hdlg, IDC_DOWNLOAD_SIZE, szDownloadSize);
        SetDlgItemText(hdlg, IDC_DOWNLOAD_RESULT, szDownloadResult);
    }

    for (int i = 0; i < ARRAYSIZE(offSumIDs); i++)
    {
        ShowWindow(GetDlgItem(hdlg, offSumIDs[i]), bShow ? SW_SHOW : SW_HIDE);
    }
}

BOOL Summary_AddPageCallback(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    return PropSheet_AddPage((HWND)lParam, hpage) ? TRUE : FALSE;
}


BOOL Summary_OnCommand(HWND hdlg, WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
    BOOL bResult = FALSE;
    BOOL bChanged = FALSE;
    POOEBuf pBuf = Summary_GetBuf(hdlg);

    switch (wID)
    {
        case IDC_MAKE_OFFLINE:
            if (BN_CLICKED == wNotifyCode)
            {
                CSubscriptionMgr *pSubsMgr = (CSubscriptionMgr*) GetWindowLongPtr(hdlg, DWLP_USER);

                BOOL bShow = IsDlgButtonChecked(hdlg, IDC_MAKE_OFFLINE);

                Summary_ShowOfflineSummary(hdlg, pBuf, bShow);
                
                if (NULL != pSubsMgr)
                {
                    if (bShow)
                    {
                        pSubsMgr->AddPages(Summary_AddPageCallback, (LPARAM)GetParent(hdlg));
                    }
                    else
                    {
                        pSubsMgr->RemovePages(GetParent(hdlg));
                    }
                }
                bChanged = TRUE;
            }
            bResult = TRUE;
            break;
    }

    if (bChanged)
    {
        PropSheet_Changed(GetParent(hdlg), hdlg);        
    }

    return bResult;
}

BOOL Summary_OnNotify(HWND hdlg, int idCtrl, LPNMHDR pnmh)
{
    BOOL bResult = TRUE;

    switch (pnmh->code)
    {
        case PSN_APPLY:
        {
            CSubscriptionMgr *pSubsMgr = (CSubscriptionMgr*) GetWindowLongPtr(hdlg, DWLP_USER);
            POOEBuf pBuf = Summary_GetBuf(hdlg);

            ASSERT(NULL != pSubsMgr);
            ASSERT(NULL != pBuf);
            
            if ((NULL != pSubsMgr) && (NULL != pBuf))
            {
                if (IsDlgButtonChecked(hdlg, IDC_MAKE_OFFLINE))
                {
                    pBuf->dwFlags = PROP_WEBCRAWL_ALL;
                    SaveBufferChange(pBuf, TRUE);
                }
                else
                {                
                    WCHAR wszURL[INTERNET_MAX_URL_LENGTH];

                    MyStrToOleStrN(wszURL, ARRAYSIZE(wszURL), pBuf->m_URL);
                    pSubsMgr->DeleteSubscription(wszURL, NULL); 
                }
            }
            break;
        }
            
        default:
            bResult = FALSE;
            break;
    }
    return bResult;
}

BOOL Summary_OnInitDialog(HWND hdlg, HWND hwndFocus, LPARAM lParam)
{
    ASSERT(NULL != ((LPPROPSHEETPAGE)lParam));
    ASSERT(NULL != ((LPPROPSHEETPAGE)lParam)->lParam);

    SetWindowLongPtr(hdlg, DWLP_USER, ((LPPROPSHEETPAGE)lParam)->lParam);

    //  Now read in values and populate the dialog
    POOEBuf pBuf = Summary_GetBuf(hdlg);
    ISubscriptionItem *psi;
    HICON hicon;
    BOOL bSubscribed;

    HRESULT hr = SubscriptionItemFromCookie(FALSE, &pBuf->m_Cookie, &psi);

    if (SUCCEEDED(hr))
    {
        bSubscribed = TRUE;
        hicon = LoadItemIcon(psi, TRUE);
        psi->Release();
    }
    else
    {
        bSubscribed = FALSE;
        hicon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_WEBDOC));
    }

    SendDlgItemMessage(hdlg, IDC_ICONEX2, STM_SETICON, (WPARAM)hicon, 0);

    if ((!IsHTTPPrefixed(pBuf->m_URL)) ||
        (bSubscribed && SHRestricted2(REST_NoRemovingSubscriptions, pBuf->m_URL, 0)) ||
        (!bSubscribed && SHRestricted2(REST_NoAddingSubscriptions, pBuf->m_URL, 0)))
    {
        EnableWindow(GetDlgItem(hdlg, IDC_MAKE_OFFLINE), FALSE);
    }
        
    SetDlgItemText(hdlg, IDC_NAME, pBuf->m_Name);
    SetDlgItemText(hdlg, IDC_URL, pBuf->m_URL);

    CheckDlgButton(hdlg, IDC_MAKE_OFFLINE, bSubscribed);

    TCHAR szVisits[256];

    BYTE cei[MY_MAX_CACHE_ENTRY_INFO];
    LPINTERNET_CACHE_ENTRY_INFO pcei = (LPINTERNET_CACHE_ENTRY_INFO)cei;
    DWORD cbcei = sizeof(cei);

    if (GetUrlCacheEntryInfo(pBuf->m_URL, pcei, &cbcei))
    {
        wnsprintf(szVisits, ARRAYSIZE(szVisits), TEXT("%d"), 
                  pcei->dwHitRate);
    }
    else
    {
        MLLoadString(IDS_VALUE_UNKNOWN, szVisits, 
                   ARRAYSIZE(szVisits));
    }
    SetDlgItemText(hdlg, IDC_VISITS, szVisits);


    Summary_ShowOfflineSummary(hdlg, pBuf, bSubscribed);

    return TRUE;
}

void Summary_OnDestroy(HWND hdlg)
{
    POOEBuf pBuf = Summary_GetBuf(hdlg);

    if ((!(pBuf->m_dwPropSheetFlags & PSF_IS_ALREADY_SUBSCRIBED)) && 
        (IsDlgButtonChecked(hdlg, IDC_MAKE_OFFLINE)))
    {
        SendUpdateRequests(NULL, &pBuf->m_Cookie, 1);
    }
}

INT_PTR CALLBACK SummaryPropDlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL bHandled = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
            bHandled = Summary_OnInitDialog(hdlg, (HWND)wParam, lParam);
            break;

        case WM_COMMAND:
            bHandled = Summary_OnCommand(hdlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
            break;

        case WM_DESTROY:
            Summary_OnDestroy(hdlg);
            break;
            
        case WM_NOTIFY:
            bHandled = Summary_OnNotify(hdlg, (int)wParam, (LPNMHDR)lParam);
            break;

        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
                    HELP_WM_HELP, (DWORD_PTR)aHelpIDs);
            bHandled = TRUE;
            break;

        case WM_CONTEXTMENU:
            SHWinHelpOnDemandWrap((HWND)wParam, c_szHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)aHelpIDs);
            bHandled = TRUE;
            break;
    }
    
    return bHandled;
}


/********************************************************************************
    Schedule property sheet code
*********************************************************************************/

void Sched_EnableScheduleButtons(HWND hdlg)
{
    POOEBuf pBuf = GetBuf(hdlg);
    ASSERT(pBuf);

    BOOL bEditAllowed = !(pBuf->m_dwPropSheetFlags & 
                            (PSF_NO_EDITING_SCHEDULES | PSF_NO_SCHEDULED_UPDATES));
    BOOL bEnable = (bEditAllowed) &&
                   ListView_GetSelectedCount(GetDlgItem(hdlg, IDC_SCHEDULE_LIST));

    EnableWindow(GetDlgItem(hdlg, IDC_SCHEDULE_NEW), bEditAllowed);
    EnableWindow(GetDlgItem(hdlg, IDC_SCHEDULE_EDIT), bEnable);

    //  TODO: Don't enable remove for publisher's schedule
    EnableWindow(GetDlgItem(hdlg, IDC_SCHEDULE_REMOVE), bEnable);
}

struct SCHED_ENUM_DATA
{
    HWND hwndSchedList;
    POOEBuf pBuf;
    SYNCSCHEDULECOOKIE customSchedule;
    BOOL bHasAtLeastOneSchedule;
};

struct SCHED_LIST_DATA
{
    SYNCSCHEDULECOOKIE SchedCookie;
    BOOL bChecked;
    BOOL bStartChecked;
};

inline int SchedList_GetIndex(HWND hwndSchedList, int index)
{
    return (index != -1) ? index :
           ListView_GetNextItem(hwndSchedList, -1, LVNI_ALL | LVNI_SELECTED);
}

void SchedList_GetName(HWND hwndSchedList, int index, TCHAR *pszSchedName, int cchSchedName)
{
    LV_ITEM lvi = {0};

    lvi.iItem = SchedList_GetIndex(hwndSchedList, index);

    if (lvi.iItem != -1)
    {
        lvi.mask = LVIF_TEXT;
        lvi.pszText = pszSchedName;
        lvi.cchTextMax = cchSchedName;
        ListView_GetItem(hwndSchedList, &lvi);
    }
}

void SchedList_SetName(HWND hwndSchedList, int index, LPTSTR pszSchedName)
{
    LV_ITEM lvi = {0};

    lvi.iItem = SchedList_GetIndex(hwndSchedList, index);

    if (lvi.iItem != -1)
    {
        lvi.mask = LVIF_TEXT;
        lvi.pszText = pszSchedName;
        ListView_SetItem(hwndSchedList, &lvi);
    }
}

SCHED_LIST_DATA *SchedList_GetData(HWND hwndSchedList, int index)
{
    SCHED_LIST_DATA *psld = NULL;
    LV_ITEM lvi = {0};

    lvi.iItem = SchedList_GetIndex(hwndSchedList, index);

    if (lvi.iItem != -1)
    {
        lvi.mask = LVIF_PARAM;
        if (ListView_GetItem(hwndSchedList, &lvi))
        {
            psld = (SCHED_LIST_DATA *)lvi.lParam;
        }
    }

    return psld;
}

void SchedList_UncheckAll(HWND hwndSchedList)
{
    int count = ListView_GetItemCount(hwndSchedList);
    
    for (int i = 0; i < count; i++)
    {
        SCHED_LIST_DATA *psld = SchedList_GetData(hwndSchedList, i);

        psld->bChecked = 0;
        ListView_SetItemState(hwndSchedList, i, COMP_UNCHECKED, LVIS_STATEIMAGEMASK);
    }
}

void SchedList_Select(HWND hwndSchedList, int index)
{
    int curIndex = ListView_GetNextItem(hwndSchedList, -1, LVNI_ALL | LVNI_SELECTED);

    if (curIndex != index)
    {
        ListView_SetItemState(hwndSchedList, curIndex, 0, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_SetItemState(hwndSchedList, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

void SchedList_DeleteData(HWND hwndSchedList, int index, BOOL bDeleteItem)
{
    LV_ITEM lvi = {0};

    lvi.iItem = SchedList_GetIndex(hwndSchedList, index);

    if (lvi.iItem != -1)
    {

        SCHED_LIST_DATA *psld = SchedList_GetData(hwndSchedList, lvi.iItem);

        lvi.mask = LVIF_PARAM;
        lvi.lParam = NULL;

        if ((NULL != psld) && ListView_SetItem(hwndSchedList, &lvi))
        {
            delete psld;
        }

        if (bDeleteItem)
        {
            ListView_DeleteItem(hwndSchedList, lvi.iItem);
        }
    }
}

void SchedList_DeleteAllData(HWND hwndSchedList)
{
    int count = ListView_GetItemCount(hwndSchedList);
    
    for (int i = 0; i < count; i++)
    {
        SchedList_DeleteData(hwndSchedList, i, FALSE);
    }
}

BOOL Sched_EnumCallback(ISyncSchedule *pSyncSchedule, 
                        SYNCSCHEDULECOOKIE *pSchedCookie,
                        LPARAM lParam)
{
    BOOL bAdded = FALSE;
    SCHED_ENUM_DATA *psed = (SCHED_ENUM_DATA *)lParam;
    DWORD dwSyncScheduleFlags;
    SCHED_LIST_DATA *psld = NULL;

    if (SUCCEEDED(pSyncSchedule->GetFlags(&dwSyncScheduleFlags)))
    {
        //  This checks to make sure we only add a publisher's schedule to the
        //  list if it belongs to this item.
        if ((!(dwSyncScheduleFlags & SYNCSCHEDINFO_FLAGS_READONLY)) ||
            (*pSchedCookie == psed->customSchedule))
        {
            psld = new SCHED_LIST_DATA;

            if (NULL != psld)
            {
                WCHAR wszName[MAX_PATH];
                DWORD cchName = ARRAYSIZE(wszName);

                if (SUCCEEDED(pSyncSchedule->GetScheduleName(&cchName, wszName)))
                {
                    TCHAR szName[MAX_PATH];

                    MyOleStrToStrN(szName, ARRAYSIZE(szName), wszName);

                    psld->SchedCookie = *pSchedCookie;
                    psld->bStartChecked = IsCookieOnSchedule(pSyncSchedule, &psed->pBuf->m_Cookie);
                    psld->bChecked = psld->bStartChecked;

                    if (psld->bStartChecked)
                    {
                        psed->bHasAtLeastOneSchedule = TRUE;
                    }

                    LV_ITEM lvItem = { 0 };

                    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
                    lvItem.iItem = (*pSchedCookie == psed->customSchedule) ? 0 : 0x7FFFFFFF;
                    lvItem.lParam = (LPARAM)psld;
                    lvItem.pszText = szName;

                    int index = ListView_InsertItem(psed->hwndSchedList, &lvItem);

                    if (index != -1)
                    {
                            
                        ListView_SetItemState(psed->hwndSchedList, index, 
                                              psld->bStartChecked ? COMP_CHECKED : COMP_UNCHECKED, 
                                              LVIS_STATEIMAGEMASK);
                        ListView_SetColumnWidth(psed->hwndSchedList, 0, LVSCW_AUTOSIZE);
                        bAdded = TRUE;
                    }
                }
            }
        }
    }

    if (!bAdded)
    {
        SAFEDELETE(psld);
    }
    
    return TRUE;
}

BOOL Sched_FillScheduleList(HWND hdlg, POOEBuf pBuf)
{
    SCHED_ENUM_DATA sed;

    sed.hwndSchedList = GetDlgItem(hdlg, IDC_SCHEDULE_LIST);
    sed.pBuf = pBuf;

    sed.customSchedule = GUID_NULL;
    
    ISubscriptionItem *psi;
    if (SUCCEEDED(SubscriptionItemFromCookie(FALSE, &pBuf->m_Cookie, &psi)))
    {
        SUBSCRIPTIONITEMINFO sii;

        sii.cbSize = sizeof(SUBSCRIPTIONITEMINFO);

        if (SUCCEEDED(psi->GetSubscriptionItemInfo(&sii)))
        {
            sed.customSchedule = sii.ScheduleGroup;
        }
        psi->Release();
    }
    sed.bHasAtLeastOneSchedule = FALSE;

    EnumSchedules(Sched_EnumCallback, (LPARAM)&sed);

    return sed.bHasAtLeastOneSchedule;
}

BOOL Sched_NewSchedule(HWND hdlg)
{
    NEWSCHED_DATA nsd;

    INT_PTR nResult = DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_NEW_SCHEDULE),
                                 hdlg, NewScheduleDlgProc, (LPARAM)&nsd);

    if (IDOK == nResult)
    {
        SCHED_LIST_DATA *psld;
        LV_FINDINFO lvfi = { 0 };
        HWND hwndSchedList = GetDlgItem(hdlg, IDC_SCHEDULE_LIST);

        lvfi.flags = LVFI_STRING;
        lvfi.psz = nsd.szSchedName;

        int index = ListView_FindItem(hwndSchedList, -1, &lvfi);

        if (index == -1)
        {

            psld = new SCHED_LIST_DATA;           
            
            if (NULL != psld)
            {
                psld->SchedCookie = nsd.SchedCookie;
                psld->bChecked = TRUE;
                psld->bStartChecked = FALSE;

                LV_ITEM lvItem = { 0 };

                lvItem.mask = LVIF_TEXT | LVIF_PARAM;
                lvItem.iItem = 0;
                lvItem.lParam = (LPARAM)psld;
                lvItem.pszText = nsd.szSchedName;

                index = ListView_InsertItem(hwndSchedList, &lvItem);
            }
        }
        else
        {
            psld = SchedList_GetData(hwndSchedList, index);
            if (NULL != psld)
            {
                psld->bChecked = TRUE;
            }
        }

        if (index != -1)
        {
            ListView_SetItemState(hwndSchedList, index, COMP_CHECKED, LVIS_STATEIMAGEMASK);
            ListView_SetColumnWidth(hwndSchedList, 0, LVSCW_AUTOSIZE);
            CheckRadioButton(hdlg, IDC_MANUAL_SYNC, IDC_SCHEDULED_SYNC, 
                             IDC_SCHEDULED_SYNC);
            SchedList_Select(hwndSchedList, index);
            Sched_EnableScheduleButtons(hdlg);
        }

    }

    SendMessage(hdlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hdlg, IDC_SCHEDULE_LIST), TRUE);

    return nResult == IDOK;
}

BOOL Sched_EditSchedule(HWND hdlg, POOEBuf pBuf)
{
    HRESULT hr;
    ISyncScheduleMgr *pSyncScheduleMgr;

    ASSERT(NULL != pBuf);

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {

        hr = CoCreateInstance(CLSID_SyncMgr, NULL, CLSCTX_ALL, 
                              IID_ISyncScheduleMgr, (void **)&pSyncScheduleMgr);

        if (SUCCEEDED(hr))
        {   
            HWND hwndSchedList = GetDlgItem(hdlg, IDC_SCHEDULE_LIST);
            int index = ListView_GetNextItem(hwndSchedList, -1, LVNI_ALL | LVNI_SELECTED);
            SCHED_LIST_DATA *psld = SchedList_GetData(hwndSchedList, index);
            ISyncSchedule *pSyncSchedule;

            hr = pSyncScheduleMgr->OpenSchedule(&psld->SchedCookie, 0, &pSyncSchedule);

            if (SUCCEEDED(hr))
            {
                if (psld->bChecked)
                {
                    hr = pSyncSchedule->SetItemCheck(CLSID_WebCheckOfflineSync,
                                                     &pBuf->m_Cookie,
                                                     SYNCMGRITEMSTATE_CHECKED);
                }
                
                hr = pSyncSchedule->EditSyncSchedule(hdlg, 0);

                if (S_OK == hr)
                {
                    psld->bChecked = IsCookieOnSchedule(pSyncSchedule, &pBuf->m_Cookie);

                    if (psld->bChecked)
                    {
                        CheckRadioButton(hdlg, IDC_MANUAL_SYNC, IDC_SCHEDULED_SYNC, 
                                         IDC_SCHEDULED_SYNC);
                    }

                    ListView_SetItemState(hwndSchedList, index, 
                                          psld->bChecked ? COMP_CHECKED : COMP_UNCHECKED, 
                                          LVIS_STATEIMAGEMASK);
                }

                WCHAR wszScheduleName[MAX_PATH];
                DWORD cchScheduleName = ARRAYSIZE(wszScheduleName);

                if (SUCCEEDED(pSyncSchedule->GetScheduleName(&cchScheduleName, wszScheduleName)))
                {

                #ifdef UNICODE
                    SchedList_SetName(hwndSchedList, index, wszScheduleName);
                #else
                    char szScheduleName[MAX_PATH];

                    SHUnicodeToAnsi(wszScheduleName, szScheduleName, ARRAYSIZE(szScheduleName));
                    SchedList_SetName(hwndSchedList, index, wszScheduleName);
                #endif

                    ListView_SetColumnWidth(hwndSchedList, 0, LVSCW_AUTOSIZE);
                }


                SendMessage(hdlg, WM_NEXTDLGCTL, (WPARAM)hwndSchedList, TRUE);

                pSyncSchedule->Release();
            }
            pSyncScheduleMgr->Release();
        }
        CoUninitialize();
    }

    //  This is not undoable by hitting cancel so don't say we changed.
    return FALSE;
}

BOOL Sched_RemoveSchedule(HWND hdlg)
{
    HWND hwndSchedList = GetDlgItem(hdlg, IDC_SCHEDULE_LIST);
    int index = ListView_GetNextItem(hwndSchedList, -1, LVNI_ALL | LVNI_SELECTED);

    if (index >= 0)
    {
        SCHED_LIST_DATA *psld = SchedList_GetData(GetDlgItem(hdlg, IDC_SCHEDULE_LIST), index);

        if (NULL != psld)
        {
            TCHAR szSchedName[MAX_PATH];

            SchedList_GetName(hwndSchedList, index, szSchedName, ARRAYSIZE(szSchedName));

            if (WCMessageBox(hdlg, IDS_CONFIRM_SCHEDULE_DELETE,
                            IDS_SCHEDULE_DELETE_CAPTION, MB_YESNO | MB_ICONQUESTION,
                            szSchedName) == IDYES)
            {
                HRESULT hr = CoInitialize(NULL);
                if (SUCCEEDED(hr))
                {
                    ISyncScheduleMgr *pSyncScheduleMgr;
                    hr = CoCreateInstance(CLSID_SyncMgr, NULL, CLSCTX_ALL, 
                                          IID_ISyncScheduleMgr, (void **)&pSyncScheduleMgr);

                    if (SUCCEEDED(hr))
                    {                
                        hr = pSyncScheduleMgr->RemoveSchedule(&psld->SchedCookie);

                        ASSERT(SUCCEEDED(hr));

                        if (SUCCEEDED(hr))
                        {
                            SchedList_DeleteData(hwndSchedList, -1, TRUE);
                            SchedList_Select(hwndSchedList, 0);
                        }

                        pSyncScheduleMgr->Release();
                    }
                    CoUninitialize();
                }
            }

            SendMessage(hdlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hdlg, IDC_SCHEDULE_LIST), TRUE);
        }
    }
    //  This is not undoable by hitting cancel so don't say we changed.
    return FALSE;
}

BOOL Sched_OnCommand(HWND hdlg, WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
    BOOL bHandled = TRUE;
    BOOL bChanged = FALSE;
    POOEBuf pBuf = GetBuf(hdlg);

    if (NULL != pBuf)
    {

        switch (wID)
        {
            case IDC_MANUAL_SYNC:
            case IDC_SCHEDULED_SYNC:
//            case IDC_IDLE_ONLY:
                if (wNotifyCode == BN_CLICKED)
                {
                    bChanged = TRUE;
                }
                break;
            
            case IDC_SCHEDULE_NEW:
                if (wNotifyCode == BN_CLICKED)
                {
                    bChanged = Sched_NewSchedule(hdlg);
                }
                break;

            case IDC_SCHEDULE_EDIT:
                if (wNotifyCode == BN_CLICKED)
                {
                    bChanged = Sched_EditSchedule(hdlg, pBuf);
                }
                break;

            case IDC_SCHEDULE_REMOVE:
                if (wNotifyCode == BN_CLICKED)
                {
                    bChanged = Sched_RemoveSchedule(hdlg);
                }
                break;

            default:
                bHandled = FALSE;
                break;
        }
    }

    if (bChanged)
    {
        PropSheet_Changed(GetParent(hdlg), hdlg);        
    }

    return bHandled;
}

BOOL Sched_Validate(HWND hdlg, POOEBuf pBuf)
{
/*
    if (IsDlgButtonChecked(hdlg, IDC_IDLE_ONLY))
    {
        pBuf->grfTaskTrigger |= TASK_FLAG_START_ONLY_IF_IDLE;
    }
    else
    {
        pBuf->grfTaskTrigger &= ~TASK_FLAG_START_ONLY_IF_IDLE;
    }
*/

    if (IsDlgButtonChecked(hdlg, IDC_SCHEDULED_SYNC))
    {
        HWND hwndSchedList = GetDlgItem(hdlg, IDC_SCHEDULE_LIST);
        int count = ListView_GetItemCount(hwndSchedList);

        BOOL bHaveASchedule = FALSE;
        
        for (int i = 0; i < count; i++)
        {
            SCHED_LIST_DATA *psld = SchedList_GetData(hwndSchedList, i);

            if (psld->bChecked)
            {
                bHaveASchedule = TRUE;
                break;
            }
        }

        if (!bHaveASchedule)
        {
            CheckRadioButton(hdlg, IDC_MANUAL_SYNC, IDC_SCHEDULED_SYNC, 
                             IDC_MANUAL_SYNC);
        }
    }

    return TRUE;
}

BOOL Sched_ApplyProps(HWND hdlg, POOEBuf pBuf)
{
    BOOL bResult;
    HRESULT hr;

    pBuf->dwFlags = PROP_WEBCRAWL_ALL;

    bResult = SUCCEEDED(SaveBufferChange(pBuf, TRUE));

    if (bResult)
    {
        
        if (IsDlgButtonChecked(hdlg, IDC_MANUAL_SYNC))
        {
            SchedList_UncheckAll(GetDlgItem(hdlg, IDC_SCHEDULE_LIST));
            hr = RemoveItemFromAllSchedules(&pBuf->m_Cookie);

            ASSERT(SUCCEEDED(hr));
        }
        else
        {
            HWND hwndSchedList = GetDlgItem(hdlg, IDC_SCHEDULE_LIST);
            int count = ListView_GetItemCount(hwndSchedList);
            
            for (int i = 0; i < count; i++)
            {
                SCHED_LIST_DATA *psld = SchedList_GetData(hwndSchedList, i);

                ASSERT(NULL != psld);

                if (NULL != psld)
                {
                    if (psld->bChecked != psld->bStartChecked)
                    {
                        if (psld->bChecked)
                        {
                            ISubscriptionItem *psi;

                            if (SUCCEEDED(SubscriptionItemFromCookie(FALSE, 
                                                                     &pBuf->m_Cookie,
                                                                     &psi)))
                            {
                                SYNC_HANDLER_ITEM_INFO shii;

                                shii.handlerID = CLSID_WebCheckOfflineSync;
                                shii.itemID = pBuf->m_Cookie;
                                shii.hIcon = NULL;
                                MyStrToOleStrN(shii.wszItemName, 
                                               ARRAYSIZE(shii.wszItemName),
                                               pBuf->m_Name);
                                shii.dwCheckState = SYNCMGRITEMSTATE_CHECKED;

                                AddScheduledItem(&shii, &psld->SchedCookie);
                            }
                        }
                        else
                        {
                            RemoveScheduledItem(&pBuf->m_Cookie, &psld->SchedCookie);
                        }
                        psld->bStartChecked = psld->bChecked;
                    }
                }
            }
        }
    }

    return bResult;
}

BOOL Sched_OnNotify(HWND hdlg, int idCtrl, LPNMHDR pnmh)
{
    BOOL bHandled = TRUE;
    POOEBuf pBuf = GetBuf(hdlg);
    ASSERT(pBuf);
    
    switch (pnmh->code)
    {
        case PSN_KILLACTIVE:
            if (!Sched_Validate(hdlg, pBuf))
            {
                SetWindowLongPtr(hdlg, DWLP_MSGRESULT, TRUE);
            }
            break;

        case PSN_APPLY:
            Sched_ApplyProps(hdlg, pBuf);
            break;

        case NM_DBLCLK:
            if (IDC_SCHEDULE_LIST == pnmh->idFrom)
            {
                Sched_EditSchedule(hdlg, pBuf);
            }
            break;
            
        case LVN_ITEMCHANGED:
        {
            NM_LISTVIEW *pnmlv = (NM_LISTVIEW *)pnmh;
            if ((pnmlv->iItem != -1) && 
                (pnmlv->uChanged & LVIF_STATE) &&
                ((pnmlv->uNewState ^ pnmlv->uOldState) & COMP_CHECKED))

            {
                SCHED_LIST_DATA *psld = SchedList_GetData(pnmh->hwndFrom, pnmlv->iItem);

                if (NULL != psld)
                {
                    psld->bChecked = (pnmlv->uNewState & COMP_CHECKED) ? TRUE : FALSE;

                    if (psld->bChecked)
                    {
                        CheckRadioButton(hdlg, IDC_MANUAL_SYNC, IDC_SCHEDULED_SYNC, 
                                         IDC_SCHEDULED_SYNC);
                    }
                    PropSheet_Changed(GetParent(hdlg), hdlg);
                }
            }

            if ((pnmlv->uChanged & LVIF_STATE) &&
                ((pnmlv->uNewState ^ pnmlv->uOldState) & LVIS_SELECTED))
            {
                Sched_EnableScheduleButtons(hdlg);
            }
            break;
        }

        default:
            bHandled = FALSE;
            break;
    }

    return bHandled;
}

void Sched_OnDestroy(HWND hdlg)
{
    HWND hwndSchedList = GetDlgItem(hdlg, IDC_SCHEDULE_LIST);

    SchedList_DeleteAllData(hwndSchedList);
}

BOOL Sched_OnInitDialog(HWND hdlg, HWND hwndFocus, LPARAM lParam)
{
    ASSERT(NULL != ((LPPROPSHEETPAGE)lParam));
    ASSERT(NULL != ((LPPROPSHEETPAGE)lParam)->lParam);

    SetWindowLongPtr(hdlg, DWLP_USER, ((LPPROPSHEETPAGE)lParam)->lParam);

    //  Now read in values and populate the dialog
    POOEBuf pBuf = (POOEBuf)((LPPROPSHEETPAGE)lParam)->lParam;

/*    CheckDlgButton(hdlg, IDC_IDLE_ONLY, 
                   pBuf->grfTaskTrigger & TASK_FLAG_START_ONLY_IF_IDLE ?
                   1 : 0);
*/

    HWND hwndSchedList = GetDlgItem(hdlg, IDC_SCHEDULE_LIST);
    ListView_SetExtendedListViewStyle(hwndSchedList, LVS_EX_CHECKBOXES);

    LV_COLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.iSubItem = 0;
    ListView_InsertColumn(hwndSchedList, 0, &lvc);

    //  Now handle restrictions
    SetPropSheetFlags(pBuf, SHRestricted2W(REST_NoScheduledUpdates, NULL, 0), PSF_NO_SCHEDULED_UPDATES);
    SetPropSheetFlags(pBuf, SHRestricted2W(REST_NoEditingScheduleGroups, NULL, 0), PSF_NO_EDITING_SCHEDULES);

    BOOL bHasSchedules = Sched_FillScheduleList(hdlg, pBuf);
    
    if (pBuf->m_dwPropSheetFlags & PSF_NO_SCHEDULED_UPDATES)
    {
        EnableWindow(GetDlgItem(hdlg, IDC_SCHEDULE_LIST), FALSE);
        EnableWindow(GetDlgItem(hdlg, IDC_SCHEDULED_SYNC), FALSE);
        CheckRadioButton(hdlg, IDC_MANUAL_SYNC, IDC_SCHEDULED_SYNC, IDC_MANUAL_SYNC);
    }
    else
    {
        CheckRadioButton(hdlg, IDC_MANUAL_SYNC, IDC_SCHEDULED_SYNC, 
                         bHasSchedules ? IDC_SCHEDULED_SYNC : IDC_MANUAL_SYNC);
    }

    SchedList_Select(hwndSchedList, 0);

    //  Finally, do the enable/disable controls thing...
    Sched_EnableScheduleButtons(hdlg);

    return TRUE;
}

INT_PTR CALLBACK SchedulePropDlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL bHandled = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
            bHandled = Sched_OnInitDialog(hdlg, (HWND)wParam, lParam);
            break;

        case WM_COMMAND:
            bHandled = Sched_OnCommand(hdlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
            break;

        case WM_NOTIFY:
            bHandled = Sched_OnNotify(hdlg, (int)wParam, (LPNMHDR)lParam);
            break;

        case WM_DESTROY:
            Sched_OnDestroy(hdlg);
            //  return 0
            break;

        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
                    HELP_WM_HELP, (DWORD_PTR)aHelpIDs);
            bHandled = TRUE;
            break;

        case WM_CONTEXTMENU:
            SHWinHelpOnDemandWrap((HWND)wParam, c_szHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)aHelpIDs);
            bHandled = TRUE;
            break;
    }
    
    return bHandled;
}

/********************************************************************************
    Download property sheet code
*********************************************************************************/

void Download_EnableFollowLinks(HWND hdlg)
{
    BOOL bTranslate;
    int i = GetDlgItemInt(hdlg, IDC_LEVELS, &bTranslate, FALSE);

    EnableWindow(GetDlgItem(hdlg, IDC_FOLLOW_LINKS), (bTranslate && i));
}

void Download_EnableLimitSpaceControls(HWND hdlg)
{
    static const int IDs[] = { IDC_LIMIT_SPACE_EDIT, IDC_LIMIT_SPACE_SPIN, IDC_LIMIT_SPACE_TEXT };
    EnableControls(hdlg, IDs, ARRAYSIZE(IDs), IsDlgButtonChecked(hdlg, IDC_LIMIT_SPACE_CHECK));
}

void Download_EnableEmailControls(HWND hdlg)
{
    static const int IDs[] = { IDC_EMAIL_ADDRESS_TEXT, IDC_EMAIL_ADDRESS, 
                               IDC_EMAIL_SERVER_TEXT, IDC_EMAIL_SERVER };
    EnableControls(hdlg, IDs, ARRAYSIZE(IDs), IsDlgButtonChecked(hdlg, IDC_EMAIL_NOTIFY));
}

BOOL Download_OnInitDialog(HWND hdlg, HWND hwndFocus, LPARAM lParam)
{
    POOEBuf pBuf;

    ASSERT(NULL != ((LPPROPSHEETPAGE)lParam));
    ASSERT(NULL != ((LPPROPSHEETPAGE)lParam)->lParam);

    SetWindowLongPtr(hdlg, DWLP_USER, ((LPPROPSHEETPAGE)lParam)->lParam);

    //  First do basic control setup
    HWND hwndLimitSpin = GetDlgItem(hdlg, IDC_LIMIT_SPACE_SPIN);
    SendMessage(hwndLimitSpin, UDM_SETRANGE, 0, 
                MAKELONG(MAX_DOWNLOAD_K, MIN_DOWNLOAD_K));

    UDACCEL ua[] = { {0, 1}, {1, 10}, {2, 100}, {3, 1000} };
    SendMessage(hwndLimitSpin, UDM_SETACCEL, ARRAYSIZE(ua), (LPARAM)ua);

    Edit_LimitText(GetDlgItem(hdlg, IDC_LIMIT_SPACE_EDIT), 5);

    //  Now read in values and populate the dialog
    pBuf = (POOEBuf)((LPPROPSHEETPAGE)lParam)->lParam;

    SUBSCRIPTIONTYPE subType = GetItemCategory(pBuf);
    switch (subType)
    {
        case SUBSTYPE_CHANNEL:
        case SUBSTYPE_DESKTOPCHANNEL:
            CheckRadioButton(hdlg, IDC_DOWNLOAD_ALL, IDC_DOWNLOAD_MIN,
                             pBuf->fChannelFlags & CHANNEL_AGENT_PRECACHE_ALL ? 
                             IDC_DOWNLOAD_ALL : IDC_DOWNLOAD_MIN);
            break;

        case SUBSTYPE_URL:
        case SUBSTYPE_DESKTOPURL:
        case SUBSTYPE_EXTERNAL:
        {
            HWND hwndLevelsSpin = GetDlgItem(hdlg, IDC_LEVELS_SPIN);
            SendMessage(hwndLevelsSpin, UDM_SETRANGE, 0, MAKELONG(MAX_WEBCRAWL_LEVELS, 0));
            SendMessage(hwndLevelsSpin, UDM_SETPOS, 0, pBuf->m_RecurseLevels);
            CheckDlgButton(hdlg, IDC_FOLLOW_LINKS, 
                           (pBuf->m_RecurseFlags & WEBCRAWL_LINKS_ELSEWHERE) || (0 == pBuf->m_RecurseLevels)
                           ? 1 : 0);
            Download_EnableFollowLinks(hdlg);
            break;
        }
    }

    SendMessage(hwndLimitSpin, UDM_SETPOS, 0, pBuf->m_SizeLimit ? pBuf->m_SizeLimit : DEF_DOWNLOAD_K);
    CheckDlgButton(hdlg, IDC_LIMIT_SPACE_CHECK, pBuf->m_SizeLimit ? 1 : 0);

    CheckDlgButton(hdlg, IDC_EMAIL_NOTIFY, pBuf->bMail ? 1 : 0);

    TCHAR szText[MAX_PATH];

    ReadDefaultEmail(szText, ARRAYSIZE(szText));
    Edit_LimitText(GetDlgItem(hdlg, IDC_EMAIL_ADDRESS), MAX_PATH - 1);
    SetDlgItemText(hdlg, IDC_EMAIL_ADDRESS, szText);

    ReadDefaultSMTPServer(szText, ARRAYSIZE(szText));
    Edit_LimitText(GetDlgItem(hdlg, IDC_EMAIL_SERVER), MAX_PATH - 1);
    SetDlgItemText(hdlg, IDC_EMAIL_SERVER, szText);

    //  Now handle restrictions
    if (SHRestricted2W(REST_NoSubscriptionPasswords, NULL, 0))
    {
        EnableWindow(GetDlgItem(hdlg, IDC_LOGIN), FALSE);
    }

    //  Finally, do the enable/disable controls thing...
    Download_EnableLimitSpaceControls(hdlg);
    Download_EnableEmailControls(hdlg);

    return TRUE;
}

BOOL Download_OnCommand(HWND hdlg, WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
    BOOL bHandled = TRUE;
    BOOL bChanged = FALSE;
    POOEBuf pBuf = GetBuf(hdlg);

    if (NULL != pBuf)
    {
        switch (wID)
        {
            case IDC_LIMIT_SPACE_EDIT:
                if (wNotifyCode == EN_CHANGE)
                {
                    if (pBuf->m_SizeLimit != 
                        LOWORD(SendDlgItemMessage(hdlg, IDC_LIMIT_SPACE_SPIN, UDM_GETPOS, 0, 0)))
                    {
                        bChanged = TRUE;
                    }
                }
                else if (wNotifyCode == EN_KILLFOCUS)
                {
                    KeepSpinNumberInRange(hdlg, IDC_LIMIT_SPACE_EDIT, 
                                          IDC_LIMIT_SPACE_SPIN, 
                                          MIN_DOWNLOAD_K, MAX_DOWNLOAD_K);
                }
                break;
            
            case IDC_LEVELS:
                if (wNotifyCode == EN_UPDATE)
                {
                    int levels = KeepSpinNumberInRange(hdlg, IDC_LEVELS, 
                                    IDC_LEVELS_SPIN, 0, MAX_WEBCRAWL_LEVELS);

                    if (pBuf->m_RecurseLevels != levels)
                    {
                        bChanged = TRUE;
                    }
                    Download_EnableFollowLinks(hdlg);
                }
                break;

            case IDC_LIMIT_SPACE_CHECK:
                if (wNotifyCode == BN_CLICKED)
                {
                    Download_EnableLimitSpaceControls(hdlg);
                    bChanged = TRUE;
                }
                break;
                
            case IDC_FOLLOW_LINKS:
                if (wNotifyCode == BN_CLICKED)
                {
                    bChanged = TRUE;
                }
                break;

            case IDC_EMAIL_NOTIFY:
                if (wNotifyCode == BN_CLICKED)
                {
                    Download_EnableEmailControls(hdlg);
                    bChanged = TRUE;
                }
                break;
                
            case IDC_EMAIL_ADDRESS:
            case IDC_EMAIL_SERVER:
                if (wNotifyCode == EN_CHANGE)
                {
                    bChanged = TRUE;
                }
                break;

            case IDC_LOGIN:
                if ((wNotifyCode == BN_CLICKED) &&
                    (DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_SUBSPROPS_LOGIN),
                                    hdlg, LoginOptionDlgProc, (LPARAM)pBuf) == IDOK))
                {
                    bChanged = TRUE;
                }
                break;

            case IDC_ADVANCED:
                if ((wNotifyCode == BN_CLICKED) &&
                    (DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_SUBSPROPS_ADVANCED),
                                    hdlg, AdvancedDownloadDlgProc, (LPARAM)pBuf) == IDOK))
                {
                    bChanged = TRUE;
                }
                break;

            default:
                bHandled = FALSE;
                break;
        }
    }

    if (bChanged)
    {
        PropSheet_Changed(GetParent(hdlg), hdlg);        
    }

    return bHandled;
}

BOOL Download_Validate(HWND hdlg, POOEBuf pBuf)
{
    pBuf->bMail = IsDlgButtonChecked(hdlg, IDC_EMAIL_NOTIFY);

    if (pBuf->bMail)
    {
        TCHAR szEmail[MAX_PATH];
        TCHAR szServer[MAX_PATH];

        GetDlgItemText(hdlg, IDC_EMAIL_ADDRESS, szEmail, ARRAYSIZE(szEmail));
        GetDlgItemText(hdlg, IDC_EMAIL_SERVER, szServer, ARRAYSIZE(szServer));

        if (!szEmail[0] || !szServer[0])
        {
            SGMessageBox(hdlg, IDS_EMAIL_INCOMPLETE, MB_ICONWARNING);
            return FALSE;
        }
    }

    SUBSCRIPTIONTYPE subType = GetItemCategory(pBuf);
    switch (subType)
    {
        case SUBSTYPE_CHANNEL:
        case SUBSTYPE_DESKTOPCHANNEL:
            pBuf->fChannelFlags &= ~(CHANNEL_AGENT_PRECACHE_SOME | CHANNEL_AGENT_PRECACHE_ALL);
            if (IsDlgButtonChecked(hdlg, IDC_DOWNLOAD_ALL))
            {
                pBuf->fChannelFlags |= CHANNEL_AGENT_PRECACHE_ALL;
            }
            else
            {
                pBuf->fChannelFlags |= CHANNEL_AGENT_PRECACHE_SOME;
            }
            break;

        case SUBSTYPE_URL:
        case SUBSTYPE_DESKTOPURL:
        case SUBSTYPE_EXTERNAL:
        {
            if (IsDlgButtonChecked(hdlg, IDC_FOLLOW_LINKS))
            {
                pBuf->m_RecurseFlags |= WEBCRAWL_LINKS_ELSEWHERE;
            }
            else
            {
                pBuf->m_RecurseFlags &= ~WEBCRAWL_LINKS_ELSEWHERE;
            }

            pBuf->m_RecurseLevels = LOWORD(SendDlgItemMessage(hdlg, 
                                           IDC_LEVELS_SPIN, UDM_GETPOS, 0, 0));

            ASSERT((pBuf->m_RecurseLevels >= 0) &&
                   (pBuf->m_RecurseLevels <= MAX_WEBCRAWL_LEVELS));
        }
    }

    pBuf->m_SizeLimit = IsDlgButtonChecked(hdlg, IDC_LIMIT_SPACE_CHECK) ? 
                        (LONG)SendDlgItemMessage(hdlg, IDC_LIMIT_SPACE_SPIN,
                                           UDM_GETPOS, 0, 0) : 
                        0;
    ASSERT((0 == pBuf->m_SizeLimit) ||
           ((pBuf->m_SizeLimit >= MIN_DOWNLOAD_K) && 
            (pBuf->m_SizeLimit <= MAX_DOWNLOAD_K)));

    return TRUE;
}

BOOL Download_ApplyProps(HWND hdlg, POOEBuf pBuf)
{
    pBuf->dwFlags = PROP_WEBCRAWL_ALL;

    if (IsDlgButtonChecked(hdlg, IDC_EMAIL_NOTIFY))
    {
        TCHAR szText[MAX_PATH];

        GetDlgItemText(hdlg, IDC_EMAIL_ADDRESS, szText, ARRAYSIZE(szText));
        WriteDefaultEmail(szText);
        GetDlgItemText(hdlg, IDC_EMAIL_SERVER, szText, ARRAYSIZE(szText));
        WriteDefaultSMTPServer(szText);
    }

    if (pBuf->bChannel)
    {
        pBuf->dwFlags |= PROP_WEBCRAWL_CHANNELFLAGS;
    }

    return SUCCEEDED(SaveBufferChange(pBuf, TRUE));
}

BOOL Download_OnNotify(HWND hdlg, int idCtrl, LPNMHDR pnmh)
{
    BOOL bHandled = TRUE;
    POOEBuf pBuf = GetBuf(hdlg);
    ASSERT(pBuf);
    
    switch (pnmh->code)
    {
        case PSN_KILLACTIVE:
            if (!Download_Validate(hdlg, pBuf))
            {
                SetWindowLongPtr(hdlg, DWLP_MSGRESULT, TRUE);
            }
            break;

        case PSN_APPLY:
            Download_ApplyProps(hdlg, pBuf);
            break;

        default:
            bHandled = FALSE;
            break;
    }

    return bHandled;
}

INT_PTR CALLBACK DownloadPropDlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL bHandled = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
            bHandled = Download_OnInitDialog(hdlg, (HWND)wParam, lParam);
            break;

        case WM_COMMAND:
            bHandled = Download_OnCommand(hdlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
            break;

        case WM_NOTIFY:
            bHandled = Download_OnNotify(hdlg, (int)wParam, (LPNMHDR)lParam);
            break;

        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
                    HELP_WM_HELP, (DWORD_PTR)aHelpIDs);
            bHandled = TRUE;
            break;

        case WM_CONTEXTMENU:
            SHWinHelpOnDemandWrap((HWND)wParam, c_szHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)aHelpIDs);
            bHandled = TRUE;
            break;
    }
    
    return bHandled;
}

/********************************************************************************
    New schedule popup code
*********************************************************************************/

BOOL NewSched_OnCommand(HWND hdlg, WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
    BOOL bHandled = FALSE;
    
    switch (wID)
    {
        case IDC_SCHEDULE_DAYS:
            if (wNotifyCode == EN_UPDATE)
            {
                KeepSpinNumberInRange(hdlg, IDC_SCHEDULE_DAYS, 
                                      IDC_SCHEDULE_DAYS_SPIN, 1, 99);
            }
#ifdef NEWSCHED_AUTONAME
            else if (wNotifyCode == EN_CHANGE)
            {
                NewSched_AutoNameHelper(hdlg);
            }
#endif
            bHandled = TRUE;
            break;

        case IDOK:
        {
            TASK_TRIGGER trig;
            NEWSCHED_DATA *pnsd = (NEWSCHED_DATA*) GetWindowLongPtr(hdlg, DWLP_USER);

            ASSERT(NULL != pnsd);
            
            if (NewSched_ResolveNameConflictHelper(hdlg, &trig, &pnsd->SchedCookie))
            {
                NewSched_CreateScheduleHelper(hdlg, &trig, &pnsd->SchedCookie);

                GetDlgItemText(hdlg, IDC_SCHEDULE_NAME, 
                               pnsd->szSchedName, ARRAYSIZE(pnsd->szSchedName));
                EndDialog(hdlg, IDOK);
            }
            break;
        }


        case IDCANCEL:
            EndDialog(hdlg, IDCANCEL);
            break;

        default:
            break;
    }

    return bHandled;
}

#ifdef NEWSCHED_AUTONAME
BOOL NewSched_OnNotify(HWND hdlg, int idCtrl, LPNMHDR pnmh)
{
    BOOL bHandled = TRUE;

    switch (pnmh->code)
    {
        case DTN_DATETIMECHANGE:
            NewSched_AutoNameHelper(hdlg);
            break;
            
        default:
            bHandled = FALSE;
            break;
    }

    return bHandled;
}
#endif

INT_PTR CALLBACK NewScheduleDlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL bHandled = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            SetWindowLongPtr(hdlg, DWLP_USER, lParam);
            NewSched_OnInitDialogHelper(hdlg);
            bHandled = TRUE;
            break;
        }

        case WM_COMMAND:
            bHandled = NewSched_OnCommand(hdlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
            break;

#ifdef NEWSCHED_AUTONAME
        case WM_NOTIFY:
            bHandled = NewSched_OnNotify(hdlg, wParam, (LPNMHDR)lParam);
            break;            
#endif

        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
                    HELP_WM_HELP, (DWORD_PTR)aHelpIDs);
            bHandled = TRUE;
            break;

        case WM_CONTEXTMENU:
            SHWinHelpOnDemandWrap((HWND)wParam, c_szHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)aHelpIDs);
            bHandled = TRUE;
            break;
    }

    return bHandled;
}


/********************************************************************************
    Advanced properties code
*********************************************************************************/

BOOL Advanced_OnInitDialog(HWND hdlg, HWND hwndFocus, LPARAM lParam)
{
    POOEBuf pBuf;

    ASSERT(NULL != (POOEBuf)lParam);

    SetWindowLongPtr(hdlg, DWLP_USER, lParam);

    pBuf = (POOEBuf)lParam;

    UINT flags = pBuf->m_RecurseFlags;
    CheckDlgButton(hdlg, IDC_DOWNLOAD_IMAGES, flags & WEBCRAWL_GET_IMAGES);
    CheckDlgButton(hdlg, IDC_DOWNLOAD_APPLETS, flags & WEBCRAWL_GET_CONTROLS);
    CheckDlgButton(hdlg, IDC_DOWNLOAD_MEDIA, 
                   flags & (WEBCRAWL_GET_BGSOUNDS | WEBCRAWL_GET_VIDEOS));

    CheckDlgButton(hdlg, IDC_DOWNLOAD_ONLY_HTML_LINKS, flags & WEBCRAWL_ONLY_LINKS_TO_HTML);

    return TRUE;
}

void Advanced_SetFlag(HWND hdlg, POOEBuf pBuf, int ID, LONG flags)
{
    if (IsDlgButtonChecked(hdlg, ID))
    {
        pBuf->m_RecurseFlags |= flags;
    }
    else
    {
        pBuf->m_RecurseFlags &= ~flags;
    }
}

BOOL Advanced_OnCommand(HWND hdlg, WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
    BOOL bHandled = TRUE;
    POOEBuf pBuf = GetBuf(hdlg);
    ASSERT(pBuf);

    switch (wID)
    {
        case IDOK:
            Advanced_SetFlag(hdlg, pBuf, IDC_DOWNLOAD_IMAGES, WEBCRAWL_GET_IMAGES);
            Advanced_SetFlag(hdlg, pBuf, IDC_DOWNLOAD_APPLETS, WEBCRAWL_GET_CONTROLS);
            Advanced_SetFlag(hdlg, pBuf, IDC_DOWNLOAD_MEDIA, 
                             WEBCRAWL_GET_BGSOUNDS | WEBCRAWL_GET_VIDEOS);
            Advanced_SetFlag(hdlg, pBuf, IDC_DOWNLOAD_ONLY_HTML_LINKS, WEBCRAWL_ONLY_LINKS_TO_HTML);
            EndDialog(hdlg, IDOK);
            break;

        case IDCANCEL:
            EndDialog(hdlg, IDCANCEL);
            break;

        case IDC_DOWNLOAD_IMAGES:
        case IDC_DOWNLOAD_APPLETS:
        case IDC_DOWNLOAD_MEDIA:
        case IDC_DOWNLOAD_ONLY_HTML_LINKS:
            break;

        default:
            bHandled = FALSE;
            break;
    }

    return bHandled;
}

INT_PTR CALLBACK AdvancedDownloadDlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL bHandled = FALSE;

    switch (message)
    {

        case WM_INITDIALOG:
            bHandled = Advanced_OnInitDialog(hdlg, (HWND)wParam, lParam);
            break;

        case WM_COMMAND:
            bHandled = Advanced_OnCommand(hdlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
            break;
            
        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
                    HELP_WM_HELP, (DWORD_PTR)aHelpIDs);
            bHandled = TRUE;
            break;

        case WM_CONTEXTMENU:
            SHWinHelpOnDemandWrap((HWND)wParam, c_szHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)aHelpIDs);
            bHandled = TRUE;
            break;
    }
    
    return bHandled;
}

/********************************************************************************
    Login properties code
*********************************************************************************/

BOOL Login_OnInitDialog(HWND hdlg, HWND hwndFocus, LPARAM lParam)
{
    POOEBuf pBuf;

    ASSERT(NULL != (POOEBuf)lParam);

    SetWindowLongPtr(hdlg, DWLP_USER, lParam);

    pBuf = (POOEBuf)lParam;

    Edit_LimitText(GetDlgItem(hdlg, IDC_USERNAME), ARRAYSIZE(pBuf->username) - 1);
    SetDlgItemText(hdlg, IDC_USERNAME, pBuf->username);

    Edit_LimitText(GetDlgItem(hdlg, IDC_PASSWORD), ARRAYSIZE(pBuf->password) - 1);
    SetDlgItemText(hdlg, IDC_PASSWORD, pBuf->password);

    Edit_LimitText(GetDlgItem(hdlg, IDC_PASSWORDCONFIRM), ARRAYSIZE(pBuf->password) - 1);
    SetDlgItemText(hdlg, IDC_PASSWORDCONFIRM, pBuf->password);

    return TRUE;
}

BOOL Login_OnCommand(HWND hdlg, WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
    BOOL bHandled = TRUE;
    POOEBuf pBuf = GetBuf(hdlg);
    ASSERT(pBuf);

    switch (wID)
    {
        case IDOK:
        {
            TCHAR szUsername[ARRAYSIZE(pBuf->username) + 1];
            TCHAR szPassword[ARRAYSIZE(pBuf->password) + 1];
            TCHAR szPasswordConfirm[ARRAYSIZE(pBuf->password) + 1];

            GetDlgItemText(hdlg, IDC_USERNAME, szUsername, ARRAYSIZE(szUsername));
            GetDlgItemText(hdlg, IDC_PASSWORD, szPassword, ARRAYSIZE(szPassword));
            GetDlgItemText(hdlg, IDC_PASSWORDCONFIRM, szPasswordConfirm, ARRAYSIZE(szPasswordConfirm));

            if (!szUsername[0] && (szPassword[0] || szPasswordConfirm[0]))
            {
                SGMessageBox(hdlg, 
                            (pBuf->bChannel ? IDS_NEEDCHANNELUSERNAME : IDS_NEEDUSERNAME), 
                            MB_ICONWARNING);
            }
            else if (szUsername[0] && !szPassword[0])
            {
                SGMessageBox(hdlg, 
                            (pBuf->bChannel ? IDS_NEEDCHANNELPASSWORD : IDS_NEEDPASSWORD), 
                            MB_ICONWARNING);
            }
            else if (StrCmp(szPassword, szPasswordConfirm) != 0)
            {
                SGMessageBox(hdlg, IDS_MISMATCHED_PASSWORDS, MB_ICONWARNING);
            }
            else
            {
                StrCpyN(pBuf->username, szUsername, ARRAYSIZE(pBuf->username));
                StrCpyN(pBuf->password, szPassword, ARRAYSIZE(pBuf->password));
                pBuf->dwFlags |= (PROP_WEBCRAWL_UNAME | PROP_WEBCRAWL_PSWD);
                EndDialog(hdlg, IDOK);
            }
            break;
        }
 
        case IDCANCEL:
            EndDialog(hdlg, IDCANCEL);
            break;

        default:
            bHandled = FALSE;
            break;
    }

    return bHandled;
}

INT_PTR CALLBACK LoginOptionDlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL bHandled = FALSE;

    switch (message)
    {

        case WM_INITDIALOG:
            bHandled = Login_OnInitDialog(hdlg, (HWND)wParam, lParam);
            break;

        case WM_COMMAND:
            bHandled = Login_OnCommand(hdlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
            break;

        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
                    HELP_WM_HELP, (DWORD_PTR)aHelpIDs);
            bHandled = TRUE;
            break;

        case WM_CONTEXTMENU:
            SHWinHelpOnDemandWrap((HWND)wParam, c_szHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)aHelpIDs);
            bHandled = TRUE;
            break;
    }
    
    return bHandled;
}

//
// Read the users default email address and smtp from the Athena Account Mgr
//
// Expects lpszEmailAddress and lpszSMTPServer to pt to char buffers of 
// CCHMAX_EMAIL_ADDRESS and CCHMAX_SERVER_NAME size, resp.
//
BOOL ReadAthenaMailSettings(LPSTR lpszEmailAddress, LPSTR lpszSMTPServer)
{
    //
    // This api gets called from threads that haven't used COM before so wrap
    // in CoInitialize/CoUninitialize
    //
    HRESULT hr = CoInitialize(NULL);
    ASSERT(SUCCEEDED(hr));

    //
    // Create an Internet Mail and News Account Manager
    //
    IImnAccountManager * pAccountManager;
    hr = CoCreateInstance(
        CLSID_ImnAccountManager,
        NULL,                       // no aggregation
        CLSCTX_INPROC_SERVER,       // inproc server implemented in webcheck.dll
        IID_IImnAccountManager,     //
        (void **)&pAccountManager);

    if (SUCCEEDED(hr)) {
        hr = pAccountManager->Init(NULL);

        if (SUCCEEDED(hr)) {
            //
            // Get the default SMTP account
            //
            IImnAccount * pAccount;
            hr = pAccountManager->GetDefaultAccount(ACCT_MAIL, &pAccount);

            if (hr == S_OK) {               
                //
                // Get the SMTP_SERVER name for this account
                //
                if (NULL != lpszSMTPServer)
                {
                    hr = pAccount->GetPropSz(
                        AP_SMTP_SERVER, 
                        lpszSMTPServer,
                        CCHMAX_SERVER_NAME);
                }
                
                //
                // Get the Users email address for this account
                //

                if (NULL != lpszEmailAddress)
                {
                    hr |= pAccount->GetPropSz(
                        AP_SMTP_EMAIL_ADDRESS, 
                        lpszEmailAddress,
                        CCHMAX_EMAIL_ADDRESS);
                }

                pAccount->Release();    // done with IImnAccount
            }
        }
        pAccountManager->Release();     // done with IImnAccountManager
    }

    //
    // This api gets called from threads that haven't used COM before so wrap
    // in CoInitialize/CoUninitialize
    //
    CoUninitialize();

    if (hr == S_OK)
        return TRUE;
    else
        return FALSE;
}

void ReadDefaultEmail(LPTSTR szBuf, UINT cch)
{
    ASSERT(szBuf);

    szBuf[0] = (TCHAR)0;
    if(ReadRegValue(HKEY_CURRENT_USER, c_szRegKey, c_szDefEmail, szBuf, cch * sizeof(TCHAR)))
        return;

    //  TODO: Look for eudora/netscape as well
    CHAR szEmailAddress[CCHMAX_EMAIL_ADDRESS];
    if (ReadAthenaMailSettings(szEmailAddress, NULL))
    {
        SHAnsiToTChar(szEmailAddress, szBuf, cch);
    }
}

void WriteDefaultEmail(LPTSTR szBuf)
{
    ASSERT(szBuf);
    WriteRegValue(HKEY_CURRENT_USER, c_szRegKey, c_szDefEmail, szBuf, (lstrlen(szBuf) + 1) * sizeof(TCHAR), REG_SZ);
}

void ReadDefaultSMTPServer(LPTSTR szBuf, UINT cch)
{
    ASSERT(szBuf);

    szBuf[0] = (TCHAR)0;
    if(ReadRegValue(HKEY_CURRENT_USER, c_szRegKey, c_szDefServer, szBuf, cch * sizeof(TCHAR)))
        return;

    //  TODO: Look for eudora/netscape as well
    CHAR szSMTPServer[CCHMAX_SERVER_NAME];
    if (ReadAthenaMailSettings(NULL, szSMTPServer))
    {
        SHAnsiToTChar(szSMTPServer, szBuf, cch);
    }
}

void WriteDefaultSMTPServer(LPTSTR szBuf)
{
    ASSERT(szBuf);
    WriteRegValue(HKEY_CURRENT_USER, c_szRegKey, c_szDefServer, szBuf, (lstrlen(szBuf) + 1) * sizeof(TCHAR), REG_SZ);
}
