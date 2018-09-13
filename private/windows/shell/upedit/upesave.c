/****************************** Module Header ******************************\
* Module Name: upesave.c
*
* Copyright (c) 1992, Microsoft Corporation
*
* Handles OPening and saving of Profiles: default, system, current and user
* profiles.
*
* History:
* 11-20-92 JohanneC       Created.
\***************************************************************************/

//#define UNICODE 1

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif
#include "upedit.h"


#define  SYSTEM_DEFAULT_SUBKEY  TEXT(".DEFAULT")
#define  TEMP_USER_SUBKEY       TEXT("TEMP_USER")
#define  TEMP_USER_HIVE_PATH    TEXT("%systemroot%\\system32\\config\\")
#define  TEMP_SAVE_HIVE         TEXT("%systemroot%\\system32\\config\\HiveSave")
#define  USERDEF_HIVE           TEXT("%systemroot%\\system32\\config\\USERDEF")
#define  USERDEF_TEMP_HIVE      TEXT("%systemroot%\\system32\\config\\USERTMP")
#define  SYSTEM_DEFAULT_HIVE    TEXT("%systemroot%\\system32\\config\\DEFAULT")
#define  SYSTEM_TEMP_HIVE       TEXT("%systemroot%\\system32\\config\\SYSTMP")

LPTSTR lpTempUserHive = NULL;
LPTSTR lpTempUserHivePath = NULL;
LPTSTR lpTempHiveKey;
extern TCHAR szDefExt[];
extern PSID gSystemSid;

BOOL AllocAndExpandEnvironmentStrings(LPTSTR String, LPTSTR *lpExpandedString);
VOID GetRegistryKeyFromPath(LPTSTR lpPath, LPTSTR *lpKey);

/***************************************************************************\
* ClearTempUserProfile
*
* Purpose : unloads the temp user profile loaded from a file, and deletes
*           the temp file
*
* History:
* 11-20-92 JohanneC       Created.
\***************************************************************************/
BOOL APIENTRY ClearTempUserProfile()
{
    BOOL bRet;

    if (hkeyCurrentUser == HKEY_CURRENT_USER)
        return(TRUE);

    //
    // Close registry keys.
    //
    if (hkeyPMSettings) {
        RegCloseKey(hkeyPMSettings);
    }
    if (hkeyPMRestrict) {
        RegCloseKey(hkeyPMRestrict);
    }
    if (hkeyProgramManager) {
        RegCloseKey(hkeyProgramManager);
    }
    if (hkeyProgramGroups) {
        RegCloseKey(hkeyProgramGroups);
    }
    if (hkeyCurrentUser) {
        RegCloseKey(hkeyCurrentUser);
    }

    hkeyCurrentUser = HKEY_CURRENT_USER;

    bRet = (RegUnLoadKey(HKEY_USERS, lpTempHiveKey) == ERROR_SUCCESS);

    if (*lpTempUserHive) {
        DeleteFile(lpTempUserHive);
        lstrcat(lpTempUserHive, TEXT(".log"));
        DeleteFile(lpTempUserHive);
        LocalFree(lpTempUserHive);
        lpTempUserHive = NULL;
    }

    return(bRet);
}

/****************************************************************************
 *
 ****************************************************************************/
VOID APIENTRY FixupNulls(LPTSTR p)
{
    while (*p != TEXT('\0')) {
        if (*p == TEXT('#'))
            *p = TEXT('\0');
#ifdef DBCS
        p = CharNext(p);
#else
        p++;
#endif
    }
}

/****************************************************************************
 *
 *   FUNCTION: GetProfileName
 *
 *   PURPOSE: Uses the OPEN or SAVE common dialog to get the profile
 *            filename to be openned or saved (depending on the flag
 *            bOpenFilename).
 *
 *
 ****************************************************************************/
BOOL APIENTRY GetProfileName(LPTSTR lpFilePath, DWORD cb, BOOL bOpenFilename)
{
    OPENFILENAME ofn;                    // Structure used to init dialog.
    TCHAR szFilters[200*sizeof(TCHAR)];  // Filters string.
    TCHAR szTitle[80*sizeof(TCHAR)];     // Title for dialog.

    /* Load filters for profile file selection. */
    LoadString(hInst, IDS_FILTERS, szFilters, sizeof(szFilters));

    /* Convert the hashes in the filter into NULLs for the dialog. */
    FixupNulls(szFilters);

    /*
     * Stomp on the file path so that the dialog doesn't
     * try to use it to initialise the dialog. The result is put
     * in here.
     */
    *lpFilePath = TEXT('\0');

    /* Setup info for comm dialog. */
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndUPE;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = szFilters;
    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex = 1;
    ofn.nMaxCustFilter = 0;
    ofn.lpstrFile = lpFilePath;
    ofn.nMaxFile =  cb;
    ofn.lpstrInitialDir = NULL;
    ofn.lpfnHook = NULL;
    ofn.lpstrDefExt = szDefExt;
    ofn.lpstrFileTitle = NULL;
    ofn.lpTemplateName = 0;

    if (bOpenFilename) {
        //
        // Open which profile ?
        //
        LoadString(hInst, IDS_OPENTITLE, szTitle, sizeof(szTitle));
        ofn.lpstrTitle = szTitle;
        ofn.Flags = OFN_SHOWHELP | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

        if (!GetOpenFileName(&ofn)) {
            return(FALSE);
        }
    }
    else {
        //
        // Save Profile in ?
        //
        LoadString(hInst, IDS_SAVEAS, szTitle, sizeof(szTitle));
        ofn.lpstrTitle = szTitle;
        ofn.Flags = OFN_SHOWHELP | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

        if (!GetSaveFileName(&ofn)) {
            return(FALSE);
        }
        lstrcpy(lpFilePath, ofn.lpstrFile);
    }

    return(TRUE);
}


BOOL GetPermittedUser(HWND hwnd, PSID *pSid)
{
    LPTSTR lpUserName = NULL;
    LPTSTR lpUserDomain = NULL;
    LPTSTR lpBuffer = NULL;
    DWORD cbAccountName = 0;
    DWORD cbUserDomain = 0;
    SID_NAME_USE SidNameUse;

    if (!*pSid) {
        GetSidFromOpenedProfile(pSid);
    }
    if (!*pSid) {
        return(FALSE);
    }
  //
  // Get the space needed for the User name and the Domain name
  //
  if (!LookupAccountSid(NULL,
                       *pSid,
                       NULL, &cbAccountName,
                       NULL, &cbUserDomain,
                       &SidNameUse
                       ) ) {
      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
          LocalFree(*pSid);
          *pSid = NULL;
          return(FALSE);
      }
  }
  lpUserName = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR) * cbAccountName);
  if (!lpUserName) {
      LocalFree(*pSid);
      *pSid = NULL;
      return(FALSE);
  }
  lpUserDomain = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR) * cbUserDomain);
  if (!lpUserDomain) {
      LocalFree(lpUserName);
      LocalFree(*pSid);
      *pSid = NULL;
      return(FALSE);
  }

  //
  // Now get the user name and domain name
  //
  if (!LookupAccountSid(NULL,
                       *pSid,
                       lpUserName, &cbAccountName,
                       lpUserDomain, &cbUserDomain,
                       &SidNameUse
                       ) ) {

      LocalFree(lpUserName);
      LocalFree(lpUserDomain);
      LocalFree(*pSid);
      *pSid = NULL;
      return(FALSE);
  }

  if (*lpUserName) {
      lpBuffer = (LPTSTR)LocalAlloc(LPTR, (lstrlen(lpUserDomain) + lstrlen(lpUserName) +
                                                     2) * sizeof(TCHAR));
      if (lpBuffer) {
          lstrcpy(lpBuffer, lpUserDomain);
          lstrcat(lpBuffer, TEXT("\\"));
          lstrcat(lpBuffer, lpUserName);
          SetDlgItemText(hwnd, IDD_USEDBY, lpBuffer);
          LocalFree(lpBuffer);
      }
  }
  LocalFree(lpUserName);
  LocalFree(lpUserDomain);
  return(TRUE);
}

/***************************************************************************\
* OpenUserProfile
*
* Purpose : Load an existing profile in the registry and unload previously
* loaded profile (and delete its tmp file).
*
* History:
* 11-20-92 JohanneC       Created.
\***************************************************************************/
BOOL APIENTRY OpenUserProfile(LPTSTR szFilePath, PSID *pUserSid)
{

    TCHAR szErrorTitle[80*sizeof(TCHAR)];     // Title for error message box.
    TCHAR szMessage[MAXMESSAGELEN*sizeof(TCHAR)];     // Title for error message box.
    DWORD err;

    // be prepared for errors.
    LoadString(hInst, IDS_OPENERROR, szErrorTitle, sizeof(szErrorTitle));

    //
    // Copy the profile to a temp hive before loading it in the registry.
    //
    if (!lpTempUserHivePath) {
        if (!AllocAndExpandEnvironmentStrings(TEMP_USER_HIVE_PATH, &lpTempUserHivePath))
            return(FALSE);
    }

    lpTempUserHive = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR) *
                              (lstrlen(lpTempUserHivePath) + 17));
    if (!lpTempUserHive) {
        return(FALSE);
    }

    if (!GetTempFileName(lpTempUserHivePath, TEXT("tmp"), 0, lpTempUserHive)) {
        lstrcpy(lpTempUserHive, lpTempUserHivePath);
        lstrcat(lpTempUserHive, TEXT("\\HiveOpen"));
    }

    if (CopyFile(szFilePath, lpTempUserHive, FALSE)) {
        GetRegistryKeyFromPath(lpTempUserHive, &lpTempHiveKey);
        if ((err = RegLoadKey(HKEY_USERS, lpTempHiveKey, lpTempUserHive)) == ERROR_SUCCESS) {
            if (RegOpenKeyEx(HKEY_USERS, lpTempHiveKey, 0,
                          MAXIMUM_ALLOWED,
                          &hkeyCurrentUser) != ERROR_SUCCESS) {
                //
                // Error, do not have access to the profile.
                //
                LoadString(hInst, IDS_OPENACCESSDENIED, szMessage, sizeof(szMessage));
                MessageBox(hwndUPE, szMessage, szErrorTitle,
                           MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
                ClearTempUserProfile();
                return(FALSE);
            }
        }
        else {
            DeleteFile(lpTempUserHive);
            lstrcat(lpTempUserHive, TEXT(".log"));
            DeleteFile(lpTempUserHive);
            LocalFree(lpTempUserHive);

            //
            // Could not load the user profile, check the error code
            //
            if (err == ERROR_BADDB) {
                // bad format: not a profile registry file
                LoadString(hInst, IDS_OPENBADFORMAT, szMessage, sizeof(szMessage));
                MessageBox(hwndUPE, szMessage, szErrorTitle,
                           MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
                return(FALSE);
            }
            else {
                // generic error message : Failed to load profile
                LoadString(hInst, IDS_OPENFAILED, szMessage, sizeof(szMessage));
                MessageBox(hwndUPE, szMessage, szErrorTitle,
                   MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
                return(FALSE);
            }
        }
    }
    else {
        //
        // An error occured trying to load the profile.
        //
        LoadString(hInst, IDS_OPENFAILED, szMessage, sizeof(szMessage));
        MessageBox(hwndUPE, szMessage, szErrorTitle,
                   MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
        return(FALSE);
    }

    //
    // Clear permitted to use profile field.
    //
    SetDlgItemText(hwndUPE, IDD_USEDBY, TEXT(""));
    *pUserSid = NULL;
    if (!GetPermittedUser(hwndUPE, pUserSid)) {
        LoadString(hInst, IDS_ACCOUNTUNKNOWN, szMessage, sizeof(szMessage));
        SetDlgItemText(hwndUPE, IDD_USEDBY, szMessage);
    }

    return(TRUE);
}

/***************************************************************************\
* SaveUserProfile
*
* Purpose : Save the loaded profile as a file.
*
* History:
* 11-20-92 JohanneC       Created.
\***************************************************************************/
BOOL APIENTRY SaveUserProfile(PSID pUserSid, LPTSTR lpFilePath)
{
    TCHAR szTitle[20*sizeof(TCHAR)];
    TCHAR szMessage[MAXMESSAGELEN*sizeof(TCHAR)];
    LPTSTR lpTmpHive = NULL;
    SID_IDENTIFIER_AUTHORITY
        NtAuthority = SECURITY_NT_AUTHORITY;
    PSECURITY_DESCRIPTOR lpSecDesc;
    LPSECURITY_ATTRIBUTES lpSecAttr;
    SECURITY_ATTRIBUTES SecAttr;
    NTSTATUS NtStatus;
    BOOL err = FALSE;


    ResetEvent(hProgramGroupsEvent);

    /* Write the selected settings in the registry. */
    if (!SaveUPESettingsToRegistry())
        return(FALSE);

    RegFlushKey(hkeyCurrentUser);

    RegNotifyChangeKeyValue(hkeyProgramGroupsCurrent, TRUE, REG_NOTIFY_CHANGE_NAME,
                                 hProgramGroupsEvent, TRUE);

    LoadString(hInst, IDS_SAVEAS, szTitle, sizeof(szTitle));

    //
    // Before the profile we need to apply protection.
    //
    NtStatus = ApplyProfileProtection(NULL, pUserSid, hkeyCurrentUser);
    if (NtStatus) {
        if (NtStatus == STATUS_INVALID_SID) {
            LoadString(hInst, IDS_INVALIDSID, szMessage, sizeof(szMessage));
        } else {
            LoadString(hInst, IDS_PROTECTERROR, szMessage, sizeof(szMessage));
        }
        MessageBox(hwndUPE, szMessage, szTitle, MB_ICONEXCLAMATION | MB_OK);
        return(FALSE);
    }

    //
    // Now that the protection is on the profile, restrict the special keys
    // like the locked groups and the restriction subkey for the Program Manager.
    //
    if (!LockGroups(FALSE)) {
        goto Error;
    }

    lpSecDesc = CreateSecurityDescriptorForFile(pUserSid, lpFilePath);
    if (lpSecDesc) {
        SecAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        SecAttr.lpSecurityDescriptor = lpSecDesc;
        SecAttr.bInheritHandle = TRUE;
        lpSecAttr = &SecAttr;
    }
    else {
        lpSecAttr = NULL;
    }


    if (RegSaveKey(hkeyCurrentUser, lpFilePath, lpSecAttr) != ERROR_SUCCESS) {

        //
        // Save the profile to a temp hive then copy it over.
        //

        if (!AllocAndExpandEnvironmentStrings(TEMP_SAVE_HIVE, &lpTmpHive))
            goto Error;

        DeleteFile(lpTmpHive);

        if (RegSaveKey(hkeyCurrentUser, lpTmpHive, NULL) != ERROR_SUCCESS) {
            LocalFree(lpTmpHive);
            lpTmpHive = NULL;
Error:
            LoadString(hInst, IDS_ERRORSAVING, szMessage, sizeof(szMessage));
            MessageBox(hwndUPE, szMessage, szTitle, MB_ICONEXCLAMATION | MB_OK);
            err = TRUE;
        }
        else {
            if (CopyFile(lpTmpHive, lpFilePath, FALSE)) {
                DeleteFile(lpTmpHive);
                LocalFree(lpTmpHive);
                lpTmpHive = NULL;
                if (lpSecDesc)
                    SetFileSecurity(lpFilePath,
                                    DACL_SECURITY_INFORMATION,
                                    lpSecDesc);
            }
            else {
              //  DbgPrint("UPEDIT: could not overwrite file %s, error = %d\n\r",
              //              lpFilePath, GetLastError());
                goto Error;
            }
        }
    }
    if (lpTmpHive)
        LocalFree(lpTmpHive);

    if (lpSecDesc)
        DeleteSecurityDescriptor(lpSecDesc);

    return(!err);

}


/***************************************************************************\
* SaveCurrentProfile
*
* Purpose : Save the current profile.
*
* History:
* 11-20-92 JohanneC       Created.
\***************************************************************************/
BOOL APIENTRY SaveCurrentProfile(PSECURITY_DESCRIPTOR pSecDesc, PSID CurrentUserSid)
{
    TCHAR szTitle[MAXTITLELEN * sizeof(TCHAR)];
    TCHAR szMessage[MAXMESSAGELEN * sizeof(TCHAR)];
    NTSTATUS NtStatus;
    DWORD err;

    LoadString(hInst, IDS_SAVECURRENT, szTitle, sizeof(szTitle));

    /* Write the selected settings in the registry. */
    if (!SaveUPESettingsToRegistry()) {
        return(FALSE);
    }


    NtStatus = ApplyProfileProtection( pSecDesc, CurrentUserSid, HKEY_CURRENT_USER );

    if (!NT_SUCCESS(NtStatus)) {
        //
        // Failed to apply protection on profile
        //
        LoadString(hInst, IDS_PROTECTERROR, szMessage, sizeof(szMessage));
        MessageBox(hwndUPE, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);
        return(FALSE);
    }

    if (!LockGroups(FALSE))
        return(FALSE);

    err = RegFlushKey(HKEY_CURRENT_USER);
    if (err != ERROR_SUCCESS) {
        LoadString(hInst, IDS_SAVECURFAILED, szMessage, sizeof(szMessage));
        MessageBox(hwndUPE, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);
        return(FALSE);
    }

    InitializeUPESettings(hwndUPE, TRUE);

    return(TRUE);
}

/***************************************************************************\
* SaveDefaultProfile
*
* Purpose : Save the current profile as the default profile.
*
* History:
* 11-20-92 JohanneC       Created.
\***************************************************************************/
BOOL APIENTRY SaveDefaultProfile()
{
    TCHAR szTitle[MAXTITLELEN * sizeof(TCHAR)];
    TCHAR szMessage[MAXMESSAGELEN * sizeof(TCHAR)];
    LPTSTR lpUserDefTmpHive = NULL;
    LPTSTR lpUserDefPath = NULL;
    NTSTATUS NtStatus;
    DWORD err;

    LoadString(hInst, IDS_SAVEDEFAULT, szTitle, sizeof(szTitle));

    /* Write the selected settings in the registry. */
    if (!SaveUPESettingsToRegistry()) {
        return(FALSE);
    }

    NtStatus = ApplyProfileProtection( NULL, gSystemSid, hkeyCurrentUser );

    if (!NT_SUCCESS(NtStatus)) {
        //
        // Failed to apply protection on profile
        //
        LoadString(hInst, IDS_PROTECTERROR, szMessage, sizeof(szMessage));
        MessageBox(hwndUPE, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);
        return(FALSE);
    }

    if (!LockGroups(FALSE)) {
        return(FALSE);
    }

    //
    // Save the User Default profile.
    //
    if (!AllocAndExpandEnvironmentStrings(USERDEF_TEMP_HIVE, &lpUserDefTmpHive))
        goto Error;

    DeleteFile(lpUserDefTmpHive);
    err = RegSaveKey(hkeyCurrentUser, lpUserDefTmpHive, NULL);
    if (!err) {
        //
        // copy the user default to %systemroot%\system32\config\USERDEF
        //
        if (!AllocAndExpandEnvironmentStrings(USERDEF_HIVE, &lpUserDefPath))
            goto Error;

        if (!CopyFile(lpUserDefTmpHive, lpUserDefPath, FALSE)) {
            goto Error;
        }

        // delete temp file
        DeleteFile(lpUserDefTmpHive);

        LocalFree(lpUserDefTmpHive);
        LocalFree(lpUserDefPath);

        return(TRUE);

    }
    else {

Error:
        if (lpUserDefTmpHive) {
            DeleteFile(lpUserDefTmpHive);
            LocalFree(lpUserDefTmpHive);
        }
        if (lpUserDefPath) {
            LocalFree(lpUserDefPath);
        }
        LoadString(hInst, IDS_SAVEDEFFAILED, szMessage, sizeof(szMessage));
        MessageBox(hwndUPE, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);

        return(FALSE);

    }

}

/***************************************************************************\
* SaveSystemProfile
*
* Purpose : Save the current profile as the system profile.
*
* History:
* 11-20-92 JohanneC       Created.
\***************************************************************************/
BOOL APIENTRY SaveSystemProfile()
{
    TCHAR szTitle[MAXTITLELEN * sizeof(TCHAR)];
    TCHAR szMessage[MAXMESSAGELEN * sizeof(TCHAR)];
    LPTSTR lpSystemTmpHive = NULL;
    LPTSTR lpSystemDefPath = NULL;
    LPTSTR lpSystemDefPathLog = NULL;
    NTSTATUS NtStatus;
    DWORD err;

    LoadString(hInst, IDS_SAVESYSTEM, szTitle, sizeof(szTitle));

    /* Write the selected settings in the registry. */
    if (!SaveUPESettingsToRegistry()) {
        return(FALSE);
    }

    NtStatus = ApplyProfileProtection( NULL, gSystemSid, hkeyCurrentUser );

    if (!NT_SUCCESS(NtStatus)) {
        //
        // Failed to apply protection on profile
        //
        LoadString(hInst, IDS_PROTECTERROR, szMessage, sizeof(szMessage));
        MessageBox(hwndUPE, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);
        return(FALSE);
    }

    //
    // Save the System profile.
    //
    if (!AllocAndExpandEnvironmentStrings(SYSTEM_TEMP_HIVE, &lpSystemTmpHive))
        goto Error;

    DeleteFile(lpSystemTmpHive);
    if (err = RegSaveKey(hkeyCurrentUser, lpSystemTmpHive, NULL))
        goto Error;

    //
    // Move the hive to the system default hive on reboot.
    //
    AllocAndExpandEnvironmentStrings(SYSTEM_DEFAULT_HIVE, &lpSystemDefPath);
    if (!lpSystemDefPath) {
        goto Error;
    }

    if (!MoveFileEx(lpSystemTmpHive, lpSystemDefPath,
                     MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING)) {
        err = GetLastError();
    }
    else {
        lpSystemDefPathLog = LocalAlloc (LPTR, (lstrlen(lpSystemDefPath) + 5) *
                                         sizeof (TCHAR));

        if (lpSystemDefPathLog) {
            lstrcpy(lpSystemDefPathLog, lpSystemDefPath);
            lstrcat(lpSystemDefPathLog, TEXT(".log"));
            if (!MoveFileEx(lpSystemDefPathLog, NULL,
                         MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING))
                err = GetLastError();
        } else {
            err = GetLastError();
        }
    }


    if (err) {
Error:
        LoadString(hInst, IDS_SAVESYSFAILED, szMessage, sizeof(szMessage));
        MessageBox(hwndUPE, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);
    }
    else {

        //
        // The system default will only be changed at next reboot.
        //
        LoadString(hInst, IDS_NEWSYSTEMDEF, szMessage, sizeof(szMessage));
        MessageBox(hwndUPE, szMessage, szTitle, MB_OK | MB_ICONINFORMATION);
    }

    if (lpSystemDefPath)
        LocalFree(lpSystemDefPath);
    if (lpSystemTmpHive)
        LocalFree(lpSystemTmpHive);

    return(!err);
}

BOOL AllocAndExpandEnvironmentStrings(LPTSTR String, LPTSTR *lpExpandedString)
{
     LPTSTR lptmp = NULL;
     DWORD cchBuffer;

     // Get the number of characters needed.
     cchBuffer = ExpandEnvironmentStrings(String, lptmp, 0);
     if (cchBuffer) {
         cchBuffer++;  // for NULL terminator
         lptmp = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR) * cchBuffer);
         if (!lptmp) {
             return(FALSE);
         }
         cchBuffer = ExpandEnvironmentStrings(String, lptmp, cchBuffer);
     }
     *lpExpandedString = lptmp;
     return(TRUE);
}

VOID GetRegistryKeyFromPath(LPTSTR lpPath, LPTSTR *lpKey)
{
    LPTSTR lptmp;

    *lpKey = lpPath;

    for (lptmp = lpPath; *lptmp; lptmp++) {
        if (*lptmp == TEXT('\\')) {
            *lpKey = lptmp+1;
        }
    }

}
