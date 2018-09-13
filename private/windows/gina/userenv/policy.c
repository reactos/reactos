//*************************************************************
//
//  Functions to apply policy
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"
#include <regstr.h>
#include <winnetwk.h>
#include <lm.h>

//
// Update mode constants
//

#define UM_OFF                  0
#define UM_AUTOMATIC            1
#define UM_MANUAL               2


//
// Prefix constants for value names
//

#define NUM_PREFIX              3
#define PREFIX_UNKNOWN          0
#define PREFIX_DELETE           1
#define PREFIX_SOFT             2
#define PREFIX_DELVALS          3


//
// Max size of a value's data
//

#define MAX_VALUE_DATA       4096

//
// Default group size
//

#define DEFAULT_GROUP_SIZE   8192


//
// Registry value names
//

TCHAR g_szUpdateModeValue[] = TEXT("UpdateMode");
TCHAR g_szNetPathValue[] = TEXT("NetworkPath");
TCHAR g_szLogonKey[] = WINLOGON_KEY;
CHAR  g_szPolicyHandler[] = "PolicyHandler";  // This needs to be ANSI
TCHAR g_szTmpKeyName[] = TEXT("AdminConfigData");
TCHAR g_szPrefixDel[] = TEXT("**del.");
TCHAR g_szPrefixSoft[] = TEXT("**soft.");
TCHAR g_szPrefixDelvals[] = TEXT("**delvals.");


//
// Function proto-types
//

HKEY OpenUserKey(HKEY hkeyRoot, LPCTSTR pszName, BOOL * pfFoundSpecific);
UINT MergeRegistryData(HKEY hkeySrc, HKEY hkeyDst, LPTSTR pszKeyNameBuffer,
                       UINT cbKeyNameBuffer);
UINT CopyKeyValues(HKEY hkeySrc,HKEY hkeyDst);
BOOL HasSpecialPrefix(LPTSTR szValueName, DWORD * pdwPrefix,
        LPTSTR szStrippedValueName);
BOOL GetGroupProcessingOrder(HKEY hkeyHiveRoot,LPTSTR * pGroupBuffer, DWORD * pdwGroupSize);
BOOL FindGroupInList(LPTSTR pszGroupName, LPTSTR pszGroupList);
LPTSTR GetUserGroups (LPCTSTR lpServerName, LPCTSTR lpUserName, HANDLE hToken, DWORD * puEntriesRead);


//*************************************************************
//
//  ApplySystemPolicy()
//
//  Purpose:    Entry point for Windows NT4 System Policy.
//
//  Parameters: dwFlags         - Flags
//              hToken          - User's token
//              hKeyCurrentUser - Registry to the root of the user's hive
//              lpUserName      - User's name
//              lpPolicyPath    - Path to the policy file (ntconfig.pol). Can be NULL.
//              lpServerName    - Domain controller name used for group
//                                membership look up.  Can be NULL.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//  Comments:
//
//  History:    Date        Author     Comment
//              05/30/95    ericflo    Created
//              10/12/98    ericflo    Update for NT5
//
//*************************************************************

BOOL WINAPI ApplySystemPolicy (DWORD dwFlags, HANDLE hToken, HKEY hKeyCurrentUser,
                               LPCTSTR lpUserName, LPCTSTR lpPolicyPath,
                               LPCTSTR lpServerName)
{
    LONG lResult;
    BOOL bResult = FALSE;
    BOOL fFoundUser=FALSE;
    HKEY hkeyMain=NULL, hkeyRoot=NULL, hkeyUser, hkeyLogon;
    DWORD dwUpdateMode=UM_AUTOMATIC;
    DWORD dwData, dwSize;
    TCHAR szFilePath[MAX_PATH];
    TCHAR szLocalPath[MAX_PATH];
    TCHAR szTempDir[MAX_PATH];
    TCHAR szTempKey[100];
    CHAR szHandler[MAX_PATH+50];  // This needs to be ANSI
    TCHAR szComputerName[MAX_PATH];
    TCHAR szBuffer[MAX_PATH+1];
    WIN32_FILE_ATTRIBUTE_DATA fad;
    LPTSTR lpEnd;
    HANDLE hOldToken;


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ApplySystemPolicy: Entering")));



    //
    // Initialize szFilePath
    //

    szFilePath[0] = TEXT('\0');


    //
    // Check the registry to see if update is specified and get update path
    //

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            REGSTR_PATH_UPDATE,
                            0,
                            KEY_READ,
                            &hkeyMain);


    if (lResult == ERROR_SUCCESS) {


        //
        // Look for the update mode.
        //

        dwSize=sizeof(dwUpdateMode);
        if (RegQueryValueEx(hkeyMain,g_szUpdateModeValue,NULL,NULL,
                (LPBYTE) &dwUpdateMode,&dwSize) != ERROR_SUCCESS) {
                dwUpdateMode = UM_OFF;
        }


        //
        // if manual update is specified, also get the path to update from
        // (UNC path or path with drive letter)
        //


        if (dwUpdateMode==UM_MANUAL) {

            dwSize=sizeof(szTempDir);

            lResult = RegQueryValueEx(hkeyMain, g_szNetPathValue, NULL, NULL,
                                      (LPBYTE) szTempDir, &dwSize);

            if (lResult != ERROR_SUCCESS) {
                RegCloseKey(hkeyMain);
                ReportError(0, IDS_MISSINGPOLICYFILEENTRY, lResult);
                return FALSE;
            }

            ExpandEnvironmentStrings (szTempDir, szFilePath, MAX_PATH);
        }

        RegCloseKey(hkeyMain);
    }


    //
    // If this machine has policy turned off, then we can exit now.
    //

    if (dwUpdateMode == UM_OFF) {
        DebugMsg((DM_VERBOSE, TEXT("ApplySystemPolicy:  Policy is turned off on this machine.")));
        return TRUE;
    }


    //
    // If we are running in automatic mode, use the supplied
    // policy file.
    //

    if (dwUpdateMode == UM_AUTOMATIC) {

        if (lpPolicyPath && *lpPolicyPath) {
            lstrcpy (szFilePath, lpPolicyPath);
        }
    }


    //
    // If we don't have a policy file, then we can exit now.
    //

    if (szFilePath[0] == TEXT('\0')) {
        DebugMsg((DM_VERBOSE, TEXT("ApplySystemPolicy:  No Policy file.  Leaving.")));
        return TRUE;
    }

    DebugMsg((DM_VERBOSE, TEXT("ApplySystemPolicy:  PolicyPath is: <%s>."), szFilePath));


    //
    // Impersonate the user
    //

    if (!ImpersonateUser(hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy: Failed to impersonate user")));
        return FALSE;
    }


    //
    // Test if the policy file exists
    //

    if (!GetFileAttributesEx (szFilePath, GetFileExInfoStandard, &fad)) {

        lResult = GetLastError();

        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy: Failed to revert to self")));
        }


        if ( (lResult == ERROR_FILE_NOT_FOUND) ||
             (lResult == ERROR_PATH_NOT_FOUND) ) {

            DebugMsg((DM_VERBOSE, TEXT("ApplySystemPolicy:  No policy file.")));
            return TRUE;

        } else {
            DebugMsg((DM_VERBOSE, TEXT("ApplySystemPolicy:  Failed to query for policy file with error %d."), lResult));
            return FALSE;
        }
    }


    //
    //  Create a temporary file name
    //

    dwSize = ARRAYSIZE(szBuffer);

    if (!GetUserProfileDirectory(hToken, szBuffer, &dwSize)) {
        DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  Failed to query user profile directory with error %d."), GetLastError()));
        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy: Failed to revert to self")));
        }
        return FALSE;
    }


    if (!GetTempFileName (szBuffer, TEXT("prf"), 0, szLocalPath)) {
        DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  Failed to create temporary filename with error %d."), GetLastError()));
        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy: Failed to revert to self")));
        }
        return FALSE;
    }


    //
    // Copy the policy hive
    //

    if (!CopyFile(szFilePath, szLocalPath, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  Failed to copy policy file with error %d."), GetLastError()));
        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy: Failed to revert to self")));
        }
        goto Exit;
    }


    //
    // Revert to being 'ourself'
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy: Failed to revert to self")));
    }


    DebugMsg((DM_VERBOSE, TEXT("ApplySystemPolicy:  Local PolicyPath is: <%s>."), szLocalPath));


    //
    // Query for the computer name
    //

    dwSize = ARRAYSIZE(szComputerName);
    if (!GetComputerName(szComputerName, &dwSize)) {
        DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  GetComputerName failed.")));
        goto Exit;
    }



    //
    // Check to see if an installable policy handler has been added.  If
    // so, call it and let it do the work.
    //

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           g_szLogonKey,
                           0,
                           KEY_READ,
                           &hkeyLogon);


    if (lResult == ERROR_SUCCESS) {
        HANDLE hDLL = NULL;
        BOOL fRet;
        PFNPROCESSPOLICIES pfn;


        dwSize = ARRAYSIZE(szHandler);
        lResult = RegQueryValueExA(hkeyLogon,
                                   g_szPolicyHandler,
                                   NULL, NULL,
                                   (LPBYTE) szHandler,
                                   &dwSize);


        RegCloseKey(hkeyLogon);

        if (lResult == ERROR_SUCCESS) {
            LPSTR lpEntryPoint = szHandler;

            DebugMsg((DM_VERBOSE, TEXT("ApplySystemPolicy:  Machine has a custom Policy Handler of:  %S."), szHandler));

            //
            // Search for the ,
            //

            while (*lpEntryPoint && *lpEntryPoint != TEXT(',')) {
                lpEntryPoint++;
            }


            //
            // Check if we found the ,
            //

            if (*lpEntryPoint) {

                *lpEntryPoint = TEXT('\0');
                lpEntryPoint++;


                hDLL = LoadLibraryA(szHandler);

                if (hDLL) {

                    pfn = (PFNPROCESSPOLICIES) GetProcAddress(hDLL, lpEntryPoint);

                    if (pfn != NULL) {

                        //
                        // Call the function.
                        // Note that the parameters are UNICODE.
                        //

                        fRet = (*pfn) (NULL,
                                       szLocalPath,
                                       lpUserName,
                                       szComputerName,
                                       0);


                        //
                        // if callout policy downloader returns FALSE, then we don't
                        // do any processing on our own.  If it returns TRUE then we
                        // go ahead and process policies normally, in addition to whatever
                        // he may have done.
                        //

                        if (!fRet) {
                            FreeLibrary(hDLL);
                            bResult = TRUE;
                            goto Exit;
                        }

                    } else {
                       DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  Failed to find entry point %S in policy dll.  Error %d."),
                                lpEntryPoint, GetLastError()));
                    }

                    FreeLibrary(hDLL);

                } else {
                   DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  Failed to load %S with error %d."),
                            szHandler, GetLastError()));
                }
            }
        }
    }


    //
    // Load the policy hive into registry
    //

    wsprintf (szTempKey, TEXT("%s (%d)"), g_szTmpKeyName, GetTickCount());

    lResult = MyRegLoadKey(HKEY_USERS, szTempKey, szLocalPath);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  Failed to load policy hive.  Error = %d"), lResult));
        goto Exit;
    }


    //
    // Open the policy hive.
    //

    lResult = RegOpenKeyEx (HKEY_USERS,
                            szTempKey,
                            0,
                            KEY_READ,
                            &hkeyMain);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  Failed to open policy hive.  Error = %d"), lResult));
        MyRegUnLoadKey(HKEY_USERS, szTempKey);
        goto Exit;
    }



    //
    // For user and machine policies, see if there is an appropriate entry
    // in policy file (either a key with user/computer name,
    // or a default user or workstation entry).  If there is, then merge
    // information under that key into registry.  (If there isn't, it's
    // not an error-- just nothing to do.)
    //

    if (dwFlags & SP_FLAG_APPLY_USER_POLICY) {

        //
        // Merge user-specific policies if user name was specified
        //

        if (RegOpenKeyEx(hkeyMain,
                         REGSTR_KEY_POL_USERS,
                         0,
                         KEY_READ,
                         &hkeyRoot) == ERROR_SUCCESS) {


            DebugMsg((DM_VERBOSE, TEXT("ApplySystemPolicy:  Looking for user specific policy.")));

            hkeyUser = OpenUserKey(hkeyRoot, lpUserName, &fFoundUser);

            if (hkeyUser) {
                MergeRegistryData(hkeyUser,hKeyCurrentUser,szBuffer, ARRAYSIZE(szBuffer));
                RegCloseKey(hkeyUser);
            }

            RegCloseKey(hkeyRoot);
        }



        //
        // Merge group specific policies if user name specified, and we
        // *didn't* find a specific user entry above
        //

        if (!fFoundUser && lpServerName && *lpServerName) {
            HKEY hkeyGroups, hkeyGroup;
            LPTSTR GroupBuffer, ApiBuf;
            DWORD dwGroupSize = DEFAULT_GROUP_SIZE;
            DWORD uEntriesRead;


            DebugMsg((DM_VERBOSE, TEXT("ApplySystemPolicy:  Processing group(s) policy.")));

            GroupBuffer = GlobalAlloc(GPTR, DEFAULT_GROUP_SIZE * sizeof(TCHAR));

            if (GroupBuffer) {

                //
                // if there is a group processing order specified in policy hive,
                // then process groups
                //

                if (RegOpenKeyEx(hkeyMain,
                                 REGSTR_KEY_POL_USERGROUPS,
                                 0,
                                 KEY_READ,
                                 &hkeyGroups) == ERROR_SUCCESS) {


                    if (GetGroupProcessingOrder(hkeyMain, &GroupBuffer, &dwGroupSize)) {

                        //
                        // Enumerate the groups that this user belongs to
                        //

                        ApiBuf = GetUserGroups (lpServerName, lpUserName, hToken, &uEntriesRead);

                        if (ApiBuf) {

                            DebugMsg((DM_VERBOSE, TEXT("ApplySystemPolicy:  User belongs to %d groups."), uEntriesRead));

                            if (uEntriesRead) {

                                //
                                // Walk through the list of groups (ordered lowest priority
                                // to highest priority).  for each group, if the user belongs
                                // to it then download policies for that group.
                                //

                                LPTSTR pszGroup = GroupBuffer;
                                TCHAR szKeyNameBuffer[MAX_PATH+1];

                                while (*pszGroup) {

                                    //
                                    // Does user belong to this group?
                                    //

                                    if (FindGroupInList(pszGroup, ApiBuf)) {

                                        //
                                        // Open the key in the hive for this group
                                        //

                                        if (RegOpenKeyEx (hkeyGroups,
                                                          pszGroup,
                                                          0,
                                                          KEY_READ,
                                                          &hkeyGroup) == ERROR_SUCCESS) {


                                            //
                                            // Merge group policies
                                            //

                                            MergeRegistryData(hkeyGroup,
                                                              hKeyCurrentUser,
                                                              szKeyNameBuffer,
                                                              ARRAYSIZE(szKeyNameBuffer));

                                            RegCloseKey (hkeyGroup);
                                        }
                                    }

                                    pszGroup += lstrlen(pszGroup) + 1;
                                }
                            }

                            GlobalFree (ApiBuf);

                        } else {
                           DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  Failed to get user's groups.")));
                        }

                    } else {
                        DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  Failed to get group processing order.")));
                    }

                    RegCloseKey(hkeyGroups);

                } else {
                    DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  Failed to allocate memory for group policy.  Error = %d"), GetLastError()));
                }

                GlobalFree (GroupBuffer);

            } else {
                DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  Failed to allocate memory for group policy.  Error = %d"), GetLastError()));
            }
        }
    }


    if (dwFlags & SP_FLAG_APPLY_MACHINE_POLICY) {

        //
        // Merge machine-specific policies if computer name was specified
        //

        if (RegOpenKeyEx(hkeyMain,
                         REGSTR_KEY_POL_COMPUTERS,
                         0,
                         KEY_READ,
                         &hkeyRoot) == ERROR_SUCCESS) {

            DebugMsg((DM_VERBOSE, TEXT("ApplySystemPolicy:  Looking for machine specific policy.")));

            hkeyUser = OpenUserKey(hkeyRoot, szComputerName, &fFoundUser);

            if (hkeyUser) {
                MergeRegistryData(hkeyUser, HKEY_LOCAL_MACHINE, szBuffer, ARRAYSIZE(szBuffer));
                RegCloseKey(hkeyUser);
            }

            RegCloseKey(hkeyRoot);
        }
    }



    //
    // Close the policy key
    //

    RegCloseKey(hkeyMain);


    //
    // Unload the policy hive.
    //

    if (!MyRegUnLoadKey(HKEY_USERS, szTempKey)) {
        DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  Failed to unload policy hive.  Error = %d"), lResult));
        goto Exit;
    }


    //
    // Success
    //

    bResult = TRUE;

Exit:

    //
    // Delete the policy files
    //

    if (!DeleteFile (szLocalPath)) {
        DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  Failed to delete policy file <%s>.  Error %d"),
                 szLocalPath, GetLastError()));
    }

    lstrcat (szLocalPath, c_szLog);
    if (!DeleteFile (szLocalPath)) {
        DebugMsg((DM_WARNING, TEXT("ApplySystemPolicy:  Failed to delete policy log file <%s>.  Error %d"),
                 szLocalPath, GetLastError()));
    }


    DebugMsg((DM_VERBOSE, TEXT("ApplySystemPolicy:  Leaving with %d"), bResult));

    return bResult;
}


//*************************************************************
//
//  OpenUserKey()
//
//  Purpose:    Attempts to open the user specific key, or the
//              .default key.
//
//  Parameters: hkeyRoot       -   Root key
//              pszName        -   User name
//              fFoundSpecific -   Found the requested key
//
//  Return:     hkey if successful
//              NULL if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              9/13/95     ericflo    Ported
//
//*************************************************************

HKEY OpenUserKey(HKEY hkeyRoot,LPCTSTR pszName, BOOL *pfFoundSpecific)
{
    HKEY hkeyTest;
    *pfFoundSpecific = FALSE;

    //
    // See if there is a subkey under the specified key with the given
    // user name
    //

    if ((RegOpenKeyEx(hkeyRoot,
                      pszName,
                      0,
                      KEY_READ,
                      &hkeyTest)) == ERROR_SUCCESS) {

        *pfFoundSpecific = TRUE;
        DebugMsg((DM_VERBOSE, TEXT("OpenUserKey:  Found specific entry for %s ignoring .Default."), pszName));
        return hkeyTest;
    }

    //
    // If not, see if there is a default key
    //

    if ((RegOpenKeyEx(hkeyRoot,
                      REGSTR_KEY_POL_DEFAULT,
                      0,
                      KEY_READ,
                      &hkeyTest)) == ERROR_SUCCESS) {

        DebugMsg((DM_VERBOSE, TEXT("OpenUserKey:  No entry for %s, using .Default instead."), pszName));
        return hkeyTest;
    }


    //
    // No entry for this name in policy file
    //

    DebugMsg((DM_VERBOSE, TEXT("OpenUserKey:  No user/machine specific policy and no .Default policy.")));
    return NULL;
}


//*************************************************************
//
//  MergeRegistryData()
//
//  Purpose:    Merges hkeySrc and subkeys into hkeyDst.
//
//  Parameters: hkeySrc          -   Source
//              hkeyDst          -   Destination
//              pszKeyNameBuffer - Key name
//              cbKeyNameBuffer  - Size of key name buffer
//      
//
//  Return:     ERROR_SUCCESS if successful
//              otherwise an error value
//
//  Comments:
//
//  History:    Date        Author     Comment
//              9/13/95     ericflo    Ported
//
//*************************************************************

UINT MergeRegistryData(HKEY hkeySrc, HKEY hkeyDst, LPTSTR pszKeyNameBuffer,
                       UINT cbKeyNameBuffer)
{
    UINT nIndex = 0,uRet=ERROR_SUCCESS;

    //
    // Look for any subkeys of the source key
    //

    while ((uRet=RegEnumKey(hkeySrc,nIndex,pszKeyNameBuffer,
        cbKeyNameBuffer)) == ERROR_SUCCESS) {

        HKEY hkeySubkeySrc,hkeySubkeyDst;


        //
        // Create the subkey under the destination key
        //

        if ((uRet=RegCreateKey(hkeyDst,pszKeyNameBuffer,
                &hkeySubkeyDst)) != ERROR_SUCCESS)
                return uRet;

        if ((uRet=RegOpenKey(hkeySrc, pszKeyNameBuffer,
                &hkeySubkeySrc)) != ERROR_SUCCESS) {
                RegCloseKey(hkeySubkeyDst);
                return uRet;
        }


        //
        // Copy the key values from source subkey to destination subkey
        //

        uRet=CopyKeyValues(hkeySubkeySrc,hkeySubkeyDst);

        if (uRet == ERROR_SUCCESS) {

             //
             // Merge recursively on subkeys of these keys, if any
             //

             uRet = MergeRegistryData(hkeySubkeySrc,hkeySubkeyDst,pszKeyNameBuffer,
                     cbKeyNameBuffer);
        }

        RegCloseKey(hkeySubkeySrc);
        RegCloseKey(hkeySubkeyDst);

        if (uRet != ERROR_SUCCESS) {
            return uRet;
        }

        nIndex ++;
    }


    if (uRet == ERROR_NO_MORE_ITEMS) {
        uRet=ERROR_SUCCESS;
    }

    return uRet;
}

//*************************************************************
//
//  CopyKeyValues()
//
//  Purpose:    Copies all key values from hkeySrc to hkeyDst
//
//  Parameters: hkeySrc -   Source
//              hkeyDst -   destination
//
//  Return:     Error code
//
//  Comments:
//
//  History:    Date        Author     Comment
//              9/14/95     ericflo    Ported
//
//*************************************************************

UINT CopyKeyValues(HKEY hkeySrc, HKEY hkeyDst)
{
    DWORD dwSubkeyCount,dwMaxSubkeyNameLen,dwMaxClassNameLen,dwValueCount,
          dwMaxValueNameLen,dwMaxValueDataLen,dwDescriptorLen,dwClassNameLen;
    FILETIME ftLastWriteTime;
    UINT uRet=ERROR_SUCCESS;
    TCHAR szClassName[255];

    //
    // Do RegQueryInfoKey to find out if there are values for the source key,
    // and the size of value name and value data buffes to alloc
    //

    dwClassNameLen = ARRAYSIZE(szClassName);

    uRet=RegQueryInfoKey(hkeySrc,szClassName,&dwClassNameLen,NULL,&dwSubkeyCount,
            &dwMaxSubkeyNameLen,&dwMaxClassNameLen,&dwValueCount,&dwMaxValueNameLen,
            &dwMaxValueDataLen,&dwDescriptorLen,&ftLastWriteTime);

    if (uRet != ERROR_SUCCESS) {
        return uRet;
    }


    //
    // If there are values...
    //


    if (dwValueCount) {
        TCHAR  ValueName[MAX_PATH];
        LPBYTE ValueData;
        DWORD  dwType,dwValueNameSize,dwValueDataSize;
        UINT nIndex = 0;


        ValueData = GlobalAlloc (GPTR, MAX_VALUE_DATA);

        if (!ValueData) {
            return GetLastError();
        }

        //
        // the "**delvals" control code is special, must be processed
        // first; look for it now and if it exists delete all existing
        // values under this key in destination registry
        //

        if (RegQueryValueEx(hkeySrc,g_szPrefixDelvals,NULL,NULL,NULL,NULL) == ERROR_SUCCESS) {

            DeleteAllValues(hkeyDst);
        }

        //
        // Enumerate the values of the source key, and create each value
        // under the destination key
        //

        do  {
            dwValueNameSize = MAX_PATH;
            dwValueDataSize = MAX_VALUE_DATA;

            if ((uRet=RegEnumValue(hkeySrc,nIndex, ValueName,
                    &dwValueNameSize,NULL,&dwType, ValueData,
                    &dwValueDataSize)) == ERROR_SUCCESS) {

                 DWORD dwPrefix;

                 //
                 // Look for special prefixes which indicate we should treat
                 // these values specially
                 //

                 if (HasSpecialPrefix(ValueName, &dwPrefix, ValueName)) {

                     //
                     // ValueName now contains real value name stripped
                     // of prefix, filled in above by HasSpecialPrefix().
                     // Adjust value name size, the value name will shorten
                     // because the prefix has been removed.
                     //

                     dwValueNameSize = lstrlen (ValueName) + 1;

                     switch (dwPrefix) {

                         case PREFIX_DELETE:

                             //
                             // Delete this value in destination
                             //

                             RegDeleteValue(hkeyDst, ValueName);
                             uRet = ERROR_SUCCESS;
                             DebugMsg((DM_VERBOSE, TEXT("Deleted value: %s"), ValueName));
                             break;

                         case PREFIX_SOFT:

                             //
                             // "soft" value, only set this if it doesn't already
                             // exist in destination
                             //

                             {

                             TCHAR TmpValueData[MAX_PATH+1];
                             DWORD dwSize=sizeof(TmpValueData);

                             if (RegQueryValueEx(hkeyDst, ValueName,
                                     NULL,NULL,(LPBYTE) TmpValueData,
                                     &dwSize) != ERROR_SUCCESS) {

                                 //
                                 // The value doesn't exist, set the value.
                                 //

                                 uRet=RegSetValueEx(hkeyDst, ValueName, 0,
                                                    dwType, ValueData,
                                                    dwValueDataSize);

                             } else {

                                 //
                                 // Value already exists, nothing to do
                                 //

                                 uRet = ERROR_SUCCESS;
                             }

                             }

                             break;

                         case PREFIX_DELVALS:
                             // processed early on above, fall through and ignore

                         default:

                             //
                             // Got some prefix that we don't understand... presumably,
                             // from a future version.  Ignore this value, rather than
                             // propagating it into the registry, prefix and all.
                             // This will give us less backward compatibility headaches
                             // down the road.
                             //

                             uRet = ERROR_SUCCESS;   // nothing to do

                             break;
                     }
                 } else {

                     //
                     // Copy the value normally to destination key
                     //

                     uRet=RegSetValueEx(hkeyDst,ValueName,0,
                             dwType,ValueData,dwValueDataSize);

#if DBG
                     if (uRet == ERROR_SUCCESS) {

                        switch (dwType) {
                            case REG_SZ:
                            case REG_EXPAND_SZ:
                                DebugMsg((DM_VERBOSE, TEXT("CopyKeyValues: %s => %s  [OK]"),
                                         ValueName, (LPTSTR)ValueData));
                                break;

                            case REG_DWORD:
                                DebugMsg((DM_VERBOSE, TEXT("CopyKeyValues: %s => %d  [OK]"),
                                         ValueName, (DWORD)*ValueData));
                                break;

                            default:
                                DebugMsg((DM_VERBOSE, TEXT("CopyKeyValues: %s was set successfully"),
                                         ValueName));
                        }

                     } else {
                         DebugMsg((DM_WARNING, TEXT("CopyKeyValues: Failed to set %s with error %d."),
                                  ValueName, uRet));
                     }
#endif

                 }
            }

            nIndex++;

        } while (uRet == ERROR_SUCCESS);


        if (uRet == ERROR_NO_MORE_ITEMS) {
            uRet=ERROR_SUCCESS;
        }

        GlobalFree (ValueData);
    }

    return uRet;
}


//*************************************************************
//
//  HasSpecialPrefix()
//
//  Purpose:    Checks to see if szValueName has a special prefix (a la
//              "**<something>."  Returns TRUE if it does, FALSE otherwise.
//              if TRUE, returns the numerical index of the prefix in *pdwPrefix,
//              and copies the rest of value name (after the ".") into
//              szStrippedValueName.  Buffer for szStrippedValueName must be at
//              least as large as szValueName.  It is safe to pass the same
//              buffer to szValueName and szStrippedValueName and have the name
//              modified in place.
//
//  Parameters: szValueName         -   Value Name
//              pdwPrefix           -   Index of the prefix
//              szStrippedValueName -   Value name without the **
//      
//
//  Return:     TRUE if value name has a prefix
//              FALSE if it does not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              9/14/95     ericflo    Ported
//
//*************************************************************

typedef struct tagPREFIXMAP {
    const LPTSTR pszPrefix;
    DWORD dwPrefixIndex;
} PREFIXMAP;



BOOL HasSpecialPrefix(LPTSTR szValueName, DWORD * pdwPrefix,
                      LPTSTR szStrippedValueName)
{

    PREFIXMAP PrefixMap[] = {
            {g_szPrefixDel, PREFIX_DELETE},
            {g_szPrefixSoft, PREFIX_SOFT},
            {g_szPrefixDelvals, PREFIX_DELVALS}
    };
    UINT nCount,nLen;


    //
    // Does the value name begin with "**"?
    //

    if (!szValueName || (lstrlen(szValueName) < 2) ||
         szValueName[0] != TEXT('*') || szValueName[1] != TEXT('*'))

        return FALSE;   // not a special prefix


    //
    // Try all the prefixes we know to try to find a match
    //

    for (nCount = 0; nCount < ARRAYSIZE(PrefixMap); nCount++) {
         nLen = lstrlen (PrefixMap[nCount].pszPrefix);

         if (CompareString (LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                            szValueName, nLen,
                            PrefixMap[nCount].pszPrefix, nLen) == 2) {

             *pdwPrefix = PrefixMap[nCount].dwPrefixIndex;

             //
             // make a copy of the value name, sans prefix, into
             // the stripped value name buffer
             //

             lstrcpy (szStrippedValueName,szValueName + nLen);
             return TRUE;
         }
    }

    //
    // this is a prefix, but not one we know.
    //

    *pdwPrefix = PREFIX_UNKNOWN;
    lstrcpy (szStrippedValueName,szValueName);
    return TRUE;
}

//*************************************************************
//
//  GetGroupProcessingOrder()
//
//  Purpose:    Gets the list of groups in order
//
//  Parameters: hkeyHiveRoot    -   Registry key
//              GroupBuffer     -   Pointer to group buffer
//              pdwBufferSize   -   Buffer size
//
//  Return:     Number of entries if successful
//              0 if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              9/14/95     ericflo    Ported
//
//*************************************************************

BOOL GetGroupProcessingOrder(HKEY hkeyHiveRoot, LPTSTR * pGroupBuffer,
                             DWORD * pdwGroupSize)
{
    DWORD cEntries,cMaxValueName,cMaxData;
    HKEY hkeyGroupData;
    UINT uRet;
    LPTSTR GroupBuffer = *pGroupBuffer;
    LPTSTR lpTemp;
    DWORD dwGroupSize = *pdwGroupSize;
    TCHAR szValueName[10], szGroupName[48+1]; // netware groups can be up to 48 chars
    DWORD dwUsed = 0, dwSize;       // amount of buffer used
    UINT nLen,nRead=0;


    //
    // Open the group data key
    //

    uRet = RegOpenKeyEx(hkeyHiveRoot,
                        REGSTR_KEY_POL_USERGROUPDATA,
                        0,
                        KEY_READ,
                        &hkeyGroupData);

    if (uRet != ERROR_SUCCESS) {

        //
        // Group data key doesn't exist (most likely), no downloading to do
        //

        return FALSE;
    }


    //
    // Find out the number of values in group data key
    //

    if ((RegQueryInfoKey (hkeyGroupData,NULL,NULL,NULL,NULL,NULL,
            NULL,&cEntries,&cMaxValueName,&cMaxData,NULL,NULL ) != ERROR_SUCCESS) ||
            !cEntries) {

        RegCloseKey(hkeyGroupData);
        return FALSE;
    }


    //
    // The values are stored as "1"="<group name>", "2"="<group name>", etc.
    // where 1 is most important.  we will pack the names into a buffer lowest
    // priority to highest.  So if we have n values, start with value name "<n>"
    // and work down to "1".
    //

    while (cEntries) {

       wsprintf(szValueName, TEXT("%lu"), cEntries);

       dwSize = ARRAYSIZE(szGroupName);

       if (RegQueryValueEx(hkeyGroupData,szValueName,NULL,NULL,
               (LPBYTE) szGroupName,&dwSize) == ERROR_SUCCESS) {

               nLen = lstrlen(szGroupName) + 1;

               //
               // Resize buffer if neccessary (add 1 for extra terminating null)
               //

               if (nLen + dwUsed + 1 > dwGroupSize) {

                   //
                   // add a little extra so we don't realloc on every item
                   //

                   dwGroupSize = dwGroupSize + nLen + 256;

                   lpTemp = GlobalReAlloc(GroupBuffer,
                                          (dwGroupSize * sizeof(TCHAR)),
                                          GMEM_MOVEABLE);

                   if (!lpTemp) {

                       RegCloseKey(hkeyGroupData);
                       return FALSE;
                   }

                   GroupBuffer = lpTemp;
               }

               lstrcpy(GroupBuffer + dwUsed, szGroupName);
               dwUsed += nLen;
               nRead++;
       }

       cEntries --;
    }

    //
    // Doubly null-terminate buffer
    //

    *(GroupBuffer + dwUsed) = TEXT('\0');

    RegCloseKey(hkeyGroupData);

    *pGroupBuffer = GroupBuffer;
    *pdwGroupSize = dwGroupSize;

    return (nRead > 0);
}

//*************************************************************
//
//  FindGroupInList()
//
//  Purpose:    Determines if the requested group
//              is in the list of groups
//
//  Parameters: pszGroupName    -   Group looking for
//              pszGroupList    -   List of groups null seperated
//
//  Return:     TRUE if found
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              9/15/95     ericflo    Ported
//
//*************************************************************

BOOL FindGroupInList(LPTSTR pszGroupName, LPTSTR pszGroupList)
{

    while (*pszGroupList) {

        if (!lstrcmpi(pszGroupList,pszGroupName)) {
            DebugMsg((DM_VERBOSE, TEXT("FindGroupInList:  User is a member of the %s group."), pszGroupName));
            return TRUE;
        }

        pszGroupList += lstrlen(pszGroupList) + 1;
    }

    DebugMsg((DM_VERBOSE, TEXT("FindGroupInList:  User is NOT a member of the %s group."), pszGroupName));
    return FALSE;
}

//*************************************************************
//
//  GetUserGroups()
//
//  Purpose:    Retrieves a list of groups this user belongs to
//
//  Parameters: lpServerName   -  Server name
//              lpUserName     -  User name
//              hToken         -  User's token
//              puEntriesRead  -  Number of groups
//
//  Return:     Pointer to list if successful
//              Null if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              9/15/95     ericflo    Created
//
//*************************************************************

LPTSTR GetUserGroups (LPCTSTR lpServerName, LPCTSTR lpUserName,
                      HANDLE hToken, DWORD * puEntriesRead)
{
    UINT nIndex;
    NET_API_STATUS status;
    LPBYTE lpGroups, lpTemp;
    PGROUP_INFO_0  pgi0;
    DWORD dwEntriesRead, dwTotalEntries;
    DWORD cchSizeNeeded;
    LPTSTR lpGroupNames, lpName;
    PNETAPI32_API pNetAPI32;
    HANDLE hOldToken;


    //
    // Load netapi32
    //

    pNetAPI32 = LoadNetAPI32();
    
    if (!pNetAPI32) {
        DebugMsg((DM_WARNING, TEXT("GetUserGroups:  Failed to load netapi32 with %d."),
                 GetLastError()));
        return NULL;
    }


    //
    // Impersonate the user
    //

    if (!ImpersonateUser(hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("GetUserGroups: Failed to impersonate user")));
        return NULL;
    }


    //
    // Query for the groups
    //

    status = pNetAPI32->pfnNetUserGetGroups (lpServerName, lpUserName,
                                           0, &lpGroups, 0xFFFFFFFF, &dwEntriesRead,
                                           &dwTotalEntries);

    if (status == NERR_Success) {

        //
        // NetUserGetGroups opens a named pipe to the server.  To close
        // it, we need to call NetUserGetInfo on the local machine
        //

        if (pNetAPI32->pfnNetUserGetInfo (NULL, lpUserName,
                                        0, &lpTemp) == NERR_Success) {
            pNetAPI32->pfnNetApiBufferFree (lpTemp);
        }


    } else {
        DebugMsg((DM_VERBOSE, TEXT("GetUserGroups: NetUserGetGroups failed with %d"), status));
        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("GetUserGroups: Failed to revert to self")));
        }
        return NULL;
    }


    //
    // Revert to self
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("GetUserGroups: Failed to revert to self")));
    }


    //
    // NetUserGetGroups returns names packed in structures with fixed-length
    // fields.  Need to copy that into caller's buffer packed with the names
    // packed end-to-end.
    //
    // Count the total buffer size we need, which will be smaller than the
    // API buffer to NetUserGetGroups because we're not using fixed-length
    // fields
    //

    cchSizeNeeded = 1;
    pgi0 = (PGROUP_INFO_0) lpGroups;

    for (nIndex=0; nIndex < dwEntriesRead; nIndex++) {

         cchSizeNeeded += lstrlen(pgi0->grpi0_name) + 1;
         pgi0++;
    }

    *puEntriesRead = dwEntriesRead;

    //
    // Build the list of group names
    //

    lpGroupNames = GlobalAlloc (GPTR, cchSizeNeeded * sizeof (TCHAR));

    if (!lpGroupNames) {
        pNetAPI32->pfnNetApiBufferFree (lpGroups);
        return NULL;
    }

    DebugMsg((DM_VERBOSE, TEXT("GetUserGroups: User is a member of the following global groups:")));

    lpName = lpGroupNames;
    pgi0 = (PGROUP_INFO_0) lpGroups;

    for (nIndex=0; nIndex < dwEntriesRead; nIndex++) {

         DebugMsg((DM_VERBOSE, TEXT("GetUserGroups:     %s"), pgi0->grpi0_name));
         lstrcpy (lpName, pgi0->grpi0_name);
         lpName += lstrlen(pgi0->grpi0_name) + 1;
         pgi0++;
    }

    //
    // Free the memory allocated by NetUserGetGroups
    //

    pNetAPI32->pfnNetApiBufferFree (lpGroups);

    return lpGroupNames;
}
