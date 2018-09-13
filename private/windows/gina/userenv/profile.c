//*************************************************************
//
//  Profile management routines
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"


//
// Local function proto-types
//

BOOL CheckNetDefaultProfile (LPPROFILE lpProfile);
BOOL ParseProfilePath(LPPROFILE lpProfile, LPTSTR lpProfilePath);
BOOL RestoreUserProfile(LPPROFILE lpProfile);
BOOL TestIfUserProfileLoaded(HANDLE hUserToken, LPPROFILEINFO lpProfileInfo);
BOOL CreateLocalProfileKey (LPPROFILE lpProfile, PHKEY phKey, BOOL *bKeyExists);
BOOL GetExistingLocalProfileImage(LPPROFILE lpProfile);
BOOL CreateLocalProfileImage(LPPROFILE lpProfile, LPTSTR lpBaseName);
BOOL IsCentralProfileReachable(LPPROFILE lpProfile, BOOL *bCreateCentralProfile,
                               BOOL *bMandatory);
BOOL IssueDefaultProfile (LPPROFILE lpProfile, LPTSTR lpDefaultProfile,
                          LPTSTR lpLocalProfile, LPTSTR lpSidString,
                          BOOL bMandatory);
BOOL UpgradeCentralProfile (LPPROFILE lpProfile, LPTSTR lpOldProfile);
BOOL UpgradeProfile (LPPROFILE lpProfile);
BOOL SaveProfileInfo (LPPROFILE lpProfile);
LPPROFILE LoadProfileInfo(HANDLE hToken, HKEY hKeyCurrentUser);
BOOL CheckForSlowLink(LPPROFILE lpProfile, DWORD dwTime, LPTSTR lpPath);
INT_PTR APIENTRY SlowLinkDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD GetUserPreferenceValue(HANDLE hToken);
BOOL APIENTRY ChooseProfileDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD ApplySecurityToRegistryTree(HKEY RootKey, PSECURITY_DESCRIPTOR pSD);
BOOL PrepareProfileForUse (LPPROFILE lpProfile);
BOOL GetUserDomainName (LPPROFILE lpProfile, LPTSTR lpDomainName, LPDWORD lpDomainNameSize);
BOOL IsCacheDeleted();
BOOL PatchNewProfileIfRequired(HANDLE hToken);
DWORD DecrementProfileRefCount(LPPROFILE lpProfile);
DWORD IncrementProfileRefCount(LPPROFILE lpProfile, BOOL bInitialize);
BOOL IsTempProfileAllowed();
BOOL SetNtUserIniAttributes(LPTSTR szDir);

//*************************************************************
//
//  LoadUserProfile()
//
//  Purpose:    Loads the user's profile, if unable to load
//              use the cached profile or issue the default profile.
//
//  Parameters: hToken          -   User's token
//              lpProfileInfo   -   Profile Information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/6/95      ericflo    Created
//
//*************************************************************
BOOL WINAPI LoadUserProfile (HANDLE hToken, LPPROFILEINFO lpProfileInfo)
{
    LPPROFILE lpProfile = NULL;
    BOOL bResult = FALSE, bNewProfileLoaded = FALSE;
    HANDLE hMutex = NULL;
    DWORD dwResult;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    HANDLE hOldToken;
    DWORD dwRef, dwErr;
    LPTSTR SidString, lpUserMutexName;

    //
    // Get the error at the beginning of the call.
    //

    dwErr = GetLastError();

    InitDebugSupport( FALSE );


    //
    //  Check Parameters
    //

    if (!lpProfileInfo) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: NULL lpProfileInfo")));
        dwErr = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    if (lpProfileInfo->dwSize != sizeof(PROFILEINFO)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: lpProfileInfo->dwSize != sizeof(PROFILEINFO)")));
        dwErr = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    if (!lpProfileInfo->lpUserName || !(*lpProfileInfo->lpUserName)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: received a NULL pointer for lpUserName.")));
        dwErr = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // if the roaming profile is greater than MAX_PATH, log the user off
    //
    
    if ((lpProfileInfo->lpProfilePath) && (lstrlen(lpProfileInfo->lpProfilePath) >= MAX_PATH)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: long profile path name %s. ignoring"), lpProfileInfo->lpProfilePath));
        ReportError(PI_NOUI, IDS_PROFILE_PATH_TOOLONG, lpProfileInfo->lpProfilePath);
        (lpProfileInfo->lpProfilePath)[0] = TEXT('\0');
    }    

    if ((lpProfileInfo->lpDefaultPath) && (lstrlen(lpProfileInfo->lpDefaultPath) >= MAX_PATH)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: long default profile path name %s. ignoring"), lpProfileInfo->lpDefaultPath));
        (lpProfileInfo->lpDefaultPath)[0] = TEXT('\0');
    }    

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("=========================================================")));

    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Entering, hToken = <0x%x>, lpProfileInfo = 0x%x"),
             hToken, lpProfileInfo));

    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Entering, hToken = <0x%x>, lpProfileInfo = 0x%x"),
             hToken, lpProfileInfo));

    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->dwFlags = <0x%x>"),
             lpProfileInfo->dwFlags));

    if (lpProfileInfo->lpUserName) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpUserName = <%s>"),
                 lpProfileInfo->lpUserName));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL user name!")));
    }

    if (lpProfileInfo->lpProfilePath) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpProfilePath = <%s>"),
                 lpProfileInfo->lpProfilePath));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL central profile path")));
    }

    if (lpProfileInfo->lpDefaultPath) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpDefaultPath = <%s>"),
                 lpProfileInfo->lpDefaultPath));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL default profile path")));
    }

    if (lpProfileInfo->lpServerName) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpServerName = <%s>"),
                 lpProfileInfo->lpServerName));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL server name")));
    }

    if (lpProfileInfo->dwFlags & PI_APPLYPOLICY) {
        if (lpProfileInfo->lpPolicyPath) {
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpPolicyPath = <%s>"),
                      lpProfileInfo->lpPolicyPath));
        } else {
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL policy path")));
        }
    }


    //
    // Make sure we can impersonate the user
    //

    if (!ImpersonateUser(hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to impersonate user with %d."), GetLastError()));
        dwErr = GetLastError();
        goto Exit;
    }

    RevertToUser(&hOldToken);


    //
    // Make sure someone isn't loading a profile during
    // GUI mode setup (eg: mapi)
    //

    if (g_hGUIModeSetup) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: LoadUserProfile can not be called during GUI mode setup.")));
        dwErr = ERROR_NOT_READY;
        goto Exit;
    }


    //
    // Wait for the profile setup event to be signalled
    //

    if (g_hProfileSetup) {
        if ((WaitForSingleObject (g_hProfileSetup, 600000) != WAIT_OBJECT_0)) {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to wait on the profile setup event.  Error = %d."),
                      GetLastError()));
            dwErr = GetLastError();
            goto Exit;
        }
    }

    //
    // Get the user's sid in string form
    //

    SidString = GetSidString(hToken);

    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile:  Failed to get sid string for user")));
        dwErr = GetLastError();
        goto Exit;;
    }


    //
    // Allocate memory for the mutex name
    //

    lpUserMutexName = LocalAlloc (LPTR, (lstrlen(SidString) + lstrlen(USER_PROFILE_MUTEX) + 1) * sizeof(TCHAR));

    if (!lpUserMutexName) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile:  Failed to allocate memory for user mutex name with %d"),
            dwErr));
        goto Exit;;
    }

    lstrcpy (lpUserMutexName, USER_PROFILE_MUTEX);
    lstrcat (lpUserMutexName, SidString);


    //
    // Create a mutex to prevent multiple threads/process from trying to
    // load user profiles at the same time.
    //

    InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );

    SetSecurityDescriptorDacl (
                                &sd,
                                TRUE,                           // Dacl present
                                NULL,                           // NULL Dacl
                                FALSE                           // Not defaulted
                            );

    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;
    sa.nLength = sizeof(sa);

    hMutex = CreateMutex (&sa, FALSE, lpUserMutexName);

    DeleteSidString (SidString);
    LocalFree (lpUserMutexName);

    if (!hMutex) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to create mutex.  Error = %d."),
            GetLastError()));
        dwErr = GetLastError();
        goto Exit;
    }


    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Created mutex.  Waiting...")));

    if ((WaitForSingleObject (hMutex, INFINITE) == WAIT_FAILED)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to wait on the mutex.  Error = %d."),
            GetLastError()));
        dwErr = GetLastError();
        goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Wait succeeded.  Mutex currently held.")));


    //-------------------  BEGIN MUTEX SECTION ------------------------
    //
    // The mutex is held at this point, no doddling now...
    //

    //
    // Check if the profile is loaded already.
    //


    if (TestIfUserProfileLoaded(hToken, lpProfileInfo)) {
        DWORD  dwFlags = lpProfileInfo->dwFlags;

        //
        // This profile is already loaded.  Grab the info from the registry
        // and add the missing chunks.
        //

        lpProfile = LoadProfileInfo (hToken, lpProfileInfo->hProfile);

        if (!lpProfile) {
            RegCloseKey (lpProfileInfo->hProfile);
            lpProfileInfo->hProfile = NULL;
            dwErr = GetLastError();
            goto Exit;
        }

        //
        // LoadProfileInfo will overwrite the dwFlags field with the
        // value from the previous profile loading.  Restore the flags.
        //

        lpProfile->dwFlags = dwFlags;

        if (lpProfile->dwFlags & PI_LITELOAD) {
            lpProfile->dwFlags |= PI_NOUI;
        }


        //
        // LoadProfileInfo doesn't restore username, servername, policypath so
        // special case these.
        //

        lpProfile->lpUserName = LocalAlloc (LPTR, (lstrlen(lpProfileInfo->lpUserName) + 1) * sizeof(TCHAR));

        if (!lpProfile->lpUserName) {
            RegCloseKey (lpProfileInfo->hProfile);
            dwErr = GetLastError();
            goto Exit;
        }

        lstrcpy (lpProfile->lpUserName, lpProfileInfo->lpUserName);

        if (lpProfileInfo->lpServerName) {
            lpProfile->lpServerName = LocalAlloc (LPTR, (lstrlen(lpProfileInfo->lpServerName) + 1) * sizeof(TCHAR));

            if (lpProfile->lpServerName) {
                lstrcpy (lpProfile->lpServerName, lpProfileInfo->lpServerName);
            }
        }

        if (lpProfileInfo->dwFlags & PI_APPLYPOLICY) {
            if (lpProfileInfo->lpPolicyPath) {
                lpProfile->lpPolicyPath = LocalAlloc (LPTR, (lstrlen(lpProfileInfo->lpPolicyPath) + 1) * sizeof(TCHAR));

                if (lpProfile->lpPolicyPath) {
                    lstrcpy (lpProfile->lpPolicyPath, lpProfileInfo->lpPolicyPath);
                }
            }
        }


        //
        // Set the USERPROFILE environment variable into this process's
        // environmnet.  This allows ExpandEnvironmentStrings to be used
        // in the policy processing.
        //

        SetEnvironmentVariable (TEXT("USERPROFILE"), lpProfile->lpLocalProfile);

        //
        // Jump to the end of the profile loading code.
        //

        goto ProfileLoaded;
    }



    //
    // If we are here, the profile isn't loaded yet, so we are
    // starting from scratch.
    //
    // Allocate an internal Profile structure to work with.
    //

    lpProfile = (LPPROFILE) LocalAlloc (LPTR, sizeof(USERPROFILE));

    if (!lpProfile) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to allocate memory")));
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // Save the data passed in.
    //

    lpProfile->dwFlags = lpProfileInfo->dwFlags;

    //
    // No UI in case of Lite_Load
    //

    if (lpProfile->dwFlags & PI_LITELOAD) {
        lpProfile->dwFlags |= PI_NOUI;
    }

    lpProfile->dwUserPreference = GetUserPreferenceValue(hToken);
    lpProfile->hToken = hToken;

    lpProfile->lpUserName = LocalAlloc (LPTR, (lstrlen(lpProfileInfo->lpUserName) + 1) * sizeof(TCHAR));

    if (!lpProfile->lpUserName) {
        dwErr = GetLastError();
        goto Exit;
    }

    lstrcpy (lpProfile->lpUserName, lpProfileInfo->lpUserName);

    if (lpProfileInfo->lpDefaultPath) {

        lpProfile->lpDefaultProfile = LocalAlloc (LPTR, (lstrlen(lpProfileInfo->lpDefaultPath) + 1) * sizeof(TCHAR));

        if (lpProfile->lpDefaultProfile) {
            lstrcpy (lpProfile->lpDefaultProfile, lpProfileInfo->lpDefaultPath);
        }
    }

    if (lpProfileInfo->lpProfilePath) {
        lpProfile->lpProfilePath = LocalAlloc (LPTR, (lstrlen(lpProfileInfo->lpProfilePath) + 1) * sizeof(TCHAR));

        if (lpProfile->lpProfilePath) {
            lstrcpy(lpProfile->lpProfilePath, lpProfileInfo->lpProfilePath);
        }
    }

    if (lpProfileInfo->lpServerName) {
        lpProfile->lpServerName = LocalAlloc (LPTR, (lstrlen(lpProfileInfo->lpServerName) + 1) * sizeof(TCHAR));

        if (lpProfile->lpServerName) {
            lstrcpy (lpProfile->lpServerName, lpProfileInfo->lpServerName);
        }
    }

    if (lpProfileInfo->dwFlags & PI_APPLYPOLICY) {
        if (lpProfileInfo->lpPolicyPath) {
            lpProfile->lpPolicyPath = LocalAlloc (LPTR, (lstrlen(lpProfileInfo->lpPolicyPath) + 1) * sizeof(TCHAR));

            if (lpProfile->lpPolicyPath) {
                lstrcpy (lpProfile->lpPolicyPath, lpProfileInfo->lpPolicyPath);
            }
        }
    }

    lpProfile->lpLocalProfile = LocalAlloc (LPTR, MAX_PATH * sizeof(TCHAR));

    if (!lpProfile->lpLocalProfile) {
        dwErr = GetLastError();
        goto Exit;
    }

    lpProfile->lpRoamingProfile = LocalAlloc (LPTR, MAX_PATH * sizeof(TCHAR));

    if (!lpProfile->lpRoamingProfile) {
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // If there is a central profile, check for 3.x or 4.0 format.
    //

    if (lpProfileInfo->lpProfilePath && (*lpProfileInfo->lpProfilePath)) {

        //
        // Call ParseProfilePath to work some magic on it
        //

        if (!ParseProfilePath(lpProfile, lpProfileInfo->lpProfilePath)) {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: ParseProfilePath returned FALSE")));
            dwErr = ERROR_INVALID_PARAMETER;
            goto Exit;
        }

        //
        // The real central profile directory is...
        //

        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: ParseProfilePath returned a directory of <%s>"),
                  lpProfile->lpRoamingProfile));
    }



    //
    // Load the user's profile
    //

    if (!RestoreUserProfile(lpProfile)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: RestoreUserProfile returned FALSE")));
        dwErr = GetLastError();
        goto Exit;
    }


    GetSystemTimeAsFileTime (&lpProfile->ftProfileLoad);

    //
    // Save the profile information in the registry
    //

    SaveProfileInfo (lpProfile);

    //
    // Set the USERPROFILE environment variable into this process's
    // environmnet.  This allows ExpandEnvironmentStrings to be used
    // in the userdiff processing.
    //

    SetEnvironmentVariable (TEXT("USERPROFILE"), lpProfile->lpLocalProfile);


    //
    // Flush the special folder pidls stored in shell32.dll
    //

    FlushSpecialFolderCache();

    //
    // Set attributes on ntuser.ini
    //

    SetNtUserIniAttributes(lpProfile->lpLocalProfile);
    

    //
    // Upgrade the profile if appropriate.
    //

    if (!(lpProfileInfo->dwFlags & PI_LITELOAD)) {
        if (!UpgradeProfile(lpProfile)) {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: UpgradeProfile returned FALSE")));
        }
    }


    //
    // Prepare the profile for use on this machine
    //

    PrepareProfileForUse (lpProfile);

    bNewProfileLoaded = TRUE;


ProfileLoaded:

    //
    // Increment the profile Ref count
    //

    dwRef = IncrementProfileRefCount(lpProfile, bNewProfileLoaded);

    if (!bNewProfileLoaded && (dwRef <= 1))
        DebugMsg((DM_WARNING, TEXT("Profile was loaded but the Ref Count is %d !!!"), dwRef));
    else
        DebugMsg((DM_VERBOSE, TEXT("Profile Ref Count is %d"), dwRef));


    //
    // This will release the mutex so other threads/process can continue.
    //

    if (hMutex) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Releasing mutex.")));
        ReleaseMutex (hMutex);
        CloseHandle (hMutex);
        hMutex = NULL;
    }


    //
    // The mutex is now released so we can do slower things like
    // apply policy...
    //
    //-------------------  END MUTEX SECTION ------------------------


    //
    // Apply Policy
    //

    if (lpProfile->dwFlags & PI_APPLYPOLICY) {
        if (!ApplySystemPolicy((SP_FLAG_APPLY_MACHINE_POLICY | SP_FLAG_APPLY_USER_POLICY),
                               lpProfile->hToken, lpProfile->hKeyCurrentUser,
                               lpProfile->lpUserName, lpProfile->lpPolicyPath,
                               lpProfile->lpServerName)) {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: ApplySystemPolicy returned FALSE")));
        }
    }


    //
    // Save the outgoing parameters
    //

    lpProfileInfo->hProfile = (HANDLE) lpProfile->hKeyCurrentUser;


    //
    // Success!
    //

    bResult = TRUE;

Exit:

    //
    // Free the structure
    //


    if (lpProfile) {

        if (lpProfile->lpUserName) {
            LocalFree (lpProfile->lpUserName);
        }

        if (lpProfile->lpDefaultProfile) {
            LocalFree (lpProfile->lpDefaultProfile);
        }

        if (lpProfile->lpProfilePath) {
            LocalFree (lpProfile->lpProfilePath);
        }

        if (lpProfile->lpServerName) {
            LocalFree (lpProfile->lpServerName);
        }

        if (lpProfile->lpPolicyPath) {
            LocalFree (lpProfile->lpPolicyPath);
        }

        if (lpProfile->lpLocalProfile) {
            LocalFree (lpProfile->lpLocalProfile);
        }

        if (lpProfile->lpRoamingProfile) {
            LocalFree (lpProfile->lpRoamingProfile);
        }

        if (lpProfile->lpExclusionList) {
            LocalFree (lpProfile->lpExclusionList);
        }

        LocalFree (lpProfile);
    }

    if (hMutex) {

        //
        // This will release the mutex so other threads/process can continue.
        //

        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Releasing mutex.")));
        ReleaseMutex (hMutex);
        CloseHandle (hMutex);
    }

    //
    // Now set the last error
    //

    SetLastError(dwErr);

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Leaving with a value of %d."), bResult));

    if (bResult) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: hProfile = <0x%x>"), lpProfileInfo->hProfile));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("GetLastError() = 0x%x"), GetLastError()));
    }

    DebugMsg((DM_VERBOSE, TEXT("=========================================================")));

    return bResult;
}


//*************************************************************
//
//  CheckNetDefaultProfile()
//
//  Purpose:    Checks if a network profile exists
//
//  Parameters: lpProfile   -   Profile information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   This routine assumes we are working
//              in the user's context.
//
//  History:    Date        Author     Comment
//              9/21/95     ericflo    Created
//              4/10/99     ushaji     modified to remove local caching
//
//*************************************************************

BOOL CheckNetDefaultProfile (LPPROFILE lpProfile)
{
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szLocalDir[MAX_PATH];
    DWORD dwSize, dwFlags, dwErr;
    LPTSTR lpEnd;
    BOOL bRetVal = FALSE;
    LPTSTR lpNetPath = lpProfile->lpDefaultProfile;
    HANDLE hOldToken;


    //
    // Get the error at the beginning of the call.
    //

    dwErr = GetLastError();


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile: Entering, lpNetPath = <%s>"),
             (lpNetPath ? lpNetPath : TEXT("NULL"))));


    if (!lpNetPath || !(*lpNetPath)) {
        return bRetVal;
    }

    //
    // Impersonate the user....
    //

    if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {

        if (lpProfile->lpDefaultProfile) {
            *lpProfile->lpDefaultProfile = TEXT('\0');
        }

        //
        // Last error is set
        //

        return bRetVal;
    }

    //
    // See if network copy exists
    //

    hFile = FindFirstFile (lpNetPath, &fd);

    if (hFile != INVALID_HANDLE_VALUE) {


        //
        // Close the find handle
        //

        FindClose (hFile);


        //
        // We found something.  Is it a directory?
        //

        if ( !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {

            DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile:  FindFirstFile found a file. ignoring Network Defaul profile")));
            dwErr = ERROR_FILE_NOT_FOUND;
            goto Exit;
        }


        //
        // Is there a ntuser.* file in this directory?
        //

        lstrcpy (szBuffer, lpNetPath);
        lpEnd = CheckSlash (szBuffer);
        lstrcpy (lpEnd, c_szNTUserStar);


        hFile = FindFirstFile (szBuffer, &fd);

        if (hFile == INVALID_HANDLE_VALUE) {
            DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile:  FindFirstFile found a directory, but no ntuser files.")));
            dwErr = ERROR_FILE_NOT_FOUND;
            goto Exit;
        }

        FindClose (hFile);


        DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile:  Found a valid network profile.")));

        bRetVal = TRUE;

    }
    else {
        dwErr = ERROR_FILE_NOT_FOUND;
    }

Exit:

    //
    // If we are leaving successfully, then
    // save the local profile directory.
    //

    if (bRetVal) {
        DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile: setting default profile to <%s>"), lpNetPath));

    } else {
        DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile: setting default profile to NULL")));

        //
        // resetting it to NULL in case we didn't see the server directory.
        //

        if (lpProfile->lpDefaultProfile) {
            *lpProfile->lpDefaultProfile = TEXT('\0');
        }
    }


    //
    // Tag the internal flags so we don't do this again.
    //

    lpProfile->dwInternalFlags |= DEFAULT_NET_READY;

    //
    // Now set the last error
    //

    //
    // Revert before trying to delete the local default network profile
    //

    RevertToUser(&hOldToken);

    //
    // We will keep this on always so that we can delete any preexisting
    // default network profile, try to delete it and ignore
    // the failure if it happens

    //
    // Expand the local profile directory
    //


    dwSize = ARRAYSIZE(szLocalDir);
    if (!GetProfilesDirectoryEx(szLocalDir, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("CheckNetDefaultProfile:  Failed to get default user profile.")));
        SetLastError(dwErr);
        return bRetVal;
    }


    lpEnd = CheckSlash (szLocalDir);
    lstrcpy (lpEnd, DEFAULT_USER_NETWORK);


    DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile:  Removing local copy of network default user profile.")));
    Delnode (szLocalDir);


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile:  Leaving with a value of %d."), bRetVal));


    SetLastError(dwErr);

    return bRetVal;
}


//*************************************************************
//
//  ParseProfilePath()
//
//  Purpose:    Parses the profile path to determine if
//              it points at a directory or a filename.
//              If the path points to a filename, a subdirectory
//              of a similar name is created.
//
//  Parameters: lpProfile       -   Profile Information
//              lpProfilePath   -   Input path
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/6/95      ericflo    Created
//
//*************************************************************

BOOL ParseProfilePath(LPPROFILE lpProfile, LPTSTR lpProfilePath)
{
    WIN32_FIND_DATA fd;
    HANDLE hResult;
    DWORD  dwError;
    TCHAR  szProfilePath[MAX_PATH];
    TCHAR  szExt[5];
    LPTSTR lpEnd;
    UINT   uiExtCount;
    BOOL   bRetVal = FALSE;
    BOOL   bUpgradeCentral = FALSE;
    BOOL   bMandatory = FALSE;
    DWORD  dwStart, dwDelta, dwErr;
    DWORD  dwStrLen;
    HANDLE hOldToken;
    TCHAR  szErr[MAX_PATH];

    //
    // Should quit earlier in case of USERINFO_LOCAL
    //

    //
    // Get the error at the beginning of the call.
    //

    dwErr = GetLastError();

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Entering, lpProfilePath = <%s>"),
             lpProfilePath));


    //
    // Check for .man extension
    //

    dwStrLen = lstrlen (lpProfilePath);

    if (dwStrLen >= 4) {

        lpEnd = lpProfilePath + dwStrLen - 4;
        if (!lstrcmpi(lpEnd, c_szMAN)) {
            bMandatory = TRUE;
            lpProfile->dwInternalFlags |= PROFILE_MANDATORY;
            DebugMsg((DM_WARNING, TEXT("ParseProfilePath: Mandatory profile (.man extension)")));
        }
    }


    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("ParseProfilePath: Failed to impersonate user")));
        // last error is already set.
        return FALSE;
    }

    //
    // Start by calling FindFirstFile so we have file attributes
    // to work with.  We have to call FindFirstFile twice.  The
    // first call sets up the session so we can call it again and
    // get accurate timing information for slow link detection.
    //


    hResult = FindFirstFile(lpProfilePath, &fd);

    if (hResult != INVALID_HANDLE_VALUE) {
        FindClose (hResult);
    }


    dwStart = GetTickCount();

    hResult = FindFirstFile(lpProfilePath, &fd);

    dwDelta = GetTickCount() - dwStart;

    DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Tick Count = %d"), dwDelta));

    //
    // It's magic time...
    //

    if (hResult != INVALID_HANDLE_VALUE) {

        //
        // FindFirst File found something.
        // First close the handle, then look at
        // the file attributes.
        //

        DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: FindFirstFile found something with attributes <0x%x>"),
                 fd.dwFileAttributes));

        FindClose(hResult);


        //
        // If we found a directory, copy the path to
        // the result buffer and exit.
        //

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Found a directory")));
            CheckForSlowLink (lpProfile, dwDelta, lpProfilePath);
            lstrcpy (lpProfile->lpRoamingProfile, lpProfilePath);
            bRetVal = TRUE;
            goto Exit;
        }


        //
        // We found a file.
        // Jump to the filename generation code.
        //

        DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Found a file")));


        //
        // We set this flag now, but it is only used if a new
        // central directory is created.
        //

        bUpgradeCentral = TRUE;

        goto GenerateDirectoryName;
    }

    //
    // FindFirstFile failed.  Look at the error to determine why.
    //

    dwError = GetLastError();
    DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: FindFirstFile failed with error %d"),
              dwError));


    //
    // To fix bug #414176, the last error code chk has been added.
    // Should rethink immediately after NT5 ships.
    //
    
    if ( (dwError == ERROR_FILE_NOT_FOUND) ||
         (dwError == ERROR_PATH_NOT_FOUND) ||
         (dwError == ERROR_BAD_NET_NAME) ) {

        //
        // Nothing found with this name.  If the name
        // does not end in .usr or .man, attempt to create
        // the directory.
        //

        //
        // One possible help for slowlink is to not try and create a directory
        // when we know that it is a slow link since we already have the data
        // in the tick count.. BUGBUG
        //

        dwStrLen = lstrlen (lpProfilePath);

        if (dwStrLen >= 4) {

            lpEnd = lpProfilePath + dwStrLen - 4;

            if ( (lstrcmpi(lpEnd, c_szUSR) != 0) &&
                 (lstrcmpi(lpEnd, c_szMAN) != 0) ) {


                if (CreateSecureDirectory(lpProfile, lpProfilePath, NULL, TRUE)) {

                    //
                    // Successfully created the directory.
                    //

                    DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Succesfully created the sub-directory")));
                    CheckForSlowLink (lpProfile, dwDelta, lpProfilePath);
                    lstrcpy (lpProfile->lpRoamingProfile, lpProfilePath);
                    bRetVal = TRUE;
                    goto Exit;

                } else {

                    //
                    // Failed to create the subdirectory, BUGBUG:: We should quit probably
                    //

                    dwErr = GetLastError();
                    DebugMsg((DM_WARNING, TEXT("ParseProfilePath: Failed to create user sub-directory.  Error = %d"),
                             GetLastError()));
                }
            }
        }
    }

    else if ((dwError == ERROR_ACCESS_DENIED) || (dwError == ERROR_LOGON_FAILURE)) {

        dwErr = dwError;

        DebugMsg((DM_WARNING, TEXT("ParseProfilePath: You don't have permission to your central profile server!  Error = %d"),
                 dwError));

        if ((lpProfile->dwUserPreference != USERINFO_LOCAL) &&
            !(lpProfile->dwInternalFlags & PROFILE_SLOW_LINK)) {

            if (bMandatory) {
                ReportError(lpProfile->dwFlags, IDS_MANDATORY_NOT_AVAILABLE2, GetErrString(dwError, szErr));
            } else {
                ReportError(lpProfile->dwFlags, IDS_ACCESSDENIED, lpProfilePath);
            }
        }

        if (IsUserAnAdminMember(lpProfile->hToken))
            goto DisableAndExit;
        else {
            if (IsTempProfileAllowed()) {
                DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Diisabling Profile share because access is denied and TempProfile is allowed.")));
                goto DisableAndExit;
            }
            else
                goto Exit;
        }
    }

    DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Did not create the sub-directory.  Generating a new name.")));


GenerateDirectoryName:

    //
    // If we made it here, either:
    //
    // 1) a file exists with the same name
    // 2) the directory couldn't be created
    // 3) the profile path ends in .usr or .man
    //
    // Make a local copy of the path so we can munge it.
    //

    lstrcpy (szProfilePath, lpProfilePath);

    DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Entering name generating code working with a path of <%s>."),
              szProfilePath));


    //
    // Does this path have a filename extension?
    //

    lpEnd = szProfilePath + lstrlen (szProfilePath) - 1;

    while (*lpEnd && (lpEnd >= szProfilePath)) {
        if (*lpEnd == TEXT('.'))
            break;

        if (*lpEnd == TEXT('\\'))
            break;

        lpEnd--;
    }

    if (*lpEnd != TEXT('.')) {

        //
        // The path does not have an extension.  Append .pds
        //

        lpEnd = szProfilePath + lstrlen (szProfilePath);
        lstrcpy (lpEnd, c_szPDS);

    } else {

        //
        // The path has an extension.  Append the new
        // directory extension (.pds or .pdm).
        //

        if (lstrcmpi(lpEnd, c_szMAN) == 0) {
            lstrcpy (lpEnd, c_szPDM);

        } else {
            lstrcpy (lpEnd, c_szPDS);
        }

    }



    //
    // Call FindFirstFile to see if this directory exists.
    //

    hResult = FindFirstFile(szProfilePath, &fd);



    if (hResult != INVALID_HANDLE_VALUE) {

        //
        // FindFirst File found something.
        // First close the handle, then look at
        // the file attributes.
        //

        DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: FindFirstFile(2) found something with attributes <0x%x>"),
                 fd.dwFileAttributes));

        FindClose(hResult);


        //
        // If we found a directory, copy the path to
        // the result buffer and exit.
        //

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Found a directory")));
            if (bMandatory) {
                lstrcpy (lpProfile->lpRoamingProfile, szProfilePath);
                lpProfile->dwInternalFlags |= PROFILE_MANDATORY;
            } else {
                CheckForSlowLink (lpProfile, dwDelta, szProfilePath);
                lstrcpy (lpProfile->lpRoamingProfile, szProfilePath);
            }
            bRetVal = TRUE;
            goto Exit;
        }


        //
        // We found a file that matches the generated name.
        //

        DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Found a file with the name we generated.")));

        if (!bMandatory) {
            CheckForSlowLink (lpProfile, dwDelta, szProfilePath);
        }

        if (bMandatory || ((lpProfile->dwUserPreference != USERINFO_LOCAL) &&
            !(lpProfile->dwInternalFlags & PROFILE_SLOW_LINK))) {

            ReportError(lpProfile->dwFlags, IDS_FAILEDDIRCREATE, szProfilePath);
        }

        goto DisableAndExit;
    }


    //
    // FindFirstFile failed.  Look at the error to determine why.
    //

    dwError = GetLastError();
    DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: FindFirstFile failed with error %d"),
              dwError));


    //
    // If we are working with a mandatory profile,
    // disable the central profile and try to log
    // on with a cache.
    //

    if (bMandatory) {

        DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Central mandatory profile is unreachable to due error %d."), dwError));

        if (IsUserAnAdminMember(lpProfile->hToken)) {
            ReportError(lpProfile->dwFlags, IDS_MANDATORY_NOT_AVAILABLE, GetErrString(dwError, szErr));

            lpProfile->lpRoamingProfile[0] = TEXT('\0');
            lpProfile->dwInternalFlags |= PROFILE_MANDATORY;
            bRetVal = TRUE;
        } else {
            ReportError(lpProfile->dwFlags, IDS_MANDATORY_NOT_AVAILABLE2, GetErrString(dwError, szErr));
        }
        goto Exit;
    }


    //
    // To fix bug #414176, the last error code chk has been added.
    // Should rethink immediately after NT5 ships.
    //
    
    if ( (dwError == ERROR_FILE_NOT_FOUND) ||
         (dwError == ERROR_PATH_NOT_FOUND) ||
         (dwError == ERROR_BAD_NET_NAME) ) {

        //
        // Attempt to create the directory.
        //

        if (CreateSecureDirectory(lpProfile, szProfilePath, NULL, TRUE)) {

            //
            // Successfully created the directory.
            //

            DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Succesfully created the sub-directory")));

            lstrcpy (lpProfile->lpRoamingProfile, szProfilePath);


            if (bUpgradeCentral) {
                bRetVal = UpgradeCentralProfile (lpProfile, lpProfilePath);

            } else {
                bRetVal = TRUE;
            }

            if (bRetVal) {

                //
                // Success
                //

                CheckForSlowLink (lpProfile, dwDelta, szProfilePath);

                goto Exit;

            } else {

                //
                // Delete the directory we created above.
                //

                Delnode (lpProfile->lpRoamingProfile);
            }


        } else {

            DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Failed to create the sub-directory")));

            if ((lpProfile->dwUserPreference != USERINFO_LOCAL) &&
                !(lpProfile->dwInternalFlags & PROFILE_SLOW_LINK)) {

                ReportError(lpProfile->dwFlags, IDS_FAILEDDIRCREATE2, szProfilePath, GetErrString(GetLastError(), szErr));
            }
            goto DisableAndExit;
        }
    }

    //
    // The central profile isn't reachable, or failed to upgrade.
    // Disable the central profile and try to log
    // on with a cache.
    //

    dwError = GetLastError();
    DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Central profile is unreachable to due error %d, switching to local profile only."), dwError));

    if ((lpProfile->dwUserPreference != USERINFO_LOCAL) &&
        !(lpProfile->dwInternalFlags & PROFILE_SLOW_LINK)) {

        ReportError(lpProfile->dwFlags, IDS_CENTRAL_NOT_AVAILABLE2, GetErrString(dwError, szErr));
    }

DisableAndExit:

    lpProfile->lpRoamingProfile[0] = TEXT('\0');

    bRetVal = TRUE;


Exit:

    //
    // Revert to being 'ourself'
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("ParseProfilePath: Failed to revert to self")));
    }

    if (!bRetVal)
        SetLastError(dwError);

    return bRetVal;
}

//*************************************************************
//
//  GetExclusionList()
//
//  Purpose:    Get's the exclusion list used at logon
//
//  Parameters: lpProfile   - Profile Information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL GetExclusionList (LPPROFILE lpProfile)
{
    TCHAR szExcludeListLocal[2 * MAX_PATH];
    TCHAR szExcludeListServer[2 * MAX_PATH];
    TCHAR szNTUserIni[MAX_PATH];
    LPTSTR lpEnd;
    HANDLE hOldToken;


    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("GetExclusionList: Failed to impersonate user")));
        // last error is set
        return FALSE;
    }


    //
    // Get the exclusion list from the server
    //

    szExcludeListServer[0] = TEXT('\0');

    lstrcpy (szNTUserIni, lpProfile->lpRoamingProfile);
    lpEnd = CheckSlash (szNTUserIni);
    lstrcpy(lpEnd, c_szNTUserIni);

    GetPrivateProfileString (PROFILE_GENERAL_SECTION,
                             PROFILE_EXCLUSION_LIST,
                             TEXT(""), szExcludeListServer,
                             ARRAYSIZE(szExcludeListServer),
                             szNTUserIni);


    //
    // Get the exclusion list from the cache
    //

    szExcludeListLocal[0] = TEXT('\0');

    lstrcpy (szNTUserIni, lpProfile->lpLocalProfile);
    lpEnd = CheckSlash (szNTUserIni);
    lstrcpy(lpEnd, c_szNTUserIni);

    GetPrivateProfileString (PROFILE_GENERAL_SECTION,
                             PROFILE_EXCLUSION_LIST,
                             TEXT(""), szExcludeListLocal,
                             ARRAYSIZE(szExcludeListLocal),
                             szNTUserIni);


    //
    // Go back to system security context
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("GetExclusionList: Failed to revert to self")));
    }


    //
    // See if the lists are the same
    //

    if (!lstrcmpi (szExcludeListServer, szExcludeListLocal)) {

        if (szExcludeListServer[0] != TEXT('\0')) {

            lpProfile->lpExclusionList = LocalAlloc (LPTR, (lstrlen (szExcludeListServer) + 1) * sizeof(TCHAR));

            if (lpProfile->lpExclusionList) {
                lstrcpy (lpProfile->lpExclusionList, szExcludeListServer);

                DebugMsg((DM_VERBOSE, TEXT("GetExclusionList:  The exclusion lists on both server and client are the same.  The list is: <%s>"),
                         szExcludeListServer));
            } else {
                DebugMsg((DM_WARNING, TEXT("GetExclusionList:  Failed to allocate memory for exclusion list with error %d"),
                         GetLastError()));
            }
        } else {
            DebugMsg((DM_VERBOSE, TEXT("GetExclusionList:  The exclusion on both server and client is empty.")));
        }

    } else {
        DebugMsg((DM_VERBOSE, TEXT("GetExclusionList:  The exclusion lists between server and client are different.  Server is <%s> and client is <%s>"),
                 szExcludeListServer, szExcludeListLocal));
    }


    return TRUE;
}

//*************************************************************
//
//  RestoreUserProfile()
//
//  Purpose:    Downloads the user's profile if possible,
//              otherwise use either cached profile or
//              default profile.
//
//  Parameters: lpProfile   -   Profile information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Created
//
//*************************************************************

BOOL RestoreUserProfile(LPPROFILE lpProfile)
{
    BOOL  IsCentralReachable = FALSE;
    BOOL  IsLocalReachable = FALSE;
    BOOL  IsMandatory = FALSE;
    BOOL  IsProfilePathNULL = FALSE;
    BOOL  bCreateCentralProfile = FALSE;
    BOOL  bDefaultUsed = FALSE;
    BOOL  bCreateLocalProfile = TRUE;
    LPTSTR lpRoamingProfile = NULL;
    LPTSTR lpLocalProfile;
    BOOL  bProfileLoaded = FALSE;
    BOOL bNewUser = TRUE;
    LPTSTR SidString;
    LONG error = ERROR_SUCCESS;
    TCHAR szProfile[MAX_PATH];
    TCHAR szDefaultProfile[MAX_PATH];
    LPTSTR lpEnd;
    BOOL bRet;
    DWORD dwSize, dwFlags, dwErr=0, dwErr1;
    HANDLE hOldToken;
    TCHAR szErr[MAX_PATH];

    //
    // Get the error at the beginning of the call.
    //

    dwErr1 = GetLastError();

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Entering")));


    //
    // Get the Sid string for the current user
    //

    SidString = GetSidString(lpProfile->hToken);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  Failed to get sid string for user")));
        return FALSE;
    }

    //
    // Test if this user is a guest.
    //

    if (IsUserAGuest(lpProfile->hToken)) {
        lpProfile->dwInternalFlags |= PROFILE_GUEST_USER;
        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  User is a Guest")));
    }

    //
    // Test if this user is an admin.
    //

    if (IsUserAnAdminMember(lpProfile->hToken)) {
        lpProfile->dwInternalFlags |= PROFILE_ADMIN_USER;
        lpProfile->dwInternalFlags &= ~PROFILE_GUEST_USER;
        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  User is a Admin")));
    }

    //
    // Decide if the central profilemage is available.
    //

    IsCentralReachable = IsCentralProfileReachable(lpProfile,
                                                   &bCreateCentralProfile,
                                                   &IsMandatory);

    if (IsCentralReachable) {

        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Central Profile is reachable")));

        if (IsMandatory) {
            lpProfile->dwInternalFlags |= PROFILE_MANDATORY;
            DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Central Profile is mandatory")));

        } else {
            lpProfile->dwInternalFlags |= PROFILE_UPDATE_CENTRAL;
            lpProfile->dwInternalFlags |= bCreateCentralProfile ? PROFILE_NEW_CENTRAL : 0;
            DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Central Profile is roaming")));

            if ((lpProfile->dwUserPreference == USERINFO_LOCAL) ||
                (lpProfile->dwInternalFlags & PROFILE_SLOW_LINK)) {
                DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Ignoring central profile due to User Preference of Local only (or slow link).")));
                IsProfilePathNULL = TRUE;
                IsCentralReachable = FALSE;
                goto CheckLocal;
            }
        }

    } else {
        if (*lpProfile->lpRoamingProfile) {
            error = GetLastError();
            DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile: IsCentralProfileReachable returned FALSE. error = %d"),
                     error));

            if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {
                dwErr = error;
                ReportError(lpProfile->dwFlags, IDS_MANDATORY_NOT_AVAILABLE2, GetErrString(error, szErr));
                goto Exit;

            } else {
                ReportError(lpProfile->dwFlags, IDS_CENTRAL_NOT_AVAILABLE, GetErrString(error, szErr));
                *lpProfile->lpRoamingProfile = TEXT('\0');
            }
        }
    }

    //
    // do not create a new profile locally if there is a central profile and
    // it is not reachable and if we do not have slow link or user preferences set.
    //

    if ((lpProfile->lpProfilePath) && (*lpProfile->lpProfilePath)) {
        if ((!IsCentralReachable) &&
            (lpProfile->dwUserPreference != USERINFO_LOCAL) && (!(lpProfile->dwInternalFlags & PROFILE_SLOW_LINK)))

            bCreateLocalProfile = FALSE;
    }

    lpRoamingProfile = lpProfile->lpRoamingProfile;

    DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Profile path = <%s>"), lpRoamingProfile ? lpRoamingProfile : TEXT("")));
    if (!lpRoamingProfile || !(*lpRoamingProfile)) {
        IsProfilePathNULL = TRUE;
    }


CheckLocal:

    //
    // Determine if the local copy of the profilemage is available.
    //

    if ((!IsProfilePathNULL) && (lpProfile->dwFlags & PI_LITELOAD) && (IsCacheDeleted())) {
        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile: Profile not loaded because Cache has to be deleted, during liteload")));
        dwErr = ERROR_INVALID_PARAMETER;
        goto Exit;
    }


    IsLocalReachable = GetExistingLocalProfileImage(lpProfile);

    if (IsLocalReachable) {
        DebugMsg((DM_VERBOSE, TEXT("Local Existing Profile Image is reachable")));
    } else {

        bNewUser = FALSE;
        if (bCreateLocalProfile)
        {
            bNewUser = CreateLocalProfileImage(lpProfile, lpProfile->lpUserName);
            if (!bNewUser) {
                DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  CreateLocalProfileImage failed. Unable to create a new profile!")));
            }
            else {
                DebugMsg((DM_VERBOSE, TEXT("Creating Local Profile")));
                IsLocalReachable = TRUE;
            }
        }

        if (!bNewUser) {

            if (lpProfile->dwFlags & PI_LITELOAD) {

                //
                // in lite load conditions we do not load a profile if it is not going to be cached on the machine.
                //

                dwErr = GetLastError();
                DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  Profile not loaded because server is unavailable during liteload")));
                goto Exit;
            }

            //
            // if the user is not admin and is not allowed to create temp profile log him out.
            //

            if ((!(lpProfile->dwInternalFlags & PROFILE_ADMIN_USER)) && (!IsTempProfileAllowed())) {
                DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  User being logged off because of no temp profile policy")));

                //
                // We have already lost the error returned from parseprofilepath. PATH_NOT_FOUND sounds quite close.
                // returning this.
                //

                dwErr = ERROR_PATH_NOT_FOUND;
                goto Exit;
            }

            if (!CreateLocalProfileImage(lpProfile, TEMP_PROFILE_NAME_BASE)) {
                dwErr = GetLastError();
                DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  CreateLocalProfileImage with TEMP failed with error %d.  Unable to issue temporary profile!"), dwErr));
                ReportError(lpProfile->dwFlags, IDS_TEMP_DIR_FAILED, GetErrString(dwErr, szErr));
                goto Exit;
            }
            else {
                ReportError(lpProfile->dwFlags, IDS_TEMPPROFILEASSIGNED);
                lpProfile->dwInternalFlags |= PROFILE_TEMP_ASSIGNED;
            }
        }

        // clear any partly loaded flag if it exists, since this is a new profile.
        lpProfile->dwInternalFlags &= ~PROFILE_PARTLY_LOADED;
        lpProfile->dwInternalFlags |= PROFILE_NEW_LOCAL;
    }


    lpLocalProfile = lpProfile->lpLocalProfile;


    DebugMsg((DM_VERBOSE, TEXT("Local profile name is <%s>"), lpLocalProfile));


    //
    // We can do a couple of quick checks here to filter out
    // new users.
    //

    if (( (lpProfile->dwInternalFlags & PROFILE_NEW_CENTRAL) &&
          (lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL) ) ||
          (!IsCentralReachable &&
          (lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL) )) {

       DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Working with a new user.  Go straight to issuing a default profile.")));
       goto IssueDefault;
    }


    //
    // If both central and local profileimages exist, reconcile them
    // and load.
    //

    if (IsCentralReachable && IsLocalReachable) {

        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Reconciling roaming profile with local profile")));

        GetExclusionList (lpProfile);


        //
        // Impersonate the user
        //

        if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile: Failed to impersonate user")));
            return FALSE;
        }


        //
        // Copy the profile
        //

        dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;

        if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {
            dwFlags |= (CPD_COPYIFDIFFERENT | CPD_SYNCHRONIZE);
        }

        if (lpProfile->dwFlags & PI_LITELOAD) {
            dwFlags |= CPD_SYSTEMFILES | CPD_SYSTEMDIRSONLY;
        }
        else
            dwFlags |= CPD_NONENCRYPTEDONLY;

        if ((lpProfile->ftProfileUnload.dwHighDateTime || lpProfile->ftProfileUnload.dwLowDateTime) &&
            lpProfile->lpExclusionList && *lpProfile->lpExclusionList) {
            dwFlags |= (CPD_USEDELREFTIME | CPD_SYNCHRONIZE | CPD_USEEXCLUSIONLIST);
        }

        bRet = CopyProfileDirectoryEx (lpRoamingProfile, lpLocalProfile,
                                       dwFlags, &lpProfile->ftProfileUnload,
                                       lpProfile->lpExclusionList);


        //
        // Revert to being 'ourself'
        //

        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile: Failed to revert to self")));
        }


        if (!bRet) {
            error = GetLastError();
            if (error == ERROR_DISK_FULL) {
                dwErr = error;
                DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  CopyProfileDirectory failed because of DISK_FULL, Exitting")));
                goto Exit;
            }

            if (error == ERROR_FILE_ENCRYPTED) {
                dwErr = error;
                DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  CopyProfileDirectory returned FALSE.  Error = %d"), error));
                ReportError(lpProfile->dwFlags, IDS_PROFILEUPDATE_6002);
                lpProfile->dwInternalFlags &= ~PROFILE_UPDATE_CENTRAL;
                // show the popup but exit only in the case if it is a new local profile

                if (lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL)
                    goto IssueDefault;
            }
            else {

                DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  CopyProfileDirectory failed.  Issuing default profile")));
                lpProfile->dwInternalFlags &= ~PROFILE_UPDATE_CENTRAL;
                lpProfile->dwInternalFlags |= PROFILE_DELETE_CACHE;
                goto IssueDefault;
            }
        }

        lstrcpy (szProfile, lpLocalProfile);
        lpEnd = CheckSlash(szProfile);

        if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {
            lstrcpy (lpEnd, c_szNTUserMan);
        } else {
            lstrcpy (lpEnd, c_szNTUserDat);
        }

        error = MyRegLoadKey(HKEY_USERS, SidString, szProfile);
        bProfileLoaded = (error == ERROR_SUCCESS);


        //
        // If we failed to load the central profile for some
        // reason, don't update it when we log off.
        //

        if (bProfileLoaded) {
            goto Exit;

        } else {
            dwErr = error;

            lpProfile->dwInternalFlags &= ~PROFILE_UPDATE_CENTRAL;
            lpProfile->dwInternalFlags |= PROFILE_DELETE_CACHE;

            if (error == ERROR_BADDB) {
                ReportError(lpProfile->dwFlags, IDS_FAILED_LOAD_1009);
                goto IssueDefault;
            } else if (error == ERROR_NO_SYSTEM_RESOURCES) {
                ReportError(lpProfile->dwFlags, IDS_FAILED_LOAD_1450);
                goto Exit;
            }
            else {
                ReportError(lpProfile->dwFlags, IDS_FAILED_LOAD_LOCAL, GetErrString(error, szErr));
                goto IssueDefault;
            }
        }
    }


    //
    // Only a local profile exists so use it.
    //

    if (!IsCentralReachable && IsLocalReachable) {

        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  No central profile.  Attempting to load local profile.")));

        lstrcpy (szProfile, lpLocalProfile);
        lpEnd = CheckSlash(szProfile);

        if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {
            lstrcpy (lpEnd, c_szNTUserMan);
        } else {
            lstrcpy (lpEnd, c_szNTUserDat);
        }

        error = MyRegLoadKey(HKEY_USERS, SidString, szProfile);
        bProfileLoaded = (error == ERROR_SUCCESS);

        if (!bProfileLoaded) {

            DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  MyRegLoadKey returned FALSE.")));
            dwErr = error;

            if (error == ERROR_BADDB) {
                ReportError(lpProfile->dwFlags, IDS_FAILED_LOAD_1009);
                lpProfile->dwInternalFlags |= PROFILE_DELETE_CACHE;
                goto IssueDefault;
            } else if (error == ERROR_NO_SYSTEM_RESOURCES) {
                ReportError(lpProfile->dwFlags, IDS_FAILED_LOAD_1450);
                goto Exit;
            } else {
                ReportError(lpProfile->dwFlags, IDS_FAILED_LOAD_LOCAL, GetErrString(error, szErr));
            }
        }

        if (!bProfileLoaded && IsProfilePathNULL) {
            DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Failed to load local profile and profile path is NULL, going to overwrite local profile")));
            lpProfile->dwInternalFlags |= PROFILE_DELETE_CACHE;
            goto IssueDefault;
        }
        goto Exit;
    }


    //
    // Last combination.  Unable to access a local profile cache,
    // but a central profile exists.  Use the temporary profile.
    //


    if (IsCentralReachable) {

        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Using temporary cache with central profile")));

        GetExclusionList (lpProfile);

        //
        // Impersonate the user
        //


        if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile: Failed to impersonate user")));
            dwErr = GetLastError();
            goto Exit;
        }


        dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;
        dwFlags |= CPD_SYNCHRONIZE;

        if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {
            dwFlags |= CPD_COPYIFDIFFERENT;
        }

        if (lpProfile->dwFlags & PI_LITELOAD) {
            dwFlags |= CPD_SYSTEMFILES | CPD_SYSTEMDIRSONLY;
        }
        else
            dwFlags |= CPD_NONENCRYPTEDONLY;

        if ((lpProfile->ftProfileUnload.dwHighDateTime || lpProfile->ftProfileUnload.dwLowDateTime) &&
            lpProfile->lpExclusionList && *lpProfile->lpExclusionList) {

            dwFlags |= (CPD_USEDELREFTIME | CPD_SYNCHRONIZE | CPD_USEEXCLUSIONLIST);
        }


        bRet = CopyProfileDirectoryEx (lpRoamingProfile,
                                       lpLocalProfile,
                                       dwFlags, &lpProfile->ftProfileUnload,
                                       lpProfile->lpExclusionList);


        //
        // Revert to being 'ourself'
        //

        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile: Failed to revert to self")));
        }


        //
        // Check return value
        //

        if (!bRet) {
            error = GetLastError();

            if (error == ERROR_FILE_ENCRYPTED) {
                dwErr = error;
                DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  CopyProfileDirectory returned FALSE.  Error = %d"), error));

                ReportError(lpProfile->dwFlags, IDS_PROFILEUPDATE_6002);
                lpProfile->dwInternalFlags &= ~PROFILE_UPDATE_CENTRAL;

                // show the popup but exit only in the case if it is a new local profile
                if (lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL)
                    goto IssueDefault;

            }
            else {
                DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  CopyProfileDirectory returned FALSE.  Error = %d"), error));
                goto Exit;
            }
        }

        lstrcpy (szProfile, lpLocalProfile);
        lpEnd = CheckSlash(szProfile);

        if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {
            lstrcpy (lpEnd, c_szNTUserMan);
        } else {
            lstrcpy (lpEnd, c_szNTUserDat);
        }

        error = MyRegLoadKey(HKEY_USERS, SidString, szProfile);

        bProfileLoaded = (error == ERROR_SUCCESS);


        if (bProfileLoaded) {
            goto Exit;
        }

        SetLastError(error);
        dwErr = error;

        if (error == ERROR_BADDB) {
            ReportError(lpProfile->dwFlags, IDS_FAILED_LOAD_1009);
            // fall through
        } else if (error == ERROR_NO_SYSTEM_RESOURCES) {
            ReportError(lpProfile->dwFlags, IDS_FAILED_LOAD_1450);
            goto Exit;
        }

        //
        // we will delete the contents at this point
        //

        lpProfile->dwInternalFlags |= PROFILE_DELETE_CACHE;
    }


IssueDefault:

    DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Issuing default profile")));

    //
    // If a cache exists, delete it, since we will
    // generate a new one below.
    //

    if (lpProfile->dwInternalFlags & PROFILE_DELETE_CACHE) {
        DWORD dwDeleteFlags=0;

        DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  Deleting cached profile directory <%s>."), lpLocalProfile));

        lpProfile->dwInternalFlags &= ~PROFILE_DELETE_CACHE;


        if ((!(lpProfile->dwInternalFlags & PROFILE_ADMIN_USER)) && (!IsTempProfileAllowed())) { 

            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  User being logged off because of no temp profile policy and is not an admin")));

            //
            // We should have some error from a prev. operation. depending on that.
            //

            goto Exit;
        }


        if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {

            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  User being logged off because the profile is mandatory")));

            //
            // We should have some error from a prev. operation. depending on that.
            //

            goto Exit;
        }


        //
        // backup only if we are not using a temp profile already.
        //

        if (!(lpProfile->dwInternalFlags & PROFILE_TEMP_ASSIGNED))
            dwDeleteFlags |= DP_BACKUP;

        if ((dwDeleteFlags & DP_BACKUP) && (lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL)) {
            dwDeleteFlags = 0;
        }

        if (!DeleteProfileEx (SidString, lpLocalProfile, dwDeleteFlags, HKEY_LOCAL_MACHINE)) {
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  DeleteProfileDirectory returned false.  Error = %d"), GetLastError()));
        }
        else {
            if (dwDeleteFlags & DP_BACKUP) {
                lpProfile->dwInternalFlags |= PROFILE_BACKUP_EXISTS;
                ReportError(PI_NOUI, IDS_PROFILE_DIR_BACKEDUP);
            }
        }

        if (lpProfile->dwFlags & PI_LITELOAD) {

            //
            // in lite load conditions we do not load a profile if it is not going to be cached on the machine.
            //

            // dwErr should be set before, use the same.

            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  Profile not loaded because server is unavailable during liteload")));
            goto Exit;
        }

        
        //
        // Create a local profile to work with
        //

        if (!CreateLocalProfileImage(lpProfile, TEMP_PROFILE_NAME_BASE)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  CreateLocalProfile Image with TEMP failed.")));
            ReportError(lpProfile->dwFlags, IDS_TEMP_DIR_FAILED, GetErrString(dwErr, szErr));
            goto Exit;
        }
        else
        {
            lpProfile->dwInternalFlags |= PROFILE_TEMP_ASSIGNED;
            lpProfile->dwInternalFlags |= PROFILE_NEW_LOCAL;
            // clear any partly loaded flag if it exists, since this is a new profile.
            lpProfile->dwInternalFlags &= ~PROFILE_PARTLY_LOADED;

            ReportError(lpProfile->dwFlags, IDS_TEMPPROFILEASSIGNED1);
        }
    }


    //
    // If a default profile location was specified, try
    // that first.
    //

    if ( !(lpProfile->dwInternalFlags & DEFAULT_NET_READY) )
    {
        CheckNetDefaultProfile (lpProfile);
    }


    if ( lpProfile->lpDefaultProfile && *lpProfile->lpDefaultProfile) {

          if (IssueDefaultProfile (lpProfile, lpProfile->lpDefaultProfile,
                                    lpLocalProfile, SidString,
                                    (lpProfile->dwInternalFlags & PROFILE_MANDATORY))) {

              DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Successfully setup the specified default.")));
              bProfileLoaded = TRUE;
              goto IssueDefaultExit;
          }

          DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  IssueDefaultProfile failed with specified default.")));
    }

    //
    // IssueLocalDefault
    //

    //
    // Issue the local default profile.
    //

    dwSize = ARRAYSIZE(szDefaultProfile);
    if (!GetDefaultUserProfileDirectory(szDefaultProfile, &dwSize)) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  Failed to get default user profile.")));
        goto Exit;
    }

    if (IssueDefaultProfile (lpProfile, szDefaultProfile,
                              lpLocalProfile, SidString,
                              (lpProfile->dwInternalFlags & PROFILE_MANDATORY))) {

        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Successfully setup the local default.")));
        bProfileLoaded = TRUE;
        goto IssueDefaultExit;
    }

    DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  IssueDefaultProfile failed with local default.")));
    dwErr = GetLastError();

IssueDefaultExit:

    //
    // If the default profile was successfully issued, then
    // we need to set the security on the hive.
    //

    if (bProfileLoaded) {
        if (!SetupNewHive(lpProfile, SidString, NULL)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  SetupNewHive failed")));
            bProfileLoaded = FALSE;
        }


    }


Exit:

    //
    // If the profile was loaded, then save the profile type in the
    // user's hive, and setup the "User Shell Folders" section for
    // Explorer.
    //

    if (bProfileLoaded) {

        //
        // Open the Current User key.  This will be closed in
        // UnloadUserProfile.
        //

        error = RegOpenKeyEx(HKEY_USERS, SidString, 0, KEY_ALL_ACCESS,
                             &lpProfile->hKeyCurrentUser);

        if (error != ERROR_SUCCESS) {
            bProfileLoaded = FALSE;
            dwErr = error;
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  Failed to open current user key. Error = %d"), error));
        }

    }

    if ((bProfileLoaded) && (!(lpProfile->dwFlags & PI_LITELOAD))) {

        //
        // merge the subtrees to create the HKCR tree
        //

        error = LoadUserClasses( lpProfile, SidString );

        if (error != ERROR_SUCCESS) {
            bProfileLoaded = FALSE;
            dwErr = error;
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  Failed to merge classes root. Error = %d"), error));
        }
    }


    if ((!bProfileLoaded) && (!(lpProfile->dwFlags & PI_LITELOAD))) {

        error = dwErr;

        //
        // If the user is an Admin, then let him/her log on with
        // either the .default profile, or an empty profile.
        //

        if (lpProfile->dwInternalFlags & PROFILE_ADMIN_USER) {
            ReportError(lpProfile->dwFlags, IDS_ADMIN_OVERRIDE, GetErrString(error, szErr));

            bProfileLoaded = TRUE;
        } else {
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile: Could not load the user profile. Error = %d"), error));
            ReportError(lpProfile->dwFlags, IDS_FAILED_LOAD_PROFILE, GetErrString(error, szErr));

            if (lpProfile->hKeyCurrentUser) {
                RegCloseKey (lpProfile->hKeyCurrentUser);
            }

            MyRegUnLoadKey(HKEY_USERS, SidString);

            if ((lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL)) {
                if (!DeleteProfileEx (SidString, lpLocalProfile, 0, HKEY_LOCAL_MACHINE)) {
                    DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  DeleteProfileDirectory returned false.  Error = %d"), GetLastError()));
                }
            }
        }
    }


    DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  About to Leave.  Final Information follows:")));
    DebugMsg((DM_VERBOSE, TEXT("Profile was %s loaded."), bProfileLoaded ? TEXT("successfully") : TEXT("NOT successfully")));
    DebugMsg((DM_VERBOSE, TEXT("lpProfile->lpRoamingProfile = <%s>"), lpProfile->lpRoamingProfile));
    DebugMsg((DM_VERBOSE, TEXT("lpProfile->lpLocalProfile = <%s>"), lpProfile->lpLocalProfile));
    DebugMsg((DM_VERBOSE, TEXT("lpProfile->dwInternalFlags = 0x%x"), lpProfile->dwInternalFlags));


    //
    // Free up the user's sid string
    //

    DeleteSidString(SidString);

    if (bProfileLoaded) {
        if (!(lpProfile->dwFlags & PI_LITELOAD)) {
            // clear any partly loaded flag if it exists, since this is a new profile.
            lpProfile->dwInternalFlags &= ~PROFILE_PARTLY_LOADED;
        }
        else {
            if (lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL) 
               lpProfile->dwInternalFlags |= PROFILE_PARTLY_LOADED;
        }
    }

    if (bProfileLoaded)
        SetLastError(dwErr1);
    else {

        //
        // Make sure that at least some error is returned.
        //

        if (!dwErr) {
            dwErr = ERROR_BAD_ENVIRONMENT;
        }
        SetLastError(dwErr);
    }


    DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Leaving.")));

    return bProfileLoaded;
}



//***************************************************************************
//
//  GetProfileSidString
//
//  Purpose:    Allocates and returns a string representing the sid that we should
//              for the profiles
//
//  Parameters: hToken          -   user's token
//
//  Return:     SidString is successful
//              NULL if an error occurs
//
//  Comments:
//              Tries to get the old sid that we used using the profile guid.
//              if it doesn't exist get the sid directly from the token
//
//  History:    Date        Author     Comment
//              11/14/95    ushaji     created
//***************************************************************************

LPTSTR GetProfileSidString(HANDLE hToken)
{
    LPTSTR lpSidString;
    TCHAR LocalProfileKey[MAX_PATH];
    LONG error;
    HKEY hSubKey;

    //
    // First, get the current user's sid and see if we have
    // profile information for them.
    //

    lpSidString = GetSidString(hToken);

    if (lpSidString) {

        lstrcpy(LocalProfileKey, PROFILE_LIST_PATH);
        lstrcat(LocalProfileKey, TEXT("\\"));
        lstrcat(LocalProfileKey, lpSidString);

        error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, LocalProfileKey, 0,
                             KEY_READ, &hSubKey);

        if (error == ERROR_SUCCESS) {
           RegCloseKey(hSubKey);
           return lpSidString;
        }

        DeleteSidString(lpSidString);
    }


    //
    // Check for an old sid string
    //

    lpSidString = GetOldSidString(hToken, PROFILE_GUID_PATH);

    if (!lpSidString) {

        //
        // Last resort, use the user's current sid
        //

        DebugMsg((DM_VERBOSE, TEXT("GetProfileSid: No Guid -> Sid Mapping available")));
        lpSidString = GetSidString(hToken);
    }

    return lpSidString;
}


//*************************************************************
//
//  TestIfUserProfileLoaded()
//
//  Purpose:    Test to see if this user's profile is loaded.
//
//  Parameters: hToken          -   user's token
//              lpProfileInfo   -   Profile information from app
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Ported
//
//*************************************************************

BOOL TestIfUserProfileLoaded(HANDLE hToken, LPPROFILEINFO lpProfileInfo)
{
    LPTSTR SidString;
    DWORD error;
    HKEY hSubKey;


    //
    // Get the Sid string for the user
    //

    SidString = GetProfileSidString(hToken);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("TestIfUserProfileLoaded:  Failed to get sid string for user")));
        return FALSE;
    }


    error = RegOpenKeyEx(HKEY_USERS, SidString, 0, KEY_ALL_ACCESS, &hSubKey);

    if (error == ERROR_SUCCESS) {

        DebugMsg((DM_VERBOSE, TEXT("TestIfUserProfileLoaded:  Profile already loaded.")));

        //
        // This key will be closed in UnloadUserProfile
        //

        lpProfileInfo->hProfile = (HANDLE) hSubKey;
    }

    SetLastError(error);
    DeleteSidString(SidString);

    return(error == ERROR_SUCCESS);
}

//*************************************************************
//
//  SecureUserKey()
//
//  Purpose:    Sets security on a key in the user's hive
//              so only admin's can change it.
//
//  Parameters: lpProfile       -   Profile Information
//              lpKey           -   Key to secure
//              pSid            -   Sid (used by CreateNewUser)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/20/95     ericflo    Created
//
//*************************************************************

BOOL SecureUserKey(LPPROFILE lpProfile, LPTSTR lpKey, PSID pSid)
{
    DWORD Error, IgnoreError;
    HKEY RootKey;
    SECURITY_DESCRIPTOR sd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUser = NULL, psidSystem = NULL, psidAdmin = NULL, psidRestricted = NULL;
    DWORD cbAcl, AceIndex, dwDisp;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bFreeSid = TRUE;
    DWORD dwFlags = 0;


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("SecureUserKey:  Entering")));


    //
    // Create the security descriptor
    //

    //
    // Give the user access by their real sid so they still have access
    // when they logoff and logon again
    //

    if (pSid) {
        psidUser = pSid;
        bFreeSid = FALSE;
        dwFlags = PI_NOUI;
    } else {
        psidUser = GetUserSid(lpProfile->hToken);
        dwFlags = lpProfile->dwFlags;
    }

    if (!psidUser) {
        DebugMsg((DM_WARNING, TEXT("SecureUserKey:  Failed to get user sid")));
        return FALSE;
    }



    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to initialize system sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to initialize admin sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the restricted sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_RESTRICTED_CODE_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidRestricted)) {
         DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to initialize restricted sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin)) + (2 * GetLengthSid (psidRestricted)) +
            sizeof(ACL) +
            (8 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    AceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidUser)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for user.  Error = %d"), GetLastError()));
        goto Exit;
    }


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for system.  Error = %d"), GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for admin.  Error = %d"), GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidRestricted)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for restricted.  Error = %d"), GetLastError()));
        goto Exit;
    }



    //
    // Now the inheritable ACEs
    //

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ, psidUser)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for user.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for system.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for admin.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ, psidRestricted)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for restricted.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Open the root of the user's profile
    //

    Error = RegCreateKeyEx(HKEY_USERS,
                         lpKey,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                         NULL,
                         &RootKey,
                         &dwDisp);

    if (Error != ERROR_SUCCESS) {

        DebugMsg((DM_WARNING, TEXT("SecureUserKey: Failed to open root of user registry, error = %d"), Error));

    } else {

        //
        // Set the security descriptor on the key
        //

        Error = ApplySecurityToRegistryTree(RootKey, &sd);


        if (Error == ERROR_SUCCESS) {
            bRetVal = TRUE;

        } else {

            DebugMsg((DM_WARNING, TEXT("SecureUserKey:  Failed to apply security to registry key, error = %d"), Error));
            SetLastError(Error);
        }

        RegCloseKey(RootKey);
    }


Exit:

    //
    // Free the sids and acl
    //

    if (bFreeSid && psidUser) {
        DeleteUserSid (psidUser);
    }

    if (psidRestricted) {
        FreeSid(psidRestricted);
    }

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("SecureUserKey:  Leaving with a return value of %d"), bRetVal));


    return(bRetVal);

}


//*************************************************************
//
//  ApplySecurityToRegistryTree()
//
//  Purpose:    Applies the passed security descriptor to the passed
//              key and all its descendants.  Only the parts of
//              the descriptor inddicated in the security
//              info value are actually applied to each registry key.
//
//  Parameters: RootKey   -     Registry key
//              pSD       -     Security Descriptor
//
//  Return:     ERROR_SUCCESS if successful
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/19/95     ericflo    Created
//
//*************************************************************

DWORD ApplySecurityToRegistryTree(HKEY RootKey, PSECURITY_DESCRIPTOR pSD)

{
    DWORD Error, IgnoreError;
    DWORD SubKeyIndex;
    LPTSTR SubKeyName;
    HKEY SubKey;
    DWORD cchSubKeySize = MAX_PATH + 1;



    //
    // First apply security
    //

    RegSetKeySecurity(RootKey, DACL_SECURITY_INFORMATION, pSD);


    //
    // Open each sub-key and apply security to its sub-tree
    //

    SubKeyIndex = 0;

    SubKeyName = GlobalAlloc (GPTR, cchSubKeySize * sizeof(TCHAR));

    if (!SubKeyName) {
        DebugMsg((DM_WARNING, TEXT("ApplySecurityToRegistryTree:  Failed to allocate memory, error = %d"), GetLastError()));
        return GetLastError();
    }

    while (TRUE) {

        //
        // Get the next sub-key name
        //

        Error = RegEnumKey(RootKey, SubKeyIndex, SubKeyName, cchSubKeySize);


        if (Error != ERROR_SUCCESS) {

            if (Error == ERROR_NO_MORE_ITEMS) {

                //
                // Successful end of enumeration
                //

                Error = ERROR_SUCCESS;

            } else {

                DebugMsg((DM_WARNING, TEXT("ApplySecurityToRegistryTree:  Registry enumeration failed with error = %d"), Error));
            }

            break;
        }


        //
        // Open the sub-key
        //

        Error = RegOpenKeyEx(RootKey,
                             SubKeyName,
                             0,
                             WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                             &SubKey);

        if (Error == ERROR_SUCCESS) {

            //
            // Apply security to the sub-tree
            //

            ApplySecurityToRegistryTree(SubKey, pSD);


            //
            // We're finished with the sub-key
            //

            RegCloseKey(SubKey);
        }


        //
        // Go enumerate the next sub-key
        //

        SubKeyIndex ++;
    }


    GlobalFree (SubKeyName);

    return Error;

}


//*************************************************************
//
//  SetDefaultUserHiveSecurity()
//
//  Purpose:    Initializes a user hive with the
//              appropriate acls
//
//  Parameters: lpProfile       -   Profile Information
//              pSid            -   Sid (used by CreateNewUser)
//              RootKey         -   registry handle to hive root
//
//  Return:     ERROR_SUCCESS if successful
//              other error code  if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/18/95     ericflo    Created as part of
//                                       SetupNewHive
//              3/29/98     adamed     Moved out of SetupNewHive
//                                       to this function
//
//*************************************************************

BOOL SetDefaultUserHiveSecurity(LPPROFILE lpProfile, PSID pSid, HKEY RootKey)
{
    DWORD Error;
    SECURITY_DESCRIPTOR sd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUser = NULL, psidSystem = NULL, psidAdmin = NULL, psidRestricted = NULL;
    DWORD cbAcl, AceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bFreeSid = TRUE;
    DWORD dwFlags = 0;


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity:  Entering")));


    //
    // Create the security descriptor that will be applied to each key
    //

    //
    // Give the user access by their real sid so they still have access
    // when they logoff and logon again
    //

    if (pSid) {
        psidUser = pSid;
        bFreeSid = FALSE;
        dwFlags = PI_NOUI;
    } else {
        psidUser = GetUserSid(lpProfile->hToken);
        dwFlags = lpProfile->dwFlags;
    }

    if (!psidUser) {
        DebugMsg((DM_WARNING, TEXT("SetDefaultUserHiveSecurity:  Failed to get user sid")));
        return FALSE;
    }



    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to initialize system sid.  Error = %d"),
                   GetLastError()));
         goto Exit;
    }


    //
    // Get the admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to initialize admin sid.  Error = %d"),
                   GetLastError()));
         goto Exit;
    }

    //
    // Get the Restricted sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_RESTRICTED_CODE_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidRestricted)) {
         DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to initialize restricted sid.  Error = %d"),
                   GetLastError()));
         goto Exit;
    }



    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin)) + (2*GetLengthSid(psidRestricted)) +
            sizeof(ACL) +
            (8 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    AceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidUser)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for user.  Error = %d"), GetLastError()));
        goto Exit;
    }


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for system.  Error = %d"), GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for admin.  Error = %d"), GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidRestricted)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for Restricted.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Now the inheritable ACEs
    //

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidUser)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for user.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for system.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for admin.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ, psidRestricted)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for restricted.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }

    //
    // Set the security descriptor on the entire tree
    //

    Error = ApplySecurityToRegistryTree(RootKey, &sd);

    if (ERROR_SUCCESS == Error) {
        bRetVal = TRUE;
    }
    else
        SetLastError(Error);

Exit:

    //
    // Free the sids and acl
    //

    if (bFreeSid && psidUser) {
        DeleteUserSid (psidUser);
    }

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (psidRestricted) {
        FreeSid(psidRestricted);
    }
    
    if (pAcl) {
        GlobalFree (pAcl);
    }

    return bRetVal;
}



//*************************************************************
//
//  SetupNewHive()
//
//  Purpose:    Initializes the new user hive created by copying
//              the default hive.
//
//  Parameters: lpProfile       -   Profile Information
//              lpSidString     -   Sid string
//              pSid            -   Sid (used by CreateNewUser)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/18/95     ericflo    Created
//
//*************************************************************

BOOL SetupNewHive(LPPROFILE lpProfile, LPTSTR lpSidString, PSID pSid)
{
    DWORD Error, IgnoreError;
    HKEY RootKey;
    BOOL bRetVal = FALSE;
    DWORD dwFlags = 0;
    TCHAR szErr[MAX_PATH];


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("SetupNewHive:  Entering")));


    if (pSid) {
        dwFlags = PI_NOUI;
    } else {
        dwFlags = lpProfile->dwFlags;
    }

    //
    // Open the root of the user's profile
    //

    Error = RegOpenKeyEx(HKEY_USERS,
                         lpSidString,
                         0,
                         WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                         &RootKey);

    if (Error != ERROR_SUCCESS) {

        DebugMsg((DM_WARNING, TEXT("SetupNewHive: Failed to open root of user registry, error = %d"), Error));

    } else {

        //
        // First Secure the entire hive -- use security that
        // will be sufficient for most of the hive.
        // After this, we can add special settings to special
        // sections of this hive.
        //

        if (SetDefaultUserHiveSecurity(lpProfile, pSid, RootKey)) {

            TCHAR szSubKey[MAX_PATH];
            LPTSTR lpEnd;

            //
            // Change the security on certain keys in the user's registry
            // so that only Admin's and the OS have write access.
            //

            lstrcpy (szSubKey, lpSidString);
            lpEnd = CheckSlash(szSubKey);
            lstrcpy (lpEnd, WINDOWS_POLICIES_KEY);

            if (!SecureUserKey(lpProfile, szSubKey, pSid)) {
                DebugMsg((DM_WARNING, TEXT("SetupNewHive: Failed to secure windows policies key")));
            }

            lstrcpy (lpEnd, ROOT_POLICIES_KEY);

            if (!SecureUserKey(lpProfile, szSubKey, pSid)) {
                DebugMsg((DM_WARNING, TEXT("SetupNewHive: Failed to secure root policies key")));
            }


            bRetVal = TRUE;

        } else {
            Error = GetLastError();
            DebugMsg((DM_WARNING, TEXT("SetupNewHive:  Failed to apply security to user registry tree, error = %d"), Error));
            ReportError(dwFlags, IDS_SECURITY_FAILED, GetErrString(Error, szErr));
        }

        RegFlushKey (RootKey);

        IgnoreError = RegCloseKey(RootKey);
        if (IgnoreError != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SetupNewHive:  Failed to close reg key, error = %d"), IgnoreError));
        }
    }

    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("SetupNewHive:  Leaving with a return value of %d"), bRetVal));

    if (!bRetVal)
        SetLastError(Error);
    return(bRetVal);

}


//*************************************************************
//
//  IsCentralProfileReachable()
//
//  Purpose:    Checks to see if the user can access the
//              central profile.
//
//  Parameters: lpProfile             - User's token
//              bCreateCentralProfile - Should the central profile be created
//              bMandatory            - Is this a mandatory profile
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/20/95     ericflo    Ported
//
//*************************************************************

BOOL IsCentralProfileReachable(LPPROFILE lpProfile, BOOL *bCreateCentralProfile,
                               BOOL *bMandatory)
{
    HANDLE  hFile;
    TCHAR   szProfile[MAX_PATH];
    LPTSTR  lpProfilePath, lpEnd;
    BOOL    bRetVal = FALSE;
    DWORD   dwError;
    HANDLE  hOldToken;


    dwError = GetLastError();

    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Entering")));


    //
    // Setup default values
    //

    *bMandatory = FALSE;
    *bCreateCentralProfile = FALSE;


    //
    // Check parameters
    //

    if (lpProfile->lpRoamingProfile[0] == TEXT('\0')) {
        DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Null path.  Leaving")));
        return FALSE;
    }


    lpProfilePath = lpProfile->lpRoamingProfile;


    //
    // Make sure we don't overrun our temporary buffer
    //

    if ((lstrlen(lpProfilePath) + 1 + lstrlen(c_szNTUserMan + 1)) > MAX_PATH) {
        DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Failed because temporary buffer is too small.")));
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }


    //
    // Copy the profile path to a temporary buffer
    // we can munge it.
    //

    lstrcpy (szProfile, lpProfilePath);


    //
    // Add the slash if appropriate and then tack on
    // ntuser.man.
    //

    lpEnd = CheckSlash(szProfile);
    lstrcpy(lpEnd, c_szNTUserMan);


    //
    // Impersonate the user
    //


    if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("IsCentralProfileReachable: Failed to impersonate user")));
        return FALSE;
    }


    //
    // See if this file exists
    //

    DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Testing <%s>"), szProfile));

    hFile = CreateFile(szProfile, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


    if (hFile != INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Found a mandatory profile.")));
        CloseHandle(hFile);
        *bMandatory = TRUE;
        bRetVal = TRUE;
        goto Exit;
    }


    dwError = GetLastError();
    DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Profile is not reachable, error = %d"),
                          dwError));


    //
    // If we received an error other than file not
    // found, bail now because we won't be able to
    // access this location.
    //

    if (dwError != ERROR_FILE_NOT_FOUND) {
        DebugMsg((DM_WARNING, TEXT("IsCentralProfileReachable:  Profile path <%s> is not reachable, error = %d"),
                                        szProfile, dwError));
        goto Exit;
    }


    //
    // Now try ntuser.dat
    //

    lstrcpy(lpEnd, c_szNTUserDat);


    //
    // See if this file exists.
    //

    DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Testing <%s>"), szProfile));

    hFile = CreateFile(szProfile, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


    if (hFile != INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Found a user profile.")));
        CloseHandle(hFile);
        bRetVal = TRUE;
        goto Exit;
    }


    dwError = GetLastError();
    DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Profile is not reachable, error = %d"),
                          dwError));


    if (dwError == ERROR_FILE_NOT_FOUND) {
        DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Ok to create a user profile.")));
        *bCreateCentralProfile = TRUE;
        bRetVal = TRUE;
        goto Exit;
    }


    DebugMsg((DM_WARNING, TEXT("IsCentralProfileReachable:  Profile path <%s> is not reachable(2), error = %d"),
                                    szProfile, dwError));

Exit:


    //
    // Go back to system security context
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("IsCentralProfileReachable: Failed to revert to self")));
    }

    return bRetVal;
}

//*************************************************************
//
//  MyRegLoadKey()
//
//  Purpose:    Loads a hive into the registry
//
//  Parameters: hKey        -   Key to load the hive into
//              lpSubKey    -   Subkey name
//              lpFile      -   hive filename
//
//  Return:     ERROR_SUCCESS if successful
//              Error number if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/22/95     ericflo    Created
//
//*************************************************************

LONG MyRegLoadKey(HKEY hKey, LPTSTR lpSubKey, LPTSTR lpFile)
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    int error;
    TCHAR szErr[MAX_PATH];

    //
    // Enable the restore privilege
    //

    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (NT_SUCCESS(Status)) {

        error = RegLoadKey(hKey, lpSubKey, lpFile);

        //
        // Restore the privilege to its previous state
        //

        Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
        if (!NT_SUCCESS(Status)) {
            DebugMsg((DM_WARNING, TEXT("MyRegLoadKey:  Failed to restore RESTORE privilege to previous enabled state")));
        }


        //
        // Convert a sharing violation error to success since the hive
        // is already loaded
        //

        if (error == ERROR_SHARING_VIOLATION) {
            error = ERROR_SUCCESS;
        }


        //
        // Check if the hive was loaded
        //

        if (error != ERROR_SUCCESS) {
            ReportError(PI_NOUI, IDS_REGLOADKEYFAILED, GetErrString(error, szErr), lpFile);
            DebugMsg((DM_WARNING, TEXT("MyRegLoadKey:  Failed to load subkey <%s>, error =%d"), lpSubKey, error));
        }

    } else {
        error = GetLastError();
        DebugMsg((DM_WARNING, TEXT("MyRegLoadKey:  Failed to enable restore privilege to load registry key")));
    }

    DebugMsg((DM_VERBOSE, TEXT("MyRegLoadKey: Mutex released.  Returning %d."), error));

    return error;
}


//*************************************************************
//
//  MyRegUnLoadKey()
//
//  Purpose:    Unloads a registry key
//
//  Parameters: hKey          -  Registry handle
//              lpSubKey      -  Subkey to be unloaded
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Ported
//
//*************************************************************

BOOL MyRegUnLoadKey(HKEY hKey, LPTSTR lpSubKey)
{
    BOOL bResult = TRUE;
    LONG error;
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    HKEY hSubKey;
    DWORD dwSize, dwType, dwCount = 0, dwMaxWait = 60;
    BOOL bDisableProfileUnloadMsg = FALSE;


    //
    // Enable the restore privilege
    //

    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (NT_SUCCESS(Status)) {

        error = RegUnLoadKey(hKey, lpSubKey);

        if (error == ERROR_ACCESS_DENIED) {

            //
            // Check for a max wait machine preference
            //

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0, KEY_READ,
                             &hSubKey) == ERROR_SUCCESS) {

                dwSize = sizeof(dwMaxWait);
                RegQueryValueEx(hSubKey, PROFILE_UNLOAD_TIMEOUT, NULL, &dwType,
                                (LPBYTE) &dwMaxWait, &dwSize);

                dwSize = sizeof(bDisableProfileUnloadMsg);
                RegQueryValueEx(hSubKey, DISABLE_PROFILE_UNLOAD_MSG, NULL, &dwType,
                                (LPBYTE) &bDisableProfileUnloadMsg, &dwSize);

                RegCloseKey(hSubKey);
            }


            //
            // Check for a max wait machine policy
            //

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SYSTEM_POLICIES_KEY, 0, KEY_READ,
                             &hSubKey) == ERROR_SUCCESS) {

                dwSize = sizeof(dwMaxWait);
                RegQueryValueEx(hSubKey, PROFILE_UNLOAD_TIMEOUT, NULL, &dwType,
                                (LPBYTE) &dwMaxWait, &dwSize);

                dwSize = sizeof(bDisableProfileUnloadMsg);
                RegQueryValueEx(hSubKey, DISABLE_PROFILE_UNLOAD_MSG, NULL, &dwType,
                                (LPBYTE) &bDisableProfileUnloadMsg, &dwSize);

                RegCloseKey(hSubKey);
            }


            //
            // If appropriate, sleep and loop trying to unload the registry hive
            //

            if (dwMaxWait) {

                DebugMsg((DM_WARNING, TEXT("MyRegUnLoadKey: Hive unload for %s failed due to open registry key.  Windows will try unloading the registry hive once a second for the next %d seconds (max)."),
                         lpSubKey, dwMaxWait));

                while (error == ERROR_ACCESS_DENIED) {

                    Sleep (1000);

                    error = RegUnLoadKey(hKey, lpSubKey);

                    dwCount++;

                    if (dwCount >= dwMaxWait) {
                        break;
                    }
                }

                if (error == ERROR_SUCCESS) {
                    DebugMsg((DM_WARNING, TEXT("MyRegUnLoadKey: The registry hive was successfully unloaded after %d seconds."), dwCount));

                    if (!bDisableProfileUnloadMsg) {
                        LogEvent (FALSE, IDS_HIVE_UNLOAD_RETRY, dwCount);
                    }
                } else {
                    DebugMsg((DM_WARNING, TEXT("MyRegUnLoadKey: Windows was not able to unload the registry hive.")));
                }
            }
        }


        if ( error != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("MyRegUnLoadKey:  Failed to unmount hive %x"), error));
            SetLastError(error);
            bResult = FALSE;
        }

        //
        // Restore the privilege to its previous state
        //

        Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
        if (!NT_SUCCESS(Status)) {
            DebugMsg((DM_WARNING, TEXT("MyRegUnLoadKey:  Failed to restore RESTORE privilege to previous enabled state")));
        }

    } else {
        DebugMsg((DM_WARNING, TEXT("MyRegUnloadKey:  Failed to enable restore privilege to unload registry key")));
        SetLastError(Status);
        bResult = FALSE;
    }

    DebugMsg((DM_VERBOSE, TEXT("MyRegUnloadKey: Mutex released.  Returning %d."), bResult));

    return bResult;
}

//*************************************************************
//
//  UpgradeLocalProfile()
//
//  Purpose:    Upgrades a local profile from a 3.x profile
//              to a profile directory structure.
//
//  Parameters: lpProfile       -   Profile Information
//              lpOldProfile    -   Previous profile file
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/6/95      ericflo    Created
//
//*************************************************************

BOOL UpgradeLocalProfile (LPPROFILE lpProfile, LPTSTR lpOldProfile)
{
    TCHAR szSrc[MAX_PATH];
    TCHAR szDest[MAX_PATH];
    LPTSTR lpSrcEnd, lpDestEnd;
    BOOL bRetVal = FALSE;
    DWORD dwSize, dwFlags;
    HANDLE hOldToken;
    DWORD dwErr;

    dwErr = GetLastError();

    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("UpgradeLocalProfile:  Entering")));


    //
    // Setup the temporary buffers
    //

    lstrcpy (szSrc, lpOldProfile);
    lstrcpy (szDest, lpProfile->lpLocalProfile);

    lpDestEnd = CheckSlash (szDest);
    lstrcpy (lpDestEnd, c_szNTUserDat);


    //
    // Copy the hive
    //

    if (!CopyFile(szSrc, szDest, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: CopyFile failed to copy hive with error = %d"),
                 GetLastError()));
        return FALSE;
    }


    //
    // Delete the old hive
    //

    DeleteFile (szSrc);



    //
    // Copy log file
    //

    lstrcat (szSrc, c_szLog);
    lstrcat (szDest, c_szLog);


    if (!CopyFile(szSrc, szDest, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: CopyFile failed to copy hive log with error = %d"),
                 GetLastError()));
    }


    //
    // Delete the old hive log
    //

    DeleteFile (szSrc);


    //
    // Copy in the new shell folders from the default
    //

    if ( !(lpProfile->dwInternalFlags & DEFAULT_NET_READY) ) {

        CheckNetDefaultProfile (lpProfile);
    }

    if (lpProfile->lpDefaultProfile && *lpProfile->lpDefaultProfile) {

        ExpandEnvironmentStrings(lpProfile->lpDefaultProfile, szSrc, MAX_PATH);

        if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed to impersonate user")));
            goto IssueLocalDefault;
        }

        dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;
        dwFlags |= CPD_IGNOREHIVE | CPD_CREATETITLE;
        dwFlags |= CPD_IGNOREENCRYPTEDFILES | CPD_IGNORELONGFILENAMES;

        if (CopyProfileDirectoryEx (szSrc, lpProfile->lpLocalProfile,
                                    dwFlags,
                                    NULL, NULL)) {

            bRetVal = TRUE;
        }

        //
        // Go back to system security context
        //

        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed to revert to self")));
        }

        if ((!bRetVal) && (GetLastError() == ERROR_DISK_FULL)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed to Copy default profile. Disk is FULL")));
            goto Exit;
        }
    }


IssueLocalDefault:

    if (!bRetVal) {

        dwSize = ARRAYSIZE(szSrc);
        if (!GetDefaultUserProfileDirectory(szSrc, &dwSize)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile:  Failed to get default user profile.")));
            goto Exit;
        }

        if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed to impersonate user")));
            goto Exit;
        }

        dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;
        dwFlags |= CPD_IGNOREHIVE | CPD_CREATETITLE;
        dwFlags |= CPD_IGNOREENCRYPTEDFILES | CPD_IGNORELONGFILENAMES;

        bRetVal = CopyProfileDirectoryEx (szSrc, lpProfile->lpLocalProfile,
                                          dwFlags,
                                          NULL, NULL);

        //
        // Go back to system security context
        //

        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed to revert to self")));
        }
    }

    if (!bRetVal)
        dwErr = GetLastError();

Exit:

    SetLastError(dwErr);

    return bRetVal;
}

//*************************************************************
//
//  UpgradeCentralProfile()
//
//  Purpose:    Upgrades a central profile from a 3.x profile
//              to a profile directory structure.
//
//  Parameters: lpProfile       -   Profile Information
//              lpOldProfile    -   Previous profile file
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/6/95      ericflo    Created
//
//*************************************************************

BOOL UpgradeCentralProfile (LPPROFILE lpProfile, LPTSTR lpOldProfile)
{
    TCHAR szSrc[MAX_PATH];
    TCHAR szDest[MAX_PATH];
    LPTSTR lpSrcEnd, lpDestEnd, lpDot;
    BOOL bRetVal = FALSE;
    BOOL bMandatory = FALSE;
    DWORD dwSize, dwFlags;
    HANDLE hOldToken;
    DWORD dwErr;

    dwErr = GetLastError();


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("UpgradeCentralProfile:  Entering")));


    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile: Failed to impersonate user")));
        return FALSE;
    }


    //
    // Setup the source buffer
    //

    lstrcpy (szSrc, lpOldProfile);


    //
    // Determine the profile type
    //

    lpDot = szSrc + lstrlen(szSrc) - 4;

    if (*lpDot == TEXT('.')) {
        if (!lstrcmpi (lpDot, c_szMAN)) {
            bMandatory = TRUE;
        }
    }


    //
    // Setup the destination buffer
    //

    lstrcpy (szDest, lpProfile->lpRoamingProfile);

    lpDestEnd = CheckSlash (szDest);

    if (bMandatory) {
        lstrcpy (lpDestEnd, c_szNTUserMan);
    } else {
        lstrcpy (lpDestEnd, c_szNTUserDat);
    }


    //
    // Copy the hive
    //

    if (!CopyFile(szSrc, szDest, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile: CopyFile failed to copy hive with error = %d"),
                 GetLastError()));
        DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile: Source = <%s>"), szSrc));
        DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile: Destination = <%s>"), szDest));
        dwErr = GetLastError();
        goto Exit;
    }



    //
    // Copy log file
    //

    lstrcpy (lpDot, c_szLog);
    lstrcat (szDest, c_szLog);


    if (!CopyFile(szSrc, szDest, FALSE)) {
        DebugMsg((DM_VERBOSE, TEXT("UpgradeCentralProfile: CopyFile failed to copy hive log with error = %d"),
                 GetLastError()));
        DebugMsg((DM_VERBOSE, TEXT("UpgradeCentralProfile: Source = <%s>"), szSrc));
        DebugMsg((DM_VERBOSE, TEXT("UpgradeCentralProfile: Destination = <%s>"), szDest));

    }


    //
    // Copy in the new shell folders from the default
    //

    if ( !(lpProfile->dwInternalFlags & DEFAULT_NET_READY) ) {
        CheckNetDefaultProfile (lpProfile);
    }


    if (lpProfile->lpDefaultProfile && *lpProfile->lpDefaultProfile) {

        ExpandEnvironmentStrings(lpProfile->lpDefaultProfile, szSrc, MAX_PATH);

        dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;
        dwFlags |= CPD_IGNOREHIVE | CPD_CREATETITLE;
        dwFlags |= CPD_IGNOREENCRYPTEDFILES | CPD_IGNORELONGFILENAMES;

        if (CopyProfileDirectoryEx (szSrc, lpProfile->lpRoamingProfile,
                                    dwFlags,
                                    NULL, NULL)) {

            bRetVal = TRUE;
        }
    }


    if (!bRetVal) {

        dwSize = ARRAYSIZE(szSrc);
        if (!GetDefaultUserProfileDirectory(szSrc, &dwSize)) {
            DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile:  Failed to get default user profile.")));
            dwErr = GetLastError();
            goto Exit;
        }

        dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;
        dwFlags |= CPD_IGNOREHIVE | CPD_CREATETITLE;
        dwFlags |= CPD_IGNOREENCRYPTEDFILES | CPD_IGNORELONGFILENAMES;

        bRetVal = CopyProfileDirectoryEx (szSrc, lpProfile->lpRoamingProfile,
                                          dwFlags,
                                          NULL, NULL);
    }

    if (!bRetVal)
        dwErr = GetLastError();


Exit:

    //
    // Go back to system security context
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile: Failed to revert to self")));
    }


    return bRetVal;
}


//*************************************************************
//
//  CreateSecureDirectory()
//
//  Purpose:    Creates a secure directory that only the user,
//              admin, and system have access to in the normal case
//              and for only the user and system in the restricted case.
//
//
//  Parameters: lpProfile   -   Profile Information
//              lpDirectory -   Directory Name
//              pSid        -   Sid (used by CreateUserProfile)
//              fRestricted -   Flag to set restricted access.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/20/95     ericflo    Created
//              9/30/98     ushaji     added fRestricted flag
//
//*************************************************************

BOOL CreateSecureDirectory (LPPROFILE lpProfile, LPTSTR lpDirectory, PSID pSid, BOOL fRestricted)
{
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUser = NULL, psidSystem = NULL, psidAdmin = NULL;
    DWORD cbAcl, aceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bFreeSid = TRUE;


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Entering with <%s>"), lpDirectory));



    if (!lpProfile && !pSid) {

        //
        // Attempt to create the directory
        //

        if (CreateNestedDirectory(lpDirectory, NULL)) {
            DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Created the directory <%s>"), lpDirectory));
            bRetVal = TRUE;

        } else {

            DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to created the directory <%s>"), lpDirectory));
        }

        goto Exit;
    }


    //
    // Get the SIDs we'll need for the DACL
    //

    if (pSid) {
        psidUser = pSid;
        bFreeSid = FALSE;
    } else {
        psidUser = GetUserSid(lpProfile->hToken);
    }



    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to initialize system sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the Admin sid only if Frestricted is off
    //

    if (!fRestricted)
    {
        if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
            DebugMsg((DM_VERBOSE, TEXT("SetupNewHive: Failed to initialize admin sid.  Error = %d"), GetLastError()));
            goto Exit;
        }
    }


    //
    // Allocate space for the ACL
    //

    if (!fRestricted)
    {
        cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
                (2 * GetLengthSid (psidAdmin)) + sizeof(ACL) +
                (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));
    }
    else
    {
        cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
                sizeof(ACL) +
                (4 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));
    }


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidUser)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }



    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidSystem)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }


    if (!fRestricted)
    {
        aceIndex++;
        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidAdmin)) {
            DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }
    }

    //
    // Now the inheritable ACEs
    //

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidUser)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);



    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    if (!fRestricted)
    {
        aceIndex++;
        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
            DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);



    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Add the security descriptor to the sa structure
    //

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;


    //
    // Attempt to create the directory
    //

    if (CreateNestedDirectory(lpDirectory, &sa)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Created the directory <%s>"), lpDirectory));
        bRetVal = TRUE;

    } else {

        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to created the directory <%s>"), lpDirectory));
    }



Exit:

    if (bFreeSid && psidUser) {
        DeleteUserSid (psidUser);
    }

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }

    return bRetVal;

}



//*************************************************************
//
//  GetUserDomainName()
//
//  Purpose:    Gets the current user's domain name
//
//  Parameters: lpProfile        - Profile Information
//              lpDomainName     - Receives the user's domain name
//              lpDomainNameSize - Size of the lpDomainName buffer (truncates the name to this size)
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL GetUserDomainName (LPPROFILE lpProfile, LPTSTR lpDomainName, LPDWORD lpDomainNameSize)
{
    BOOL bResult = FALSE;
    LPTSTR lpTemp, lpDomain = NULL;
    HANDLE hOldToken;
    DWORD dwErr;

    dwErr = GetLastError();


    //
    // if no lpProfile is passed e.g. in setup.c and so just ignore.
    //

    lpDomainName[0] = TEXT('\0');

    if (!lpProfile) {
        DebugMsg((DM_WARNING, TEXT("GetUserDomainName: lpProfile structure is NULL, returning")));
        return FALSE;
    }

    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("GetUserDomainName: Failed to impersonate user")));
        dwErr = GetLastError();
        goto Exit;
    }

    //
    // Get the username in NT4 format
    //

    lpDomain = MyGetUserName (NameSamCompatible);

    RevertToUser(&hOldToken);

    if (!lpDomain) {
        DebugMsg((DM_WARNING, TEXT("GetUserDomainName:  MyGetUserName failed for NT4 style name with %d"),
                 GetLastError()));
        dwErr = GetLastError();
        LogEvent (TRUE, IDS_FAILED_USERNAME, GetLastError());
        goto Exit;
    }


    //
    // Look for the \ between the domain and username and replace
    // it with a NULL
    //

    lpTemp = lpDomain;

    while (*lpTemp && ((*lpTemp) != TEXT('\\')))
        lpTemp++;


    if (*lpTemp != TEXT('\\')) {
        DebugMsg((DM_WARNING, TEXT("GetUserDomainName:  Failed to find slash in NT4 style name:  <%s>"),
                 lpDomain));
        dwErr = ERROR_INVALID_DATA;
        goto Exit;
    }

    *lpTemp = TEXT('\0');

    lstrcpyn (lpDomainName, lpDomain, (*lpDomainNameSize)-1);


    //
    // Success
    //

    DebugMsg((DM_VERBOSE, TEXT("GetUserDomainName: DomainName = <%s>"), lpDomainName));

    bResult = TRUE;

Exit:

    if (lpDomain) {
        LocalFree (lpDomain);
    }

    SetLastError(dwErr);

    return bResult;
}

//*************************************************************
//
//  ComputeLocalProfileName()
//
//  Purpose:    Constructs the pathname of the local profile
//              for this user.  It will attempt to create
//              a directory of the username, and then if
//              unsccessful it will try the username.xxx
//              where xxx is a three digit number
//
//  Parameters: lpProfile             -   Profile Information
//              lpUserName            -   UserName
//              lpProfileImage        -   Profile directory (unexpanded)
//              cchMaxProfileImage    -   lpProfileImage buffer size
//              lpExpProfileImage     -   Expanded directory
//              cchMaxExpProfileImage -   lpExpProfileImage buffer size
//              pSid                  -   User's sid
//              bWin9xUpg             -   Flag to say whether it is win9x upgrade
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   lpProfileImage should be initialized with the
//              root profile path and the trailing backslash.
//              if it is a win9x upgrade give back the user's dir and don't do
//              conflict resolution.
//
//  History:    Date        Author     Comment
//              6/20/95     ericflo    Created
//
//*************************************************************

BOOL ComputeLocalProfileName (LPPROFILE lpProfile, LPCTSTR lpUserName,
                              LPTSTR lpProfileImage, DWORD  cchMaxProfileImage,
                              LPTSTR lpExpProfileImage, DWORD  cchMaxExpProfileImage,
                              PSID pSid, BOOL bWin9xUpg)
{
    int i = 0;
    TCHAR szNumber[5], lpUserDomain[50], szDomainName[50+3];
    LPTSTR lpEnd;
    BOOL bRetVal = FALSE;
    BOOL bResult;
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    DWORD   dwDomainNamelen;
    DWORD dwErr;

    //
    // Check buffer size
    //

    dwDomainNamelen = ARRAYSIZE(lpUserDomain);

    if ((DWORD)(lstrlen(lpProfileImage) + lstrlen(lpUserName) + dwDomainNamelen + 2 + 5 + 1) > cchMaxProfileImage) {
        DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: buffer too small")));
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }

    //
    // Place the username onto the end of the profile image
    //

    lpEnd = CheckSlash (lpProfileImage);
    lstrcpy (lpEnd, lpUserName);


    //
    // Expand the profile path
    //

    ExpandEnvironmentStrings(lpProfileImage, lpExpProfileImage, cchMaxExpProfileImage);



    //
    // Does this directory exist?
    //

    hFile = FindFirstFile (lpExpProfileImage, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

        //
        // Attempt to create the directory, if it returns an error bail
        // CreateSecureDirectory does not return an error for already_exists
        // so this should be ok.
        //

        bResult = CreateSecureDirectory(lpProfile, lpExpProfileImage, pSid, FALSE);

        if (bResult) {
            DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: generated the profile directory <%s>"), lpExpProfileImage));
            bRetVal = TRUE;
        }
        else {
            DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: trying to create dir <%s> returned %d"), lpExpProfileImage, GetLastError()));
            bRetVal = FALSE;
        }
        goto Exit;

    } else {

        FindClose (hFile);
        if (bWin9xUpg) {
            DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: generated the profile directory <%s> in win9xupg case"), lpExpProfileImage));
            bRetVal = TRUE;
            goto Exit;
        }
    }



    //
    // get the User Domain Name
    //

    if (!GetUserDomainName(lpProfile, lpUserDomain, &dwDomainNamelen)) {
        DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: Couldn't get the User Domain")));
        *lpUserDomain = TEXT('\0');
    }

    lpEnd = lpProfileImage + lstrlen(lpProfileImage);

    //
    // Place the " (DomainName)" onto the end of the username
    //

    if ((*lpUserDomain) != TEXT('\0')) {
        TCHAR szFormat[30];

        LoadString (g_hDllInstance, IDS_PROFILEDOMAINNAME_FORMAT, szFormat,
                            ARRAYSIZE(szFormat));
        wsprintf(szDomainName, szFormat, lpUserDomain);
        lstrcpy(lpEnd, szDomainName);

        //
        // Expand the profile path
        //

        ExpandEnvironmentStrings(lpProfileImage, lpExpProfileImage, cchMaxExpProfileImage);


        //
        // Does this directory exist?
        //

        hFile = FindFirstFile (lpExpProfileImage, &fd);

        if (hFile == INVALID_HANDLE_VALUE) {

            //
            // Attempt to create the directory
            //

            bResult = CreateSecureDirectory(lpProfile, lpExpProfileImage, pSid, FALSE);

            if (bResult) {
                DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: generated the profile directory <%s>"), lpExpProfileImage));
                bRetVal = TRUE;
            }
            else {
                DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: trying to create dir <%s> returned %d"), lpExpProfileImage, GetLastError()));
                bRetVal = FALSE;
            }

            goto Exit;

        } else {

            FindClose (hFile);
        }
    }

    //
    // Failed to create the directory for some reason.
    // Now try username (DomanName).000, username (DomanName).001, etc
    //

    lpEnd = lpProfileImage + lstrlen(lpProfileImage);

    for (i=0; i < 1000; i++) {

        //
        // Convert the number to a string and attach it.
        //

        wsprintf (szNumber, TEXT(".%.3d"), i);
        lstrcpy (lpEnd, szNumber);


        //
        // Expand the profile path
        //

        ExpandEnvironmentStrings(lpProfileImage, lpExpProfileImage, cchMaxExpProfileImage);


        //
        // Does this directory exist?
        //

        hFile = FindFirstFile (lpExpProfileImage, &fd);

        if (hFile == INVALID_HANDLE_VALUE) {

            //
            // Attempt to create the directory
            //

            bResult = CreateSecureDirectory(lpProfile, lpExpProfileImage, pSid, FALSE);

            if (bResult) {
                DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: generated the profile directory <%s>"), lpExpProfileImage));
                bRetVal = TRUE;
            }
            else {
                DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: trying to create dir <%s> returned %d"), lpExpProfileImage, GetLastError()));
                bRetVal = FALSE;
            }

            goto Exit;

        } else {

            FindClose (hFile);
        }
    }


    DebugMsg((DM_WARNING, TEXT("ComputeLocalProfileName: Could not generate a profile directory.  Error = %d"), GetLastError()));

Exit:


    return bRetVal;
}

//*************************************************************
//
//  SetMachineProfileKeySecurity
//
//  Purpose:    Sets the security on the profile key under HKLM/ProfileList
//
//  Parameters: lpProfile   -   Profile information
//              phKey       -   Handle to registry key opened
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/22/99     ushaji     adapted
//
//*************************************************************

BOOL SetMachineProfileKeySecurity (LPPROFILE lpProfile, LPTSTR lpKeyName)
{
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidSystem = NULL, psidAdmin = NULL, psidUsers = NULL;
    PSID  psidThisUser = NULL;
    DWORD cbAcl, aceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    HKEY hKeyProfile=NULL;
    DWORD Error, dwDisp;

    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DebugMsg((DM_WARNING, TEXT("SetMachineProfileKeySecurity: Failed to initialize system sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DebugMsg((DM_WARNING, TEXT("SetMachineProfileKeySecurity: Failed to initialize admin sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the users sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_USERS,
                                  0, 0, 0, 0, 0, 0, &psidUsers)) {

         DebugMsg((DM_WARNING, TEXT("SetMachineProfileKeySecurity: Failed to initialize authenticated users sid.  Error = %d"), GetLastError()));
         goto Exit;
    }

    //
    // Get this users sid
    //

    psidThisUser = GetUserSid(lpProfile->hToken);

    //
    // Allocate space for the ACL. (No Inheritable Aces)
    //

    cbAcl = (GetLengthSid (psidSystem))    +
            (GetLengthSid (psidAdmin))     +
            (GetLengthSid (psidUsers))     +
            (GetLengthSid (psidThisUser))  +
            sizeof(ACL) +
            (4 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("SetMachineProfileKeySecurity: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Add Aces.  Non-inheritable ACEs first
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem)) {
        DebugMsg((DM_WARNING, TEXT("SetMachineProfileKeySecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin)) {
        DebugMsg((DM_WARNING, TEXT("SetMachineProfileKeySecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidUsers)) {
        DebugMsg((DM_WARNING, TEXT("SetMachineProfileKeySecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS ^ (WRITE_DAC | WRITE_OWNER), psidThisUser)) {
        DebugMsg((DM_WARNING, TEXT("SetMachineProfileKeySecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("SetMachineProfileKeySecurity: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("SetMachineProfileKeySecurity: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }

    Error = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           lpKeyName,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                           NULL,
                           &hKeyProfile,
                           &dwDisp);

    if (Error != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("SetMachineProfileKeySecurity: Couldn't open registry key to set security.  Error = %d"), Error));
        SetLastError(Error);
        goto Exit;
    }


    //
    // Set the security
    //

    Error = RegSetKeySecurity(hKeyProfile, DACL_SECURITY_INFORMATION, &sd);

    if (Error != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("SetMachineProfileKeySecurity: Couldn't set security.  Error = %d"), Error));
        SetLastError(Error);
        goto Exit;
    }
    else {
        bRetVal = TRUE;
    }


Exit:

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (psidUsers) {
        FreeSid(psidUsers);
    }

    if (psidThisUser) {
        FreeSid(psidThisUser);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }

    if (hKeyProfile) {
        RegCloseKey(hKeyProfile);
    }

    return bRetVal;
}


//*************************************************************
//
//  CreateLocalProfileKey()
//
//  Purpose:    Creates a registry key pointing at the user profile
//
//  Parameters: lpProfile   -   Profile information
//              phKey       -   Handle to registry key if successful
//              bKeyExists  -   TRUE if the registry key already existed
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/20/95     ericflo    Ported
//
//*************************************************************

BOOL CreateLocalProfileKey (LPPROFILE lpProfile, PHKEY phKey, BOOL *bKeyExists)
{
    TCHAR LocalProfileKey[MAX_PATH];
    DWORD Disposition;
    DWORD RegErr = ERROR_SUCCESS + 1;
    BOOL Result;
    LPTSTR SidString;
    DWORD dwErr;

    dwErr = GetLastError();

    SidString = GetSidString(lpProfile->hToken);
    if (SidString != NULL) {

        //
        // Call the RegCreateKey api in the user's context
        //

        lstrcpy(LocalProfileKey, PROFILE_LIST_PATH);
        lstrcat(LocalProfileKey, TEXT("\\"));
        lstrcat(LocalProfileKey, SidString);

        RegErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, LocalProfileKey, 0, 0, 0,
                                KEY_READ | KEY_WRITE, NULL, phKey, &Disposition);
        if (RegErr == ERROR_SUCCESS) {

            *bKeyExists = (BOOL)(Disposition & REG_OPENED_EXISTING_KEY);

            //
            // If the key didn't exist before and profile is not mandatory, set the security on it.
            //

            if ((!(*bKeyExists)) && (!(lpProfile->dwInternalFlags & PROFILE_MANDATORY))) {
                if (!SetMachineProfileKeySecurity(lpProfile, LocalProfileKey)) {
                    DebugMsg((DM_WARNING, TEXT("CreateLocalProfileKey:  SetMachineProfileKeySecurity Failed. Error = %d"), GetLastError()));
                }
            }
            else {
               DebugMsg((DM_VERBOSE, TEXT("CreateLocalProfileKey:  Not setting additional Security")));
            }

        } else {
           DebugMsg((DM_WARNING, TEXT("CreateLocalProfileKey:  Failed trying to create the local profile key <%s>, error = %d."), LocalProfileKey, RegErr));
           dwErr = RegErr;
        }

        DeleteSidString(SidString);
    }

    SetLastError(dwErr);    
    return(RegErr == ERROR_SUCCESS);
}


//*************************************************************
//
//  GetExistingLocalProfileImage()
//
//  Purpose:    opens the profileimagepath
//
//  Parameters: lpProfile   -   Profile information
//
//  Return:     TRUE if the profile image is reachable
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/20/95     ericflo    Ported
//              9/26/98     ushaji     Modified
//
//*************************************************************
BOOL GetExistingLocalProfileImage(LPPROFILE lpProfile)
{
    HKEY hKey;
    BOOL bKeyExists;
    TCHAR lpProfileImage[MAX_PATH];
    TCHAR lpExpProfileImage[MAX_PATH];
    TCHAR lpOldProfileImage[MAX_PATH];
    LPTSTR lpExpandedPath, lpEnd;
    DWORD cbExpProfileImage = sizeof(TCHAR)*MAX_PATH;
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    DWORD cb;
    DWORD err;
    DWORD dwType;
    DWORD dwSize;
    LONG lResult;
    DWORD dwInternalFlags = 0;
    HANDLE fh;
    BOOL bRetVal = FALSE;
    LPTSTR SidString;
    HANDLE hOldToken;

    lpProfile->lpLocalProfile[0] = TEXT('\0');


    if (!PatchNewProfileIfRequired(lpProfile->hToken)) {
        DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage: Patch Profile Image failed")));
        return FALSE;
    }

    if (!CreateLocalProfileKey(lpProfile, &hKey, &bKeyExists)) {
        return FALSE;   // not reachable and cannot keep a local copy
    }

    if (bKeyExists) {

        //
        // Check if the local profile image is valid.
        //

        DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Found entry in profile list for existing local profile")));

        err = RegQueryValueEx(hKey, PROFILE_IMAGE_VALUE_NAME, 0, &dwType,
            (LPBYTE)lpExpProfileImage, &cbExpProfileImage);
        if (err == ERROR_SUCCESS && cbExpProfileImage) {
            DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Local profile image filename = <%s>"), lpExpProfileImage));

            if (dwType == REG_EXPAND_SZ) {

                //
                // Expand the profile image filename
                //

                cb = sizeof(lpExpProfileImage);
                lpExpandedPath = LocalAlloc(LPTR, cb);
                if (lpExpandedPath) {
                    ExpandEnvironmentStrings(lpExpProfileImage, lpExpandedPath, cb);
                    lstrcpy(lpExpProfileImage, lpExpandedPath);
                    LocalFree(lpExpandedPath);
                }

                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Expanded local profile image filename = <%s>"), lpExpProfileImage));
            }


            //
            // Query for the internal flags
            //

            dwSize = sizeof(DWORD);
            err = RegQueryValueEx (hKey, PROFILE_STATE, NULL,
                &dwType, (LPBYTE) &dwInternalFlags, &dwSize);

            if (err != ERROR_SUCCESS) {
                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Failed to query internal flags with error %d"), err));
            }


            //
            // if we do not have a fully loaded profile, mark it as new
            // if it was not called with Liteload
            //

            if (dwInternalFlags & PROFILE_PARTLY_LOADED) {
                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  We do not have a fully loaded profile on this machine")));

                //
                // retain the partially loaded flag and remove it at the end of
                // restoreuserprofile..
                //

                lpProfile->dwInternalFlags |= PROFILE_PARTLY_LOADED;

                if (!(lpProfile->dwFlags & PI_LITELOAD)) {
                    DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Treating this profile as new")));
                    lpProfile->dwInternalFlags |= PROFILE_NEW_LOCAL;
                }
            }


            //
            //  Call FindFirst to see if we need to migrate this profile
            //

            hFile = FindFirstFile (lpExpProfileImage, &fd);

            if (hFile == INVALID_HANDLE_VALUE) {
                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Local profile image filename we got from our profile list doesn't exit.  Error = %d"), GetLastError()));
                bRetVal = FALSE;
                goto Exit;
            }

            FindClose(hFile);


            //
            // If this is a file, then we need to migrate it to
            // the new directory structure. (from a 3.5 machine)
            //

            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                lstrcpy (lpOldProfileImage, lpExpProfileImage);

                if (CreateLocalProfileImage(lpProfile, lpProfile->lpUserName)) {
                    if (UpgradeLocalProfile (lpProfile, lpOldProfileImage))
                        bRetVal = TRUE;
                    else {
                        DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage:  Failed to upgrade 3.5 profiles")));
                        bRetVal = FALSE;
                    }
                }
                else {
                    DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage:  Failed to create a new profile to upgrade")));
                    bRetVal = FALSE;
                }
                goto Exit;
            }

            //
            // Test if a mandatory profile exists
            //

            lpEnd = CheckSlash (lpExpProfileImage);
            lstrcpy (lpEnd, c_szNTUserMan);

            //
            // Impersonate the user, before trying to access ntuser, ntuser.man
            // fail, if we can not access..
            //

            if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {
                DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage: Failed to impersonate user")));
                bRetVal = FALSE;
                goto Exit;
            }

            fh = CreateFile(lpExpProfileImage, GENERIC_READ, FILE_SHARE_READ, NULL,
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (fh != INVALID_HANDLE_VALUE) {
                lpProfile->dwInternalFlags |= PROFILE_MANDATORY;
                CloseHandle(fh);

                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Found local mandatory profile image file ok <%s>"),
                    lpExpProfileImage));

                *(lpEnd - 1) = TEXT('\0');
                lstrcpy(lpProfile->lpLocalProfile, lpExpProfileImage);
                RegCloseKey(hKey);

                RevertToUser(&hOldToken);

                return TRUE;  // local copy is valid and reachable
            } else {
                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  No local mandatory profile.  Error = %d"), GetLastError()));
            }


            //
            // Test if a normal profile exists
            //

            lstrcpy (lpEnd, c_szNTUserDat);

            fh = CreateFile(lpExpProfileImage, GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            //
            // Revert to User before continuing
            //

            RevertToUser(&hOldToken);

            if (fh != INVALID_HANDLE_VALUE) {
                CloseHandle(fh);

                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Found local profile image file ok <%s>"),
                    lpExpProfileImage));

                *(lpEnd - 1) = TEXT('\0');
                lstrcpy(lpProfile->lpLocalProfile, lpExpProfileImage);


                //
                // Read the time this profile was unloaded
                //

                dwSize = sizeof(lpProfile->ftProfileUnload.dwLowDateTime);

                lResult = RegQueryValueEx (hKey,
                    PROFILE_UNLOAD_TIME_LOW,
                    NULL,
                    &dwType,
                    (LPBYTE) &lpProfile->ftProfileUnload.dwLowDateTime,
                    &dwSize);

                if (lResult == ERROR_SUCCESS) {

                    dwSize = sizeof(lpProfile->ftProfileUnload.dwHighDateTime);

                    lResult = RegQueryValueEx (hKey,
                        PROFILE_UNLOAD_TIME_HIGH,
                        NULL,
                        &dwType,
                        (LPBYTE) &lpProfile->ftProfileUnload.dwHighDateTime,
                        &dwSize);

                    if (lResult != ERROR_SUCCESS) {
                        DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Failed to query high profile unload time with error %d"), lResult));
                        lpProfile->ftProfileUnload.dwLowDateTime = 0;
                        lpProfile->ftProfileUnload.dwHighDateTime = 0;
                    }

                } else {
                    DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Failed to query low profile unload time with error %d"), lResult));
                    lpProfile->ftProfileUnload.dwLowDateTime = 0;
                    lpProfile->ftProfileUnload.dwHighDateTime = 0;
                }

                RegCloseKey(hKey);
                return TRUE;  // local copy is valid and reachable
            } else {
                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Local profile image filename we got from our profile list doesn't exit.  <%s>  Error = %d"),
                    lpExpProfileImage, GetLastError()));
            }
        }
    }

Exit:

    err = RegCloseKey(hKey);

    if (err != STATUS_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage:  Failed to close registry key, error = %d"), err));
    }

    return bRetVal;
}

//*************************************************************
//
//  CreateLocalProfileImage()
//
//  Purpose:    creates the profileimagepath
//
//  Parameters: lpProfile   -   Profile information
//              lpBaseName  -   Base Name from which profile dir name
//                              will be generated.
//
//  Return:     TRUE if the profile image is creatable
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/20/95     ericflo    Ported
//              9/26/98     ushaji     Modified
//
//*************************************************************
BOOL CreateLocalProfileImage(LPPROFILE lpProfile, LPTSTR lpBaseName)
{
    HKEY hKey;
    BOOL bKeyExists;
    TCHAR lpProfileImage[MAX_PATH];
    TCHAR lpExpProfileImage[MAX_PATH];
    DWORD cbExpProfileImage = sizeof(TCHAR)*MAX_PATH;
    DWORD err;
    DWORD dwSize;
    PSID UserSid;
    BOOL bRetVal = FALSE;

    lpProfile->lpLocalProfile[0] = TEXT('\0');

    if (!CreateLocalProfileKey(lpProfile, &hKey, &bKeyExists)) {
        return FALSE;   // not reachable and cannot keep a local copy
    }

    //
    // No local copy found, try to create a new one.
    //

    DebugMsg((DM_VERBOSE, TEXT("CreateLocalProfileImage:  One way or another we haven't got an existing local profile, try and create one")));

    dwSize = ARRAYSIZE(lpProfileImage);
    if (!GetProfilesDirectoryEx(lpProfileImage, &dwSize, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("CreateLocalProfileImage:  Failed to get profile root directory.")));
        goto Exit;
    }

    if (ComputeLocalProfileName(lpProfile, lpBaseName,
        lpProfileImage, MAX_PATH,
        lpExpProfileImage, MAX_PATH, NULL, FALSE)) {


        //
        // Add this image file to our profile list for this user
        //

        err = RegSetValueEx(hKey,
            PROFILE_IMAGE_VALUE_NAME,
            0,
            REG_EXPAND_SZ,
            (LPBYTE)lpProfileImage,
            sizeof(TCHAR)*(lstrlen(lpProfileImage) + 1));

        if (err == ERROR_SUCCESS) {

            lstrcpy(lpProfile->lpLocalProfile, lpExpProfileImage);

            //
            // Get the sid of the logged on user
            //

            UserSid = GetUserSid(lpProfile->hToken);
            if (UserSid != NULL) {

                //
                // Store the user sid under the Sid key of the local profile
                //

                err = RegSetValueEx(hKey,
                    TEXT("Sid"),
                    0,
                    REG_BINARY,
                    UserSid,
                    RtlLengthSid(UserSid));


                if (err != ERROR_SUCCESS) {
                    DebugMsg((DM_WARNING, TEXT("CreateLocalProfileImage:  Failed to set 'sid' value of user in profile list, error = %d"), err));
                    SetLastError(err);
                }

                //
                // We're finished with the user sid
                //

                DeleteUserSid(UserSid);

                bRetVal = TRUE;

            } else {
                DebugMsg((DM_WARNING, TEXT("CreateLocalProfileImage:  Failed to get sid of logged on user, so unable to update profile list")));
                SetLastError(err);
            }
        } else {
            DebugMsg((DM_WARNING, TEXT("CreateLocalProfileImage:  Failed to update profile list for user with local profile image filename, error = %d"), err));
            SetLastError(err);
        }
    }


Exit:
    err = RegCloseKey(hKey);

    if (err != STATUS_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CreateLocalProfileImage:  Failed to close registry key, error = %d"), err));
        SetLastError(err);
    }

    return bRetVal;
}

//*************************************************************
//
//  IssueDefaultProfile()
//
//  Purpose:    Issues the specified default profile to a user
//
//  Parameters: lpProfile         -   Profile Information
//              lpDefaultProfile  -   Default profile location
//              lpLocalProfile    -   Local profile location
//              lpSidString       -   User's sid
//              bMandatory        -   Issue mandatory profile
//
//  Return:     TRUE if profile was successfully setup
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/22/95     ericflo    Created
//
//*************************************************************

BOOL IssueDefaultProfile (LPPROFILE lpProfile, LPTSTR lpDefaultProfile,
                          LPTSTR lpLocalProfile, LPTSTR lpSidString,
                          BOOL bMandatory)
{
    LPTSTR lpEnd, lpTemp;
    TCHAR szProfile[MAX_PATH];
    TCHAR szTempProfile[MAX_PATH];
    BOOL bProfileLoaded = FALSE;
    WIN32_FIND_DATA fd;
    HANDLE hFile;
    LONG error;
    DWORD dwFlags;
    HANDLE hOldToken;


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  Entering.  lpDefaultProfile = <%s> lpLocalProfile = <%s>"),
             lpDefaultProfile, lpLocalProfile));


    //
    // First expand the default profile
    //

    if (!ExpandEnvironmentStrings(lpDefaultProfile, szProfile, MAX_PATH)) {
        DebugMsg((DM_WARNING, TEXT("IssueDefaultProfile: ExpandEnvironmentStrings Failed with error %d"), GetLastError()));
        return FALSE;
    }


    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("IssueDefaultProfile: Failed to impersonate user")));
        return FALSE;
    }


    //
    // Does the default profile directory exist?
    //

    hFile = FindFirstFile (szProfile, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  Default profile <%s> does not exist."), szProfile));
        RevertToUser(&hOldToken);
        return FALSE;
    }

    FindClose(hFile);


    //
    // Copy profile to user profile
    //

    dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;
    dwFlags |= CPD_CREATETITLE | CPD_IGNORESECURITY | CPD_IGNORELONGFILENAMES;

    if (lpProfile->dwFlags & PI_LITELOAD) {
        dwFlags |=  CPD_SYSTEMFILES | CPD_SYSTEMDIRSONLY;
    }
    else
        dwFlags |= CPD_IGNOREENCRYPTEDFILES;

    //
    // Call it with force copy unless there might be a partial profile locally
    //

    if (!(lpProfile->dwInternalFlags & PROFILE_PARTLY_LOADED)) {
        dwFlags |= CPD_FORCECOPY;
    }

    if (!CopyProfileDirectoryEx (szProfile, lpLocalProfile, dwFlags, NULL, NULL)) {
        DebugMsg((DM_WARNING, TEXT("IssueDefaultProfile:  CopyProfileDirectory returned FALSE.  Error = %d"), GetLastError()));
        RevertToUser(&hOldToken);
        return FALSE;
    }

    //
    // Rename the profile is a mandatory one was requested.
    //

    lstrcpy (szProfile, lpLocalProfile);
    lpEnd = CheckSlash (szProfile);

    if (bMandatory) {

        DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  Mandatory profile was requested.")));

        lstrcpy (szTempProfile, szProfile);
        lstrcpy (lpEnd, c_szNTUserMan);

        hFile = FindFirstFile (szProfile, &fd);

        if (hFile != INVALID_HANDLE_VALUE) {
            DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  Mandatory profile already exists.")));
            FindClose(hFile);

        } else {
            DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  Renaming ntuser.dat to ntuser.man")));

            lpTemp = CheckSlash(szTempProfile);
            lstrcpy (lpTemp, c_szNTUserDat);

            if (!MoveFile(szTempProfile, szProfile)) {
                DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  MoveFile returned false.  Error = %d"), GetLastError()));
            }
        }

    } else {
        lstrcpy (lpEnd, c_szNTUserDat);
    }

    //
    // Revert to being 'ourself'
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("IssueDefaultProfile: Failed to revert to self")));
    }


    //
    // Try to load the new profile
    //

    error = MyRegLoadKey(HKEY_USERS, lpSidString, szProfile);

    bProfileLoaded = (error == ERROR_SUCCESS);


    if (!bProfileLoaded) {
        DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  MyRegLoadKey failed with error %d"),
                 error));

        SetLastError(error);

        if (error == ERROR_BADDB) {
            ReportError(lpProfile->dwFlags, IDS_FAILED_LOAD_1009);
        } else if (error == ERROR_NO_SYSTEM_RESOURCES) {
            ReportError(lpProfile->dwFlags, IDS_FAILED_LOAD_1450);
        }

        return FALSE;
    }

    
    DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  Leaving successfully")));

    return TRUE;
}


//*************************************************************
//
//  DeleteProfileEx ()
//
//  Purpose:    Deletes the specified profile from the
//              registry and disk.
//
//  Parameters: lpSidString     -   Registry subkey
//              lpProfileDir    -   Profile directory
//              bBackup         -   Backup profile before deleting
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/23/95     ericflo    Created
//
//*************************************************************

BOOL DeleteProfileEx (LPCTSTR lpSidString, LPTSTR lpLocalProfile, DWORD dwDeleteFlags, HKEY hKeyLM)
{
    LONG lResult;
    TCHAR szTemp[MAX_PATH];
    TCHAR szUserGuid[MAX_PATH], szBuffer[MAX_PATH];
    TCHAR szRegBackup[MAX_PATH];
    HKEY hKey;
    DWORD dwType, dwSize, dwErr;
    BOOL bRetVal=TRUE;
    LPTSTR lpEnd = NULL;
    BOOL bBackup;

    bBackup = dwDeleteFlags & DP_BACKUP;

    dwErr = GetLastError();

    //
    // Cleanup the registry first.
    // delete the guid only if we don't have a bak to keep track of
    //

    if (lpSidString && *lpSidString) {

        if (!(dwDeleteFlags & DP_BACKUPEXISTS)) {
            
            lstrcpy(szTemp, PROFILE_LIST_PATH);
            lstrcat(szTemp, TEXT("\\"));
            lstrcat(szTemp, lpSidString);


            //
            // get the user guid
            //

            lResult = RegOpenKeyEx(hKeyLM, szTemp, 0, KEY_READ, &hKey);

            if (lResult == ERROR_SUCCESS) {

                //
                // Query for the user guid
                //

                dwSize = MAX_PATH * sizeof(TCHAR);
                lResult = RegQueryValueEx (hKey, PROFILE_GUID, NULL, &dwType, (LPBYTE) szUserGuid, &dwSize);

                if (lResult != ERROR_SUCCESS) {
                    DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Failed to query profile guid with error %d"), lResult));
                }
                else {


                    lstrcpy(szTemp, PROFILE_GUID_PATH);
                    lstrcat(szTemp, TEXT("\\"));
                    lstrcat(szTemp, szUserGuid);

                    //
                    // Delete the profile guid from the guid list
                    //

                    lResult = RegDeleteKey(hKeyLM, szTemp);

                    if (lResult != ERROR_SUCCESS) {
                        DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  failed to delete profile guid.  Error = %d"), lResult));
                    }
                }

                RegCloseKey(hKey);
            }
        }
        

        lstrcpy(szTemp, PROFILE_LIST_PATH);
        lstrcat(szTemp, TEXT("\\"));
        lstrcat(szTemp, lpSidString);

        if (bBackup) {

            lstrcpy(szRegBackup, szTemp);
            lstrcat(szRegBackup, c_szBAK);
            
            lResult = RegRenameKey(hKeyLM, szTemp, szRegBackup);

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Unable to rename registry entry.  Error = %d"), lResult));
                dwErr = lResult;
                bRetVal = FALSE;
            }
        }
        else {
            lResult = RegDeleteKey(hKeyLM, szTemp);

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Unable to delete registry entry.  Error = %d"), lResult));
                dwErr = lResult;
                bRetVal = FALSE;
            }
        }
    }


    if (bBackup) {

        //
        // Generate the backup name
        //

        lstrcpy (szTemp, lpLocalProfile);
        lstrcat (szTemp, c_szBAK);

        //
        // First delete any previous backup
        //

        Delnode (szTemp);

        //
        // Attempt to rename the directory
        //

        if (!MoveFileEx(lpLocalProfile, szTemp, 0)) {

            DebugMsg((DM_VERBOSE, TEXT("DeleteProfileEx:  Failed to rename the directory.  Error = %d"), GetLastError()));
            dwErr = GetLastError();
            bRetVal = FALSE;
        }

        lResult = RegOpenKeyEx(hKeyLM, szRegBackup, 0, KEY_ALL_ACCESS, &hKey);

        if (lResult == ERROR_SUCCESS) {
            DWORD dwInternalFlags;
            TCHAR szProfilePath[MAX_PATH];
            
            dwSize = sizeof(DWORD);
            lResult = RegQueryValueEx (hKey, PROFILE_STATE, NULL, &dwType, (LPBYTE)&dwInternalFlags, &dwSize);

            if (lResult == ERROR_SUCCESS) {

                dwInternalFlags |= PROFILE_THIS_IS_BAK;
                lResult = RegSetValueEx (hKey, PROFILE_STATE, 0, REG_DWORD,
                                 (LPBYTE) &dwInternalFlags, sizeof(dwInternalFlags));
            }
            else {
                DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Failed to query profile internalflags  with error %d"), lResult));
            }


            dwSize = sizeof(TCHAR)*MAX_PATH;            
            lResult = RegQueryValueEx (hKey, PROFILE_IMAGE_VALUE_NAME, NULL, &dwType, (LPBYTE)&szProfilePath, &dwSize);

            if (lResult == ERROR_SUCCESS) {

                lstrcat(szProfilePath, c_szBAK);
                lResult = RegSetValueEx (hKey, PROFILE_IMAGE_VALUE_NAME, 0, dwType,
                                 (LPBYTE) szProfilePath, sizeof(TCHAR)*(lstrlen(szProfilePath)+1));

            }
            else {
                DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Failed to query imagefile pathname  with error %d"), lResult));
            }
            
            RegCloseKey(hKey);
        }


    } else {

        if (!Delnode (lpLocalProfile)) {
            DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Delnode failed.  Error = %d"), GetLastError()));
            dwErr = GetLastError();
            bRetVal = FALSE;
        }
    }

    if (dwDeleteFlags & DP_DELBACKUP) {
        goto Exit;
        // don't delete any more stuff because the user actually might be logged in.
    }

    //
    // Delete appmgmt specific stuff..
    //

    if (!ExpandEnvironmentStrings(APPMGMT_DIR_ROOT, szBuffer, MAX_PATH)) {
        DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to expand %s, error %d"), APPMGMT_DIR_ROOT, GetLastError()));
        goto Exit;
    }
    
    //
    // Delete the appmgmt directory
    //

    lpEnd = CheckSlash(szBuffer);
    lstrcpy(lpEnd, lpSidString);

    if (!Delnode(szBuffer)) {
        DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to delete the appmgmt dir %s, error %d"), szBuffer, GetLastError()));
    }

    //
    // Delete the registry values
    //

    lstrcpy(szBuffer, APPMGMT_REG_MANAGED);
    lpEnd = CheckSlash(szBuffer);
    lstrcpy(lpEnd, lpSidString);

    if (!RegDelnode (hKeyLM, szBuffer)) {
        DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to delete the appmgmt key %s"), szBuffer));
    }


    //
    // Delete the Group Policy per user stuff..
    //

    lstrcpy(szBuffer, GP_XXX_SID_PREFIX);
    lpEnd = CheckSlash(szBuffer);
    lstrcpy(lpEnd, lpSidString);

    if (!RegDelnode (hKeyLM, szBuffer)) {
        DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to delete the group policy key %s"), szBuffer));
    }


    lstrcpy(szBuffer, GP_EXTENSIONS_SID_PREFIX);
    lpEnd = CheckSlash(szBuffer);
    lstrcpy(lpEnd, lpSidString);

    if (!RegDelnode (hKeyLM, szBuffer)) {
        DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to delete the group policy extensions key %s"), szBuffer));
    }

Exit:
    SetLastError(dwErr);
    return bRetVal;
}


//*************************************************************
//
//  UpgradeProfile()
//
//  Purpose:    Called after a profile is successfully loaded.
//              Stamps build number into the profile, and if
//              appropriate upgrades the per-user settings
//              that NT setup wants done.
//
//  Parameters: lpProfile   -   Profile Information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/7/95     ericflo    Created
//
//*************************************************************

BOOL UpgradeProfile (LPPROFILE lpProfile)
{
    HKEY hKey;
    DWORD dwDisp, dwType, dwSize, dwBuildNumber;
    LONG lResult;
    BOOL bUpgrade = FALSE;
    BOOL bDoUserdiff = TRUE;


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("UpgradeProfile: Entering")));


    //
    // Query for the build number
    //

    lResult = RegCreateKeyEx (lpProfile->hKeyCurrentUser, WINLOGON_KEY,
                              0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("UpgradeProfile: Failed to open winlogon key. Error = %d"), lResult));
        return FALSE;
    }


    dwSize = sizeof(dwBuildNumber);
    lResult = RegQueryValueEx (hKey, PROFILE_BUILD_NUMBER,
                               NULL, &dwType, (LPBYTE)&dwBuildNumber,
                               &dwSize);

    if (lResult == ERROR_SUCCESS) {

        //
        // Found the build number.  If the profile build is greater,
        // we don't want to process the userdiff hive
        //

        if (dwBuildNumber >= g_dwBuildNumber) {
            DebugMsg((DM_VERBOSE, TEXT("UpgradeProfile: Build numbers match")));
            bDoUserdiff = FALSE;
        }
    } else {

        dwBuildNumber = 0;
    }


    if (bDoUserdiff) {

        //
        // Set the build number
        //

        lResult = RegSetValueEx (hKey, PROFILE_BUILD_NUMBER, 0, REG_DWORD,
                                 (LPBYTE) &g_dwBuildNumber, sizeof(g_dwBuildNumber));

        if (lResult != ERROR_SUCCESS) {
           DebugMsg((DM_WARNING, TEXT("UpgradeProfile: Failed to set build number. Error = %d"), lResult));
        }
    }


    //
    // Close the registry key
    //

    RegCloseKey (hKey);



    if (bDoUserdiff) {

        //
        // Apply changes to user's hive that NT setup needs.
        //

        if (!ProcessUserDiff(lpProfile, dwBuildNumber)) {
            DebugMsg((DM_WARNING, TEXT("UpgradeProfile: ProcessUserDiff failed")));
        }
    }

    DebugMsg((DM_VERBOSE, TEXT("UpgradeProfile: Leaving Successfully")));

    return TRUE;

}

//*************************************************************
//
//  SetProfileTime()
//
//  Purpose:    Sets the timestamp on the remote profile and
//              local profile to be the same regardless of the
//              file system type being used.
//
//  Parameters: lpProfile   -   Profile Information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              9/25/95     ericflo    Ported
//
//*************************************************************

BOOL SetProfileTime(LPPROFILE lpProfile)
{
    HANDLE hFileCentral;
    HANDLE hFileLocal;
    FILETIME ft;
    TCHAR szProfile[MAX_PATH];
    LPTSTR lpEnd;
    HANDLE hOldToken;


    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("SetProfileTime: Failed to impersonate user")));
        return FALSE;
    }


    //
    // Create the central filename
    //

    lstrcpy (szProfile, lpProfile->lpRoamingProfile);
    lpEnd = CheckSlash (szProfile);

    if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {
        lstrcpy (lpEnd, c_szNTUserMan);
    } else {
        lstrcpy (lpEnd, c_szNTUserDat);
    }


    hFileCentral = CreateFile(szProfile,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFileCentral == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_WARNING, TEXT("SetProfileTime:  couldn't open central profile <%s>, error = %d"),
                 szProfile, GetLastError()));
        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("SetProfileTime: Failed to revert to self")));
        }
        return FALSE;

    } else {

        if (!GetFileTime(hFileCentral, NULL, NULL, &ft)) {
            DebugMsg((DM_WARNING, TEXT("SetProfileTime:  couldn't get time of central profile, error = %d"), GetLastError()));
        }
    }

    //
    // Revert to being 'ourself'
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("SetProfileTime: Failed to revert to self")));
    }


    //
    // Create the local filename
    //

    lstrcpy (szProfile, lpProfile->lpLocalProfile);
    lpEnd = CheckSlash (szProfile);

    if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {
        lstrcpy (lpEnd, c_szNTUserMan);
    } else {
        lstrcpy (lpEnd, c_szNTUserDat);
    }


    hFileLocal = CreateFile(szProfile,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFileLocal == INVALID_HANDLE_VALUE) {

        DebugMsg((DM_WARNING, TEXT("SetProfileTime:  couldn't open local profile <%s>, error = %d"),
                 szProfile, GetLastError()));

    } else {

        if (!SetFileTime(hFileLocal, NULL, NULL, &ft)) {
            DebugMsg((DM_WARNING, TEXT("SetProfileTime: couldn't set time on local profile, error = %d"), GetLastError()));
        }
        if (!GetFileTime(hFileLocal, NULL, NULL, &ft)) {
            DebugMsg((DM_WARNING, TEXT("SetProfileTime:  couldn't get time on local profile, error = %d"), GetLastError()));
        }
        CloseHandle(hFileLocal);
    }

    //
    // Reset time of central profile in case of discrepencies in
    // times of different file systems.
    //

    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("SetProfileTime: Failed to impersonate user")));
        CloseHandle(hFileCentral);
        return FALSE;
    }


    //
    // Set the time on the central profile
    //

    if (!SetFileTime(hFileCentral, NULL, NULL, &ft)) {
         DebugMsg((DM_WARNING, TEXT("SetProfileTime:  couldn't set time on local profile, error = %d"), GetLastError()));
    }

    CloseHandle(hFileCentral);


    //
    // Revert to being 'ourself'
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("SetProfileTime: Failed to revert to self")));
    }

    return TRUE;
}

//*************************************************************
//
//  IsCacheDeleted()
//
//  Purpose:    Determines if the locally cached copy of the
//              roaming profile should be deleted.
//
//  Parameters: void
//
//  Return:     TRUE if local cache should be deleted
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/28/96     ericflo    Created
//
//*************************************************************

BOOL IsCacheDeleted (void)
{
    BOOL bRetVal = FALSE;
    DWORD dwSize, dwType;
    HKEY hKey;

    //
    // Open the winlogon registry key
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      WINLOGON_KEY,
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS) {

        //
        // Check for the flag.
        //

        dwSize = sizeof(BOOL);
        RegQueryValueEx (hKey,
                         DELETE_ROAMING_CACHE,
                         NULL,
                         &dwType,
                         (LPBYTE) &bRetVal,
                         &dwSize);

        RegCloseKey (hKey);
    }


    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      SYSTEM_POLICIES_KEY,
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS) {

        //
        // Check for the flag.
        //

        dwSize = sizeof(BOOL);
        RegQueryValueEx (hKey,
                         DELETE_ROAMING_CACHE,
                         NULL,
                         &dwType,
                         (LPBYTE) &bRetVal,
                         &dwSize);

        RegCloseKey (hKey);
    }

    return bRetVal;
}

//*************************************************************
//
//  GetProfileType()
//
//  Purpose:    Finds out some characterstics of a loaded profile
//
//  Parameters: dwFlags   -   Returns the various profile flags
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments: should be called after impersonation.
//
//  History:    Date        Author     Comment
//              11/10/98    ushaji     Created
//
//*************************************************************

BOOL WINAPI GetProfileType(DWORD *dwFlags)
{
    LPTSTR SidString;
    DWORD error, dwErr;
    HKEY hSubKey;
    BOOL bRetVal = FALSE;
    LPPROFILE lpProfile = NULL;
    HANDLE hToken;

    if (!dwFlags) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *dwFlags = 0;

    dwErr = GetLastError();

    //
    // Get the token for the caller
    //

    if (!OpenThreadToken (GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken)) {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            DebugMsg((DM_WARNING, TEXT("GetProfileType: Failed to get token with %d"), GetLastError()));
            return FALSE;
        }
    }

    //
    // Get the Sid string for the user
    //

    SidString = GetProfileSidString(hToken);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("GetProfileType:  Failed to get sid string for user")));
        dwErr = GetLastError();
        goto Exit;
    }

    error = RegOpenKeyEx(HKEY_USERS, SidString, 0, KEY_READ, &hSubKey);

    if (error == ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("GetProfileType:  Profile already loaded.")));
        RegCloseKey(hSubKey);
    }
    else {
        DebugMsg((DM_WARNING, TEXT("GetProfileType:  Profile is not loaded.")));
        dwErr = error;
        goto Exit;
    }

    lpProfile = LoadProfileInfo(hToken, NULL);

    if (!lpProfile) {
        DebugMsg((DM_WARNING, TEXT("GetProfileType:  Couldn't load Profile Information.")));
        dwErr = GetLastError();
        *dwFlags = 0;
        goto Exit;
    }

    if (lpProfile->dwInternalFlags & PROFILE_GUEST_USER)
        *dwFlags |= PT_TEMPORARY;

    if (lpProfile->dwInternalFlags & PROFILE_MANDATORY)
        *dwFlags |= PT_MANDATORY;

    if (((lpProfile->dwUserPreference != USERINFO_LOCAL)) && 
        ((lpProfile->dwInternalFlags & PROFILE_UPDATE_CENTRAL) ||
        (lpProfile->dwInternalFlags & PROFILE_NEW_CENTRAL))) {

        *dwFlags |= PT_ROAMING;

        if (IsCacheDeleted()) {
            DebugMsg((DM_VERBOSE, TEXT("GetProfileType:  Profile is to be deleted")));
            *dwFlags |= PT_TEMPORARY;
        }
    }


    if (lpProfile->dwInternalFlags & PROFILE_TEMP_ASSIGNED)
        *dwFlags |= PT_TEMPORARY;

    bRetVal = TRUE;

Exit:
    if (SidString)
        DeleteSidString(SidString);

    SetLastError(dwErr);

    if (lpProfile) {

        if (lpProfile->lpLocalProfile) {
            LocalFree (lpProfile->lpLocalProfile);
        }

        if (lpProfile->lpRoamingProfile) {
            LocalFree (lpProfile->lpRoamingProfile);
        }

        LocalFree (lpProfile);
    }

    CloseHandle (hToken);

    if (bRetVal) {
        DebugMsg((DM_VERBOSE, TEXT("GetProfileType: ProfileFlags is %d"), *dwFlags));
    }

    return bRetVal;
}


//*************************************************************
//
//  DumpOpenRegistryHandle()
//
//  Purpose:    Dumps the existing reg handle into the debugger
//
//  Parameters: lpKeyName -   The key name to the key in the form of
//                            \registry\user....
//
//
//  Return:     Nothing
//
//  Comments:
//
//  History:    Date        Author     Comment
//*************************************************************

void DumpOpenRegistryHandle(LPTSTR lpkeyName)
{

    UNICODE_STRING      UnicodeKeyName;
    OBJECT_ATTRIBUTES   keyAttributes;
    ULONG               HandleCount = 0;
    BOOL                bBreakOnUnloadFailure=FALSE;
    HKEY                hKey;
    DWORD               dwSize, dwType;


    //
    // Initialize unicode string for our in params
    //

    RtlInitUnicodeString(&UnicodeKeyName, lpkeyName);
    
    //
    // Initialize the Obja structure
    //

    InitializeObjectAttributes(
            &keyAttributes,
            &UnicodeKeyName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );


    NtQueryOpenSubKeys(&keyAttributes, &HandleCount);

    DebugMsg((DM_WARNING, TEXT("DumpOpenRegistryHandle: %d user registry Handles leaked from %s"), HandleCount, lpkeyName));

    //
    // for debugging sometimes it is necessary to break at the point of
    // failure
    //


    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      WINLOGON_KEY,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bBreakOnUnloadFailure);
        RegQueryValueEx (hKey,
                         TEXT("BreakOnHiveUnloadFailure"),
                         NULL,
                         &dwType,
                         (LPBYTE) &bBreakOnUnloadFailure,
                         &dwSize);

        RegCloseKey (hKey);
    }


    if (bBreakOnUnloadFailure)
        DebugBreak();

}


//*************************************************************
//
//  UnloadUserProfile()
//
//  Purpose:    Unloads the user's profile.
//
//  Parameters: hToken    -   User's token
//              hProfile  -   Profile handle created in LoadUserProfile
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/7/95      ericflo    Created
//
//*************************************************************

BOOL WINAPI UnloadUserProfile (HANDLE hToken, HANDLE hProfile)
{
    LPPROFILE lpProfile=NULL;
    LPTSTR lpSidString = NULL, lpEnd, SidStringTemp, lpUserMutexName;
    LONG err, IgnoreError, lResult;
    BOOL bProfileCopied = FALSE, bRetVal = FALSE, bDeleteCache, bRoaming = FALSE;
    HANDLE hMutex = NULL;
    HKEY hKey;
    DWORD dwResult, dwSize, dwType, dwDisp;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    TCHAR szExcludeList1[MAX_PATH];
    TCHAR szExcludeList2[MAX_PATH];
    TCHAR szExcludeList[2 * MAX_PATH];
    TCHAR szBuffer[MAX_PATH];
    DWORD dwFlags, dwRef = 0;
    HANDLE hOldToken;
    DWORD dwErr=0, dwErr1;
    TCHAR szErr[MAX_PATH];
    TCHAR szKeyName[MAX_PATH];


    dwErr1 = GetLastError();

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Entering, hProfile = <0x%x>"),
             hProfile));


    //
    // Check Parameters
    //

    if (!hProfile) {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: received a NULL hProfile.")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    //
    // Make sure we can impersonate the user
    //

    if (!ImpersonateUser(hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to impersonate user with %d."), GetLastError()));
        dwErr1 = GetLastError();
        goto Exit;
    }

    RevertToUser(&hOldToken);


    //
    // Load profile information
    //

    lpProfile = LoadProfileInfo(hToken, hProfile);

    if (!lpProfile) {
        dwErr1 = GetLastError();
        RegCloseKey((HKEY)hProfile);
        goto Exit;
    }


    //
    // Get the Sid string for the current user
    //

    lpSidString = GetProfileSidString(lpProfile->hToken);

    if (!lpSidString) {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: Failed to get sid string for user")));
        dwErr1 = GetLastError();
        RegCloseKey(lpProfile->hKeyCurrentUser);
        goto Exit;
    }


    //
    // Get the user's sid in string form
    //

    SidStringTemp = GetSidString(hToken);

    if (!SidStringTemp) {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile:  Failed to get sid string for user")));
        dwErr1 = GetLastError();
        RegCloseKey(lpProfile->hKeyCurrentUser);
        goto Exit;
    }

    //
    // Allocate memory for the mutex name
    //

    lpUserMutexName = LocalAlloc (LPTR, (lstrlen(SidStringTemp) + lstrlen(USER_PROFILE_MUTEX) + 1) * sizeof(TCHAR));


    if (!lpUserMutexName) {
        dwErr1 = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile:  Failed to allocate memory for user mutex name with %d"),
            dwErr1));
        RegCloseKey(lpProfile->hKeyCurrentUser);
        goto Exit;
    }

    lstrcpy (lpUserMutexName, USER_PROFILE_MUTEX);
    lstrcat (lpUserMutexName, SidStringTemp);

    //
    // Create a mutex to prevent multiple threads/process from trying to
    // unload the profile at the same time.
    //

    InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );

    SetSecurityDescriptorDacl (
                                &sd,
                                TRUE,                           // Dacl present
                                NULL,                           // NULL Dacl
                                FALSE                           // Not defaulted
                                );

    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;
    sa.nLength = sizeof(sa);

    hMutex = CreateMutex (&sa, FALSE, lpUserMutexName);

    DeleteSidString (SidStringTemp);
    LocalFree (lpUserMutexName);

    if (!hMutex) {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: Failed to create mutex.  Error = %d."),
                              GetLastError()));
        dwErr = GetLastError();
        RegCloseKey(lpProfile->hKeyCurrentUser);
        goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Created mutex.  Waiting...")));

    if ((WaitForSingleObject (hMutex, INFINITE) == WAIT_FAILED)) {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: Failed to wait on the mutex.  Error = %d."),
                               GetLastError()));
        dwErr = GetLastError();
        RegCloseKey(lpProfile->hKeyCurrentUser);
        goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Wait succeeded.  Mutex currently held.")));

    //
    // Check for a list of directories to exclude both user preferences
    // and user policy
    //

    szExcludeList1[0] = TEXT('\0');
    if (RegOpenKeyEx (lpProfile->hKeyCurrentUser,
                      WINLOGON_KEY,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(szExcludeList1);
        RegQueryValueEx (hKey,
                         TEXT("ExcludeProfileDirs"),
                         NULL,
                         &dwType,
                         (LPBYTE) szExcludeList1,
                         &dwSize);

        RegCloseKey (hKey);
    }

    szExcludeList2[0] = TEXT('\0');
    if (RegOpenKeyEx (lpProfile->hKeyCurrentUser,
                      SYSTEM_POLICIES_KEY,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(szExcludeList2);
        RegQueryValueEx (hKey,
                         TEXT("ExcludeProfileDirs"),
                         NULL,
                         &dwType,
                         (LPBYTE) szExcludeList2,
                         &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Merge the user preferences and policy together
    //

    szExcludeList[0] = TEXT('\0');

    if (szExcludeList1[0] != TEXT('\0')) {
        CheckSemicolon(szExcludeList1);
        lstrcpy (szExcludeList, szExcludeList1);
    }

    if (szExcludeList2[0] != TEXT('\0')) {
        lstrcat (szExcludeList, szExcludeList2);
    }

    //
    // Check if the cached copy of the profile should be deleted
    //

    bDeleteCache = IsCacheDeleted();


    //
    // Flush out the profile which will also sync the log.
    //

    err = RegFlushKey(lpProfile->hKeyCurrentUser);
    if (err != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile:  Failed to flush the current user key, error = %d"), err));
    }

    //
    // Close the current user key that was opened in LoadUserProfile.
    //

    err = RegCloseKey(lpProfile->hKeyCurrentUser);
    if (err != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile:  Failed to close the current user key, error = %d"), err));
    }


    dwRef = DecrementProfileRefCount(lpProfile);

    if (dwRef != 0) {
        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile:  Didn't unload user profile, Ref Count is %d"), dwRef));
        bRetVal = TRUE;
        goto Exit;
    }

    
    //
    //  Unload the user profile
    //

    err = MyRegUnLoadKey(HKEY_USERS, lpSidString);

    if (!err) {
        dwErr1 = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile:  Didn't unload user profile <err = %d>"), dwErr1));

        if (!(lpProfile->dwFlags & PI_LITELOAD)) {

            //
            // Call Special Registry APIs to dump handles
            //
            //
            // only if it is not called through Lite_load
            // there are known problems with liteLoad loading because
            // of which eventlog can get full during stress
            //

            lstrcpy(szKeyName, TEXT("\\Registry\\User\\"));
            lstrcat(szKeyName, lpSidString);
            DumpOpenRegistryHandle(szKeyName);

            ReportError(PI_NOUI, IDS_FAILED_HIVE_UNLOAD, GetErrString(dwErr1, szErr), g_dwBuildNumber);
        }

        
        goto Exit;
        

    } else {
        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile:  Succesfully unloaded profile")));
    }


    //
    //  Unload HKCU
    //

    if (!(lpProfile->dwFlags & PI_LITELOAD)) {
        err = UnloadClasses(lpSidString);

        if (!err) {
            DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile:  Didn't unload user classes.")));

            //
            // Call Special Registry APIs to dump handles
            //
            
            lstrcpy(szKeyName, TEXT("\\Registry\\User\\"));
            lstrcat(szKeyName, lpSidString);
            lstrcat(szKeyName, TEXT("_Classes"));

            DumpOpenRegistryHandle(szKeyName);

            ReportError(PI_NOUI, IDS_FAILED_HIVE_UNLOAD1, GetErrString(GetLastError(), szErr), g_dwBuildNumber);

            dwErr = GetLastError();
            bRetVal = TRUE;
            goto Exit;

        } else {
            DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile:  Successfully unloaded user classes")));
        }
    }


    //
    // If this is a mandatory or a guest profile, unload it now.
    // Guest profiles are always deleted so one guest can't see
    // the profile of a previous guest.
    //

    if ((lpProfile->dwInternalFlags & PROFILE_MANDATORY) ||
        (lpProfile->dwInternalFlags & PROFILE_GUEST_USER)) {

        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile:  flushing HKEY_USERS")));
        
        IgnoreError = RegFlushKey(HKEY_USERS);
        if (IgnoreError != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfile:  Failed to flush HKEY_USERS, error = %d"), IgnoreError));
        }

        
        if (bDeleteCache || (lpProfile->dwInternalFlags & PROFILE_GUEST_USER)) {

            //
            // Delete the profile, including all other user related stuff
            //

            DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: deleting profile because it is a guest user or cache needs to be deleted")));

            if (!DeleteProfile (lpSidString, NULL, NULL)) {
                DebugMsg((DM_WARNING, TEXT("UnloadUserProfile:  DeleteProfileDirectory returned false.  Error = %d"), GetLastError()));
            }
            DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Successfully deleted profile because it is a guest/mandatory user")));
        }

        if (err) {
            bRetVal = TRUE;
        }


        if (lpProfile->dwInternalFlags & PROFILE_TEMP_ASSIGNED) {

            //
            // Just delete the user profile, backup should never exist for mandatory profile
            //

            if (!DeleteProfileEx (lpSidString, lpProfile->lpLocalProfile, 0, HKEY_LOCAL_MACHINE)) {
                DebugMsg((DM_WARNING, TEXT("UnloadUserProfile:  DeleteProfileDirectory returned false (2).  Error = %d"), GetLastError()));
            }
        }

        goto Exit;
    }

    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hToken, &hOldToken)) {
        dwErr1 = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: Failed to impersonate user")));
        goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Impersonated user")));
    //
    // Copy local profileimage to remote profilepath
    //

    if ( ((lpProfile->dwInternalFlags & PROFILE_UPDATE_CENTRAL) ||
          (lpProfile->dwInternalFlags & PROFILE_NEW_CENTRAL)) ) {

        if ((lpProfile->dwUserPreference != USERINFO_LOCAL) &&
            !(lpProfile->dwInternalFlags & PROFILE_SLOW_LINK)) {

            //
            // Copy to the profile path
            //

            if (*lpProfile->lpRoamingProfile) {
                DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile:  Copying profile back to %s"),
                                lpProfile->lpRoamingProfile));

                bRoaming = TRUE;

                //
                // Copy the profile
                //

                dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;
                dwFlags |= (lpProfile->dwFlags & PI_LITELOAD) ? (CPD_SYSTEMFILES | CPD_SYSTEMDIRSONLY) :
                                                                (CPD_SYNCHRONIZE | CPD_NONENCRYPTEDONLY);

                dwFlags |= CPD_USEDELREFTIME |
                           CPD_USEEXCLUSIONLIST | CPD_DELDESTEXCLUSIONS;

                bProfileCopied = CopyProfileDirectoryEx (lpProfile->lpLocalProfile,
                                               lpProfile->lpRoamingProfile,
                                               dwFlags,
                                               &lpProfile->ftProfileLoad,
                                               (szExcludeList[0] != TEXT('\0')) ?
                                               szExcludeList : NULL);

                //
                // Save the exclusion list we used for the profile copy
                //

                if (bProfileCopied) {
                    // save it on the roaming profile.
                    lstrcpy (szBuffer, lpProfile->lpRoamingProfile);
                    lpEnd = CheckSlash (szBuffer);
                    lstrcpy (lpEnd, c_szNTUserIni);

                    bProfileCopied = WritePrivateProfileString (PROFILE_GENERAL_SECTION,
                                               PROFILE_EXCLUSION_LIST,
                                               (szExcludeList[0] != TEXT('\0')) ?
                                               szExcludeList : NULL,
                                               szBuffer);

                    if (!bProfileCopied) {
                        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: Failed to write to ntuser.ini on profile server with error 0x%x"), GetLastError()));
                        dwErr = GetLastError();
                    }
                    else {
                        SetFileAttributes (szBuffer, FILE_ATTRIBUTE_HIDDEN);
                    }
                }
                else {
                    dwErr = GetLastError();

                    if (dwErr == ERROR_FILE_ENCRYPTED) {
                        ReportError(lpProfile->dwFlags, IDS_PROFILEUPDATE_6002);
                    }
                }

                //
                // Check return value
                //

                if (bProfileCopied) {

                    //
                    // The profile is copied, now we want to make sure the timestamp on
                    // both the remote profile and the local copy are the same, so we don't
                    // ask the user to update when it's not necessary.
                    //

                    SetProfileTime(lpProfile);

                } else {
                    DebugMsg((DM_WARNING, TEXT("UnloadUserProfile:  CopyProfileDirectory returned FALSE for primary profile.  Error = %d"), dwErr));
                    ReportError(lpProfile->dwFlags, IDS_CENTRAL_UPDATE_FAILED, GetErrString(dwErr, szErr));
                }
            }
        }
    }

    //
    // if it is roaming, write only if copy succeeded otherwise write
    //

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Writing local ini file")));
    if (!bRoaming || bProfileCopied) {

        //
        // Mark the file with system bit before trying to write to it
        //

        SetNtUserIniAttributes(lpProfile->lpLocalProfile);

        // save it locally
        
        lstrcpy (szBuffer, (lpProfile->lpLocalProfile));
        lpEnd = CheckSlash (szBuffer);
        lstrcpy (lpEnd, c_szNTUserIni);

        err = WritePrivateProfileString (PROFILE_GENERAL_SECTION,
                                        PROFILE_EXCLUSION_LIST,
                                        (szExcludeList[0] != TEXT('\0')) ?
                                        szExcludeList : NULL,
                                        szBuffer);

        if (!err) {
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: Failed to write to ntuser.ini on client with error 0x%x"), GetLastError()));
            dwErr = GetLastError();
        }
    }


    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: Failed to revert to self")));
    }

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Reverting to Self")));

    //
    // Save the profile unload time
    //

    if (bProfileCopied && !bDeleteCache && !(lpProfile->dwFlags & PI_LITELOAD) &&
        !(lpProfile->dwInternalFlags & PROFILE_TEMP_ASSIGNED)) {

        GetSystemTimeAsFileTime (&lpProfile->ftProfileUnload);

        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Got the System Time")));
        
        lstrcpy(szBuffer, PROFILE_LIST_PATH);
        lpEnd = CheckSlash (szBuffer);
        lstrcpy(lpEnd, lpSidString);

        lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, 0, 0,
                                 KEY_WRITE, NULL, &hKey, &dwDisp);

        if (lResult == ERROR_SUCCESS) {

            lResult = RegSetValueEx (hKey,
                                     PROFILE_UNLOAD_TIME_LOW,
                                     0,
                                     REG_DWORD,
                                     (LPBYTE) &lpProfile->ftProfileUnload.dwLowDateTime,
                                     sizeof(DWORD));

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_WARNING, TEXT("UnloadUserProfile:  Failed to save low profile load time with error %d"), lResult));
            }


            lResult = RegSetValueEx (hKey,
                                     PROFILE_UNLOAD_TIME_HIGH,
                                     0,
                                     REG_DWORD,
                                     (LPBYTE) &lpProfile->ftProfileUnload.dwHighDateTime,
                                     sizeof(DWORD));

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_WARNING, TEXT("UnloadUserProfile:  Failed to save high profile load time with error %d"), lResult));
            }


            RegCloseKey (hKey);

            DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Setting the unload Time")));
        }
    }


    if (lpProfile->dwInternalFlags & PROFILE_TEMP_ASSIGNED) {
        DWORD dwDeleteFlags=0;
        
        //
        // Just delete the user profile
        //

        if (lpProfile->dwInternalFlags & PROFILE_BACKUP_EXISTS) {
            dwDeleteFlags |= DP_BACKUPEXISTS;
        }
        
        if (!DeleteProfileEx (lpSidString, lpProfile->lpLocalProfile, dwDeleteFlags, HKEY_LOCAL_MACHINE)) {
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfile:  DeleteProfileDirectory returned false (2).  Error = %d"), GetLastError()));
        }
    }


    if (bRoaming && bProfileCopied && bDeleteCache) {

        //
        // Delete the profile and all the related stuff
        //

        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Deleting the cached profile")));
        if (!DeleteProfile (lpSidString, NULL, NULL)) {
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfile:  DeleteProfileDirectory returned false (2).  Error = %d"), GetLastError()));
        }
    }

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: exitting and cleaning up")));

    //
    // Success
    //

    bRetVal = TRUE;


Exit:

    if (hMutex) {

        //
        // This will release the mutex so other threads/process can continue.
        //

        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Releasing mutex.")));
        ReleaseMutex (hMutex);
        CloseHandle (hMutex);
    }


    if (lpSidString) {
        DeleteSidString(lpSidString);
    }

    if (lpProfile) {

        if (lpProfile->lpLocalProfile) {
            LocalFree (lpProfile->lpLocalProfile);
        }

        if (lpProfile->lpRoamingProfile) {
            LocalFree (lpProfile->lpRoamingProfile);
        }

        LocalFree (lpProfile);
    }


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Leaving with a return value of %d"), bRetVal));

    SetLastError(dwErr1);

    return bRetVal;
}


//*************************************************************
//
//  ExtractProfileFromBackup()
//
//  Purpose:  Extracts the profile from backup if required.
//
//  Parameters: hToken          -   User Token
//              SidString       -   
//              dwBackupFlags   -   Backup Flags.
//                                  indicating that profile already exists.
//                                  Profile created from backup
//                                  0 indicates no such profile exists
//
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              9/21/99     ushaji     Created
//
//*************************************************************

#define EX_ALREADY_EXISTS   1
#define EX_PROFILE_CREATED  2

BOOL ExtractProfileFromBackup(HANDLE hToken, LPTSTR SidString, DWORD *dwBackupFlags)
{
    TCHAR LocalKey[MAX_PATH], *lpEnd, szLocalProfile;
    TCHAR LocalBackupKey[MAX_PATH];
    HKEY  hKey=NULL;
    DWORD dwType, dwSize;
    DWORD lResult;
    LPTSTR lpExpandedPath;
    DWORD cbExpProfileImage = sizeof(TCHAR)*MAX_PATH;
    TCHAR lpExpProfileImage[MAX_PATH];
    TCHAR lpBakProfileImage[MAX_PATH];
    BOOL  bRetVal = TRUE;
    DWORD dwInternalFlags;
    DWORD cb;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    

    *dwBackupFlags = 0;
    
    lstrcpy(LocalKey, PROFILE_LIST_PATH);
    lpEnd = CheckSlash (LocalKey);
    lstrcpy(lpEnd, SidString);

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, LocalKey, 0, KEY_ALL_ACCESS, &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(DWORD);
        lResult = RegQueryValueEx (hKey, PROFILE_STATE, NULL,
                               &dwType, (LPBYTE) &dwInternalFlags, &dwSize);

        if (lResult == ERROR_SUCCESS) {

            //
            // if there is a sid key, check whether this is a temp profile
            //

            if (dwInternalFlags & PROFILE_TEMP_ASSIGNED) {
                DWORD dwDeleteFlags = 0;

                if (dwInternalFlags & PROFILE_BACKUP_EXISTS) {
                    dwDeleteFlags |= DP_BACKUPEXISTS;
                }


                //
                // We need the path to pass to DeleteProfile
                //        

                lResult = RegQueryValueEx(hKey, PROFILE_IMAGE_VALUE_NAME, 0, &dwType,
                                        (LPBYTE)lpExpProfileImage, &cbExpProfileImage);

                if (lResult == ERROR_SUCCESS && cbExpProfileImage) {
                    DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Local profile image filename = <%s>"), lpExpProfileImage));

                    if (dwType == REG_EXPAND_SZ) {

                        //
                        // Expand the profile image filename
                        //

                        cb = sizeof(lpExpProfileImage);
                        lpExpandedPath = LocalAlloc(LPTR, cb);
                        if (lpExpandedPath) {
                            ExpandEnvironmentStrings(lpExpProfileImage, lpExpandedPath, cb);
                            lstrcpy(lpExpProfileImage, lpExpandedPath);
                            LocalFree(lpExpandedPath);
                        }

                        DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Expanded local profile image filename = <%s>"), lpExpProfileImage));
                    }

                    if (!DeleteProfileEx (SidString, lpExpProfileImage, dwDeleteFlags, HKEY_LOCAL_MACHINE)) {
                        DebugMsg((DM_WARNING, TEXT("ExtractProfileFromBackup:  DeleteProfileDirectory returned false (2).  Error = %d"), GetLastError()));
                        lResult = GetLastError();
                        goto Exit;
                    }
                    else {
                        if (!(dwInternalFlags & PROFILE_BACKUP_EXISTS)) {
                            DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Temprorary profile but there is no backup")));
                            bRetVal = TRUE;
                            goto Exit;
                        }
                    }
                }
                else {
                    DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Couldn't get the local profile path")));
                    bRetVal = FALSE;
                    goto Exit;
                }
            }                
            else {
                *dwBackupFlags |= EX_ALREADY_EXISTS;        
                DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  A profile already exists")));
                goto Exit;
            }
        }
        else {
            DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Failed to query internal flags with error %d"), lResult));
            bRetVal = FALSE;
            goto Exit;
        }    

        RegCloseKey(hKey);
        hKey = NULL;
    }
    else {
       DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Failed to open key %s with error %d"), LocalKey, lResult));
    }


    //
    // Now try to get the profile from the backup
    //

    lstrcpy(LocalBackupKey, LocalKey);
    lstrcat(LocalBackupKey, c_szBAK);


    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, LocalBackupKey, 0, KEY_ALL_ACCESS, &hKey);

    if (lResult == ERROR_SUCCESS) {


        //
        // get the local path, while we have the key open
        //        

        lResult = RegQueryValueEx(hKey, PROFILE_IMAGE_VALUE_NAME, 0, &dwType,
                                (LPBYTE)lpExpProfileImage, &cbExpProfileImage);

        RegCloseKey(hKey);
        hKey = NULL;
        

        if (lResult == ERROR_SUCCESS && cbExpProfileImage) {
            DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Local backup profile image filename = <%s>"), lpExpProfileImage));

            if (dwType == REG_EXPAND_SZ) {

                //
                // Expand the profile image filename
                //

                cb = sizeof(lpExpProfileImage);
                lpExpandedPath = LocalAlloc(LPTR, cb);
                if (lpExpandedPath) {
                    ExpandEnvironmentStrings(lpExpProfileImage, lpExpandedPath, cb);
                    lstrcpy(lpExpProfileImage, lpExpandedPath);
                    LocalFree(lpExpandedPath);
                }

                DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Expanded backup local profile image filename = <%s>"), lpExpProfileImage));
            }


            //
            // Check whether the key exists should already be done before this
            //

            lResult = RegRenameKey(HKEY_LOCAL_MACHINE, LocalBackupKey, LocalKey);
            if (lResult == ERROR_SUCCESS) {
                lstrcpy(lpBakProfileImage, lpExpProfileImage);
                
                if (lstrlen(lpExpProfileImage) > lstrlen(c_szBAK)) {
                    lpEnd = lpExpProfileImage+lstrlen(lpExpProfileImage)-lstrlen(c_szBAK);
                    *lpEnd = TEXT('\0');
                }


                //
                // Delete the profile only if there is backup directory
                //
                
                if (GetFileAttributesEx(lpBakProfileImage, GetFileExInfoStandard, &fad)) {
                    Delnode (lpExpProfileImage);
                }
                else {
                    DebugMsg((DM_WARNING, TEXT("ExtractProfileFromBackup:  Failed to get attributes for the backup directory.  Error = %d"), GetLastError()));
                    lResult = GetLastError();
                    bRetVal = FALSE;
                    goto Exit;
                }


                //
                // Attempt to rename the directory
                //

                if (!MoveFileEx(lpBakProfileImage, lpExpProfileImage, 0)) {                

                    //
                    // if it failed to move, rename it back again.
                    //
                    
                    RegRenameKey(HKEY_LOCAL_MACHINE, LocalKey, LocalBackupKey);

                    DebugMsg((DM_WARNING, TEXT("ExtractProfileFromBackup:  Failed to rename the directory.  Error = %d"), GetLastError()));
                    lResult = GetLastError();
                    bRetVal = FALSE;
                    goto Exit;
                }
                else {

                    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, LocalKey, 0, KEY_ALL_ACCESS, &hKey);

                    if (lResult == ERROR_SUCCESS) {
                        DWORD dwInternalFlags;
            

                        dwSize = sizeof(DWORD);
                        lResult = RegQueryValueEx (hKey, PROFILE_STATE, NULL, &dwType, (LPBYTE)&dwInternalFlags, &dwSize);

                        if (lResult == ERROR_SUCCESS) {

                            dwInternalFlags &= ~PROFILE_THIS_IS_BAK;
                            lResult = RegSetValueEx (hKey, PROFILE_STATE, 0, REG_DWORD,
                                             (LPBYTE) &dwInternalFlags, sizeof(dwInternalFlags));
                        }


                        dwSize = sizeof(TCHAR)*(MAX_PATH);
                        lResult = RegQueryValueEx (hKey, PROFILE_IMAGE_VALUE_NAME, NULL, &dwType, (LPBYTE)lpExpProfileImage, &dwSize);                        
                                             
                        if (lResult == ERROR_SUCCESS) {
                        
                            if (lstrlen(lpExpProfileImage) > lstrlen(c_szBAK)) {
                                lpEnd = lpExpProfileImage+lstrlen(lpExpProfileImage)-lstrlen(c_szBAK);
                                *lpEnd = TEXT('\0');
                            }
                        
                            lResult = RegSetValueEx (hKey, PROFILE_IMAGE_VALUE_NAME, 0, dwType,
                                             (LPBYTE) lpExpProfileImage, sizeof(TCHAR)*(lstrlen(lpExpProfileImage)+1));
                        }

                        
                        RegCloseKey(hKey);
                        hKey = NULL;
                    }                                         
                    else {
                        DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Failed to open LocalKey with error %d"), lResult));
                    }
                
                    bRetVal = TRUE;
                    *dwBackupFlags |= EX_PROFILE_CREATED;
                    DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Profile created from Backup")));
                    goto Exit;
                }
            }
            else {
                DebugMsg((DM_WARNING, TEXT("ExtractProfileFromBackup:  Couldn't rename key %s -> %s.  Error = %d"), LocalBackupKey, LocalKey, lResult));
                bRetVal = FALSE;
                goto Exit;
            }    
        }
        else {
            DebugMsg((DM_WARNING, TEXT("ExtractProfileFromBackup:  Couldn't get backup profile image.  Error = %d"), lResult));
            bRetVal = FALSE;
            goto Exit;
        }
    }
    else {
        DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Couldn't open backup profile key.  Error = %d"), lResult));
    }

Exit:
    if (hKey)
        RegCloseKey(hKey);

    if (!bRetVal)
        SetLastError(lResult);

    return bRetVal;        
}


//*************************************************************
//
//  PatchNewProfileIfRequired()
//
//  Purpose:  if the old sid and the new sid are not the same, delete the old
//             from the profile list and update the guidlist
//
//  Parameters: hToken   -   User Token
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/16/98    ushaji     Created
//
//*************************************************************
BOOL PatchNewProfileIfRequired(HANDLE hToken)
{
    TCHAR LocalOldProfileKey[MAX_PATH], LocalNewProfileKey[MAX_PATH], *lpEnd;
    HKEY  hNewKey=NULL;
    BOOL bRetVal = FALSE;
    DWORD dwType, dwDisp, dwSize;
    LONG lResult;
    LPTSTR OldSidString=NULL, SidString=NULL;
    PSID UserSid;
    DWORD dwBackupFlags;

    //
    // Get the current sid.
    //

    SidString = GetSidString(hToken);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("PatchNewProfileIfRequred: No SidString found")));
        return FALSE;
    }

    if (ExtractProfileFromBackup(hToken, SidString, &dwBackupFlags)) {
        if ((dwBackupFlags & EX_ALREADY_EXISTS) || (dwBackupFlags & EX_PROFILE_CREATED)) {
            DebugMsg((DM_VERBOSE, TEXT("PatchNewProfileIfRequred: A profile already exists with the current sid, exitting")));
            bRetVal = TRUE;
            goto Exit;
        }
    }
    else {
    
        //
        // Treat it as if no such profile exists
        //
        DebugMsg((DM_VERBOSE, TEXT("PatchNewProfileIfRequred: ExtractProfileFromBackup returned error %d"), GetLastError()));
    }


    //
    // Get the old sid.
    //

    OldSidString = GetOldSidString(hToken, PROFILE_GUID_PATH);

    if (!OldSidString) {
        DebugMsg((DM_VERBOSE, TEXT("PatchNewProfileIfRequred: No OldSidString found")));
        bRetVal = TRUE;
        goto Exit;
    }


    //
    // if old sid and new sid are the same quit
    //

    if (lstrcmpi(OldSidString, SidString) == 0) {
        DebugMsg((DM_VERBOSE, TEXT("PatchNewProfileIfRequred: Old and the new sid are the same, exitting")));
        bRetVal = TRUE;
        goto Exit;
    }


    if (ExtractProfileFromBackup(hToken, OldSidString, &dwBackupFlags)) {
        if ((dwBackupFlags & EX_ALREADY_EXISTS) || (dwBackupFlags & EX_PROFILE_CREATED)) {
            DebugMsg((DM_VERBOSE, TEXT("PatchNewProfileIfRequred: A profile with the old sid found")));
        }
    }
    else {
    
        //
        // Treat it as if no such profile exists
        //
        DebugMsg((DM_VERBOSE, TEXT("PatchNewProfileIfRequred: ExtractProfileFromBackup returned error %d"), GetLastError()));
    }
    

        
    lstrcpy(LocalNewProfileKey, PROFILE_LIST_PATH);
    lpEnd = CheckSlash (LocalNewProfileKey);
    lstrcpy(lpEnd, SidString);


    lstrcpy(LocalOldProfileKey, PROFILE_LIST_PATH);
    lpEnd = CheckSlash (LocalOldProfileKey);
    lstrcpy(lpEnd, OldSidString);


    lResult = RegRenameKey(HKEY_LOCAL_MACHINE, LocalOldProfileKey, LocalNewProfileKey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("PatchNewProfileIfRequred:  Failed to rename profile mapping key with error %d"), lResult));
        goto Exit;
    }

    
    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, LocalNewProfileKey, 0, 0, 0,
                             KEY_WRITE, NULL, &hNewKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("PatchNewProfileIfRequred:  Failed to open new profile mapping key with error %d"), lResult));
        goto Exit;
    }

    //
    // Get the sid of the logged on user
    //

    UserSid = GetUserSid(hToken);
    if (UserSid != NULL) {

        //
        // Store the user sid under the Sid key of the local profile
        //

        lResult = RegSetValueEx(hNewKey,
                    TEXT("Sid"),
                    0,
                    REG_BINARY,
                    UserSid,
                    RtlLengthSid(UserSid));


        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("CreateLocalProfileImage:  Failed to set 'sid' value of user in profile list, error = %d"), lResult));
        }

        //
        // We're finished with the user sid
        //

         DeleteUserSid(UserSid);
    }
    
         
    if (!SetOldSidString(hToken, SidString, PROFILE_GUID_PATH)) {
        DebugMsg((DM_WARNING, TEXT("SaveProfileInfo: Couldn't set the old Sid in the GuidList")));
    }


    //
    // the guid->sid corresp. for the next time will be saved in SaveProfileInfo above.
    //


    bRetVal = TRUE;

Exit:

    if (SidString)
        DeleteSidString(SidString);

    if (OldSidString)
        DeleteSidString(OldSidString);


    if (hNewKey)
        RegCloseKey(hNewKey);

    return bRetVal;
}

//*************************************************************
//
//  IncrementProfileRefCount()
//
//  Purpose:    Increments Profile Ref Count
//
//  Parameters: lpProfile   -   Profile Information
//              bInitilize  -   dwRef should be initialized
//
//  Return:     Ref Count
//
//  Comments:   This functions ref counts independent of ability
//              to load/unload the hive.
//
//  Caveat:
//              We have changed the machanism here to use ref counting
//              and not depend on unloadability of ntuser.dat. NT4
//              apps might have forgotten to unloaduserprofiles
//              and might still be working because the handle got
//              closed automatically when processes
//              exitted. This will be treated as an App Bug.
//
//
//  History:    Date        Author     Comment
//              1/12/99     ushaji     Created
//
//*************************************************************

DWORD IncrementProfileRefCount(LPPROFILE lpProfile, BOOL bInitialize)
{
    LPTSTR SidString, lpEnd;
    TCHAR LocalProfileKey[MAX_PATH];
    LONG lResult;
    HKEY hKey;
    DWORD dwType, dwSize, dwCount, dwDisp, dwRef=0;

    //
    // Get the Sid string for the user
    //

    SidString = GetSidString(lpProfile->hToken);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("IncrementProfileRefCount:  Failed to get sid string for user")));
        return 0;
    }


    //
    // Open the profile mapping
    //

    lstrcpy(LocalProfileKey, PROFILE_LIST_PATH);
    lpEnd = CheckSlash (LocalProfileKey);
    lstrcpy(lpEnd, SidString);

    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, LocalProfileKey, 0, 0, 0,
                             KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("IncrementProfileRefCount:  Failed to open profile mapping key with error %d"), lResult));
        DeleteSidString(SidString);
        return 0;
    }

    //
    // Query for the profile ref count.
    //

    dwSize = sizeof(DWORD);

    if (!bInitialize) {
        lResult = RegQueryValueEx (hKey,
                                   PROFILE_REF_COUNT,
                                   0,
                                   &dwType,
                                   (LPBYTE) &dwRef,
                                   &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_VERBOSE, TEXT("IncrementProfileRefCount:  Failed to query profile reference count with error %d"), lResult));
        }
    }

    dwRef++;

    //
    // Set the profile Ref count
    //

    lResult = RegSetValueEx (hKey,
                            PROFILE_REF_COUNT,
                            0,
                            REG_DWORD,
                            (LPBYTE) &dwRef,
                            sizeof(DWORD));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("IncrementProfileRefCount:  Failed to save profile reference count with error %d"), lResult));
    }


    DeleteSidString(SidString);

    RegCloseKey (hKey);

    return dwRef;

}

//*************************************************************
//
//  DecrementProfileRefCount()
//
//  Purpose:    Deccrements Profile Ref Count
//
//  Parameters: lpProfile   -   Profile Information
//
//  Return:     Ref Count
//
//  Comments:   This functions ref counts independent of ability
//              to load/unload the hive.
//
//  History:    Date        Author     Comment
//              1/12/99     ushaji     Created
//
//*************************************************************

DWORD DecrementProfileRefCount(LPPROFILE lpProfile)
{
    LPTSTR SidString, lpEnd;
    TCHAR LocalProfileKey[MAX_PATH];
    LONG lResult;
    HKEY hKey;
    DWORD dwType, dwSize, dwCount, dwDisp, dwRef=0;

    //
    // Get the Sid string for the user
    //

    SidString = GetSidString(lpProfile->hToken);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("DecrementProfileRefCount:  Failed to get sid string for user")));
        return 0;
    }


    //
    // Open the profile mapping
    //

    lstrcpy(LocalProfileKey, PROFILE_LIST_PATH);
    lpEnd = CheckSlash (LocalProfileKey);
    lstrcpy(lpEnd, SidString);

    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, LocalProfileKey, 0, 0, 0,
                             KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("DecrementProfileRefCount:  Failed to open profile mapping key with error %d"), lResult));
        DeleteSidString(SidString);
        return 0;
    }

    //
    // Query for the profile ref count.
    //

    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx (hKey,
                            PROFILE_REF_COUNT,
                            0,
                            &dwType,
                            (LPBYTE) &dwRef,
                            &dwSize);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("DecrementProfileRefCount:  Failed to query profile reference count with error %d"), lResult));
    }


    if (dwRef) {
        dwRef--;
    }
    else {
        DebugMsg((DM_WARNING, TEXT("DecrementRefCount: Ref Count is already zero !!!!!!")));
    }


    //
    // Set the profile Ref count
    //

    lResult = RegSetValueEx (hKey,
                            PROFILE_REF_COUNT,
                            0,
                            REG_DWORD,
                            (LPBYTE) &dwRef,
                            sizeof(DWORD));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("DecrementProfileRefCount:  Failed to save profile reference count with error %d"), lResult));
    }


    DeleteSidString(SidString);

    RegCloseKey (hKey);

    return dwRef;

}

//*************************************************************
//
//  SaveProfileInfo()
//
//  Purpose:    Saves key parts of the lpProfile structure
//              in the registry for UnloadUserProfile to use.
//
//  Parameters: lpProfile   -   Profile information
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              12/4/95     ericflo    Created
//
//*************************************************************

BOOL SaveProfileInfo(LPPROFILE lpProfile)
{
    LPTSTR SidString, lpEnd;
    TCHAR LocalProfileKey[MAX_PATH], lpUnexpLocalImage[MAX_PATH];
    LONG lResult;
    HKEY hKey;
    DWORD dwType, dwSize, dwCount, dwDisp;
    LPTSTR szUserGuid = NULL;

    //
    // Get the Sid string for the user
    //

    SidString = GetSidString(lpProfile->hToken);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to get sid string for user")));
        return FALSE;
    }


    //
    // Open the profile mapping
    //

    lstrcpy(LocalProfileKey, PROFILE_LIST_PATH);
    lpEnd = CheckSlash (LocalProfileKey);
    lstrcpy(lpEnd, SidString);

    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, LocalProfileKey, 0, 0, 0,
                             KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to open profile mapping key with error %d"), lResult));
        SetLastError(lResult);
        DeleteSidString(SidString);
        return FALSE;
    }

    //
    // Save the flags
    //
    lResult = RegSetValueEx (hKey,
                            PROFILE_FLAGS,
                            0,
                            REG_DWORD,
                            (LPBYTE) &lpProfile->dwFlags,
                            sizeof(DWORD));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to save flags with error %d"), lResult));
    }


    //
    // Save the internal flags
    //

    lResult = RegSetValueEx (hKey,
                            PROFILE_STATE,
                            0,
                            REG_DWORD,
                            (LPBYTE) &lpProfile->dwInternalFlags,
                            sizeof(DWORD));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to save flags2 with error %d"), lResult));
    }


    //
    // Save the central profile path
    //

    lResult = RegSetValueEx (hKey,
                            PROFILE_CENTRAL_PROFILE,
                            0,
                            REG_SZ,
                            (LPBYTE) lpProfile->lpRoamingProfile,
                            (lstrlen(lpProfile->lpRoamingProfile) + 1) * sizeof(TCHAR));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to save central profile with error %d"), lResult));
    }


    //
    // local profile path, saved in CreateLocalProfileImage
    //

    //
    // Save the profile load time
    //

    if (!(lpProfile->dwFlags & PI_LITELOAD)) {

        lResult = RegSetValueEx (hKey,
                                PROFILE_LOAD_TIME_LOW,
                                0,
                                REG_DWORD,
                                (LPBYTE) &lpProfile->ftProfileLoad.dwLowDateTime,
                                sizeof(DWORD));

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to save low profile load time with error %d"), lResult));
        }


        lResult = RegSetValueEx (hKey,
                                PROFILE_LOAD_TIME_HIGH,
                                0,
                                REG_DWORD,
                                (LPBYTE) &lpProfile->ftProfileLoad.dwHighDateTime,
                                sizeof(DWORD));

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to save high profile load time with error %d"), lResult));
        }
    }

  
    szUserGuid = GetUserGuid(lpProfile->hToken);

    if (szUserGuid) {
        lResult = RegSetValueEx (hKey,
                                PROFILE_GUID,
                                0,
                                REG_SZ,
                                (LPBYTE) szUserGuid,
                                (lstrlen(szUserGuid)+1)*sizeof(TCHAR));

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to save user guid with error %d"), lResult));
        }

        LocalFree(szUserGuid);
    }

    //
    // Save the guid->sid corresp. for the next time
    //

    if (!SetOldSidString(lpProfile->hToken, SidString, PROFILE_GUID_PATH)) {
        DebugMsg((DM_WARNING, TEXT("SaveProfileInfo: Couldn't set the old Sid in the GuidList")));
    }

    DeleteSidString(SidString);

    RegCloseKey (hKey);


    return(TRUE);
}

//*************************************************************
//
//  LoadProfileInfo()
//
//  Purpose:    Loads key parts of the lpProfile structure
//              in the registry for UnloadUserProfile to use.
//
//  Parameters: hToken            -   User's token
//              hKeyCurrentUser   -   User registry key handle
//
//  Return:     LPPROFILE if successful
//              NULL if not
//
//  Comments:   This function doesn't re-initialize all of the
//              fields in the PROFILE structure.
//
//  History:    Date        Author     Comment
//              12/5/95     ericflo    Created
//
//*************************************************************

LPPROFILE LoadProfileInfo (HANDLE hToken, HKEY hKeyCurrentUser)
{
    LPPROFILE lpProfile;
    LPTSTR SidString = NULL, lpEnd;
    TCHAR szBuffer[MAX_PATH];
    LONG lResult;
    HKEY hKey = NULL;
    DWORD dwType, dwSize;
    UINT i;
    BOOL bSuccess = FALSE;
    DWORD dwErr = 0;

    dwErr = GetLastError();

    //
    // Allocate an internal Profile structure to work with.
    //

    lpProfile = (LPPROFILE) LocalAlloc (LPTR, sizeof(USERPROFILE));

    if (!lpProfile) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo: Failed to allocate memory")));
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // Save the data passed in.
    //

    lpProfile->hToken = hToken;
    lpProfile->hKeyCurrentUser = hKeyCurrentUser;


    //
    // Allocate memory for the various paths
    //

    lpProfile->lpLocalProfile = LocalAlloc (LPTR, MAX_PATH * sizeof(TCHAR));

    if (!lpProfile->lpLocalProfile) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to alloc memory for local profile path.  Error = %d"),
                 GetLastError()));
        dwErr = GetLastError();
        goto Exit;
    }


    lpProfile->lpRoamingProfile = LocalAlloc (LPTR, MAX_PATH * sizeof(TCHAR));

    if (!lpProfile->lpRoamingProfile) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to alloc memory for central profile path.  Error = %d"),
                 GetLastError()));
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // Get the Sid string for the user
    //

    SidString = GetProfileSidString(lpProfile->hToken);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to get sid string for user")));
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // Open the profile mapping
    //

    lstrcpy(szBuffer, PROFILE_LIST_PATH);
    lpEnd = CheckSlash (szBuffer);
    lstrcpy(lpEnd, SidString);

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0,
                             KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to open profile mapping key with error %d"), lResult));
        dwErr = lResult;
        goto Exit;
    }


    //
    // Query for the flags
    //

    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx (hKey,
                               PROFILE_FLAGS,
                               NULL,
                               &dwType,
                               (LPBYTE) &lpProfile->dwFlags,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to query flags with error %d"), lResult));
        dwErr = lResult;
        goto Exit;
    }


    //
    // Query for the internal flags
    //

    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx (hKey,
                               PROFILE_STATE,
                               NULL,
                               &dwType,
                               (LPBYTE) &lpProfile->dwInternalFlags,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to query internal flags with error %d"), lResult));
        dwErr = lResult;
        goto Exit;
    }


    //
    // Query for the user preference value
    //


    lpProfile->dwUserPreference = USERINFO_UNDEFINED;
    dwSize = sizeof(DWORD);

    RegQueryValueEx (hKey,
                     USER_PREFERENCE,
                     NULL,
                     &dwType,
                     (LPBYTE) &lpProfile->dwUserPreference,
                     &dwSize);



    //
    // Query for the central profile path
    //

    dwSize = MAX_PATH * sizeof(TCHAR);
    lResult = RegQueryValueEx (hKey,
                               PROFILE_CENTRAL_PROFILE,
                               NULL,
                               &dwType,
                               (LPBYTE) lpProfile->lpRoamingProfile,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to query central profile with error %d"), lResult));
        dwErr = lResult;
        goto Exit;
    }


    //
    // Query for the local profile path.  The local profile path
    // needs to be expanded so read it into the temporary buffer.
    //

    dwSize = sizeof(szBuffer);
    lResult = RegQueryValueEx (hKey,
                               PROFILE_IMAGE_VALUE_NAME,
                               NULL,
                               &dwType,
                               (LPBYTE) szBuffer,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to query local profile with error %d"), lResult));
        dwErr = lResult;
        goto Exit;
    }

    //
    // Expand the local profile
    //

    ExpandEnvironmentStrings(szBuffer, lpProfile->lpLocalProfile, MAX_PATH);

    //
    // Query for the profile load time
    //

    lpProfile->ftProfileLoad.dwLowDateTime = 0;
    lpProfile->ftProfileLoad.dwHighDateTime = 0;

    if (!(lpProfile->dwFlags & PI_LITELOAD)) {
        dwSize = sizeof(lpProfile->ftProfileLoad.dwLowDateTime);

        lResult = RegQueryValueEx (hKey,
            PROFILE_LOAD_TIME_LOW,
            NULL,
            &dwType,
            (LPBYTE) &lpProfile->ftProfileLoad.dwLowDateTime,
            &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to query low profile load time with error %d"), lResult));
            dwErr = lResult;
            goto Exit;
        }


        dwSize = sizeof(lpProfile->ftProfileLoad.dwHighDateTime);

        lResult = RegQueryValueEx (hKey,
            PROFILE_LOAD_TIME_HIGH,
            NULL,
            &dwType,
            (LPBYTE) &lpProfile->ftProfileLoad.dwHighDateTime,
            &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to query high profile load time with error %d"), lResult));
            dwErr = lResult;
            goto Exit;
        }
    }

    //
    //  Sucess!
    //

    bSuccess = TRUE;


Exit:

    if (hKey) {
        RegCloseKey (hKey);
    }


    if (SidString) {
        DeleteSidString(SidString);
    }

    //
    // If the profile information was successfully loaded, return
    // lpProfile now.  Otherwise, free any memory and return NULL.
    //

    if (bSuccess) {
        SetLastError(dwErr);
        return lpProfile;
    }

    if (lpProfile) {

        if (lpProfile->lpRoamingProfile) {
            LocalFree (lpProfile->lpRoamingProfile);
        }

        if (lpProfile->lpLocalProfile) {
            LocalFree (lpProfile->lpLocalProfile);
        }

        LocalFree (lpProfile);
    }

    SetLastError(dwErr);

    return NULL;
}

//*************************************************************
//
//  CheckForSlowLink()
//
//  Purpose:    Checks if the network connection is slow.
//
//  Parameters: lpProfile   -   Profile Information
//              dwTime      -   Time delta
//              lpPath      -   UNC path to test
//
//  Return:     TRUE if profile should be downloaded
//              FALSE if not (use local)
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/21/96     ericflo    Created
//
//*************************************************************

BOOL CheckForSlowLink(LPPROFILE lpProfile, DWORD dwTime, LPTSTR lpPath)
{
    DWORD dwSlowTimeOut, dwSlowDlgTimeOut, dwSlowLinkDetectEnabled, dwSlowLinkUIEnabled;
    ULONG ulTransferRate;
    SLOWLINKDLGINFO info;
    DWORD dwType, dwSize;
    BOOL bRetVal = TRUE;
    HKEY hKey;
    LONG lResult;
    BOOL bSlow = FALSE;
    BOOL bLegacyCheck = TRUE;
    LPTSTR lpPathTemp, lpTempSrc, lpTempDest;
    LPSTR lpPathTempA;
    struct hostent *hostp;
    ULONG inaddr, ulSpeed;
    DWORD dwResult;
    PWSOCK32_API pWSock32;



    //
    // If the User Preferences states to always use the local
    // profile then we can exit now with true.  The profile
    // won't actually be downloaded.  In RestoreUserProfile,
    // this will be filtered out, and only the local will be used.
    //

    if (lpProfile->dwUserPreference == USERINFO_LOCAL) {
        return TRUE;
    }


    //
    // Get the slow link detection flag, slow link timeout,
    // dialog box timeout values, and default profile to use.
    //

    dwSlowTimeOut = SLOW_LINK_TIMEOUT;
    dwSlowDlgTimeOut = PROFILE_DLG_TIMEOUT;
    dwSlowLinkDetectEnabled = 1;
    dwSlowLinkUIEnabled = 0;  
    ulTransferRate = SLOW_LINK_TRANSFER_RATE;
    bRetVal = FALSE;


    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           WINLOGON_KEY,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkDetectEnabled"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowLinkDetectEnabled,
                         &dwSize);

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkTimeOut"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowTimeOut,
                         &dwSize);

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("ProfileDlgTimeOut"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowDlgTimeOut,
                         &dwSize);

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkUIEnabled"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowLinkUIEnabled,
                         &dwSize);

        dwSize = sizeof(BOOL);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkProfileDefault"),
                         NULL,
                         &dwType,
                         (LPBYTE) &bRetVal,
                         &dwSize);

        dwSize = sizeof(ULONG);
        RegQueryValueEx (hKey,
                         TEXT("UserProfileMinTransferRate"),
                         NULL,
                         &dwType,
                         (LPBYTE) &ulTransferRate,
                         &dwSize);

        RegCloseKey (hKey);
    }


    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           SYSTEM_POLICIES_KEY,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkDetectEnabled"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowLinkDetectEnabled,
                         &dwSize);

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkTimeOut"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowTimeOut,
                         &dwSize);

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("ProfileDlgTimeOut"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowDlgTimeOut,
                         &dwSize);

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkUIEnabled"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowLinkUIEnabled,
                         &dwSize);

        dwSize = sizeof(BOOL);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkProfileDefault"),
                         NULL,
                         &dwType,
                         (LPBYTE) &bRetVal,
                         &dwSize);

        dwSize = sizeof(ULONG);
        RegQueryValueEx (hKey,
                         TEXT("UserProfileMinTransferRate"),
                         NULL,
                         &dwType,
                         (LPBYTE) &ulTransferRate,
                         &dwSize);

        RegCloseKey (hKey);
    }


    //
    // If slow link detection is disabled, then always download
    // the profile.
    //

    if (!dwSlowLinkDetectEnabled || !ulTransferRate) {
        return TRUE;
    }


    //
    // If lpPath is  UNC path, try pinging the server
    //

    if ((*lpPath == TEXT('\\')) && (*(lpPath+1) == TEXT('\\'))) {

        lpPathTemp = LocalAlloc (LPTR, (lstrlen(lpPath)+1) * sizeof(TCHAR));

        if (lpPathTemp) {
            lpTempSrc = lpPath+2;
            lpTempDest = lpPathTemp;

            while ((*lpTempSrc != TEXT('\\')) && *lpTempSrc) {
                *lpTempDest = *lpTempSrc;
                lpTempDest++;
                lpTempSrc++;
            }

            lpPathTempA = ProduceAFromW(lpPathTemp);

            if (lpPathTempA) {

                pWSock32 = LoadWSock32();

                if ( pWSock32 ) {

                    hostp = pWSock32->pfngethostbyname(lpPathTempA);

                    if (hostp) {
                        inaddr = *(long *)hostp->h_addr;

                        dwResult = PingComputer (inaddr, &ulSpeed);

                        if (dwResult == ERROR_SUCCESS) {

                            if (ulSpeed) {

                                //
                                // If the delta time is greater that the timeout time, then this
                                // is a slow link.
                                //

                                if (ulSpeed < ulTransferRate) {
                                    bSlow = TRUE;
                                }
                            }

                            bLegacyCheck = FALSE;
                        }
                    }
                }

                FreeProducedString(lpPathTempA);
            }

            LocalFree (lpPathTemp);
        }
    }

    
    if (bLegacyCheck) {

        //
        // If the delta time is less that the timeout time, then it
        // is ok to download their profile (fast enough net connection).
        //

        if (dwTime < dwSlowTimeOut) {
            return TRUE;
        }

    } else {

        if (!bSlow) {
            return TRUE;
        }
    }


    // BugBug:
    // Default is not download. There is no UI support any longer. This can go..
    //
    //

#if 0
    //
    // If the User Preferences states to always use the local
    // profile on slow links, then we can exit now with false.
    // 
    //

    if (lpProfile->dwUserPreference == USERINFO_LOCAL_SLOW_LINK) {
        lpProfile->dwInternalFlags |= PROFILE_SLOW_LINK;
        DebugMsg((DM_VERBOSE, TEXT("CheckForSlowLink:  The profile is across a slow link")));
        return FALSE;
    }
#endif

    //
    // Display the slow link dialog
    //
    // If someone sets the dialog box timeout to 0, then we
    // don't want to prompt the user.  Just do the default
    //

    
    if ((dwSlowLinkUIEnabled) && (dwSlowDlgTimeOut > 0) && (!(lpProfile->dwFlags & PI_NOUI))) {

        info.dwTimeout = dwSlowDlgTimeOut;
        info.bDownloadDefault = bRetVal;

        DebugMsg((DM_VERBOSE, TEXT("CheckForSlowLink:: Calling DialogBoxParam")));
        bRetVal = (BOOL)DialogBoxParam (g_hDllInstance, MAKEINTRESOURCE(IDD_SLOW_LINK),
                                        NULL, SlowLinkDlgProc, (LPARAM) &info);

    }

    if (!bRetVal) {
        lpProfile->dwInternalFlags |= PROFILE_SLOW_LINK;
        DebugMsg((DM_VERBOSE, TEXT("CheckForSlowLink:  The profile is across a slow link")));
    }

    return bRetVal;
}


//*************************************************************
//
//  SlowLinkDlgProc()
//
//  Purpose:    Dialog box procedure for the slow link dialog
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/13/96     ericflo    Created
//
//*************************************************************

INT_PTR APIENTRY SlowLinkDlgProc (HWND hDlg, UINT uMsg,
                               WPARAM wParam, LPARAM lParam)
{
    TCHAR szBuffer[10];
    static DWORD dwSlowLinkTime;
    BOOL bDownloadDefault;

    switch (uMsg) {

        case WM_INITDIALOG:
           CenterWindow (hDlg);

           //
           // Set the default button and focus
           //

           if (((LPSLOWLINKDLGINFO)lParam)->bDownloadDefault) {

                SetFocus (GetDlgItem(hDlg, IDC_DOWNLOAD));

           } else {
                HWND hwnd;
                LONG style;

                //
                // Set the default button to Local
                //

                hwnd = GetDlgItem (hDlg, IDC_DOWNLOAD);
                style = GetWindowLong (hwnd, GWL_STYLE);
                style &= ~(BS_DEFPUSHBUTTON | BS_NOTIFY);
                style |= BS_PUSHBUTTON;
                SetWindowLong (hwnd, GWL_STYLE, style);

                hwnd = GetDlgItem (hDlg, IDC_LOCAL);
                style = GetWindowLong (hwnd, GWL_STYLE);
                style &= ~(BS_PUSHBUTTON | BS_DEFPUSHBUTTON);
                style |= (BS_DEFPUSHBUTTON | BS_NOTIFY);
                SetWindowLong (hwnd, GWL_STYLE, style);

                SetFocus (GetDlgItem(hDlg, IDC_LOCAL));
           }

           SetWindowLongPtr (hDlg, DWLP_USER, ((LPSLOWLINKDLGINFO)lParam)->bDownloadDefault);
           dwSlowLinkTime = ((LPSLOWLINKDLGINFO)lParam)->dwTimeout;
           wsprintf (szBuffer, TEXT("%d"), dwSlowLinkTime);
           SetDlgItemText (hDlg, IDC_TIMEOUT, szBuffer);
           SetTimer (hDlg, 1, 1000, NULL);
           return FALSE;

        case WM_TIMER:

           if (dwSlowLinkTime >= 1) {

               dwSlowLinkTime--;
               wsprintf (szBuffer, TEXT("%d"), dwSlowLinkTime);
               SetDlgItemText (hDlg, IDC_TIMEOUT, szBuffer);

           } else {

               //
               // Time's up.  Do the default action.
               //

               bDownloadDefault = (BOOL) GetWindowLongPtr (hDlg, DWLP_USER);

               if (bDownloadDefault) {
                   PostMessage (hDlg, WM_COMMAND, IDC_DOWNLOAD, 0);

               } else {
                   PostMessage (hDlg, WM_COMMAND, IDC_LOCAL, 0);
               }
           }
           break;

        case WM_COMMAND:

          switch (LOWORD(wParam)) {

              case IDC_DOWNLOAD:
                  if (HIWORD(wParam) == BN_KILLFOCUS) {
                      bDownloadDefault = (BOOL) GetWindowLongPtr (hDlg, DWLP_USER);
                      if (bDownloadDefault) {
                          KillTimer (hDlg, 1);
                          ShowWindow(GetDlgItem(hDlg, IDC_TIMEOUT), SW_HIDE);
                          ShowWindow(GetDlgItem(hDlg, IDC_TIMETITLE), SW_HIDE);
                      }
                  } else if (HIWORD(wParam) == BN_CLICKED) {
                      DebugMsg((DM_VERBOSE, TEXT("CheckForSlowLink:: Killing DialogBox because download button was clicked")));
                      KillTimer (hDlg, 1);
                      EndDialog(hDlg, TRUE);
                  }
                  break;

              case IDC_LOCAL:
                  if (HIWORD(wParam) == BN_KILLFOCUS) {
                      bDownloadDefault = (BOOL) GetWindowLongPtr (hDlg, DWLP_USER);
                      if (!bDownloadDefault) {
                          KillTimer (hDlg, 1);
                          ShowWindow(GetDlgItem(hDlg, IDC_TIMEOUT), SW_HIDE);
                          ShowWindow(GetDlgItem(hDlg, IDC_TIMETITLE), SW_HIDE);
                      }
                      break;
                  } else if (HIWORD(wParam) == BN_CLICKED) {
                      DebugMsg((DM_VERBOSE, TEXT("CheckForSlowLink:: Killing DialogBox because local button was clicked")));
                      KillTimer (hDlg, 1);
                      EndDialog(hDlg, FALSE);
                  }
                  break;

              case IDCANCEL:
                  bDownloadDefault = (BOOL) GetWindowLongPtr (hDlg, DWLP_USER);

                  //
                  // Nothing to do.  Save the state and return.
                  //

                  DebugMsg((DM_VERBOSE, TEXT("CheckForSlowLink:: Killing DialogBox because local/cancel button was clicked")));
                  KillTimer (hDlg, 1);

                  //
                  // Return Whatever is the default in this case..
                  //

                  EndDialog(hDlg, bDownloadDefault);
                  break;

              default:
                  break;

          }
          break;

    }

    return FALSE;
}

//*************************************************************
//
//  GetUserPreferenceValue()
//
//  Purpose:    Gets the User Preference flags
//
//  Parameters: hToken  -   User's token
//
//  Return:     Value
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/22/96     ericflo    Created
//
//*************************************************************

DWORD GetUserPreferenceValue(HANDLE hToken)
{
    TCHAR LocalProfileKey[MAX_PATH];
    DWORD RegErr, dwType, dwSize, dwRetVal = USERINFO_UNDEFINED;
    LPTSTR lpEnd;
    LPTSTR SidString;
    HKEY hkeyProfile;


    SidString = GetProfileSidString(hToken);
    if (SidString != NULL) {

        //
        // Query for the UserPreference value
        //

        lstrcpy(LocalProfileKey, PROFILE_LIST_PATH);
        lpEnd = CheckSlash (LocalProfileKey);
        lstrcpy(lpEnd, SidString);

        RegErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              LocalProfileKey,
                              0,
                              KEY_READ,
                              &hkeyProfile);


        if (RegErr == ERROR_SUCCESS) {

            dwSize = sizeof(dwRetVal);
            RegQueryValueEx(hkeyProfile,
                            USER_PREFERENCE,
                            NULL,
                            &dwType,
                            (LPBYTE) &dwRetVal,
                            &dwSize);

            RegCloseKey (hkeyProfile);
        }

        DeleteSidString(SidString);
    }

    return dwRetVal;
}


//*************************************************************
//
//  IsTempProfileAllowed()
//
//  Purpose:    Gets the temp profile policy
//
//  Parameters:
//
//  Return:     true if temp profile can be created, false otherwise
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/8/99      ushaji     Created
//
//*************************************************************

BOOL IsTempProfileAllowed()
{
    HKEY hKey;
    LONG lResult;
    DWORD dwSize, dwType;
    DWORD dwRetVal = PROFILEERRORACTION_TEMP;

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           SYSTEM_POLICIES_KEY,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("ProfileErrorAction"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwRetVal,
                         &dwSize);

        RegCloseKey (hKey);
    }

    DebugMsg((DM_VERBOSE, TEXT("IsTempProfileAllowed:  Returning %d"), (dwRetVal == PROFILEERRORACTION_TEMP)));
    return (dwRetVal == PROFILEERRORACTION_TEMP);
}

//*************************************************************
//
//  MoveUserProfiles()
//
//  Purpose:    Moves all user profiles from source location
//              to the new profile location
//
//  Parameters: lpSrcDir   -   Source directory
//              lpDestDir  -   Destination directory
//
//  Notes:      The source directory should be given in the same
//              format as the pathnames appear in the ProfileList
//              registry key.  eg:  normally the profile paths
//              are in this form:  %SystemRoot%\Profiles.  The
//              path passed to this function should be in the unexpanded
//              format.
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL MoveUserProfiles (LPCTSTR lpSrcDir, LPCTSTR lpDestDir)
{
    BOOL bResult = TRUE;
    LONG lResult;
    DWORD dwIndex, dwType, dwSize, dwDisp;
    DWORD dwLength, dwLengthNeeded, dwStrLen;
    PSECURITY_DESCRIPTOR pSD;
    LPTSTR lpEnd, lpNewPathEnd, lpNameEnd;
    TCHAR szName[75];
    TCHAR szTemp[MAX_PATH + 1];
    TCHAR szOldProfilePath[MAX_PATH + 1];
    TCHAR szNewProfilePath[MAX_PATH + 1];
    TCHAR szExpOldProfilePath[MAX_PATH + 1] = {0};
    TCHAR szExpNewProfilePath[MAX_PATH + 1];
    WIN32_FILE_ATTRIBUTE_DATA fad;
    INT iSrcDirLen;
    HKEY hKeyProfileList, hKeyProfile, hKeyFolders;
    FILETIME ftWrite;


    //
    // Make sure we don't try to move on top of ourselves
    //

    if (lstrcmpi (lpSrcDir, lpDestDir) == 0) {
        DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Old profiles directory and new profiles directory are the same.")));
        bResult = FALSE;
        goto Exit;
    }


    //
    // Open the profile list
    //

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH,
                            0, KEY_READ, &hKeyProfileList);

    if (lResult != ERROR_SUCCESS) {
        if (lResult != ERROR_PATH_NOT_FOUND) {
            DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to open profile list registry key with %d"), lResult));
            bResult = FALSE;
        }
        goto DoDefaults;
    }


    //
    // Enumerate the profiles
    //

    lstrcpy (szTemp, PROFILE_LIST_PATH);
    lpEnd = CheckSlash (szTemp);
    iSrcDirLen = lstrlen (lpSrcDir);

    dwIndex = 0;
    dwSize = ARRAYSIZE(szName);

    while (RegEnumKeyEx (hKeyProfileList, dwIndex, szName, &dwSize, NULL, NULL,
                  NULL, &ftWrite) == ERROR_SUCCESS) {


        //
        // Check if this profile is in use
        //

        if (RegOpenKeyEx(HKEY_USERS, szName, 0, KEY_READ,
                         &hKeyProfile) == ERROR_SUCCESS) {

            DebugMsg((DM_VERBOSE, TEXT("MoveUserProfiles:  Skipping <%s> because it is in use."), szName));
            RegCloseKey (hKeyProfile);
            goto LoopAgain;
        }


        //
        // Open the key for a specific profile
        //

        lstrcpy (lpEnd, szName);

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTemp, 0,
                     KEY_READ | KEY_WRITE, &hKeyProfile) == ERROR_SUCCESS) {


            //
            // Query for the previous profile location
            //

            szOldProfilePath[0] = TEXT('\0');
            dwSize = ARRAYSIZE(szOldProfilePath) * sizeof(TCHAR);

            RegQueryValueEx (hKeyProfile, PROFILE_IMAGE_VALUE_NAME, NULL,
                             &dwType, (LPBYTE) szOldProfilePath, &dwSize);


            //
            // If the profile is located in the source directory,
            // move it to the new profiles directory.
            //

            if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                               szOldProfilePath, iSrcDirLen,
                               lpSrcDir, iSrcDirLen) == CSTR_EQUAL) {

                //
                // Copy the user's name into a buffer we can change
                //

                lstrcpy (szName, (szOldProfilePath + iSrcDirLen + 1));


                //
                // If the user's name has a .000, .001, etc at the end,
                // remove that.
                //

                dwStrLen = lstrlen(szName);
                if (dwStrLen > 3) {
                    lpNameEnd = szName + dwStrLen - 4;

                    if (*lpNameEnd == TEXT('.')) {
                        *lpNameEnd = TEXT('\0');
                    }
                }


                //
                // Call ComputeLocalProfileName to get the new
                // profile directory (this also creates the directory)
                //

                lstrcpy (szNewProfilePath, lpDestDir);

                if (!ComputeLocalProfileName (NULL, szName,
                              szNewProfilePath, ARRAYSIZE(szNewProfilePath),
                              szExpNewProfilePath, ARRAYSIZE(szExpNewProfilePath),
                              NULL, FALSE)) {
                    DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to generate unique directory name for <%s>"),
                              szName));
                    goto LoopAgain;
                }


                DebugMsg((DM_VERBOSE, TEXT("MoveUserProfiles:  Moving <%s> to <%s>"),
                          szOldProfilePath, szNewProfilePath));

                ExpandEnvironmentStrings (szOldProfilePath, szExpOldProfilePath,
                                          ARRAYSIZE(szExpOldProfilePath));


                //
                // Copy the ACLs from the old location to the new
                //

                dwLength = 1024;

                pSD = (PSECURITY_DESCRIPTOR)LocalAlloc (LPTR, dwLength);

                if (pSD) {

                    if (GetFileSecurity (szExpOldProfilePath,
                                         DACL_SECURITY_INFORMATION,
                                         pSD, dwLength, &dwLengthNeeded) &&
                        (dwLengthNeeded == 0)) {

                        SetFileSecurity (szExpNewProfilePath,
                                         DACL_SECURITY_INFORMATION, pSD);
                    } else {
                        DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to allocate get security descriptor with %d.  dwLengthNeeded = %d"),
                                 GetLastError(), dwLengthNeeded));
                    }

                    LocalFree (pSD);

                } else {
                    DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to allocate memory for SD with %d."),
                             GetLastError()));
                }


                //
                // Copy the files from the old location to the new
                //

                if (CopyProfileDirectory (szExpOldProfilePath, szExpNewProfilePath,
                                          CPD_COPYIFDIFFERENT)) {

                    DebugMsg((DM_VERBOSE, TEXT("MoveUserProfiles:  Profile copied successfully.")));


                    //
                    // Change the registry to point at the new profile
                    //

                    lResult = RegSetValueEx (hKeyProfile, PROFILE_IMAGE_VALUE_NAME, 0,
                                             REG_EXPAND_SZ, (LPBYTE) szNewProfilePath,
                                             ((lstrlen(szNewProfilePath) + 1) * sizeof(TCHAR)));

                    if (lResult == ERROR_SUCCESS) {

                        //
                        // Delete the old profile
                        //

                        Delnode (szExpOldProfilePath);

                    } else {
                        DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to set new profile path in registry with %d."), lResult));
                    }


                } else {
                    DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  CopyProfileDirectory failed.")));
                }
            }

            RegCloseKey (hKeyProfile);
        }

LoopAgain:

        dwIndex++;
        dwSize = ARRAYSIZE(szName);
    }

    RegCloseKey (hKeyProfileList);


DoDefaults:


    lstrcpy (szOldProfilePath, lpSrcDir);
    ExpandEnvironmentStrings (szOldProfilePath, szExpOldProfilePath,
                              ARRAYSIZE(szExpOldProfilePath));

    lpEnd = CheckSlash(szExpOldProfilePath);


    //
    // Now try to move the Default User profile
    //

    lstrcpy (lpEnd, DEFAULT_USER);
    if (GetFileAttributesEx (szExpOldProfilePath, GetFileExInfoStandard, &fad)) {

        dwSize = ARRAYSIZE(szExpNewProfilePath);
        if (!GetDefaultUserProfileDirectoryEx(szExpNewProfilePath, &dwSize, TRUE)) {
            DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to query default user profile directory.")));
            goto Exit;
        }

        if (CopyProfileDirectory (szExpOldProfilePath, szExpNewProfilePath,
                                  CPD_COPYIFDIFFERENT)) {
            Delnode (szExpOldProfilePath);
        }
    }


    //
    // Delnode the Network Default User profile if it exists
    //

    lstrcpy (lpEnd, DEFAULT_USER_NETWORK);
    Delnode (szExpOldProfilePath);


    //
    // Now try to move the All Users profile
    //

    lstrcpy (lpEnd, ALL_USERS);
    if (GetFileAttributesEx (szExpOldProfilePath, GetFileExInfoStandard, &fad)) {

        dwSize = ARRAYSIZE(szExpNewProfilePath);
        if (!GetAllUsersProfileDirectoryEx(szExpNewProfilePath, &dwSize, TRUE)) {
            DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to query all users profile directory.")));
            goto Exit;
        }

        if (CopyProfileDirectory (szExpOldProfilePath, szExpNewProfilePath,
                                  CPD_COPYIFDIFFERENT)) {
            Delnode (szExpOldProfilePath);
        }
    }


    //
    // If possible, remove the old profiles directory
    //

    ExpandEnvironmentStrings (lpSrcDir, szExpOldProfilePath,
                              ARRAYSIZE(szExpOldProfilePath));

    RemoveDirectory (szExpOldProfilePath);


Exit:

    return bResult;
}


//*************************************************************
//
//  PrepareProfileForUse()
//
//  Purpose:    Prepares the profile for use on this machine.
//
//  Parameters: lpProfile  -  Profile information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL PrepareProfileForUse (LPPROFILE lpProfile)
{
    TCHAR szTemp[MAX_PATH];
    TCHAR szExpTemp[MAX_PATH];
    HKEY hKey, hKeyShellFolders = NULL;
    DWORD dwSize, dwType, dwDisp, dwStrLen, i;


    //
    // Calculate the length of the user profile environment variable
    //

    dwStrLen = lstrlen (TEXT("%USERPROFILE%"));


    //
    // Open the Shell Folders key
    //

    RegCreateKeyEx(lpProfile->hKeyCurrentUser, SHELL_FOLDERS, 0, 0, 0,
                   KEY_WRITE, NULL, &hKeyShellFolders, &dwDisp);


    //
    // Open the User Shell Folders key
    //

    if (RegOpenKeyEx (lpProfile->hKeyCurrentUser,
                      USER_SHELL_FOLDERS, 0, KEY_READ,
                      &hKey) == ERROR_SUCCESS) {


        //
        // Enumerate the folders we know about
        //

        for (i=0; i < g_dwNumShellFolders; i++) {

            //
            // Query for the unexpanded path name
            //

            szTemp[0] = TEXT('\0');
            dwSize = sizeof(szTemp);
            if (RegQueryValueEx (hKey, c_ShellFolders[i].lpFolderName, NULL,
                                &dwType, (LPBYTE) szTemp, &dwSize) == ERROR_SUCCESS) {


                //
                // Expand the path name
                //

                ExpandEnvironmentStrings (szTemp, szExpTemp, ARRAYSIZE(szExpTemp));


                //
                // If this is a local directory, create it and set the
                // hidden bit if appropriate
                //

                if (c_ShellFolders[i].bLocal) {

                    if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                       TEXT("%USERPROFILE%"), dwStrLen,
                                       szTemp, dwStrLen) == CSTR_EQUAL) {

                        if (CreateNestedDirectory (szExpTemp, NULL)) {

                            if (c_ShellFolders[i].bHidden) {
                                SetFileAttributes(szExpTemp, FILE_ATTRIBUTE_HIDDEN);
                            } else {
                                SetFileAttributes(szExpTemp, FILE_ATTRIBUTE_NORMAL);
                            }

                        } else {
                            DebugMsg((DM_WARNING, TEXT("PrepareProfileForUse:  Failed to create directory <%s> with %d."),
                                     szExpTemp, GetLastError()));
                        }
                    }
                }


                //
                // Set the expanded path in the Shell Folders key.
                // This helps some apps that look at the Shell Folders
                // key directly instead of using the shell api
                //

                if (hKeyShellFolders) {

                    RegSetValueEx (hKeyShellFolders, c_ShellFolders[i].lpFolderName, 0,
                                   REG_SZ, (LPBYTE) szExpTemp,
                                   ((lstrlen(szExpTemp) + 1) * sizeof(TCHAR)));
                }
            }
        }

        RegCloseKey (hKey);
    }


    //
    // Close the Shell Folders key
    //

    if (hKeyShellFolders) {
        RegCloseKey (hKeyShellFolders);
    }


    //
    // Now check that the temp directory exists.
    //

    if (RegOpenKeyEx (lpProfile->hKeyCurrentUser,
                      TEXT("Environment"), 0, KEY_READ,
                      &hKey) == ERROR_SUCCESS) {

        //
        // Check for TEMP
        //

        szTemp[0] = TEXT('\0');
        dwSize = sizeof(szTemp);
        if (RegQueryValueEx (hKey, TEXT("TEMP"), NULL, &dwType,
                             (LPBYTE) szTemp, &dwSize) == ERROR_SUCCESS) {

            if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                               TEXT("%USERPROFILE%"), dwStrLen,
                               szTemp, dwStrLen) == CSTR_EQUAL) {

                ExpandEnvironmentStrings (szTemp, szExpTemp, ARRAYSIZE(szExpTemp));
                if (!CreateNestedDirectory (szExpTemp, NULL)) {
                    DebugMsg((DM_WARNING, TEXT("PrepareProfileForUse:  Failed to create temp directory <%s> with %d."),
                             szExpTemp, GetLastError()));
                }
            }
        }


        //
        // Check for TMP
        //

        szTemp[0] = TEXT('\0');
        dwSize = sizeof(szTemp);
        if (RegQueryValueEx (hKey, TEXT("TMP"), NULL, &dwType,
                             (LPBYTE) szTemp, &dwSize) == ERROR_SUCCESS) {

            if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                               TEXT("%USERPROFILE%"), dwStrLen,
                               szTemp, dwStrLen) == CSTR_EQUAL) {

                ExpandEnvironmentStrings (szTemp, szExpTemp, ARRAYSIZE(szExpTemp));
                if (!CreateNestedDirectory (szExpTemp, NULL)) {
                    DebugMsg((DM_WARNING, TEXT("PrepareProfileForUse:  Failed to create temp directory with %d."),
                             GetLastError()));
                }
            }
        }

        RegCloseKey (hKey);
    }

    return TRUE;
}



//*************************************************************
//
//  DeleteProfile()
//
//  Purpose:    Deletes the profile
//
//  Parameters:
//
//  Return:     true if successful
//
//  Comments:
//
//  History:    Date        Author     Comment
//              4/12/99     ushaji     Created
//
// TBD: Change some of the DeleteProfileEx calls to DeleteProfile
//
//*************************************************************

BOOL 
DeleteProfile (LPCTSTR lpSidString, LPCTSTR lpProfilePath, LPCTSTR szComputerName)
{
    LPTSTR lpEnd;
    TCHAR  szBuffer[MAX_PATH], szProfilePath[MAX_PATH];
    LONG   lResult;
    HKEY   hKey = NULL;
    HKEY   hKeyCurrentVersion = NULL;
    HKEY   hKeyNetCache = NULL;
    DWORD  dwType, dwSize;
    BOOL   bSuccess = FALSE;
    DWORD  dwErr = 0;
    HKEY   hKeyLocalLM;
    BOOL   bRemoteReg = FALSE;
    BOOL   bEnvVarsSet = FALSE;
    TCHAR  szOrigSysRoot[MAX_PATH], szOrigSysDrive[MAX_PATH], tDrive;
    TCHAR  szShareName[MAX_PATH], szFileSystemName[MAX_PATH];
    DWORD  MaxCompLen, FileSysFlags;
    TCHAR  szSystemRoot[MAX_PATH], szSystemDrive[MAX_PATH];
    DWORD  dwBufferSize;
    TCHAR  szTemp[MAX_PATH];
    DWORD  dwInternalFlags=0, dwDeleteFlags=0;

    if (!lpSidString) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (szComputerName) {

        GetEnvironmentVariable(TEXT("SystemRoot"), szOrigSysRoot, MAX_PATH);
        GetEnvironmentVariable(TEXT("SystemDrive"), szOrigSysDrive, MAX_PATH);

        lResult = RegConnectRegistry(szComputerName, HKEY_LOCAL_MACHINE, &hKeyLocalLM);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to open remote registry %d"), lResult));
            dwErr = lResult;
            goto Exit;
        }

        bRemoteReg = TRUE;

        //
        // Get the value of %SystemRoot% and %SystemDrive% relative to the computer
        //

        lResult = RegOpenKeyEx(hKeyLocalLM,
                               TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion"),
                               0,
                               KEY_READ,
                               &hKeyCurrentVersion);
   
            
        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to open remote registry CurrentVersion %d"), lResult));
            dwErr = lResult;
            goto Exit;
        }

        dwBufferSize = MAX_PATH * sizeof(TCHAR);

        lResult = RegQueryValueEx(hKeyCurrentVersion,
                                  TEXT("SystemRoot"),
                                  NULL,
                                  NULL,
                                  (BYTE *) szTemp,
                                  &dwBufferSize);

        RegCloseKey (hKeyCurrentVersion);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to open remote registry SystemRoot %d"), lResult));
            dwErr = lResult;
            goto Exit;
        }

        szTemp[1] = TEXT('$');

        //
        // These needs to be set if there are additional places below which uses envvars...
        //

        lstrcpy(szSystemRoot, szComputerName); lstrcat(szSystemRoot, TEXT("\\"));
        lstrcpy(szSystemDrive, szComputerName); lstrcat(szSystemDrive, TEXT("\\"));

        lpEnd = szSystemDrive+lstrlen(szSystemDrive);
        lstrcpyn(lpEnd, szTemp, 3);

        lpEnd = szSystemRoot+lstrlen(szSystemRoot);
        lstrcpy(lpEnd, szTemp);

        SetEnvironmentVariable(TEXT("SystemRoot"), szSystemRoot);
        SetEnvironmentVariable(TEXT("SystemDrive"), szSystemDrive);

        bEnvVarsSet = TRUE;

    }
    else {
        hKeyLocalLM = HKEY_LOCAL_MACHINE;
    }


    dwErr = GetLastError();

    if (!lpProfilePath) {

        TCHAR szTemp[MAX_PATH];
        
        //
        // Open the profile mapping
        //
        
        lstrcpy(szProfilePath, PROFILE_LIST_PATH);
        lpEnd = CheckSlash (szProfilePath);
        lstrcpy(lpEnd, lpSidString);
        
        lResult = RegOpenKeyEx(hKeyLocalLM, szProfilePath, 0,
            KEY_READ, &hKey);
        
        
        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to open profile mapping key with error %d"), lResult));
            dwErr = lResult;
            goto Exit;
        }
        
        //
        // Get the profile path...
        //
        
        dwSize = sizeof(szTemp);
        lResult = RegQueryValueEx (hKey,
                                   PROFILE_IMAGE_VALUE_NAME,
                                   NULL,
                                   &dwType,
                                   (LPBYTE) szTemp,
                                   &dwSize);
                                   
        
        dwSize = sizeof(DWORD);
        lResult = RegQueryValueEx (hKey, PROFILE_STATE, NULL, &dwType, (LPBYTE)&dwInternalFlags, &dwSize);


        if (hKey) 
            RegCloseKey(hKey);
        
        hKey = NULL;

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to query local profile with error %d"), lResult));
            dwErr = lResult;
            goto Exit;
        }
        
        if (!ExpandEnvironmentStrings(szTemp, szBuffer, MAX_PATH)) {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to expand %s, error %d"), szTemp, GetLastError()));
            dwErr = lResult;
            goto Exit;
        }

    }
    else {
        lstrcpy(szBuffer, lpProfilePath);
    }

    if (dwInternalFlags & PROFILE_THIS_IS_BAK) 
        dwDeleteFlags |= DP_DELBACKUP;

    //
    // Do not fail if for some reason we could not delete the profiledir
    //

    bSuccess = DeleteProfileEx(lpSidString, szBuffer, dwDeleteFlags, hKeyLocalLM);

    if (!bSuccess) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to delete directory, %s with error %d"), szBuffer, dwErr));
    }

    //
    // Delete the user's trash..
    //
    
    if (szComputerName) {
        lstrcpy (szShareName, szComputerName); lstrcat(szShareName, TEXT("\\"));
        lpEnd = szShareName+lstrlen(szShareName); 
        lstrcat(lpEnd, TEXT("A$\\"));
    }
    else {
        lstrcpy(szShareName, TEXT("a:\\"));
        lpEnd = szShareName;
    }
        

    for (tDrive = TEXT('A'); tDrive <= TEXT('Z'); tDrive++) {
        *lpEnd = tDrive;

        if ((!szComputerName) && (GetDriveType(szShareName) == DRIVE_REMOTE)) {
            DebugMsg((DM_VERBOSE, TEXT("DeleteProfile: Ignoring Drive %s because it is not local"), szShareName));            
            continue;
        }

        
        if (!GetVolumeInformation(szShareName, NULL, 0,
                                NULL, &MaxCompLen, &FileSysFlags, 
                                szFileSystemName, MAX_PATH)) 
            continue;

        if ((szFileSystemName) && (lstrcmp(szFileSystemName, TEXT("NTFS")) == 0)) {
            TCHAR szRecycleBin[MAX_PATH];
            
            lstrcpy(szRecycleBin, szShareName); 
            lstrcat(szRecycleBin, TEXT("Recycler\\"));
            
            lstrcat(szRecycleBin, lpSidString);
            
            Delnode(szRecycleBin);
            
            DebugMsg((DM_VERBOSE, TEXT("DeleteProfile: Deleting trash directory at %s"), szRecycleBin));            
        }
    }

    //
    // Queue for csc cleanup..
    //

    if (RegOpenKeyEx(hKeyLocalLM, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\NetCache"), 0, 
                     KEY_WRITE, &hKeyNetCache) == ERROR_SUCCESS) {
                     
        HKEY hKeyNextLogOff;
        
        if (RegCreateKey(hKeyNetCache, TEXT("PurgeAtNextLogoff"), &hKeyNextLogOff) == ERROR_SUCCESS) {

          if (RegSetValueEx(hKeyNextLogOff, lpSidString, 0, REG_SZ, (BYTE *)TEXT(""), sizeof(TCHAR)) == ERROR_SUCCESS) {                              

                DebugMsg((DM_VERBOSE, TEXT("DeleteProfile: Queued for csc cleanup at next logoff")));            
            }
            else {
                DebugMsg((DM_WARNING, TEXT("DeleteProfile: Could not set the Sid Value under NextLogoff key")));            
            }
            
            RegCloseKey(hKeyNextLogOff);
        }
        else {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile: Could not create the PurgeAtNextLogoff key")));            
        }

        RegCloseKey(hKeyNetCache);
    }
    else {
        DebugMsg((DM_WARNING, TEXT("DeleteProfile: Could not open the NetCache key")));            
    }


Exit:

    if (hKey) 
        RegCloseKey(hKey);

    if (bRemoteReg) {
        RegCloseKey(hKeyLocalLM);
    }

    if (bEnvVarsSet) {
        SetEnvironmentVariable(TEXT("SystemRoot"), szOrigSysRoot);
        SetEnvironmentVariable(TEXT("SystemDrive"), szOrigSysDrive);
    }

    SetLastError(dwErr);

    return bSuccess;
}


//*************************************************************
//
//  SetNtUserIniAttributes()
//
//  Purpose:    Sets system-bit on ntuser.ini
//
//  Parameters:
//
//  Return:     true if successful
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/7/99     ushaji     Created
//
//*************************************************************

BOOL SetNtUserIniAttributes(LPTSTR szDir)
{

    TCHAR szBuffer[MAX_PATH];
    HANDLE hFileNtUser;
    LPTSTR lpEnd;
    DWORD       dwWritten;
    

    lstrcpy (szBuffer, szDir);
    lpEnd = CheckSlash (szBuffer);
    lstrcpy (lpEnd, c_szNTUserIni);

    //
    // Mark the file with system bit
    //

    hFileNtUser = CreateFile(szBuffer, GENERIC_ALL, 0, NULL, CREATE_NEW, 
                           FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, NULL);
                                   

    if (INVALID_HANDLE_VALUE == hFileNtUser) 
        SetFileAttributes (szBuffer, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    else {
    
        //
        // The WritePrivateProfile* functions do not write in unicode
        // unless the file already exists in unicode format. Therefore,
        // Precreate a unicode file so that
        // the WritePrivateProfile* functions can preserve the 
        // Make sure that the ini file is unicode by writing spaces into it.
        //

        WriteFile(hFileNtUser, L"\xfeff\r\n", 3 * sizeof(WCHAR), &dwWritten, NULL);
        WriteFile(hFileNtUser, L"     \r\n", 7 * sizeof(WCHAR),
                          &dwWritten, NULL);
        CloseHandle(hFileNtUser);
    }
    
    return TRUE;
}
