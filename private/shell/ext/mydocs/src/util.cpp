/*----------------------------------------------------------------------------
/ Title;
/   util.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   utility functions for My Documents.
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#pragma hdrstop

//*************************************************************
//
//  CheckSlash()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Created
//
//*************************************************************
LPTSTR CheckSlash (LPTSTR lpDir)
{
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}

//*************************************************************
//
//  RegDelnodeRecurse()
//
//  Purpose:    Deletes a registry key and all it's subkeys / values.
//              Called by RegDelnode
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
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

BOOL RegDelnodeRecurse (HKEY hKeyRoot, LPTSTR lpSubKey)
{
    LPTSTR lpEnd;
    LONG lResult;
    DWORD dwSize;
    TCHAR szName[MAX_PATH];
    HKEY hKey;
    FILETIME ftWrite;

    //
    // First, see if we can delete the key without having
    // to recurse.
    //


    lResult = RegDeleteKey(hKeyRoot, lpSubKey);

    if (lResult == ERROR_SUCCESS) {
        return TRUE;
    }


    lResult = RegOpenKeyEx (hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {
        return FALSE;
    }


    lpEnd = CheckSlash(lpSubKey);

    //
    // Enumerate the keys
    //

    dwSize = MAX_PATH;
    lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
                           NULL, NULL, &ftWrite);

    if (lResult == ERROR_SUCCESS) {

        do {

            lstrcpy (lpEnd, szName);

            if (!RegDelnodeRecurse(hKeyRoot, lpSubKey)) {
                break;
            }

            //
            // Enumerate again
            //

            dwSize = MAX_PATH;

            lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
                                   NULL, NULL, &ftWrite);


        } while (lResult == ERROR_SUCCESS);
    }

    lpEnd--;
    *lpEnd = TEXT('\0');


    RegCloseKey (hKey);


    //
    // Try again to delete the key
    //

    lResult = RegDeleteKey(hKeyRoot, lpSubKey);

    if (lResult == ERROR_SUCCESS) {
        return TRUE;
    }

    return FALSE;
}

//*************************************************************
//
//  RegDelnode()
//
//  Purpose:    Deletes a registry key and all it's subkeys / values
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
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

BOOL RegDelnode (HKEY hKeyRoot, LPTSTR lpSubKey)
{
    TCHAR szDelKey[2 * MAX_PATH];

    lstrcpy (szDelKey, lpSubKey);

    return RegDelnodeRecurse(hKeyRoot, szDelKey);

}

/*---------------------------------------------------------------------------
/ PathIsRootOfDrive
/ -----------------------
/
/ Returns TRUE for any string of the type "C:", "C:\", etc., FALSE otherwise.
/
/---------------------------------------------------------------------------*/
BOOL PathIsRootOfDrive( LPTSTR pPath )
{

    INT i;

    //
    // Check to make sure it's not a drive root
    //

    i  = lstrlen(pPath);

    //
    // Error check
    //

    if (i == 1)
    {
        return FALSE;
    }

    //
    // Can only be a drive if it's 3 characters or less (excluding NULL)
    //

    if (i <= 3)
    {
        TCHAR ch1, ch2, ch3 = 0;

        ch1 = pPath[0];
        ch2 = pPath[1];

        if (i == 3)
        {
            ch3 = pPath[2];
        }

        //
        // Determine if it's a drive
        //

        if ( ( ((ch1 >= TEXT('a')) && (ch1 <= TEXT('z'))) ||
               ((ch1 >= TEXT('A')) && (ch1 <= TEXT('Z')))
              ) &&
             (ch2 == TEXT(':')) &&
             ((ch3 == 0) || (ch3 == TEXT('\\')))
            )
        {
            return TRUE;
        }
    }

    return FALSE;
}


/*---------------------------------------------------------------------------
/ PathIsLocalAndWriteable
/ -----------------------
/
/ Returns FALSE for UNC paths, net drive paths and CDROMs.  TRUE for
/ everything else...
/
/---------------------------------------------------------------------------*/
BOOL PathIsLocalAndWriteable( LPTSTR pPath )
{
    // Check to see if this is a network path or a redirected network drive...
    if ((!pPath) || (!(*pPath)) || PathIsUNC( pPath ))
    {
        return FALSE;
    }

    if (lstrlen( pPath ) > 3)
    {
        TCHAR cSave;
        UINT uDriveType;

        // strip off after drive & slash, C:\, etc.
        cSave = pPath[ 3 ];
        pPath[ 3 ] = 0;
        uDriveType = GetDriveType( pPath );
        pPath[ 3 ] = cSave;

        if ( (uDriveType == DRIVE_REMOTE) ||
             (uDriveType == DRIVE_CDROM)
            )
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*---------------------------------------------------------------------------
/ GetMyDocumentsDisplayName
/ -------------------------
/
/ Return the current display name for "My Documents"
/
/---------------------------------------------------------------------------*/
void GetMyDocumentsDisplayName( LPTSTR pPath, UINT cch )
{
    TCHAR szBuffer[ MAX_PATH ];
    HKEY hkey = NULL;
    HKEY hkeyToOpen = HKEY_CURRENT_USER;
    LPCTSTR pSubKeyToOpen = c_szPerUserCLSID;

    szBuffer[0] = 0;
    *pPath = 0;

try_again:
    //
    // Try per user settings first...
    //
    if ( ERROR_SUCCESS == RegOpenKeyEx( hkeyToOpen, pSubKeyToOpen, 0,
                                        KEY_READ, &hkey )
       )
    {
        LONG cbSize = sizeof(szBuffer);

        if (ERROR_SUCCESS == RegQueryValue( hkey, NULL, szBuffer, &cbSize ))
        {
            if (cbSize <= (LONG)((cch*sizeof(TCHAR))))
            {
                lstrcpy( pPath, szBuffer );
            }
        }

        RegCloseKey( hkey );

    }

    if (!szBuffer[0] && (pSubKeyToOpen == c_szPerUserCLSID))
    {
        pSubKeyToOpen = c_szCLSIDFormat;
        hkeyToOpen = HKEY_CLASSES_ROOT;
        goto try_again;
    }

    return;

}

/*---------------------------------------------------------------------------
/ IsWinstone97
/ ----------------
/
/ Determine if we're running under Winstone 97 (at least whether we're in
/ the state where Winstone messes up CSIDL_PERSONAL)
/
/---------------------------------------------------------------------------*/
BOOL IsWinstone97( LPTSTR pPath )
{
    HKEY hkey = NULL;
    BOOL fRet = FALSE;
    TCHAR szPath[ MAX_PATH ];
    DWORD dwType, cbSize = SIZEOF(szPath);

    MDTraceEnter( TRACE_UTIL, "IsWinstone97" );

    if (!pPath)
    {
        pPath = szPath;
    }

    if ( ERROR_SUCCESS ==
         RegOpenKey( HKEY_LOCAL_MACHINE, c_szRegistrySettings, &hkey )
        )
    {
        if (ERROR_SUCCESS ==
            RegQueryValueEx( hkey, c_szPersonal, NULL, &dwType, (LPBYTE)pPath, &cbSize))
        {
            if ( ( (dwType == REG_SZ) ||
                   (dwType == REG_EXPAND_SZ)
                  ) &&
                 (cbSize > (3 * sizeof(TCHAR)))
                )
            {
                //
                // Winstone97 sets CSIDL_PERSONAL in HKEY_LOCAL_MACHINE
                // to be C:\wstemp\mydocs.  szPath+3 is to skip the
                // drive letter & :\...
                //

                if (lstrcmpi( &pPath[3], TEXT("wstemp\\mydocs"))==0)
                {
                    MDTrace(TEXT("Detected Winstone97"));
                    fRet = TRUE;
                }
                else
                {
                    MDTrace(TEXT("Didn't detect Winston97, path was %s"), pPath);
                }
            }
            else
            {
                MDTrace(TEXT("Didn't detect Winston97"));
            }
        }
        else
        {
            MDTrace(TEXT("Couldn't query %s value"), c_szPersonal );
        }
    }
    else
    {
        MDTrace(TEXT("Couldn't open %s key"), c_szRegistrySettings );
    }

    if (hkey)
    {
        RegCloseKey( hkey );
    }

    MDTraceLeave();
    return fRet;
}

/*---------------------------------------------------------------------------
/ AddNewMenuToRegistry
/ --------------------
/
/ Adds new menu registry goo...
/
/---------------------------------------------------------------------------*/
void AddNewMenuToRegistry( void )
{
    TCHAR szString[ MAX_PATH ];
    INT cch;

    MDTraceEnter( TRACE_UTIL, "AddNewMenuToRegistry" );

    CallRegInstall( g_hInstance, "RegNewMenu" );

    cch = LoadString( g_hInstance,
                      IDS_MYDOCS_NEW_COMMAND_VALUE,
                      szString,
                      ARRAYSIZE(szString)
                     );

    if (cch)
    {
        MDTrace(TEXT("Setting %s to %s"), c_szMyDocsValue, szString);
        RegSetValue( HKEY_CLASSES_ROOT,
                     c_szMyDocsValue,
                     REG_SZ,
                     szString,
                     cch*sizeof(TCHAR)
                    );
    }

    MDTraceLeave();
}


/*---------------------------------------------------------------------------
/ RemoveMenuRegStuff
/ -------------------
/
/ Removes the registry goo for both the "new" and "sendto" menus in the
/ registry...
/
/---------------------------------------------------------------------------*/
void RemoveMenuStuff( void )
{
    MDTraceEnter( TRACE_UTIL, "RemoveMenuStuff" );

    // Make sure the keys aren't in the registry...
    RegDelnode( HKEY_CLASSES_ROOT, (LPTSTR)c_szMyDocsExt );
    RegDelnode( HKEY_CLASSES_ROOT, (LPTSTR)c_szMyDocsValue );

    MDTraceLeave();
}

/*---------------------------------------------------------------------------
/ RestoreMyDocsFolder
/ -------------------
/
/ Resets special registry value so that My Docs will be visible...
/
/---------------------------------------------------------------------------*/
void RestoreMyDocsFolder( HWND hwnd,
                          HINSTANCE hInstance,
                          LPTSTR pszCmdLine,
                          int nCmdShow
                         )
{
    HKEY hkey = NULL;
    TCHAR szBuffer[ 64 ];
    DWORD dwType, dwSize = SIZEOF(szBuffer);

    MDTraceEnter( TRACE_UTIL, "RestoreMyDocsFolder" );

    // Open the Docuements key...
    if (ERROR_SUCCESS!=RegOpenKey( HKEY_CURRENT_USER, c_szDocumentSettings, &hkey ))
    {
        MDTrace(TEXT("Unable to open Document key"));
        goto exit_gracefully;
    }

    if (ERROR_SUCCESS!=RegQueryValueEx( hkey, c_szHidden, NULL, &dwType, (LPBYTE)szBuffer, &dwSize ))
    {
        TCHAR szName[ MAX_PATH ];

        MDTrace(TEXT("No HideMyDocs value...one must already exist!"));

        dwSize = SIZEOF(szName);
        if ( ERROR_SUCCESS != RegQueryValue(HKEY_CLASSES_ROOT, c_szCLSIDFormat, szName, (LONG *)&dwSize))
        {
            MDTrace(TEXT("Couldn't get display name"));
            goto exit_gracefully;
        }

        ShellMessageBox( g_hInstance, NULL,
                         (LPTSTR)IDS_MYDOCS_ALREADY_EXISTS, 0,
                         MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST, szName
                        );

        goto exit_gracefully;

    }

    // delete the HideMyDocs value
    if (ERROR_SUCCESS!=RegDeleteValue( hkey, c_szHidden ))
    {
        MDTrace(TEXT("Unable to delete HideMyDocs value"));
    }

    // Remove new menu stuff
    RemoveMenuStuff();

    // Add sendto menu stuff
    CallRegInstall( g_hInstance, "RegSendTo" );

    // Add item to sendto directory
    UpdateSendToFile( FALSE );

    // Tell the shell we're back...
    IDREGITEM idl;

    idl.cb = SIZEOF(idl) - sizeof(WORD);
    idl.bFlags = SHID_ROOT_REGITEM;
    idl.bReserved = MYDOCS_SORT_INDEX;   // this is our sort order index
    idl.clsid = CLSID_MyDocumentsExt;
    idl.next = 0;

    SHChangeNotify(SHCNE_CREATE,     SHCNF_IDLIST, (LPITEMIDLIST)&idl, NULL);
    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST, (LPITEMIDLIST)&idl, NULL);

exit_gracefully:
    if (hkey)
    {
        RegCloseKey( hkey );
    }

    MDTraceLeave( );
}


/*---------------------------------------------------------------------------
/ UpdateSendToFile
/ ----------------
/
/ Create/Updates file in SendTo directory to have current display name
/
/---------------------------------------------------------------------------*/
void UpdateSendToFile( BOOL bDeleteOnly )
{
    TCHAR szSendToDir[ MAX_PATH ];
    TCHAR szFile[ MAX_PATH ];
    TCHAR szName[ MAX_PATH ];
    WIN32_FIND_DATA fd;
    HANDLE hFind = NULL;
    INT iCount, iLenName, iLenFile;
    TCHAR tch;
    BOOL bWrongName = FALSE;

    MDTraceEnter( TRACE_UTIL, "UpdateSendToFile" );
    MDTrace(TEXT("bDeleteOnly = %d"),bDeleteOnly);

    //
    // First, get sendto directory...
    //

    if (!SHGetSpecialFolderPath( NULL, szSendToDir, CSIDL_SENDTO, FALSE ))
    {
        MDTrace(TEXT("Couldn't get szSendToDir"));
        return;
    }

    //
    // Get current display name
    //
    GetMyDocumentsDisplayName( szName, ARRAYSIZE(szName) );

    //
    // Now, check to see if the file that exists has the correct display name.
    // if not, then delete and create a new one.  (Or, delete it if we are
    // told to via the bDelete flag).
    //

    lstrcpy( szFile, szSendToDir );
    lstrcat( szFile, TEXT("\\*") );
    lstrcat( szFile, c_szMyDocsExt );

    iCount = 0;
    hFind = FindFirstFile( szFile, &fd );
    while( hFind!=INVALID_HANDLE_VALUE )
    {
        //
        // Check the file for the correct name...
        //
        iLenFile = lstrlen( fd.cFileName );
        iLenName = lstrlen( szName );

        if (iLenName < iLenFile)
        {
            tch = fd.cFileName[ iLenName ];
            fd.cFileName[ iLenName ] = 0;
            if (lstrcmpi( szName, fd.cFileName )!=0)
            {
                bWrongName = TRUE;
            }
            fd.cFileName[ iLenName ] = tch;

        }
        else
        {
            bWrongName = TRUE;
        }

        iCount++;
        if ((iCount > 1) || bDeleteOnly || bWrongName)
        {
            lstrcpy( szFile, szSendToDir );
            lstrcat( szFile, TEXT("\\") );
            lstrcat( szFile, fd.cFileName );
            if (DeleteFile( szFile ))
            {
                MDTrace( TEXT("deleted %s"), szFile );
            }
        }

        if (!FindNextFile( hFind, &fd ))
        {
            FindClose( hFind );
            hFind = INVALID_HANDLE_VALUE;
        }
    }

    if (!bDeleteOnly && (bWrongName || iCount == 0))
    {
        //
        // Create the new file w/the correct name...
        //

        lstrcpy( szFile, szSendToDir );
        lstrcat( szFile, TEXT("\\" ) );
        lstrcat( szFile, szName );
        lstrcat( szFile, c_szMyDocsExt );

        hFind = CreateFile( szFile, GENERIC_WRITE,
                            0, NULL, CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL, NULL
                           );

        if (hFind != INVALID_HANDLE_VALUE)
        {
            DWORD dw;

            MDTrace(TEXT("Writing out %s"), szFile);
            WriteFile( hFind,
                       "Send To My Documents\n\n",
                       23, // lstren("Send To My Documents")+1
                       &dw,
                       NULL
                      );
            FlushFileBuffers( hFind );
            CloseHandle(hFind);
        }
        else
        {
            MDTrace(TEXT("Failed to create %s, GLE = %d"), GetLastError() );
        }
    }

    MDTraceLeave();

}

/*---------------------------------------------------------------------------
/ DoRemovePrompt
/ ----------------
/
/ Checks special registry value to see if My Docs folder should be hidden
/
/---------------------------------------------------------------------------*/
VOID DoRemovePrompt( VOID )
{
    HKEY hkey = NULL, hkey2 = NULL;
    TCHAR szTitle[ 128 ];
    TCHAR szDir[ MAX_PATH ];
    TCHAR szName[ MAX_PATH ];
    TCHAR szFormat[ MAX_PATH ];
    TCHAR szPrompt[ 512 ];
    LONG  Size = ARRAYSIZE(szName);
    int iRes;

    MDTraceEnter( TRACE_UTIL, "ShowRemovePrompt" );


    if (0==LoadString( g_hInstance, IDS_REMOVE_PROMPT, szFormat, ARRAYSIZE(szFormat) ))
    {
        MDTrace(TEXT("LoadString( IDS_REMOVE_PROMPT ) failed") );
        goto exit_gracefully;
    }

    GetMyDocumentsDisplayName( szName, ARRAYSIZE(szName) );

    if (!GetPersonalPath( szDir, FALSE ))
    {
        MDTrace(TEXT("Unable to get PersonalPath"));
        goto exit_gracefully;
    }

    wsprintf( szPrompt, szFormat, szName, szDir );

    if (0==LoadString( g_hInstance, IDS_REMOVE_TITLE, szFormat, ARRAYSIZE(szFormat) ))
    {
        MDTrace(TEXT("LoadString( IDS_REMOVE_TITLE ) failed") );
        goto exit_gracefully;
    }

    wsprintf( szTitle, szFormat, szName );

    iRes = MessageBox( NULL, szPrompt, szTitle,
                       MB_OKCANCEL | MB_ICONWARNING | MB_DEFBUTTON2 | MB_TASKMODAL | MB_TOPMOST
                      );

    if (iRes == IDOK)
    {
        IDREGITEM idri;
        TCHAR ch = 0;

        MDTrace(TEXT("Was asked to remove the icon..."));
        if (ERROR_SUCCESS!=RegOpenKey( HKEY_CURRENT_USER, c_szDocumentSettings, &hkey2 ))
        {
            MDTrace(TEXT("Unable to open Document key"));
            goto exit_gracefully;
        }

        if (ERROR_SUCCESS!=RegSetValueEx( hkey2, c_szHidden, NULL, REG_SZ, (LPBYTE)&ch, SIZEOF(TCHAR) ))
        {
            MDTrace(TEXT("error settting hidden registry flag"));
            goto exit_gracefully;
        }

        // Remove old sendto menu stuff
        RemoveMenuStuff();
        UpdateSendToFile( TRUE );

        // Add "New MyDocuments folder on Dekstop" back to new menu
        AddNewMenuToRegistry();

        // Remove personal settings...
        RegDelnode( HKEY_CURRENT_USER, (LPTSTR)c_szPerUserCLSID );


        idri.cb = (sizeof(IDREGITEM) - sizeof(WORD));
        idri.bFlags = SHID_ROOT_REGITEM;
        idri.clsid = CLSID_MyDocumentsExt;
        idri.next = 0;

        // tell the shell
        SHChangeNotify( SHCNE_DELETE,     SHCNF_IDLIST, &idri, NULL );
        SHChangeNotify( SHCNE_UPDATEITEM, SHCNF_IDLIST, &idri, NULL );

    }

exit_gracefully:
    if (hkey)
    {
        RegCloseKey( hkey );
    }

    if (hkey2)
    {
        RegCloseKey( hkey2 );
    }

    MDTraceLeave();
}


/*---------------------------------------------------------------------------
/ IsMyDocsHidden
/ ----------------
/
/ Checks special registry value to see if My Docs folder should be hidden
/
/---------------------------------------------------------------------------*/
BOOL IsMyDocsHidden( VOID )
{
    HKEY hkey = NULL;
    DWORD dwSize;
    BOOL fRet = FALSE;

    MDTraceEnter( TRACE_UTIL, "IsMyDocsHidden" );

    if (ERROR_SUCCESS!=RegOpenKey( HKEY_CURRENT_USER, c_szDocumentSettings, &hkey ))
        goto exit_gracefully;

    if (ERROR_SUCCESS==RegQueryValueEx( hkey, c_szHidden, NULL, NULL, NULL, &dwSize ))
    {
        MDTrace(TEXT("returning TRUE (Hidden)"));
        fRet = TRUE;
    }
#ifdef DEBUG
    else
    {
        MDTrace(TEXT("returning FALSE (not hidden)"));
    }
#endif

exit_gracefully:
    if (hkey)
    {
        RegCloseKey( hkey );
    }

    MDTraceLeave();
    return fRet;
}


/*---------------------------------------------------------------------------
/ ComparePaths
/ ------------
/
/ Checks the path to see whether it is the same, etc., as another path
/
/---------------------------------------------------------------------------*/
DWORD ComparePaths( LPTSTR pPath, LPTSTR pCheck )
{
    DWORD dwRet = PATH_IS_DIFFERENT;
    INT iLenCheck = lstrlen( pCheck );
    INT iLenPath = lstrlen( pPath );

    //MDTraceEnter( TRACE_UTIL, "ComparePath" );
    //MDTrace(TEXT("Comparing %s against %s"), pPath, pCheck );

    if (iLenCheck <= iLenPath)
    {
        LPTSTR pPathSlice = pPath + iLenCheck;
        TCHAR cSave = *pPathSlice;

        *pPathSlice = 0;

        if (lstrcmpi( pPath, pCheck ) == 0 )
        {
            if (iLenPath > iLenCheck)
            {
                LPTSTR pTmp = pPathSlice + 1;

                while( *pTmp && *pTmp!=TEXT('\\') )
                {
                    pTmp++;
                }

                if (!(*pTmp))
                {
                    dwRet = PATH_IS_CHILD;
                }

            }
            else
            {
                dwRet = PATH_IS_EQUAL;
            }
        }

        *pPathSlice = cSave;
    }


/*
#ifdef DEBUG
    switch (dwRet)
    {
    case PATH_IS_CHILD:
        MDTrace(TEXT("returning PATH_IS_CHILD"));
        break;

    case PATH_IS_DIFFERENT:
        MDTrace(TEXT("returning PATH_IS_DIFFERENT"));
        break;

    case PATH_IS_EQUAL:
        MDTrace(TEXT("returning PATH_IS_EQUAL"));
        break;

    }
#endif
*/


    //MDTraceLeave();
    return dwRet;
}


/*---------------------------------------------------------------------------
/ IsPathAlreadyShellFolder
/ ------------------------
/
/ Checks the path to see if it is marked as system or read only and
/ then check desktop.ini for CLSID or CLSID2 entry...
/
/---------------------------------------------------------------------------*/
BOOL IsPathAlreadyShellFolder( LPTSTR pPath, DWORD dwAttr )
{

    TCHAR szDesktopIni[ MAX_PATH ];
    TCHAR szBuffer[ MAX_PATH ];

    MDTraceEnter( TRACE_UTIL, "IsPathAlreadyShellFolder" );
    MDTrace(TEXT("pPath = %s, dwAttr = 0x%X"), pPath, dwAttr);

    if (!dwAttr)
    {
        dwAttr = GetFileAttributes( pPath );
    }

    if ( (dwAttr == 0xFFFFFFFF)                 ||
         (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY)) ||
         (!(dwAttr & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY)))
        )
    {
        MDTrace(TEXT("pPath failed attribute check, returning FALSE"));
        MDTraceLeave();
        return FALSE;
    }

    lstrcpy( szDesktopIni, pPath );
    lstrcat( szDesktopIni, c_szDesktopIni );

    MDTrace(TEXT("checking %s..."), szDesktopIni );
    //
    // Check for CLSID entry...
    //
    GetPrivateProfileString( c_szShellInfo,
                             c_szCLSID,
                             c_szMyDocsExt,
                             szBuffer,
                             ARRAYSIZE(szBuffer),
                             szDesktopIni
                            );

    if ( (lstrcmpi( szBuffer, c_szMyDocsExt )!=0) &&
         (lstrcmpi( szBuffer, c_szMyDocsCLSID )!=0)
        )
    {
        MDTrace(TEXT("found desktop.ini w/CLSID in it, returning TRUE"));
        MDTraceLeave();
        return TRUE;
    }

    //
    // Check for CLSID2 entry...
    //
    GetPrivateProfileString( c_szShellInfo,
                             c_szCLSID2,
                             c_szMyDocsExt,
                             szBuffer,
                             ARRAYSIZE(szBuffer),
                             szDesktopIni
                            );

    if ( (lstrcmpi( szBuffer, c_szMyDocsExt )!=0) &&
         (lstrcmpi( szBuffer, c_szMyDocsCLSID )!=0)
        )
    {
        MDTrace(TEXT("found desktop.ini w/CLSID2 in it, returning TRUE"));
        MDTraceLeave();
        return TRUE;
    }

    MDTrace(TEXT("returning FALSE"));
    MDTraceLeave();
    return FALSE;
}

const struct
{
    DWORD dwDir;
    DWORD dwFlags;
    DWORD dwRet;
}
_adirs[] =
{
    { CSIDL_DESKTOP,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_DESKTOP   },
    { CSIDL_PERSONAL,           PATH_IS_EQUAL                , PATH_IS_MYDOCS    },
    { CSIDL_SENDTO,             PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_SENDTO    },
    { CSIDL_RECENT,             PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_RECENT    },
    { CSIDL_HISTORY,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_HISTORY   },
    { CSIDL_COOKIES,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_COOKIES   },
    { CSIDL_PRINTHOOD,          PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_PRINTHOOD },
    { CSIDL_NETHOOD,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_NETHOOD   },
    { CSIDL_STARTMENU,          PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_STARTMENU },
    { CSIDL_TEMPLATES,          PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_TEMPLATES },
    { CSIDL_FAVORITES,          PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_FAVORITES },
    { CSIDL_FONTS,              PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_FONTS     },
    { CSIDL_APPDATA,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_APPDATA   },
    { CSIDL_INTERNET_CACHE,     PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_TEMP_INET },
    { CSIDL_COMMON_STARTMENU,   PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_STARTMENU },
    { CSIDL_COMMON_DESKTOPDIRECTORY, PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_DESKTOP },
};

/*---------------------------------------------------------------------------
/ IsPathGoodMyDocsPath
/ --------------------
/
/ Checks the path to see if it is okay as a MyDocs path
/
/---------------------------------------------------------------------------*/
DWORD IsPathGoodMyDocsPath( HWND hwnd, LPTSTR pPath )
{
    TCHAR szPathToCheck[ MAX_PATH ];
    DWORD dwRes, dwAttr;
    INT i;

    MDTraceEnter( TRACE_UTIL, "IsPathGoodMyDocsPath" );
    MDTrace(TEXT("pPath is %s"), pPath);

    if ((!pPath) || (!(*pPath)))
    {
        return PATH_IS_ERROR;
    }

    //
    // Check to make sure it's a folder
    //

    dwAttr = GetFileAttributes( pPath );
    if (dwAttr == 0xFFFFFFFF)
    {
        MDTrace(TEXT("returning PATH_IS_NONEXISTENT"));
        MDTraceLeave();
        return PATH_IS_NONEXISTENT;
    }

    if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        MDTrace(TEXT("returning PATH_IS_NONDIR"));
        MDTraceLeave();
        return PATH_IS_NONDIR;
    }

    if ( dwAttr & FILE_ATTRIBUTE_READONLY )
    {
        MDTrace(TEXT("returning PATH_IS_READONLY"));
        MDTraceLeave();
        return PATH_IS_READONLY;
    }

    for (i = 0; i < ARRAYSIZE(_adirs); i++)
    {
        //
        // Check for various special shell folders
        //
        if (SHGetSpecialFolderPath( hwnd, szPathToCheck, _adirs[i].dwDir, FALSE ))
        {
            dwRes = ComparePaths( pPath, szPathToCheck );

            if (dwRes & _adirs[i].dwFlags)
            {
#ifdef DEBUG
                switch( _adirs[i].dwRet )
                {
                case PATH_IS_DESKTOP:
                    MDTrace(TEXT("returning PATH_IS_DESKTOP"));
                    break;

                case PATH_IS_MYDOCS:
                    MDTrace(TEXT("returning PATH_IS_MYDOCS"));
                    break;

                case PATH_IS_SENDTO:
                    MDTrace(TEXT("returning PATH_IS_SENDTO"));
                    break;

                }
#endif
                //
                // The inevitable exceptions
                //
                switch(_adirs[i].dwDir) 
                {
                case CSIDL_DESKTOP:
                    if (PATH_IS_CHILD == dwRes) {
                        MDTrace(TEXT("allowing subfolder of CSIDL_DESKTOP"));
                        continue;
                    } // if
                    break;

                default:
                    break;
                } // switch

                MDTraceLeave();
                return _adirs[i].dwRet;
            }
        }
    }

    //
    // Check for the profile directory.  It's not allowed.
    //
    szPathToCheck[0] = 0;
    ExpandEnvironmentStrings( TEXT("%USERPROFILE%"),
                              szPathToCheck,
                              ARRAYSIZE(szPathToCheck)
                             );
    dwRes = ComparePaths( pPath, szPathToCheck );
    if (dwRes == PATH_IS_EQUAL)
    {
        MDTrace(TEXT("returning PATH_IS_PROFILE"));
        MDTraceLeave();
        return PATH_IS_PROFILE;
    }

    //
    // Check for the system directory.  It's not allowed.
    //
    szPathToCheck[0] = 0;
    GetSystemDirectory( szPathToCheck, ARRAYSIZE(szPathToCheck) );
    dwRes = ComparePaths( pPath, szPathToCheck );
    if ((dwRes == PATH_IS_EQUAL) || (dwRes == PATH_IS_CHILD))
    {
        MDTrace(TEXT("returning PATH_IS_SYSTEM"));
        MDTraceLeave();
        return PATH_IS_SYSTEM;
    }


    //
    // Check the window directory.  It's not allowed.
    //
    szPathToCheck[0] = 0;
    GetWindowsDirectory( szPathToCheck, ARRAYSIZE(szPathToCheck) );
    dwRes = ComparePaths( pPath, szPathToCheck );
    if (dwRes == PATH_IS_EQUAL)
    {
        MDTrace(TEXT("returning PATH_IS_WINDOWS"));
        MDTraceLeave();
        return PATH_IS_WINDOWS;
    }

    //
    // Make sure path isn't set as a system or some other kind of
    // folder that already has a CLSID or CLSID2 entry...
    //
    if (IsPathAlreadyShellFolder( pPath, dwAttr ))
    {
        MDTrace(TEXT("returning PATH_IS_SHELLFOLDER"));
        MDTraceLeave();
        return PATH_IS_SHELLFOLDER;
    }

    MDTrace(TEXT("returning PATH_IS_GOOD"));
    MDTraceLeave();
    return PATH_IS_GOOD;

}

/*---------------------------------------------------------------------------
/ GetDefaultPersonalPath
/ ----------------------
/
/ return default personal path for this system/user
/
/ pPath is assumed to be MAX_PATH big
/
/---------------------------------------------------------------------------*/
HRESULT GetDefaultPersonalPath( LPTSTR pPath, BOOL bDefaultDisplayName )
{

    HKEY hkey = NULL;
    TCHAR szDisplayName[ MAX_PATH ];
    TCHAR szMyDocsCLSIDRegPath[ MAX_PATH ];
    HRESULT hr = S_OK;
    ULONG cbSize = sizeof(szDisplayName);
    UINT cchDisplay;

    MDTraceEnter( TRACE_UTIL, "GetDefaultPersonalPath" );

    if (bDefaultDisplayName)
    {
        goto JustDoDefault;
    }

    //
    // Get current display name
    //
    wsprintf(szMyDocsCLSIDRegPath, TEXT("%s\\%s"), c_szCLSID, c_szMyDocsCLSID);

    if ( ERROR_SUCCESS !=
         RegOpenKey(HKEY_CLASSES_ROOT, szMyDocsCLSIDRegPath, &hkey)
        )
        ExitGracefully( hr, E_FAIL, "couldn't open CLSID\\{...} key" );

    if ( ERROR_SUCCESS !=
         RegQueryValueEx( hkey, NULL, 0, NULL, (LPBYTE)szDisplayName, &cbSize )
        )
    {
        MDTrace(TEXT("Couldn't get display name from registry, using resource") );

JustDoDefault:
        cchDisplay = LoadString( g_hInstance,
                                 IDS_DISPLAY_NAME,
                                 szDisplayName,
                                 ARRAYSIZE(szDisplayName)
                                );

        if (!cchDisplay)
            ExitGracefully( hr, E_FAIL, "LoadString failed for display name" );
    }



#ifdef WINNT
    //
    // On NT, the first part of the personal path is %USERPROFILE%...
    //

    if ( !LoadString( g_hInstance, IDS_MYDOCS_ROOT, pPath, MAX_PATH) )
        ExitGracefully( hr, E_FAIL, "couldn't LoadString mydocs path root" );
#else
    if (hkey)
    {
        RegCloseKey( hkey );
        hkey = NULL;
    }

    //
    // On Memphis with profiles turned on, the profile path is in the
    // registry...the c_szProfRecKey only exists if profiles are turned on
    //
    if (ERROR_SUCCESS == RegOpenKey( HKEY_CURRENT_USER, c_szProfRecKey, &hkey))
    {
        DWORD dwType, cbData = MAX_PATH;
        LONG lRes;

        lRes = RegQueryValueEx( hkey,
                                TEXT("ProfileDirectory"),
                                NULL,
                                &dwType,
                                (LPBYTE)pPath,
                                &cbData
                               );

        if ((lRes != ERROR_SUCCESS) || (dwType != REG_SZ))
        {
            goto no_memphis_profiles;
        }

        lstrcat( pPath, TEXT("\\") );
    }
    else
    {
no_memphis_profiles:
        //
        // On Memphis wihout profiles, the first part of the personal path is C:\
        // (or whatever drive Memphis is install on).
        //

        UINT cch;
        LPTSTR pTmp;

        cch = GetWindowsDirectory( pPath, MAX_PATH );
        if ((!cch) || (cch > MAX_PATH))
            ExitGracefully( hr, E_FAIL, "GetWindowDirectory failed" );

        // Now, truncate string after drive specification (after first '\'),
        // ex:  c:\

        for( pTmp = pPath; *pTmp && *pTmp != TEXT('\\'); pTmp++)
            ;

        if (*pTmp != TEXT('\\'))
            ExitGracefully( hr, E_FAIL, "whoops, didn't find a \\ in the windows dir string!" );

        //
        // Now, step one character past the '\' and whack in a '\0'
        pTmp++;
        *pTmp = 0;
    }

#endif

    //
    // Tack "My Documents" onto the first part of the path now...
    //

    lstrcat( pPath, szDisplayName );

exit_gracefully:

    if (hkey)
    {
        RegCloseKey( hkey );
    }

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------
/ IsDekstopIniEmpty
/ -----------------
/
/ Scans a desktop.ini file for sections to see if all of them are empty...
/
/---------------------------------------------------------------------------*/
BOOL IsDesktopIniEmpty( LPTSTR pIniFile )
{
    DWORD nSize = 1024, nRet;
    LPTSTR pTmp, pBuffer = NULL;
    BOOL bRes = FALSE;
    LPTSTR pSection = NULL;

    MDTraceEnter( TRACE_UTIL, "IsDesktopIniEmpty" );

    //
    // First, make sure the file exists!
    //

    if (GetFileAttributes( pIniFile ) == 0xFFFFFFFF)
    {
        return TRUE;
    }

    //
    // Get the section names...
    //

try_again:
    pBuffer = (LPTSTR)LocalAlloc( LPTR, nSize*sizeof(TCHAR) );
    if (!pBuffer)
    {
        MDTrace(TEXT("Not able to allocate %d chars for pBuffer"), nSize);
        goto exit_gracefully;
    }

    nRet = GetPrivateProfileSectionNames( pBuffer, nSize, pIniFile );

    if (!nRet)
    {
        MDTrace(TEXT("GetPrivateProfilesSectionNames(%s) returned 0"),pIniFile);
        goto exit_gracefully;
    }


    if (nRet == (nSize - 2))
    {
        if (nSize <= 8096)
        {
            nSize = (nSize << 1);   // double the size
            LocalFree( pBuffer );
            goto try_again;
        }
        else
        {
            goto exit_gracefully;
        }
    }

    //
    // Got the section names, now check to see if each one is empty...
    //

    pTmp = pBuffer;
    nSize = 1024;
    while( *pTmp )
    {

try_again_section:
        if (pSection)
        {
            LocalFree( pSection );
        }

        pSection = (LPTSTR)LocalAlloc( LPTR, nSize*sizeof(TCHAR) );
        if (!pSection)
        {
            MDTrace(TEXT("Not able to allocate %d chars for pSection"), nSize);
            goto exit_gracefully;
        }

        GetPrivateProfileSection( pTmp, pSection, nSize, pIniFile );

        if (nRet == (nSize - 2))
        {
            if (nSize <= 8096)
            {
                nSize = (nSize << 1);   // double the size
                goto try_again_section;
            }
            else
            {
                goto exit_gracefully;
            }
        }

#ifdef DEBUG
        if (!nRet)
        {
            MDTrace(TEXT("GetPrivateProfileSection(%s) returned 0"),pTmp);
        }
#endif

        if (nRet)
        {
            if (*pSection)
            {
                MDTrace(TEXT("Section %s is not empty!"), pTmp );
                goto exit_gracefully;
            }
        }

#ifdef DEBUG
        MDTrace(TEXT("Section %s is empty"), pTmp );
#endif
        pTmp += (lstrlen( pTmp )+1);

    }

    bRes = TRUE;

exit_gracefully:

    if (pBuffer)
    {
        LocalFree( pBuffer );
    }

    if (pSection)
    {
        LocalFree( pSection );
    }

    MDTraceLeave();
    return bRes;

}

/*---------------------------------------------------------------------------
/ MyDocsUnmakeSystemFolder
/ ------------------------
/
/ Remove our entries from the desktop.ini file in this directory, and
/ then test the desktop.ini to see if it's empty.  If it is, delete it
/ and remove the system/readonly bit from the directory...
/
/---------------------------------------------------------------------------*/
void MyDocsUnmakeSystemFolder( LPTSTR pPath )
{
    TCHAR szIniFile[ MAX_PATH ];

    lstrcpy( szIniFile, pPath );
    lstrcat( szIniFile, c_szDesktopIni );

    // Remove CLSID2
    WritePrivateProfileString( c_szShellInfo,
                               c_szCLSID2,
                               NULL,
                               szIniFile
                              );


    // Remove InfoTip
    WritePrivateProfileString( c_szShellInfo,
                               TEXT("InfoTip"),
                               NULL,
                               szIniFile
                              );

    if (IsDesktopIniEmpty(szIniFile))
    {
        DWORD dwAttrb = GetFileAttributes( szIniFile );

        if (dwAttrb != 0xFFFFFFFF)
        {
            dwAttrb &= (~( FILE_ATTRIBUTE_SYSTEM |
                           FILE_ATTRIBUTE_READONLY |
                           FILE_ATTRIBUTE_HIDDEN
                          )
                        );
            SetFileAttributes( szIniFile, dwAttrb );
            DeleteFile( szIniFile );
        }
        PathUnmakeSystemFolder( pPath );
    }

}


/*---------------------------------------------------------------------------
/ ChangePersonalPath
/ ------------------
/
/ Change CSIDL_PERSONAL to the specified path.  If pOldPath is NULL we
/ assume we're creating CSIDL_PERSONAL from setup, and can therefore
/ whack the registry directly.
/
/---------------------------------------------------------------------------*/
HRESULT ChangePersonalPath( LPTSTR pNewPath, LPTSTR pOldPath )
{
    HKEY hkey = NULL;
    HRESULT hr = S_OK;
    LPITEMIDLIST pidlOld = NULL, pidlNew = NULL;
    TCHAR szNewPath[ MAX_PATH ];
    TCHAR szOldPath[ MAX_PATH ];
    TCHAR szExpPath[ MAX_PATH ];
    INT iLen, iLenPath;
    TCHAR cSave;

    MDTraceEnter( TRACE_UTIL, "ChangePersonalPath" );

    *szNewPath = 0; *szOldPath = 0;
    if (pNewPath)
        ExpandEnvironmentStrings( pNewPath, szNewPath, MAX_PATH );

    if (pOldPath)
        ExpandEnvironmentStrings( pOldPath, szOldPath, MAX_PATH );

    if (pOldPath && pNewPath)
    {
        if ( (!(*szOldPath)) || (!(*szNewPath)) )
        {
            ExitGracefully( hr, E_INVALIDARG, "One of the paths is blank!" );
        }

        pidlOld = ILCreateFromPath( szOldPath );
        //
        // pidlOld will be NULL if the szOldPath was deleted somehow.
        // We must deal with this case.
        //

        pidlNew = ILCreateFromPath( szNewPath );
        if (!pidlNew)
        {
            ExitGracefully( hr, E_OUTOFMEMORY, "unable to allocate pidlNew" );
        }

        //SHChangeNotify( SHCNE_RENAMEFOLDER, SHCNF_IDLIST, pidlOld, pidlNew );

        //
        // Clean up old attributes and desktop.ini, if possible...
        // Will fail silently if szOldPath doesn't exist.  Should be
        // fine.
        //
        MyDocsUnmakeSystemFolder( szOldPath );


    }

    //
    // Check if new path should be %USERPROFILE%...
    //

    ExpandEnvironmentStrings( TEXT("%USERPROFILE%"), szExpPath, ARRAYSIZE(szExpPath) );
    iLen = lstrlen( szExpPath );
    iLenPath = lstrlen( szNewPath );
    if (iLen < iLenPath)
    {
        cSave = szNewPath[ iLen ];
        szNewPath[ iLen ] = 0;

        //
        // Is it the profile directory?
        //
        if ( (lstrcmpi( szNewPath, szExpPath )==0) &&
             (cSave == TEXT('\\'))
            )
        {
            lstrcpy( szExpPath, TEXT("%USERPROFILE%\\") );
            lstrcat( szExpPath, &szNewPath[ iLen+1 ] );
            lstrcpy( szNewPath, szExpPath );
        }
        else
        {
            szNewPath[ iLen ] = cSave;
        }
    }

    if ( ERROR_SUCCESS !=
         RegCreateKey(HKEY_CURRENT_USER, c_szRegistrySettings, &hkey)
        )
    {
        ExitGracefully( hr, E_FAIL, "couldn't open or create user shell folder key" );
    }

    #ifdef WINNT
    #define MY_REG_TYPE REG_EXPAND_SZ
    #else
    #define MY_REG_TYPE REG_SZ
    #endif

    // add our CSIDL_PERSONAL path...
    if ( ERROR_SUCCESS !=
         RegSetValueEx( hkey, c_szPersonal, 0, MY_REG_TYPE,
                        (CONST BYTE *)szNewPath,
                        (lstrlen(szNewPath)+1)*sizeof(TCHAR)
                       )
        )
    {
        ExitGracefully( hr, E_FAIL, "couldn't set CSIDL_PERSONAL path" );
    }


    //
    // Tell the shell that their cached pidl is probably out of date...
    //
    SHFlushSFCache();

exit_gracefully:
    if (pidlOld)
    {
        SHChangeNotify( SHCNE_UPDATEITEM, SHCNF_IDLIST, pidlOld, NULL );
        ILFree( pidlOld );
    }

    if (pidlNew)
    {
        SHChangeNotify( SHCNE_UPDATEITEM, SHCNF_IDLIST, pidlNew, NULL );
        ILFree( pidlNew );
    }

    if (hkey)
    {
        RegCloseKey( hkey );
    }

    MDTraceLeaveResult(hr);
}


/*---------------------------------------------------------------------------
/ StampMyDocsFolder
/ ------------------
/
/ Stamp desktop.ini & attrbiutes into specified folder
/
/ pPath is assumed to be at least MAX_PATH big (and writeable)
/
/---------------------------------------------------------------------------*/
HRESULT StampMyDocsFolder( LPTSTR pPath )
{
    DWORD dwAttr;
    HRESULT hr = S_OK;
    TCHAR szInfo[ MAX_PATH ];
    TCHAR szPath[ MAX_PATH ];

    MDTraceEnter( TRACE_UTIL, "StampMyDocsFolder" );

    // Make sure it doesn't need to be expanded...
    ExpandEnvironmentStrings( pPath, szPath, MAX_PATH );

#if 0
    // Don't want to stamp things that aren't in our system...
    if (!PathIsLocalAndWriteable( szPath ))
        ExitGracefully( hr, S_OK, "Not stamping non-local path" );

    // Don't want to stamp root of drives
    if (PathIsRootOfDrive( szPath ))
        ExitGracefully( hr, S_OK, "Not stamping root of drive" );
#endif

    // Mark the folder as read only or system...
    PathMakeSystemFolder( szPath );

    lstrcat( szPath, c_szDesktopIni );

    // set UICLSID
    if (!WritePrivateProfileString( c_szShellInfo,
                                    TEXT("UICLSID"),
                                    NULL,
                                    szPath
                                   )
        )
        ExitGracefully( hr, E_FAIL, "WritePrivateProfileString on UICLSID failed!" );

    // set CLSID
    if (!WritePrivateProfileString( c_szShellInfo,
                                    c_szCLSID,
                                    NULL,
                                    szPath
                                   )
        )
        ExitGracefully( hr, E_FAIL, "WritePrivateProfileString on CLSID failed!" );



    // Add CLSID2
    if (!WritePrivateProfileString( c_szShellInfo,
                                    c_szCLSID2,
                                    c_szMyDocsCLSID,
                                    szPath
                                   )
        )
        ExitGracefully( hr, E_FAIL, "WritePrivateProfileString on CLSID2 failed!" );

    // Add InfoTip
    szInfo[0] = 0;
    LoadString( g_hInstance, IDS_INFOTIP_VALUE, szInfo, ARRAYSIZE(szInfo) );
    if (!WritePrivateProfileString( c_szShellInfo,
                                    TEXT("InfoTip"),
                                    szInfo,
                                    szPath
                                   )
        )
        ExitGracefully( hr, E_FAIL, "WritePrivateProfileString on CLSID failed!" );



    // Mark desktop.ini as a hidden file...
    dwAttr = GetFileAttributes( szPath );

    if ((dwAttr != 0xFFFFFFFF) && (!(dwAttr & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))))
    {
        SetFileAttributes( szPath, dwAttr | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    }

exit_gracefully:

    MDTraceLeaveResult(hr);
}


/*---------------------------------------------------------------------------
/ QueryCreateTheDirectory
/ -----------------------
/
/ Ask the user if they want to create the directory of a given path.  Return
/ FILE_ATTRIBUTE_DIRECTORY if yes, otherwise return 0.
/
/---------------------------------------------------------------------------*/
DWORD QueryCreateTheDirectory( HWND hDlg, LPTSTR pPath )
{
    if (IDNO == ShellMessageBox( g_hInstance, hDlg,
                                 (LPTSTR)IDS_CREATE_FOLDER, (LPTSTR)IDS_CREATE_FOLDER_TITLE,
                                 MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL,
                                 pPath
                                )
        )
    {
        return 0;
    }

    // user asked us to create the folder
    if (CreateDirectory( pPath, NULL ))
    {
        return GetFileAttributes( pPath );
    }

    return 0;

}

/*---------------------------------------------------------------------------
/ CanChangePersonalPath
/ ---------------------
/
/ Check known key in the registry to see if policy has disabled changing
/ of My Docs location.
/
/---------------------------------------------------------------------------*/
BOOL CanChangePersonalPath( void )
{
    HKEY hkey;
    BOOL bChange = TRUE;

    if ( ERROR_SUCCESS ==
             RegOpenKeyEx( HKEY_CURRENT_USER, c_szPolicy, 0, KEY_READ, &hkey )
        )
    {
        DWORD dwType, dwValue, dwSize = sizeof(DWORD);


        if ( ERROR_SUCCESS ==
                 RegQueryValueEx( hkey, c_szDisableChange, NULL,
                                  &dwType, (LPBYTE)&dwValue, &dwSize )
            )
        {
            if ((dwType == REG_DWORD) && (dwValue == 1))
            {
                bChange = FALSE;
            }
        }

        RegCloseKey( hkey );
    }

    return bChange;

}

/*---------------------------------------------------------------------------
/ MyItoA
/ ------
/
/ Assumes a lot -- MAX_INT is 2,000,000,000 and pNum can hold up to
/ ten digits at least.
/
/---------------------------------------------------------------------------*/
void MyIToA( INT iNum, LPTSTR pNum )
{
    INT dv = 1000000000;
    INT d;
    BOOL bDigit = FALSE;

    if (iNum < 0)
    {
        *pNum++ = TEXT('-');
        iNum = -iNum;
    }

    while( dv )
    {
        d = (iNum / dv);

        if (d || bDigit)
        {
            bDigit = TRUE;

            *pNum++ = (TCHAR)((INT)TEXT('0')+d);

            iNum -= (d * dv);
        }

        dv = (dv / 10);
    }

    *pNum = 0;

}


/*---------------------------------------------------------------------------
/ ManageMyDocsIconPathAndIndex
/ ----------------------------
/
/ Either sets or retrives the current settings (per-user if possible) of
/ the default MyDocs icon.  pIconPath is assumed to be MAX_PATH big.
/
/---------------------------------------------------------------------------*/
const TCHAR c_szDefaultIcon[] = TEXT("\\DefaultIcon");
void ManageMyDocsIconPathAndIndex( LPTSTR pIconPath, INT cch, INT * pIndex, BOOL bSet )
{
    HKEY hkey = NULL;
    TCHAR szDigit[ 15 ];
    TCHAR szIcon[ MAX_PATH ];
    TCHAR szPath[ MAX_PATH ];


    lstrcpy( szPath, c_szPerUserCLSID );
    lstrcat( szPath, c_szDefaultIcon );

    if (bSet)
    {
        MyIToA( *pIndex, szDigit );
        lstrcpy( szIcon, pIconPath );
        lstrcat( szIcon, TEXT(",") );
        lstrcat( szIcon, szDigit );


        if (ERROR_SUCCESS == RegCreateKey( HKEY_CURRENT_USER,
                                           szPath,
                                           &hkey
                                          )
            )
        {
            RegSetValue( hkey,
                         NULL,
                         REG_SZ,
                         szIcon,
                         lstrlen(szIcon)*sizeof(TCHAR)
                        );
        }

    }
    else
    {
        szIcon[0] = 0;

        // Try to get per-user icon settings first...
        if (ERROR_SUCCESS == RegOpenKey( HKEY_CURRENT_USER, szPath, &hkey ))
        {
            LONG cbSize = sizeof(szIcon);

            RegQueryValue( hkey, NULL, szIcon, &cbSize );
        }

        // If no per-user, then check CLSID key...
        if (szIcon[0] == 0)
        {
            if (hkey)
            {
                RegCloseKey( hkey );
                hkey = NULL;
            }

            lstrcpy( szPath, c_szCLSIDFormat );
            lstrcat( szPath, c_szDefaultIcon );
            if (ERROR_SUCCESS == RegOpenKey( HKEY_CLASSES_ROOT, szPath, &hkey ))
            {
                LONG cbSize = sizeof(szIcon);

                RegQueryValue( hkey, NULL, szIcon, &cbSize );
            }
        }

        // If no per-user or CLSID settings, use hard wired default
        if (szIcon[0] == 0)
        {
            lstrcpy( szIcon, TEXT("mydocs.dll,0") );
        }

        lstrcpyn( pIconPath, szIcon, cch );
        *pIndex = PathParseIconLocation( pIconPath );
    }

    if (hkey)
    {
        RegCloseKey( hkey );
    }
}

#ifdef DEBUG
/*---------------------------------------------------------------------------
/ GetPersonalPath
/ ----------------
/
/---------------------------------------------------------------------------*/
BOOL GetPersonalPath( LPTSTR pPath, BOOL fCreate )
{

    BOOL bRes;

    MDTraceEnter( TRACE_UTIL, "GetPersonalPath" );

    bRes = SHGetSpecialFolderPath( NULL, pPath, CSIDL_PERSONAL, fCreate );

    if (!bRes)
    {
        MDTrace(TEXT("call to SHGetSpecialFolderPath failed!" ));
    }
    else
    {
        MDTrace( TEXT("SHGetSpecFolder yields pPath as -%s-"), pPath );
    }

    MDTraceLeave();
    return bRes;

}
#endif

#if (defined(DEBUG) && defined(SHOW_PATHS))
/*---------------------------------------------------------------------------
/ PrintPath
/ ----------------
/   Debug code to print out a path, given a pidl.
/
/---------------------------------------------------------------------------*/
void PrintPath( LPITEMIDLIST pidl )
{
    TCHAR szPath[ MAX_PATH ];
    LPITEMIDLIST pidlTmp;

    pidlTmp = SHIDLFromMDIDL( pidl );

    if (SHGetPathFromIDList( pidlTmp, szPath ))
    {
        LPTSTR pFileName = PathFindFileName( szPath );

        MDTrace(TEXT("shell pidl points to '%s'"), pFileName );
    }
    else
    {
        MDTrace(TEXT("*** Couldn't get path from shell pidl! ***"));
    }
}

/*---------------------------------------------------------------------------
/ StrretToString
/ ----------------
/   Converts a strret to a sz
/
/---------------------------------------------------------------------------*/
void StrretToString( LPSTRRET pStr, LPITEMIDLIST pidl, LPTSTR psz, UINT cch )
{
    switch( pStr->uType )
    {
        case STRRET_WSTR:
#ifdef UNICODE
            lstrcpy( psz, pStr->pOleStr );
#else
            lstrcpy( psz, TEXT("NO CONV OLESTR <--> ANSI") );
#endif
            break;

        case STRRET_OFFSET:
#ifdef UNICODE
            MultiByteToWideChar( CP_ACP, 0,
                                 (LPSTR)((LPBYTE)pidl + pStr->uOffset),
                                 -1, psz, cch
                                );
#else
            lstrcpy( psz, (LPTSTR)((LPBYTE)pidl + pStr->uOffset) );
#endif
            break;

        case STRRET_CSTR:
#ifdef UNICODE
            MultiByteToWideChar( CP_ACP, 0,
                                 pStr->cStr, -1,
                                 psz, cch
                                );
#else
            lstrcpy( psz, pStr->cStr );
#endif
    }

}
#endif


#if (defined(DEBUG) && defined(SHOW_ATTRIBUTES))
/*---------------------------------------------------------------------------
/ PrintAttributes
/ ----------------
/
/
/---------------------------------------------------------------------------*/
void PrintAttributes( DWORD dwAttr )
{
    TCHAR sz[ MAX_PATH ];

    lstrcpy( sz, TEXT("Attribs = ") );
    if (dwAttr & SFGAO_CANCOPY)
    {
        lstrcat( sz, TEXT("SFGAO_CANCOPY ") );
    }
    if (dwAttr & SFGAO_CANMOVE)
    {
        lstrcat( sz,TEXT("SFGAO_CANMOVE "));
    }
    if (dwAttr & SFGAO_CANLINK)
    {
        lstrcat( sz, TEXT("SFGAO_CANLINK "));
    }
    if (dwAttr & SFGAO_CANRENAME)
    {
        lstrcat( sz, TEXT("SFGAO_CANRENAME "));
    }
    if (dwAttr & SFGAO_CANDELETE)
    {
        lstrcat( sz, TEXT("SFGAO_CANDELETE "));
    }
    if (dwAttr & SFGAO_HASPROPSHEET)
    {
        lstrcat( sz, TEXT("SFGAO_HASPROPSHEET "));
    }
    if (dwAttr & SFGAO_DROPTARGET)
    {
        lstrcat( sz, TEXT("SFGAO_DROPTARGET "));
    }
    if (dwAttr & SFGAO_LINK)
    {
        lstrcat( sz, TEXT("SFGAO_LINK "));
    }
    if (dwAttr & SFGAO_SHARE)
    {
        lstrcat( sz, TEXT("SFGAO_SHARE "));
    }
    if (dwAttr & SFGAO_READONLY)
    {
        lstrcat( sz, TEXT("SFGAO_READONLY "));
    }
    if (dwAttr & SFGAO_GHOSTED)
    {
        lstrcat( sz, TEXT("SFGAO_GHOSTED "));
    }
    if (dwAttr & SFGAO_HIDDEN)
    {
        lstrcat( sz, TEXT("SFGAO_HIDDEN "));
    }
    if (dwAttr & SFGAO_FILESYSANCESTOR)
    {
        lstrcat( sz, TEXT("SFGAO_FILESYSANCESTOR "));
    }
    if (dwAttr & SFGAO_FILESYSTEM)
    {
        lstrcat( sz, TEXT("SFGAO_FILESYSTEM "));
    }
    if (dwAttr & SFGAO_HASSUBFOLDER)
    {
        lstrcat( sz, TEXT("SFGAO_HASSUBFOLDER "));
    }
    if (dwAttr & SFGAO_VALIDATE)
    {
        lstrcat( sz, TEXT("SFGAO_VALIDATE "));
    }
    if (dwAttr & SFGAO_REMOVABLE)
    {
        lstrcat( sz, TEXT("SFGAO_REMOVABLE "));
    }
    if (dwAttr & SFGAO_COMPRESSED)
    {
        lstrcat( sz, TEXT("SFGAO_COMPRESSED "));
    }
    if (dwAttr & SFGAO_BROWSABLE)
    {
        lstrcat( sz, TEXT("SFGAO_BROWSABLE "));
    }
    if (dwAttr & SFGAO_NONENUMERATED)
    {
        lstrcat( sz, TEXT("SFGAO_NONENUMERATED "));
    }
    if (dwAttr & SFGAO_NEWCONTENT)
    {
        lstrcat( sz, TEXT("SFGAO_NEWCONTENT "));
    }
    MDTrace( sz );
}
#endif
