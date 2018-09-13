///////////////////////////////////////////////////////////////////////////////
/*  File: userprop.cpp

    Description: Provides implementations for quota user property page.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
    06/25/98    Replaced AddUserPropSheet with AddUserDialog.        BrianAu
                Now that we're getting user info from the DS
                object picker, the prop sheet idea doesn't work
                so well.  A std dialog is better.
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h" // PCH
#pragma hdrstop

#include <lm.h>
#include "undo.h"
#include "userprop.h"
#include "userbat.h"
#include "uihelp.h"
#include "progress.h"
#include "uiutils.h"

//
// Context help IDs.
//
#pragma data_seg(".text", "CODE")
const static DWORD rgUserPropSheetHelpIDs[] =
{
    IDC_ICON_USER,               DWORD(-1),
    IDC_STATIC2,                 DWORD(-1),
    IDC_TXT_USERNAME,            IDH_TXT_USERNAME,
    IDC_TXT_SPACEUSED,           IDH_TXT_SPACEUSED,
    IDC_TXT_SPACEREMAINING,      IDH_TXT_SPACEREMAINING,
    IDC_LBL_SPACEUSED,           DWORD(-1),
    IDC_LBL_SPACEREMAINING,      DWORD(-1),
    IDC_ICON_USERSTATUS,         IDH_ICON_USERSTATUS,
    IDC_RBN_USER_NOLIMIT,        IDH_RBN_USER_NOLIMIT,
    IDC_RBN_USER_LIMIT,          IDH_RBN_USER_LIMIT,
    IDC_TXT_WARN_LEVEL,          DWORD(-1),
    IDC_EDIT_USER_LIMIT,         IDH_EDIT_USER_LIMIT,
    IDC_EDIT_USER_THRESHOLD,     IDH_EDIT_USER_THRESHOLD,
    IDC_CMB_USER_LIMIT,          IDH_CMB_USER_LIMIT,
    IDC_CMB_USER_THRESHOLD,      IDH_CMB_USER_THRESHOLD,
    0,0
};

#pragma data_seg()


//
// Messages for querying property page for icon images.
//
#define DQM_QUERY_STATUS_ICON      (WM_USER + 1)
#define DQM_QUERY_USER_ICON        (WM_USER + 2)
#define DQM_ENABLE_APPLY_BUTTON    (WM_USER + 3)

///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::UserPropSheet

    Description: Constructor for a user property sheet object.
        Initializes the members that hold user quota data.

    Arguments: None.

    Returns: Nothing.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
UserPropSheet::UserPropSheet(
    PDISKQUOTA_CONTROL pQuotaControl,
    const CVolumeID& idVolume,
    HWND hWndParent,
    LVSelection& LVSel,
    UndoList& UndoList
    ) : m_cVolumeMaxBytes(0),
        m_pQuotaControl(pQuotaControl),
        m_UndoList(UndoList),
        m_LVSelection(LVSel),
        m_hWndParent(hWndParent),
        m_bIsDirty(FALSE),
        m_bHomogeneousSelection(TRUE),  // Assume selection is homogeneous.
        m_pxbQuotaLimit(NULL),
        m_pxbQuotaThreshold(NULL),
        m_idVolume(idVolume),
        m_strPageTitle(g_hInstDll, IDS_TITLE_GENERAL),
        m_idCtlNextFocus(-1)
{
    DBGASSERT((NULL != m_pQuotaControl));
    DBGASSERT((NULL != m_hWndParent));
    DBGTRACE((DM_UPROP, DL_HIGH, TEXT("UserPropSheet::UserPropSheet")));

    m_llQuotaUsed      = 0;
    m_llQuotaLimit     = 0;
    m_llQuotaThreshold = 0;

    DBGASSERT((0 == iICON_USER_SINGLE));
    DBGASSERT((1 == iICON_USER_MULTIPLE));
    m_hIconUser[0]     = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_SINGLE_USER));
    m_hIconUser[1]     = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_MULTI_USER));

    DBGASSERT((0 == iICON_STATUS_OK));
    DBGASSERT((1 == iICON_STATUS_OVER_THRESHOLD));
    DBGASSERT((2 == iICON_STATUS_OVER_LIMIT));
    m_hIconStatus[0]   = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_OKBUBBLE));
    m_hIconStatus[1]   = LoadIcon(NULL, IDI_WARNING);
    m_hIconStatus[2]   = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_WARNERR));
}



UserPropSheet::~UserPropSheet(
    VOID
    )
{
    DBGTRACE((DM_UPROP, DL_HIGH, TEXT("UserPropSheet::~UserPropSheet")));
    INT i = 0;

    if (NULL != m_pQuotaControl)
        m_pQuotaControl->Release();

    if (NULL != m_pxbQuotaLimit)
        delete m_pxbQuotaLimit;
    if (NULL != m_pxbQuotaThreshold)
        delete m_pxbQuotaThreshold;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::Run

    Description: Creates and runs the property sheet dialog.
        This is the only method a client needs to call once the object
        is created.

    Arguments: None.

    Returns:
        NO_ERROR
        E_FAIL      - Couldn't create property sheet.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
UserPropSheet::Run(
    VOID
    )
{
    HRESULT hResult = NO_ERROR; // Assume success.

    PROPSHEETHEADER psh;
    PROPSHEETPAGE   psp;

    ZeroMemory(&psh, sizeof(psh));
    ZeroMemory(&psp, sizeof(psp));

    //
    // Define page.
    //
    psp.dwSize          = sizeof(PROPSHEETPAGE);
    psp.dwFlags         = PSP_USEREFPARENT | PSP_USETITLE;
    psp.hInstance       = g_hInstDll;
    psp.pszTemplate     = MAKEINTRESOURCE(IDD_PROPPAGE_USERQUOTA);
    psp.pszTitle        = (LPCTSTR)m_strPageTitle;
    psp.pfnDlgProc      = (DLGPROC)DlgProc;
    psp.lParam          = (LPARAM)this;
    psp.pcRefParent     = (UINT *)& g_cRefThisDll;

    //
    // Define sheet.
    //
    psh.dwSize          = sizeof(PROPSHEETHEADER);
    psh.dwFlags         = PSH_PROPSHEETPAGE;
    psh.hwndParent      = m_hWndParent;
    psh.hInstance       = g_hInstDll;
    psh.pszIcon         = NULL;
    psh.pszCaption      = NULL;
    psh.nPages          = 1;
    psh.nStartPage      = 0;
    psh.ppsp            = (LPCPROPSHEETPAGE)&psp;

    if (0 <= PropertySheet(&psh))
        hResult = E_FAIL;

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::DlgProc

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
UserPropSheet::DlgProc(
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
    UserPropSheet *pThis = (UserPropSheet *)GetWindowLongPtr(hDlg, DWLP_USER);

    try
    {
        switch(message)
        {
            case WM_INITDIALOG:
                DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_INITDIALOG")));
                bResult = OnInitDialog(hDlg, wParam, lParam);
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
                break;

            case WM_CLOSE:
            case WM_ENDSESSION:
                DestroyWindow(hDlg);
                break;

            case DQM_ENABLE_APPLY_BUTTON:
                pThis->m_bIsDirty = TRUE;
                bResult = PropSheet_Changed(GetParent(hDlg), hDlg);
                break;

            //
            // These two icon query messages are for automated testing
            // of the UI.
            //
            case DQM_QUERY_USER_ICON:
                bResult = pThis->QueryUserIcon(hDlg);
                break;

            case DQM_QUERY_STATUS_ICON:
                bResult = pThis->QueryUserStatusIcon(hDlg);
                break;

            default:
                break;
        }

    }
    catch(CAllocException& e)
    {
        DiskQuotaMsgBox(GetDesktopWindow(),
                        IDS_OUTOFMEMORY,
                        IDS_TITLE_DISK_QUOTA,
                        MB_ICONERROR | MB_OK);
    }

    return bResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::OnInitDialog

    Description: Handler for WM_INITDIALOG.  Retrieves the "this" pointer from
        the PROPSHEETPAGE structure (pointed to by lParam) and saves it in
        the window's USERDATA.

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
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
UserPropSheet::OnInitDialog(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HRESULT hResult = NO_ERROR;

    PROPSHEETPAGE *pPage = (PROPSHEETPAGE *)lParam;
    UserPropSheet *pThis = (UserPropSheet *)pPage->lParam;
    DWORD dwSectorsPerCluster = 0;
    DWORD dwBytesPerSector    = 0;
    DWORD dwFreeClusters      = 0;
    DWORD dwTotalClusters     = 0;

    DBGASSERT((NULL != pThis));

    //
    // Save "this" in the window's userdata.
    //
    SetWindowLongPtr(hDlg, DWLP_USER, (INT_PTR)pThis);

    //
    // Read quota info from NTFS.
    // For single selection, we cache the selected user's info.
    // For multi selection, we cache the defaults for the volume.
    // If adding a new user (count == 0), we also use the defaults for the
    // volume.
    //
    pThis->RefreshCachedQuotaInfo();

    //
    // Calculate the volume's size.
    // We'll use this to limit user threshold and quota limit entries.
    //
    if (GetDiskFreeSpace(pThis->m_idVolume.ForParsing(),
                         &dwSectorsPerCluster,
                         &dwBytesPerSector,
                         &dwFreeClusters,
                         &dwTotalClusters))
    {
        pThis->m_cVolumeMaxBytes = (LONGLONG)dwSectorsPerCluster *
                                   (LONGLONG)dwBytesPerSector *
                                   (LONGLONG)dwTotalClusters;
    }

    pThis->m_pxbQuotaLimit     = new XBytes(hDlg,
                                            IDC_EDIT_USER_LIMIT,
                                            IDC_CMB_USER_LIMIT,
                                            pThis->m_llQuotaLimit);
    pThis->m_pxbQuotaThreshold = new XBytes(hDlg,
                                            IDC_EDIT_USER_THRESHOLD,
                                            IDC_CMB_USER_THRESHOLD,
                                            pThis->m_llQuotaThreshold);

    pThis->InitializeControls(hDlg);

    return TRUE;  // Set focus to default control.
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::RefreshCachedQuotaInfo

    Description: Reads the quota limit, threshold and used values from the
        property sheet's user object.  If multiple users are selected,
        only the first one is read.

    Arguments: None.

    Returns: Result of read operation.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
UserPropSheet::RefreshCachedQuotaInfo(
    VOID
    )
{
    HRESULT hResult = NO_ERROR;
    PDISKQUOTA_USER pUser = NULL;
    INT cSelectedUsers = m_LVSelection.Count();

    m_LVSelection.Retrieve(0, &pUser);

    //
    // Read quota threshold.  Multi-user selections use the volume's default.
    //
    if (1 == cSelectedUsers)
    {
        hResult = pUser->GetQuotaThreshold(&m_llQuotaThreshold);
    }
    else
    {
        hResult = m_pQuotaControl->GetDefaultQuotaThreshold(&m_llQuotaThreshold);
    }
    if (FAILED(hResult))
        goto refresh_quota_info_failed;

    //
    // Read quota limit.  Multi-user selections use the volume's default.
    //
    if (1 == cSelectedUsers)
    {
        hResult = pUser->GetQuotaLimit(&m_llQuotaLimit);
    }
    else
    {
        hResult = m_pQuotaControl->GetDefaultQuotaLimit(&m_llQuotaLimit);
    }
    if (FAILED(hResult))
        goto refresh_quota_info_failed;


    //
    // Read quota used.
    //
    if (1 == cSelectedUsers)
    {
        hResult = pUser->GetQuotaUsed(&m_llQuotaUsed);
    }
    else
    {
        m_llQuotaUsed = 0;
    }

refresh_quota_info_failed:
    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::OnCommand

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
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
UserPropSheet::OnCommand(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DWORD dwCtlId        = LOWORD(wParam);
    HWND hWndCtl         = (HWND)lParam;
    DWORD dwNotifyCode   = HIWORD(wParam);
    INT_PTR bResult      = FALSE;

    switch(dwCtlId)
    {
        case IDC_TXT_USERNAME:
            if (EN_SETFOCUS == dwNotifyCode && IDC_EDIT_USER_THRESHOLD == m_idCtlNextFocus)
            {
                //
                // Focus is being set as a result of an invalid entry
                // in the warning level field.  Force input focus to the
                // field and select the entire contents.  User can then just
                // enter a new value.
                //
                SetFocus(GetDlgItem(hDlg, IDC_EDIT_USER_THRESHOLD));
                SendDlgItemMessage(hDlg, IDC_EDIT_USER_THRESHOLD, EM_SETSEL, 0, -1);
                m_idCtlNextFocus = -1;
            }
            break;

        case IDC_RBN_USER_NOLIMIT:
            if (m_pxbQuotaThreshold->IsEnabled())
            {
                //
                // This is simple.  Just set both the limit and threshold controls
                // to "no limit".
                //
                m_pxbQuotaThreshold->SetBytes(NOLIMIT);
                m_pxbQuotaLimit->SetBytes(NOLIMIT);
                m_bIsDirty = TRUE;
            }
            break;

        case IDC_RBN_USER_LIMIT:
        {
            LONGLONG llValue;

            //
            // This handler needs some logic.  We have to handle several
            // scenarios/rules with this one.
            // 1. Single vs. Multiple selection.
            // 2. Single selection for Administrator account.
            // 3. Multi selection homogeneous/heterogenous with respect to
            //    quota limit and threshold values.
            // 4. Can't display "No Limit" in edit controls when they're active.
            // 5. Use volume defaults for new user and hetergenous multi-select.
            //
            if (!m_pxbQuotaThreshold->IsEnabled())
            {
                enum use_types { USE_CACHED, USE_VOLDEF, USE_NOLIMIT };

                INT iUseAsValue = USE_CACHED;
                INT cSelected   = m_LVSelection.Count();

                ///////////////////////////////////////////////////////////////
                // First set the quota limit controls.
                ///////////////////////////////////////////////////////////////
                if (0 == cSelected)                     // Adding new user...
                {
                    iUseAsValue = USE_VOLDEF;
                }
                else if (1 == cSelected)                // One user selected...
                {
                    PDISKQUOTA_USER pUser = NULL;
                    m_LVSelection.Retrieve(0, &pUser);
                    if (UserIsAdministrator(pUser))
                    {
                        //
                        // If user is administrator, the limit is always "No Limit".
                        // This will disable the "Limit" controls and prevent
                        // user from setting a limit on this account.
                        //
                        iUseAsValue = USE_NOLIMIT;
                    }
                    else if (NOLIMIT == m_llQuotaLimit)
                    {
                        //
                        // Account isn't Administrator AND the limit is NOLIMIT.
                        // Use the volume's default "new user" limit value.
                        //
                        iUseAsValue = USE_VOLDEF;
                    }
                }
                else if (!m_bHomogeneousSelection || NOLIMIT == m_llQuotaLimit) // Multiple user.
                {
                    //
                    // Multiple non-homogeneous users get the volume's default limit.
                    // Multiple homogeneous users get their current cached setting unless
                    // the cached setting is NOLIMIT; in which case, they get the
                    // volume's defaults.
                    //
                    iUseAsValue = USE_VOLDEF;
                }

                //
                // Set the proper quota limit value in the edit/combo box control.
                //
                llValue = 0;
                switch(iUseAsValue)
                {
                    case USE_VOLDEF:
                        m_pQuotaControl->GetDefaultQuotaLimit(&llValue);
                        //
                        // If default is NOLIMIT, display 0 MB.  We can't display an
                        // "editable" No Limit in the edit control.  Only numbers.
                        //
                        if (NOLIMIT == llValue)
                            llValue = 0;
                        break;

                    case USE_NOLIMIT:
                        llValue = NOLIMIT;
                        break;

                    case USE_CACHED:
                        llValue = m_llQuotaLimit;
                        break;
                }

                m_pxbQuotaLimit->SetBytes(llValue);


                ///////////////////////////////////////////////////////////////
                // Now the threshold controls...
                ///////////////////////////////////////////////////////////////
                llValue = 0;
                iUseAsValue       = USE_CACHED;
                if (0 == cSelected)
                {
                    iUseAsValue = USE_VOLDEF;
                }
                else if (1 == cSelected)
                {
                    if (NOLIMIT == m_llQuotaThreshold)
                    {
                        iUseAsValue = USE_VOLDEF;
                    }
                }
                else if (!m_bHomogeneousSelection || NOLIMIT == m_llQuotaThreshold)
                {
                    iUseAsValue = USE_VOLDEF;
                }

                //
                // Set the proper quota threshold value in the edit/combo box control.
                //
                switch(iUseAsValue)
                {
                    case USE_VOLDEF:
                        m_pQuotaControl->GetDefaultQuotaThreshold(&llValue);
                        //
                        // If default is NOLIMIT, display 0 MB.  We can't display an
                        // "editable" No Limit in the edit control.  Only numbers.
                        //
                        if (NOLIMIT == llValue)
                            llValue = 0;
                        break;

                    case USE_NOLIMIT:
                        llValue = NOLIMIT;
                        break;

                    case USE_CACHED:
                        llValue = m_llQuotaThreshold;
                        break;
                }

                m_pxbQuotaThreshold->SetBytes(llValue);

                m_bIsDirty = TRUE;
            }
            break;
        }

        case IDC_EDIT_USER_LIMIT:
        case IDC_EDIT_USER_THRESHOLD:
            switch(dwNotifyCode)
            {
                case EN_UPDATE:
                    DBGPRINT((DM_WND, DL_MID, TEXT("OnCommand, EN_CHANGE")));
                    bResult = OnEditNotifyUpdate(hDlg, wParam, lParam);
                    m_bIsDirty = TRUE;
                    break;

                case EN_KILLFOCUS:
                    DBGPRINT((DM_WND, DL_MID, TEXT("OnCommand, EN_KILLFOCUS")));
                    bResult = OnEditNotifyKillFocus(hDlg, wParam, lParam);
                    break;

                default:
                    break;
            }
            break;

        case IDC_CMB_USER_LIMIT:
        case IDC_CMB_USER_THRESHOLD:
            switch(dwNotifyCode)
            {
                case CBN_SELCHANGE:
                    DBGPRINT((DM_WND, DL_MID, TEXT("OnCommand, CBN_CHANGE")));
                    bResult = OnComboNotifySelChange(hDlg, wParam, lParam);
                    m_bIsDirty = TRUE;
                    break;

                default:
                    break;
            }
            break;

        default:
            bResult = TRUE;  // Didn't handle message.
            break;
    }


    if (m_bIsDirty)
        PropSheet_Changed(GetParent(hDlg), hDlg);

    return bResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::OnNotify

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
UserPropSheet::OnNotify(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    INT_PTR bResult = FALSE;

    switch(((NMHDR *)lParam)->code)
    {
        case PSN_SETACTIVE:
            DBGPRINT((DM_WND, DL_MID, TEXT("OnNotify, PSN_SETACTIVE")));
            bResult = OnSheetNotifySetActive(hDlg, wParam, lParam);
            break;

        case PSN_APPLY:
            DBGPRINT((DM_WND, DL_MID, TEXT("OnNotify, PSN_APPLY")));
            bResult = OnSheetNotifyApply(hDlg, wParam, lParam);
            break;

        case PSN_KILLACTIVE:
            DBGPRINT((DM_WND, DL_MID, TEXT("OnNotify, PSN_KILLACTIVE")));
            bResult = OnSheetNotifyKillActive(hDlg, wParam, lParam);
            break;

        case PSN_RESET:
            DBGPRINT((DM_WND, DL_MID, TEXT("OnNotify, PSN_RESET")));
            //
            // Fall through.
            //
        default:
            break;
    }
    return bResult;
}




///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::OnSheetNotifySetActive

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
UserPropSheet::OnSheetNotifySetActive(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
    return TRUE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::OnSheetNotifyApply

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
UserPropSheet::OnSheetNotifyApply(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HRESULT hResult  = NO_ERROR;
    LONG dwPSNReturn = PSNRET_NOERROR;

    //
    // Only apply settings if the "Apply" button is enabled indicating
    // that something has been changed.  No need to apply unchanged
    // settings when the OK button is pressed.
    //
    if (m_bIsDirty)
    {
        if (PSNRET_NOERROR == dwPSNReturn)
        {
            //
            // We need to do this because if you activate the apply button
            // with Alt-A we receive PSN_APPLY before EN_KILLFOCUS.
            //
            m_pxbQuotaThreshold->OnEditKillFocus((LPARAM)GetDlgItem(hDlg, IDC_EDIT_USER_THRESHOLD));
            m_pxbQuotaLimit->OnEditKillFocus((LPARAM)GetDlgItem(hDlg, IDC_EDIT_USER_LIMIT));

            //
            // Ensure warning threshold is not above limit.
            //
            INT64 iThreshold = m_pxbQuotaThreshold->GetBytes();
            INT64 iLimit     = m_pxbQuotaLimit->GetBytes();

            if (NOLIMIT != iLimit && iThreshold > iLimit)
            {
                TCHAR szLimit[40], szThreshold[40];
                XBytes::FormatByteCountForDisplay(iLimit, szLimit, ARRAYSIZE(szLimit));
                XBytes::FormatByteCountForDisplay(iThreshold, szThreshold, ARRAYSIZE(szThreshold));

                CString s(g_hInstDll, IDS_FMT_ERR_WARNOVERLIMIT, szThreshold, szLimit, szLimit);
                switch(DiskQuotaMsgBox(hDlg, s, IDS_TITLE_DISK_QUOTA, MB_ICONWARNING | MB_YESNO))
                {
                    case IDYES:
                        m_pxbQuotaThreshold->SetBytes(iLimit);
                        break;

                    case IDNO:
                        //
                        // This m_idCtlNextFocus hack stuff is here because I can't get
                        // the @#$%! prop sheet to return focus to the threshold control.
                        // The only way I've been able to get this to happen is to
                        // cache this ID value then on the EN_SETFOCUS generated when
                        // the page is activated, set focus to the control.
                        // Gross but it works without too much hassle. [brianau]
                        //
                        m_idCtlNextFocus = IDC_EDIT_USER_THRESHOLD;
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
                INT idMsg   = IDS_UNKNOWN_ERROR;
                UINT uFlags = MB_OK;
                switch(hResult)
                {
                    case E_FAIL:
                        idMsg = IDS_WRITE_ERROR;
                        uFlags |= MB_ICONERROR;
                        break;

                    default:
                        switch(HRESULT_CODE(hResult))
                        {

//                      case ERROR_USER_EXISTS:
//                          idMsg = IDS_NOADD_EXISTING_USER;
//                          uFlags |= MB_ICONWARNING;
//                          break;
//
// BUGBUG:  Still valid?  [brianau - 5/27/98]
//
                            case ERROR_NO_SUCH_USER:
                                idMsg = IDS_NOADD_UNKNOWN_USER;
                                uFlags |= MB_ICONWARNING;
                                break;

                            case ERROR_ACCESS_DENIED:
                                idMsg  = IDS_NO_WRITE_ACCESS;
                                uFlags |= MB_ICONWARNING;
                                break;

                            default:
                            uFlags |= MB_ICONERROR;
                            break;
                        }
                        break;
                }

                DiskQuotaMsgBox(GetDesktopWindow(),
                                idMsg,
                                IDS_TITLE_DISK_QUOTA,
                                uFlags);

                dwPSNReturn = PSNRET_INVALID;
            }
            else
            {
                m_bIsDirty = FALSE;
            }
        }
    }

    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, dwPSNReturn);

    return TRUE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::OnSheetNotifyKillActive

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
UserPropSheet::OnSheetNotifyKillActive(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    BOOL bAllDataIsValid = TRUE;

    //
    // No sheet-level validation performed at this time.
    //
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, !bAllDataIsValid);

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::OnHelp

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
UserPropSheet::OnHelp(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, STR_DSKQUOUI_HELPFILE,
                HELP_WM_HELP, (DWORD_PTR)(LPTSTR) rgUserPropSheetHelpIDs);
    return TRUE;
}


INT_PTR
UserPropSheet::OnContextMenu(
    HWND hwndItem,
    int xPos,
    int yPos
    )
{
    int idCtl = GetDlgCtrlID(hwndItem);
    WinHelp(hwndItem,
            UseWindowsHelp(idCtl) ? NULL : STR_DSKQUOUI_HELPFILE,
            HELP_CONTEXTMENU,
            (DWORD_PTR)((LPTSTR)rgUserPropSheetHelpIDs));

    return FALSE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropPage::OnEditNotifyUpdate

    Description: Handler for WM_COMMAND, EN_UPDATE.
        Called whenever a character is entered in an edit control.

    Arguments:

    Returns: FALSE;

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/03/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
UserPropSheet::OnEditNotifyUpdate(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    XBytes *pxb = NULL;

    switch(LOWORD(wParam))
    {
        case IDC_EDIT_USER_LIMIT:
            pxb = m_pxbQuotaLimit;
            break;

        case IDC_EDIT_USER_THRESHOLD:
            pxb = m_pxbQuotaThreshold;
            break;

        default:
            break;
    }

    if (NULL != pxb)
        pxb->OnEditNotifyUpdate(lParam);

    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::OnEditNotifyKillFocus

    Description: Handler for WM_COMMAND, EN_KILLFOCUS.
        Called whenever focus leaves an edit control.
        Validates the value in the edit control and adjusts it if necessary.

    Arguments:

    Returns: FALSE;

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/17/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
UserPropSheet::OnEditNotifyKillFocus(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    XBytes *pxb = NULL;

    switch(LOWORD(wParam))
    {
        case IDC_EDIT_USER_LIMIT:
            pxb = m_pxbQuotaLimit;
            break;

        case IDC_EDIT_USER_THRESHOLD:
            pxb = m_pxbQuotaThreshold;
            break;

        default:
            break;
    }

    if (NULL != pxb)
        pxb->OnEditKillFocus(lParam);

    return FALSE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropPage::OnComboNotifySelChange

    Description: Handler for WM_COMMAND, CBN_SELCHANGE.
        Called whenever the user selects the combo box.

    Arguments: Std DlgProc args.

    Returns: FALSE;

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/03/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
UserPropSheet::OnComboNotifySelChange(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    XBytes *pxb = NULL;

    switch(LOWORD(wParam))
    {
        case IDC_CMB_USER_LIMIT:
            pxb = m_pxbQuotaLimit;
            break;

        case IDC_CMB_USER_THRESHOLD:
            pxb = m_pxbQuotaThreshold;
            break;

        default:
            break;
    }
    if (NULL != pxb)
       pxb->OnComboNotifySelChange(lParam);

    return FALSE;
}




///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::ApplySettings

    Description: Applies the current settings to the user's quota information
        if they have not changed from the original settings.

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
    01/24/98    Added bUndo argument.                                BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
UserPropSheet::ApplySettings(
    HWND hDlg,
    bool bUndo    // Default == true.
    )
{
    HRESULT hResult         = NO_ERROR;
    BOOL  bTranslated       = FALSE;
    com_autoptr<DISKQUOTA_USER> ptrUser;
    UINT cUsers             = m_LVSelection.Count();
    UINT i                  = 0;
    LONGLONG llThreshold;
    LONGLONG llLimit;
    CAutoSetRedraw autoredraw(m_hWndParent);

    if (bUndo)
        m_UndoList.Clear();  // Clear current undo list.

    //
    // Determine what threshold and limit to apply.
    //
    if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RBN_USER_NOLIMIT))
    {
        llThreshold = NOLIMIT;
        llLimit     = NOLIMIT;
    }
    else
    {
        llThreshold = m_pxbQuotaThreshold->GetBytes();
        llLimit     = m_pxbQuotaLimit->GetBytes();
    }

    if (cUsers > 1)
    {
        //
        // Create batch object and do batch update for multiple users.
        //
        com_autoptr<DISKQUOTA_USER_BATCH> ptrBatch;

        hResult = m_pQuotaControl->CreateUserBatch(ptrBatch.getaddr());
        if (SUCCEEDED(hResult))
        {
            for (i = 0; i < cUsers; i++)
            {
                m_LVSelection.Retrieve(i, ptrUser.getaddr());
                if (bUndo)
                {
                    //
                    // Add an entry to the undo list.
                    //
                    LONGLONG LimitUndo;
                    LONGLONG ThresholdUndo;
                    ptrUser->GetQuotaThreshold(&ThresholdUndo);
                    ptrUser->GetQuotaLimit(&LimitUndo);
                    //
                    // Use a local autoptr to ensure proper release of
                    // iface in case adding to the undo list throws an exception.
                    // On success, disown the real ptr so that the object
                    // stays with the undo list.
                    //
                    com_autoptr<DISKQUOTA_USER> ptrQuotaUser(ptrUser);
                    ptrUser->AddRef();
                    m_UndoList.Add(new UndoModify(ptrUser, ThresholdUndo, LimitUndo));
                    ptrQuotaUser.disown();
                }

                ptrUser->SetQuotaThreshold(llThreshold, FALSE);

                if (UserIsAdministrator(ptrUser) && NOLIMIT != llLimit)
                {
                    //
                    // User is the Administrator account AND
                    // We're trying to set the limit to something other than NOLIMIT.
                    // Can't set a limit on the administrator account.
                    //
                    DiskQuotaMsgBox(GetDesktopWindow(),
                                    IDS_CANT_SET_ADMIN_LIMIT,
                                    IDS_TITLE_DISK_QUOTA,
                                    MB_ICONWARNING | MB_OK);
                }
                else
                {
                    //
                    // OK to set quota limit.
                    //
                    ptrUser->SetQuotaLimit(llLimit, FALSE);
                }

                ptrBatch->Add(ptrUser);
            }

            hResult = ptrBatch->FlushToDisk();
        }
    }
    else
    {
        //
        // Do single user update or add new user.
        //
        m_LVSelection.Retrieve(0, ptrUser.getaddr());
        DBGASSERT((NULL != ptrUser.get()));

        if (bUndo)
        {
            //
            // Add an entry to the undo list.
            //
            LONGLONG LimitUndo;
            LONGLONG ThresholdUndo;
            ptrUser->GetQuotaThreshold(&ThresholdUndo);
            ptrUser->GetQuotaLimit(&LimitUndo);
            //
            // Use local autoptr to ensure proper release of iface ptr if
            // an exception is thrown.  Disown real ptr on success.
            //
            com_autoptr<DISKQUOTA_USER> ptrQuotaUser(ptrUser);
            ptrUser->AddRef();
            m_UndoList.Add(new UndoModify(ptrUser, ThresholdUndo, LimitUndo));
            ptrQuotaUser.disown();
        }

        if (llThreshold != m_llQuotaThreshold)
        {
            hResult = ptrUser->SetQuotaThreshold(llThreshold, TRUE);

            if (FAILED(hResult))
                goto apply_failed;

            m_llQuotaThreshold = llThreshold;
        }

        if (llLimit != m_llQuotaLimit)
        {
            hResult = ptrUser->SetQuotaLimit(llLimit, TRUE);
            if (FAILED(hResult))
                goto apply_failed;

            m_llQuotaLimit = llLimit;
        }

        //
        // Update the user's status icon and %used to reflect the new settings.
        //
        UpdateUserStatusIcon(hDlg,
                             m_llQuotaUsed,
                             m_llQuotaThreshold,
                             m_llQuotaLimit);

        UpdateSpaceUsed(hDlg,
                        m_llQuotaUsed,
                        m_llQuotaLimit,
                        1);
    }

    //
    // Update the listview item(s) so the user sees a visual response to
    // pressing the "Apply" button.
    //
    autoredraw.Set(false);
    for (i = 0; i < cUsers; i++)
    {
        INT iItem = 0;
        m_LVSelection.Retrieve(i, &iItem);
        ListView_Update(m_hWndParent, iItem);
    }
    autoredraw.Set(true);
    InvalidateRect(m_hWndParent, NULL, FALSE);

apply_failed:

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::InitializeControls

    Description: Initializes the page controls based on the user's
        quota settings.

    Arguments:
        hDlg - Dialog window handle.

    Returns:
        NO_ERROR - Always returns NO_ERROR.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
UserPropSheet::InitializeControls(
    HWND hDlg
    )
{
    PDISKQUOTA_USER pUser = NULL;
    UINT cUsers           = m_LVSelection.Count();

    if (1 == cUsers)
    {
        //
        // Initialize controls for a single user.
        //
        m_LVSelection.Retrieve(0, &pUser);

        //
        // Configure the Limit/NoLimit radio buttons.
        // Must examine the current threshold rather than the limit because of the
        // special-case for the Administrator account.  That account can have a
        // threshold value but quota limit must always be "No Limit".
        //
        CheckDlgButton(hDlg, IDC_RBN_USER_LIMIT,   NOLIMIT != m_llQuotaThreshold);
        CheckDlgButton(hDlg, IDC_RBN_USER_NOLIMIT, NOLIMIT == m_llQuotaThreshold);
        if (UserIsAdministrator(pUser))
        {
            //
            // Override initialization of Quota Limit control with "No Limit".
            //
            m_pxbQuotaLimit->SetBytes(NOLIMIT);
        }

        //
        // Note that the XBytes controls have already been set for single-user.
        // See OnInitDialog().
        //

        //
        // Configure the remaining dialog controls.
        //
        UpdateUserName(hDlg, pUser);
        UpdateSpaceUsed(hDlg, m_llQuotaUsed, m_llQuotaLimit, cUsers);
        UpdateUserStatusIcon(hDlg,
                             m_llQuotaUsed,
                             m_llQuotaThreshold,
                             m_llQuotaLimit);
    }
    else
    {
        //
        // Initialize controls for multiple users.
        //
        LONGLONG llLimit         = 0;
        LONGLONG llLastLimit     = 0;
        LONGLONG llThreshold     = 0;
        LONGLONG llLastThreshold = 0;
        LONGLONG llUsed          = 0;
        LONGLONG llTotalUsed     = 0;

        //
        // Add up the total usage by all users.
        //
        for (UINT i = 0; i < cUsers; i++)
        {
            m_LVSelection.Retrieve(i, &pUser);
            pUser->GetQuotaLimit(&llLimit);
            pUser->GetQuotaThreshold(&llThreshold);
            pUser->GetQuotaUsed(&llUsed);

            llTotalUsed += llUsed;
            if (m_bHomogeneousSelection)
            {
                //
                // Determine if at least one user has a different
                // threshold or limit. If all are the same, we can display
                // the values in the edit controls.  Otherwise, we default
                // to "No Limit".  Radio buttons don't provide an
                // indeterminate state like checkboxes.
                //
                if (i > 0 &&
                    (llLimit != llLastLimit ||
                     llThreshold != llLastThreshold))
                {
                    m_bHomogeneousSelection = FALSE;
                }
                else
                {
                    llLastLimit     = llLimit;
                    llLastThreshold = llThreshold;
                }
            }
        }

        //
        // If all selected objects have the same limit and threshold,
        // set the cached data to represent multiple-selection.
        // If any one is different, we stick with the volume's default
        // values set in RefreshCachedQuotaInfo().
        //
        if (m_bHomogeneousSelection)
        {
            m_llQuotaThreshold = llLastThreshold;
            m_llQuotaLimit     = llLastLimit;
        }
        else
        {
            //
            // Since not all selected users have the same limit/thresold,
            // the number we're displaying will be a change for at least
            // one user.  Activate the "Apply" button.
            //
            //
            PostMessage(hDlg, DQM_ENABLE_APPLY_BUTTON, 0, 0);
        }

        m_pxbQuotaThreshold->SetBytes(m_llQuotaThreshold);
        m_pxbQuotaLimit->SetBytes(m_llQuotaLimit);

        //
        // Configure the Limit/NoLimit radio buttons.
        //
        CheckDlgButton(hDlg,
                       IDC_RBN_USER_NOLIMIT,
                       NOLIMIT == m_llQuotaThreshold);
        CheckDlgButton(hDlg,
                       IDC_RBN_USER_LIMIT,
                       NOLIMIT != m_llQuotaThreshold);

        UpdateUserName(hDlg, cUsers);
        UpdateSpaceUsed(hDlg, llTotalUsed, NOLIMIT, cUsers);
        //
        // Don't display any user status icon for multi-users.
        //
    }


    //
    // If "No Limit" radio button is checked, set limit and threshold controls
    // to the "No Limit" state (disabled and displaying "No Limit" text).
    // This may override any setting we made above.
    //
    if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RBN_USER_NOLIMIT))
    {
        m_pxbQuotaThreshold->SetBytes(NOLIMIT);
        m_pxbQuotaLimit->SetBytes(NOLIMIT);
    }

    //
    // Set user icon.
    //
    SendMessage(GetDlgItem(hDlg, IDC_ICON_USER),
                STM_SETICON,
                (WPARAM)m_hIconUser[1 == cUsers ? iICON_USER_SINGLE :
                                                  iICON_USER_MULTIPLE],
                0);

    //
    // Force the property sheet to disable the "Apply" button.
    // The way I have set up the "Apply" enabling logic through OnCommand(),
    // merely initializing the edit controls on the page causes the Apply
    // button to become enabled.  Since the user hasn't changed anything
    // yet, it should be disabled.
    //
    m_bIsDirty = FALSE;
    PropSheet_UnChanged(GetParent(hDlg), hDlg);

    return NO_ERROR;
}





///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::QueryUserStatusIcon


    Description: This function is provided for automated testing of the UI.
        It is used by test scripts to determine which user status icon is
        currently displayed.

    Arguments:
        hDlg - Dialog handle.


    Returns: -1 = No icon displayed.
              0 = "Everything OK" icon.
              1 = Threshold exceded icon.
              2 = Limit exceded icon.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT
UserPropSheet::QueryUserStatusIcon(
    HWND hDlg
    ) const
{
    HICON hicon = (HICON)SendMessage(GetDlgItem(hDlg, IDC_ICON_USERSTATUS),
                                    STM_GETICON,
                                    0, 0);

    for (UINT i = 0; i < cSTATUS_ICONS; i++)
    {
        if (hicon == m_hIconStatus[i])
            return i;
    }
    return -1;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::QueryUserIcon


    Description: This function is provided for automated testing of the UI.
        It is used by test scripts to determine which user status icon is
        currently displayed.

    Arguments:
        hDlg - Dialog handle.


    Returns: -1 = No icon displayed.
              0 = Single-user icon.
              1 = Multi-user icon.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT
UserPropSheet::QueryUserIcon(
    HWND hDlg
    ) const
{
    HICON hicon = (HICON)SendMessage(GetDlgItem(hDlg, IDC_ICON_USER),
                                     STM_GETICON,
                                     0, 0);

    for (UINT i = 0; i < cUSER_ICONS; i++)
    {
        if (hicon == m_hIconUser[i])
            return i;
    }
    return -1;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::UpdateUserStatusIcon

    Description: Updates the quota status icon in the dialog box.  This icon
        must match the icon displayed in the listview for the selected user.

    Arguments:
        hDlg - Dialog handle.

        iUsed - Quota bytes charged to user.

        iThreshold - Quota warning threshold (bytes).

        iLimit - User's quota limit.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
UserPropSheet::UpdateUserStatusIcon(
    HWND hDlg,
    LONGLONG iUsed,
    LONGLONG iThreshold,
    LONGLONG iLimit
    )
{
    //
    // Set the user status icon if user is exceding the
    // quota threshold or the limit.  This is the same icon that is
    // displayed in the listview status column.  This logic must
    // mirror that used in DetailsView::GetDispInfo_Image().
    //
    INT iIcon = iICON_STATUS_OK;
    if (NOLIMIT != iLimit && iUsed > iLimit)
    {
        iIcon = iICON_STATUS_OVER_LIMIT;
    }
    else if (NOLIMIT != iThreshold && iUsed > iThreshold)
    {
        iIcon = iICON_STATUS_OVER_THRESHOLD;
    }

    SendMessage(GetDlgItem(hDlg, IDC_ICON_USERSTATUS),
                STM_SETICON,
                (WPARAM)m_hIconStatus[iIcon],
                0);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::UpdateUserName

    Description: Updates the Domain\Name text with the user's domain name
        and account name strings.  This method is called for a single-user
        selection.

        Also sets the property sheet title text.

    Arguments:
        hDlg - Dialog handle.

        pUser - Address of user's IDiskQuotaUser interface.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
    08/05/97    Added code to set prop sheet title text.             BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
UserPropSheet::UpdateUserName(
    HWND hDlg,
    PDISKQUOTA_USER pUser
    )
{
    DBGASSERT((NULL != pUser));
    //
    // Display the user name, or some status text
    // if the name hasn't been resolved.
    //
    CString strLogonName;
    DWORD dwAccountStatus = 0;

    pUser->GetAccountStatus(&dwAccountStatus);

    if (DISKQUOTA_USER_ACCOUNT_RESOLVED == dwAccountStatus)
    {
        //
        // User account name has been resolved.  Display it.
        //
        TCHAR szLogonName[MAX_USERNAME];
        TCHAR szDisplayName[MAX_FULL_USERNAME];

        pUser->GetName(NULL, 0,
                       szLogonName, ARRAYSIZE(szLogonName),
                       szDisplayName, ARRAYSIZE(szDisplayName));

        if (TEXT('\0') != szLogonName[0])
        {
            if (TEXT('\0') != szDisplayName[0])
            {
                strLogonName.Format(g_hInstDll,
                                    IDS_FMT_DISPLAY_LOGON,
                                    szDisplayName,
                                    szLogonName);
            }
            else
            {
                strLogonName = szLogonName;
            }
        }
    }
    else
    {
        //
        // User account name has not been resolved or cannot
        // be resolved for some reason.  Display appropriate
        // status text.  This is the same text displayed in the
        // listview when the user's name has not been resolved.
        //
        INT idText = IDS_USER_ACCOUNT_UNKNOWN;

        switch(dwAccountStatus)
        {
            case DISKQUOTA_USER_ACCOUNT_UNAVAILABLE:
                idText = IDS_USER_ACCOUNT_UNAVAILABLE;
                break;

            case DISKQUOTA_USER_ACCOUNT_DELETED:
                idText = IDS_USER_ACCOUNT_DELETED;
                break;

            case DISKQUOTA_USER_ACCOUNT_INVALID:
                idText = IDS_USER_ACCOUNT_INVALID;
                break;

            case DISKQUOTA_USER_ACCOUNT_UNRESOLVED:
                idText = IDS_USER_ACCOUNT_UNRESOLVED;
                break;

            case DISKQUOTA_USER_ACCOUNT_UNKNOWN:
            default:
                break;
        }
        strLogonName.Format(g_hInstDll, idText);
    }
    SetDlgItemText(hDlg, IDC_TXT_USERNAME, strLogonName);

    //
    // Format and draw the prop sheet title string.
    //
    CString strSheetTitle(g_hInstDll,
                          IDS_TITLE_EDIT_USER,
                          (LPCTSTR)strLogonName);

    PropSheet_SetTitle(GetParent(hDlg), 0, (LPCTSTR)strSheetTitle);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::UpdateUserName

    Description: Replaces the user Domain\Name text with a message showing
        how many users are selected.   This is used for multi-user selections
        where no single user name is applicable.

        Also sets the property sheet title text.

    Arguments:
        hDlg - Dialog handle.

        cUsers - Number of users represented in the property dialog.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
    08/05/97    Added code to set prop sheet title text.             BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
UserPropSheet::UpdateUserName(
    HWND hDlg,
    INT cUsers
    )
{
    //
    // Hide name edit control.  Can't display names for all users.
    // Display "Multiple Quota Users." instead.
    //
    CString strText(g_hInstDll, IDS_TITLE_MULTIUSER, cUsers);
    SetDlgItemText(hDlg, IDC_TXT_USERNAME, strText);

    //
    // Set the title of the property sheet.
    //
    CString strSheetTitle(g_hInstDll, IDS_TITLE_EDIT_MULTIUSER);
    PropSheet_SetTitle(GetParent(hDlg), 0, (LPCTSTR)strSheetTitle);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: UserPropSheet::UpdateSpaceUsed

    Description: Updates the "space used" and "remaining" fields on the user
        property sheet.

    Arguments:
        hDlg - Dialog handle.

        iUsed - Quota bytes charged to user(s).

        iLimit - User's quota limit.

        cUsers - Number of users represented in the property dialog.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
UserPropSheet::UpdateSpaceUsed(
    HWND hDlg,
    LONGLONG iUsed,
    LONGLONG iLimit,
    INT cUsers
    )
{
    TCHAR szText[80];

    //
    // Display - Used: 999XB (99%)
    //
    XBytes::FormatByteCountForDisplay(iUsed,
                                      szText, ARRAYSIZE(szText));
    CString strText(szText);
    if (1 == cUsers)
    {
        //
        // Only single-user page gets (99%) appended.
        // Pct quota is meaningless for multiple users.
        //
        if (0 != iLimit && NOLIMIT != iLimit)
        {
            UINT iPct = (INT)((iUsed * 100) / iLimit);

            strText.Format(g_hInstDll,
                           IDS_QUOTA_USED_SINGLEUSER,
                           szText,
                           iPct);
        }
    }

    SetDlgItemText(hDlg,
                  IDC_TXT_SPACEUSED,
                  strText);


    //
    // Display - Remaining: 999XB
    //
    strText = szText;
    if (NOLIMIT != iLimit)
    {
        LONGLONG iAmount = 0;

        if (iUsed <= iLimit)
            iAmount = iLimit - iUsed;

        XBytes::FormatByteCountForDisplay(iAmount,
                                          strText.GetBuffer(80), 80);
        strText.ReleaseBuffer();
    }
    else
    {
        //
        // Display "N/A" if limit is NOLIMIT.
        //
        strText.Format(g_hInstDll, IDS_NOT_APPLICABLE);
    }

    SetDlgItemText(hDlg,
                   IDC_TXT_SPACEREMAINING,
                   strText);
}


