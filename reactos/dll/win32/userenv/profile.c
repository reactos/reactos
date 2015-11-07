/*
 *  ReactOS kernel
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/userenv/profile.c
 * PURPOSE:         User profile code
 * PROGRAMMERS:     Eric Kohl
 *                  Hervé Poussineau
 */

#include "precomp.h"

#include <sddl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

BOOL
AppendSystemPostfix(LPWSTR lpName,
                    DWORD dwMaxLength)
{
    WCHAR szSystemRoot[MAX_PATH];
    LPWSTR lpszPostfix;
    LPWSTR lpszPtr;

    /* Build profile name postfix */
    if (!ExpandEnvironmentStringsW(L"%SystemRoot%",
                                   szSystemRoot,
                                   MAX_PATH))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        return FALSE;
    }

    _wcsupr(szSystemRoot);

    /* Get name postfix */
    szSystemRoot[2] = L'.';
    lpszPostfix = &szSystemRoot[2];
    lpszPtr = lpszPostfix;
    while (*lpszPtr != (WCHAR)0)
    {
        if (*lpszPtr == L'\\')
            *lpszPtr = '_';
        lpszPtr++;
    }

    if (wcslen(lpName) + wcslen(lpszPostfix) + 1 >= dwMaxLength)
    {
        DPRINT1("Error: buffer overflow\n");
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }

    wcscat(lpName, lpszPostfix);

    return TRUE;
}


static
BOOL
AcquireRemoveRestorePrivilege(IN BOOL bAcquire)
{
    BOOL bRet = FALSE;
    HANDLE Token;
    TOKEN_PRIVILEGES TokenPriv;

    DPRINT("AcquireRemoveRestorePrivilege(%d)\n", bAcquire);

    if (OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES,
                         &Token))
    {
        TokenPriv.PrivilegeCount = 1;
        TokenPriv.Privileges[0].Attributes = (bAcquire ? SE_PRIVILEGE_ENABLED : 0);

        if (LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &TokenPriv.Privileges[0].Luid))
        {
            bRet = AdjustTokenPrivileges(Token, FALSE, &TokenPriv, 0, NULL, NULL);

            if (!bRet)
            {
                DPRINT1("AdjustTokenPrivileges() failed with error %lu\n", GetLastError());
            }
            else if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
            {
                DPRINT1("AdjustTokenPrivileges() succeeded, but with not all privileges assigned\n");
                bRet = FALSE;
            }
        }
        else
        {
            DPRINT1("LookupPrivilegeValue() failed with error %lu\n", GetLastError());
        }

        CloseHandle(Token);
    }
    else
    {
        DPRINT1("OpenProcessToken() failed with error %lu\n", GetLastError());
    }

    return bRet;
}


BOOL
WINAPI
CreateUserProfileA(PSID Sid,
                   LPCSTR lpUserName)
{
    UNICODE_STRING UserName;
    BOOL bResult;

    if (!RtlCreateUnicodeStringFromAsciiz(&UserName,
                                          (LPSTR)lpUserName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    bResult = CreateUserProfileW(Sid, UserName.Buffer);

    RtlFreeUnicodeString(&UserName);

    return bResult;
}


BOOL
WINAPI
CreateUserProfileW(PSID Sid,
                   LPCWSTR lpUserName)
{
    WCHAR szRawProfilesPath[MAX_PATH];
    WCHAR szProfilesPath[MAX_PATH];
    WCHAR szUserProfilePath[MAX_PATH];
    WCHAR szDefaultUserPath[MAX_PATH];
    WCHAR szUserProfileName[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    LPWSTR SidString;
    DWORD dwLength;
    DWORD dwDisposition;
    UINT i;
    HKEY hKey;
    BOOL bRet = TRUE;
    LONG Error;

    DPRINT("CreateUserProfileW() called\n");

    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                          0,
                          KEY_QUERY_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    /* Get profiles path */
    dwLength = MAX_PATH * sizeof(WCHAR);
    Error = RegQueryValueExW(hKey,
                             L"ProfilesDirectory",
                             NULL,
                             NULL,
                             (LPBYTE)szRawProfilesPath,
                             &dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    /* Expand it */
    if (!ExpandEnvironmentStringsW(szRawProfilesPath,
                                   szProfilesPath,
                                   MAX_PATH))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        RegCloseKey(hKey);
        return FALSE;
    }

    /* create the profiles directory if it does not yet exist */
    if (!CreateDirectoryW(szProfilesPath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            DPRINT1("Error: %lu\n", GetLastError());
            return FALSE;
        }
    }

    /* Get default user path */
    dwLength = MAX_PATH * sizeof(WCHAR);
    Error = RegQueryValueExW(hKey,
                             L"DefaultUserProfile",
                             NULL,
                             NULL,
                             (LPBYTE)szBuffer,
                             &dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    RegCloseKey (hKey);

    wcscpy(szUserProfileName, lpUserName);

    wcscpy(szUserProfilePath, szProfilesPath);
    wcscat(szUserProfilePath, L"\\");
    wcscat(szUserProfilePath, szUserProfileName);

    wcscpy(szDefaultUserPath, szProfilesPath);
    wcscat(szDefaultUserPath, L"\\");
    wcscat(szDefaultUserPath, szBuffer);

    /* Create user profile directory */
    if (!CreateDirectoryW(szUserProfilePath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            DPRINT1("Error: %lu\n", GetLastError());
            return FALSE;
        }

        for (i = 0; i < 1000; i++)
        {
            swprintf(szUserProfileName, L"%s.%03u", lpUserName, i);

            wcscpy(szUserProfilePath, szProfilesPath);
            wcscat(szUserProfilePath, L"\\");
            wcscat(szUserProfilePath, szUserProfileName);

            if (CreateDirectoryW(szUserProfilePath, NULL))
                break;

            if (GetLastError() != ERROR_ALREADY_EXISTS)
            {
                DPRINT1("Error: %lu\n", GetLastError());
                return FALSE;
            }
        }
    }

    /* Copy default user directory */
    if (!CopyDirectory(szUserProfilePath, szDefaultUserPath))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        return FALSE;
    }

    /* Add profile to profile list */
    if (!ConvertSidToStringSidW(Sid,
                                &SidString))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        return FALSE;
    }

    wcscpy(szBuffer,
           L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
    wcscat(szBuffer, SidString);

    /* Create user profile key */
    Error = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                            szBuffer,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            &dwDisposition);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        bRet = FALSE;
        goto Done;
    }

    /* Create non-expanded user profile path */
    wcscpy(szBuffer, szRawProfilesPath);
    wcscat(szBuffer, L"\\");
    wcscat(szBuffer, szUserProfileName);

    /* Set 'ProfileImagePath' value (non-expanded) */
    Error = RegSetValueExW(hKey,
                           L"ProfileImagePath",
                           0,
                           REG_EXPAND_SZ,
                           (LPBYTE)szBuffer,
                           (wcslen (szBuffer) + 1) * sizeof(WCHAR));
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        bRet = FALSE;
        goto Done;
    }

    /* Set 'Sid' value */
    Error = RegSetValueExW(hKey,
                           L"Sid",
                           0,
                           REG_BINARY,
                           Sid,
                           GetLengthSid(Sid));
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        bRet = FALSE;
        goto Done;
    }

    RegCloseKey(hKey);

    /* Create user hive name */
    wcscpy(szBuffer, szUserProfilePath);
    wcscat(szBuffer, L"\\ntuser.dat");

    /* Acquire restore privilege */
    if (!AcquireRemoveRestorePrivilege(TRUE))
    {
        Error = GetLastError();
        DPRINT1("Error: %lu\n", Error);
        bRet = FALSE;
        goto Done;
    }

    /* Create new user hive */
    Error = RegLoadKeyW(HKEY_USERS,
                        SidString,
                        szBuffer);
    AcquireRemoveRestorePrivilege(FALSE);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        bRet = FALSE;
        goto Done;
    }

    /* Initialize user hive */
    if (!CreateUserHive(SidString, szUserProfilePath))
    {
        Error = GetLastError();
        DPRINT1("Error: %lu\n", Error);
        bRet = FALSE;
    }

    /* Unload the hive */
    AcquireRemoveRestorePrivilege(TRUE);
    RegUnLoadKeyW(HKEY_USERS, SidString);
    AcquireRemoveRestorePrivilege(FALSE);

Done:
    LocalFree((HLOCAL)SidString);
    SetLastError((DWORD)Error);

    DPRINT("CreateUserProfileW() done\n");

    return bRet;
}


BOOL
WINAPI
CreateUserProfileExA(IN PSID pSid,
                     IN LPCSTR lpUserName,
                     IN LPCSTR lpUserHive OPTIONAL,
                     OUT LPSTR lpProfileDir OPTIONAL,
                     IN DWORD dwDirSize,
                     IN BOOL bWin9xUpg)
{
    DPRINT1("CreateUserProfileExA() not implemented!\n");
    return FALSE;
}


BOOL
WINAPI
CreateUserProfileExW(IN PSID pSid,
                     IN LPCWSTR lpUserName,
                     IN LPCWSTR lpUserHive OPTIONAL,
                     OUT LPWSTR lpProfileDir OPTIONAL,
                     IN DWORD dwDirSize,
                     IN BOOL bWin9xUpg)
{
    DPRINT1("CreateUserProfileExW() not implemented!\n");
    return FALSE;
}


BOOL
WINAPI
GetAllUsersProfileDirectoryA(LPSTR lpProfileDir,
                             LPDWORD lpcchSize)
{
    LPWSTR lpBuffer;
    BOOL bResult;

    lpBuffer = GlobalAlloc(GMEM_FIXED,
                           *lpcchSize * sizeof(WCHAR));
    if (lpBuffer == NULL)
        return FALSE;

    bResult = GetAllUsersProfileDirectoryW(lpBuffer,
                                           lpcchSize);
    if (bResult)
    {
        WideCharToMultiByte(CP_ACP,
                            0,
                            lpBuffer,
                            -1,
                            lpProfileDir,
                            *lpcchSize,
                            NULL,
                            NULL);
    }

    GlobalFree(lpBuffer);

    return bResult;
}


BOOL
WINAPI
GetAllUsersProfileDirectoryW(LPWSTR lpProfileDir,
                             LPDWORD lpcchSize)
{
    WCHAR szProfilePath[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    DWORD dwLength;
    HKEY hKey;
    LONG Error;

    if (!lpcchSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                          0,
                          KEY_QUERY_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    /* Get profiles path */
    dwLength = sizeof(szBuffer);
    Error = RegQueryValueExW(hKey,
                             L"ProfilesDirectory",
                             NULL,
                             NULL,
                             (LPBYTE)szBuffer,
                             &dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    /* Expand it */
    if (!ExpandEnvironmentStringsW(szBuffer,
                                   szProfilePath,
                                   MAX_PATH))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        RegCloseKey (hKey);
        return FALSE;
    }

    /* Get 'AllUsersProfile' name */
    dwLength = sizeof(szBuffer);
    Error = RegQueryValueExW(hKey,
                             L"AllUsersProfile",
                             NULL,
                             NULL,
                             (LPBYTE)szBuffer,
                             &dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    RegCloseKey (hKey);

    wcscat(szProfilePath, L"\\");
    wcscat(szProfilePath, szBuffer);

    dwLength = wcslen(szProfilePath) + 1;
    if (lpProfileDir != NULL)
    {
        if (*lpcchSize < dwLength)
        {
            *lpcchSize = dwLength;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        wcscpy(lpProfileDir, szProfilePath);
    }

    *lpcchSize = dwLength;

    return TRUE;
}


BOOL
WINAPI
GetDefaultUserProfileDirectoryA(LPSTR lpProfileDir,
                                LPDWORD lpcchSize)
{
    LPWSTR lpBuffer;
    BOOL bResult;

    lpBuffer = GlobalAlloc(GMEM_FIXED,
                           *lpcchSize * sizeof(WCHAR));
    if (lpBuffer == NULL)
        return FALSE;

    bResult = GetDefaultUserProfileDirectoryW(lpBuffer,
                                              lpcchSize);
    if (bResult)
    {
        WideCharToMultiByte(CP_ACP,
                            0,
                            lpBuffer,
                            -1,
                            lpProfileDir,
                            *lpcchSize,
                            NULL,
                            NULL);
    }

    GlobalFree(lpBuffer);

    return bResult;
}


BOOL
WINAPI
GetDefaultUserProfileDirectoryW(LPWSTR lpProfileDir,
                                LPDWORD lpcchSize)
{
    WCHAR szProfilePath[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    DWORD dwLength;
    HKEY hKey;
    LONG Error;

    if (!lpcchSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                          0,
                          KEY_QUERY_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    /* Get profiles path */
    dwLength = sizeof(szBuffer);
    Error = RegQueryValueExW(hKey,
                             L"ProfilesDirectory",
                             NULL,
                             NULL,
                             (LPBYTE)szBuffer,
                             &dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    /* Expand it */
    if (!ExpandEnvironmentStringsW(szBuffer,
                                   szProfilePath,
                                   MAX_PATH))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Get 'DefaultUserProfile' name */
    dwLength = sizeof(szBuffer);
    Error = RegQueryValueExW(hKey,
                             L"DefaultUserProfile",
                             NULL,
                             NULL,
                             (LPBYTE)szBuffer,
                             &dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    RegCloseKey(hKey);

    wcscat(szProfilePath, L"\\");
    wcscat(szProfilePath, szBuffer);

    dwLength = wcslen(szProfilePath) + 1;
    if (lpProfileDir != NULL)
    {
        if (*lpcchSize < dwLength)
        {
            *lpcchSize = dwLength;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        wcscpy(lpProfileDir, szProfilePath);
    }

    *lpcchSize = dwLength;

    return TRUE;
}


BOOL
WINAPI
GetProfilesDirectoryA(LPSTR lpProfileDir,
                      LPDWORD lpcchSize)
{
    LPWSTR lpBuffer;
    BOOL bResult;

    if (!lpProfileDir || !lpcchSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lpBuffer = GlobalAlloc(GMEM_FIXED,
                           *lpcchSize * sizeof(WCHAR));
    if (lpBuffer == NULL)
        return FALSE;

    bResult = GetProfilesDirectoryW(lpBuffer,
                                    lpcchSize);
    if (bResult)
    {
        bResult = WideCharToMultiByte(CP_ACP,
                                      0,
                                      lpBuffer,
                                      -1,
                                      lpProfileDir,
                                      *lpcchSize,
                                      NULL,
                                      NULL);
    }

    GlobalFree(lpBuffer);

    return bResult;
}


BOOL
WINAPI
GetProfilesDirectoryW(LPWSTR lpProfilesDir,
                      LPDWORD lpcchSize)
{
    WCHAR szProfilesPath[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    DWORD dwLength;
    HKEY hKey;
    LONG Error;
    BOOL bRet = FALSE;

    if (!lpcchSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                          0,
                          KEY_QUERY_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    /* Get profiles path */
    dwLength = sizeof(szBuffer);
    Error = RegQueryValueExW(hKey,
                             L"ProfilesDirectory",
                             NULL,
                             NULL,
                             (LPBYTE)szBuffer,
                             &dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    RegCloseKey(hKey);

    /* Expand it */
    if (!ExpandEnvironmentStringsW(szBuffer,
                                   szProfilesPath,
                                   MAX_PATH))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        return FALSE;
    }

    dwLength = wcslen (szProfilesPath) + 1;
    if (lpProfilesDir != NULL)
    {
        if (*lpcchSize < dwLength)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
            wcscpy(lpProfilesDir, szProfilesPath);
            bRet = TRUE;
        }
    }
    else
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    *lpcchSize = dwLength;

    return bRet;
}


BOOL
WINAPI
GetUserProfileDirectoryA(HANDLE hToken,
                         LPSTR lpProfileDir,
                         LPDWORD lpcchSize)
{
    LPWSTR lpBuffer;
    BOOL bResult;

    if (!lpProfileDir || !lpcchSize)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    lpBuffer = GlobalAlloc(GMEM_FIXED,
                           *lpcchSize * sizeof(WCHAR));
    if (lpBuffer == NULL)
        return FALSE;

    bResult = GetUserProfileDirectoryW(hToken,
                                       lpBuffer,
                                       lpcchSize);
    if (bResult)
    {
        WideCharToMultiByte(CP_ACP,
                            0,
                            lpBuffer,
                            -1,
                            lpProfileDir,
                            *lpcchSize,
                            NULL,
                            NULL);
    }

    GlobalFree(lpBuffer);

    return bResult;
}


BOOL
WINAPI
GetUserProfileDirectoryW(HANDLE hToken,
                         LPWSTR lpProfileDir,
                         LPDWORD lpcchSize)
{
    UNICODE_STRING SidString;
    WCHAR szKeyName[MAX_PATH];
    WCHAR szRawImagePath[MAX_PATH];
    WCHAR szImagePath[MAX_PATH];
    DWORD dwLength;
    HKEY hKey;
    LONG Error;

    if (!hToken)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!lpcchSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!GetUserSidStringFromToken(hToken,
                                   &SidString))
    {
        DPRINT1("GetUserSidFromToken() failed\n");
        return FALSE;
    }

    DPRINT("SidString: '%wZ'\n", &SidString);

    wcscpy(szKeyName,
           L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
    wcscat(szKeyName,
           SidString.Buffer);

    RtlFreeUnicodeString(&SidString);

    DPRINT("KeyName: '%S'\n", szKeyName);

    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          szKeyName,
                          0,
                          KEY_QUERY_VALUE,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    dwLength = sizeof(szRawImagePath);
    Error = RegQueryValueExW(hKey,
                             L"ProfileImagePath",
                             NULL,
                             NULL,
                             (LPBYTE)szRawImagePath,
                             &dwLength);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    RegCloseKey(hKey);

    DPRINT("RawImagePath: '%S'\n", szRawImagePath);

    /* Expand it */
    if (!ExpandEnvironmentStringsW(szRawImagePath,
                                   szImagePath,
                                   MAX_PATH))
    {
        DPRINT1 ("Error: %lu\n", GetLastError());
        return FALSE;
    }

    DPRINT("ImagePath: '%S'\n", szImagePath);

    dwLength = wcslen (szImagePath) + 1;
    if (*lpcchSize < dwLength)
    {
        *lpcchSize = dwLength;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    *lpcchSize = dwLength;
    wcscpy(lpProfileDir, szImagePath);

    return TRUE;
}


static
BOOL
CheckForLoadedProfile(HANDLE hToken)
{
    UNICODE_STRING SidString;
    HKEY hKey;

    DPRINT("CheckForLoadedProfile() called\n");

    if (!GetUserSidStringFromToken(hToken,
                                   &SidString))
    {
        DPRINT1("GetUserSidFromToken() failed\n");
        return FALSE;
    }

    if (RegOpenKeyExW(HKEY_USERS,
                      SidString.Buffer,
                      0,
                      MAXIMUM_ALLOWED,
                      &hKey))
    {
        DPRINT("Profile not loaded\n");
        RtlFreeUnicodeString(&SidString);
        return FALSE;
    }

    RegCloseKey(hKey);

    RtlFreeUnicodeString(&SidString);

    DPRINT("Profile already loaded\n");

    return TRUE;
}


BOOL
WINAPI
LoadUserProfileA(IN HANDLE hToken,
                 IN OUT LPPROFILEINFOA lpProfileInfo)
{
    BOOL bResult = FALSE;
    PROFILEINFOW ProfileInfoW = {0};
    int len;

    DPRINT("LoadUserProfileA() called\n");

    /* Check profile info */
    if (!lpProfileInfo || (lpProfileInfo->dwSize != sizeof(PROFILEINFOA)) ||
        (lpProfileInfo->lpUserName == NULL) || (lpProfileInfo->lpUserName[0] == 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Convert the structure to UNICODE... */
    ProfileInfoW.dwSize = sizeof(PROFILEINFOW);
    ProfileInfoW.dwFlags = lpProfileInfo->dwFlags;

    if (lpProfileInfo->lpUserName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpProfileInfo->lpUserName, -1, NULL, 0);
        ProfileInfoW.lpUserName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!ProfileInfoW.lpUserName)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpProfileInfo->lpUserName, -1, ProfileInfoW.lpUserName, len);
    }

    if (lpProfileInfo->lpProfilePath)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpProfileInfo->lpProfilePath, -1, NULL, 0);
        ProfileInfoW.lpProfilePath = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!ProfileInfoW.lpProfilePath)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpProfileInfo->lpProfilePath, -1, ProfileInfoW.lpProfilePath, len);
    }

    if (lpProfileInfo->lpDefaultPath)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpProfileInfo->lpDefaultPath, -1, NULL, 0);
        ProfileInfoW.lpDefaultPath = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!ProfileInfoW.lpDefaultPath)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpProfileInfo->lpDefaultPath, -1, ProfileInfoW.lpDefaultPath, len);
    }

    if (lpProfileInfo->lpServerName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpProfileInfo->lpServerName, -1, NULL, 0);
        ProfileInfoW.lpServerName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!ProfileInfoW.lpServerName)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpProfileInfo->lpServerName, -1, ProfileInfoW.lpServerName, len);
    }

    if ((ProfileInfoW.dwFlags & PI_APPLYPOLICY) != 0 && lpProfileInfo->lpPolicyPath)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpProfileInfo->lpPolicyPath, -1, NULL, 0);
        ProfileInfoW.lpPolicyPath = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!ProfileInfoW.lpPolicyPath)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpProfileInfo->lpPolicyPath, -1, ProfileInfoW.lpPolicyPath, len);
    }

    /* ... and call the UNICODE function */
    bResult = LoadUserProfileW(hToken, &ProfileInfoW);

    /* Save the returned value */
    lpProfileInfo->hProfile = ProfileInfoW.hProfile;

cleanup:
    /* Memory cleanup */
    if (ProfileInfoW.lpUserName)
        HeapFree(GetProcessHeap(), 0, ProfileInfoW.lpUserName);

    if (ProfileInfoW.lpProfilePath)
        HeapFree(GetProcessHeap(), 0, ProfileInfoW.lpProfilePath);

    if (ProfileInfoW.lpDefaultPath)
        HeapFree(GetProcessHeap(), 0, ProfileInfoW.lpDefaultPath);

    if (ProfileInfoW.lpServerName)
        HeapFree(GetProcessHeap(), 0, ProfileInfoW.lpServerName);

    if ((ProfileInfoW.dwFlags & PI_APPLYPOLICY) != 0 && ProfileInfoW.lpPolicyPath)
        HeapFree(GetProcessHeap(), 0, ProfileInfoW.lpPolicyPath);

    return bResult;
}


BOOL
WINAPI
LoadUserProfileW(IN HANDLE hToken,
                 IN OUT LPPROFILEINFOW lpProfileInfo)
{
    WCHAR szUserHivePath[MAX_PATH];
    LPWSTR UserName = NULL, Domain = NULL;
    DWORD UserNameLength = 0, DomainLength = 0;
    PTOKEN_USER UserSid = NULL;
    SID_NAME_USE AccountType;
    UNICODE_STRING SidString = { 0, 0, NULL };
    LONG Error;
    BOOL ret = FALSE;
    DWORD dwLength = sizeof(szUserHivePath) / sizeof(szUserHivePath[0]);

    DPRINT("LoadUserProfileW() called\n");

    /* Check profile info */
    if (!lpProfileInfo || (lpProfileInfo->dwSize != sizeof(PROFILEINFOW)) ||
        (lpProfileInfo->lpUserName == NULL) || (lpProfileInfo->lpUserName[0] == 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Don't load a profile twice */
    if (CheckForLoadedProfile(hToken))
    {
        DPRINT ("Profile already loaded\n");
        lpProfileInfo->hProfile = NULL;
        return TRUE;
    }

    if (lpProfileInfo->lpProfilePath)
    {
        wcscpy(szUserHivePath, lpProfileInfo->lpProfilePath);
    }
    else
    {
        /* FIXME: check if MS Windows allows lpProfileInfo->lpProfilePath to be NULL */
        if (!GetProfilesDirectoryW(szUserHivePath, &dwLength))
        {
            DPRINT1("GetProfilesDirectoryW() failed (error %ld)\n", GetLastError());
            return FALSE;
        }
    }

    /* Create user hive name */
    wcscat(szUserHivePath, L"\\");
    wcscat(szUserHivePath, lpProfileInfo->lpUserName);
    wcscat(szUserHivePath, L"\\ntuser.dat");
    DPRINT("szUserHivePath: %S\n", szUserHivePath);

    /* Create user profile directory if needed */
    if (GetFileAttributesW(szUserHivePath) == INVALID_FILE_ATTRIBUTES)
    {
        /* Get user sid */
        if (GetTokenInformation(hToken, TokenUser, NULL, 0, &dwLength) ||
            GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            DPRINT1 ("GetTokenInformation() failed\n");
            return FALSE;
        }

        UserSid = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), 0, dwLength);
        if (!UserSid)
        {
            DPRINT1("HeapAlloc() failed\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }

        if (!GetTokenInformation(hToken, TokenUser, UserSid, dwLength, &dwLength))
        {
            DPRINT1("GetTokenInformation() failed\n");
            goto cleanup;
        }

        /* Get user name */
        do
        {
            if (UserNameLength > 0)
            {
                HeapFree(GetProcessHeap(), 0, UserName);
                UserName = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, UserNameLength * sizeof(WCHAR));
                if (!UserName)
                {
                    DPRINT1("HeapAlloc() failed\n");
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    goto cleanup;
                }
            }
            if (DomainLength > 0)
            {
                HeapFree(GetProcessHeap(), 0, Domain);
                Domain = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, DomainLength * sizeof(WCHAR));
                if (!Domain)
                {
                    DPRINT1("HeapAlloc() failed\n");
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    goto cleanup;
                }
            }
            ret = LookupAccountSidW(NULL,
                                    UserSid->User.Sid,
                                    UserName,
                                    &UserNameLength,
                                    Domain,
                                    &DomainLength,
                                    &AccountType);
        } while (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

        if (!ret)
        {
            DPRINT1("LookupAccountSidW() failed\n");
            goto cleanup;
        }

        /* Create profile */
        /* FIXME: ignore Domain? */
        DPRINT("UserName %S, Domain %S\n", UserName, Domain);
        ret = CreateUserProfileW(UserSid->User.Sid, UserName);
        if (!ret)
        {
            DPRINT1("CreateUserProfileW() failed\n");
            goto cleanup;
        }
    }

    /* Get user SID string */
    ret = GetUserSidStringFromToken(hToken, &SidString);
    if (!ret)
    {
        DPRINT1("GetUserSidFromToken() failed\n");
        goto cleanup;
    }
    ret = FALSE;

    /* Acquire restore privilege */
    if (!AcquireRemoveRestorePrivilege(TRUE))
    {
        DPRINT1("AcquireRemoveRestorePrivilege() failed (Error %ld)\n", GetLastError());
        goto cleanup;
    }

    /* Load user registry hive */
    Error = RegLoadKeyW(HKEY_USERS,
                        SidString.Buffer,
                        szUserHivePath);
    AcquireRemoveRestorePrivilege(FALSE);

    /* HACK: Do not fail if the profile has already been loaded! */
    if (Error == ERROR_SHARING_VIOLATION)
        Error = ERROR_SUCCESS;

    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("RegLoadKeyW() failed (Error %ld)\n", Error);
        SetLastError((DWORD)Error);
        goto cleanup;
    }

    /* Open future HKEY_CURRENT_USER */
    Error = RegOpenKeyExW(HKEY_USERS,
                          SidString.Buffer,
                          0,
                          MAXIMUM_ALLOWED,
                          (PHKEY)&lpProfileInfo->hProfile);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("RegOpenKeyExW() failed (Error %ld)\n", Error);
        SetLastError((DWORD)Error);
        goto cleanup;
    }

    ret = TRUE;

cleanup:
    HeapFree(GetProcessHeap(), 0, UserSid);
    HeapFree(GetProcessHeap(), 0, UserName);
    HeapFree(GetProcessHeap(), 0, Domain);
    RtlFreeUnicodeString(&SidString);

    DPRINT("LoadUserProfileW() done\n");
    return ret;
}


BOOL
WINAPI
UnloadUserProfile(HANDLE hToken,
                  HANDLE hProfile)
{
    UNICODE_STRING SidString;
    LONG Error;

    DPRINT("UnloadUserProfile() called\n");

    if (hProfile == NULL)
    {
        DPRINT1("Invalid profile handle\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RegCloseKey(hProfile);

    if (!GetUserSidStringFromToken(hToken,
                                   &SidString))
    {
        DPRINT1("GetUserSidFromToken() failed\n");
        return FALSE;
    }

    DPRINT("SidString: '%wZ'\n", &SidString);

    /* Acquire restore privilege */
    if (!AcquireRemoveRestorePrivilege(TRUE))
    {
        DPRINT1("AcquireRemoveRestorePrivilege() failed (Error %ld)\n", GetLastError());
        RtlFreeUnicodeString(&SidString);
        return FALSE;
    }

    /* Unload the hive */
    Error = RegUnLoadKeyW(HKEY_USERS,
                          SidString.Buffer);

    /* Remove restore privilege */
    AcquireRemoveRestorePrivilege(FALSE);

    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("RegUnLoadKeyW() failed (Error %ld)\n", Error);
        RtlFreeUnicodeString(&SidString);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    RtlFreeUnicodeString(&SidString);

    DPRINT("UnloadUserProfile() done\n");

    return TRUE;
}


BOOL
WINAPI
DeleteProfileW(LPCWSTR lpSidString,
               LPCWSTR lpProfilePath,
               LPCWSTR lpComputerName)
{
    DPRINT1("DeleteProfileW() not implemented!\n");
    return FALSE;
}


BOOL
WINAPI
DeleteProfileA(LPCSTR lpSidString,
               LPCSTR lpProfilePath,
               LPCSTR lpComputerName)
{
    BOOL bResult;
    UNICODE_STRING SidString, ProfilePath, ComputerName;

    DPRINT("DeleteProfileA() called\n");

    /* Conversion to UNICODE */
    if (lpSidString)
        RtlCreateUnicodeStringFromAsciiz(&SidString,
                                         (LPSTR)lpSidString);

    if (lpProfilePath)
        RtlCreateUnicodeStringFromAsciiz(&ProfilePath,
                                         (LPSTR)lpProfilePath);

    if (lpComputerName)
        RtlCreateUnicodeStringFromAsciiz(&ComputerName,
                                         (LPSTR)lpComputerName);

    /* Call the UNICODE function */
    bResult = DeleteProfileW(SidString.Buffer,
                             ProfilePath.Buffer,
                             ComputerName.Buffer);

    /* Memory cleanup */
    if (lpSidString)
        RtlFreeUnicodeString(&SidString);

    if (lpProfilePath)
        RtlFreeUnicodeString(&ProfilePath);

    if (lpComputerName)
        RtlFreeUnicodeString(&ComputerName);

    return bResult;
}


BOOL
WINAPI
GetProfileType(OUT PDWORD pdwFlags)
{
    DPRINT1("GetProfileType() not implemented!\n");
    return FALSE;
}

/* EOF */
