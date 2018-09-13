//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       acelist.cpp
//
//  This file contains the implementation for the advanced ACE list editor
//  permission and auditing pages.
//
//--------------------------------------------------------------------------

#include "aclpriv.h"
#include <accctrl.h>


LPARAM
GetSelectedItemData(HWND hListView, int *pIndex)
{
    int iSelected = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);

    if (iSelected == -1)
        return NULL;

    if (pIndex)
        *pIndex = iSelected;

    LV_ITEM lvi;

    lvi.mask     = LVIF_PARAM;
    lvi.iItem    = iSelected;
    lvi.iSubItem = 0;
    lvi.lParam   = NULL;

    ListView_GetItem(hListView, &lvi);

    return lvi.lParam;
}


void
SelectListViewItem(HWND hListView, int iSelected)
{
    ListView_SetItemState(hListView,
                          iSelected,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(hListView, iSelected, FALSE);
}

void
EnsureListViewSelectionIsVisible(HWND hListView)
{
    int iSelected = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (-1 != iSelected)
        ListView_EnsureVisible(hListView, iSelected, FALSE);
}


INT_PTR
_ConfirmAclProtectProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        Static_SetIcon(GetDlgItem(hDlg, IDC_CONFIRM_ICON), LoadIcon(NULL, IDI_QUESTION));
        return TRUE;

    case WM_COMMAND:
        if (BN_CLICKED == GET_WM_COMMAND_CMD(wParam, lParam))
        {
            EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

int
ConfirmAclProtect(HWND hwndParent, BOOL bDacl)
{
    return (int)DialogBox(::hModule,
                          MAKEINTRESOURCE(bDacl ? IDD_CONFIRM_DACL_PROTECT : IDD_CONFIRM_SACL_PROTECT),
                          hwndParent,
                          _ConfirmAclProtectProc);
}


//
//  Context Help IDs.
//
const static DWORD aAceListPermHelpIDs[] =
{
    IDC_ACEL_DETAILS,           IDH_ACEL_PERM_DETAILS,
    IDC_ACEL_ADD,               IDH_ACEL_PERM_ADD,
    IDC_ACEL_REMOVE,            IDH_ACEL_PERM_REMOVE,
    IDC_ACEL_EDIT,              IDH_ACEL_PERM_EDIT,
    IDC_ACEL_RESET,             IDH_ACEL_PERM_RESET,
    IDC_ACEL_PROTECT,           IDH_ACEL_PERM_PROTECT,
    IDC_ACEL_DESCRIPTION,       IDH_NOHELP,
    IDC_ACEL_RESET_ACL_TREE,    IDH_ACEL_PERM_RESET_ACL_TREE,
    0, 0
};

const static DWORD aAceListAuditHelpIDs[] =
{
    IDC_ACEL_DETAILS,           IDH_ACEL_AUDIT_DETAILS,
    IDC_ACEL_ADD,               IDH_ACEL_AUDIT_ADD,
    IDC_ACEL_REMOVE,            IDH_ACEL_AUDIT_REMOVE,
    IDC_ACEL_EDIT,              IDH_ACEL_AUDIT_EDIT,
    IDC_ACEL_RESET,             IDH_ACEL_AUDIT_RESET,
    IDC_ACEL_PROTECT,           IDH_ACEL_AUDIT_PROTECT,
    IDC_ACEL_DESCRIPTION,       IDH_NOHELP,
    IDC_ACEL_RESET_ACL_TREE,    IDH_ACEL_AUDIT_RESET_ACL_TREE,
    0, 0
};


class CAdvancedListPage : public CSecurityPage
{
private:
    PSI_ACCESS          m_pAccess;
    ULONG               m_cAccesses;
    PSI_INHERIT_TYPE    m_pInheritType;
    ULONG               m_cInheritTypes;
    int                 m_iLastColumnClick;
    int                 m_iSortDirection;
    BOOL                m_fPageDirty:1;
    BOOL                m_bReadOnly:1;
    BOOL                m_bAuditPolicyOK:1;
    BOOL                m_bWasDenyAcl:1;

public:
    CAdvancedListPage( LPSECURITYINFO psi, SI_PAGE_TYPE siType )
        : CSecurityPage(psi, siType), m_iLastColumnClick(0), m_iSortDirection(1) {}

private:
    virtual BOOL DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    PACL GetACL(PSECURITY_DESCRIPTOR *ppSD, LPBOOL pbProtected, BOOL bDefault);
    void FillAceList(HWND hListView, PACL pAcl, BOOL bSortList = TRUE);
    void InitDlg( HWND hDlg );
    int AddAce(HWND hListView, PACE_HEADER pAceHeader, int iRow);
    int AddAce(HWND hListView, PACE pAce, int iRow);
    LPCTSTR TranslateAceIntoRights(DWORD dwAceFlags,
                                   DWORD dwMask,
                                   const GUID *pObjectType,
                                   const GUID *pInheritedObjectType,
                                   LPCTSTR *ppszInheritType);
    LPCTSTR GetItemString(LPCTSTR pszItem, LPTSTR pszBuffer, UINT ccBuffer);
    void UpdateButtons(HWND hDlg);
    void GetPermissionDescription(PACE pAce, LPTSTR* ppszDesc);
    void BuildAcl(HWND hListView,
                  PACL *ppAcl);
    HRESULT ApplyAudits(HWND hDlg, HWND hListView, BOOL fProtected);
    HRESULT ApplyPermissions(HWND hDlg, HWND hListView, BOOL fProtected);
    void OnApply(HWND hDlg, BOOL bClose);
    void OnAdd(HWND hDlg);
    void OnRemove(HWND hDlg);
    void OnReset(HWND hDlg);
    void OnProtect(HWND hDlg);
    void OnEdit(HWND hDlg);
    int AddAcesFromDPA(HWND hListView, HDPA hEntries, int iSelected);
    void EditAce(HWND hDlg, PACE pAce, BOOL bDeleteSelection, LONG iSelected = MAXLONG);
    void CheckAuditPolicy(HWND hwndOwner);
};
typedef CAdvancedListPage *PADVANCEDLISTPAGE;


int CALLBACK
AceListCompareProc(LPARAM lParam1,
                   LPARAM lParam2,
                   LPARAM lParamSort)
{
    int iResult = 0;
    PACE pAce1 = (PACE)lParam1;
    PACE pAce2 = (PACE)lParam2;
    short iColumn = LOWORD(lParamSort);
    short iSortDirection = HIWORD(lParamSort);
    LPTSTR psz1 = NULL;
    LPTSTR psz2 = NULL;

    TraceEnter(TRACE_ACELIST, "AceListCompareProc");

    if (iSortDirection == 0)
        iSortDirection = 1;

    if (pAce1 && pAce2)
    {
        switch (iColumn)
        {
        case 0:
            iResult = pAce1->CompareType(pAce2);
        // Fall through and use the name to differentiate ACEs of the same type
        case 1:
            psz1 = pAce1->GetName();
            psz2 = pAce2->GetName();
            break;

        case 2:
            psz1 = pAce1->GetAccessType();
            psz2 = pAce2->GetAccessType();
            break;

        case 3:
            psz1 = pAce1->GetInheritType();
            psz2 = pAce2->GetInheritType();
            break;
        }

        if (iResult == 0 && psz1 && psz2)
        {
            iResult = CompareString(LOCALE_USER_DEFAULT, 0, psz1, -1, psz2, -1) - 2;
        }

        iResult *= iSortDirection;
    }

    TraceLeaveValue(iResult);
}


//
// CAdvancedListPage implementation
//
LPCTSTR
CAdvancedListPage::TranslateAceIntoRights(DWORD dwAceFlags,
                                          DWORD dwMask,
                                          const GUID *pObjectType,
                                          const GUID *pInheritedObjectType,
                                          LPCTSTR *ppszInheritType)
{
    LPCTSTR     pszName = NULL;
    PSI_ACCESS  pAccess = m_pAccess;
    ULONG       cAccess = m_cAccesses;
    UINT        iItem;

    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::TranslateAceIntoRights");
    TraceAssert(pObjectType != NULL);
    TraceAssert(pInheritedObjectType != NULL);
    TraceAssert(!m_bAbortPage);

    // If this ACE applies to a different object type, ask the client
    // for the appropriate SI_ACCESS list.
    if ((m_siObjectInfo.dwFlags & SI_OBJECT_GUID)
        && !IsNullGUID(pInheritedObjectType)
        && !IsSameGUID(pInheritedObjectType, &m_siObjectInfo.guidObjectType))
    {
        ULONG iDefaultAccess;
        DWORD dwFlags = SI_ADVANCED;
        if (m_siPageType == SI_PAGE_AUDIT)
            dwFlags |= SI_EDIT_AUDITS;
        if (FAILED(m_psi->GetAccessRights(pInheritedObjectType,
                                          dwFlags,
                                          &pAccess,
                                          &cAccess,
                                          &iDefaultAccess)))
        {
            pAccess = m_pAccess;
            cAccess = m_cAccesses;
        }
    }

    if (pAccess && cAccess)
    {
        // Look for a name for the mask
        for (iItem = 0; iItem < cAccess; iItem++)
        {
            if ( dwMask == pAccess[iItem].mask &&
                 IsSameGUID(pObjectType, pAccess[iItem].pguid) )
            {
                pszName = pAccess[iItem].pszName;
                break;
            }
        }
    }

    // Look for a name for the inheritance type
    if ((m_siObjectInfo.dwFlags & SI_CONTAINER) && ppszInheritType)
    {
        // Check these inherit bits for a match
        DWORD dwInheritMask = INHERIT_ONLY_ACE | ACE_INHERIT_ALL;

        // Don't check INHERIT_ONLY_ACE if the ACE inherit type
        // matches the current object
        if ((m_siObjectInfo.dwFlags & SI_OBJECT_GUID) &&
            IsSameGUID(&m_siObjectInfo.guidObjectType, pInheritedObjectType))
        {
            dwInheritMask &= ~INHERIT_ONLY_ACE;
        }

        *ppszInheritType = NULL;

        for (iItem = 0; iItem < m_cInheritTypes; iItem++)
        {
            if ((m_pInheritType[iItem].dwFlags & dwInheritMask) == (ULONG)(dwAceFlags & dwInheritMask)
                && IsSameGUID(pInheritedObjectType, m_pInheritType[iItem].pguid))
            {
                *ppszInheritType = m_pInheritType[iItem].pszName;
                break;
            }
        }
    }

    TraceLeaveValue(pszName);
}


LPCTSTR
CAdvancedListPage::GetItemString(LPCTSTR pszItem,
                                 LPTSTR pszBuffer,
                                 UINT ccBuffer)
{
    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::GetItemString");

    if (pszItem == NULL)
    {
        LoadString(::hModule, IDS_SPECIAL, pszBuffer, ccBuffer);
        pszItem = pszBuffer;
    }
    else if (IS_INTRESOURCE(pszItem))
    {
        if (LoadString(m_siObjectInfo.hInstance,
                       (UINT)((ULONG_PTR)pszItem),
                       pszBuffer,
                       ccBuffer) == 0)
        {
            LoadString(::hModule, IDS_SPECIAL, pszBuffer, ccBuffer);
        }
        pszItem = pszBuffer;
    }

    TraceLeaveValue(pszItem);
}


int
CAdvancedListPage::AddAce(HWND hListView, PACE_HEADER pAceHeader, int iRow)
{
    PACE pAce = new CAce(pAceHeader);
    if (pAce)
        iRow = AddAce(hListView, pAce, iRow);
    return iRow;
}

int
CAdvancedListPage::AddAce(HWND hListView, PACE pAceNew, int iRow)
{
    PACE    pAceCompare;
    TCHAR   szBuffer[MAX_COLUMN_CHARS];
    LPCTSTR pszInheritType;
    LPCTSTR pszRights;
    LV_ITEM lvi;
    UINT    id = IDS_UNKNOWN;
    int     iItem;
    int     cItems;

    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::AddAce");
    TraceAssert(hListView != NULL);
    TraceAssert(!m_bAbortPage);

    if (pAceNew == NULL)
        TraceLeaveValue(-1);

    m_psi->MapGeneric(&pAceNew->ObjectType, &pAceNew->AceFlags, &pAceNew->Mask);

    //
    // Try to merge the new ACE with an existing entry in the list.
    //
    cItems = ListView_GetItemCount(hListView);
    lvi.iSubItem = 0;
    lvi.mask = LVIF_PARAM;

    while (cItems > 0)
    {
        --cItems;
        lvi.iItem = cItems;

        ListView_GetItem(hListView, &lvi);
        pAceCompare = (PACE)lvi.lParam;

        if (pAceCompare != NULL)
        {
            switch (pAceNew->Merge(pAceCompare))
            {
            case MERGE_MODIFIED_FLAGS:
            case MERGE_MODIFIED_MASK:
                // The ACEs were merged into pAceNew.
            case MERGE_OK_1:
                //
                // The new ACE implies the existing ACE, so the existing
                // ACE can be removed.
                //
                // First copy the name so we don't have to look
                // it up again.  (Don't copy the other strings
                // since they may be different.)
                //
                // Then keep looking.  Maybe we can remove some more entries
                // before adding the new one.
                //
                if (pAceNew->GetName() == NULL)
                    pAceNew->SetName(pAceCompare->GetName());
                ListView_DeleteItem(hListView, cItems);
                iRow = cItems;  // try to insert here
                break;

            case MERGE_OK_2:
                //
                // The existing ACE implies the new ACE, so we don't
                // need to do anything here.
                //
                delete pAceNew;
                TraceLeaveValue(cItems);
                break;
            }
        }
    }


    //
    // Make sure we have a name for the SID.
    //
    pAceNew->LookupName(m_siObjectInfo.pszServerName, m_psi2);

    //
    // Get the Access Type and Inherit Type strings
    //
    pszRights = TranslateAceIntoRights(pAceNew->AceFlags,
                                       pAceNew->Mask,
                                       &pAceNew->ObjectType,
                                       &pAceNew->InheritedObjectType,
                                       &pszInheritType);

    //
    // If this is a property ACE, give it a name like "Read property" or
    // "Write property".  Also, remember that it's a property ACE so we
    // can show the Property page first when editing this ACE.
    //
    // This is a bit slimy, since it assumes DS property access bits are
    // the only ones that will ever be used on the properties page.
    //
    if ((m_siObjectInfo.dwFlags & SI_EDIT_PROPERTIES) &&
        (pAceNew->Flags & ACE_OBJECT_TYPE_PRESENT) &&
        (pAceNew->Mask & (ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP)) &&
        !(pAceNew->Mask & ~(ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP)))
    {
        pAceNew->SetPropertyAce(TRUE);

        if (pszRights == NULL)
        {
            UINT idString = 0;

            switch (pAceNew->Mask & (ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP))
            {
            case ACTRL_DS_READ_PROP:
                idString = IDS_READ_PROP;
                break;

            case ACTRL_DS_WRITE_PROP:
                idString = IDS_WRITE_PROP;
                break;

            case (ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP):
                idString = IDS_READ_WRITE_PROP;
                break;
            }

            if (idString)
            {
                LoadString(::hModule, idString, szBuffer, ARRAYSIZE(szBuffer));
                pszRights = szBuffer;
            }
        }
    }

    pszRights = GetItemString(pszRights, szBuffer, ARRAYSIZE(szBuffer));
    pAceNew->SetAccessType(pszRights);

    if (m_siObjectInfo.dwFlags & SI_CONTAINER)
    {
        pszInheritType = GetItemString(pszInheritType,
                                       szBuffer,
                                       ARRAYSIZE(szBuffer));
        pAceNew->SetInheritType(pszInheritType);
    }

    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    lvi.state = 0;
#ifdef USE_OVERLAY_IMAGE
    lvi.stateMask = LVIS_CUT | LVIS_OVERLAYMASK;
#else
    lvi.stateMask = LVIS_CUT;
#endif
    lvi.iItem = iRow;
    lvi.iSubItem = 0;
    lvi.pszText = LPSTR_TEXTCALLBACK;
    lvi.iImage = -1;
    lvi.lParam = (LPARAM)pAceNew;

    if (pAceNew->AceFlags & INHERITED_ACE)
    {
#ifdef USE_OVERLAY_IMAGE
        lvi.state = LVIS_CUT | INDEXTOOVERLAYMASK(1);
#else
        lvi.state = LVIS_CUT;
#endif
    }

    //
    // Get the image index and the string ID for the Type column
    //
    if (m_siPageType == SI_PAGE_ADVPERM)
    {
        switch(pAceNew->AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                id = IDS_ALLOW;
                lvi.iImage = 0;
                break;

            case ACCESS_DENIED_ACE_TYPE:
            case ACCESS_DENIED_OBJECT_ACE_TYPE:
                id = IDS_DENY;
                lvi.iImage = 1;
                break;

            case SYSTEM_AUDIT_ACE_TYPE:
            case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
                id = IDS_AUDIT;
                lvi.iImage = 2;
                break;

            case SYSTEM_ALARM_ACE_TYPE:
            case SYSTEM_ALARM_OBJECT_ACE_TYPE:
                id = IDS_ALARM;
                lvi.iImage = 2;
                break;
        }
    }
    else
    {
        lvi.iImage = 2;

        switch(pAceNew->AceFlags & (SUCCESSFUL_ACCESS_ACE_FLAG|FAILED_ACCESS_ACE_FLAG))
        {
            case SUCCESSFUL_ACCESS_ACE_FLAG:
                id = IDS_AUDITPASS;
                break;

            case FAILED_ACCESS_ACE_FLAG:
                id = IDS_AUDITFAIL;
                break;

            case SUCCESSFUL_ACCESS_ACE_FLAG | FAILED_ACCESS_ACE_FLAG:
                id = IDS_AUDITBOTH;
                break;
        }
    }

    // Load the Type string
    LoadString(::hModule, id, szBuffer, ARRAYSIZE(szBuffer));
    pAceNew->SetType(szBuffer);

    //
    // Finally, insert the item into the list
    //
    iItem = ListView_InsertItem(hListView, &lvi);

    if (iItem == -1)
        delete pAceNew;

    TraceLeaveValue(iItem);
}


void
CAdvancedListPage::UpdateButtons( HWND hDlg )
{
    HWND hListView;
    BOOL fEnableButtons = FALSE;
    LVITEM lvi = {0};

    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::UpdateButtons");

    hListView = GetDlgItem(hDlg, IDC_ACEL_DETAILS);

    if (!m_bAbortPage)
    {
        LPTSTR pszPermissionDescription = NULL;
        lvi.iItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);

        // Decide whether or not to enable edit button and decide what description
        // to display for the ACE
        if (lvi.iItem != -1)
        {
            fEnableButtons = TRUE;

            lvi.mask = LVIF_PARAM | LVIF_STATE;
            lvi.stateMask = LVIS_CUT;
            ListView_GetItem(hListView, &lvi);

            // Set the permission description string appropriately
            GetPermissionDescription((PACE)lvi.lParam, &pszPermissionDescription);
        }

        HWND hwndDesc = GetDlgItem(hDlg, IDC_ACEL_DESCRIPTION);
        SetWindowText(hwndDesc, pszPermissionDescription ? pszPermissionDescription : TEXT(""));
        EnableWindow(hwndDesc, NULL != pszPermissionDescription);
        LocalFreeString(&pszPermissionDescription);

        HWND hwndEdit = GetDlgItem(hDlg, IDC_ACEL_EDIT);

        // If we're disabling the edit button, make sure it doesn't have
        // focus or keyboard access gets hosed.
        if (!fEnableButtons && GetFocus() == hwndEdit)
            SetFocus(hListView);

        EnableWindow(hwndEdit, fEnableButtons);
    }

    if (m_bReadOnly)
    {
        const int idDisable[] =
        {
            IDC_ACEL_ADD,
            IDC_ACEL_REMOVE,
            IDC_ACEL_RESET,
            IDC_ACEL_PROTECT,
            IDC_ACEL_RESET_ACL_TREE,
        };
        for (int i = 0; i < ARRAYSIZE(idDisable); i++)
            EnableWindow(GetDlgItem(hDlg, idDisable[i]), FALSE);

        if (m_siPageType == SI_PAGE_ADVPERM)
        {
            TCHAR szViewTitle[64];

            // Change name of "Edit" button to "View"
            if (LoadString(::hModule, IDS_VIEW, szViewTitle, ARRAYSIZE(szViewTitle)))
                SetDlgItemText(hDlg, IDC_ACEL_EDIT, szViewTitle);
        }
        else
        {
            // If we're auditing and the read-only flag is set, then there
            // must have been an error reading the SACL or else the
            // propsheetpage callback failed the PSPCB_CREATE call.
            //
            // If we can't read the SACL, we won't be able to write it
            // either, so auditing is disabled.
        }
    }
    else
    {
        // The Remove button is disabled if the selected ACE is inherited
        if (lvi.state == LVIS_CUT)
        {
            fEnableButtons = FALSE;
        }

        HWND hwndRemove = GetDlgItem(hDlg, IDC_ACEL_REMOVE);

        // If we're disabling the remove button, make sure it doesn't have
        // focus or keyboard access gets hosed.
        if (!fEnableButtons && GetFocus() == hwndRemove)
            SetFocus(hListView);

        EnableWindow(hwndRemove, fEnableButtons);
    }

    TraceLeaveVoid();
}

void
CAdvancedListPage::GetPermissionDescription(PACE pAce, LPTSTR* ppszDesc)
{
    LPTSTR pszFirstSentence = NULL;
    LPTSTR pszSecondSentence = NULL;
    UINT idStr;
    UINT iOffset;

    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::GetPermissionDescription");
    TraceAssert(ppszDesc);

    if (NULL == pAce)
        TraceLeaveVoid();

    // First, determine if we are looking at permissions or audit entries
    // String IDs for audits are one greater than the corresponding permission, so:
    if (m_siPageType == SI_PAGE_ADVPERM)
        iOffset = 0;
    else
        iOffset = 1;

    // Load the first sentence. This sentence describes whether the permission
    // is inherited and if an inherited permission applies to this object.

    idStr = IDS_PERMISSION_LOCALACE_DESC;  // assume non-inherited
    if (pAce->AceFlags & INHERITED_ACE)
    {
        // This is an inherited ACE; it may be inherit-only
        if (pAce->AceFlags & INHERIT_ONLY_ACE)
        {
            // Inherit only!
            idStr = IDS_PERMISSION_INHERITEDONLYACE_DESC;
        }
        else
        {
            // A normal inherited ACE
            idStr = IDS_PERMISSION_INHERITEDACE_DESC;
        }
    }
    LoadStringAlloc(&pszFirstSentence, ::hModule, idStr + iOffset);


    // Now load the second sentence. This sentence tells the user if the
    // ACE will be inherited by subcontainers, subobjects, or both.

    if (pAce->AceFlags & (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE))
    {
        // Either objects or containers may inherit
        idStr = IDS_PERMISSION_DOESINHERIT_DESC;
    }
    else
    {
        // Nothing inherits
        idStr = IDS_PERMISSION_NOINHERIT_DESC;
    }
    LoadStringAlloc(&pszSecondSentence, ::hModule, idStr + iOffset);

    // Put the 2 sentences together
    if (pszFirstSentence && pszSecondSentence)
        FormatStringID(ppszDesc, ::hModule, IDS_FMT_ACE_DESC, pszFirstSentence, pszSecondSentence);

    LocalFreeString(&pszFirstSentence);
    LocalFreeString(&pszSecondSentence);

    TraceLeaveVoid();
}

PACL
CAdvancedListPage::GetACL(PSECURITY_DESCRIPTOR *ppSD, LPBOOL pbProtected, BOOL bDefault)
{
    PACL pAcl = NULL;
    SECURITY_DESCRIPTOR_CONTROL sdControl = 0;
    DWORD dwRevision;
    BOOL bPresent;
    HRESULT hr;

    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::GetACL");
    TraceAssert(ppSD != NULL);
    TraceAssert(pbProtected != NULL);
    TraceAssert(m_psi != NULL);
    TraceAssert(!m_bAbortPage);

    *pbProtected = FALSE;

    if (m_siPageType == SI_PAGE_ADVPERM)
    {
        hr = m_psi->GetSecurity(DACL_SECURITY_INFORMATION, ppSD, bDefault);

        if (SUCCEEDED(hr) && *ppSD != NULL)
        {
            GetSecurityDescriptorControl(*ppSD, &sdControl, &dwRevision);
            *pbProtected = ((sdControl & SE_DACL_PROTECTED) != 0);
            GetSecurityDescriptorDacl(*ppSD, &bPresent, &pAcl, &bDefault);
        }
    }
    else
    {
        DWORD dwPriv = SE_SECURITY_PRIVILEGE;
        HANDLE hToken = EnablePrivileges(&dwPriv, 1);

        hr = m_psi->GetSecurity(SACL_SECURITY_INFORMATION, ppSD, bDefault);

        ReleasePrivileges(hToken);

        if (SUCCEEDED(hr))
        {
            if (*ppSD != NULL)
            {
                GetSecurityDescriptorControl(*ppSD, &sdControl, &dwRevision);
                *pbProtected = ((sdControl & SE_SACL_PROTECTED) != 0);
                GetSecurityDescriptorSacl(*ppSD, &bPresent, &pAcl, &bDefault);
            }
        }
        else
        {
            // If we can't read the SACL, we can't write it either
            m_bReadOnly = TRUE;
        }
    }

    TraceLeaveValue(pAcl);
}


void
CAdvancedListPage::FillAceList(HWND hListView, PACL pAcl, BOOL bSortList)
{
    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::FillAceList");
    TraceAssert(!m_bAbortPage);

    //
    // Enumerate the ACL into the ListView
    //

    // Turn off redraw and empty out the list
    SendMessage(hListView, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hListView);

    if (pAcl)
    {
        PACE_HEADER pAceHeader;
        UINT AceCount;
        int iRow = 0;

        //
        // Enumerate all of the ACEs, putting the data into the list view
        //
        for (AceCount = pAcl->AceCount, pAceHeader = (PACE_HEADER)FirstAce(pAcl);
             AceCount > 0;
             AceCount--, pAceHeader = (PACE_HEADER)NextAce(pAceHeader))
        {
            iRow = AddAce(hListView, pAceHeader, iRow) + 1;
        }
    }

    if (bSortList)
    {
        //
        // Sort the list
        //
        ListView_SortItems(hListView,
                           AceListCompareProc,
                           MAKELPARAM(m_iLastColumnClick, m_iSortDirection));
    }

    //
    // Now select the first item
    //
    SelectListViewItem(hListView, 0);

    // Redraw the list
    SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
    ListView_RedrawItems(hListView, 0, -1);

    TraceLeaveVoid();
}


typedef struct _col_for_listview
{
    UINT    idText;     // Resource Id for column name
    UINT    iPercent;   // Percent of width
} COL_FOR_LV;

COL_FOR_LV perm_col_for_container[] =
{
    IDS_ACE_PERM_COLUMN_TYPE,       13,
    IDS_ACE_PERM_COLUMN_NAME,       31,
    IDS_ACE_PERM_COLUMN_ACCESS,     20,
    IDS_ACE_PERM_COLUMN_INHERIT,    36,
};

COL_FOR_LV perm_col_for_noncontainer[] =
{
    IDS_ACE_PERM_COLUMN_TYPE,       13,
    IDS_ACE_PERM_COLUMN_NAME,       57,
    IDS_ACE_PERM_COLUMN_ACCESS,     30,
};

COL_FOR_LV audit_col_for_container[] =
{
    IDS_ACE_AUDIT_COLUMN_TYPE,      13,
    IDS_ACE_AUDIT_COLUMN_NAME,      31,
    IDS_ACE_AUDIT_COLUMN_ACCESS,    20,
    IDS_ACE_AUDIT_COLUMN_INHERIT,   36,
};

COL_FOR_LV audit_col_for_noncontainer[] =
{
    IDS_ACE_AUDIT_COLUMN_TYPE,      13,
    IDS_ACE_AUDIT_COLUMN_NAME,      57,
    IDS_ACE_AUDIT_COLUMN_ACCESS,    30,
};


void
CAdvancedListPage::InitDlg( HWND hDlg )
{
    HWND        hListView;
    RECT        rc;
    TCHAR       szBuffer[MAX_COLUMN_CHARS];
    LV_COLUMN   col;
    UINT        iTotal = 0;
    HCURSOR     hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::InitDlg");

    // Hide the Reset button if it isn't supported.
    if (!(m_siObjectInfo.dwFlags & SI_RESET))
    {
        HWND hwnd = GetDlgItem(hDlg, IDC_ACEL_RESET);
        ShowWindow(hwnd, SW_HIDE);
        EnableWindow(hwnd, FALSE);
    }

    if (m_siObjectInfo.dwFlags & SI_NO_ACL_PROTECT)
    {
        // Hide the "Inherit permissions" box
        HWND hwnd = GetDlgItem(hDlg, IDC_ACEL_PROTECT);
        ShowWindow(hwnd, SW_HIDE);
        EnableWindow(hwnd, FALSE);
    }

    if (!(m_siObjectInfo.dwFlags & SI_CONTAINER) ||
        !(m_siObjectInfo.dwFlags & (m_siPageType == SI_PAGE_ADVPERM ? SI_RESET_DACL_TREE : SI_RESET_SACL_TREE)))
    {
        // Hide the "Reset ACL" box
        HWND hwnd = GetDlgItem(hDlg, IDC_ACEL_RESET_ACL_TREE);
        ShowWindow(hwnd, SW_HIDE);
        EnableWindow(hwnd, FALSE);
    }

    if (m_siPageType == SI_PAGE_ADVPERM)
    {
        m_bReadOnly = !!(m_siObjectInfo.dwFlags & SI_READONLY);
    }

    hListView = GetDlgItem( hDlg, IDC_ACEL_DETAILS );

    if (m_bAbortPage)
    {
        //
        // Disable everything
        //
        m_bReadOnly = TRUE;
        EnableWindow(hListView, FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_ACEL_EDIT), FALSE);
    }
    else
    {
        //
        // Get the ACL
        //
        PSECURITY_DESCRIPTOR pSD = NULL;
        BOOL        fProtected = FALSE;
        PACL        pAcl = GetACL(&pSD, &fProtected, FALSE);

        if (m_siPageType == SI_PAGE_AUDIT)
        {
            if (pAcl && pAcl->AceCount)
            {
                // Audits are already in place, don't bother checking
                // whether auditing is enabled later.
                m_bAuditPolicyOK = TRUE;
            }
        }
        else
        {
            DWORD dwFullControl = GENERIC_ALL;
            UCHAR aceFlags = 0;

            m_psi->MapGeneric(NULL, &aceFlags, &dwFullControl);
            if (IsDenyACL(pAcl,
                          fProtected,
                          dwFullControl,
                          NULL))
            {
                // Already have Deny ACEs, don't bother warning again later.
                m_bWasDenyAcl = TRUE;
            }
        }

        //
        // Set up the listview control
        //

        // Set extended LV style for whole line selection with InfoTips
        ListView_SetExtendedListViewStyleEx(hListView,
                                            LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP,
                                            LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

        // Create & set the image list for the listview
        HIMAGELIST himlLocks = LoadImageList(::hModule, MAKEINTRESOURCE(IDB_LOCKS));
#ifdef USE_OVERLAY_IMAGE
        if (himlLocks != NULL)
            ImageList_SetOverlayImage(himlLocks, ImageList_GetImageCount(himlLocks) - 1, 1);
#endif
        ListView_SetImageList(hListView, himlLocks, LVSIL_SMALL);

        //
        // Add appropriate columns
        //
        GetClientRect(hListView, &rc);
        if (pAcl && pAcl->AceCount > 10)
            rc.right -= GetSystemMetrics(SM_CYHSCROLL); // Make room for scrollbar

        COL_FOR_LV *cfl;
        UINT iColCount;

        if (m_siObjectInfo.dwFlags & SI_CONTAINER)
        {
            // Get the inherit types for filling in the inherit column
            m_cInheritTypes = 0;
            m_pInheritType = NULL;
            m_psi->GetInheritTypes(&m_pInheritType, &m_cInheritTypes);

            cfl = perm_col_for_container;
            iColCount = ARRAYSIZE(perm_col_for_container);

            if (m_siPageType == SI_PAGE_AUDIT)
            {
                cfl = audit_col_for_container;
                iColCount = ARRAYSIZE(audit_col_for_container);
            }
        }
        else
        {
            // There is no inherit column for non-containers

            cfl = perm_col_for_noncontainer;
            iColCount = ARRAYSIZE(perm_col_for_noncontainer);

            if (m_siPageType == SI_PAGE_AUDIT)
            {
                cfl = audit_col_for_noncontainer;
                iColCount = ARRAYSIZE(audit_col_for_noncontainer);
            }
        }

        UINT iCol;

        iCol = 0;
        while (iCol < iColCount)
        {
            LoadString(::hModule, cfl[iCol].idText,
                       szBuffer, ARRAYSIZE(szBuffer));
            col.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
            col.fmt = LVCFMT_LEFT;
            col.pszText = szBuffer;
            col.iSubItem = iCol;

            if (iCol == iColCount - 1)
                col.cx = rc.right - iTotal;
            else
                col.cx = (rc.right * cfl[iCol].iPercent) / 100;

            ListView_InsertColumn(hListView, iCol, &col);
            iTotal += col.cx;
            iCol++;
        }

        //
        // Get the access list for filling in the Rights column
        //
        ULONG iDefaultAccess;
        DWORD dwFlags = SI_ADVANCED;
        if (m_siPageType == SI_PAGE_AUDIT)
            dwFlags |= SI_EDIT_AUDITS;
        m_psi->GetAccessRights(NULL,
                               dwFlags,
                               &m_pAccess,
                               &m_cAccesses,
                               &iDefaultAccess);

        //
        // Enumerate the ACL into the ListView
        //
        FillAceList(hListView, pAcl, FALSE);

        // Set the protection checkbox
        CheckDlgButton(hDlg, IDC_ACEL_PROTECT, !fProtected);

        if (pSD)
            LocalFree(pSD);     // We're done with it, now free it
    } // !m_bAbortPage

    // Update the other controls
    UpdateButtons(hDlg);

    SetCursor(hcur);

    TraceLeaveVoid();
}


void
CAdvancedListPage::BuildAcl(HWND hListView,
                            PACL *ppAcl)
{
    long cAces;
    long iEntry;
    long iLength = SIZEOF(ACL);
    PACE pAce;
    PACL pACL = NULL;
    LV_ITEM lvi;
    lvi.mask     = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.lParam   = NULL;

    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::BuildAcl");
    TraceAssert(hListView != NULL);
    TraceAssert(ppAcl != NULL);

    *ppAcl = NULL;

    cAces = ListView_GetItemCount(hListView);

    //
    // Iterate through all of the ace's counting up size.
    // If there are no ACEs, create an empty ACL.
    //
    for (iEntry = 0; iEntry < cAces; iEntry++)
    {
        lvi.iItem = iEntry;
        ListView_GetItem(hListView, &lvi);

        pAce = (PACE)lvi.lParam;
        if (pAce)
        {
            if (!(pAce->AceFlags & INHERITED_ACE))
                iLength += pAce->AceSize;
        }
    }

    pACL = (PACL)LocalAlloc(LPTR, iLength);
    if (pACL)
    {
        PACE_HEADER pAceDest;

        InitializeAcl(pACL, iLength, ACL_REVISION);

        for (iEntry = 0, pAceDest = (PACE_HEADER)FirstAce(pACL);
             iEntry < cAces;
             iEntry++)
        {
            lvi.iItem = iEntry;
            ListView_GetItem(hListView, &lvi);

            pAce = (PACE)lvi.lParam;
            if (pAce)
            {
                if (!(pAce->AceFlags & INHERITED_ACE))
                {
                    //
                    // Special case for CreatorOwner/CreatorGroup,
                    // which are only useful if inherit bits are set.
                    //
                    if (IsCreatorSid(pAce->psid) && !(pAce->AceFlags & INHERIT_ONLY_ACE))
                    {
                        pAce->AceFlags |= INHERIT_ONLY_ACE;
                        if (!(pAce->AceFlags & ACE_INHERIT_ALL))
                        {
                            pAce->AceFlags |= ACE_INHERIT_ALL;
                            m_psi->MapGeneric(&pAce->ObjectType, &pAce->AceFlags, &pAce->Mask);
                        }
                    }

                    pAce->CopyTo(pAceDest);
                    pACL->AceCount++;
                    pAceDest = (PACE_HEADER)NextAce(pAceDest);

                    // Is this an object ACE?  If so, reset the ACL revision.
                    if (pAce->Flags != 0 && pACL->AclRevision < ACL_REVISION_DS)
                        pACL->AclRevision = ACL_REVISION_DS;
                }
            }
        }

        iLength = (ULONG)((PBYTE)pAceDest - (PBYTE)pACL);
        TraceAssert(pACL->AclSize >= iLength);

        if (pACL->AclSize > iLength)
            pACL->AclSize = (WORD)iLength;

        TraceAssert(IsValidAcl(pACL));
    }

    *ppAcl = pACL;

    TraceLeaveVoid();
}


HRESULT
CAdvancedListPage::ApplyPermissions(HWND hDlg,
                                    HWND hListView,
                                    BOOL fProtected)
{
    HRESULT hr = S_OK;
    PACL pACL = NULL;
    SECURITY_DESCRIPTOR sd;
    SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
    BOOL bIsDenyAcl = FALSE;

    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::ApplyPermissions");
    TraceAssert(hDlg != NULL);
    TraceAssert(hListView != NULL);
    TraceAssert(!m_bReadOnly);
    TraceAssert(!m_bAbortPage);

    if (IsDlgButtonChecked(hDlg, IDC_ACEL_RESET_ACL_TREE))
    {
        // Confirm this operation
        if (IDNO == MsgPopup(hDlg,
                             MAKEINTRESOURCE(IDS_RESET_DACL_WARNING),
                             MAKEINTRESOURCE(IDS_SECURITY),
                             MB_YESNO | MB_ICONWARNING,
                             ::hModule,
                             m_siObjectInfo.pszObjectName))
        {
            // Return PSNRET_INVALID to abort the Apply and tell the sheet to
            // select this page as the active page.
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
            ExitGracefully(hr, S_OK, "ApplyPermissions aborting");
        }

        si |= SI_RESET_DACL_TREE;
    }

    // Make sure the DACL is in canonical order.  Note that OnApply
    // will re-read the DACL and reinitialize the list with the
    // current sort order.
    ListView_SortItems(hListView,
                       AceListCompareProc,
                       MAKELPARAM(0, 1));

    // Build the new DACL
    BuildAcl(hListView, &pACL);

    // Check for Deny ACEs in the ACL
    if (!m_bWasDenyAcl)
    {
        DWORD dwWarning = 0;
        DWORD dwFullControl = GENERIC_ALL;
        UCHAR aceFlags = 0;

        m_psi->MapGeneric(NULL, &aceFlags, &dwFullControl);
        bIsDenyAcl = IsDenyACL(pACL,
                               fProtected,
                               dwFullControl,
                               &dwWarning);
        if (bIsDenyAcl)
        {
            TraceAssert(dwWarning != 0);

            // Warn the user about Deny ACEs
            if (IDNO == MsgPopup(hDlg,
                                 MAKEINTRESOURCE(dwWarning),
                                 MAKEINTRESOURCE(IDS_SECURITY),
                                 MB_YESNO | MB_ICONWARNING,
                                 ::hModule,
                                 m_siObjectInfo.pszObjectName))
            {
                // Return PSNRET_INVALID to abort the Apply and tell the sheet to
                // select this page as the active page.
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                ExitGracefully(hr, S_OK, "ApplyPermissions aborting");
            }
        }
    }

    // Build the security descriptor
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, !!(pACL), pACL, FALSE);
    sd.Control |= SE_DACL_AUTO_INHERIT_REQ;

    if (fProtected)
        sd.Control |= SE_DACL_PROTECTED;

    // Write out the new DACL
    hr = m_psi->SetSecurity(si, &sd);

    if (bIsDenyAcl && S_OK == hr)
        m_bWasDenyAcl = TRUE;

exit_gracefully:

    if (pACL)
        LocalFree(pACL);

    TraceLeaveResult(hr);
}


HRESULT
CAdvancedListPage::ApplyAudits(HWND hDlg,
                               HWND hListView,
                               BOOL fProtected)
{
    HRESULT hr = S_OK;
    PACL pACL = NULL;
    SECURITY_DESCRIPTOR sd;
    SECURITY_INFORMATION si = SACL_SECURITY_INFORMATION;
    DWORD dwPriv = SE_SECURITY_PRIVILEGE;
    HANDLE hToken = INVALID_HANDLE_VALUE;

    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::ApplyAudits");
    TraceAssert(!m_bReadOnly);
    TraceAssert(!m_bAbortPage);

    if (IsDlgButtonChecked(hDlg, IDC_ACEL_RESET_ACL_TREE))
        si |= SI_RESET_SACL_TREE;

    // Build the SACL
    BuildAcl(hListView, &pACL);

#if 0
    // Added this to fix 174167, removed it to fix 359975
    if (pACL && 0 == pACL->AceCount)
    {
        LocalFree(pACL);
        pACL = NULL;
    }
#endif

    // Build the security descriptor
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorSacl(&sd, !!(pACL), pACL, FALSE);
    sd.Control |= SE_SACL_AUTO_INHERIT_REQ;

    if (fProtected)
        sd.Control |= SE_SACL_PROTECTED;

    // Enable the security privilege and write out the new SACL
    hToken = EnablePrivileges(&dwPriv, 1);

    hr = m_psi->SetSecurity(si, &sd);

    ReleasePrivileges(hToken);

    if (pACL)
        LocalFree(pACL);

    if (S_OK == hr)
        CheckAuditPolicy(hDlg);

    TraceLeaveResult(hr);
}


void
CAdvancedListPage::OnApply(HWND hDlg, BOOL bClose)
{
    HRESULT hr = S_OK;
    HWND hListView;
    BOOL fProtected;

    if (!m_fPageDirty)
        return;

    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::OnApply");
    TraceAssert(hDlg != NULL);
    TraceAssert(!m_bReadOnly);
    TraceAssert(!m_bAbortPage);

    hListView = GetDlgItem(hDlg, IDC_ACEL_DETAILS);
    fProtected = !IsDlgButtonChecked(hDlg, IDC_ACEL_PROTECT);

    if (m_siPageType == SI_PAGE_ADVPERM)
    {
        hr = ApplyPermissions(hDlg, hListView, fProtected);
    }
    else
    {
        hr = ApplyAudits(hDlg, hListView, fProtected);
    }

    if (FAILED(hr))
    {
        // Tell the user there was a problem.  If they choose to cancel
        // and the dialog is closing, do nothing (let the dialog close).
        // Otherwise, tell the property sheet that we had a problem.
        UINT nMsgID = IDS_PERM_WRITE_FAILED;
        if (m_siPageType == SI_PAGE_AUDIT)
            nMsgID = IDS_AUDIT_WRITE_FAILED;

        if (IDCANCEL != SysMsgPopup(hDlg,
                                    MAKEINTRESOURCE(nMsgID),
                                    MAKEINTRESOURCE(IDS_SECURITY),
                                    (bClose ? MB_RETRYCANCEL : MB_OK) | MB_ICONERROR,
                                    ::hModule,
                                    hr,
                                    m_siObjectInfo.pszObjectName))
        {
            // Return PSNRET_INVALID to abort the Apply and cause the sheet to
            // select this page as the active page.
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
        }
    }
    else if (S_FALSE == hr)
    {
        // S_FALSE is silent failure (the client should put up UI
        // during SetSecurity before returning S_FALSE).
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
    }
    else
    {
        m_fPageDirty = FALSE;

        // If ApplyPermissions bailed due to user action ("No"),
        // then the dialog won't be closing
        if (PSNRET_INVALID == GetWindowLongPtr(hDlg, DWLP_MSGRESULT))
            bClose = FALSE;

        if (!bClose)
        {
            //
            // Re-read the security descriptor and reinitialize the dialog
            //
            PSECURITY_DESCRIPTOR pSD = NULL;
            BOOL        fProtected = FALSE;

            FillAceList(hListView, GetACL(&pSD, &fProtected, FALSE));

            // Set the button states
            CheckDlgButton(hDlg, IDC_ACEL_PROTECT, !fProtected);
            CheckDlgButton(hDlg, IDC_ACEL_RESET_ACL_TREE, BST_UNCHECKED);
            UpdateButtons(hDlg);

            if (pSD)
                LocalFree(pSD);     // We're done with it, now free it
        }
    }

    TraceLeaveVoid();
}

void
CAdvancedListPage::OnAdd(HWND hDlg)
{
    PUSER_LIST pUserList = NULL;

    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::OnAdd");
    TraceAssert(!m_bReadOnly);

    if (S_OK == GetUserGroup(hDlg, FALSE, &pUserList))
    {
        // Build an empty ACE (mask = 0) using the SID we just
        // got in pUserList.
        CAce ace;

        TraceAssert(NULL != pUserList);
        TraceAssert(1 == pUserList->cUsers);

        if (m_siPageType == SI_PAGE_AUDIT)
            ace.AceType = SYSTEM_AUDIT_ACE_TYPE;

        if (m_siObjectInfo.dwFlags & SI_CONTAINER)
            ace.AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;

        ace.SetSid(pUserList->rgUsers[0].pSid,
                   pUserList->rgUsers[0].pszName,
                   pUserList->rgUsers[0].pszLogonName,
                   pUserList->rgUsers[0].SidType);

        // Done with this now
        LocalFree(pUserList);

        // Edit the ACE
        EditAce(hDlg, &ace, FALSE);
    }

    TraceLeaveVoid();
}


void
CAdvancedListPage::OnRemove(HWND hDlg)
{
    HWND hListView = GetDlgItem(hDlg, IDC_ACEL_DETAILS);
    int iSelected;
    PACE pAce = (PACE)GetSelectedItemData(hListView, &iSelected);

    if (pAce && (pAce->AceFlags & INHERITED_ACE))
        return;

    if (iSelected != -1)
    {
        ListView_DeleteItem(hListView, iSelected);

        // If we deleted the last one, select the previous one
        if (ListView_GetItemCount(hListView) <= iSelected)
            --iSelected;

        SelectListViewItem(hListView, iSelected);

        PropSheet_Changed(GetParent(hDlg),hDlg);
        m_fPageDirty = TRUE;
    }
}


void
CAdvancedListPage::OnReset(HWND hDlg)
{
    //
    // Get a default ACL and enumerate it into the ListView
    //
    PSECURITY_DESCRIPTOR pSD = NULL;
    BOOL fProtected = FALSE;

    FillAceList(GetDlgItem(hDlg, IDC_ACEL_DETAILS),
                GetACL(&pSD, &fProtected, TRUE /*default*/));

    // Set the button states
    CheckDlgButton(hDlg, IDC_ACEL_PROTECT, !fProtected);
    UpdateButtons(hDlg);

    if (pSD)
        LocalFree(pSD);     // We're done with it, now free it

    // Notify the property sheet that we've changed
    PropSheet_Changed(GetParent(hDlg),hDlg);
    m_fPageDirty = TRUE;
}


void
CAdvancedListPage::OnProtect(HWND hDlg)
{
    // The "Inherit permissions" checkbox was clicked

    if (!IsDlgButtonChecked(hDlg, IDC_ACEL_PROTECT))
    {
        BOOL bHaveInheritedAces = FALSE;
        HWND hListView = GetDlgItem(hDlg, IDC_ACEL_DETAILS);
        int cItems = ListView_GetItemCount(hListView);
        int i;
        PACE pAce;
        LV_ITEM lvItem;
        lvItem.iSubItem = 0;
        lvItem.mask = LVIF_PARAM;

        // Are there any inherited aces?
        for (i = 0; i < cItems && !bHaveInheritedAces; i++)
        {
            lvItem.iItem = i;
            ListView_GetItem(hListView, &lvItem);
            pAce = (PACE)lvItem.lParam;
            if (pAce)
                bHaveInheritedAces = (pAce->AceFlags & INHERITED_ACE);
        }

        if (bHaveInheritedAces)
        {
            int nResult;
            int iSelected;

            // Turning protection on.  Ask the user whether to convert
            // inherited aces to non-inherited aces, or delete them.
            nResult = ConfirmAclProtect(hDlg, m_siPageType == SI_PAGE_ADVPERM);

            if (nResult == IDCANCEL)
            {
                // Reset the checkbox and bail
                CheckDlgButton(hDlg, IDC_ACEL_PROTECT, BST_CHECKED);
                return;
            }

            //
            // Remember the current selection, if any.
            //
            iSelected = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);

            //
            // Convert or delete inherited aces
            //
            while (cItems > 0)
            {
                --cItems;
                lvItem.iItem = cItems;

                //
                // The AddAce call below merges entries, which
                // can potentially remove entries from the list,
                // so check the return value here.  This also
                // means we could see the same item more than
                // once here, but after the first time it won't
                // have the INHERITED_ACE flag set.
                //
                if (!ListView_GetItem(hListView, &lvItem))
                    continue;

                pAce = (PACE)lvItem.lParam;

                if (pAce && (pAce->AceFlags & INHERITED_ACE))
                {
                    if (nResult == IDC_CONFIRM_REMOVE)
                    {
                        // Delete it
                        ListView_DeleteItem(hListView, cItems);
                    }
                    else
                    {
                        //
                        // Convert it to non-inherited.  Do this
                        // by deleting and re-adding without
                        // INHERITED_ACE set.  AddAce will try
                        // to merge into existing entries.
                        //
                        // Before deleting, be sure to set
                        // lParam to zero so pAce doesn't
                        // get freed.
                        //
                        pAce->AceFlags &= ~INHERITED_ACE;
                        lvItem.lParam = 0;
                        ListView_SetItem(hListView, &lvItem);
                        ListView_DeleteItem(hListView, cItems);
                        AddAce(hListView, pAce, cItems);
                    }
                }
            }

            //
            // Reset the selection
            //
            iSelected = min(ListView_GetItemCount(hListView)-1, iSelected);
            SelectListViewItem(hListView, iSelected);
        }
    }
    else
    {
        // BUGBUG warn that allowing inheritance could
        // cause new permissions to magically appear.
    }

    PropSheet_Changed(GetParent(hDlg), hDlg);
    m_fPageDirty = TRUE;
}


void
CAdvancedListPage::OnEdit(HWND hDlg)
{
    HWND hListView;
    PACE pAce;
    int  iSelected;

    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::OnEdit");
    TraceAssert(hDlg != NULL);
    TraceAssert(!m_bAbortPage);

    hListView = GetDlgItem(hDlg, IDC_ACEL_DETAILS);
    pAce = (PACE)GetSelectedItemData(hListView, &iSelected);
    EditAce(hDlg, pAce, TRUE, iSelected);

    TraceLeaveVoid();
}


int
CAdvancedListPage::AddAcesFromDPA(HWND hListView, HDPA hEntries, int iSelected)
{
    UINT iItems = 0;

    if (hEntries)
        iItems = DPA_GetPtrCount(hEntries);

    while (iItems)
    {
        --iItems;
        iSelected = AddAce(hListView,
                           (PACE_HEADER)DPA_FastGetPtr(hEntries, iItems),
                           iSelected) + 1;
    }

    return iSelected;
}

void
CAdvancedListPage::EditAce(HWND hDlg, PACE pAce, BOOL bDeleteSelection, LONG iSelected)
{
    HWND hListView;
    HDPA hEntries = NULL;
    HDPA hPropertyEntries = NULL;
    UINT iItems = 0;
    UINT iPropertyItems = 0;
    BOOL bUpdateList;
    UINT nStartPage = 0;
    DWORD dwResult = 0;

    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::EditAce");
    TraceAssert(hDlg != NULL);
    TraceAssert(!m_bAbortPage);

    hListView = GetDlgItem(hDlg, IDC_ACEL_DETAILS);

    if (pAce)
    {
        // If the ACE is inherited, don't delete it.
        if (pAce->AceFlags & INHERITED_ACE)
            bDeleteSelection = FALSE;

        // If this is a property ACE, we want to show the property page first.
        if (pAce->IsPropertyAce())
        {
            TraceAssert(m_siObjectInfo.dwFlags & SI_EDIT_PROPERTIES);
            nStartPage = 1;
        }
    }

    bUpdateList = EditACEEntry(hDlg,
                               m_psi,
                               pAce,
                               m_siPageType,
                               m_siObjectInfo.pszObjectName,
                               m_bReadOnly,
                               &dwResult,
                               &hEntries,
                               (m_siObjectInfo.dwFlags & SI_EDIT_PROPERTIES) ? &hPropertyEntries : NULL,
                               nStartPage)
                  && !m_bReadOnly;

    if (bUpdateList)
    {
        if (hEntries)
            iItems = DPA_GetPtrCount(hEntries);

        if (hPropertyEntries)
            iPropertyItems = DPA_GetPtrCount(hPropertyEntries);

        if (iItems + iPropertyItems)
        {
            if (bDeleteSelection)
            {
                if ((nStartPage == 0 && iItems != 0) || (nStartPage == 1 && iPropertyItems != 0))
                {
                    // The previous ace was modified, so delete it here.
                    ListView_DeleteItem(hListView, iSelected);
                }
                else if (iPropertyItems != 0 &&
                         !(pAce->Flags & ACE_OBJECT_TYPE_PRESENT) &&
                         (pAce->Mask & (ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP)))
                {
                    //
                    // iPropertyItems != 0 implies nStartPage = 0 or else the
                    // "if" condition above would be true.  nStartPage = 0
                    // implies iItems = 0 for the same reason.  That means the
                    // ace has more in it that just property stuff (or it's a
                    // control access right), but the only changes occurred on the
                    // Property page.  Make sure we get rid of any property bits
                    // in the original ace so that we don't incorrectly merge
                    // property changes into the original ace.
                    //
                    // An example of this:
                    // Suppose pAce->Mask == READ_CONTROL | ACTRL_DS_READ_PROP and
                    // no property GUID is present.  Suppose the user edits the ace
                    // and clicks on the Property tab, then unchecks a bunch of
                    // things, including "Read all properties".  The result
                    // "should be" a bunch of "Read <specific property>" aces.
                    // If we don't remove ACTRL_DS_READ_PROP from the original ace,
                    // then all of the "Read <specific property>" aces get merged
                    // back into the original ace for no net effect.
                    //
                    // The reverse situation (nStartPage = 1, iPropertyItems = 0,
                    // iItems != 0) is not a problem.  In that case merging will
                    // occur correctly,
                    //

                    // Make a copy with everything except the property bits
                    if (pAce->Mask & ~(ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP))
                    {
EditAce_MakeCopyWithoutProperties:
                        PACE_HEADER pAceHeader = pAce->Copy();
                        if (pAceHeader != NULL)
                        {
                            // Turn off property bits
                            ((PKNOWN_ACE)pAceHeader)->Mask &= ~(ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP);
                            TraceAssert(((PKNOWN_ACE)pAceHeader)->Mask != 0);
                            // 370573
                            // Add it to hPropertyEntries instead of hEntries,
                            // since hEntries can be NULL here but we know
                            // hPropertyEntries is non-NULL (iPropertyItems!=0)
                            DPA_AppendPtr(hPropertyEntries, pAceHeader);
                        }
                    }
                    // Delete the old ace
                    ListView_DeleteItem(hListView, iSelected);
                }
            }

            //
            // Now merge the new aces into the existing list
            //
            iSelected = AddAcesFromDPA(hListView, hEntries, iSelected);
            iSelected = AddAcesFromDPA(hListView, hPropertyEntries, iSelected);

            //
            // Now select the last item inserted
            //
            ListView_SetItemState(hListView,
                                  iSelected-1,
                                  LVIS_SELECTED | LVIS_FOCUSED,
                                  LVIS_SELECTED | LVIS_FOCUSED);

            // Re-sort the list so the new and/or modified entries
            // appear in the right place.
            ListView_SortItems(hListView,
                               AceListCompareProc,
                               MAKELPARAM(m_iLastColumnClick, m_iSortDirection));

            // After sorting, make sure the selection is visible.
            EnsureListViewSelectionIsVisible(hListView);
        }
        else if (bDeleteSelection && dwResult)
        {
            // Everything succeeded, something was edited, but nothing was created
            // (probably all boxes were unchecked).  Delete the previous selection.

            // 370573
            // If the only change occurred on the Properties page, then we
            // only want to turn off the property bits. We don't want to
            // delete the whole thing.
            if (EAE_NEW_PROPERTY_ACE == dwResult &&
                (pAce->Mask & ~(ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP)))
            {
                if (!hPropertyEntries)
                    hPropertyEntries = DPA_Create(1);
                if (hPropertyEntries)
                    goto EditAce_MakeCopyWithoutProperties;
            }

            // Delete the previous selection.
            ListView_DeleteItem(hListView, iSelected);
        }

        // Was anything edited?
        if (dwResult)
        {
            PropSheet_Changed(GetParent(hDlg),hDlg);
            m_fPageDirty = TRUE;
        }
    }

    if (hEntries)
        DestroyDPA(hEntries);

    if (hPropertyEntries)
        DestroyDPA(hPropertyEntries);

    TraceLeaveVoid();
}


void
CAdvancedListPage::CheckAuditPolicy(HWND hwndOwner)
{
    //
    // Check whether auditing is turned on and warn the user if not.
    //
    TraceEnter(TRACE_ACELIST, "CAdvancedListPage::CheckAuditPolicy");

    if (!m_bAuditPolicyOK)
    {
        LSA_HANDLE hPolicy = GetLSAConnection(m_siObjectInfo.pszServerName,
                                              POLICY_VIEW_AUDIT_INFORMATION);

        if (hPolicy != NULL)
        {
            PPOLICY_AUDIT_EVENTS_INFO pAuditInfo = NULL;

            LsaQueryInformationPolicy(hPolicy,
                                      PolicyAuditEventsInformation,
                                      (PVOID*)&pAuditInfo);

            if (pAuditInfo != NULL)
            {
                // We don't need to do this work again
                m_bAuditPolicyOK = TRUE;

                if (!pAuditInfo->AuditingMode)
                {
                    // Auditing is not on... warn the user
                    MsgPopup(hwndOwner,
                             MAKEINTRESOURCE(IDS_AUDIT_OFF_WARNING),
                             MAKEINTRESOURCE(IDS_SECURITY),
                             MB_OK | MB_ICONWARNING,
                             ::hModule);
                }

                LsaFreeMemory(pAuditInfo);
            }
            else
            {
                TraceMsg("LsaQueryInformationPolicy failed");
            }

            LsaClose(hPolicy);
        }
        else
        {
            TraceMsg("Unable to open LSA policy handle");
        }
    }

    TraceLeaveVoid();
}


BOOL
CAdvancedListPage::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PACE pAce;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        InitDlg(hDlg);
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            LPNM_LISTVIEW pnmlv = (LPNM_LISTVIEW)lParam;

            // Set default return value
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

            switch (pnmh->code)
            {
            case NM_DBLCLK:
                if (pnmh->idFrom == IDC_ACEL_DETAILS)
                    OnEdit(hDlg);
                break;

            case LVN_ITEMCHANGED:
                if (pnmlv->uChanged & LVIF_STATE)
                    UpdateButtons(hDlg);
                break;

            case LVN_DELETEITEM:
                pAce = (PACE)pnmlv->lParam;
                delete pAce;
                break;

            case LVN_KEYDOWN:
                if (((LPNMLVKEYDOWN)pnmh)->wVKey == VK_DELETE)
                    SendMessage(hDlg,
                                WM_COMMAND,
                                GET_WM_COMMAND_MPS(IDC_ACEL_REMOVE, NULL, 0));
                break;

#define lvi (((NMLVDISPINFO*)lParam)->item)

            case LVN_GETDISPINFO:
                pAce = (PACE)lvi.lParam;
                if ((lvi.mask & LVIF_TEXT) && pAce)
                {
                    switch (lvi.iSubItem)
                    {
                    case 0:
                        lvi.pszText = pAce->GetType();
                        break;

                    case 1:
                        lvi.pszText = pAce->GetName();
                        break;

                    case 2:
                        lvi.pszText = pAce->GetAccessType();
                        break;

                    case 3:
                        lvi.pszText = pAce->GetInheritType();
                        break;
                    }
                }
                break;
#undef lvi

            case LVN_COLUMNCLICK:
                if (m_iLastColumnClick == pnmlv->iSubItem)
                    m_iSortDirection = -m_iSortDirection;
                else
                    m_iSortDirection = 1;
                m_iLastColumnClick = pnmlv->iSubItem;
                ListView_SortItems(pnmh->hwndFrom,
                                   AceListCompareProc,
                                   MAKELPARAM(m_iLastColumnClick, m_iSortDirection));
                EnsureListViewSelectionIsVisible(pnmh->hwndFrom);
                break;

            case PSN_APPLY:
                OnApply(hDlg, (BOOL)(((LPPSHNOTIFY)lParam)->lParam));
                break;
            }
        }
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_ACEL_ADD:
            OnAdd(hDlg);
            break;

        case IDC_ACEL_REMOVE:
            OnRemove(hDlg);
            break;

        case IDC_ACEL_EDIT:
            OnEdit(hDlg);
            break;

        case IDC_ACEL_RESET:
            OnReset(hDlg);
            break;

        case IDC_ACEL_RESET_ACL_TREE:
            if (!m_fPageDirty)
            {
                PropSheet_Changed(GetParent(hDlg),hDlg);
                m_fPageDirty = TRUE;
            }
            break;

        case IDC_ACEL_PROTECT:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED
                && !m_bReadOnly)
            {
                OnProtect(hDlg);
            }
            break;

        default:
            return FALSE;
        }
        break;

    case WM_HELP:
        if (IsWindowEnabled(hDlg))
        {
            const DWORD *pdwHelpIDs = aAceListPermHelpIDs;

            if (m_siPageType == SI_PAGE_AUDIT)
                pdwHelpIDs = aAceListAuditHelpIDs;

            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    c_szAcluiHelpFile,
                    HELP_WM_HELP,
                    (DWORD_PTR)pdwHelpIDs);
        }
        break;

    case WM_CONTEXTMENU:
        if (IsWindowEnabled(hDlg))
        {
            const DWORD *pdwHelpIDs = aAceListPermHelpIDs;

            if (m_siPageType == SI_PAGE_AUDIT)
                pdwHelpIDs = aAceListAuditHelpIDs;

            WinHelp(hDlg,
                    c_szAcluiHelpFile,
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)pdwHelpIDs);
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

HPROPSHEETPAGE
CreateAdvPermissionPage( LPSECURITYINFO psi )
{
    HPROPSHEETPAGE hPage = NULL;
    PADVANCEDLISTPAGE pPage;

    TraceEnter(TRACE_ACELIST, "CreateAdvPermissionPage");

    pPage = new CAdvancedListPage( psi, SI_PAGE_ADVPERM );

    if (pPage)
    {
        hPage = pPage->CreatePropSheetPage(MAKEINTRESOURCE(IDD_ACELIST_PERM_PAGE));

        if (!hPage)
            delete pPage;
    }

    TraceLeaveValue(hPage);
}

HPROPSHEETPAGE
CreateAdvAuditPage( LPSECURITYINFO psi )
{
    HPROPSHEETPAGE hPage = NULL;
    PADVANCEDLISTPAGE pPage;

    TraceEnter(TRACE_ACELIST, "CreateAdvAuditPage");

    pPage = new CAdvancedListPage( psi, SI_PAGE_AUDIT );

    if (pPage)
    {
        hPage = pPage->CreatePropSheetPage(MAKEINTRESOURCE(IDD_ACELIST_AUDIT_PAGE));

        if (!hPage)
            delete pPage;
    }

    TraceLeaveValue(hPage);
}


//
// Expose an api to get at the ace list editor
//
BOOL
ACLUIAPI
EditSecurityEx( HWND hwndOwner, LPSECURITYINFO psi, UINT nStartPage )
{
    HPROPSHEETPAGE hPage[3];
    UINT cPages = 0;
    BOOL bResult = FALSE;
    SI_OBJECT_INFO siObjectInfo = {0};
    HRESULT hr;

    TraceEnter(TRACE_ACELIST, "EditSecurityEx");

    // Get flags and object name information
    hr = psi->GetObjectInformation(&siObjectInfo);

    if (FAILED(hr))
    {
        SysMsgPopup(hwndOwner,
                    MAKEINTRESOURCE(IDS_OPERATION_FAILED),
                    MAKEINTRESOURCE(IDS_SECURITY),
                    MB_OK | MB_ICONERROR,
                    ::hModule,
                    hr);
        TraceLeaveValue(FALSE);
    }

    hPage[cPages] = CreateAdvPermissionPage( psi );
    if (hPage[cPages])
        cPages++;

    if (siObjectInfo.dwFlags & SI_EDIT_AUDITS)
    {
        hPage[cPages] = CreateAdvAuditPage( psi );
        if (hPage[cPages])
            cPages++;
    }

    if (siObjectInfo.dwFlags & SI_EDIT_OWNER)
    {
        hPage[cPages] = CreateOwnerPage( psi, &siObjectInfo );
        if (hPage[cPages])
            cPages++;
    }

    if (cPages)
    {
        // Build dialog title string
        LPTSTR pszCaption = NULL;
        FormatStringID(&pszCaption, ::hModule, IDS_ACEL_TITLE, siObjectInfo.pszObjectName);

        PROPSHEETHEADER psh = {0};
        psh.dwSize = SIZEOF(psh);
        psh.dwFlags = PSH_DEFAULT;
        psh.hwndParent = hwndOwner;
        psh.hInstance = ::hModule;
        psh.pszCaption = pszCaption;
        psh.nPages = cPages;
        psh.nStartPage = 0;
        psh.phpage = &hPage[0];

        if (nStartPage < cPages)
            psh.nStartPage = nStartPage;

        bResult = (BOOL)(PropertySheet(&psh) + 1);

        LocalFreeString(&pszCaption);
    }

    TraceLeaveValue(bResult);
}
