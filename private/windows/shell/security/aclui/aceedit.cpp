//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       aceedit.cpp
//
//  This file contains the implementation for the advanced ACE editor
//  page.
//
//--------------------------------------------------------------------------

#include "aclpriv.h"
#include "sddl.h"       // ConvertSidToStringSid

#define PWM_SELECT_PAGE             (WM_APP - 1)

//
//  Context Help IDs.
//
const static DWORD aAcePermHelpIDs[] =
{
    IDC_ACEE_INHERITWARNING,        IDH_NOHELP,
    IDC_ACEE_NAME_STATIC,           IDH_ACEE_PERM_NAME,
    IDC_ACEE_NAME,                  IDH_ACEE_PERM_NAME,
    IDC_ACEE_NAMEBUTTON,            IDH_ACEE_PERM_NAMEBUTTON,
    IDC_ACEE_APPLYONTO_STATIC,      IDH_ACEE_PERM_INHERITTYPE,
    IDC_ACEE_INHERITTYPE,           IDH_ACEE_PERM_INHERITTYPE,
    IDC_ACEE_ACCESS,                IDH_ACEE_PERM_LIST,
    IDC_ACEE_ALLOW,                 IDH_ACEE_PERM_LIST,
    IDC_ACEE_DENY,                  IDH_ACEE_PERM_LIST,
    IDC_ACEE_LIST,                  IDH_ACEE_PERM_LIST,
    IDC_ACEE_INHERITIMMEDIATE,      IDH_ACEE_PERM_INHERITIMMEDIATE,
    IDC_ACEE_CLEAR,                 IDH_ACEE_PERM_CLEAR,
    0, 0
};
const static DWORD aAceAuditHelpIDs[] =
{
    IDC_ACEE_INHERITWARNING,        IDH_NOHELP,
    IDC_ACEE_NAME_STATIC,           IDH_ACEE_AUDIT_NAME,
    IDC_ACEE_NAME,                  IDH_ACEE_AUDIT_NAME,
    IDC_ACEE_NAMEBUTTON,            IDH_ACEE_AUDIT_NAMEBUTTON,
    IDC_ACEE_APPLYONTO_STATIC,      IDH_ACEE_AUDIT_INHERITTYPE,
    IDC_ACEE_INHERITTYPE,           IDH_ACEE_AUDIT_INHERITTYPE,
    IDC_ACEE_ACCESS,                IDH_ACEE_AUDIT_LIST,
    IDC_ACEE_ALLOW,                 IDH_ACEE_AUDIT_LIST,
    IDC_ACEE_DENY,                  IDH_ACEE_AUDIT_LIST,
    IDC_ACEE_LIST,                  IDH_ACEE_AUDIT_LIST,
    IDC_ACEE_INHERITIMMEDIATE,      IDH_ACEE_AUDIT_INHERITIMMEDIATE,
    IDC_ACEE_CLEAR,                 IDH_ACEE_AUDIT_CLEAR,
    0, 0
};


class CACEPage : public CSecurityPage
{
private:
    PACE            m_pAce;
    HDPA           *m_phEntries;
    PSID            m_pSid;
    DWORD           m_siFlags;
    DWORD          *m_pdwResult;
    GUID            m_guidInheritType;
    BOOL            m_fInheritImmediateEnabled;
    BOOL            m_fPreviousImmediateSetting;
    BOOL            m_fReadOnly;
    BOOL            m_fPageDirty;
    SI_INHERIT_TYPE m_siInheritUnknown;

public:
    CACEPage(LPSECURITYINFO psi,
             SI_PAGE_TYPE siType,
             PACE pAce,
             BOOL bReadOnly,
             DWORD dwFlags,
             DWORD *pdwResult,
             HDPA *phEntries);
    virtual ~CACEPage();

private:
    virtual BOOL DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Prevent propsheet callbacks from reaching the object that invoked us
    virtual UINT PSPageCallback(HWND, UINT, LPPROPSHEETPAGE) { return 1; }

    void EmptyCheckList(HWND hwndList);
    LONG CheckPermBoxes(HWND hwndList, PACE pAce, DWORD dwState);
    LONG InitCheckList(HWND hDlg, PACE pAce);
    LONG ReInitCheckList(HWND hDlg, HDPA hEntries);
    void HideInheritedAceWarning(HWND hDlg);
    void InitDlg(HWND hDlg);
    BOOL OnChangeName(HWND hDlg);
    BOOL OnClearAll(HWND hDlg);
    void HandleSelChange(HWND hDlg, HWND hWnd);
    LONG OnApply(HWND hDlg, BOOL bClose);
};
typedef class CACEPage *LPACEPAGE;

CACEPage::CACEPage(LPSECURITYINFO psi,
                   SI_PAGE_TYPE siType,
                   PACE pAce,
                   BOOL bReadOnly,
                   DWORD dwFlags,
                   DWORD *pdwResult,
                   HDPA *phEntries)
: CSecurityPage(psi, siType), m_pAce(pAce), m_fReadOnly(bReadOnly),
    m_siFlags(dwFlags), m_pdwResult(pdwResult), m_phEntries(phEntries)
{
    if (m_pdwResult)
        *m_pdwResult = 0;
}

CACEPage::~CACEPage()
{
    if (m_pSid)
        LocalFree(m_pSid);
}


void
CACEPage::EmptyCheckList(HWND hwndList)
{
    SendMessage(hwndList, CLM_RESETCONTENT, 0, 0);
}


LONG
CACEPage::CheckPermBoxes(HWND hwndList, PACE pAce, DWORD dwState)
{
    LONG nLastChecked = -1;
    UINT cItems;
    BOOL bColumnAllow = FALSE;
    BOOL bColumnDeny = FALSE;

    //
    // Check all boxes that correspond to a particular ACE
    //

    if (hwndList == NULL || pAce == NULL)
        return -1;

    if (pAce->AceFlags & INHERITED_ACE)
        dwState |= CLST_DISABLED;

    if (m_siPageType == SI_PAGE_ADVPERM)
    {
        // Only check one column (either allow or deny)

        if (IsEqualACEType(pAce->AceType, ACCESS_ALLOWED_ACE_TYPE))
            bColumnAllow = TRUE;        // Access allowed
        else if (IsEqualACEType(pAce->AceType, ACCESS_DENIED_ACE_TYPE))
            bColumnDeny = TRUE;         // Access denied
        else
            return -1;  // Bogus ACE
    }
    else if (m_siPageType == SI_PAGE_AUDIT)
    {
        // Either or both columns can be checked for audits

        if (pAce->AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG)
            bColumnAllow = TRUE;        // Audit successful access

        if (pAce->AceFlags & FAILED_ACCESS_ACE_FLAG)
            bColumnDeny = TRUE;         // Audit failed access
    }
    else
        return -1;

    cItems = (UINT)SendMessage(hwndList, CLM_GETITEMCOUNT, 0, 0);

    while (cItems > 0)
    {
        PSI_ACCESS pAccess;

        --cItems;
        pAccess = (PSI_ACCESS)SendMessage(hwndList, CLM_GETITEMDATA, cItems, 0);

        //
        // The below expression tests to see if this access mask enables
        // this access "rights" line.  It could have more bits enabled, but
        // as long as it has all of the ones from the pAccess->mask then
        // it effectively has that option enabled.
        //

        if (pAccess &&
            AllFlagsOn(pAce->Mask, pAccess->mask) &&
            (!(pAce->Flags & ACE_OBJECT_TYPE_PRESENT) ||
             IsSameGUID(pAccess->pguid, &pAce->ObjectType)))
        {
            WPARAM wItem;

            nLastChecked = cItems;

            // BUGBUG We should probably make sure we aren't checking the
            // same item in both columns for permissions.  (Audits can
            // have both columns checked.)

            if (bColumnAllow)
            {
                wItem = MAKELONG((WORD)cItems, COLUMN_ALLOW);
                SendMessage(hwndList,
                            CLM_SETSTATE,
                            wItem,
                            (LPARAM)dwState);
            }

            if (bColumnDeny)
            {
                wItem = MAKELONG((WORD)cItems, COLUMN_DENY);
                SendMessage(hwndList,
                            CLM_SETSTATE,
                            wItem,
                            (LPARAM)dwState);
            }
        }
    }

    return nLastChecked;
}


LONG
CACEPage::InitCheckList(HWND hDlg, PACE pAce)
{
    LONG nTopItemChecked;
    HDPA hList = NULL;

    TraceEnter(TRACE_ACEEDIT, "CACEPage::InitCheckList");
    TraceAssert(hDlg != NULL);

    if (m_siPageType == SI_PAGE_AUDIT)
        SendDlgItemMessage(hDlg, IDC_ACEE_LIST, CLM_SETCOLUMNWIDTH, 0, 40);

    hList = DPA_Create(1);
    if (hList && pAce)
        DPA_AppendPtr(hList, pAce->Copy());

    nTopItemChecked = ReInitCheckList(hDlg, hList);

    if (hList)
        DestroyDPA(hList);

    TraceLeaveValue(nTopItemChecked);
}


LONG
CACEPage::ReInitCheckList(HWND hDlg, HDPA hEntries)
{
    LONG nTopItemChecked = -1;
    HWND hwndList;
    DWORD dwFlags;
    HRESULT hr;
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    TraceEnter(TRACE_ACEEDIT, "CACEPage::ReInitCheckList");
    TraceAssert(hDlg != NULL);

    hwndList = GetDlgItem(hDlg, IDC_ACEE_LIST);

    EmptyCheckList(hwndList);

    dwFlags = SI_ADVANCED;

    if (m_siPageType == SI_PAGE_AUDIT)
        dwFlags |= SI_EDIT_AUDITS;

    if (m_siFlags == SI_ACCESS_PROPERTY)
        dwFlags |= SI_EDIT_PROPERTIES;

    //
    // Enumerate the permissions and add to the checklist
    //
    hr = _InitCheckList(hwndList,
                        m_psi,
                        &m_guidInheritType,
                        dwFlags,
                        m_siObjectInfo.hInstance,
                        m_siFlags,
                        NULL);
    if (SUCCEEDED(hr))
    {
        UINT cItems = (UINT)SendMessage(hwndList, CLM_GETITEMCOUNT, 0, 0);
        ULONG cAces = 0;
        
        if (hEntries)
            cAces = DPA_GetPtrCount(hEntries);

        //
        // Check the appropriate boxes
        //
        nTopItemChecked = MAXLONG;
        while (cAces > 0)
        {
            PACE_HEADER pAceHeader;

            --cAces;
            pAceHeader = (PACE_HEADER)DPA_FastGetPtr(hEntries, cAces);
            if (pAceHeader)
            {
                CAce Ace(pAceHeader);
                LONG nTop = CheckPermBoxes(hwndList, &Ace, CLST_CHECKED);
                if (-1 != nTop)
                    nTopItemChecked = min(nTopItemChecked, nTop);
            }
        }
        if (MAXLONG == nTopItemChecked)
            nTopItemChecked = -1;

        // Make sure the top item checked is scrolled into view.
        // (-1 scrolls to the top, same as 0.)
        SendMessage(hwndList, CLM_ENSUREVISIBLE, nTopItemChecked, 0);

        // Disable all of the boxes if we're in read-only mode
        if (m_fReadOnly)
            SendMessage(hwndList, WM_ENABLE, FALSE, 0);
    }

    SetCursor(hcur);
    TraceLeaveValue(nTopItemChecked);
}

void CACEPage::HideInheritedAceWarning(HWND hDlg)
// Hides the message informing the user that the current ACE is inherited from
// the parent. Also moves and resizes controls as appropriate.
{
    // Array of control IDs to move up
    static UINT rgMoveControls[] =
    {
        IDC_ACEE_NAME_STATIC,
        IDC_ACEE_NAME,
        IDC_ACEE_NAMEBUTTON,
        IDC_ACEE_APPLYONTO_STATIC,
        IDC_ACEE_INHERITTYPE,
        IDC_ACEE_ACCESS,
        IDC_ACEE_ALLOW,
        IDC_ACEE_DENY,
    };

    // Get the message window dimensions
    HWND hwndControl = GetDlgItem(hDlg, IDC_ACEE_INHERITWARNING);
    RECT rect;
    GetWindowRect(hwndControl, &rect);

    // We need to move controls up this amount:
    int nMoveUpAmount = rect.bottom - rect.top;

    // Rather than hide the message window, destroy it altogether so WinHelp
    // doesn't confuse it with the "Name:" static during WM_CONTEXTMENU.
    DestroyWindow(hwndControl);

    // Move each of the controls we need to move up
    for (int nControl = 0; nControl < ARRAYSIZE(rgMoveControls); nControl++)
    {
        hwndControl = GetDlgItem(hDlg, rgMoveControls[nControl]);
        GetWindowRect(hwndControl, &rect);
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rect, 2);
        SetWindowPos(hwndControl,
                     NULL,
                     rect.left,
                     rect.top - nMoveUpAmount,
                     0,
                     0,
                     SWP_NOSIZE | SWP_NOZORDER);
    }

    // Finally, we need to resize the list control, including adjusting its height
    hwndControl = GetDlgItem(hDlg, IDC_ACEE_LIST);
    GetWindowRect(hwndControl, &rect);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&rect, 2);
    SetWindowPos(hwndControl,
                 NULL,
                 rect.left,
                 rect.top - nMoveUpAmount,
                 rect.right - rect.left,
                 rect.bottom - (rect.top - nMoveUpAmount),
                 SWP_NOZORDER);
}


//
// Default "Apply onto" strings for when GetInheritTypes
// fails or we don't find a matching inherit type.
//
// If desirable, different strings can be created for
// CONTAINER_INHERIT_ACE vs OBJECT_INHERIT_ACE.
//
static const UINT s_aInheritTypes[] =
{
    IDS_THIS_OBJECT_ONLY,           // 0 = <no inheritance>
    IDS_THIS_OBJECT_AND_SUBOBJECTS, // 1 = OBJECT_INHERIT_ACE
    IDS_THIS_OBJECT_AND_SUBOBJECTS, // 2 = CONTAINER_INHERIT_ACE
    IDS_THIS_OBJECT_AND_SUBOBJECTS, // 3 = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
};

// These are used when INHERIT_ONLY_ACE is present
static const UINT s_aInheritOnlyTypes[] =
{
    IDS_INVALID_INHERIT,            // 0 = <invalid>
    IDS_SUBOBJECTS_ONLY,            // 1 = OBJECT_INHERIT_ACE
    IDS_SUBOBJECTS_ONLY,            // 2 = CONTAINER_INHERIT_ACE
    IDS_SUBOBJECTS_ONLY,            // 3 = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
};


static int
_AddInheritType(HWND hInheritType,
                PSI_INHERIT_TYPE psiInheritType,
                HINSTANCE hInstance)
{
    UINT iIndex;
    TCHAR szName[MAX_PATH];
    LPCTSTR pszName = psiInheritType->pszName;

    if (IS_INTRESOURCE(pszName))
    {
        if (LoadString(hInstance,
                       (ULONG)((ULONG_PTR)pszName),
                       szName,
                       ARRAYSIZE(szName)) == 0)
        {
            LoadString(::hModule,
                       IDS_UNKNOWN,
                       szName,
                       ARRAYSIZE(szName));
        }
        pszName = szName;
    }

    iIndex = (UINT)SendMessage(hInheritType, CB_ADDSTRING, 0, (LPARAM)pszName);

    if (CB_ERR != iIndex)
        SendMessage(hInheritType, CB_SETITEMDATA, iIndex, (LPARAM)psiInheritType);

    return iIndex;
}

void
CACEPage::InitDlg(HWND hDlg)
{
    UCHAR   AceFlags = 0;
    PSID    pSid = NULL;
    LPCTSTR pszName = NULL;
    LPTSTR  pszNameT = NULL;
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    HRESULT hr;

    TraceEnter(TRACE_ACEEDIT, "CACEPage::InitDlg");

    if (m_pAce)
    {
        AceFlags = m_pAce->AceFlags;
        m_guidInheritType = m_pAce->InheritedObjectType;
        pSid = m_pAce->psid;
        pszName = m_pAce->LookupName(m_siObjectInfo.pszServerName, m_psi2);
    }
    else
    {
        if (m_siObjectInfo.dwFlags & SI_CONTAINER)
            AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;

        pSid = QuerySystemSid(UI_SID_World);
    }

    // Make sure the AceFlags are valid
    ACCESS_MASK Mask = 0;
    m_psi->MapGeneric(&m_guidInheritType, &AceFlags, &Mask);

    // Hide the inherit warning and adjust other control positions
    if (!(AceFlags & INHERITED_ACE))
        HideInheritedAceWarning(hDlg);

    // Make a copy of the sid and get the name
    if (pSid)
    {
        PUSER_LIST pUserList = NULL;

        m_pSid = LocalAllocSid(pSid);

        if (pszName == NULL)
        {
            // This should only happen when m_pAce is NULL and we're
            // using UI_SID_World
            if (LookupSid(pSid, m_siObjectInfo.pszServerName, m_psi2, &pUserList))
            {
                TraceAssert(NULL != pUserList);
                TraceAssert(1 == pUserList->cUsers);

                if (BuildUserDisplayName(&pszNameT,
                                         pUserList->rgUsers[0].pszName,
                                         pUserList->rgUsers[0].pszLogonName)
                    || ConvertSidToStringSid(pSid, &pszNameT))
                {
                    pszName = pszNameT;
                }
            }
        }

        SetDlgItemText(hDlg, IDC_ACEE_NAME, pszName);

        if (NULL != pUserList)
            LocalFree(pUserList);
    }

    // Get the list of permissions and initialize the check boxes
    LONG nTopChecked = InitCheckList(hDlg, m_pAce);

    if (-1 == nTopChecked &&
        m_pAce &&
        m_pAce->Mask &&
        (m_siObjectInfo.dwFlags & SI_EDIT_PROPERTIES) &&
        m_siFlags != SI_ACCESS_PROPERTY)
    {
        // Must be a property ACE, switch to the Properties page
        PostMessage(hDlg, PWM_SELECT_PAGE, 1, 0);
    }

    HWND hInheritType = GetDlgItem(hDlg, IDC_ACEE_INHERITTYPE);
    HWND hInheritImmed = GetDlgItem(hDlg, IDC_ACEE_INHERITIMMEDIATE);

    if (m_siObjectInfo.dwFlags & SI_NO_TREE_APPLY)
    {
        ShowWindow(hInheritImmed, SW_HIDE);
        EnableWindow(hInheritImmed, FALSE);
    }

    //
    // Get inherit types from callback
    //
    ULONG cItems = 0;
    PSI_INHERIT_TYPE psiInheritType = NULL;

    hr = m_psi->GetInheritTypes(&psiInheritType, &cItems);
    if (SUCCEEDED(hr))
    {
        // Check these inherit bits for a match
        DWORD dwInheritMask = INHERIT_ONLY_ACE | ACE_INHERIT_ALL;

        // Don't check INHERIT_ONLY_ACE if the ACE inherit type
        // matches the current object
        if ((m_siObjectInfo.dwFlags & SI_OBJECT_GUID) &&
            IsSameGUID(&m_siObjectInfo.guidObjectType, &m_guidInheritType))
        {
            dwInheritMask &= ~INHERIT_ONLY_ACE;
        }

        //
        // Add inherit types to combobox
        //
        for ( ; cItems > 0; cItems--, psiInheritType++)
        {
            UINT iIndex = _AddInheritType(hInheritType,
                                          psiInheritType,
                                          m_siObjectInfo.hInstance);

            // See if this entry matches the incoming ACE
            if ((psiInheritType->dwFlags & dwInheritMask) == (ULONG)(AceFlags & dwInheritMask)
                && IsSameGUID(&m_guidInheritType, psiInheritType->pguid))
            {
                // Got a match, select this entry
                SendMessage(hInheritType, CB_SETCURSEL, iIndex, 0);
            }
        }
    }

    //
    // If GetInheritTypes failed, or we failed to find a match,
    // pick a default string and build an appropriate inherit type.
    //
    if (FAILED(hr) || CB_ERR == SendMessage(hInheritType, CB_GETCURSEL, 0, 0))
    {
        // Pick a default string
        UINT ids = IDS_SPECIAL;
        if (IsNullGUID(&m_guidInheritType))
        {
            if (AceFlags & INHERIT_ONLY_ACE)
                ids = s_aInheritOnlyTypes[AceFlags & ACE_INHERIT_ALL];
            else
                ids = s_aInheritTypes[AceFlags & ACE_INHERIT_ALL];
        }

        // Fill in m_siInheritUnknown with the pertinent info
        m_siInheritUnknown.pguid   = &m_guidInheritType;
        m_siInheritUnknown.dwFlags = AceFlags & (INHERIT_ONLY_ACE | ACE_INHERIT_ALL);
        m_siInheritUnknown.pszName = MAKEINTRESOURCE(ids);

        // Insert and select it
        UINT iIndex = _AddInheritType(hInheritType,
                                      &m_siInheritUnknown,
                                      ::hModule);
        SendMessage(hInheritType, CB_SETCURSEL, iIndex, 0);

        if (FAILED(hr))
        {
            // GetInheritTypes failed, which means the only entry is the
            // default one we just added.  Disable the combo.
            EnableWindow(hInheritType, FALSE);
        }
    }


    //
    // Select the options which match the incoming ace
    //

    if (!(AceFlags & (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE)))
    {
        SendMessage(hInheritImmed, BM_SETCHECK, BST_UNCHECKED, 0);
        EnableWindow(hInheritImmed, FALSE);
        m_fInheritImmediateEnabled = FALSE;
        m_fPreviousImmediateSetting = BST_UNCHECKED;
    }
    else
    {
        SendMessage(hInheritImmed,
                    BM_SETCHECK,
                    (AceFlags & NO_PROPAGATE_INHERIT_ACE) ? BST_CHECKED : BST_UNCHECKED,
                    0);
        m_fInheritImmediateEnabled = TRUE;
    }

    if (!(m_siObjectInfo.dwFlags & SI_CONTAINER) || m_fReadOnly || (AceFlags & INHERITED_ACE))
    {
        // Disable all inheritance
        EnableWindow(hInheritType, FALSE);
        EnableWindow(hInheritImmed, FALSE);
    }

    if (m_fReadOnly || (AceFlags & INHERITED_ACE))
    {
        // Disable the "change name" and "clear all" buttons
        EnableWindow(GetDlgItem(hDlg, IDC_ACEE_NAMEBUTTON), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_ACEE_CLEAR), FALSE);
    }

    LocalFreeString(&pszNameT);
    SetCursor(hcur);

    TraceLeaveVoid();
}

BOOL
CACEPage::OnChangeName(HWND hDlg)
{
    PUSER_LIST pUserList = NULL;
    BOOL bResult = FALSE;

    TraceEnter(TRACE_ACEEDIT, "CACEPage::OnChangeName");

    if (S_OK == GetUserGroup(hDlg, FALSE, &pUserList))
    {
        TraceAssert(NULL != pUserList);
        TraceAssert(1 == pUserList->cUsers);

        // Free up previous sid
        if (m_pSid)
            LocalFree(m_pSid);

        // Copy the new sid
        m_pSid = LocalAllocSid(pUserList->rgUsers[0].pSid);
        if (m_pSid)
        {
            SetDlgItemText(hDlg, IDC_ACEE_NAME, pUserList->rgUsers[0].pszName);
            bResult = TRUE;
        }
        LocalFree(pUserList);
    }

    TraceLeaveValue(bResult);
}

BOOL
CACEPage::OnClearAll(HWND hDlg)
{
    HWND hwndList;
    ULONG cPermissions;

    TraceEnter(TRACE_ACEEDIT, "CACEPage::OnClearAll");
    TraceAssert(!m_fReadOnly);

    hwndList = GetDlgItem(hDlg, IDC_ACEE_LIST);
    cPermissions = (ULONG)SendMessage(hwndList, CLM_GETITEMCOUNT, 0, 0);

    while (cPermissions != 0)
    {
        WORD wCol = COLUMN_ALLOW;

        cPermissions--;

        while (wCol == COLUMN_ALLOW || wCol == COLUMN_DENY)
        {
            WPARAM wItem = MAKELONG((WORD)cPermissions, wCol);

            if (!(CLST_DISABLED & SendMessage(hwndList, CLM_GETSTATE, wItem, 0)))
                SendMessage(hwndList, CLM_SETSTATE, wItem, CLST_UNCHECKED);

            wCol++;
        }
    }

    TraceLeaveValue(TRUE);
}

void
CACEPage::HandleSelChange(HWND hDlg, HWND hWnd) // inherit type change
{
    PSI_INHERIT_TYPE psiInheritType;
    BOOL fEnableInheritImmediate = FALSE;
    const GUID *pguidInheritType = &GUID_NULL;

    TraceEnter(TRACE_ACEEDIT, "CACEPage::HandleSelChange");

    psiInheritType = (PSI_INHERIT_TYPE)SendMessage(hWnd,
                                                   CB_GETITEMDATA,
                                                   SendMessage(hWnd, CB_GETCURSEL, 0, 0),
                                                   0);

    if (psiInheritType != (PSI_INHERIT_TYPE)CB_ERR && psiInheritType != NULL)
    {
        if (psiInheritType->dwFlags & (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE))
            fEnableInheritImmediate = TRUE;

        pguidInheritType = psiInheritType->pguid;
    }

    if (fEnableInheritImmediate != m_fInheritImmediateEnabled)
    {
        HWND hInheritImmediate = GetDlgItem(hDlg, IDC_ACEE_INHERITIMMEDIATE);

        if (fEnableInheritImmediate)
        {
            SendMessage(hInheritImmediate, BM_SETCHECK, m_fPreviousImmediateSetting, 0);
        }
        else
        {
            m_fPreviousImmediateSetting = (BOOL)SendMessage(hInheritImmediate,
                                                            BM_GETCHECK,
                                                            0,
                                                            0);
            SendMessage(hInheritImmediate, BM_SETCHECK, BST_UNCHECKED, 0);
        }

        EnableWindow(hInheritImmediate, fEnableInheritImmediate);
        m_fInheritImmediateEnabled = fEnableInheritImmediate;
    }

    // If the inherit type GUID has changed, reinitialize the checklist.
    if (!IsSameGUID(pguidInheritType, &m_guidInheritType))
    {
        HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
        HDPA hNewEntries = DPA_Create(4);

        if (hNewEntries)
        {
            GetAcesFromCheckList(GetDlgItem(hDlg, IDC_ACEE_LIST),
                                 m_pSid,
                                 m_siPageType == SI_PAGE_ADVPERM,
                                 TRUE,
                                 0,         // don't care about flags
                                 NULL,      // or inherit type here
                                 hNewEntries);

            // Save new inherit type and reset the checklist
            m_guidInheritType = *pguidInheritType; // BUGBUG check for NULL
            ReInitCheckList(hDlg, hNewEntries);
            DestroyDPA(hNewEntries);
        }

        SetCursor(hcur);
    }

    TraceLeaveVoid();
}

void
HandleListClick(PNM_CHECKLIST pnmc, SI_PAGE_TYPE siType, BOOL bInheritFlags)
{
    HWND            hChkList;
    UINT            iRow;
    WORD            wCol;
    PSI_ACCESS      pAccess;
    DWORD           dwState;
    BOOL            bNullGuid;
    UINT            iRowCompare;
    PSI_ACCESS      pAccessCompare;

    TraceEnter(TRACE_MISC, "HandleListClick");
    TraceAssert(pnmc != NULL);

    hChkList = pnmc->hdr.hwndFrom;
    iRow    = pnmc->iItem;
    wCol    = (WORD)pnmc->iSubItem;       // 1 = Allow, 2 = Deny
    pAccess = (PSI_ACCESS)pnmc->dwItemData;
    dwState = pnmc->dwState;

    if (pAccess == NULL)
        TraceLeaveVoid();

    bNullGuid = IsNullGUID(pAccess->pguid);

    iRowCompare = (UINT)SendMessage(hChkList, CLM_GETITEMCOUNT, 0, 0);

    while (iRowCompare != 0)
    {
        WPARAM wItem;
        DWORD  dwStateCompareOriginal;
        DWORD  dwStateCompare;
        WORD   wColCompare;
        BOOL   bSameGuid;
        BOOL   bNullGuidCompare;

        --iRowCompare;
        pAccessCompare = (PSI_ACCESS)SendMessage(hChkList, CLM_GETITEMDATA, iRowCompare, 0);

        if (!pAccessCompare)
            continue;

        bSameGuid = IsSameGUID(pAccessCompare->pguid, pAccess->pguid);
        bNullGuidCompare = IsNullGUID(pAccessCompare->pguid);

        // If the GUIDs are incompatible, then we can't do anything
        if (!(bSameGuid || bNullGuid || bNullGuidCompare))
            continue;

        //
        // Yukky, complicated mechanism to determine whether to
        // turn on or off the allow or deny check marks.
        //
        // REVIEW: This algorithm of changing check marks based on other
        // checkmarks handles a lot of cases, but it doesn't handle a
        // two good ones.
        //
        // (1) If you have a right which implies other rights and you turn,
        // it off, then maybe we should turn off all of the implied ones
        // too. For example, you turn off change (which is the combination
        // of read and write) maybe we should turn off both read and write.
        //
        // (2) If you turn on all of the component rights of one which
        // implies them all, then we should turn on that one (because
        // it implies them all).
        //
#ifdef NO_RADIOBUTTON_BEHAVIOR
        wColCompare = wCol;
#else
        for (wColCompare = COLUMN_ALLOW; wColCompare <= COLUMN_DENY; wColCompare++)
#endif
        {
            wItem = MAKELONG((WORD)iRowCompare, wColCompare);
            dwStateCompareOriginal = (DWORD)SendMessage(hChkList, CLM_GETSTATE, wItem, 0);

            //
            // If the other box is disabled, then it represents an
            // inherited right so don't do anything with it.
            //
            // nb: Depending on NO_RADIOBUTTON_BEHAVIOR, this may continue to the
            // for(wColCompare) loop or the while(iRowCompare) loop as appropriate
            //
            if (dwStateCompareOriginal & CLST_DISABLED)
                continue;

            dwStateCompare = dwStateCompareOriginal;

            if (dwState & CLST_CHECKED)
            {
                if (wCol == wColCompare)
                {
                    //
                    // If this right implies some other right,
                    // then turn it on too.
                    //
                    if ((bSameGuid || bNullGuid) && AllFlagsOn(pAccess->mask, pAccessCompare->mask))
                    {
                        if (!bInheritFlags ||
                            AllFlagsOn(pAccess->dwFlags & ACE_INHERIT_ALL, pAccessCompare->dwFlags & ACE_INHERIT_ALL))
                        {
                            dwStateCompare |= CLST_CHECKED;
                        }
                    }
                }
                else
                {
#ifndef NO_RADIOBUTTON_BEHAVIOR
                    //
                    // If this right implies or is implied by some other
                    // right in the other column, then turn it off.
                    //
                    if ( (siType == SI_PAGE_PERM || siType == SI_PAGE_ADVPERM) &&
                         (((bSameGuid || bNullGuid) && AllFlagsOn(pAccess->mask, pAccessCompare->mask)) ||
                          ((bSameGuid || bNullGuidCompare) && AllFlagsOn(pAccessCompare->mask, pAccess->mask))) )
                    {
                        if (!bInheritFlags ||
                            (AllFlagsOn(pAccessCompare->dwFlags & ACE_INHERIT_ALL, pAccess->dwFlags & ACE_INHERIT_ALL) ||
                             AllFlagsOn(pAccess->dwFlags & ACE_INHERIT_ALL, pAccessCompare->dwFlags & ACE_INHERIT_ALL)) )
                        {
                            dwStateCompare &= ~(CLST_CHECKED);
                        }
                    }
#endif
                }
            }
            else
            {
                if (wCol == wColCompare)
                {
                    //
                    // If this right is implied by some other right, then
                    // turn it off too.
                    //
                    if ((bSameGuid || bNullGuidCompare) && AllFlagsOn(pAccessCompare->mask, pAccess->mask))
                    {
                        if (!bInheritFlags ||
                            AllFlagsOn(pAccessCompare->dwFlags & ACE_INHERIT_ALL, pAccess->dwFlags & ACE_INHERIT_ALL))
                        {
                            dwStateCompare &= ~(CLST_CHECKED);
                        }
                    }
                }
            }

            if (dwStateCompareOriginal != dwStateCompare)
                SendMessage(hChkList, CLM_SETSTATE, wItem, (LPARAM)dwStateCompare);
        }
    }

    TraceLeaveVoid();
}


UINT
GetAcesFromCheckList(HWND hChkList,
                     PSID pSid,                 // All aces get this SID
                     BOOL fPerm,                // Create ACCESS or AUDIT aces?
                     BOOL fAceFlagsProvided,    // Use uAceFlagsAll instead of pAccess->dwFlags
                     UCHAR uAceFlagsAll,        // All aces get these flags
                     const GUID *pInheritGUID,  // All aces get this inherit GUID
                     HDPA hEntries)             // Store new aces here
{
    UINT cCheckRows;
    UINT iCheckRow;
    UINT cbSidSize;
    UINT iCount;
    BOOL bInheritTypePresent = FALSE;

    TraceEnter(TRACE_MISC, "GetAcesFromCheckList");
    TraceAssert(hChkList != NULL);
    TraceAssert(pSid != NULL);
    TraceAssert(hEntries != NULL);

    cbSidSize = GetLengthSid(pSid);

    if (pInheritGUID == NULL)
        pInheritGUID = &GUID_NULL;
    else if (!IsNullGUID(pInheritGUID))
        bInheritTypePresent = TRUE;

    //
    // First clear out the old HDPA
    //
    iCount = DPA_GetPtrCount(hEntries);
    while (iCount != 0)
    {
        --iCount;
        LocalFree(DPA_FastGetPtr(hEntries, iCount));
        DPA_DeletePtr(hEntries, iCount);
    }

    cCheckRows = (UINT)SendMessage(hChkList, CLM_GETITEMCOUNT, 0, 0);
    for (iCheckRow = 0; iCheckRow < cCheckRows; iCheckRow++)
    {
        PSI_ACCESS pAccess;
        DWORD dwObjectFlagsNew;
        WORD wCol;
        UCHAR uAceFlagsNew;

        pAccess = (PSI_ACCESS)SendMessage(hChkList, CLM_GETITEMDATA, iCheckRow, 0);

        uAceFlagsNew = (UCHAR)(fAceFlagsProvided ? uAceFlagsAll : pAccess->dwFlags);

        dwObjectFlagsNew = 0;
        if (!IsNullGUID(pAccess->pguid))
            dwObjectFlagsNew |= ACE_OBJECT_TYPE_PRESENT;

        if (bInheritTypePresent)
            dwObjectFlagsNew |= ACE_INHERITED_OBJECT_TYPE_PRESENT;

        wCol = COLUMN_ALLOW;
        while (wCol == COLUMN_ALLOW || wCol == COLUMN_DENY)
        {
            WPARAM        wItem;
            DWORD         dwState;

            wItem = MAKELONG((WORD)iCheckRow, wCol);
            dwState = (DWORD)SendMessage(hChkList, CLM_GETSTATE, wItem, 0);

            if ((dwState & CLST_CHECKED) && !(dwState & CLST_DISABLED))
            {
                //
                // Ok, time to make an ACE for this check mark, see if we
                // can merge it into an already existing ACE, or whether we
                // we need to create a new entry
                //
                UCHAR uAceTypeNew;
                DWORD dwMaskNew = pAccess->mask;
                UINT cbSize = SIZEOF(KNOWN_ACE);

                if (fPerm)
                {
                    if (wCol == COLUMN_ALLOW)
                        uAceTypeNew = ACCESS_ALLOWED_ACE_TYPE;
                    else
                        uAceTypeNew = ACCESS_DENIED_ACE_TYPE;
                }
                else
                {
                    uAceTypeNew = SYSTEM_AUDIT_ACE_TYPE;
                    uAceFlagsNew &= ~(SUCCESSFUL_ACCESS_ACE_FLAG | FAILED_ACCESS_ACE_FLAG);

                    if (wCol == COLUMN_ALLOW)
                        uAceFlagsNew |= SUCCESSFUL_ACCESS_ACE_FLAG;
                    else
                        uAceFlagsNew |= FAILED_ACCESS_ACE_FLAG;
                }

                if (dwObjectFlagsNew != 0)
                {
                    uAceTypeNew += (ACCESS_ALLOWED_OBJECT_ACE_TYPE - ACCESS_ALLOWED_ACE_TYPE);
                    cbSize = SIZEOF(KNOWN_OBJECT_ACE);

                    if (dwObjectFlagsNew & ACE_OBJECT_TYPE_PRESENT)
                        cbSize += SIZEOF(GUID);

                    if (dwObjectFlagsNew & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                        cbSize += SIZEOF(GUID);
                }

                cbSize += cbSidSize - SIZEOF(ULONG);

                //
                // See if it exists
                //
                iCount = DPA_GetPtrCount(hEntries);
                while(iCount != 0)
                {
                    PACE_HEADER pAce;
                    BOOL bObjectTypePresent = FALSE;
                    const GUID *pObjectType = NULL;

                    --iCount;
                    pAce = (PACE_HEADER)DPA_FastGetPtr(hEntries, iCount);

                    if (IsObjectAceType(pAce))
                        pObjectType = RtlObjectAceObjectType(pAce);

                    if (!pObjectType)
                        pObjectType = &GUID_NULL;
                    else
                        bObjectTypePresent = TRUE;

                    //
                    // Test the new ACE against each existing ACE to see if
                    // we can combine them.
                    //
                    if (IsEqualACEType(pAce->AceType, uAceTypeNew))
                    {
                        DWORD dwMergeFlags = 0;
                        DWORD dwMergeStatus;
                        DWORD dwMergeResult;

                        if (dwObjectFlagsNew & ACE_OBJECT_TYPE_PRESENT)
                            dwMergeFlags |= MF_OBJECT_TYPE_1_PRESENT;

                        if (bObjectTypePresent)
                            dwMergeFlags |= MF_OBJECT_TYPE_2_PRESENT;

                        if (!(dwMergeFlags & (MF_OBJECT_TYPE_1_PRESENT | MF_OBJECT_TYPE_2_PRESENT)))
                        {
                            // Neither are present, so they are the same
                            dwMergeFlags |= MF_OBJECT_TYPE_EQUAL;
                        }
                        else if (IsSameGUID(pAccess->pguid, pObjectType))
                            dwMergeFlags |= MF_OBJECT_TYPE_EQUAL;

                        if (!fPerm)
                            dwMergeFlags |= MF_AUDIT_ACE_TYPE;

                        dwMergeStatus = MergeAceHelper(uAceFlagsNew,
                                                       dwMaskNew,
                                                       pAce->AceFlags,
                                                       ((PKNOWN_ACE)pAce)->Mask,
                                                       dwMergeFlags,
                                                       &dwMergeResult);

                        if (dwMergeStatus == MERGE_MODIFIED_FLAGS)
                        {
                            uAceFlagsNew = (UCHAR)dwMergeResult;
                            dwMergeStatus = MERGE_OK_1;
                        }
                        else if (dwMergeStatus == MERGE_MODIFIED_MASK)
                        {
                            dwMaskNew = dwMergeResult;
                            dwMergeStatus = MERGE_OK_1;
                        }

                        if (dwMergeStatus == MERGE_OK_1)
                        {
                            //
                            // The new ACE implies the existing ACE, so
                            // the existing one can be removed.
                            //
                            LocalFree(pAce);
                            DPA_DeletePtr(hEntries, iCount);
                            //
                            // Keep looking.  Maybe we can remove some more entries
                            // before adding the new one.
                            //
                        }
                        else if (dwMergeStatus == MERGE_OK_2)
                        {
                            iCount = 1;     // non-zero for match found
                            break;
                        }
                    }
                }

                //
                // Otherwise, add it
                //
                if (iCount == 0)
                {
                    PACE_HEADER pAce = (PACE_HEADER)LocalAlloc(LPTR, cbSize);

                    if (pAce)
                    {
                        PSID pSidT;

                        pAce->AceType  = uAceTypeNew;
                        pAce->AceFlags = uAceFlagsNew;
                        pAce->AceSize  = (USHORT)cbSize;
                        ((PKNOWN_ACE)pAce)->Mask = dwMaskNew;
                        pSidT = &((PKNOWN_ACE)pAce)->SidStart;

                        if (dwObjectFlagsNew != 0)
                        {
                            GUID *pGuid;

                            ((PKNOWN_OBJECT_ACE)pAce)->Flags = dwObjectFlagsNew;

                            pGuid = RtlObjectAceObjectType(pAce);
                            if (pGuid)
                            {
                                if (pAccess->pguid)
                                    *pGuid = *pAccess->pguid;
                                else
                                    *pGuid = GUID_NULL;
                            }

                            pGuid = RtlObjectAceInheritedObjectType(pAce);
                            if (pGuid)
                                *pGuid = *pInheritGUID;

                            pSidT = RtlObjectAceSid(pAce);
                        }

                        CopyMemory(pSidT, pSid, cbSidSize);
                        DPA_AppendPtr(hEntries, pAce);
                    }
                }
            }

            wCol++;
        }
    }

    iCount = DPA_GetPtrCount(hEntries);
    TraceLeaveValue(iCount);
}

LONG
CACEPage::OnApply(HWND hDlg, BOOL /*bClose*/)
{
    const GUID *pInheritGUID;
    UCHAR uAceFlagsNew = 0;
    HDPA hEntries;

    if (!m_fPageDirty)
        return PSNRET_NOERROR;

    TraceEnter(TRACE_ACEEDIT, "CACEPage::Apply");

    //
    // Determine inheritance for containers
    //
    pInheritGUID = &GUID_NULL;
    if ((m_siObjectInfo.dwFlags & SI_CONTAINER) != 0)
    {
        PSI_INHERIT_TYPE psiInheritType = NULL;
        HWND hInheritType = GetDlgItem( hDlg, IDC_ACEE_INHERITTYPE);

        int iSel = (int)SendMessage(hInheritType, CB_GETCURSEL, 0,0);

        if (iSel != CB_ERR)
        {
            psiInheritType = (PSI_INHERIT_TYPE)SendMessage(hInheritType,
                                                           CB_GETITEMDATA,
                                                           iSel,
                                                           0);
        }

        if (psiInheritType != (PSI_INHERIT_TYPE)CB_ERR && psiInheritType != NULL)
        {
            pInheritGUID = psiInheritType->pguid;
            uAceFlagsNew = (UCHAR)(psiInheritType->dwFlags & VALID_INHERIT_FLAGS);
        }
        else if (m_pAce)
        {
            uAceFlagsNew = m_pAce->AceFlags;
        }

        if (m_fInheritImmediateEnabled)
        {
            if (IsDlgButtonChecked(hDlg, IDC_ACEE_INHERITIMMEDIATE) == BST_CHECKED)
                uAceFlagsNew |= NO_PROPAGATE_INHERIT_ACE;
            else
                uAceFlagsNew &= ~NO_PROPAGATE_INHERIT_ACE;
        }
    }

    if (m_phEntries != NULL)
    {
        if (*m_phEntries == NULL)
            *m_phEntries = DPA_Create(4);

        GetAcesFromCheckList(GetDlgItem(hDlg, IDC_ACEE_LIST),
                             m_pSid,
                             m_siPageType == SI_PAGE_ADVPERM,
                             TRUE,
                             uAceFlagsNew,
                             pInheritGUID,
                             *m_phEntries);
    }

    if (m_pdwResult)
        *m_pdwResult |= (m_siFlags == SI_ACCESS_PROPERTY ? EAE_NEW_PROPERTY_ACE : EAE_NEW_OBJECT_ACE);
    m_fPageDirty = FALSE;

    TraceLeaveValue(PSNRET_NOERROR);
}

BOOL
CACEPage::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        InitDlg(hDlg);
        break;

    case WM_DESTROY:
        EmptyCheckList(GetDlgItem(hDlg, IDC_ACEE_LIST));
        break;

    case PWM_SELECT_PAGE:
        PropSheet_SetCurSel(GetParent(hDlg), lParam, wParam);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_ACEE_NAMEBUTTON:
            if (OnChangeName(hDlg))
            {
                PropSheet_Changed(GetParent(hDlg), hDlg);
                m_fPageDirty = TRUE;
            }
            break;

        case IDC_ACEE_INHERITTYPE:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
            {
                HandleSelChange(hDlg, (HWND)lParam);
                PropSheet_Changed(GetParent(hDlg), hDlg);
                m_fPageDirty = TRUE;
            }
            break;

        case IDC_ACEE_INHERITIMMEDIATE:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
            {
                PropSheet_Changed(GetParent(hDlg), hDlg);
                m_fPageDirty = TRUE;
            }
            break;

        case IDC_ACEE_CLEAR:
            if (OnClearAll(hDlg))
            {
                PropSheet_Changed(GetParent(hDlg), hDlg);
                m_fPageDirty = TRUE;
            }
            break;

        default:
            return FALSE;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
        case CLN_CLICK:
            if (lParam)
            {
                HandleListClick((PNM_CHECKLIST)lParam, m_siPageType, FALSE);
                PropSheet_Changed(GetParent(hDlg), hDlg);
                m_fPageDirty = TRUE;
            }
            break;

        case CLN_GETCOLUMNDESC:
            {
                PNM_CHECKLIST pnmc = (PNM_CHECKLIST)lParam;
                GetDlgItemText(hDlg,
                               IDC_ACEE_ALLOW - 1 + pnmc->iSubItem,
                               pnmc->pszText,
                               pnmc->cchTextMax);
            }
            break;

        case PSN_APPLY:
            SetWindowLongPtr(hDlg,
                             DWLP_MSGRESULT,
                             OnApply(hDlg, (BOOL)(((LPPSHNOTIFY)lParam)->lParam)));
            break;
        }
        break;

    case WM_HELP:
        if (IsWindowEnabled(hDlg))
        {
            const DWORD *pdwHelpIDs = aAcePermHelpIDs;

            if (m_siPageType == SI_PAGE_AUDIT)
                pdwHelpIDs = aAceAuditHelpIDs;

            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    c_szAcluiHelpFile,
                    HELP_WM_HELP,
                    (DWORD_PTR)pdwHelpIDs);
        }
        break;

    case WM_CONTEXTMENU:
        if (IsWindowEnabled(hDlg))
        {
            HWND hwnd = (HWND)wParam;
            const DWORD *pdwHelpIDs = aAcePermHelpIDs;

            if (m_siPageType == SI_PAGE_AUDIT)
                pdwHelpIDs = aAceAuditHelpIDs;

            //
            // Some of the checkboxes may be scrolled out of view, but
            // they are still detected by WinHelp, so we jump through
            // a few extra hoops here.
            //
            if (hwnd == hDlg)
            {
                POINT pt;
                pt.x = LOWORD(lParam);
                pt.y = HIWORD(lParam);

                ScreenToClient(hDlg, &pt);
                hwnd = ChildWindowFromPoint(hDlg, pt);
                if (hDlg == hwnd)
                    break;
            }

            //
            // WinHelp looks for child windows, but we don't have help id's
            // for the permission checkboxes.  If the request is for the
            // checklist window, fake out WinHelp by referring to one of
            // the static labels just above the list.
            //
            if (GetDlgCtrlID(hwnd) == IDC_ACEE_LIST)
                hwnd = GetWindow((HWND)wParam, GW_HWNDPREV);    // Static label "Deny"

            WinHelp(hwnd,
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
CreateACEPage(LPSECURITYINFO psi,
              SI_PAGE_TYPE siType,
              PACE pAce,
              BOOL bReadOnly,
              DWORD dwFlags,
              DWORD *pdwResult,
              HDPA *phEntries)
{
    HPROPSHEETPAGE hPage = NULL;
    LPCTSTR pszTitle = NULL;
    LPACEPAGE pPage;

    TraceEnter(TRACE_ACEEDIT, "CreateACEPage");
    TraceAssert(psi != NULL);
    TraceAssert(phEntries != NULL);

    pPage = new CACEPage(psi,
                         siType,
                         pAce,
                         bReadOnly,
                         dwFlags,
                         pdwResult,
                         phEntries);
    if (pPage)
    {
        int iDlgTemplate = IDD_ACEENTRY_PERM_PAGE;

        if (siType == SI_PAGE_AUDIT)
            iDlgTemplate = IDD_ACEENTRY_AUDIT_PAGE;

        if (dwFlags == SI_ACCESS_PROPERTY)
            pszTitle = MAKEINTRESOURCE(IDS_ACEE_PROPERTY_TITLE);

        hPage = pPage->CreatePropSheetPage(MAKEINTRESOURCE(iDlgTemplate), pszTitle);
    }

    TraceLeaveValue(hPage);
}

BOOL
EditACEEntry(HWND hwndOwner,
             LPSECURITYINFO psi,
             PACE pAce,
             SI_PAGE_TYPE siType,
             LPCTSTR pszObjectName,
             BOOL bReadOnly,
             DWORD *pdwResult,
             HDPA *phEntries,
             HDPA *phPropertyEntries,
             UINT nStartPage)
{
    HPROPSHEETPAGE hPage[2];
    UINT cPages = 0;
    BOOL bResult = FALSE;

    TraceEnter(TRACE_ACEEDIT, "EditACEEntry");
    TraceAssert(psi != NULL);

    if (phEntries)
    {
        hPage[cPages] = CreateACEPage(psi,
                                      siType,
                                      pAce,
                                      bReadOnly,
                                      SI_ACCESS_SPECIFIC,
                                      pdwResult,
                                      phEntries);
        if (hPage[cPages])
            cPages++;
    }

    if (phPropertyEntries)
    {
        hPage[cPages] = CreateACEPage(psi,
                                      siType,
                                      pAce,
                                      bReadOnly,
                                      SI_ACCESS_PROPERTY,
                                      pdwResult,
                                      phPropertyEntries);
        if (hPage[cPages])
            cPages++;
    }

    if (cPages)
    {
        // Build dialog title string
        LPTSTR pszCaption = NULL;
        FormatStringID(&pszCaption,
                       ::hModule,
                       siType == SI_PAGE_AUDIT ? IDS_ACEE_AUDIT_TITLE : IDS_ACEE_PERM_TITLE,
                       pszObjectName);

        PROPSHEETHEADER psh;
        psh.dwSize = SIZEOF(psh);
        psh.dwFlags = PSH_DEFAULT | PSH_NOAPPLYNOW;
        psh.hwndParent = hwndOwner;
        psh.hInstance = ::hModule;
        psh.pszCaption = pszCaption;
        psh.nPages = cPages;
        psh.nStartPage = 0;
        psh.phpage = &hPage[0];

        if (nStartPage < cPages)
            psh.nStartPage = nStartPage;

        bResult = (PropertySheet(&psh) == IDOK);

        LocalFreeString(&pszCaption);
    }

    TraceLeaveValue(bResult);
}
