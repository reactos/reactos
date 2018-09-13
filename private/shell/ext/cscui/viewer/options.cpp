//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       options.cpp
//
//--------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
/*  File: options.cpp

    Description: Displays a property-sheet-like dialog containing
        optional settings for CSC.


        Classes:
            COfflineFilesPage - Contains basic CSC settings.  Designed
                to be dynamically added to the shell's View->Folder Options
                property sheet.

            CustomGOAAddDlg - Dialog for adding custom go-offline actions to
                the "advanced" dialog.

            CustomGOAEditDlg - Dialog for editing custom go-offline actions
                in the "advanced" dialog.

            CscOptPropSheetExt - Shell property sheet extension object for 
                adding the COfflineFilesPage to the shell's View->Folder Options
                property sheet.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    12/03/97    Initial creation.                                    BrianAu
    05/28/97    Removed CscOptPropSheet class.  Obsolete.            BrianAu
                Renamed AdvancedPage to CAdvOptDlg.  This better
                reflects the new behavior of the "advanced" dlg
                as a dialog rather than a property page as first
                designed.  
    07/29/98    Removed CscOptPropPage class.  Now we only have      BrianAu
                a single prop page so there was no reason for
                a common base class implementation.  All base
                class functionality has been moved up into the
                COfflineFilesPage class.
                Renamed "GeneralPage" class to "COfflineFilesPage"
                to reflect the current naming in the UI.
    08/21/98    Added PurgeCache and PurgeCacheCallback.             BrianAu
    08/27/98    Options dialog re-layout per PM changes.             BrianAu
                - Replaced part/full sync radio buttons with cbx.
                - Added reminder balloon controls.
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop


#include <math.h>
#include <prsht.h>
#include <resource.h>
#include <winnetwk.h>
#include <shlguidp.h>
#include <process.h>
#include <systrayp.h>   // STWM_ENABLESERVICE, etc.
#include <mobsyncp.h>
#include "options.h"
#include "autoptr.h"
#include "ccinline.h"
#include "msgbox.h"
#include "registry.h"
#include "regstr.h"
#include "filesize.h"
#include "uuid.h"
#include "config.h"
#include "pathstr.h"
#include "osver.h"
#include "uihelp.h"
#include "cscst.h"   // For PWM_SETREMINDERTIMER
#include "util.h"    // Utils from "dll" directory.
#include "folder.h"
#include "purge.h"


//
// Simple inline helper.  Why this isn't this a Win32 macro?
//
inline void EnableDialogItem(HWND hwnd, UINT idCtl, bool bEnable)
{
    EnableWindow(GetDlgItem(hwnd, idCtl), bEnable);
}



    

//-----------------------------------------------------------------------------
// COfflineFilesPage
//-----------------------------------------------------------------------------
const DWORD COfflineFilesPage::m_rgHelpIDs[] = {
    IDC_CBX_ENABLE_CSC,         HIDC_CBX_ENABLE_CSC,
    IDC_CBX_FULLSYNC_AT_LOGOFF, HIDC_RBN_FULLSYNC_AT_LOGOFF,
    IDC_CBX_LINK_ON_DESKTOP,    HIDC_CBX_LINK_ON_DESKTOP,
    IDC_CBX_REMINDERS,          HIDC_REMINDERS_ENABLE,
    IDC_TXT_REMINDERS1,         HIDC_REMINDERS_PERIOD,
    IDC_EDIT_REMINDERS,         HIDC_REMINDERS_PERIOD,
    IDC_SPIN_REMINDERS,         HIDC_REMINDERS_PERIOD,
    IDC_TXT_REMINDERS2,         DWORD(-1),               // "minutes."
    IDC_LBL_CACHESIZE_PCT,      DWORD(-1),               
    IDC_SLIDER_CACHESIZE_PCT,   HIDC_CACHESIZE_PCT,
    IDC_TXT_CACHESIZE_PCT,      DWORD(-1),
    IDC_BTN_DELETE_CACHE,       HIDC_BTN_DELETE_CACHE,
    IDC_BTN_VIEW_CACHE,         HIDC_BTN_VIEW_CACHE,
    IDC_BTN_ADVANCED,           HIDC_BTN_ADVANCED,
    IDC_STATIC2,                DWORD(-1),               // Icon
    IDC_STATIC3,                DWORD(-1),               // Icon's text.
    0, 0
    };

BOOL
COfflineFilesPage::OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lInitParam
    )
{
    DBGTRACE((DM_OPTDLG, DL_MID, TEXT("COfflineFilesPage::OnInitDialog")));

    m_hwndDlg = hwnd;

    m_config.Load();

    //
    // "Enable" checkbox.  This reflects the true state of CSC.  
    // Not the state of a registry setting.
    //
    CheckDlgButton(hwnd, 
                   IDC_CBX_ENABLE_CSC, 
                   IsCSCEnabled() ? BST_CHECKED : BST_UNCHECKED);
    //
    // "Sync at logoff actions" radio buttons.
    //
    CConfig::SyncAction action = m_config.SyncAtLogoff();

    CheckDlgButton(hwnd, 
                   IDC_CBX_FULLSYNC_AT_LOGOFF, 
                   CConfig::eSyncFull == action ? BST_CHECKED : BST_UNCHECKED);

    //
    // Configure the "reminder" controls.
    //
    HWND hwndSpin = GetDlgItem(hwnd, IDC_SPIN_REMINDERS);
    HWND hwndEdit = GetDlgItem(hwnd, IDC_EDIT_REMINDERS);
    SendMessage(hwndSpin, UDM_SETRANGE, 0, MAKELONG((short)9999, (short)1));
    SendMessage(hwndSpin, UDM_SETBASE, 10, 0);

    UDACCEL rgAccel[] = {{ 2, 1  },
                         { 4, 10 },
                           6, 100};

    SendMessage(hwndSpin, UDM_SETACCEL, (WPARAM)ARRAYSIZE(rgAccel), (LPARAM)rgAccel);

    SendMessage(hwndEdit, EM_SETLIMITTEXT, 4, 0);

    CheckDlgButton(hwnd, 
                   IDC_CBX_REMINDERS, 
                   m_config.NoReminders() ? BST_UNCHECKED : BST_CHECKED);

    SetDlgItemInt(hwnd, IDC_EDIT_REMINDERS, m_config.ReminderFreqMinutes(), FALSE);

    if (IsLinkOnDesktop())
    {
        CheckDlgButton(hwnd, IDC_CBX_LINK_ON_DESKTOP, BST_CHECKED);
    }

    //
    // "Cache Size" slider
    //
    //
    CSCSPACEUSAGEINFO sui;
    GetCscSpaceUsageInfo(&sui);

    m_hwndSlider = GetDlgItem(hwnd, IDC_SLIDER_CACHESIZE_PCT);
    InitSlider(hwnd, sui.llBytesOnVolume, sui.llBytesTotalInCache);

    //
    // Save away the initial page state.  This will be used to determine when to 
    // enable the "Apply" button.  See HandlePageStateChange().
    //
    GetPageState(&m_state);

    EnableCtls(m_hwndDlg);

    return TRUE;
}


INT_PTR CALLBACK
COfflineFilesPage::DlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    BOOL bResult = FALSE;

    //
    // Retrieve the "this" pointer from the dialog's userdata.
    // It was placed there in OnInitDialog().
    //
    COfflineFilesPage *pThis = (COfflineFilesPage *)GetWindowLongPtr(hDlg, DWLP_USER);

    try
    {
        switch(message)
        {
            case WM_INITDIALOG:
            {
                PROPSHEETPAGE *pPage = (PROPSHEETPAGE *)lParam;
                pThis = (COfflineFilesPage *)pPage->lParam;

                DBGASSERT((NULL != pThis));
                SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pThis);
                bResult = pThis->OnInitDialog(hDlg, (HWND)wParam, lParam);
                break;
            }

            case WM_NOTIFY:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnNotify(hDlg, (int)wParam, (LPNMHDR)lParam);
                break;

            case WM_COMMAND:
                if (NULL != pThis)
                    bResult = pThis->OnCommand(hDlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
                break;

            case WM_HELP:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnHelp(hDlg, (LPHELPINFO)lParam);
                break;
 
            case WM_CONTEXTMENU:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
                break;

            case WM_DESTROY:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnDestroy(hDlg);
                break;
 
            case WM_SETTINGCHANGE:
            case WM_SYSCOLORCHANGE:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnSettingChange(hDlg, message, wParam, lParam);
                break;

            case WM_HSCROLL:
                //
                // The cache-size slider generates horizontal scroll messages.
                //
                DBGASSERT((NULL != pThis));
                pThis->OnHScroll(hDlg,
                                (HWND)lParam,         // hwndSlider
                                (int)LOWORD(wParam),  // notify code
                                (int)HIWORD(wParam)); // thumb pos
                break;

            default:
                break;
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in COfflineFilesPage::DlgProc"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }

    return bResult;
}


//
// Forward all WM_SETTINGCHANGE and WM_SYSCOLORCHANGE messages
// to controls that need to stay in sync with color changes.
//
BOOL
COfflineFilesPage::OnSettingChange(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND rghwndCtls[] = { m_hwndSlider };

    for (int i = 0; i < ARRAYSIZE(rghwndCtls); i++)
    {
        SendMessage(rghwndCtls[i], uMsg, wParam, lParam);
    }
    return TRUE;
}


BOOL 
COfflineFilesPage::OnHelp(
    HWND hDlg, 
    LPHELPINFO pHelpInfo
    )
{
    DBGTRACE((DM_OPTDLG, DL_LOW, TEXT("COfflineFilesPage::_OnHelp")));

    if (HELPINFO_WINDOW == pHelpInfo->iContextType)
    {
        int idCtl = GetDlgCtrlID((HWND)pHelpInfo->hItemHandle);
        WinHelp((HWND)pHelpInfo->hItemHandle, 
                 UseWindowsHelp(idCtl) ? NULL : c_szHelpFile,
                 HELP_WM_HELP, 
                 (DWORD_PTR)((LPTSTR)m_rgHelpIDs));
    }
    return TRUE;
}


BOOL
COfflineFilesPage::OnContextMenu(
    HWND hwndItem,
    int xPos,
    int yPos
    )
{
    int idCtl = GetDlgCtrlID(hwndItem);
    WinHelp(hwndItem, 
            UseWindowsHelp(idCtl) ? NULL : c_szHelpFile,
            HELP_CONTEXTMENU, 
            (DWORD_PTR)((LPTSTR)m_rgHelpIDs));

    return FALSE;
}



UINT CALLBACK
COfflineFilesPage::PageCallback(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp
    )
{
    DBGTRACE((DM_OPTDLG, DL_MID, TEXT("COfflineFilesPage::PageCallback")));
    UINT uReturn = 1;
    COfflineFilesPage *pThis = (COfflineFilesPage *)ppsp->lParam;
    DBGASSERT((NULL != pThis));

    switch(uMsg)
    {
        case PSPCB_CREATE:
            DBGPRINT((DM_OPTDLG, DL_MID, TEXT("Callback for PSPCB_CREATE")));
            //
            // uReturn == 0 means Don't create the prop page.
            //
            uReturn = 1;
            break;

        case PSPCB_RELEASE:
            DBGPRINT((DM_OPTDLG, DL_MID, TEXT("Callback for PSPCB_RELEASE")));
            //
            // This will release the extension and call the virtual
            // destructor (which will destroy the prop page object).
            //
            pThis->m_pUnkOuter->Release();
            break;
    }
    return uReturn;
}



BOOL
COfflineFilesPage::OnCommand(
    HWND hwnd,
    WORD wNotifyCode,
    WORD wID,
    HWND hwndCtl
    )
{
    BOOL bResult = TRUE;
    switch(wNotifyCode)
    {
        case BN_CLICKED:
            switch(wID)
            {
                case IDC_CBX_ENABLE_CSC:
                    if (IsDlgButtonChecked(m_hwndDlg, IDC_CBX_ENABLE_CSC))
                    {
                        //
                        // Checked the "enable CSC" checkbox.
                        // Set the cache size slider to the default pct-used value (10%)
                        //
                        TrackBar_SetPos(m_hwndSlider, ThumbAtPctDiskSpace(0.10), true);
                        SetCacheSizeDisplay(GetDlgItem(m_hwndDlg, IDC_TXT_CACHESIZE_PCT), TrackBar_GetPos(m_hwndSlider));
                        CheckDlgButton(hwnd, 
                                       IDC_CBX_LINK_ON_DESKTOP, 
                                       IsLinkOnDesktop() ? BST_CHECKED : BST_UNCHECKED);
                    }
                    else
                    {
                        //
                        // If CSC is disabled we remove the Offline Files
                        // folder shortcut from the user's desktop.
                        //
                        CheckDlgButton(hwnd, IDC_CBX_LINK_ON_DESKTOP, BST_UNCHECKED);
                    }
                    //
                    // Fall through...
                    //
                case IDC_CBX_REMINDERS:
                    EnableCtls(hwnd);
                    //
                    // Fall through...
                    //
                case IDC_EDIT_REMINDERS:
                case IDC_CBX_FULLSYNC_AT_LOGOFF:
                case IDC_SLIDER_CACHESIZE_PCT:
                case IDC_CBX_LINK_ON_DESKTOP:
                    HandlePageStateChange();
                    bResult = FALSE;
                    break;

                case IDC_BTN_VIEW_CACHE:
                    COfflineFilesFolder::Open();
                    bResult = FALSE;
                    break;

                case IDC_BTN_DELETE_CACHE:
                    //
                    // Ctl-Shift when pressing "Delete Files..." 
                    // is a special entry to reformatting the cache.
                    //
                    if ((0x8000 & GetAsyncKeyState(VK_SHIFT)) &&
                        (0x8000 & GetAsyncKeyState(VK_CONTROL)))
                    {
                        OnFormatCache();
                    }
                    else
                    {
                        OnDeleteCache();
                    }
                    bResult = FALSE;
                    break;

                case IDC_BTN_ADVANCED:
                {
                    CAdvOptDlg dlg(m_hInstance, m_hwndDlg);
                    dlg.Run();
                    break;
                }

                default:
                    break;
            }
            break;
    
        case EN_UPDATE:
            if (IDC_EDIT_REMINDERS == wID)
            {
                static bool bResetting; // prevent reentrancy.
                if (!bResetting)
                {
                    //
                    // The edit control is configured for a max of 4 digits and
                    // numbers-only.  Therefore the user can enter anything between
                    // 0 and 9999.  We don't want to allow 0 so we need this extra
                    // check.  The spinner has been set for a range of 0-9999.
                    //
                    int iValue = GetDlgItemInt(hwnd, IDC_EDIT_REMINDERS, NULL, FALSE);
                    if (0 == iValue)
                    {
                        bResetting = true;
                        SetDlgItemInt(hwnd, IDC_EDIT_REMINDERS, 1, FALSE);
                        bResetting = false;
                    }
                }
                HandlePageStateChange();
            }
            break;
    }
    return bResult;
}



//
// Gather the state of the page and store it in a PgState object.
//
void
COfflineFilesPage::GetPageState(
    PgState *pps
    )
{
    pps->SetCscEnabled(BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, IDC_CBX_ENABLE_CSC));
    pps->SetLinkOnDesktop(BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, IDC_CBX_LINK_ON_DESKTOP));
    pps->SetFullSync(BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, IDC_CBX_FULLSYNC_AT_LOGOFF));
    pps->SetSliderPos(TrackBar_GetPos(m_hwndSlider));
    pps->SetRemindersEnabled(BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, IDC_CBX_REMINDERS));
    pps->SetReminderFreq(GetDlgItemInt(m_hwndDlg, IDC_EDIT_REMINDERS, NULL, FALSE));
}

void
COfflineFilesPage::HandlePageStateChange(
    void
    )
{
    PgState s;
    GetPageState(&s);
    if (s == m_state)
        PropSheet_UnChanged(GetParent(m_hwndDlg), m_hwndDlg);
    else
        PropSheet_Changed(GetParent(m_hwndDlg), m_hwndDlg);
}


//
// Handle horizontal scroll messages generated by the cache-size slider.
//
void
COfflineFilesPage::OnHScroll(
    HWND hwndDlg,
    HWND hwndCtl,
    int iCode,
    int iPos
    )
{
    if (TB_THUMBPOSITION != iCode && TB_THUMBTRACK != iCode)
        iPos = TrackBar_GetPos(hwndCtl);

    SetCacheSizeDisplay(GetDlgItem(hwndDlg, IDC_TXT_CACHESIZE_PCT), iPos);
    if (TB_ENDTRACK == iCode)
        HandlePageStateChange();
}


//
// Update the cache size display "95.3 MB (23% of drive)" string.
//
void
COfflineFilesPage::SetCacheSizeDisplay(
    HWND hwndCtl,
    int iThumbPos
    )
{
    //
    // First convert the thumb position to a disk space value.
    //
    CString strSize;
    FileSize fs(DiskSpaceAtThumb(iThumbPos));
    fs.GetString(&strSize);
    //
    // Convert the thumb position to a percent-disk space value.
    //
    double x = 0.0;
    if (0 < iThumbPos)
        x = MAX(1.0, Rx(iThumbPos) * 100.00);
    //
    // Convert the percent-disk space value to a text string.
    //
    TCHAR szPct[10];
    wsprintf(szPct, TEXT("%d"), (DWORD)x);
    //
    // Format the result and display in the dialog.
    //
    CString s(m_hInstance, IDS_FMT_CACHESIZE_DISPLAY, strSize.Cstr(), szPct);
    SetWindowText(hwndCtl, s);
}


void
COfflineFilesPage::InitSlider(
    HWND hwndDlg,
    LONGLONG llSpaceMax,
    LONGLONG llSpaceUsed
    )
{
    double pctUsed = 0.0; // Default
    
    //
    // Protect against:
    // 1. Div-by-zero
    // 2. Invalid FP operation. (i.e. 0.0 / 0.0)
    //
    if (0 != llSpaceMax)
       pctUsed = double(llSpaceUsed) / double(llSpaceMax);

    //
    // Change the resolution of the slider as drives get larger.
    //
    m_iSliderMax = 100;     // < 1GB
    if (llSpaceMax > 0x0000010000000000i64)
        m_iSliderMax = 500; // >= 1TB
    else if (llSpaceMax > 0x0000000040000000i64)
        m_iSliderMax = 300; // >= 1GB
                        
    m_llAvailableDiskSpace = llSpaceMax;

    TrackBar_SetTicFreq(m_hwndSlider, m_iSliderMax / 10);
    TrackBar_SetPageSize(m_hwndSlider, m_iSliderMax / 10);
    TrackBar_SetRange(m_hwndSlider, 0, m_iSliderMax, false);
    TrackBar_SetPos(m_hwndSlider, ThumbAtPctDiskSpace(pctUsed), true);
    SetCacheSizeDisplay(GetDlgItem(hwndDlg, IDC_TXT_CACHESIZE_PCT), TrackBar_GetPos(m_hwndSlider));
}


//
// Enable/disable page controls.
//
void
COfflineFilesPage::EnableCtls(
    HWND hwnd
    )
{

    typedef bool (CConfigItems::*PBMF)(void) const;

    static const struct
    {
        UINT idCtl;
        PBMF pfnRestricted;

    } rgCtls[] = { { IDC_CBX_FULLSYNC_AT_LOGOFF, &CConfigItems::NoConfigSyncAtLogoff },
                   { IDC_CBX_REMINDERS,          &CConfigItems::NoConfigReminders    },
                   { IDC_CBX_LINK_ON_DESKTOP,    NULL                                },
                   { IDC_TXT_CACHESIZE_PCT,      NULL                                },
                   { IDC_SLIDER_CACHESIZE_PCT,   &CConfigItems::NoConfigCacheSize    },
                   { IDC_LBL_CACHESIZE_PCT,      &CConfigItems::NoConfigCacheSize    },
                   { IDC_BTN_VIEW_CACHE,         NULL                                },
                   { IDC_BTN_ADVANCED,           NULL                                },
                   { IDC_BTN_DELETE_CACHE,       NULL                                }
                 };

    bool bCscEnabled = BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CBX_ENABLE_CSC);
    bool bEnable;
    for (int i = 0; i < ARRAYSIZE(rgCtls); i++)
    {
        bEnable = bCscEnabled;
        if (bEnable)
        {
            //
            // Apply any policy restrictions.
            //
            PBMF pfn = rgCtls[i].pfnRestricted;
            if (NULL != pfn && (m_config.*pfn)())
                bEnable = false;

            if (bEnable)
            {
                //
                // "View..." button requires special handling as it isn't based off of a 
                // boolean restriction function.
                //
                if ((IDC_BTN_VIEW_CACHE == rgCtls[i].idCtl || IDC_CBX_LINK_ON_DESKTOP == rgCtls[i].idCtl) && m_config.NoCacheViewer())
                {
                    bEnable = false;
                }
            }
        }
        EnableDialogItem(hwnd, rgCtls[i].idCtl, bEnable);
    }

    //
    // Reminder controls are dependent upon several inputs.
    //
    bEnable = bCscEnabled && 
              (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CBX_REMINDERS)) &&
              !m_config.NoConfigReminders() &&
              !m_config.NoConfigReminderFreqMinutes();

    EnableDialogItem(hwnd, IDC_TXT_REMINDERS1, bEnable);
    EnableDialogItem(hwnd, IDC_TXT_REMINDERS2, bEnable);
    EnableDialogItem(hwnd, IDC_EDIT_REMINDERS, bEnable);
    EnableDialogItem(hwnd, IDC_SPIN_REMINDERS, bEnable);
    //
    // "Enabled" checkbox requires special handling.
    // It can't be included with the other controls because it will be disabled
    // when the user unchecks it.  Then there's no way to re-enable it.
    // Disable the checkbox if any of the following is true:
    //  1. Admin policy has enabled/disabled CSC.
    //  2. Client is a remote-boot machine (NT5 only).
    //
    bEnable = !m_config.NoConfigCscEnabled();
#if defined(WINNT) && defined(REMOTE_BOOT)
    OsVersion osver;
    if (OsVersion::NT5 == osver.Get())
    {
        DWORD dwRemoteBoot = 0;
        DWORD cbRemoteBoot = sizeof(dwRemoteBoot);
        GetSystemInfoEx(SystemInfoRemoteBoot,
                        &dwRemoteBoot,
                        &cbRemoteBoot);

        bEnable = (0 == dwRemoteBoot);
    }
#endif
    EnableWindow(GetDlgItem(hwnd, IDC_CBX_ENABLE_CSC), bEnable);
}


BOOL 
COfflineFilesPage::OnNotify(
    HWND hDlg, 
    int idCtl, 
    LPNMHDR pnmhdr
    )
{
    BOOL bResult = TRUE;

    switch(pnmhdr->code)
    {
        case PSN_APPLY:
            bResult = ApplySettings(hDlg);
            break;

        case PSN_SETACTIVE:
            //
            // Enable/disable controls whenever the page becomes active.
            //
            EnableCtls(hDlg);
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
            bResult = FALSE;
            break;
    }
    return bResult;
}


BOOL
COfflineFilesPage::ApplySettings(
    HWND hwnd
    )
{
    DBGTRACE((DM_OPTDLG, DL_MID, TEXT("COfflineFilesPage::ApplySettings")));

    bool bUpdateSystrayUI = false;

    RegKey keyLM(HKEY_LOCAL_MACHINE, REGSTR_KEY_OFFLINEFILES);
    HRESULT hr = keyLM.Open(KEY_WRITE, true);
    if (FAILED(hr))
    {
        DBGERROR((TEXT("Error 0x%08X opening NetCache machine settings key"), hr));
        return FALSE;
    }

    RegKey keyCU(HKEY_CURRENT_USER, REGSTR_KEY_OFFLINEFILES);
    hr = keyCU.Open(KEY_WRITE, true);
    if (FAILED(hr))
    {
        DBGERROR((TEXT("Error 0x%08X opening NetCache user settings key"), hr));
        return FALSE;
    }

    PgState s;
    GetPageState(&s);

    //
    // Process the "enabled" setting even if the page state hasn't
    // changed.  This is a special case because we initialize the 
    // "enabled" checkbox from IsCSCEnabled() but we change the
    // enabled/disabled state by setting a registry value and 
    // possibly rebooting.
    //
    hr = keyLM.SetValue(REGSTR_VAL_CSCENABLED, DWORD(s.GetCscEnabled()));
    if (FAILED(hr))
    {
        DBGERROR((TEXT("Error 0x%08X setting reg value \"%s\""), hr, REGSTR_VAL_CSCENABLED));
    }

    //
    // Handle any enabling/disabling of CSC.
    //
    if (s.GetCscEnabled() != boolify(IsCSCEnabled()))
    {
        bool bReboot  = false;
        DWORD dwError = ERROR_SUCCESS;
        if (EnableOrDisableCsc(s.GetCscEnabled(), &bReboot, &dwError))
        {
            if (bReboot)
            {
                //
                // Requires a reboot.
                //
                PropSheet_RebootSystem(GetParent(hwnd));
            }
            else
            {
                //
                // It's dynamic (no reboot) so update the systray UI.
                // Note that we want to update the systray UI AFTER we've
                // made any configuration changes to the registry 
                // (i.e. balloon settings).
                //
                bUpdateSystrayUI = true;
            }
        }
        else
        {
            //
            // Error trying to enable or disable CSC.
            //
            CscMessageBox(m_hwndDlg,
                          MB_ICONERROR | MB_OK,
                          Win32Error(dwError),
                          m_hInstance,
                          s.GetCscEnabled() ? IDS_ERR_ENABLECSC : IDS_ERR_DISABLECSC);
        }
    }

    if (s != m_state)
    {
        DBGPRINT((DM_OPTDLG, DL_MID, TEXT("Applying COfflineFilesPage settings")));
        //
        // Write "sync-at-logoff" (quick vs. full) setting.
        //
        hr = keyCU.SetValue(REGSTR_VAL_SYNCATLOGOFF, DWORD(s.GetFullSync()));
        if (SUCCEEDED(hr))
        {
            if (!m_state.GetFullSync() && s.GetFullSync())
            {
                //
                // If the user has just turned on full sync we want to 
                // make sure SyncMgr is enabled for sync-at-logoff.  
                // There are some weirdnesses with doing this but it's the most
                // consistent behavior we can offer the user given
                // the current design of SyncMgr and CSC.  Internal use and beta
                // testing shows that users expect Sync-at-logoff to be enabled
                // when this checkbox is checked.
                //
                RegisterForSyncAtLogonAndLogoff(SYNCMGRREGISTERFLAG_PENDINGDISCONNECT, 
                                                SYNCMGRREGISTERFLAG_PENDINGDISCONNECT);
            }                                            
        }
        else
        {
            DBGERROR((TEXT("Error 0x%08X setting reg value \"%s\""), hr, REGSTR_VAL_SYNCATLOGOFF));
        }

        hr = keyCU.SetValue(REGSTR_VAL_NOREMINDERS, DWORD(!s.GetRemindersEnabled()));
        if (FAILED(hr))
        {
            DBGERROR((TEXT("Error 0x%08X setting reg value \"%s\""), hr, REGSTR_VAL_NOREMINDERS));
        }

        hr = keyCU.SetValue(REGSTR_VAL_REMINDERFREQMINUTES, DWORD(s.GetReminderFreq()));
        if (FAILED(hr))
        {
            DBGERROR((TEXT("Error 0x%08X setting reg value \"%s\""), hr, REGSTR_VAL_REMINDERFREQMINUTES));
        }

        if (m_state.GetReminderFreq() != s.GetReminderFreq())
        {
            PostToSystray(PWM_RESET_REMINDERTIMER, 0, 0);
        }

        //
        // Create or remove the folder link on the desktop.
        //
        if (m_state.GetLinkOnDesktop() != s.GetLinkOnDesktop())
        {
            TCHAR szLinkPath[MAX_PATH];
            bool bLinkFileIsOnDesktop = IsLinkOnDesktop(szLinkPath, ARRAYSIZE(szLinkPath));
            if (bLinkFileIsOnDesktop && !s.GetLinkOnDesktop())
            {
                DeleteFile(szLinkPath);
            }
            else if (!bLinkFileIsOnDesktop && s.GetLinkOnDesktop())
            {
                COfflineFilesFolder::CreateLinkOnDesktop(m_hwndDlg);
            }
        }                

        //
        // Write cache size as pct * 10,000.
        //
        double pctCacheSize = Rx(TrackBar_GetPos(m_hwndSlider));
        hr = keyLM.SetValue(REGSTR_VAL_DEFCACHESIZE, DWORD(pctCacheSize * 10000.00));
        if (FAILED(hr))
        {
            DBGERROR((TEXT("Error 0x%08X setting reg value \"%s\""), hr, REGSTR_VAL_DEFCACHESIZE));
        }

        ULARGE_INTEGER ulCacheSize;

        ulCacheSize.QuadPart = DWORDLONG(m_llAvailableDiskSpace * pctCacheSize);
        if (!CSCSetMaxSpace(ulCacheSize.HighPart, ulCacheSize.LowPart))
        {
            DBGERROR((TEXT("Error %d setting cache size"), GetLastError()));
        }

        GetPageState(&m_state);
    }


    if (bUpdateSystrayUI)
    {
        HWND hwndNotify = NULL;
        if (!s.GetCscEnabled())
        {
            //
            // If we're disabling CSC, refresh the shell windows BEFORE we 
            // destroy the SysTray CSCUI "service".
            //
            hwndNotify = _FindNotificationWindow();
            if (IsWindow(hwndNotify))
            {
                SendMessage(hwndNotify, PWM_REFRESH_SHELL, 0, 0);
            }
        }

        HWND hwndSysTray = FindWindow(SYSTRAY_CLASSNAME, NULL);
        if (IsWindow(hwndSysTray))
        {
            SendMessage(hwndSysTray, STWM_ENABLESERVICE, STSERVICE_CSC, s.GetCscEnabled());
        }

        if (s.GetCscEnabled())
        {
            SHLoadNonloadedIconOverlayIdentifiers();

            //
            // If we're enabling CSC, refresh the shell windows AFTER we 
            // create the SysTray CSCUI "service".
            //
            hwndNotify = _FindNotificationWindow();
            if (IsWindow(hwndNotify))
            {
                PostMessage(hwndNotify, PWM_REFRESH_SHELL, 0, 0);
            }
        }
    }

    return TRUE;
}


//
// Enables or disables CSC according to the bEnable arg.
//
// Returns:
//
//  TRUE   == Operation successful (reboot may be required).
//  FALSE  == Operation failed.  See *pdwError for cause.
//
//  *pbReboot indicates if a reboot is required.
//  *pdwError returns any error code.
// 
bool
COfflineFilesPage::EnableOrDisableCsc(
    bool bEnable,
    bool *pbReboot,
    DWORD *pdwError
    )
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // We'll assume no reboot required.
    //
    if (NULL != pbReboot)
        *pbReboot = false;

    if (!CSCDoEnableDisable(bEnable))
    {
        //
        // Tried to enable or disable but failed.
        // If enabling, it's just a failure and we return.
        // If disabling, it may have failed because there are open files.
        //
        dwError = GetLastError();
        if (!bEnable && ERROR_BUSY == dwError)
        {
            //
            // Failed to disable and there are open files.
            // Tell the user to close all open files then try again.
            //
            CscMessageBox(m_hwndDlg,
                          MB_ICONINFORMATION | MB_OK,
                          m_hInstance,
                          IDS_OPENFILESONDISABLE);

            if (!CSCDoEnableDisable(bEnable))
            {
                dwError = GetLastError();
                if (ERROR_BUSY == dwError)
                {
                    //
                    // Still can't disable CSC because there are open files.
                    // This means we'll have to reboot.
                    //
                    if (NULL != pbReboot)
                        *pbReboot = true;
                }
            }
        }
    }
    //
    // Return error code to caller.
    //
    if (NULL != *pdwError)
        *pdwError = dwError;

    return ERROR_SUCCESS == dwError || ERROR_BUSY == dwError;
}


//
// UI info passed to PurgeCache then returned to PurgeCacheCallback.
//
struct PROGRESS_UI_INFO
{
    HINSTANCE hInstance;     // Module containing UI text strings.
    HWND hwndParent;         // Parent window for error dialog.
    IProgressDialog *pDlg;   // Progress dialog.
};


//
// This callback updates the progress UI for deleting cached items.
// 
//
BOOL
COfflineFilesPage::PurgeCacheCallback(
    CCachePurger *pPurger
    )
{
    BOOL bContinue = TRUE;
    PROGRESS_UI_INFO *ppui = (PROGRESS_UI_INFO *)pPurger->CallbackData();
    IProgressDialog *pDlg  = ppui->pDlg;

    const DWORD dwPhase = pPurger->Phase();

    //
    // Adjust dialog appearance at start of each phase.
    //
    if (0 == pPurger->FileOrdinal())
    {
        CString s(ppui->hInstance, 
                  PURGE_PHASE_SCAN == dwPhase ? IDS_SCANNING_DOTDOTDOT : IDS_DELETING_DOTDOTDOT);
        pDlg->SetLine(2, s, FALSE, NULL);
        //
        // We don't start registering progress until the "delete" phase.
        // To keep this code simple we just set the progress bar at 0% at the beginning
        // of each phase.  This way it will be 0% throughout the scanning phase and then
        // during the delete phase we'll increment it.  The scanning phase is very fast.
        //
        pDlg->SetProgress(0, 100);
    }

    if (PURGE_PHASE_SCAN == dwPhase)
    {
        //
        // Do nothing.  We've already set the "Scanning..." text above at the
        // start of the phase.
        //
    }
    else if (PURGE_PHASE_DELETE == dwPhase)
    {
        DWORD dwResult = pPurger->FileDeleteResult();
        //
        // Divide each value by 1,000 so that our numbers aren't so large.  This
        // means that if you're deleting less than 1,000 bytes of files, progress won't register.
        // I don't think that's a very likely scenario.  The DWORD() casts are required because
        // IProgressDialog::SetProgress only accepts DWORDs.  Dividing by 1,000 makes the 
        // likelihood of DWORD overflow very low.  To overflow the DWORD you'd need to be deleting
        // 4.294 e12 bytes from the cache.  The current limit on cache size is 4GB so that's
        // not going to happen in Win2000.
        //
        pDlg->SetProgress(DWORD(pPurger->BytesDeleted() / 1000), DWORD(pPurger->BytesToDelete() / 1000));
        if (ERROR_SUCCESS != dwResult)
        {
            //
            // The purger won't call us for directory entries.  Only files.
            //
            INT iUserResponse = IDOK;
            if (ERROR_BUSY == dwResult)
            {
                //
                // Special case for ERROR_BUSY.  This means that the
                // file is currently open for use by some process.
                // Most likely the network redirector.  I don't like the
                // standard text for ERROR_BUSY from winerror.
                //
                iUserResponse = CscMessageBox(ppui->hwndParent,
                                              MB_OKCANCEL | MB_ICONERROR,
                                              ppui->hInstance,
                                              IDS_FMT_ERR_DELFROMCACHE_BUSY,
                                              pPurger->FileName());
            }
            else
            {
                iUserResponse = CscMessageBox(ppui->hwndParent,
                                              MB_OKCANCEL | MB_ICONERROR,
                                              Win32Error(dwResult),
                                              ppui->hInstance,
                                              IDS_FMT_ERR_DELFROMCACHE,
                                              pPurger->FileName());
            }
            if (IDCANCEL == iUserResponse)
            {
                bContinue = FALSE;  // User cancelled through error dialog.
            }
        }
    }
    if (pDlg->HasUserCancelled())
        bContinue = FALSE;   // User cancelled through progress dialog.

    return bContinue;
}

//
// This feature has been included for use by PSS when there's no other
// way of fixing CSC operation.  Note this is only a last resort.
// It will wipe out all the contents of the CSC cache including the 
// notion of which files are pinned.  It does not affect any files
// on the network.  It does require a system reboot.  Again, use only
// as a last resort when CSC cache corruption is suspected.
//
void
COfflineFilesPage::OnFormatCache(
    void
    )
{
    if (IDYES == CscMessageBox(m_hwndDlg, 
                               MB_YESNO | MB_ICONWARNING,
                               m_hInstance,
                               IDS_REFORMAT_CACHE))
    {
        RegKey key(HKEY_LOCAL_MACHINE, REGSTR_KEY_OFFLINEFILES);
        HRESULT hr = key.Open(KEY_WRITE, true);
        if (SUCCEEDED(hr))
        {
            hr = key.SetValue(REGSTR_VAL_FORMATCSCDB, 1);
            if (SUCCEEDED(hr))
            {
                //
                // Tell prop sheet to return ID_PSREBOOTSYSTEM from PropertySheet().
                // Caller of PropertySheet is responsible for prompting user if they
                // want to reboot now or not.
                //
                PropSheet_RebootSystem(GetParent(m_hwndDlg));
            }
        }
        if (FAILED(hr))
        {
            DBGERROR((TEXT("Format failed with error %d"), HRESULT_CODE(hr)));
            //
            // BUGBUG:  This needs a reformat-specific message.
            //
            CscWin32Message(m_hwndDlg, HRESULT_CODE(hr), CSCUI::SEV_ERROR);
        }
    }
}        


//
// Invoked when user selects "Delete Files..." button in the CSC
// options dialog.
//
void
COfflineFilesPage::OnDeleteCache(
    void
    )
{
    //
    // Ask the user if they want to delete only temporary files
    // from the cache or both temp and pinned files.  Also gives
    // them an opportunity to cancel before beginning the deletion
    // operation.
    //
    CCachePurgerSel sel;
    CCachePurger::AskUserWhatToPurge(m_hwndDlg, &sel);
    if (PURGE_FLAG_NONE != sel.Flags())
    {
        CCoInit coinit;
        if (SUCCEEDED(coinit.Result()))
        {
            //
            // Use the shell's standard progress dialog.
            //
            com_autoptr<IProgressDialog> ptrpd;
            if (SUCCEEDED(CoCreateInstance(CLSID_ProgressDialog, 
                                           NULL, 
                                           CLSCTX_INPROC_SERVER, 
                                           IID_IProgressDialog, 
                                           (void **)ptrpd.getaddr())))
            {
                //
                // Set up the progress dialog using the standard shell "file delete"
                // animation (the one without the recycle bin).  The dialog
                // is modal.
                //
                CString strTitle(m_hInstance, IDS_APPLICATION);
                ptrpd->SetTitle(strTitle);
                ptrpd->SetAnimation(m_hInstance, IDA_FILEDEL);
                ptrpd->StartProgressDialog(m_hwndDlg, NULL, PROGDLG_AUTOTIME | PROGDLG_MODAL, NULL);
                //
                // Pass this info to the progress callback so we can display UI.
                //
                PROGRESS_UI_INFO pui;
                pui.hInstance  = m_hInstance;
                pui.hwndParent = m_hwndDlg;
                pui.pDlg       = ptrpd;
                //
                // Purge the cache files.  Will provide progress info through
                // the callback PurgeCacheCallback.
                //
                CCachePurger purger(sel, PurgeCacheCallback, &pui);
                purger.Scan();
                purger.Delete();

                ptrpd->StopProgressDialog();
                //
                // Display message to user.
                // "Deleted 10 files (2.5 MB)."
                //
                CString strBytesDeleted;
                FileSize fs(purger.BytesDeleted());
                fs.GetString(&strBytesDeleted);

                if (0 < purger.FilesDeleted())
                {
                    CscMessageBox(m_hwndDlg, 
                                  MB_OK | MB_ICONINFORMATION,
                                  m_hInstance,
                                  1 == purger.FilesDeleted() ? IDS_FMT_DELCACHE_FILEDELETED :
                                                               IDS_FMT_DELCACHE_FILESDELETED,
                                  purger.FilesDeleted(),
                                  strBytesDeleted.Cstr());
                }
                else
                {
                    CscMessageBox(m_hwndDlg, 
                                  MB_OK | MB_ICONINFORMATION,
                                  m_hInstance,
                                  IDS_DELCACHE_NOFILESDELETED);
                }
            }
        }
    }
}


//
// Determine if there's a shortcut to the offline files folder
// on the desktop.
// 
bool
COfflineFilesPage::IsLinkOnDesktop(
    LPTSTR pszPathOut,
    UINT cchPathOut
    )
{
    return S_OK == COfflineFilesFolder::IsLinkOnDesktop(m_hwndDlg, pszPathOut, cchPathOut);
}


//-----------------------------------------------------------------------------
// CAdvOptDlg
//-----------------------------------------------------------------------------
const CAdvOptDlg::CtlActions CAdvOptDlg::m_rgCtlActions[CConfig::eNumOfflineActions] = {
    { IDC_RBN_GOOFFLINE_SILENT, CConfig::eGoOfflineSilent },
    { IDC_RBN_GOOFFLINE_FAIL,   CConfig::eGoOfflineFail   }
                };


const DWORD CAdvOptDlg::m_rgHelpIDs[] = {
    IDOK,                           IDH_OK,
    IDCANCEL,                       IDH_CANCEL,
    IDC_RBN_GOOFFLINE_SILENT,       HIDC_RBN_GOOFFLINE_SILENT,
    IDC_RBN_GOOFFLINE_FAIL,         HIDC_RBN_GOOFFLINE_FAIL,
    IDC_GRP_CUSTGOOFFLINE,          HIDC_LV_CUSTGOOFFLINE,
    IDC_LV_CUSTGOOFFLINE,           HIDC_LV_CUSTGOOFFLINE,
    IDC_BTN_ADD_CUSTGOOFFLINE,      HIDC_BTN_ADD_CUSTGOOFFLINE,
    IDC_BTN_EDIT_CUSTGOOFFLINE,     HIDC_BTN_EDIT_CUSTGOOFFLINE,
    IDC_BTN_DELETE_CUSTGOOFFLINE,   HIDC_BTN_DELETE_CUSTGOOFFLINE,
    IDC_STATIC2,                    DWORD(-1),                    // Icon
    IDC_STATIC3,                    DWORD(-1),                    // Icon's text
    IDC_STATIC4,                    DWORD(-1),                    // Grp box #1
    0, 0
    };


int
CAdvOptDlg::Run(
    void
    )
{
    DBGTRACE((DM_OPTDLG, DL_HIGH, TEXT("CAdvOptDlg::Run")));
    int iResult = (int)DialogBoxParam(m_hInstance,
                                      MAKEINTRESOURCE(IDD_CSC_ADVOPTIONS),
                                      m_hwndParent,
                                      DlgProc,
                                      (LPARAM)this);

    if (-1 == iResult || 0 == iResult)
    {
        DBGERROR((TEXT("Error %d creating CSC advanced options dialog"),
                 GetLastError()));
    }
    return iResult;
}


INT_PTR CALLBACK
CAdvOptDlg::DlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    BOOL bResult = FALSE;

    //
    // Retrieve the "this" pointer from the dialog's userdata.
    // It was placed there in OnInitDialog().
    //
    CAdvOptDlg *pThis = (CAdvOptDlg *)GetWindowLongPtr(hDlg, DWLP_USER);

    try
    {
        switch(message)
        {
            case WM_INITDIALOG:
            {
                pThis = reinterpret_cast<CAdvOptDlg *>(lParam);
                SetWindowLongPtr(hDlg, DWLP_USER, (INT_PTR)pThis);
                bResult = pThis->OnInitDialog(hDlg, (HWND)wParam, lParam);
                break;
            }

            case WM_NOTIFY:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnNotify(hDlg, (int)wParam, (LPNMHDR)lParam);
                break;

            case WM_COMMAND:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnCommand(hDlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
                break;

            case WM_HELP:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnHelp(hDlg, (LPHELPINFO)lParam);
                break;
 
            case WM_CONTEXTMENU:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnContextMenu(wParam, lParam);
                break;

            case WM_DESTROY:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnDestroy(hDlg);
                break;
 
            default:
                break;
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in CAdvOptDlg::DlgProc"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }

    return bResult;
}


BOOL
CAdvOptDlg::OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lInitParam
    )
{
    DBGTRACE((DM_OPTDLG, DL_HIGH, TEXT("CAdvOptDlg::OnInitDialog")));
    BOOL bResult = FALSE;
    try
    {
        CArray<CConfig::CustomGOA> rgGOA;
        CConfig& config = CConfig::GetSingleton();

        m_hwndDlg = hwnd;
        m_hwndLV  = GetDlgItem(hwnd, IDC_LV_CUSTGOOFFLINE);

        CreateListColumns(m_hwndLV);
        ListView_SetExtendedListViewStyle(m_hwndLV, LVS_EX_FULLROWSELECT);

        //
        // Set the default go-offline action radio buttons.
        //
        CConfig::OfflineAction action = (CConfig::OfflineAction)config.GoOfflineAction(&m_bNoConfigGoOfflineAction);
        for (int i = 0; i < ARRAYSIZE(m_rgCtlActions); i++)
        {
            CheckDlgButton(hwnd, 
                           m_rgCtlActions[i].idRbn, 
                           m_rgCtlActions[i].action == action ? BST_CHECKED : BST_UNCHECKED);
        }
        //
        // Fill the custom go-offline actions listview.
        //
        config.GetCustomGoOfflineActions(&rgGOA, &m_bNoCustomizeGoOfflineAction);

        int cGOA = rgGOA.Count();
        for (i = 0; i < cGOA; i++)
            AddGOAToListView(m_hwndLV, i, rgGOA[i]);


        //
        // Adjust "enabledness" of controls for system policy.
        //
        EnableCtls(m_hwndDlg);
        //
        // Remember the initial page state so we can intelligently apply changes.
        //
        GetPageState(&m_state);

        bResult = TRUE;
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in CAdvOptDlg::OnInitDialog"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }
    return bResult;
}



BOOL
CAdvOptDlg::OnHelp(
    HWND hDlg, 
    LPHELPINFO pHelpInfo
    )
{
    if (HELPINFO_WINDOW == pHelpInfo->iContextType)
    {
        int idCtl = GetDlgCtrlID((HWND)pHelpInfo->hItemHandle);
        WinHelp((HWND)pHelpInfo->hItemHandle, 
                 UseWindowsHelp(idCtl) ? NULL : c_szHelpFile,
                 HELP_WM_HELP, 
                 (DWORD_PTR)((LPTSTR)m_rgHelpIDs));
    }

    return TRUE;
}


void
CAdvOptDlg::CreateListColumns(
    HWND hwndList
    )
{
    //
    // Create the header titles.
    //
    CString strServer(m_hInstance,  IDS_TITLE_COL_SERVER);
    CString strAction(m_hInstance, IDS_TITLE_COL_ACTION);

    RECT rcList;
    GetClientRect(hwndList, &rcList);
    int cxList = rcList.right - rcList.left - GetSystemMetrics(SM_CXVSCROLL);

#define LVCOLMASK (LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM)

    LV_COLUMN rgCols[] = { 
         { LVCOLMASK, LVCFMT_LEFT, (2 * cxList)/3, strServer, 0, iLVSUBITEM_SERVER },
         { LVCOLMASK, LVCFMT_LEFT, (1 * cxList)/3, strAction, 0, iLVSUBITEM_ACTION }
                         };
    //
    // Add the columns to the listview.
    //
    for (INT i = 0; i < ARRAYSIZE(rgCols); i++)
    {
        if (-1 == ListView_InsertColumn(hwndList, i, &rgCols[i]))
        {
            DBGERROR((TEXT("CAdvOptDlg::CreateListColumns failed to add column %d"), i));
        }
    }
}


int
CAdvOptDlg::GetFirstSelectedLVItemRect(
    RECT *prc
    )
{
    int iSel = ListView_GetNextItem(m_hwndLV, -1, LVNI_SELECTED);
    if (-1 != iSel)
    {
        if (ListView_GetItemRect(m_hwndLV, iSel, prc, LVIR_SELECTBOUNDS))
        {
            ClientToScreen(m_hwndLV, (LPPOINT)&prc->left);
            ClientToScreen(m_hwndLV, (LPPOINT)&prc->right);
            return iSel;
        }
    }
    return -1;
}



BOOL
CAdvOptDlg::OnContextMenu(
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND hwndItem = (HWND)wParam;
    INT xPos = -1;
    INT yPos = -1;
    INT iHit = -1;
    BOOL bResult = FALSE;

    if (-1 == lParam)
    {
        //
        // Not invoked with mouse click.  Probably Shift F10.
        // Pretend mouse was clicked in center of first selected item.
        //
        RECT rc;
        iHit = GetFirstSelectedLVItemRect(&rc);
        if (-1 != iHit)
        {
            xPos = rc.left + ((rc.right - rc.left) / 2);
            yPos = rc.top + ((rc.bottom - rc.top) / 2);
        }
    }
    else
    {
        //
        // Invoked with mouse click.  Now find out if a LV item was
        // selected directly.
        //
        xPos = LOWORD(lParam);
        yPos = HIWORD(lParam);

        LVHITTESTINFO hti;
        hti.pt.x  = xPos;
        hti.pt.y  = yPos;
        hti.flags = LVHT_ONITEM;
        ScreenToClient(m_hwndLV, &hti.pt);
        iHit = (INT)SendMessage(m_hwndLV, LVM_HITTEST, 0, (LPARAM)&hti);
    }
    if (-1 == iHit)
    {
        //
        // LV item not selected directly or Shift-F10 was not pressed.
        // Display "what's this" help for the listview control.
        //
        WinHelp(hwndItem, 
                UseWindowsHelp(GetDlgCtrlID(hwndItem)) ? NULL : c_szHelpFile,
                HELP_CONTEXTMENU, 
                (DWORD_PTR)((LPTSTR)m_rgHelpIDs));
    }
    else
    {
        //
        // LV item selected directly or Shift F10 pressed.  Display context menu for item.
        // 
        if (0 < ListView_GetSelectedCount(m_hwndLV) && IsCustomActionListviewEnabled())
        {
            HMENU hMenu = LoadMenu(m_hInstance, MAKEINTRESOURCE(IDR_ADVOPTIONS_CONTEXTMENU));
            if (NULL != hMenu)
            {
                HMENU hmenuTrackPopup = GetSubMenu(hMenu, 0);
                int cSetByPolicy = 0;
                CountSelectedListviewItems(&cSetByPolicy);
                if (0 < cSetByPolicy)
                {
                    //
                    // Disable menu items if any item in selection was set by policy.
                    //
                    int cItems = GetMenuItemCount(hmenuTrackPopup);
                    for (int i = 0; i < cItems; i++)
                    {
                        EnableMenuItem(hmenuTrackPopup, i, MF_GRAYED | MF_BYPOSITION);
                    }
                }
                else
                {
                    //
                    // Build a mask indicating which actions are present in the selected
                    // listview items.
                    //
                    int iItem = -1;
                    const DWORD fSilent = 0x00000001;
                    const DWORD fFail   = 0x00000002;
                    DWORD fActions = 0;
                    CConfig::CustomGOA *pGOA = NULL;
                    while(-1 != (iItem = ListView_GetNextItem(m_hwndLV, iItem, LVNI_SELECTED)))
                    {
                        pGOA = GetListviewObject(m_hwndLV, iItem);
                        DBGASSERT((!pGOA->SetByPolicy()));
                        switch(pGOA->GetAction())
                        {
                            case CConfig::eGoOfflineSilent: fActions |= fSilent; break;
                            case CConfig::eGoOfflineFail:   fActions |= fFail;   break;
                            default: break;
                        }
                    }
                    //
                    // Calculate how many bits are set in the action mask.
                    // If there's only one action set, we check that item in the menu.
                    // Otherwise, we leave them all unchecked to indicate a heterogeneous
                    // set.
                    //
                    int c = 0; // Count of bits set.
                    DWORD dw = fActions;
                    for (c = 0; 0 != dw; c++)
                        dw &= dw - 1;

                    //
                    // If the selection is homogeneous with respect to the action,
                    // check the menu item corresponding to the action.  Otherwise
                    // leave all items unchecked.
                    //
                    if (1 == c)
                    {
                        const struct
                        {
                            DWORD fMask;
                            UINT  idCmd;
                        } rgCmds[] = { { fSilent, IDM_ACTION_WORKOFFLINE },
                                       { fFail,   IDM_ACTION_FAIL        }
                                     };

                        for (int i = 0; i < ARRAYSIZE(rgCmds); i++)
                        {
                            if ((fActions & rgCmds[i].fMask) == rgCmds[i].fMask)
                            {
                                CheckMenuRadioItem(hmenuTrackPopup,
                                                   IDM_ACTION_WORKOFFLINE,
                                                   IDM_ACTION_FAIL,
                                                   rgCmds[i].idCmd,
                                                   MF_BYCOMMAND);
                                break;
                            }
                        }
                    }
                }

                TrackPopupMenu(hmenuTrackPopup,
                               TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                               xPos,
                               yPos,
                               0,
                               GetParent(hwndItem),
                               NULL);
            }
            DestroyMenu(hMenu);
        }
        bResult = TRUE;
    }
    return bResult;
}



//
// Return the offline action code associated with the currently-checked
// offline-action radio button.
//
CConfig::OfflineAction
CAdvOptDlg::GetCurrentGoOfflineAction(
    void
    ) const
{
    CConfig::OfflineAction action = CConfig::eNumOfflineActions;
    for (int i = 0; i < ARRAYSIZE(m_rgCtlActions); i++)
    {
        if (BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, m_rgCtlActions[i].idRbn))
        {
            action = m_rgCtlActions[i].action;
            break;
        }
    }
    DBGASSERT(CConfig::eNumOfflineActions != action);
    return action;
}


BOOL
CAdvOptDlg::OnCommand(
    HWND hwnd,
    WORD wNotifyCode,
    WORD wID,
    HWND hwndCtl
    )
{
    BOOL bResult = TRUE;
    if (BN_CLICKED == wNotifyCode)
    {
        switch(wID)
        {
            case IDOK:
                ApplySettings();
                //
                // Fall through...
                //
            case IDCANCEL:
                EndDialog(hwnd, wID);
                break;

            case IDC_BTN_ADD_CUSTGOOFFLINE:
                OnAddCustomGoOfflineAction();
                bResult = FALSE;
                break;

            case IDC_BTN_EDIT_CUSTGOOFFLINE:
                OnEditCustomGoOfflineAction();
                bResult = FALSE;
                break;

            case IDC_BTN_DELETE_CUSTGOOFFLINE:
                OnDeleteCustomGoOfflineAction();
                FocusOnSomethingInListview();
                if (0 < ListView_GetItemCount(m_hwndLV))
                    SetFocus(GetDlgItem(hwnd, IDC_BTN_DELETE_CUSTGOOFFLINE));
                bResult = FALSE;
                break;

            case IDM_ACTION_WORKOFFLINE:
            case IDM_ACTION_FAIL:
            case IDM_ACTION_DELETE:
                OnContextMenuItemSelected(wID);
                break;

            default:
                break;
        }
    
    }
    return bResult;
}


void
CAdvOptDlg::ApplySettings(
    void
    )
{
    DBGTRACE((DM_OPTDLG, DL_HIGH, TEXT("CAdvOptDlg::ApplySettings")));
    //
    // Now store changes from the "Advanced" dialog.
    //
    PgState s;
    GetPageState(&s);
    if (m_state != s)
    {
        DBGPRINT((DM_OPTDLG, DL_MID, TEXT("Applying advanced settings.")));

        RegKey keyCU(HKEY_CURRENT_USER, REGSTR_KEY_OFFLINEFILES);
        HRESULT hr = keyCU.Open(KEY_WRITE, true);
        if (SUCCEEDED(hr))
        {
            hr = keyCU.SetValue(REGSTR_VAL_GOOFFLINEACTION, 

                               (DWORD)s.GetDefGoOfflineAction());
            if (FAILED(hr))
            {
                DBGERROR((TEXT("Error 0x%08X setting reg value \"%s\""), hr, REGSTR_VAL_GOOFFLINEACTION));
            }

            //
            // Need "query" access because SaveCustomGoOfflineActions needs to 
            // delete all of the existing values before saving the new ones.
            //
            RegKey key(keyCU, REGSTR_SUBKEY_CUSTOMGOOFFLINEACTIONS);
            hr = key.Open(KEY_WRITE | KEY_QUERY_VALUE, true);
            if (SUCCEEDED(hr))
            {
                hr = CConfig::SaveCustomGoOfflineActions(key, 
                                                         s.GetCustomGoOfflineActions());
                if (FAILED(hr))
                {
                    DBGERROR((TEXT("Error 0x%08X setting custom offline actions"), hr));
                }
            }
        }
        else
        {
            DBGERROR((TEXT("Error 0x%08X opening advanced settings user key"), hr));
        }
    }
}



void
CAdvOptDlg::DeleteSelectedListviewItems(
    void
    )
{
    DBGTRACE((DM_OPTDLG, DL_MID, TEXT("CAdvOptDlg::DeleteSelectedListviewItems")));
    int iItem = -1;
    CConfig::CustomGOA *pGOA = NULL;
    CAutoSetRedraw autoredraw(m_hwndLV, false);
    while(-1 != (iItem = ListView_GetNextItem(m_hwndLV, -1, LVNI_SELECTED)))
    {
        DBGPRINT((DM_OPTDLG, DL_LOW, TEXT("Item %d selected for deletion"), iItem));
        pGOA = GetListviewObject(m_hwndLV, iItem);
        DBGASSERT((!pGOA->SetByPolicy()));
        ListView_DeleteItem(m_hwndLV, iItem);
        delete pGOA;
    }
    //
    // If the list is empty, this will disable the "Delete" and
    // "Edit" buttons.
    //
    EnableCtls(m_hwndDlg);
}


void
CAdvOptDlg::SetSelectedListviewItemsAction(
    CConfig::OfflineAction action
    )
{
    int iItem = -1;
    CConfig::CustomGOA *pGOA = NULL;
    CAutoSetRedraw autoredraw(m_hwndLV, false);
    while(-1 != (iItem = ListView_GetNextItem(m_hwndLV, iItem, LVNI_SELECTED)))
    {
        pGOA = GetListviewObject(m_hwndLV, iItem);
        DBGASSERT((!pGOA->SetByPolicy()));
        pGOA->SetAction(action);
        ListView_RedrawItems(m_hwndLV, iItem, iItem);
    }
}

int
CAdvOptDlg::CountSelectedListviewItems(
    int *pcSetByPolicy
    )
{
    DBGASSERT((NULL != pcSetByPolicy));
    int iItem = -1;
    int cSelected = 0;
    CConfig::CustomGOA *pGOA = NULL;
    while(-1 != (iItem = ListView_GetNextItem(m_hwndLV, iItem, LVNI_SELECTED)))
    {
        cSelected++;
        pGOA = GetListviewObject(m_hwndLV, iItem);
        if (pGOA->SetByPolicy())
            (*pcSetByPolicy)++;
    }
    return cSelected;
}


void
CAdvOptDlg::OnContextMenuItemSelected(
    int idMenuItem
    )
{
    if (IDM_ACTION_DELETE == idMenuItem)
    {
        DeleteSelectedListviewItems();
    }
    else
    {
        CConfig::OfflineAction action = CConfig::eNumOfflineActions;
        switch(idMenuItem)
        {
            case IDM_ACTION_WORKOFFLINE: action = CConfig::eGoOfflineSilent; break;
            case IDM_ACTION_FAIL:        action = CConfig::eGoOfflineFail;   break;
            default: break;
        }
        DBGASSERT((CConfig::eNumOfflineActions != action));

        SetSelectedListviewItemsAction(action);
    }
}



//
// Responds to the user pressing the "Add..." button.
//
void
CAdvOptDlg::OnAddCustomGoOfflineAction(
    void
    )
{
    CConfig::OfflineAction action = GetCurrentGoOfflineAction();
    CPath strServer;
    bool bDone = false;
    while(!bDone)
    {
        //
        // Run the "Add custom go-offline action" dialog.
        // User enters a server name and selects an action
        // from a set of radio buttons.
        //
        CustomGOAAddDlg dlg(m_hInstance, m_hwndDlg, &strServer, &action);
        int iResult = dlg.Run();

        if (IDCANCEL == iResult || strServer.IsEmpty())
        {
            //
            // User cancelled or didn't enter anything.
            //
            bDone = true;
        }
        else
        {
            //
            // User entered a server name.  Check if it's already in 
            // our list.
            //
            int iItem = -1;
            CConfig::CustomGOA *pObj = FindGOAInListView(m_hwndLV, strServer, &iItem);
            if (NULL != pObj)
            {
                //
                // Already an entry in list for this server.
                // If not set by policy, can replace it if desired.
                // If set by policy, can't change or delete it.
                //
                bool bSetByPolicy = pObj->SetByPolicy();
                DWORD idMsg   = bSetByPolicy ? IDS_ERR_GOOFFLINE_DUPACTION_NOCHG : IDS_ERR_GOOFFLINE_DUPACTION;
                DWORD dwFlags = bSetByPolicy ? MB_OK : MB_YESNO;
                if (IDYES == CscMessageBox(m_hwndDlg,
                                           dwFlags | MB_ICONWARNING,
                                           m_hInstance,
                                           idMsg,
                                           strServer.Cstr()))
                {
                    ReplaceCustomGoOfflineAction(pObj, iItem, action);
                    bDone = true;
                }
            }
            else
            {
                //
                // Server doesn't already exist in list.
                // Check if it's available on the net.
                //
                CAutoWaitCursor waitcursor;
                DWORD dwNetErr = CheckNetServer(strServer);
                switch(dwNetErr)
                {
                    case ERROR_SUCCESS:
                        //
                        // Server is available.  Add the entry to the listview.
                        //
                        AddCustomGoOfflineAction(strServer, action);
                        bDone = true;
                        break;

                    default:
                    {
                        CString strNetMsg;
                        if (ERROR_EXTENDED_ERROR == dwNetErr)
                        {
                            TCHAR szNetProvider[MAX_PATH];
                            WNetGetLastError(&dwNetErr,
                                             strNetMsg.GetBuffer(MAX_PATH),
                                             MAX_PATH,
                                             szNetProvider,
                                             ARRAYSIZE(szNetProvider));
                            strNetMsg.ReleaseBuffer();
                        }
                        else
                        {
                            strNetMsg.FormatSysError(dwNetErr);
                        }
                                         
                        //
                        // "The server 'servername' is either invalid
                        // or cannot be verified at this time.  Add anyway?"
                        // [Yes] [No] [Cancel].
                        //
                        switch(CscMessageBox(m_hwndDlg,
                                             MB_YESNOCANCEL | MB_ICONWARNING,
                                             m_hInstance,
                                             IDS_ERR_INVALIDSERVER,
                                             strServer.Cstr(),
                                             strNetMsg.Cstr()))
                        {
                            case IDYES:
                                AddCustomGoOfflineAction(strServer, action);
                                //
                                // Fall through...
                                //
                            case IDCANCEL:
                                bDone = true;
                                //
                                // Fall through...
                                //
                            case IDNO:
                                break;
                        }
                        break;
                    }
                }
            }
        }
    }
}


//
// Verify a server by going out to the net.
// Assumes pszServer points to a properly-formatted
// server name. (i.e. "Server" or "\\Server")
//
DWORD
CAdvOptDlg::CheckNetServer(
    LPCTSTR pszServer
    )
{
    DBGASSERT((NULL != pszServer));

    TCHAR rgchResult[MAX_PATH];
    DWORD cbResult = sizeof(rgchResult);
    LPTSTR pszSystem = NULL;

    //
    // Ensure the server name has a preceding "\\" for the
    // call to WNetGetResourceInformation.
    //
    CPath strServer(TEXT("\\\\"));
    while(*pszServer && TEXT('\\') == *pszServer)
        pszServer++;
    strServer += CString(pszServer);

    NETRESOURCE nr;
    nr.dwScope          = RESOURCE_GLOBALNET;
    nr.dwType           = RESOURCETYPE_ANY;
    nr.dwDisplayType    = 0;
    nr.dwUsage          = 0;
    nr.lpLocalName      = NULL;
    nr.lpRemoteName     = (LPTSTR)strServer.Cstr();
    nr.lpComment        = NULL;
    nr.lpProvider       = NULL;

    return WNetGetResourceInformation(&nr, rgchResult, &cbResult, &pszSystem);
}


//
// Adds a CustomGOA object to the listview.
//
void
CAdvOptDlg::AddCustomGoOfflineAction(
    LPCTSTR pszServer,
    CConfig::OfflineAction action
    )
{
    DBGTRACE((DM_OPTDLG, DL_LOW, TEXT("CAdvOptDlg::AddCustomGoOfflineAction")));
    DBGPRINT((DM_OPTDLG, DL_LOW, TEXT("Server = \"%s\", action = %d"), pszServer, action));

    AddGOAToListView(m_hwndLV, 0, CConfig::CustomGOA(pszServer, action, false));
}


//
// Replaces the action for an object in the listview.
//
void
CAdvOptDlg::ReplaceCustomGoOfflineAction(
    CConfig::CustomGOA *pGOA,
    int iItem,
    CConfig::OfflineAction action
    )
{
    DBGTRACE((DM_OPTDLG, DL_LOW, TEXT("CAdvOptDlg::ReplaceCustomGoOfflineAction")));
    DBGPRINT((DM_OPTDLG, DL_LOW, TEXT("Item = %d, action = %d"), iItem, action));

    pGOA->SetAction(action);
    ListView_RedrawItems(m_hwndLV, iItem, iItem);
}


//
// Create and add a CustomGOA object to the listview.
//
int
CAdvOptDlg::AddGOAToListView(
    HWND hwndLV, 
    int iItem, 
    const CConfig::CustomGOA& goa
    )
{
    LVITEM item;
    item.iSubItem = 0;
    item.mask     = LVIF_PARAM | LVIF_TEXT;
    item.pszText  = LPSTR_TEXTCALLBACK;
    item.iItem    = -1 == iItem ? ListView_GetItemCount(hwndLV) : iItem;
    item.lParam   = (LPARAM)new CConfig::CustomGOA(goa);
    return ListView_InsertItem(hwndLV, &item);
}


//
// Find the matching CustomGOA object in the listview for a given
// server.
//
CConfig::CustomGOA *
CAdvOptDlg::FindGOAInListView(
    HWND hwndLV,
    LPCTSTR pszServer,
    int *piItem
    )
{
    CString strServerInList;
    int cItems = ListView_GetItemCount(hwndLV);
    for (int iItem = 0; iItem < cItems; iItem++)
    {
        CConfig::CustomGOA *pObj = GetListviewObject(hwndLV, iItem);
        pObj->GetServerName(&strServerInList);
        if (0 == strServerInList.CompareNoCase(pszServer))
        {
            if (piItem)
                *piItem = iItem;
            return pObj;
        }
    }
    return NULL;
}

            
void
CAdvOptDlg::OnEditCustomGoOfflineAction(
    void
    )
{
    int cSetByPolicy = 0;
    int cSelected = CountSelectedListviewItems(&cSetByPolicy);

    if (0 < cSelected && 0 == cSetByPolicy)
    {
        DBGASSERT((0 == cSetByPolicy));
        CConfig::OfflineAction action = GetCurrentGoOfflineAction();
        CString strServer;
        CConfig::CustomGOA *pGOA = NULL;
        int iItem = -1;
        //
        // At least one selected item wasn't set by policy.
        //
        if (1 == cSelected)
        {
            //
            // Only one item selected so we can display a server name
            // in the dialog and indicate it's currently-set action.
            //
            iItem  = ListView_GetNextItem(m_hwndLV, -1, LVNI_SELECTED);
            pGOA   = GetListviewObject(m_hwndLV, iItem);
            action = pGOA->GetAction();
            pGOA->GetServerName(&strServer);
        }

        //
        // Display the "edit" dialog and let the user select a new action.
        //
        CustomGOAEditDlg dlg(m_hInstance, m_hwndDlg, strServer, &action);
        if (IDOK == dlg.Run())
        {
            SetSelectedListviewItemsAction(action);
        }
    }
}


void
CAdvOptDlg::OnDeleteCustomGoOfflineAction(
    void
    )
{
    int cSetByPolicy = 0;
    int cSelected = CountSelectedListviewItems(&cSetByPolicy);

    if (0 < cSelected)
    {
        DeleteSelectedListviewItems();
    }
}



BOOL 
CAdvOptDlg::OnNotify(
    HWND hDlg, 
    int idCtl, 
    LPNMHDR pnmhdr
    )
{
    BOOL bResult = TRUE;

    switch(pnmhdr->code)
    {
        case NM_SETFOCUS:
            if (IDC_LV_CUSTGOOFFLINE == idCtl)
                FocusOnSomethingInListview();
            break;

        case LVN_GETDISPINFO:
            OnLVN_GetDispInfo((LV_DISPINFO *)pnmhdr);
            break;

        case LVN_COLUMNCLICK:
            OnLVN_ColumnClick((NM_LISTVIEW *)pnmhdr);
            break;

        case LVN_ITEMCHANGED:
            OnLVN_ItemChanged((NM_LISTVIEW *)pnmhdr);
            break;

        case LVN_ITEMACTIVATE:
            OnEditCustomGoOfflineAction();
            break;

        case LVN_KEYDOWN:
            OnLVN_KeyDown((NMLVKEYDOWN *)pnmhdr);
            break;
    }

    return bResult;
}


void
CAdvOptDlg::FocusOnSomethingInListview(
    void
    )
{
    //
    // Focus on something.
    //
    int iFocus = ListView_GetNextItem(m_hwndLV, -1, LVNI_FOCUSED);
    if (-1 == iFocus)
        iFocus = 0;

    ListView_SetItemState(m_hwndLV, iFocus, LVIS_FOCUSED | LVIS_SELECTED,
                                            LVIS_FOCUSED | LVIS_SELECTED);

}


int CALLBACK 
CAdvOptDlg::CompareLVItems(
    LPARAM lParam1, 
    LPARAM lParam2,
    LPARAM lParamSort
    )
{
    CAdvOptDlg *pdlg = reinterpret_cast<CAdvOptDlg *>(lParamSort);
    int diff = 0;
    try
    {
        CConfig::CustomGOA *pGOA1 = (CConfig::CustomGOA *)lParam1;
        CConfig::CustomGOA *pGOA2 = (CConfig::CustomGOA *)lParam2;
        static CString s1, s2;

        //
        // This array controls the comparison column IDs used when
        // values for the selected column are equal.  These should
        // remain in order of the iLVSUBITEM_xxxxx enumeration with
        // respect to the first element in each row.
        //
        static const int rgColComp[2][2] = { 
            { iLVSUBITEM_SERVER, iLVSUBITEM_ACTION },
            { iLVSUBITEM_ACTION, iLVSUBITEM_SERVER }
                                           };
        int iCompare = 0;
        while(0 == diff && iCompare < ARRAYSIZE(rgColComp))
        {
            switch(rgColComp[pdlg->m_iLastColSorted][iCompare++])
            {
                case iLVSUBITEM_SERVER:
                    diff = pGOA1->GetServerName().CompareNoCase(pGOA2->GetServerName());
                    break;

                case iLVSUBITEM_ACTION:
                    s1.Format(pdlg->m_hInstance,
                              IDS_GOOFFLINE_ACTION_FIRST + (DWORD)pGOA1->GetAction());

                    s2.Format(pdlg->m_hInstance,
                              IDS_GOOFFLINE_ACTION_FIRST + (DWORD)pGOA2->GetAction());

                    diff = s1.CompareNoCase(s2);
                    break;

                default:
                    //
                    // If you hit this, you need to update this function
                    // to handle the new column you've added to the listview.
                    //
                    DBGASSERT((false));
                    break;
            }
        }
        //
        // Don't need contents of static strings between function invocations.
        // The strings are static to avoid repeated construction/destruction.
        // It's only a minor optimization.
        //
        s1.Empty();
        s2.Empty();
    }
    catch(CException& e)
    {
        //
        // Do nothing.  Just return diff "as is".  
        // Don't want to throw an exception back into comctl32.
        //
        DBGERROR((TEXT("C++ exception %d caught in CAdvOptDlg::CompareLVItems"), e.dwError));
    }
    return pdlg->m_bSortAscending ? diff : -1 * diff;
}



void
CAdvOptDlg::OnLVN_ItemChanged(
    NM_LISTVIEW *pnmlv
    )
{
    static const int rgBtns[] = { IDC_BTN_EDIT_CUSTGOOFFLINE,
                                  IDC_BTN_DELETE_CUSTGOOFFLINE };

    //
    // LVN_ITEMCHANGED is sent multiple times when you move the
    // highlight bar in a listview.  
    // Only run this code when the "focused" state bit is set
    // for the "new state".  This should be the last call in 
    // the series.
    // 
    if (LVIS_FOCUSED & pnmlv->uNewState && IsCustomActionListviewEnabled())
    {
        bool bEnable = 0 < ListView_GetSelectedCount(m_hwndLV);
        if (bEnable)
        {
            //
            // Only enable if all items weren't set by policy.
            //
            int cSetByPolicy = 0;
            CountSelectedListviewItems(&cSetByPolicy);
            bEnable = (0 == cSetByPolicy);
        }

        for (int i = 0; i < ARRAYSIZE(rgBtns); i++)
        {
            HWND hwnd    = GetDlgItem(m_hwndDlg, rgBtns[i]);
            if (bEnable != boolify(IsWindowEnabled(hwnd)))
            {
                EnableWindow(hwnd, bEnable);
            }
        }
    }
}

void
CAdvOptDlg::OnLVN_ColumnClick(
    NM_LISTVIEW *pnmlv
    )
{
    if (m_iLastColSorted != pnmlv->iSubItem)
    {
        m_bSortAscending = true;
        m_iLastColSorted = pnmlv->iSubItem;
    }
    else
    {
        m_bSortAscending = !m_bSortAscending;
    }

    ListView_SortItems(m_hwndLV, CompareLVItems, LPARAM(this));
}



void
CAdvOptDlg::OnLVN_KeyDown(
    NMLVKEYDOWN *plvkd
    )
{
    if (VK_DELETE == plvkd->wVKey && IsCustomActionListviewEnabled())
    {
        int cSetByPolicy = 0;
        CountSelectedListviewItems(&cSetByPolicy);
        if (0 == cSetByPolicy)
        {
            OnDeleteCustomGoOfflineAction();
            FocusOnSomethingInListview();
        }
        else
        {
            //
            // Provide feedback that deleting things set by policy
            // isn't allowed.  Visual feedback is that the "Remove"
            // button and context menu item are disabled.  That
            // doesn't help when user hits the [Delete] key.
            //
            MessageBeep(MB_OK);
        }
    }
}


void
CAdvOptDlg::OnLVN_GetDispInfo(
    LV_DISPINFO *plvdi
    )
{
    static CString strText;

    CConfig::CustomGOA* pObj = (CConfig::CustomGOA *)plvdi->item.lParam;

    if (LVIF_TEXT & plvdi->item.mask)
    {
        switch(plvdi->item.iSubItem)
        {
            case iLVSUBITEM_SERVER:
                if (pObj->SetByPolicy())
                {
                    //
                    // Format as "server ( policy )"
                    //
                    strText.Format(m_hInstance, 
                                   IDS_FMT_GOOFFLINE_SERVER_POLICY, 
                                   pObj->GetServerName().Cstr());
                }
                else
                {
                    //
                    // Just plain 'ol "server".
                    //
                    pObj->GetServerName(&strText);
                }
                break;
                
            case iLVSUBITEM_ACTION:
                strText.Format(m_hInstance,
                               IDS_GOOFFLINE_ACTION_FIRST + (DWORD)pObj->GetAction());
                break;

            default:
                strText.Empty();
                break;
        }
        plvdi->item.pszText = (LPTSTR)strText.Cstr();
    }

    if (LVIF_IMAGE & plvdi->item.mask)
    {
        //
        // Not displaying any images.  This is just a placeholder.
        // Should be optimized out by compiler.
        //
    }
}


CConfig::CustomGOA *
CAdvOptDlg::GetListviewObject(
    HWND hwndLV,
    int iItem
    )
{
    LVITEM item;
    item.iItem    = iItem;
    item.iSubItem = 0;
    item.mask     = LVIF_PARAM;
    if (-1 != ListView_GetItem(hwndLV, &item))
    {
        return (CConfig::CustomGOA *)item.lParam;
    }
    return NULL;
}
    


BOOL 
CAdvOptDlg::OnDestroy(
    HWND hwnd
    )
{
    if (NULL != m_hwndLV)
    {
        int cItems = ListView_GetItemCount(m_hwndLV);
        for (int i = 0; i < cItems; i++)
        {
            CConfig::CustomGOA *pObj = GetListviewObject(m_hwndLV, i);
            delete pObj;
        }
        ListView_DeleteAllItems(m_hwndLV);
    }
    return FALSE;
}
        


void
CAdvOptDlg::EnableCtls(
    HWND hwnd
    )
{
    static const struct
    {
        UINT idCtl;
        bool bRestricted;

    } rgCtls[] = { { IDC_RBN_GOOFFLINE_SILENT,     m_bNoConfigGoOfflineAction    },
                   { IDC_RBN_GOOFFLINE_FAIL,       m_bNoConfigGoOfflineAction    },
                   { IDC_GRP_GOOFFLINE_DEFAULTS,   m_bNoConfigGoOfflineAction    },
                   { IDC_GRP_CUSTGOOFFLINE,        m_bNoCustomizeGoOfflineAction },
                   { IDC_BTN_ADD_CUSTGOOFFLINE,    m_bNoCustomizeGoOfflineAction },
                   { IDC_BTN_EDIT_CUSTGOOFFLINE,   m_bNoCustomizeGoOfflineAction },
                   { IDC_BTN_DELETE_CUSTGOOFFLINE, m_bNoCustomizeGoOfflineAction }
                 };

    //
    // bCscEnabled is always true here.  The logic in the options prop page won't
    // let us display the "Advanced" dialog if it isn't.
    //
    bool bCscEnabled = true;
    for (int i = 0; i < ARRAYSIZE(rgCtls); i++)
    {
        bool bEnable = bCscEnabled;
        HWND hwndCtl = GetDlgItem(hwnd, rgCtls[i].idCtl);
        if (bEnable)
        {
            //
            // May be some further policy restrictions.
            //
            if (rgCtls[i].bRestricted)
                bEnable = false;

            if (bEnable)
            {
                if (IDC_BTN_EDIT_CUSTGOOFFLINE == rgCtls[i].idCtl ||
                    IDC_BTN_DELETE_CUSTGOOFFLINE == rgCtls[i].idCtl)
                {
                    //
                    // Only enable "Edit" and "Delete" buttons if there's an active
                    // selection in the listview.
                    //
                    bEnable = (0 < ListView_GetSelectedCount(m_hwndLV));
                }
            }
        }

        if (!bEnable)
        {
            if (GetFocus() == hwndCtl)
            {
                //
                // If disabling a control that has focus, advance the 
                // focus to the next control in the tab order before
                // disabling the current control.  Otherwise, it will
                // be stuck with focus and tabbing is busted.
                //
                SetFocus(GetNextDlgTabItem(hwnd, hwndCtl, FALSE));
            }
        }
        EnableWindow(GetDlgItem(hwnd, rgCtls[i].idCtl), bEnable);
    }
}



void
CAdvOptDlg::GetPageState(
    PgState *pps
    )
{
    pps->SetDefGoOfflineAction(GetCurrentGoOfflineAction());
    pps->SetCustomGoOfflineActions(m_hwndLV);
}



//
// Retrieve the records from the "custom go-offline actions" listview and place them
// into a member array, sorted by server name.
//
void
CAdvOptDlg::PgState::SetCustomGoOfflineActions(
    HWND hwndLV
    )
{
    int iItem = -1;
    LVITEM item;
    item.iSubItem = 0;
    item.mask     = LVIF_PARAM;

    CString s1, s2;
    m_rgCustomGoOfflineActions.Clear();
    int cItems = ListView_GetItemCount(hwndLV);
    for (int i = 0; i < cItems; i++)
    {
        CConfig::CustomGOA *pGOA = CAdvOptDlg::GetListviewObject(hwndLV, i);
        if (!pGOA->SetByPolicy())
        {
            int cGOA = m_rgCustomGoOfflineActions.Count();
            for (int i = 0; i < cGOA; i++)
            {
                pGOA->GetServerName(&s1);
                m_rgCustomGoOfflineActions[i].GetServerName(&s2);
                if (s1 < s2)
                    break;
            }
            if (i < cGOA)
                m_rgCustomGoOfflineActions.Insert(*pGOA, i);
            else
                m_rgCustomGoOfflineActions.Append(*pGOA);
        }
    }
}

//
// Two page states are equal if their Default go-offline actions are equal and their
// customized go-offline actions are equal.  Action is tested first because it's a 
// faster test.
//
bool
CAdvOptDlg::PgState::operator == (
    const CAdvOptDlg::PgState& rhs
    ) const
{
    bool bMatch = false;
    if (m_DefaultGoOfflineAction == rhs.m_DefaultGoOfflineAction)
    {
        if (m_rgCustomGoOfflineActions.Count() == rhs.m_rgCustomGoOfflineActions.Count())
        {
            CString s1, s2;
            CConfig::OfflineAction a1, a2;
            int cGOA = m_rgCustomGoOfflineActions.Count();
            for (int i = 0; i < cGOA; i++)
            {
                a1 = m_rgCustomGoOfflineActions[i].GetAction();
                a2 = rhs.m_rgCustomGoOfflineActions[i].GetAction();
                if (a1 != a2)
                    break;

                m_rgCustomGoOfflineActions[i].GetServerName(&s1);
                rhs.m_rgCustomGoOfflineActions[i].GetServerName(&s2);
                if (0 != s1.CompareNoCase(s2))
                    break;
            }
            bMatch = (i == cGOA);
        }
    }
    return bMatch;
}


//-----------------------------------------------------------------------------
// CustomGOAAddDlg
// "GOA" == Go Offline Action
// This dialog is for adding customized offline actions for particular
// network servers.
// It is invoked via the "Add..." button on the "Advanced" dialog.
//-----------------------------------------------------------------------------
const CustomGOAAddDlg::CtlActions CustomGOAAddDlg::m_rgCtlActions[CConfig::eNumOfflineActions] = {
    { IDC_RBN_GOOFFLINE_SILENT, CConfig::eGoOfflineSilent },
    { IDC_RBN_GOOFFLINE_FAIL,   CConfig::eGoOfflineFail   }
                };


const DWORD CustomGOAAddDlg::m_rgHelpIDs[] = {
    IDOK,                        IDH_OK,
    IDCANCEL,                    IDH_CANCEL,
    IDC_EDIT_GOOFFLINE_SERVER,   HIDC_EDIT_GOOFFLINE_SERVER,
    IDC_STATIC4,                 HIDC_EDIT_GOOFFLINE_SERVER, // "Computer:"
    IDC_RBN_GOOFFLINE_SILENT,    HIDC_RBN_GOOFFLINE_SILENT,
    IDC_RBN_GOOFFLINE_FAIL,      HIDC_RBN_GOOFFLINE_FAIL,
    IDC_BTN_BROWSEFORSERVER,     HIDC_BTN_BROWSEFORSERVER,
    IDC_GRP_GOOFFLINE_DEFAULTS,  DWORD(-1),
    IDC_STATIC2,                 DWORD(-1),                  // icon
    IDC_STATIC3,                 DWORD(-1),                  // icon's text
    0, 0
    };


CustomGOAAddDlg::CustomGOAAddDlg(
    HINSTANCE hInstance, 
    HWND hwndParent, 
    CString *pstrServer, 
    CConfig::OfflineAction *pAction
    ) : m_hInstance(hInstance),
        m_hwndParent(hwndParent),
        m_hwndDlg(NULL),
        m_hwndEdit(NULL),
        m_pstrServer(pstrServer),
        m_pAction(pAction) 
{ 
    DBGTRACE((DM_OPTDLG, DL_MID, TEXT("CustomGOAAddDlg::CustomGOAAddDlg")));
    DBGASSERT((NULL != pstrServer));
    DBGASSERT((NULL != pAction));
}


int 
CustomGOAAddDlg::Run(
    void
    )
{
    DBGTRACE((DM_OPTDLG, DL_HIGH, TEXT("CustomGOAAddDlg::Run")));
    return (int)DialogBoxParam(m_hInstance,
                               MAKEINTRESOURCE(IDD_CSC_ADVOPTIONS_ADD),
                               m_hwndParent,
                               DlgProc,
                               (LPARAM)this);
}


INT_PTR CALLBACK 
CustomGOAAddDlg::DlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    BOOL bResult = FALSE;

    CustomGOAAddDlg *pThis = (CustomGOAAddDlg *)GetWindowLongPtr(hDlg, DWLP_USER);
    try
    {
        switch(message)
        {
            case WM_INITDIALOG:
            {
                pThis = (CustomGOAAddDlg *)lParam;
                DBGASSERT((NULL != pThis));
                SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pThis);
                bResult = pThis->OnInitDialog(hDlg, (HWND)wParam, lParam);
                break;
            }

            case WM_COMMAND:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnCommand(hDlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
                break;

            case WM_HELP:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnHelp(hDlg, (LPHELPINFO)lParam);
                break;
 
            case WM_CONTEXTMENU:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
                break;

            case WM_DESTROY:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnDestroy(hDlg);
                break;
 
            default:
                break;
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in CustomGOAAddDlg::DlgProc"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }

    return bResult;
}

BOOL 
CustomGOAAddDlg::OnInitDialog(
    HWND hDlg, 
    HWND hwndFocus, 
    LPARAM lInitParam
    )
{
    DBGTRACE((DM_OPTDLG, DL_HIGH, TEXT("CustomGOAAddDlg::OnInitDialog")));
    m_hwndDlg = hDlg;
    //
    // Set the default go-offline action radio buttons.
    //
    for (int i = 0; i < ARRAYSIZE(m_rgCtlActions); i++)
    {
        CheckDlgButton(hDlg, 
                       m_rgCtlActions[i].idRbn, 
                       m_rgCtlActions[i].action == *m_pAction ? BST_CHECKED : BST_UNCHECKED);
    }
    m_hwndEdit = GetDlgItem(hDlg, IDC_EDIT_GOOFFLINE_SERVER);

    SetWindowText(m_hwndEdit, *m_pstrServer);

    return TRUE;
}

void
CustomGOAAddDlg::GetEnteredServerName(
    CString *pstrServer,
    bool bTrimLeadingJunk
    )
{
    //
    // Get the server name.
    //
    int cchServer = GetWindowTextLength(m_hwndEdit) + 1;
    LPTSTR pszServer = pstrServer->GetBuffer(cchServer);
    GetWindowText(m_hwndEdit, pszServer, cchServer);
    if (bTrimLeadingJunk)
    {
        //
        // Remove any leading "\\" or space user might have entered.
        //
        LPTSTR pszLookahead = pszServer;
        while(*pszLookahead && (TEXT('\\') == *pszLookahead || TEXT(' ') == *pszLookahead))
            pszLookahead++;

        if (pszLookahead > pszServer)
        {
            lstrcpy(pszServer, pszLookahead);
        }
    }
    pstrServer->ReleaseBuffer();
}


//
// Query the dialog and return the state through pointer args.
//
void
CustomGOAAddDlg::GetActionInfo(
    CString *pstrServer,
    CConfig::OfflineAction *pAction
    )
{
    DBGASSERT((NULL != pstrServer));
    DBGASSERT((NULL != pAction));
    //
    // Get the action radio button setting.
    //
    *pAction = CConfig::eNumOfflineActions;
    for(int i = 0; i < ARRAYSIZE(m_rgCtlActions); i++)
    {
        if (BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, m_rgCtlActions[i].idRbn))
        {
            *pAction = m_rgCtlActions[i].action;
            break;
        }
    }
    DBGASSERT((CConfig::eNumOfflineActions != *pAction));

    //
    // Retrieve the entered server name with leading junk removed.
    // Should have just a bare server name (i.e. "worf").
    //
    GetEnteredServerName(pstrServer, true);
}


//
// See if server name entered could be valid.
// This weeds out things like entering a UNC share name
// instead of a server name.  
//
// "\\rastaman" or "rastaman" is good.
// "\\rastaman\ntwin" is bad.
//
// This function will display error UI if the name is not valid.
// Returns:
//     true  = Name is a UNC server name.
//     false = Name is not a UNC server name.
//
bool
CustomGOAAddDlg::CheckServerNameEntered(
    void
    )
{

    CString strRaw;       // Name "as entered".
    GetEnteredServerName(&strRaw, false);

    CString strClean;     // Name with leading "\\" and any spaces removed.
    GetEnteredServerName(&strClean, true);

    CPath strPath(TEXT("\\\\"));
    strPath += strClean;

    if (!strPath.IsUNCServer())
    {
        //
        // Name provided is not a UNC server name.
        //
        CscMessageBox(m_hwndDlg,
                      MB_OK | MB_ICONERROR,
                      m_hInstance,
                      IDS_ERR_NOTSERVERNAME,
                      strRaw.Cstr());
        return false;
    }
    return true;
}



BOOL 
CustomGOAAddDlg::OnCommand(
    HWND hDlg, 
    WORD wNotifyCode, 
    WORD wID, 
    HWND hwndCtl
    )
{
    switch(wID)
    {
        case IDOK:
            //
            // First see if server name entered could be valid.
            // This weeds out things like entering a UNC share name
            // instead of a server name.  
            //
            // "\\rastaman" or "rastaman" is good.
            // "\\rastaman\ntwin" is bad.
            //
            // This function will display error UI if the name is not valid.
            //
            if (!CheckServerNameEntered())
            {
                //
                // Invalid name. Return focus to the server name edit control.
                //
                SetFocus(GetDlgItem(hDlg, IDC_EDIT_GOOFFLINE_SERVER));
                break;
            }

            GetActionInfo(m_pstrServer, m_pAction);            
            //
            // Fall through...
            //
        case IDCANCEL:
            EndDialog(hDlg, wID);
            break;

        case IDC_BTN_BROWSEFORSERVER:
        {
            CString strServer;
            BrowseForServer(hDlg, &strServer);
            if (!strServer.IsEmpty())
                SetWindowText(GetDlgItem(hDlg, IDC_EDIT_GOOFFLINE_SERVER), strServer);
            break;
        }
    }
    return FALSE;
}


//
// Use the SHBrowseForFolder dialog to find a server.
//
void
CustomGOAAddDlg::BrowseForServer(
    HWND hDlg,
    CString *pstrServer   // Server name returned in this.
    )
{
    DBGASSERT((NULL != pstrServer));
 
    TCHAR szServer[MAX_PATH];
    BROWSEINFO bi;
    ZeroMemory(&bi, sizeof(bi));

    CString strTitle(m_hInstance, IDS_BROWSEFORSERVER);
    //
    // Start browsing in the network folder.
    //
    sh_autoptr<ITEMIDLIST> ptrIdlRoot = NULL;
    SHGetSpecialFolderLocation(hDlg, CSIDL_NETWORK, ptrIdlRoot.getaddr());

    bi.hwndOwner      = hDlg;
    bi.pidlRoot       = ptrIdlRoot;
    bi.pszDisplayName = szServer;
    bi.lpszTitle      = strTitle.Cstr();
    bi.ulFlags        = BIF_BROWSEFORCOMPUTER;
    bi.lpfn           = NULL;
    bi.lParam         = NULL;
    bi.iImage         = 0;

    if (SHBrowseForFolder(&bi))
        *pstrServer = szServer;
}


BOOL 
CustomGOAAddDlg::OnHelp(
    HWND hDlg, 
    LPHELPINFO pHelpInfo
    )
{
    if (HELPINFO_WINDOW == pHelpInfo->iContextType)
    {
        int idCtl = GetDlgCtrlID((HWND)pHelpInfo->hItemHandle);
        WinHelp((HWND)pHelpInfo->hItemHandle, 
                 UseWindowsHelp(idCtl) ? NULL : c_szHelpFile,
                 HELP_WM_HELP, 
                 (DWORD_PTR)((LPTSTR)m_rgHelpIDs));
    }
    return FALSE;
}


BOOL
CustomGOAAddDlg::OnContextMenu(
    HWND hwndItem,
    int xPos,
    int yPos
    )
{
    int idCtl = GetDlgCtrlID(hwndItem);
    WinHelp(hwndItem, 
            UseWindowsHelp(idCtl) ? NULL : c_szHelpFile,
            HELP_CONTEXTMENU, 
            (DWORD_PTR)((LPTSTR)m_rgHelpIDs));

    return FALSE;
}


BOOL 
CustomGOAAddDlg::OnDestroy(
    HWND hDlg
    )
{

    return FALSE;
}


//-----------------------------------------------------------------------------
// CustomGOAEditDlg
// "GOA" == Go Offline Action
// This dialog is for editing customized offline actions for particular
// network servers.
// It is invoked via the "Edit..." button on the "Advanced" dialog.
//-----------------------------------------------------------------------------
const CustomGOAEditDlg::CtlActions CustomGOAEditDlg::m_rgCtlActions[CConfig::eNumOfflineActions] = {
    { IDC_RBN_GOOFFLINE_SILENT, CConfig::eGoOfflineSilent },
    { IDC_RBN_GOOFFLINE_FAIL,   CConfig::eGoOfflineFail   },
                };


const DWORD CustomGOAEditDlg::m_rgHelpIDs[] = {
    IDOK,                           IDH_OK,
    IDCANCEL,                       IDH_CANCEL,
    IDC_TXT_GOOFFLINE_SERVER,       HIDC_TXT_GOOFFLINE_SERVER,
    IDC_STATIC4,                    HIDC_TXT_GOOFFLINE_SERVER, // "Computer:"
    IDC_RBN_GOOFFLINE_SILENT,       HIDC_RBN_GOOFFLINE_SILENT,
    IDC_RBN_GOOFFLINE_FAIL,         HIDC_RBN_GOOFFLINE_FAIL,
    IDC_GRP_GOOFFLINE_DEFAULTS,     DWORD(-1),
    IDC_STATIC2,                    DWORD(-1),                 // icon
    IDC_STATIC3,                    DWORD(-1),                 // icon's text.
    0, 0
    };

CustomGOAEditDlg::CustomGOAEditDlg(
    HINSTANCE hInstance, 
    HWND hwndParent, 
    LPCTSTR pszServer, 
    CConfig::OfflineAction *pAction
    ) : m_hInstance(hInstance),
        m_hwndParent(hwndParent),
        m_hwndDlg(NULL),
        m_strServer(pszServer),
        m_pAction(pAction) 
{ 
    DBGTRACE((DM_OPTDLG, DL_MID, TEXT("CustomGOAEditDlg::CustomGOAEditDlg")));
    DBGASSERT((NULL != pszServer));
    DBGASSERT((NULL != pAction));
    if (m_strServer.IsEmpty())
        m_strServer.Format(m_hInstance, IDS_GOOFFLINE_MULTISERVER);
}



int 
CustomGOAEditDlg::Run(
    void
    )
{
    DBGTRACE((DM_OPTDLG, DL_HIGH, TEXT("CustomGOAEditDlg::Run")));
    return (int)DialogBoxParam(m_hInstance,
                               MAKEINTRESOURCE(IDD_CSC_ADVOPTIONS_EDIT),
                               m_hwndParent,
                               DlgProc,
                               (LPARAM)this);
}


INT_PTR CALLBACK 
CustomGOAEditDlg::DlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    BOOL bResult = FALSE;

    CustomGOAEditDlg *pThis = (CustomGOAEditDlg *)GetWindowLongPtr(hDlg, DWLP_USER);
    try
    {
        switch(message)
        {
            case WM_INITDIALOG:
            {
                pThis = (CustomGOAEditDlg *)lParam;
                DBGASSERT((NULL != pThis));
                SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pThis);
                bResult = pThis->OnInitDialog(hDlg, (HWND)wParam, lParam);
                break;
            }

            case WM_COMMAND:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnCommand(hDlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
                break;

            case WM_HELP:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnHelp(hDlg, (LPHELPINFO)lParam);
                break;
 
           case WM_CONTEXTMENU:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
                break;

            case WM_DESTROY:
                DBGASSERT((NULL != pThis));
                bResult = pThis->OnDestroy(hDlg);
                break;
 
            default:
                break;
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in CustomGOAEditDlg::DlgProc"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }

    return bResult;
}

BOOL 
CustomGOAEditDlg::OnInitDialog(
    HWND hDlg, 
    HWND hwndFocus, 
    LPARAM lInitParam
    )
{
    DBGTRACE((DM_OPTDLG, DL_MID, TEXT("CustomGOAEditDlg::OnInitDialog")));

    m_hwndDlg = hDlg;
    //
    // Set the default go-offline action radio buttons.
    //
    for (int i = 0; i < ARRAYSIZE(m_rgCtlActions); i++)
    {
        CheckDlgButton(hDlg, 
                       m_rgCtlActions[i].idRbn, 
                       m_rgCtlActions[i].action == *m_pAction ? BST_CHECKED : BST_UNCHECKED);
    }
    SetWindowText(GetDlgItem(hDlg, IDC_TXT_GOOFFLINE_SERVER), m_strServer);

    return TRUE;
}

//
// Query the dialog and return the state through pointer args.
//
void
CustomGOAEditDlg::GetActionInfo(
    CConfig::OfflineAction *pAction
    )
{
    DBGASSERT((NULL != pAction));
    //
    // Get the action radio button setting.
    //
    *pAction = CConfig::eNumOfflineActions;
    for(int i = 0; i < ARRAYSIZE(m_rgCtlActions); i++)
    {
        if (BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, m_rgCtlActions[i].idRbn))
        {
            *pAction = m_rgCtlActions[i].action;
            break;
        }
    }
    DBGASSERT((CConfig::eNumOfflineActions != *pAction));
}


BOOL 
CustomGOAEditDlg::OnCommand(
    HWND hDlg, 
    WORD wNotifyCode, 
    WORD wID, 
    HWND hwndCtl
    )
{
    switch(wID)
    {
        case IDOK:
            GetActionInfo(m_pAction);            
            //
            // Fall through...
            //
        case IDCANCEL:
            EndDialog(hDlg, wID);
            break;
    }
    return FALSE;
}


BOOL 
CustomGOAEditDlg::OnHelp(
    HWND hDlg, 
    LPHELPINFO pHelpInfo
    )
{
    if (HELPINFO_WINDOW == pHelpInfo->iContextType)
    {
        int idCtl = GetDlgCtrlID((HWND)pHelpInfo->hItemHandle);
        WinHelp((HWND)pHelpInfo->hItemHandle, 
                 UseWindowsHelp(idCtl) ? NULL : c_szHelpFile,
                 HELP_WM_HELP, 
                 (DWORD_PTR)((LPTSTR)m_rgHelpIDs));
    }

    return FALSE;
}

BOOL
CustomGOAEditDlg::OnContextMenu(
    HWND hwndItem,
    int xPos,
    int yPos
    )
{
    int idCtl = GetDlgCtrlID(hwndItem);
    WinHelp(hwndItem, 
            UseWindowsHelp(idCtl) ? NULL : c_szHelpFile,
            HELP_CONTEXTMENU, 
            (DWORD_PTR)((LPTSTR)m_rgHelpIDs));

    return FALSE;
}


BOOL 
CustomGOAEditDlg::OnDestroy(
    HWND hDlg
    )
{

    return FALSE;
}



//-----------------------------------------------------------------------------
// COfflineFilesSheet
//-----------------------------------------------------------------------------
COfflineFilesSheet::COfflineFilesSheet(
    HINSTANCE hInstance,
    LONG *pDllRefCount,
    HWND hwndParent
    ) : m_hInstance(hInstance),
        m_pDllRefCount(pDllRefCount),
        m_hwndParent(hwndParent)
{
    InterlockedIncrement(m_pDllRefCount);
}

COfflineFilesSheet::~COfflineFilesSheet(
    void
    )
{
    InterlockedDecrement(m_pDllRefCount);
}


BOOL CALLBACK
COfflineFilesSheet::AddPropSheetPage(
    HPROPSHEETPAGE hpage,
    LPARAM lParam
    )
{
    DBGTRACE((DM_OPTDLG, DL_MID, TEXT("COfflineFilesSheet::AddPropSheetPage")));
    PROPSHEETHEADER * ppsh = (PROPSHEETHEADER *)lParam;

    if (ppsh->nPages < COfflineFilesSheet::MAXPAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }

    return FALSE;
}


//
// Static function for creating and running an instance of the
// CSCUI options dialog.  This is the ONLY function callable
// by non-member code to create and run an options dialog.
//
DWORD
COfflineFilesSheet::CreateAndRun(
    HINSTANCE hInstance,
    HWND hwndParent,
    LONG *pDllRefCount
    )
{
    try
    {
        //
        // First try to activate an existing instance of the prop sheet.
        //
        CString strSheetTitle(hInstance, IDS_CSCOPT_PROPSHEET_TITLE);
        HWND hwnd = FindWindowEx(NULL, NULL, WC_DIALOG, strSheetTitle);
        if (NULL == hwnd || !SetForegroundWindow(hwnd))
        {
            //
            // This thread param buffer will be deleted by the
            // thread proc.
            //
            ThreadParams *ptp = new ThreadParams(hwndParent, pDllRefCount);
            if (NULL != ptp)
            {
                //
                // LoadLibrary on ourselves so that we stay in memory even
                // if the caller calls FreeLibrary.  We'll call FreeLibrary
                // when the thread proc exits.
                //
                ptp->SetModuleHandle(LoadLibrary(TEXT("cscui.dll")));

                DWORD idThread;
                HANDLE hThread = (HANDLE)_beginthreadex(NULL,
                                           0,
                                           ThreadProc,
                                           (LPVOID)ptp,
                                           0,
                                           (UINT *)&idThread);

                if (INVALID_HANDLE_VALUE != hThread)
                {
                    CloseHandle(hThread);
                }
            }
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in CSCOfflineFilesOptions"), e.dwError));
        CscWin32Message(NULL, e. dwError, CSCUI::SEV_ERROR);
    }
    return 0;
}


//
// The share dialog's thread proc.
//
UINT WINAPI
COfflineFilesSheet::ThreadProc(
    LPVOID pvParam
    )
{
    ThreadParams *ptp = reinterpret_cast<ThreadParams *>(pvParam);
    DBGASSERT((NULL != ptp));

    HINSTANCE hInstance = ptp->m_hInstance; // Save local copy.

    try
    {
        COfflineFilesSheet dlg(ptp->m_hInstance,
                               ptp->m_pDllRefCount,
                               ptp->m_hwndParent);
        dlg.Run();
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in COfflineFilesSheet::ThreadProc"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }
    delete ptp;

    if (NULL != hInstance)
        FreeLibraryAndExitThread(hInstance, 0);

    return 0;
}


DWORD
COfflineFilesSheet::Run(
    void
    )
{
    DWORD dwError = ERROR_SUCCESS;
    try
    {
        if (CConfig::GetSingleton().NoConfigCache())
        {
            DBGERROR((TEXT("System policy restricts configuration of Offline Files cache")));
            return ERROR_SUCCESS;
        }

        CString strSheetTitle(m_hInstance, IDS_CSCOPT_PROPSHEET_TITLE);

        HPROPSHEETPAGE rghPages[COfflineFilesSheet::MAXPAGES];
        PROPSHEETHEADER psh;
        ZeroMemory(&psh, sizeof(psh));
        //
        // Define sheet.
        //
        psh.dwSize          = sizeof(PROPSHEETHEADER);
        psh.dwFlags         = 0;
        psh.hInstance       = m_hInstance;
        psh.hwndParent      = m_hwndParent;
        psh.pszIcon         = MAKEINTRESOURCE(IDI_CSCUI_ICON);
        psh.pszCaption      = strSheetTitle;
        psh.nPages          = 0;
        psh.nStartPage      = 0;
        psh.phpage          = rghPages;

        //
        // Policy doesn't prevent user from configuring CSC cache.
        // Add the dynamic page(s).
        //
        CCoInit coinit;
        HRESULT hr = coinit.Result();
        if (SUCCEEDED(hr))
        {
            com_autoptr<IShellExtInit> psei;
            hr = CoCreateInstance(CLSID_OfflineFilesOptions,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IShellExtInit,
                                  reinterpret_cast<void **>(psei.getaddr()));

            if (SUCCEEDED(hr))
            {
                com_autoptr<IShellPropSheetExt> pspse;
                hr = psei->QueryInterface(IID_IShellPropSheetExt,
                                          reinterpret_cast<void **>(pspse.getaddr()));
                if (SUCCEEDED(hr))
                {
                    hr = pspse->AddPages(AddPropSheetPage, (LPARAM)&psh);
                }

                switch(PropertySheet(&psh))
                {
                    case ID_PSREBOOTSYSTEM:
                        //
                        // User wants to change enabled state of CSC.  Requires reboot.
                        //
                        if (IDYES == CscMessageBox(m_hwndParent, 
                                                   MB_YESNO | MB_ICONINFORMATION,
                                                   m_hInstance,
                                                   IDS_REBOOTSYSTEM))
                        {                                     
                            dwError = CSCUIRebootSystem();
                            if (ERROR_SUCCESS != dwError)
                            {
                                DBGERROR((TEXT("Reboot failed with error %d"), dwError));
                                //
                                // BUGBUG:  This needs a reboot-specific message.
                                //
                                CscWin32Message(m_hwndParent, dwError, CSCUI::SEV_ERROR);
                            }
                        }
                        dwError = ERROR_SUCCESS;  // Run() succeeded.
                        break;

                    case -1:
                    {
                        dwError = GetLastError();
                        DBGERROR((TEXT("PropertySheet failed with error %d"), dwError));
                        CscWin32Message(m_hwndParent, dwError, CSCUI::SEV_ERROR);
                        break;
                    }
                    default:
                        break;
                }
            }
            else
            {
                DBGERROR((TEXT("CoCreateInstance failed with result 0x%08X"), hr));
            }
        }
        else
        {
            DBGERROR((TEXT("CoInitialize failed with result 0x%08X"), hr));
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in COfflineFilesSheet::Run"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }

    return dwError;
}



//-----------------------------------------------------------------------------
// CscOptPropSheetExt
// This is the shell prop sheet extension implementation for creating
// the "Offline Folders" property page.
//-----------------------------------------------------------------------------
CscOptPropSheetExt::CscOptPropSheetExt(
    HINSTANCE hInstance,
    LONG *pDllRefCnt
    ) : m_cRef(0),
        m_pDllRefCnt(pDllRefCnt),
        m_hInstance(hInstance),
        m_pOfflineFoldersPg(NULL)
{
    DBGTRACE((DM_OPTDLG, DL_MID, TEXT("CscOptPropSheetExt::CscOptPropSheetExt")));
    InterlockedIncrement(m_pDllRefCnt);
}

CscOptPropSheetExt::~CscOptPropSheetExt(
    void
    )
{
    DBGTRACE((DM_OPTDLG, DL_MID, TEXT("CscOptPropSheetExt::~CscOptPropSheetExt")));
    delete m_pOfflineFoldersPg;
    InterlockedDecrement(m_pDllRefCnt);
}


HRESULT
CscOptPropSheetExt::QueryInterface(
    REFIID riid,
    void **ppvOut
    )
{
    DBGTRACE((DM_OPTDLG, DL_MID, TEXT("CscOptPropSheetExt::QueryInterface")));
    HRESULT hr = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;
    if (IID_IUnknown == riid ||
        IID_IShellExtInit == riid)
    {
        *ppvOut = static_cast<IShellExtInit *>(this);
    }
    else if (IID_IShellPropSheetExt == riid)
    {
        *ppvOut = static_cast<IShellPropSheetExt *>(this);
    }
    if (NULL != *ppvOut)
    {
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hr = NOERROR;
    }
    return hr;
}


ULONG
CscOptPropSheetExt::AddRef(
    void
    )
{
    DBGPRINT((DM_OPTDLG, DL_LOW, TEXT("CscOptPropSheetExt::AddRef %d -> %d"),
              m_cRef, m_cRef + 1));
    ULONG ulReturn = m_cRef + 1;
    InterlockedIncrement(&m_cRef);
    return ulReturn;
}

ULONG
CscOptPropSheetExt::Release(
    void
    )
{
    DBGPRINT((DM_OPTDLG, DL_LOW, TEXT("CscOptPropSheetExt::Release %d -> %d"),
              m_cRef, m_cRef - 1));
    ULONG ulReturn = m_cRef - 1;
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}

HRESULT
CscOptPropSheetExt::Initialize(
    LPCITEMIDLIST pidlFolder, 
    LPDATAOBJECT pdtobj,
    HKEY hkeyProgID
    )
{
    return NOERROR;
}


HRESULT
CscOptPropSheetExt::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam
    )
{
    DBGTRACE((DM_OPTDLG, DL_MID, TEXT("CscOptPropSheetExt::AddPages")));

    DBGASSERT((NULL != lpfnAddPage));
    DBGASSERT((NULL == m_pOfflineFoldersPg));

    HRESULT hr = E_FAIL; // Assume failure.

    try
    {
        if (!CConfig::GetSingleton().NoConfigCache())
        {
            HPROPSHEETPAGE hOfflineFoldersPg = NULL;
            m_pOfflineFoldersPg  = new COfflineFilesPage(m_hInstance, 
                                                         static_cast<IShellPropSheetExt *>(this));

            hr = AddPage(lpfnAddPage, lParam, *m_pOfflineFoldersPg, &hOfflineFoldersPg);
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in CscOptPropSheetExt::AddPages"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }

    return hr;
}


HRESULT
CscOptPropSheetExt::AddPage(
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam,
    const COfflineFilesPage& pg,
    HPROPSHEETPAGE *phPage
    )
{
    DBGTRACE((DM_OPTDLG, DL_MID, TEXT("CscOptPropSheetExt::AddPage")));
    DBGASSERT((NULL != lpfnAddPage));
    DBGASSERT((NULL != phPage));

    HRESULT hr = E_FAIL;

    PROPSHEETPAGE psp;

    psp.dwSize          = sizeof(psp);
    psp.dwFlags         = PSP_USECALLBACK | PSP_USEREFPARENT;
    psp.hInstance       = m_hInstance;
    psp.pszTemplate     = MAKEINTRESOURCE(pg.GetDlgTemplateID());
    psp.hIcon           = NULL;
    psp.pszTitle        = NULL;
    psp.pfnDlgProc      = (DLGPROC)pg.GetDlgProcPtr();
    psp.lParam          = (LPARAM)&pg;
    psp.pcRefParent     = (UINT *)m_pDllRefCnt;
    psp.pfnCallback     = (LPFNPSPCALLBACK)pg.GetCallbackFuncPtr();

    *phPage = CreatePropertySheetPage(&psp);
    if (NULL != *phPage)
    {
        if (!lpfnAddPage(*phPage, lParam))
        {
            DBGERROR((TEXT("AddPage Failed to add page.")));
            DestroyPropertySheetPage(*phPage);
            *phPage = NULL;
        }
    }
    else
    {
        DBGERROR((TEXT("CreatePropertySheetPage failed.")));
    }
    if (NULL != *phPage)
    {
        AddRef();
        hr = NOERROR;
    }
    return hr;
}


STDAPI 
COfflineFilesOptions_CreateInstance(
    REFIID riid, 
    void **ppv
    )
{
    HRESULT hr = E_NOINTERFACE;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IShellPropSheetExt) ||
        IsEqualIID(riid, IID_IShellExtInit))
    {
        //
        // Create the property sheet extension to handle the CSC options property
        // pages.
        //
        CscOptPropSheetExt *pse = new CscOptPropSheetExt(g_hInstance, &g_cRefCount);
        if (NULL != pse)
        {
            pse->AddRef();
            hr = pse->QueryInterface(riid, ppv);
            pse->Release();
        }
        else
            hr = E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        *ppv = NULL;
    }

    return hr;
}


//
// Initialize the "config items" object.
// This loads all of the user preference/policy information for the page when
// the page is first created.
//
void
COfflineFilesPage::CConfigItems::Load(
    void
    )
{
#define LOADCFG(i, f) m_rgItems[i].dwValue = DWORD(c.f(&m_rgItems[i].bSetByPolicy))

    CConfig& c = CConfig::GetSingleton();

    LOADCFG(iCFG_NOCONFIGCACHE,       NoConfigCache);
    LOADCFG(iCFG_SYNCATLOGOFF,        SyncAtLogoff);
    LOADCFG(iCFG_NOREMINDERS,         NoReminders);
    LOADCFG(iCFG_REMINDERFREQMINUTES, ReminderFreqMinutes);
    LOADCFG(iCFG_DEFCACHESIZE,        DefaultCacheSize);
    LOADCFG(iCFG_NOCACHEVIEWER,       NoCacheViewer);
    LOADCFG(iCFG_CSCENABLED,          CscEnabled);

#undef LOADCFG
}



