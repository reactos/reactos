//*************************************************************
//  File name: profile.c
//
//  Description:   Fixes hard coded paths in the registry for
//                 special folder locations.  Also fixes security
//                 on a few registry keys.
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1996
//  All rights reserved
//
//*************************************************************

#include <windows.h>
#include <shlobj.h>
#include <tchar.h>
#include "shmgdefs.h"

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
//              6/16/96     bobday     Stolen directly from USERENV
//
//*************************************************************

DWORD ApplySecurityToRegistryTree(HKEY RootKey, PSECURITY_DESCRIPTOR pSD)

{
    DWORD Error;
    DWORD SubKeyIndex;
    LPTSTR SubKeyName;
    HKEY SubKey;
    DWORD cchSubKeySize = MAX_PATH + 1;



    //
    // First apply security
    //

    Error = RegSetKeySecurity(RootKey, DACL_SECURITY_INFORMATION, pSD);

    if (Error != ERROR_SUCCESS) {
        return Error;
    }


    //
    // Open each sub-key and apply security to its sub-tree
    //

    SubKeyIndex = 0;

    SubKeyName = GlobalAlloc (GPTR, cchSubKeySize * sizeof(TCHAR));

    if (!SubKeyName) {
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

        if (Error != ERROR_SUCCESS) {
            break;
        }

        //
        // Apply security to the sub-tree
        //

        Error = ApplySecurityToRegistryTree(SubKey, pSD);


        //
        // We're finished with the sub-key
        //

        RegCloseKey(SubKey);

        //
        // See if we set the security on the sub-tree successfully.
        //

        if (Error != ERROR_SUCCESS) {
            break;
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
//  MakeKeyOrTreeSecure()
//
//  Purpose:    Sets the attributes on the registry key and possibly sub-keys
//              such that Administrators and the OS can delete it and Everyone
//              else has read permission only (OR general read/write access)
//
//  Parameters: RootKey -   Key to set security on
//              fWrite  -   Allow write (or just read)
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/6/95     ericflo    Created
//              06/16/96    bobday     Ported from MakeFileSecure in USERENV
//
//*************************************************************

BOOL MakeKeyOrTreeSecure (HKEY RootKey, BOOL fWrite)
{
    SECURITY_DESCRIPTOR sd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY authWorld = SECURITY_WORLD_SID_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidSystem = NULL, psidAdmin = NULL, psidEveryone = NULL;
    DWORD cbAcl, aceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    DWORD Error;
    DWORD dwAccess;



    if (fWrite) {
        dwAccess = KEY_ALL_ACCESS;
    } else {
        dwAccess = KEY_READ;
    }

    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         goto Exit;
    }


    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         goto Exit;
    }


    //
    // Get the World sid
    //

    if (!AllocateAndInitializeSid(&authWorld, 1, SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidEveryone)) {

         goto Exit;
    }


    //
    // Allocate space for the ACL
    //

    cbAcl = (3 * GetLengthSid (psidSystem)) +
            (3 * GetLengthSid (psidAdmin)) + sizeof(ACL) +
            (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        goto Exit;
    }



    //
    // Add Aces.  Non-inheritable ACEs first
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem)) {
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin)) {
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, dwAccess, psidEveryone)) {
        goto Exit;
    }



    //
    // Now the inheritable ACEs
    //
    if (fWrite) {
        dwAccess = GENERIC_ALL;
    } else {
        dwAccess = GENERIC_READ;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, dwAccess, psidEveryone)) {
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        goto Exit;
    }


    //
    // Set the security
    //
    Error = ApplySecurityToRegistryTree(RootKey, &sd);

    if (Error == ERROR_SUCCESS) {
        bRetVal = TRUE;
    }


Exit:

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }


    if (psidEveryone) {
        FreeSid(psidEveryone);
    }


    if (pAcl) {
        GlobalFree (pAcl);
    }

    return bRetVal;
}

void FixWindowsProfileSecurity( void )
{
    HKEY    hkeyWindows;
    HKEY    hkeyShellExt;
    DWORD   Error;

    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          TEXT("Software\\Microsoft\\Windows"),
                          0,
                          WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                          &hkeyWindows);

    if (Error == ERROR_SUCCESS)
    {
        MakeKeyOrTreeSecure(hkeyWindows, TRUE);
        RegCloseKey(hkeyWindows);
    }

    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions"),
                          0,
                          WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                          &hkeyShellExt);
    if (Error == ERROR_SUCCESS)
    {
        MakeKeyOrTreeSecure(hkeyShellExt, FALSE);
        RegCloseKey(hkeyShellExt);
    }
}

void FixUserProfileSecurity( void )
{
    HKEY    hkeyPolicies;
    DWORD   Error;

    Error = RegOpenKeyEx( HKEY_CURRENT_USER,
                          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies"),
                          0,
                          WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                          &hkeyPolicies);

    if (Error == ERROR_SUCCESS)
    {
        MakeKeyOrTreeSecure(hkeyPolicies, FALSE);
        RegCloseKey(hkeyPolicies);
    }
}

void FixPoliciesSecurity( void )
{
    HKEY    hkeyPolicies;
    DWORD   Error, dwDisp;

    Error = RegCreateKeyEx( HKEY_CURRENT_USER,
                            TEXT("Software\\Policies"),
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                            NULL,
                            &hkeyPolicies,
                            &dwDisp);

    if (Error == ERROR_SUCCESS)
    {
        MakeKeyOrTreeSecure(hkeyPolicies, FALSE);
        RegCloseKey(hkeyPolicies);
    }
}

void SetSystemBitOnCAPIDir(void)
{
    HRESULT hr;
    TCHAR szPath[MAX_PATH];
    TCHAR szAppData[MAX_PATH];
    DWORD FileAttributes;


    if (S_OK == SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, szAppData)){

        //
        // It is better to use tcscpy and tcscat. This is just a temp fix and it is build with
        // Unicode anyway. Not worth to include extra header file.
        // We do not check error case here. This is just a best effort. If we got error, so what?
        // MAX_PATH should be enough for these DIRs. Not worth to take care of \\?\ format.
        //

        wcscpy(szPath, szAppData);
        wcscat(szPath, TEXT("\\Microsoft\\Protect"));
        FileAttributes = GetFileAttributes(szPath);
        if ((FileAttributes != -1) && ((FileAttributes & FILE_ATTRIBUTE_SYSTEM) == 0)) {
            FileAttributes |= FILE_ATTRIBUTE_SYSTEM;
            SetFileAttributes(szPath, FileAttributes);
        }

        wcscpy(szPath, szAppData);
        wcscat(szPath, TEXT("\\Microsoft\\Crypto"));
        FileAttributes = GetFileAttributes(szPath);
        if ((FileAttributes != -1) && ((FileAttributes & FILE_ATTRIBUTE_SYSTEM) == 0)) {
            FileAttributes |= FILE_ATTRIBUTE_SYSTEM;
            SetFileAttributes(szPath, FileAttributes);
        }
    }

}

#define OTHERSIDS_EVERYONE             1
#define OTHERSIDS_POWERUSERS           2

BOOL MakeFileSecure (LPTSTR lpFile, DWORD dwOtherSids)
{
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY authWORLD = SECURITY_WORLD_SID_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidSystem = NULL, psidAdmin = NULL, psidUsers = NULL, psidPowerUsers = NULL;
    PSID  psidEveryOne = NULL;
    DWORD cbAcl, aceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bAddPowerUsersAce=TRUE;
    BOOL bAddEveryOneAce=FALSE;
    DWORD dwAccMask;


    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         goto Exit;
    }


    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         goto Exit;
    }


    //
    // Get the users sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_USERS,
                                  0, 0, 0, 0, 0, 0, &psidUsers)) {

         goto Exit;
    }


    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin))  +
            (2 * GetLengthSid (psidUsers))  +
            sizeof(ACL) +
            (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));

    //
    // Get the power users sid, if required.
    // Don't fail if you don't get because it might not be available on DCs??
    //

    bAddPowerUsersAce = TRUE;
    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_POWER_USERS, 0, 0, 0, 0, 0, 0, &psidPowerUsers)) {

        bAddPowerUsersAce = FALSE;
    }

    if (bAddPowerUsersAce)
        cbAcl += (2 * GetLengthSid (psidPowerUsers)) + (2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));

    //
    // Get the EveryOne sid, if required.
    //

    if (dwOtherSids & OTHERSIDS_EVERYONE) {
        bAddEveryOneAce = TRUE;
        if (!AllocateAndInitializeSid(&authWORLD, 1, SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidEveryOne)) {

            goto Exit;
        }
    }

    if (bAddEveryOneAce)
        cbAcl += (2 * GetLengthSid (psidEveryOne)) + (2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        goto Exit;
    }


    //
    // Add Aces.  Non-inheritable ACEs first
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidSystem)) {
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidAdmin)) {
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, /*GENERIC_READ | GENERIC_EXECUTE,*/ psidUsers)) {
        goto Exit;
    }


    if (bAddPowerUsersAce) {

        //
        // By default give read permissions, otherwise give modify permissions
        //

        dwAccMask = (dwOtherSids & OTHERSIDS_POWERUSERS) ? (FILE_ALL_ACCESS ^ (WRITE_DAC | WRITE_OWNER)):
                                                           (GENERIC_READ | GENERIC_EXECUTE);

        aceIndex++;
        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, /*dwAccMask,*/ psidPowerUsers)) {
            goto Exit;
        }
    }

    if (bAddEveryOneAce) {
        aceIndex++;
        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, /*GENERIC_READ | GENERIC_EXECUTE,*/ psidEveryOne)) {
            goto Exit;
        }
    }

    //
    // Now the inheritable ACEs
    //

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, /*GENERIC_READ | GENERIC_EXECUTE,*/ psidUsers)) {
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    if (bAddPowerUsersAce) {
        aceIndex++;
        dwAccMask = (dwOtherSids & OTHERSIDS_POWERUSERS) ? (FILE_ALL_ACCESS ^ (WRITE_DAC | WRITE_OWNER)):
                                                           (GENERIC_READ | GENERIC_EXECUTE);

        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, /*dwAccMask,*/ psidPowerUsers)) {
            goto Exit;
        }

        if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
            goto Exit;
        }

        lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    }

    if (bAddEveryOneAce) {
        aceIndex++;

        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, /*GENERIC_READ | GENERIC_EXECUTE,*/ psidEveryOne)) {
            goto Exit;
        }

        if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
            goto Exit;
        }

        lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    }

    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        goto Exit;
    }


    //
    // Set the security
    //

    if (SetFileSecurity (lpFile, DACL_SECURITY_INFORMATION, &sd)) {
        bRetVal = TRUE;
    } else {
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

    if ((bAddPowerUsersAce) && (psidPowerUsers)) {
        FreeSid(psidPowerUsers);
    }

    if ((bAddEveryOneAce) && (psidEveryOne)) {
        FreeSid(psidEveryOne);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }

    return bRetVal;
}


#ifdef SHMG_DBG

void SHMGLogErrMsg(char *szErrMsg, DWORD dwError)
{
    DWORD   dwBytesWritten = 0;
    char    szMsg[256];
    static HANDLE  hLogFile = 0;

    if (!hLogFile) {
        hLogFile = CreateFile(_T("shmgrate.log"), GENERIC_WRITE,
                                FILE_SHARE_WRITE, 0,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0 );
    }                                
                                                                   
    sprintf(szMsg, "%s : (%X)\r\n", szErrMsg, dwError);
    WriteFile(hLogFile, szMsg, strlen(szMsg), &dwBytesWritten, 0);
}            

#endif

void
FixHtmlHelp(
    void
    )
{
    TCHAR AppDataPath[MAX_PATH*2];
    TCHAR HtmlHelpPath[MAX_PATH];

    SHMGLogErrMsg("FixHtmlHelp Called",0);
    
    if (SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, AppDataPath) == S_OK &&
        LoadString( GetModuleHandle(NULL), IDS_HTML_HELP_DIR, HtmlHelpPath, sizeof(HtmlHelpPath)/sizeof(WCHAR) ) > 0)
    {
        _tcscat( AppDataPath, HtmlHelpPath );
                        
        if (CreateDirectory( AppDataPath, NULL )) {
            if (!MakeFileSecure(AppDataPath,OTHERSIDS_EVERYONE|OTHERSIDS_POWERUSERS)) 
                SHMGLogErrMsg("Could not apply security attributes", 0);
        }        
        else
            SHMGLogErrMsg("Could not create the directory", GetLastError());
    } 
    else
       SHMGLogErrMsg("Could not get APPDATA path", GetLastError());
}
