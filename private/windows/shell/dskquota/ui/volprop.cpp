///////////////////////////////////////////////////////////////////////////////
/*  File: volprop.cpp

    Description: Provides implementations for quota property pages.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
    08/01/97    Removed IDC_CBX_WARN_THRESHOLD from UI.              BrianAu
    11/27/98    Added logging checkboxes back in.                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include "dskquota.h"
#include "volprop.h"
#include "uihelp.h"
#include "registry.h"
#include "guidsp.h"
#include "uiutils.h"

#ifdef POLICY_MMC_SNAPIN
#   include "snapin.h"
#endif

//
// Context help IDs.
//
#pragma data_seg(".text", "CODE")
const static DWORD rgVolumePropPageHelpIDs[] =
{
    IDC_TRAFFIC_LIGHT,          IDH_TRAFFIC_LIGHT,
    IDC_TXT_QUOTA_STATUS,       IDH_TXT_QUOTA_STATUS,
    IDC_TXT_QUOTA_STATUS_LABEL, DWORD(-1),
    IDC_CBX_ENABLE_QUOTA,       IDH_CBX_ENABLE_QUOTA,
    IDC_CBX_DENY_LIMIT,         IDH_CBX_DENY_LIMIT,
    IDC_RBN_DEF_NOLIMIT,        IDH_RBN_DEF_NO_LIMIT,
    IDC_RBN_DEF_LIMIT,          IDH_RBN_DEF_LIMIT,
    IDC_EDIT_DEF_LIMIT,         IDH_EDIT_DEF_LIMIT,
    IDC_EDIT_DEF_THRESHOLD,     IDH_EDIT_DEF_THRESHOLD,
    IDC_CMB_DEF_LIMIT,          IDH_CMB_DEF_LIMIT,
    IDC_CMB_DEF_THRESHOLD,      IDH_CMB_DEF_THRESHOLD,
    IDC_BTN_DETAILS,            IDH_BTN_DETAILS,
    IDC_BTN_EVENTLOG,           IDH_BTN_EVENTLOG,
    IDC_CBX_LOG_OVERWARNING,    IDH_CBX_LOG_OVERWARNING,
    IDC_CBX_LOG_OVERLIMIT,      IDH_CBX_LOG_OVERLIMIT,
    IDC_TXT_DEFAULTS,           IDH_GRP_DEFAULTS,
    IDC_TXT_LOGGING,            DWORD(-1),
    IDC_TXT_WARN_LEVEL,         DWORD(-1),
    0,0
};


#pragma data_seg()

#ifdef POLICY_MMC_SNAPIN
const TCHAR c_szSnapInPrefs[] = TEXT("GPTEditorExtData");
#endif

extern TCHAR c_szWndClassDetailsView[]; // defined in details.cpp

/*
// BUGBUG:  This code has been disabled.
//          I've left in case we decide to launch the event viewer from
//          the volume prop page again. [brianau - 3/23/98]
//
const TCHAR c_szVerbOpen[]          = TEXT("Open");
const TCHAR c_szManagementConsole[] = TEXT("MMC.EXE");
const TCHAR c_szMMCInitFile[]       = TEXT("%SystemRoot%\\System32\\EVENTVWR.MSC");
*/


#define VPPM_FOCUS_ON_THRESHOLDEDIT  (WM_USER + 1)


///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::VolumePropPage

    Description: Constructor for a volume property page object.
        Initializes the members that hold volume quota data.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VolumePropPage::VolumePropPage(VOID)
    : m_dwQuotaState(0),
      m_dwQuotaLogFlags(0),
      m_idStatusUpdateTimer(0),
      m_dwLastStatusMsgID(0),
      m_cVolumeMaxBytes(NOLIMIT),
      m_pxbDefaultLimit(NULL),
      m_pxbDefaultThreshold(NULL),
      m_llDefaultQuotaThreshold(0),
      m_llDefaultQuotaLimit(0),
      m_idCtlNextFocus(-1)
{

}


///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::~VolumePropPage

    Description: Destructor for a volume property page object.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VolumePropPage::~VolumePropPage(
    VOID
    )
{
    delete m_pxbDefaultLimit;
    delete m_pxbDefaultThreshold;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::DlgProc

    Description: Static method called by windows to process messages for the
        property page dialog.  Since it's static, we have to save the "this"
        pointer in the window's USERDATA.

    Arguments: Standard WndProc-type arguments.

    Returns: Standard WndProc-type return values.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR APIENTRY
VolumePropPage::DlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    INT_PTR bResult = FALSE;

    //
    // Retrieve the "this" pointer from the dialog's userdata.
    // It was placed there in OnInitDialog().
    //
    VolumePropPage *pThis = (VolumePropPage *)GetWindowLongPtr(hDlg, DWLP_USER);

    try
    {
        switch(message)
        {
            case WM_INITDIALOG:
            {
                DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_INITDIALOG")));
                PROPSHEETPAGE *pPage = (PROPSHEETPAGE *)lParam;
                pThis = (VolumePropPage *)pPage->lParam;

                DBGASSERT((NULL != pThis));
                //
                // pThis pointer AddRef'd in AddPages().
                // Save it in the window's userdata.
                //
                SetWindowLongPtr(hDlg, DWLP_USER, (INT_PTR)pThis);
                bResult = pThis->OnInitDialog(hDlg, wParam, lParam);
                break;
            }

            case WM_SYSCOLORCHANGE:
                bResult = pThis->m_TrafficLight.ForwardMessage(message, wParam, lParam);
                break;

            case WM_NOTIFY:
                DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_NOTIFY")));
                bResult = pThis->OnNotify(hDlg, wParam, lParam);
                break;

            case WM_COMMAND:
                DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_COMMAND")));
                bResult = pThis->OnCommand(hDlg, wParam, lParam);
                break;

            case WM_HELP:
                DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_HELP")));
                bResult = pThis->OnHelp(hDlg, wParam, lParam);
                break;

            case WM_CONTEXTMENU:
                bResult = pThis->OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
                break;

            case WM_DESTROY:
                DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_DESTROY")));
                pThis->KillStatusUpdateTimer(hDlg);
                //
                // Nothing to do.
                //
                break;

            case WM_TIMER:
                DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_TIMER")));
                bResult = pThis->OnTimer(hDlg, wParam, lParam);
                break;

            case VPPM_FOCUS_ON_THRESHOLDEDIT:
                //
                // This is sort of a hack because of the way the prop sheet
                // code in comctl32 sets focus after a page has returned
                // PSNRET_INVALID.  It automatically activates the problem
                // page and sets focus to the FIRST control in the tab order.
                // Since the only failure we generate is from the threshold
                // exceeding the limit, I want to return focus to the threshold
                // edit control so that the user can directly change the offending
                // value.  Posting this custom message was the only way I
                // could get this to work. [brianau].
                //
                SetFocus((HWND)lParam);
                SendMessage((HWND)lParam, EM_SETSEL, 0, -1);
                break;

            default:
                break;
        }
    }
    catch(CAllocException& me)
    {
        //
        // Announce any out-of-memory errors associated with running the
        // volume Quota property page.
        //
        DiskQuotaMsgBox(GetDesktopWindow(),
                        IDS_OUTOFMEMORY,
                        IDS_TITLE_DISK_QUOTA,
                        MB_ICONERROR | MB_OK);
    }

    return bResult;
}






///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::OnInitDialog

    Description: Handler for WM_INITDIALOG.

    Arguments:
        hDlg - Dialog window handle.

        wParam - Handle of control to receive focus if we return FALSE.

        lParam - Pointer to PROPSHEETPAGE structure for the property page.

    Returns:
        TRUE  = Tells windows to assign focus to the control in wParam.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
    02/10/98    Converted from static to a virtual function to       BrianAu
                support addition of SnapInVolPropPage class.
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
VolumePropPage::OnInitDialog(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HRESULT hResult = NO_ERROR;
    DWORD dwSectorsPerCluster = 0;
    DWORD dwBytesPerSector    = 0;
    DWORD dwFreeClusters      = 0;
    DWORD dwTotalClusters     = 0;

    //
    // Load the volume's quota information into member variables.
    //
    hResult = RefreshCachedVolumeQuotaInfo();

    //
    // Calculate the volume's size.
    // We'll use this to limit user threshold and quota limit entries.
    //
    if (GetDiskFreeSpace(m_idVolume.ForParsing(),
                         &dwSectorsPerCluster,
                         &dwBytesPerSector,
                         &dwFreeClusters,
                         &dwTotalClusters))
    {
        m_cVolumeMaxBytes = (UINT64)dwSectorsPerCluster *
                            (UINT64)dwBytesPerSector *
                            (UINT64)dwTotalClusters;
    }

    //
    // Create the XBytes objects to manage the relationship between the
    // limit/threshold edit controls and their combo boxes.
    //
    m_pxbDefaultLimit     = new XBytes(hDlg,
                                       IDC_EDIT_DEF_LIMIT,
                                       IDC_CMB_DEF_LIMIT,
                                       m_llDefaultQuotaLimit);
    m_pxbDefaultThreshold = new XBytes(hDlg,
                                       IDC_EDIT_DEF_THRESHOLD,
                                       IDC_CMB_DEF_THRESHOLD,
                                       m_llDefaultQuotaThreshold);

    m_TrafficLight.Initialize(GetDlgItem(hDlg, IDC_TRAFFIC_LIGHT), IDR_AVI_TRAFFIC);

    InitializeControls(hDlg);

    return TRUE;  // Set focus to default control.
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::OnCommand

    Description: Handler for WM_COMMAND.

    Arguments:
        hDlg - Dialog window handle.

        wParam - ID of selected control and notification code.

        lParam - HWND of selected control.

    Returns:
        TRUE  = Message wasn't handled.
        FALSE = Message was handled.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
    08/01/97    Removed IDC_CBX_WARN_THRESHOLD from UI.              BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
VolumePropPage::OnCommand(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    INT_PTR bResult = TRUE;
    DWORD dwCtlId        = LOWORD(wParam);
    HWND hWndCtl         = (HWND)lParam;
    DWORD dwNotifyCode   = HIWORD(wParam);
    BOOL bIsChecked      = FALSE;
    BOOL bEnableApplyBtn = FALSE;

    switch(dwCtlId)
    {
        case IDC_CBX_ENABLE_QUOTA:
        {
            //
            // This is executed when the user checks or unchecks the
            // "Enable quota management checkbox.
            //
            bIsChecked = IsDlgButtonChecked(hDlg, IDC_CBX_ENABLE_QUOTA);
            //
            // Remember: Limit/Threshold Edit and combo boxes are enabled/disabled
            //           through XBytes::SetBytes().
            //
            if (!bIsChecked)
            {
                //
                // We always show "No Limit" if quotas are disabled.
                //
                m_pxbDefaultLimit->SetBytes(NOLIMIT);
                m_pxbDefaultThreshold->SetBytes(NOLIMIT);
            }
            else
            {
                m_pxbDefaultLimit->SetBytes(NOLIMIT == m_llDefaultQuotaLimit ?
                                            0 : m_llDefaultQuotaLimit);
                m_pxbDefaultThreshold->SetBytes(NOLIMIT == m_llDefaultQuotaThreshold ?
                                                0 : m_llDefaultQuotaThreshold);
            }
            CheckDlgButton(hDlg, IDC_RBN_DEF_NOLIMIT, NOLIMIT == m_pxbDefaultLimit->GetBytes());
            CheckDlgButton(hDlg, IDC_RBN_DEF_LIMIT,   BST_CHECKED != IsDlgButtonChecked(hDlg, IDC_RBN_DEF_NOLIMIT));
            CheckDlgButton(hDlg, IDC_CBX_DENY_LIMIT,  bIsChecked && DISKQUOTA_IS_ENFORCED(m_dwQuotaState));
            CheckDlgButton(hDlg,
                           IDC_CBX_LOG_OVERWARNING,
                           bIsChecked &&
                           DISKQUOTA_IS_LOGGED_USER_THRESHOLD(m_dwQuotaLogFlags));
            CheckDlgButton(hDlg,
                           IDC_CBX_LOG_OVERLIMIT,
                           bIsChecked &&
                           DISKQUOTA_IS_LOGGED_USER_LIMIT(m_dwQuotaLogFlags));

            EnableControls(hDlg);

            bEnableApplyBtn = TRUE;
            bResult = FALSE;
            break;
        }

        case IDC_CBX_DENY_LIMIT:
            bResult = FALSE;
            bEnableApplyBtn = TRUE;
            break;

        case IDC_RBN_DEF_NOLIMIT:
            DBGASSERT((IsDlgButtonChecked(hDlg, IDC_CBX_ENABLE_QUOTA)));

            if (m_pxbDefaultLimit->IsEnabled())
            {
                m_pxbDefaultThreshold->SetBytes(NOLIMIT);
                m_pxbDefaultLimit->SetBytes(NOLIMIT);
                bEnableApplyBtn = TRUE;
            }
            bResult = FALSE;
            break;

        case IDC_RBN_DEF_LIMIT:
            //
            // If the original threshold was -1 (no limit), set to 0.
            // Otherwise, set to the original value.
            //
            DBGASSERT((IsDlgButtonChecked(hDlg, IDC_CBX_ENABLE_QUOTA)));
            if (!m_pxbDefaultLimit->IsEnabled())
            {
                m_pxbDefaultLimit->SetBytes(NOLIMIT == m_llDefaultQuotaLimit ?
                                            0 : m_llDefaultQuotaLimit);
                m_pxbDefaultThreshold->SetBytes(NOLIMIT == m_llDefaultQuotaThreshold ?
                                                0 : m_llDefaultQuotaThreshold);


                EnableControls(hDlg);
                bEnableApplyBtn = TRUE;
            }
            bResult = FALSE;
            break;

        case IDC_EDIT_DEF_LIMIT:
        case IDC_EDIT_DEF_THRESHOLD:
            switch(dwNotifyCode)
            {
                case EN_UPDATE:
                    DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc; WM_COMMAND, EN_CHANGE")));
                    bResult = OnEditNotifyUpdate(hDlg, wParam, lParam);
                    bEnableApplyBtn = TRUE;
                    break;

                case EN_KILLFOCUS:
                    DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc; WM_COMMAND, EN_KILLFOCUS")));
                    bResult = OnEditNotifyKillFocus(hDlg, wParam, lParam);
                    break;

                case EN_SETFOCUS:
                    DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc; WM_COMMAND, EN_SETFOCUS")));
                    bResult = OnEditNotifySetFocus(hDlg, wParam, lParam);
                    break;

                default:
                    break;
            }
            break;

        case IDC_CMB_DEF_LIMIT:
        case IDC_CMB_DEF_THRESHOLD:
            switch(dwNotifyCode)
            {
                case CBN_SELCHANGE:
                    DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_COMMAND, CBN_CHANGE")));
                    bResult = OnComboNotifySelChange(hDlg, wParam, lParam);
                    bEnableApplyBtn = TRUE;
                    break;

                default:
                    break;
            }
            break;

        case IDC_BTN_DETAILS:
            bResult = OnButtonDetails(hDlg, wParam, lParam);
            break;

        case IDC_CBX_LOG_OVERLIMIT:
        case IDC_CBX_LOG_OVERWARNING:
            bEnableApplyBtn = TRUE;
            break;

/*
//
// BUGBUG: This code disabled until we decide to launch the event viewer
//         from the volume prop page.  Probably won't happen because we
//         can't define a quota-specific error type for NT events.
//         If we can't filter an event viewer list on quota-only events,
//         there's not much use in getting to the event viewer from here.
//         [brianau - 3/23/98]
//
        case IDC_BTN_EVENTLOG:
            bResult = OnButtonEventLog(hDlg, wParam, lParam);
            break;
*/

        default:
            break;
    }

    if (bEnableApplyBtn)
        PropSheet_Changed(GetParent(hDlg), hDlg);

    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::OnNotify

    Description: Handler for WM_NOTIFY.

    Arguments:
        hDlg - Dialog window handle.

        wParam - ID of selected control and notification code.

        lParam - HWND of selected control.

    Returns:
        TRUE  = Message wasn't handled.
        FALSE = Message was handled.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
VolumePropPage::OnNotify(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DBGTRACE((DM_VPROP, DL_MID, TEXT("VolumePropPage::OnNotify")));
    INT_PTR bResult = TRUE;

    switch(((NMHDR *)lParam)->code)
    {
        case PSN_SETACTIVE:
            DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_NOTIFY, PSN_SETACTIVE")));
            bResult = OnSheetNotifySetActive(hDlg, wParam, lParam);
            break;

        case PSN_APPLY:
            DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_NOTIFY, PSN_APPLY")));
            bResult = OnSheetNotifyApply(hDlg, wParam, lParam);
            break;

        case PSN_KILLACTIVE:
            DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_NOTIFY, PSN_KILLACTIVE")));
            bResult = OnSheetNotifyKillActive(hDlg, wParam, lParam);
            break;

        case PSN_RESET:
            DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_NOTIFY, PSN_RESET")));
            bResult = OnSheetNotifyReset(hDlg, wParam, lParam);
            break;

        default:
            break;
    }
    return bResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::OnSheetNotifySetActive

    Description: Handler for WM_NOTIFY - PSN_SETACTIVE.

    Arguments:
        hDlg - Dialog window handle.

        wParam - ID of control.

        lParam - Address of NMHDR structure.

    Returns:
        FALSE = Accept activation.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
VolumePropPage::OnSheetNotifySetActive(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DBGTRACE((DM_VPROP, DL_HIGH, TEXT("VolumePropPage::OnSheetNotifySetActive")));

    //
    // Update the status text and set the status update timer.
    //
    UpdateStatusIndicators(hDlg);
    SetStatusUpdateTimer(hDlg);

    if (IDC_EDIT_DEF_THRESHOLD == m_idCtlNextFocus)
    {
        //
        // Focus is being set as a result of an invalid entry
        // in the warning level field.  Force input focus to the
        // field and select the entire contents.  User can then just
        // enter a new value.
        //
        PostMessage(hDlg,
                    VPPM_FOCUS_ON_THRESHOLDEDIT,
                    0,
                    (LPARAM)GetDlgItem(hDlg, IDC_EDIT_DEF_THRESHOLD));

        m_idCtlNextFocus = -1;
    }

    return 0;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::OnSheetNotifyApply

    Description: Handler for WM_NOTIFY - PSN_APPLY.

    Arguments:
        hDlg - Dialog window handle.

        wParam - ID of control.

        lParam - Address of NMHDR structure.

    Returns:
        TRUE = PSN return value set using SetWindowLong.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
VolumePropPage::OnSheetNotifyApply(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DBGTRACE((DM_VPROP, DL_HIGH, TEXT("VolumePropPage::OnSheetNotifyApply")));
    HRESULT hResult  = NO_ERROR;
    LONG dwPSNReturn = PSNRET_NOERROR;
    INT idMsg        = -1;

    if (!IsDlgButtonChecked(hDlg, IDC_CBX_ENABLE_QUOTA) &&
        !DISKQUOTA_IS_DISABLED(m_dwQuotaState))
    {
        idMsg = IDS_DISABLE_QUOTA_WARNING;
    }
    else if (IsDlgButtonChecked(hDlg, IDC_CBX_ENABLE_QUOTA) &&
        DISKQUOTA_IS_DISABLED(m_dwQuotaState))
    {
        idMsg = IDS_ENABLE_QUOTA_WARNING;
    }

    if (-1 != idMsg)
    {
        //
        // User wants to disable or enable quotas.
        // Warn about what this means and let them know that
        // re-activation of quotas requires a quota file rebuild.
        //
        if (IDCANCEL == DiskQuotaMsgBox(hDlg,
                                        idMsg,
                                        IDS_TITLE_DISK_QUOTA,
                                        MB_ICONWARNING | MB_OKCANCEL))
        {
            //
            // User decided to not continue the action.
            // Restore the checkbox to it's previous setting and abort the
            // settings change.
            // Sending the message to our DlgProc resets the dependent controls
            // to their proper states.
            //
            CheckDlgButton(hDlg,
                           IDC_CBX_ENABLE_QUOTA,
                           !IsDlgButtonChecked(hDlg, IDC_CBX_ENABLE_QUOTA));

            SendMessage(hDlg,
                        WM_COMMAND,
                        (WPARAM)MAKELONG((WORD)IDC_CBX_ENABLE_QUOTA, (WORD)0),
                        (LPARAM)GetDlgItem(hDlg, IDC_CBX_ENABLE_QUOTA));

            dwPSNReturn = PSNRET_INVALID;
        }
    }

    if (PSNRET_NOERROR == dwPSNReturn)
    {
        //
        // We need to do this because if you activate the apply button
        // with Alt-A we receive PSN_APPLY before EN_KILLFOCUS.
        //
        m_pxbDefaultThreshold->OnEditKillFocus((LPARAM)GetDlgItem(hDlg, IDC_EDIT_DEF_THRESHOLD));
        m_pxbDefaultLimit->OnEditKillFocus((LPARAM)GetDlgItem(hDlg, IDC_EDIT_DEF_LIMIT));

        //
        // Ensure warning threshold is not above limit.
        //
        INT64 iThreshold = m_pxbDefaultThreshold->GetBytes();
        INT64 iLimit     = m_pxbDefaultLimit->GetBytes();

        if (iThreshold > iLimit)
        {
            TCHAR szLimit[40], szThreshold[40];
            XBytes::FormatByteCountForDisplay(iLimit, szLimit, ARRAYSIZE(szLimit));
            XBytes::FormatByteCountForDisplay(iThreshold, szThreshold, ARRAYSIZE(szThreshold));

            CString s(g_hInstDll, IDS_FMT_ERR_WARNOVERLIMIT, szThreshold, szLimit, szLimit);
            switch(DiskQuotaMsgBox(hDlg, s, IDS_TITLE_DISK_QUOTA, MB_ICONWARNING | MB_YESNO))
            {
                case IDYES:
                    m_pxbDefaultThreshold->SetBytes(iLimit);
                    break;

                case IDNO:
                    m_idCtlNextFocus = IDC_EDIT_DEF_THRESHOLD;
                    dwPSNReturn = PSNRET_INVALID;
                    break;
            }
        }
    }

    if (PSNRET_NOERROR == dwPSNReturn)
    {
        hResult = ApplySettings(hDlg);
        if (FAILED(hResult))
        {
             DiskQuotaMsgBox(hDlg,
                             IDS_APPLY_SETTINGS_ERROR,
                             IDS_TITLE_DISK_QUOTA,
                             MB_ICONERROR | MB_OK);
            dwPSNReturn = PSNRET_INVALID;
            InitializeControls(hDlg);
        }
    }

    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, dwPSNReturn);

    return TRUE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::OnSheetNotifyKillActive

    Description: Handler for WM_NOTIFY - PSN_KILLACTIVE.

    Arguments:
        hDlg - Dialog window handle.

        wParam - ID of control.

        lParam - Address of NMHDR structure.

    Returns:
        TRUE  = Invalid data entered.  Don't kill page.
        FALSE = All data is valid.  Ok to kill page.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
VolumePropPage::OnSheetNotifyKillActive(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DBGTRACE((DM_VPROP, DL_HIGH, TEXT("VolumePropPage::OnSheetNotifyKillActive")));
    BOOL bAllDataIsValid = TRUE;

    if (bAllDataIsValid)
    {
        KillStatusUpdateTimer(hDlg);
    }

    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, !bAllDataIsValid);

    //
    // Must release quota controller whenever the sheet is deactivated.
    // Without this we were holding open a handle to the volume.  This prevented
    // the disk check utility ("Tools" page) from accessing the volume.
    // Whenever we need an IDiskQuotaControl ptr we call GetQuotaController which
    // will create a new controller if necessary.
    //
    if (NULL != m_pQuotaControl)
    {
        m_pQuotaControl->Release();
        m_pQuotaControl = NULL;
    }

    return TRUE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::OnSheetNotifyReset

    Description: Handler for WM_NOTIFY - PSN_RESET.

    Arguments:
        hDlg - Dialog window handle.

        wParam - ID of control.

        lParam - Address of NMHDR structure.

    Returns:
        No return value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
VolumePropPage::OnSheetNotifyReset(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DBGTRACE((DM_VPROP, DL_HIGH, TEXT("VolumePropPage::OnSheetNotifyReset")));
    HRESULT hResult = NO_ERROR;

    //
    // Nothing to do right now.
    //

    return FALSE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::OnHelp

    Description: Handler for WM_HELP.  Displays context sensitive help.

    Arguments:
        lParam - Pointer to a HELPINFO structure.

    Returns: TRUE;

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/17/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
VolumePropPage::OnHelp(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, STR_DSKQUOUI_HELPFILE,
                HELP_WM_HELP, (DWORD_PTR)(LPTSTR) rgVolumePropPageHelpIDs);
    return TRUE;
}


INT_PTR
VolumePropPage::OnContextMenu(
    HWND hwndItem,
    int xPos,
    int yPos
    )
{
    int idCtl = GetDlgCtrlID(hwndItem);
    WinHelp(hwndItem,
            UseWindowsHelp(idCtl) ? NULL : STR_DSKQUOUI_HELPFILE,
            HELP_CONTEXTMENU,
            (DWORD_PTR)((LPTSTR)rgVolumePropPageHelpIDs));

    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::OnTimer

    Description: Handler for WM_TIMER.  Updates the quota status text and
        traffic light.

    Arguments:
        wParam - Timer ID.

    Returns: FALSE (0);

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/17/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
VolumePropPage::OnTimer(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (wParam == m_idStatusUpdateTimer)
    {
        UpdateStatusIndicators(hDlg);
    }

    return FALSE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::OnEditNotifyUpdate

    Description: Handler for WM_COMMAND, EN_UPDATE.
        Called whenever a character is entered in an edit control.

    Arguments:

    Returns: FALSE;

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/17/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
VolumePropPage::OnEditNotifyUpdate(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    XBytes *rgpxb[2] = { m_pxbDefaultLimit, m_pxbDefaultThreshold };
    const int iLIMIT     = 0;
    const int iTHRESHOLD = 1;
    int iCurrent         = iLIMIT;

    if (IDC_EDIT_DEF_THRESHOLD == LOWORD(wParam))
        iCurrent = iTHRESHOLD;

    if (NULL != rgpxb[iCurrent])
        rgpxb[iCurrent]->OnEditNotifyUpdate(lParam);

    return FALSE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::OnEditNotifyKillFocus

    Description: Handler for WM_COMMAND, EN_KILLFOCUS.
        Called whenever focus leaves an edit control.
        Validates the value in the edit control and adjusts it if necessary.

    Arguments:

    Returns: FALSE;

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/17/96    Initial creation.                                    BrianAu
    11/12/98    Added code to call XBytes::OnEditKillFocus.          BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
VolumePropPage::OnEditNotifyKillFocus(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    XBytes *rgpxb[2] = { m_pxbDefaultLimit, m_pxbDefaultThreshold };
    const int iLIMIT     = 0;
    const int iTHRESHOLD = 1;
    int iCurrent         = iLIMIT;

    if (IDC_EDIT_DEF_THRESHOLD == LOWORD(wParam))
        iCurrent = iTHRESHOLD;

    if (NULL != rgpxb[iCurrent])
        rgpxb[iCurrent]->OnEditKillFocus(lParam);

    return FALSE;
}


INT_PTR
VolumePropPage::OnEditNotifySetFocus(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    // Nothing to do.
    // BUGBUG:  Delete this method?
    //
    return FALSE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::OnComboNotifySelChange

    Description: Handler for WM_COMMAND, CBN_SELCHANGE.
        Called whenever the user selects the combo box.

    Arguments: Std DlgProc args.

    Returns: FALSE;

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/17/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
VolumePropPage::OnComboNotifySelChange(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    XBytes *rgpxb[2] = { m_pxbDefaultLimit, m_pxbDefaultThreshold };
    const int iLIMIT     = 0;
    const int iTHRESHOLD = 1;
    int iCurrent         = iLIMIT;

    if (IDC_CMB_DEF_THRESHOLD == LOWORD(wParam))
        iCurrent = iTHRESHOLD;

    if (NULL != rgpxb[iCurrent])
        rgpxb[iCurrent]->OnComboNotifySelChange(lParam);

    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::OnButtonDetails

    Description: Called when the user selects the "Details" button.
        If a details view is already active for this prop page, it is brought
        to the foreground.  If no details view is already active, a new one
        is created.

    Arguments: Standard DlgProc arguments.

    Returns:

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
VolumePropPage::OnButtonDetails(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (!ActivateExistingDetailsView())
    {
        //
        // This property page doesn't have an active details view.
        // Create one.  Note:  If something fails in the details view
        // creation, it isn't displayed.  The DetailsView code is
        // responsible for reporting any errors to the user.
        //
        // NOTE:  The VolumePropPage object never calls "delete"
        //        on the pDetailsView pointer.  The details view
        //        object must live on it's own (modeless) after it is created.
        //        If the VolumePropPage object (this object) is still alive
        //        when the details view object is destroyed, it will receive a
        //        WM_DETAILS_VIEW_DESTROYED message from the view object.  That's
        //        why we pass the hDlg in this constructor.  When this message
        //        is received, we set m_pDetailsView to NULL so that OnButtonDetails
        //        will know to create a new view object.
        //
        DetailsView *pDetailsView = new DetailsView;

        if (!pDetailsView->Initialize(m_idVolume))
        {
            //
            // Something failed.  Either out of memory or the view's thread
            // couldn't start.  Either way, the view won't run.
            // Need to call delete to clean up any partially-completed initialization.
            //
            delete pDetailsView;
        }
    }
    return FALSE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::OnButtonEventLog

    Description: Called when the user selects the "Event Log" button.
        Invokes the NT Event Viewer application.

    Arguments: Standard DlgProc arguments.

    Returns:

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/14/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifdef __NEVER__
//
// BUGBUG:  This code has been disabled.  I just left it in case we ever
//          decide to launch the event viewer from the volume prop page again.
//          [brianan - 3/23/98]
//
/*
INT_PTR
VolumePropPage::OnButtonEventLog(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    // Expand the %SystemRoot% env var in the path to the MMC
    // initialization file (Event viewer).
    //
    CString strMMCInitFile(c_szMMCInitFile);
    strMMCInitFile.ExpandEnvironmentStrings();

    HINSTANCE SEhInstance = ShellExecute(hDlg,
                                c_szVerbOpen,
                                c_szManagementConsole,
                                strMMCInitFile,
                                NULL,
                                SW_SHOWNORMAL);

    if ((DWORD)SEhInstance <= 32)
    {
        //
        // Something failed.
        //
        INT iMsg = 0;
        switch((DWORD)SEhInstance)
        {
            case 0:
                throw CAllocException();
                break;

            case ERROR_FILE_NOT_FOUND:
                iMsg = IDS_ERROR_FILE_NOT_FOUND;
                break;

            case ERROR_BAD_FORMAT:
                iMsg = IDS_ERROR_FILE_CORRUPT;
                break;

            case SE_ERR_DDEBUSY:
            case SE_ERR_DDEFAIL:
            case SE_ERR_DDETIMEOUT:
                iMsg = IDS_ERROR_DDE_EXECUTE;
                break;

            default:
                //
                // The rest of the ShellExecute errors shouldn't happen
                // in this situation.
                //
                break;
        }

        if (0 != iMsg)
        {
            //
            // Display error message.
            // Note that this assumes error string resources have a %1 embedded
            // for displaying the filename.  If they don't, the following
            // logic will need to be changed.
            //
            CString strMsg(g_hInstDll, iMsg, c_szManagementConsole);
            DiskQuotaMsgBox(GetDesktopWindow(),
                            strMsg,
                            IDS_TITLE_DISK_QUOTA,
                            MB_ICONERROR | MB_OK);
        }
    }
    return 0;
}
*/
#endif // __NEVER__

///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::ActivateExistingDetailsView

    Description: Called by OnButtonDetails to see if there's already a details
                 view active for this volume.  If there is, open it.

    Arguments: None.

    Returns:
        TRUE  = Existing details view was found and promoted to the foreground.
        FALSE = Either no existing view was found or an existing one could
                not be promoted to the foreground.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    02/25/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
VolumePropPage::ActivateExistingDetailsView(
    VOID
    ) const
{
    BOOL bResult = FALSE;
    CString strVolDisplayName;
    DetailsView::CreateVolumeDisplayName(m_idVolume, &strVolDisplayName);

    CString strDetailsViewTitle(g_hInstDll, IDS_TITLE_MAINWINDOW, (LPCTSTR)strVolDisplayName);

    HWND hwndDetailsView = FindWindow(c_szWndClassDetailsView,
                                      strDetailsViewTitle);

    if (NULL != hwndDetailsView)
    {
        //
        // Restore the details view and bring it to the front.
        //
        ShowWindow(hwndDetailsView, SW_RESTORE);
        bResult = SetForegroundWindow(hwndDetailsView);
    }

    return bResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::ApplySettings

    Description: Applies the current settings to the volume if they have
        changed from the original settings.

    Arguments:
        hDlg - Dialog window handle.

    Returns:
        NO_ERROR            - Success.
        E_INVALIDARG        - One of the settings was invalid.
        ERROR_ACCESS_DENIED (hr) - No WRITE access to quota device.
        E_FAIL              - Any other error.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
VolumePropPage::ApplySettings(
    HWND hDlg
    )
{
    HRESULT hResult         = NO_ERROR;
    DWORD dwStateSetting    = 0;
    DWORD dwLogFlagSettings = m_dwQuotaLogFlags;
    BOOL  bTranslated       = FALSE;
    LONGLONG llThreshold;
    LONGLONG llLimit;
    IDiskQuotaControl *pqc;

    hResult = GetQuotaController(&pqc);
    if (SUCCEEDED(hResult))
    {
        //
        // Set quota state if changed.
        //
        QuotaStateFromControls(hDlg, &dwStateSetting);
        if (dwStateSetting != (m_dwQuotaState & DISKQUOTA_STATE_MASK))
        {
            hResult = pqc->SetQuotaState(dwStateSetting);
            if (FAILED(hResult))
                goto apply_failed;

            m_dwQuotaState = dwStateSetting;
        }

        //
        // Set quota log flags if changed.
        //
        LogFlagsFromControls(hDlg, &dwLogFlagSettings);
        if (dwLogFlagSettings != m_dwQuotaLogFlags)
        {
            hResult = pqc->SetQuotaLogFlags(dwLogFlagSettings);
            if (FAILED(hResult))
                goto apply_failed;

            m_dwQuotaLogFlags = dwLogFlagSettings;
        }

        //
        // Get current default quota threshold and limit values.
        //
        if (IsDlgButtonChecked(hDlg, IDC_RBN_DEF_NOLIMIT))
        {
            llThreshold = NOLIMIT;
            llLimit     = NOLIMIT;
        }
        else
        {
            llThreshold = m_pxbDefaultThreshold->GetBytes();
            llLimit     = m_pxbDefaultLimit->GetBytes();
        }

        //
        // Set default quota threshold if changed.
        //
        if (llThreshold != m_llDefaultQuotaThreshold)
        {
            hResult = pqc->SetDefaultQuotaThreshold(llThreshold);
            if (FAILED(hResult))
                goto apply_failed;

            m_llDefaultQuotaThreshold = llThreshold;
        }

        //
        // Set default quota limit if changed.
        //
        if (llLimit != m_llDefaultQuotaLimit)
        {
            hResult = pqc->SetDefaultQuotaLimit(llLimit);
            if (FAILED(hResult))
                goto apply_failed;

            m_llDefaultQuotaLimit = llLimit;
        }

apply_failed:

        pqc->Release();
    }
    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::RefreshCachedVolumeInfo

    Description: Reads the volume's quota information and stores it in
        member variables.

    Arguments: None.

    Returns:
        NO_ERROR            - Success.
        ERROR_ACCESS_DENIED (hr) - No READ access to quota device.
        E_FAIL              - Any other error.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
VolumePropPage::RefreshCachedVolumeQuotaInfo(
    VOID
    )
{
    HRESULT hResult = NO_ERROR;

    IDiskQuotaControl *pqc;

    hResult = GetQuotaController(&pqc);
    if (SUCCEEDED(hResult))
    {
        //
        // Read quota state.
        //
        hResult = pqc->GetQuotaState(&m_dwQuotaState);
        if (FAILED(hResult))
            goto refresh_vol_info_failed;

        //
        // Read quota log flags.
        //
        hResult = pqc->GetQuotaLogFlags(&m_dwQuotaLogFlags);
        if (FAILED(hResult))
            goto refresh_vol_info_failed;

        //
        // Read default quota threshold.
        //
        hResult = pqc->GetDefaultQuotaThreshold(&m_llDefaultQuotaThreshold);
        if (FAILED(hResult))
            goto refresh_vol_info_failed;

        //
        // Read default quota limit.
        //
        hResult = pqc->GetDefaultQuotaLimit(&m_llDefaultQuotaLimit);

refresh_vol_info_failed:

        pqc->Release();
    }

    return hResult;
}


//
// Determine if a given disk quota policy value is set.
//
bool
VolumePropPage::SetByPolicy(
    LPCTSTR pszPolicyValue
    )
{
    DWORD dwData;
    DWORD dwType;
    DWORD cbData = sizeof(dwData);

    return (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE,
                                        REGSTR_KEY_POLICYDATA,
                                        pszPolicyValue,
                                        &dwType,
                                        &dwData,
                                        &cbData));
}                                         


HRESULT
VolumePropPage::EnableControls(
    HWND hwndDlg
    )
{    
    BOOL bQuotaEnabled = (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CBX_ENABLE_QUOTA));
    BOOL bEnable;

    //
    // "Enable quota management" checkbox.
    //
    //  Policy   Quota Enabled     Ctl Enabled
    //    0            0                1
    //    0            1                1
    //    1            0                0
    //    1            1                0
    //
    EnableWindow(GetDlgItem(hwndDlg, IDC_CBX_ENABLE_QUOTA), 
                 !SetByPolicy(REGSTR_VAL_POLICY_ENABLE));
    //
    // "Deny disk space..." checkbox.
    //
    //  Policy   Quota Enabled     Ctl Enabled
    //    0            0                0
    //    0            1                1
    //    1            0                0
    //    1            1                0
    //
    EnableWindow(GetDlgItem(hwndDlg, IDC_CBX_DENY_LIMIT), 
                 bQuotaEnabled && !SetByPolicy(REGSTR_VAL_POLICY_ENFORCE));
    //                 
    // Log event checkboxes
    //
    //  Policy   Quota Enabled     Ctl Enabled
    //    0            0                0
    //    0            1                1
    //    1            0                0
    //    1            1                0
    //
    EnableWindow(GetDlgItem(hwndDlg, IDC_CBX_LOG_OVERLIMIT),
                 bQuotaEnabled && !SetByPolicy(REGSTR_VAL_POLICY_LOGLIMIT));

    EnableWindow(GetDlgItem(hwndDlg, IDC_CBX_LOG_OVERWARNING),
                 bQuotaEnabled && !SetByPolicy(REGSTR_VAL_POLICY_LOGTHRESHOLD));

    //
    // "Do not limit disk usage" radio button
    // "Limit disk space to" radio button
    //
    //  Policy    Quota Enabled     No Limit    Ctl Enabled
    //    0            0               0            0
    //    0            0               1            0
    //    0            1               0            0
    //    0            1               1            1
    //    1            0               0            0
    //    1            0               1            0
    //    1            1               0            0
    //    1            1               1            0
    //
    bEnable = bQuotaEnabled && !SetByPolicy(REGSTR_VAL_POLICY_LIMIT);
              
    EnableWindow(GetDlgItem(hwndDlg, IDC_RBN_DEF_NOLIMIT),    bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_RBN_DEF_LIMIT),      bEnable);
    //
    //  "Limit disk space" edit and combo controls.
    //
    //  Policy    Quota Enabled     No Limit    Ctl Enabled
    //    0            0               0            0
    //    0            0               1            0
    //    0            1               0            1
    //    0            1               1            0
    //    1            0               0            0
    //    1            0               1            0
    //    1            1               0            0
    //    1            1               1            0
    //
    bEnable = bQuotaEnabled && 
              !SetByPolicy(REGSTR_VAL_POLICY_LIMIT) &&
              NOLIMIT != m_pxbDefaultLimit->GetBytes();

    EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_DEF_LIMIT),     bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_CMB_DEF_LIMIT),      bEnable);

    bEnable = bQuotaEnabled && 
              !SetByPolicy(REGSTR_VAL_POLICY_THRESHOLD) &&
              NOLIMIT != m_pxbDefaultThreshold->GetBytes();

    EnableWindow(GetDlgItem(hwndDlg, IDC_TXT_WARN_LEVEL),     bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_DEF_THRESHOLD), bEnable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_CMB_DEF_THRESHOLD),  bEnable);
    //
    // Miscellaneous text controls.
    //
    //  Quota Enabled     Ctl Enabled
    //        0                0
    //        1                1
    //
    EnableWindow(GetDlgItem(hwndDlg, IDC_TXT_DEFAULTS), bQuotaEnabled);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TXT_LOGGING),  bQuotaEnabled);

    return NOERROR;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::InitializeControls

    Description: Initializes the page controls based on the volume's
        quota settings.

    Arguments:
        hDlg - Dialog window handle.

    Returns:
        NO_ERROR - Always returns NO_ERROR.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
    08/01/97    Removed IDC_CBX_WARN_THRESHOLD from UI.              BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
VolumePropPage::InitializeControls(
    HWND hDlg
    )
{
    BOOL bQuotaEnabled  = !(DISKQUOTA_IS_DISABLED(m_dwQuotaState));
    BOOL bUnlimited     = (NOLIMIT == m_llDefaultQuotaLimit);

    //
    // Remember:  The Limit/threshold edit/combo controls are enabled/disabled
    //            through XBytes::SetBytes().
    //
    if (!bQuotaEnabled)
    {
        //
        // Quotas are disabled.  Display "No Limit" in threshold and
        // limit Xbytes controls and disable them.  Setting the value to
        // NOLIMIT does all this.
        //
        m_pxbDefaultThreshold->SetBytes(NOLIMIT);
        m_pxbDefaultLimit->SetBytes(NOLIMIT);
    }

    CheckDlgButton(hDlg,
                   IDC_CBX_ENABLE_QUOTA,
                   bQuotaEnabled);

    CheckDlgButton(hDlg,
                   IDC_CBX_DENY_LIMIT,
                   DISKQUOTA_IS_ENFORCED(m_dwQuotaState));

    CheckDlgButton(hDlg,
                   IDC_CBX_LOG_OVERWARNING,
                   !DISKQUOTA_IS_DISABLED(m_dwQuotaState) &&
                   DISKQUOTA_IS_LOGGED_USER_THRESHOLD(m_dwQuotaLogFlags));

    CheckDlgButton(hDlg,
                   IDC_CBX_LOG_OVERLIMIT,
                   !DISKQUOTA_IS_DISABLED(m_dwQuotaState) &&
                   DISKQUOTA_IS_LOGGED_USER_LIMIT(m_dwQuotaLogFlags));

    CheckDlgButton(hDlg, IDC_RBN_DEF_NOLIMIT,  bUnlimited);
    CheckDlgButton(hDlg, IDC_RBN_DEF_LIMIT,   !bUnlimited);

    EnableControls(hDlg);

    return NO_ERROR;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::UpdateStatusIndicators
    Description: Updates the "Status" text message at the top of the property
        page according to the actual quota system state.  Also updates the
        traffic light AVI clip.

    Arguments:
        hDlg - Dialog handle.

    Returns:
        Always returns NO_ERROR.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/18/96    Initial creation.                                    BrianAu
    08/28/96    Added stoplight icon.                                BrianAu
    09/10/96    Converted stoplight from an icon to an AVI clip.     BrianAu
                Call it a traffic light now.
    07/14/97    Removed distinct "enforce" and "tracking" messages   BrianAu
                and replaced with a single "active" message.
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
VolumePropPage::UpdateStatusIndicators(
    HWND hDlg
    )
{
    HRESULT hResult = NO_ERROR;
    DWORD dwMsgID   = IDS_STATUS_UNKNOWN;
    IDiskQuotaControl *pqc;

    hResult = GetQuotaController(&pqc);
    if (SUCCEEDED(hResult))
    {
        //
        // Update cached state information.
        //
        hResult = pqc->GetQuotaState(&m_dwQuotaState);
        pqc->Release();
        pqc = NULL;

    }
    if (SUCCEEDED(hResult))
    {
        //
        // Figure out what message to display.
        // "Rebuilding" overrides any other state.
        //
        if (DISKQUOTA_FILE_REBUILDING(m_dwQuotaState))
        {
            dwMsgID = IDS_STATUS_REBUILDING;
        }
        else switch(m_dwQuotaState & DISKQUOTA_STATE_MASK)
        {
            case DISKQUOTA_STATE_DISABLED:
                dwMsgID = IDS_STATUS_DISABLED;
                break;
            case DISKQUOTA_STATE_TRACK:
            case DISKQUOTA_STATE_ENFORCE:
                dwMsgID = IDS_STATUS_ACTIVE;
                break;
            default:
                break;
        }
    }

    if (dwMsgID != m_dwLastStatusMsgID)
    {
        //
        // Format the status text and configure the traffic light.
        //
        // Traffic light states:
        // RED             = Quotas disabled.
        // GREEN           = Quotas enabled.
        // Flashing YELLOW = Quota file is rebuilding.
        //
        INT iTrafficLightState = TrafficLight::GREEN;

        if (DISKQUOTA_FILE_REBUILDING(m_dwQuotaState))
            iTrafficLightState = TrafficLight::FLASHING_YELLOW;
        else if (DISKQUOTA_IS_DISABLED(m_dwQuotaState))
            iTrafficLightState = TrafficLight::RED;

        m_TrafficLight.Show(iTrafficLightState);

        CString strStatus(g_hInstDll, dwMsgID);
        SetWindowText(GetDlgItem(hDlg, IDC_TXT_QUOTA_STATUS), strStatus);

        m_dwLastStatusMsgID = dwMsgID;
        //
        // Re-initialize the controls based on the new state.
        //
        InitializeControls(hDlg);
    }

    return NO_ERROR;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::QuotaStateFromControls

    Description: Determines the quota state from the states of the individual
        controls on the page.

    Arguments:
        hDlg - Dialog's window handle.

        pdwState - Address of DWORD variable to receive state bits.

    Returns:
        Always returns NO_ERROR.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
VolumePropPage::QuotaStateFromControls(
    HWND hDlg,
    LPDWORD pdwState
    ) const
{
    DBGASSERT((NULL != pdwState));

    //
    // Set quota state if changed.
    //
    if (IsDlgButtonChecked(hDlg, IDC_CBX_ENABLE_QUOTA))
    {
        if (IsDlgButtonChecked(hDlg, IDC_CBX_DENY_LIMIT))
        {
            *pdwState = DISKQUOTA_STATE_ENFORCE;
        }
        else
            *pdwState = DISKQUOTA_STATE_TRACK;
    }
    else
        *pdwState = DISKQUOTA_STATE_DISABLED;

    return NO_ERROR;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::LogFlagsFromControls

    Description: Determines the log flags state from the states of the
        individual controls on the page.

    Arguments:
        hDlg - Dialog's window handle.

        pdwLogFlags - Address of DWORD variable to receive flag bits.

    Returns:
        Always returns NO_ERROR.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/19/96    Initial creation.                                    BrianAu
    08/01/97    Removed IDC_CBX_WARN_THRESHOLD from UI.              BrianAu
    11/20/98    Added "log over limit" and "log over warning"        BrianAu
                controls.
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
VolumePropPage::LogFlagsFromControls(
    HWND hDlg,
    LPDWORD pdwLogFlags
    ) const
{
    DBGASSERT((NULL != pdwLogFlags));
    DISKQUOTA_SET_LOG_USER_LIMIT(*pdwLogFlags,
                                 IsDlgButtonChecked(hDlg, IDC_CBX_LOG_OVERLIMIT));

    DISKQUOTA_SET_LOG_USER_THRESHOLD(*pdwLogFlags,
                                     IsDlgButtonChecked(hDlg, IDC_CBX_LOG_OVERWARNING));

    return NO_ERROR;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::TrafficLight::Initialize

    Description: Initializes the traffic light by opening the AVI clip.

    Arguments:
        hwndAnimateCtl - Handle to the animation control in the dialog.

        idAviClipRes - Resource ID of the AVI clip resource.

    Returns: Nothing.  If the thing doesn't load, it just won't play.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/10/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
VolumePropPage::TrafficLight::Initialize(
    HWND hwndAnimateCtl,
    INT idAviClipRes
    )
{
    DBGASSERT((NULL != hwndAnimateCtl));

    m_hwndAnimateCtl = hwndAnimateCtl;
    m_idAviClipRes   = idAviClipRes;

    Animate_Open(m_hwndAnimateCtl, MAKEINTRESOURCE(idAviClipRes));
//
// See note in TrafficLight::Show below.
//
//    Animate_SetFrameTime(m_hwndAnimateCtl, GetCaretBlinkTime());
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: VolumePropPage::TrafficLight::Show

    Description: Shows the traffic light in one of it's states.

    Arguments:
        eShow - One of the following enumerated constant values:

            OFF, YELLOW, RED, GREEN, FLASHING_YELLOW.

        NOTE:   THIS IS VERY IMPORTANT!!!

                The definitions of these constants MUST match as follows
                with the frame numbers in the AVI clip TRAFFIC.AVI.  If
                you change either, it won't work.

                Frame  Constant         Value
                ------ ---------------- ------
                  0     OFF               0
                  1     YELLOW            1
                  2     RED               2
                  3     GREEN             3
                N/A     FLASHING_YELLOW   4

                Flashing yellow is created by playing frames 0 and 1
                repeatedly.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/10/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
VolumePropPage::TrafficLight::Show(
    INT eShow
    )
{
    switch(eShow)
    {
        case OFF:
        case YELLOW:
        case RED:
        case GREEN:
            Animate_Seek(m_hwndAnimateCtl, eShow);
            break;

        case FLASHING_YELLOW:
            Animate_Seek(m_hwndAnimateCtl, YELLOW);
//
// NOTE:
//
// The common control guys didn't want me to add the ACM_SETFRAMETIME
// message so we can't vary the rate of the animation.  Since we can't
// have a fixed-rate blinking control, I'm just fixing the traffic light
// at yellow rather than flashing.  If we can ever add the frame time
// modification message to the animation control, we can activate
// this functionality.  A flashing light isn't worth the trouble of
// a unique implementation.  I really wanted this.  It looks cool.
//
// BUGBUG:  If we have time.  Make this work without the animation control.
//          Note that I tried just setting the icon.  But since the volume
//          status checking is done on the same thread that processes the
//          STM_SETICON messgae, flashing of the icon is erratic.
//
//            Animate_Play(m_hwndAnimateCtl, YELLOW, OFF, (UINT)-1);
            break;

        default:
            break;
    }
}


INT_PTR
VolumePropPage::TrafficLight::ForwardMessage(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return SendMessage(m_hwndAnimateCtl, uMsg, wParam, lParam);
}

#ifdef POLICY_MMC_SNAPIN

SnapInVolPropPage::~SnapInVolPropPage(
    void
    )
{
    if (NULL != m_pPolicy)
        m_pPolicy->Release();
}


BOOL
SnapInVolPropPage::OnInitDialog(
    HWND hDlg,
    UINT wParam,
    LONG lParam
    )
{
    DBGTRACE((DM_VPROP, DL_HIGH, TEXT("SnapInVolPropPage::OnInitDialog")));
    HRESULT hr = NO_ERROR;
    bool bRemovableMedia = false;

    //
    // Policy object should have already been created and initialized
    // by the creator of the property sheet.  See CSnapInCompData::PropPageThreadProc.
    //
    DBGASSERT((NULL != m_pPolicy));

    //
    // Retrieve and restore the previous dialog state from when the
    // user last used the snapin.  We use the GPTInfo structure for
    // convenience since we're storing the same information that is
    // used for setting policy.
    //
    DISKQUOTAPOLICYINFO dqpi;
    RegKey keyPref(HKEY_CURRENT_USER, REGSTR_KEY_DISKQUOTA);
    if (SUCCEEDED(keyPref.Open(KEY_READ)) &&
        SUCCEEDED(keyPref.GetValue(c_szSnapInPrefs, (LPBYTE)&dqpi, sizeof(dqpi))) &&
        sizeof(dqpi) == dqpi.cb)
    {
        m_llDefaultQuotaLimit     = dqpi.llDefaultQuotaLimit;
        m_llDefaultQuotaThreshold = dqpi.llDefaultQuotaThreshold;
        m_dwQuotaState            = dqpi.dwQuotaState;
        m_dwQuotaLogFlags         = dqpi.dwQuotaLogFlags;
        bRemovableMedia           = dqpi.bRemovableMedia;
    }
    else
        DBGERROR((TEXT("Error loading policy dialog information")));

    //
    // Create the XBytes objects to manage the relationship between the
    // limit/threshold edit controls and their combo boxes.
    //
    m_pxbDefaultLimit     = new XBytes(hDlg,
                                       IDC_EDIT_DEF_LIMIT,
                                       IDC_CMB_DEF_LIMIT,
                                       m_llDefaultQuotaLimit);

    m_pxbDefaultThreshold = new XBytes(hDlg,
                                       IDC_EDIT_DEF_THRESHOLD,
                                       IDC_CMB_DEF_THRESHOLD,
                                       m_llDefaultQuotaThreshold);

    InitializeControls(hDlg);

    return TRUE;  // Set focus to default control.
}


BOOL
SnapInVolPropPage::OnSheetNotifySetActive(
    HWND hDlg,
    UINT wParam,
    LONG lParam
    )
{
    DBGTRACE((DM_VPROP, DL_HIGH, TEXT("SnapInVolPropPage::OnSheetNotifySetActive")));
    return FALSE;
}


BOOL
SnapInVolPropPage::OnSheetNotifyApply(
    HWND hDlg,
    UINT wParam,
    LONG lParam
    )
{
    DBGTRACE((DM_VPROP, DL_HIGH, TEXT("SnapInVolPropPage::OnSheetNotifyApply")));
    LONG dwPSNReturn = PSNRET_NOERROR;
    HRESULT hr = NOERROR;

    DWORD dwState, dwLogFlags;
    LONGLONG llThreshold, llLimit;

    //
    // Policy object should have already been created and initialized
    // by the creator of the property sheet.  See CSnapInCompData::PropPageThreadProc.
    //
    DBGASSERT((NULL != m_pPolicy));

    //
    // Query the state of the dialog to get the quota settings.
    //
    QuotaStateFromControls(hDlg, &dwState);
    LogFlagsFromControls(hDlg, &dwLogFlags);
    if (IsDlgButtonChecked(hDlg, IDC_RBN_DEF_NOLIMIT))
    {
        llThreshold = NOLIMIT;
        llLimit     = NOLIMIT;
    }
    else
    {
        llThreshold = m_pxbDefaultThreshold->GetBytes();
        llLimit     = m_pxbDefaultLimit->GetBytes();
    }

    DISKQUOTAPOLICYINFO dqpi;
    dqpi.cb                      = sizeof(dqpi);
    dqpi.llDefaultQuotaThreshold = llThreshold;
    dqpi.llDefaultQuotaLimit     = llLimit;
    dqpi.dwQuotaState            = dwState;
    dqpi.dwQuotaLogFlags         = dwLogFlags;
    dqpi.bRemovableMedia         = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RBN_POLICY_REMOVABLE));

    //
    // Save the policy information to the registry.
    //
    hr = m_pPolicy->Save(&dqpi);
    if (FAILED(hr))
    {
        DBGERROR((TEXT("Error 0x%08X saving policy information"), hr));
        //
        // Show some UI?
        //
    }

    //
    // Save current dialog state so that we can restore it next time the
    // user opens the snapin.  This way, if the user typically sets
    // quotas to the same values, the UI will appear much more friendly.
    //
    RegKey keyPref(HKEY_CURRENT_USER, REGSTR_KEY_DISKQUOTA);
    if (FAILED(keyPref.Open(KEY_WRITE)) ||
        FAILED(keyPref.SetValue(c_szSnapInPrefs, (LPBYTE)&dqpi, sizeof(dqpi))))
    {
        DBGERROR((TEXT("Error saving policy dialog information")));
    }

    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, dwPSNReturn);

    return TRUE;
}


BOOL
SnapInVolPropPage::OnSheetNotifyKillActive(
    HWND hDlg,
    UINT wParam,
    LONG lParam
    )
{
    DBGTRACE((DM_VPROP, DL_HIGH, TEXT("SnapInVolPropPage::OnSheetNotifyKillActive")));
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
    return TRUE;
}



BOOL
SnapInVolPropPage::OnSheetNotifyReset(
    HWND hDlg,
    UINT wParam,
    LONG lParam
    )
{
    DBGTRACE((DM_VPROP, DL_HIGH, TEXT("SnapInVolPropPage::OnSheetNotifyReset")));
    return FALSE;
}


HRESULT
SnapInVolPropPage::CreateDiskQuotaPolicyObject(
    IDiskQuotaPolicy **ppOut
    )
{
    DBGTRACE((DM_VPROP, DL_HIGH, TEXT("SnapInVolPropPage::CreateDiskQuotaPolicyObject")));
    HRESULT hr = NOERROR;
    try
    {
        CDiskQuotaPolicy *pPolicy = new CDiskQuotaPolicy;
        hr = pPolicy->QueryInterface(IID_IDiskQuotaPolicy,
                                     reinterpret_cast<void **>(&m_pPolicy));
        if (SUCCEEDED(hr))
        {
            m_pPolicy->AddRef();
            *ppOut = m_pPolicy;
        }
    }
    catch(CAllocException& e)
    {
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        hr = E_UNEXPECTED;
    }
    return hr;
}

#endif // POLICY_MMC_SNAPIN
