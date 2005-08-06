/*
 * ReactOS Access Control List Editor
 * Copyright (C) 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id$
 *
 * PROJECT:         ReactOS Access Control List Editor
 * FILE:            lib/aclui/aclui.c
 * PURPOSE:         Access Control List Editor
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *      08/10/2004  Created
 */
#include <precomp.h>

HINSTANCE hDllInstance;

static PCWSTR ObjectPickerAttributes[] =
{
    L"ObjectSid",
};

static VOID
DestroySecurityPage(IN PSECURITY_PAGE sp)
{
    if(sp->hiPrincipals != NULL)
    {
        ImageList_Destroy(sp->hiPrincipals);
    }

    HeapFree(GetProcessHeap(),
             0,
             sp);

    CoUninitialize();
}

static VOID
FreePrincipalsList(IN PPRINCIPAL_LISTITEM *PrincipalsListHead)
{
    PPRINCIPAL_LISTITEM CurItem, NextItem;
    
    CurItem = *PrincipalsListHead;
    while (CurItem != NULL)
    {
        /* free the SID string if present */
        if (CurItem->DisplayString != NULL)
        {
            LocalFree((HLOCAL)CurItem->DisplayString);
        }

        /* free the ACE list item */
        NextItem = CurItem->Next;
        HeapFree(GetProcessHeap(),
                 0,
                 CurItem);
        CurItem = NextItem;
    }
    
    *PrincipalsListHead = NULL;
}

static PPRINCIPAL_LISTITEM
FindSidInPrincipalsList(IN PPRINCIPAL_LISTITEM PrincipalsListHead,
                        IN PSID Sid)
{
    PPRINCIPAL_LISTITEM CurItem;
    
    for (CurItem = PrincipalsListHead;
         CurItem != NULL;
         CurItem = CurItem->Next)
    {
        if (EqualSid((PSID)(CurItem + 1),
                     Sid))
        {
            return CurItem;
        }
    }
    
    return NULL;
}

static VOID
ReloadPrincipalsList(IN PSECURITY_PAGE sp)
{
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    BOOL DaclPresent, DaclDefaulted;
    PACL Dacl = NULL;
    HRESULT hRet;

    /* delete the cached ACL */
    FreePrincipalsList(&sp->PrincipalsListHead);

    /* query the ACL */
    hRet = sp->psi->lpVtbl->GetSecurity(sp->psi,
                                        DACL_SECURITY_INFORMATION,
                                        &SecurityDescriptor,
                                        FALSE);
    if (SUCCEEDED(hRet) && SecurityDescriptor != NULL)
    {
        if (GetSecurityDescriptorDacl(SecurityDescriptor,
                                      &DaclPresent,
                                      &Dacl,
                                      &DaclDefaulted))
        {
            PPRINCIPAL_LISTITEM AceListItem, *NextAcePtr;
            PSID Sid;
            PVOID Ace;
            ULONG AceIndex;
            DWORD AccountNameSize, DomainNameSize, SidLength;
            SID_NAME_USE SidNameUse;
            DWORD LookupResult;
            
            NextAcePtr = &sp->PrincipalsListHead;
            
            for (AceIndex = 0;
                 AceIndex < Dacl->AceCount;
                 AceIndex++)
            {
                GetAce(Dacl,
                       AceIndex,
                       &Ace);

                Sid = (PSID)&((PACCESS_ALLOWED_ACE)Ace)->SidStart;

                if (!FindSidInPrincipalsList(sp->PrincipalsListHead,
                                             Sid))
                {
                    SidLength = GetLengthSid(Sid);

                    AccountNameSize = 0;
                    DomainNameSize = 0;

                    /* calculate the size of the buffer we need to calculate */
                    LookupAccountSid(sp->ServerName,
                                     Sid,
                                     NULL,
                                     &AccountNameSize,
                                     NULL,
                                     &DomainNameSize,
                                     &SidNameUse);

                    /* allocate the ace */
                    AceListItem = HeapAlloc(GetProcessHeap(),
                                            0,
                                            sizeof(PRINCIPAL_LISTITEM) +
                                            SidLength +
                                            ((AccountNameSize + DomainNameSize) * sizeof(WCHAR)));
                    if (AceListItem != NULL)
                    {
                        AceListItem->AccountName = (LPWSTR)((ULONG_PTR)(AceListItem + 1) + SidLength);
                        AceListItem->DomainName = AceListItem->AccountName + AccountNameSize;

                        CopySid(SidLength,
                                (PSID)(AceListItem + 1),
                                Sid);

                        LookupResult = ERROR_SUCCESS;
                        if (!LookupAccountSid(sp->ServerName,
                                              Sid,
                                              AceListItem->AccountName,
                                              &AccountNameSize,
                                              AceListItem->DomainName,
                                              &DomainNameSize,
                                              &SidNameUse))
                        {
                            LookupResult = GetLastError();
                            if (LookupResult != ERROR_NONE_MAPPED)
                            {
                                HeapFree(GetProcessHeap(),
                                         0,
                                         AceListItem);
                                continue;
                            }
                        }

                        if (AccountNameSize == 0)
                        {
                            AceListItem->AccountName = NULL;
                        }
                        if (DomainNameSize == 0)
                        {
                            AceListItem->DomainName = NULL;
                        }

                        AceListItem->Next = NULL;
                        if (LookupResult == ERROR_NONE_MAPPED)
                        {
                            if (!ConvertSidToStringSid(Sid,
                                                       &AceListItem->DisplayString))
                            {
                                AceListItem->DisplayString = NULL;
                            }
                        }
                        else
                        {
                            LSA_HANDLE LsaHandle;
                            NTSTATUS Status;

                            AceListItem->DisplayString = NULL;

                            /* read the domain of the SID */
                            if (OpenLSAPolicyHandle(sp->ServerName,
                                                    POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,
                                                    &LsaHandle))
                            {
                                PLSA_REFERENCED_DOMAIN_LIST ReferencedDomain;
                                PLSA_TRANSLATED_NAME Names;
                                PLSA_TRUST_INFORMATION Domain;
                                PLSA_UNICODE_STRING DomainName;
                                PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo = NULL;

                                Status = LsaLookupSids(LsaHandle,
                                                       1,
                                                       &Sid,
                                                       &ReferencedDomain,
                                                       &Names);
                                if (NT_SUCCESS(Status))
                                {
                                    if (ReferencedDomain != NULL &&
                                        Names->DomainIndex >= 0)
                                    {
                                        Domain = &ReferencedDomain->Domains[Names->DomainIndex];
                                        DomainName = &Domain->Name;
                                    }
                                    else
                                    {
                                        Domain = NULL;
                                        DomainName = NULL;
                                    }

                                    AceListItem->SidNameUse = Names->Use;

                                    switch (Names->Use)
                                    {
                                        case SidTypeAlias:
                                            if (Domain != NULL)
                                            {
                                                /* query the domain name for BUILTIN accounts */
                                                Status = LsaQueryInformationPolicy(LsaHandle,
                                                                                   PolicyAccountDomainInformation,
                                                                                   (PVOID*)&PolicyAccountDomainInfo);
                                                if (NT_SUCCESS(Status))
                                                {
                                                    DomainName = &PolicyAccountDomainInfo->DomainName;

                                                    /* make the user believe this is a group */
                                                    AceListItem->SidNameUse = SidTypeGroup;
                                                }
                                            }
                                            /* fall through */

                                        case SidTypeUser:
                                        {
                                            if (Domain != NULL)
                                            {
                                                AceListItem->DisplayString = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                                                                                (AccountNameSize * sizeof(WCHAR)) +
                                                                                                (DomainName->Length + sizeof(WCHAR)) +
                                                                                                (Names->Name.Length + sizeof(WCHAR)) +
                                                                                                (4 * sizeof(WCHAR)));
                                                if (AceListItem->DisplayString != NULL)
                                                {
                                                    WCHAR *s;

                                                    /* NOTE: LSA_UNICODE_STRINGs are not always NULL-terminated! */

                                                    wcscpy(AceListItem->DisplayString,
                                                           AceListItem->AccountName);
                                                    wcscat(AceListItem->DisplayString,
                                                           L" (");
                                                    s = AceListItem->DisplayString + wcslen(AceListItem->DisplayString);
                                                    CopyMemory(s,
                                                               DomainName->Buffer,
                                                               DomainName->Length);
                                                    s += DomainName->Length / sizeof(WCHAR);
                                                    *(s++) = L'\\';
                                                    CopyMemory(s,
                                                               Names->Name.Buffer,
                                                               Names->Name.Length);
                                                    s += Names->Name.Length / sizeof(WCHAR);
                                                    *(s++) = L')';
                                                    *s = L'\0';
                                                }

                                                /* mark the ace as a user unless it's a
                                                   BUILTIN account */
                                                if (PolicyAccountDomainInfo == NULL)
                                                {
                                                    AceListItem->SidNameUse = SidTypeUser;
                                                }
                                            }
                                            break;
                                        }

                                        case SidTypeWellKnownGroup:
                                        {
                                            /* make the user believe this is a group */
                                            AceListItem->SidNameUse = SidTypeGroup;
                                            break;
                                        }

                                        default:
                                        {
                                            DPRINT("Unhandled SID type: 0x%x\n", Names->Use);
                                            break;
                                        }
                                    }

                                    if (PolicyAccountDomainInfo != NULL)
                                    {
                                        LsaFreeMemory(PolicyAccountDomainInfo);
                                    }

                                    LsaFreeMemory(ReferencedDomain);
                                    LsaFreeMemory(Names);
                                }
                                LsaClose(LsaHandle);
                            }
                        }

                        /* append item to the cached ACL */
                        *NextAcePtr = AceListItem;
                        NextAcePtr = &AceListItem->Next;
                    }
                }
            }
        }
        LocalFree((HLOCAL)SecurityDescriptor);
    }
}

static INT
AddPrincipalListEntry(IN PSECURITY_PAGE sp,
                      IN PPRINCIPAL_LISTITEM PrincipalListItem,
                      IN INT Index,
                      IN BOOL Selected)
{
    LVITEM li;

    li.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE | LVIF_TEXT;
    li.iItem = Index;
    li.iSubItem = 0;
    li.state = (Selected ? LVIS_SELECTED : 0);
    li.stateMask = LVIS_SELECTED;
    li.pszText = (PrincipalListItem->DisplayString != NULL ? PrincipalListItem->DisplayString : PrincipalListItem->AccountName);
    switch (PrincipalListItem->SidNameUse)
    {
        case SidTypeUser:
            li.iImage = 0;
            break;
        case SidTypeGroup:
            li.iImage = 1;
            break;
        default:
            li.iImage = -1;
            break;
    }
    li.lParam = (LPARAM)PrincipalListItem;

    return ListView_InsertItem(sp->hWndPrincipalsList,
                               &li);
}

static VOID
FillPrincipalsList(IN PSECURITY_PAGE sp)
{
    LPARAM SelLParam;
    PPRINCIPAL_LISTITEM CurItem;
    RECT rcLvClient;

    SelLParam = ListViewGetSelectedItemData(sp->hWndPrincipalsList);

    DisableRedrawWindow(sp->hWndPrincipalsList);

    ListView_DeleteAllItems(sp->hWndPrincipalsList);

    ReloadPrincipalsList(sp);

    for (CurItem = sp->PrincipalsListHead;
         CurItem != NULL;
         CurItem = CurItem->Next)
    {
        AddPrincipalListEntry(sp,
                              CurItem,
                              -1,
                              (SelLParam == (LPARAM)CurItem));
    }
    
    EnableRedrawWindow(sp->hWndPrincipalsList);
    
    GetClientRect(sp->hWndPrincipalsList, &rcLvClient);
    
    ListView_SetColumnWidth(sp->hWndPrincipalsList,
                            0,
                            rcLvClient.right);
}

static VOID
UpdateControlStates(IN PSECURITY_PAGE sp)
{
    PPRINCIPAL_LISTITEM Selected = (PPRINCIPAL_LISTITEM)ListViewGetSelectedItemData(sp->hWndPrincipalsList);

    EnableWindow(sp->hBtnRemove, Selected != NULL);
    EnableWindow(sp->hAceCheckList, Selected != NULL);
    
    if (Selected != NULL)
    {
        LPWSTR szLabel;

        if (LoadAndFormatString(hDllInstance,
                                IDS_PERMISSIONS_FOR,
                                &szLabel,
                                Selected->AccountName))
        {
            SetWindowText(sp->hPermissionsForLabel,
                          szLabel);

            LocalFree((HLOCAL)szLabel);
        }
        
        /* FIXME - update the checkboxes */
    }
    else
    {
        WCHAR szPermissions[255];
        
        if (LoadString(hDllInstance,
                       IDS_PERMISSIONS,
                       szPermissions,
                       sizeof(szPermissions) / sizeof(szPermissions[0])))
        {
            SetWindowText(sp->hPermissionsForLabel,
                          szPermissions);
        }

        SendMessage(sp->hAceCheckList,
                    CLM_CLEARCHECKBOXES,
                    0,
                    0);
    }
}

static UINT CALLBACK
SecurityPageCallback(IN HWND hwnd,
                     IN UINT uMsg,
                     IN LPPROPSHEETPAGE ppsp)
{
    PSECURITY_PAGE sp = (PSECURITY_PAGE)ppsp->lParam;
    
    switch (uMsg)
    {
        case PSPCB_CREATE:
        {
            return TRUE;
        }
        case PSPCB_RELEASE:
        {
            DestroySecurityPage(sp);
            UnregisterCheckListControl();
            return FALSE;
        }
    }

    return FALSE;
}

static VOID
SetAceCheckListColumns(IN HWND hAceCheckList,
                       IN UINT Button,
                       IN HWND hLabel)
{
    POINT pt;
    RECT rcLabel;

    GetWindowRect(hLabel,
                  &rcLabel);
    pt.y = 0;
    pt.x = (rcLabel.right - rcLabel.left) / 2;
    MapWindowPoints(hLabel,
                    hAceCheckList,
                    &pt,
                    1);

    SendMessage(hAceCheckList,
                CLM_SETCHECKBOXCOLUMN,
                Button,
                pt.x);
}

static VOID
LoadPermissionsList(IN PSECURITY_PAGE sp,
                    IN GUID *GuidObjectType,
                    IN DWORD dwFlags,
                    OUT SI_ACCESS *DefaultAccess)
{
    HRESULT hRet;
    PSI_ACCESS AccessList;
    ULONG nAccessList, DefaultAccessIndex;
    
    /* clear the permissions list */
    
    SendMessage(sp->hAceCheckList,
                CLM_CLEAR,
                0,
                0);
    
    /* query the access rights from the server */
    hRet = sp->psi->lpVtbl->GetAccessRights(sp->psi,
                                            GuidObjectType,
                                            dwFlags,
                                            &AccessList,
                                            &nAccessList,
                                            &DefaultAccessIndex);
    if (SUCCEEDED(hRet) && nAccessList != 0)
    {
        LPCWSTR NameStr;
        PSI_ACCESS CurAccess, LastAccess;
        WCHAR NameBuffer[MAX_PATH];
        
        /* save the default access rights to be used when adding ACEs later */
        if (DefaultAccess != NULL)
        {
            *DefaultAccess = AccessList[DefaultAccessIndex];
        }
        
        LastAccess = AccessList + nAccessList;
        for (CurAccess = &AccessList[0];
             CurAccess != LastAccess;
             CurAccess++)
        {
            if (CurAccess->dwFlags & dwFlags)
            {
                /* get the permission name, load it from a string table if necessary */
                if (IS_INTRESOURCE(CurAccess->pszName))
                {
                    if (!LoadString(sp->ObjectInfo.hInstance,
                                    (UINT)((ULONG_PTR)CurAccess->pszName),
                                    NameBuffer,
                                    sizeof(NameBuffer) / sizeof(NameBuffer[0])))
                    {
                        LoadString(hDllInstance,
                                   IDS_UNKNOWN,
                                   NameBuffer,
                                   sizeof(NameBuffer) / sizeof(NameBuffer[0]));
                    }
                    NameStr = NameBuffer;
                }
                else
                {
                    NameStr = CurAccess->pszName;
                }

                SendMessage(sp->hAceCheckList,
                            CLM_ADDITEM,
                            CIS_NONE,
                            (LPARAM)NameStr);
            }
        }
    }
}

static VOID
ResizeControls(IN PSECURITY_PAGE sp,
               IN INT Width,
               IN INT Height)
{
    HWND hWndAllow, hWndDeny;
    RECT rcControl, rcControl2, rcControl3, rcWnd;
    INT cxWidth, cxEdge, btnSpacing;
    POINT pt, pt2;
    HDWP dwp;
    INT nControls = 7;
    LVCOLUMN lvc;
    
    hWndAllow = GetDlgItem(sp->hWnd,
                           IDC_LABEL_ALLOW);
    hWndDeny = GetDlgItem(sp->hWnd,
                          IDC_LABEL_DENY);
    
    GetWindowRect(sp->hWnd,
                  &rcWnd);

    cxEdge = GetSystemMetrics(SM_CXEDGE);
    
    /* use the left margin of the principal list view control for all control
       margins */
    pt.x = 0;
    pt.y = 0;
    MapWindowPoints(sp->hWndPrincipalsList,
                    sp->hWnd,
                    &pt,
                    1);
    cxWidth = Width - (2 * pt.x);
    
    if (sp->ObjectInfo.dwFlags & SI_ADVANCED)
    {
        nControls += 2;
    }
    
    if (!(dwp = BeginDeferWindowPos(nControls)))
    {
        return;
    }
    
    /* resize the Principal list view */
    GetWindowRect(sp->hWndPrincipalsList,
                  &rcControl);
    if (!(dwp = DeferWindowPos(dwp,
                               sp->hWndPrincipalsList,
                               NULL,
                               0,
                               0,
                               cxWidth,
                               rcControl.bottom - rcControl.top,
                               SWP_NOMOVE | SWP_NOZORDER)))
    {
        return;
    }
    
    /* move the Add Principal button */
    GetWindowRect(sp->hBtnAdd,
                  &rcControl);
    GetWindowRect(sp->hBtnRemove,
                  &rcControl2);
    btnSpacing = rcControl2.left - rcControl.right;
    pt2.x = 0;
    pt2.y = 0;
    MapWindowPoints(sp->hBtnAdd,
                    sp->hWnd,
                    &pt2,
                    1);
    if (!(dwp = DeferWindowPos(dwp,
                               sp->hBtnAdd,
                               NULL,
                               pt.x + cxWidth - (rcControl2.right - rcControl2.left) -
                                   (rcControl.right - rcControl.left) -
                                   btnSpacing - cxEdge,
                               pt2.y,
                               0,
                               0,
                               SWP_NOSIZE | SWP_NOZORDER)))
    {
        return;
    }
    
    /* move the Delete Principal button */
    pt2.x = 0;
    pt2.y = 0;
    MapWindowPoints(sp->hBtnRemove,
                    sp->hWnd,
                    &pt2,
                    1);
    if (!(dwp = DeferWindowPos(dwp,
                               sp->hBtnRemove,
                               NULL,
                               pt.x + cxWidth - (rcControl2.right - rcControl2.left) - cxEdge,
                               pt2.y,
                               0,
                               0,
                               SWP_NOSIZE | SWP_NOZORDER)))
    {
        return;
    }
    
    /* move the Permissions For label */
    GetWindowRect(hWndAllow,
                  &rcControl);
    GetWindowRect(hWndDeny,
                  &rcControl2);
    GetWindowRect(sp->hPermissionsForLabel,
                  &rcControl3);
    pt2.x = 0;
    pt2.y = 0;
    MapWindowPoints(sp->hPermissionsForLabel,
                    sp->hWnd,
                    &pt2,
                    1);
    if (!(dwp = DeferWindowPos(dwp,
                               sp->hPermissionsForLabel,
                               NULL,
                               0,
                               0,
                               cxWidth - (rcControl2.right - rcControl2.left) -
                                   (rcControl.right - rcControl.left) -
                                   (2 * btnSpacing) - cxEdge,
                               rcControl3.bottom - rcControl3.top,
                               SWP_NOMOVE | SWP_NOZORDER)))
    {
        return;
    }
    
    /* move the Allow label */
    pt2.x = 0;
    pt2.y = 0;
    MapWindowPoints(hWndAllow,
                    sp->hWnd,
                    &pt2,
                    1);
    if (!(dwp = DeferWindowPos(dwp,
                               hWndAllow,
                               NULL,
                               cxWidth - (rcControl2.right - rcControl2.left) -
                                   (rcControl.right - rcControl.left) -
                                   btnSpacing - cxEdge,
                               pt2.y,
                               0,
                               0,
                               SWP_NOSIZE | SWP_NOZORDER)))
    {
        return;
    }
    
    /* move the Deny label */
    pt2.x = 0;
    pt2.y = 0;
    MapWindowPoints(hWndDeny,
                    sp->hWnd,
                    &pt2,
                    1);
    if (!(dwp = DeferWindowPos(dwp,
                               hWndDeny,
                               NULL,
                               cxWidth - (rcControl2.right - rcControl2.left) - cxEdge,
                               pt2.y,
                               0,
                               0,
                               SWP_NOSIZE | SWP_NOZORDER)))
    {
        return;
    }
    
    /* resize the Permissions check list box */
    GetWindowRect(sp->hAceCheckList,
                  &rcControl);
    GetWindowRect(sp->hBtnAdvanced,
                  &rcControl2);
    GetWindowRect(GetDlgItem(sp->hWnd,
                             IDC_LABEL_ADVANCED),
                  &rcControl3);
    if (!(dwp = DeferWindowPos(dwp,
                               sp->hAceCheckList,
                               NULL,
                               0,
                               0,
                               cxWidth,
                               ((sp->ObjectInfo.dwFlags & SI_ADVANCED) ?
                                   Height - (rcControl.top - rcWnd.top) - (rcControl3.bottom - rcControl3.top) - pt.x - btnSpacing :
                                   Height - (rcControl.top - rcWnd.top) - pt.x),
                               SWP_NOMOVE | SWP_NOZORDER)))
    {
        return;
    }
    
    if (sp->ObjectInfo.dwFlags & SI_ADVANCED)
    {
        /* move and resize the Advanced label */
        if (!(dwp = DeferWindowPos(dwp,
                                   GetDlgItem(sp->hWnd,
                                              IDC_LABEL_ADVANCED),
                                   NULL,
                                   pt.x,
                                   Height - (rcControl3.bottom - rcControl3.top) - pt.x,
                                   cxWidth - (rcControl2.right - rcControl2.left) - cxEdge,
                                   rcControl3.bottom - rcControl3.top,
                                   SWP_NOZORDER)))
        {
            return;
        }
        
        /* move and resize the Advanced button */
        if (!(dwp = DeferWindowPos(dwp,
                                   sp->hBtnAdvanced,
                                   NULL,
                                   cxWidth - (rcControl2.right - rcControl2.left) + pt.x,
                                   Height - (rcControl2.bottom - rcControl2.top) - pt.x,
                                   0,
                                   0,
                                   SWP_NOSIZE | SWP_NOZORDER)))
        {
            return;
        }
    }
    
    EndDeferWindowPos(dwp);
    
    /* update the width of the principal list view column */
    GetClientRect(sp->hWndPrincipalsList,
                  &rcControl);
    lvc.mask = LVCF_WIDTH;
    lvc.cx = rcControl.right;
    ListView_SetColumn(sp->hWndPrincipalsList,
                       0,
                       &lvc);

    /* calculate the columns of the allow/deny checkboxes */
    SetAceCheckListColumns(sp->hAceCheckList,
                           CLB_ALLOW,
                           hWndAllow);
    SetAceCheckListColumns(sp->hAceCheckList,
                           CLB_DENY,
                           hWndDeny);
}

static INT_PTR CALLBACK
SecurityPageProc(IN HWND hwndDlg,
                 IN UINT uMsg,
                 IN WPARAM wParam,
                 IN LPARAM lParam)
{
    PSECURITY_PAGE sp;

    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            NMHDR *pnmh = (NMHDR*)lParam;
            sp = (PSECURITY_PAGE)GetWindowLongPtr(hwndDlg,
                                                  DWL_USER);
            if (sp != NULL)
            {
                if (pnmh->hwndFrom == sp->hWndPrincipalsList)
                {
                    switch (pnmh->code)
                    {
                        case LVN_ITEMCHANGED:
                        {
                            LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
                            
                            if ((pnmv->uChanged & LVIF_STATE) &&
                                ((pnmv->uOldState & (LVIS_FOCUSED | LVIS_SELECTED)) ||
                                 (pnmv->uNewState & (LVIS_FOCUSED | LVIS_SELECTED))))
                            {
                                UpdateControlStates(sp);
                            }
                            break;
                        }
                    }
                }
                else if (pnmh->hwndFrom == sp->hAceCheckList)
                {
                    switch (pnmh->code)
                    {
                        case CLN_CHANGINGITEMCHECKBOX:
                        {
                            PNMCHANGEITEMCHECKBOX pcicb = (PNMCHANGEITEMCHECKBOX)lParam;
                            
                            /* make sure only one of both checkboxes is only checked
                               at the same time */
                            if (pcicb->Checked)
                            {
                                pcicb->NewState &= ~((pcicb->CheckBox != CLB_DENY) ? CIS_DENY : CIS_ALLOW);
                            }
                            break;
                        }
                    }
                }
            }
            break;
        }
        
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_ADD_PRINCIPAL:
                {
                    HRESULT hRet;
                    IDsObjectPicker *pDsObjectPicker = NULL;
                    IDataObject *Selections = NULL;
                    
                    sp = (PSECURITY_PAGE)GetWindowLongPtr(hwndDlg,
                                                          DWL_USER);
                    
                    hRet = InitializeObjectPicker(sp->ServerName,
                                                  &sp->ObjectInfo,
                                                  ObjectPickerAttributes,
                                                  &pDsObjectPicker);
                    if (SUCCEEDED(hRet))
                    {
                        hRet = pDsObjectPicker->lpVtbl->InvokeDialog(pDsObjectPicker,
                                                                     hwndDlg,
                                                                     &Selections);
                        if (FAILED(hRet))
                        {
                            MessageBox(hwndDlg, L"InvokeDialog failed!\n", NULL, 0);
                        }
                        
                        /* delete the instance */
                        pDsObjectPicker->lpVtbl->Release(pDsObjectPicker);
                    }
                    break;
                }
            }
            break;
        }
        
        case WM_SIZE:
        {
            sp = (PSECURITY_PAGE)GetWindowLongPtr(hwndDlg,
                                                  DWL_USER);

            ResizeControls(sp,
                           (INT)LOWORD(lParam),
                           (INT)HIWORD(lParam));
            break;
        }
        
        case WM_INITDIALOG:
        {
            sp = (PSECURITY_PAGE)((LPPROPSHEETPAGE)lParam)->lParam;
            if(sp != NULL)
            {
                LV_COLUMN lvc;
                RECT rcLvClient;
                
                sp->hWnd = hwndDlg;
                sp->hWndPrincipalsList = GetDlgItem(hwndDlg, IDC_PRINCIPALS);
                sp->hBtnAdd = GetDlgItem(hwndDlg, IDC_ADD_PRINCIPAL);
                sp->hBtnRemove = GetDlgItem(hwndDlg, IDC_REMOVE_PRINCIPAL);
                sp->hBtnAdvanced = GetDlgItem(hwndDlg, IDC_ADVANCED);
                sp->hAceCheckList = GetDlgItem(hwndDlg, IDC_ACE_CHECKLIST);
                sp->hPermissionsForLabel = GetDlgItem(hwndDlg, IDC_LABEL_PERMISSIONS_FOR);
                
                sp->SpecialPermCheckIndex = -1;
                
                if ((sp->ObjectInfo.dwFlags & SI_SERVER_IS_DC) &&
                    sp->ObjectInfo.pszServerName != NULL &&
                    sp->ObjectInfo.pszServerName[0] != L'\0')
                {
                    sp->ServerName = sp->ObjectInfo.pszServerName;
                }

                /* save the pointer to the structure */
                SetWindowLongPtr(hwndDlg,
                                 DWL_USER,
                                 (DWORD_PTR)sp);

                sp->hiPrincipals = ImageList_LoadBitmap(hDllInstance,
                                                  MAKEINTRESOURCE(IDB_USRGRPIMAGES),
                                                  16,
                                                  3,
                                                  0);

                /* setup the listview control */
                ListView_SetExtendedListViewStyleEx(sp->hWndPrincipalsList,
                                                    LVS_EX_FULLROWSELECT,
                                                    LVS_EX_FULLROWSELECT);
                ListView_SetImageList(sp->hWndPrincipalsList,
                                      sp->hiPrincipals,
                                      LVSIL_SMALL);

                GetClientRect(sp->hWndPrincipalsList, &rcLvClient);
                
                /* add a column to the list view */
                lvc.mask = LVCF_FMT | LVCF_WIDTH;
                lvc.fmt = LVCFMT_LEFT;
                lvc.cx = rcLvClient.right;
                ListView_InsertColumn(sp->hWndPrincipalsList, 0, &lvc);

                FillPrincipalsList(sp);
                
                ListViewSelectItem(sp->hWndPrincipalsList,
                                   0);

                /* calculate the columns of the allow/deny checkboxes */
                SetAceCheckListColumns(sp->hAceCheckList,
                                       CLB_ALLOW,
                                       GetDlgItem(hwndDlg, IDC_LABEL_ALLOW));
                SetAceCheckListColumns(sp->hAceCheckList,
                                       CLB_DENY,
                                       GetDlgItem(hwndDlg, IDC_LABEL_DENY));

                LoadPermissionsList(sp,
                                    NULL,
                                    SI_ACCESS_GENERAL |
                                    ((sp->ObjectInfo.dwFlags & SI_CONTAINER) ? SI_ACCESS_CONTAINER : 0),
                                    &sp->DefaultAccess);

                /* hide controls in case the flags aren't present */
                if (sp->ObjectInfo.dwFlags & SI_ADVANCED)
                {
                    WCHAR szSpecialPermissions[255];
                    
                    /* editing the permissions is least the user can do when
                       the advanced button is showed */
                    sp->ObjectInfo.dwFlags |= SI_EDIT_PERMS;
                    
                    if (LoadString(hDllInstance,
                                   IDS_SPECIAL_PERMISSIONS,
                                   szSpecialPermissions,
                                   sizeof(szSpecialPermissions) / sizeof(szSpecialPermissions[0])))
                    {
                        /* add the special permissions check item */
                        sp->SpecialPermCheckIndex = (INT)SendMessage(sp->hAceCheckList,
                                                                     CLM_ADDITEM,
                                                                     CIS_ALLOWDISABLED | CIS_DENYDISABLED | CIS_NONE,
                                                                     (LPARAM)szSpecialPermissions);
                    }
                }
                else
                {
                    ShowWindow(sp->hBtnAdvanced,
                               SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_LABEL_ADVANCED),
                               SW_HIDE);
                }
                
                /* enable quicksearch for the permissions checklist control */
                SendMessage(sp->hAceCheckList,
                            CLM_ENABLEQUICKSEARCH,
                            TRUE,
                            0);

                UpdateControlStates(sp);
            }
            break;
        }
    }
    return 0;
}


/*
 * CreateSecurityPage							EXPORTED
 *
 * @implemented
 */
HPROPSHEETPAGE
WINAPI
CreateSecurityPage(IN LPSECURITYINFO psi)
{
    PROPSHEETPAGE psp;
    PSECURITY_PAGE sPage;
    SI_OBJECT_INFO ObjectInfo;
    HRESULT hRet;

    if (psi == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);

        DPRINT("No ISecurityInformation class passed!\n");
        return NULL;
    }

    /* get the object information from the server. Zero the structure before
       because some applications seem to return SUCCESS but only seem to set the
       fields they care about. */
    ZeroMemory(&ObjectInfo, sizeof(ObjectInfo));
    hRet = psi->lpVtbl->GetObjectInformation(psi, &ObjectInfo);

    if (FAILED(hRet))
    {
        SetLastError(hRet);

        DPRINT("CreateSecurityPage() failed! Failed to query the object information!\n");
        return NULL;
    }
    
    hRet = CoInitialize(NULL);
    if (FAILED(hRet))
    {
        DPRINT("CoInitialize failed!\n");
        return NULL;
    }
    
    if (!RegisterCheckListControl(hDllInstance))
    {
        DPRINT("Registering the CHECKLIST_ACLUI class failed!\n");
        return NULL;
    }
    
    sPage = HeapAlloc(GetProcessHeap(),
                      HEAP_ZERO_MEMORY,
                      sizeof(SECURITY_PAGE));
    if (sPage == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        
        DPRINT("Not enough memory to allocate a SECURITY_PAGE!\n");
        return NULL;
    }
    sPage->psi = psi;
    sPage->ObjectInfo = ObjectInfo;

    ZeroMemory(&psp, sizeof(psp));

    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_USECALLBACK;
    psp.hInstance = hDllInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_SECPAGE);
    psp.pfnDlgProc = SecurityPageProc;
    psp.lParam = (LPARAM)sPage;
    psp.pfnCallback = SecurityPageCallback;

    if (ObjectInfo.dwFlags & SI_PAGE_TITLE)
    {
        psp.pszTitle = ObjectInfo.pszPageTitle;

        if (psp.pszTitle != NULL)
        {
            psp.dwFlags |= PSP_USETITLE;
        }
    }
    else
    {
        psp.pszTitle = NULL;
    }
    
    /* NOTE: the SECURITY_PAGE structure will be freed by the property page
             callback! */

    return CreatePropertySheetPage(&psp);
}


/*
 * EditSecurity								EXPORTED
 *
 * @implemented
 */
BOOL
WINAPI
EditSecurity(IN HWND hwndOwner,
             IN LPSECURITYINFO psi)
{
    HRESULT hRet;
    SI_OBJECT_INFO ObjectInfo;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE hPages[1];
    LPWSTR lpCaption;
    BOOL Ret;

    if (psi == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);

        DPRINT("No ISecurityInformation class passed!\n");
        return FALSE;
    }

    /* get the object information from the server. Zero the structure before
       because some applications seem to return SUCCESS but only seem to set the
       fields they care about. */
    ZeroMemory(&ObjectInfo, sizeof(ObjectInfo));
    hRet = psi->lpVtbl->GetObjectInformation(psi, &ObjectInfo);

    if (FAILED(hRet))
    {
        SetLastError(hRet);

        DPRINT("GetObjectInformation() failed!\n");
        return FALSE;
    }

    /* create the page */
    hPages[0] = CreateSecurityPage(psi);
    if (hPages[0] == NULL)
    {
        DPRINT("CreateSecurityPage(), couldn't create property sheet!\n");
        return FALSE;
    }

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_DEFAULT;
    psh.hwndParent = hwndOwner;
    psh.hInstance = hDllInstance;

    /* Set the page title to the object name, make sure the format string
       has "%1" NOT "%s" because it uses FormatMessage() to automatically
       allocate the right amount of memory. */
    LoadAndFormatString(hDllInstance,
                        IDS_PSP_TITLE,
                        &lpCaption,
                        ObjectInfo.pszObjectName);
    psh.pszCaption = lpCaption;

    psh.nPages = sizeof(hPages) / sizeof(HPROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.phpage = hPages;

    Ret = (PropertySheet(&psh) != -1);

    if (lpCaption != NULL)
    {
        LocalFree((HLOCAL)lpCaption);
    }

    return Ret;
}

BOOL STDCALL
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hDllInstance = hinstDLL;
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

