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
} FOLDERDATA, *PFOLDERDATA;


static FOLDERDATA
UserShellFolders[] =
{
    {L"AppData", L"Application Data", IDS_APPDATA, TRUE, TRUE, TRUE},
    {L"Desktop", L"Desktop", IDS_DESKTOP, FALSE, TRUE, TRUE},
    {L"Favorites", L"Favorites", IDS_FAVORITES, FALSE, TRUE, TRUE},
    {L"Personal", L"My Documents", IDS_MYDOCUMENTS, FALSE, TRUE, TRUE},
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
    {L"Common Documents", L"My Documents", IDS_MYDOCUMENTS, FALSE, TRUE, TRUE},
    {L"Common Templates", L"Templates", IDS_TEMPLATES, TRUE, TRUE, TRUE},
    {L"Common Start Menu", L"Start Menu", IDS_STARTMENU, FALSE, TRUE, TRUE},
    {L"Common Programs", L"Start Menu\\Programs", IDS_PROGRAMS, FALSE, TRUE, TRUE},
    {L"Common Startup", L"Start Menu\\Programs\\Startup", IDS_STARTUP, FALSE, TRUE, TRUE},
    {NULL, NULL, -1, FALSE, FALSE, FALSE}
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
        L"Default User", L"DefaultUserProfile",
        L"USERPROFILE", L"%USERPROFILE%",
        UserShellFolders,
        HKEY_USERS,
        L".Default\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        L".Default\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"
    },
    {
        L"All Users", L"AllUsersProfile",
        L"ALLUSERSPROFILE", L"%ALLUSERSPROFILE%",
        CommonShellFolders,
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"
    },
};


static
BOOL
CreateStandardProfile(IN LPCWSTR pszProfilesPath,
                      IN HKEY hProfileListKey,
                      IN PPROFILEPARAMS pProfileParams)
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
        StringCbCopyW(szBuffer, sizeof(szBuffer), szProfilePath);
        StringCbCatW(szBuffer, sizeof(szBuffer), L"\\");

        /* Append the folder name */
        dwLength = wcslen(szBuffer);
        if (!LoadStringW(hInstance,
                         lpFolderData->uId,
                         &szBuffer[dwLength],
                         ARRAYSIZE(szBuffer) - dwLength))
        {
            /* Use the default name instead */
            StringCbCatW(szBuffer, sizeof(szBuffer), lpFolderData->lpPath);
        }

        // FIXME: Security!
        if (!CreateDirectoryW(szBuffer, NULL))
        {
            if (GetLastError() != ERROR_ALREADY_EXISTS)
            {
                DPRINT1("Error: %lu\n", GetLastError());
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
            StringCbCopyW(szBuffer, sizeof(szBuffer), szProfilePath);
            StringCbCatW(szBuffer, sizeof(szBuffer), L"\\");

            /* Append the folder name */
            dwLength = wcslen(szBuffer);
            if (!LoadStringW(hInstance,
                             lpFolderData->uId,
                             &szBuffer[dwLength],
                             ARRAYSIZE(szBuffer) - dwLength))
            {
                /* Use the default name instead */
                StringCbCatW(szBuffer, sizeof(szBuffer), lpFolderData->lpPath);
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

            /* Append the folder name */
            dwLength = wcslen(szBuffer);
            if (!LoadStringW(hInstance,
                             lpFolderData->uId,
                             &szBuffer[dwLength],
                             ARRAYSIZE(szBuffer) - dwLength))
            {
                /* Use the default name instead */
                StringCbCatW(szBuffer, sizeof(szBuffer), lpFolderData->lpPath);
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
    WCHAR szBuffer[MAX_PATH];

    DPRINT("InitializeProfiles()\n");

    /* Load profiles directory path */
    if (!LoadStringW(hInstance,
                     IDS_PROFILEPATH,
                     szBuffer,
                     ARRAYSIZE(szBuffer)))
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

    /* Expand it */
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
    if (!CreateStandardProfile(szProfilesPath, hKey, &StandardProfiles[0]))
    {
        DPRINT1("CreateStandardProfile(L\"%S\") failed.\n", StandardProfiles[0].pszProfileName);
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Create 'All Users' profile directory path */
    if (!CreateStandardProfile(szProfilesPath, hKey, &StandardProfiles[1]))
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

            /* Append the folder name */
            dwLength = wcslen(szBuffer);
            if (!LoadStringW(hInstance,
                             lpFolderData->uId,
                             &szBuffer[dwLength],
                             ARRAYSIZE(szBuffer) - dwLength))
            {
                /* Use the default name instead */
                StringCbCatW(szBuffer, sizeof(szBuffer), lpFolderData->lpPath);
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
