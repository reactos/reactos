///////////////////////////////////////////////////////////////////////////////
/*  File: adusrdlg.cpp

    Description: Provides implementations for the "Add User" dialog.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h" // PCH
#pragma hdrstop

#include <lm.h>
#include "undo.h"
#include "adusrdlg.h"
#include "uihelp.h"
#include "progress.h"
#include "uiutils.h"


//
// Context help IDs.
//
#pragma data_seg(".text", "CODE")
const static DWORD rgAddUserDialogHelpIDs[] =
{
    IDC_ICON_USER,               DWORD(-1),
    IDC_STATIC2,                 DWORD(-1),
    IDC_TXT_DEFAULTS,            DWORD(-1),
    IDC_TXT_USERNAME,            IDH_TXT_USERNAME,
    IDC_TXT_SPACEUSED,           IDH_TXT_SPACEUSED,
    IDC_TXT_SPACEREMAINING,      IDH_TXT_SPACEREMAINING,
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


///////////////////////////////////////////////////////////////////////////////
/*  Function: AddUserDialog::AddUserDialog

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
AddUserDialog::AddUserDialog(
    PDISKQUOTA_CONTROL pQuotaControl,
    const CVolumeID& idVolume,
    HINSTANCE hInstance,
    HWND hwndParent,
    HWND hwndDetailsLV,
    UndoList& UndoList
    ) : m_cVolumeMaxBytes(0),
        m_pQuotaControl(pQuotaControl),
        m_idVolume(idVolume),
        m_UndoList(UndoList),
        m_hInstance(hInstance),
        m_hwndParent(hwndParent),
        m_hwndDetailsLV(hwndDetailsLV),
        m_pxbQuotaLimit(NULL),
        m_pxbQuotaThreshold(NULL),
        m_llQuotaLimit(0),
        m_llQuotaThreshold(0),
        m_pSelectionList(NULL),  // Object instance doesn't own this memory.
        m_cfSelectionList((CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST))
{
    DBGASSERT((NULL != m_pQuotaControl));
    DBGASSERT((NULL != m_hwndParent));
    DBGTRACE((DM_UPROP, DL_HIGH, TEXT("AddUserDialog::AddUserDialog")));

    DBGASSERT((0 == iICON_USER_SINGLE));
    DBGASSERT((1 == iICON_USER_MULTIPLE));
    m_hIconUser[0]     = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_SINGLE_USER));
    m_hIconUser[1]     = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_MULTI_USER));
}



AddUserDialog::~AddUserDialog(
    VOID
    )
{
    DBGTRACE((DM_UPROP, DL_HIGH, TEXT("AddUserDialog::~AddUserDialog")));
    INT i = 0;

    if (NULL != m_pQuotaControl)
        m_pQuotaControl->Release();

    if (NULL != m_pxbQuotaLimit)
        delete m_pxbQuotaLimit;
    if (NULL != m_pxbQuotaThreshold)
        delete m_pxbQuotaThreshold;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: AddUserDialog::Run

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
AddUserDialog::Run(
    VOID
    )
{
    //
    // Invoke the standard object picker dialog.
    //
    IDataObject *pdtobj = NULL;
    HRESULT hr = BrowseForUsers(m_hwndParent, &pdtobj);
    if (S_OK == hr)
    {
        //
        // Retrieve the data object representing the selected user objects.
        //
        FORMATETC fe = { m_cfSelectionList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM stg;
        hr = pdtobj->GetData(&fe, &stg);
        {
            //
            // Cache the data obj ptr so the dialog can have access.
            //
            m_pSelectionList = (DS_SELECTION_LIST *)GlobalLock(stg.hGlobal);

            if (NULL != m_pSelectionList)
            {
                hr = (HRESULT) DialogBoxParam(m_hInstance,
                                              MAKEINTRESOURCE(IDD_ADDUSER),
                                              m_hwndParent,
                                              (DLGPROC)DlgProc,
                                              (LPARAM)this);
                GlobalUnlock(stg.hGlobal);
                m_pSelectionList = NULL;
            }
            ReleaseStgMedium(&stg);
        }
        pdtobj->Release();
    }
    return hr;

}



///////////////////////////////////////////////////////////////////////////////
/*  Function: AddUserDialog::DlgProc

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
INT_PTR CALLBACK
AddUserDialog::DlgProc(
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
    AddUserDialog *pThis = (AddUserDialog *)GetWindowLongPtr(hDlg, DWLP_USER);

    try
    {
        switch(message)
        {
            case WM_INITDIALOG:
                DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_INITDIALOG")));
                pThis = (AddUserDialog *)lParam;
                DBGASSERT((NULL != pThis));
                //
                // Save "this" in the window's userdata.
                //
                SetWindowLongPtr(hDlg, DWLP_USER, (INT_PTR)pThis);
                bResult = pThis->OnInitDialog(hDlg, wParam, lParam);
                break;

            case WM_COMMAND:
                DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_COMMAND")));
                bResult = pThis->OnCommand(hDlg, wParam, lParam);
                break;

            case WM_CONTEXTMENU:
                bResult = pThis->OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
                break;

            case WM_HELP:
                DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_HELP")));
                bResult = pThis->OnHelp(hDlg, wParam, lParam);
                break;

            case WM_DESTROY:
                DBGPRINT((DM_WND, DL_MID, TEXT("DlgProc: WM_DESTROY")));
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

INT_PTR
AddUserDialog::OnInitDialog(
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
    // The "new user" dialog is initialized with the volume's default quota
    // limit and threshold for new users.
    //
    m_pQuotaControl->GetDefaultQuotaLimit(&m_llQuotaLimit);
    m_pQuotaControl->GetDefaultQuotaThreshold(&m_llQuotaThreshold);

    //
    // Configure the Limit/NoLimit radio buttons.
    //
    if (NOLIMIT == m_llQuotaThreshold)
    {
        CheckDlgButton(hDlg, IDC_RBN_USER_LIMIT,   FALSE);
        CheckDlgButton(hDlg, IDC_RBN_USER_NOLIMIT, TRUE);
    }
    else
    {
        CheckDlgButton(hDlg, IDC_RBN_USER_LIMIT,   TRUE);
        CheckDlgButton(hDlg, IDC_RBN_USER_NOLIMIT, FALSE);
    }

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
        m_cVolumeMaxBytes = (LONGLONG)dwSectorsPerCluster *
                            (LONGLONG)dwBytesPerSector *
                            (LONGLONG)dwTotalClusters;
    }

    m_pxbQuotaLimit     = new XBytes(hDlg,
                                     IDC_EDIT_USER_LIMIT,
                                     IDC_CMB_USER_LIMIT,
                                     m_llQuotaLimit);

    m_pxbQuotaLimit->SetBytes(m_llQuotaLimit);

    m_pxbQuotaThreshold = new XBytes(hDlg,
                                     IDC_EDIT_USER_THRESHOLD,
                                     IDC_CMB_USER_THRESHOLD,
                                     m_llQuotaThreshold);

    m_pxbQuotaThreshold->SetBytes(m_llQuotaThreshold);

    DBGASSERT((0 < m_pSelectionList->cItems));
    if (1 == m_pSelectionList->cItems)
    {
        SetDlgItemText(hDlg,
                       IDC_TXT_USERNAME,
                       GetDsSelUserName(m_pSelectionList->aDsSelection[0]));
    }
    else
    {
        CString strMultiple(m_hInstance, IDS_MULTIPLE);
        SetDlgItemText(hDlg, IDC_TXT_USERNAME, strMultiple);
    }

    SendMessage(GetDlgItem(hDlg, IDC_ICON_USER),
                STM_SETICON,
                (WPARAM)m_hIconUser[1 == m_pSelectionList->cItems ? iICON_USER_SINGLE :
                                                                    iICON_USER_MULTIPLE],
                0);


    return TRUE;  // Set focus to default control.
}

//
// The Object Picker scope definition structure looks like this
// JeffreyS created these helper macros for working with the object picker
// in the ACLEDIT security UI. Thanks Jeff!
//
#if 0
{   // DSOP_SCOPE_INIT_INFO
    cbSize,
    flType,
    flScope,
    {   // DSOP_FILTER_FLAGS
        {   // DSOP_UPLEVEL_FILTER_FLAGS
            flBothModes,
            flMixedModeOnly,
            flNativeModeOnly
        },
        flDownlevel
    },
    pwzDcName,
    pwzADsPath,
    hr // OUT
}
#endif

//
// macro for declaring one of the above
//
#define DECLARE_SCOPE(t,f,b,m,n,d)  \
{ sizeof(DSOP_SCOPE_INIT_INFO), (t), (f), { { (b), (m), (n) }, (d) }, NULL, NULL, S_OK }


#define COMMON_SCOPE_FLAGS    (DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP | DSOP_SCOPE_FLAG_WANT_SID_PATH)

#define TARGET_COMPUTER_SCOPE                             \
DECLARE_SCOPE(                                            \
    DSOP_SCOPE_TYPE_TARGET_COMPUTER,                      \
    COMMON_SCOPE_FLAGS,                                   \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_DOWNLEVEL_FILTER_USERS)

#define JOINED_UPLEVEL_DOMAIN_SCOPE                       \
DECLARE_SCOPE(                                            \
    DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,                \
    COMMON_SCOPE_FLAGS | DSOP_SCOPE_FLAG_STARTING_SCOPE,  \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_DOWNLEVEL_FILTER_USERS)

#define JOINED_DOWNLEVEL_DOMAIN_SCOPE                     \
DECLARE_SCOPE(                                            \
    DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN,              \
    COMMON_SCOPE_FLAGS,                                   \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_DOWNLEVEL_FILTER_USERS)

#define ENTERPRISE_DOMAIN_SCOPE                           \
DECLARE_SCOPE(                                            \
    DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN,                    \
    COMMON_SCOPE_FLAGS,                                   \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_DOWNLEVEL_FILTER_USERS)

#define EXTERNAL_UPLEVEL_DOMAIN_SCOPE                     \
DECLARE_SCOPE(                                            \
    DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN,              \
    COMMON_SCOPE_FLAGS,                                   \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_DOWNLEVEL_FILTER_USERS)

#define EXTERNAL_DOWNLEVEL_DOMAIN_SCOPE                   \
DECLARE_SCOPE(                                            \
    DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN,            \
    COMMON_SCOPE_FLAGS,                                   \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_DOWNLEVEL_FILTER_USERS)

#define GLOBAL_CATALOG_SCOPE                              \
DECLARE_SCOPE(                                            \
    DSOP_SCOPE_TYPE_GLOBAL_CATALOG,                       \
    COMMON_SCOPE_FLAGS,                                   \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_DOWNLEVEL_FILTER_USERS)

#define WORKGROUP_SCOPE                                   \
DECLARE_SCOPE(                                            \
    DSOP_SCOPE_TYPE_WORKGROUP,                            \
    COMMON_SCOPE_FLAGS,                                   \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_FILTER_USERS,                                    \
    DSOP_DOWNLEVEL_FILTER_USERS)

//
// Invokes the standard DS object picker dialog.
// Returns a list of DS_SELECTION structures in a data object
// representing the selected user objects.
//
HRESULT
AddUserDialog::BrowseForUsers(
    HWND hwndParent,
    IDataObject **ppdtobj
    )
{
    DBGASSERT((NULL != hwndParent));
    DBGASSERT((NULL != ppdtobj));

    *ppdtobj = NULL;

    IDsObjectPicker *pop = NULL;
    HRESULT hr = CoCreateInstance(CLSID_DsObjectPicker,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IDsObjectPicker,
                                  (void **)&pop);
    if (SUCCEEDED(hr))
    {
        //
        // This array initializes the scopes of the DS object picker.
        // The first entry is the "default" scope.
        //
        DSOP_SCOPE_INIT_INFO rgdsii[] = {
                JOINED_UPLEVEL_DOMAIN_SCOPE,
                JOINED_DOWNLEVEL_DOMAIN_SCOPE,
                ENTERPRISE_DOMAIN_SCOPE,
                EXTERNAL_UPLEVEL_DOMAIN_SCOPE,
                EXTERNAL_DOWNLEVEL_DOMAIN_SCOPE,
                GLOBAL_CATALOG_SCOPE,
                WORKGROUP_SCOPE,
                TARGET_COMPUTER_SCOPE
                };

        DSOP_INIT_INFO dii;
        dii.cbSize             = sizeof(dii);
        dii.pwzTargetComputer  = NULL;
        dii.cDsScopeInfos      = ARRAYSIZE(rgdsii);
        dii.aDsScopeInfos      = rgdsii;
        dii.flOptions          = DSOP_FLAG_MULTISELECT;
        dii.cAttributesToFetch = 0;
        dii.apwzAttributeNames = NULL;
        //
        // Init and run the object picker dialog.
        //
        hr = pop->Initialize(&dii);
        if (SUCCEEDED(hr))
        {
            hr = pop->InvokeDialog(hwndParent, ppdtobj);
        }
        pop->Release();
    }

    return hr;
}


INT_PTR
AddUserDialog::OnCommand(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DWORD dwCtlId      = LOWORD(wParam);
    HWND hWndCtl       = (HWND)lParam;
    DWORD dwNotifyCode = HIWORD(wParam);
    INT_PTR bResult    = FALSE;

    switch(dwCtlId)
    {
        case IDC_RBN_USER_NOLIMIT:
            if (m_pxbQuotaThreshold->IsEnabled())
            {
                //
                // This is simple.  Just set both the limit and threshold controls
                // to "no limit".
                //
                m_pxbQuotaThreshold->SetBytes(NOLIMIT);
                m_pxbQuotaLimit->SetBytes(NOLIMIT);
            }
            break;

        case IDC_RBN_USER_LIMIT:
            if (!m_pxbQuotaThreshold->IsEnabled())
            {
                LONGLONG llValue = 0;
                m_pQuotaControl->GetDefaultQuotaLimit(&llValue);
                m_pxbQuotaLimit->SetBytes(NOLIMIT == llValue ? 0 : llValue);

                llValue = 0;
                m_pQuotaControl->GetDefaultQuotaThreshold(&llValue);
                m_pxbQuotaThreshold->SetBytes(NOLIMIT == llValue ? 0 : llValue);
            }
            break;

        case IDC_EDIT_USER_LIMIT:
        case IDC_EDIT_USER_THRESHOLD:
            switch(dwNotifyCode)
            {
                case EN_UPDATE:
                    bResult = OnEditNotifyUpdate(hDlg, wParam, lParam);
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
                    bResult = OnComboNotifySelChange(hDlg, wParam, lParam);
                    break;

                default:
                    break;
            }
            break;

        case IDOK:
            if (!OnOk(hDlg, wParam, lParam))
                return FALSE;
            //
            // Fall through...
            //
        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;

        default:
            bResult = TRUE;  // Didn't handle message.
            break;
    }

    return bResult;
}




INT_PTR
AddUserDialog::OnOk(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HRESULT hResult  = NO_ERROR;

    //
    // We need to do this because if you activate the OK button
    // with [Return] we receive the WM_COMMAND before EN_KILLFOCUS.
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

        CString s(m_hInstance, IDS_FMT_ERR_WARNOVERLIMIT, szThreshold, szLimit, szLimit);
        switch(DiskQuotaMsgBox(hDlg, s, IDS_TITLE_DISK_QUOTA, MB_ICONWARNING | MB_YESNO))
        {
            case IDYES:
                m_pxbQuotaThreshold->SetBytes(iLimit);
                break;

            case IDNO:
                //
                // Set focus to threshold edit box so user can correct
                // the entry.  Return early with FALSE value.
                //
                SetFocus(GetDlgItem(hDlg, IDC_EDIT_USER_THRESHOLD));
                SendMessage(GetDlgItem(hDlg, IDC_EDIT_USER_THRESHOLD), EM_SETSEL, 0, -1);
                return FALSE;
        }
    }

    //
    // Only apply settings if the "Apply" button is enabled indicating
    // that something has been changed.  No need to apply unchanged
    // settings when the OK button is pressed.
    //
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
    }
    return TRUE;
}



INT_PTR
AddUserDialog::OnHelp(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, STR_DSKQUOUI_HELPFILE,
                HELP_WM_HELP, (DWORD_PTR)(LPTSTR) rgAddUserDialogHelpIDs);
    return TRUE;
}


INT_PTR
AddUserDialog::OnContextMenu(
    HWND hwndItem,
    int xPos,
    int yPos
    )
{
    int idCtl = GetDlgCtrlID(hwndItem);
    WinHelp(hwndItem,
            UseWindowsHelp(idCtl) ? NULL : STR_DSKQUOUI_HELPFILE,
            HELP_CONTEXTMENU,
            (DWORD_PTR)((LPTSTR)rgAddUserDialogHelpIDs));

    return FALSE;
}



INT_PTR
AddUserDialog::OnEditNotifyUpdate(
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


INT_PTR
AddUserDialog::OnComboNotifySelChange(
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


//
// Retrieve from a DS_SELECTION structure the name to display for
// a user object.
//
LPCWSTR
AddUserDialog::GetDsSelUserName(
    const DS_SELECTION& sel
    )
{
    return sel.pwzUPN && *sel.pwzUPN ? sel.pwzUPN : sel.pwzName;
}


//
// Convert two hex chars into a single byte value.
// Assumes input string is in upper case.
//
HRESULT
AddUserDialog::HexCharsToByte(
    LPTSTR pszByteIn,
    LPBYTE pbOut
    )
{
    static const int iShift[] = { 4, 0 };

    *pbOut = (BYTE)0;
    for (int i = 0; i < 2; i++)
    {
        TCHAR ch = *(pszByteIn + i);
        BYTE b   = (BYTE)0;
        if (TEXT('0') <= ch && TEXT('9') >= ch)
        {
            b = ch - TEXT('0');
        }
        else if (TEXT('A') <= ch && TEXT('F') >= ch)
        {
            b = 10 + (ch - TEXT('A'));
        }
        else
        {
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        *pbOut |= (b << iShift[i]);
    }
    return NOERROR;
}

//
// Returns:
//
//  NOERROR
//  ERROR_INSUFFICIENT_BUFFER (as hresult)
//  ERROR_INVALID_DATA (as hresult)
//
HRESULT
AddUserDialog::GetDsSelUserSid(
    const DS_SELECTION& sel,
    LPBYTE pbSid,
    int cbSid
    )
{
    static const WCHAR szPrefix[] = L"LDAP://<SID=";
    static const WCHAR chTerm     = L'>';

    LPWSTR pszLDAP         = CharUpper(sel.pwzADsPath);
    HRESULT hr             = NOERROR;
    int cb                 = 0;

    //
    // First check for the required prefix.
    //
    if (0 == StrCmpNW(pszLDAP, szPrefix, ARRAYSIZE(szPrefix) - 1))
    {
        //
        // Advance ptr beyond prefix and convert the hex string
        // into a SID.  Process chars until we hit a '>'.
        //
        pszLDAP += ARRAYSIZE(szPrefix) - 1;

        while(SUCCEEDED(hr) && *pszLDAP && chTerm != *pszLDAP)
        {
            if (0 < cbSid--)
            {
                hr = HexCharsToByte(pszLDAP, pbSid++);
                pszLDAP += 2;
            }
            else
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        if (SUCCEEDED(hr) && chTerm != *pszLDAP)
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    else
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);


    if (FAILED(hr))
    {
        //
        // BUGBUG:  This can be removed once I'm comfortable that all
        //          ADs paths returned from the object picker contain
        //          a SID.
        //
        DBGERROR((TEXT("GetDsSelUserSid returning hr = 0x%08X for path \"%s\""),
                  hr, sel.pwzADsPath));
    }

    return hr;
}


HRESULT
AddUserDialog::ApplySettings(
    HWND hDlg,
    bool bUndo
    )
{
    HRESULT hResult = E_FAIL;
    int cUsers = m_pSelectionList->cItems;
    CAutoWaitCursor wait_cursor;

    //
    // Retrieve limit and threshold values from dialog controls.
    //
    if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RBN_USER_NOLIMIT))
    {
        m_llQuotaThreshold = NOLIMIT;
        m_llQuotaLimit     = NOLIMIT;
    }
    else
    {
        m_llQuotaThreshold = m_pxbQuotaThreshold->GetBytes();
        m_llQuotaLimit     = m_pxbQuotaLimit->GetBytes();
    }


    if (bUndo)
        m_UndoList.Clear();

    ProgressDialog dlgProgress(IDD_PROGRESS,
                               IDC_PROGRESS_BAR,
                               IDC_TXT_PROGRESS_DESCRIPTION,
                               IDC_TXT_PROGRESS_FILENAME);
    if (2 < cUsers)
    {
        //
        // Create and display a progress dialog if we're adding more than 2
        // users.
        //
        HWND hwndParent = IsWindowVisible(hDlg) ? hDlg : GetParent(hDlg);
        if (dlgProgress.Create(m_hInstance, hwndParent))
        {
            dlgProgress.ProgressBarInit(0, cUsers, 1);
            dlgProgress.SetDescription(MAKEINTRESOURCE(IDS_PROGRESS_ADDUSER));
            dlgProgress.Show();
        }
    }

    bool bCancelled = false;
    for (int i = 0; i < cUsers && !bCancelled; i++)
    {
        DS_SELECTION *pdss = &(m_pSelectionList->aDsSelection[i]);
        LPCWSTR pwzName = GetDsSelUserName(*pdss);

        //
        // Add a user to the quota file.  This will add it using the defaults
        // for new users.  We get back an interface to the new user object.
        // Also specify async name resolution.
        //
        if (NULL == pwzName)
        {
            dlgProgress.ProgressBarAdvance();
            continue;
        }

        dlgProgress.SetFileName(pwzName);

        com_autoptr<DISKQUOTA_USER> ptrUser;
        DiskQuotaControl *pDQC = static_cast<DiskQuotaControl *>(m_pQuotaControl);

        BYTE sid[MAX_SID_LEN];
        hResult = GetDsSelUserSid(*pdss, sid, ARRAYSIZE(sid));
        if (SUCCEEDED(hResult))
        {
            hResult = pDQC->AddUserSid(sid,
                                       DISKQUOTA_USERNAME_RESOLVE_ASYNC,
                                       ptrUser.getaddr());

            if (SUCCEEDED(hResult))
            {
                if (S_FALSE == hResult)
                {
                    hResult = HRESULT_FROM_WIN32(ERROR_USER_EXISTS);
                }
                else
                {
                    if (SUCCEEDED(hResult = ptrUser->SetQuotaLimit(m_llQuotaLimit, TRUE)) &&
                        SUCCEEDED(hResult = ptrUser->SetQuotaThreshold(m_llQuotaThreshold, TRUE)))
                    {
                        if (bUndo)
                        {
                            //
                            // Create local autoptrs to ensure iface release if an
                            // exception is thrown.
                            //
                            com_autoptr<DISKQUOTA_CONTROL> ptrQuotaControl(m_pQuotaControl);
                            com_autoptr<DISKQUOTA_USER> ptrQuotaUser(ptrUser);
                            ptrQuotaUser->AddRef();
                            ptrQuotaControl->AddRef();
                            m_UndoList.Add(new UndoAdd(ptrUser, m_pQuotaControl));
                            //
                            // Successfully added to undo list.  Disown real ptrs so
                            // ref count stays with undo list.  If an exception was
                            // thrown, the local com_autoptr objects will automatically
                            // release the interfaces.
                            //
                            ptrQuotaUser.disown();
                            ptrQuotaControl.disown();
                        }

                        //
                        // Add the user to the listview.
                        //
                        SendMessage(m_hwndDetailsLV,
                                    WM_ADD_USER_TO_DETAILS_VIEW,
                                    0,
                                    (LPARAM)ptrUser.get());
                        //
                        // iface pointer added to listview.  autoptr disowns the real
                        // pointer so the autoptr's dtor doesn't release it.
                        //
                        ptrUser.disown();
                    }
                }
            }
        }
        if (FAILED(hResult))
        {
            INT idMsg   = IDS_UNKNOWN_ERROR;
            UINT uFlags = MB_OKCANCEL;
            switch(hResult)
            {
                case E_FAIL:
                    idMsg = IDS_WRITE_ERROR;
                    uFlags |= MB_ICONERROR;
                    break;

                default:
                    switch(HRESULT_CODE(hResult))
                    {
                        case ERROR_USER_EXISTS:
                            idMsg = IDS_NOADD_EXISTING_USER;
                            uFlags |= MB_ICONWARNING;
                            break;

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

            //
            // Display message box with msg formatted as:
            //
            //      The user already exists and could not be added.
            //
            //      User:  brianau
            //      In Folder: Domain/Folder: ntdev.microsoft.com/US SOS-...
            //
            CString strError(m_hInstance, idMsg);
            CString strMsg(m_hInstance, IDS_FMT_ERR_ADDUSER, strError.Cstr(), pwzName);

            HWND hwndMsgBoxParent = (NULL != dlgProgress.m_hWnd && IsWindowVisible(dlgProgress.m_hWnd)) ?
                                    dlgProgress.m_hWnd : hDlg;

            if (IDCANCEL == DiskQuotaMsgBox(hwndMsgBoxParent,
                                            strMsg.Cstr(),
                                            IDS_TITLE_DISK_QUOTA,
                                            uFlags))
            {
                bCancelled = true;
            }
        }
        dlgProgress.ProgressBarAdvance();
        bCancelled = bCancelled || dlgProgress.UserCancelled();
    }

    return NOERROR;
}


