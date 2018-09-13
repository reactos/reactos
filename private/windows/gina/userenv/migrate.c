//*************************************************************
//
//  User Profile migration routines
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//*************************************************************

#include "uenv.h"

UINT CountItems (LPTSTR lpDirectory);

BOOL SearchAndReplaceIEHistory(LPTSTR szIEHistKeyRoot, LPTSTR szHistOld, LPTSTR szHistNew);


//*************************************************************
//
//  DetermineLocalSettingsLocation()
//
//  Purpose:    Determines where to put the local settings
//
//  Parameters: none
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
// Look at all the shell folders and see which of them are expected to go to
// local settings on nt5. Some of them might already be moved to random
// locations based on the localisations. We should figure out where it is
// pointing to by looking at these locations and make a call
//*************************************************************


BOOL DetermineLocalSettingsLocation(LPTSTR szLocalSettings)
{   
    TCHAR  szPath[MAX_PATH];
    LPTSTR lpEnd, lpBgn;
    HKEY hKey, hKeyRoot;
    DWORD dwDisp, dwSize, dwType, i;

    if (RegOpenCurrentUser(KEY_READ | KEY_WRITE, &hKeyRoot) == ERROR_SUCCESS) {

        if (RegOpenKeyEx(hKeyRoot, USER_SHELL_FOLDERS,
                            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                            

            for (i=0; i < g_dwNumShellFolders; i++) {            
                if (c_ShellFolders[i].bNewNT5 && c_ShellFolders[i].bLocalSettings) {

                    dwSize = sizeof(szPath);

                    if (RegQueryValueEx(hKey, c_ShellFolders[i].lpFolderName,
                                        0, &dwType, (LPBYTE)szPath, &dwSize) == ERROR_SUCCESS) {

                        if (lstrlen(szPath) > lstrlen(TEXT("%userprofile%"))) {

                            DebugMsg((DM_VERBOSE, TEXT("DetermineLocalSettingsLocation: Considering shell folder %s"), szPath));

                            //
                            // Move the pointer upto the next slash
                            //
                            
                            lpBgn = szPath + lstrlen(TEXT("%userprofile%"))+1;
                            lpEnd = lpBgn;
                            
                            for (;(*lpEnd != TEXT('\0'));lpEnd++) {

                                //
                                // we have found a shellfolder of the form %userprofile%\subdir\xxx
                                // assume this subdir as the localsettings path
                                //
                                
                                if (( (*lpEnd) == TEXT('\\') ) && ( (*(lpEnd+1)) != TEXT('\0')) )
                                    break;
                            }

                            
                            if ((*lpEnd == TEXT('\\')) && (*(lpEnd+1) != TEXT('\0'))) {
                                *lpEnd = TEXT('\0');
                                lstrcpy(szLocalSettings, lpBgn);

                                DebugMsg((DM_VERBOSE, TEXT("DetermineLocalSettingsLocation: Assuming %s to be the local settings directory"), lpBgn));                                                        
                                return TRUE;
                            }
                            
                        }
                    }
                }                    
            }                              

            RegCloseKey(hKey);
        }
        
        RegCloseKey(hKeyRoot);
    }

    //
    // otherwise load it from the rc file
    //
    
    LoadString (g_hDllInstance, IDS_SH_LOCALSETTINGS, szLocalSettings, MAX_FOLDER_SIZE);
    DebugMsg((DM_VERBOSE, TEXT("DetermineLocalSettingsLocation: No Local Settings was found, using %s"), szLocalSettings));                                                        
    
    return TRUE;
}


//*************************************************************
//
//  MigrateNT4ToNT5()
//
//  Purpose:    Migrates a user profile from NT4 to NT5
//
//  Parameters: none
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL WINAPI MigrateNT4ToNT5 (void)
{
    TCHAR szTemp[MAX_PATH];
    TCHAR szTemp2[MAX_PATH];
    TCHAR szTemp3[MAX_PATH];
    TCHAR szLocalSettings[MAX_PATH];
    LPTSTR lpEnd, lpEnd1, lpBgn=NULL;
    HKEY hKey, hKeyRoot = HKEY_CURRENT_USER;
    DWORD dwDisp, dwSize, dwType, i;
    BOOL bSetTemp = TRUE;
    BOOL bCleanUpTemp = FALSE;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    const LPTSTR szUserProfile = TEXT("%USERPROFILE%\\");


    //
    // Get the root registry key handle
    //

    if (RegOpenCurrentUser(KEY_READ | KEY_WRITE, &hKeyRoot) != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("MigrateNT4ToNT5: Failed to get root registry key with %d"),
                 GetLastError()));
    }


    //
    // Convert Personal to My Documents
    //
    // We have to be careful with this directory.  We'll rename
    // Personal to My Documents only if the Personal directory
    // is empty.  After this, we'll fix up the registry special folder
    // location only if it is still pointing at the default Personal location.
    //

    lstrcpy (szTemp, szUserProfile);
    
    if ( LoadString (g_hDllInstance, IDS_SH_PERSONAL2, szTemp2, ARRAYSIZE(szTemp2)) )
    {
        lstrcat (szTemp, szTemp2);

        ExpandEnvironmentStrings (szTemp, szTemp2, ARRAYSIZE(szTemp2));


        //
        // Check if the personal directory exists
        //

        if (GetFileAttributesEx (szTemp2, GetFileExInfoStandard, &fad)) {

            //
            // Check if the personal directory is empty
            //

            if (!CountItems (szTemp2)) {

                //
                // The directory is empty, so rename it to My Documents
                //

                LoadString (g_hDllInstance, IDS_SH_PERSONAL, szTemp3, ARRAYSIZE(szTemp3));
                lstrcpy (szTemp, szUserProfile);
                lstrcat (szTemp, szTemp3);
                ExpandEnvironmentStrings (szTemp, szTemp3, ARRAYSIZE(szTemp3));

                if (MoveFileEx(szTemp2, szTemp3, 0)) {

                    //
                    // Now we need to fix up the registry value if it is still set
                    // to the default of %USERPROFILE%\Personal
                    //

                    if (RegOpenKeyEx (hKeyRoot, USER_SHELL_FOLDERS,
                                  0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {

                        dwSize = sizeof(szTemp3);
                        szTemp3[0] = TEXT('\0');
                        if (RegQueryValueEx (hKey, TEXT("Personal"), NULL, &dwType,
                                         (LPBYTE) szTemp3, &dwSize) == ERROR_SUCCESS) {

                            LoadString (g_hDllInstance, IDS_SH_PERSONAL2, szTemp2, ARRAYSIZE(szTemp2));
                            lstrcpy (szTemp, szUserProfile);
                            lstrcat (szTemp, szTemp2);

                            if (lstrcmpi(szTemp, szTemp3) == 0) {

                                LoadString (g_hDllInstance, IDS_SH_PERSONAL, szTemp3, ARRAYSIZE(szTemp3));
                                lstrcpy (szTemp, szUserProfile);
                                lstrcat (szTemp, szTemp3);
    
                                RegSetValueEx (hKey, TEXT("Personal"), 0, REG_EXPAND_SZ,
                                            (LPBYTE) szTemp, (lstrlen(szTemp) + 1) * sizeof(TCHAR));


                                //
                                // We need to reinitialize the global variables now because
                                // the path to the My Documents and My Pictures folder has changed.
                                //

                                InitializeGlobals(g_hDllInstance);
                            }
                        }
    
                        RegCloseKey (hKey);
                    }
                }
            }
        }
    }


    //
    // Get the user profile directory
    //

    GetEnvironmentVariable (TEXT("USERPROFILE"), szTemp, ARRAYSIZE (szTemp));

    DebugMsg((DM_VERBOSE, TEXT("MigrateNT4ToNT5: Upgrading <%s> from NT4 to NT5"),
             szTemp));


    //
    // Hide ntuser.dat and ntuser.dat.log
    //

    lpEnd = CheckSlash (szTemp);
    lstrcpy (lpEnd, TEXT("ntuser.dat"));
    SetFileAttributes (szTemp, FILE_ATTRIBUTE_HIDDEN);

    lstrcpy (lpEnd, TEXT("ntuser.dat.log"));
    SetFileAttributes (szTemp, FILE_ATTRIBUTE_HIDDEN);


    DetermineLocalSettingsLocation(szLocalSettings);


    //
    // Check if Temporary Internet Files exists in the root of the
    // user's profile.  If so, move it to the new location
    //
    // migrate these stuff before we nuke the old User_shell_folders
    //

    if (RegOpenKeyEx(hKeyRoot, USER_SHELL_FOLDERS,
                        0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = ARRAYSIZE(szTemp);
        
        if (RegQueryValueEx(hKey, TEXT("Cache"), 0, &dwType, (LPBYTE)szTemp, &dwSize) != ERROR_SUCCESS) {

            //
            // if this value is not there go by the default location from the
            // resources
            //
            
            LoadString (g_hDllInstance, IDS_TEMPINTERNETFILES, szTemp2, ARRAYSIZE(szTemp2));

            lstrcpy (szTemp, szUserProfile);
            lstrcat (szTemp, szTemp2);
        }

        
        ExpandEnvironmentStrings (szTemp, szTemp2, ARRAYSIZE(szTemp2));

        if (GetFileAttributesEx (szTemp2, GetFileExInfoStandard, &fad)) {

            LoadString (g_hDllInstance, IDS_SH_CACHE, szTemp3, ARRAYSIZE(szTemp3));
            lstrcpy (szTemp, szUserProfile);

            //
            // append the newly found localsettings
            //
            
            lstrcat (szTemp, szLocalSettings);
            CheckSlash(szTemp);
            lstrcat (szTemp, szTemp3);
            ExpandEnvironmentStrings (szTemp, szTemp3, ARRAYSIZE(szTemp3));


            DebugMsg((DM_VERBOSE, TEXT("MigrateNT4ToNT5: New temp int files folder (expand path) %s"), szTemp3));    
            DebugMsg((DM_VERBOSE, TEXT("MigrateNT4ToNT5: Old temp int files folder (expand path) %s"), szTemp2));                                                        

            if (lstrcmpi(szTemp2, szTemp3) != 0) {
                if (CopyProfileDirectory (szTemp2, szTemp3, CPD_IGNOREHIVE)) {
                    Delnode (szTemp2);
                }
            }
        }


        //
        // Check if History exists in the root of the user's profile.
        // If so, move it to the new location
        //

        dwSize = ARRAYSIZE(szTemp);
        
        if (RegQueryValueEx(hKey, TEXT("History"), 0, &dwType, (LPBYTE)szTemp, &dwSize) != ERROR_SUCCESS) {

            //
            // if this value is not there go by the default location from the
            // resources
            //

            LoadString (g_hDllInstance, IDS_HISTORY, szTemp2, ARRAYSIZE(szTemp2));

            lstrcpy (szTemp, szUserProfile);
            lstrcat (szTemp, szTemp2);
        }

        
        ExpandEnvironmentStrings (szTemp, szTemp2, ARRAYSIZE(szTemp2));

        if (GetFileAttributesEx (szTemp2, GetFileExInfoStandard, &fad)) {

            LoadString (g_hDllInstance, IDS_SH_HISTORY, szTemp3, ARRAYSIZE(szTemp3));
            lstrcpy (szTemp, szUserProfile);
            
            //
            // append the newly found localsettings
            //
            
            lstrcat (szTemp, szLocalSettings);
            CheckSlash(szTemp);

            
            lstrcat (szTemp, szTemp3);
            ExpandEnvironmentStrings (szTemp, szTemp3, ARRAYSIZE(szTemp3));
            
            DebugMsg((DM_VERBOSE, TEXT("MigrateNT4ToNT5: New histroy folder (expand path) %s"), szTemp3));    
            DebugMsg((DM_VERBOSE, TEXT("MigrateNT4ToNT5: Old histroy folder (expand path) %s"), szTemp2));                                                        

            
            if (lstrcmpi(szTemp2, szTemp3) != 0) {
                if (CopyProfileDirectory (szTemp2, szTemp3, CPD_IGNOREHIVE)) {
                    Delnode (szTemp2);
                    SearchAndReplaceIEHistory(IE4_CACHE_KEY, szTemp2, szTemp3);
                    SearchAndReplaceIEHistory(IE5_CACHE_KEY, szTemp2, szTemp3);
                }
            }
        }

        RegCloseKey(hKey);
    }    


    //
    // Update the local settings key with the new value
    //

    lstrcpy (szTemp, TEXT("%USERPROFILE%"));
    lpEnd = CheckSlash (szTemp);
    
    if (RegCreateKeyEx (hKeyRoot, USER_SHELL_FOLDERS,
                        0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE, NULL, &hKey,
                        &dwDisp) == ERROR_SUCCESS) {

        lstrcpy(lpEnd, szLocalSettings);                        

        RegSetValueEx (hKey, TEXT("Local Settings"),
                     0, REG_EXPAND_SZ, (LPBYTE) szTemp,
                     ((lstrlen(szTemp) + 1) * sizeof(TCHAR)));

        RegCloseKey (hKey);
    }                        

    DebugMsg((DM_VERBOSE, TEXT("MigrateNT4ToNT5: Update the local settings folder with %s"), szTemp));    


    //
    // Globals needs to reinitialised because LocalSettings might be different from
    // the one specified in the rc file
    //
    
    InitializeGlobals(g_hDllInstance);
    

    //
    // Get the user profile directory
    //

    GetEnvironmentVariable (TEXT("USERPROFILE"), szTemp, ARRAYSIZE (szTemp));

    DebugMsg((DM_VERBOSE, TEXT("MigrateNT4ToNT5: Upgrading <%s> from NT4 to NT5"),
             szTemp));

    lpEnd = CheckSlash (szTemp);


    //
    // Create the new special folders
    //

    for (i=0; i < g_dwNumShellFolders; i++) {

        if (c_ShellFolders[i].bNewNT5) {

            lstrcpy (lpEnd, c_ShellFolders[i].lpFolderLocation);

            if (!CreateNestedDirectory(szTemp, NULL)) {
                DebugMsg((DM_WARNING, TEXT("MigrateNT4ToNT5: Failed to create the destination directory <%s>.  Error = %d"),
                         szTemp, GetLastError()));
            }

            if (c_ShellFolders[i].bHidden) {
                SetFileAttributes(szTemp, FILE_ATTRIBUTE_HIDDEN);
            } else {
                SetFileAttributes(szTemp, FILE_ATTRIBUTE_NORMAL);
            }
        }
    }


    //
    // Set the new special folders in the User Shell Folder registry key
    //

    lstrcpy (szTemp, TEXT("%USERPROFILE%"));
    lpEnd = CheckSlash (szTemp);

    if (RegCreateKeyEx (hKeyRoot, USER_SHELL_FOLDERS,
                        0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE, NULL, &hKey,
                        &dwDisp) == ERROR_SUCCESS) {

        for (i=0; i < g_dwNumShellFolders; i++) {

            if (c_ShellFolders[i].bNewNT5 && c_ShellFolders[i].bAddCSIDL) {
                lstrcpy (lpEnd, c_ShellFolders[i].lpFolderLocation);

                RegSetValueEx (hKey, c_ShellFolders[i].lpFolderName,
                             0, REG_EXPAND_SZ, (LPBYTE) szTemp,
                             ((lstrlen(szTemp) + 1) * sizeof(TCHAR)));
            }
        }

        RegCloseKey (hKey);
    }



    //
    // Query the user's environment for a TEMP environment variable.
    //

    if (RegCreateKeyEx (hKeyRoot, TEXT("Environment"), 0,
                        NULL, REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE, NULL, &hKey,
                        &dwDisp) == ERROR_SUCCESS) {


        szTemp2[0] = TEXT('\0');
        dwSize = sizeof(szTemp2);
        RegQueryValueEx (hKey, TEXT("TEMP"), NULL, &dwType,
                         (LPBYTE) szTemp2, &dwSize);


        //
        // Decide if we should set the temp and tmp environment variables.
        // We need to be careful to not blast someone's custom temp variable
        // if it already exists, but at the same time it's ok to remap this if
        // temp is still set to the NT4 default of %SystemDrive%\TEMP.
        //

        if (szTemp2[0] != TEXT('\0')) {
            if (lstrcmpi (szTemp2, TEXT("%SystemDrive%\\TEMP")) != 0) {
                bSetTemp = FALSE;
            }

            if (lstrcmpi (szTemp2, TEXT("%USERPROFILE%\\TEMP")) == 0) {
                bSetTemp = TRUE;
                bCleanUpTemp = TRUE;
            }
        }


        if (bSetTemp) {
            LoadString (g_hDllInstance, IDS_SH_TEMP, szTemp2, ARRAYSIZE(szTemp2));
            lstrcpy(lpEnd, szLocalSettings);
            CheckSlash(lpEnd);
            lstrcat (lpEnd, szTemp2);

            DebugMsg((DM_VERBOSE, TEXT("MigrateNT4ToNT5: Setting the Temp directory to <%s>"), szTemp));
            

            RegSetValueEx (hKey, TEXT("TEMP"), 0, REG_EXPAND_SZ,
                           (LPBYTE) szTemp, (lstrlen (szTemp) + 1) * sizeof(TCHAR));

            RegSetValueEx (hKey, TEXT("TMP"), 0, REG_EXPAND_SZ,
                           (LPBYTE) szTemp, (lstrlen (szTemp) + 1) * sizeof(TCHAR));
        }

        if (bCleanUpTemp) {
            ExpandEnvironmentStrings (szTemp, szTemp2, ARRAYSIZE(szTemp2));
            ExpandEnvironmentStrings (TEXT("%USERPROFILE%\\TEMP"), szTemp, ARRAYSIZE(szTemp));

            if (CopyProfileDirectory (szTemp, szTemp2, CPD_IGNOREHIVE)) {
                Delnode (szTemp);
            }
        }

        RegCloseKey (hKey);
    }


    //
    // Migrate the Template Directory if it exists. Copy it from %systemroot%\shellnew
    // to Templates directory userprofile..
    // 

    if ((LoadString (g_hDllInstance, IDS_SH_TEMPLATES2, szTemp2, ARRAYSIZE(szTemp2))) && 
            (ExpandEnvironmentStrings (szTemp2, szTemp3, ARRAYSIZE(szTemp3))) &&
            (LoadString (g_hDllInstance, IDS_SH_TEMPLATES, szTemp2, ARRAYSIZE(szTemp2)))) {

        //
        // if all of the above succeeded
        // szTemp3 will have the full path for the old templates dir..
        //

        lstrcpy (szTemp, szUserProfile);
        lstrcat (szTemp, szTemp2);
        
        ExpandEnvironmentStrings (szTemp, szTemp2, ARRAYSIZE(szTemp2));

        if (GetFileAttributesEx (szTemp3, GetFileExInfoStandard, &fad)) {
            DebugMsg((DM_VERBOSE, TEXT("MigrateNT4ToNT5: Copying Template files from %s to %s"), szTemp3, szTemp2));
            CopyProfileDirectory(szTemp3, szTemp2, CPD_IGNORECOPYERRORS | CPD_IGNOREHIVE);
        }
    }


    //
    // Set the user preference exclusion list.  This will
    // prevent the Local Settings folder from roaming
    //

    if (LoadString (g_hDllInstance, IDS_EXCLUSIONLIST, 
                    szTemp, ARRAYSIZE(szTemp))) {

        if (RegCreateKeyEx (hKeyRoot, WINLOGON_KEY,
                            0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WRITE, NULL, &hKey,
                            &dwDisp) == ERROR_SUCCESS) {


            dwSize = sizeof(szTemp);

            RegQueryValueEx (hKey, TEXT("ExcludeProfileDirs"),
                                 NULL, &dwType, (LPBYTE) szTemp,
                                 &dwSize);

            //
            // Read in the value of the local settings
            //
            
            LoadString (g_hDllInstance, IDS_SH_LOCALSETTINGS, 
                    szTemp2, ARRAYSIZE(szTemp2));

            
            //
            // Loop through the list
            //

            lpBgn = lpEnd = szTemp;                    
            *szTemp3 = TEXT('\0');
            
            while (*lpEnd) {

                //
                // Look for the semicolon separator
                //

                while (*lpEnd && ((*lpEnd) != TEXT(';'))) {
                    lpEnd++;
                }


                //
                // Remove any leading spaces
                //

                while (*lpBgn == TEXT(' ')) {
                    lpBgn++;
                }
                

                //
                // if it has come here we are going to attach something
                // to the end of szTmp3
                //


                if (*szTemp3)
                    lstrcat(szTemp3, TEXT(";"));
                    
                if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                   lpBgn, (INT)(lpEnd-lpBgn), szTemp2, lstrlen(szTemp2)) != CSTR_EQUAL) {

                    lstrcpyn(szTemp3+lstrlen(szTemp3), lpBgn, (INT)(lpEnd-lpBgn+1));
                }
                else {
                    lstrcat(szTemp3, szLocalSettings);                                   
                }
                
                
                //
                // If we are at the end of the exclusion list, we're done
                //

                if (!(*lpEnd)) {
                    break;
                }


                //
                // Prep for the next entry
                //

                lpEnd++;
                lpBgn = lpEnd;
            }

        
            RegSetValueEx (hKey, TEXT("ExcludeProfileDirs"),
                           0, REG_SZ, (LPBYTE) szTemp3,
                           ((lstrlen(szTemp3) + 1) * sizeof(TCHAR)));

            DebugMsg((DM_VERBOSE, TEXT("MigrateNT4ToNT5: Set the exclusionlist value to default")));

            RegCloseKey (hKey);
        }
    }


    //
    // Make sure the hidden bit is set correctly for each special folder
    //

    if (RegOpenKeyEx (hKeyRoot, USER_SHELL_FOLDERS,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        for (i=0; i < g_dwNumShellFolders; i++) {

            dwSize = sizeof(szTemp);
            szTemp[0] = TEXT('\0');

            if (RegQueryValueEx (hKey, c_ShellFolders[i].lpFolderName,
                                 NULL, &dwType, (LPBYTE) szTemp,
                                  &dwSize) == ERROR_SUCCESS) {

                ExpandEnvironmentStrings (szTemp, szTemp2, ARRAYSIZE(szTemp2));

                if (c_ShellFolders[i].bHidden) {
                    SetFileAttributes(szTemp2, FILE_ATTRIBUTE_HIDDEN);
                } else {
                    SetFileAttributes(szTemp2, FILE_ATTRIBUTE_NORMAL);
                }
            }
        }

        RegCloseKey (hKey);
    }


    if (hKeyRoot != HKEY_CURRENT_USER) {
        RegCloseKey (hKeyRoot);
    }

    DebugMsg((DM_VERBOSE, TEXT("MigrateNT4ToNT5: Finished.")));

    return TRUE;
}

//*************************************************************
//
//  ResetUserSpecialFolderPaths()
//
//  Purpose:    Sets all of the user special folder paths back
//              to their defaults
//
//  Parameters: none
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL WINAPI ResetUserSpecialFolderPaths(void)
{
    TCHAR szDirectory [MAX_PATH];
    HKEY hKey, hKeyRoot;
    DWORD dwDisp, i;
    LPTSTR lpEnd;


    //
    // Set the User Shell Folder paths in the registry
    //

    lstrcpy (szDirectory, TEXT("%USERPROFILE%"));
    lpEnd = CheckSlash (szDirectory);

    if (RegOpenCurrentUser(KEY_WRITE, &hKeyRoot) == ERROR_SUCCESS) {

        if (RegCreateKeyEx (hKeyRoot, USER_SHELL_FOLDERS,
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
        RegCloseKey (hKeyRoot);
    }

    return TRUE;
}

//*************************************************************
//
//  CountItems()
//
//  Purpose:    Counts the number of files and subdirectories
//              in the given subdirectory
//
//  Parameters: lpDirectory - parent directory
//
//  Return:     Item count
//
//*************************************************************

UINT CountItems (LPTSTR lpDirectory)
{
    TCHAR szDirectory[MAX_PATH];
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    UINT uiCount = 0;


    //
    // Search through the directory
    //

    lstrcpy (szDirectory, lpDirectory);
    lstrcat (szDirectory, TEXT("\\*.*"));

    hFile = FindFirstFile(szDirectory, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {
        return uiCount;
    }


    do {

        //
        // Check for "." and ".."
        //

        if (!lstrcmpi(fd.cFileName, TEXT("."))) {
            continue;
        }

        if (!lstrcmpi(fd.cFileName, TEXT(".."))) {
            continue;
        }

        uiCount++;

        //
        // Find the next entry
        //

    } while (FindNextFile(hFile, &fd));


    FindClose(hFile);

    return uiCount;
}


//*************************************************************
//
//  SearchAndReplaceIEHistory()
//
//  Purpose:    Searches and Replaces the registry kesy pointing to 
//              the old location in IE Cahe to point to New Location
//
//  Parameters: 
//          szIEHistKeyRoot     - Root of the history key
//          szHistOld           - Old History Key
//          szHistNew           - New Location of History Key
//
//  Return:     TRUE if success, else False
//  
//  Created:
//
// Notes:
//      Change the "HKCU\S\M\W\CV\Internet Settings\Cache\Extensible Cache\"MSHist***\CachePath" and
//      Change the "HKCU\S\M\W\CV\Internet Settings\5.0\Cache\Extensible Cache\MSHis***\CachePath"  
//      value to the new place
//*************************************************************

BOOL SearchAndReplaceIEHistory(LPTSTR szIEHistKeyRoot, LPTSTR szHistOld, LPTSTR szHistNew)
{
    DWORD dwIndex = 0, dwMsHistLen, dwLen;
    TCHAR szSubKey[MAX_PATH+1], szSubKey1[MAX_PATH+1];
    TCHAR szCachePath[MAX_PATH];
    TCHAR szCachePath1[MAX_PATH];
    FILETIME ftWrite;
    HKEY hIECacheKey, hKey;

    DebugMsg((DM_VERBOSE, TEXT("SearchAndReplaceIEHistory: Fixing up the IE Registry keys")));
        
    if (RegOpenKeyEx(HKEY_CURRENT_USER, szIEHistKeyRoot, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {


        DebugMsg((DM_VERBOSE, TEXT("SearchAndReplaceIEHistory: Enumerating the keys under %s"), szIEHistKeyRoot));

        dwMsHistLen = lstrlen(IE_CACHEKEY_PREFIX);

        dwLen = ARRAYSIZE(szSubKey);
        
        while (RegEnumKeyEx(hKey, dwIndex, szSubKey, &dwLen, NULL, NULL, NULL, &ftWrite) == ERROR_SUCCESS) {

            DebugMsg((DM_VERBOSE, TEXT("SearchAndReplaceIEHistory: Testing Key %s"), szSubKey));
                
            lstrcpy(szSubKey1, szSubKey);
            szSubKey1[dwMsHistLen] = TEXT('\0');
                
            //
            // if the key name starts with MSHist
            //
            

            if (lstrcmpi(szSubKey1, IE_CACHEKEY_PREFIX) == 0) {

                if (RegOpenKeyEx(hKey, szSubKey, 0, KEY_ALL_ACCESS, &hIECacheKey) == ERROR_SUCCESS) {
                    DWORD dwLen1;

                    //
                    // Get the current value
                    //


                    dwLen1 = ARRAYSIZE(szCachePath);
                    if (RegQueryValueEx(hIECacheKey, TEXT("CachePath"), 0, NULL, (LPBYTE)szCachePath, &dwLen1) == ERROR_SUCCESS) {

                        //
                        // Replace the szHistOld prefix with szHistNew value
                        //
                        
                        lstrcpy(szCachePath1, szHistNew);

                        lstrcpy(szSubKey1, szCachePath);
                        szSubKey1[lstrlen(szHistOld)] = TEXT('\0');
                        
                        if (lstrcmpi(szSubKey1, szHistOld) == 0) {


                            lstrcat(szCachePath1, szCachePath+lstrlen(szHistOld));
                                
                            RegSetValueEx(hIECacheKey, TEXT("CachePath"), 0, REG_SZ, (LPBYTE)szCachePath1, (lstrlen(szCachePath1)+1)*sizeof(TCHAR));

                            DebugMsg((DM_VERBOSE, TEXT("SearchAndReplaceIEHistory: Finally,  under %s Replacing %s with %s"), szSubKey, szCachePath, szCachePath1));
                                
                        } 
                        else {
                            DebugMsg((DM_VERBOSE, TEXT("SearchAndReplaceIEHistory: Existing CachePath %s doesn't have %s, skipping.."), szCachePath, szHistOld));
                        }                            
                    }
                    else {
                        DebugMsg((DM_VERBOSE, TEXT("SearchAndReplaceIEHistory: Could not open CachePath value")));
                    }
                        
                    RegCloseKey(hIECacheKey);
                }
                else {
                    DebugMsg((DM_VERBOSE, TEXT("SearchAndReplaceIEHistory: Could not open %s subkey"), szSubKey));
                }
           }
           else {
               DebugMsg((DM_VERBOSE, TEXT("SearchAndReplaceIEHistory: %s subkey does not have %s prefix"), szSubKey, IE_CACHEKEY_PREFIX));
           }
                
           dwIndex++;
           dwLen = ARRAYSIZE(szSubKey);

          DebugMsg((DM_VERBOSE, TEXT("SearchAndReplaceIEHistory: Getting %d subkey next.."), dwIndex+1));
           
        }

        //
        // Close if the open succeeded
        //
        
        RegCloseKey(hKey);
    }
    else {
          DebugMsg((DM_VERBOSE, TEXT("SearchAndReplaceIEHistory: Failed to open the root of the key %s"), szIEHistKeyRoot));
    }

    return TRUE;
}
