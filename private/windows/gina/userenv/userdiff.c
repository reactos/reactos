//*************************************************************
//
//  Userdiff.c
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"

#define MAX_KEY_NAME    MAX_PATH

BOOL AddUDNode (LPUDNODE *lpList, LPTSTR lpBuildNumber);
BOOL FreeUDList (LPUDNODE lpList);
BOOL ProcessBuild(LPPROFILE lpProfile, LPUDNODE lpItem);
BOOL ProcessHive(LPPROFILE lpProfile, LPUDNODE lpItem, HKEY hKey);
BOOL ProcessFiles(LPPROFILE lpProfile, LPUDNODE lpItem, HKEY hKey);
BOOL ProcessPrograms(LPPROFILE lpProfile, LPUDNODE lpItem, HKEY hKey);
BOOL OkToProcessItem(DWORD dwProductType);

//*************************************************************
//
//  ProcessUserDiff()
//
//  Purpose:    Processes the userdiff hive
//
//  Parameters: lpProfile   -   Profile information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/2/95     ericflo    Created
//
//*************************************************************

BOOL ProcessUserDiff (LPPROFILE lpProfile, DWORD dwBuildNumber)
{
    TCHAR szUserDiff[MAX_PATH] = {0};
    TCHAR szName[MAX_KEY_NAME];
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    LPUDNODE lpList = NULL, lpItem;
    LONG lResult;
    HKEY hKeyUserDiff;
    UINT Index = 0;
    DWORD dwSize;
    FILETIME ftWrite;


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ProcessUserDiff:  Entering.")));


    //
    // Test if the hive exists, first look for USERDIFR
    //

    ExpandEnvironmentStrings(USERDIFR_LOCATION, szUserDiff, MAX_PATH);
    hFile = FindFirstFile (szUserDiff, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessUserDiff:  userdifr hive doesn't exist.  Trying userdiff.")));

        ExpandEnvironmentStrings(USERDIFF_LOCATION, szUserDiff, MAX_PATH);
        hFile = FindFirstFile (szUserDiff, &fd);

        if (hFile == INVALID_HANDLE_VALUE) {
            DebugMsg((DM_WARNING, TEXT("ProcessUserDiff:  userdiff hive doesn't exist.  Leaving.")));
            return TRUE;
        }
    }

    FindClose (hFile);


    //
    // Load the hive
    //

    if (MyRegLoadKey(HKEY_USERS, USERDIFF, szUserDiff) != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("ProcessUserDiff:  Failed to load userdiff.")));
        return FALSE;
    }


    //
    // Open the key
    //

    lResult = RegOpenKeyEx(HKEY_USERS, USERDIFF, 0, KEY_READ, &hKeyUserDiff);

    if (lResult != ERROR_SUCCESS) {
        MyRegUnLoadKey(HKEY_USERS, USERDIFF);
        DebugMsg((DM_WARNING, TEXT("ProcessUserDiff:  failed to open registry root (%d)"), lResult));
        return FALSE;
    }



    //
    // Enumerate the build numbers
    //

    dwSize = MAX_KEY_NAME;
    lResult = RegEnumKeyEx(hKeyUserDiff, Index, szName, &dwSize, NULL,
                           NULL, NULL, &ftWrite);

    if (lResult == ERROR_SUCCESS) {

        do {

            //
            // Add the node
            //

            if (!AddUDNode (&lpList, szName)) {
                break;
            }

            Index++;
            dwSize = MAX_KEY_NAME;

            lResult = RegEnumKeyEx(hKeyUserDiff, Index, szName, &dwSize, NULL,
                                   NULL, NULL, &ftWrite);


        } while (lResult == ERROR_SUCCESS);
    }


    //
    // Close the open key
    //

    RegCloseKey(hKeyUserDiff);


    //
    // Process the builds
    //

    lpItem = lpList;

    while (lpItem) {

        //
        // Only want to apply changes that occurred in
        // builds after the one the user is running.
        //

        if ( (lpItem->dwBuildNumber > dwBuildNumber) &&
              (lpItem->dwBuildNumber <= g_dwBuildNumber) )  {
            ProcessBuild(lpProfile, lpItem);
        }

        lpItem = lpItem->pNext;
    }


    //
    // Free the link list
    //

    FreeUDList (lpList);


    //
    // Unload the hive
    //

    MyRegUnLoadKey(HKEY_USERS, USERDIFF);


    //
    // Success
    //

    DebugMsg((DM_VERBOSE, TEXT("ProcessUserDiff:  Leaving successfully.")));

    return TRUE;

}

//*************************************************************
//
//  AddUDNode()
//
//  Purpose:    Adds a build node to the link listed
//              sorted by build number
//
//  Parameters: lpList         -   Link list of nodes
//              lpBuildNumber  -   New node name
//      
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//
//*************************************************************

BOOL AddUDNode (LPUDNODE *lpList, LPTSTR lpBuildNumber)
{
    LPUDNODE lpNewItem;
    LPUDNODE lpHead, lpPrev;

    if (!lpBuildNumber || !*lpBuildNumber) {
        return TRUE;
    }


    //
    // Setup the new node
    //

    lpNewItem = (LPUDNODE) LocalAlloc(LPTR, sizeof(UDNODE));

    if (!lpNewItem) {
        return FALSE;
    }

    lstrcpy (lpNewItem->szBuildNumber, lpBuildNumber);
    lpNewItem->dwBuildNumber = StringToInt(lpBuildNumber);
    lpNewItem->pNext = NULL;


    //
    // Now add it to the list sorted
    //

    lpHead = *lpList;
    lpPrev = NULL;


    if (!lpHead) {

        //
        // First item in the list
        //

        *lpList = lpNewItem;

        return TRUE;
    }


    //
    // If we made it here, there is one or more items in the list
    //


    while (lpHead) {

        if (lpNewItem->dwBuildNumber <= lpHead->dwBuildNumber) {

            if (lpPrev) {

                //
                // Insert the item
                //

                lpPrev->pNext = lpNewItem;
                lpNewItem->pNext = lpHead;
                return TRUE;

            } else {

                //
                // Head of the list
                //

                lpNewItem->pNext = lpHead;
                *lpList = lpNewItem;
                return TRUE;

            }

        }

        lpPrev = lpHead;
        lpHead = lpHead->pNext;
    }


    //
    // Add node to the end of the list
    //

    lpPrev->pNext = lpNewItem;


    return TRUE;
}


//*************************************************************
//
//  FreeUDList()
//
//  Purpose:    Free's a UDNODE link list
//
//  Parameters: lpList  -   List to be freed
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//
//*************************************************************

BOOL FreeUDList (LPUDNODE lpList)
{
    LPUDNODE lpNext;


    if (!lpList) {
        return TRUE;
    }


    lpNext = lpList->pNext;

    while (lpList) {
        LocalFree (lpList);
        lpList = lpNext;

        if (lpList) {
            lpNext = lpList->pNext;
        }
    }

    return TRUE;
}

//*************************************************************
//
//  ProcessBuild()
//
//  Purpose:    Processes the changes for a specific build
//
//  Parameters: lpProfile   -   Profile information
//              lpItem  -   Build item to process
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//
//*************************************************************

BOOL ProcessBuild(LPPROFILE lpProfile, LPUDNODE lpItem)
{
    TCHAR szSubKey[MAX_PATH];
    LONG lResult;
    HKEY hKey;

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ProcessBuild:  Entering with build <%s>."),
             lpItem->szBuildNumber));


    //
    // Open "Hive" subkey
    //

    wsprintf (szSubKey, TEXT("%s\\%s\\Hive"), USERDIFF, lpItem->szBuildNumber);
    lResult = RegOpenKeyEx (HKEY_USERS, szSubKey, 0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS) {
        ProcessHive(lpProfile, lpItem, hKey);
        RegCloseKey (hKey);
    }


    //
    // Open "Files" subkey
    //

    wsprintf (szSubKey, TEXT("%s\\%s\\Files"), USERDIFF, lpItem->szBuildNumber);
    lResult = RegOpenKeyEx (HKEY_USERS, szSubKey, 0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS) {
        ProcessFiles(lpProfile, lpItem, hKey);
        RegCloseKey (hKey);
    }


    //
    // Open "Execute" subkey
    //

    wsprintf (szSubKey, TEXT("%s\\%s\\Execute"), USERDIFF, lpItem->szBuildNumber);
    lResult = RegOpenKeyEx (HKEY_USERS, szSubKey, 0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS) {
        ProcessPrograms(lpProfile, lpItem, hKey);
        RegCloseKey (hKey);
    }

    //
    // Success
    //

    DebugMsg((DM_VERBOSE, TEXT("ProcessBuild:  Leaving successfully.")));

    return TRUE;

}

//*************************************************************
//
//  ProcessHive()
//
//  Purpose:    Processes the Hive entry for a build
//
//  Parameters: lpProfile   -   Profile information
//              lpItem      -   Build item
//              hKey        -   Registry key to enumerate
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//
//*************************************************************

BOOL ProcessHive(LPPROFILE lpProfile, LPUDNODE lpItem, HKEY hKey)
{
    TCHAR szSubKey[MAX_PATH];
    TCHAR szValueName[MAX_KEY_NAME];
    DWORD dwSize, dwType, dwAction, dwDisp, dwFlags, dwProductType;
    LPBYTE lpValueData;
    LONG lResult;
    UINT Index = 1;
    FILETIME ftWrite;
    HKEY hKeyEntry, hKeyTemp;
    LPTSTR lpName;


    DebugMsg((DM_VERBOSE, TEXT("ProcessHive:  Entering.")));

    //
    // Process the entry
    //

    wsprintf (szSubKey, TEXT("%s\\%s\\Hive\\%d"), USERDIFF, lpItem->szBuildNumber, Index);
    lResult = RegOpenKeyEx (HKEY_USERS, szSubKey, 0, KEY_READ, &hKeyEntry);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessHive:  No hive entries.")));
        goto Exit;
    }


    do {

        //
        // Query for the product type
        //

        dwSize = sizeof(dwProductType);
        lResult = RegQueryValueEx(hKeyEntry, UD_PRODUCTTYPE, NULL, &dwType,
                                  (LPBYTE)&dwProductType, &dwSize);


        //
        // It's ok to not have a product type listed in userdiff.ini.
        // In this case, we always apply the change regardless of the
        // platform.
        //

        if (lResult == ERROR_SUCCESS) {

            //
            // A specific product was listed.  Check if
            // we can process this entry.
            //

            if (!OkToProcessItem(dwProductType)) {
                DebugMsg((DM_VERBOSE, TEXT("ProcessHive:  Skipping Item %d due to product type mismatch."), Index));
                goto LoopAgain;
            }
        }


        //
        // Query for the action type
        //

        dwSize = sizeof(dwAction);
        lResult = RegQueryValueEx(hKeyEntry, UD_ACTION, NULL, &dwType,
                                  (LPBYTE)&dwAction, &dwSize);

        if (lResult == ERROR_SUCCESS) {

            switch (dwAction) {

                DebugMsg((DM_VERBOSE, TEXT("ProcessHive:  Item %d has an action of %d."),
                         Index, dwAction));

                case 1: {
                    //
                    // Add New Key
                    //
                    // Get the key name
                    //

                   dwSize = MAX_PATH * sizeof(TCHAR);
                   lResult = RegQueryValueEx(hKeyEntry, UD_KEYNAME, NULL, &dwType,
                                             (LPBYTE)szSubKey, &dwSize);

                   if (lResult == ERROR_SUCCESS) {

                       lResult = RegCreateKeyEx (lpProfile->hKeyCurrentUser,
                                                 szSubKey, 0, NULL,
                                                 REG_OPTION_NON_VOLATILE,
                                                 KEY_ALL_ACCESS, NULL,
                                                 &hKeyTemp, &dwDisp);

                       if (lResult == ERROR_SUCCESS) {

                           DebugMsg((DM_VERBOSE, TEXT("ProcessHive:  Created subkey <%s>."),
                                    szSubKey));

                           RegCloseKey(hKeyTemp);
                       } else {

                           DebugMsg((DM_WARNING, TEXT("ProcessHive:  Failed to create subkey <%s> with error %d."),
                                    szSubKey, lResult));
                       }
                   }

                   }
                   break;

                case 2: {
                    //
                    // Delete a key and all it's subkeys
                    //
                    // Get the key name
                    //

                   dwSize = MAX_PATH * sizeof(TCHAR);
                   lResult = RegQueryValueEx(hKeyEntry, UD_KEYNAME, NULL, &dwType,
                                             (LPBYTE)szSubKey, &dwSize);

                   if (lResult == ERROR_SUCCESS) {

                       DebugMsg((DM_VERBOSE, TEXT("ProcessHive:  Calling RegDelnode on <%s>."),
                                szSubKey));

                       RegDelnode (lpProfile->hKeyCurrentUser, szSubKey);
                   }

                   }
                   break;

                case 3: {
                    //
                    // Add a new value
                    //
                    // Get the key name
                    //

                   DebugMsg((DM_VERBOSE, TEXT("ProcessHive:  Adding a new value.")));

                   dwSize = MAX_PATH * sizeof(TCHAR);
                   lResult = RegQueryValueEx(hKeyEntry, UD_KEYNAME, NULL, &dwType,
                                             (LPBYTE)szSubKey, &dwSize);

                   if (lResult != ERROR_SUCCESS) {
                       DebugMsg((DM_WARNING, TEXT("ProcessHive:  Failed to get UD_KEYNAME with error %d."), lResult));
                       goto LoopAgain;
                   }

                   lResult = RegCreateKeyEx (lpProfile->hKeyCurrentUser,
                                             szSubKey, 0, NULL,
                                             REG_OPTION_NON_VOLATILE,
                                             KEY_ALL_ACCESS, NULL,
                                             &hKeyTemp, &dwDisp);

                   if (lResult != ERROR_SUCCESS) {
                       DebugMsg((DM_WARNING, TEXT("ProcessHive:  Failed to create UD_KEYNAME with error %d."), lResult));
                       goto LoopAgain;
                   }


                   //
                   // Query for the value name
                   //

                   dwSize = MAX_KEY_NAME * sizeof(TCHAR);
                   lResult = RegQueryValueEx(hKeyEntry, UD_VALUENAME, NULL, &dwType,
                                             (LPBYTE)szValueName, &dwSize);

                   if (lResult != ERROR_SUCCESS) {
                       DebugMsg((DM_WARNING, TEXT("ProcessHive:  Failed to query UD_VALUENAME with error %d."), lResult));
                       RegCloseKey(hKeyTemp);
                       goto LoopAgain;
                   }


                   //
                   // Query for the value data size
                   //

                   dwSize = 0;
                   lResult = RegQueryValueEx(hKeyEntry, UD_VALUE, NULL, &dwType,
                                             NULL, &dwSize);

                   if (lResult != ERROR_SUCCESS) {
                       DebugMsg((DM_WARNING, TEXT("ProcessHive:  Failed to query UD_VALUE with error %d."), lResult));
                       RegCloseKey(hKeyTemp);
                       goto LoopAgain;
                   }


                   //
                   // Allocate space for the data
                   //

                   lpValueData = LocalAlloc (LPTR, dwSize);

                   if (!lpValueData) {
                       DebugMsg((DM_WARNING, TEXT("ProcessHive:  LocalAlloc failed (%d)."), GetLastError()));
                       RegCloseKey(hKeyTemp);
                       goto LoopAgain;
                   }


                   //
                   // Query for the value data
                   //

                   lResult = RegQueryValueEx(hKeyEntry, UD_VALUE, NULL, &dwType,
                                             lpValueData, &dwSize);

                   if (lResult != ERROR_SUCCESS) {
                       DebugMsg((DM_WARNING, TEXT("ProcessHive:  Failed to query value data with error %d."), lResult));
                       LocalFree (lpValueData);
                       RegCloseKey(hKeyTemp);
                       goto LoopAgain;
                   }


                   //
                   // Set the new value
                   //

                   RegSetValueEx(hKeyTemp, szValueName, 0, dwType,
                                 lpValueData, dwSize);


                   //
                   // Clean up
                   //

                   LocalFree (lpValueData);

                   RegCloseKey(hKeyTemp);

                   DebugMsg((DM_VERBOSE, TEXT("ProcessHive:  Finished adding value <%s>."), szValueName));
                   }
                   break;

                case 4: {
                   //
                   // Delete value(s)
                   //
                   // Get the key name
                   //

                   DebugMsg((DM_VERBOSE, TEXT("ProcessHive:  Entering delete a value.")));

                   dwSize = ARRAYSIZE(szSubKey);
                   lResult = RegQueryValueEx(hKeyEntry, UD_KEYNAME, NULL, &dwType,
                                             (LPBYTE)szSubKey, &dwSize);

                   if (lResult != ERROR_SUCCESS) {
                       DebugMsg((DM_WARNING, TEXT("ProcessHive:  Failed to query for value to delete (%d)."), lResult));
                       goto LoopAgain;
                   }

                   lResult = RegCreateKeyEx (lpProfile->hKeyCurrentUser,
                                             szSubKey, 0, NULL,
                                             REG_OPTION_NON_VOLATILE,
                                             KEY_ALL_ACCESS, NULL,
                                             &hKeyTemp, &dwDisp);

                   if (lResult != ERROR_SUCCESS) {
                       DebugMsg((DM_WARNING, TEXT("ProcessHive:  Failed to create key (%s) for value to delete (%d)."), szSubKey, lResult));
                       goto LoopAgain;
                   }


                   //
                   // Query for the flags
                   //

                   dwSize = sizeof(dwFlags);
                   lResult = RegQueryValueEx(hKeyEntry, UD_FLAGS, NULL, &dwType,
                                             (LPBYTE)&dwFlags, &dwSize);

                   if (lResult != ERROR_SUCCESS) {
                       dwFlags = 0;
                   }


                   //
                   // Process the flags
                   //

                   if (dwFlags == 2) {
                       DebugMsg((DM_VERBOSE, TEXT("ProcessHive:  Calling DeleteAllValues.")));
                       DeleteAllValues (hKeyTemp);
                       RegCloseKey(hKeyTemp);
                       goto LoopAgain;
                   }


                   //
                   // Query for the value names size
                   //

                   dwSize = 0;
                   lResult = RegQueryValueEx(hKeyEntry, UD_VALUENAMES, NULL, &dwType,
                                             NULL, &dwSize);

                   if (lResult != ERROR_SUCCESS) {
                       DebugMsg((DM_WARNING, TEXT("ProcessHive:  Failed to query for value names to delete (%d)."), lResult));
                       RegCloseKey(hKeyTemp);
                       goto LoopAgain;
                   }


                   //
                   // Allocate space for the data
                   //

                   lpValueData = LocalAlloc (LPTR, dwSize);

                   if (!lpValueData) {
                       DebugMsg((DM_WARNING, TEXT("ProcessHive:  LocalAlloc failed (%d)."), GetLastError()));
                       RegCloseKey(hKeyTemp);
                       goto LoopAgain;
                   }


                   //
                   // Query for the value data
                   //

                   lResult = RegQueryValueEx(hKeyEntry, UD_VALUENAMES, NULL, &dwType,
                                             lpValueData, &dwSize);

                   if (lResult != ERROR_SUCCESS) {
                       DebugMsg((DM_WARNING, TEXT("ProcessHive:  Failed to query for value data to delete (%d)."), lResult));
                       LocalFree (lpValueData);
                       RegCloseKey(hKeyTemp);
                       goto LoopAgain;
                   }


                   //
                   // Delete the values
                   //

                   lpName = (LPTSTR) lpValueData;

                   while (*lpName) {
                       DebugMsg((DM_VERBOSE, TEXT("ProcessHive:  Deleting (%s)."), lpName));
                       RegDeleteValue (hKeyTemp, lpName);
                       lpName += lstrlen(lpName) + 1;
                   }


                   //
                   // Delete the no-name value if appropriate
                   //

                   if (dwFlags == 1) {
                       DebugMsg((DM_VERBOSE, TEXT("ProcessHive:  Deleting no name value.")));
                       RegDeleteValue (hKeyTemp, NULL);
                   }


                   //
                   // Clean up
                   //

                   LocalFree (lpValueData);
                   RegCloseKey(hKeyTemp);

                   DebugMsg((DM_VERBOSE, TEXT("ProcessHive:  Leaving deletion code.")));
                   }

                   break;
            }

        }

LoopAgain:

        //
        // Close the registry key
        //

        RegCloseKey(hKeyEntry);


        //
        // Enumerate again
        //

        Index++;

        wsprintf (szSubKey, TEXT("%s\\%s\\Hive\\%d"), USERDIFF, lpItem->szBuildNumber, Index);
        lResult = RegOpenKeyEx (HKEY_USERS, szSubKey, 0, KEY_READ, &hKeyEntry);

    } while (lResult == ERROR_SUCCESS);

Exit:

    DebugMsg((DM_VERBOSE, TEXT("ProcessHive:  Leaving.")));

    return TRUE;
}

//*************************************************************
//
//  ProcessFiles()
//
//  Purpose:    Processes the Files entry for a build
//
//  Parameters: lpProfile - Profile information
//              lpItem    -   Build item
//              hKey      -   Registry key to enumerate
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//
//*************************************************************

BOOL ProcessFiles(LPPROFILE lpProfile, LPUDNODE lpItem, HKEY hKey)
{
    TCHAR szSubKey[MAX_PATH];
    TCHAR szSrc[MAX_PATH];
    TCHAR szDest[MAX_PATH];
    TCHAR szItem[MAX_PATH];
    LPTSTR lpEnd, lpTemp;
    DWORD dwSize, dwType, dwAction, dwProductType;
    LONG lResult;
    UINT Index = 1;
    FILETIME ftWrite;
    HKEY hKeyEntry;


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("ProcessFiles:  Entering.")));


    //
    // Process the entry
    //

    wsprintf (szSubKey, TEXT("%s\\%s\\Files\\%d"), USERDIFF, lpItem->szBuildNumber, Index);
    lResult = RegOpenKeyEx (HKEY_USERS, szSubKey, 0, KEY_READ, &hKeyEntry);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessFiles:  No Files entries.")));
        goto Exit;
    }


    do {

        //
        // Query for the product type
        //

        dwSize = sizeof(dwProductType);
        lResult = RegQueryValueEx(hKeyEntry, UD_PRODUCTTYPE, NULL, &dwType,
                                  (LPBYTE)&dwProductType, &dwSize);


        //
        // It's ok to not have a product type listed in userdiff.ini.
        // In this case, we always apply the change regardless of the
        // platform.
        //

        if (lResult == ERROR_SUCCESS) {

            //
            // A specific product was listed.  Check if
            // we can process this entry.
            //

            if (!OkToProcessItem(dwProductType)) {
                DebugMsg((DM_VERBOSE, TEXT("ProcessFiles:  Skipping Item %d due to product type mismatch."), Index));
                goto LoopAgain;
            }
        }


        //
        // Query for the action type
        //

        dwSize = sizeof(dwAction);
        lResult = RegQueryValueEx(hKeyEntry, UD_ACTION, NULL, &dwType,
                                  (LPBYTE)&dwAction, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ProcessFiles:  Failed to query action type (%d)."), lResult));
            goto LoopAgain;
        }


        //
        // Query for the item
        //

        dwSize = ARRAYSIZE(szItem);
        lResult = RegQueryValueEx(hKeyEntry, UD_ITEM, NULL, &dwType,
                                  (LPBYTE)szItem, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("ProcessFiles:  Failed to query UD_ITEM type (%d)."), lResult));
            goto LoopAgain;
        }

        DebugMsg((DM_VERBOSE, TEXT("ProcessFiles:  Item %d has an action of %d."),
                 Index, dwAction));

        switch (dwAction) {

            case 1:

               //
               // Create new program group
               //

               GetSpecialFolderPath (CSIDL_PROGRAMS, szDest);
               lpEnd = CheckSlash(szDest);
               lstrcpy (lpEnd, szItem);

               if (CreateNestedDirectory(szDest, NULL)) {
                   DebugMsg((DM_VERBOSE, TEXT("ProcessFiles:  Created new group (%s)."), szDest));
               } else {
                   DebugMsg((DM_WARNING, TEXT("ProcessFiles:  Failed to created new group (%s) with (%d)."),
                            szDest, GetLastError()));
               }

               break;

            case 2:
               //
               // Delete a program group
               //

               GetSpecialFolderPath (CSIDL_PROGRAMS, szDest);
               lpEnd = CheckSlash(szDest);
               lstrcpy (lpEnd, szItem);

               Delnode(szDest);

               DebugMsg((DM_VERBOSE, TEXT("ProcessFiles:  Deleted group (%s)."), szDest));

               break;

            case 3:
               {
               TCHAR szStartMenu [MAX_FOLDER_SIZE];
               DWORD dwSize;

               //
               // Add a new item
               //

               dwSize = ARRAYSIZE(szSrc);
               if (!GetDefaultUserProfileDirectory(szSrc, &dwSize)) {
                   DebugMsg((DM_WARNING, TEXT("ProcessFiles:  Failed to get default user profile.")));
                   goto LoopAgain;
               }

               lpEnd = CheckSlash(szSrc);

               if (LoadString (g_hDllInstance, IDS_SH_PROGRAMS, szStartMenu,
                           MAX_FOLDER_SIZE)) {

                   lstrcpy (lpEnd, szStartMenu);
                   lpEnd = CheckSlash(szSrc);
                   lstrcpy (lpEnd, szItem);


                   GetSpecialFolderPath (CSIDL_PROGRAMS, szDest);
                   lpEnd = CheckSlash(szDest);
                   lstrcpy (lpEnd, szItem);

                   if (CopyFile (szSrc, szDest, FALSE)) {
                       DebugMsg((DM_VERBOSE, TEXT("ProcessFiles:  <%s> ==> <%s>  [OK]."),
                                szSrc, szDest));
                   } else {
                       DebugMsg((DM_VERBOSE, TEXT("ProcessFiles:  <%s> ==> <%s>  [FAILED %d]."),
                                szSrc, szDest, GetLastError()));
                   }
               }
               }

               break;

            case 4:
               //
               // Delete a program item
               //

               GetSpecialFolderPath (CSIDL_PROGRAMS, szDest);
               lpEnd = CheckSlash(szDest);
               lstrcpy (lpEnd, szItem);

               if (DeleteFile(szDest)) {
                   DebugMsg((DM_VERBOSE, TEXT("ProcessFiles:  Deleted <%s>"), szDest));
               } else {
                   DebugMsg((DM_WARNING, TEXT("ProcessFiles:  Failed to deleted <%s> with %d"), szDest, GetLastError()));
               }


               //
               // Attempt to delete the directory
               //

               lpTemp = szDest + lstrlen(szDest) - 1;
               lpEnd--;

               while ((*lpTemp != TEXT('\\')) && lpTemp > lpEnd) {
                   lpTemp--;
               }

               if (lpTemp == lpEnd) {
                   break;
               }

               *lpTemp = TEXT('\0');

               if (RemoveDirectory(szDest)) {
                   DebugMsg((DM_VERBOSE, TEXT("ProcessFiles:  Deleted directory <%s>"), szDest));
               } else {
                   DebugMsg((DM_VERBOSE, TEXT("ProcessFiles:  Failed to delete directory <%s> with %d"), szDest, GetLastError()));
               }

               break;
        }


LoopAgain:

        //
        // Close the registry key
        //

        RegCloseKey(hKeyEntry);


        //
        // Enumerate again
        //

        Index++;

        wsprintf (szSubKey, TEXT("%s\\%s\\Files\\%d"), USERDIFF, lpItem->szBuildNumber, Index);
        lResult = RegOpenKeyEx (HKEY_USERS, szSubKey, 0, KEY_READ, &hKeyEntry);

    } while (lResult == ERROR_SUCCESS);

Exit:

    DebugMsg((DM_VERBOSE, TEXT("ProcessFiles:  Leaving.")));

    return TRUE;
}

//*************************************************************
//
//  ProcessPrograms()
//
//  Purpose:    Processes the Execute entry for a build
//
//  Parameters: lpProfile - Profile information
//              lpItem    -   Build item
//              hKey      -   Registry key to enumerate
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/16/95    ericflo    Created
//
//*************************************************************

BOOL ProcessPrograms (LPPROFILE lpProfile, LPUDNODE lpItem, HKEY hKey)
{
    TCHAR szSubKey[MAX_PATH];
    TCHAR szCmdLine[MAX_PATH];
    TCHAR szFullPath[MAX_PATH];
    DWORD dwSize, dwType, dwProductType;
    LONG lResult;
    UINT Index = 1;
    HKEY hKeyEntry;
    STARTUPINFO si;
    PROCESS_INFORMATION ProcessInformation;
    BOOL Result;


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ProcessPrograms:  Entering.")));


    //
    // Process the entry
    //

    wsprintf (szSubKey, TEXT("%s\\%s\\Execute\\%d"), USERDIFF, lpItem->szBuildNumber, Index);
    lResult = RegOpenKeyEx (HKEY_USERS, szSubKey, 0, KEY_READ, &hKeyEntry);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessPrograms:  No execute entries.")));
        goto Exit;
    }


    do {

        //
        // Query for the product type
        //

        dwSize = sizeof(dwProductType);
        lResult = RegQueryValueEx(hKeyEntry, UD_PRODUCTTYPE, NULL, &dwType,
                                  (LPBYTE)&dwProductType, &dwSize);


        //
        // It's ok to not have a product type listed in userdiff.ini.
        // In this case, we always apply the change regardless of the
        // platform.
        //

        if (lResult == ERROR_SUCCESS) {

            //
            // A specific product was listed.  Check if
            // we can process this entry.
            //

            if (!OkToProcessItem(dwProductType)) {
                DebugMsg((DM_VERBOSE, TEXT("ProcessPrograms:  Skipping Item %d due to product type mismatch."), Index));
                goto LoopAgain;
            }
        }


        //
        // Query for the command line
        //


        dwSize = MAX_PATH * sizeof(TCHAR);
        lResult = RegQueryValueEx(hKeyEntry, UD_COMMANDLINE, NULL, &dwType,
                                  (LPBYTE)szCmdLine, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            goto LoopAgain;
        }


        //
        // If we have a NULL path, loop again.
        //

        if (szCmdLine[0] == TEXT('\0')) {
            goto LoopAgain;
        }


        //
        // Expand the command line
        //

        ExpandEnvironmentStrings (szCmdLine, szFullPath, MAX_PATH);


        //
        // Initialize process startup info
        //

        si.cb = sizeof(STARTUPINFO);
        si.lpReserved = NULL;
        si.lpTitle = NULL;
        si.lpDesktop = NULL;
        si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOWNORMAL;
        si.lpReserved2 = NULL;
        si.cbReserved2 = 0;


        //
        // Start the app
        //

        Result = CreateProcessAsUser(lpProfile->hToken, NULL, szFullPath,
                                     NULL, NULL, FALSE,
                                     NORMAL_PRIORITY_CLASS, NULL, NULL,
                                     &si, &ProcessInformation);

        if (Result) {

            DebugMsg((DM_VERBOSE, TEXT("ProcessPrograms:  Spawned <%s>.  Waiting for it to complete."),
                      szFullPath));

            //
            // Wait for the app to complete (3 minutes max)
            //

            WaitForSingleObject(ProcessInformation.hProcess, 180000);


            DebugMsg((DM_VERBOSE, TEXT("ProcessPrograms:  Finished waiting for <%s>."),
                     szFullPath));


            //
            // Close our handles to the process and thread
            //

            CloseHandle(ProcessInformation.hProcess);
            CloseHandle(ProcessInformation.hThread);

        } else {
            DebugMsg((DM_WARNING, TEXT("ProcessPrograms:  Failed to execute <%s>, error = %d"),
                      szFullPath, GetLastError()));
        }

LoopAgain:

        //
        // Close the registry key
        //

        RegCloseKey(hKeyEntry);


        //
        // Enumerate again
        //

        Index++;

        wsprintf (szSubKey, TEXT("%s\\%s\\Execute\\%d"), USERDIFF, lpItem->szBuildNumber, Index);
        lResult = RegOpenKeyEx (HKEY_USERS, szSubKey, 0, KEY_READ, &hKeyEntry);

    } while (lResult == ERROR_SUCCESS);

Exit:

    DebugMsg((DM_VERBOSE, TEXT("ProcessPrograms:  Leaving.")));

    return TRUE;
}

//*************************************************************
//
//  OkToProcessItem()
//
//  Purpose:    Determines if the platform currently running
//              on should have the change in userdiff.ini applied.
//
//  Parameters: dwProductType - ProductType for a specific entry
//                              in userdiff.ini
//
//  Return:     TRUE if change should be applied
//              FALSE if not
//
//  Comments:   dwProductType can be one of these values:
//
//              0 = All platforms
//              1 = All server platforms
//              2 = Workstation
//              3 = Server
//              4 = Domain Controller
//
//  History:    Date        Author     Comment
//              4/08/96     ericflo    Created
//
//*************************************************************

BOOL OkToProcessItem(DWORD dwProductType)
{
    BOOL bRetVal = FALSE;


    switch (g_ProductType) {

        case PT_WORKSTATION:

            if ( (dwProductType == 0) ||
                 (dwProductType == 2) ) {

                bRetVal = TRUE;
            }

            break;

        case PT_SERVER:

            if ( (dwProductType == 0) ||
                 (dwProductType == 1) ||
                 (dwProductType == 3) ) {

                bRetVal = TRUE;
            }

            break;

        case PT_DC:
            if ( (dwProductType == 0) ||
                 (dwProductType == 1) ||
                 (dwProductType == 4) ) {

                bRetVal = TRUE;
            }

            break;

    }

    return bRetVal;
}
