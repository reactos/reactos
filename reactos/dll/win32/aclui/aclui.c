/*
 * ReactOS Access Control List Editor
 * Copyright (C) 2004-2005 ReactOS Team
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

#define NDEBUG
#include <debug.h>

HINSTANCE hDllInstance;

#define SIDN_LOOKUPSUCCEEDED    (0x101)
typedef struct _SIDLOOKUPNOTIFYINFO
{
    NMHDR nmh;
    PSID Sid;
    PSIDREQRESULT SidRequestResult;
} SIDLOOKUPNOTIFYINFO, *PSIDLOOKUPNOTIFYINFO;

static PSID
AceHeaderToSID(IN PACE_HEADER AceHeader)
{
    PSID Sid = NULL;
    switch (AceHeader->AceType)
    {
        case ACCESS_ALLOWED_ACE_TYPE:
            Sid = (PSID)&((PACCESS_ALLOWED_ACE)AceHeader)->SidStart;
            break;
#if 0
        case ACCESS_ALLOWED_CALLBACK_ACE_TYPE:
            Sid = (PSID)&((PACCESS_ALLOWED_CALLBACK_ACE)AceHeader)->SidStart;
            break;
        case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE:
            Sid = (PSID)&((PACCESS_ALLOWED_CALLBACK_OBJECT_ACE)AceHeader)->SidStart;
            break;
#endif
        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
            Sid = (PSID)&((PACCESS_ALLOWED_OBJECT_ACE)AceHeader)->SidStart;
            break;
        case ACCESS_DENIED_ACE_TYPE:
            Sid = (PSID)&((PACCESS_DENIED_ACE)AceHeader)->SidStart;
            break;
#if 0
        case ACCESS_DENIED_CALLBACK_ACE_TYPE:
            Sid = (PSID)&((PACCESS_DENIED_CALLBACK_ACE)AceHeader)->SidStart;
            break;
        case ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE:
            Sid = (PSID)&((PACCESS_DENIED_CALLBACK_OBJECT_ACE)AceHeader)->SidStart;
            break;
#endif
        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
            Sid = (PSID)&((PACCESS_DENIED_OBJECT_ACE)AceHeader)->SidStart;
            break;
    }

    return Sid;
}

static VOID
DestroySecurityPage(IN PSECURITY_PAGE sp)
{
    if(sp->hiPrincipals != NULL)
    {
        ImageList_Destroy(sp->hiPrincipals);
    }

    DestroySidCacheMgr(sp->SidCacheMgr);

    if (sp->OwnerSid != NULL)
        LocalFree((HLOCAL)sp->OwnerSid);

    HeapFree(GetProcessHeap(),
             0,
             sp);

    CoUninitialize();
}

static VOID
FreePrincipalsList(IN PSECURITY_PAGE sp,
                   IN PPRINCIPAL_LISTITEM *PrincipalsListHead)
{
    PPRINCIPAL_LISTITEM CurItem, NextItem;
    PACE_ENTRY AceEntry, NextAceEntry;

    CurItem = *PrincipalsListHead;
    while (CurItem != NULL)
    {
        /* Free all ACEs */
        AceEntry = CurItem->ACEs;
        while (AceEntry != NULL)
        {
            NextAceEntry = AceEntry->Next;
            HeapFree(GetProcessHeap(),
                     0,
                     AceEntry);
            AceEntry = NextAceEntry;
        }

        /* free the SID string if present */
        if (CurItem->SidReqResult != NULL)
        {
            DereferenceSidReqResult(sp->SidCacheMgr,
                                    CurItem->SidReqResult);
        }

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

static PACE_ENTRY
AddAceToPrincipal(IN PPRINCIPAL_LISTITEM Principal,
                  IN PACE_HEADER AceHeader)
{
    PACE_ENTRY AceEntry, *AceLink;

    AceEntry = HeapAlloc(GetProcessHeap(),
                         0,
                         sizeof(ACE_ENTRY) + AceHeader->AceSize);
    if (AceEntry != NULL)
    {
        AceEntry->Next = NULL;

        /* copy the ACE */
        CopyMemory(AceEntry + 1,
                   AceHeader,
                   AceHeader->AceSize);

        /* append it to the list */
        AceLink = &Principal->ACEs;
        while (*AceLink != NULL)
        {
            AceLink = &(*AceLink)->Next;
        }
        *AceLink = AceEntry;
    }

    return AceEntry;
}

static PPRINCIPAL_LISTITEM
FindSidInPrincipalsListAddAce(IN PPRINCIPAL_LISTITEM PrincipalsListHead,
                              IN PSID Sid,
                              IN PACE_HEADER AceHeader)
{
    PPRINCIPAL_LISTITEM CurItem;

    for (CurItem = PrincipalsListHead;
         CurItem != NULL;
         CurItem = CurItem->Next)
    {
        if (EqualSid((PSID)(CurItem + 1),
                     Sid))
        {
            if (AddAceToPrincipal(CurItem,
                                  AceHeader) != NULL)
            {
                return CurItem;
            }

            /* unable to add the ACE to the principal */
            break;
        }
    }

    return NULL;
}

static VOID
SidLookupCompletion(IN HANDLE SidCacheMgr,
                    IN PSID Sid,
                    IN PSIDREQRESULT SidRequestResult,
                    IN PVOID Context)
{
    PSECURITY_PAGE sp = (PSECURITY_PAGE)Context;

    /* NOTE: this routine may be executed in a different thread
             than the GUI! */

    if (SidRequestResult != NULL)
    {
        SIDLOOKUPNOTIFYINFO LookupInfo;

        LookupInfo.nmh.hwndFrom = sp->hWnd;
        LookupInfo.nmh.idFrom = 0;
        LookupInfo.nmh.code = SIDN_LOOKUPSUCCEEDED;
        LookupInfo.Sid = Sid;
        LookupInfo.SidRequestResult = SidRequestResult;

        /* notify the page that the sid lookup succeeded */
        SendMessage(sp->hWnd,
                    WM_NOTIFY,
                    (WPARAM)LookupInfo.nmh.idFrom,
                    (LPARAM)&LookupInfo.nmh);
    }
}

static PPRINCIPAL_LISTITEM
AddPrincipalToList(IN PSECURITY_PAGE sp,
                   IN PSID Sid,
                   IN PACE_HEADER AceHeader,
                   OUT BOOL *LookupDeferred  OPTIONAL)
{
    PPRINCIPAL_LISTITEM PrincipalListItem = NULL, *PrincipalLink;
    PACE_ENTRY AceEntry;
    BOOL Deferred = FALSE;

    if (!FindSidInPrincipalsListAddAce(sp->PrincipalsListHead,
                                       Sid,
                                       AceHeader))
    {
        DWORD SidLength;

        PrincipalLink = &sp->PrincipalsListHead;
        while (*PrincipalLink != NULL)
        {
            PrincipalLink = &(*PrincipalLink)->Next;
        }

        SidLength = GetLengthSid(Sid);

        /* allocate the principal */
        PrincipalListItem = HeapAlloc(GetProcessHeap(),
                                      0,
                                      sizeof(PRINCIPAL_LISTITEM) + SidLength);
        if (PrincipalListItem != NULL)
        {
            PrincipalListItem->DisplayString = NULL;
            PrincipalListItem->SidReqResult = NULL;

            CopySid(SidLength,
                    (PSID)(PrincipalListItem + 1),
                    Sid);

            /* allocate some memory for the ACE and copy it */
            AceEntry = HeapAlloc(GetProcessHeap(),
                                 0,
                                 sizeof(ACE_ENTRY) + AceHeader->AceSize);
            if (AceEntry != NULL)
            {
                AceEntry->Next = NULL;
                CopyMemory(AceEntry + 1,
                           AceHeader,
                           AceHeader->AceSize);

                /* add the ACE to the list */
                PrincipalListItem->ACEs = AceEntry;

                PrincipalListItem->Next = NULL;

                /* append item to the principals list */
                *PrincipalLink = PrincipalListItem;

                /* lookup the SID now */
                Deferred = !LookupSidCache(sp->SidCacheMgr,
                                           Sid,
                                           SidLookupCompletion,
                                           sp);
            }
            else
            {
                HeapFree(GetProcessHeap(),
                         0,
                         PrincipalListItem);
                PrincipalListItem = NULL;
            }
        }
    }

    if (PrincipalListItem != NULL && LookupDeferred != NULL)
    {
        *LookupDeferred = Deferred;
    }

    return PrincipalListItem;
}

static LPWSTR
GetDisplayStringFromSidRequestResult(IN PSIDREQRESULT SidReqResult)
{
    LPWSTR lpDisplayString = NULL;

    if (SidReqResult->SidNameUse == SidTypeUser ||
        SidReqResult->SidNameUse == SidTypeGroup)
    {
        LoadAndFormatString(hDllInstance,
                            IDS_USERDOMAINFORMAT,
                            &lpDisplayString,
                            SidReqResult->AccountName,
                            SidReqResult->DomainName,
                            SidReqResult->AccountName);
    }
    else
    {
        LoadAndFormatString(hDllInstance,
                            IDS_USERFORMAT,
                            &lpDisplayString,
                            SidReqResult->AccountName);
    }

    return lpDisplayString;
}

static LPWSTR
GetPrincipalDisplayString(IN PPRINCIPAL_LISTITEM PrincipalListItem)
{
    LPWSTR lpDisplayString = NULL;

    if (PrincipalListItem->SidReqResult != NULL)
    {
        lpDisplayString = GetDisplayStringFromSidRequestResult(PrincipalListItem->SidReqResult);
    }
    else
    {
        ConvertSidToStringSidW((PSID)(PrincipalListItem + 1),
                               &lpDisplayString);
    }

    return lpDisplayString;
}

static LPWSTR
GetPrincipalAccountNameString(IN PPRINCIPAL_LISTITEM PrincipalListItem)
{
    LPWSTR lpDisplayString = NULL;

    if (PrincipalListItem->SidReqResult != NULL)
    {
        LoadAndFormatString(hDllInstance,
                            IDS_USERFORMAT,
                            &lpDisplayString,
                            PrincipalListItem->SidReqResult->AccountName);
    }
    else
    {
        ConvertSidToStringSid((PSID)(PrincipalListItem + 1),
                              &lpDisplayString);
    }

    return lpDisplayString;
}

static VOID
CreatePrincipalListItem(OUT LVITEM *li,
                        IN PSECURITY_PAGE sp,
                        IN PPRINCIPAL_LISTITEM PrincipalListItem,
                        IN INT Index,
                        IN BOOL Selected)
{
    INT ImageIndex = 2;

    if (PrincipalListItem->SidReqResult != NULL)
    {
        switch (PrincipalListItem->SidReqResult->SidNameUse)
        {
            case SidTypeUser:
                ImageIndex = 0;
                break;
            case SidTypeWellKnownGroup:
            case SidTypeGroup:
                ImageIndex = 1;
                break;
            default:
                break;
        }
    }

    li->mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE | LVIF_TEXT;
    li->iItem = Index;
    li->iSubItem = 0;
    li->state = (Selected ? LVIS_SELECTED : 0);
    li->stateMask = LVIS_SELECTED;
    li->pszText = PrincipalListItem->DisplayString;
    li->iImage = ImageIndex;
    li->lParam = (LPARAM)PrincipalListItem;
}

static INT
AddPrincipalListEntry(IN PSECURITY_PAGE sp,
                      IN PPRINCIPAL_LISTITEM PrincipalListItem,
                      IN INT Index,
                      IN BOOL Selected)
{
    LVITEM li;
    INT Ret;

    if (PrincipalListItem->DisplayString != NULL)
    {
        LocalFree((HLOCAL)PrincipalListItem->DisplayString);
    }
    PrincipalListItem->DisplayString = GetPrincipalDisplayString(PrincipalListItem);

    CreatePrincipalListItem(&li,
                            sp,
                            PrincipalListItem,
                            Index,
                            Selected);

    Ret = ListView_InsertItem(sp->hWndPrincipalsList,
                              &li);

    return Ret;
}

static int CALLBACK
PrincipalCompare(IN LPARAM lParam1,
                 IN LPARAM lParam2,
                 IN LPARAM lParamSort)
{
    PPRINCIPAL_LISTITEM Item1 = (PPRINCIPAL_LISTITEM)lParam1;
    PPRINCIPAL_LISTITEM Item2 = (PPRINCIPAL_LISTITEM)lParam2;

    if (Item1->DisplayString != NULL && Item2->DisplayString != NULL)
    {
        return wcscmp(Item1->DisplayString,
                      Item2->DisplayString);
    }

    return 0;
}

static VOID
UpdatePrincipalListItem(IN PSECURITY_PAGE sp,
                        IN INT PrincipalIndex,
                        IN PPRINCIPAL_LISTITEM PrincipalListItem,
                        IN PSIDREQRESULT SidReqResult)
{
    LVITEM li;

    /* replace the request result structure */
    if (PrincipalListItem->SidReqResult != NULL)
    {
        DereferenceSidReqResult(sp->SidCacheMgr,
                                PrincipalListItem->SidReqResult);
    }

    ReferenceSidReqResult(sp->SidCacheMgr,
                          SidReqResult);
    PrincipalListItem->SidReqResult = SidReqResult;

    /* update the display string */
    if (PrincipalListItem->DisplayString != NULL)
    {
        LocalFree((HLOCAL)PrincipalListItem->DisplayString);
    }
    PrincipalListItem->DisplayString = GetPrincipalDisplayString(PrincipalListItem);

    /* update the list item */
    CreatePrincipalListItem(&li,
                            sp,
                            PrincipalListItem,
                            PrincipalIndex,
                            FALSE);

    /* don't change the list item state */
    li.mask &= ~(LVIF_STATE | LVIF_PARAM);

    (void)ListView_SetItem(sp->hWndPrincipalsList,
                           &li);

    /* sort the principals list view again */
    (void)ListView_SortItems(sp->hWndPrincipalsList,
                             PrincipalCompare,
                             (LPARAM)sp);
}

static VOID
ReloadPrincipalsList(IN PSECURITY_PAGE sp)
{
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    BOOL DaclPresent, DaclDefaulted, OwnerDefaulted;
    PACL Dacl = NULL;
    PSID OwnerSid = NULL;
    LPTSTR OwnerSidString;
    DWORD SidLen;
    HRESULT hRet;

    /* delete the cached ACL */
    FreePrincipalsList(sp,
                       &sp->PrincipalsListHead);

    /* query the ACL */
    hRet = sp->psi->lpVtbl->GetSecurity(sp->psi,
                                        DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
                                        &SecurityDescriptor,
                                        FALSE);
    if (SUCCEEDED(hRet) && SecurityDescriptor != NULL)
    {
        if (GetSecurityDescriptorOwner(SecurityDescriptor,
                                       &OwnerSid,
                                       &OwnerDefaulted))
        {
            sp->OwnerDefaulted = OwnerDefaulted;
            if (sp->OwnerSid != NULL)
            {
                LocalFree((HLOCAL)sp->OwnerSid);
                sp->OwnerSid = NULL;
            }

            SidLen = GetLengthSid(OwnerSid);
            if (SidLen == 0)
                goto ClearOwner;

            sp->OwnerSid = (PSID)LocalAlloc(LMEM_FIXED,
                                            SidLen);
            if (sp->OwnerSid != NULL)
            {
                if (CopySid(SidLen,
                            sp->OwnerSid,
                            OwnerSid))
                {
                    /* Lookup the SID now */
                    if (!LookupSidCache(sp->SidCacheMgr,
                                        sp->OwnerSid,
                                        SidLookupCompletion,
                                        sp))
                    {
                        /* Lookup was deferred */
                        if (ConvertSidToStringSid(sp->OwnerSid,
                                                  &OwnerSidString))
                        {
                            SetDlgItemText(sp->hWnd,
                                           IDC_OWNER,
                                           OwnerSidString);
                            LocalFree((HLOCAL)OwnerSidString);
                        }
                        else
                            goto ClearOwner;
                    }
                }
                else
                    goto ClearOwner;
            }
            else
                goto ClearOwner;
        }
        else
        {
ClearOwner:
            SetDlgItemText(sp->hWnd,
                           IDC_OWNER,
                           NULL);
        }

        if (GetSecurityDescriptorDacl(SecurityDescriptor,
                                      &DaclPresent,
                                      &Dacl,
                                      &DaclDefaulted) &&
            DaclPresent && Dacl != NULL)
        {
            PSID Sid;
            PACE_HEADER AceHeader;
            ULONG AceIndex;

            for (AceIndex = 0;
                 AceIndex < Dacl->AceCount;
                 AceIndex++)
            {
                if (GetAce(Dacl,
                           AceIndex,
                           (LPVOID*)&AceHeader) &&
                    AceHeader != NULL)
                {
                    BOOL LookupDeferred;
                    PPRINCIPAL_LISTITEM PrincipalListItem;

                    Sid = AceHeaderToSID(AceHeader);

                    PrincipalListItem = AddPrincipalToList(sp,
                                                           Sid,
                                                           AceHeader,
                                                           &LookupDeferred);

                    if (PrincipalListItem != NULL && LookupDeferred)
                    {
                        AddPrincipalListEntry(sp,
                                              PrincipalListItem,
                                              -1,
                                              FALSE);
                    }
                }
            }
        }
        LocalFree((HLOCAL)SecurityDescriptor);
    }
}

static VOID
UpdateControlStates(IN PSECURITY_PAGE sp)
{
    PPRINCIPAL_LISTITEM Selected = (PPRINCIPAL_LISTITEM)ListViewGetSelectedItemData(sp->hWndPrincipalsList);

    EnableWindow(sp->hBtnRemove,
                 Selected != NULL);
    EnableWindow(sp->hAceCheckList,
                 Selected != NULL);

    if (Selected != NULL)
    {
        LPWSTR szLabel;
        LPWSTR szDisplayString;

        szDisplayString = GetPrincipalAccountNameString(Selected);
        if (LoadAndFormatString(hDllInstance,
                                IDS_PERMISSIONS_FOR,
                                &szLabel,
                                szDisplayString))
        {
            SetWindowText(sp->hPermissionsForLabel,
                          szLabel);

            LocalFree((HLOCAL)szLabel);
        }

        LocalFree((HLOCAL)szDisplayString);

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

static void
UpdatePrincipalInfo(IN PSECURITY_PAGE sp,
                    IN PSIDLOOKUPNOTIFYINFO LookupInfo)
{
    PPRINCIPAL_LISTITEM CurItem;
    LPWSTR DisplayName;

    if (sp->OwnerSid != NULL &&
        EqualSid(sp->OwnerSid,
                 LookupInfo->Sid))
    {
        if (LookupInfo->SidRequestResult != NULL)
            DisplayName = GetDisplayStringFromSidRequestResult(LookupInfo->SidRequestResult);
        else if (!ConvertSidToStringSidW(LookupInfo->Sid,
                                         &DisplayName))
        {
            DisplayName = NULL;
        }

        if (DisplayName != NULL)
        {
            SetDlgItemTextW(sp->hWnd,
                            IDC_OWNER,
                            DisplayName);

            LocalFree((HLOCAL)DisplayName);
        }
    }

    for (CurItem = sp->PrincipalsListHead;
         CurItem != NULL;
         CurItem = CurItem->Next)
    {
        if (EqualSid((PSID)(CurItem + 1),
                     LookupInfo->Sid))
        {
            INT PrincipalIndex;
            LVFINDINFO lvfi;

            /* find the principal in the list */
            lvfi.flags = LVFI_PARAM;
            lvfi.lParam = (LPARAM)CurItem;
            PrincipalIndex = ListView_FindItem(sp->hWndPrincipalsList,
                                               -1,
                                               &lvfi);

            if (PrincipalIndex != -1)
            {
                /* update the principal in the list view control */
                UpdatePrincipalListItem(sp,
                                        PrincipalIndex,
                                        CurItem,
                                        LookupInfo->SidRequestResult);

                if (ListViewGetSelectedItemData(sp->hWndPrincipalsList) == (LPARAM)CurItem)
                {
                    UpdateControlStates(sp);
                }
            }
            else
            {
                AddPrincipalListEntry(sp,
                                      CurItem,
                                      -1,
                                      FALSE);
            }
            break;
        }
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
    WCHAR szSpecialPermissions[255];
    BOOLEAN SpecialPermissionsPresent = FALSE;
    ACCESS_MASK SpecialPermissionsMask = 0;

    /* clear the permissions list */

    SendMessage(sp->hAceCheckList,
                CLM_CLEAR,
                0,
                0);

    /* query the access rights from the server */
    hRet = sp->psi->lpVtbl->GetAccessRights(sp->psi,
                                            GuidObjectType,
                                            dwFlags, /* FIXME */
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
                            (WPARAM)CurAccess->mask,
                            (LPARAM)NameStr);
            }
            else if (CurAccess->dwFlags & SI_ACCESS_SPECIFIC)
            {
                SpecialPermissionsPresent = TRUE;
                SpecialPermissionsMask |= CurAccess->mask;
            }
        }
    }

    /* add the special permissions check item in case the specific access rights
       aren't displayed */
    if (SpecialPermissionsPresent &&
        LoadString(hDllInstance,
                   IDS_SPECIAL_PERMISSIONS,
                   szSpecialPermissions,
                   sizeof(szSpecialPermissions) / sizeof(szSpecialPermissions[0])))
    {
        /* add the special permissions check item */
        sp->SpecialPermCheckIndex = (INT)SendMessage(sp->hAceCheckList,
                                                     CLM_ADDITEM,
                                                     (WPARAM)SpecialPermissionsMask,
                                                     (LPARAM)szSpecialPermissions);
        if (sp->SpecialPermCheckIndex != -1)
        {
            SendMessage(sp->hAceCheckList,
                        CLM_SETITEMSTATE,
                        (WPARAM)sp->SpecialPermCheckIndex,
                        CIS_ALLOWDISABLED | CIS_DENYDISABLED | CIS_NONE);
        }
    }
}

static VOID
ResizeControls(IN PSECURITY_PAGE sp,
               IN INT Width,
               IN INT Height)
{
    HWND hWndAllow, hWndDeny, hWndOwnerEdit;
    RECT rcControl, rcControl2, rcControl3, rcWnd;
    INT cxWidth, cxEdge, btnSpacing;
    POINT pt, pt2;
    HDWP dwp;
    INT nControls = 8;
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

    if ((dwp = BeginDeferWindowPos(nControls)))
    {
        /* resize the owner edit field */
        hWndOwnerEdit = GetDlgItem(sp->hWnd,
                                   IDC_OWNER);
        GetWindowRect(hWndOwnerEdit,
                      &rcControl);
        pt2.x = 0;
        pt2.y = 0;
        MapWindowPoints(hWndOwnerEdit,
                        sp->hWnd,
                        &pt2,
                        1);
        if (!(dwp = DeferWindowPos(dwp,
                                   hWndOwnerEdit,
                                   NULL,
                                   0,
                                   0,
                                   Width - pt.x - pt2.x,
                                   rcControl.bottom - rcControl.top,
                                   SWP_NOMOVE | SWP_NOZORDER)))
        {
            goto EndDeferWnds;
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
            goto EndDeferWnds;
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
            goto EndDeferWnds;
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
            goto EndDeferWnds;
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
            goto EndDeferWnds;
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
            goto EndDeferWnds;
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
            goto EndDeferWnds;
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
                                       Height - (rcControl.top - rcWnd.top) -
                                           (rcControl3.bottom - rcControl3.top) - pt.x - btnSpacing :
                                       Height - (rcControl.top - rcWnd.top) - pt.x),
                                   SWP_NOMOVE | SWP_NOZORDER)))
        {
            goto EndDeferWnds;
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
                goto EndDeferWnds;
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
                goto EndDeferWnds;
            }
        }

        EndDeferWindowPos(dwp);
    }

EndDeferWnds:
    /* update the width of the principal list view column */
    GetClientRect(sp->hWndPrincipalsList,
                  &rcControl);
    lvc.mask = LVCF_WIDTH;
    lvc.cx = rcControl.right;
    (void)ListView_SetColumn(sp->hWndPrincipalsList,
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

static PACE_HEADER
BuildDefaultPrincipalAce(IN PSECURITY_PAGE sp,
                         IN PSID pSid)
{
    PACCESS_ALLOWED_ACE Ace;
    DWORD SidLen;
    WORD AceSize;

    SidLen = GetLengthSid(pSid);
    AceSize = FIELD_OFFSET(ACCESS_ALLOWED_ACE,
                           SidStart) + (WORD)SidLen;
    Ace = HeapAlloc(GetProcessHeap(),
                    0,
                    AceSize);
    if (Ace != NULL)
    {
        Ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        Ace->Header.AceFlags = 0; /* FIXME */
        Ace->Header.AceSize = AceSize;
        Ace->Mask = sp->DefaultAccess.mask;

        if (CopySid(SidLen,
                    (PSID)&Ace->SidStart,
                    pSid))
        {
            return &Ace->Header;
        }

        HeapFree(GetProcessHeap(),
                 0,
                 Ace);
    }

    return NULL;
}

static BOOL
AddSelectedPrincipal(IN IDsObjectPicker *pDsObjectPicker,
                     IN HWND hwndParent  OPTIONAL,
                     IN PSID pSid,
                     IN PVOID Context  OPTIONAL)
{
    PACE_HEADER AceHeader;
    PSECURITY_PAGE sp = (PSECURITY_PAGE)Context;

    AceHeader = BuildDefaultPrincipalAce(sp,
                                         pSid);
    if (AceHeader != NULL)
    {
        PPRINCIPAL_LISTITEM PrincipalListItem;
        BOOL LookupDeferred;

        PrincipalListItem = AddPrincipalToList(sp,
                                               pSid,
                                               AceHeader,
                                               &LookupDeferred);

        if (PrincipalListItem != NULL && LookupDeferred)
        {
            AddPrincipalListEntry(sp,
                                  PrincipalListItem,
                                  -1,
                                  FALSE);
        }

        HeapFree(GetProcessHeap(),
                 0,
                 AceHeader);
    }

    return TRUE;
}

static INT_PTR CALLBACK
SecurityPageProc(IN HWND hwndDlg,
                 IN UINT uMsg,
                 IN WPARAM wParam,
                 IN LPARAM lParam)
{
    PSECURITY_PAGE sp;
    INT_PTR Ret = FALSE;

    sp = (PSECURITY_PAGE)GetWindowLongPtr(hwndDlg,
                                          DWL_USER);
    if (sp != NULL || uMsg == WM_INITDIALOG)
    {
        switch (uMsg)
        {
            case WM_NOTIFY:
            {
                NMHDR *pnmh = (NMHDR*)lParam;

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
                else if (pnmh->hwndFrom == sp->hWnd)
                {
                    switch(pnmh->code)
                    {
                        case SIDN_LOOKUPSUCCEEDED:
                        {
                            PSIDLOOKUPNOTIFYINFO LookupInfo = CONTAINING_RECORD(lParam,
                                                                                SIDLOOKUPNOTIFYINFO,
                                                                                nmh);

                            /* a SID lookup succeeded, update the information */
                            UpdatePrincipalInfo(sp,
                                                LookupInfo);
                            break;
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

                        hRet = InitializeObjectPicker(sp->ServerName,
                                                      &sp->ObjectInfo,
                                                      &sp->pDsObjectPicker);
                        if (SUCCEEDED(hRet))
                        {
                            hRet = InvokeObjectPickerDialog(sp->pDsObjectPicker,
                                                            hwndDlg,
                                                            AddSelectedPrincipal,
                                                            sp);
                            if (FAILED(hRet))
                            {
                                MessageBox(hwndDlg, L"InvokeObjectPickerDialog failed!\n", NULL, 0);
                            }

                            /* delete the instance */
                            FreeObjectPicker(sp->pDsObjectPicker);
                        }
                        else
                        {
                            MessageBox(hwndDlg, L"InitializeObjectPicker failed!\n", NULL, 0);
                        }
                        break;
                    }

                    case IDC_REMOVE_PRINCIPAL:
                    {
                        PPRINCIPAL_LISTITEM SelectedPrincipal;

                        SelectedPrincipal = (PPRINCIPAL_LISTITEM)ListViewGetSelectedItemData(sp->hWndPrincipalsList);
                        if (SelectedPrincipal != NULL)
                        {
                            /* FIXME */
                        }
                        break;
                    }
                }
                break;
            }

            case WM_SIZE:
            {
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

                    /* save the pointer to the structure */
                    SetWindowLongPtr(hwndDlg,
                                     DWL_USER,
                                     (DWORD_PTR)sp);

                    (void)ListView_SetExtendedListViewStyleEx(sp->hWndPrincipalsList,
                                                              LVS_EX_FULLROWSELECT,
                                                              LVS_EX_FULLROWSELECT);

                    sp->hiPrincipals = ImageList_Create(16,
                                                        16,
                                                        ILC_COLOR32 | ILC_MASK,
                                                        0,
                                                        3);
                    if (sp->hiPrincipals != NULL)
                    {
                        HBITMAP hbmImages;

                        hbmImages = LoadBitmap(hDllInstance,
                                               MAKEINTRESOURCE(IDB_USRGRPIMAGES));
                        if (hbmImages != NULL)
                        {
                            ImageList_AddMasked(sp->hiPrincipals,
                                                hbmImages,
                                                RGB(255,
                                                    0,
                                                    255));

                            DeleteObject(hbmImages);
                        }
                    }

                    /* setup the listview control */
                    if (sp->hiPrincipals != NULL)
                    {
                        (void)ListView_SetImageList(sp->hWndPrincipalsList,
                                                    sp->hiPrincipals,
                                                    LVSIL_SMALL);
                    }

                    GetClientRect(sp->hWndPrincipalsList,
                                  &rcLvClient);

                    /* add a column to the list view */
                    lvc.mask = LVCF_FMT | LVCF_WIDTH;
                    lvc.fmt = LVCFMT_LEFT;
                    lvc.cx = rcLvClient.right;
                    (void)ListView_InsertColumn(sp->hWndPrincipalsList,
                                                0,
                                                &lvc);

                    ReloadPrincipalsList(sp);

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
                        /* editing the permissions is least the user can do when
                           the advanced button is showed */
                        sp->ObjectInfo.dwFlags |= SI_EDIT_PERMS;
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

                Ret = TRUE;
                break;
            }
        }
    }
    return Ret;
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
    PROPSHEETPAGE psp = {0};
    PSECURITY_PAGE sPage;
    SI_OBJECT_INFO ObjectInfo = {0};
    HANDLE SidCacheMgr;
    LPCWSTR SystemName = NULL;
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
    hRet = psi->lpVtbl->GetObjectInformation(psi,
                                             &ObjectInfo);

    if (FAILED(hRet))
    {
        SetLastError(hRet);

        DPRINT("CreateSecurityPage() failed! Failed to query the object information!\n");
        return NULL;
    }

    if ((ObjectInfo.dwFlags & SI_SERVER_IS_DC) &&
         ObjectInfo.pszServerName != NULL &&
         ObjectInfo.pszServerName[0] != L'\0')
    {
        SystemName = ObjectInfo.pszServerName;
    }

    SidCacheMgr = CreateSidCacheMgr(GetProcessHeap(),
                                    SystemName);
    if (SidCacheMgr == NULL)
    {
        DPRINT("Creating the SID cache failed!\n");
        return NULL;
    }

    hRet = CoInitialize(NULL);
    if (FAILED(hRet))
    {
        DestroySidCacheMgr(SidCacheMgr);
        DPRINT("CoInitialize failed!\n");
        return NULL;
    }

    sPage = HeapAlloc(GetProcessHeap(),
                      HEAP_ZERO_MEMORY,
                      sizeof(SECURITY_PAGE));
    if (sPage == NULL)
    {
        DestroySidCacheMgr(SidCacheMgr);
        CoUninitialize();

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);

        DPRINT("Not enough memory to allocate a SECURITY_PAGE!\n");
        return NULL;
    }

    ZeroMemory(sPage,
               sizeof(*sPage));

    sPage->psi = psi;
    sPage->ObjectInfo = ObjectInfo;
    sPage->ServerName = SystemName;
    sPage->SidCacheMgr = SidCacheMgr;

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
    SI_OBJECT_INFO ObjectInfo = {0};
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE hPages[1];
    LPWSTR lpCaption = NULL;
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
    hRet = psi->lpVtbl->GetObjectInformation(psi,
                                             &ObjectInfo);

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
    if (LoadAndFormatString(hDllInstance,
                            IDS_PSP_TITLE,
                            &lpCaption,
                            ObjectInfo.pszObjectName))
    {
        psh.pszCaption = lpCaption;
    }
    else
    {
        psh.pszCaption = ObjectInfo.pszObjectName;
    }

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

BOOL
WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hDllInstance = hinstDLL;

            DisableThreadLibraryCalls(hinstDLL);

            if (!RegisterCheckListControl(hinstDLL))
            {
                DPRINT("Registering the CHECKLIST_ACLUI class failed!\n");
                return FALSE;
            }
            break;

        case DLL_PROCESS_DETACH:
            UnregisterCheckListControl(hinstDLL);
            break;
    }

    return TRUE;
}

