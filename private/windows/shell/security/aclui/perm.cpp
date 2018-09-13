//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       perm.cpp
//
//  This file contains the implementation for the simple permission
//  editor page.
//
//--------------------------------------------------------------------------

#include "aclpriv.h"
#include "permset.h"
#include "sddl.h"       // ConvertSidToStringSid

#define IDN_CHECKSELECTION  1

void SelectListViewItem(HWND hwndList, int iItem);


//
//  Context Help IDs.
//
const static DWORD aPermPageHelpIDs[] =
{
    IDC_SPP_PRINCIPALS, IDH_SPP_PRINCIPALS,
    IDC_SPP_ADD,        IDH_SPP_ADD,
    IDC_SPP_REMOVE,     IDH_SPP_REMOVE,
    IDC_SPP_RESET,      IDH_SPP_RESET,
    IDC_SPP_ACCESS,     IDH_SPP_PERMS,
    IDC_SPP_ALLOW,      IDH_SPP_PERMS,
    IDC_SPP_DENY,       IDH_SPP_PERMS,
    IDC_SPP_PERMS,      IDH_SPP_PERMS,
    IDC_SPP_ADVANCED,   IDH_SPP_ADVANCED,
    IDC_SPP_PROTECT,    IDH_SPP_PROTECT,
    IDC_SPP_MORE_MSG,   IDH_NOHELP,
    0, 0,
};


class CPrincipal;
typedef class CPrincipal *LPPRINCIPAL;
class CSecurityInfo;


class CPermPage : public CSecurityPage
{
private:
    SECURITY_DESCRIPTOR_CONTROL m_wSDControl;
    WORD            m_wDaclRevision;
    PSI_ACCESS      m_pDefaultAccess;
    BOOL            m_fPageDirty;
    BOOL            m_fBusy;
    BOOL            m_bWasDenyAcl;
    HCURSOR         m_hcurBusy;

public:
    CPermPage(LPSECURITYINFO psi)
        : CSecurityPage(psi, SI_PAGE_PERM), m_wDaclRevision(ACL_REVISION)
        { m_hcurBusy = LoadCursor(NULL, IDC_APPSTARTING); }

private:
    virtual BOOL DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL InitDlg(HWND hDlg);
    void InitPrincipalList(HWND hDlg, PACL pDacl);
    HRESULT InitCheckList(HWND hDlg);
    void EnumerateAcl(HWND hwndList, PACL pAcl);
    HRESULT SetPrincipalNamesInList(HWND hwndList, PSID pSid = NULL);

    int AddPrincipalToList(HWND hwndList, LPPRINCIPAL pPrincipal);
    BOOL OnNotify(HWND hDlg, int idCtrl, LPNMHDR pnmh);
    void OnSelChange(HWND hDlg, BOOL bClearFirst = TRUE);
    void OnApply(HWND hDlg, BOOL bClose);
    HRESULT BuildDacl(HWND hDlg,
                      PSECURITY_DESCRIPTOR *ppSD,
                      BOOL fIncludeInherited);
    HRESULT SetDacl(HWND hDlg,
                    PSECURITY_DESCRIPTOR psd,
                    BOOL bDirty = FALSE);
    void OnAddPrincipal(HWND hDlg);
    void OnRemovePrincipal(HWND hDlg);
    void OnAdvanced(HWND hDlg);
    void OnReset(HWND hDlg);
    void OnProtect(HWND hDlg);
    void EnablePrincipalControls(HWND hDlg, BOOL fEnable);
    void CommitCurrent(HWND hDlg, int iPrincipal = -1);
    void OnSize(HWND hDlg, DWORD dwSizeType, ULONG nWidth, ULONG nHeight);
    void ClearPermissions(HWND hwndList, BOOL bDisabled = TRUE);
    void SetDirty(HWND hDlg, BOOL bDefault = FALSE);

    friend class CPrincipal;
    friend class CSecurityInfo;
};
typedef class CPermPage *PPERMPAGE;

class CPrincipal
{
private:
    PPERMPAGE       m_pPage;
    LPTSTR          m_pszName;
    PSID            m_pSID;
    SID_IMAGE_INDEX m_nImageIndex;
    BOOL            m_bHaveRealName;

public:
    CPermissionSet  m_permDeny;
    CPermissionSet  m_permAllow;
    CPermissionSet  m_permInheritedDeny;
    CPermissionSet  m_permInheritedAllow;

public:
    CPrincipal(CPermPage *pPage) : m_pPage(pPage), m_nImageIndex(SID_IMAGE_UNKNOWN) {}
    ~CPrincipal();

    BOOL    SetPrincipal(PSID pSID,
                         SID_NAME_USE sidType = SidTypeUnknown,
                         LPCTSTR pszName = NULL,
                         LPCTSTR pszLogonName = NULL);
    BOOL    SetName(LPCTSTR pszName, LPCTSTR pszLogonName = NULL);
    void    SetSidType(SID_NAME_USE sidType) { m_nImageIndex = GetSidImageIndex(m_pSID, sidType); }
    PSID    GetSID()  const { return m_pSID; }
    LPCTSTR GetName() const { return m_pszName; }
    int     GetImageIndex() const { return m_nImageIndex; }

    BOOL    HaveRealName() { return m_bHaveRealName; }

    BOOL    AddAce(PACE_HEADER pAce);
    ULONG   GetAclLength(DWORD dwFlags);
    BOOL    AppendToAcl(PACL pAcl, DWORD dwFlags, PACE_HEADER *ppAcePos);

    BOOL    HaveInheritedAces(void);
    void    ConvertInheritedAces(BOOL bDelete);

    void    AddPermission(BOOL bAllow, PPERMISSION pperm);
    void    RemovePermission(BOOL bAllow, PPERMISSION pperm);

private:
    CPermissionSet* GetPermSet(DWORD dwType, BOOL bInherited);
    BOOL AddNormalAce(DWORD dwType, DWORD dwFlags, ACCESS_MASK mask, const GUID *pObjectType);
    BOOL AddAdvancedAce(DWORD dwType, PACE_HEADER pAce);
};

// flag bits for GetAclLength & AppendToAcl
#define ACL_NONINHERITED    0x00010000L
#define ACL_INHERITED       0x00020000L
#define ACL_DENY            0x00040000L
#define ACL_ALLOW           0x00080000L
#define ACL_CHECK_CREATOR   0x00100000L
#define ACL_NONOBJECT       PS_NONOBJECT
#define ACL_OBJECT          PS_OBJECT


//
// Wrapper for ISecurityInformation.  Used when invoking
// the advanced ACL editor
//
class CSecurityInfo : public ISecurityInformation, ISecurityInformation2
#if(_WIN32_WINNT >= 0x0500)
    , IDsObjectPicker
#endif
{
private:
    ULONG       m_cRef;
    PPERMPAGE   m_pPage;
    HWND        m_hDlg;

public:
    CSecurityInfo(PPERMPAGE pPage, HWND hDlg)
        : m_cRef(1), m_pPage(pPage), m_hDlg(hDlg) {}

    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ISecurityInformation methods
    STDMETHODIMP GetObjectInformation(PSI_OBJECT_INFO pObjectInfo);
    STDMETHODIMP GetSecurity(SECURITY_INFORMATION si,
                             PSECURITY_DESCRIPTOR *ppSD,
                             BOOL fDefault);
    STDMETHODIMP SetSecurity(SECURITY_INFORMATION si,
                             PSECURITY_DESCRIPTOR pSD);
    STDMETHODIMP GetAccessRights(const GUID* pguidObjectType,
                                 DWORD dwFlags,
                                 PSI_ACCESS *ppAccess,
                                 ULONG *pcAccesses,
                                 ULONG *piDefaultAccess);
    STDMETHODIMP MapGeneric(const GUID* pguidObjectType,
                            UCHAR *pAceFlags,
                            ACCESS_MASK *pmask);
    STDMETHODIMP GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
                                 ULONG *pcInheritTypes);
    STDMETHODIMP PropertySheetPageCallback(HWND hwnd, UINT uMsg, SI_PAGE_TYPE uPage);

    // ISecurityInformation2 methods
    STDMETHODIMP_(BOOL) IsDaclCanonical(PACL pDacl);
    STDMETHODIMP        LookupSids(ULONG cSids, PSID *rgpSids, LPDATAOBJECT *ppdo);

    // IDsObjectPicker methods
#if(_WIN32_WINNT >= 0x0500)
    STDMETHODIMP Initialize(PDSOP_INIT_INFO pInitInfo);
    STDMETHODIMP InvokeDialog(HWND hwndParent, IDataObject **ppdoSelection);
#endif
};


//
// CPrincipal implementation
//

CPrincipal::~CPrincipal()
{
    if (NULL != m_pSID)
        LocalFree(m_pSID);

    LocalFreeString(&m_pszName);
}


BOOL
CPrincipal::SetPrincipal(PSID pSID,
                         SID_NAME_USE sidType,
                         LPCTSTR pszName,
                         LPCTSTR pszLogonName)
{
    DWORD dwLength;

    TraceEnter(TRACE_PRINCIPAL, "CPrincipal::SetPrincipal");
    TraceAssert(pSID != NULL);
    TraceAssert(IsValidSid(pSID));

    if (NULL != m_pSID)
        LocalFree(m_pSID);

    m_pSID = LocalAllocSid(pSID);

    SetSidType(sidType);
    SetName(pszName, pszLogonName);

    TraceLeaveValue(NULL != m_pSID);
}


BOOL
CPrincipal::SetName(LPCTSTR pszName, LPCTSTR pszLogonName)
{
    LocalFreeString(&m_pszName);
    m_bHaveRealName = FALSE;

    if (BuildUserDisplayName(&m_pszName, pszName, pszLogonName))
        m_bHaveRealName = TRUE;
    else
        ConvertSidToStringSid(m_pSID, &m_pszName);

    return (NULL != m_pszName);
}


CPermissionSet*
CPrincipal::GetPermSet(DWORD dwType, BOOL bInherited)
{
    CPermissionSet *pPermSet = NULL;

    TraceEnter(TRACE_PRINCIPAL, "CPrincipal::GetPermSet");

    switch (dwType)
    {
    case ACCESS_DENIED_ACE_TYPE:
        if (bInherited)
            pPermSet = &m_permInheritedDeny;
        else
            pPermSet = &m_permDeny;
        break;

    case ACCESS_ALLOWED_ACE_TYPE:
        if (bInherited)
            pPermSet = &m_permInheritedAllow;
        else
            pPermSet = &m_permAllow;
        break;

    case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        // We don't handle compound ACEs
        TraceMsg("Ignoring ACCESS_ALLOWED_COMPOUND_ACE");
        break;

#ifdef DEBUG
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
    default:
        // We only process the various ACCESS_ALLOWED_* and ACCESS_DENIED_*
        // ACE types, except for ACCESS_ALLOWED_COMPOUND_ACE_TYPE, and these
        // are all accounted for above.  Something is very wrong if we get
        // an audit/alarm ACE or some unknown/future ACE type.
        TraceAssert(FALSE);
        break;
#endif
    }

    TraceLeaveValue(pPermSet);
}

BOOL
CPrincipal::AddNormalAce(DWORD dwType, DWORD dwFlags, ACCESS_MASK mask, const GUID *pObjectType)
{
    BOOL fResult = FALSE;

    TraceEnter(TRACE_PRINCIPAL, "CPrincipal::AddNormalAce");

    CPermissionSet *pPermSet = GetPermSet(dwType, (BOOL)(dwFlags & INHERITED_ACE));
    if (pPermSet)
        fResult = pPermSet->AddAce(pObjectType, mask, dwFlags);

    TraceLeaveValue(fResult);
}


BOOL
CPrincipal::AddAdvancedAce(DWORD dwType, PACE_HEADER pAce)
{
    BOOL fResult = FALSE;

    TraceEnter(TRACE_PRINCIPAL, "CPrincipal::AddAdvancedAce");

    CPermissionSet *pPermSet = GetPermSet(dwType, AceInherited(pAce));
    if (pPermSet)
        fResult = pPermSet->AddAdvancedAce(pAce);

    TraceLeaveValue(fResult);
}


BOOL
CPrincipal::AddAce(PACE_HEADER pAce)
{
    TraceEnter(TRACE_PRINCIPAL, "CPrincipal::AddAce");
    TraceAssert(pAce != NULL);

    BOOL fResult = FALSE;
    const GUID *pObjectType = NULL;
    UCHAR AceType = pAce->AceType;
    UCHAR AceFlags = pAce->AceFlags;
    ACCESS_MASK AccessMask = ((PKNOWN_ACE)pAce)->Mask;
    ULONG ulObjectFlags = 0;

    // Get the object type GUID from object ACEs
    if (IsObjectAceType(pAce))
    {
        AceType -= (ACCESS_ALLOWED_OBJECT_ACE_TYPE - ACCESS_ALLOWED_ACE_TYPE);
        ulObjectFlags = ((PKNOWN_OBJECT_ACE)pAce)->Flags;

        if (m_pPage->m_wDaclRevision < ACL_REVISION_DS)
            m_pPage->m_wDaclRevision = ACL_REVISION_DS;

        pObjectType = RtlObjectAceObjectType(pAce);
    }

    if (!pObjectType)
        pObjectType = &GUID_NULL;

    // Map any generic bits to standard & specific bits.
    m_pPage->m_psi->MapGeneric(pObjectType, &AceFlags, &AccessMask);

    // Can't have INHERIT_ONLY_ACE without either CONTAINER_INHERIT_ACE or
    // OBJECT_INHERIT_ACE, so if we find one of these, skip it.
    if ((AceFlags & (INHERIT_ONLY_ACE | ACE_INHERIT_ALL)) != INHERIT_ONLY_ACE)
    {
        if (ulObjectFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
        {
            // If we have an inherit object type without INHERIT_ONLY_ACE,
            // and the inherit object type matches the current object,
            // then it applies to this object.  Simulate this (per the
            // ACL inheritance spec) with 2 ACEs: one with no inheritance
            // at all, and one with the inherit type + INHERIT_ONLY_ACE.

            // Does it apply to this object?
            if ((m_pPage->m_siObjectInfo.dwFlags & SI_OBJECT_GUID) &&
                !(AceFlags & INHERIT_ONLY_ACE) &&
                IsSameGUID(&m_pPage->m_siObjectInfo.guidObjectType, RtlObjectAceInheritedObjectType(pAce)))
            {
                // Mask out all flags except INHERITED_ACE and add it
                AddNormalAce(AceType, (AceFlags & INHERITED_ACE), AccessMask, pObjectType);

                // Turn on INHERIT_ONLY_ACE before adding the "advanced" ACE.
                pAce->AceFlags |= INHERIT_ONLY_ACE;
            }

            // The ACE does not apply directly to this object
            fResult = AddAdvancedAce(AceType, pAce);
        }
        else
        {
            fResult = AddNormalAce(AceType, AceFlags, AccessMask, pObjectType);
        }
    }

    TraceLeaveValue(fResult);
}


ULONG
CPrincipal::GetAclLength(DWORD dwFlags)
{
    // Return an estimate of the buffer size needed to hold the
    // requested ACEs. The size of the ACL header is NOT INCLUDED.

    // The following flags are always assumed:
    // ACL_DENY | ACL_ALLOW | ACL_NONOBJECT | ACL_OBJECT

    ULONG nAclLength = 0;
    ULONG nSidLength;

    TraceEnter(TRACE_PRINCIPAL, "CPrincipal::GetAclLength");
    TraceAssert(NULL != m_pSID);

    if (NULL == m_pSID)
        TraceLeaveValue(0);

    nSidLength = GetLengthSid(m_pSID);

    if (dwFlags & ACL_NONINHERITED)
    {
        nAclLength += m_permDeny.GetAclLength(nSidLength);
        nAclLength += m_permAllow.GetAclLength(nSidLength);
    }

    if (dwFlags & ACL_INHERITED)
    {
        nAclLength += m_permInheritedDeny.GetAclLength(nSidLength);
        nAclLength += m_permInheritedAllow.GetAclLength(nSidLength);
    }

    TraceLeaveValue(nAclLength);
}

BOOL
CPrincipal::AppendToAcl(PACL pAcl,
                        DWORD dwFlags,
                        PACE_HEADER *ppAcePos)  // position to copy first ACE
{
    PACE_HEADER pAceT;

    TraceEnter(TRACE_PRINCIPAL, "CPrincipal::AppendToAcl");
    TraceAssert(pAcl != NULL && IsValidAcl(pAcl));
    TraceAssert(ppAcePos != NULL);
    TraceAssert(NULL != m_pSID);

    if (NULL == m_pSID)
        TraceLeaveValue(FALSE);

    pAceT = *ppAcePos;

    // Build the ACL in the following order:
    //      Deny
    //      Allow
    //      Inherited Deny
    //      Inherited Allow

    if (dwFlags & ACL_NONINHERITED)
    {
        if (dwFlags & ACL_DENY)
            m_permDeny.AppendToAcl(pAcl, ppAcePos, m_pSID, FALSE, dwFlags);

        if (dwFlags & ACL_ALLOW)
            m_permAllow.AppendToAcl(pAcl, ppAcePos, m_pSID, TRUE, dwFlags);
    }

    if (dwFlags & ACL_INHERITED)
    {
        if (dwFlags & ACL_DENY)
            m_permInheritedDeny.AppendToAcl(pAcl, ppAcePos, m_pSID, FALSE, dwFlags);

        if (dwFlags & ACL_ALLOW)
            m_permInheritedAllow.AppendToAcl(pAcl, ppAcePos, m_pSID, TRUE, dwFlags);
    }

    if ((dwFlags & ACL_CHECK_CREATOR) && IsCreatorSid(m_pSID))
    {
        //
        // Special case for CreatorOwner/CreatorGroup,
        // which are only useful if inherit bits are set.
        //
        for (; pAceT < *ppAcePos; pAceT = (PACE_HEADER)NextAce(pAceT))
        {
            pAceT->AceFlags |= INHERIT_ONLY_ACE;
            if (!(pAceT->AceFlags & ACE_INHERIT_ALL))
            {
                pAceT->AceFlags |= ACE_INHERIT_ALL;

                //
                // Give the client a chance to adjust the flags.
                // E.g. DS always turns off OBJECT_INHERIT_ACE
                //
                UCHAR AceFlags = pAceT->AceFlags;
                ACCESS_MASK Mask = GENERIC_ALL;
                m_pPage->m_psi->MapGeneric(NULL, &AceFlags, &Mask);
                pAceT->AceFlags = AceFlags;
            }
        }
    }

    TraceAssert(IsValidAcl(pAcl));
    TraceLeaveValue(TRUE);
}


BOOL
CPrincipal::HaveInheritedAces(void)
{
    return (m_permInheritedAllow.GetPermCount(TRUE) || m_permInheritedDeny.GetPermCount(TRUE));
}


void
CPrincipal::ConvertInheritedAces(BOOL bDelete)
{
    if (bDelete)
    {
        m_permInheritedDeny.Reset();
        m_permInheritedAllow.Reset();
    }
    else
    {
        m_permDeny.ConvertInheritedAces(m_permInheritedDeny);
        m_permAllow.ConvertInheritedAces(m_permInheritedAllow);
    }
}


void
CPrincipal::AddPermission(BOOL bAllow, PPERMISSION pperm)
{
    if (bAllow)
        m_permAllow.AddPermission(pperm);
    else
        m_permDeny.AddPermission(pperm);
}


void
CPrincipal::RemovePermission(BOOL bAllow, PPERMISSION pperm)
{
    if (bAllow)
        m_permAllow.RemovePermission(pperm);
    else
        m_permDeny.RemovePermission(pperm);
}


//
// CPermPage implementation
//

void
CPermPage::InitPrincipalList(HWND hDlg, PACL pDacl)
{
    TraceEnter(TRACE_PERMPAGE, "CPermPage::InitPrincipalList");
    TraceAssert(hDlg != NULL);

    HWND hwndList = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);
    TraceAssert(hwndList != NULL);

    // Save the DACL revision
    if (pDacl != NULL)
    {
        m_wDaclRevision = pDacl->AclRevision;
    }

    // If we have a selection, remember the SID for later
    PSID psidTemp = NULL;
    LPPRINCIPAL pPrincipal = (LPPRINCIPAL)GetSelectedItemData(hwndList, NULL);
    if (pPrincipal != NULL)
        psidTemp = LocalAllocSid(pPrincipal->GetSID());

    // Empty out the list
    ListView_DeleteAllItems(hwndList);

    // Enumerate the new DACL and fill the list
    EnumerateAcl(hwndList, pDacl);

    // Try to re-select the previously selection
    if (psidTemp != NULL)
    {
        int cItems = ListView_GetItemCount(hwndList);

        LV_ITEM lvItem;
        lvItem.iSubItem = 0;
        lvItem.mask = LVIF_PARAM;

        // Look for the previously selected principal in the list
        while (cItems > 0)
        {
            --cItems;
            lvItem.iItem = cItems;

            ListView_GetItem(hwndList, &lvItem);
            pPrincipal = (LPPRINCIPAL)lvItem.lParam;

            if (EqualSid(psidTemp, pPrincipal->GetSID()))
            {
                SelectListViewItem(hwndList, cItems);
                break;
            }
        }

        LocalFree(psidTemp);
    }

    TraceLeaveVoid();
}


STDMETHODIMP
_InitCheckList(HWND hwndList,
               LPSECURITYINFO psi,
               const GUID* pguidObjectType,
               DWORD dwFlags,
               HINSTANCE hInstance,
               DWORD dwType,
               PSI_ACCESS *ppDefaultAccess)
{
    HRESULT hr;
    PSI_ACCESS pAccess;
    ULONG cAccesses;
    ULONG iDefaultAccess;
    TCHAR szName[MAX_PATH];

    TraceEnter(TRACE_MISC, "_InitCheckList");
    TraceAssert(psi != NULL);

    //
    // Retrieve the permission list
    //
    hr = psi->GetAccessRights(pguidObjectType,
                              dwFlags,
                              &pAccess,
                              &cAccesses,
                              &iDefaultAccess);
    if (SUCCEEDED(hr) && cAccesses > 0)
    {
        if (ppDefaultAccess != NULL)
            *ppDefaultAccess = &pAccess[iDefaultAccess];

        // Enumerate the permissions and add to the checklist
        for (ULONG i = 0; i < cAccesses; i++, pAccess++)
        {
            LPCTSTR pszName;

            // Only add permissions that have any of the flags specified in dwType
            if (!(pAccess->dwFlags & dwType))
                continue;

            pszName = pAccess->pszName;
            if (IS_INTRESOURCE(pszName))
            {
                TraceAssert(hInstance != NULL);

                if (LoadString(hInstance,
                               (UINT)((ULONG_PTR)pszName),
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

            if (SendMessage(hwndList,
                            CLM_ADDITEM,
                            (WPARAM)pszName,
                            (LPARAM)pAccess) == -1)
            {
                DWORD dwErr = GetLastError();
                ExitGracefully(hr, HRESULT_FROM_WIN32(dwErr), "Failed to add item to checklist");
            }
        }
    }

exit_gracefully:

    TraceLeaveResult(hr);
}


HRESULT
CPermPage::InitCheckList(HWND hDlg)
{
    HRESULT hr;

    TraceEnter(TRACE_PERMPAGE, "CPermPage::InitCheckList");
    TraceAssert(hDlg != NULL);

    HWND hwndList = GetDlgItem(hDlg, IDC_SPP_PERMS);    // checklist window
    TraceAssert(hwndList != NULL);

    DWORD dwType = SI_ACCESS_GENERAL;
    if (m_siObjectInfo.dwFlags & SI_CONTAINER)
        dwType |= SI_ACCESS_CONTAINER;

    // Enumerate the permissions and add to the checklist
    hr = _InitCheckList(hwndList,
                        m_psi,
                        NULL,
                        0,
                        m_siObjectInfo.hInstance,
                        dwType,
                        &m_pDefaultAccess);

    TraceLeaveResult(hr);
}


//
// CAUTION  - This function modifies the ACEs in the ACL by setting
//            the AceType to 0xff (an invalid ACE type).
//
void
CPermPage::EnumerateAcl(HWND hwndList, PACL pAcl)
{
    LPPRINCIPAL pPrincipal;
    PACE_HEADER pAce;
    int         iEntry;
    int         iTemp;
    PACE_HEADER paceTemp;
    HDPA        hSids = NULL;

    if (pAcl == NULL)
        return;

    TraceEnter(TRACE_PERMPAGE, "CPermPage::EnumerateAcl");

    TraceAssert(IsValidAcl(pAcl));
    TraceAssert(hwndList != NULL);

    hSids = DPA_Create(4);

    if (NULL == hSids)
        TraceLeaveVoid();

    for (iEntry = 0, pAce = (PACE_HEADER)FirstAce(pAcl);
         iEntry < pAcl->AceCount;
         iEntry++, pAce = (PACE_HEADER)NextAce(pAce))
    {
        // Skip ACEs that we've already seen
        if (pAce->AceType == 0xff)
            continue;

        // Found an ACE we haven't seen yet, must be a new principal
        pPrincipal = new CPrincipal(this);
        if (pPrincipal == NULL)
            continue;  // memory error (try to continue)

        // Initialize new principal
        if (!pPrincipal->SetPrincipal(GetAceSid(pAce)))
        {
            delete pPrincipal;
            continue;  // probably memory error (try to continue)
        }

        // Remember the SIDs so that later we can look up all the names
        // at once and then add them to the listview.
        DPA_AppendPtr(hSids, pPrincipal->GetSID());

         // The current ACE belongs to this principal, so add it
        pPrincipal->AddAce(pAce);

        // Mark the ACE so we don't look at it again
        pAce->AceType = 0xff;

        // Loop through the rest of the ACEs in the ACL looking
        // for the same SID
        paceTemp = pAce;
        for (iTemp = iEntry + 1; iTemp < pAcl->AceCount; iTemp++)
        {
            // Move pointer to the current ACE
            paceTemp = (PACE_HEADER)NextAce(paceTemp);

            // If this ACE belongs to the current principal, add it
            if (paceTemp->AceType != 0xff &&
                EqualSid(GetAceSid(paceTemp), pPrincipal->GetSID()))
            {
                // Same principal, add the ACE
                pPrincipal->AddAce(paceTemp);

                // Mark the ACE so we don't look at it again
                paceTemp->AceType = 0xff;
            }
        }

        if (-1 == AddPrincipalToList(hwndList, pPrincipal))
        {
            delete pPrincipal;
        }
    }

    // Launch thread to look up sids
    m_fBusy = TRUE;
    LookupSidsAsync(hSids,
                    m_siObjectInfo.pszServerName,
                    m_psi2,
                    GetParent(hwndList),
                    UM_SIDLOOKUPCOMPLETE);
    DPA_Destroy(hSids);

    TraceLeaveVoid();
}


HRESULT
CPermPage::SetPrincipalNamesInList(HWND hwndList, PSID pSid)
{
    TraceEnter(TRACE_PERMPAGE, "CPermPage::SetPrincipalNamesInList");

    HRESULT hr = S_OK;
    PUSER_LIST  pUserList = NULL;
    LPPRINCIPAL pPrincipal = NULL;
    LVITEM lvItem = {0};
    int cListItems;
    int iListItem;
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Enumerate through each entry in the list view
    cListItems = ListView_GetItemCount(hwndList);
    for (iListItem = 0; iListItem < cListItems; iListItem++)
    {
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = iListItem;
        lvItem.iSubItem = 0;

        if (ListView_GetItem(hwndList, &lvItem))
        {
            pPrincipal = (LPPRINCIPAL) lvItem.lParam;

            if (pPrincipal != NULL)
            {
                // Are we looking for a particular principal?
                if (pSid && !EqualSid(pSid, pPrincipal->GetSID()))
                    continue;

                // Do we already have a good name?
                if (pPrincipal->HaveRealName())
                {
                    if (pSid)
                        break;  // only care about this principal, stop here
                    else
                        continue;   // skip this one and check the rest
                }

                // Lookup the SID for this principal in the cache
                LookupSid(pPrincipal->GetSID(),
                          m_siObjectInfo.pszServerName,
                          m_psi2,
                          &pUserList);

                if ((pUserList != NULL) && (pUserList->cUsers == 1))
                {
                    // The list should contain a single item
                    PUSER_INFO pUserInfo = &pUserList->rgUsers[0];

                    // Update the principal with this new name information
                    pPrincipal->SetSidType(pUserInfo->SidType);
                    pPrincipal->SetName(pUserInfo->pszName, pUserInfo->pszLogonName);

                    // Set the text of this item to the name we've found
                    lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
                    lvItem.pszText = (LPTSTR)pPrincipal->GetName();
                    lvItem.iImage = pPrincipal->GetImageIndex();
                    ListView_SetItem(hwndList, &lvItem);

                    LocalFree(pUserList);
                }
            }
        }
    }

    SetCursor(hcur);

    TraceLeaveResult(hr);
}


int
CPermPage::AddPrincipalToList(HWND hwndList, LPPRINCIPAL pPrincipal)
{
    LVITEM lvItem;
    int iIndex = -1;

    TraceEnter(TRACE_PERMPAGE, "CPermPage::AddPrincipalToList");
    TraceAssert(hwndList != NULL);
    TraceAssert(pPrincipal != NULL);

    // Insert new principal into listview
    lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    lvItem.iItem = 0;
    lvItem.iSubItem = 0;
    lvItem.lParam = (LPARAM)pPrincipal;
    lvItem.pszText = (LPTSTR)pPrincipal->GetName();
    lvItem.iImage = pPrincipal->GetImageIndex();

    iIndex = ListView_InsertItem(hwndList, &lvItem);

    TraceLeaveValue(iIndex);
}


BOOL
CPermPage::InitDlg(HWND hDlg)
{
    HRESULT hr = S_OK;
    HWND hwnd;
    HWND hwndList;
    RECT rc;
    LV_COLUMN col;
    TCHAR szBuffer[MAX_COLUMN_CHARS];
    PSECURITY_DESCRIPTOR pSD = NULL;
    BOOL bUserNotified = FALSE;
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    TraceEnter(TRACE_PERMPAGE, "CPermPage::InitDlg");
    TraceAssert(hDlg != NULL);
    TraceAssert(m_psi != NULL);

    hwndList = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);

    //
    // Create & set the image list for the listview.  If there is a
    // problem CreateSidImageList will return NULL which won't hurt
    // anything. In that case we'll just continue without an image list.
    //
    ListView_SetImageList(hwndList,
                          LoadImageList(::hModule, MAKEINTRESOURCE(IDB_SID_ICONS)),
                          LVSIL_SMALL);

    // Set extended LV style for whole line selection with InfoTips
    ListView_SetExtendedListViewStyleEx(hwndList,
                                        LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP,
                                        LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

    //
    // Add appropriate listview columns
    //
    GetClientRect(hwndList, &rc);

    LoadString(::hModule, IDS_NAME, szBuffer, ARRAYSIZE(szBuffer));
    col.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    col.pszText = szBuffer;
    col.iSubItem = 0;
    col.cx = rc.right;
    ListView_InsertColumn(hwndList, 0, &col);

    if (!(m_siObjectInfo.dwFlags & SI_RESET))
    {
        // Hide the Reset button.
        hwnd = GetDlgItem(hDlg, IDC_SPP_RESET);
        ShowWindow(hwnd, SW_HIDE);
        EnableWindow(hwnd, FALSE);
    }

    if (!(m_siObjectInfo.dwFlags & SI_ADVANCED))
    {
        // Hide the Advanced button
        hwnd = GetDlgItem(hDlg, IDC_SPP_ADVANCED);
        ShowWindow(hwnd, SW_HIDE);
        EnableWindow(hwnd, FALSE);
        hwnd = GetDlgItem(hDlg, IDC_SPP_MORE_MSG);
        ShowWindow(hwnd, SW_HIDE);
        EnableWindow(hwnd, FALSE);
    }

    if (m_siObjectInfo.dwFlags & SI_NO_ACL_PROTECT)
    {
        // Hide the "Inherit permissions" box
        hwnd = GetDlgItem(hDlg, IDC_SPP_PROTECT);
        ShowWindow(hwnd, SW_HIDE);
        EnableWindow(hwnd, FALSE);
    }

    if (S_FALSE == m_hrLastPSPCallbackResult)
    {
        // The propsheetpage callback told us to not show any messages here.
        bUserNotified = TRUE;
    }

    if (m_bAbortPage)
    {
        // Disable everything except the Advanced button
        m_siObjectInfo.dwFlags |= SI_READONLY;
        EnableWindow(hwndList, FALSE);

        // The user should have been notified during the propsheetpage
        // callback, so don't put up another message now.
        bUserNotified = TRUE;
    }
    else
    {
        //
        // Initialize the checklist window
        //
        hr = InitCheckList(hDlg);
        FailGracefully(hr, "Failed to initialize checklist");

        //
        // Retrieve the DACL from the object and set it into the dialog
        //
        hr = m_psi->GetSecurity(DACL_SECURITY_INFORMATION, &pSD, FALSE);

        if (SUCCEEDED(hr))
        {
            // We always disable the advanced button until the SID name cache
            // is filled on our other thread. See the DlgProc handler for
            // UM_SIDLOOKUPCOMPLETE
            EnableWindow(GetDlgItem(hDlg, IDC_SPP_ADVANCED), FALSE);

            hr = SetDacl(hDlg, pSD);
            FailGracefully(hr, "SetDacl failed");
        }
        else if (hr == E_ACCESSDENIED)
        {
            if (!bUserNotified)
            {
                //
                // Can't read the DACL or Owner, figure out what we CAN do.
                //
                UINT idMsg = IDS_PERM_NO_ACCESS;
                UINT mbType = MB_OK | MB_ICONWARNING;

                if (!(m_siObjectInfo.dwFlags & SI_READONLY))
                {
                    idMsg = IDS_PERM_CANT_READ_CAN_WRITE_DACL;
                }
                else
                {
                    //
                    // Can't write the DACL, can we write the owner or edit the SACL?
                    //
                    DWORD dwFlags = m_siObjectInfo.dwFlags & (SI_EDIT_AUDITS | SI_OWNER_READONLY);

                    // If we're not editing the owner, then we can't write it.
                    if (!(m_siObjectInfo.dwFlags & SI_EDIT_OWNER))
                        dwFlags |= SI_OWNER_READONLY;

                    switch(dwFlags)
                    {
                    case 0:
                        // Can write the Owner but can't edit the SACL
                        idMsg = IDS_PERM_CANT_READ_CAN_WRITE_OWNER;
                        break;

                    case SI_EDIT_AUDITS:
                        // Can edit the SACL and write the Owner
                        idMsg = IDS_PERM_CANT_READ_CAN_AUDIT_WRITE_OWNER;
                        break;

                    case SI_OWNER_READONLY:
                        // No Access
                        break;

                    case SI_OWNER_READONLY | SI_EDIT_AUDITS:
                        // Can edit the SACL but can't write the Owner
                        idMsg = IDS_PERM_CANT_READ_CAN_AUDIT;
                        break;
                    }
                }

                if (idMsg == IDS_PERM_NO_ACCESS)
                    mbType = MB_OK | MB_ICONERROR;

                MsgPopup(hDlg,
                         MAKEINTRESOURCE(idMsg),
                         MAKEINTRESOURCE(IDS_SECURITY),
                         mbType,
                         ::hModule,
                         m_siObjectInfo.pszObjectName);
                bUserNotified = TRUE;
            }

            EnablePrincipalControls(hDlg, FALSE);
            hr = S_OK;
        }
        else
        {
            FailGracefully(hr, "GetSecurity failed");
        }
    } // !m_bAbortPage

    if (m_siObjectInfo.dwFlags & SI_READONLY)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_SPP_ADD), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_SPP_RESET), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_SPP_PROTECT), FALSE);
        EnablePrincipalControls(hDlg, FALSE);

        // Tell the user that we're in read-only mode
        if (!bUserNotified)
        {
            MsgPopup(hDlg,
                     MAKEINTRESOURCE(IDS_PERM_READONLY),
                     MAKEINTRESOURCE(IDS_SECURITY),
                     MB_OK | MB_ICONINFORMATION,
                     ::hModule,
                     m_siObjectInfo.pszObjectName);

            bUserNotified = TRUE;
        }
    }

exit_gracefully:

    if (pSD != NULL)
        LocalFree(pSD);

    SetCursor(hcur);

    if (FAILED(hr))
    {
        // Hide and disable everything
        for (hwnd = GetWindow(hDlg, GW_CHILD);
             hwnd != NULL;
             hwnd = GetWindow(hwnd, GW_HWNDNEXT))
        {
            ShowWindow(hwnd, SW_HIDE);
            EnableWindow(hwnd, FALSE);
        }

        // Enable and show the "No Security" message
        hwnd = GetDlgItem(hDlg, IDC_SPP_NO_SECURITY);
        EnableWindow(hwnd, TRUE);
        ShowWindow(hwnd, SW_SHOW);
    }

    TraceLeaveValue(TRUE);
}


BOOL
CPermPage::OnNotify(HWND hDlg, int /*idCtrl*/, LPNMHDR pnmh)
{
    LPNM_LISTVIEW pnmlv = (LPNM_LISTVIEW)pnmh;

    TraceEnter(TRACE_PERMPAGE, "CPermPage::OnNotify");
    TraceAssert(hDlg != NULL);
    TraceAssert(pnmh != NULL);

    // Set default return value
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

    switch (pnmh->code)
    {
    case LVN_ITEMCHANGED:
        if (pnmlv->uChanged & LVIF_STATE)
        {
            // item *gaining* selection
            if ((pnmlv->uNewState & LVIS_SELECTED) &&
                !(pnmlv->uOldState & LVIS_SELECTED))
            {
                OnSelChange(hDlg);
            }
            // item *losing* selection
            else if (!(pnmlv->uNewState & LVIS_SELECTED) &&
                     (pnmlv->uOldState & LVIS_SELECTED))
            {
                // Post ourselves a message to check for a new selection later.
                // If we haven't gotten a new selection by the time we process
                // this message, then assume the user clicked inside the listview
                // but not on an item, thus causing the listview to remove the
                // selection.  In that case, disable the combobox & Remove button.
                //
                // Do this via WM_COMMAND rather than WM_NOTIFY so we don't
                // have to allocate/free a NMHDR structure.
                PostMessage(hDlg,
                            WM_COMMAND,
                            GET_WM_COMMAND_MPS(pnmh->idFrom, pnmh->hwndFrom, IDN_CHECKSELECTION));
            }
        }
        break;

    case LVN_DELETEITEM:
        delete (LPPRINCIPAL)pnmlv->lParam;
        break;

    case LVN_KEYDOWN:
        if (((LPNMLVKEYDOWN)pnmh)->wVKey == VK_DELETE)
            OnRemovePrincipal(hDlg);
        break;

#ifdef UNUSED
    case NM_DBLCLK:
        if (pnmh->idFrom == IDC_SPP_PRINCIPALS)
        {
            // Must have a selection to get here
            TraceAssert(ListView_GetSelectedCount(pnmh->hwndFrom) == 1);

            // do something here
        }
        break;
#endif

    case CLN_CLICK:
        if (pnmh->idFrom == IDC_SPP_PERMS)
        {
            LPPRINCIPAL pPrincipal;
            int iIndex = -1;

            //
            // HandleListClick decides which boxes should be checked and
            // unchecked, however, we can't rely only on that to generate
            // ACLs (we used to).  Suppose the principal has Full Control and
            // the user unchecks "Delete" which is a single bit.  If there is
            // no checkbox corresponding to "Full Control minus Delete" then
            // the principal would also lose other bits, such as WRITE_DAC.
            //
            // So let HandleListClick do its thing. Then remove permission
            // bits according to what was checked or unchecked.
            //
            // But wait, there's more. Removing permission bits turns off
            // too much. For example, if the principal has Full Control and
            // the user turns off Full Control, then the principal ends up
            // with nothing, even though HandleListClick leaves Modify
            // checked.
            //
            // So after removing what was just (un)checked, build new
            // permissions from what is still checked and add them.
            //
            // This yields the correct results, and also keeps the principal
            // up-to-date so we don't need to call CommitCurrent anywhere else.
            //
            // Raid 260952
            //

            // Check/uncheck appropriate boxes in both columns
            HandleListClick((PNM_CHECKLIST)pnmh,
                            SI_PAGE_PERM,
                            m_siObjectInfo.dwFlags & SI_CONTAINER);

            pPrincipal = (LPPRINCIPAL)GetSelectedItemData(GetDlgItem(hDlg, IDC_SPP_PRINCIPALS), &iIndex);
            if (pPrincipal)
            {
                PNM_CHECKLIST   pnmc    = (PNM_CHECKLIST)pnmh;
                PSI_ACCESS      pAccess = (PSI_ACCESS)pnmc->dwItemData;
                PERMISSION      perm    = { pAccess->mask, 0, 0 };

                // If the user UNchecked a box, we want to
                // remove from the corresponding list.
                // If the user checked a box, we want to
                // remove from the opposite list.

                // Which column was clicked?
                BOOL bRemoveFromAllow = (1 == pnmc->iSubItem);    // 1 = Allow, 2 = Deny

                // Was it checked or UNchecked?
                if (pnmc->dwState & CLST_CHECKED)
                    bRemoveFromAllow = !bRemoveFromAllow; // checked --> remove from opposite

                if (pAccess->pguid)
                    perm.guid = *pAccess->pguid;

                if (m_siObjectInfo.dwFlags & SI_CONTAINER)
                    perm.dwFlags = pAccess->dwFlags & VALID_INHERIT_FLAGS;

                pPrincipal->RemovePermission(bRemoveFromAllow, &perm);

                if (pnmc->dwState & CLST_CHECKED)
                    pPrincipal->AddPermission(1 == pnmc->iSubItem, &perm);
            }

            SetDirty(hDlg);

            // Add perms according to what is still checked
            CommitCurrent(hDlg, iIndex);

            // Reset the "There is more stuff" message
            OnSelChange(hDlg, FALSE);
        }
        break;

    case CLN_GETCOLUMNDESC:
        {
            PNM_CHECKLIST pnmc = (PNM_CHECKLIST)pnmh;
            GetDlgItemText(hDlg,
                           IDC_SPP_ALLOW - 1 + pnmc->iSubItem,
                           pnmc->pszText,
                           pnmc->cchTextMax);
        }
        break;

    case PSN_APPLY:
        OnApply(hDlg, (BOOL)(((LPPSHNOTIFY)pnmh)->lParam));
        break;

    default:
        TraceLeaveValue(FALSE); // message not handled
    }

    TraceLeaveValue(TRUE);  // message handled
}

BOOL
CheckPermissions(HWND hwndList,
                 const CPermissionSet &PermSet,
                 WORD wColumn,
                 BOOL bDisabled,
                 BOOL bInheritFlags)
{
    UINT cRights = (UINT)SendMessage(hwndList, CLM_GETITEMCOUNT, 0, 0);
    UINT cAces = PermSet.GetPermCount();
    BOOL bMorePresent = FALSE;
    WORD wOtherColumn;
    DWORD dwState = CLST_CHECKED;

    TraceEnter(TRACE_MISC, "CheckPermissions");

    if (wColumn == 1)
        wOtherColumn = 2;
    else
        wOtherColumn = 1;

    if (bDisabled)
        dwState |= CLST_DISABLED;

    for (UINT j = 0; j < cAces; j++)
    {
        ACCESS_MASK maskChecked = 0;
        PPERMISSION pPerm = PermSet[j];
        BOOL bIsNullGuid = IsNullGUID(&pPerm->guid);

        for (UINT i = 0; i < cRights; i++)
        {
            PSI_ACCESS pAccess = (PSI_ACCESS)SendMessage(hwndList,
                                                         CLM_GETITEMDATA,
                                                         i,
                                                         0);
            //
            // The below expression tests to see if this access mask enables
            // this access "rights" line.  It could have more bits enabled, but
            // as long as it has all of the ones from the pAccess[i].mask then
            // it effectively has that option enabled.
            //
            if ( (pPerm->mask & pAccess->mask) == pAccess->mask &&
                 (bIsNullGuid || IsSameGUID(&pPerm->guid, pAccess->pguid)) )
            {
                DWORD dwStateCompare;

                //
                // Next, check the inherit flags.
                //
                if (bInheritFlags)
                {
                    DWORD dwCommonFlags = pPerm->dwFlags & pAccess->dwFlags;

                    //
                    // This expression tests to see whether the ACE applies
                    // to all objects that this access rights line applies to.
                    // The ACE must have at least as many of (CONTAINER_INHERIT_ACE,
                    // OBJECT_INHERIT_ACE) turned on as the rights line, and
                    // if the ACE has INHERIT_ONLY_ACE, then so must the rights line.
                    //
                    if (!((dwCommonFlags & ACE_INHERIT_ALL) == (pAccess->dwFlags & ACE_INHERIT_ALL)
                          && (dwCommonFlags & INHERIT_ONLY_ACE) == (pPerm->dwFlags & INHERIT_ONLY_ACE)))
                    continue;
                }

                // The bits say it's checked. We may not actually check the box
                // below, but for other reasons. In any case, we don't want the
                // "Additional stuff is here but I can't show it" message to
                // display because of this perm.
                maskChecked |= pAccess->mask;

                //
                // Ok, the bits say that this box should be checked, but
                // if the other column is already checked and has the same
                // enabled/disabled state, then we don't check this one.
                // This keeps us from having both Allow and Deny checked &
                // enabled on the same line (nonsense) or checked & disabled
                // on the same line (both inherited; Deny wins since it comes
                // first, but you can't override it by checking Allow).
                //
#ifndef NO_RADIOBUTTON_BEHAVIOR
                dwStateCompare = (DWORD)SendMessage(hwndList,
                                                    CLM_GETSTATE,
                                                    MAKELONG((WORD)i, wOtherColumn),
                                                    0);
                if ((dwStateCompare & CLST_CHECKED) &&
                    ((dwStateCompare & CLST_DISABLED) == (dwState & CLST_DISABLED)))
                    continue;
#endif

                //
                // Next, see if the box is already checked. If so, leave it
                // alone. Note that we don't compare the enabled/disabled
                // state.  The effect here is that the first check wins.
                // Raid 326000
                //
                dwStateCompare = (DWORD)SendMessage(hwndList,
                                                    CLM_GETSTATE,
                                                    MAKELONG((WORD)i, wColumn),
                                                    0);
                if (dwStateCompare & CLST_CHECKED)
                    continue;

                //
                // Finally, check the box.
                //
                SendMessage(hwndList,
                            CLM_SETSTATE,
                            MAKELONG((WORD)i, wColumn),
                            dwState);
            }
        }

        // Does this ACE have bits that aren't shown on this dialog?
        if (maskChecked != pPerm->mask)
            bMorePresent = TRUE;
    }

    // Does this permission set have "advanced" ACEs that aren't shown
    // on this dialog?
    if (!bMorePresent && cAces != PermSet.GetPermCount(TRUE))
        bMorePresent = TRUE;

    TraceLeaveValue(bMorePresent);
}


void
CPermPage::OnSelChange(HWND hDlg, BOOL bClearFirst)
{
    BOOL bDisabled = m_siObjectInfo.dwFlags & SI_READONLY;

    TraceEnter(TRACE_PERMPAGE, "CPermPage::OnSelChange");
    TraceAssert(hDlg != NULL);

    //
    // If the principal list is empty or there is no selection, then we need
    // to disable all of the controls that operate on items in the listbox.
    //
    HWND hwndList = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);
    TraceAssert(hwndList != NULL);

    // Get the selected principal
    LPPRINCIPAL pPrincipal = (LPPRINCIPAL)GetSelectedItemData(hwndList, NULL);

    // Enable/disable the other controls
    if (!bDisabled)
        EnablePrincipalControls(hDlg, pPrincipal != NULL);

    if (pPrincipal == NULL)
        TraceLeaveVoid();   // no selection or empty list

    //
    // Check/uncheck the permission boxes
    //

    hwndList = GetDlgItem(hDlg, IDC_SPP_PERMS);
    TraceAssert(hwndList != NULL);

    if (bClearFirst)
    {
        // First need to uncheck everything
        ClearPermissions(hwndList, bDisabled);
    }

    BOOL bIsContainer = m_siObjectInfo.dwFlags & SI_CONTAINER;
    BOOL bMorePresent = FALSE;

    bMorePresent |= CheckPermissions(hwndList, pPrincipal->m_permDeny, 2, bDisabled, bIsContainer);
    bMorePresent |= CheckPermissions(hwndList, pPrincipal->m_permAllow, 1, bDisabled, bIsContainer);
    bMorePresent |= CheckPermissions(hwndList, pPrincipal->m_permInheritedDeny, 2, TRUE, bIsContainer);
    bMorePresent |= CheckPermissions(hwndList, pPrincipal->m_permInheritedAllow, 1, TRUE, bIsContainer);

    if (m_siObjectInfo.dwFlags & SI_ADVANCED)
    {
        ShowWindow(GetDlgItem(hDlg, IDC_SPP_MORE_MSG),
                   (bMorePresent ? SW_SHOW : SW_HIDE));
    }
    else if (bMorePresent)
    {
        TraceMsg("Ignoring unknown permissions");
    }

    TraceLeaveVoid();
}

void
CPermPage::OnApply(HWND hDlg, BOOL bClose)
{
    HRESULT hr = S_OK;
    PSECURITY_DESCRIPTOR pSD;

    TraceEnter(TRACE_PERMPAGE, "CPermPage::OnApply");

    // Build a new DACL without the inherited ACEs.
    if (m_fPageDirty && SUCCEEDED(hr = BuildDacl(hDlg, &pSD, FALSE)))
    {
        PISECURITY_DESCRIPTOR psd = (PISECURITY_DESCRIPTOR)pSD;
        DWORD dwWarning = 0;
        SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;

        TraceAssert(pSD != NULL);
        TraceAssert(m_psi != NULL);

        // Check for Deny ACEs in the ACL
        if (!m_bWasDenyAcl)
        {
            DWORD dwFullControl = GENERIC_ALL;
            UCHAR aceFlags = 0;

            m_psi->MapGeneric(NULL, &aceFlags, &dwFullControl);
            if (IsDenyACL(psd->Dacl,
                          (psd->Control & SE_DACL_PROTECTED),
                          dwFullControl,
                          &dwWarning))
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
                    hr = S_FALSE;
                }
            }
        }

        if (S_FALSE != hr)
        {
            // Apply the new security descriptor on the object
            hr = m_psi->SetSecurity(si, pSD);
        }

        if (S_OK == hr)
        {
            LocalFree(pSD);
            pSD = NULL;
            m_fPageDirty = FALSE;

            if (!bClose)
            {
                //
                // Read the new DACL back from the object.  This ensures that we
                // have the "real" current DACL in case it was modified by the
                // object.  For example, inherited aces may have been added.
                //
                // This also resets the dialog to the initial state if the
                // user chose No in the confirmation dialog above.
                //
                if (SUCCEEDED(m_psi->GetSecurity(DACL_SECURITY_INFORMATION, &pSD, FALSE)))
                    SetDacl(hDlg, pSD);
            }
        }
        else if (S_FALSE == hr)
        {
            // S_FALSE is silent failure (the client should put up UI
            // during SetSecurity before returning S_FALSE).
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
        }

        if (pSD != NULL)
            LocalFree(pSD);
    }

    if (FAILED(hr))
    {
        // Tell the user there was a problem.  If they choose to cancel
        // and the dialog is closing, do nothing (let the dialog close).
        // Otherwise, tell the property sheet that we had a problem.
        if (IDCANCEL != SysMsgPopup(hDlg,
                                    MAKEINTRESOURCE(IDS_PERM_WRITE_FAILED),
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

    TraceLeaveVoid();
}


HRESULT
CPermPage::BuildDacl(HWND hDlg,
                     PSECURITY_DESCRIPTOR *ppSD,
                     BOOL fIncludeInherited)
{
    PISECURITY_DESCRIPTOR pSD;
    ULONG nAclSize;
    LPPRINCIPAL pPrincipal;
    int cPrincipals = 0;
    DWORD dwFlags;
    int i, j;
    HCURSOR hcur = NULL;
    HWND hwndList = NULL;
    LV_ITEM lvItem;
    lvItem.mask = LVIF_PARAM;
    lvItem.iSubItem = 0;

    static DWORD dwCanonicalFlags[] =
    {
        ACL_DENY | ACL_NONOBJECT,
        ACL_DENY | ACL_OBJECT,
        ACL_ALLOW | ACL_NONOBJECT,
        ACL_ALLOW | ACL_OBJECT
    };

    TraceEnter(TRACE_PERMPAGE, "CPermPage::BuildDacl");
    TraceAssert(hDlg != NULL);
    TraceAssert(ppSD != NULL);

    *ppSD = NULL;

    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    // Estimate the size of the buffer necessary to build the
    // Security Descriptor.
    //

    hwndList = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);
    cPrincipals = ListView_GetItemCount(hwndList);

    dwFlags = ACL_NONINHERITED;
    if (fIncludeInherited)
        dwFlags |= ACL_INHERITED;

    nAclSize = SIZEOF(ACL);

    for (i = 0; i < cPrincipals; i++)
    {
        lvItem.iItem = i;
        if (ListView_GetItem(hwndList, &lvItem))
        {
            pPrincipal = (LPPRINCIPAL)lvItem.lParam;
            nAclSize += pPrincipal->GetAclLength(dwFlags);
        }
    }

    //
    // Now that we have the size estimate, allocate the buffer.  Note that
    // we allocate enough memory for a self-relative security descriptor, but
    // don't set the SE_SELF_RELATIVE flag in pSD->Control.  This lets us
    // use pSD->Dacl, etc. as pointers rather than offsets.
    //

    *ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH + nAclSize);
    if (*ppSD == NULL)
        TraceLeaveResult(E_OUTOFMEMORY);

    InitializeSecurityDescriptor(*ppSD, SECURITY_DESCRIPTOR_REVISION);

    pSD = (PISECURITY_DESCRIPTOR)*ppSD;

    //
    // Finally, build the security descriptor
    //
    pSD->Control |= SE_DACL_PRESENT | SE_DACL_AUTO_INHERIT_REQ
                    | (m_wSDControl & (SE_DACL_PROTECTED | SE_DACL_AUTO_INHERITED));

    if (nAclSize > 0)
    {
        pSD->Dacl = (PACL)(pSD + 1);
        pSD->Dacl->AclRevision = ACL_REVISION;
        pSD->Dacl->AclSize = (WORD)nAclSize;
        pSD->Dacl->AceCount = 0;

        PACE_HEADER pAcePos = (PACE_HEADER)FirstAce(pSD->Dacl);

        DWORD dwExtraFlags = fIncludeInherited ? 0 : ACL_CHECK_CREATOR;

        // Build the DACL in the following order:
        //      Deny
        //      Allow
        //      Inherited Deny
        //      Inherited Allow

        for (j = 0; j < ARRAYSIZE(dwCanonicalFlags); j++)
        {
            for (i = 0; i < cPrincipals; i++)
            {
                lvItem.iItem = i;
                if (ListView_GetItem(hwndList, &lvItem))
                {
                    pPrincipal = (LPPRINCIPAL)lvItem.lParam;
                    pPrincipal->AppendToAcl(pSD->Dacl,
                                            ACL_NONINHERITED | dwCanonicalFlags[j] | dwExtraFlags,
                                            &pAcePos);
                }
            }
        }

        if (fIncludeInherited)
        {
            for (j = 0; j < ARRAYSIZE(dwCanonicalFlags); j++)
            {
                for (i = 0; i < cPrincipals; i++)
                {
                    lvItem.iItem = i;
                    if (ListView_GetItem(hwndList, &lvItem))
                    {
                        pPrincipal = (LPPRINCIPAL)lvItem.lParam;
                        pPrincipal->AppendToAcl(pSD->Dacl,
                                                ACL_INHERITED | dwCanonicalFlags[j] | dwExtraFlags,
                                                &pAcePos);
                    }
                }
            }
        }

        // Set accurate size information for the ACL
        nAclSize = (ULONG)((PBYTE)pAcePos - (PBYTE)pSD->Dacl);
        TraceAssert(nAclSize >= SIZEOF(ACL));
        TraceAssert(pSD->Dacl->AclSize >= nAclSize);

        if (pSD->Dacl->AclSize > nAclSize)
            pSD->Dacl->AclSize = (WORD)nAclSize;

        TraceAssert(m_psi2 || IsDACLCanonical(pSD->Dacl));
    }

    TraceAssert(pSD && IsValidSecurityDescriptor(pSD));

    SetCursor(hcur);

    TraceLeaveResult(S_OK);
}


HRESULT
CPermPage::SetDacl(HWND hDlg,
                   PSECURITY_DESCRIPTOR pSD,
                   BOOL bDirty)
{
    HRESULT hr = S_OK;
    PACL pAcl = NULL;
    PACL paclAllowAll = NULL;
    BOOL bDefaulted;
    BOOL bPresent;
    SECURITY_DESCRIPTOR_CONTROL wSDControl = 0;
    PSECURITY_DESCRIPTOR pSDDefault = NULL;
    DWORD dwRevision;

    TraceEnter(TRACE_PERMPAGE, "CPermPage::SetDacl");
    TraceAssert(hDlg != NULL);

    if (pSD != NULL && !IsValidSecurityDescriptor(pSD))
        TraceLeaveResult(E_INVALIDARG);

    if (pSD != NULL)
        GetSecurityDescriptorControl(pSD, &wSDControl, &dwRevision);

    // Save the DACL protection and auto-inherited bits
    m_wSDControl &= ~(SE_DACL_DEFAULTED | SE_DACL_PROTECTED | SE_DACL_AUTO_INHERITED);
    m_wSDControl |= (wSDControl & (SE_DACL_DEFAULTED | SE_DACL_PROTECTED | SE_DACL_AUTO_INHERITED));

    // Get a pointer to the new DACL
    if (pSD != NULL)
        GetSecurityDescriptorDacl(pSD, &bPresent, &pAcl, &bDefaulted);

    if (!(m_siObjectInfo.dwFlags & SI_READONLY))
    {
        // Check for canonical ordering (Deny, Allow, Inherited Deny, Inherited Allow)
        if ((m_psi2 && !m_psi2->IsDaclCanonical(pAcl))
            || (!m_psi2 && !IsDACLCanonical(pAcl)))
        {
            TraceMsg("DACL not in canonical order!");

            // Ask the user whether to canonicalize the DACL or
            // blow it away completely.
            if (IDCANCEL == MsgPopup(hDlg,
                                     MAKEINTRESOURCE(IDS_PERM_NOT_CANONICAL),
                                     MAKEINTRESOURCE(IDS_SECURITY),
                                     MB_OKCANCEL | MB_ICONWARNING,
                                     ::hModule,
                                     m_siObjectInfo.pszObjectName))
            {
                // Blow it away and start over.
                pAcl = NULL;

                // Does the caller support a default ACL?  If so, get it now.
                if (m_siObjectInfo.dwFlags & SI_RESET)
                {
                    hr = m_psi->GetSecurity(DACL_SECURITY_INFORMATION,
                                            &pSDDefault,
                                            TRUE);

                    if (SUCCEEDED(hr) && pSDDefault != NULL)
                    {
                        // Save the DACL control bits
                        GetSecurityDescriptorControl(pSDDefault, &wSDControl, &dwRevision);
                        m_wSDControl &= ~(SE_DACL_DEFAULTED | SE_DACL_PROTECTED | SE_DACL_AUTO_INHERITED);
                        m_wSDControl |= SE_DACL_DEFAULTED | (wSDControl & (SE_DACL_PROTECTED | SE_DACL_AUTO_INHERITED));

                        // Get a pointer to the new DACL
                        GetSecurityDescriptorDacl(pSDDefault, &bPresent, &pAcl, &bDefaulted);
                    }
                    // else go with a NULL DACL
                }
            }
            // else simply continuing and re-saving will
            // cause the DACL to get sorted correctly

            // This causes a PropSheet_Changed notification to be sent below
            bDirty = TRUE;
        }
    }

    m_bWasDenyAcl = FALSE;

    // A NULL ACL implies "Everyone Full control", so
    // create such an ACL here
    if (pAcl == NULL)
    {
        PSID psidWorld = QuerySystemSid(UI_SID_World);
        DWORD dwSidLength = GetLengthSid(psidWorld);
        DWORD dwAclLength = SIZEOF(ACL) + SIZEOF(ACCESS_ALLOWED_ACE)
                            - SIZEOF(DWORD) + dwSidLength;

        m_wDaclRevision = ACL_REVISION;

        paclAllowAll = (PACL)LocalAlloc(LPTR, dwAclLength);
        if (paclAllowAll != NULL)
        {
            paclAllowAll->AclRevision = ACL_REVISION;
            paclAllowAll->AclSize = (WORD)dwAclLength;
#if 0 //(_WIN32_WINNT >= 0x0500)
            paclAllowAll->AceCount = 0;

            AddAccessAllowedAceEx(paclAllowAll,
                                  ACL_REVISION,
                                  ACE_INHERIT_ALL,
                                  GENERIC_ALL,
                                  psidWorld);
#else
            paclAllowAll->AceCount = 1;

            PACE_HEADER pAce = (PACE_HEADER)FirstAce(paclAllowAll);
            pAce->AceType = ACCESS_ALLOWED_ACE_TYPE;
            pAce->AceFlags = ACE_INHERIT_ALL;
            pAce->AceSize = (WORD)dwAclLength - SIZEOF(ACL);
            ((PACCESS_ALLOWED_ACE)pAce)->Mask = GENERIC_ALL;
            CopyMemory(&((PACCESS_ALLOWED_ACE)pAce)->SidStart, psidWorld, dwSidLength);
#endif
            pAcl = paclAllowAll;
        }
    }
    else
    {
        DWORD dwFullControl = GENERIC_ALL;
        UCHAR aceFlags = 0;

        m_psi->MapGeneric(NULL, &aceFlags, &dwFullControl);
        if (IsDenyACL(pAcl,
                      (m_wSDControl & SE_DACL_PROTECTED),
                      dwFullControl,
                      NULL))
        {
            // Already have Deny ACEs, don't bother warning again later.
            m_bWasDenyAcl = TRUE;
        }
    }

    // Set the state of the "Inherit permissions" button
    CheckDlgButton(hDlg, IDC_SPP_PROTECT, !(m_wSDControl & SE_DACL_PROTECTED));

    // Reset the list of principals
    InitPrincipalList(hDlg, pAcl);

    // If there aren't any entries, fake a sel change to update
    // (i.e. disable) the other controls.
    if (pAcl == NULL || pAcl->AceCount == 0)
        OnSelChange(hDlg);

    if (bDirty)
        SetDirty(hDlg, TRUE);

    if (paclAllowAll != NULL)
        LocalFree(paclAllowAll);

    if (pSDDefault != NULL)
        LocalFree(pSDDefault);

    TraceLeaveResult(hr);
}


void
CPermPage::OnAddPrincipal(HWND hDlg)
{
    PUSER_LIST pUserList = NULL;

    TraceEnter(TRACE_PERMPAGE, "CPermPage::OnAddPrincipal");
    TraceAssert(hDlg != NULL);

    if (S_OK == GetUserGroup(hDlg, TRUE, &pUserList))
    {
        PUSER_INFO pUserInfo;
        DWORD i;
        BOOL fPageModified = FALSE;
        int iItem = -1;

        TraceAssert(NULL != pUserList);

        HWND hwndList = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);
        TraceAssert(hwndList != NULL);

        for (i = 0; i < pUserList->cUsers; i++)
        {
            int cItems;
            LV_ITEM lvItem;
            LPPRINCIPAL pPrincipal;
            BYTE buffer[SIZEOF(KNOWN_OBJECT_ACE) + SIZEOF(GUID)];
            PACE_HEADER pAce = (PACE_HEADER)buffer;

            pUserInfo = &pUserList->rgUsers[i];
            iItem = -1;

            // Check whether the new principal is already in our list.
            // If so, don't add it again.
            cItems = ListView_GetItemCount(hwndList);
            lvItem.iSubItem = 0;
            lvItem.mask = LVIF_PARAM;
            while (cItems > 0)
            {
                LPPRINCIPAL pPrincipal;

                --cItems;
                lvItem.iItem = cItems;

                ListView_GetItem(hwndList, &lvItem);
                pPrincipal = (LPPRINCIPAL)lvItem.lParam;

                if (EqualSid(pPrincipal->GetSID(), pUserInfo->pSid))
                {
                    iItem = lvItem.iItem;
                    break;
                }
            }

            // Did we find it?
            if (iItem != -1)
                continue;

            // ListView_FindItem failed to find a match.  Add a
            // new principal.

            pPrincipal = new CPrincipal(this);
            if (!pPrincipal)
                continue;

            // Initialize principal
            if (!pPrincipal->SetPrincipal(pUserInfo->pSid,
                                          pUserInfo->SidType,
                                          pUserInfo->pszName,
                                          pUserInfo->pszLogonName))
            {
                delete pPrincipal;
                continue;
            }

            lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            lvItem.iItem = 0;
            lvItem.iSubItem = 0;
            lvItem.pszText = (LPTSTR)pPrincipal->GetName();
            lvItem.iImage = pPrincipal->GetImageIndex();
            lvItem.lParam = (LPARAM)pPrincipal;

            // Insert principal into list
            iItem = ListView_InsertItem(hwndList, &lvItem);
            if (-1 == iItem)
            {
                delete pPrincipal;
                continue;
            }

            // Add ace with default access
            pAce->AceType = ACCESS_ALLOWED_ACE_TYPE;
            pAce->AceFlags = 0;
            pAce->AceSize = SIZEOF(ACCESS_ALLOWED_ACE);
            ((PACCESS_ALLOWED_ACE)pAce)->Mask = m_pDefaultAccess->mask;

            if (m_siObjectInfo.dwFlags & SI_CONTAINER)
            {
                // Pick up inherit bits from the default access
                pAce->AceFlags = (UCHAR)(m_pDefaultAccess->dwFlags & (VALID_INHERIT_FLAGS & ~INHERITED_ACE));

                //
                // Special case for CreatorOwner/CreatorGroup,
                // which are only useful if inherit bits are set.
                //
                if (IsCreatorSid(pUserInfo->pSid))
                {
                    // Make sure it inherits onto something
                    if (!(pAce->AceFlags & ACE_INHERIT_ALL))
                        pAce->AceFlags = ACE_INHERIT_ALL;

                    // It never applies to the current object
                    pAce->AceFlags |= INHERIT_ONLY_ACE;

                    // Set it up so whoever creates an object
                    // gets full control by default
                    ((PACCESS_ALLOWED_ACE)pAce)->Mask = GENERIC_ALL;
                }
            }

            if (!IsNullGUID(m_pDefaultAccess->pguid))
            {
                pAce->AceType = ACCESS_ALLOWED_OBJECT_ACE_TYPE;
                pAce->AceSize = SIZEOF(KNOWN_OBJECT_ACE) + SIZEOF(GUID);
                ((PKNOWN_OBJECT_ACE)pAce)->Flags = ACE_OBJECT_TYPE_PRESENT;
                *RtlObjectAceObjectType(pAce) = *m_pDefaultAccess->pguid;
            }

            pPrincipal->AddAce(pAce);
            fPageModified = TRUE;
        }

        // Done with this now
        LocalFree(pUserList);

        if (fPageModified)
        {
            // If we've added items, resize the Name column
            //ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);

            SetDirty(hDlg);
        }

        if (iItem != -1)
        {
            // Select the last one inserted.
            SelectListViewItem(hwndList, iItem);
        }
    }

    TraceLeaveVoid();
}


void
CPermPage::OnRemovePrincipal(HWND hDlg)
{
    HWND hwndList;
    int iIndex;
    LPPRINCIPAL pPrincipal;

    TraceEnter(TRACE_PERMPAGE, "CPermPage::OnRemovePrincipal");

    hwndList = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);
    pPrincipal = (LPPRINCIPAL)GetSelectedItemData(hwndList, &iIndex);

    if (pPrincipal)
    {
        BOOL bDirty = FALSE;

        if (pPrincipal->GetAclLength(ACL_INHERITED) > 0)
        {
            // This principal has inherited ACEs so we can't remove the principal
            // from the list. Instead, simply remove the non-inherited ACEs from
            // the principal.
            if (pPrincipal->GetAclLength(ACL_NONINHERITED) > 0)
            {
                pPrincipal->m_permDeny.Reset();
                pPrincipal->m_permAllow.Reset();
                bDirty = TRUE;

                // Update the other controls (this happens automatically in the
                // ListView_DeleteItem case below).
                OnSelChange(hDlg);
            }
            else
            {
                // Notify the user that we can't remove inherited ACEs.
                MsgPopup(hDlg,
                         MAKEINTRESOURCE(IDS_PERM_CANT_REMOVE),
                         MAKEINTRESOURCE(IDS_SECURITY),
                         MB_OK | MB_ICONWARNING,
                         ::hModule,
                         pPrincipal->GetName());
            }
        }
        else
        {
             ListView_DeleteItem(hwndList, iIndex);
            //
            // If we just removed the only item, move the focus to the Add button
            // (the Remove button will be disabled in OnSelChange).
            //
            int cItems = ListView_GetItemCount(hwndList);
            if (cItems == 0)
                SetFocus(GetDlgItem(hDlg, IDC_SPP_ADD));
            else
            {
                // If we deleted the last one, select the previous one
                if (cItems <= iIndex)
                    --iIndex;

                SelectListViewItem(hwndList, iIndex);
            }
            bDirty = TRUE;
        }

        // Notify the property sheet that we've changed
        if (bDirty)
            SetDirty(hDlg);
    }

    TraceLeaveVoid();
}


void
CPermPage::OnAdvanced(HWND hDlg)
{
    LPSECURITYINFO psi;

    TraceEnter(TRACE_PERMPAGE, "CPermPage::OnAdvanced");

    //
    // Create an ISecurityInformation wrapper to give to the advanced
    // dialog.  The wrapper intercepts GetSecurity & SetSecurity.
    //
    psi = new CSecurityInfo(this, hDlg);

    if (psi != NULL)
    {
        // Invoke the advanced ACL editor
        EditSecurityEx(hDlg, psi, 0);
        psi->Release();   // release initial reference
    }
    else
    {
        MsgPopup(hDlg,
                 MAKEINTRESOURCE(IDS_OUT_OF_MEMORY),
                 MAKEINTRESOURCE(IDS_SECURITY),
                 MB_OK | MB_ICONERROR,
                 ::hModule);
    }

    TraceLeaveVoid();
}


void
CPermPage::OnReset(HWND hDlg)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    HRESULT hr;

    TraceEnter(TRACE_PERMPAGE, "CPermPage::OnReset");
    TraceAssert(m_psi != NULL);
    TraceAssert(m_siObjectInfo.dwFlags & SI_RESET);

    hr = m_psi->GetSecurity(DACL_SECURITY_INFORMATION, &pSD, TRUE);

    if (SUCCEEDED(hr))
    {
        // The Security Descriptor should have the SE_DACL_DEFAULTED bit set,
        // but we'll make sure by setting it after calling SetDacl.
        hr = SetDacl(hDlg, pSD, TRUE);
        if (SUCCEEDED(hr))
            m_wSDControl |= SE_DACL_DEFAULTED;
        LocalFree(pSD);
    }

    if (FAILED(hr))
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


void
CPermPage::OnProtect(HWND hDlg)
{
    // The "Inherit permissions" checkbox was clicked

    if (!IsDlgButtonChecked(hDlg, IDC_SPP_PROTECT))
    {
        BOOL bHaveInheritedAces = FALSE;
        HWND hwndList = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);
        int cItems = ListView_GetItemCount(hwndList);
        int i;
        LPPRINCIPAL pPrincipal;
        LV_ITEM lvItem;
        lvItem.iSubItem = 0;
        lvItem.mask = LVIF_PARAM;

        // Are there any inherited aces?
        for (i = 0; i < cItems && !bHaveInheritedAces; i++)
        {
            lvItem.iItem = i;
            ListView_GetItem(hwndList, &lvItem);
            pPrincipal = (LPPRINCIPAL)lvItem.lParam;
            bHaveInheritedAces = pPrincipal->HaveInheritedAces();
        }

        if (bHaveInheritedAces)
        {
            // Get confirmation
            int nResult = ConfirmAclProtect(hDlg, TRUE);

            if (nResult == IDCANCEL)
            {
                // Reset the checkbox and bail
                CheckDlgButton(hDlg, IDC_SPP_PROTECT, BST_CHECKED);
                return;
            }

            //
            // Convert inherited aces to non-inherited (or delete them)
            //
            while (cItems > 0)
            {
                --cItems;
                lvItem.iItem = cItems;

                ListView_GetItem(hwndList, &lvItem);
                pPrincipal = (LPPRINCIPAL)lvItem.lParam;
                pPrincipal->ConvertInheritedAces(nResult == IDC_CONFIRM_REMOVE);

                if (pPrincipal->GetAclLength(ACL_NONINHERITED) == 0)
                     ListView_DeleteItem(hwndList, cItems);
            }

            OnSelChange(hDlg);
        }

        m_wSDControl |= SE_DACL_PROTECTED;
    }
    else
    {
        m_wSDControl &= ~SE_DACL_PROTECTED;

        // BUGBUG warn that allowing inheritance could
        // cause new permissions to magically appear.
    }

    SetDirty(hDlg);
}


void
CPermPage::EnablePrincipalControls(HWND hDlg, BOOL fEnable)
{
    TraceEnter(TRACE_PERMPAGE, "CPermPage::EnablePrincipalControls");

    EnableWindow(GetDlgItem(hDlg, IDC_SPP_PERMS), fEnable);

    if (!fEnable)
    {
        ShowWindow(GetDlgItem(hDlg, IDC_SPP_MORE_MSG), SW_HIDE);
    }
    else
    {
#if 0
        LPPRINCIPAL pPrincipal
            = (LPPRINCIPAL)GetSelectedItemData(GetDlgItem(hDlg, IDC_SPP_PRINCIPALS),
                                               NULL);

        // If the selected principal has only inherited ACEs, then disable
        // the Remove button.
        if (pPrincipal &&
            pPrincipal->GetAclLength(ACL_INHERITED) > 0 &&
            pPrincipal->GetAclLength(ACL_NONINHERITED) == 0)
        {
            fEnable = FALSE;
        }
#endif
    }
    EnableWindow(GetDlgItem(hDlg, IDC_SPP_REMOVE), fEnable);

    TraceLeaveVoid();
}

void
CPermPage::CommitCurrent(HWND hDlg, int iPrincipal)
{
    // Commit any outstanding bit changes

    HWND hwndList = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);

    TraceEnter(TRACE_PERMPAGE, "CPermPage::CommitCurrent");

    // If an index isn't provided, get the index of the currently
    // selected principal.
    if (iPrincipal == -1)
        iPrincipal = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

    if (iPrincipal != -1)
    {
        // Get the Principal from the selection.
        LV_ITEM lvItem;
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = iPrincipal;
        lvItem.iSubItem = 0;
        lvItem.lParam = 0;

        ListView_GetItem(hwndList, &lvItem);
        LPPRINCIPAL pPrincipal = (LPPRINCIPAL)lvItem.lParam;

        if (pPrincipal != NULL)
        {
            // Get new ACEs from the checklist window

            HDPA hAceEntries = DPA_Create(4);

            if (hAceEntries != NULL)
            {
                hwndList = GetDlgItem(hDlg, IDC_SPP_PERMS);
                UINT iCount = GetAcesFromCheckList(hwndList,
                                                   pPrincipal->GetSID(),
                                                   TRUE,
                                                   FALSE,
                                                   0,
                                                   &GUID_NULL,
                                                   hAceEntries);

                // Merge new ACEs into the principal
                while (iCount != 0)
                {
                    --iCount;
                    PACE_HEADER pAce = (PACE_HEADER)DPA_FastGetPtr(hAceEntries, iCount);
                    // Shouldn't get any inherited ACEs here
                    TraceAssert(!(pAce->AceFlags & INHERITED_ACE));
                    pPrincipal->AddAce(pAce);
                    LocalFree(pAce);
                    DPA_DeletePtr(hAceEntries, iCount);
                }

                TraceAssert(DPA_GetPtrCount(hAceEntries) == 0);
                DPA_Destroy(hAceEntries);
            }
        }
    }

    TraceLeaveVoid();
}


void
CPermPage::OnSize(HWND hDlg, DWORD dwSizeType, ULONG /*nWidth*/, ULONG /*nHeight*/)
{
    RECT rc;
    RECT rcDlg;
    LONG dx;
    LONG dy;
    HWND hwndProtect;
    HWND hwndAdvButton;
    HWND hwndPermList;
    HWND hwndPrincipalList;
    HWND hwndBottom;
    HWND hwnd;
    LONG i;

    TraceEnter(TRACE_PERMPAGE, "CPermPage::OnSize");

    if (dwSizeType != SIZE_RESTORED)
        TraceLeaveVoid();

    hwndPrincipalList = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);
    hwndPermList = GetDlgItem(hDlg, IDC_SPP_PERMS);
    hwndAdvButton = GetDlgItem(hDlg, IDC_SPP_ADVANCED);
    hwndProtect = GetDlgItem(hDlg, IDC_SPP_PROTECT);

    GetClientRect(hDlg, &rcDlg);

    GetWindowRect(hwndPrincipalList, &rc);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);   // map from screen to dlg

    InflateRect(&rcDlg, -rc.left, -rc.top);         // account for margins

    if (GetWindowLong(hwndProtect, GWL_STYLE) & WS_VISIBLE)
    {
        if (!(GetWindowLong(hwndAdvButton, GWL_STYLE) & WS_VISIBLE))
        {
            GetWindowRect(hwndAdvButton, &rc);
            dy = rc.top;    // save top coord of Advanced button

            GetWindowRect(hwndProtect, &rc);

            rc.top = dy;    // adjust y-pos of Protect box
            MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);

            SetWindowPos(hwndProtect,
                         NULL,
                         rc.left,
                         rc.top,
                         0,
                         0,
                         SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
        }

        hwndBottom = hwndProtect;
    }
    else if (GetWindowLong(hwndAdvButton, GWL_STYLE) & WS_VISIBLE)
    {
        hwndBottom = hwndAdvButton;
    }
    else
    {
        hwndBottom = hwndPermList;
    }

    GetWindowRect(hwndBottom, &rc);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);

    dy = rcDlg.bottom - rc.bottom;

    GetWindowRect(hwndPermList, &rc);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);

    dx = rcDlg.right - rc.right;

    //
    // Never make things smaller, and only make things
    // bigger if the change is worthwhile.
    //
    dx = max(dx, 0);
    if (dx < 5)
        dx = 0;
    dy = max(dy, 0);
    if (dy < 5)
        dy = 0;

    //
    // Reposition and/or resize all controls
    //
    if (dx > 0)
    {
        // Add, Remove, Reset buttons
        for (i = IDC_SPP_ADD; i <= IDC_SPP_RESET; i++)
        {
            hwnd = GetDlgItem(hDlg, i);
            GetWindowRect(hwnd, &rc);
            MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);
            SetWindowPos(hwnd,
                         NULL,
                         rc.left + dx,
                         rc.top,
                         0,
                         0,
                         SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
        }
    }

    if (dx > 0 || dy > 0)
    {
        // Listview containing User/Group names
        GetWindowRect(hwndPrincipalList, &rc);
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);
        SetWindowPos(hwndPrincipalList,
                     NULL,
                     0,
                     0,
                     rc.right - rc.left + dx,
                     rc.bottom - rc.top + dy/2,
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

        // Widen the name column if necessary
        GetClientRect(hwndPrincipalList, &rc);
        if (ListView_GetColumnWidth(hwndPrincipalList, 0) < rc.right)
            ListView_SetColumnWidth(hwndPrincipalList, 0, rc.right);
    }

    if (dy > 0)
    {
        // Static control "Access"
        hwnd = GetDlgItem(hDlg, IDC_SPP_ACCESS);
        GetWindowRect(hwnd, &rc);
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);
        SetWindowPos(hwnd,
                     NULL,
                     rc.left,
                     rc.top + dy/2,
                     0,
                     0,
                     SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    }

    if (dx > 0 || dy > 0)
    {
        // Static controls "Allow" and "Deny"
        for (i = IDC_SPP_ALLOW; i <= IDC_SPP_DENY; i++)
        {
            hwnd = GetDlgItem(hDlg, i);
            GetWindowRect(hwnd, &rc);
            MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);
            SetWindowPos(hwnd,
                         NULL,
                         rc.left + dx,
                         rc.top + dy/2,
                         0,
                         0,
                         SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
        }

        // List of permission checkboxes
        GetWindowRect(hwndPermList, &rc);
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);
        SetWindowPos(hwndPermList,
                     NULL,
                     rc.left,
                     rc.top + dy/2,
                     rc.right - rc.left + dx,
                     rc.bottom - rc.top + dy/2,
                     SWP_NOACTIVATE | SWP_NOZORDER);
    }

    if (dy > 0)
    {
        // Advanced button
        GetWindowRect(hwndAdvButton, &rc);
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);
        SetWindowPos(hwndAdvButton,
                     NULL,
                     rc.left,
                     rc.top + dy,
                     0,
                     0,
                     SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

        // "More stuff is present but not viewable" message
        hwnd = GetDlgItem(hDlg, IDC_SPP_MORE_MSG);
        GetWindowRect(hwnd, &rc);
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);
        SetWindowPos(hwnd,
                     NULL,
                     rc.left,
                     rc.top + dy,
                     0,
                     0,
                     SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

        // "Inherit from parent" checkbox
        GetWindowRect(hwndProtect, &rc);
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);
        SetWindowPos(hwndProtect,
                     NULL,
                     rc.left,
                     rc.top + dy,
                     0,
                     0,
                     SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    }

    TraceLeaveVoid();
}


void
CPermPage::ClearPermissions(HWND hwndList, BOOL bDisabled)
{
    // Uncheck everything
    UINT cRights = 0;
    DWORD dwState = CLST_UNCHECKED;

    if (bDisabled)
        dwState |= CLST_DISABLED;

    if (hwndList)
        cRights = (UINT)SendMessage(hwndList, CLM_GETITEMCOUNT, 0, 0);

    while (cRights > 0)
    {
        cRights--;
        SendMessage(hwndList, CLM_SETSTATE, MAKELONG((WORD)cRights, 1), dwState);
        SendMessage(hwndList, CLM_SETSTATE, MAKELONG((WORD)cRights, 2), dwState);
    }
}


void
CPermPage::SetDirty(HWND hDlg, BOOL bDefault)
{
    if (!bDefault)
        m_wSDControl &= ~SE_DACL_DEFAULTED;
    m_fPageDirty = TRUE;
    PropSheet_Changed(GetParent(hDlg), hDlg);
}


BOOL
CPermPage::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // First check to see if its time to update listview names
    if (uMsg == UM_SIDLOOKUPCOMPLETE)
    {
        HWND hwndList = GetDlgItem(hDlg, IDC_SPP_PRINCIPALS);
        SetPrincipalNamesInList(hwndList, (PSID)lParam);

        // lParam is zero when all remaining names are looked up
        if (0 == lParam)
        {
            // Sort using the real names
            ListView_SortItems(hwndList, NULL, 0);

            // Make sure the selected item is visible
            int iSelItem;
            if (NULL == GetSelectedItemData(hwndList, &iSelItem))
            {
                // No selection, select the first item
                SelectListViewItem(hwndList, 0);
            }
            else
            {
                ListView_EnsureVisible(hwndList, iSelItem, FALSE);
            }

            // Show normal cursor now
            m_fBusy = FALSE;
            SetCursor(LoadCursor(NULL, IDC_ARROW));

            // Enable the Advanced button if appropriate
            EnableWindow(GetDlgItem(hDlg, IDC_SPP_ADVANCED),
                (m_siObjectInfo.dwFlags & SI_ADVANCED));
        }
        return TRUE;
    }

    switch(uMsg)
    {
    case WM_SETCURSOR:
        if (m_fBusy)
        {
            SetCursor(m_hcurBusy);
            SetWindowLong(hDlg, DWLP_MSGRESULT, TRUE);
            return TRUE;
        }
        else
            return FALSE;
        break;

    case WM_INITDIALOG:
        return InitDlg(hDlg);

    case WM_NOTIFY:
        return OnNotify(hDlg, (int)wParam, (LPNMHDR)lParam);

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_SPP_ADD:
            OnAddPrincipal(hDlg);
            break;

        case IDC_SPP_REMOVE:
            OnRemovePrincipal(hDlg);
            break;

        case IDC_SPP_ADVANCED:
            OnAdvanced(hDlg);
            break;

        case IDC_SPP_RESET:
            OnReset(hDlg);
            break;

        case IDC_SPP_PRINCIPALS:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == IDN_CHECKSELECTION)
            {
                // See if we have gotten a new selection.  If not, then the
                // user must have clicked inside the listview but not on an item,
                // thus causing the listview to remove the selection.  In that
                // case, disable the other controls

                if (ListView_GetSelectedCount(GET_WM_COMMAND_HWND(wParam, lParam)) == 0)
                {
                    // Uncheck everything first
                    ClearPermissions(GetDlgItem(hDlg, IDC_SPP_PERMS));
                    EnablePrincipalControls(hDlg, FALSE);
                }
            }
            break;

        case IDC_SPP_PROTECT:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED &&
                !(m_siObjectInfo.dwFlags & SI_READONLY))
            {
                OnProtect(hDlg);
            }
            break;

        default:
            // Command not handled
            return FALSE;
        }
        break;

    case WM_SIZE:
        OnSize(hDlg, (LONG)wParam, (ULONG)LOWORD(lParam), (ULONG)HIWORD(lParam));
        break;

    case WM_HELP:
        if (IsWindowEnabled(hDlg))
        {
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    c_szAcluiHelpFile,
                    HELP_WM_HELP,
                    (DWORD_PTR)aPermPageHelpIDs);
        }
        break;

    case WM_CONTEXTMENU:
        if (IsWindowEnabled(hDlg))
        {
            HWND hwnd = (HWND)wParam;

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
            if (GetDlgCtrlID(hwnd) == IDC_SPP_PERMS)
                hwnd = GetDlgItem(hDlg, IDC_SPP_ACCESS);

            WinHelp(hwnd,
                    c_szAcluiHelpFile,
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)aPermPageHelpIDs);
        }
        break;

    default:
        // Message not handled
        return FALSE;
    }

    return TRUE;
}


//
// CSecurityInfo implementation
//
STDMETHODIMP_(ULONG)
CSecurityInfo::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CSecurityInfo::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CSecurityInfo::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ISecurityInformation))
        *ppv = static_cast<LPSECURITYINFO>(this);
    else if (IsEqualIID(riid, IID_ISecurityInformation2))
    {
        if (m_pPage->m_psi2)
            *ppv = static_cast<LPSECURITYINFO2>(this);
    }
#if(_WIN32_WINNT >= 0x0500)
    else if (IsEqualIID(riid, IID_IDsObjectPicker))
        *ppv = static_cast<IDsObjectPicker*>(this);
#endif

    if (*ppv)
    {
        m_cRef++;
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP
CSecurityInfo::GetObjectInformation(PSI_OBJECT_INFO pObjectInfo)
{
    TraceEnter(TRACE_SI, "CSecurityInfo::GetObjectInformation");
    TraceAssert(m_pPage != NULL);

    *pObjectInfo = m_pPage->m_siObjectInfo;

    TraceLeaveResult(S_OK);
}

STDMETHODIMP
CSecurityInfo::GetSecurity(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR *ppSD,
                           BOOL fDefault)
{
    HRESULT hr;

    TraceEnter(TRACE_SI, "CSecurityInfo::GetSecurity");
    TraceAssert(si != 0);
    TraceAssert(ppSD != NULL);
    TraceAssert(m_pPage != NULL);
    TraceAssert(m_hDlg != NULL);

    *ppSD = NULL;

    if (!fDefault && (si & DACL_SECURITY_INFORMATION) && m_pPage->m_fPageDirty)
    {
        // We only get asked for one thing at a time
        TraceAssert(si == DACL_SECURITY_INFORMATION);

        // Return current DACL, including inherited ACEs
        hr = m_pPage->BuildDacl(m_hDlg, ppSD, TRUE);
    }
    else
    {
        TraceAssert(m_pPage->m_psi != NULL);

        // Get it from the object
        hr = m_pPage->m_psi->GetSecurity(si, ppSD, fDefault);
    }

    TraceLeaveResult(hr);
}

STDMETHODIMP
CSecurityInfo::SetSecurity(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR pSD)
{
    HRESULT hr = E_FAIL;

    TraceEnter(TRACE_SI, "CSecurityInfo::SetSecurity");
    TraceAssert(si != 0);
    TraceAssert(pSD != NULL);
    TraceAssert(m_pPage != NULL);
    TraceAssert(m_hDlg != NULL);

    // Write out the new security descriptor
    if (m_pPage->m_psi != NULL)
        hr = m_pPage->m_psi->SetSecurity(si, pSD);

    if (S_OK == hr && (si & DACL_SECURITY_INFORMATION))
    {
        PSECURITY_DESCRIPTOR psd = NULL;

        m_pPage->m_fPageDirty = FALSE;

        // Read the new DACL back from the object, that is, don't use the one
        // from the passed-in security descriptor.  This ensures that we have
        // the "real" current DACL in case it was modified somewhere en route.
        if (SUCCEEDED(m_pPage->m_psi->GetSecurity(DACL_SECURITY_INFORMATION, &psd, FALSE)))
            pSD = psd;

        // Reinitialize the dialog using the new DACL
        m_pPage->SetDacl(m_hDlg, pSD);

        if (psd != NULL)
            LocalFree(psd);
    }

    TraceLeaveResult(hr);
}

STDMETHODIMP
CSecurityInfo::GetAccessRights(const GUID* pguidObjectType,
                               DWORD dwFlags,
                               PSI_ACCESS *ppAccess,
                               ULONG *pcAccesses,
                               ULONG *piDefaultAccess)
{
    HRESULT hr = E_FAIL;

    TraceEnter(TRACE_SI, "CSecurityInfo::GetAccessRights");
    TraceAssert(m_pPage != NULL);

    if (m_pPage->m_psi != NULL)
        hr = m_pPage->m_psi->GetAccessRights(pguidObjectType,
                                             dwFlags,
                                             ppAccess,
                                             pcAccesses,
                                             piDefaultAccess);
    TraceLeaveResult(hr);
}

STDMETHODIMP
CSecurityInfo::MapGeneric(const GUID* pguidObjectType,
                          UCHAR *pAceFlags,
                          ACCESS_MASK *pmask)
{
    HRESULT hr = E_FAIL;

    TraceEnter(TRACE_SI, "CSecurityInfo::MapGeneric");
    TraceAssert(m_pPage != NULL);

    if (m_pPage->m_psi != NULL)
        hr = m_pPage->m_psi->MapGeneric(pguidObjectType, pAceFlags, pmask);

    TraceLeaveResult(hr);
}

STDMETHODIMP
CSecurityInfo::GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
                               ULONG *pcInheritTypes)
{
    HRESULT hr = E_FAIL;

    TraceEnter(TRACE_SI, "CSecurityInfo::GetInheritTypes");
    TraceAssert(m_pPage != NULL);
    TraceAssert(ppInheritTypes != NULL);
    TraceAssert(pcInheritTypes != NULL);

    *ppInheritTypes = NULL;
    *pcInheritTypes = 0;

    if (m_pPage->m_psi != NULL)
        hr = m_pPage->m_psi->GetInheritTypes(ppInheritTypes,
                                             pcInheritTypes);
    TraceLeaveResult(hr);
}

STDMETHODIMP
CSecurityInfo::PropertySheetPageCallback(HWND hwnd, UINT uMsg, SI_PAGE_TYPE uPage)
{
    HRESULT hr = S_OK;

    TraceEnter(TRACE_SI, "CSecurityInfo::PropertySheetPageCallback");
    TraceAssert(m_pPage != NULL);

    //
    // Pass the call on to the client
    //
    if (m_pPage->m_psi != NULL)
        hr = m_pPage->m_psi->PropertySheetPageCallback(hwnd, uMsg, uPage);

    //
    // If the simple perm page is disabled, make sure the advanced perm
    // page is as well.
    //
    if (SUCCEEDED(hr) && uPage == SI_PAGE_ADVPERM && m_pPage->m_bAbortPage)
        hr = E_FAIL;

    TraceLeaveResult(hr);
}


//
// ISecurityInformation2 methods
//
STDMETHODIMP_(BOOL)
CSecurityInfo::IsDaclCanonical(PACL pDacl)
{
    BOOL bResult = TRUE;

    TraceEnter(TRACE_SI, "CSecurityInfo::IsDaclCanonical");
    TraceAssert(m_pPage != NULL);

    if (m_pPage->m_psi2 != NULL)
        bResult = m_pPage->m_psi2->IsDaclCanonical(pDacl);

    TraceLeaveValue(bResult);
}

STDMETHODIMP
CSecurityInfo::LookupSids(ULONG cSids, PSID *rgpSids, LPDATAOBJECT *ppdo)
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter(TRACE_SI, "CSecurityInfo::LookupSids");
    TraceAssert(m_pPage != NULL);

    if (m_pPage->m_psi2 != NULL)
        hr = m_pPage->m_psi2->LookupSids(cSids, rgpSids, ppdo);

    TraceLeaveResult(hr);
}


//
// IDsObjectPicker methods
//
#if(_WIN32_WINNT >= 0x0500)
STDMETHODIMP CSecurityInfo::Initialize(PDSOP_INIT_INFO pInitInfo)
{
    HRESULT hr;
    IDsObjectPicker *pObjectPicker = NULL;

    hr = m_pPage->GetObjectPicker(&pObjectPicker);

    if (SUCCEEDED(hr))
    {
        if (m_pPage->m_flLastOPOptions != pInitInfo->flOptions)
        {
            m_pPage->m_flLastOPOptions = (DWORD)-1;

            hr = pObjectPicker->Initialize(pInitInfo);

            if (SUCCEEDED(hr))
            {
                m_pPage->m_flLastOPOptions = pInitInfo->flOptions;
            }
        }
        pObjectPicker->Release();
    }

    return hr;
}

STDMETHODIMP CSecurityInfo::InvokeDialog(HWND hwndParent,
                                         IDataObject **ppdoSelection)
{
    HRESULT hr;
    IDsObjectPicker *pObjectPicker = NULL;

    hr = m_pPage->GetObjectPicker(&pObjectPicker);

    if (SUCCEEDED(hr))
    {
        hr = pObjectPicker->InvokeDialog(hwndParent, ppdoSelection);
        pObjectPicker->Release();
    }

    return hr;
}
#endif  // _WIN32_WINNT >= 0x0500

//
// Expose an api to get at the simple permission editor
//

HPROPSHEETPAGE
ACLUIAPI
CreateSecurityPage(LPSECURITYINFO psi)
{
    HPROPSHEETPAGE hPage = NULL;
    PPERMPAGE pPage;
    PSIDCACHE pSidCache;

    TraceEnter(TRACE_PERMPAGE, "CreateSecurityPage");

    // Create the global SID Cache
    pSidCache = GetSidCache();

    if (NULL == psi)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        TraceLeaveValue(NULL);
    }

    pPage = new CPermPage(psi);

    if (pPage)
    {
        SI_OBJECT_INFO siObjectInfo = {0};
        LPCTSTR pszTitle = NULL;

        if (SUCCEEDED(psi->GetObjectInformation(&siObjectInfo)) &&
            (siObjectInfo.dwFlags & SI_PAGE_TITLE))
        {
            pszTitle = siObjectInfo.pszPageTitle;
        }

        hPage = pPage->CreatePropSheetPage(MAKEINTRESOURCE(IDD_SIMPLE_PERM_PAGE), pszTitle);

        if (!hPage)
            delete pPage;
    }

    if (pSidCache)
        pSidCache->Release();

    TraceLeaveValue(hPage);
}

BOOL
ACLUIAPI
EditSecurity( HWND hwndOwner, LPSECURITYINFO psi )
{
    HPROPSHEETPAGE hPage[1];
    UINT cPages = 0;
    BOOL bResult = FALSE;
    SI_OBJECT_INFO siObjectInfo = {0};
    HRESULT hr;

    TraceEnter(TRACE_PERMPAGE, "EditSecurity");

    // Get object name for dialog title
    hr = psi->GetObjectInformation(&siObjectInfo);

    if (FAILED(hr))
    {
        if (!GetLastError())
            SetLastError(hr);

        TraceLeaveValue(FALSE);
    }

    hPage[cPages] = CreateSecurityPage( psi );
    if (hPage[cPages])
        cPages++;

    if (cPages)
    {
        // Build dialog title string
        LPTSTR pszCaption = NULL;

        PROPSHEETHEADER psh;
        psh.dwSize = SIZEOF(psh);
        psh.dwFlags = PSH_DEFAULT;
        psh.hwndParent = hwndOwner;
        psh.hInstance = ::hModule;
        psh.nPages = cPages;
        psh.nStartPage = 0;
        psh.phpage = &hPage[0];

// There has been a request for customization of this dialog title,
// but this probably isn't the best way to do it, since the dlg title
// and page title will be the same.
#if 0
        if ((siObjectInfo.dwFlags & SI_PAGE_TITLE)
            && siObjectInfo.pszPageTitle
            && siObjectInfo.pszPageTitle[0])
        {
            psh.pszCaption = siObjectInfo.pszPageTitle;
        }
        else
#endif
        {
            FormatStringID(&pszCaption, ::hModule, IDS_SPP_TITLE, siObjectInfo.pszObjectName);
            psh.pszCaption = pszCaption;
        }

        bResult = (BOOL)(PropertySheet(&psh) + 1);

        LocalFreeString(&pszCaption);
    }

    TraceLeaveValue(bResult);
}
