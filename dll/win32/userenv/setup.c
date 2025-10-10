/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/userenv/setup.c
 * PURPOSE:         Profile setup functions
 * PROGRAMMERS:     Eric Kohl
 *                  Hermes Belusca-Maito
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

#include "resources.h"

typedef struct _FOLDERDATA
{
    LPWSTR lpValueName;
    LPWSTR lpPath;
    UINT uId;
    BOOL bHidden;
    BOOL bShellFolder;
    BOOL bUserShellFolder;
    BOOL bProgramData;
} FOLDERDATA, *PFOLDERDATA;


static FOLDERDATA
UserShellFolders[] =
{
    {L"AppData", L"AppData\\Roaming", IDS_ROAMINGAPPDATA, FALSE, TRUE, TRUE, FALSE},
    {L"Desktop", L"Desktop", IDS_DESKTOP, FALSE, TRUE, TRUE, FALSE},
    {L"{374DE290-123F-4565-9164-39C4925E467B}", L"Downloads", IDS_DESKTOP, FALSE, TRUE, TRUE, FALSE},
    {L"Favorites", L"Favorites", IDS_FAVORITES, FALSE, TRUE, TRUE, FALSE},
    {L"Personal", L"Documents", IDS_MYDOCUMENTS, FALSE, TRUE, TRUE, FALSE},
    {L"My Music", L"Music", IDS_MYDOCUMENTS, FALSE, TRUE, TRUE, FALSE},
    {L"My Pictures", L"Music", IDS_MYDOCUMENTS, FALSE, TRUE, TRUE, FALSE},
    {L"My Video", L"Videos", IDS_MYDOCUMENTS, FALSE, TRUE, TRUE, FALSE},
    {L"NetHood", L"AppData\\Roaming\\Microsoft\\Windows\\Network Shortcuts", IDS_NETHOOD, TRUE, TRUE, TRUE, FALSE},
    {L"PrintHood", L"AppData\\Roaming\\Microsoft\\Windows\\Printer Shortcuts", IDS_PRINTHOOD, TRUE, TRUE, TRUE, FALSE},
    {L"Recent", L"AppData\\Roaming\\Microsoft\\Windows\\Recent", IDS_RECENT, TRUE, TRUE, TRUE, FALSE},
    {L"SendTo", L"AppData\\Roaming\\Microsoft\\Windows\\SendTo", IDS_SENDTO, FALSE, TRUE, TRUE, FALSE},
    {L"CD Burning", L"AppData\\Roaming\\Microsoft\\Windows\\Burn\\Burn", IDS_SENDTO, FALSE, TRUE, TRUE, FALSE},
    {L"Templates", L"Templates", IDS_TEMPLATES, FALSE, TRUE, TRUE, FALSE},
    {L"Start Menu", L"AppData\\Roaming\\Microsoft\\Windows\\Start Menu", IDS_STARTMENU, FALSE, TRUE, TRUE, FALSE},
    {L"Programs", L"AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs", IDS_PROGRAMS, FALSE, TRUE, TRUE, FALSE},
    {L"Administrative Tools", L"AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Administrative Tools", IDS_PROGRAMS, FALSE, TRUE, TRUE, FALSE},
    {L"Startup", L"AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup", IDS_STARTUP, FALSE, TRUE, TRUE, FALSE},
    {L"Local AppData", L"AppData\\Local", IDS_LOCALSETTINGS, FALSE, TRUE, TRUE, FALSE},
    {L"{A520A1A4-1780-4FF6-BD18-167343C5AF16}", L"AppData\\LocalLow", IDS_LOCALLOWAPPDATA, FALSE, FALSE, FALSE, FALSE},
    {L"Temp", L"Appdata\\Local\\Temp", IDS_TEMP, FALSE, FALSE, FALSE, FALSE},
    {L"Cache", L"Appdata\\Local\\Temporary Internet Files", IDS_CACHE, FALSE, TRUE, TRUE, FALSE},
    {L"History", L"AppData\\Local\\Microsoft\\Windows\\History", IDS_HISTORY, FALSE, TRUE, TRUE, FALSE},
    {L"Cookies", L"AppData\\Local\\Microsoft\\Windows\\INetCookies", IDS_COOKIES, FALSE, TRUE, TRUE, FALSE},
    {NULL, NULL, -1, FALSE, FALSE, FALSE, FALSE}
};


static FOLDERDATA
CommonShellFolders[] =
{
    {L"Common Desktop", L"Desktop", IDS_DESKTOP, FALSE, TRUE, TRUE, FALSE},
    {L"Common Favorites", L"Favorites", IDS_FAVORITES, FALSE, TRUE, TRUE, FALSE},
    {L"Common Documents", L"Documents", IDS_MYDOCUMENTS, FALSE, TRUE, TRUE, FALSE},
    {L"Common Templates", L"Microsoft\\Windows\\Templates", IDS_TEMPLATES, TRUE, TRUE, TRUE, TRUE},
    {L"Common Start Menu", L"Microsoft\\Windows\\Start Menu", IDS_STARTMENU, FALSE, TRUE, TRUE, TRUE},
    {L"Common Programs", L"Microsoft\\Windows\\Start Menu\\Programs", IDS_PROGRAMS, FALSE, TRUE, TRUE, TRUE},
    {L"Common Startup", L"Microsoft\\Windows\\Start Menu\\Programs\\Startup", IDS_STARTUP, FALSE, TRUE, TRUE, TRUE},
    {NULL, NULL, -1, FALSE, FALSE, FALSE, FALSE}
};


typedef struct _PROFILEPARAMS
{
    LPCWSTR pszProfileName;
    LPCWSTR pszProfileRegValue;
    LPCWSTR pszEnvVar;
    LPCWSTR pszEnvVarProfilePath;
    PFOLDERDATA pFolderList;
    HKEY hRootKey;
    LPCWSTR pszShellFoldersKey;
    LPCWSTR pszUserShellFoldersKey;
} PROFILEPARAMS, *PPROFILEPARAMS;


static PROFILEPARAMS
StandardProfiles[] =
{
    {
        L"Default", L"Default",
        L"USERPROFILE", L"%USERPROFILE%",
        UserShellFolders,
        HKEY_USERS,
        L".Default\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        L".Default\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"
    },
    {
        L"Public", L"PublicProfile",
        L"PUBLIC", L"%PUBLIC%",
        CommonShellFolders,
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"
    },
};

static BOOL
CreateDirectoryRecursive(
    _In_ PCWSTR Path,
    _In_ LPSECURITY_ATTRIBUTES SecurityAttributes)
{
    WCHAR Parent[MAX_PATH];
    DWORD Error;

    if (!Path || !*Path)
        return FALSE;

    if (CreateDirectoryW(Path, NULL))
        return TRUE;

    Error = GetLastError();

    if (Error == ERROR_ALREADY_EXISTS)
        return TRUE;

    if (Error != ERROR_PATH_NOT_FOUND)
        return FALSE;

    if (FAILED(StringCchCopyW(Parent, ARRAYSIZE(Parent), Path)))
        return FALSE;

    if (FAILED(PathCchRemoveFileSpec(Parent, ARRAYSIZE(Parent))))
        return FALSE;

    if (!Parent[0] || !lstrcmpW(Parent, Path))
        return FALSE;

    if (!CreateDirectoryRecursive(Parent, SecurityAttributes))
        return FALSE;

    if (CreateDirectoryW(Path, SecurityAttributes))
        return TRUE;

    return GetLastError() == ERROR_ALREADY_EXISTS;
}

static
BOOL
CreateStandardProfile(
    _In_ LPCWSTR pszProfilesPath,
    _In_ LPCWSTR pszProgramDataPath,
    _In_ HKEY hProfileListKey,
    _In_ PPROFILEPARAMS pProfileParams)
{
    LONG Error;
    PFOLDERDATA lpFolderData;
    HKEY hKey;
    DWORD dwLength;
    WCHAR szProfilePath[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];

    /*
     * Create the standard profile main directory
     */

    StringCbCopyW(szBuffer, sizeof(szBuffer), pProfileParams->pszProfileName);

    /* Build the profile directory path */
    StringCbCopyW(szProfilePath, sizeof(szProfilePath), pszProfilesPath);
    StringCbCatW(szProfilePath, sizeof(szProfilePath), L"\\");
    StringCbCatW(szProfilePath, sizeof(szProfilePath), szBuffer);

    /* Attempt profile directory creation */
    // FIXME: Security!
    if (!CreateDirectoryW(szProfilePath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            DPRINT1("Error: %lu\n", GetLastError());
            return FALSE;
        }

        /* Directory existed, let's try to append the postfix */
        if (!AppendSystemPostfix(szBuffer, ARRAYSIZE(szBuffer)))
        {
            DPRINT1("AppendSystemPostfix() failed\n", GetLastError());
            return FALSE;
        }

        /* Attempt again creation with appended postfix */
        StringCbCopyW(szProfilePath, sizeof(szProfilePath), pszProfilesPath);
        StringCbCatW(szProfilePath, sizeof(szProfilePath), L"\\");
        StringCbCatW(szProfilePath, sizeof(szProfilePath), szBuffer);

        // FIXME: Security!
        if (!CreateDirectoryW(szProfilePath, NULL))
        {
            if (GetLastError() != ERROR_ALREADY_EXISTS)
            {
                DPRINT1("Error: %lu\n", GetLastError());
                return FALSE;
            }
        }
    }

    /* Set 'DefaultUserProfile' / 'AllUsersProfile' value */
    /* Store the default user / all users profile path in the registry */
    dwLength = (wcslen(szBuffer) + 1) * sizeof(WCHAR);
    Error = RegSetValueExW(hProfileListKey,
                           pProfileParams->pszProfileRegValue,
                           0,
                           REG_SZ,
                           (LPBYTE)szBuffer,
                           dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    /* Set 'Default User' / 'All Users' profile */
    SetEnvironmentVariableW(pProfileParams->pszEnvVar, szProfilePath);


    /*
     * Create the standard profile sub-directories and associated registry keys
     */

    /* Create 'Default User' / 'All Users' subdirectories */
    /* FIXME: Take these paths from the registry */

    lpFolderData = pProfileParams->pFolderList;
    while (lpFolderData->lpValueName != NULL)
    {
        if (lpFolderData->bProgramData)
        {
            StringCbCopyW(szBuffer, sizeof(szBuffer), pszProgramDataPath);
        }
        else
        {
            StringCbCopyW(szBuffer, sizeof(szBuffer), szProfilePath);
        }

        StringCbCatW(szBuffer, sizeof(szBuffer), L"\\");
        StringCbCatW(szBuffer, sizeof(szBuffer), lpFolderData->lpPath);

        // FIXME: Security!
        if (!CreateDirectoryRecursive(szBuffer, NULL))
        {
            if (GetLastError() != ERROR_ALREADY_EXISTS)
            {
                DPRINT1("Failed to create %ws, error: %lu\n", szBuffer, GetLastError());
                __debugbreak();
                return FALSE;
            }
        }

        if (lpFolderData->bHidden)
            SetFileAttributesW(szBuffer, FILE_ATTRIBUTE_HIDDEN);

        lpFolderData++;
    }

    /* Set 'Shell Folders' values */
    Error = RegOpenKeyExW(pProfileParams->hRootKey,
                          pProfileParams->pszShellFoldersKey,
                          0,
                          KEY_SET_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    /*
     * NOTE: This is identical to UpdateUsersShellFolderSettings().
     */
    lpFolderData = pProfileParams->pFolderList;
    while (lpFolderData->lpValueName != NULL)
    {
        if (lpFolderData->bShellFolder)
        {
            if (lpFolderData->bProgramData)
            {
                StringCbCopyW(szBuffer, sizeof(szBuffer), pszProgramDataPath);
            }
            else
            {
                StringCbCopyW(szBuffer, sizeof(szBuffer), szProfilePath);
            }
            
            StringCbCatW(szBuffer, sizeof(szBuffer), L"\\");
            StringCbCatW(szBuffer, sizeof(szBuffer), lpFolderData->lpPath);

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

    /* Set 'User Shell Folders' values */
    Error = RegOpenKeyExW(pProfileParams->hRootKey,
                          pProfileParams->pszUserShellFoldersKey,
                          0,
                          KEY_SET_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    lpFolderData = pProfileParams->pFolderList;
    while (lpFolderData->lpValueName != NULL)
    {
        if (lpFolderData->bUserShellFolder)
        {
            StringCbCopyW(szBuffer, sizeof(szBuffer), pProfileParams->pszEnvVarProfilePath);
            StringCbCatW(szBuffer, sizeof(szBuffer), L"\\");
            StringCbCatW(szBuffer, sizeof(szBuffer), lpFolderData->lpPath);

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

    return TRUE;
}


BOOL
WINAPI
InitializeProfiles(VOID)
{
    LONG Error;
    HKEY hKey;
    DWORD dwLength;
    WCHAR szProfilesPath[MAX_PATH];
    WCHAR szProgramDataPath[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];

    DPRINT("InitializeProfiles()\n");

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

    /* Create ProgramData directory */
    StringCchCopyW(szBuffer, _countof(szBuffer), L"%SystemDrive%\\ProgramData");
    if (!ExpandEnvironmentStringsW(szBuffer,
                                   szProgramDataPath,
                                   ARRAYSIZE(szProgramDataPath)))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        RegCloseKey(hKey);
        return FALSE;
    }
    if (!CreateDirectoryW(szProgramDataPath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            DPRINT1("Error: %lu\n", GetLastError());
            RegCloseKey(hKey);
            return FALSE;
        }
    }
    SetFileAttributesW(szProgramDataPath, FILE_ATTRIBUTE_HIDDEN);

    /* Store the profiles directory path (unexpanded) in the registry */
    dwLength = (wcslen(szBuffer) + 1) * sizeof(WCHAR);
    Error = RegSetValueExW(hKey,
                           L"ProgramData",
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

    /* Expand users path */
    wcscpy(szBuffer, L"%SystemDrive%\\Users");
    if (!ExpandEnvironmentStringsW(szBuffer,
                                   szProfilesPath,
                                   ARRAYSIZE(szProfilesPath)))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Create profiles directory */
    // FIXME: Security!
    if (!CreateDirectoryW(szProfilesPath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            DPRINT1("Error: %lu\n", GetLastError());
            RegCloseKey(hKey);
            return FALSE;
        }
    }

    /* Store the profiles directory path (unexpanded) in the registry */
    dwLength = (wcslen(szBuffer) + 1) * sizeof(WCHAR);
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

    /* Create 'Default User' profile directory path */
    if (!CreateStandardProfile(szProfilesPath, szProgramDataPath, hKey, &StandardProfiles[0]))
    {
        DPRINT1("CreateStandardProfile(L\"%S\") failed.\n", StandardProfiles[0].pszProfileName);
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Create 'All Users' profile directory path */
    if (!CreateStandardProfile(szProfilesPath, szProgramDataPath, hKey, &StandardProfiles[1]))
    {
        DPRINT1("CreateStandardProfile(L\"%S\") failed.\n", StandardProfiles[1].pszProfileName);
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);

    DPRINT("Success\n");

    return TRUE;
}


/*
 * NOTE: See CreateStandardProfile() too.
 * Used by registry.c!CreateUserHive()
 */
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
            StringCbCopyW(szBuffer, sizeof(szBuffer), lpUserProfilePath);
            StringCbCatW(szBuffer, sizeof(szBuffer), L"\\");
            StringCbCatW(szBuffer, sizeof(szBuffer), lpFolderData->lpPath);

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
