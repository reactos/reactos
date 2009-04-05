/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/setup.c
 * PURPOSE:         Profile setup functions
 * PROGRAMMER:      Eric Kohl
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>


typedef struct _FOLDERDATA
{
    LPWSTR lpValueName;
    LPWSTR lpPath;
    UINT uId;
    BOOL bHidden;
    BOOL bShellFolder;
    BOOL bUserShellFolder;
} FOLDERDATA, *PFOLDERDATA;


static FOLDERDATA
UserShellFolders[] =
{
    {L"AppData", L"Application Data", IDS_APPDATA, TRUE, TRUE, TRUE},
    {L"Desktop", L"Desktop", IDS_DESKTOP, FALSE, TRUE, TRUE},
    {L"Favorites", L"Favorites", IDS_FAVORITES, FALSE, TRUE, TRUE},
    {L"Personal", L"My Documents", IDS_MYDOCUMENTS, FALSE, TRUE, TRUE},
    {L"My Pictures", L"My Documents\\My Pictures", IDS_MYPICTURES, FALSE, TRUE, TRUE},
    {L"My Music", L"My Documents\\My Music", IDS_MYMUSIC, FALSE, TRUE, TRUE},
    {L"My Video", L"My Documents\\My Videos", IDS_MYVIDEOS, FALSE, TRUE, TRUE},
    {L"NetHood", L"NetHood", IDS_NETHOOD, TRUE, TRUE, TRUE},
    {L"PrintHood", L"PrintHood", IDS_PRINTHOOD, TRUE, TRUE, TRUE},
    {L"Recent", L"Recent", IDS_RECENT, TRUE, TRUE, TRUE},
    {L"SendTo", L"SendTo", IDS_SENDTO, FALSE, TRUE, TRUE},
    {L"Templates", L"Templates", IDS_TEMPLATES, FALSE, TRUE, TRUE},
    {L"Start Menu", L"Start Menu", IDS_STARTMENU, FALSE, TRUE, TRUE},
    {L"Programs", L"Start Menu\\Programs", IDS_PROGRAMS, FALSE, TRUE, TRUE},
    {L"Startup", L"Start Menu\\Programs\\Startup", IDS_STARTUP, FALSE, TRUE, TRUE},
    {L"Local Settings", L"Local Settings", IDS_LOCALSETTINGS, TRUE, TRUE, TRUE},
    {L"Local AppData", L"Local Settings\\Application Data", IDS_LOCALAPPDATA, TRUE, TRUE, TRUE},
    {L"Temp", L"Local Settings\\Temp", IDS_TEMP, FALSE, FALSE, FALSE},
    {L"Cache", L"Local Settings\\Temporary Internet Files", IDS_CACHE, FALSE, TRUE, TRUE},
    {L"History", L"Local Settings\\History", IDS_HISTORY, FALSE, TRUE, TRUE},
    {L"Cookies", L"Cookies", IDS_COOKIES, FALSE, TRUE, TRUE},
    {NULL, NULL, -1, FALSE, FALSE, FALSE}
};


static FOLDERDATA
CommonShellFolders[] =
{
    {L"Common AppData", L"Application Data", IDS_APPDATA, TRUE, TRUE, TRUE},
    {L"Common Desktop", L"Desktop", IDS_DESKTOP, FALSE, TRUE, TRUE},
    {L"Common Favorites", L"Favorites", IDS_FAVORITES, FALSE, TRUE, TRUE},
    {L"Common Start Menu", L"Start Menu", IDS_STARTMENU, FALSE, TRUE, TRUE},
    {L"Common Programs", L"Start Menu\\Programs", IDS_PROGRAMS, FALSE, TRUE, TRUE},
    {L"Common Administrative Tools", L"Start Menu\\Programs\\Administrative Tools", IDS_ADMINTOOLS, FALSE, TRUE, FALSE},
    {L"Common Startup", L"Start Menu\\Programs\\Startup", IDS_STARTUP, FALSE, TRUE, TRUE},
    {L"Common Templates", L"Templates", IDS_TEMPLATES, TRUE, TRUE, TRUE},
    {L"Common Documents", L"My Documents", IDS_MYDOCUMENTS, FALSE, TRUE, TRUE},
    {L"CommonPictures", L"My Documents\\My Pictures", IDS_MYPICTURES, FALSE, TRUE, TRUE},
    {L"CommonMusic", L"My Documents\\My Music", IDS_MYMUSIC, FALSE, TRUE, TRUE},
    {L"CommonVideo", L"My Documents\\My Videos", IDS_MYVIDEOS, FALSE, TRUE, TRUE},
    {NULL, NULL, -1, FALSE, FALSE, FALSE}
};


void
DebugPrint(char* fmt,...)
{
    char buffer[512];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    OutputDebugStringA(buffer);
}


BOOL
WINAPI
InitializeProfiles(VOID)
{
    WCHAR szProfilesPath[MAX_PATH];
    WCHAR szProfilePath[MAX_PATH];
    WCHAR szCommonFilesDirPath[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    DWORD dwLength;
    PFOLDERDATA lpFolderData;
    HKEY hKey;
    LONG Error;

    DPRINT("InitializeProfiles()\n");

    /* Load profiles directory path */
    if (!LoadStringW(hInstance,
                     IDS_PROFILEPATH,
                     szBuffer,
                     MAX_PATH))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        return FALSE;
    }

    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                          0,
                          KEY_SET_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    /* Store profiles directory path */
    dwLength = (wcslen (szBuffer) + 1) * sizeof(WCHAR);
    Error = RegSetValueExW(hKey,
                           L"ProfilesDirectory",
                           0,
                           REG_EXPAND_SZ,
                           (LPBYTE)szBuffer,
                           dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    /* Expand it */
    if (!ExpandEnvironmentStringsW(szBuffer,
                                   szProfilesPath,
                                   MAX_PATH))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Create profiles directory */
    if (!CreateDirectoryW(szProfilesPath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            DPRINT1("Error: %lu\n", GetLastError());
            RegCloseKey(hKey);
            return FALSE;
        }
    }

    /* Set 'DefaultUserProfile' value */
    wcscpy(szBuffer, L"Default User");
    if (!AppendSystemPostfix(szBuffer, MAX_PATH))
    {
        DPRINT1("AppendSystemPostfix() failed\n", GetLastError());
        RegCloseKey(hKey);
        return FALSE;
    }

    dwLength = (wcslen (szBuffer) + 1) * sizeof(WCHAR);
    Error = RegSetValueExW(hKey,
                           L"DefaultUserProfile",
                           0,
                           REG_SZ,
                           (LPBYTE)szBuffer,
                           dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    RegCloseKey(hKey);

    /* Create 'Default User' profile directory */
    wcscpy(szProfilePath, szProfilesPath);
    wcscat(szProfilePath, L"\\");
    wcscat(szProfilePath, szBuffer);
    if (!CreateDirectoryW (szProfilePath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            DPRINT1("Error: %lu\n", GetLastError());
            return FALSE;
        }
    }

    /* Set current user profile */
    SetEnvironmentVariableW(L"USERPROFILE", szProfilePath);

    /* Create 'Default User' subdirectories */
    /* FIXME: Get these paths from the registry */
    lpFolderData = &UserShellFolders[0];
    while (lpFolderData->lpValueName != NULL)
    {
        wcscpy(szBuffer, szProfilePath);
        wcscat(szBuffer, L"\\");

        /* Append the folder name */
        dwLength = wcslen(szBuffer);
        if (!LoadStringW(hInstance,
                         lpFolderData->uId,
                         &szBuffer[dwLength],
                         MAX_PATH - dwLength))
        {
            /* Use the default name instead */
            wcscat(szBuffer, lpFolderData->lpPath);
        }

        if (!CreateDirectoryW(szBuffer, NULL))
        {
            if (GetLastError() != ERROR_ALREADY_EXISTS)
            {
                DPRINT1("Error: %lu\n", GetLastError());
                return FALSE;
            }
        }

        if (lpFolderData->bHidden == TRUE)
        {
            SetFileAttributesW(szBuffer,
                               FILE_ATTRIBUTE_HIDDEN);
        }

        lpFolderData++;
    }

    /* Set default 'Shell Folders' values */
    Error = RegOpenKeyExW(HKEY_USERS,
                          L".Default\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                          0,
                          KEY_SET_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    lpFolderData = &UserShellFolders[0];
    while (lpFolderData->lpValueName != NULL)
    {
        if (lpFolderData->bShellFolder)
        {
            wcscpy(szBuffer, szProfilePath);
            wcscat(szBuffer, L"\\");

            /* Append the folder name */
            dwLength = wcslen(szBuffer);
            if (!LoadStringW(hInstance,
                             lpFolderData->uId,
                             &szBuffer[dwLength],
                             MAX_PATH - dwLength))
            {
                /* Use the default name instead */
                wcscat(szBuffer, lpFolderData->lpPath);
            }

            dwLength = (wcslen(szBuffer) + 1) * sizeof(WCHAR);
            Error = RegSetValueExW(hKey,
                                   lpFolderData->lpValueName,
                                   0,
                                   REG_SZ,
                                   (LPBYTE)szBuffer,
                                   dwLength);
            if (Error != ERROR_SUCCESS)
            {
                DPRINT1("Error: %lu\n", Error);
                RegCloseKey(hKey);
                SetLastError((DWORD)Error);
                return FALSE;
            }
        }

        lpFolderData++;
    }

    /* Set 'Fonts' folder path */
    GetWindowsDirectoryW(szBuffer, MAX_PATH);
    wcscat(szBuffer, L"\\fonts");

    dwLength = (wcslen(szBuffer) + 1) * sizeof(WCHAR);
    Error = RegSetValueExW(hKey,
                           L"Fonts",
                           0,
                           REG_SZ,
                           (LPBYTE)szBuffer,
                           dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    RegCloseKey(hKey);

    /* Set default 'User Shell Folders' values */
    Error = RegOpenKeyExW(HKEY_USERS,
                          L".Default\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
                          0,
                          KEY_SET_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    lpFolderData = &UserShellFolders[0];
    while (lpFolderData->lpValueName != NULL)
    {
        if (lpFolderData->bUserShellFolder)
        {
            wcscpy(szBuffer, L"%USERPROFILE%\\");

            /* Append the folder name */
            dwLength = wcslen(szBuffer);
            if (!LoadStringW(hInstance,
                             lpFolderData->uId,
                             &szBuffer[dwLength],
                             MAX_PATH - dwLength))
            {
                /* Use the default name instead */
                wcscat(szBuffer, lpFolderData->lpPath);
            }

            dwLength = (wcslen(szBuffer) + 1) * sizeof(WCHAR);
            Error = RegSetValueExW(hKey,
                                   lpFolderData->lpValueName,
                                   0,
                                   REG_EXPAND_SZ,
                                   (LPBYTE)szBuffer,
                                   dwLength);
            if (Error != ERROR_SUCCESS)
            {
                DPRINT1("Error: %lu\n", Error);
                RegCloseKey(hKey);
                SetLastError((DWORD)Error);
                return FALSE;
            }
        }

        lpFolderData++;
    }

    RegCloseKey(hKey);

    /* Set 'AllUsersProfile' value */
    wcscpy(szBuffer, L"All Users");
    if (!AppendSystemPostfix(szBuffer, MAX_PATH))
    {
        DPRINT1("AppendSystemPostfix() failed\n", GetLastError());
        return FALSE;
    }

    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                          0,
                          KEY_SET_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    dwLength = (wcslen(szBuffer) + 1) * sizeof(WCHAR);
    Error = RegSetValueExW(hKey,
                           L"AllUsersProfile",
                           0,
                           REG_SZ,
                           (LPBYTE)szBuffer,
                           dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    RegCloseKey(hKey);

    /* Create 'All Users' profile directory */
    wcscpy(szProfilePath, szProfilesPath);
    wcscat(szProfilePath, L"\\");
    wcscat(szProfilePath, szBuffer);
    if (!CreateDirectoryW(szProfilePath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            DPRINT1("Error: %lu\n", GetLastError());
            return FALSE;
        }
    }

    /* Set 'All Users' profile */
    SetEnvironmentVariableW(L"ALLUSERSPROFILE", szProfilePath);

    /* Create 'All Users' subdirectories */
    /* FIXME: Take these paths from the registry */
    lpFolderData = &CommonShellFolders[0];
    while (lpFolderData->lpValueName != NULL)
    {
        wcscpy(szBuffer, szProfilePath);
        wcscat(szBuffer, L"\\");

        /* Append the folder name */
        dwLength = wcslen(szBuffer);
        if (!LoadStringW(hInstance,
                         lpFolderData->uId,
                         &szBuffer[dwLength],
                         MAX_PATH - dwLength))
        {
            /* Use the default name instead */
            wcscat(szBuffer, lpFolderData->lpPath);
        }

        if (!CreateDirectoryW(szBuffer, NULL))
        {
            if (GetLastError() != ERROR_ALREADY_EXISTS)
            {
                DPRINT1("Error: %lu\n", GetLastError());
                return FALSE;
            }
        }

        if (lpFolderData->bHidden)
        {
            SetFileAttributesW(szBuffer,
                               FILE_ATTRIBUTE_HIDDEN);
        }

        lpFolderData++;
    }

    /* Set common 'Shell Folders' values */
    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                          0,
                          KEY_SET_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    lpFolderData = &CommonShellFolders[0];
    while (lpFolderData->lpValueName != NULL)
    {
        if (lpFolderData->bShellFolder)
        {
            wcscpy(szBuffer, szProfilePath);
            wcscat(szBuffer, L"\\");

            /* Append the folder name */
            dwLength = wcslen(szBuffer);
            if (!LoadStringW(hInstance,
                             lpFolderData->uId,
                             &szBuffer[dwLength],
                             MAX_PATH - dwLength))
            {
                /* Use the default name instead */
                wcscat(szBuffer, lpFolderData->lpPath);
            }

            dwLength = (wcslen(szBuffer) + 1) * sizeof(WCHAR);
            Error = RegSetValueExW(hKey,
                                   lpFolderData->lpValueName,
                                   0,
                                   REG_SZ,
                                   (LPBYTE)szBuffer,
                                   dwLength);
            if (Error != ERROR_SUCCESS)
            {
                DPRINT1("Error: %lu\n", Error);
                RegCloseKey(hKey);
                SetLastError((DWORD)Error);
                return FALSE;
            }
        }

        lpFolderData++;
    }

    RegCloseKey(hKey);

    /* Set common 'User Shell Folders' values */
    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
                          0,
                          KEY_SET_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    lpFolderData = &CommonShellFolders[0];
    while (lpFolderData->lpValueName != NULL)
    {
        if (lpFolderData->bUserShellFolder)
        {
            wcscpy(szBuffer, L"%ALLUSERSPROFILE%\\");

            /* Append the folder name */
            dwLength = wcslen(szBuffer);
            if (!LoadStringW(hInstance,
                             lpFolderData->uId,
                             &szBuffer[dwLength],
                             MAX_PATH - dwLength))
            {
                /* Use the default name instead */
                wcscat(szBuffer, lpFolderData->lpPath);
            }

            dwLength = (wcslen(szBuffer) + 1) * sizeof(WCHAR);
            Error = RegSetValueExW(hKey,
                                   lpFolderData->lpValueName,
                                   0,
                                   REG_EXPAND_SZ,
                                   (LPBYTE)szBuffer,
                                   dwLength);
            if (Error != ERROR_SUCCESS)
            {
                DPRINT1("Error: %lu\n", Error);
                RegCloseKey(hKey);
                SetLastError((DWORD)Error);
                return FALSE;
            }
        }

        lpFolderData++;
    }

    RegCloseKey(hKey);

    /* Load 'Program Files' location */
    if (!LoadStringW(hInstance,
                     IDS_PROGRAMFILES,
                     szBuffer,
                     MAX_PATH))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        return FALSE;
    }

    if (!LoadStringW(hInstance,
                     IDS_COMMONFILES,
                     szCommonFilesDirPath,
                     MAX_PATH))
    {
        DPRINT1("Warning: %lu\n", GetLastError());
    }

    /* Expand it */
    if (!ExpandEnvironmentStringsW(szBuffer,
                                   szProfilesPath,
                                   MAX_PATH))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        return FALSE;
    }

    wcscpy(szBuffer, szProfilesPath);
    wcscat(szBuffer, L"\\");
    wcscat(szBuffer, szCommonFilesDirPath);

    if (!ExpandEnvironmentStringsW(szBuffer,
                                  szCommonFilesDirPath,
                                  MAX_PATH))
    {
        DPRINT1("Warning: %lu\n", GetLastError());
    }

    /* Store it */
    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion",
                          0,
                          KEY_SET_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    dwLength = (wcslen (szProfilesPath) + 1) * sizeof(WCHAR);
    Error = RegSetValueExW(hKey,
                           L"ProgramFilesDir",
                           0,
                           REG_SZ,
                           (LPBYTE)szProfilesPath,
                           dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    dwLength = (wcslen(szCommonFilesDirPath) + 1) * sizeof(WCHAR);
    Error = RegSetValueExW(hKey,
                           L"CommonFilesDir",
                           0,
                           REG_SZ,
                           (LPBYTE)szCommonFilesDirPath,
                           dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Warning: %lu\n", Error);
    }

    RegCloseKey (hKey);

    /* Create directory */
    if (!CreateDirectoryW(szProfilesPath, NULL))
    {
        if (GetLastError () != ERROR_ALREADY_EXISTS)
        {
            DPRINT1("Error: %lu\n", GetLastError());
            return FALSE;
        }
    }

    /* Create directory */
    if (!CreateDirectoryW(szCommonFilesDirPath, NULL))
    {
        if (GetLastError () != ERROR_ALREADY_EXISTS)
        {
            DPRINT1("Warning: %lu\n", GetLastError());
        }
    }

    DPRINT("Success\n");

    return TRUE;
}


BOOL
UpdateUsersShellFolderSettings(LPCWSTR lpUserProfilePath,
                               HKEY hUserKey)
{
    WCHAR szBuffer[MAX_PATH];
    DWORD dwLength;
    PFOLDERDATA lpFolderData;
    HKEY hFoldersKey;
    LONG Error;

    DPRINT("UpdateUsersShellFolderSettings() called\n");

    DPRINT("User profile path: %S\n", lpUserProfilePath);

    Error = RegOpenKeyExW(hUserKey,
                          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                          0,
                          KEY_SET_VALUE,
                          &hFoldersKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    lpFolderData = &UserShellFolders[0];
    while (lpFolderData->lpValueName != NULL)
    {
        if (lpFolderData->bShellFolder)
        {
            wcscpy(szBuffer, lpUserProfilePath);
            wcscat(szBuffer, L"\\");

            /* Append the folder name */
            dwLength = wcslen(szBuffer);
            if (!LoadStringW(hInstance,
                             lpFolderData->uId,
                             &szBuffer[dwLength],
                             MAX_PATH - dwLength))
            {
                /* Use the default name instead */
                wcscat(szBuffer, lpFolderData->lpPath);
            }

            DPRINT("%S: %S\n", lpFolderData->lpValueName, szBuffer);

            dwLength = (wcslen(szBuffer) + 1) * sizeof(WCHAR);
            Error = RegSetValueExW(hFoldersKey,
                                   lpFolderData->lpValueName,
                                   0,
                                   REG_SZ,
                                   (LPBYTE)szBuffer,
                                   dwLength);
            if (Error != ERROR_SUCCESS)
            {
                DPRINT1("Error: %lu\n", Error);
                RegCloseKey(hFoldersKey);
                SetLastError((DWORD)Error);
                return FALSE;
            }
        }

        lpFolderData++;
    }

    RegCloseKey(hFoldersKey);

    DPRINT("UpdateUsersShellFolderSettings() done\n");

    return TRUE;
}

/* EOF */
