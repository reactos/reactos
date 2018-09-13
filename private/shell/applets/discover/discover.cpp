//////////////////////////////////////////////////////////////////////////
//
//  Discover.cpp
//
//      This file contains the main entry point into the application and
//      the implementation of the discover program.
//
//  (C) Copyright 1997 by Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <shlwapi.h>    // for string and path functions
#include <shlobj.h>     // SHBrowseForFolder
#include <shlobjp.h>    // ILFree
#include "resource.h"
#include "webapp.h"

#pragma hdrstop


//////////////////////////////////////////////////////////////////////////
// Global Data
//////////////////////////////////////////////////////////////////////////
TCHAR szInstallPath[MAX_PATH];      // OS install path
TCHAR szLastUsedPath[MAX_PATH];     // last path used, or empty sting if there's no last used path
TCHAR szzCDRoms[MAX_PATH];          // list of all available CDRom drives

TCHAR g_szPath[MAX_PATH];           // The tour location.  Ex. "e:\discover\default.htm" or "\\ntbuilds\...\discover\default.htm"

//////////////////////////////////////////////////////////////////////////
// Prototypes
//////////////////////////////////////////////////////////////////////////
BOOL GetPathFromReg(HKEY hkey, LPTSTR szSubKey, LPTSTR szPath, int cch);
BOOL GetCDRomPaths(LPTSTR szzPaths, int cch);
BOOL CheckAllPaths(BOOL bCDOnly);
BOOL CheckForTour(LPTSTR pszPath);
INT_PTR DlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

//////////////////////////////////////////////////////////////////////////
// Macros
//////////////////////////////////////////////////////////////////////////
#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))

#define SZ_INSTALLKEY   TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup")
#define SZ_TOURKEY      TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Applets\\Discover Tour")


/**
*  This function is the main entry point into our application.
*
*  @return     int     Exit code.
*/

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLin, int nShowCmd )
{
    HWND hwndDesktop = GetDesktopWindow();
    HRESULT hrInit = CoInitialize(NULL);        // needed for SHAutoComplete used below


    // Look in the registry to determine the tour path.  We want to try both the last path used and
    // windows installation directory.

    GetPathFromReg(HKEY_LOCAL_MACHINE, SZ_INSTALLKEY, szInstallPath,  ARRAYSIZE(szInstallPath));
    GetPathFromReg(HKEY_CURRENT_USER,  SZ_TOURKEY,    szLastUsedPath, ARRAYSIZE(szLastUsedPath));
    GetCDRomPaths(szzCDRoms, ARRAYSIZE(szzCDRoms));


    // Look in several common locations to find the discover tour.  We want to check the following
    // locations (in order):  the last used directory, the install directory, all CD Rom drives.  As
    // soon as we find the tour, stop looking and show the tour.

    BOOL bDiscoverFound = CheckAllPaths( FALSE );

    // if the install path is a CD and the tour isn't found ask them to insert the Windows CD
    if ( !bDiscoverFound && (DRIVE_CDROM == GetDriveType(szInstallPath)) )
    {
        // TODO: supress CD Autoplay during this interval, but only for the Windows 2000 CD.
        // do this by creating the "autorun is running" mutex.

        while (!bDiscoverFound)
        {
            TCHAR szCaption[128];
            TCHAR szText[1024];

            LoadString(hInstance, IDS_NOCDTITLE, szCaption, ARRAYSIZE(szCaption));
            LoadString(hInstance, IDS_NOCDTEXT, szText, ARRAYSIZE(szText));
            if ( IDOK != MessageBox(hwndDesktop, szText, szCaption, MB_OKCANCEL) )
            {
                break;
            }

            // we check all available CD-Rom drives by calling CheckAllPaths and passing TRUE
            bDiscoverFound = CheckAllPaths( TRUE );
        }

        // TODO: release the mutex created above
    }


    // While not found, ask user to type or browse to a new location (network case)

    while ( !bDiscoverFound )
    {
        LONG result;
        result = (LONG)DialogBox(hInstance, MAKEINTRESOURCE(IDD_TOUR), hwndDesktop, DlgProc);
        if ( IDCANCEL == result )
        {
            break;
        }

        // The above dialog sets the szLastUsedPath to the path entered by the user.
        // Check for the tour on that path:
        bDiscoverFound = CheckForTour(szLastUsedPath);
    }


    // 4.) Launch an explorer control to display the tour.  We have to host a theater mode
    //      control because this is what the code expects.  We need to provide a window.external
    //      with the exit command as well.

    if ( bDiscoverFound )
    {
        HKEY hkey;

        if ( ERROR_SUCCESS == RegCreateKey( HKEY_CURRENT_USER, SZ_TOURKEY, &hkey ) )
        {
            // We always store the szLastUsedPath.  This path might be an empty string but if it is then
            // we want to replace any previous value.
            RegSetValueEx(hkey, TEXT("SourcePath"), 0, REG_SZ, (LPBYTE)szLastUsedPath, ARRAYSIZE(szLastUsedPath));

            // The Direct Music control needs to know the file path for the tour files.  Since this
            // path might be different every time we run, we set the value into the registry:
            TCHAR szMusicPath[MAX_PATH];
            StrCpy( szMusicPath, g_szPath );
            PathRemoveFileSpec( szMusicPath );
            PathAppend( szMusicPath, TEXT("music") );
            RegSetValueEx(hkey, TEXT("DirectMusicPath"), 0, REG_SZ, (LPBYTE)szMusicPath, ARRAYSIZE(szMusicPath));
            RegCloseKey(hkey);
        }

        CWebApp webapp;

        if ( webapp.Register(hInstance) )
        {
            webapp.Create();
            webapp.MessageLoop();
        }
    }


    // 5.) Shutdown and clean up

    if ( SUCCEEDED(hrInit) )
    {
        CoUninitialize();
    }

    return 0;
}

INT_PTR DlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND hwndEdit = GetDlgItem(hwnd, IDC_PATH);
            SetWindowText(hwndEdit, ((*szLastUsedPath)?szLastUsedPath:szInstallPath));
            SendMessage(hwndEdit, EM_SETSEL, 0, -1);
            SetFocus(hwndEdit);
            //SHAutoComplete(hwndEdit, SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);
        }
        return FALSE;

    case WM_COMMAND:
        switch ( LOWORD(wParam) )
        {
        case IDOK:
            // update szPath
            GetDlgItemText(hwnd, IDC_PATH, szLastUsedPath, ARRAYSIZE(szLastUsedPath));

            // fall through
        case IDCANCEL:
            EndDialog(hwnd, LOWORD(wParam));
            break;

        case IDC_BROWSE:
            {
                BROWSEINFO bi = {0};
                LPITEMIDLIST pidl;
                TCHAR szName[1024];
                HINSTANCE hinst = GetModuleHandle(NULL);

                LoadString(hinst, IDS_BROWSETEXT, szName, ARRAYSIZE(szName));

                bi.hwndOwner = hwnd;
                bi.pszDisplayName = szName;
                bi.lpszTitle = szName;
                bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
                pidl = SHBrowseForFolder(&bi);

                if ( NULL != pidl )
                {
                    SHGetPathFromIDList(pidl, szLastUsedPath);
                    ILFree(pidl);
                    SetDlgItemText(hwnd, IDC_PATH, szLastUsedPath);
                    SetFocus(GetDlgItem(hwnd, IDOK));
                }
            }
            break;

        default:
            return 0;
        }
        break;

    default:
        return 0;
    }

    return 1;
}


// GetPathFromReg
//
// Reads a path from the "SourcePath" value of the given HKEY and subkey and expandes
// environment variables if needed.
BOOL GetPathFromReg(HKEY hkey, LPTSTR szSubKey, LPTSTR szPath, int cch)
{
    DWORD result;

    result = RegOpenKeyEx(hkey, szSubKey, 0, KEY_QUERY_VALUE, &hkey);
    if ( ERROR_SUCCESS == result )
    {
        DWORD dwType;
        DWORD dwSize = cch;

        result = RegQueryValueEx( hkey, TEXT("SourcePath"), 0, &dwType, (LPBYTE)szPath, &dwSize );
        if ( ERROR_SUCCESS == result )
        {
            if ( dwType == REG_EXPAND_SZ )
            {
                // Expand environment variables
                TCHAR szTemp[MAX_PATH];
                ExpandEnvironmentStrings( szPath, szTemp, ARRAYSIZE(szTemp) );
                StrCpyN( szPath, szTemp, cch );
            }
            else if ( dwType != REG_SZ )
            {
                // set to any old error value.
                result = ERROR_MORE_DATA;

                // NULL out whatever bogus data we read
                *szPath = NULL;
            }
        }
        else
        {
            // ensure NULL termination if QueryValue fails
            *szPath = NULL;
        }

        RegCloseKey(hkey);
    }

    return (ERROR_SUCCESS == result);
}


// GetCDRomPaths
//
// Gets a double null terminated list of CDRom paths.  Excludes the CD Rom if it is already listed
// in the InstallPath or LastUsed path.
BOOL GetCDRomPaths(LPTSTR szzPaths, int cch)
{
    int i;
    TCHAR szDrive[4] = TEXT("A:\\");
    DWORD dwDrives = GetLogicalDrives();
    DWORD dwMask;

    // reserve one space for final NULL character
    cch--;

    for ( i=0, dwMask=1; i<26; i++, dwMask*=2 )
    {
        if ( dwDrives & dwMask )
        {
            szDrive[0] = TEXT('A') + i;
            if ( GetDriveType(szDrive) == DRIVE_CDROM )
            {
                if ( (0!=StrCmpI(szDrive, szInstallPath)) && (0!=StrCmpI(szDrive, szLastUsedPath)))
                {
                    int cchSzDrive = lstrlen(szDrive) + 1;
                    StrCpyN(szzPaths, szDrive, cch);
                    cch -= cchSzDrive;
                    szzPaths += cchSzDrive;
                    *szzPaths = 0;
                }
            }
        }
    }

    return TRUE;
}

// CheckAllPaths
//
// Check all paths in szPathList to see if the discover tour can be found.
// If found the path to the tour is placed in g_szPath.
BOOL CheckAllPaths(BOOL bCDOnly)
{
    if ( !bCDOnly )
    {
        if ( CheckForTour(szLastUsedPath) || CheckForTour(szInstallPath) )
        {
            return TRUE;
        }
    }

    LPTSTR psz = szzCDRoms;
    while (*psz)
    {
        if (CheckForTour(psz))
        {
            return TRUE;
        }

        psz += lstrlen(psz) + 1;
    }

    return FALSE;
}

// CheckForTour
//
// Check the given path for the discover tour.  If found, the tour path is placed into g_szPath.
BOOL CheckForTour(LPTSTR pszPath)
{
    BOOL bDiscoverFound;

    // The user is expected to select their installation path, from there we look for
    // the discover directory:
    PathCombine(g_szPath, pszPath, TEXT("Discover\\default.htm"));
    bDiscoverFound = PathFileExists(g_szPath);

    // In case the user is one step ahead of us and actually selected the discover
    // tour directory we search for the tour in the given location also:
    if ( !bDiscoverFound )
    {
        PathCombine(g_szPath, pszPath, TEXT("default.htm"));
        bDiscoverFound = PathFileExists(g_szPath);
    }

    return bDiscoverFound;
}