/****************************** Module Header ******************************\
* Module Name: Upeutil.c
*
* Copyright (c) 1992, Microsoft Corporation
*
* Handles notification of key value changes in the registry that affect
* the Program Manager's groups.
* All security handling (ACLs on user profile).
* User Browser call to obtain user sid.
*
* History:
* 04-16-92 JohanneC       Created.
\***************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "upedit.h"

////////////////////////////////////////////////////////////////////////
//
// Global varaibles and constants used in this module
//
////////////////////////////////////////////////////////////////////////

SID_IDENTIFIER_AUTHORITY gNtAuthority = SECURITY_NT_AUTHORITY;
SID_IDENTIFIER_AUTHORITY gLocalSidAuthority = SECURITY_LOCAL_SID_AUTHORITY;
PSID gSystemSid;         // Initialized in 'InitializeGlobalSids'
PSID gAdminsLocalGroup;  // Initialized in 'InitializeGlobalSids'



////////////////////////////////////////////////////////////////////////
//
// Private Prototypes
//
////////////////////////////////////////////////////////////////////////

void GetGroupName(HKEY hkeyGroups, LPTSTR lpSubKey, LPTSTR lpGroupName);

/***************************************************************************\
* CentreWindow
*
* Purpose : Positions a window so that it is centred in its parent
*
* History:
* 11-14-92 JohanneC       Created.
\***************************************************************************/
void CentreWindow(HWND hwnd)
{
    RECT    rect;
    RECT    rectParent;
    HWND    hwndParent;
    LONG    dx, dy;
    LONG    dxParent, dyParent;

    // Get window rect
    GetWindowRect(hwnd, &rect);

    dx = rect.right - rect.left;
    dy = rect.bottom - rect.top;

    hwndParent = GetDesktopWindow();
    GetWindowRect(hwndParent, &rectParent);

    dxParent = rectParent.right - rectParent.left;
    dyParent = rectParent.bottom - rectParent.top;

    // Centre the child in the parent
    rect.left = (dxParent - dx) / 2;
    rect.top  = (dyParent - dy) / 3;

    // Move the child into position
    SetWindowPos(hwnd, NULL, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    //SetForegroundWindow(hwnd);
}

/***************************************************************************\
* CreateProgramGroupsEvent
*
* Create an event to handle a change in the program groups.
*
* History:
* 04-16-92 JohanneC       Created
\***************************************************************************/
HANDLE APIENTRY CreateProgramGroupsEvent()
{
    TCHAR szGroups[] = TEXT("ProgmanGroups");

    return(CreateEvent(NULL, FALSE, FALSE, szGroups));
}

/***************************************************************************\
* ResetProgramGroupsEvent
*
*
* History:
* 04-16-92 JohanneC       Created
\***************************************************************************/
VOID APIENTRY ResetProgramGroupsEvent()
{
    if (hProgramGroupsEvent && hkeyProgramGroups) {
        ResetEvent(hProgramGroupsEvent);
        RegNotifyChangeKeyValue(hkeyProgramGroupsCurrent, TRUE, REG_NOTIFY_CHANGE_NAME,
                                 hProgramGroupsEvent, TRUE);
    }
}

/***************************************************************************\
* HasProgramGroupsKeyChanged
*
* returns TRUE if a program group was modified.
*
* History:
* 04-16-92 JohanneC       Created
\***************************************************************************/
BOOL APIENTRY HasProgramGroupsKeyChanged()
{
    DWORD dwTimeOut = 2;
    DWORD err;

    if (!hProgramGroupsEvent) {
        return(FALSE);
    }
    if ((err = WaitForSingleObject(hProgramGroupsEvent, dwTimeOut))
                                                        != WAIT_TIMEOUT)
        RegNotifyChangeKeyValue(hkeyProgramGroupsCurrent, TRUE, REG_NOTIFY_CHANGE_NAME,
                                 hProgramGroupsEvent, TRUE);

    if (err || !bWorkFromCurrent) {
        //
        // Don't update the groups lists if we are not showing the current
        // profile information in UPE.
        //
        return(FALSE);
    }

    return(TRUE);
}



/***************************************************************************\
*
* HandleProgramGroupsKeyChange
*
*
* History:
* 04-16-91 Johannec       Created
\***************************************************************************/
VOID APIENTRY HandleProgramGroupsKeyChange(HWND hwnd)
{
    int i = 0;
    int index = 0;
    int cb;
    DWORD cbGroupKey = MAXKEYLEN;
    TCHAR szGroupKey[MAXKEYLEN];
    TCHAR szText[MAXKEYLEN];
    HKEY hkey;
    HWND hwndLocked;
    HWND hwndUnlocked;
    FILETIME ft;
    BOOL bFoundIt;
    LPGROUPDATA lpGroupData;

    ResetEvent(hProgramGroupsEvent);
    hwndLocked = GetDlgItem(hwnd, IDD_LOCKEDGRPS);
    hwndUnlocked = GetDlgItem(hwnd, IDD_UNLOCKEDGRPS);

    if ((cb = (int)SendMessage(hwndLocked, LB_GETCOUNT, 0, 0)) != LB_ERR)
        while (index < cb) {
            if (lpGroupData = (LPGROUPDATA)SendMessage(hwndLocked, LB_GETITEMDATA, index, 0)){
                if (!lpGroupData->lpGroupKey) {
                    index++;
                    continue;
                }
                lstrcpy(szText, lpGroupData->lpGroupKey);
                if (RegOpenKeyEx(hkeyProgramGroups, szText, 0, KEY_READ, &hkey)
                                         != ERROR_SUCCESS) {

                    SendMessage(hwndLocked, LB_DELETESTRING, index, 0);

                    //
                    // delete it from the startup group combobox.
                    //
                    if ((i = (int)SendDlgItemMessage(hwnd, IDD_STARTUP, CB_FINDSTRING,
                                   (WPARAM)-1, (LPARAM)szText)) != CB_ERR) {
                        SendDlgItemMessage(hwnd, IDD_STARTUP, CB_DELETESTRING,
                                   i, 0);
                    }
                    index--;
                    cb--;
                }
                else
                    RegCloseKey(hkey);
            }
            index++;
        }
    index = 0;
    if ((cb = (int)SendMessage(hwndUnlocked, LB_GETCOUNT, 0, 0)) != LB_ERR)
        while (index < cb) {
            if (lpGroupData = (LPGROUPDATA)SendMessage(hwndUnlocked, LB_GETITEMDATA, index, 0)){
                if (!lpGroupData->lpGroupKey) {
                    index++;
                    continue;
                }
                lstrcpy(szText, lpGroupData->lpGroupKey);
                if (RegOpenKeyEx(hkeyProgramGroups, szText, 0, KEY_READ, &hkey)
                                         != ERROR_SUCCESS) {

                    // can't find group so delete it from the list box.
                    SendMessage(hwndUnlocked, LB_DELETESTRING, index, 0);
                    //
                    // delete it from the startup group combobox.
                    //
                    if ((i = (int)SendDlgItemMessage(hwnd, IDD_STARTUP, CB_FINDSTRING,
                                   (WPARAM)-1, (LPARAM)szText)) != CB_ERR) {
                        SendDlgItemMessage(hwnd, IDD_STARTUP, CB_DELETESTRING,
                                   i, 0);
                    }
                    index--;
                    cb--;
                }
                else
                    RegCloseKey(hkey);
            }
            index++;
        }


    index = 0;
    while (!RegEnumKeyEx(hkeyProgramGroups, index, szGroupKey, &cbGroupKey, 0, 0, 0, &ft)) {
        if (!cbGroupKey)
            goto NextKey;

        // check if it is in the the list boxes, if not ADD the new group
        // to the unlocked listbox.
        // need to find a way to remove groups that were deleted from
        // the program manager...
        bFoundIt = FALSE;
        //
        // check the locked group list box
        //

        if ((cb = (int)SendMessage(hwndLocked, LB_GETCOUNT, 0, 0)) != LB_ERR) {
            for (i = 0; i < cb; i++) {
                if ((INT_PTR)(lpGroupData = (LPGROUPDATA)SendMessage(hwndLocked, LB_GETITEMDATA, i, 0))
                                                 != LB_ERR) {

                    if (lpGroupData-> lpGroupKey &&
                              !lstrcmp(lpGroupData->lpGroupKey, szGroupKey)) {
                        bFoundIt = TRUE;
                        break;
                    }
                }
            }
        }
        if (bFoundIt)
            goto NextKey;
        //
        // check the unlocked group list box
        //
        if ((cb = (int)SendMessage(hwndUnlocked, LB_GETCOUNT, 0, 0)) != LB_ERR) {
            for (i = 0; i < cb; i++) {
                if ((INT_PTR)(lpGroupData = (LPGROUPDATA)SendMessage(hwndUnlocked, LB_GETITEMDATA, i, 0))
                                                 != LB_ERR) {
                    if (lpGroupData-> lpGroupKey &&
                              !lstrcmp(lpGroupData->lpGroupKey, szGroupKey)) {
                        bFoundIt = TRUE;
                        break;
                    }
                }
            }
        }
        if (bFoundIt)
            goto NextKey;

        //
        // didn't find the group, add it to one of the list boxes.
        //
        GetGroupName(hkeyProgramGroups, szGroupKey, szText);
        if (*szText) {
            if (!(lpGroupData = (LPGROUPDATA)GlobalAlloc(GPTR, sizeof(GROUPDATA))))
                goto NextKey;

            if (lpGroupData->lpGroupKey = (LPTSTR) GlobalAlloc(GPTR,
                                 (lstrlen(szGroupKey) + 1) * sizeof(TCHAR))) {
                lstrcpy(lpGroupData->lpGroupKey, szGroupKey);
            }

            //
            // Need to get the subkey's security to find out
            // if group goes in Unlocked or Locked list box.
            //
            if (lpGroupData->bOrgLock = IsGroupLocked(szGroupKey)) {
                if ((i = (int)SendMessage(hwndLocked, LB_ADDSTRING, 0, (LPARAM)szText)) != LB_ERR)
                    SendMessage(hwndLocked, LB_SETITEMDATA, i, (LPARAM)lpGroupData);
            }
            else {
                if ((i = (int)SendMessage(hwndUnlocked, LB_ADDSTRING, 0, (LPARAM)szText)) != LB_ERR)
                    SendMessage(hwndUnlocked, LB_SETITEMDATA, i, (LPARAM)lpGroupData);
            }

            //
            // add to the startup group combobox.
            //
            SendDlgItemMessage(hwnd, IDD_STARTUP, CB_ADDSTRING, 0, (LPARAM)szText);
        }
NextKey:
        index++;
        cbGroupKey = MAXKEYLEN;
    }
    //
    // Check it there is still something selected in the list boxes,
    // and enable/disable the Lock and Unlock buttons as apprpriate.
    //
    if (SendMessage(hwndLocked, LB_GETSELCOUNT, 0, 0) == 0) {
        EnableWindow(GetDlgItem(hwnd, IDD_UNLOCK), FALSE);
    }
    if (SendMessage(hwndUnlocked, LB_GETSELCOUNT, 0, 0) == 0) {
        EnableWindow(GetDlgItem(hwnd, IDD_LOCK), FALSE);
    }

    if (SendMessage(hwndUnlocked, LB_GETCOUNT, 0, 0) == 0) {
        //
        // Disable the listbox window.
        //
        EnableWindow(hwndUnlocked, FALSE);

        if (SendMessage(hwndLocked, LB_GETCOUNT, 0, 0) > 0) {
            EnableWindow(hwndLocked, TRUE);
            SetFocus(hwndLocked);
        }
        else {
            EnableWindow(hwndLocked, FALSE);
            SetFocus(GetDlgItem(hwnd, IDD_NORUN));
        }
    }
    else {
        EnableWindow(hwndUnlocked, TRUE);
        SetFocus(hwndUnlocked);
    }

    //
    // Start the notification again.
    //
    RegNotifyChangeKeyValue(hkeyProgramGroupsCurrent, TRUE, REG_NOTIFY_CHANGE_NAME,
                                 hProgramGroupsEvent, TRUE);
}


/***************************************************************************\
* EnablePrivilege
*
* Enables/disabled the specified well-known privilege in the
* current process context
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
\***************************************************************************/
BOOL
EnablePrivilege(
    DWORD Privilege,
    BOOL Enable
    )
{
    NTSTATUS Status;
#if 0
    BOOL WasEnabled;
    Status = RtlAdjustPrivilege(Privilege, Enable, TRUE, (PBOOLEAN)&WasEnabled);
    return(NT_SUCCESS(Status));
#else
    HANDLE ProcessToken;
    LUID LuidPrivilege;
    PTOKEN_PRIVILEGES NewPrivileges;
    DWORD Length;

    //
    // Open our own token
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_ADJUST_PRIVILEGES,
                 &ProcessToken
                 );
    if (!NT_SUCCESS(Status)) {
        KdPrint(("Upedit: Can't open own process token for adjust_privilege access\n\r"));
        return(FALSE);
    }

    //
    // Initialize the privilege adjustment structure
    //

    LuidPrivilege = RtlConvertLongToLuid(Privilege);

    NewPrivileges = (PTOKEN_PRIVILEGES) LocalAlloc(LPTR, sizeof(TOKEN_PRIVILEGES) +
                         (1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES));
    if (NewPrivileges == NULL) {
        KdPrint(("UPEdit failed to allocate memory for new privileges\n\r"));
        NtClose(ProcessToken);
        return(FALSE);
    }

    NewPrivileges->PrivilegeCount = 1;
    NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    //
    // BUG BUG because of a bug in NtAdjustPrivileges which
    // returns an error when you try to enable a privilege
    // that is already enabled, we first try to disable it.
    // to be removed when api is fixed.
    //
    NewPrivileges->Privileges[0].Attributes = 0;

    Status = NtAdjustPrivilegesToken(
                 ProcessToken,                     // TokenHandle
                 (BOOLEAN)FALSE,                   // DisableAllPrivileges
                 NewPrivileges,                    // NewPrivileges
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &Length                           // ReturnLength
                 );

    NewPrivileges->Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;
    //
    // Enable the privilege
    //
    Status = NtAdjustPrivilegesToken(
                 ProcessToken,                     // TokenHandle
                 (BOOLEAN)FALSE,                   // DisableAllPrivileges
                 NewPrivileges,                    // NewPrivileges
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &Length                           // ReturnLength
                 );

    LocalFree(NewPrivileges);

    NtClose(ProcessToken);

    if (Status) {
        KdPrint(("UPEdit failed to enable privilege\n\r"));
        return(FALSE);
    }

    return(TRUE);
#endif
}

/***************************************************************************\
*
*
\***************************************************************************/
BOOL APIENTRY GetCurrentUserSid(PVOID *pCurrentUserSid)
{
  HANDLE hToken;
  DWORD BytesRequired = 0;
  PTOKEN_USER pUserToken;
  NTSTATUS status;

  if (!OpenProcessToken(GetCurrentProcess(),
                       TOKEN_QUERY,
                       &hToken) ){
      return(FALSE);
  }

  //
  // Get space needed for token information
  //
  if (!GetTokenInformation(hToken,
                           TokenUser,
                           NULL,
                           0,
                           &BytesRequired) ) {

      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
          return(FALSE);
      }
  }

  //
  // Get the actual token information
  //
  pUserToken = (PTOKEN_USER)LocalAlloc(LPTR, BytesRequired);
  if (pUserToken == NULL) {
      return(FALSE);
  }
  if (!GetTokenInformation(hToken,
                           TokenUser,
                           pUserToken,
                           BytesRequired,
                           &BytesRequired) ) {
      LocalFree(pUserToken);
      return(FALSE);
  }

  BytesRequired = RtlLengthSid(pUserToken->User.Sid);
  *pCurrentUserSid = (PSID)LocalAlloc(LPTR, BytesRequired);
  if (*pCurrentUserSid == NULL) {
      LocalFree(pUserToken);
      return(TRUE);
  }

  status = RtlCopySid(BytesRequired, *pCurrentUserSid, pUserToken->User.Sid);
  if (!NT_SUCCESS(status)) {
      LocalFree(*pCurrentUserSid);
      *pCurrentUserSid = NULL;
      return(FALSE);
  }

  LocalFree(pUserToken);
  return(TRUE);

}

/***************************************************************************\
*
*
\***************************************************************************/
BOOL APIENTRY GetUserOrGroup(PVOID *pUserOrGroupSID, LPTSTR lpUserOrGroupName, DWORD cb)
{

    USERBROWSER UserBrowser;
    UCHAR chUserDetailsBuffer[4096 + 1];
    PUSERDETAILS pUserDetails;
    WCHAR szBrowserTitle[100];
    TCHAR szBrowserError[100];
    DWORD cbData = 4096;
    HUSERBROW hUserBrowser;
    ULONG BytesRequired;
    NTSTATUS Status;


    LoadStringW(hInst, IDS_BROWSERTITLE, szBrowserTitle, sizeof(szBrowserTitle));

    UserBrowser.ulStructSize = sizeof(USERBROWSER);
    UserBrowser.fUserCancelled = FALSE;
    UserBrowser.fExpandNames = FALSE;
    UserBrowser.hwndOwner = hwndUPE;
    UserBrowser.pszTitle = szBrowserTitle;
    UserBrowser.pszInitialDomain = NULL;
    UserBrowser.ulHelpContext = IDH_USERBROWSERDLG;
    UserBrowser.pszHelpFileName = L"UPEDIT.HLP";
    UserBrowser.Flags =USRBROWS_SINGLE_SELECT |
                        USRBROWS_INCL_REMOTE_USERS |
                        USRBROWS_INCL_INTERACTIVE |
                        USRBROWS_INCL_EVERYONE |
                        USRBROWS_SHOW_ALL;
    hUserBrowser = OpenUserBrowser(&UserBrowser);

    if (!hUserBrowser) {
        if (GetLastError()) {
            LoadString(hInst, IDS_BROWSERERROR, szBrowserError, sizeof(szBrowserError));
            MessageBox(hwndUPE, szBrowserError, szUPETitle,
                   MB_OK | MB_ICONEXCLAMATION );

        }
        return(FALSE);
    }

    pUserDetails = (PUSERDETAILS)chUserDetailsBuffer;

    if (!EnumUserBrowserSelection(hUserBrowser, pUserDetails, &cbData)) {

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            CloseUserBrowser(hUserBrowser);
            return(FALSE);
        }

        pUserDetails = (PUSERDETAILS)LocalAlloc(LPTR, cbData + 2);
        if (!pUserDetails) {
            CloseUserBrowser(hUserBrowser);
            return(FALSE);
        }

        if (!EnumUserBrowserSelection(hUserBrowser, pUserDetails, &cbData)) {
            GetLastError();
            LocalFree(pUserDetails);
            CloseUserBrowser(hUserBrowser);
            return(FALSE);
        }
    }

    //
    // Get the user or group sid, and name to display in editfield
    //
    BytesRequired = RtlLengthSid(pUserDetails->psidUser);
    if (*pUserOrGroupSID) {
        *pUserOrGroupSID = (PSID)LocalReAlloc(*pUserOrGroupSID, BytesRequired, LMEM_MOVEABLE);
    }
    else {
        *pUserOrGroupSID = (PSID)LocalAlloc(LPTR, BytesRequired);
    }
    if (*pUserOrGroupSID == NULL) {
        CloseUserBrowser(hUserBrowser);
        return(FALSE);
    }

    Status = RtlCopySid(BytesRequired, *pUserOrGroupSID, pUserDetails->psidUser);
    if (!NT_SUCCESS(Status)) {
        CloseUserBrowser(hUserBrowser);
        LocalFree(*pUserOrGroupSID);
        *pUserOrGroupSID = NULL;
        return(FALSE);
    }

#ifdef UNICODE

    lstrcpy(lpUserOrGroupName, pUserDetails->pszDomainName);
    lstrcat(lpUserOrGroupName, L"\\");
    lstrcat(lpUserOrGroupName, pUserDetails->pszAccountName);

#else
    {
    LPSTR lpName;

    lpName = (LPSTR)LocalAlloc(LPTR, cb);
    RtlUnicodeToMultiByteN(lpName, cb, &cbData,
                                pUserDetails->pszDomainName,
                                lstrlenW(pUserDetails->pszDomainName) * sizeof(WCHAR));
    lstrcpy(lpUserOrGroupName, lpName);
    lstrcat(lpUserOrGroupName, "\\");
    RtlUnicodeToMultiByteN(lpName, cb, &cbData,
                                pUserDetails->pszAccountName,
                                lstrlenW(pUserDetails->pszAccountName) * sizeof(WCHAR));
    *(lpName+cbData) = 0;
    lstrcat(lpUserOrGroupName, lpName);
    LocalFree(lpName);
    }
#endif
    CloseUserBrowser(hUserBrowser);


    return(TRUE);
}
/***************************************************************************\
*
*
\***************************************************************************/
BOOL LockGroups(BOOL bResetOriginalLock)
{
    INT   i,j;
    INT   iSelCount;
    LPGROUPDATA lpGroupData;
    TCHAR szGroupName[(MAXGROUPNAMELEN + 1)*sizeof(TCHAR)];
    TCHAR szLockError[(MAXMESSAGELEN + 1)*sizeof(TCHAR)];
    TCHAR szT[(MAXMESSAGELEN + 1)*sizeof(TCHAR)];
    UNICODE_STRING UnicodeName;
    HWND hwndLB;
    NTSTATUS Status;

    if (bResetOriginalLock) {
        LoadString(hInst, IDS_RESETLOCKERROR, szLockError, sizeof(szLockError));
        j = IDD_UNLOCKEDGRPS;
    }
    else {
        LoadString(hInst, IDS_LOCKERROR, szLockError, sizeof(szLockError));
        j = IDD_LOCKEDGRPS;
    }
  for (; j <= IDD_LOCKEDGRPS; j++) {

    hwndLB = GetDlgItem(hwndUPE, j);

    iSelCount = (int)SendMessage(hwndLB, LB_GETCOUNT, 0, 0);
    for (i = 0; i < iSelCount; i++) {
        SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)szGroupName);
        lpGroupData = (LPGROUPDATA)SendMessage(hwndLB, LB_GETITEMDATA, i, 0);
        if (!lpGroupData || !(lpGroupData->lpGroupKey)) {
            goto Error;
        }

        if (bResetOriginalLock && !lpGroupData->bOrgLock) {
            continue;
        }

        RtlInitUnicodeString(&UnicodeName, lpGroupData->lpGroupKey);

        //
        // Change the protection on the group key.
        //
        Status = MakeKeyUserAdminWriteableOnly(hkeyProgramGroups, &UnicodeName);

        if (!NT_SUCCESS(Status)) {
Error:
            /* Could not Lock group %s */
            wsprintf(szT, szLockError, szGroupName);
            if (MessageBox(hwndUPE, szT, szUPETitle,
                   MB_OKCANCEL | MB_ICONEXCLAMATION | MB_SYSTEMMODAL) == IDCANCEL)
                return(FALSE);
            else
                continue;
        }
    }
  }

    //
    // Also protect the subkeys under
    // Software\Microsoft\windowsNT\CurrentVersion\ProgramManager\Restriction
    //
    RtlInitUnicodeString(&UnicodeName, szRestrict);

    Status = MakeKeyUserAdminWriteableOnly(hkeyProgramManager, &UnicodeName);

    return(TRUE);
}

/***************************************************************************\
* InitializeGlobalSids
*
* Initializes the various global Sids used in this module.
*
* History:
* 04-28-93 JohanneC       Created
\***************************************************************************/
VOID InitializeGlobalSids()
{
    NTSTATUS Status;

    //
    // Build the admins local group SID
    //

    Status = RtlAllocateAndInitializeSid(
                 &gNtAuthority,
                 2,
                 SECURITY_BUILTIN_DOMAIN_RID,
                 DOMAIN_ALIAS_RID_ADMINS,
                 0, 0, 0, 0, 0, 0,
                 &gAdminsLocalGroup
                 );

    //
    // create System Sid
    //

    Status = RtlAllocateAndInitializeSid(
                   &gNtAuthority,
                   1,
                   SECURITY_LOCAL_SYSTEM_RID,
                   0, 0, 0, 0, 0, 0, 0,
                   &gSystemSid
                   );

}



NTSTATUS
MakeKeyUserAdminWriteableOnly(
    IN HANDLE RootKey,
    IN PUNICODE_STRING RelativeName
    )


/*++

Routine Description:

    This function queries the specified registry key's DACL
    and changes all AccessAllowed ACEs (except those for the
    administrators local group) so that they grant read access
    only.


Arguments:

    RootKey - A handle to a registy key.

    RelativeName - The name of the sub-key whose protection is to be
        changed to read-only.


Return Value:

    STATUS_SUCCESSFUL - The protection has been changed.

    STATUS_NO_MEMORY - Memory could not be allocated to perform the
        operation.

    Any other status is returned by services while trying to apply the
        new protection.  In this case, the protection left on the
        profile is either the original protection or the new protection.


--*/

{
    NTSTATUS Status, TempStatus;
    HANDLE Key;
    ULONG LengthNeeded, AceIndex;
    PACL Acl;
    PACE_HEADER Ace;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    ULONG StackBuffer[140];
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN DaclPresent, IgnoreBoolean;


    if (!gAdminsLocalGroup) {
        return(STATUS_INVALID_SID);
    }


    //
    // Open the specified key
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        RelativeName,
        OBJ_CASE_INSENSITIVE,
        RootKey,
        NULL
        );

    Status = NtOpenKey (
                 &Key,
                 READ_CONTROL | WRITE_DAC,
                 &ObjectAttributes
                 );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    //
    // Read the existing ACL
    //

    SecurityDescriptor = (PSECURITY_DESCRIPTOR)&StackBuffer[0];
    Status = NtQuerySecurityObject(
                 Key,
                 DACL_SECURITY_INFORMATION,
                 SecurityDescriptor,
                 140*sizeof(ULONG),         //size of stack buffer
                 &LengthNeeded
                 );

    if (Status == STATUS_BUFFER_OVERFLOW ||
        Status == STATUS_BUFFER_TOO_SMALL) {

        //
        // Stack buffer wasn't large enough - allocate one.
        //
        SecurityDescriptor = RtlAllocateHeap( RtlProcessHeap(), 0, LengthNeeded);

        Status = NtQuerySecurityObject(
                     Key,
                     DACL_SECURITY_INFORMATION,
                     SecurityDescriptor,
                     LengthNeeded,
                     &LengthNeeded
                     );

    }



    //
    // See if there is an acl
    //

    if (NT_SUCCESS(Status)) {
        Status = RtlGetDaclSecurityDescriptor(
                     SecurityDescriptor,
                     &DaclPresent,
                     &Acl,
                     &IgnoreBoolean
                     );
        ASSERT( NT_SUCCESS(Status) );       // don't expect this to be able to fail

        if (DaclPresent) {
            if (Acl != NULL) {

                //
                // Walk the acl changing each ACCESS_ALLOWED ace's AccessMask
                // (except those granting access to Administrators).
                //

                for (AceIndex = 0;
                     AceIndex < (ULONG)Acl->AceCount && NT_SUCCESS(Status);
                     AceIndex++) {

                    Status = RtlGetAce ( Acl, AceIndex, (PVOID *)&Ace );
                    ASSERT( NT_SUCCESS(Status) );  //Shouldn't be able to fail

                    //
                    // Change to read-only if AccessAllowed and not
                    // administrators
                    //

                    if (Ace->AceType == ACCESS_ALLOWED_ACE_TYPE) {
                        if (!RtlEqualSid(
                                (PSID)&(((PACCESS_ALLOWED_ACE)Ace)->SidStart),
                                gAdminsLocalGroup)
                                        ) {
                            //
                            // Change it to read-only
                            //

                            ((PACCESS_ALLOWED_ACE)Ace)->Mask &= ~(KEY_WRITE | DELETE);

                            //There's some overlap in KEY_READ and KEY_WRITE
                            ((PACCESS_ALLOWED_ACE)Ace)->Mask |= KEY_READ;

                        } //end_if Sid is gAdminsLocalGroup
                    } //end_if ACE is AccessAllowed type
                } //end_for (loop through ACEs)

                //
                // We've changed the ACL
                // Now try to put it back on the key.
                //

                Status = NtSetSecurityObject(
                             Key,
                             DACL_SECURITY_INFORMATION,
                             SecurityDescriptor
                             );

                //
                // Status now contains our completion status.
                // Careful not to accidently change it.
                //


            } //end_if DACL
        }
    }





    //
    // If we didn't use our stack buffer, free the allocated memory
    //

    if (SecurityDescriptor != (PSECURITY_DESCRIPTOR)&StackBuffer[0]) {

        RtlFreeHeap( RtlProcessHeap(), 0, SecurityDescriptor );
    }


    //
    // Close the key we opened
    //

    TempStatus = NtClose( Key );
#if DBG
    //
    // The close might fail for things like insufficient disk space
    // to store the registry change log.  But, there really isn't
    // much we can do about it, so just assert it in our debug system.
    //

    ASSERT(NT_SUCCESS(TempStatus));
#endif


    return(Status);


}

BOOL
IsKeyUserAdminWriteableOnly(
    IN HANDLE RootKey,
    IN PUNICODE_STRING RelativeName
    )


/*++

Routine Description:

    This function queries the specified registry key's DACL
    and returns TRUE if the AccessAllowed ACEs not belonging to the
    administrators local group is read access only.


Arguments:

    RootKey - A handle to a registy key.

    RelativeName - The name of the sub-key whose protection is to be
        tested for read-only.


Return Value:

    STATUS_SUCCESSFUL - The protection has been changed.

    STATUS_NO_MEMORY - Memory could not be allocated to perform the
        operation.

    Any other status is returned by services while trying to apply the
        new protection.  In this case, the protection left on the
        profile is either the original protection or the new protection.


--*/

{
    NTSTATUS Status, TempStatus;
    HANDLE Key;
    ULONG LengthNeeded, AceIndex;
    PACL Acl;
    PACE_HEADER Ace;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    ULONG StackBuffer[140];
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN DaclPresent;
    BOOLEAN IgnoreBoolean;
    BOOLEAN bReadOnly = FALSE;

    if (!gAdminsLocalGroup) {
        return(STATUS_INVALID_SID);
    }



    //
    // Open the specified key
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        RelativeName,
        OBJ_CASE_INSENSITIVE,
        RootKey,
        NULL
        );

    Status = NtOpenKey (
                 &Key,
                 READ_CONTROL | WRITE_DAC,
                 &ObjectAttributes
                 );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    //
    // Read the existing ACL
    //

    SecurityDescriptor = (PSECURITY_DESCRIPTOR)&StackBuffer[0];
    Status = NtQuerySecurityObject(
                 Key,
                 DACL_SECURITY_INFORMATION,
                 SecurityDescriptor,
                 140*sizeof(ULONG),         //size of stack buffer
                 &LengthNeeded
                 );

    if (Status == STATUS_BUFFER_OVERFLOW ||
        Status == STATUS_BUFFER_TOO_SMALL) {

        //
        // Stack buffer wasn't large enough - allocate one.
        //
        SecurityDescriptor = RtlAllocateHeap( RtlProcessHeap(), 0, LengthNeeded);

        Status = NtQuerySecurityObject(
                     Key,
                     DACL_SECURITY_INFORMATION,
                     SecurityDescriptor,
                     LengthNeeded,
                     &LengthNeeded
                     );

    }



    //
    // See if there is an acl
    //

    if (NT_SUCCESS(Status)) {
        Status = RtlGetDaclSecurityDescriptor(
                     SecurityDescriptor,
                     &DaclPresent,
                     &Acl,
                     &IgnoreBoolean
                     );
        ASSERT( NT_SUCCESS(Status) );       // don't expect this to be able to fail

        if (DaclPresent) {
            if (Acl != NULL) {

                //
                // Walk the acl changing each ACCESS_ALLOWED ace's AccessMask
                // (except those granting access to Administrators).
                //

                for (AceIndex = 0;
                     AceIndex < (ULONG)Acl->AceCount && NT_SUCCESS(Status);
                     AceIndex++) {

                    Status = RtlGetAce ( Acl, AceIndex, (PVOID *)&Ace );
                    ASSERT( NT_SUCCESS(Status) );  //Shouldn't be able to fail

                    //
                    // Change to read-only if AccessAllowed and not
                    // administrators
                    //

                    if (Ace->AceType == ACCESS_ALLOWED_ACE_TYPE) {
                        if (!RtlEqualSid(
                                (PSID)&(((PACCESS_ALLOWED_ACE)Ace)->SidStart),
                                gAdminsLocalGroup)
                                        ) {
                            //
                            // test for read-only
                            //

                            if (! (((PACCESS_ALLOWED_ACE)Ace)->Mask &
                                                  (KEY_WRITE | DELETE) & ~KEY_READ)) {
                                bReadOnly = TRUE;
                                break;
                            }

                        } //end_if Sid is gAdminsLocalGroup
                    } //end_if ACE is AccessAllowed type
                } //end_for (loop through ACEs)

            } //end_if DACL
        }
    }

    //
    // If we didn't use our stack buffer, free the allocated memory
    //

    if (SecurityDescriptor != (PSECURITY_DESCRIPTOR)&StackBuffer[0]) {

        RtlFreeHeap( RtlProcessHeap(), 0, SecurityDescriptor );
    }


    //
    // Close the key we opened
    //

    TempStatus = NtClose( Key );
#if DBG
    //
    // The close might fail for things like insufficient disk space
    // to store the registry change log.  But, there really isn't
    // much we can do about it, so just assert it in our debug system.
    //

    ASSERT(NT_SUCCESS(TempStatus));
#endif


    return(bReadOnly);


}

//
// Returns the security descriptor of the current profile root key. The sec.
// desc. will be used when exiting upedit, to reset the protection/permission
// on the keys in the current profile (the permission might have changed while
// running upedit).
//
BOOL GetCurrentProfileSecurityDescriptor(PSECURITY_DESCRIPTOR *pSecDesc)
{
    DWORD cbSecDesc = 0;
    LONG Error;

    Error = RegGetKeySecurity(hkeyCurrentUser,
                             DACL_SECURITY_INFORMATION,
                             *pSecDesc, &cbSecDesc);
    if (Error == ERROR_INSUFFICIENT_BUFFER) {
        *pSecDesc = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, cbSecDesc);
        if (*pSecDesc) {
            Error = RegGetKeySecurity(hkeyCurrentUser,
                                  DACL_SECURITY_INFORMATION,
                                  *pSecDesc, &cbSecDesc);
        }
    }
    if (Error) {
        if (*pSecDesc) {
            LocalFree(*pSecDesc);
        }
        return(FALSE);
    }
    return(TRUE);
}

BOOL
GetSidFromOpenedProfile(
    PSID *pSid
    )

/*++

Routine Description:

    This function returns a pointer to the sid of the newly openned profile.
    THis is done by comparing the sid in the aces of one of the ACLs in the
    profile, if the sid is not admin or system, it's the one we want.

Arguments:

    pSid -

Return Value:

    BOOL - returns true if the sid was found, false otherwwise

--*/

{
    NTSTATUS Status, TempStatus;
    HANDLE RootKey, Key;
    ULONG LengthNeeded, AceIndex;
    PACL Acl;
    PACE_HEADER Ace;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    ULONG StackBuffer[140];
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NullName;
    BOOLEAN DaclPresent;
    BOOLEAN IgnoreBoolean;
    BOOLEAN OpenedCurrentUserKey;
    DWORD BytesRequired = 0;


    if (!gAdminsLocalGroup || !gSystemSid) {
        return(STATUS_INVALID_SID);
    }


    //
    // Open the specified key
    //
    RootKey = hkeyCurrentUser;
    if (RootKey == HKEY_CURRENT_USER) {
        // get the actual handle to the current user
        Status = RtlOpenCurrentUser(MAXIMUM_ALLOWED, &RootKey);

        if (!NT_SUCCESS(Status)) {
            return(Status);
        }

        OpenedCurrentUserKey = TRUE;
    } else {
        OpenedCurrentUserKey = FALSE;
    }


    RtlInitUnicodeString( &NullName, L"");
    InitializeObjectAttributes(
        &ObjectAttributes,
        &NullName,                  // Name is null to open same key
        OBJ_CASE_INSENSITIVE,
        RootKey,
        NULL
        );

    Status = NtOpenKey (
                 &Key,
                 READ_CONTROL | WRITE_DAC,
                 &ObjectAttributes
                 );
    if (!NT_SUCCESS(Status)) {
        if (OpenedCurrentUserKey) {
            NtClose( RootKey );
        }
        return(Status);
    }


    //
    // Read the existing ACL
    //

    SecurityDescriptor = (PSECURITY_DESCRIPTOR)&StackBuffer[0];
    Status = NtQuerySecurityObject(
                 Key,
                 DACL_SECURITY_INFORMATION,
                 SecurityDescriptor,
                 140*sizeof(ULONG),         //size of stack buffer
                 &LengthNeeded
                 );

    if (Status == STATUS_BUFFER_OVERFLOW ||
        Status == STATUS_BUFFER_TOO_SMALL) {

        //
        // Stack buffer wasn't large enough - allocate one.
        //
        SecurityDescriptor = RtlAllocateHeap( RtlProcessHeap(), 0, LengthNeeded);

        Status = NtQuerySecurityObject(
                     Key,
                     DACL_SECURITY_INFORMATION,
                     SecurityDescriptor,
                     LengthNeeded,
                     &LengthNeeded
                     );

    }


    //
    // See if there is an acl
    //

    if (NT_SUCCESS(Status)) {
        Status = RtlGetDaclSecurityDescriptor(
                     SecurityDescriptor,
                     &DaclPresent,
                     &Acl,
                     &IgnoreBoolean
                     );
        ASSERT( NT_SUCCESS(Status) );       // don't expect this to be able to fail

        if (DaclPresent) {
            if (Acl != NULL) {

                //
                // Walk the acl.
                //

                for (AceIndex = 0;
                     AceIndex < (ULONG)Acl->AceCount && NT_SUCCESS(Status);
                     AceIndex++) {

                    Status = RtlGetAce ( Acl, AceIndex, (PVOID *)&Ace );
                    ASSERT( NT_SUCCESS(Status) );  //Shouldn't be able to fail

                    //
                    // Get the sid that is not the admin sid
                    //

                    if (Ace->AceType == ACCESS_ALLOWED_ACE_TYPE) {
                        if (!RtlEqualSid(
                                (PSID)&(((PACCESS_ALLOWED_ACE)Ace)->SidStart),
                                gAdminsLocalGroup) &&
                            !RtlEqualSid(
                                (PSID)&(((PACCESS_ALLOWED_ACE)Ace)->SidStart),
                                gSystemSid)
                                        ) {

                                //
                                // we've got the sid
                                //
                                //
                                // Get the user or group sid, and name to
                                // display in editfield
                                //
                                BytesRequired = RtlLengthSid((PSID)&(((PACCESS_ALLOWED_ACE)Ace)->SidStart));
                                if (*pSid) {
                                     *pSid = (PSID)LocalReAlloc(*pSid,
                                                                BytesRequired,
                                                                LMEM_MOVEABLE);
                                }
                                else {
                                     *pSid = (PSID)LocalAlloc(LPTR, BytesRequired);
                                }
                                if (*pSid == NULL) {
                                    goto Exit;
                                }

                                Status = RtlCopySid(BytesRequired, *pSid,
                                        (PSID)&(((PACCESS_ALLOWED_ACE)Ace)->SidStart));
                                if (!NT_SUCCESS(Status)) {
                                    LocalFree(*pSid);
                                    *pSid = NULL;
                                    goto Exit;
                                }

                                break;

                        } //end_if Sid is gAdminsLocalGroup
                    } //end_if ACE is AccessAllowed type
                } //end_for (loop through ACEs)

            } //end_if DACL
        }
    }

    //
    // If we didn't use our stack buffer, free the allocated memory
    //

    if (SecurityDescriptor != (PSECURITY_DESCRIPTOR)&StackBuffer[0]) {

        RtlFreeHeap( RtlProcessHeap(), 0, SecurityDescriptor );
    }

Exit:
    if (OpenedCurrentUserKey) {
        NtClose( RootKey );
    }

    //
    // Close the key we opened
    //

    TempStatus = NtClose( Key );
#if DBG
    //
    // The close might fail for things like insufficient disk space
    // to store the registry change log.  But, there really isn't
    // much we can do about it, so just assert it in our debug system.
    //

    ASSERT(NT_SUCCESS(TempStatus));
#endif


    return(NT_SUCCESS(Status));


}



NTSTATUS
ApplyProfileProtection(
    IN PSECURITY_DESCRIPTOR pSecDesc,
    IN PSID UserOrGroup,
    IN HANDLE RootKey
    )


/*++

Routine Description:

    This function applies appropriate protection to an entire profile.
    That is, this function builds an ACL and applies that ACL to a
    sub-tree of the registry.  The RootKey parameter is assumed to be
    a handle to the root key of the profile tree in the registry.

Arguments:

    UserOrGroup - The SID of the user or group the profile will be used
        by.

    RootKey - A key open to the root of the registry tree in which the
        profile is loaded.


Return Value:

    STATUS_SUCCESSFUL - The protection has been applied.

    STATUS_NO_MEMORY - Memory could not be allocated to build the ACL.

    Any other status is returned by services while trying to apply the
        new protection.  In this case, the protection left on the
        profile is in an undefined state.  That is, it is not necessarily
        the original protection nor the new protection.


--*/

{
    NTSTATUS Status;
    ULONG AclLength;
    PACL Acl = NULL;
    PACE_HEADER Ace;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    BOOLEAN OpenedCurrentUserKey;

    //
    // If a security descriptor was passed in then use it to set the permission
    // on the key of the registry tree. If not create one using the UserOrGroup
    // sid.
    //

    if (pSecDesc) {
        Status = STATUS_SUCCESS;
        goto GotSecDesc;
    }

    //
    // A little debugging aid - only valid sids should come into this
    // routine.
    //

    if (!UserOrGroup || !RtlValidSid(UserOrGroup) ){
        //
        // Let admin know this sid in invalid and the profile will not
        // be saved.
        //
        return(STATUS_INVALID_SID);
    }


    if (!gAdminsLocalGroup) {
        return(STATUS_INVALID_SID);
    }



    //
    // Now calculate how long the ACL will be and allocate that much
    // memory.
    //

    AclLength = sizeof(ACL)                         + // An ACL
                2*sizeof(ACCESS_ALLOWED_ACE)        + // With 2 grant ACEs
                RtlLengthSid( UserOrGroup )         + // One with this sid
                RtlLengthSid( gAdminsLocalGroup );     // the other this sid

    Acl = RtlAllocateHeap( RtlProcessHeap(), 0, AclLength );

    if (Acl == NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, gAdminsLocalGroup );
        return(STATUS_NO_MEMORY);
    }

    //
    // Now build the ACL and set it in a security descriptor
    //
    //      Ace1:  { Grant; UserOrGroup; All access }
    //      Ace2:  { Grant; Admins;      All access }
    //

    Status = RtlCreateAcl( Acl, AclLength, ACL_REVISION2);
    if (NT_SUCCESS(Status)) {
        Status = RtlAddAccessAllowedAce (
                     Acl,
                     ACL_REVISION2,
                     GENERIC_ALL,
                     UserOrGroup
                     );
        if (NT_SUCCESS(Status)) {
            Status = RtlAddAccessAllowedAce (
                         Acl,
                         ACL_REVISION2,
                         GENERIC_ALL,
                         gAdminsLocalGroup
                         );
        }
        if (NT_SUCCESS(Status)) {

            //
            // Mark the ACEs as inheritable and put the ACL
            // in a security descriptor.  We just added the
            // two ACEs, so we can assert SUCCESS when we
            // look them up.
            //

            Status = RtlGetAce ( Acl, 0, (PVOID *)&Ace );
            ASSERT( NT_SUCCESS(Status) );
            Ace->AceFlags |= CONTAINER_INHERIT_ACE;
            Status = RtlGetAce ( Acl, 1, (PVOID *)&Ace );
            ASSERT( NT_SUCCESS(Status) );
            Ace->AceFlags |= CONTAINER_INHERIT_ACE;

            Status = RtlCreateSecurityDescriptor (
                         &SecurityDescriptor,
                         SECURITY_DESCRIPTOR_REVISION1
                         );
            ASSERT(NT_SUCCESS(Status));             // Shouldn't fail.

            Status = RtlSetDaclSecurityDescriptor (
                         &SecurityDescriptor,
                         TRUE,                      //DaclPresent,
                         Acl,                       //Dacl,
                         FALSE                      //DaclDefaulted
                         );
            ASSERT(NT_SUCCESS(Status));             // Shouldn't fail.

            pSecDesc = &SecurityDescriptor;
        }
    }

GotSecDesc:

    //
    // If we have the ACL built, then apply it to the tree.
    //

    if (NT_SUCCESS(Status)) {

        if (RootKey == HKEY_CURRENT_USER) {
            // get the actual handle to the current user
            Status = RtlOpenCurrentUser(MAXIMUM_ALLOWED, &RootKey);
            OpenedCurrentUserKey = TRUE;

        } else {

            OpenedCurrentUserKey = FALSE;
        }

        if (NT_SUCCESS(Status)) {
            Status = ApplyAclToRegistryTree( pSecDesc, RootKey );

            if (OpenedCurrentUserKey) {
                NtClose( RootKey );
            }
        }
    }

    //
    // Free allocated heap
    //
    if (Acl) {
        RtlFreeHeap( RtlProcessHeap(), 0, Acl );
    }

    return(Status);
}




NTSTATUS
ApplyAclToRegistryTree (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE RootKey
    )

/*++

Routine Description:

    This function applies the provided ACL to an entire registry
    subtree.

    In the case of failure, the ACL may have been applied to some
    keys in the tree.

Arguments:

    SecurityDescriptor - points to a security descriptor containing
        DACL to be applied.


    RootKey - A key open to the root of the registry tree to which
        the ACL is to be applied.


Return Value:

    STATUS_SUCCESSFUL - The ACL has been applied.

    Any other status is returned by services while trying to apply the
        new protection.  In this case, the protection left on the
        profile is in an undefined state.  That is, it is not necessarily
        the original protection nor the new protection.


--*/

{
    NTSTATUS Status;


    //
    // The passed key must successfully allow us to apply the ACL
    // to the root of the tree.
    //

    Status = NtSetSecurityObject(
                 RootKey,
                 DACL_SECURITY_INFORMATION,
                 SecurityDescriptor
                 );

    //
    // Now apply it to the children
    //

    if (NT_SUCCESS(Status)) {
        Status = ApplyAclToChildren( SecurityDescriptor, RootKey );
    }

    return(Status);

}



NTSTATUS
ApplyAclToChildren (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE Parent
    )

/*++

Routine Description:

    This function applies the provided ACL to all ancestors
    of a provided parent registry key.

    In the case of failure, the ACL may have been applied to some
    keys in the tree.

Arguments:

    SecurityDescriptor - points to a security descriptor containing
        DACL to be applied.

    Parent - A key open to the parent key.


Return Value:

    STATUS_SUCCESSFUL - The ACL has been applied.

    Any other status is returned by services while trying to apply the
        new protection.  In this case, the protection left on the
        profile is in an undefined state.  That is, it is not necessarily
        the original protection nor the new protection.


--*/

{
    NTSTATUS Status, IgnoreStatus;
    ULONG Index, KeyInfoBuffer[200], RequiredLength;
    PKEY_BASIC_INFORMATION KeyInfo;
    HANDLE EffectiveKey, Child;
    UNICODE_STRING NullName, ChildName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    OBJECT_BASIC_INFORMATION HandleInformation;

    //
    // OK, we have:
    //
    //      1) A handle to our parent key.  Assuming this key
    //         may have had its protection just changed, it is
    //         not known whether the original protection grants
    //         us the ability to list sub-keys.  If not, we
    //         must open the key again and try it with the new
    //         protection.
    //
    // We must be able to list the sub-keys to continue working.
    // Furthermore, we will reach this same state at every level
    // of our tree traversal.
    //

    //
    // Get a handle we can use for enumeration
    // (the one we were passed might work, otherwise try openning one).
    //

    EffectiveKey = Parent;
    Status = NtQueryObject(
                 Parent,
                 ObjectBasicInformation,
                 &HandleInformation,
                 sizeof(OBJECT_BASIC_INFORMATION),
                 NULL
                 );
    ASSERT(NT_SUCCESS(Status)); // I don't expect this to be able to fail

    if (!(HandleInformation.GrantedAccess & KEY_ENUMERATE_SUB_KEYS)) {

        //
        // Try opening the key again.
        //

        RtlInitUnicodeString( &NullName, L"");
        InitializeObjectAttributes(
            &ObjectAttributes,
            &NullName,                  // Name is null to open same key
            OBJ_CASE_INSENSITIVE,       // Attributes
            Parent,                     // Root key
            NULL                        // SecurityDescriptor
            );
        Status = NtOpenKey (
                     &EffectiveKey,
                     KEY_ENUMERATE_SUB_KEYS,
                     &ObjectAttributes
                     );
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }



    }


    //
    // OK, we have a handle that we can use for enumeration.
    //



    InitializeObjectAttributes(
        &ObjectAttributes,
        &ChildName,                 // Name gets initialized inside the loop.
        OBJ_CASE_INSENSITIVE,       // Attributes
        Parent,                     // Root key
        NULL                        // SecurityDescriptor
        );
    Index = 0;
    while (NT_SUCCESS(Status)) {
        KeyInfo = (PKEY_BASIC_INFORMATION)&KeyInfoBuffer[0];
        Status = NtEnumerateKey (
                     EffectiveKey,
                     Index,
                     KeyBasicInformation,
                     (PVOID)KeyInfo,
                     (200*sizeof(ULONG)),
                     &RequiredLength
                     );
        Index ++;

        if (NT_SUCCESS(Status)) {

            //
            // Get the relative name so we can open this key
            //

            ChildName.Buffer = &KeyInfo->Name[0];
            ChildName.Length = (USHORT)KeyInfo->NameLength;
            ChildName.MaximumLength = (USHORT)KeyInfo->NameLength;

            //
            // Open this key and apply protection to this sub-tree
            // This must allow us to replace the DAC, and might also
            // allow us to enumerate sub-keys.
            //

            Status = NtOpenKey(
                        &Child,
                        MAXIMUM_ALLOWED | WRITE_DAC,
                        &ObjectAttributes
                        );

            if (NT_SUCCESS(Status)) {
                Status = ApplyAclToRegistryTree( SecurityDescriptor, Child);
                IgnoreStatus = NtClose( Child );
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }
        }



    }

    if (Status == STATUS_NO_MORE_ENTRIES) {
        Status = STATUS_SUCCESS;
    }


    return(Status);

}

/***************************************************************************\
* CreateSecurityDescriptorForFile
*
* Creates a security descriptor containing an ACL containing the ACE from
* the directory where the file is to be saved. Other ACE ia added: an
* ACE allowing the user read access if the profile is mandatory or
* read/write access if the profile is a personal profile.
*
* A SD created with this routine should be destroyed using
* DeleteSecurityDescriptor
*
* Returns a pointer to the security descriptor or NULL on failure.
*
* 12-14-93 Davidc       Created.
\***************************************************************************/

PSECURITY_DESCRIPTOR CreateSecurityDescriptorForFile(PSID pSid, LPTSTR lpFile)
{
    TCHAR lptmp[MAX_PATH];
    LPTSTR lpFilePart;
    TCHAR ch;
    PSECURITY_DESCRIPTOR pSecDesc = NULL;
    PSECURITY_DESCRIPTOR pSecDescNew = NULL;
    PACL pAcl;
    PACL pAclNew = NULL;
    BOOLEAN fDaclPresent, fDaclDefaulted;
    PACE_HEADER pAce;
    DWORD dwAccessMask;
    DWORD cb;
    INT i;
    NTSTATUS Status;

    //
    // Get the directory security and propagate it to the file, if it doesn't
    // exist, then use the default security descriptor.
    //
    GetFullPathName(lpFile, MAX_PATH, lptmp, &lpFilePart);
    ch = *lpFilePart;
    *lpFilePart = 0;

    GetFileSecurity(lptmp, DACL_SECURITY_INFORMATION, pSecDesc, 0, &cb);
    if (!cb)
        goto Exit;

    pSecDesc = LocalAlloc(LPTR, cb);
    if (!pSecDesc)
        goto Exit;
    if (!GetFileSecurity(lptmp, DACL_SECURITY_INFORMATION, pSecDesc, cb, &cb))
        goto Exit;

    *lpFilePart = ch;

    //
    // Get the security descriptor's acl.
    //
    Status = RtlGetDaclSecurityDescriptor(pSecDesc, &fDaclPresent, &pAcl, &fDaclDefaulted);
    if (!NT_SUCCESS(Status) || !fDaclPresent || !pAcl){
        goto Exit;
    }

    cb = pAcl->AclSize + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSid);

    if (pAclNew = LocalAlloc(LPTR, cb)) {
        memcpy(pAclNew, pAcl, cb);
        pAclNew->AclSize = (WORD)cb;
        pAcl = pAclNew;
    }

    //
    // Remove the directory specific flags from the existent aces.
    //
    for (i=0; i<pAcl->AceCount; i++) {
        if (RtlGetAce(pAcl, i, (PVOID *)&pAce) == STATUS_SUCCESS)
            pAce->AceFlags = 0;
    }

    //
    // Set access mask for new ACE.
    //
    lpFilePart += (lstrlen(lpFilePart) - 4); // go to file extension
    if (!lstrcmpi(lpFilePart, TEXT(".man"))) {
        dwAccessMask = GENERIC_READ | GENERIC_EXECUTE;
    }
    else {
        dwAccessMask = GENERIC_READ | GENERIC_EXECUTE | GENERIC_WRITE | DELETE;
    }

    Status = RtlAddAccessAllowedAce(pAcl, ACL_REVISION, dwAccessMask, pSid);
    if (!NT_SUCCESS(Status))
        goto Exit;

    //
    // Create the new security descriptor for the profile file.
    //
    pSecDescNew = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (!pSecDescNew) {
        LocalFree(pAclNew);
        goto Exit;
    }

    Status = RtlCreateSecurityDescriptor(pSecDescNew, SECURITY_DESCRIPTOR_REVISION);

    Status = RtlSetDaclSecurityDescriptor(pSecDescNew, (BOOLEAN)TRUE, pAcl, fDaclDefaulted);

Exit:
    LocalFree(pSecDesc);
    return(pSecDescNew);

}

/***************************************************************************\
* DeleteSecurityDescriptor
*
* Deletes a security descriptor created using CreateSecurityDescriptorForFile
*
* Returns TRUE on success, FALSE on failure
*
\***************************************************************************/

BOOL
DeleteSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    NTSTATUS Status;
    PACL    pAcl;
    BOOLEAN Present;
    BOOLEAN Defaulted;

    //
    // Get the ACL
    //
    Status = RtlGetDaclSecurityDescriptor(pSecurityDescriptor,
                                          &Present, &pAcl, &Defaulted);
    if (NT_SUCCESS(Status)) {

        //
        // Destroy the ACL
        //
        if (Present && (pAcl != NULL)) {
            LocalFree(pAcl);
        }
    }

    //
    // Destroy the Security Descriptor
    //
    LocalFree(pSecurityDescriptor);

    return(TRUE);
}
