/*++

Copyright (c) 1995  Microsoft Corporation

File Name:

     cachecpl.c

Module :

    inetcpl.cpl

Abstract:

    This file contains code to set cache config information from the internet
    control panel

Author:

    Shishir Pardikar

6/22/96 t-gpease    moved entire dailog to this file from "dialdlg.c"

Environment:

    User Mode - Win32

Revision History:

--*/

#include "inetcplp.h"
#include "cachecpl.h"

#include <mluisupp.h>
#include <winnls.h>

#ifdef unix
#define DIR_SEPARATOR_CHAR TEXT('/')
#else
#define DIR_SEPARATOR_CHAR TEXT('\\')
#endif /* unix */

#define CONSTANT_MEGABYTE   (1024*1024)

INT_PTR CALLBACK 
EmptyCacheDlgProc(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam,
    LPARAM lParam);


#ifdef UNICODE
/* GetDiskInfo
    A nice way to get volume information
*/
BOOL GetDiskInfo(PTSTR pszPath, PDWORD pdwClusterSize, PDWORDLONG pdlAvail, 
PDWORDLONG pdlTotal)
{
    CHAR  szGDFSEXA[MAX_PATH];
    SHUnicodeToAnsi(pszPath, szGDFSEXA, ARRAYSIZE(szGDFSEXA));
    return GetDiskInfoA(szGDFSEXA, pdwClusterSize, pdlAvail, pdlTotal);
}
#else
#define GetDiskInfo     GetDiskInfoA
#endif

/* DispMessage
    A quick and easy way to display messages for the cachecpl
*/

INT DispMessage(HWND hWnd, UINT Msg, UINT Title, UINT Type)
{
    TCHAR szTitle[80];
    TCHAR szMessage[1024];
    
    // something went wrong with the registry
    // notify user
    MLLoadShellLangString(Msg, szMessage, ARRAYSIZE(szMessage));
    MLLoadShellLangString(Title, szTitle, ARRAYSIZE(szTitle));

    return MessageBox(hWnd, szMessage, szTitle, Type);
}

typedef HRESULT (*PFNSHGETFOLDERPATH)(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath);

#undef SHGetFolderPath
#ifdef UNICODE
#define SHGETFOLDERPATH_STR "SHGetFolderPathW"
#else
#define SHGETFOLDERPATH_STR "SHGetFolderPathA"
#endif

HRESULT SHGetFolderPath(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath)
{
    HMODULE hmodSHFolder = LoadLibrary(TEXT("shfolder.dll"));
    HRESULT hr = E_FAIL;
    
    if (hmodSHFolder) 
    {
        PFNSHGETFOLDERPATH pfn = (PFNSHGETFOLDERPATH)GetProcAddress(hmodSHFolder, SHGETFOLDERPATH_STR);
        if (pfn)
        {
            hr = pfn(hwnd, csidl, hToken, dwFlags, pszPath);
        }
        FreeLibrary(hmodSHFolder);
    }
    return hr;
}


// Cache maximum/minimum in MB
#define CACHE_SIZE_CAP 32000
#define CACHE_SIZE_MIN 1


DWORD UpdateCacheQuotaInfo(LPTEMPDLG pTmp, BOOL fUpdate)
{
    // The following probably needs to be fixed.
    DWORDLONG cKBLimit = pTmp->uiDiskSpaceTotal, cKBSpare = pTmp->uiCacheQuota;

    if (cKBLimit==0)
    {
        return GetLastError();
    }

    // What's happening in the following sequence:
    // We want to ensure that the cache size is
    // 1.   less than the drive's size (if larger, then reduce to 75% of drive's space
    // 2.   less than 32 GB

    // And adjust percentage accordingly.
    
    if (fUpdate)
    {
        ASSERT(pTmp->iCachePercent<=100);
        if (pTmp->iCachePercent==0)
        {
            cKBSpare = CACHE_SIZE_MIN;
        }
        else
        {
            cKBSpare = (cKBLimit * pTmp->iCachePercent)/ 100;
        }
        if (cKBSpare > cKBLimit)
        {
            pTmp->iCachePercent = 75;
            cKBSpare = (cKBLimit * pTmp->iCachePercent) / 100;
        }
        pTmp->uiCacheQuota = (DWORD)cKBSpare;
        SetDlgItemInt(pTmp->hDlg, IDC_ADVANCED_CACHE_TEXT_PERCENT, pTmp->uiCacheQuota, FALSE);
    }

    if (cKBSpare > CACHE_SIZE_CAP)
    {
        if (fUpdate)
        {
            cKBSpare = pTmp->uiCacheQuota = CACHE_SIZE_CAP;
            SetDlgItemInt(pTmp->hDlg, IDC_ADVANCED_CACHE_TEXT_PERCENT, pTmp->uiCacheQuota, FALSE);
        }
        fUpdate = FALSE;
    }
    else if (cKBSpare < CACHE_SIZE_MIN)
    {
        if (fUpdate)
        {
            cKBSpare = pTmp->uiCacheQuota = CACHE_SIZE_MIN;
            SetDlgItemInt(pTmp->hDlg, IDC_ADVANCED_CACHE_TEXT_PERCENT, pTmp->uiCacheQuota, FALSE);
        }
        fUpdate = FALSE;
    } 
    else if (cKBSpare > cKBLimit)
    {
        if (fUpdate)
        {
            cKBSpare = pTmp->uiCacheQuota = (DWORD)cKBLimit;
            SetDlgItemInt(pTmp->hDlg, IDC_ADVANCED_CACHE_TEXT_PERCENT, pTmp->uiCacheQuota, FALSE);
        }
        fUpdate = FALSE;
    }

    if (!fUpdate)
    {
        pTmp->iCachePercent = (WORD)((cKBSpare * 100 + (cKBLimit/2))/cKBLimit);
        if (pTmp->iCachePercent>100)
        {
            pTmp->iCachePercent = 100;
        }
        SendMessage( pTmp->hwndTrack, TBM_SETPOS, TRUE, pTmp->iCachePercent );
    }
    return ERROR_SUCCESS;
}

VOID AdjustCacheRange(LPTEMPDLG pTmp)
{
    UINT uiMax = 10;
    DWORDLONG dlTotal = 0;

    if (GetDiskInfo(pTmp->bChangedLocation ? pTmp->szNewCacheLocation : pTmp->szCacheLocation, NULL, NULL, &dlTotal))
    {
        dlTotal /= (DWORDLONG)CONSTANT_MEGABYTE;
        uiMax = (dlTotal < CACHE_SIZE_CAP) ? (UINT)dlTotal : CACHE_SIZE_CAP;
    }
    SendDlgItemMessage(pTmp->hDlg, IDC_ADVANCED_CACHE_SIZE_SPIN, UDM_SETRANGE, FALSE, MAKELPARAM(uiMax, CACHE_SIZE_MIN));
}

BOOL InvokeCachevu(HWND hDlg)
{
    TCHAR szCache[MAX_PATH];

    HRESULT hres = SHGetFolderPath(NULL, CSIDL_INTERNET_CACHE | CSIDL_FLAG_CREATE, NULL, 0, szCache);
    if (hres == S_OK)
    {
        DWORD dwAttrib = GetFileAttributes(szCache);

        TCHAR szIniFile[MAX_PATH];
        PathCombine(szIniFile, szCache, TEXT("desktop.ini"));

        if (GetFileAttributes(szIniFile) == -1)
        {
            DWORD dwAttrib = GetFileAttributes(szCache);
            dwAttrib &= ~FILE_ATTRIBUTE_HIDDEN;
            dwAttrib |=  FILE_ATTRIBUTE_SYSTEM;

            // make sure system, but not hidden
            SetFileAttributes(szCache, dwAttrib);

            WritePrivateProfileString(TEXT(".ShellClassInfo"), TEXT("ConfirmFileOp"), TEXT("0"), szIniFile);
            WritePrivateProfileString(TEXT(".ShellClassInfo"), TEXT("UICLSID"), TEXT("{7BD29E00-76C1-11CF-9DD0-00A0C9034933}"), szIniFile);
        }

        // All seems well, launch it.
        SHELLEXECUTEINFO ei = { sizeof(SHELLEXECUTEINFO), 0};
        ei.hwnd = hDlg;
        ei.lpFile = szCache;
        ei.nShow = SW_SHOWNORMAL;
        return ShellExecuteEx(&ei);
    }

    return FALSE;
}

// Following flag swiped from wininet
#define FIND_FLAGS_RETRIEVE_ONLY_STRUCT_INFO    0x2

#define DISK_SPACE_MARGIN   4*1024*1024

// IsEnoughDriveSpace
// verifies that there will enough space for the current contents of the cache
// on the new destination

BOOL IsEnoughDriveSpace(DWORD dwClusterSize, DWORDLONG dlAvailable)
{
    // Adjust dlAvailable to leave some space free
    if ((DISK_SPACE_MARGIN/dwClusterSize) > dlAvailable)
    {
        return FALSE;
    }
    else
    {
        dlAvailable -= DISK_SPACE_MARGIN/dwClusterSize;
    };
    
    // Now, iterate through the cache to discover the actual size.
    INTERNET_CACHE_ENTRY_INFOA cei;
    DWORD dwSize = sizeof(cei);
    DWORDLONG dlClustersNeeded = 0;    
    BOOL fResult = FALSE;
    HANDLE hFind = FindFirstUrlCacheEntryExA(NULL, 
                                    FIND_FLAGS_RETRIEVE_ONLY_STRUCT_INFO,
                                    NORMAL_CACHE_ENTRY,
                                    NULL,
                                    &cei, 
                                    &dwSize,
                                    NULL,
                                    NULL,
                                    NULL);

    if (hFind!=NULL)
    {
        do
        {
            ULARGE_INTEGER ulFileSize;
            ulFileSize.LowPart = cei.dwSizeLow;
            ulFileSize.HighPart = cei.dwSizeHigh;
            dlClustersNeeded += (ulFileSize.QuadPart / (DWORDLONG)dwClusterSize) + 1;
            fResult = FindNextUrlCacheEntryExA(hFind, &cei, &dwSize, NULL, NULL, NULL) && (dlClustersNeeded < dlAvailable);
        } 
        while (fResult);
        FindCloseUrlCache(hFind);

        if (GetLastError()==ERROR_NO_MORE_ITEMS)
        {
            fResult = dlClustersNeeded < dlAvailable;
        } 
    }
    else
    {
        fResult = TRUE;
    }
    return fResult;
}


//
// SaveTemporarySettings
//
// Save the Temporary Files Dialog (Cache) settings.
//
// History:
//
// 6/14/96  t-gpease  created
//
BOOL SaveTemporarySettings(LPTEMPDLG pTmp)
{
    if ((pTmp->uiCacheQuota<1) || (pTmp->uiCacheQuota>pTmp->uiDiskSpaceTotal))
    {
        TCHAR szError[1024], szTemp[100];
        
        MLLoadShellLangString(IDS_SIZE_FORMAT, szTemp, ARRAYSIZE(szTemp));
        wnsprintf(szError, ARRAYSIZE(szError), szTemp, pTmp->uiDiskSpaceTotal);
        MLLoadShellLangString(IDS_ERROR, szTemp, ARRAYSIZE(szTemp));
        MessageBox(pTmp->hDlg, szError, szTemp, MB_OK | MB_ICONEXCLAMATION);
        SetFocus(GetDlgItem(pTmp->hDlg, IDC_ADVANCED_CACHE_TEXT_PERCENT));
        return FALSE;
    }
    
    if (pTmp->bChanged)
    {
        // derive the syncmode for the radio buttons
        if (IsDlgButtonChecked(pTmp->hDlg, IDC_ADVANCED_CACHE_AUTOMATIC))
            
            pTmp->iCacheUpdFrequency = WININET_SYNC_MODE_AUTOMATIC;
        
        else if (IsDlgButtonChecked(pTmp->hDlg, IDC_ADVANCED_CACHE_NEVER))

            pTmp->iCacheUpdFrequency = WININET_SYNC_MODE_NEVER;

        else if (IsDlgButtonChecked(pTmp->hDlg, IDC_ADVANCED_CACHE_ALWAYS))

            pTmp->iCacheUpdFrequency = WININET_SYNC_MODE_ALWAYS;

        else {
            ASSERT(IsDlgButtonChecked(pTmp->hDlg, IDC_ADVANCED_CACHE_ONCEPERSESS));
            pTmp->iCacheUpdFrequency = WININET_SYNC_MODE_ONCE_PER_SESSION;
        }

        // notify IE
        INTERNET_CACHE_CONFIG_INFOA cci;
        cci.dwContainer = CONTENT;
        cci.dwQuota = pTmp->uiCacheQuota * 1024; // Make into KB
        cci.dwSyncMode = pTmp->iCacheUpdFrequency;

        ASSERT(cci.dwQuota);
        SetUrlCacheConfigInfoA(&cci, CACHE_CONFIG_SYNC_MODE_FC | CACHE_CONFIG_QUOTA_FC);
    }

    if (pTmp->bChangedLocation)
    {
        OSVERSIONINFOA VerInfo;
        VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        GetVersionExA(&VerInfo);

        if (g_hwndPropSheet)
        {
            PropSheet_Apply(g_hwndPropSheet);
        }
        
        BOOL fRunningOnNT = (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
        // Well, we're going to have to force a reboot now. Ciao. Confirm.
        if (IDYES==DispMessage(pTmp->hDlg, 
                               fRunningOnNT ? IDS_LOGOFF_WARNING : IDS_REBOOTING_WARNING, 
                               fRunningOnNT ? IDS_LOGOFF_TITLE : IDS_REBOOTING_TITLE, 
                               MB_YESNO | MB_ICONEXCLAMATION))
        {
            // fix registry entries and add RunOnce command
            // NOTE: a REBOOT must be done for changes to take effect
            //       (see SetCacheLocation() ).
            // On NT, we must adjust the token privileges
            BOOL fSuccess = TRUE;
            if (fRunningOnNT) 
            {
                HANDLE hToken;
                TOKEN_PRIVILEGES tkp;
                // get a token from this process
                if (fSuccess=OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
                {
                    // get the LUID for the shutdown privilege
                    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

                    tkp.PrivilegeCount = 1;
                    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                    //get the shutdown privilege for this proces
                    fSuccess = AdjustTokenPrivileges( hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0 );
                }
            }
            if (fSuccess)
            {
#ifdef UNICODE  //UpdateUrlCacheContentPath takes LPSTR
                char szNewPath[MAX_PATH];
                SHTCharToAnsi(pTmp->szNewCacheLocation, szNewPath, ARRAYSIZE(szNewPath));
                UpdateUrlCacheContentPath(szNewPath);
#else
                UpdateUrlCacheContentPath(pTmp->szNewCacheLocation);
#endif
                ExitWindowsEx((fRunningOnNT ? EWX_LOGOFF : EWX_REBOOT), 0);
            }
            else
            {
                DispMessage(pTmp->hDlg, IDS_ERROR_MOVE_MSG, IDS_ERROR_MOVE_TITLE, MB_OK | MB_ICONEXCLAMATION);
            }
        }
    }
    return TRUE;
} // SaveTemporarySettings()

//
// IsValidDirectory()
//
// Checks out the path for mistakes... like just machine names...
// SHBrowseForFolder should NOT just return a machine name... BUG is
// shell code.
//
BOOL IsValidDirectory(LPTSTR szDir)
{
    if (szDir)
    {
        if (!*szDir)
            return FALSE;   // it's empty... that's not good
        if (*szDir!= DIR_SEPARATOR_CHAR)
            return TRUE;    // not a machine path... then OK

        // move forward two chars ( the '\''\')
        ++szDir;
        ++szDir;

        while ((*szDir) && (*szDir!=DIR_SEPARATOR_CHAR))
            szDir++;

        if (*szDir==DIR_SEPARATOR_CHAR)
            return TRUE;    // found another '\' so we are happy.

        return FALSE; // machine name only... ERROR!
    }

    return FALSE;

} // IsValidDirecotry()

#define NUM_LEVELS      3   // random dir + cachefile + safety (installed containers)

DWORD g_ccBrandName = 0;

int CALLBACK MoveFolderCallBack(
    HWND hwnd,
    UINT uMsg,
    LPARAM lParam,
    LPARAM lpData
    )
{
    if (uMsg==BFFM_SELCHANGED)
    {
        TCHAR szNewDest[1024];
        TCHAR szStatusText[256];
        UINT uErr = 0;
        LONG fValid = FALSE;
        
        if (SHGetPathFromIDList((LPCITEMIDLIST)lParam, szNewDest))
        {
            // Account for "Temporary Internet Files\Content.IE?\randmdir.ext" + NUM_LEVELS*10
            DWORD ccAvail = MAX_PATH - g_ccBrandName -1 - ARRAYSIZE("CONTENT.IE?\\") - (NUM_LEVELS*10);
            if ((DWORD)lstrlen(szNewDest)>ccAvail) // Win95 limit on how long paths can be
            {
                uErr = IDS_ERROR_ARCHITECTURE;
            }
            else if (StrStrI(szNewDest, TEXT("Content.IE")))
            {
                uErr = IDS_ERROR_WRONG_PLACE;
            }
            else if (!IsValidDirectory(szNewDest))
            {
                uErr = IDS_ERROR_INVALID_PATH_MSG;
            } 
            else if (GetFileAttributes(szNewDest) & FILE_ATTRIBUTE_READONLY)
            {
                uErr = IDS_ERROR_STRANGENESS;
            }
            else
            {
#ifdef UNICODE
                CHAR szAnsiPath[MAX_PATH];
                BOOL fProblems;
                WideCharToMultiByte(CP_ACP, NULL, szNewDest, -1, szAnsiPath, ARRAYSIZE(szAnsiPath),
                                    NULL, &fProblems);
                if (fProblems)
                {
                    uErr = IDS_ERROR_INVALID_PATH;
                }
                else
#endif
                {    
                TCHAR szSystemPath[MAX_PATH+1];

                GetSystemDirectory(szSystemPath, MAX_PATH);
                if (StrStrI(szNewDest, szSystemPath))
                {
                    uErr = IDC_ERROR_USING_SYSTEM_DIR;
                }
                else
                {
                    fValid = TRUE;
                }
                }
            }
        }
        else
        {
            uErr = IDS_ERROR_STRANGENESS;        
        }
        
        if (uErr)
        {
            MLLoadShellLangString(uErr, szStatusText, ARRAYSIZE(szStatusText));
        }
        else
        {
            szStatusText[0] = 0;
        }

        SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM)fValid);
        SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)szStatusText);
    }

    return 0;
}


//
// MoveFolder()
//
// Handles the moving of the Temporary Files (Cache) Folder to
// another location. It checks for the existence of the new folder.
// It warns the user that a REBOOT is necessary before changes
// are made.
//
// History:
//
// 6/18/96  t-gpease    created.
//
void MoveFolder(LPTEMPDLG pTmp)
{
    TCHAR szTemp [1024];
    TCHAR szWindowsPath [MAX_PATH+1];
    BROWSEINFO biToFolder;

    biToFolder.hwndOwner = pTmp->hDlg;
    biToFolder.pidlRoot = NULL;                 // start on the Desktop
    biToFolder.pszDisplayName = szWindowsPath;  // not used, just making it happy...

    TCHAR szBrandName[MAX_PATH];
    MLLoadString(IDS_BRAND_NAME, szBrandName, ARRAYSIZE(szBrandName));
    g_ccBrandName = lstrlen(szBrandName);
    
    // load the title of the dialog box
    MLLoadShellLangString(IDS_SELECT_CACHE, szTemp, ARRAYSIZE(szTemp));
    biToFolder.lpszTitle = szTemp;

    biToFolder.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;  // folders... nothing else
    biToFolder.lpfn = MoveFolderCallBack; // nothing special

    while (1)
    {
        // start shell dialog
        LPITEMIDLIST pidl = SHBrowseForFolder(&biToFolder);
        if (pidl)   // if everything went OK
        {
            DWORD dwClusterSize;
            DWORDLONG dlAvailable;
            DWORD dwError;

            // get the choice the user selected
            SHGetPathFromIDList(pidl, pTmp->szNewCacheLocation);
            SHFree(pidl);

            // Resolve local device to UNC if possible
            if ((GetDriveType(pTmp->szNewCacheLocation)==DRIVE_REMOTE) && (pTmp->szNewCacheLocation[0]!=DIR_SEPARATOR_CHAR))
            {
                TCHAR szPath[MAX_PATH];
                DWORD dwLen = ARRAYSIZE(szPath);

                pTmp->szNewCacheLocation[2] = '\0';

                dwError = WNetGetConnection(pTmp->szNewCacheLocation, szPath, &dwLen);
                if (dwError!=ERROR_SUCCESS)
                {
                    DispMessage(pTmp->hDlg, IDS_ERROR_CANT_CONNECT, IDS_ERROR_MOVE_TITLE, MB_OK | MB_ICONEXCLAMATION);
                    continue;
                }
                memcpy(pTmp->szNewCacheLocation, szPath, dwLen+1);
            }
            
            if (!GetDiskInfo(pTmp->szNewCacheLocation, &dwClusterSize, &dlAvailable, NULL))
            {
                DispMessage(pTmp->hDlg, IDS_ERROR_CANT_CONNECT, IDS_ERROR_MOVE_TITLE, MB_OK | MB_ICONEXCLAMATION);
                continue;
            }

            if (((*pTmp->szNewCacheLocation==*pTmp->szCacheLocation) && (pTmp->szNewCacheLocation[0]!=DIR_SEPARATOR_CHAR))
                ||
                (IsEnoughDriveSpace(dwClusterSize, dlAvailable)
                &&
                (GetLastError()==ERROR_NO_MORE_ITEMS)))
            {
                pTmp->bChangedLocation = TRUE;
            }
            else
            {
                DispMessage(pTmp->hDlg, IDS_ERROR_CANT_MOVE_TIF, IDS_ERROR_MOVE_TITLE, MB_OK | MB_ICONEXCLAMATION);
                continue;
            }
        }
        break;
    }

    if (pTmp->bChangedLocation)
    {
        DWORDLONG cbTotal;

        pTmp->uiDiskSpaceTotal = 0;
        if (GetDiskInfo(pTmp->szNewCacheLocation, NULL, NULL, &cbTotal))
        {
            pTmp->uiDiskSpaceTotal = (UINT)(cbTotal / (DWORDLONG)CONSTANT_MEGABYTE);
        }

        DWORD ccPath = lstrlen(pTmp->szNewCacheLocation);
        if (pTmp->szNewCacheLocation[ccPath-1]!=DIR_SEPARATOR_CHAR)
        {
            pTmp->szNewCacheLocation[ccPath] = DIR_SEPARATOR_CHAR;
            ccPath++;
        }
        memcpy(pTmp->szNewCacheLocation + ccPath, szBrandName, (g_ccBrandName+1)*sizeof(TCHAR));

        if (pTmp->uiCacheQuota > pTmp->uiDiskSpaceTotal)
        {
            pTmp->uiCacheQuota = pTmp->uiDiskSpaceTotal;
            SetDlgItemInt(pTmp->hDlg, IDC_ADVANCED_CACHE_TEXT_PERCENT, pTmp->uiCacheQuota, FALSE);
        }

        SetDlgItemText( pTmp->hDlg, IDC_ADVANCED_CACHE_LOCATION, pTmp->szNewCacheLocation);

        // set dialog text
        MLLoadString(IDS_STATUS_FOLDER_NEW, szTemp, ARRAYSIZE(szTemp));
        SetDlgItemText( pTmp->hDlg, IDC_ADVANCED_CACHE_STATUS, szTemp);

        UpdateCacheQuotaInfo(pTmp, FALSE);
        AdjustCacheRange(pTmp);
    }
} // MoveFolder()


//
// TemporaryInit()
//
// Handles the initialization of Temporary Files Dialog (Cache)
//
// History:
//
// 6/13/96  t-gpease  created
//
BOOL TemporaryInit(HWND hDlg)
{
    LPTEMPDLG pTmp;
    BOOL bAlways, bOnce, bNever, bAuto;

    pTmp = (LPTEMPDLG)LocalAlloc(LPTR, sizeof(*pTmp));
    if (!pTmp)
    {
        EndDialog(hDlg, 0);
        return FALSE;   // no memory?
    }

    // tell dialog where to get info
    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pTmp);

    // get dialog item handles
    pTmp->hDlg = hDlg;
    pTmp->hwndTrack = GetDlgItem( hDlg, IDC_ADVANCED_CACHE_PERCENT );

    INTERNET_CACHE_CONFIG_INFOA icci;
    icci.dwContainer = CONTENT;
    if (GetUrlCacheConfigInfoA(&icci, NULL, CACHE_CONFIG_QUOTA_FC 
                                    | CACHE_CONFIG_DISK_CACHE_PATHS_FC
                                    | CACHE_CONFIG_SYNC_MODE_FC))
    {
        SHAnsiToTChar(icci.CachePath, pTmp->szCacheLocation, ARRAYSIZE(pTmp->szCacheLocation));
        pTmp->iCachePercent = 0;
        pTmp->uiCacheQuota = icci.dwQuota / 1024;
        pTmp->iCacheUpdFrequency = (WORD)icci.dwSyncMode;
    }
    else
    {
        // GUCCIEx CAN NEVER FAIL.
        ASSERT(FALSE);
        pTmp->iCacheUpdFrequency = WININET_SYNC_MODE_DEFAULT;
        pTmp->iCachePercent = 3;  // YUCK, magic number.
        pTmp->uiCacheQuota = 0;
    }
    SetDlgItemInt(pTmp->hDlg, IDC_ADVANCED_CACHE_TEXT_PERCENT, pTmp->uiCacheQuota, FALSE);
//    SendDlgItemMessage(pTmp->hDlg, IDC_ADVANCED_CACHE_TEXT_PERCENT, pTmp->uiCacheQuota, FALSE);
    SendDlgItemMessage(pTmp->hDlg, IDC_ADVANCED_CACHE_TEXT_PERCENT, EM_SETLIMITTEXT, 6, 0);

    // update cache fields
    SendMessage( pTmp->hwndTrack, TBM_SETTICFREQ, 5, 0 );
    SendMessage( pTmp->hwndTrack, TBM_SETRANGE, FALSE, MAKELONG(0, 100) );
    SendMessage( pTmp->hwndTrack, TBM_SETPAGESIZE, 0, 5 );

    DWORDLONG cbTotal;
    pTmp->uiDiskSpaceTotal = 0;
    if (GetDiskInfo(pTmp->szCacheLocation, NULL, NULL, &cbTotal))
    {
        pTmp->uiDiskSpaceTotal = (UINT)(cbTotal / (DWORDLONG)CONSTANT_MEGABYTE);
    }
    UpdateCacheQuotaInfo(pTmp, FALSE);
    AdjustCacheRange(pTmp);

    // set the rest of the dialog's items
    TCHAR szBuf[MAX_PATH];

    // Is the following line necessary? 
    ExpandEnvironmentStrings(pTmp->szCacheLocation,szBuf, ARRAYSIZE(szBuf));

    // NOTE NOTE NOTE The following code might have to be altered if we start using
    // shfolder.dll to gather the location of the cache
    // pszEnd = szBuf + 3 because UNCs are "\\x*" and local drives are "C:\*"

    // Move to the end of the string, before the traiiling slash. (This is how wininet works.)
    PTSTR pszLast = szBuf + lstrlen(szBuf) - 2;
    while ((pszLast>=szBuf) && (*pszLast!=DIR_SEPARATOR_CHAR))
    {
        pszLast--;
    }
    // The terminator should always be placed between the \Temporary Internet Files and the
    // \Content.IE?. This must always be present.
    *(pszLast+1) = TEXT('\0');
    
    SetDlgItemText( hDlg, IDC_ADVANCED_CACHE_LOCATION, szBuf );

    MLLoadString(IDS_STATUS_FOLDER_CURRENT, szBuf, ARRAYSIZE(szBuf));
    SetDlgItemText( hDlg, IDC_ADVANCED_CACHE_STATUS, szBuf );
    
    // activate the correct radio button
    bAlways = bOnce = bNever = bAuto = FALSE;
    if (pTmp->iCacheUpdFrequency == WININET_SYNC_MODE_AUTOMATIC)
        bAuto = TRUE;
    else if (pTmp->iCacheUpdFrequency == WININET_SYNC_MODE_NEVER)
        bNever = TRUE;
    else if (pTmp->iCacheUpdFrequency == WININET_SYNC_MODE_ALWAYS)
        bAlways = TRUE;
    else
        bOnce = TRUE;   // if something got screwed up... reset to Once Per Session

    CheckDlgButton(hDlg, IDC_ADVANCED_CACHE_ALWAYS,      bAlways);
    CheckDlgButton(hDlg, IDC_ADVANCED_CACHE_ONCEPERSESS, bOnce);
    CheckDlgButton(hDlg, IDC_ADVANCED_CACHE_AUTOMATIC,   bAuto);
    CheckDlgButton(hDlg, IDC_ADVANCED_CACHE_NEVER,       bNever);

    // nothing has chagned yet...
    pTmp->bChanged = pTmp->bChangedLocation = FALSE;

    if( g_restrict.fCache )
    {
        EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_CACHE_ALWAYS), FALSE );
        EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_CACHE_ONCEPERSESS), FALSE );
        EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_CACHE_AUTOMATIC), FALSE );
        EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_CACHE_NEVER), FALSE );
        EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_CACHE_PERCENT), FALSE );
        EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_CACHE_PERCENT_ACC), FALSE );
        EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_CACHE_TEXT_PERCENT), FALSE );
        EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_CACHE_SIZE_SPIN), FALSE );
        EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_MOVE_CACHE_LOCATION), FALSE );
        EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_CACHE_EMPTY), FALSE );
    }

    return TRUE;    // worked!
}


//
// TemporaryOnCommand()
//
// Handles Temporary Files dialogs WM_COMMAND messages
//
// History:
//
// 6/13/96  t-gpease   created
//
void TemporaryOnCommand(LPTEMPDLG pTmp, UINT id, UINT nCmd)
{
    switch (id) {
        case IDC_ADVANCED_CACHE_TEXT_PERCENT:
        case IDC_ADVANCED_CACHE_SIZE_SPIN:
            if (pTmp && nCmd == EN_CHANGE)
            {
                UINT uiVal;
                BOOL fSuccess;
                uiVal = GetDlgItemInt(pTmp->hDlg, IDC_ADVANCED_CACHE_TEXT_PERCENT, &fSuccess, FALSE);
                if (fSuccess)
                {
                    pTmp->uiCacheQuota = uiVal;
                    UpdateCacheQuotaInfo(pTmp, FALSE);
                    pTmp->bChanged = TRUE;
                }
            }
            break;

        case IDC_ADVANCED_CACHE_ALWAYS:
        case IDC_ADVANCED_CACHE_ONCEPERSESS:
        case IDC_ADVANCED_CACHE_AUTOMATIC:
        case IDC_ADVANCED_CACHE_NEVER:
            pTmp->bChanged = TRUE;
            break;

        case IDOK:
            // save it
            if (!SaveTemporarySettings(pTmp))
            {
                break;
            }
            // Fall through

        case IDCANCEL:
            EndDialog(pTmp->hDlg, id);
            break; // IDCANCEL

        case IDC_ADVANCED_CACHE_BROWSE:
            InvokeCachevu(pTmp->hDlg);
            break;

        case IDC_ADVANCED_MOVE_CACHE_LOCATION:
            MoveFolder(pTmp);
            break; // IDC_ADVANCED_MOVE_CACHE_LOCATION

        case IDC_ADVANCED_DOWNLOADED_CONTROLS:
        {
            TCHAR szPath[MAX_PATH];
#ifdef UNIX
            TCHAR szExpPath[MAX_PATH];
#endif
            DWORD cb=SIZEOF(szPath);

            if (SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"),
                           TEXT("ActiveXCache"), NULL, szPath, &cb) == ERROR_SUCCESS)
            {
                SHELLEXECUTEINFO ei;
#ifdef UNIX
                int cbExp = ExpandEnvironmentStrings(szPath,szExpPath,MAX_PATH);
#endif

                ei.cbSize       = sizeof(SHELLEXECUTEINFO);
                ei.hwnd         = pTmp->hDlg;
                ei.lpVerb       = NULL;
#ifndef UNIX
                ei.lpFile        = szPath;
#else
                if( cbExp > 0 && cbExp < MAX_PATH )
                    ei.lpFile = szExpPath;
                else
                    ei.lpFile = szPath;
#endif
                ei.lpParameters    = NULL;
                ei.lpDirectory    = NULL;
                ei.nShow        = SW_SHOWNORMAL;
                ei.fMask        = 0;
                ShellExecuteEx(&ei);
            }
            break;
        }

    } // switch

} // TemporaryOnCommand()

//
// TemporaryDlgProc
//
// Take care of "Temporary Files" (Cache)
//
// History:
//
// ??/??/??   God      created
// 6/13/96  t-gpease   cleaned up code, separated functions, and
//                     changed it into a Dialog (was property
//                     sheet).
//
INT_PTR CALLBACK TemporaryDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, 
                                  LPARAM lParam)
{
    LPTEMPDLG pTmp = (LPTEMPDLG) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) {

    case WM_INITDIALOG:
        return TemporaryInit(hDlg);

    case WM_HSCROLL:
        pTmp->iCachePercent = (WORD)SendMessage( pTmp->hwndTrack, TBM_GETPOS, 0, 0 );
        UpdateCacheQuotaInfo(pTmp, TRUE);
        pTmp->bChanged = TRUE;
        return TRUE;

    case WM_COMMAND:
        TemporaryOnCommand(pTmp, LOWORD(wParam), HIWORD(wParam));
        return TRUE;

    case WM_HELP:                   // F1
        ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                    HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
    break;

    case WM_CONTEXTMENU:        // right mouse click
        ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                    HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
    break;

    case WM_DESTROY:
        ASSERT(pTmp);
        LocalFree(pTmp);
        break;

   }
    return FALSE;
}

INT_PTR CALLBACK EmptyCacheDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam)
{
    switch (uMsg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;
        case IDOK:
#ifndef UNIX
            if (Button_GetCheck(GetDlgItem(hDlg, IDC_DELETE_SUB)))
                EndDialog(hDlg, 3);
            else
                EndDialog(hDlg, 1);
#else
            // On Unix we alway return from this dialog with delete channel content
            // option set, though we have removed this option from the UI.
            EndDialog(hDlg, 3);
#endif
            break;
        }
        return TRUE;
   }
    return FALSE;
}

