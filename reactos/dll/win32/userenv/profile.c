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
                                   ARRAYSIZE(szSystemRoot)))
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
            *lpszPtr = L'_';
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
CreateUserProfileA(
    _In_ PSID pSid,
    _In_ LPCSTR lpUserName)
{
    LPWSTR pUserNameW = NULL;
    INT nLength;
    BOOL bResult;

    DPRINT("CreateUserProfileA(%p %s)\n", pSid, lpUserName);

    /* Convert lpUserName to Unicode */
    nLength = MultiByteToWideChar(CP_ACP, 0, lpUserName, -1, NULL, 0);
    pUserNameW = HeapAlloc(GetProcessHeap(), 0, nLength * sizeof(WCHAR));
    if (pUserNameW == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    MultiByteToWideChar(CP_ACP, 0, lpUserName, -1, pUserNameW, nLength);

    /* Call the Ex function */
    bResult = CreateUserProfileExW(pSid,
                                   pUserNameW,
                                   NULL,
                                   NULL,
                                   0,
                                   FALSE);

    HeapFree(GetProcessHeap(), 0, pUserNameW);

    return bResult;
}


BOOL
WINAPI
CreateUserProfileW(
    _In_ PSID pSid,
    _In_ LPCWSTR lpUserName)
{
    DPRINT("CreateUserProfileW(%p %S)\n", pSid, lpUserName);

    /* Call the Ex function */
    return CreateUserProfileExW(pSid,
                                lpUserName,
                                NULL,
                                NULL,
                                0,
                                FALSE);
}


BOOL
WINAPI
CreateUserProfileExA(
    _In_ PSID pSid,
    _In_ LPCSTR lpUserName,
    _In_opt_ LPCSTR lpUserHive,
    _Out_opt_ LPSTR lpProfileDir,
    _In_ DWORD dwDirSize,
    _In_ BOOL bWin9xUpg)
{
    LPWSTR pUserNameW = NULL;
    LPWSTR pUserHiveW = NULL;
    LPWSTR pProfileDirW = NULL;
    INT nLength;
    BOOL bResult = FALSE;

    DPRINT("CreateUserProfileExA(%p %s %s %p %lu %d)\n",
           pSid, lpUserName, lpUserHive, lpProfileDir, dwDirSize, bWin9xUpg);

    /* Check the parameters */
    if (lpProfileDir != NULL && dwDirSize == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Convert lpUserName to Unicode */
    nLength = MultiByteToWideChar(CP_ACP, 0, lpUserName, -1, NULL, 0);
    pUserNameW = HeapAlloc(GetProcessHeap(), 0, nLength * sizeof(WCHAR));
    if (pUserNameW == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto done;
    }
    MultiByteToWideChar(CP_ACP, 0, lpUserName, -1, pUserNameW, nLength);

    /* Convert lpUserHive to Unicode */
    if (lpUserHive != NULL)
    {
        nLength = MultiByteToWideChar(CP_ACP, 0, lpUserHive, -1, NULL, 0);
        pUserHiveW = HeapAlloc(GetProcessHeap(), 0, nLength * sizeof(WCHAR));
        if (pUserHiveW == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto done;
        }
        MultiByteToWideChar(CP_ACP, 0, lpUserHive, -1, pUserHiveW, nLength);
    }

    /* Allocate a Unicode buffer for lpProfileDir */
    if (lpProfileDir != NULL)
    {
        pProfileDirW = HeapAlloc(GetProcessHeap(), 0, dwDirSize * sizeof(WCHAR));
        if (pProfileDirW == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto done;
        }
    }

    /* Call the Unicode function */
    bResult = CreateUserProfileExW(pSid,
                                   (LPCWSTR)pUserNameW,
                                   (LPCWSTR)pUserHiveW,
                                   pProfileDirW,
                                   dwDirSize,
                                   bWin9xUpg);

    /* Convert the profile path to ANSI */
    if (bResult && lpProfileDir != NULL)
    {
        WideCharToMultiByte(CP_ACP, 0, pProfileDirW, -1, lpProfileDir, dwDirSize, NULL, NULL);
    }

done:
    /* Free the buffers */
    if (pProfileDirW != NULL)
        HeapFree(GetProcessHeap(), 0, pProfileDirW);

    if (pUserHiveW != NULL)
        HeapFree(GetProcessHeap(), 0, pUserHiveW);

    if (pUserNameW != NULL)
        HeapFree(GetProcessHeap(), 0, pUserNameW);

    return bResult;
}


BOOL
WINAPI
CreateUserProfileExW(
    _In_ PSID pSid,
    _In_ LPCWSTR lpUserName,
    _In_opt_ LPCWSTR lpUserHive,
    _Out_opt_ LPWSTR lpProfileDir,
    _In_ DWORD dwDirSize,
    _In_ BOOL bWin9xUpg)
{
    WCHAR szRawProfilesPath[MAX_PATH];
    WCHAR szProfilesPath[MAX_PATH];
    WCHAR szUserProfilePath[MAX_PATH];
    WCHAR szDefaultUserPath[MAX_PATH];
    WCHAR szUserProfileName[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    LPWSTR SidString;
    DWORD dwType, dwLength;
    DWORD dwDisposition;
    UINT i;
    HKEY hKey;
    BOOL bRet = TRUE;
    LONG Error;

    DPRINT("CreateUserProfileExW(%p %S %S %p %lu %d)\n",
           pSid, lpUserName, lpUserHive, lpProfileDir, dwDirSize, bWin9xUpg);

    /* Parameters validation */
    if (!pSid || !lpUserName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /*
     * TODO:
     *  - Add support for lpUserHive.
     *  - bWin9xUpg is obsolete. Don't waste your time implementing this.
     */

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
    dwLength = sizeof(szRawProfilesPath);
    Error = RegQueryValueExW(hKey,
                             L"ProfilesDirectory",
                             NULL,
                             &dwType,
                             (LPBYTE)szRawProfilesPath,
                             &dwLength);
    if ((Error != ERROR_SUCCESS) || (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    /* Expand it */
    if (!ExpandEnvironmentStringsW(szRawProfilesPath,
                                   szProfilesPath,
                                   ARRAYSIZE(szProfilesPath)))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Create the profiles directory if it does not exist yet */
    // FIXME: Security!
    if (!CreateDirectoryW(szProfilesPath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            DPRINT1("Error: %lu\n", GetLastError());
            return FALSE;
        }
    }

    /* Get default user path */
    dwLength = sizeof(szBuffer);
    Error = RegQueryValueExW(hKey,
                             L"DefaultUserProfile",
                             NULL,
                             &dwType,
                             (LPBYTE)szBuffer,
                             &dwLength);
    if ((Error != ERROR_SUCCESS) || (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    RegCloseKey(hKey);

    StringCbCopyW(szUserProfileName, sizeof(szUserProfileName), lpUserName);

    /* Create user profile directory */

    StringCbCopyW(szUserProfilePath, sizeof(szUserProfilePath), szProfilesPath);
    StringCbCatW(szUserProfilePath, sizeof(szUserProfilePath), L"\\");
    StringCbCatW(szUserProfilePath, sizeof(szUserProfilePath), szUserProfileName);

    // FIXME: Security!
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

            StringCbCopyW(szUserProfilePath, sizeof(szUserProfilePath), szProfilesPath);
            StringCbCatW(szUserProfilePath, sizeof(szUserProfilePath), L"\\");
            StringCbCatW(szUserProfilePath, sizeof(szUserProfilePath), szUserProfileName);

            // FIXME: Security!
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

    StringCbCopyW(szDefaultUserPath, sizeof(szDefaultUserPath), szProfilesPath);
    StringCbCatW(szDefaultUserPath, sizeof(szDefaultUserPath), L"\\");
    StringCbCatW(szDefaultUserPath, sizeof(szDefaultUserPath), szBuffer);

    // FIXME: Security!
    if (!CopyDirectory(szUserProfilePath, szDefaultUserPath))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        return FALSE;
    }

    /* Add profile to profile list */
    if (!ConvertSidToStringSidW(pSid,
                                &SidString))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        return FALSE;
    }

    StringCbCopyW(szBuffer, sizeof(szBuffer),
                  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
    StringCbCatW(szBuffer, sizeof(szBuffer), SidString);

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
        goto done;
    }

    /* Create non-expanded user profile path */
    StringCbCopyW(szBuffer, sizeof(szBuffer), szRawProfilesPath);
    StringCbCatW(szBuffer, sizeof(szBuffer), L"\\");
    StringCbCatW(szBuffer, sizeof(szBuffer), szUserProfileName);

    /* Set 'ProfileImagePath' value (non-expanded) */
    Error = RegSetValueExW(hKey,
                           L"ProfileImagePath",
                           0,
                           REG_EXPAND_SZ,
                           (LPBYTE)szBuffer,
                           (wcslen(szBuffer) + 1) * sizeof(WCHAR));
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        bRet = FALSE;
        goto done;
    }

    /* Set 'Sid' value */
    Error = RegSetValueExW(hKey,
                           L"Sid",
                           0,
                           REG_BINARY,
                           pSid,
                           GetLengthSid(pSid));
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        bRet = FALSE;
        goto done;
    }

    RegCloseKey(hKey);

    /* Create user hive file */

    /* Use the default hive file name */
    StringCbCopyW(szBuffer, sizeof(szBuffer), szUserProfilePath);
    StringCbCatW(szBuffer, sizeof(szBuffer), L"\\ntuser.dat");

    /* Acquire restore privilege */
    if (!AcquireRemoveRestorePrivilege(TRUE))
    {
        Error = GetLastError();
        DPRINT1("Error: %lu\n", Error);
        bRet = FALSE;
        goto done;
    }

    /* Load the user hive */
    Error = RegLoadKeyW(HKEY_USERS,
                        SidString,
                        szBuffer);
    AcquireRemoveRestorePrivilege(FALSE);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", Error);
        bRet = FALSE;
        goto done;
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

    /*
     * If the caller wants to retrieve the user profile path,
     * give it now. 'dwDirSize' is the number of characters.
     */
    if (lpProfileDir && dwDirSize)
        StringCchCopyW(lpProfileDir, dwDirSize, szUserProfilePath);

done:
    LocalFree((HLOCAL)SidString);
    SetLastError((DWORD)Error);

    DPRINT("CreateUserProfileExW() done\n");

    return bRet;
}


BOOL
WINAPI
GetAllUsersProfileDirectoryA(
    _Out_opt_ LPSTR lpProfileDir,
    _Inout_ LPDWORD lpcchSize)
{
    LPWSTR lpBuffer;
    BOOL bResult;

    if (!lpcchSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lpBuffer = GlobalAlloc(GMEM_FIXED,
                           *lpcchSize * sizeof(WCHAR));
    if (lpBuffer == NULL)
        return FALSE;

    bResult = GetAllUsersProfileDirectoryW(lpBuffer,
                                           lpcchSize);
    if (bResult && lpProfileDir)
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
GetAllUsersProfileDirectoryW(
    _Out_opt_ LPWSTR lpProfileDir,
    _Inout_ LPDWORD lpcchSize)
{
    WCHAR szProfilePath[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    DWORD dwType, dwLength;
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
                             &dwType,
                             (LPBYTE)szBuffer,
                             &dwLength);
    if ((Error != ERROR_SUCCESS) || (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    /* Expand it */
    if (!ExpandEnvironmentStringsW(szBuffer,
                                   szProfilePath,
                                   ARRAYSIZE(szProfilePath)))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Get 'AllUsersProfile' name */
    dwLength = sizeof(szBuffer);
    Error = RegQueryValueExW(hKey,
                             L"AllUsersProfile",
                             NULL,
                             &dwType,
                             (LPBYTE)szBuffer,
                             &dwLength);
    if ((Error != ERROR_SUCCESS) || (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    RegCloseKey(hKey);

    StringCbCatW(szProfilePath, sizeof(szProfilePath), L"\\");
    StringCbCatW(szProfilePath, sizeof(szProfilePath), szBuffer);

    dwLength = wcslen(szProfilePath) + 1;
    if (lpProfileDir && (*lpcchSize >= dwLength))
    {
        StringCchCopyW(lpProfileDir, *lpcchSize, szProfilePath);
        *lpcchSize = dwLength;
        return TRUE;
    }
    else // if (!lpProfileDir || (*lpcchSize < dwLength))
    {
        *lpcchSize = dwLength;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
}


BOOL
WINAPI
GetDefaultUserProfileDirectoryA(
    _Out_opt_ LPSTR lpProfileDir,
    _Inout_ LPDWORD lpcchSize)
{
    LPWSTR lpBuffer;
    BOOL bResult;

    if (!lpcchSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lpBuffer = GlobalAlloc(GMEM_FIXED,
                           *lpcchSize * sizeof(WCHAR));
    if (lpBuffer == NULL)
        return FALSE;

    bResult = GetDefaultUserProfileDirectoryW(lpBuffer,
                                              lpcchSize);
    if (bResult && lpProfileDir)
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
GetDefaultUserProfileDirectoryW(
    _Out_opt_ LPWSTR lpProfileDir,
    _Inout_ LPDWORD lpcchSize)
{
    WCHAR szProfilePath[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    DWORD dwType, dwLength;
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
                             &dwType,
                             (LPBYTE)szBuffer,
                             &dwLength);
    if ((Error != ERROR_SUCCESS) || (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    /* Expand it */
    if (!ExpandEnvironmentStringsW(szBuffer,
                                   szProfilePath,
                                   ARRAYSIZE(szProfilePath)))
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
                             &dwType,
                             (LPBYTE)szBuffer,
                             &dwLength);
    if ((Error != ERROR_SUCCESS) || (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
    {
        DPRINT1("Error: %lu\n", Error);
        RegCloseKey(hKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    RegCloseKey(hKey);

    StringCbCatW(szProfilePath, sizeof(szProfilePath), L"\\");
    StringCbCatW(szProfilePath, sizeof(szProfilePath), szBuffer);

    dwLength = wcslen(szProfilePath) + 1;
    if (lpProfileDir && (*lpcchSize >= dwLength))
    {
        StringCchCopyW(lpProfileDir, *lpcchSize, szProfilePath);
        *lpcchSize = dwLength;
        return TRUE;
    }
    else // if (!lpProfileDir || (*lpcchSize < dwLength))
    {
        *lpcchSize = dwLength;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
}


BOOL
WINAPI
GetProfilesDirectoryA(
    _Out_ LPSTR lpProfileDir, // _Out_opt_
    _Inout_ LPDWORD lpcchSize)
{
    LPWSTR lpBuffer;
    BOOL bResult;

    if (!lpcchSize)
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
    if (bResult && lpProfileDir)
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
GetProfilesDirectoryW(
    _Out_ LPWSTR lpProfilesDir, // _Out_opt_
    _Inout_ LPDWORD lpcchSize)
{
    WCHAR szProfilesPath[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    DWORD dwType, dwLength;
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
                             &dwType,
                             (LPBYTE)szBuffer,
                             &dwLength);
    if ((Error != ERROR_SUCCESS) || (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
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
                                   ARRAYSIZE(szProfilesPath)))
    {
        DPRINT1("Error: %lu\n", GetLastError());
        return FALSE;
    }

    dwLength = wcslen(szProfilesPath) + 1;
    if (lpProfilesDir && (*lpcchSize >= dwLength))
    {
        StringCchCopyW(lpProfilesDir, *lpcchSize, szProfilesPath);
        *lpcchSize = dwLength;
        return TRUE;
    }
    else // if (!lpProfilesDir || (*lpcchSize < dwLength))
    {
        *lpcchSize = dwLength;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
}


BOOL
WINAPI
GetUserProfileDirectoryA(
    _In_ HANDLE hToken,
    _Out_opt_ LPSTR lpProfileDir,
    _Inout_ LPDWORD lpcchSize)
{
    LPWSTR lpBuffer;
    BOOL bResult;

    if (!lpcchSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lpBuffer = GlobalAlloc(GMEM_FIXED,
                           *lpcchSize * sizeof(WCHAR));
    if (lpBuffer == NULL)
        return FALSE;

    bResult = GetUserProfileDirectoryW(hToken,
                                       lpBuffer,
                                       lpcchSize);
    if (bResult && lpProfileDir)
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
GetUserProfileDirectoryW(
    _In_ HANDLE hToken,
    _Out_opt_ LPWSTR lpProfileDir,
    _Inout_ LPDWORD lpcchSize)
{
    UNICODE_STRING SidString;
    WCHAR szKeyName[MAX_PATH];
    WCHAR szRawImagePath[MAX_PATH];
    WCHAR szImagePath[MAX_PATH];
    DWORD dwType, dwLength;
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

    /* Get the user SID string */
    if (!GetUserSidStringFromToken(hToken, &SidString))
    {
        DPRINT1("GetUserSidStringFromToken() failed\n");
        return FALSE;
    }

    DPRINT("SidString: '%wZ'\n", &SidString);

    StringCbCopyW(szKeyName, sizeof(szKeyName),
                  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
    StringCbCatW(szKeyName, sizeof(szKeyName), SidString.Buffer);

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
                             &dwType,
                             (LPBYTE)szRawImagePath,
                             &dwLength);
    if ((Error != ERROR_SUCCESS) || (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
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
                                   ARRAYSIZE(szImagePath)))
    {
        DPRINT1 ("Error: %lu\n", GetLastError());
        return FALSE;
    }

    DPRINT("ImagePath: '%S'\n", szImagePath);

    dwLength = wcslen(szImagePath) + 1;
    if (lpProfileDir && (*lpcchSize >= dwLength))
    {
        StringCchCopyW(lpProfileDir, *lpcchSize, szImagePath);
        *lpcchSize = dwLength;
        return TRUE;
    }
    else // if (!lpProfileDir || (*lpcchSize < dwLength))
    {
        *lpcchSize = dwLength;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
}


static
BOOL
CheckForLoadedProfile(HANDLE hToken)
{
    UNICODE_STRING SidString;
    HKEY hKey;

    DPRINT("CheckForLoadedProfile() called\n");

    /* Get the user SID string */
    if (!GetUserSidStringFromToken(hToken, &SidString))
    {
        DPRINT1("GetUserSidStringFromToken() failed\n");
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
LoadUserProfileA(
    _In_ HANDLE hToken,
    _Inout_ LPPROFILEINFOA lpProfileInfo)
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
    ProfileInfoW.dwSize = sizeof(ProfileInfoW);
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
LoadUserProfileW(
    _In_ HANDLE hToken,
    _Inout_ LPPROFILEINFOW lpProfileInfo)
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
        /* Use the caller's specified roaming user profile path */
        StringCbCopyW(szUserHivePath, sizeof(szUserHivePath), lpProfileInfo->lpProfilePath);
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
    StringCbCatW(szUserHivePath, sizeof(szUserHivePath), L"\\");
    StringCbCatW(szUserHivePath, sizeof(szUserHivePath), lpProfileInfo->lpUserName);
    StringCbCatW(szUserHivePath, sizeof(szUserHivePath), L"\\ntuser.dat");
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

    /* Get the user SID string */
    ret = GetUserSidStringFromToken(hToken, &SidString);
    if (!ret)
    {
        DPRINT1("GetUserSidStringFromToken() failed\n");
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
UnloadUserProfile(
    _In_ HANDLE hToken,
    _In_ HANDLE hProfile)
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

    /* Get the user SID string */
    if (!GetUserSidStringFromToken(hToken, &SidString))
    {
        DPRINT1("GetUserSidStringFromToken() failed\n");
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

    /* HACK */
    {
        HKEY hUserKey;

        Error = RegOpenKeyExW(HKEY_USERS,
                              SidString.Buffer,
                              0,
                              KEY_WRITE,
                              &hUserKey);
        if (Error == ERROR_SUCCESS)
        {
            RegDeleteKeyW(hUserKey,
                          L"Volatile Environment");

            RegCloseKey(hUserKey);
        }
    }
    /* End of HACK */

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
DeleteProfileW(
    _In_ LPCWSTR lpSidString,
    _In_opt_ LPCWSTR lpProfilePath,
    _In_opt_ LPCWSTR lpComputerName)
{
    DPRINT1("DeleteProfileW() not implemented!\n");
    return FALSE;
}


BOOL
WINAPI
DeleteProfileA(
    _In_ LPCSTR lpSidString,
    _In_opt_ LPCSTR lpProfilePath,
    _In_opt_ LPCSTR lpComputerName)
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
GetProfileType(_Out_ PDWORD pdwFlags)
{
    DPRINT1("GetProfileType() not implemented!\n");
    return FALSE;
}

/* EOF */
