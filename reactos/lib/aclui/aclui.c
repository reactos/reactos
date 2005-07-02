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
#include "acluilib.h"

HINSTANCE hDllInstance;

static VOID
DestroySecurityPage(IN PSECURITY_PAGE sp)
{
    if(sp->hiUsrs != NULL)
    {
        ImageList_Destroy(sp->hiUsrs);
    }

    HeapFree(GetProcessHeap(),
             0,
             sp);
}

static VOID
FreeAceList(IN PACE_LISTITEM *AceListHead)
{
    PACE_LISTITEM CurItem, NextItem;
    
    CurItem = *AceListHead;
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
    
    *AceListHead = NULL;
}

static PACE_LISTITEM
FindSidInAceList(IN PACE_LISTITEM AceListHead,
                 IN PSID Sid)
{
    PACE_LISTITEM CurItem;
    
    for (CurItem = AceListHead;
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
ReloadUsersGroupsList(IN PSECURITY_PAGE sp)
{
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    BOOL DaclPresent, DaclDefaulted;
    PACL Dacl = NULL;
    HRESULT hRet;

    /* delete the cached ACL */
    FreeAceList(&sp->AceListHead);

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
            PACE_LISTITEM AceListItem, *NextAcePtr;
            PSID Sid;
            PVOID Ace;
            ULONG AceIndex;
            DWORD AccountNameSize, DomainNameSize, SidLength;
            SID_NAME_USE SidNameUse;
            DWORD LookupResult;
            
            NextAcePtr = &sp->AceListHead;
            
            for (AceIndex = 0;
                 AceIndex < Dacl->AceCount;
                 AceIndex++)
            {
                GetAce(Dacl,
                       AceIndex,
                       &Ace);

                Sid = (PSID)&((PACCESS_ALLOWED_ACE)Ace)->SidStart;

                if (!FindSidInAceList(sp->AceListHead,
                                      Sid))
                {
                    SidLength = GetLengthSid(Sid);
                    
                    AccountNameSize = 0;
                    DomainNameSize = 0;

                    /* calculate the size of the buffer we need to calculate */
                    LookupAccountSid(NULL, /* FIXME */
                                     Sid,
                                     NULL,
                                     &AccountNameSize,
                                     NULL,
                                     &DomainNameSize,
                                     &SidNameUse);

                    /* allocate the ace */
                    AceListItem = HeapAlloc(GetProcessHeap(),
                                            0,
                                            sizeof(ACE_LISTITEM) +
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
                        if (!LookupAccountSid(NULL, /* FIXME */
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
                            if (OpenLSAPolicyHandle(NULL, /* FIXME */
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
AddAceListEntry(IN PSECURITY_PAGE sp,
                IN PACE_LISTITEM AceListItem,
                IN INT Index,
                IN BOOL Selected)
{
    LVITEM li;

    li.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE | LVIF_TEXT;
    li.iItem = Index;
    li.iSubItem = 0;
    li.state = (Selected ? LVIS_SELECTED : 0);
    li.stateMask = LVIS_SELECTED;
    li.pszText = (AceListItem->DisplayString != NULL ? AceListItem->DisplayString : AceListItem->AccountName);
    switch (AceListItem->SidNameUse)
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
    li.lParam = (LPARAM)AceListItem;

    return ListView_InsertItem(sp->hWndAceList,
                               &li);
}

static VOID
FillUsersGroupsList(IN PSECURITY_PAGE sp)
{
    LPARAM SelLParam;
    PACE_LISTITEM CurItem;
    RECT rcLvClient;

    SelLParam = ListViewGetSelectedItemData(sp->hWndAceList);

    DisableRedrawWindow(sp->hWndAceList);

    ListView_DeleteAllItems(sp->hWndAceList);

    ReloadUsersGroupsList(sp);

    for (CurItem = sp->AceListHead;
         CurItem != NULL;
         CurItem = CurItem->Next)
    {
        AddAceListEntry(sp,
                        CurItem,
                        -1,
                        (SelLParam == (LPARAM)CurItem));
    }
    
    EnableRedrawWindow(sp->hWndAceList);
    
    GetClientRect(sp->hWndAceList, &rcLvClient);
    
    ListView_SetColumnWidth(sp->hWndAceList,
                            0,
                            rcLvClient.right);
}

static VOID
UpdateControlStates(IN PSECURITY_PAGE sp)
{
    BOOL UserOrGroupSelected;
    
    UserOrGroupSelected = (ListViewGetSelectedItemData(sp->hWndAceList) != 0);

    EnableWindow(sp->hBtnRemove, UserOrGroupSelected);
}

static UINT CALLBACK
SecurityPageCallback(IN HWND hwnd,
                     IN UINT uMsg,
                     IN LPPROPSHEETPAGE ppsp)
{
    PSECURITY_PAGE sp = (PSECURITY_PAGE)ppsp->lParam;
    
    switch(uMsg)
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
                            CIS_DISABLED | CIS_NONE,
                            (LPARAM)NameStr);
            }
        }
    }
}

static INT_PTR CALLBACK
SecurityPageProc(IN HWND hwndDlg,
                 IN UINT uMsg,
                 IN WPARAM wParam,
                 IN LPARAM lParam)
{
    PSECURITY_PAGE sp;

    switch(uMsg)
    {
        case WM_NOTIFY:
        {
            NMHDR *pnmh = (NMHDR*)lParam;
            if (pnmh->idFrom == IDC_ACELIST)
            {
                sp = (PSECURITY_PAGE)GetWindowLongPtr(hwndDlg,
                                                      DWL_USER);
                if (sp != NULL)
                {
                    switch(pnmh->code)
                    {
                        case LVN_ITEMCHANGED:
                        {
                            UpdateControlStates(sp);
                            break;
                        }
                    }
                }
            }
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
                sp->hWndAceList = GetDlgItem(hwndDlg, IDC_ACELIST);
                sp->hBtnRemove = GetDlgItem(hwndDlg, IDC_ACELIST_REMOVE);
                sp->hAceCheckList = GetDlgItem(hwndDlg, IDC_ACE_CHECKLIST);

                /* save the pointer to the structure */
                SetWindowLongPtr(hwndDlg,
                                 DWL_USER,
                                 (DWORD_PTR)sp);

                sp->hiUsrs = ImageList_LoadBitmap(hDllInstance,
                                                  MAKEINTRESOURCE(IDB_USRGRPIMAGES),
                                                  16,
                                                  3,
                                                  0);

                /* setup the listview control */
                ListView_SetExtendedListViewStyleEx(sp->hWndAceList,
                                                    LVS_EX_FULLROWSELECT,
                                                    LVS_EX_FULLROWSELECT);
                ListView_SetImageList(sp->hWndAceList,
                                      sp->hiUsrs,
                                      LVSIL_SMALL);

                GetClientRect(sp->hWndAceList, &rcLvClient);
                
                /* add a column to the list view */
                lvc.mask = LVCF_FMT | LVCF_WIDTH;
                lvc.fmt = LVCFMT_LEFT;
                lvc.cx = rcLvClient.right;
                ListView_InsertColumn(sp->hWndAceList, 0, &lvc);

                FillUsersGroupsList(sp);
                
                ListViewSelectItem(sp->hWndAceList,
                                   0);

                /* calculate the columns of the allow/deny checkboxes */
                SetAceCheckListColumns(sp->hAceCheckList,
                                       CLB_ALLOW,
                                       GetDlgItem(hwndDlg, IDC_LABEL_ALLOW));
                SetAceCheckListColumns(sp->hAceCheckList,
                                       CLB_DENY,
                                       GetDlgItem(hwndDlg, IDC_LABEL_DENY));

                /* FIXME - hide controls in case the flags aren't present */

                LoadPermissionsList(sp,
                                    NULL,
                                    SI_ACCESS_GENERAL |
                                    ((sp->ObjectInfo.dwFlags & SI_CONTAINER) ? SI_ACCESS_CONTAINER : 0),
                                    &sp->DefaultAccess);
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

    if(psi == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);

        DPRINT("No ISecurityInformation class passed!\n");
        return NULL;
    }

    /* get the object information from the server interface */
    hRet = psi->lpVtbl->GetObjectInformation(psi, &ObjectInfo);

    if(FAILED(hRet))
    {
        SetLastError(hRet);

        DPRINT("CreateSecurityPage() failed! Failed to query the object information!\n");
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

    if((ObjectInfo.dwFlags & SI_PAGE_TITLE) &&
       ObjectInfo.pszPageTitle != NULL && ObjectInfo.pszPageTitle[0] != L'\0')
    {
        /* Set the page title if the flag is present and the string isn't empty */
        psp.pszTitle = ObjectInfo.pszPageTitle;
        psp.dwFlags |= PSP_USETITLE;
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

    if(psi == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);

        DPRINT("No ISecurityInformation class passed!\n");
        return FALSE;
    }

    /* get the object information from the server interface */
    hRet = psi->lpVtbl->GetObjectInformation(psi, &ObjectInfo);

    if(FAILED(hRet))
    {
        SetLastError(hRet);

        DPRINT("GetObjectInformation() failed!\n");
        return FALSE;
    }

    /* create the page */
    hPages[0] = CreateSecurityPage(psi);
    if(hPages[0] == NULL)
    {
        DPRINT("CreateSecurityPage(), couldn't create property sheet!\n");
        return FALSE;
    }

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_DEFAULT;
    psh.hwndParent = hwndOwner;
    psh.hInstance = hDllInstance;
    if((ObjectInfo.dwFlags & SI_PAGE_TITLE) &&
       ObjectInfo.pszPageTitle != NULL && ObjectInfo.pszPageTitle[0] != L'\0')
    {
        /* Set the page title if the flag is present and the string isn't empty */
        psh.pszCaption = ObjectInfo.pszPageTitle;
        psh.dwFlags |= PSH_PROPTITLE;
        lpCaption = NULL;
    }
    else
    {
        /* Set the page title to the object name, make sure the format string
           has "%1" NOT "%s" because it uses FormatMessage() to automatically
           allocate the right amount of memory. */
        LoadAndFormatString(hDllInstance,
                            IDS_PSP_TITLE,
                            &lpCaption,
                            ObjectInfo.pszObjectName);
        psh.pszCaption = lpCaption;
    }

    psh.nPages = sizeof(hPages) / sizeof(HPROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.phpage = hPages;

    Ret = (PropertySheet(&psh) != -1);

    if(lpCaption != NULL)
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

