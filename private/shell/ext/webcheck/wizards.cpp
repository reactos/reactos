#include "private.h"
#include "offl_cpp.h"
#include "helper.h"

#include <mluisupp.h>

//  BUGBUG: (tnoonan) This whole file needs some cleanup - starting with a base dialog class.

const TCHAR  c_szStrEmpty[] = TEXT("");

const TCHAR  c_szStrBoot[] = TEXT("boot");
const TCHAR  c_szStrScrnSave[] = TEXT("scrnsave.exe");
const TCHAR  c_szStrSystemIni[] = TEXT("system.ini");
const TCHAR  c_szShowWelcome[] = TEXT("ShowWelcome");

TCHAR g_szDontAskScreenSaver[] = TEXT("DontAskAboutScreenSaver");

extern BOOL CALLBACK MailOptionDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern void ReadDefaultEmail(LPTSTR, UINT);
extern BOOL CALLBACK LoginOptionDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern void ReadDefaultEmail(LPTSTR szBuf, UINT cch);
extern void ReadDefaultSMTPServer(LPTSTR szBuf, UINT cch);

INT_PTR CALLBACK WelcomeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DownloadDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PickScheduleDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NewScheduleWizDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK LoginDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#define WIZPAGE_NOINTRO         0x0001
#define WIZPAGE_NODOWNLOAD      0x0002
#define WIZPAGE_NOLOGIN         0x0004

struct WizInfo 
{
    SUBSCRIPTIONTYPE    subType;
    POOEBuf             pOOE;
    DWORD               dwExceptFlags;
    BOOL                bShowWelcome;
    BOOL                bIsNewSchedule;
    NOTIFICATIONCOOKIE  newCookie;
};

struct WizPage
{
    int     nResourceID;
    DLGPROC dlgProc;
    DWORD   dwExceptFlags;
};

const WizPage WizPages[] =
{
    { IDD_WIZARD0, WelcomeDlgProc,          WIZPAGE_NOINTRO         },   
    { IDD_WIZARD1, DownloadDlgProc,         WIZPAGE_NODOWNLOAD      },
    { IDD_WIZARD2, PickScheduleDlgProc,     0                       },
    { IDD_WIZARD3, NewScheduleWizDlgProc,   0                       },
    { IDD_WIZARD4, LoginDlgProc,            WIZPAGE_NOLOGIN         }
};

//  Helper functions

inline BOOL IsDesktop(SUBSCRIPTIONTYPE subType)
{
    return (subType == SUBSTYPE_DESKTOPURL) || (subType == SUBSTYPE_DESKTOPCHANNEL);
}

inline BOOL IsChannel(SUBSCRIPTIONTYPE subType)
{
    return (subType == SUBSTYPE_CHANNEL) || (subType == SUBSTYPE_DESKTOPCHANNEL);
}

inline DWORD GetShowWelcomeScreen()
{
    DWORD dwShowWelcome = TRUE;
    
    ReadRegValue(HKEY_CURRENT_USER, c_szRegKey, c_szShowWelcome, &dwShowWelcome, sizeof(DWORD));

    return dwShowWelcome;
}

inline void SetShowWelcomeScreen(DWORD dwShowWelcome)
{
    WriteRegValue(HKEY_CURRENT_USER, c_szRegKey, c_szShowWelcome, &dwShowWelcome, sizeof(DWORD), REG_DWORD);
}

//
//  Explanation of logic for the back/next/finish button
//
//  Wiz0 - welcome
//  Wiz1 - download
//  Wiz2 - pick schedule
//  Wiz3 - create schedule
//  Wiz4 - login
//
// A state machine can be derived to determine the different possibilities.
// The resulting state table is as follows:
//
// Wiz0:    Always has next button
//
// Wiz1:    Show back if Wiz0 was shown
//          Always has next button
//
// Wiz2:    Show back if Wiz0 or Wiz1 was shown
//          Show next if create new schedule or show login, otherwise show finish
//
// Wiz3:    Always has back button
//          Show next if show login, otherwise show finish
//
// Wiz4:    Always has back button
//          Always has finish button
//

void SetWizButtons(HWND hDlg, INT_PTR resID, WizInfo *pwi)
{
    DWORD dwButtons;

    switch (resID)
    {
        case IDD_WIZARD0:
            dwButtons = PSWIZB_NEXT;
            break;
            
        case IDD_WIZARD1:
            dwButtons = PSWIZB_NEXT;
            if (!(pwi->dwExceptFlags & WIZPAGE_NOINTRO))
            {
                dwButtons |= PSWIZB_BACK;
            }
            break;

        case IDD_WIZARD2:
            if ((!(pwi->dwExceptFlags & WIZPAGE_NODOWNLOAD)) ||
                (!(pwi->dwExceptFlags & WIZPAGE_NOINTRO)))
            {
                dwButtons = PSWIZB_BACK;
            }
            else
            {
                dwButtons = 0;
            }

            dwButtons |= (pwi->bIsNewSchedule || (!(pwi->dwExceptFlags & WIZPAGE_NOLOGIN)))
                         ? PSWIZB_NEXT : PSWIZB_FINISH;
            break;

        case IDD_WIZARD3:
            dwButtons = PSWIZB_BACK | 
                        ((!(pwi->dwExceptFlags & WIZPAGE_NOLOGIN)) ? PSWIZB_NEXT : PSWIZB_FINISH);
            break;

        case IDD_WIZARD4:
            dwButtons = PSWIZB_BACK | PSWIZB_FINISH;
            break;

        default:
            dwButtons = 0;
            ASSERT(FALSE);
            break;
    }
    
    PropSheet_SetWizButtons(GetParent(hDlg), dwButtons);
}

HRESULT CreateAndAddPage(PROPSHEETHEADER& psh, PROPSHEETPAGE& psp, int nPageIndex, DWORD dwExceptFlags)
{
    HRESULT hr = S_OK;
    
    if (!(WizPages[nPageIndex].dwExceptFlags & dwExceptFlags))
    {
        psp.pszTemplate = MAKEINTRESOURCE(WizPages[nPageIndex].nResourceID);
        psp.pfnDlgProc = WizPages[nPageIndex].dlgProc;

        HPROPSHEETPAGE hpage = CreatePropertySheetPage(&psp);

        if (NULL != hpage)
        {
            psh.phpage[psh.nPages++] = hpage;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CreateWizard(HWND hwndParent, SUBSCRIPTIONTYPE subType, POOEBuf pOOE)
{
    HRESULT hr = S_OK;
    UINT i;
    HPROPSHEETPAGE hPropPage[ARRAYSIZE(WizPages)];
    PROPSHEETPAGE psp = { 0 };
    PROPSHEETHEADER psh = { 0 };
    WizInfo wi;

    ASSERT(NULL != pOOE);
    ASSERT((subType >= SUBSTYPE_URL) && (subType <= SUBSTYPE_DESKTOPCHANNEL));

    wi.subType = subType;
    wi.pOOE = pOOE;
    wi.dwExceptFlags = 0;
    wi.bShowWelcome = GetShowWelcomeScreen();
    wi.bIsNewSchedule = FALSE;

    if (FALSE == wi.bShowWelcome)
    {
        wi.dwExceptFlags |= WIZPAGE_NOINTRO;
    }

    if (IsDesktop(subType))
    {
        wi.dwExceptFlags |= WIZPAGE_NODOWNLOAD;
    }

    if ((pOOE->bChannel && (!pOOE->bNeedPassword)) || 
        SHRestricted2W(REST_NoSubscriptionPasswords, NULL, 0))
    {
        wi.dwExceptFlags |= WIZPAGE_NOLOGIN;
    }

    // initialize propsheet header.
    psh.dwSize      = sizeof(PROPSHEETHEADER);
    psh.dwFlags     = PSH_WIZARD;
    psh.hwndParent  = hwndParent;
    psh.pszCaption  = NULL;
    psh.hInstance   = MLGetHinst();
    psh.nPages      = 0;
    psh.nStartPage  = 0;
    psh.phpage      = hPropPage;

    // initialize propsheet page.
    psp.dwSize          = sizeof(PROPSHEETPAGE);
    psp.dwFlags         = PSP_DEFAULT;
    psp.hInstance       = MLGetHinst();
    psp.pszIcon         = NULL;
    psp.pszTitle        = NULL;
    psp.lParam          = (LPARAM)&wi;

    for (i = 0; (i < ARRAYSIZE(WizPages)) && (SUCCEEDED(hr)); i++)
    {
        hr = CreateAndAddPage(psh, psp, i, wi.dwExceptFlags);
    }

    if (SUCCEEDED(hr))
    {
        // invoke the property sheet
        INT_PTR nResult = PropertySheet(&psh);

        if (nResult > 0)
        {
            SetShowWelcomeScreen(wi.bShowWelcome);
            hr = S_OK;
        }
        else if (nResult == 0)
        {
            hr = E_ABORT;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        for (i = 0; i < psh.nPages; i++)
        {
            DestroyPropertySheetPage(hPropPage[i]);
        }
    }

    return hr;
}


//--------------------------------------------------------------------
// Helper functions

BOOL IsADScreenSaverActive()
{
    BOOL bEnabled = FALSE;

    for (;;)
    {
        // NOTE: This is always written as an 8.3 filename
        TCHAR szCurrScrnSavePath[MAX_PATH];
        if (GetPrivateProfileString(c_szStrBoot,
                                    c_szStrScrnSave,
                                    c_szStrEmpty, 
                                    szCurrScrnSavePath,
                                    ARRAYSIZE(szCurrScrnSavePath),
                                    c_szStrSystemIni) == 0)
        {
            break;
        }

        TraceMsg(TF_ALWAYS, "szCurrScrnSavePath = %s", szCurrScrnSavePath);

        // If scrnsave = [none], we will get a null string back!
        if (!(*szCurrScrnSavePath))
            break;
        
        TCHAR szScrnSavePath[MAX_PATH];
        MLLoadString(IDS_SCREENSAVEREXE, szScrnSavePath, 
                   ARRAYSIZE(szScrnSavePath));

        TCHAR szFullScrnSavePath[MAX_PATH];
        TCHAR szWinPath[MAX_PATH];
        
        // Find the full file name and path of the screen saver
        // GetFileAttributes returns 0xFFFFFFFF if there is an error (ie: no file!)
        if (GetWindowsDirectory(szWinPath, ARRAYSIZE(szWinPath)))
        {
            PathCombine(szFullScrnSavePath, szWinPath, szScrnSavePath);
            if (GetFileAttributes(szFullScrnSavePath) == 0xFFFFFFFF)
            {
                TCHAR szSysPath[MAX_PATH];
                if (GetSystemDirectory(szSysPath, ARRAYSIZE(szSysPath)))
                {
                    PathCombine(szFullScrnSavePath, szSysPath, szScrnSavePath);
                    if (GetFileAttributes(szFullScrnSavePath) == 0xFFFFFFFF)
                    {
                        PathCombine(szFullScrnSavePath, szWinPath, TEXT("actsaver.scr"));
                        if (GetFileAttributes(szFullScrnSavePath) == 0xFFFFFFFF)
                        {
                            PathCombine(szFullScrnSavePath, szSysPath, TEXT("actsaver.scr"));
                            if (GetFileAttributes(szFullScrnSavePath) == 0xFFFFFFFF)
                                break;
                        }
                    }
                }
                else
                    break;
            }
        }
        else
            break;
        
        // Convert to 8.3 -- for some reason this is the 
        // only format that the CPL applet recognizes...
        TCHAR szShortScrnSavePath[MAX_PATH];
        if (GetShortPathName(szFullScrnSavePath, szShortScrnSavePath, ARRAYSIZE(szShortScrnSavePath)))
        {
            TraceMsg(TF_ALWAYS, "szCurrScrnSavePath = %s", szCurrScrnSavePath);
            TraceMsg(TF_ALWAYS, "szShortScrnSavePath = %s", szShortScrnSavePath);
                
            bEnabled = (StrCmpI(PathFindFileName(szCurrScrnSavePath),
                                PathFindFileName(szShortScrnSavePath)) == 0);
        }
        break;
    }
         
    return bEnabled;
}

HRESULT MakeADScreenSaverActive()
{
    TCHAR szScrnSavePath[MAX_PATH];
    MLLoadString(IDS_SCREENSAVEREXE, szScrnSavePath, 
               ARRAYSIZE(szScrnSavePath));

    if (PathFindOnPath(szScrnSavePath, NULL))
    {
        // Convert to 8.3 -- for some reason this is the 
        // only format that the CPL applet recognizes...
        TCHAR szShortScrnSavePath[MAX_PATH];
        GetShortPathName(szScrnSavePath, szShortScrnSavePath, ARRAYSIZE(szShortScrnSavePath));

        WritePrivateProfileString(  c_szStrBoot,
                                    c_szStrScrnSave, 
                                    szShortScrnSavePath,
                                    c_szStrSystemIni);

        // Flip the screen saver ON
        SystemParametersInfo(   SPI_SETSCREENSAVEACTIVE,
                                TRUE,
                                NULL, 
                                SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

        return S_OK;
    }
    else
        return E_FAIL;
}

//--------------------------------------------------------------------
// Dialog Procs

INT_PTR CALLBACK WelcomeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet =(LPPROPSHEETPAGE) GetWindowLongPtr(hDlg, DWLP_USER);
    WizInfo *pWiz = lpPropSheet ? (WizInfo *)lpPropSheet->lParam : NULL;
    NMHDR FAR *lpnm;
    BOOL result = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            result = TRUE;
            break;

        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:

                    ASSERT(NULL != lpPropSheet);
                    ASSERT(NULL != pWiz);
                    
                    SetWizButtons(hDlg, (INT_PTR) lpPropSheet->pszTemplate, pWiz);
                    result = TRUE;
                    break;
                    
                case PSN_KILLACTIVE:

                    ASSERT(NULL != pWiz);
                    
                    pWiz->bShowWelcome = !IsDlgButtonChecked(hDlg, IDC_WIZ_DONT_SHOW_INTRO);
                    result = TRUE;
                    break;
            }
            break;
            
    }

    return result;
}

void EnableLevelsDeep(HWND hwndDlg, BOOL fEnable)
{

    ASSERT(hwndDlg != NULL);

    EnableWindow(GetDlgItem(hwndDlg,IDC_WIZ_LINKSDEEP_STATIC1), fEnable);
    EnableWindow(GetDlgItem(hwndDlg,IDC_WIZ_LINKSDEEP_STATIC2), fEnable);
    EnableWindow(GetDlgItem(hwndDlg,IDC_WIZ_LINKSDEEP_EDIT), fEnable);
    EnableWindow(GetDlgItem(hwndDlg,IDC_WIZ_LINKSDEEP_SPIN), fEnable);

    return;

}

//
// shows or hides the UI for specifying the number "levels deep" to recurse
//
void ShowLevelsDeep(HWND hwndDlg, BOOL fShow)
{

    INT nCmdShow = fShow ? SW_SHOW: SW_HIDE;
    ASSERT(hwndDlg != NULL);

    ShowWindow(GetDlgItem(hwndDlg,IDC_WIZ_LINKSDEEP_STATIC1), nCmdShow);
    ShowWindow(GetDlgItem(hwndDlg,IDC_WIZ_LINKSDEEP_STATIC2), nCmdShow);
    ShowWindow(GetDlgItem(hwndDlg,IDC_WIZ_LINKSDEEP_EDIT), nCmdShow);
    ShowWindow(GetDlgItem(hwndDlg,IDC_WIZ_LINKSDEEP_SPIN), nCmdShow);

    return;

}

//
// enables or disables the UI for specifying the number "levels deep" to recurse
//
INT_PTR CALLBACK DownloadDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet =(LPPROPSHEETPAGE) GetWindowLongPtr(hDlg, DWLP_USER);
    WizInfo *pWiz = lpPropSheet ? (WizInfo *)lpPropSheet->lParam : NULL;
    POOEBuf  pBuf = pWiz ? pWiz->pOOE : NULL;
    NMHDR FAR *lpnm;
    BOOL result = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            TCHAR szBuf[256];
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);

            pWiz = (WizInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
            pBuf = pWiz ? pWiz->pOOE : NULL;
            
            SetListViewToString(GetDlgItem (hDlg, IDC_NAME), pBuf->m_Name);
            SetListViewToString(GetDlgItem (hDlg, IDC_URL), pBuf->m_URL);

            MLLoadString(
                IsChannel(pWiz->subType) ? IDS_WIZ_GET_LINKS_CHANNEL : IDS_WIZ_GET_LINKS_URL,
                szBuf, ARRAYSIZE(szBuf));

            SetDlgItemText(hDlg, IDC_WIZ_GET_LINKS_TEXT, szBuf);

            int checked;

            if ((pBuf->bChannel && (pBuf->fChannelFlags & CHANNEL_AGENT_PRECACHE_ALL)) ||
                (!pBuf->bChannel && ((pBuf->m_RecurseLevels) > 0)))
            {
                checked = IDC_WIZ_LINKS_YES;
            }
            else
            {
                checked = IDC_WIZ_LINKS_NO;
            }
            
            CheckRadioButton(hDlg, IDC_WIZ_LINKS_YES, IDC_WIZ_LINKS_NO, checked);

            //
            // Initialize the spin control for "levels deep" UI
            //
            HWND hwndLevelsSpin = GetDlgItem(hDlg,IDC_WIZ_LINKSDEEP_SPIN);
            SendMessage(hwndLevelsSpin, UDM_SETRANGE, 0, MAKELONG(MAX_WEBCRAWL_LEVELS, 1));
            SendMessage(hwndLevelsSpin, UDM_SETPOS, 0, pBuf->m_RecurseLevels);
            ShowLevelsDeep(hDlg,!pBuf->bChannel);
            EnableLevelsDeep(hDlg,!pBuf->bChannel && IDC_WIZ_LINKS_YES==checked);

            result = TRUE;
            break;
        }

        case WM_COMMAND:

            switch (HIWORD(wParam))
            {

            case BN_CLICKED:
                
                if (!pBuf->bChannel)
                    switch (LOWORD(wParam))
                    {

                    case IDC_WIZ_LINKS_YES:
                        EnableLevelsDeep(hDlg,TRUE);
                        break;

                    case IDC_WIZ_LINKS_NO:
                        EnableLevelsDeep(hDlg,FALSE);
                        break;

                    }
                break;

            case EN_KILLFOCUS:

                //
                // This code checks for bogus values in the "levels deep"
                // edit control and replaces them with something valid
                //
                if (LOWORD(wParam)==IDC_WIZ_LINKSDEEP_EDIT)
                {
                    BOOL fTranslated = FALSE;
                    UINT cLevels = GetDlgItemInt(hDlg,IDC_WIZ_LINKSDEEP_EDIT,&fTranslated,FALSE);

                    if (!fTranslated || cLevels < 1)
                    {
                        SetDlgItemInt(hDlg,IDC_WIZ_LINKSDEEP_EDIT,1,FALSE);
                    }
                    else if (cLevels > MAX_WEBCRAWL_LEVELS)
                    {
                        SetDlgItemInt(hDlg,IDC_WIZ_LINKSDEEP_EDIT,MAX_WEBCRAWL_LEVELS,FALSE);
                    }

                }

                break;

            }
            break;

        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:

                    ASSERT(NULL != lpPropSheet);
                    ASSERT(NULL != pWiz);
                    
                    SetWizButtons(hDlg, (INT_PTR) lpPropSheet->pszTemplate, pWiz);
                    result = TRUE;
                    break;

                case PSN_KILLACTIVE:

                    ASSERT(NULL != pBuf);

                    if (IsDlgButtonChecked(hDlg, IDC_WIZ_LINKS_YES))
                    {
                        if (IsChannel(pWiz->subType))
                        {
                            pBuf->fChannelFlags |= CHANNEL_AGENT_PRECACHE_ALL;
                        }
                        else
                        {
                            if (pBuf->m_RecurseLevels < 1)
                            {
                                DWORD dwPos = (DWORD)SendDlgItemMessage(hDlg,IDC_WIZ_LINKSDEEP_SPIN,UDM_GETPOS,0,0);

                                //
                                // Set the m_RecurseLevels field to the given by the
                                // spin control.  HIWORD(dwPos) indicated errror.
                                //
                                if (HIWORD(dwPos))
                                    pBuf->m_RecurseLevels = 1;
                                else
                                    pBuf->m_RecurseLevels = LOWORD(dwPos);

                            }
                            pBuf->m_RecurseFlags |= WEBCRAWL_LINKS_ELSEWHERE;
                        }
                    }
                    else
                    {
                        if (IsChannel(pWiz->subType))
                        {
                            pBuf->fChannelFlags &= ~CHANNEL_AGENT_PRECACHE_ALL;
                            pBuf->fChannelFlags |= CHANNEL_AGENT_PRECACHE_SOME;
                        }
                        else
                        {
                            pBuf->m_RecurseLevels = 0;
                            pBuf->m_RecurseFlags &= ~WEBCRAWL_LINKS_ELSEWHERE;
                        }
                    }
                    break;

            }
            break;
            
    }

    return result;
}

void HandleScheduleButtons(HWND hDlg, LPPROPSHEETPAGE lpPropSheet, WizInfo *pWiz)
{
    ASSERT(NULL != lpPropSheet);
    ASSERT(NULL != pWiz);
    
    EnableWindow(GetDlgItem(hDlg, IDC_WIZ_SCHEDULE_LIST),
        IsDlgButtonChecked(hDlg, IDC_WIZ_SCHEDULE_EXISTING));

    pWiz->bIsNewSchedule = IsDlgButtonChecked(hDlg, IDC_WIZ_SCHEDULE_NEW);

    SetWizButtons(hDlg, (INT_PTR) lpPropSheet->pszTemplate, pWiz);
}

struct PICKSCHED_LIST_DATA
{
    SYNCSCHEDULECOOKIE SchedCookie;
};

struct PICKSCHED_ENUM_DATA
{
    HWND hwndSchedList;
    POOEBuf pBuf;
    SYNCSCHEDULECOOKIE defSchedule;
    SYNCSCHEDULECOOKIE customSchedule;
    int *pnDefaultSelection;
    BOOL bHasAtLeastOneSchedule:1;
    BOOL bFoundCustomSchedule:1;
};

BOOL PickSched_EnumCallback(ISyncSchedule *pSyncSchedule, 
                            SYNCSCHEDULECOOKIE *pSchedCookie,
                            LPARAM lParam)
{
    BOOL bAdded = FALSE;
    PICKSCHED_ENUM_DATA *psed = (PICKSCHED_ENUM_DATA *)lParam;
    DWORD dwSyncScheduleFlags;
    PICKSCHED_LIST_DATA *psld = NULL;

    ASSERT(NULL != pSyncSchedule);  

    if (SUCCEEDED(pSyncSchedule->GetFlags(&dwSyncScheduleFlags)))
    {
        //  This checks to make sure we only add a publisher's schedule to the
        //  list if it belongs to this item.
        if ((!(dwSyncScheduleFlags & SYNCSCHEDINFO_FLAGS_READONLY)) ||
            (*pSchedCookie == psed->customSchedule))
        {
            psld = new PICKSCHED_LIST_DATA;

            if (NULL != psld)
            {
                WCHAR wszName[MAX_PATH];
                DWORD cchName = ARRAYSIZE(wszName);

                if (SUCCEEDED(pSyncSchedule->GetScheduleName(&cchName, wszName)))
                {
                    TCHAR szName[MAX_PATH];

                    MyOleStrToStrN(szName, ARRAYSIZE(szName), wszName);

                    psed->bHasAtLeastOneSchedule = TRUE;

                    psld->SchedCookie = *pSchedCookie;

                    int index;
                    if (*pSchedCookie == psed->customSchedule)
                    {
                        index = ComboBox_InsertString(psed->hwndSchedList, 0, szName);
                        if ((index >= 0) && (psed->defSchedule == GUID_NULL))
                        {
                            //  Do this always for custom schedules if there
                            //  is no defSchedule set
                            *psed->pnDefaultSelection = index;
                            psed->bFoundCustomSchedule = TRUE;
                        }
                    }
                    else
                    {
                        index = ComboBox_AddString(psed->hwndSchedList, szName);
                    }

                    if (index >= 0)
                    {
                        bAdded = (ComboBox_SetItemData(psed->hwndSchedList, index, psld) != CB_ERR);

                        if ((psed->defSchedule == *pSchedCookie)
                            ||
                            ((-1 == *psed->pnDefaultSelection) &&
                                IsCookieOnSchedule(pSyncSchedule, &psed->pBuf->m_Cookie)))
                        {
                            *psed->pnDefaultSelection = index;
                        }
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

BOOL PickSched_FillSchedList(HWND hDlg, POOEBuf pBuf, int *pnDefaultSelection)
{
    PICKSCHED_ENUM_DATA sed;

    sed.hwndSchedList = GetDlgItem(hDlg, IDC_WIZ_SCHEDULE_LIST);
    sed.pBuf = pBuf;
    sed.customSchedule = GUID_NULL;
    sed.pnDefaultSelection = pnDefaultSelection;
    sed.bHasAtLeastOneSchedule = FALSE;
    sed.bFoundCustomSchedule = FALSE;
    sed.defSchedule = pBuf->groupCookie;    //  usually GUID_NULL, but if the user hits
                                            //  customize multiple times, he/she would
                                            //  expect it to be highlighted

    EnumSchedules(PickSched_EnumCallback, (LPARAM)&sed);

    if (!sed.bFoundCustomSchedule && pBuf->bChannel && 
        (sizeof(TASK_TRIGGER) == pBuf->m_Trigger.cbTriggerSize))
    {
        //  This item has a custom schedule but it isn't an existing
        //  schedule (actually, this is the normal case).  We now
        //  have to add a fake entry.
       
        PICKSCHED_LIST_DATA *psld = new PICKSCHED_LIST_DATA;

        if (NULL != psld)
        {
            TCHAR szSchedName[MAX_PATH];
            BOOL bAdded = FALSE;

            CreatePublisherScheduleName(szSchedName, ARRAYSIZE(szSchedName), 
                                        pBuf->m_Name, NULL);

            int index = ComboBox_InsertString(sed.hwndSchedList, 0, szSchedName);

            if (index >= 0)
            {
                bAdded = (ComboBox_SetItemData(sed.hwndSchedList, index, psld) != CB_ERR);
                sed.bHasAtLeastOneSchedule = TRUE;
                *pnDefaultSelection = index;
            }

            if (!bAdded)
            {
                delete psld;
            }
        }
    }

    return sed.bHasAtLeastOneSchedule;    
}

PICKSCHED_LIST_DATA *PickSchedList_GetData(HWND hwndSchedList, int index)
{
    PICKSCHED_LIST_DATA *psld = NULL;

    if (index < 0)
    {
        index = ComboBox_GetCurSel(hwndSchedList);
    }

    if (index >= 0)
    {
        psld = (PICKSCHED_LIST_DATA *)ComboBox_GetItemData(hwndSchedList, index);
        if (psld == (PICKSCHED_LIST_DATA *)CB_ERR)
        {
            psld = NULL;
        }
    }

    return psld;
}

void PickSchedList_FreeAllData(HWND hwndSchedList)
{
    int count = ComboBox_GetCount(hwndSchedList);

    for (int i = 0; i < count; i++)
    {
        PICKSCHED_LIST_DATA *psld = PickSchedList_GetData(hwndSchedList, i);
        if (NULL != psld)
        {
            delete psld;
        }
    }
}

INT_PTR CALLBACK PickScheduleDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet =(LPPROPSHEETPAGE)GetWindowLongPtr(hDlg, DWLP_USER);
    WizInfo *pWiz = lpPropSheet ? (WizInfo *)lpPropSheet->lParam : NULL;
    POOEBuf  pBuf = pWiz ? pWiz->pOOE : NULL;
    NMHDR FAR *lpnm;
    BOOL result = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);

            lpPropSheet = (LPPROPSHEETPAGE)lParam;
            pWiz = (WizInfo *)lpPropSheet->lParam;

            int nDefaultSelection = -1;
            BOOL bHaveSchedules = PickSched_FillSchedList(hDlg, pWiz->pOOE, 
                                                          &nDefaultSelection);
            BOOL bNoScheduledUpdates = SHRestricted2W(REST_NoScheduledUpdates, NULL, 0);
            int defID = IDC_WIZ_SCHEDULE_NONE;

            if (!bHaveSchedules)
            {
                ShowWindow(GetDlgItem(hDlg, IDC_WIZ_SCHEDULE_EXISTING), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_WIZ_SCHEDULE_LIST), SW_HIDE);
            }
            else if (!bNoScheduledUpdates)
            {
                if (-1 == nDefaultSelection)
                {
                    //  This item isn't on any schedule yet
                    nDefaultSelection = 0;
                }
                else
                {
                    //  This item is on at least one schedule
                    defID = IDC_WIZ_SCHEDULE_EXISTING;
                }
                
                ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_WIZ_SCHEDULE_LIST), 
                                   nDefaultSelection);
            }
            CheckRadioButton(hDlg, IDC_WIZ_SCHEDULE_NONE, IDC_WIZ_SCHEDULE_EXISTING, 
                             defID);

            ASSERT(NULL != lpPropSheet);
            ASSERT(NULL != pWiz);

            if (bNoScheduledUpdates)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_WIZ_SCHEDULE_NEW), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_WIZ_SCHEDULE_EXISTING), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_WIZ_SCHEDULE_LIST), FALSE);
            }
            else if (SHRestricted2(REST_NoEditingScheduleGroups, NULL, 0))
            {
                EnableWindow(GetDlgItem(hDlg, IDC_WIZ_SCHEDULE_NEW), FALSE);
            }

            HandleScheduleButtons(hDlg, lpPropSheet, pWiz);
            result = TRUE;
            break;
        }

        case WM_DESTROY:
            PickSchedList_FreeAllData(GetDlgItem(hDlg, IDC_WIZ_SCHEDULE_LIST));
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_WIZ_SCHEDULE_EXISTING:
                case IDC_WIZ_SCHEDULE_NEW:
                case IDC_WIZ_SCHEDULE_NONE:

                    ASSERT(NULL != lpPropSheet);
                    ASSERT(NULL != pWiz);

                    HandleScheduleButtons(hDlg, lpPropSheet, pWiz);
                    result = TRUE;
                    break;
            }
            break;

        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:

                    ASSERT(NULL != lpPropSheet);
                    ASSERT(NULL != pWiz);
                    
                    SetWizButtons(hDlg, (INT_PTR) lpPropSheet->pszTemplate, pWiz);
                    result = TRUE;
                    break;

                case PSN_WIZNEXT:
                case PSN_WIZFINISH:
                    if (IsDlgButtonChecked(hDlg, IDC_WIZ_SCHEDULE_NONE))
                    {
                        pBuf->groupCookie = NOTFCOOKIE_SCHEDULE_GROUP_MANUAL;
                    }
                    else if (IsDlgButtonChecked(hDlg, IDC_WIZ_SCHEDULE_EXISTING))
                    {
                        PICKSCHED_LIST_DATA *psld = 
                            PickSchedList_GetData(GetDlgItem(hDlg, IDC_WIZ_SCHEDULE_LIST), -1);

                        if (NULL != psld)
                        {
                            pBuf->groupCookie = psld->SchedCookie;
                        }
                    }
                    result = TRUE;
                    break;
                    
            }
            break;
            
    }

    return result;
}

#ifdef NEWSCHED_AUTONAME
void NewSchedWiz_AutoName(HWND hDlg, POOEBuf pBuf)
{
    if (!(pBuf->m_dwPropSheetFlags & PSF_NO_AUTO_NAME_SCHEDULE))
    {
        pBuf->m_dwPropSheetFlags &= ~PSF_NO_CHECK_SCHED_CONFLICT;

        NewSched_AutoNameHelper(hDlg);
    }
}
#endif

/*
struct SUBCLASS_DATA
{
    POOEBuf pBuf;
    WNDPROC lpfnOldWndProc;
};

const TCHAR c_szSubClassProp[] = TEXT("WCSubClass");

LRESULT EditSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    SUBCLASS_DATA *psd = (SUBCLASS_DATA *)GetProp(hwnd, c_szSubClassProp);
    WNDPROC lpfnOldWndProc = psd->lpfnOldWndProc;

    switch (message)
    {
        //  BUGBUG: How many other possibilities need to get handled?
        case WM_CHAR:
            SetPropSheetFlags(psd->pBuf, TRUE, PSF_NO_AUTO_NAME_SCHEDULE);
            break;

        case WM_DESTROY:
            SetWindowLong(hwnd, GWL_WNDPROC, (LONG)lpfnOldWndProc);
            RemoveProp(hwnd, c_szSubClassProp);
            delete psd;
            break;
    }

    return CallWindowProc(lpfnOldWndProc, hwnd, message, wParam, lParam);
}
*/

BOOL NewSchedWiz_ResolveNameConflict(HWND hDlg, POOEBuf pBuf)
{
    BOOL bResult = TRUE;

    if (!(pBuf->m_dwPropSheetFlags & PSF_NO_CHECK_SCHED_CONFLICT))
    {
        bResult = NewSched_ResolveNameConflictHelper(hDlg, &pBuf->m_Trigger, 
                                                     &pBuf->groupCookie);
    }

    if (bResult)
    {
        pBuf->m_dwPropSheetFlags |= PSF_NO_CHECK_SCHED_CONFLICT;
    }

    return bResult;
}

inline void NewSchedWiz_CreateSchedule(HWND hDlg, POOEBuf pBuf)
{
    ASSERT(pBuf->m_dwPropSheetFlags & PSF_NO_CHECK_SCHED_CONFLICT);

    NewSched_CreateScheduleHelper(hDlg, &pBuf->m_Trigger,
                                  &pBuf->groupCookie);
}

INT_PTR CALLBACK NewScheduleWizDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet =(LPPROPSHEETPAGE)GetWindowLongPtr(hDlg, DWLP_USER);
    WizInfo *pWiz = lpPropSheet ? (WizInfo *)lpPropSheet->lParam : NULL;
    POOEBuf  pBuf = pWiz ? pWiz->pOOE : NULL;
    NMHDR *lpnm;
    BOOL result = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            NewSched_OnInitDialogHelper(hDlg);

            pWiz = (WizInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
            pBuf = pWiz->pOOE;

            pBuf->hwndNewSchedDlg = hDlg;

/*
            SUBCLASS_DATA *psd = new SUBCLASS_DATA;

            if (NULL != psd)
            {
                HWND hwndEdit = GetDlgItem(hDlg, IDC_SCHEDULE_NAME);
                psd->pBuf = pBuf;
                psd->lpfnOldWndProc = (WNDPROC)GetWindowLong(hwndEdit, GWL_WNDPROC);
                if (SetProp(hwndEdit, c_szSubClassProp, (HANDLE)psd))
                {
                    SubclassWindow(hwndEdit, EditSubclassProc);
                }
                else
                {
                    delete psd;
                }
            }
*/
            result = TRUE;
            break;
        }

        case WM_COMMAND:
            if (NULL != pBuf)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_SCHEDULE_DAYS:
                        if (HIWORD(wParam) == EN_UPDATE)
                        {
                            if (LOWORD(wParam) == IDC_SCHEDULE_DAYS)
                            {
                                KeepSpinNumberInRange(hDlg, IDC_SCHEDULE_DAYS, 
                                                      IDC_SCHEDULE_DAYS_SPIN, 1, 99);

                                pBuf->m_dwPropSheetFlags &= ~PSF_NO_CHECK_SCHED_CONFLICT;

                                result = TRUE;
                            }
                        }
#ifdef NEWSCHED_AUTONAME
                        else if (HIWORD(wParam) == EN_CHANGE)
                        {
                            NewSchedWiz_AutoName(hDlg, pBuf);
                            result = TRUE;
                        }
#endif
                        break;

                    case IDC_SCHEDULE_NAME:
                        if (HIWORD(wParam) == EN_CHANGE)
                        {
                            pBuf->m_dwPropSheetFlags &= ~PSF_NO_CHECK_SCHED_CONFLICT;
                            result = TRUE;
                        }
                        break;
/*
                    case IDC_SCHEDULE_NAME:
                        if (HIWORD(wParam) == EN_CHANGE)
                        {
                            TCHAR szName[MAX_PATH];
                            GetDlgItemText(hDlg, IDC_SCHEDULE_NAME, szName, ARRAYSIZE(szName));

                            if (lstrlen(szName) == 0)
                            {
                                pBuf->m_dwPropSheetFlags &= ~PSF_NO_AUTO_NAME_SCHEDULE;
                            }
                        }
                        break;
*/
                }
            }
            break;

        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:

                    ASSERT(NULL != lpPropSheet);
                    ASSERT(NULL != pWiz);
                    if (!pWiz->bIsNewSchedule)
                    {
                        //  If the user didn't pick a new schedule, move on
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    }
                    else
                    {                    
                        SetWizButtons(hDlg, (INT_PTR) lpPropSheet->pszTemplate, pWiz);
                    }
                    result = TRUE;
                    break;

#ifdef NEWSCHED_AUTONAME
                case DTN_DATETIMECHANGE:
                    if (NULL != pBuf)
                    {
                        NewSchedWiz_AutoName(hDlg, pBuf);
                    }
                    break;
#endif

                case PSN_KILLACTIVE:
                    result = TRUE;
                    break;

                case PSN_WIZNEXT:
                    if (!NewSchedWiz_ResolveNameConflict(hDlg, pBuf))
                    {
                        //  Don't proceed
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                    }
                    result = TRUE;
                    break;
                    
                case PSN_WIZFINISH:
                    if (NewSchedWiz_ResolveNameConflict(hDlg, pBuf))
                    {
                        NewSchedWiz_CreateSchedule(hDlg, pBuf);
                    }
                    else
                    {
                        //  Don't proceed
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                    }
                    result = TRUE;
                    break;
            }
            break;            
    }

    return result;
}

void Login_EnableControls(HWND hDlg, BOOL bEnable)
{
    int IDs[] = { 
        IDC_USERNAME_LABEL, 
        IDC_USERNAME,
        IDC_PASSWORD_LABEL,
        IDC_PASSWORD,
        IDC_PASSWORDCONFIRM_LABEL,
        IDC_PASSWORDCONFIRM
    };

    for (int i = 0; i < ARRAYSIZE(IDs); i++)
    {
        EnableWindow(GetDlgItem(hDlg, IDs[i]), bEnable);
    }
}

INT_PTR CALLBACK LoginDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet =(LPPROPSHEETPAGE)GetWindowLongPtr(hDlg, DWLP_USER);
    WizInfo *pWiz = lpPropSheet ? (WizInfo *)lpPropSheet->lParam : NULL;
    POOEBuf  pBuf = pWiz ? pWiz->pOOE : NULL;
    NMHDR FAR *lpnm;
    BOOL result = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);

            lpPropSheet = (LPPROPSHEETPAGE)lParam;
            pWiz = (WizInfo *)lpPropSheet->lParam;
            pBuf = pWiz->pOOE;

            if (pBuf->bChannel)
            {
                ShowWindow(GetDlgItem(hDlg, IDC_PASSWORD_NO), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_PASSWORD_YES), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_LOGIN_PROMPT_URL), SW_HIDE);
            }
            else
            {
                CheckRadioButton(hDlg, IDC_PASSWORD_NO, IDC_PASSWORD_YES,
                    (((pBuf->username[0] == 0) && (pBuf->password[0] == 0)) ?
                        IDC_PASSWORD_NO : IDC_PASSWORD_YES));

                ShowWindow(GetDlgItem(hDlg, IDC_LOGIN_PROMPT), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_LOGIN_PROMPT_CHANNEL), SW_HIDE);
            }
                       
            Edit_LimitText(GetDlgItem(hDlg, IDC_USERNAME), ARRAYSIZE(pBuf->username) - 1);
            SetDlgItemText(hDlg, IDC_USERNAME, pBuf->username);

            Edit_LimitText(GetDlgItem(hDlg, IDC_PASSWORD), ARRAYSIZE(pBuf->password) - 1);
            SetDlgItemText(hDlg, IDC_PASSWORD, pBuf->password);

            Edit_LimitText(GetDlgItem(hDlg, IDC_PASSWORDCONFIRM), ARRAYSIZE(pBuf->password) - 1);
            SetDlgItemText(hDlg, IDC_PASSWORDCONFIRM, pBuf->password);

            Login_EnableControls(hDlg, (IsDlgButtonChecked(hDlg, IDC_PASSWORD_YES) || pBuf->bChannel));

            result = TRUE;
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_PASSWORD_YES:
                case IDC_PASSWORD_NO:
                    if (BN_CLICKED == HIWORD(wParam))
                    {
                        Login_EnableControls(hDlg, IsDlgButtonChecked(hDlg, IDC_PASSWORD_YES));
                        result = TRUE;
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:

                    ASSERT(NULL != lpPropSheet);
                    ASSERT(NULL != pWiz);
                    
                    SetWizButtons(hDlg, (INT_PTR) lpPropSheet->pszTemplate, pWiz);
                    result = TRUE;
                    break;

                case PSN_WIZFINISH:
                {
                    BOOL bFinishOK = TRUE;
                    
                    if (pBuf->bChannel || IsDlgButtonChecked(hDlg, IDC_PASSWORD_YES))
                    {
                        TCHAR szUsername[ARRAYSIZE(pBuf->username) + 1];
                        TCHAR szPassword[ARRAYSIZE(pBuf->password) + 1];
                        TCHAR szPasswordConfirm[ARRAYSIZE(pBuf->password) + 1];

                        GetDlgItemText(hDlg, IDC_USERNAME, szUsername, ARRAYSIZE(szUsername));
                        GetDlgItemText(hDlg, IDC_PASSWORD, szPassword, ARRAYSIZE(szPassword));
                        GetDlgItemText(hDlg, IDC_PASSWORDCONFIRM, szPasswordConfirm, ARRAYSIZE(szPasswordConfirm));

                        if (!szUsername[0] && (szPassword[0] || szPasswordConfirm[0]))
                        {
                            SGMessageBox(hDlg, 
                                        (pBuf->bChannel ? IDS_NEEDCHANNELUSERNAME : IDS_NEEDUSERNAME), 
                                        MB_ICONWARNING);
                            bFinishOK = FALSE;
                        }
                        else if (szUsername[0] && !szPassword[0])
                        {
                            SGMessageBox(hDlg, 
                                        (pBuf->bChannel ? IDS_NEEDCHANNELPASSWORD : IDS_NEEDPASSWORD), 
                                        MB_ICONWARNING);
                            bFinishOK = FALSE;
                        }
                        else if (StrCmp(szPassword, szPasswordConfirm) != 0)
                        {
                            SGMessageBox(hDlg, IDS_MISMATCHED_PASSWORDS, MB_ICONWARNING);
                            bFinishOK = FALSE;
                        }
                        else
                        {
                            StrCpyN(pBuf->username, szUsername, ARRAYSIZE(pBuf->username));
                            StrCpyN(pBuf->password, szPassword, ARRAYSIZE(pBuf->password));
                            pBuf->dwFlags |= (PROP_WEBCRAWL_UNAME | PROP_WEBCRAWL_PSWD);
                        }

                    }
                    if (!bFinishOK)
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                    }
                    else if (pWiz->bIsNewSchedule)
                    {
                        NewSchedWiz_CreateSchedule(pBuf->hwndNewSchedDlg, pBuf);
                    }

                    result = TRUE;
                    break;
                }
            }        
    }
    
    return result;
}

INT_PTR CALLBACK EnableScreenSaverDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                {
                    DWORD dwChecked = IsDlgButtonChecked(hDlg, IDC_DONTASKAGAIN);

                    WriteRegValue(  HKEY_CURRENT_USER,
                                    WEBCHECK_REGKEY, 
                                    g_szDontAskScreenSaver,
                                    &dwChecked, 
                                    sizeof(DWORD),
                                    REG_DWORD);

                    if (wParam == IDOK)
                        MakeADScreenSaverActive();                

                    EndDialog(hDlg, wParam);
                    break;
                }

                default:
                    return FALSE;
            }

            break;
        }

        default:
            return FALSE;
    }

    return TRUE;
}
