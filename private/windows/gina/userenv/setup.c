//*************************************************************
//
//  SETUP.C  -    API's used by setup to create groups/items
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include <uenv.h>


BOOL PrependPath(LPCTSTR szFile, LPTSTR szResult);
BOOL CheckProfile (LPTSTR lpProfilesDir, LPTSTR lpProfileValue,
                   LPTSTR lpProfileName);

//*************************************************************
//
//  CreateGroup()
//
//  Purpose:    Creates a program group (sub-directory)
//
//  Parameters: lpGroupName     -   Name of group
//              bCommonGroup    -   Common or Personal group
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              8/08/95     ericflo    Created
//
//*************************************************************

BOOL WINAPI CreateGroup(LPCTSTR lpGroupName, BOOL bCommonGroup)
{
    TCHAR      szDirectory[MAX_PATH];
    LPTSTR     lpEnd;
    LPTSTR     lpAdjustedGroupName;
    int        csidl;
    PSHELL32_API    pShell32Api;
    
    //
    // Validate parameters
    //

    if (!lpGroupName || !(*lpGroupName)) {
        DebugMsg((DM_WARNING, TEXT("CreateGroup:  Failing due to NULL group name.")));
        return FALSE;
    }

    DebugMsg((DM_VERBOSE, TEXT("CreateGroup:  Entering with <%s>."), lpGroupName));


    pShell32Api = LoadShell32Api();

    if ( !pShell32Api ) {
        return FALSE;
    }
    
    //
    // Extract the CSIDL (if any) from lpGroupName
    //

    csidl = ExtractCSIDL(lpGroupName, &lpAdjustedGroupName);

    if (-1 != csidl)
    {
        //
        // Use this csidl
        // WARNING: if a CSIDL is provided, the bCommonGroup flag is meaningless
        //

        DebugMsg((DM_VERBOSE,
            TEXT("CreateGroup:  CSIDL = <0x%x> contained in lpGroupName replaces csidl."),
            csidl));
    }
    else
    {
        //
        // Default to CSIDL_..._PROGRAMS
        //
        csidl = bCommonGroup ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS;
    }

    //
    // Get the programs directory
    //

    if (!GetSpecialFolderPath (csidl, szDirectory)) {
        return FALSE;
    }


    //
    // Now append the requested directory
    //

    lpEnd = CheckSlash (szDirectory);
    lstrcpy (lpEnd, lpAdjustedGroupName);


    //
    // Create the group (directory)
    //
    DebugMsg((DM_VERBOSE, TEXT("CreateGroup:  Calling CreatedNestedDirectory with <%s>"),
        szDirectory));

    if (!CreateNestedDirectory(szDirectory, NULL)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateGroup:  CreatedNestedDirectory failed.")));
        return FALSE;
    }


    //
    // Success
    //

    pShell32Api->pfnShChangeNotify (SHCNE_MKDIR, SHCNF_PATH, szDirectory, NULL);

    DebugMsg((DM_VERBOSE, TEXT("CreateGroup:  Leaving successfully.")));

    return TRUE;
}


//*************************************************************
//
//  DeleteGroup()
//
//  Purpose:    Deletes a program group (sub-directory)
//
//  Parameters: lpGroupName     -   Name of group
//              bCommonGroup    -   Common or Personal group
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              8/10/95     ericflo    Created
//
//*************************************************************

BOOL WINAPI DeleteGroup(LPCTSTR lpGroupName, BOOL bCommonGroup)
{
    TCHAR     szDirectory[MAX_PATH];
    LPTSTR    lpEnd;
    LPTSTR    lpAdjustedGroupName;
    int       csidl;
    PSHELL32_API pShell32Api;

    //
    // Validate parameters
    //

    if (!lpGroupName || !(*lpGroupName)) {
        DebugMsg((DM_WARNING, TEXT("DeleteGroup:  Failing due to NULL group name.")));
        return FALSE;
    }

    DebugMsg((DM_VERBOSE, TEXT("DeleteGroup:  Entering with <%s>."), lpGroupName));

    pShell32Api = LoadShell32Api();

    if ( !pShell32Api ) {
        return FALSE;
    }

    //
    // Extract the CSIDL (if any) from lpGroupName
    //

    csidl = ExtractCSIDL(lpGroupName, &lpAdjustedGroupName);

    if (-1 != csidl)
    {
        //
        // Use this csidl
        // WARNING: if a CSIDL is provided, the bCommonGroup flag is meaningless
        //

        DebugMsg((DM_VERBOSE,
            TEXT("CreateGroup:  CSIDL = <0x%x> contained in lpGroupName replaces csidl."),
            csidl));
    }
    else
    {
        //
        // Default to CSIDL_..._PROGRAMS
        //

        csidl = bCommonGroup ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS;
    }

    //
    // Get the programs directory
    //

    if (!GetSpecialFolderPath (csidl, szDirectory)) {
        return FALSE;
    }


    //
    // Now append the requested directory
    //

    lpEnd = CheckSlash (szDirectory);
    lstrcpy (lpEnd, lpAdjustedGroupName);


    //
    // Delete the group (directory)
    //

    if (!Delnode(szDirectory)) {
        DebugMsg((DM_VERBOSE, TEXT("DeleteGroup:  Delnode failed.")));
        return FALSE;
    }


    //
    // Success
    //
    pShell32Api->pfnShChangeNotify (SHCNE_RMDIR, SHCNF_PATH, szDirectory, NULL);

    DebugMsg((DM_VERBOSE, TEXT("DeleteGroup:  Leaving successfully.")));

    return TRUE;
}

//*************************************************************
//
//  CreateLinkFile()
//
//  Purpose:    Creates a link file in the specified directory
//
//  Parameters: cidl            -   CSIDL_ of a special folder
//              lpSubDirectory  -   Subdirectory of special folder
//              lpFileName      -   File name of item
//              lpCommandLine   -   Command line (including args)
//              lpIconPath      -   Icon path (can be NULL)
//              iIconIndex      -   Index of icon in icon path
//              lpWorkingDir    -   Working directory
//              wHotKey         -   Hot key
//              iShowCmd        -   ShowWindow flag
//              lpDescription   -   Description of the item
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              3/26/98     ericflo    Created
//
//*************************************************************

BOOL WINAPI CreateLinkFile(INT     csidl,              LPCTSTR lpSubDirectory,
                           LPCTSTR lpFileName,         LPCTSTR lpCommandLine,
                           LPCTSTR lpIconPath,         int iIconIndex,
                           LPCTSTR lpWorkingDirectory, WORD wHotKey,
                           int     iShowCmd,           LPCTSTR lpDescription)
{
    TCHAR                szItem[MAX_PATH];
    TCHAR                szArgs[MAX_PATH];
    TCHAR                szLinkName[MAX_PATH];
    TCHAR                szPath[MAX_PATH];
    LPTSTR               lpArgs, lpEnd;
    IShellLink          *psl;
    IPersistFile        *ppf;
    BOOL                 bRetVal = FALSE;
    HINSTANCE            hInstOLE32 = NULL;
    PFNCOCREATEINSTANCE  pfnCoCreateInstance;
    PFNCOINITIALIZE      pfnCoInitialize;
    PFNCOUNINITIALIZE    pfnCoUninitialize;
    LPTSTR               lpAdjustedSubDir = NULL;
    PSHELL32_API         pShell32Api;
    PSHLWAPI_API         pShlwapiApi;
    
    //
    // Verbose output
    //

#if DBG
    DebugMsg((DM_VERBOSE, TEXT("CreateLinkFile:  Entering.")));
    DebugMsg((DM_VERBOSE, TEXT("CreateLinkFile:  csidl = <0x%x>."), csidl));
    if (lpSubDirectory && *lpSubDirectory) {
        DebugMsg((DM_VERBOSE, TEXT("CreateLinkFile:  lpSubDirectory = <%s>."), lpSubDirectory));
    }
    DebugMsg((DM_VERBOSE, TEXT("CreateLinkFile:  lpFileName = <%s>."), lpFileName));
    DebugMsg((DM_VERBOSE, TEXT("CreateLinkFile:  lpCommandLine = <%s>."), lpCommandLine));

    if (lpIconPath) {
       DebugMsg((DM_VERBOSE, TEXT("CreateLinkFile:  lpIconPath = <%s>."), lpIconPath));
       DebugMsg((DM_VERBOSE, TEXT("CreateLinkFile:  iIconIndex = <%d>."), iIconIndex));
    }

    if (lpWorkingDirectory) {
        DebugMsg((DM_VERBOSE, TEXT("CreateLinkFile:  lpWorkingDirectory = <%s>."), lpWorkingDirectory));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("CreateLinkFile:  Null working directory.  Setting to %%HOMEDRIVE%%%%HOMEPATH%%")));
    }

    DebugMsg((DM_VERBOSE, TEXT("CreateLinkFile:  wHotKey = <%d>."), wHotKey));
    DebugMsg((DM_VERBOSE, TEXT("CreateLinkFile:  iShowCmd = <%d>."), iShowCmd));

    if (lpDescription) {
        DebugMsg((DM_VERBOSE, TEXT("CreateLinkFile:  lpDescription = <%s>."), lpDescription));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("CreateLinkFile:  Null description.")));
    }
#endif

    //
    // Load a few functions we need
    //

    hInstOLE32 = LoadLibrary (TEXT("ole32.dll"));

    if (!hInstOLE32) {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFile:  Failed to load ole32 with %d."),
                 GetLastError()));
        goto ExitNoFree;
    }


    pfnCoCreateInstance = (PFNCOCREATEINSTANCE)GetProcAddress (hInstOLE32,
                                        "CoCreateInstance");

    if (!pfnCoCreateInstance) {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFile:  Failed to find CoCreateInstance with %d."),
                 GetLastError()));
        goto ExitNoFree;
    }

    pfnCoInitialize = (PFNCOINITIALIZE)GetProcAddress (hInstOLE32,
                                                       "CoInitialize");

    if (!pfnCoInitialize) {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFile:  Failed to find CoInitialize with %d."),
                 GetLastError()));
        goto ExitNoFree;
    }

    pfnCoUninitialize = (PFNCOUNINITIALIZE)GetProcAddress (hInstOLE32,
                                                          "CoUninitialize");

    if (!pfnCoUninitialize) {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFile:  Failed to find CoUninitialize with %d."),
                 GetLastError()));
        goto ExitNoFree;
    }

    pShell32Api = LoadShell32Api();

    if ( !pShell32Api ) {
        goto ExitNoFree;
    }


    pShlwapiApi = LoadShlwapiApi();

    if ( !pShlwapiApi ) {
        goto ExitNoFree;
    }

    //
    // Get the special folder directory
    // First check if there is a CSIDL in the subdirectory
    //

    if (lpSubDirectory && *lpSubDirectory) {

        int csidl2 = ExtractCSIDL(lpSubDirectory, &lpAdjustedSubDir);

        if (-1 != csidl2)
        {
            csidl = csidl2;
            DebugMsg((DM_VERBOSE,
                TEXT("CreateLinkFile:  CSIDL = <0x%x> contained in lpSubDirectory replaces csidl."),
                csidl));
        }
    }

    szLinkName[0] = TEXT('\0');
    if (csidl && !GetSpecialFolderPath (csidl, szLinkName)) {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFile:  Failed to get profiles directory.")));
        goto ExitNoFree;
    }


    if (lpAdjustedSubDir && *lpAdjustedSubDir) {

        if (szLinkName[0] != TEXT('\0')) {
            lpEnd = CheckSlash (szLinkName);

        } else {
            lpEnd = szLinkName;
        }

        lstrcpy (lpEnd, lpAdjustedSubDir);
    }


    //
    // Create the target directory
    //

    if (!CreateNestedDirectory(szLinkName, NULL)) {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFile:  Failed to create subdirectory <%s> with %d"),
                 szLinkName, GetLastError()));
        goto ExitNoFree;
    }


    //
    // Now tack on the filename and extension.
    //

    lpEnd = CheckSlash (szLinkName);
    lstrcpy (lpEnd, lpFileName);
    lstrcat (lpEnd, c_szLNK);


    //
    // Split the command line into the executable name
    // and arguments.
    //

    lstrcpy (szItem, lpCommandLine);

    lpArgs = pShlwapiApi->pfnPathGetArgs(szItem);

    if (*lpArgs) {
        lstrcpy (szArgs, lpArgs);

        lpArgs--;
        while (*lpArgs == TEXT(' ')) {
            lpArgs--;
        }
        lpArgs++;
        *lpArgs = TEXT('\0');
    } else {
        szArgs[0] = TEXT('\0');
    }

    pShlwapiApi->pfnPathUnquoteSpaces (szItem);


    //
    // Create an IShellLink object
    //

    pfnCoInitialize(NULL);

    if (FAILED(pfnCoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                              &IID_IShellLink, (LPVOID*)&psl)))
    {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFile:  Could not create instance of IShellLink .")));
        goto ExitNoFree;
    }


    //
    // Query for IPersistFile
    //

    if (FAILED(psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf)))
    {
        DebugMsg((DM_WARNING, TEXT("CreateLinkFile:  QueryInterface of IShellLink failed.")));
        goto ExitFreePSL;
    }

    //
    // Set the item information
    //

    if (lpDescription) {
        psl->lpVtbl->SetDescription(psl, lpDescription);
    }

    PrependPath(szItem, szPath);
    psl->lpVtbl->SetPath(psl, szPath);


    psl->lpVtbl->SetArguments(psl, szArgs);
    if (lpWorkingDirectory) {
        psl->lpVtbl->SetWorkingDirectory(psl, lpWorkingDirectory);
    } else {
        psl->lpVtbl->SetWorkingDirectory(psl, TEXT("%HOMEDRIVE%%HOMEPATH%"));
    }

    PrependPath(lpIconPath, szPath);
    psl->lpVtbl->SetIconLocation(psl, szPath, iIconIndex);

    psl->lpVtbl->SetHotkey(psl, wHotKey);
    psl->lpVtbl->SetShowCmd(psl, iShowCmd);


    //
    // Save the item to disk
    //

    bRetVal = SUCCEEDED(ppf->lpVtbl->Save(ppf, szLinkName, TRUE));

    if (bRetVal) {
        pShell32Api->pfnShChangeNotify (SHCNE_CREATE, SHCNF_PATH, szLinkName, NULL);
    }

    //
    // Release the IPersistFile object
    //

    ppf->lpVtbl->Release(ppf);


ExitFreePSL:

    //
    // Release the IShellLink object
    //

    psl->lpVtbl->Release(psl);

    pfnCoUninitialize();

ExitNoFree:

    if (hInstOLE32) {
        FreeLibrary (hInstOLE32);
    }

    //
    // Finished.
    //

    DebugMsg((DM_VERBOSE, TEXT("CreateLinkFile:  Leaving with status of %d."), bRetVal));

    return bRetVal;
}


//*************************************************************
//
//  DeleteLinkFile()
//
//  Purpose:    Deletes the specified link file
//
//  Parameters: csidl               -   CSIDL of a special folder
//              lpSubDirectory      -   Subdirectory of special folder
//              lpFileName          -   File name of item
//              bDeleteSubDirectory -   Delete the subdirectory if possible
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              3/26/98     ericflo    Created
//
//*************************************************************

BOOL WINAPI DeleteLinkFile(INT csidl, LPCTSTR lpSubDirectory,
                           LPCTSTR lpFileName, BOOL bDeleteSubDirectory)
{
    TCHAR   szLinkName[MAX_PATH];
    LPTSTR  lpEnd;
    LPTSTR  lpAdjustedSubDir = NULL;
    PSHELL32_API pShell32Api;

    //
    // Verbose output
    //

#if DBG

    DebugMsg((DM_VERBOSE, TEXT("DeleteLinkFile:  Entering.")));
    DebugMsg((DM_VERBOSE, TEXT("DeleteLinkFile:  csidl = 0x%x."), csidl));
    if (lpSubDirectory && *lpSubDirectory) {
        DebugMsg((DM_VERBOSE, TEXT("DeleteLinkFile:  lpSubDirectory = <%s>."), lpSubDirectory));
    }
    DebugMsg((DM_VERBOSE, TEXT("DeleteLinkFile:  lpFileName = <%s>."), lpFileName));
    DebugMsg((DM_VERBOSE, TEXT("DeleteLinkFile:  bDeleteSubDirectory = %d."), bDeleteSubDirectory));

#endif

    pShell32Api = LoadShell32Api();

    if ( !pShell32Api ) {
        return FALSE;
    }
    
    //
    // Get the special folder directory
    // First check if there is a CSIDL in the subdirectory
    //

    if (lpSubDirectory && *lpSubDirectory) {

        int csidl2 = ExtractCSIDL(lpSubDirectory, &lpAdjustedSubDir);

        if (-1 != csidl2)
        {
            csidl = csidl2;
            DebugMsg((DM_VERBOSE,
                TEXT("CreateLinkFile:  CSIDL = <0x%x> contained in lpSubDirectory replaces csidl."),
                csidl));
        }
    }

    szLinkName[0] = TEXT('\0');
    if (csidl && !GetSpecialFolderPath (csidl, szLinkName)) {
        return FALSE;
    }

    if (lpAdjustedSubDir && *lpAdjustedSubDir) {
        if (szLinkName[0] != TEXT('\0')) {
            lpEnd = CheckSlash (szLinkName);
        } else {
            lpEnd = szLinkName;
        }

        lstrcpy (lpEnd, lpAdjustedSubDir);
    }

    //
    // Now tack on the filename and extension.
    //

    lpEnd = CheckSlash (szLinkName);
    lstrcpy (lpEnd, lpFileName);
    lstrcat (lpEnd, c_szLNK);

    //
    // Delete the file
    //

    if (!DeleteFile (szLinkName)) {
        DebugMsg((DM_VERBOSE, TEXT("DeleteLinkFile: Failed to delete <%s>.  Error = %d"),
                szLinkName, GetLastError()));
        return FALSE;
    }

    pShell32Api->pfnShChangeNotify (SHCNE_DELETE, SHCNF_PATH, szLinkName, NULL);

    //
    // Delete the subdirectory if appropriate (and possible).
    //

    if (bDeleteSubDirectory) {
        *(lpEnd-1) = TEXT('\0');
        if (RemoveDirectory(szLinkName)) {
            pShell32Api->pfnShChangeNotify (SHCNE_RMDIR, SHCNF_PATH, szLinkName, NULL);
        }
    }

    //
    // Success
    //

    DebugMsg((DM_VERBOSE, TEXT("DeleteLinkFile:  Leaving successfully.")));

    return TRUE;
}

//*************************************************************
//
//  PrependPath()
//
//  Purpose:    Expands the given filename to have %systemroot%
//              if appropriate
//
//  Parameters: lpFile   -  File to check
//              lpResult -  Result buffer (MAX_PATH chars in size)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/11/95    ericflo    Created
//
//*************************************************************

BOOL PrependPath(LPCTSTR lpFile, LPTSTR lpResult)
{
    TCHAR szReturn [MAX_PATH];
    TCHAR szSysRoot[MAX_PATH];
    LPTSTR lpFileName;
    DWORD dwSysLen;


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("PrependPath: Entering with <%s>"),
             lpFile ? lpFile : TEXT("NULL")));


    if (!lpFile || !*lpFile) {
        DebugMsg((DM_VERBOSE, TEXT("PrependPath: lpFile is NULL, setting lpResult to a null string")));
        *lpResult = TEXT('\0');
        return TRUE;
    }


    //
    // Call SearchPath to find the filename
    //

    if (!SearchPath (NULL, lpFile, TEXT(".exe"), MAX_PATH, szReturn, &lpFileName)) {
        DebugMsg((DM_VERBOSE, TEXT("PrependPath: SearchPath failed with error %d.  Using input string"), GetLastError()));
        lstrcpy (lpResult, lpFile);
        return TRUE;
    }


    UnExpandSysRoot(szReturn, lpResult);

    DebugMsg((DM_VERBOSE, TEXT("PrependPath: Leaving with <%s>"), lpResult));

    return TRUE;
}


//*************************************************************
//
//  SetCommonAppDataSecurity()
//
//  Purpose:    Sets permissions for CommonAppData
//
//  Parameters: lpFile  -   File to set security on
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//
// Admins, System, Creator Owner: Full Control - Container Inherit, Object Inherit
// Users, Power Users: Read - Container Inherit, Object Inherit.
//                     Execute Permissions for the group above to fix office
//                     accessing the docs
// Users, Power Users: Write - Container Inherit
// Everyone: Read - Container Inherit, Object Inherit
//                     Execute Permissions for the group above to fix office
//                     accessing the docs
//
//*************************************************************

BOOL SetCommonDocumentsSecurity(LPTSTR lpFile)
{
    SECURITY_DESCRIPTOR sd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY authWORLD = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY authCreatorAuth = SECURITY_CREATOR_SID_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUsers = NULL, psidSystem = NULL, psidEveryOne = NULL;
    PSID  psidAdmin = NULL, psidPowerUsers = NULL, psidCreatorOwner=NULL;
    DWORD cbAcl, aceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bAddPowerUsersAce=TRUE;


    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to initialize system sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the Users sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_USERS,
                                  0, 0, 0, 0, 0, 0, &psidUsers)) {

         DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to initialize authenticated users sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to initialize admin sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the Everyone sid
    //

    if (!AllocateAndInitializeSid(&authWORLD, 1, SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidEveryOne)) {
                                  
        DebugMsg((DM_WARNING, TEXT("AddPowerUserAce: Failed to initialize power users sid.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Get the creator owner sid
    //

    if (!AllocateAndInitializeSid(&authCreatorAuth, 1, SECURITY_CREATOR_OWNER_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidCreatorOwner)) {
         DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to initialize creator owner sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Allocate space for the ACL
    //

    cbAcl = (GetLengthSid (psidSystem)) +
            (GetLengthSid (psidAdmin))  +
            (GetLengthSid (psidCreatorOwner))  +
            (2 * GetLengthSid (psidUsers))  +
            (GetLengthSid (psidEveryOne))  +
            sizeof(ACL) +
            (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    //
    // Get the power users sid, if required.
    // Don't fail if you don't get because it might not be available on DCs??
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                        DOMAIN_ALIAS_RID_POWER_USERS, 0, 0, 0, 0, 0, 0, &psidPowerUsers)) {

        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to initialize power users sid.  Error = %d"), GetLastError()));
        bAddPowerUsersAce = FALSE;
    }

    if (bAddPowerUsersAce)
        cbAcl += (2 * GetLengthSid (psidPowerUsers)) + (2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Add Aces. 
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidSystem)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidAdmin)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidCreatorOwner)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_GENERIC_READ | FILE_GENERIC_EXECUTE, psidUsers)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);


    if (bAddPowerUsersAce) {
        aceIndex++;
        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_GENERIC_READ | FILE_GENERIC_EXECUTE, psidPowerUsers)) {
            DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }

        if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
            DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }
        
        lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_GENERIC_WRITE, psidUsers)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= CONTAINER_INHERIT_ACE;


    if (bAddPowerUsersAce) {
        aceIndex++;
        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_GENERIC_WRITE, psidPowerUsers)) {
            DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }

        if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
            DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }
        
        lpAceHeader->AceFlags |= CONTAINER_INHERIT_ACE;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_GENERIC_READ | FILE_GENERIC_EXECUTE, psidEveryOne)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Set the security
    //

    if (SetFileSecurity (lpFile, DACL_SECURITY_INFORMATION, &sd)) {
        bRetVal = TRUE;
    } else {
        DebugMsg((DM_WARNING, TEXT("SetCommonDocumentsSecurity: SetFileSecurity failed.  Error = %d"), GetLastError()));
    }



Exit:

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidCreatorOwner) {
        FreeSid(psidCreatorOwner);
    }

    if (psidUsers) {
        FreeSid(psidUsers);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (psidPowerUsers) {
        FreeSid(psidPowerUsers);
    }

    if (psidEveryOne) {
        FreeSid(psidEveryOne);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }

    return bRetVal;
}

//*************************************************************
//
//  SetCommonAppDataSecurity()
//
//  Purpose:    Stes specific permisions to 
//
//  Parameters: lpFile  -   File to set security on
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//
// Admins, System, Creator Owner: Full Control - Container Inherit, Object Inherit
// Power Users: Modify - Container Inherit, Object Inherit
// Users: Read - Container Inherit, Object Inherit
// Everyone: Read - Container Inherit, Object Inherit
//
//*************************************************************

BOOL SetCommonAppDataSecurity(LPTSTR lpFile)
{
    SECURITY_DESCRIPTOR sd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY authWORLD = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY authCreatorAuth = SECURITY_CREATOR_SID_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUsers = NULL, psidSystem = NULL, psidEveryOne = NULL;
    PSID  psidAdmin = NULL, psidPowerUsers = NULL, psidCreatorOwner=NULL;
    DWORD cbAcl, aceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bAddPowerUsersAce=TRUE;


    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to initialize system sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the Users sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_USERS,
                                  0, 0, 0, 0, 0, 0, &psidUsers)) {

         DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to initialize authenticated users sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to initialize admin sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the Everyone sid
    //

    if (!AllocateAndInitializeSid(&authWORLD, 1, SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidEveryOne)) {
                                  
        DebugMsg((DM_WARNING, TEXT("AddPowerUserAce: Failed to initialize power users sid.  Error = %d"), GetLastError()));
        goto Exit;
    }
    
    //
    // Get the creator owner sid
    //


    if (!AllocateAndInitializeSid(&authCreatorAuth, 1, SECURITY_CREATOR_OWNER_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidCreatorOwner)) {
         DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to initialize creator owner sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Allocate space for the ACL
    //

    cbAcl = (GetLengthSid (psidSystem)) +
            (GetLengthSid (psidAdmin))  +
            (GetLengthSid (psidCreatorOwner))  +
            (GetLengthSid (psidUsers))  +
            (GetLengthSid (psidEveryOne))  +
            sizeof(ACL) +
            (5 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    //
    // Get the power users sid, if required.
    // Don't fail if you don't get because it might not be available on DCs??
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                        DOMAIN_ALIAS_RID_POWER_USERS, 0, 0, 0, 0, 0, 0, &psidPowerUsers)) {

        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to initialize power users sid.  Error = %d"), GetLastError()));
        bAddPowerUsersAce = FALSE;
    }

    if (bAddPowerUsersAce)
        cbAcl += (GetLengthSid (psidPowerUsers)) + ((sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Add Aces. 
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidSystem)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidAdmin)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidCreatorOwner)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_GENERIC_READ | FILE_GENERIC_EXECUTE, psidUsers)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);


    if (bAddPowerUsersAce) {
        aceIndex++;
        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS ^ (WRITE_DAC | WRITE_OWNER), psidPowerUsers)) {
            DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }

        if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
            DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }
        
        lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_GENERIC_READ | FILE_GENERIC_EXECUTE, psidEveryOne)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Set the security
    //

    if (SetFileSecurity (lpFile, DACL_SECURITY_INFORMATION, &sd)) {
        bRetVal = TRUE;
    } else {
        DebugMsg((DM_WARNING, TEXT("SetCommonAppDataSecurity: SetFileSecurity failed.  Error = %d"), GetLastError()));
    }



Exit:

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidCreatorOwner) {
        FreeSid(psidCreatorOwner);
    }

    if (psidUsers) {
        FreeSid(psidUsers);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (psidPowerUsers) {
        FreeSid(psidPowerUsers);
    }

    if (psidEveryOne) {
        FreeSid(psidEveryOne);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }

    return bRetVal;
}


//*************************************************************
//
//  ConvertCommonGroups()
//
//  Purpose:    Calls grpconv.exe to convert progman common groups
//              to Explorer common groups, and create floppy links.
//
//              NT 4 appended " (Common)" to the common groups.  For
//              NT 5, we are going to remove this tag.
//
//  Parameters: none
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//  Comments:
//
//  History:    Date        Author     Comment
//              10/1/95     ericflo    Created
//              12/5/96     ericflo    Remove common tag
//
//*************************************************************

BOOL ConvertCommonGroups (void)
{
    STARTUPINFO si;
    PROCESS_INFORMATION ProcessInformation;
    BOOL Result;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szBuffer2[MAX_PATH];
    DWORD dwType, dwSize, dwConvert;
    BOOL bRunGrpConv = TRUE;
    LONG lResult;
    HKEY hKey;
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    TCHAR szCommon[30] = {0};
    UINT cchCommon, cchFileName;
    LPTSTR lpTag, lpEnd, lpEnd2;


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:  Entering.")));


    //
    // Check if we have run grpconv before.
    //

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            TEXT("Software\\Program Groups"),
                            0,
                            KEY_ALL_ACCESS,
                            &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(dwConvert);

        lResult = RegQueryValueEx (hKey,
                                   TEXT("ConvertedToLinks"),
                                   NULL,
                                   &dwType,
                                   (LPBYTE)&dwConvert,
                                   &dwSize);

        if (lResult == ERROR_SUCCESS) {

            //
            // If dwConvert is 1, then grpconv has run before.
            // Don't run it again.
            //

            if (dwConvert) {
                bRunGrpConv = FALSE;
            }
        }

        //
        // Now set the value to prevent grpconv from running in the future
        //

        dwConvert = 1;
        RegSetValueEx (hKey,
                       TEXT("ConvertedToLinks"),
                       0,
                       REG_DWORD,
                       (LPBYTE) &dwConvert,
                       sizeof(dwConvert));


        RegCloseKey (hKey);
    }


    if (bRunGrpConv) {

        //
        // Initialize process startup info
        //

        si.cb = sizeof(STARTUPINFO);
        si.lpReserved = NULL;
        si.lpDesktop = NULL;
        si.lpTitle = NULL;
        si.dwFlags = 0;
        si.lpReserved2 = NULL;
        si.cbReserved2 = 0;


        //
        // Spawn grpconv
        //

        lstrcpy (szBuffer, TEXT("grpconv -n"));

        Result = CreateProcess(
                          NULL,
                          szBuffer,
                          NULL,
                          NULL,
                          FALSE,
                          NORMAL_PRIORITY_CLASS,
                          NULL,
                          NULL,
                          &si,
                          &ProcessInformation
                          );

        if (!Result) {
            DebugMsg((DM_WARNING, TEXT("ConvertCommonGroups:  grpconv failed to start due to error %d."), GetLastError()));
            return FALSE;

        } else {

            //
            // Wait for up to 2 minutes
            //

            WaitForSingleObject(ProcessInformation.hProcess, 120000);

            //
            // Close our handles to the process and thread
            //

            CloseHandle(ProcessInformation.hProcess);
            CloseHandle(ProcessInformation.hThread);

        }
    }


    //
    //  Loop through all the program groups in the All Users profile
    //  and remove the " (Common)" tag.
    //

    LoadString (g_hDllInstance, IDS_COMMON, szCommon, 30);
    cchCommon = lstrlen (szCommon);

    GetSpecialFolderPath (CSIDL_COMMON_PROGRAMS, szBuffer2);
    lstrcpy (szBuffer, szBuffer2);

    lpEnd = CheckSlash (szBuffer);
    lpEnd2 = CheckSlash (szBuffer2);

    lstrcpy (lpEnd, c_szStarDotStar);

    hFile = FindFirstFile (szBuffer, &fd);

    if (hFile != INVALID_HANDLE_VALUE) {

        do  {

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                cchFileName = lstrlen (fd.cFileName);

                if (cchFileName > cchCommon) {
                    lpTag = fd.cFileName + cchFileName - cchCommon;

                    if (!lstrcmpi(lpTag, szCommon)) {

                        lstrcpy (lpEnd, fd.cFileName);
                        *lpTag = TEXT('\0');
                        lstrcpy (lpEnd2, fd.cFileName);

                        if (MoveFileEx (szBuffer, szBuffer2, MOVEFILE_REPLACE_EXISTING)) {

                            DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:  Successfully changed group name:")));
                            DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:      Orginial:  %s"), szBuffer));
                            DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:      New:       %s"), szBuffer2));

                        } else {
                            DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:  Failed to change group name with error %d."), GetLastError()));
                            DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:      Orginial:  %s"), szBuffer));
                            DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:      New:       %s"), szBuffer2));
                        }
                    }
                }
            }

        } while (FindNextFile(hFile, &fd));

        FindClose (hFile);
    }


    //
    // Success
    //

    DebugMsg((DM_VERBOSE, TEXT("ConvertCommonGroups:  Leaving Successfully.")));

    return TRUE;
}

//*************************************************************
//
//  DetermineProfilesLocation()
//
//  Purpose:    Determines if the profiles directory
//              should be in the old NT4 location or
//              the new NT5 location
//
//  Parameters: none
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL WINAPI DetermineProfilesLocation (BOOL bCleanInstall)
{
    TCHAR szDirectory[MAX_PATH];
    TCHAR szDest[MAX_PATH];
    WIN32_FILE_ATTRIBUTE_DATA fad;
    DWORD dwSize, dwDisp;
    HKEY hKey;
    LPTSTR lpEnd;


    //
    // Check for an unattended entry first
    //

    if (bCleanInstall) {

        if (!ExpandEnvironmentStrings (TEXT("%SystemRoot%\\System32\\$winnt$.inf"), szDirectory,
                                       ARRAYSIZE(szDirectory))) {

            DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  ExpandEnvironmentStrings failed with error %d"), GetLastError()));
            return FALSE;
        }

        szDest[0] = TEXT('\0');
        GetPrivateProfileString (TEXT("guiunattended"), TEXT("profilesdir"), TEXT(""),
                                 szDest, MAX_PATH, szDirectory);

        if (szDest[0] != TEXT('\0')) {

            //
            // The unattend profile directory exists.  We need to set this
            // path in the registry
            //

            if (RegCreateKeyEx (HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH,
                                0, NULL, REG_OPTION_NON_VOLATILE,
                                KEY_WRITE, NULL, &hKey,
                                &dwDisp) == ERROR_SUCCESS) {

                if (RegSetValueEx (hKey, PROFILES_DIRECTORY,
                                   0, REG_EXPAND_SZ, (LPBYTE) szDest,
                                   ((lstrlen(szDest) + 1) * sizeof(TCHAR))) == ERROR_SUCCESS) {

                    DebugMsg((DM_VERBOSE, TEXT("DetermineProfilesLocation:  Using unattend location %s for user profiles"), szDest));
                }

                RegCloseKey (hKey);
            }
        }

    } else {

        //
        // By default, the OS will try to use the new location for
        // user profiles, but if we are doing an upgrade of a machine
        // with profiles in the NT4 location, we want to continue
        // to use that location.
        //
        // Build a test path to the old All Users directory on NT4
        // to determine which location to use.
        //

        if (!ExpandEnvironmentStrings (NT4_PROFILES_DIRECTORY, szDirectory,
                                       ARRAYSIZE(szDirectory))) {

            DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  ExpandEnvironmentStrings failed with error %d, setting the dir unexpanded"), GetLastError()));
            return FALSE;
        }

        lpEnd = CheckSlash (szDirectory);
        lstrcpy (lpEnd, ALL_USERS);

        if (GetFileAttributesEx (szDirectory, GetFileExInfoStandard, &fad)) {

            //
            // An app was found that creates an "All Users" directory under NT4 profiles directory
            // Check for Default User as well.
            //

            lstrcpy (lpEnd, DEFAULT_USER);

            if (GetFileAttributesEx (szDirectory, GetFileExInfoStandard, &fad)) {

                //
                // The old profiles directory exists.  We need to set this
                // path in the registry
                //

                if (RegCreateKeyEx (HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH,
                                    0, NULL, REG_OPTION_NON_VOLATILE,
                                    KEY_WRITE, NULL, &hKey,
                                    &dwDisp) == ERROR_SUCCESS) {

                    if (RegSetValueEx (hKey, PROFILES_DIRECTORY,
                                       0, REG_EXPAND_SZ, (LPBYTE) NT4_PROFILES_DIRECTORY,
                                       ((lstrlen(NT4_PROFILES_DIRECTORY) + 1) * sizeof(TCHAR))) == ERROR_SUCCESS) {

                        DebugMsg((DM_VERBOSE, TEXT("DetermineProfilesLocation:  Using NT4 location for user profiles")));
                    }

                    RegCloseKey (hKey);
                }
            }
        }
    }


    //
    // Check if the profiles directory exists.
    //

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetProfilesDirectoryEx(szDirectory, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to query profiles directory root.")));
        return FALSE;
    }

    if (!CreateSecureAdminDirectory(szDirectory, OTHERSIDS_EVERYONE)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to create profiles subdirectory <%s>.  Error = %d."),
                 szDirectory, GetLastError()));
        return FALSE;
    }


    //
    // Decide where the Default User profile should be
    //

    if (!CheckProfile (szDirectory, DEFAULT_USER_PROFILE, DEFAULT_USER)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to check default user profile  Error = %d."),
                 GetLastError()));
        return FALSE;
    }


    //
    // Check if the profiles\Default User directory exists
    //

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetDefaultUserProfileDirectoryEx(szDirectory, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to query default user profile directory.")));
        return FALSE;
    }

    if (!CreateSecureAdminDirectory (szDirectory, OTHERSIDS_EVERYONE)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to create Default User subdirectory <%s>.  Error = %d."),
                 szDirectory, GetLastError()));
        return FALSE;
    }

    SetFileAttributes (szDirectory, FILE_ATTRIBUTE_HIDDEN);


    //
    // Decide where the All Users profile should be
    //

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetProfilesDirectoryEx(szDirectory, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to query profiles directory root.")));
        return FALSE;
    }

    if (!CheckProfile (szDirectory, ALL_USERS_PROFILE, ALL_USERS)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to check all users profile  Error = %d."),
                 GetLastError()));
        return FALSE;
    }


    //
    // Check if the profiles\All Users directory exists
    //

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetAllUsersProfileDirectoryEx(szDirectory, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to query all users profile directory.")));
        return FALSE;
    }

    //
    // Give additional permissions for power users/everyone
    //

    if (!CreateSecureAdminDirectory (szDirectory, OTHERSIDS_POWERUSERS | OTHERSIDS_EVERYONE)) {
        DebugMsg((DM_WARNING, TEXT("DetermineProfilesLocation:  Failed to create All Users subdirectory <%s>.  Error = %d."),
                 szDirectory, GetLastError()));
        return FALSE;
    }


    return TRUE;
}

//*************************************************************
//
//  InitializeProfiles()
//
//  Purpose:    Confirms / Creates the profile, Default User,
//              and All Users directories, and converts any
//              existing common groups.
//
//  Parameters:
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   This should only be called by GUI mode setup!
//
//  History:    Date        Author     Comment
//              8/08/95     ericflo    Created
//
//*************************************************************

BOOL WINAPI InitializeProfiles (BOOL bGuiModeSetup)
{
    TCHAR szDirectory[MAX_PATH];
    TCHAR szTemp[MAX_PATH], szTemp2[MAX_PATH];
    DWORD dwSize, dwDisp;
    LPTSTR lpEnd;
    DWORD i;
    HKEY hKey;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    BOOL bRetVal = FALSE;
    DWORD dwErr;

    dwErr = GetLastError();

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("InitializeProfiles:  Entering.")));



    //
    // Create a named mutex that represents GUI mode setup running.
    // This allows other processes that load userenv to detect that
    // setup is running.
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

    if (bGuiModeSetup) {
        g_hGUIModeSetup = CreateMutex (&sa, FALSE, GUIMODE_SETUP_MUTEX);

        if (!g_hGUIModeSetup) {
            DebugMsg((DM_WARNING, TEXT("InitializeProfiles: Failed to create mutex.  Error = %d."),
                     GetLastError()));
            dwErr = GetLastError();
            goto Exit;
        }
    }
    else {
        //
        // if it is called from anywhere else, just block loading user profile
        //

        if (g_hProfileSetup) {
            ResetEvent (g_hProfileSetup);
        }
    }


#if 0  //ds
    //
    // Move any profiles in the old NT4 location to the new location
    //

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetProfilesDirectoryEx(szDirectory, &dwSize, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("InitializeProfiles:  Failed to query profiles directory root.")));
        dwErr = GetLastError();
        goto Exit;
    }

    if (!MoveUserProfiles(NT4_PROFILES_DIRECTORY, szDirectory)) {
        DebugMsg((DM_WARNING, TEXT("InitializeProfiles: MoveUserProfiles failed.")));
    }


    //
    // Move any profiles in %SystemDrive%\Users to the new location
    // BUGBUG:  this can be removed after NT5 Beta 2
    //

    if (!MoveUserProfiles(TEXT("%SystemDrive%\\Users"), szDirectory)) {
        DebugMsg((DM_WARNING, TEXT("InitializeProfiles: MoveUserProfiles failed.")));
    }
#endif //ds


    //
    // Requery for the default user profile directory (expanded version)
    //

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetDefaultUserProfileDirectoryEx(szDirectory, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("InitializeProfiles:  Failed to query default user profile directory.")));
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // Set the USERPROFILE environment variable
    //

    SetEnvironmentVariable (TEXT("USERPROFILE"), szDirectory);


    //
    // Create all the folders under Default User
    //

    lpEnd = CheckSlash (szDirectory);


    //
    // Loop through the shell folders
    //

    for (i=0; i < g_dwNumShellFolders; i++) {

        lstrcpy (lpEnd,  c_ShellFolders[i].lpFolderLocation);

        if (!CreateNestedDirectory(szDirectory, NULL)) {
            DebugMsg((DM_WARNING, TEXT("InitializeProfiles: Failed to create the destination directory <%s>.  Error = %d"),
                     szDirectory, GetLastError()));
            dwErr = GetLastError();
            goto Exit;
        }

        if (c_ShellFolders[i].bHidden) {
            SetFileAttributes(szDirectory, FILE_ATTRIBUTE_HIDDEN);
        } else {
            SetFileAttributes(szDirectory, FILE_ATTRIBUTE_NORMAL);
        }

    }


    //
    // Remove the %USERPROFILE%\Personal directory if it exists.
    // Windows NT 4.0 had a Personal folder in the root of the
    // user's profile.  NT 5.0 renames this folder to My Documents
    //

    if (LoadString (g_hDllInstance, IDS_SH_PERSONAL2, szTemp, ARRAYSIZE(szTemp))) {
        lstrcpy (lpEnd,  szTemp);
        RemoveDirectory(szDirectory);
    }


    //
    // Migrate the Template Directory if it exists. Copy it from %systemroot%\shellnew
    // to Templates directory under default user. Do the same for existing profiles.
    // 

    if ((LoadString (g_hDllInstance, IDS_SH_TEMPLATES2, szTemp, ARRAYSIZE(szTemp))) && 
            (ExpandEnvironmentStrings (szTemp, szTemp2, ARRAYSIZE(szTemp2))) &&
            (LoadString (g_hDllInstance, IDS_SH_TEMPLATES, szTemp, ARRAYSIZE(szTemp)))) {

        //
        // if all of the above succeeded
        //

        lstrcpy (lpEnd, szTemp);
        DebugMsg((DM_VERBOSE, TEXT("InitializeProfiles: Copying <%s> to %s.  Error = %d"), szTemp2, szDirectory));
        CopyProfileDirectory(szTemp2, szDirectory, CPD_IGNORECOPYERRORS | CPD_IGNOREHIVE);
    }


    //
    // Remove %USERPROFILE%\Temp if it exists.  The Temp directory
    // will now be in the Local Settings folder
    //

    lstrcpy (lpEnd, TEXT("Temp"));
    Delnode(szDirectory);


    //
    // Remove %USERPROFILE%\Temporary Internet Files if it exists.  The
    // Temporary Internet Files directory will now be in the Local Settings
    // folder
    //

    if (LoadString (g_hDllInstance, IDS_TEMPINTERNETFILES, szTemp, ARRAYSIZE(szTemp))) {
        lstrcpy (lpEnd,  szTemp);
        Delnode(szDirectory);
    }


    //
    // Remove %USERPROFILE%\History if it exists.  The History
    // directory will now be in the Local Settings folder
    //

    if (LoadString (g_hDllInstance, IDS_HISTORY, szTemp, ARRAYSIZE(szTemp))) {
        lstrcpy (lpEnd,  szTemp);
        Delnode(szDirectory);
    }


    //
    // Set the User Shell Folder paths in the registry
    //

    lstrcpy (szTemp, TEXT(".Default"));
    lpEnd = CheckSlash (szTemp);
    lstrcpy(lpEnd, USER_SHELL_FOLDERS);

    lstrcpy (szDirectory, TEXT("%USERPROFILE%"));
    lpEnd = CheckSlash (szDirectory);

    if (RegCreateKeyEx (HKEY_USERS, szTemp,
                        0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE, NULL, &hKey,
                        &dwDisp) == ERROR_SUCCESS) {

        for (i=0; i < g_dwNumShellFolders; i++) {

            if (c_ShellFolders[i].bAddCSIDL) {
                lstrcpy (lpEnd, c_ShellFolders[i].lpFolderLocation);

                RegSetValueEx (hKey, c_ShellFolders[i].lpFolderName,
                             0, REG_EXPAND_SZ, (LPBYTE) szDirectory,
                             ((lstrlen(szDirectory) + 1) * sizeof(TCHAR)));
            }
        }

        RegCloseKey (hKey);
    }


    //
    // Set the Shell Folder paths in the registry
    //

    lstrcpy (szTemp, TEXT(".Default"));
    lpEnd = CheckSlash (szTemp);
    lstrcpy(lpEnd, SHELL_FOLDERS);

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetDefaultUserProfileDirectoryEx(szDirectory, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("InitializeProfiles:  Failed to query default user profile directory.")));
        dwErr = GetLastError();
        goto Exit;
    }

    lpEnd = CheckSlash (szDirectory);

    if (RegCreateKeyEx (HKEY_USERS, szTemp,
                        0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE, NULL, &hKey,
                        &dwDisp) == ERROR_SUCCESS) {

        for (i=0; i < g_dwNumShellFolders; i++) {

            if (c_ShellFolders[i].bAddCSIDL) {
                lstrcpy (lpEnd, c_ShellFolders[i].lpFolderLocation);

                RegSetValueEx (hKey, c_ShellFolders[i].lpFolderName,
                             0, REG_SZ, (LPBYTE) szDirectory,
                             ((lstrlen(szDirectory) + 1) * sizeof(TCHAR)));
            }
        }

        RegCloseKey (hKey);
    }


    //
    // Set the per user TEMP and TMP environment variables
    //

    if (LoadString (g_hDllInstance, IDS_SH_TEMP,
                    szTemp, ARRAYSIZE(szTemp))) {

        lstrcpy (szDirectory, TEXT("%USERPROFILE%"));
        lpEnd = CheckSlash (szDirectory);

        LoadString (g_hDllInstance, IDS_SH_LOCALSETTINGS,
                    lpEnd, ARRAYSIZE(szTemp)-(lstrlen(szDirectory)));

        lpEnd = CheckSlash (szDirectory);
        lstrcpy (lpEnd,  szTemp);
        

        if (RegCreateKeyEx (HKEY_USERS, TEXT(".Default\\Environment"),
                            0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_WRITE, NULL, &hKey,
                            &dwDisp) == ERROR_SUCCESS) {

            RegSetValueEx (hKey, TEXT("TEMP"),
                           0, REG_EXPAND_SZ, (LPBYTE) szDirectory,
                           ((lstrlen(szDirectory) + 1) * sizeof(TCHAR)));

            RegSetValueEx (hKey, TEXT("TMP"),
                           0, REG_EXPAND_SZ, (LPBYTE) szDirectory,
                           ((lstrlen(szDirectory) + 1) * sizeof(TCHAR)));

            RegCloseKey (hKey);
        }
    }


    //
    // Set the user preference exclusion list.  This will
    // prevent the Local Settings folder from roaming
    //

    if (LoadString (g_hDllInstance, IDS_EXCLUSIONLIST,
                    szDirectory, ARRAYSIZE(szDirectory))) {

        lstrcpy (szTemp, TEXT(".Default"));
        lpEnd = CheckSlash (szTemp);
        lstrcpy(lpEnd, WINLOGON_KEY);

        if (RegCreateKeyEx (HKEY_USERS, szTemp,
                            0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WRITE, NULL, &hKey,
                            &dwDisp) == ERROR_SUCCESS) {

            RegSetValueEx (hKey, TEXT("ExcludeProfileDirs"),
                           0, REG_SZ, (LPBYTE) szDirectory,
                           ((lstrlen(szDirectory) + 1) * sizeof(TCHAR)));

            RegCloseKey (hKey);
        }
    }


    //
    // Requery for the all users profile directory (expanded version)
    //

    dwSize = ARRAYSIZE(szDirectory);
    if (!GetAllUsersProfileDirectoryEx(szDirectory, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("InitializeProfiles:  Failed to query all users profile directory.")));
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // Set the ALLUSERSPROFILE environment variable
    //

    SetEnvironmentVariable (TEXT("ALLUSERSPROFILE"), szDirectory);


    //
    // Create all the folders under All Users
    //


    lpEnd = CheckSlash (szDirectory);


    //
    // Loop through the shell folders
    //

    for (i=0; i < g_dwNumCommonShellFolders; i++) {

        lstrcpy (lpEnd,  c_CommonShellFolders[i].lpFolderLocation);

        if (!CreateNestedDirectory(szDirectory, NULL)) {
            DebugMsg((DM_WARNING, TEXT("InitializeProfiles: Failed to create the destination directory <%s>.  Error = %d"),
                     szDirectory, GetLastError()));
            dwErr = GetLastError();
            goto Exit;
        }

        if (c_CommonShellFolders[i].bHidden) {
            SetFileAttributes(szDirectory, FILE_ATTRIBUTE_HIDDEN);
        } else {
            SetFileAttributes(szDirectory, FILE_ATTRIBUTE_NORMAL);
        }
    }


    //
    // Unsecure the Documents and App Data folders in the All Users profile
    //

    if (LoadString (g_hDllInstance, IDS_SH_SHAREDDOCS, szTemp, ARRAYSIZE(szTemp))) {
        lstrcpy (lpEnd,  szTemp);
        SetCommonDocumentsSecurity(szDirectory);
    }

    if (LoadString (g_hDllInstance, IDS_SH_APPDATA, szTemp, ARRAYSIZE(szTemp))) {
        lstrcpy (lpEnd,  szTemp);
        SetCommonAppDataSecurity(szDirectory);
    }


    //
    // Set the User Shell Folder paths in the registry
    //

    lstrcpy (szDirectory, TEXT("%ALLUSERSPROFILE%"));
    lpEnd = CheckSlash (szDirectory);

    if (RegCreateKeyEx (HKEY_LOCAL_MACHINE, USER_SHELL_FOLDERS,
                        0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE, NULL, &hKey,
                        &dwDisp) == ERROR_SUCCESS) {

        for (i=0; i < g_dwNumCommonShellFolders; i++) {

            if (c_ShellFolders[i].bAddCSIDL) {
                lstrcpy (lpEnd, c_CommonShellFolders[i].lpFolderLocation);

                RegSetValueEx (hKey, c_CommonShellFolders[i].lpFolderName,
                             0, REG_EXPAND_SZ, (LPBYTE) szDirectory,
                             ((lstrlen(szDirectory) + 1) * sizeof(TCHAR)));
            }
        }

        RegCloseKey (hKey);
    }


    //
    // Convert any Program Manager common groups
    //

    if (!ConvertCommonGroups()) {
        DebugMsg((DM_WARNING, TEXT("InitializeProfiles: ConvertCommonGroups failed.")));
    }


    //
    // Success
    //

    DebugMsg((DM_VERBOSE, TEXT("InitializeProfiles:  Leaving successfully.")));

    bRetVal = TRUE;

Exit:

    if ((!bGuiModeSetup) && (g_hProfileSetup)) {
        SetEvent (g_hProfileSetup);
    }

    SetLastError(dwErr);
    return bRetVal;
}

//*************************************************************
//
//  CheckProfile()
//
//  Purpose:    Checks and creates a storage location for either
//              the Default User or All Users profile
//
//  Parameters: LPTSTR lpProfilesDir  - Root of the profiles
//              LPTSTR lpProfileValue - Profile registry value name
//              LPTSTR lpProfileName  - Default profile name
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL CheckProfile (LPTSTR lpProfilesDir, LPTSTR lpProfileValue,
                   LPTSTR lpProfileName)
{
    TCHAR szTemp[MAX_PATH];
    TCHAR szTemp2[MAX_PATH];
    TCHAR szName[MAX_PATH];
    TCHAR szFormat[30];
    DWORD dwSize, dwDisp, dwType;
    LPTSTR lpEnd;
    LONG lResult;
    HKEY hKey;
    INT iStrLen;
    WIN32_FILE_ATTRIBUTE_DATA fad;


    //
    // Open the ProfileList key
    //

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH,
                              0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CheckProfile:  Failed to open profile list key with %d."),
                 lResult));
        return FALSE;
    }


    //
    // Check the registry to see if this folder is defined already
    //

    dwSize = sizeof(szTemp);
    if (RegQueryValueEx (hKey, lpProfileValue, NULL, &dwType,
                         (LPBYTE) szTemp, &dwSize) == ERROR_SUCCESS) {
        RegCloseKey (hKey);
        return TRUE;
    }


    //
    // Generate the default name
    //

    lstrcpy (szTemp, lpProfilesDir);
    lpEnd = CheckSlash (szTemp);
    lstrcpy (lpEnd, lpProfileName);
    lstrcpy (szName, lpProfileName);


    //
    //  Check if this directory exists
    //

    ExpandEnvironmentStrings (szTemp, szTemp2, ARRAYSIZE(szTemp2));

    if (GetFileAttributesEx (szTemp2, GetFileExInfoStandard, &fad)) {

        if (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {


            //
            // Check if this directory is under the system root.
            // If so, this is ok, we don't need to generate a unique
            // name for this.
            //

            ExpandEnvironmentStrings (TEXT("%SystemRoot%"), szTemp,
                                      ARRAYSIZE(szTemp));

            iStrLen = lstrlen (szTemp);

            if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                               szTemp, iStrLen, szTemp2, iStrLen) != CSTR_EQUAL) {


                //
                // The directory exists already.  Use a new name of
                // Profile Name (SystemDirectory)
                //
                // eg:  Default User (WINNT)
                //

                lpEnd = szTemp + lstrlen(szTemp) - 1;

                while ((lpEnd > szTemp) && ((*lpEnd) != TEXT('\\')))
                    lpEnd--;

                if (*lpEnd == TEXT('\\')) {
                    lpEnd++;
                }

                LoadString (g_hDllInstance, IDS_PROFILE_FORMAT, szFormat,
                            ARRAYSIZE(szFormat));
                wsprintf (szName, szFormat, lpProfileName, lpEnd);

                //
                // To prevent reusing the directories, delete it first..
                //

                lstrcpy (szTemp, lpProfilesDir);
                lpEnd = CheckSlash (szTemp);
                lstrcpy (lpEnd, szName);

                if (ExpandEnvironmentStrings (szTemp, szTemp2, ARRAYSIZE(szTemp2))) {
                    DebugMsg((DM_VERBOSE, TEXT("InitializeProfiles:  Delnoding directory.. %s"), szTemp2));
                    Delnode(szTemp2);
                }
            }
        }
    }


    //
    // Save the profile name in the registry
    //

    RegSetValueEx (hKey, lpProfileValue,
                 0, REG_SZ, (LPBYTE) szName,
                 ((lstrlen(szName) + 1) * sizeof(TCHAR)));

    RegCloseKey (hKey);


    DebugMsg((DM_VERBOSE, TEXT("InitializeProfiles:  The %s profile is mapped to %s"),
             lpProfileName, szName));

    return TRUE;
}


//*************************************************************
//
//  CreateUserProfile()
//
//  Purpose:    Creates a new user profile, but does not load
//              the hive.
//
//  Parameters: pSid         -   SID pointer
//              lpUserName   -   User name
//              lpUserHive   -   Optional user hive
//              lpProfileDir -   Receives the new profile directory
//              dwDirSize    -   Size of lpProfileDir
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   If a user hive isn't specified the default user
//              hive will be used.
//
//  History:    Date        Author     Comment
//              9/12/95     ericflo    Created
//
//*************************************************************

BOOL WINAPI CreateUserProfile (PSID pSid, LPCTSTR lpUserName, LPCTSTR lpUserHive,
                               LPTSTR lpProfileDir, DWORD dwDirSize)
{
    TCHAR szProfileDir[MAX_PATH];
    TCHAR szExpProfileDir[MAX_PATH] = {0};
    TCHAR szDirectory[MAX_PATH];
    TCHAR LocalProfileKey[MAX_PATH];
    UNICODE_STRING UnicodeString;
    LPTSTR lpSidString, lpEnd, lpSave;
    NTSTATUS NtStatus;
    LONG lResult;
    DWORD dwDisp;
    DWORD dwError;
    DWORD dwSize;
    DWORD dwType;
    HKEY hKey;



    //
    // Check parameters
    //

    if (!lpUserName || !lpUserName[0]) {
        DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Null username.")));
        return FALSE;
    }

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("CreateUserProfile:  Entering with <%s>."), lpUserName));
    DebugMsg((DM_VERBOSE, TEXT("CreateUserProfile:  Entering with user hive of <%s>."),
             lpUserHive ? lpUserHive : TEXT("NULL")));


    //
    // Convert the sid into text format
    //

    NtStatus = RtlConvertSidToUnicodeString(&UnicodeString, pSid, (BOOLEAN)TRUE);

    if (!NT_SUCCESS(NtStatus)) {
        DebugMsg((DM_WARNING, TEXT("CreateUserProfile: RtlConvertSidToUnicodeString failed, status = 0x%x"),
                 NtStatus));
        return FALSE;
    }

    lpSidString = UnicodeString.Buffer;


    //
    // Check if this user's profile exists already
    //

    lstrcpy(LocalProfileKey, PROFILE_LIST_PATH);
    lstrcat(LocalProfileKey, TEXT("\\"));
    lstrcat(LocalProfileKey, lpSidString);

    szProfileDir[0] = TEXT('\0');

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, LocalProfileKey,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(szProfileDir);
        RegQueryValueEx (hKey, PROFILE_IMAGE_VALUE_NAME, NULL,
                         &dwType, (LPBYTE) szProfileDir, &dwSize);

        RegCloseKey (hKey);
    }


    if (szProfileDir[0] == TEXT('\0')) {

        //
        // Make the user's directory
        //

        dwSize = ARRAYSIZE(szProfileDir);
        if (!GetProfilesDirectoryEx(szProfileDir, &dwSize, FALSE)) {
            DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Failed to get profile root directory.")));
            RtlFreeUnicodeString(&UnicodeString);
            return FALSE;
        }

        if (!ComputeLocalProfileName (NULL, lpUserName, szProfileDir, ARRAYSIZE(szProfileDir),
                                      szExpProfileDir, ARRAYSIZE(szExpProfileDir), pSid, TRUE)) {
            DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Failed to create directory.")));
            RtlFreeUnicodeString(&UnicodeString);
            return FALSE;
        }


        //
        // Copy the default user profile into this directory
        //

        dwSize = ARRAYSIZE(szDirectory);
        if (!GetDefaultUserProfileDirectoryEx(szDirectory, &dwSize, TRUE)) {
            DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Failed to get default user profile.")));
            RtlFreeUnicodeString(&UnicodeString);
            return FALSE;
        }


        if (lpUserHive) {

            //
            // Copy the default user profile without the hive.
            //

            if (!CopyProfileDirectory (szDirectory, szExpProfileDir, CPD_IGNORECOPYERRORS | CPD_IGNOREHIVE | CPD_IGNORELONGFILENAMES)) {
                DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   CopyProfileDirectory failed with error %d."), GetLastError()));
                RtlFreeUnicodeString(&UnicodeString);
                return FALSE;
            }

            //
            // Now copy the hive
            //

            lstrcpy (szDirectory, szExpProfileDir);
            lpEnd = CheckSlash (szDirectory);
            lstrcpy (lpEnd, c_szNTUserDat);

            if (!CopyFile (lpUserHive, szDirectory, FALSE)) {
                DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   Failed to copy user hive with error %d."), GetLastError()));
                DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   Source:  %s."), lpUserHive));
                DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   Destination:  %s."), szDirectory));
                RtlFreeUnicodeString(&UnicodeString);
                return FALSE;
            }

        } else {

            //
            // Copy the default user profile and the hive.
            //

            if (!CopyProfileDirectory (szDirectory, szExpProfileDir, CPD_IGNORECOPYERRORS | CPD_IGNORELONGFILENAMES)) {
                DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   CopyProfileDirectory failed with error %d."), GetLastError()));
                RtlFreeUnicodeString(&UnicodeString);
                return FALSE;
            }
        }


        //
        // Save the user's profile in the registry.
        //

        lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, LocalProfileKey, 0, 0, 0,
                                KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {

           DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Failed trying to create the local profile key <%s>, error = %d."), LocalProfileKey, lResult));
           RtlFreeUnicodeString(&UnicodeString);
           return FALSE;
        }



        //
        // Add the profile directory
        //

        lResult = RegSetValueEx(hKey, PROFILE_IMAGE_VALUE_NAME, 0,
                            REG_EXPAND_SZ,
                            (LPBYTE)szProfileDir,
                            sizeof(TCHAR)*(lstrlen(szProfileDir) + 1));


        if (lResult != ERROR_SUCCESS) {

           DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  First RegSetValueEx failed, error = %d."), lResult));
           RegCloseKey (hKey);
           RtlFreeUnicodeString(&UnicodeString);
           return FALSE;
        }


        //
        // Add the users's SID
        //

        lResult = RegSetValueEx(hKey, TEXT("Sid"), 0,
                            REG_BINARY, pSid, RtlLengthSid(pSid));


        if (lResult != ERROR_SUCCESS) {

           DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Second RegSetValueEx failed, error = %d."), lResult));
        }


        //
        // Close the registry key
        //

        RegCloseKey (hKey);

    } else {

        //
        // The user already has a profile, so just copy the hive if
        // appropriate.
        //

        ExpandEnvironmentStrings (szProfileDir, szExpProfileDir,
                                  ARRAYSIZE(szExpProfileDir));

        if (lpUserHive) {

            //
            // Copy the hive
            //

            lstrcpy (szDirectory, szExpProfileDir);
            lpEnd = CheckSlash (szDirectory);
            lstrcpy (lpEnd, c_szNTUserDat);

            SetFileAttributes (szDirectory, FILE_ATTRIBUTE_NORMAL);

            if (!CopyFile (lpUserHive, szDirectory, FALSE)) {
                DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   Failed to copy user hive with error %d."), GetLastError()));
                DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   Source:  %s."), lpUserHive));
                DebugMsg((DM_WARNING, TEXT("CreateUserProfile:   Destination:  %s."), szDirectory));
                RtlFreeUnicodeString(&UnicodeString);
                return FALSE;
            }

        }
    }


    //
    // Now load the hive temporary so the security can be fixed
    //

    lpEnd = CheckSlash (szExpProfileDir);
    lpSave = lpEnd - 1;
    lstrcpy (lpEnd, c_szNTUserDat);

    lResult = MyRegLoadKey(HKEY_USERS, lpSidString, szExpProfileDir);

    *lpSave = TEXT('\0');

    if (lResult != ERROR_SUCCESS) {

        DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  Failed to load hive, error = %d."), lResult));
        dwError = GetLastError();
        DeleteProfileEx (lpSidString, szExpProfileDir, FALSE, HKEY_LOCAL_MACHINE);
        RtlFreeUnicodeString(&UnicodeString);
        SetLastError(dwError);
        return FALSE;
    }

    if (!SetupNewHive(NULL, lpSidString, pSid)) {

        DebugMsg((DM_WARNING, TEXT("CreateUserProfile:  SetupNewHive failed.")));
        dwError = GetLastError();
        MyRegUnLoadKey(HKEY_USERS, lpSidString);
        DeleteProfileEx (lpSidString, szExpProfileDir, FALSE, HKEY_LOCAL_MACHINE);
        RtlFreeUnicodeString(&UnicodeString);
        SetLastError(dwError);
        return FALSE;

    }


    //
    // Unload the hive
    //

    MyRegUnLoadKey(HKEY_USERS, lpSidString);


    //
    // Free the sid string
    //

    RtlFreeUnicodeString(&UnicodeString);


    //
    // Save the profile path if appropriate
    //

    if (lpProfileDir) {

        if ((DWORD)lstrlen(szExpProfileDir) < dwDirSize) {
            lstrcpy (lpProfileDir, szExpProfileDir);
        }
    }


    //
    // Success
    //

    DebugMsg((DM_VERBOSE, TEXT("CreateUserProfile:  Leaving successfully.")));

    return TRUE;

}
