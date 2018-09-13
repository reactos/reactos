//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       owner.cpp
//
//  This file contains the implementation of the Owner page.
//
//--------------------------------------------------------------------------

#include "aclpriv.h"
#include "sddl.h"       // ConvertSidToStringSid


//
//  Context Help IDs.
//
const static DWORD aOwnerHelpIDs[] =
{
    IDC_OWN_CURRENTOWNER_STATIC,    IDH_OWN_CURRENTOWNER,
    IDC_OWN_CURRENTOWNER,           IDH_OWN_CURRENTOWNER,
    IDC_OWN_OWNERLIST_STATIC,       IDH_OWN_OWNERLIST,
    IDC_OWN_OWNERLIST,              IDH_OWN_OWNERLIST,
    IDC_OWN_RECURSE,                IDH_OWN_RECURSE,
    IDC_OWN_RESET,                  IDH_OWN_RESET,
    0, 0
};

//
// These SIDs are always added to the list of possible owners
//
const static UI_TokenSid g_uiTokenSids[] =
{
    UI_TSID_CurrentProcessUser,
    UI_TSID_CurrentProcessOwner,
    //UI_TSID_CurrentProcessPrimaryGroup,
};


class COwnerPage : public CSecurityPage
{
private:
    PSID    m_psidOriginal;
    PSID    m_psidNetID;
    HANDLE  m_hSidThread;

public:
    COwnerPage(LPSECURITYINFO psi, SI_OBJECT_INFO *psiObjectInfo);
    virtual ~COwnerPage(void);

private:
    virtual BOOL DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void InitDlg(HWND hDlg);
    int  AddSid(HWND hOwner, PSID psid, LPCTSTR pszServerName = NULL);
    void OnApply(HWND hDlg, BOOL bClose);
    void OnReset(HWND hDlg);
};


HPROPSHEETPAGE
CreateOwnerPage(LPSECURITYINFO psi, SI_OBJECT_INFO *psiObjectInfo)
{
    HPROPSHEETPAGE hPage = NULL;
    COwnerPage *pPage;

    TraceEnter(TRACE_OWNER, "CreateOwnerPage");

    pPage = new COwnerPage(psi, psiObjectInfo);

    if (pPage)
    {
        hPage = pPage->CreatePropSheetPage(MAKEINTRESOURCE(IDD_OWNER_PAGE));

        if (!hPage)
            delete pPage;
    }

    TraceLeaveValue(hPage);
}


COwnerPage::COwnerPage(LPSECURITYINFO psi, SI_OBJECT_INFO *psiObjectInfo)
: CSecurityPage(psi, SI_PAGE_OWNER), m_psidOriginal(NULL), m_psidNetID(NULL),
  m_hSidThread(NULL)
{
    // Lookup known SIDs asynchronously so the dialog
    // will initialize faster
    HDPA hSids = DPA_Create(ARRAYSIZE(g_uiTokenSids));
    if (hSids)
    {
        USES_CONVERSION;

        LPCWSTR pszServer = NULL;
        if (psiObjectInfo)
            pszServer = psiObjectInfo->pszServerName;

        for (int i = 0; i < ARRAYSIZE(g_uiTokenSids); i++)
            DPA_AppendPtr(hSids, QueryTokenSid(g_uiTokenSids[i]));

        m_psidNetID = GetAuthenticationID(pszServer);
        if (m_psidNetID)
            DPA_AppendPtr(hSids, m_psidNetID);

        LookupSidsAsync(hSids, W2CT(pszServer), m_psi2, NULL, 0, &m_hSidThread);
        DPA_Destroy(hSids);
    }
}


COwnerPage::~COwnerPage(void)
{
    if (m_hSidThread)
        CloseHandle(m_hSidThread);

    if (m_psidOriginal)
        LocalFree(m_psidOriginal);

    if (m_psidNetID)
        LocalFree(m_psidNetID);
}

int
COwnerPage::AddSid(HWND hOwner, PSID psid, LPCTSTR pszServerName)
{
    PUSER_LIST pUserList = NULL;
    SID_NAME_USE sidType = SidTypeUnknown;
    LPCTSTR pszName = NULL;
    LPCTSTR pszLogonName = NULL;
    int iItem = -1;
    int cItems;
    LV_ITEM lvItem;

    TraceEnter(TRACE_OWNER, "COwnerPage::AddSid");
    TraceAssert(!m_bAbortPage);

    if (!psid || !IsValidSid(psid))
        ExitGracefully(iItem, -1, "Bad SID parameter");

    // Get the name for this SID
    if (LookupSid(psid, pszServerName, m_psi2, &pUserList))
    {
        TraceAssert(NULL != pUserList);
        TraceAssert(1 == pUserList->cUsers);

        sidType = pUserList->rgUsers[0].SidType;
        pszName = pUserList->rgUsers[0].pszName;
        pszLogonName = pUserList->rgUsers[0].pszLogonName;
    }

    switch (sidType)
    {
    case SidTypeDomain:
    case SidTypeDeletedAccount:
    case SidTypeInvalid:
    case SidTypeUnknown:
    case SidTypeComputer:
        ExitGracefully(iItem, -1, "SID invalid on target");
        break;
    }

    cItems = ListView_GetItemCount(hOwner);
    lvItem.mask     = LVIF_PARAM;
    lvItem.iSubItem = 0;

    // See if this SID is already in the list
    for (iItem = 0; iItem < cItems; iItem++)
    {
        lvItem.iItem    = iItem;
        lvItem.lParam   = NULL;
        ListView_GetItem(hOwner, &lvItem);

        if (lvItem.lParam && EqualSid(psid, (PSID)lvItem.lParam))
        {
            // This is a hack.  We often see alias sids more than once when
            // filling the list, e.g. BUILTIN\Administrators.  We want to use
            // the version of the name that includes the target domain, if
            // provided.  That is, if pszServerName is non-NULL here, switch
            // to the version of the name that goes with pszServerName.
            if (pszServerName)
            {
                lvItem.mask = LVIF_TEXT;
                lvItem.pszText = NULL;
                if (BuildUserDisplayName(&lvItem.pszText, pszName, pszLogonName)
                    || ConvertSidToStringSid(psid, &lvItem.pszText))
                {
                    ListView_SetItem(hOwner, &lvItem);
                    LocalFreeString(&lvItem.pszText);
                }
            }
            break;
        }
    }

    if (iItem == cItems)
    {
        // The SID doesn't exist in the list.  Add a new entry.

        PSID psidCopy = LocalAllocSid(psid);
        if (psidCopy)
        {
            lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            lvItem.iItem = 0;
            lvItem.iSubItem = 0;
            lvItem.pszText = NULL;
            if (!BuildUserDisplayName(&lvItem.pszText, pszName, pszLogonName))
                ConvertSidToStringSid(psid, &lvItem.pszText);
            lvItem.iImage = GetSidImageIndex(psid, sidType);
            lvItem.lParam = (LPARAM)psidCopy;

            // Insert principal into list
            iItem = ListView_InsertItem(hOwner, &lvItem);
            LocalFreeString(&lvItem.pszText);
        }
    }

exit_gracefully:

    if (NULL != pUserList)
        LocalFree(pUserList);

    TraceLeaveValue(iItem);
}

void
COwnerPage::InitDlg(HWND hDlg)
{
    TCHAR       szBuffer[MAX_PATH];
    BOOL        bReadOnly;
    HWND        hOwner = GetDlgItem(hDlg, IDC_OWN_OWNERLIST);
    HCURSOR     hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    TraceEnter(TRACE_OWNER, "COwnerPage::InitDlg");

    // Hide the Reset button if it isn't supported.
    if (!(m_siObjectInfo.dwFlags & SI_RESET))
    {
        ShowWindow(GetDlgItem(hDlg, IDC_OWN_RESET), SW_HIDE);
    }

    // Hide the Recurse checkbox if it isn't supported.
    if ((m_siObjectInfo.dwFlags & (SI_OWNER_RECURSE | SI_CONTAINER)) != (SI_OWNER_RECURSE | SI_CONTAINER))
    {
        m_siObjectInfo.dwFlags &= ~SI_OWNER_RECURSE;
        HWND hwndRecurse = GetDlgItem(hDlg, IDC_OWN_RECURSE);
        ShowWindow(hwndRecurse, SW_HIDE);
        EnableWindow(hwndRecurse, FALSE);
    }

    if (m_bAbortPage)
    {
        //
        // Disable everything
        //
        bReadOnly = TRUE;
    }
    else
    {
        // Create & set the image list for the listview
        ListView_SetImageList(hOwner,
                              LoadImageList(::hModule, MAKEINTRESOURCE(IDB_SID_ICONS)),
                              LVSIL_SMALL);

        //
        // Add the "Name" column (the only column on this page)
        //
        RECT rc;
        GetClientRect(hOwner, &rc);

        LoadString(::hModule, IDS_NAME, szBuffer, ARRAYSIZE(szBuffer));

        LV_COLUMN col;
        col.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
        col.fmt = LVCFMT_LEFT;
        col.pszText = szBuffer;
        col.iSubItem = 0;
        col.cx = rc.right;
        ListView_InsertColumn(hOwner, 0, &col);

        //
        // BUGBUG - There was code in the original ACLEDIT.DLL which tried
        // running the sid through a name translation on the server onto which
        // the security is being applied.  If the translation fails, then they
        // switched into a fakery mode.  The fakery mode would take the sid
        // and look it up locally, then take that name and look it up on the
        // server.  Then they would use the resulting sid.
        //
        // This seems quite bogus.  Does it really still need to be done?
        //

        //
        // Make a copy of the current owner sid
        //
        PSECURITY_DESCRIPTOR pSD = NULL;

        HRESULT hr = m_psi->GetSecurity(OWNER_SECURITY_INFORMATION, &pSD, FALSE);
        if (pSD)
        {
            PSID psidOwner = NULL;
            BOOL bDefaulted;

            GetSecurityDescriptorOwner(pSD, &psidOwner, &bDefaulted);

            if (psidOwner)
            {
                UINT iLength = GetLengthSid(psidOwner);
                m_psidOriginal = LocalAlloc(LPTR, iLength);
                if (m_psidOriginal)
                    CopyMemory(m_psidOriginal, psidOwner, iLength);
            }
            LocalFree(pSD);
        }

        // Test for writeability
        bReadOnly = !!(m_siObjectInfo.dwFlags & SI_OWNER_READONLY);
    } // !m_bAbortPage

    //
    // Iterate through the groups on this process's token looking for
    // the SE_GROUP_OWNER attribute.
    //
    if (!bReadOnly)
    {
        HANDLE hProcessToken = NULL;

        //
        // Wait for the known SIDs to be resolved so we don't try
        // to look them up twice.
        //
        if (m_hSidThread)
        {
            WaitForSingleObject(m_hSidThread, INFINITE);
            CloseHandle(m_hSidThread);
            m_hSidThread = NULL;
        }

        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcessToken))
        {
            // Allocate a buffer for the TOKEN_GROUPS information
            ULONG  cbBuffer = 1024; // start with 1k
            LPVOID pBuffer = LocalAlloc(LPTR, cbBuffer);

            if (pBuffer)
            {
                if (!GetTokenInformation(hProcessToken,
                                         TokenGroups,
                                         pBuffer,
                                         cbBuffer,
                                         &cbBuffer))
                {
                    LocalFree(pBuffer);
                    pBuffer = NULL;

                    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                    {
                        pBuffer = LocalAlloc(LPTR, cbBuffer);// size returned above
                        if (pBuffer && !GetTokenInformation(hProcessToken,
                                                            TokenGroups,
                                                            pBuffer,
                                                            cbBuffer,
                                                            &cbBuffer))
                        {
                            LocalFree(pBuffer);
                            pBuffer = NULL;
                        }
                    }
                }

                if (pBuffer)
                {
                    PTOKEN_GROUPS ptg = (PTOKEN_GROUPS)pBuffer;
                    for (ULONG i = 0; i < ptg->GroupCount; i++)
                    {
                        DWORD dwAttr = ptg->Groups[i].Attributes;
                        if ((dwAttr & SE_GROUP_OWNER) && !(dwAttr & SE_GROUP_LOGON_ID))
                        {
                            AddSid(hOwner, ptg->Groups[i].Sid, m_siObjectInfo.pszServerName);
                        }
                    }
                }
                if (pBuffer != NULL)
                    LocalFree(pBuffer);
            }
            CloseHandle(hProcessToken);
        }

        //
        // Now add in the additional possible sids
        //
        for (int i = 0; i < ARRAYSIZE(g_uiTokenSids); i++)
            AddSid(hOwner, QueryTokenSid(g_uiTokenSids[i]));

        AddSid(hOwner, m_psidNetID, m_siObjectInfo.pszServerName);
    }

    if (!m_bAbortPage)
    {
        PUSER_LIST pUserList = NULL;

        LoadString(::hModule, IDS_OWNER_CANT_DISPLAY, szBuffer, ARRAYSIZE(szBuffer));

        // Finally, look up a name for the original SID.
        if (m_psidOriginal)
        {
            LPTSTR pszName = NULL;

            // Get the "S-1-5-blah" form of the SID in case the lookup fails
            if (ConvertSidToStringSid(m_psidOriginal, &pszName))
            {
                lstrcpyn(szBuffer, pszName, ARRAYSIZE(szBuffer));
                LocalFreeString(&pszName);
            }

            if (LookupSid(m_psidOriginal, m_siObjectInfo.pszServerName, m_psi2, &pUserList))
            {
                TraceAssert(NULL != pUserList);
                TraceAssert(1 == pUserList->cUsers);

                if (BuildUserDisplayName(&pszName, pUserList->rgUsers[0].pszName, pUserList->rgUsers[0].pszLogonName))
                {
                    lstrcpyn(szBuffer, pszName, ARRAYSIZE(szBuffer));
                    LocalFreeString(&pszName);
                }
                LocalFree(pUserList);
            }
        }
        SetDlgItemText(hDlg, IDC_OWN_CURRENTOWNER, szBuffer);
    }

    //
    // If the current user cannot change owners, gray out the list box.
    //
    if (bReadOnly)
    {
        // Disable the list and notify the user that it's read-only.
        EnableWindow(hOwner, FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_OWN_RESET), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_OWN_RECURSE), FALSE);

        //
        // If we're aborting, then the user should have been notified
        // during the propsheetpage callback.  Don't put up another
        // message here.
        //
        if (S_OK == m_hrLastPSPCallbackResult)
        {
            MsgPopup(hDlg,
                     MAKEINTRESOURCE(IDS_OWNER_READONLY),
                     MAKEINTRESOURCE(IDS_SECURITY),
                     MB_OK | MB_ICONINFORMATION,
                     ::hModule,
                     m_siObjectInfo.pszObjectName);
        }
    }

    SetCursor(hcur);

    TraceLeaveVoid();
}

void
COwnerPage::OnApply(HWND hDlg, BOOL bClose)
{
    int  iSelected = -1;
    HWND hwndOwnerList;
    PSID psid;
    BOOL bRecurse = FALSE;
    SECURITY_INFORMATION si = OWNER_SECURITY_INFORMATION;

    TraceEnter(TRACE_OWNER, "COwnerPage::OnApply");

    hwndOwnerList = GetDlgItem(hDlg, IDC_OWN_OWNERLIST);
    psid = (PSID)GetSelectedItemData(hwndOwnerList, &iSelected);

    // If there is no selection, use the original
    if (!psid)
        psid = m_psidOriginal;

    if ((m_siObjectInfo.dwFlags & SI_OWNER_RECURSE)
        && IsDlgButtonChecked(hDlg, IDC_OWN_RECURSE) == BST_CHECKED)
    {
        bRecurse = TRUE;
    }

    // Has anything changed?
    if (m_psidOriginal
        && EqualSid(m_psidOriginal, psid)
        && !bRecurse)
    {
        // Nothing has changed
        TraceLeaveVoid();
    }

    SECURITY_DESCRIPTOR sd = {0};
    DWORD dwPriv = SE_TAKE_OWNERSHIP_PRIVILEGE;
    HANDLE hToken = INVALID_HANDLE_VALUE;
    HRESULT hr;

    TraceAssert(!m_bAbortPage);

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorOwner(&sd, psid, FALSE);

    // BUGBUG
    // ISecurityInformation::SetSecurity doesn't have a parameter to indicate
    // that the owner should be recursively applied.  We could add a parameter,
    // but for now, just use one of the unused SECURITY_INFORMATION bits.
    // The security descriptor structure is unlikely to change so this should
    // be ok for now.
    if (bRecurse)
        si |= SI_OWNER_RECURSE;

    hToken = EnablePrivileges(&dwPriv, 1);

    hr = m_psi->SetSecurity(si, &sd);

    ReleasePrivileges(hToken);

    if (S_FALSE == hr)
    {
        // S_FALSE is silent failure (the client should put up UI
        // during SetSecurity before returning S_FALSE).
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
    }
    else if (S_OK == hr && !bClose)
    {
        UINT iLength = GetLengthSid(psid);

        if (-1 != iSelected)
        {
            TCHAR szName[MAX_PATH];
            szName[0] = TEXT('\0');
            ListView_GetItemText(hwndOwnerList, iSelected, 0, szName, ARRAYSIZE(szName));
            SetDlgItemText(hDlg, IDC_OWN_CURRENTOWNER, szName);
        }

        if (m_psidOriginal)
        {
            UINT iLengthOriginal = (UINT)LocalSize(m_psidOriginal);
            if (iLengthOriginal < iLength)
            {
                LocalFree(m_psidOriginal);
                m_psidOriginal = NULL;
            }
            else
            {
                ZeroMemory(m_psidOriginal, iLengthOriginal);
            }
        }

        if (!m_psidOriginal)
            m_psidOriginal = LocalAlloc(LPTR, iLength);

        if (m_psidOriginal)
        {
            CopyMemory(m_psidOriginal, psid, iLength);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (m_siObjectInfo.dwFlags & SI_OWNER_RECURSE)
            CheckDlgButton(hDlg, IDC_OWN_RECURSE, BST_UNCHECKED);
    }

    if (FAILED(hr))
    {
        SysMsgPopup(hDlg,
                    MAKEINTRESOURCE(IDS_OWNER_WRITE_FAILED),
                    MAKEINTRESOURCE(IDS_SECURITY),
                    MB_OK | MB_ICONERROR,
                    ::hModule,
                    hr,
                    m_siObjectInfo.pszObjectName);
    }

    TraceLeaveVoid();
}

void
COwnerPage::OnReset(HWND hDlg)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    HWND hOwner;
    PSID psid;
    HRESULT hr;

    TraceEnter(TRACE_OWNER, "COwnerPage::OnReset");
    TraceAssert(!m_bAbortPage);

    hOwner = GetDlgItem(hDlg, IDC_OWN_OWNERLIST);
    psid = (PSID)GetSelectedItemData(hOwner, NULL);

    hr = m_psi->GetSecurity(OWNER_SECURITY_INFORMATION, &pSD, TRUE);
    if (SUCCEEDED(hr))
    {
        PSID psidDefault = NULL;
        BOOL bDefaulted;

        if (pSD)
            GetSecurityDescriptorOwner(pSD, &psidDefault, &bDefaulted);

        if (psidDefault && !EqualSid(psidDefault, psid))
        {
            int iSel = AddSid(hOwner, psidDefault, m_siObjectInfo.pszServerName);

            if (iSel != -1)
            {
                ListView_SetItemState(hOwner, iSel, LVIS_SELECTED, LVIS_SELECTED);
                PropSheet_Changed(GetParent(hDlg), hDlg);
            }
        }
        LocalFree(pSD);
    }
    else
    {
        SysMsgPopup(hDlg,
                    MAKEINTRESOURCE(IDS_OPERATION_FAILED),
                    MAKEINTRESOURCE(IDS_SECURITY),
                    MB_OK | MB_ICONERROR,
                    ::hModule,
                    hr);
    }

    TraceLeaveVoid();
}

BOOL
COwnerPage::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = TRUE;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        InitDlg(hDlg);
        break;

    case WM_NOTIFY:
        {
            LPNM_LISTVIEW pnmlv = (LPNM_LISTVIEW)lParam;

            switch (((LPNMHDR)lParam)->code)
            {
            case LVN_ITEMCHANGED:
                if (pnmlv->uChanged & LVIF_STATE)
                {
                    // item *gaining* selection
                    if ((pnmlv->uNewState & LVIS_SELECTED) &&
                        !(pnmlv->uOldState & LVIS_SELECTED))
                    {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    }
                }
                break;

            case LVN_DELETEITEM:
                if (pnmlv->lParam)
                    LocalFree((PSID)pnmlv->lParam);
                break;

            case NM_SETFOCUS:
                if (((LPNMHDR)lParam)->idFrom == IDC_OWN_OWNERLIST)
                {
                    // Make sure the listview is always focused on something,
                    // otherwise you can't tab into the control.
                    HWND hwndLV = GetDlgItem(hDlg, IDC_OWN_OWNERLIST);
                    if (-1 == ListView_GetNextItem(hwndLV, -1, LVNI_FOCUSED))
                        ListView_SetItemState(hwndLV, 0, LVIS_FOCUSED, LVIS_FOCUSED);
                }
                break;

            case PSN_QUERYINITIALFOCUS:
                {
                    // Set initial focus to the list of potential owners
                    HWND hwndLV = GetDlgItem(hDlg, IDC_OWN_OWNERLIST);
                    if (IsWindowEnabled(hwndLV))
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)hwndLV);
                    else
                        bResult = FALSE;
                }
                break;

            case PSN_APPLY:
                OnApply(hDlg, (BOOL)(((LPPSHNOTIFY)lParam)->lParam));
                break;

            default:
                bResult = FALSE;
            }
        }
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_OWN_RECURSE:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                PropSheet_Changed(GetParent(hDlg), hDlg);
            break;

        case IDC_OWN_RESET:
            OnReset(hDlg);
            break;

        default:
            bResult = FALSE;
        }
        break;

    case WM_HELP:
        if (IsWindowEnabled(hDlg))
        {
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    c_szAcluiHelpFile,
                    HELP_WM_HELP,
                    (DWORD_PTR)aOwnerHelpIDs);
        }
        break;

    case WM_CONTEXTMENU:
        if (IsWindowEnabled(hDlg))
        {
            WinHelp(hDlg,
                    c_szAcluiHelpFile,
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)aOwnerHelpIDs);
        }
        break;

    default:
        bResult = FALSE;
    }

    return bResult;
}
