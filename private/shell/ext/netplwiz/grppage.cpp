/********************************************************
 grppg.cpp

  User Manager group membership prop page implementation

 History:
  09/23/98: dsheldon created
********************************************************/

#include "stdafx.h"
#include "resource.h"

#include "grppage.h"
#include "misc.h"

/**************************************************************
 CGroupPageBase Implementation

  Functions common to both the group prop page and the group
  wizard page.
**************************************************************/

INT_PTR CGroupPageBase::HandleGroupMessage(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
    };

    return FALSE;
}

void CGroupPageBase::InitializeLocalGroupCombo(HWND hwndCombo)
{
    TraceEnter(TRACE_USR_CORE, "CGroupPageBase::InitializeLocalGroupCombo");

    ComboBox_ResetContent(hwndCombo);

    // Add all of the groups in the list to the box
    for(int i = 0; i < m_pGroupList->GetPtrCount(); i ++)
    {
        CGroupInfo* pGroupInfo = m_pGroupList->GetPtr(i);
        int index = ComboBox_AddString(hwndCombo, pGroupInfo->m_szGroup);
        ComboBox_SetItemData(hwndCombo, index, pGroupInfo->m_szComment);
    }

    TCHAR szSelectGroup[MAX_GROUP + 1];
    // Load a local group name from the resources to select by default
    LoadString(g_hInstance, IDS_USR_DEFAULTGROUP, szSelectGroup, ARRAYSIZE(szSelectGroup));

    if (ComboBox_SelectString(hwndCombo, 0, szSelectGroup) == CB_ERR)
    {
        ComboBox_SetCurSel(hwndCombo, 0);
    }

    TraceLeaveVoid();
}

void CGroupPageBase::SetGroupDescription(HWND hwndCombo, HWND hwndEdit)
{
    TraceEnter(TRACE_USR_CORE, "CGroupPageBase::SetGroupDescription");

    int iItem = ComboBox_GetCurSel(hwndCombo);
    TraceAssert(iItem != CB_ERR);

    TCHAR* pszDescription = (TCHAR*) ComboBox_GetItemData(hwndCombo, iItem);
    SetWindowText(hwndEdit, pszDescription);

    TraceLeaveVoid();
}

BOOL CGroupPageBase::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TraceEnter(TRACE_USR_CORE, "CGroupPageBase::OnInitDialog");

    // Fill in the local group combo box
    HWND hwndCombo = GetDlgItem(hwnd, IDC_GROUPS);
    InitializeLocalGroupCombo(hwndCombo);

    HWND hwndEdit = GetDlgItem(hwnd, IDC_GROUPDESC);
    
    if ((NULL != m_pUserInfo) && (m_pUserInfo->m_szGroups[0] != TEXT('\0')))
    {
        // Select the local group corresponding to the first one in the user's groups
        // string
        TCHAR szSelect[MAX_GROUP + 1];

        // Copy the string since we might shorten our copy
        lstrcpyn(szSelect, m_pUserInfo->m_szGroups, ARRAYSIZE(szSelect));
        
        TCHAR* pchEndOfFirst = StrChr(szSelect, TEXT(';'));

        if (pchEndOfFirst)
        {
            // More than one group; we'll fix that!
            *pchEndOfFirst = TEXT('\0');
        }

        SelectGroup(hwnd, szSelect);
    }
    else
    {
        // Select the power user group by default 
        SendDlgItemMessage(hwnd, IDC_POWERUSERS, BM_SETCHECK, 
            (WPARAM) BST_CHECKED, 0);

        OnRadioChanged(hwnd, IDC_POWERUSERS);
    }
   
    SetGroupDescription(hwndCombo, hwndEdit);

    // Bold the group names
    BoldGroupNames(hwnd);

    TraceLeaveValue(TRUE);
}

BOOL CGroupPageBase::GetSelectedGroup(HWND hwnd, LPTSTR pszGroupOut, DWORD cchGroup, CUserInfo::GROUPPSEUDONYM* pgsOut)
{
    TraceEnter(TRACE_USR_CORE, "CGroupPageBase::GetSelectedGroup");
    
    BOOL fSuccess = FALSE;
    *pgsOut = CUserInfo::USEGROUPNAME;

    UINT idString = 0;
    if (BST_CHECKED == Button_GetCheck(GetDlgItem(hwnd, IDC_POWERUSERS)))
    {
        idString = IDS_USR_POWERUSERS;
        *pgsOut = CUserInfo::STANDARD;
    }
    else if (BST_CHECKED == Button_GetCheck(GetDlgItem(hwnd, IDC_USERS)))
    {
        idString = IDS_USR_USERS;
        *pgsOut = CUserInfo::RESTRICTED;
    }
    
    if (0 != idString)
    {
        LoadString(g_hInstance, idString, pszGroupOut, cchGroup);

        // Success

        fSuccess = TRUE;
    }
    else
    {
        // 'other' must be selected; get the string from the dropdown
        GetWindowText(GetDlgItem(hwnd, IDC_GROUPS), pszGroupOut, cchGroup);
    }

    TraceLeaveValue(fSuccess);
}

// Returns IDC_OTHER if no radio button id corresponds to the group
UINT CGroupPageBase::RadioIdForGroup(LPCTSTR pszGroup)
{
    TraceEnter(TRACE_USR_CORE, "CGroupPageBase::RadioIdForGroup");

    TCHAR szPowerUsers[MAX_GROUP + 1];
    TCHAR szUsers[MAX_GROUP + 1];

    LoadString(g_hInstance, IDS_USR_POWERUSERS, szPowerUsers,
        ARRAYSIZE(szPowerUsers));

    LoadString(g_hInstance, IDS_USR_USERS, szUsers,
        ARRAYSIZE(szUsers));

    // Assume IDC_OTHER to start
    UINT uiRadio = IDC_OTHER;

    if (0 == StrCmpI(pszGroup, szPowerUsers))
    {
        uiRadio = IDC_POWERUSERS;
    }
    else if (0 == StrCmpI(pszGroup, szUsers))
    {
        uiRadio = IDC_USERS;
    }

    TraceLeaveValue(uiRadio);
}

// Disable/update as appropriate when radio selection changes
void CGroupPageBase::OnRadioChanged(HWND hwnd, UINT idRadio)
{
    TraceEnter(TRACE_USR_CORE, "CGroupPageBase::OnRadioChanged");

    BOOL fEnableGroupDropdown = (IDC_OTHER == idRadio);

    EnableWindow(GetDlgItem(hwnd, IDC_GROUPS), fEnableGroupDropdown);
    EnableWindow(GetDlgItem(hwnd, IDC_OTHER_STATIC), fEnableGroupDropdown);

    ShowWindow(GetDlgItem(hwnd, IDC_GROUPDESC), 
        fEnableGroupDropdown ? SW_SHOW : SW_HIDE);

    TraceLeaveVoid();
}

void CGroupPageBase::SelectGroup(HWND hwnd, LPCTSTR pszSelect)
{
    TraceEnter(TRACE_USR_CORE, "CGroupPageBase::SelectGroup");

    // Always select the group in the 'other' dropdown
    ComboBox_SelectString(GetDlgItem(hwnd, IDC_GROUPS),
        -1, pszSelect);
    
    // Check the appropriate radio button
    UINT idRadio = RadioIdForGroup(pszSelect);
    Button_SetCheck(GetDlgItem(hwnd, idRadio), BST_CHECKED);

    OnRadioChanged(hwnd, idRadio);

    TraceLeaveVoid();
}


void CGroupPageBase::BoldGroupNames(HWND hwnd)
{
    TraceEnter(TRACE_USR_CORE, "CCroupPageBase::BoldGroupNames");

    HWND hwndPowerUsers = GetDlgItem(hwnd, IDC_POWERUSERS);

    HFONT hfont = (HFONT) SendMessage(hwndPowerUsers, WM_GETFONT, 0, 0);

    if (hfont)
    {
        LOGFONT lf;
        if (FALSE != GetObject((HGDIOBJ) hfont, sizeof(lf), &lf))
        {
            lf.lfWeight = FW_BOLD;

            m_hBoldFont = CreateFontIndirect(&lf);

            if (NULL != m_hBoldFont)
            {
                // Set the font
                SendMessage(hwndPowerUsers, WM_SETFONT, 
                    (WPARAM) m_hBoldFont, 0);

                SendDlgItemMessage(hwnd, IDC_USERS,
                    WM_SETFONT, (WPARAM) m_hBoldFont, 0);

                SendDlgItemMessage(hwnd, IDC_OTHER,
                    WM_SETFONT, (WPARAM) m_hBoldFont, 0);
            }
        }
    }

    TraceLeaveVoid();
}

BOOL CGroupPageBase::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    TraceEnter(TRACE_USR_CORE, "CGroupPageBase::OnCommand");

    switch(codeNotify)
    {
    case CBN_SELCHANGE:
        SetGroupDescription(hwndCtl, GetDlgItem(hwnd, IDC_GROUPDESC));
        PropSheet_Changed(GetParent(hwnd), hwnd);
        break;

    case BN_CLICKED:
        // Handle radio clicks
        switch (id)
        {
        case IDC_POWERUSERS:
        case IDC_USERS:
        case IDC_OTHER:
            PropSheet_Changed(GetParent(hwnd), hwnd);
            OnRadioChanged(hwnd, id);
        }
        break;
    }

    TraceLeaveValue(FALSE);
}

/**************************************************************
 CGroupWizardPage Implementation
**************************************************************/

INT_PTR CGroupWizardPage::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
    }

    return FALSE;
}

BOOL CGroupWizardPage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    TraceEnter(TRACE_USR_CORE, "CGroupWizardPage::OnNotify");
           
    switch (pnmh->code)
    {
    case PSN_SETACTIVE:
        PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK | PSWIZB_FINISH);
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 0);
        TraceLeaveValue(TRUE);
    case PSN_WIZFINISH:
        {
            // Read in the local group name
            CUserInfo::GROUPPSEUDONYM gs;
            GetSelectedGroup(hwnd, m_pUserInfo->m_szGroups,
                ARRAYSIZE(m_pUserInfo->m_szGroups), &gs);

            // Don't close wizard by default
            LONG_PTR finishResult = (LONG_PTR) hwnd;

            SetCursor(LoadCursor(NULL, IDC_WAIT));
            if (SUCCEEDED(m_pUserInfo->Create(hwnd, gs)))
            {
                m_pUserInfo->m_fHaveExtraUserInfo = FALSE;
                // Close wizard
                finishResult = 0;
            }

            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, finishResult);
            TraceLeaveValue(TRUE);
        }
    }

    TraceLeaveValue(FALSE);
}

/**************************************************************
 CGroupPropertyPage Implementation
**************************************************************/

INT_PTR CGroupPropertyPage::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
    }

    return FALSE;
}

BOOL CGroupPropertyPage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    BOOL fReturn = FALSE;

    switch(pnmh->code)
    {
    case PSN_APPLY:
        {
            // Check to see if the group needs updating on Apply
            TCHAR szTemp[MAX_GROUP + 1];

            // Read in the local group name
            CUserInfo::GROUPPSEUDONYM gs;
            GetSelectedGroup(hwnd, szTemp,
                ARRAYSIZE(szTemp), &gs);

            if (StrCmp(szTemp, m_pUserInfo->m_szGroups) != 0)
            {
                HRESULT hr = m_pUserInfo->UpdateGroup(hwnd, szTemp, gs);

                if (SUCCEEDED(hr))
                {
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
                }
                else
                {
                    TCHAR szDomainUser[MAX_DOMAIN + MAX_USER + 2];
                    MakeDomainUserString(m_pUserInfo->m_szDomain, m_pUserInfo->m_szUsername,
                        szDomainUser, ARRAYSIZE(szDomainUser));

                    ::DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION, 
                        IDS_USR_UPDATE_GROUP_ERROR, MB_ICONERROR | MB_OK,
                        szDomainUser);

                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                }
            }
        }
        fReturn = TRUE;
        break;
    default:
        fReturn = FALSE;
        break;
    }

    TraceLeaveValue(fReturn);
}
