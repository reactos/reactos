/****************************** Module Header ******************************\
* Module Name: logon.c
*
* Copyright (c) 1992, Microsoft Corporation
*
* Handles loading and unloading user profiles.
*
* History:
* 2-25-92 JohanneC       Created -
*
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <lmjoin.h>


/////////////////////////////////////////////////////////////////////////
//
// Global variables for this module.
//
/////////////////////////////////////////////////////////////////////////
PSID gGuestsDomainSid = NULL;
SID_IDENTIFIER_AUTHORITY gNtAuthority = SECURITY_NT_AUTHORITY;


//
// Defines
//

#define GPO_SCRIPT_VARIALBE_NAME   TEXT("UserInitGPOScriptType")

//
// Types
//

typedef struct {
    HANDLE hToken;
    PTERMINAL pTerm;
} ADMIN_PW_ARGS, *PADMIN_PW_ARGS;

//
// Function proto-types
//

BOOL ExecuteGPOScripts (PTERMINAL pTerm, BOOL bUser, BOOL bSync,
                        LPWSTR lpDesktop, PVOID pEnvironment);

BOOL CheckForGPOScripts (HKEY hKeyRoot, LPTSTR lpScriptType);


LRESULT
WINAPI
AdminPwDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );
/***************************************************************************\
* IsUserAGuest
*
* returns TRUE if the user is a member of the Guests domain. If so, no
* cached profile should be created and when the profile is not available
* use the user default  profile.
*
* History:
* 04-30-93  Johannec     Created
*
\***************************************************************************/
BOOL IsUserAGuest(PWINDOWSTATION pWS)
{
    NTSTATUS Status;
    ULONG InfoLength;
    PTOKEN_GROUPS TokenGroupList;
    ULONG GroupIndex;
    BOOL FoundGuests;


    if (TestTokenForAdmin(pWS->UserProcessData.UserToken)) {
        //
        // The user is an admin, ignore the fact that the user could be a
        // guest too.
        //
        return(FALSE);
    }
    if (!gGuestsDomainSid) {

        //
        // Create Guests domain sid.
        //
        Status = RtlAllocateAndInitializeSid(
                   &gNtAuthority,
                   2,
                   SECURITY_BUILTIN_DOMAIN_RID,
                   DOMAIN_ALIAS_RID_GUESTS,
                   0, 0, 0, 0, 0, 0,
                   &gGuestsDomainSid
                   );
    }

    //
    // Test if user is in the Guests domain
    //

    //
    // Get a list of groups in the token
    //

    Status = NtQueryInformationToken(
                 pWS->UserProcessData.UserToken,      // Handle
                 TokenGroups,              // TokenInformationClass
                 NULL,                     // TokenInformation
                 0,                        // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );

    if ((Status != STATUS_SUCCESS) && (Status != STATUS_BUFFER_TOO_SMALL)) {

        DebugLog((DEB_ERROR, "failed to get group info for guests token, status = 0x%lx", Status));
        return(FALSE);
    }


    TokenGroupList = Alloc(InfoLength);

    if (TokenGroupList == NULL) {
        DebugLog((DEB_ERROR, "unable to allocate memory for token groups\n"));
        return(FALSE);
    }

    Status = NtQueryInformationToken(
                 pWS->UserProcessData.UserToken,      // Handle
                 TokenGroups,              // TokenInformationClass
                 TokenGroupList,           // TokenInformation
                 InfoLength,               // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );

    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "failed to query groups for guests token, status = 0x%lx", Status));
        Free(TokenGroupList);
        return(FALSE);
    }


    //
    // Search group list for guests alias
    //

    FoundGuests = FALSE;

    for (GroupIndex=0; GroupIndex < TokenGroupList->GroupCount; GroupIndex++ ) {

        if (RtlEqualSid(TokenGroupList->Groups[GroupIndex].Sid, gGuestsDomainSid)) {
            FoundGuests = TRUE;
            break;
        }
    }

    //
    // Tidy up
    //

    Free(TokenGroupList);

    return(FoundGuests);
}

/***************************************************************************\
* MergeProfiles
*
* Asks the user if they want to merge their local and domain profiles.  If
* they say Yes, then we attempt to merge the profiles.  If that fails, we
* prompt for Administrator credentials.
*
* Returns ERROR_SUCCESS if the merge was successful, or if the user does not
* want to merge the profiles.  Returns ERROR_REQUEST_ABORTED if logon should
* continue, but the profile merge could not be completed.  Returns an error
* code otherwise, which causes the logon to stop.
*
* History:
* 25-May-1999   jimschm     Created
*
\***************************************************************************/

NTSTATUS
MergeProfiles (
    PTERMINAL pTerm
    )
{
    PWINDOWSTATION pWS = pTerm->pWinStaWinlogon;
    DWORD rc;
    PCTSTR ArgArray[2];
    PTSTR MsgBuf;
    TCHAR Caption[MAX_PATH];
    TCHAR StackMsgBuf[MAX_PATH];
    DWORD Size;
    TCHAR Computer[MAX_PATH];
    TCHAR LocalUser[MAX_PATH];
    TCHAR DomainUser[MAX_PATH];
    DWORD Error;
    BOOL b;
    WCHAR Password[MAX_PATH];
    ADMIN_PW_ARGS PwArgs;
    BOOL LogoffIfError = FALSE;
    HCURSOR hOldCursor;

    //
    // Build strings
    //

    Size = MAX_PATH;
    if (!GetComputerName (Computer, &Size)) {
        return ERROR_REQUEST_ABORTED;
    }

    Size = lstrlen (Computer) + 2 + lstrlen (pWS->UserName);
    if (Size > MAX_PATH) {
        return ERROR_REQUEST_ABORTED;
    }

    wsprintf (LocalUser, TEXT("%s\\%s"), Computer, pWS->UserName);

    Size = lstrlen (pWS->Domain) + 2 + lstrlen (pWS->UserName);
    if (Size > MAX_PATH) {
        return ERROR_REQUEST_ABORTED;
    }

    wsprintf (DomainUser, TEXT("%s\\%s"), pWS->Domain, pWS->UserName);

    //
    // Verify domain and computer name are different
    //

    if (!lstrcmpi (pWS->Domain, Computer)) {
        return ERROR_REQUEST_ABORTED;
    }

    //
    // Prompt the user
    //

    ArgArray[0] = pWS->Domain;
    ArgArray[1] = pWS->UserName;

    rc = FormatMessageW (
            FORMAT_MESSAGE_ALLOCATE_BUFFER|
                FORMAT_MESSAGE_ARGUMENT_ARRAY|
                FORMAT_MESSAGE_FROM_HMODULE,
            (LPVOID) GetModuleHandle(NULL),
            (DWORD) MSG_MOVE_USER_QUESTION,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPVOID) &MsgBuf,
            0,
            (va_list *) ArgArray
            );

    if (rc == 0) {
        return ERROR_REQUEST_ABORTED;
    }

    LoadString(NULL, IDS_LOGON_MESSAGE, Caption, ARRAYSIZE(Caption));

    rc = WlxMessageBox(pTerm, NULL, MsgBuf, Caption, MB_ICONQUESTION|MB_YESNO);

    LocalFree (MsgBuf);

    if (rc == IDNO) {
        return ERROR_CONTINUE;
    }

    if (rc != IDYES) {
        return ERROR_CANCELLED;
    }

    //
    // User has chosen to merge their profile
    //

    hOldCursor = SetCursor (LoadCursor(NULL, IDC_WAIT));
    ImpersonateLoggedOnUser (pWS->hToken);

    b = RemapAndMoveUser (NULL, 0, LocalUser, DomainUser);
    Error = GetLastError();

    RevertToSelf();
    SetCursor (hOldCursor);

    if (!b && Error == ERROR_ACCESS_DENIED) {
        do {
            //
            // Prompt for administrator credentials, then try again.
            //

            PwArgs.pTerm = pTerm;

            rc = WlxDialogBoxParam(pTerm,
                                   g_hInstance,
                                   (LPTSTR)IDD_ADMIN_PW,
                                   NULL,
                                   AdminPwDlgProc,
                                   (LPARAM) &PwArgs
                                   );

            if (rc != IDOK) {
                return ERROR_CANCELLED;
            }

            hOldCursor = SetCursor (LoadCursor(NULL, IDC_WAIT));
            ImpersonateLoggedOnUser (PwArgs.hToken);

            b = RemapAndMoveUser (NULL, REMAP_PROFILE_KEEPLOCALACCOUNT, LocalUser, DomainUser);
            Error = GetLastError();

            RevertToSelf();
            SetCursor (hOldCursor);

            if (Error == ERROR_ACCESS_DENIED) {
                //
                // If non-admin credentials were entered, then try again
                //

                LoadString (NULL, IDS_REMAP_PROFILE_RETRY, StackMsgBuf, ARRAYSIZE(StackMsgBuf));
                WlxMessageBox (pTerm, NULL, StackMsgBuf, Caption, MB_OK);
            }

        } while (Error == ERROR_ACCESS_DENIED);

        LogoffIfError = TRUE;
    }

    if (!b) {
        //
        // An error occurred moving the profile.
        //

        ArgArray[0] = (PWSTR) Error;

        rc = FormatMessageW (
                FORMAT_MESSAGE_ALLOCATE_BUFFER|
                    FORMAT_MESSAGE_ARGUMENT_ARRAY|
                    FORMAT_MESSAGE_FROM_HMODULE,
                (LPVOID) GetModuleHandle(NULL),
                (DWORD) MSG_MOVE_USER_ERROR,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPVOID) &MsgBuf,
                0,
                (va_list *) ArgArray
                );

        if (rc == 0) {
            return ERROR_REQUEST_ABORTED;
        }

        WlxMessageBox(pTerm, NULL, MsgBuf, Caption, MB_ICONEXCLAMATION|MB_OK);

        LocalFree (MsgBuf);
        return LogoffIfError ? ERROR_CANCELLED : ERROR_REQUEST_ABORTED;

    } else {

        LoadString (NULL, IDS_REMAP_PROFILE_DONE, StackMsgBuf, ARRAYSIZE(StackMsgBuf));
        WlxMessageBox (pTerm, NULL, StackMsgBuf, Caption, MB_OK);

    }

    return ERROR_SUCCESS;
}


/***************************************************************************\
* FUNCTION: AdminPwDlgProc
*
* PURPOSE:  Prompts the user for an administrator password
*
* RETURNS:  TRUE/FALSE
*
* HISTORY:
*
*   05-26-99 jimschm      Created.
*
\***************************************************************************/

LRESULT
WINAPI
AdminPwDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    TCHAR Password[MAX_PATH];
    TCHAR Caption[MAX_PATH];
    TCHAR Administrator[MAX_PATH];
    TCHAR Domain[MAX_PATH];
    TCHAR Computer[MAX_PATH];
    DWORD Size;
    BOOL b;
    HWND hCombo;
    DWORD rc;
    LRESULT Index;
    PCWSTR ArgArray[2];
    PWSTR PrimaryDomain;
    PWSTR MsgBuf;
    PWSTR Domains;
    PCWSTR p;
    HCURSOR hOldCursor;
    NETSETUP_JOIN_STATUS Type;
    static PADMIN_PW_ARGS PwArgs;

    switch (message) {

    case WM_INITDIALOG:
        //
        // Init
        //

        PwArgs = (PADMIN_PW_ARGS) lParam;
        CentreWindow(hDlg);

        //
        // Fill default user name to Administrator
        //

        LoadString(NULL, IDS_ADMIN_ACCOUNT_NAME, Administrator, ARRAYSIZE(Administrator));
        SetDlgItemText (hDlg, IDC_USER_NAME, Administrator);

        //
        // Fill the domain list
        //

        hCombo = GetDlgItem (hDlg, IDC_DOMAIN);

        //
        // Add the primary domain
        //

        rc = NetGetJoinInformation (
                NULL,
                &PrimaryDomain,
                &Type
                );

        if (rc != ERROR_SUCCESS) {
            PrimaryDomain = NULL;
        } else {
            Index = SendMessage (hCombo, CB_ADDSTRING, 0, (LPARAM) PrimaryDomain);
            SendMessage (hCombo, CB_SETITEMDATA, Index, 0);
        }

        //
        // Add the trusted domains
        //

        rc = NetEnumerateTrustedDomains (NULL, &Domains);

        if (rc == ERROR_SUCCESS) {

            for (p = Domains ; *p ; p = wcschr (p, 0) + 1) {
                if (!PrimaryDomain || lstrcmpi (PrimaryDomain, p)) {
                    Index = SendMessage (hCombo, CB_ADDSTRING, 0, (LPARAM) p);
                    SendMessage (hCombo, CB_SETITEMDATA, Index, 0);
                }
            }

            NetApiBufferFree (Domains);
        }

        if (PrimaryDomain) {
            NetApiBufferFree (PrimaryDomain);
        }

        //
        // Add the current computer
        //

        Size = MAX_PATH;
        if (GetComputerName (Computer, &Size)) {

            ArgArray[0] = Computer;

            rc = FormatMessageW (
                    FORMAT_MESSAGE_ALLOCATE_BUFFER|
                        FORMAT_MESSAGE_ARGUMENT_ARRAY|
                        FORMAT_MESSAGE_FROM_HMODULE,
                    (LPVOID) GetModuleHandle(NULL),
                    (DWORD) MSG_MY_COMPUTER,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPVOID) &MsgBuf,
                    0,
                    (va_list *) ArgArray
                    );

            if (rc != 0) {
                Index = SendMessage (hCombo, CB_ADDSTRING, 0, (LPARAM) MsgBuf);
                LocalFree (MsgBuf);
                SendMessage (hCombo, CB_SETITEMDATA, Index, 1);
                SendMessage (hCombo, CB_SETCURSEL, Index, 0);
            }
        }

        //
        // Set focus to the password control
        //

        SetFocus (GetDlgItem (hDlg, IDC_PASSWORD));
        return FALSE;

    case WM_COMMAND:

        switch (HIWORD(wParam)) {

        default:

            switch (LOWORD(wParam)) {

            case IDOK:

                hOldCursor = SetCursor (LoadCursor(NULL, IDC_WAIT));

                GetDlgItemText (hDlg, IDC_USER_NAME, Administrator, ARRAYSIZE(Administrator));
                GetDlgItemText (hDlg, IDC_PASSWORD, Password, ARRAYSIZE(Password));

                hCombo = GetDlgItem (hDlg, IDC_DOMAIN);
                Index = SendMessage (hCombo, CB_GETCURSEL, 0, 0);
                if (SendMessage (hCombo, CB_GETITEMDATA, (WPARAM) Index, 0)) {
                    lstrcpy (Domain, L".");
                } else {
                    SendMessage (hCombo, CB_GETLBTEXT, (WPARAM) Index, (LPARAM) Domain);
                }

                if (!lstrcmpi (Domain, PwArgs->pTerm->pWinStaWinlogon->Domain) &&
                    !lstrcmpi (Administrator, PwArgs->pTerm->pWinStaWinlogon->UserName)
                    ) {

                    rc = TimeoutMessageBox (
                            PwArgs->pTerm,
                            hDlg,
                            IDS_SAME_ACCOUNT_WARNING,
                            IDS_LOGON_MESSAGE,
                            MB_ICONINFORMATION|MB_OK
                            );

                    if (rc != IDOK) {
                        EndDialog (hDlg, IDCANCEL);
                    }

                    break;
                }

                b = LogonUser (
                        Administrator,
                        Domain,
                        Password,
                        LOGON32_LOGON_NETWORK,
                        LOGON32_PROVIDER_DEFAULT,
                        &PwArgs->hToken
                        );

                if (!b) {
                    ArgArray[0] = Administrator;
                    ArgArray[1] = Domain;

                    rc = FormatMessageW (
                            FORMAT_MESSAGE_ALLOCATE_BUFFER|
                                FORMAT_MESSAGE_ARGUMENT_ARRAY|
                                FORMAT_MESSAGE_FROM_HMODULE,
                            (LPVOID) GetModuleHandle(NULL),
                            (DWORD) Domain[0] == L'.' ? MSG_LOCAL_BAD_PASSWORD : MSG_DOMAIN_BAD_PASSWORD,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPVOID) &MsgBuf,
                            0,
                            (va_list *) ArgArray
                            );

                    if (rc == 0) {
                        return ERROR_REQUEST_ABORTED;
                    }

                    SetCursor (hOldCursor);

                    LoadString(NULL, IDS_LOGON_MESSAGE, Caption, ARRAYSIZE(Caption));

                    WlxMessageBox (PwArgs->pTerm, hDlg, MsgBuf, Caption, MB_ICONEXCLAMATION|MB_OK);

                    SetDlgItemText (hDlg, IDC_PASSWORD, TEXT(""));
                    SetFocus (GetDlgItem (hDlg, IDC_PASSWORD));
                    break;
                }

                SetCursor (hOldCursor);
                EndDialog(hDlg, IDOK);
                return(TRUE);

            case IDCANCEL:
                EndDialog(hDlg, IDCANCEL);
                return(TRUE);

            }
            break;

        }
        break;
    }

    // We didn't process the message
    return(FALSE);
}



//*************************************************************
//
//  IsNullGUID()
//
//  Purpose:    Determines if the passed in GUID is all zeros
//
//  Parameters: pguid   GUID to compare
//
//  Return:     TRUE if the GUID is all zeros
//              FALSE if not
//
//*************************************************************

BOOL IsNullGUID (GUID *pguid)
{

    return ( (pguid->Data1 == 0)    &&
             (pguid->Data2 == 0)    &&
             (pguid->Data3 == 0)    &&
             (pguid->Data4[0] == 0) &&
             (pguid->Data4[1] == 0) &&
             (pguid->Data4[2] == 0) &&
             (pguid->Data4[3] == 0) &&
             (pguid->Data4[4] == 0) &&
             (pguid->Data4[5] == 0) &&
             (pguid->Data4[6] == 0) &&
             (pguid->Data4[7] == 0) );
}

//*************************************************************
//
//  GetMachineAccountInfo()
//
//  Purpose:    Determines if the machine is in a NT4 domain or not
//
//  Parameters: bNT4        - receives TRUE or FALSE if the machine
//                            is part of a NT4 domain
//              bStandalone - Receives TRUE or FALSE if the machine
//                            is not part of a domain
//
//
//  Return:     ERROR_SUCCESS if successful
//              Win32 error code if an error occurs
//
//*************************************************************

DWORD GetMachineAccountInfo (BOOL * bNT4, BOOL *bStandalone)
{
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pBasic;
    DWORD dwResult;


    //
    // Set the default
    //

    *bNT4 = FALSE;
    *bStandalone = FALSE;


    //
    // Query the machine's role
    //

    dwResult = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic,
                                                 (PBYTE *)&pBasic);

    if (dwResult != ERROR_SUCCESS) {
        DebugLog((DEB_ERROR, "GetMachineAccountInfo: DsRoleGetPrimaryDomainInformation failed with %d.\n", dwResult));
        return dwResult;
    }


    //
    // Check if the machine is running standalone (not part of any domain)
    //

    if ((pBasic->MachineRole == DsRole_RoleStandaloneWorkstation)    ||
        (pBasic->MachineRole == DsRole_RoleStandaloneServer)) {

        *bStandalone = TRUE;
        DsRoleFreeMemory (pBasic);
        return ERROR_SUCCESS;
    }


    //
    // Check if the machine is a domain controller.  If so, we know
    // this is a w2k domain
    //

    if ((pBasic->MachineRole == DsRole_RoleBackupDomainController)   ||
        (pBasic->MachineRole == DsRole_RolePrimaryDomainController)) {

        DsRoleFreeMemory (pBasic);
        return ERROR_SUCCESS;
    }


    //
    // Check if a GUID is present.  If so, this is a w2k domain
    //

    if ((!(pBasic->Flags & DSROLE_PRIMARY_DOMAIN_GUID_PRESENT)) ||
        (IsNullGUID(&pBasic->DomainGuid))) {
            *bNT4 = TRUE;
    }


    DsRoleFreeMemory (pBasic);

    return ERROR_SUCCESS;
}

//*************************************************************
//
//  MyGetUserName()
//
//  Purpose:    Gets the user name in the requested format
//
//  Parameters: NameFormat   - GetUserNameEx naming format
//
//  Return:     lpUserName if successful
//              NULL if an error occurs
//
//*************************************************************

LPTSTR MyGetUserName (EXTENDED_NAME_FORMAT  NameFormat)
{
    DWORD dwCount = 0, dwError = ERROR_SUCCESS;
    LPTSTR lpUserName = NULL, lpTemp;
    ULONG ulUserNameSize;



    //
    // Allocate a buffer for the user name
    //

    ulUserNameSize = 75;

    if (NameFormat == NameFullyQualifiedDN) {
        ulUserNameSize = 200;
    }


    lpUserName = LocalAlloc (LPTR, ulUserNameSize * sizeof(TCHAR));

    if (!lpUserName) {
        dwError = GetLastError();
        goto Exit;
    }


    //
    // Get the username in the requested format
    //

    while (TRUE) {

        if (GetUserNameEx (NameFormat, lpUserName, &ulUserNameSize)) {

            dwError = ERROR_SUCCESS;
            goto Exit;

        } else {

            //
            // Get the error code
            //

            dwError = GetLastError();


            //
            // If the call failed due to insufficient memory, realloc
            // the buffer and try again.  Otherwise, check the pass
            // count and retry if appropriate.
            //

            if ((dwError == ERROR_INSUFFICIENT_BUFFER) &&
                (dwError == ERROR_MORE_DATA)) {

                lpTemp = LocalReAlloc (lpUserName, (ulUserNameSize * sizeof(TCHAR)),
                                       LMEM_MOVEABLE);

                if (!lpTemp) {
                    dwError = GetLastError();
                    LocalFree (lpUserName);
                    lpUserName = NULL;
                    goto Exit;
                }

                lpUserName = lpTemp;

            } else if (dwError == ERROR_NONE_MAPPED) {
                LocalFree (lpUserName);
                lpUserName = NULL;
                goto Exit;

            } else {

                dwCount++;

                if (dwCount > 3) {
                    LocalFree (lpUserName);
                    lpUserName = NULL;
                    goto Exit;
                }

                Sleep(1000);
            }
        }
    }

Exit:

    SetLastError(dwError);

    return lpUserName;
}

//*************************************************************
//
//  DoesUserDNSDomainExist()
//
//  Purpose:    Determines if USERDNSDOMAIN exists in the
//              environment block passed back from msgina
//
//  Parameters: pTerm
//
//
//  Return:     TRUE if it exists
//              FALSE if not
//
//*************************************************************

BOOL DoesUserDNSDomainExist(PTERMINAL pTerm)
{
    LPTSTR lpTemp;


    lpTemp = pTerm->pWinStaWinlogon->UserProfile.Environment;

    while (lpTemp && *lpTemp)
    {
        //
        // Skip leading blanks
        //

        while (*lpTemp == TEXT(' ')) {
             lpTemp++;
        }

        if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, lpTemp, 13,
                          TEXT("USERDNSDOMAIN"), 13) == CSTR_EQUAL) {
            return TRUE;
        }

        lpTemp += lstrlen(lpTemp) + 1;
    }

    return FALSE;
}

//*************************************************************
//
//  GetUserAccountInfo()
//
//  Purpose:    Determines if the user account is NT4, W2K or local
//
//  Parameters: bLocalUser - set to TRUE if the account is local
//              bNT4User   - set to TRUE if the user account is in a NT4 domain
//
//
//  Return:     ERROR_SUCCESS if successful
//              WIN32 error code if an error occurs
//
//*************************************************************

DWORD GetUserAccountInfo (PTERMINAL pTerm, BOOL *bLocalUser, BOOL *bNT4User)
{
    HANDLE uh;
    LPTSTR lpTemp, lpDomain = NULL;
    TCHAR szComputerName[MAX_PATH];
    DWORD dwError, dwSize;
    PUSER_INFO_0 pUserInfo;
    PDOMAIN_CONTROLLER_INFO pDCI;


    //
    // Default is not a local user and not a NT4 user
    //

    *bLocalUser = FALSE;
    *bNT4User = FALSE;


    //
    // Impersonate the user
    //

    uh = ImpersonateUser(&(pTerm->pWinStaWinlogon->UserProcessData), NULL);

    if (!uh) {
        DebugLog((DEB_ERROR, "GetUserAccountInfo: Failed to impersonate user\n"));
        return GetLastError();
    }


    //
    // Get the username in NT4 format
    //

    lpDomain = MyGetUserName (NameSamCompatible);

    if (!lpDomain) {
        dwError = GetLastError();
        DebugLog((DEB_ERROR, "GetUserAccountInfo:  MyGetUserName failed for NT4 style name with %d\n",
                 dwError));
        StopImpersonating(uh);
        return dwError;
    }

    StopImpersonating(uh);


    //
    // Look for the \ between the domain and username and replace
    // it with a NULL
    //

    lpTemp = lpDomain;

    while (*lpTemp && ((*lpTemp) != TEXT('\\')))
        lpTemp++;


    if (*lpTemp != TEXT('\\')) {
        DebugLog((DEB_ERROR, "GetUserAccountInfo: Failed to find slash in NT4 style name\n"));
        LocalFree (lpDomain);
        return ERROR_INVALID_DATA;
    }

    *lpTemp = TEXT('\0');
    lpTemp++;



    //
    // Check if this is a local user account
    //

    dwSize = ARRAYSIZE(szComputerName);
    if (!GetComputerName (szComputerName, &dwSize)) {
        dwError = GetLastError();
        DebugLog((DEB_ERROR, "GetUserAccountInfo: Failed to query computer name\n"));
        LocalFree (lpDomain);
        return dwError;
    }


    if (!lstrcmpi(szComputerName, lpDomain)) {

        if (NetUserGetInfo (NULL, lpTemp, 0, (LPBYTE *)&pUserInfo) == NERR_Success) {
            *bLocalUser = TRUE;
            NetApiBufferFree(pUserInfo);
            LocalFree (lpDomain);
            return ERROR_SUCCESS;
        }
    }


    //
    // Check if the domain is a NT5 domain (it has a DS)
    //

    dwError = DsGetDcName (NULL, lpDomain, NULL, NULL,
                            DS_DIRECTORY_SERVICE_PREFERRED, &pDCI);

    if (dwError != ERROR_SUCCESS) {
        DebugLog((DEB_ERROR, "GetUserAccountInfo: DsGetDcName failed with %d\n", dwError));
        LocalFree (lpDomain);
        return dwError;
    }

    if (!(pDCI->Flags & DS_DS_FLAG)) {
        if (!DoesUserDNSDomainExist(pTerm)) {
            *bNT4User = TRUE;
        }
    }


    NetApiBufferFree(pDCI);
    LocalFree (lpDomain);

    return ERROR_SUCCESS;
}

//*************************************************************
//
//  GetSystemPolicySetting()
//
//  Purpose:    Determines which types of System Policy
//              should be applied
//
//  Parameters: pTerm
//
//  Return:     DWORD System policy flags
//              0 if no processing should be done.
//
//*************************************************************

DWORD GetSystemPolicySetting (PTERMINAL pTerm)
{
    DWORD dwRetVal = 0;
    BOOL bLocalUser, bNT4User, bNT4Machine, bStandalone;


    //
    // Query some information about the user
    //

    if (GetUserAccountInfo (pTerm, &bLocalUser, &bNT4User) != ERROR_SUCCESS) {
        DebugLog((DEB_ERROR, "GetSystemPolicySetting: GetUserAccountInfo failed.\n"));
        return 0;
    }


    //
    // If this is a NT4 user account, we should apply user system policy
    //

    if (bNT4User || bLocalUser) {
        dwRetVal |= SP_FLAG_APPLY_USER_POLICY;
    }


    //
    // Query some information about the machine
    //

    if (GetMachineAccountInfo (&bNT4Machine, &bStandalone) != ERROR_SUCCESS) {
        DebugLog((DEB_ERROR, "GetSystemPolicySetting: GetMachineAccountInfo failed.\n"));
        return 0;
    }

    if (bNT4Machine || bStandalone) {
        dwRetVal |= SP_FLAG_APPLY_MACHINE_POLICY;
    }

    return dwRetVal;
}


/***************************************************************************\
* RestoreUserProfile
*
* Downloads the user's profile if possible, otherwise use either cached
* profile or default Windows profile.
*
* Returns TRUE on success, FALSE on failure.
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
NTSTATUS
RestoreUserProfile(
    PTERMINAL pTerm
    )
{
    PROFILEINFO pi;
    BOOL bSilent = FALSE;
    HKEY hKey;
    LONG lResult;
    DWORD dwType, dwSize;
    DWORD dwVal;
    PWINDOWSTATION pWS = pTerm->pWinStaWinlogon;


    //
    // Get the system policy processing flags
    //

    pWS->UserProfile.dwSysPolicyFlags = GetSystemPolicySetting(pTerm);


    //
    // Check if the "NoPopups" flag is set.
    //

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            TEXT("System\\CurrentControlSet\\Control\\Windows"),
                            0,
                            KEY_READ,
                            &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof (bSilent);

        RegQueryValueEx(hKey,
                        TEXT("NoPopupsOnBoot"),
                        NULL,
                        &dwType,
                        (LPBYTE) &bSilent,
                        &dwSize);

        RegCloseKey (hKey);
    }

    //
    // Turn on status UI
    //

    StatusMessage(FALSE, 0, IDS_STATUS_LOAD_PROFILE);


    //
    // Load the profile
    //

    ZeroMemory (&pi, sizeof(pi));
    pi.dwSize = sizeof(PROFILEINFO);

    if (bSilent) {
        pi.dwFlags |= PI_NOUI;
    }

    pi.lpUserName = pWS->UserName;
    pi.lpProfilePath = pWS->UserProfile.ProfilePath;
    pi.lpDefaultPath = pWS->UserProfile.NetworkDefaultUserProfile;


    if (LoadUserProfile(pWS->UserProcessData.UserToken, &pi)) {
        pWS->UserProfile.hProfile = pi.hProfile;


        //
        // Apply System Policy if appropriate
        //

        if (pWS->UserProfile.dwSysPolicyFlags) {
            ApplySystemPolicy(pWS->UserProfile.dwSysPolicyFlags, pWS->UserProcessData.UserToken, pi.hProfile,
                              pWS->UserName, pWS->UserProfile.PolicyPath,
                              pWS->UserProfile.ServerName);
        }

        return ERROR_SUCCESS;

    } else {
        pWS->UserProfile.hProfile = NULL;
    }

    //
    // Failure
    //

    return GetLastError();
}

VOID
FreeUserSpecificData(
    PWINDOWSTATION pWS
    )
{
     if (pWS->UserProcessData.CurrentDirectory) {
         FreeAndNull(pWS->UserProcessData.CurrentDirectory);
     }

     if (pWS->UserProcessData.pEnvironment) {
         RtlDestroyEnvironment(pWS->UserProcessData.pEnvironment);
         pWS->UserProcessData.pEnvironment = NULL ;
     }

     if (pWS->UserProfile.ProfilePath) {
         FreeAndNull(pWS->UserProfile.ProfilePath);
     }

     if (pWS->UserProfile.PolicyPath) {
         FreeAndNull(pWS->UserProfile.PolicyPath);
     }

     if (pWS->UserProfile.NetworkDefaultUserProfile) {
         FreeAndNull(pWS->UserProfile.NetworkDefaultUserProfile);
     }

     if (pWS->UserProfile.ServerName) {
         FreeAndNull(pWS->UserProfile.ServerName);
     }

     if (pWS->UserProfile.Environment) {
         FreeAndNull(pWS->UserProfile.Environment);
     }

     if (pWS->UserName) {
         FreeAndNull(pWS->UserName);
     }

     if (pWS->Domain) {
         FreeAndNull(pWS->Domain);
     }

     if (pWS->LogonScripts) {
         FreeAndNull(pWS->LogonScripts);
     }

}

/***************************************************************************\
* SaveUserProfile
*
* Saves the user's profile changes.
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL
SaveUserProfile(
    PWINDOWSTATION pWS
    )
{
    HANDLE hEventStart, hEventDone;

    //
    // Notify RAS Autodial service that the
    // user has is logging off.
    //
    hEventStart = OpenEvent(SYNCHRONIZE|EVENT_MODIFY_STATE, FALSE, L"RasAutodialLogoffUser");
    hEventDone = OpenEvent(SYNCHRONIZE|EVENT_MODIFY_STATE, FALSE, L"RasAutodialLogoffUserDone");

    if (hEventStart != NULL && hEventDone != NULL) {

        //
        // Toggle the event so RAS can save it's settings and
        // close it's HKEY_CURRENT_USER key.
        //

        SetEvent(hEventStart);


        //
        // Autodial will toggle the event again when it's finished.
        //
        WaitForSingleObject(hEventDone, 20000);

        CloseHandle(hEventStart);
        CloseHandle(hEventDone);
    }

    if ( pWS->UserProfile.NetworkProviderTask )
    {
        UnregisterWait( pWS->UserProfile.NetworkProviderTask );

        pWS->UserProfile.NetworkProviderTask = NULL ;
    }


    //
    // Make sure HKEY_CURRENT_USER is closed before unloading the
    // user's profile.
    //

    try {

        RegCloseKey(HKEY_CURRENT_USER);

    } except(EXCEPTION_EXECUTE_HANDLER) {};

    FreeUserSpecificData( pWS );

    return UnloadUserProfile (pWS->UserProcessData.UserToken,
                              pWS->UserProfile.hProfile);

}


VOID InitializeGPOSupport (PTERMINAL pTerm)
{
    HKEY hKey;
    DWORD dwType, dwSize;
    BOOL bSyncMachineGroupPolicy = TRUE;
    BOOL bSyncUserGroupPolicy = TRUE;
    DWORD dwOptionValue = 0;


    //
    // Check if the machine is booted in safe mode with no
    // networking support
    //

    if ( pTerm->SafeMode ) {

        return ;
    }


    //
    // First, check for machine preferences
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bSyncMachineGroupPolicy);
        RegQueryValueEx (hKey, SYNC_MACHINE_GROUP_POLICY, NULL, &dwType,
                         (LPBYTE) &bSyncMachineGroupPolicy, &dwSize);

        dwSize = sizeof(bSyncUserGroupPolicy);
        RegQueryValueEx (hKey, SYNC_USER_GROUP_POLICY, NULL, &dwType,
                         (LPBYTE) &bSyncUserGroupPolicy, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Second, check for previous machine policy
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_POLICY_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bSyncMachineGroupPolicy);
        RegQueryValueEx (hKey, SYNC_MACHINE_GROUP_POLICY, NULL, &dwType,
                         (LPBYTE) &bSyncMachineGroupPolicy, &dwSize);

        dwSize = sizeof(bSyncUserGroupPolicy);
        RegQueryValueEx (hKey, SYNC_USER_GROUP_POLICY, NULL, &dwType,
                         (LPBYTE) &bSyncUserGroupPolicy, &dwSize);

        RegCloseKey (hKey);
    }


    if (g_Console) {
        WlAddInternalNotify(StartMachineGPOProcessing, WL_NOTIFY_STARTUP, !bSyncMachineGroupPolicy, FALSE, TEXT("Begin Machine Group Policy"), 3600 );
        WlAddInternalNotify(StopMachineGPOProcessing, WL_NOTIFY_SHUTDOWN, FALSE, FALSE, TEXT("Finish Machine Group Policy"), 600 );
    }
    WlAddInternalNotify(StartUserGPOProcessing, WL_NOTIFY_LOGON, !bSyncUserGroupPolicy, FALSE, TEXT("Begin User Group Policy"), 3600 );
    WlAddInternalNotify(StopUserGPOProcessing, WL_NOTIFY_LOGOFF, FALSE, FALSE, TEXT("Finish User Group Policy"), 600 );
    WlAddInternalNotify(RunGPOLogonScripts, WL_NOTIFY_STARTSHELL, FALSE, FALSE, TEXT("Run Group Policy Logon Scripts"), 600 );

}

VOID MachineGPNotification (PVOID pArg, BOOL bTimerFired)
{
    PTERMINAL pTerm = (PTERMINAL) pArg;
    PWINDOWSTATION pWS =  pTerm->pWinStaWinlogon;

    //ResetEvent (pTerm->hGPONotifyEvent);

    //
    // Check if we should apply machine system policy
    //

    if (pWS->UserProfile.dwSysPolicyFlags & SP_FLAG_APPLY_MACHINE_POLICY) {

        ApplySystemPolicy(SP_FLAG_APPLY_MACHINE_POLICY, pWS->UserProcessData.UserToken,
                          pWS->UserProfile.hProfile,
                          pWS->UserName, pWS->UserProfile.PolicyPath,
                          pWS->UserProfile.ServerName);
    }
}

DWORD StartMachineGPOProcessing (PWLX_NOTIFICATION_INFO pNotifyInfo)
{
    HKEY hKey;
    DWORD dwType, dwSize, dwFlags;
    BOOL bDisableBackgroundPolicy = FALSE;
    BOOL bSync = TRUE;
    PTERMINAL pTerm = g_pTerminals;
    TCHAR szEventName[200];
    HANDLE hAutoEnrollmentThread = 0;
    WCHAR szDesktop[MAX_PATH];
    PVOID pEnvironment;
    BOOL bNT4Machine, bStandalone;


    //
    // Query if the machine is running standalone, joined to a NT4 domain, or
    // joined to a W2K domain
    //

    if (GetMachineAccountInfo (&bNT4Machine, &bStandalone) != ERROR_SUCCESS) {
        DebugLog((DEB_ERROR, "StartMachineGPOProcessing: GetMachineAccountInfo failed. No machine Group Policy will be applied!\n"));
        return ERROR_SUCCESS;
    }

    StatusMessage (FALSE, 0, IDS_STATUS_COMPUTER_SETTINGS);


    //
    // First, check for machine preferences
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0,
                            KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bDisableBackgroundPolicy);
        RegQueryValueEx (hKey, DISABLE_BKGND_POLICY, NULL, &dwType,
                         (LPBYTE) &bDisableBackgroundPolicy, &dwSize);

        dwSize = sizeof(bSync);
        RegQueryValueEx (hKey, SYNC_STARTUP_SCRIPT, NULL, &dwType,
                         (LPBYTE) &bSync, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Second, check for previous machine policy
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_POLICY_KEY, 0,
                            KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bDisableBackgroundPolicy);
        RegQueryValueEx (hKey, DISABLE_BKGND_POLICY, NULL, &dwType,
                         (LPBYTE) &bDisableBackgroundPolicy, &dwSize);

        dwSize = sizeof(bSync);
        RegQueryValueEx (hKey, SYNC_STARTUP_SCRIPT, NULL, &dwType,
                         (LPBYTE) &bSync, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Start the machine GPO processing
    //

     if (OpenProcessToken(GetCurrentProcess(),
                         TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE, &pTerm->hToken))
     {

        //
        // Create a unique event name
        //

        wsprintf (szEventName, TEXT("winlogon:  machine GPO Event %d"), GetTickCount());
        pTerm->hGPOEvent = CreateEvent (NULL, TRUE, FALSE, szEventName);

        if (pTerm->hGPOEvent) {


            //
            // Create the notify event
            //

            pTerm->hGPONotifyEvent = CreateEvent (NULL, FALSE, FALSE, NULL);

            if (pTerm->hGPONotifyEvent) {
                RegisterGPNotification (pTerm->hGPONotifyEvent, TRUE);

                RegisterWaitForSingleObject (&pTerm->hGPOWaitEvent, pTerm->hGPONotifyEvent,
                                            (WAITORTIMERCALLBACK) MachineGPNotification, pTerm, INFINITE, 0);
            }


            //
            // Start the processing
            //

            dwFlags = GP_MACHINE;

            if (!bNT4Machine && !bStandalone) {
                dwFlags |= GP_APPLY_DS_POLICY;
            }

            if (!bDisableBackgroundPolicy) {
                dwFlags |= GP_BACKGROUND_REFRESH;
            }

            pTerm->hGPOThread = ApplyGroupPolicy (dwFlags,
                                                  pTerm->hToken,
                                                  pTerm->hGPOEvent,
                                                  HKEY_LOCAL_MACHINE,
                                                  (PFNSTATUSMESSAGECALLBACK)StatusMessage2);

            if (!pTerm->hGPOThread || bDisableBackgroundPolicy) {
                CloseHandle (pTerm->hGPOEvent);
                pTerm->hGPOEvent = NULL;
            }

            // Register for auto-enrollment.  Note, this is done here because we
            // must gurantee that The group policy notification event is set up
            // before we register for auto-enrollment.
            pTerm->hAutoEnrollmentHandler = RegisterAutoEnrollmentProcessing(TRUE, pTerm->hToken);



            //
            // Check if any startup scripts exist
            //

            if (CheckForGPOScripts (HKEY_LOCAL_MACHINE, TEXT("Startup"))) {

                StatusMessage (FALSE, 0, IDS_STATUS_STARTUP_SCRIPTS);

                //
                // Execute the startup scripts
                //

                if (CreateEnvironmentBlock(&pEnvironment, NULL, FALSE)) {

                    if (SetUserEnvironmentVariable(&pEnvironment,
                                                   GPO_SCRIPT_VARIALBE_NAME,
                                                   TEXT("Startup"),
                                                   TRUE)) {
                        wsprintfW (szDesktop, L"%s\\%s", g_pTerminals->pWinStaWinlogon->lpWinstaName, WINLOGON_DESKTOP_NAME);
                        ExecuteGPOScripts (pTerm, FALSE, bSync, szDesktop, pEnvironment);
                    }

                    DestroyEnvironmentBlock(pEnvironment);
                }
            }
        }
    }

    RemoveStatusMessage(FALSE);


    return ERROR_SUCCESS;
}

DWORD StopMachineGPOProcessing (PWLX_NOTIFICATION_INFO pNotifyInfo)
{
    PTERMINAL pTerm = g_pTerminals;
    WCHAR szDesktop[MAX_PATH];
    PVOID pEnvironment;

    // Shut down auto-enrollment
    if (pTerm->hAutoEnrollmentHandler)
    {
       DeRegisterAutoEnrollment(pTerm->hAutoEnrollmentHandler);
       pTerm->hAutoEnrollmentHandler = NULL;
    }

    //
    // Check if any shutdown scripts exist
    //

    if (CheckForGPOScripts (HKEY_LOCAL_MACHINE, TEXT("Shutdown"))) {

        StatusMessage (FALSE, 0, IDS_STATUS_SHUTDOWN_SCRIPTS);

        //
        // Execute the shutdown scripts
        //

        if (CreateEnvironmentBlock(&pEnvironment, NULL, FALSE)) {

            if (SetUserEnvironmentVariable(&pEnvironment,
                                           GPO_SCRIPT_VARIALBE_NAME,
                                           TEXT("Shutdown"),
                                           TRUE)) {
                wsprintfW (szDesktop, L"%s\\%s", g_pTerminals->pWinStaWinlogon->lpWinstaName, WINLOGON_DESKTOP_NAME);
                ExecuteGPOScripts (pTerm, FALSE, TRUE, szDesktop, pEnvironment);
            }

            DestroyEnvironmentBlock(pEnvironment);
        }

        RemoveStatusMessage(FALSE);
    }


    //
    // Stop the machine GPO processing
    //

    ShutdownGPOProcessing( TRUE );

    if (pTerm->hGPOEvent) {

        SetEvent(pTerm->hGPOEvent);

        DebugLog((DEB_TRACE, "StopMachineGPOProcessing: Waiting for machine group policy thread to terminate.\n"));

        StatusMessage(TRUE, 0, IDS_STATUS_WAIT_MACHINE_GPO_FINISH);

        WaitForSingleObject (pTerm->hGPOThread, 120000);

        StatusMessage(TRUE, 0, IDS_STATUS_MACHINE_GPO_FINISHED);

        DebugLog((DEB_TRACE, "StopMachineGPOProcessing: Machine group policy thread has terminated.\n"));

        CloseHandle(pTerm->hGPOEvent);
        CloseHandle(pTerm->hGPOThread);
    }

    if (pTerm->hGPOWaitEvent) {
        UnregisterWaitEx (pTerm->hGPOWaitEvent, NULL);
    }

    if (pTerm->hGPONotifyEvent) {
        UnregisterGPNotification (pTerm->hGPONotifyEvent);
        CloseHandle (pTerm->hGPONotifyEvent);
    }

    if ( pTerm->hToken )
    {
        CloseHandle( pTerm->hToken );
    }

    return ERROR_SUCCESS;
}

VOID UserGPNotification (PVOID pArg, BOOL bTimerFired)
{
    PWINDOWSTATION pWS =  ((PTERMINAL) pArg)->pWinStaWinlogon;

  //  ResetEvent (pWS->UserProfile.hGPONotifyEvent);

    //
    // Check if we should apply user system policy
    //

    if (pWS->UserProfile.dwSysPolicyFlags & SP_FLAG_APPLY_USER_POLICY) {

        ApplySystemPolicy(SP_FLAG_APPLY_USER_POLICY, pWS->UserProcessData.UserToken,
                          pWS->UserProfile.hProfile,
                          pWS->UserName, pWS->UserProfile.PolicyPath,
                          pWS->UserProfile.ServerName);
    }
}


DWORD StartUserGPOProcessing (PWLX_NOTIFICATION_INFO pNotifyInfo)
{
    HKEY hKey;
    DWORD dwType, dwSize, dwFlags;
    BOOL bDisableBackgroundPolicy = FALSE;
    TCHAR szEventName[200];
    PTERMINAL pTerm = g_pTerminals;
    PWINDOWSTATION pWS = pTerm->pWinStaWinlogon;
    HANDLE hAutoEnrollmentThread = 0;
    BOOL bLocalUser, bNT4User;


    //
    // If we didn't load the profile, then we can't do GPO processing
    //

    if (!pWS->UserProfile.hProfile) {
        return ERROR_SUCCESS;
    }


    //
    // Query some information about the user
    //

    if (GetUserAccountInfo (pTerm, &bLocalUser, &bNT4User) != ERROR_SUCCESS) {
        DebugLog((DEB_ERROR, "StartUserGPOProcessing: GetUserAccountInfo failed. User Group Policy will not be applied!\n"));
        return ERROR_SUCCESS;
    }


    StatusMessage(FALSE, 0, IDS_STATUS_USER_SETTINGS);


    //
    // First, check for machine preferences
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0,
                            KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bDisableBackgroundPolicy);
        RegQueryValueEx (hKey, DISABLE_BKGND_POLICY, NULL, &dwType,
                         (LPBYTE) &bDisableBackgroundPolicy, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Second, check for previous machine policy
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_POLICY_KEY, 0,
                            KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bDisableBackgroundPolicy);
        RegQueryValueEx (hKey, DISABLE_BKGND_POLICY, NULL, &dwType,
                         (LPBYTE) &bDisableBackgroundPolicy, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Start the User GPO processing
    //

    wsprintf (szEventName, TEXT("winlogon:  User GPO Event %d"), GetTickCount());
    pWS->UserProfile.hGPOEvent = CreateEvent (NULL, TRUE, FALSE, szEventName);

    if (pWS->UserProfile.hGPOEvent) {

        //
        // Create the notify event
        //

        pWS->UserProfile.hGPONotifyEvent = CreateEvent (NULL, FALSE, FALSE, NULL);

        if (pWS->UserProfile.hGPONotifyEvent) {
            RegisterGPNotification (pWS->UserProfile.hGPONotifyEvent, FALSE);

            RegisterWaitForSingleObject (&pWS->UserProfile.hGPOWaitEvent,
                                         pWS->UserProfile.hGPONotifyEvent,
                                         (WAITORTIMERCALLBACK) UserGPNotification,
                                         pTerm, INFINITE, 0);
        }

        //
        // Start the processing
        //

        dwFlags = 0;

        if (!bLocalUser && !bNT4User) {
            dwFlags |= GP_APPLY_DS_POLICY;
        }

        if (!bDisableBackgroundPolicy) {
            dwFlags |= GP_BACKGROUND_REFRESH;
        }

        pWS->UserProfile.hGPOThread = ApplyGroupPolicy (dwFlags,
                                                        pWS->UserProcessData.UserToken,
                                                        pWS->UserProfile.hGPOEvent,
                                                        pWS->UserProfile.hProfile,
                                                        (PFNSTATUSMESSAGECALLBACK)StatusMessage2);

        if (!pWS->UserProfile.hGPOThread || bDisableBackgroundPolicy) {
            CloseHandle (pWS->UserProfile.hGPOEvent);
            pWS->UserProfile.hGPOEvent = NULL;
        }

        // Start user auto-enrollment handling.  We must do it here because
        // we have a dependency on the GPO download event

        pWS->UserProfile.hAutoEnrollmentHandler =
            RegisterAutoEnrollmentProcessing(FALSE, pWS->UserProcessData.UserToken);
    }


    return ERROR_SUCCESS;
}

DWORD StopUserGPOProcessing (PWLX_NOTIFICATION_INFO pNotifyInfo)
{
    PWINDOWSTATION pWS = g_pTerminals->pWinStaWinlogon;
    WCHAR szDesktop[MAX_PATH];
    PVOID pEnvironment;


    // Shut down user auto-enrollment
    if (pWS->UserProfile.hAutoEnrollmentHandler)
    {
       DeRegisterAutoEnrollment(pWS->UserProfile.hAutoEnrollmentHandler);
       pWS->UserProfile.hAutoEnrollmentHandler = NULL;
    }


    //
    // Check if any logoff scripts exist
    //

    if (CheckForGPOScripts (pWS->UserProfile.hProfile, TEXT("Logoff"))) {

        StatusMessage(FALSE, 0, IDS_STATUS_LOGOFF_SCRIPTS);

        //
        // Execute the logoff scripts
        //

        if (CreateEnvironmentBlock(&pEnvironment, pWS->UserProcessData.UserToken, FALSE)) {
            if (SetUserEnvironmentVariable(&pEnvironment,
                                           GPO_SCRIPT_VARIALBE_NAME,
                                           TEXT("Logoff"),
                                           TRUE)) {
                wsprintfW (szDesktop, L"%s\\%s", g_pTerminals->pWinStaWinlogon->lpWinstaName, APPLICATION_DESKTOP_NAME);
                ExecuteGPOScripts (g_pTerminals, TRUE, TRUE, szDesktop, pEnvironment);
            }

            DestroyEnvironmentBlock(pEnvironment);
        }
    }


    //
    // Stop the User GPO processing
    //

    ShutdownGPOProcessing( FALSE );

    if (pWS->UserProfile.hGPOEvent) {

        SetEvent(pWS->UserProfile.hGPOEvent);

        DebugLog((DEB_TRACE, "StopUserGPOProcessing: Waiting for user group policy thread to terminate.\n"));

        StatusMessage(TRUE, 0, IDS_STATUS_WAIT_USER_GPO_FINISH);

        WaitForSingleObject (pWS->UserProfile.hGPOThread, 120000);

        StatusMessage(TRUE, 0, IDS_STATUS_USER_GPO_FINISHED);

        DebugLog((DEB_TRACE, "StopUserGPOProcessing: User group policy thread has terminated.\n"));

        CloseHandle(pWS->UserProfile.hGPOEvent);
        CloseHandle(pWS->UserProfile.hGPOThread);
    }

    if (pWS->UserProfile.hGPOWaitEvent) {
        UnregisterWaitEx (pWS->UserProfile.hGPOWaitEvent, NULL);
    }

    if (pWS->UserProfile.hGPONotifyEvent) {
        UnregisterGPNotification (pWS->UserProfile.hGPONotifyEvent);
        CloseHandle (pWS->UserProfile.hGPONotifyEvent);
    }


    return ERROR_SUCCESS;
}

DWORD RunGPOLogonScripts (PWLX_NOTIFICATION_INFO pNotifyInfo)
{
    HKEY hKey;
    DWORD dwType, dwSize;
    BOOL bSync = FALSE;
    PWINDOWSTATION pWS = g_pTerminals->pWinStaWinlogon;
    WCHAR szDesktop[MAX_PATH];
    PVOID pEnvironment;

    //
    // If we didn't load the profile, then we can't do GPO processing
    //

    if (!pWS->UserProfile.hProfile) {
        return ERROR_SUCCESS;
    }


    //
    // Check if any logon scripts exist
    //

    if (!CheckForGPOScripts (pWS->UserProfile.hProfile, TEXT("Logon"))) {
        return ERROR_SUCCESS;
    }


    StatusMessage(TRUE, 0, IDS_STATUS_LOGON_SCRIPTS);


    //
    // First, check for a user preference
    //

    if (RegOpenKeyEx ((HKEY)pWS->UserProfile.hProfile, WINLOGON_KEY, 0,
                            KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bSync);
        RegQueryValueEx (hKey, SYNC_LOGON_SCRIPT, NULL, &dwType,
                         (LPBYTE) &bSync, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Check for a machine preference
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0,
                            KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bSync);
        RegQueryValueEx (hKey, SYNC_LOGON_SCRIPT, NULL, &dwType,
                         (LPBYTE) &bSync, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Check for a user policy
    //

    if (RegOpenKeyEx ((HKEY)pWS->UserProfile.hProfile, WINLOGON_POLICY_KEY, 0,
                            KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bSync);
        RegQueryValueEx (hKey, SYNC_LOGON_SCRIPT, NULL, &dwType,
                         (LPBYTE) &bSync, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Check for a machine policy
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_POLICY_KEY, 0,
                            KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bSync);
        RegQueryValueEx (hKey, SYNC_LOGON_SCRIPT, NULL, &dwType,
                         (LPBYTE) &bSync, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Execute the GPO logon scripts
    //

    if (CreateEnvironmentBlock(&pEnvironment, pWS->UserProcessData.UserToken, FALSE)) {
        if (SetUserEnvironmentVariable(&pEnvironment,
                                       GPO_SCRIPT_VARIALBE_NAME,
                                       TEXT("Logon"),
                                       TRUE)) {
            //
            // user scripts can only run on application desktop
            //
            wsprintfW (szDesktop, L"%s\\%s", g_pTerminals->pWinStaWinlogon->lpWinstaName, APPLICATION_DESKTOP_NAME);
            ExecuteGPOScripts (g_pTerminals, TRUE, bSync, szDesktop, pEnvironment);
        }

        DestroyEnvironmentBlock(pEnvironment);
    }

    return ERROR_SUCCESS;
}

BOOL CheckForGPOScripts (HKEY hKeyRoot, LPTSTR lpScriptType)
{
    HKEY hKey;
    BOOL bResult = FALSE;
    DWORD dwSize, dwType;


    if (RegOpenKeyEx (hKeyRoot, TEXT("Software\\Policies\\Microsoft\\Windows\\System\\Scripts") ,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        if (RegQueryValueEx (hKey, lpScriptType, NULL, &dwType,
                             NULL, &dwSize) == ERROR_SUCCESS) {
            bResult = TRUE;
        }

        RegCloseKey (hKey);
    }

    return bResult;
}

BOOL ExecuteGPOScripts (PTERMINAL pTerm, BOOL bUser, BOOL bSync,
                        LPWSTR lpDesktop, PVOID pEnvironment)
{
    BOOL Result = FALSE;
    TCHAR szCmdLine[MAX_PATH];
    HKEY hKey;
    DWORD dwSize, dwType;
    DWORD dwMaxWait = 600;  // seconds
    PWINLOGON_JOB Job = NULL ;

    DebugLog((DEB_TRACE, "ExecuteGPOScripts: Entering bSync = %d\n", bSync));

    //
    // Setup the command line
    //

    lstrcpy (szCmdLine, TEXT("userinit.exe"));


    //
    // If we are executing the scripts synchronously, then put userinit
    // in a job object so we can kill it
    //

    if (bSync) {

        //
        // Look up the max wait time for a script
        //

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0,
                          KEY_READ, &hKey) == ERROR_SUCCESS) {

            dwSize = sizeof(dwMaxWait);
            RegQueryValueEx (hKey, MAX_GPO_SCRIPT_WAIT, NULL, &dwType,
                             (LPBYTE) &dwMaxWait, &dwSize);

            RegCloseKey (hKey);
        }


        //
        // Check for machine policy
        //

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_POLICY_KEY, 0,
                          KEY_READ, &hKey) == ERROR_SUCCESS) {

            dwSize = sizeof(dwMaxWait);
            RegQueryValueEx (hKey, MAX_GPO_SCRIPT_WAIT, NULL, &dwType,
                             (LPBYTE) &dwMaxWait, &dwSize);
            RegCloseKey (hKey);
        }


        //
        // Convert wait time to ms
        //

        if (dwMaxWait == 0) {
            dwMaxWait = INFINITE;

        } else if (dwMaxWait >= 32000) {
            dwMaxWait = 32000 * 1000;

        } else {
            dwMaxWait *= 1000;
        }


        //
        // Create a job object
        //

        Job = CreateWinlogonJob();

        if ( !Job )
        {
            DebugLog(( DEB_WARN, "ExecuteGPOScripts: Failed to create winlogon job\n" ));
            goto Exit ;
        }

        SetWinlogonJobOption( Job, WINLOGON_JOB_MONITOR_ROOT_PROCESS );

        Result = StartProcessInJob(
                        pTerm,
                        (bUser ? ProcessAsUser : ProcessAsSystem),
                        lpDesktop,
                        pEnvironment,
                        szCmdLine,
                        CREATE_UNICODE_ENVIRONMENT,
                        0,
                        Job );




        //
        // If the app was started, assign the process to the job
        //

        if (Result) {
            BOOL bSwitchedDesktop = FALSE;

            //
            // user scripts run on application desktop
            // application desktop needs to be made visible.
            //
            
            if ( bUser )
            {
                bSwitchedDesktop = SetActiveDesktop(pTerm, Desktop_Application);
            }

            if ( !WaitForJob( Job, dwMaxWait ) )
            {
                TerminateJob( Job, 0 );
            }
            
            if ( bSwitchedDesktop )
            {
                SetActiveDesktop(pTerm, Desktop_Previous);
            }

        }

    } else {

        //
        // Start userinit asynchronously
        //

        if (bUser) {
            StartApplication(pTerm, lpDesktop, pEnvironment, szCmdLine);

        } else {
            StartSystemProcess(szCmdLine, lpDesktop, 0, 0, pEnvironment,
                               0, NULL, NULL);
        }
    }

Exit:

    if ( Job )
    {
        DeleteJob( Job );
    }

    DebugLog((DEB_TRACE, "ExecuteGPOScripts: Leaving.\n"));

    return Result;
}
