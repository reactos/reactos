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


static
HANDLE
CreateProfileMutex(
    _In_ PWSTR pszSidString)
{
    SECURITY_DESCRIPTOR SecurityDescriptor;
    SECURITY_ATTRIBUTES SecurityAttributes;
    PWSTR pszMutexName = NULL;
    HANDLE hMutex = NULL;

    pszMutexName = HeapAlloc(GetProcessHeap(),
                             0,
                             (wcslen(L"Global\\userenv:  User Profile Mutex for ") + wcslen(pszSidString) + 1) * sizeof(WCHAR));
    if (pszMutexName == NULL)
    {
        DPRINT("Failed to allocate the mutex name buffer!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    /* Build the profile mutex name */
    wcscpy(pszMutexName, L"Global\\userenv:  User Profile Mutex for ");
    wcscat(pszMutexName, pszSidString);

    /* Initialize the security descriptor */
    InitializeSecurityDescriptor(&SecurityDescriptor,
                                 SECURITY_DESCRIPTOR_REVISION);

    /* Set a NULL-DACL (everyone has access) */
    SetSecurityDescriptorDacl(&SecurityDescriptor,
                              TRUE,
                              NULL,
                              FALSE);

    /* Initialize the security attributes */
    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = &SecurityDescriptor;
    SecurityAttributes.bInheritHandle = FALSE;

    /* Create the profile mutex */
    hMutex = CreateMutexW(&SecurityAttributes,
                          FALSE,
                          pszMutexName);
    if (hMutex == NULL)
    {
        DPRINT1("Failed to create the profile mutex (Error %lu)\n", GetLastError());
    }

    HeapFree(GetProcessHeap(), 0, pszMutexName);

    return hMutex;
}


static
DWORD
IncrementRefCount(
    PWSTR pszSidString,
    PDWORD pdwRefCount)
{
    HKEY hProfilesKey = NULL, hProfileKey = NULL;
    DWORD dwRefCount = 0, dwLength, dwType;
    DWORD dwError;

    DPRINT1("IncrementRefCount(%S %p)\n",
            pszSidString, pdwRefCount);

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                            0,
                            KEY_QUERY_VALUE,
                            &hProfilesKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        goto done;
    }

    dwError = RegOpenKeyExW(hProfilesKey,
                            pszSidString,
                            0,
                            KEY_QUERY_VALUE | KEY_SET_VALUE,
                            &hProfileKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        goto done;
    }

    /* Get the reference counter */
    dwLength = sizeof(dwRefCount);
    RegQueryValueExW(hProfileKey,
                     L"RefCount",
                     NULL,
                     &dwType,
                     (PBYTE)&dwRefCount,
                     &dwLength);

    dwRefCount++;

    dwLength = sizeof(dwRefCount);
    dwError = RegSetValueExW(hProfileKey,
                             L"RefCount",
                             0,
                             REG_DWORD,
                             (PBYTE)&dwRefCount,
                             dwLength);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        goto done;
    }

    if (pdwRefCount != NULL)
        *pdwRefCount = dwRefCount;

done:
    if (hProfileKey != NULL)
        RegCloseKey(hProfileKey);

    if (hProfilesKey != NULL)
        RegCloseKey(hProfilesKey);

    return dwError;
}


static
DWORD
DecrementRefCount(
    PWSTR pszSidString,
    PDWORD pdwRefCount)
{
    HKEY hProfilesKey = NULL, hProfileKey = NULL;
    DWORD dwRefCount = 0, dwLength, dwType;
    DWORD dwError;

    DPRINT1("DecrementRefCount(%S %p)\n",
            pszSidString, pdwRefCount);

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                            0,
                            KEY_QUERY_VALUE,
                            &hProfilesKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        goto done;
    }

    dwError = RegOpenKeyExW(hProfilesKey,
                            pszSidString,
                            0,
                            KEY_QUERY_VALUE | KEY_SET_VALUE,
                            &hProfileKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        goto done;
    }

    /* Get the reference counter */
    dwLength = sizeof(dwRefCount);
    dwError = RegQueryValueExW(hProfileKey,
                               L"RefCount",
                               NULL,
                               &dwType,
                               (PBYTE)&dwRefCount,
                               &dwLength);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        goto done;
    }

    dwRefCount--;

    dwLength = sizeof(dwRefCount);
    dwError = RegSetValueExW(hProfileKey,
                             L"RefCount",
                             0,
                             REG_DWORD,
                             (PBYTE)&dwRefCount,
                             dwLength);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        goto done;
    }

    if (pdwRefCount != NULL)
        *pdwRefCount = dwRefCount;

done:
    if (hProfileKey != NULL)
        RegCloseKey(hProfileKey);

    if (hProfilesKey != NULL)
        RegCloseKey(hProfilesKey);

    return dwError;
}


static
DWORD
CheckForGuestsAndAdmins(
    _In_ HANDLE hToken,
    _Out_ PDWORD pdwState)
{
    PTOKEN_GROUPS pGroupInfo = NULL;
    PSID pAdministratorsSID = NULL;
    PSID pGuestsSID = NULL;
    DWORD i, dwSize = 0;
    DWORD dwError = ERROR_SUCCESS;

    DPRINT("CheckForGuestsAndAdmins(%p %p)\n", hToken, pdwState);

    /* Get the buffer size */
    if (!GetTokenInformation(hToken, TokenGroups, NULL, dwSize, &dwSize))
    {
        dwError = GetLastError();
        if (dwError != ERROR_INSUFFICIENT_BUFFER)
        {
            DPRINT1("GetTokenInformation() failed (Error %lu)\n", dwError);
            return dwError;
        }

        dwError = ERROR_SUCCESS;
    }

    /* Allocate the buffer */
    pGroupInfo = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(), 0, dwSize);
    if (pGroupInfo == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        DPRINT1("HeapAlloc() failed (Error %lu)\n", dwError);
        goto done;
    }

    /* Get the token groups */
    if (!GetTokenInformation(hToken, TokenGroups, pGroupInfo, dwSize, &dwSize))
    {
        dwError = GetLastError();
        DPRINT1("GetTokenInformation() failed (Error %lu)\n", dwError);
        goto done;
    }

    /* Build the Administrators Group SID */
    if(!AllocateAndInitializeSid(&LocalSystemAuthority,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0,
                                 &pAdministratorsSID))
    {
        dwError = GetLastError();
        DPRINT1("AllocateAndInitializeSid() failed (Error %lu)\n", dwError);
        goto done;
    }

    /* Build the Guests Group SID */
    if(!AllocateAndInitializeSid(&LocalSystemAuthority,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_GUESTS,
                                 0, 0, 0, 0, 0, 0,
                                 &pGuestsSID))
    {
        dwError = GetLastError();
        DPRINT1("AllocateAndInitializeSid() failed (Error %lu)\n", dwError);
        goto done;
    }

    /* Check for Administratos or Guests group memberships */
    for (i = 0; i < pGroupInfo->GroupCount; i++)
    {
        if (EqualSid(pAdministratorsSID, pGroupInfo->Groups[i].Sid))
        {
            *pdwState |= 0x0100; // PROFILE_ADMIN_USER
        }

        if (EqualSid(pGuestsSID, pGroupInfo->Groups[i].Sid))
        {
            *pdwState |= 0x0080; // PROFILE_GUESTS_USER
        }
    }

    dwError = ERROR_SUCCESS;

done:
    if (pGuestsSID != NULL)
        FreeSid(pGuestsSID);

    if (pAdministratorsSID != NULL)
        FreeSid(pAdministratorsSID);

    if (pGroupInfo != NULL)
        HeapFree(GetProcessHeap(), 0, pGroupInfo);

    return dwError;
}


static
DWORD
SetProfileData(
    _In_ PWSTR pszSidString,
    _In_ DWORD dwFlags,
    _In_ HANDLE hToken)
{
    HKEY hProfilesKey = NULL, hProfileKey = NULL;
    FILETIME LoadTime;
    DWORD dwLength, dwState = 0;
    DWORD dwError;

    DPRINT("SetProfileData(%S %p)\n", pszSidString, hToken);

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                            0,
                            KEY_QUERY_VALUE,
                            &hProfilesKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        goto done;
    }

    dwError = RegOpenKeyExW(hProfilesKey,
                            pszSidString,
                            0,
                            KEY_QUERY_VALUE | KEY_SET_VALUE,
                            &hProfileKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        goto done;
    }

    /* Set the profile load time */
    GetSystemTimeAsFileTime(&LoadTime);

    dwLength = sizeof(LoadTime.dwLowDateTime);
    dwError = RegSetValueExW(hProfileKey,
                             L"ProfileLoadTimeLow",
                             0,
                             REG_DWORD,
                             (PBYTE)&LoadTime.dwLowDateTime,
                             dwLength);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        goto done;
    }

    dwLength = sizeof(LoadTime.dwHighDateTime);
    dwError = RegSetValueExW(hProfileKey,
                             L"ProfileLoadTimeHigh",
                             0,
                             REG_DWORD,
                             (PBYTE)&LoadTime.dwHighDateTime,
                             dwLength);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        goto done;
    }

    dwLength = sizeof(dwFlags);
    dwError = RegSetValueExW(hProfileKey,
                             L"Flags",
                             0,
                             REG_DWORD,
                             (PBYTE)&dwFlags,
                             dwLength);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        goto done;
    }

    dwError = CheckForGuestsAndAdmins(hToken,
                                      &dwState);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        goto done;
    }

    dwLength = sizeof(dwState);
    dwError = RegSetValueExW(hProfileKey,
                             L"State",
                             0,
                             REG_DWORD,
                             (PBYTE)&dwState,
                             dwLength);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        goto done;
    }

done:
    if (hProfileKey != NULL)
        RegCloseKey(hProfileKey);

    if (hProfilesKey != NULL)
        RegCloseKey(hProfilesKey);

    return dwError;
}


/* PUBLIC FUNCTIONS ********************************************************/

BOOL
WINAPI
CopySystemProfile(
    _In_ ULONG Unused)
{
    WCHAR szKeyName[MAX_PATH];
    WCHAR szRawProfilePath[MAX_PATH];
    WCHAR szProfilePath[MAX_PATH];
    WCHAR szDefaultProfilePath[MAX_PATH];
    UNICODE_STRING SidString = {0, 0, NULL};
    HANDLE hToken = NULL;
    PSID pUserSid = NULL;
    HKEY hProfileKey = NULL;
    DWORD dwDisposition;
    BOOL bResult = FALSE;
    DWORD cchSize;
    DWORD dwError;

    DPRINT1("CopySystemProfile()\n");

    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_QUERY | TOKEN_IMPERSONATE,
                          &hToken))
    {
        DPRINT1("Failed to open the process token (Error %lu)\n", GetLastError());
        return FALSE;
    }

    pUserSid = GetUserSid(hToken);
    if (pUserSid == NULL)
    {
        DPRINT1("Failed to get the users SID (Error %lu)\n", GetLastError());
        goto done;
    }

    /* Get the user SID string */
    if (!GetUserSidStringFromToken(hToken, &SidString))
    {
        DPRINT1("GetUserSidStringFromToken() failed\n");
        goto done;
    }

    StringCbCopyW(szKeyName, sizeof(szKeyName),
                  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
    StringCbCatW(szKeyName, sizeof(szKeyName), SidString.Buffer);

    RtlFreeUnicodeString(&SidString);

    dwError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                              szKeyName,
                              0, NULL, 0,
                              KEY_WRITE,
                              NULL,
                              &hProfileKey,
                              &dwDisposition);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Failed to create the profile key for the %s profile (Error %lu)\n",
                SidString.Buffer, dwError);
        goto done;
    }

    dwError = RegSetValueExW(hProfileKey,
                             L"Sid",
                             0,
                             REG_BINARY,
                             (PBYTE)pUserSid,
                             RtlLengthSid(pUserSid));
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Failed to set the SID value (Error %lu)\n", dwError);
        goto done;
    }

    wcscpy(szRawProfilePath,
           L"%systemroot%\\system32\\config\\systemprofile");

    dwError = RegSetValueExW(hProfileKey,
                             L"ProfileImagePath",
                             0,
                             REG_EXPAND_SZ,
                             (PBYTE)szRawProfilePath,
                             (wcslen(szRawProfilePath) + 1) * sizeof(WCHAR));
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Failed to set the ProfileImagePath value (Error %lu)\n", dwError);
        goto done;
    }

    /* Expand the raw profile path */
    if (!ExpandEnvironmentStringsW(szRawProfilePath,
                                   szProfilePath,
                                   ARRAYSIZE(szProfilePath)))
    {
        DPRINT1("Failled to expand the raw profile path (Error %lu)\n", GetLastError());
        goto done;
    }

    /* Create the profile directory if it does not exist yet */
    // FIXME: Security!
    if (!CreateDirectoryW(szProfilePath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            DPRINT1("Failed to create the profile directory (Error %lu)\n", GetLastError());
            goto done;
        }
    }

    /* Get the path of the default profile */
    cchSize = ARRAYSIZE(szDefaultProfilePath);
    if (!GetDefaultUserProfileDirectoryW(szDefaultProfilePath, &cchSize))
    {
        DPRINT1("Failed to create the default profile path (Error %lu)\n", GetLastError());
        goto done;
    }

    /* Copy the default profile into the new profile directory */
    // FIXME: Security!
    if (!CopyDirectory(szProfilePath, szDefaultProfilePath))
    {
        DPRINT1("Failed to copy the default profile directory (Error %lu)\n", GetLastError());
        goto done;
    }

    bResult = TRUE;

done:
    if (hProfileKey != NULL)
        RegCloseKey(hProfileKey);

    RtlFreeUnicodeString(&SidString);

    if (pUserSid != NULL)
        LocalFree(pUserSid);

    if (hToken != NULL)
        CloseHandle(hToken);

    return bResult;
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
DeleteProfileA(
    _In_ LPCSTR lpSidString,
    _In_opt_ LPCSTR lpProfilePath,
    _In_opt_ LPCSTR lpComputerName)
{
    BOOL bResult;
    UNICODE_STRING SidString = {0, 0, NULL}, ProfilePath = {0, 0, NULL}, ComputerName = {0, 0, NULL};

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
DeleteProfileW(
    _In_ LPCWSTR lpSidString,
    _In_opt_ LPCWSTR lpProfilePath,
    _In_opt_ LPCWSTR lpComputerName)
{
    DPRINT1("DeleteProfileW(%S %S %S) not implemented!\n", lpSidString, lpProfilePath, lpComputerName);
    return TRUE; //FALSE;
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

    if (!lpcchSize || !lpProfileDir)
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
GetProfileType(
    _Out_ PDWORD pdwFlags)
{
    UNICODE_STRING SidString = {0, 0, NULL};
    HANDLE hToken;
    HKEY hProfilesKey = NULL, hProfileKey = NULL;
    DWORD dwType, dwLength, dwState = 0;
    DWORD dwError;
    BOOL bResult = FALSE;

    DPRINT("GetProfileType(%p)\n", pdwFlags);

    if (pdwFlags == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
    {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            DPRINT1("Failed to open a token (Error %lu)\n", GetLastError());
            return FALSE;
        }
    }

    /* Get the user SID string */
    if (!GetUserSidStringFromToken(hToken, &SidString))
    {
        DPRINT1("GetUserSidStringFromToken() failed\n");
        goto done;
    }

    DPRINT("SID: %wZ\n", &SidString);

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                            0,
                            KEY_QUERY_VALUE,
                            &hProfilesKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        SetLastError(dwError);
        goto done;
    }

    dwError = RegOpenKeyExW(hProfilesKey,
                            SidString.Buffer,
                            0,
                            KEY_QUERY_VALUE | KEY_SET_VALUE,
                            &hProfileKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        SetLastError(dwError);
        goto done;
    }

    /* Get the State value */
    dwLength = sizeof(dwState);
    dwError = RegQueryValueExW(hProfileKey,
                               L"State",
                               NULL,
                               &dwType,
                               (PBYTE)&dwState,
                               &dwLength);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Error: %lu\n", dwError);
        SetLastError(dwError);
        goto done;
    }

    *pdwFlags = 0;

    if (dwState & 0x80) /* PROFILE_GUEST_USER */
        *pdwFlags |= PT_TEMPORARY;

    /* FIXME: Add checks for PT_MANDATORY and PT_ROAMING */

    bResult = TRUE;

done:
    if (hProfileKey != NULL)
        RegCloseKey(hProfileKey);

    if (hProfilesKey != NULL)
        RegCloseKey(hProfilesKey);

    RtlFreeUnicodeString(&SidString);

    CloseHandle(hToken);

    return bResult;
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

    if (!lpcchSize || !lpProfileDir)
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
        DPRINT1("Error: %lu\n", GetLastError());
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
    PTOKEN_USER UserSid = NULL;
    UNICODE_STRING SidString = { 0, 0, NULL };
    HANDLE hProfileMutex = NULL;
    LONG Error;
    BOOL ret = FALSE;
    DWORD dwLength = sizeof(szUserHivePath) / sizeof(szUserHivePath[0]);

    DPRINT("LoadUserProfileW(%p %p)\n", hToken, lpProfileInfo);

    /* Check profile info */
    if (!lpProfileInfo || (lpProfileInfo->dwSize != sizeof(PROFILEINFOW)) ||
        (lpProfileInfo->lpUserName == NULL) || (lpProfileInfo->lpUserName[0] == 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DPRINT("UserName: %S\n", lpProfileInfo->lpUserName);

    /* Get the user SID string */
    ret = GetUserSidStringFromToken(hToken, &SidString);
    if (!ret)
    {
        DPRINT1("GetUserSidStringFromToken() failed\n");
        goto cleanup;
    }
    ret = FALSE;

    /* Create the profile mutex */
    hProfileMutex = CreateProfileMutex(SidString.Buffer);
    if (hProfileMutex == NULL)
    {
        DPRINT1("Failed to create the profile mutex\n");
        goto cleanup;
    }

    /* Wait for the profile mutex */
    WaitForSingleObject(hProfileMutex, INFINITE);

    /* Don't load a profile twice */
    if (CheckForLoadedProfile(hToken))
    {
        DPRINT1("Profile %S already loaded\n", SidString.Buffer);
    }
    else
    {
        DPRINT1("Loading profile %S\n", SidString.Buffer);

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
                goto cleanup;
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
                goto cleanup;
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

            /* Create profile */
            ret = CreateUserProfileW(UserSid->User.Sid, lpProfileInfo->lpUserName);
            if (!ret)
            {
                DPRINT1("CreateUserProfileW() failed\n");
                goto cleanup;
            }
        }

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

        SetProfileData(SidString.Buffer,
                       lpProfileInfo->dwFlags,
                       hToken);
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

    Error = IncrementRefCount(SidString.Buffer, NULL);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("IncrementRefCount() failed (Error %ld)\n", Error);
        SetLastError((DWORD)Error);
        goto cleanup;
    }

    ret = TRUE;

cleanup:
    if (UserSid != NULL)
        HeapFree(GetProcessHeap(), 0, UserSid);

    if (hProfileMutex != NULL)
    {
        ReleaseMutex(hProfileMutex);
        CloseHandle(hProfileMutex);
    }

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
    UNICODE_STRING SidString = {0, 0, NULL};
    HANDLE hProfileMutex = NULL;
    HKEY hProfilesKey = NULL, hProfileKey = NULL;
    DWORD dwRefCount = 0, dwLength, dwType, dwState = 0;
    DWORD dwError;
    BOOL bRet = FALSE;

    DPRINT("UnloadUserProfile() called\n");

    if (hProfile == NULL)
    {
        DPRINT1("Invalid profile handle\n");
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

    /* Create the profile mutex */
    hProfileMutex = CreateProfileMutex(SidString.Buffer);
    if (hProfileMutex == NULL)
    {
        DPRINT1("Failed to create the profile mutex\n");
        goto cleanup;
    }

    /* Wait for the profile mutex */
    WaitForSingleObject(hProfileMutex, INFINITE);

    /* Close the profile handle */
    RegFlushKey(hProfile);
    RegCloseKey(hProfile);

    dwError = DecrementRefCount(SidString.Buffer, &dwRefCount);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("DecrementRefCount() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        goto cleanup;
    }

    if (dwRefCount == 0)
    {
        DPRINT("RefCount is 0: Unload the Hive!\n");

        /* Acquire restore privilege */
        if (!AcquireRemoveRestorePrivilege(TRUE))
        {
            DPRINT1("AcquireRemoveRestorePrivilege() failed (Error %lu)\n", GetLastError());
            goto cleanup;
        }

        /* HACK */
        {
            HKEY hUserKey;

            dwError = RegOpenKeyExW(HKEY_USERS,
                                    SidString.Buffer,
                                    0,
                                    KEY_WRITE,
                                    &hUserKey);
            if (dwError == ERROR_SUCCESS)
            {
                RegDeleteKeyW(hUserKey,
                              L"Volatile Environment");

                RegCloseKey(hUserKey);
            }
        }
        /* End of HACK */

        /* Unload the hive */
        dwError = RegUnLoadKeyW(HKEY_USERS,
                                SidString.Buffer);

        /* Remove restore privilege */
        AcquireRemoveRestorePrivilege(FALSE);

        if (dwError != ERROR_SUCCESS)
        {
            DPRINT1("RegUnLoadKeyW() failed (Error %lu)\n", dwError);
            SetLastError(dwError);
            goto cleanup;
        }

        dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                                0,
                                KEY_QUERY_VALUE,
                                &hProfilesKey);
        if (dwError != ERROR_SUCCESS)
        {
            DPRINT1("RegOpenKeyExW() failed (Error %lu)\n", dwError);
            SetLastError(dwError);
            goto cleanup;
        }

        dwError = RegOpenKeyExW(hProfilesKey,
                                SidString.Buffer,
                                0,
                                KEY_QUERY_VALUE,
                                &hProfileKey);
        if (dwError != ERROR_SUCCESS)
        {
            DPRINT1("RegOpenKeyExW() failed (Error %lu)\n", dwError);
            SetLastError(dwError);
            goto cleanup;
        }

        /* Get the State value */
        dwLength = sizeof(dwState);
        dwError = RegQueryValueExW(hProfileKey,
                                   L"State",
                                   NULL,
                                   &dwType,
                                   (PBYTE)&dwState,
                                   &dwLength);
        if (dwError != ERROR_SUCCESS)
        {
            DPRINT1("RegQueryValueExW() failed (Error %lu)\n", dwError);
            SetLastError(dwError);
            goto cleanup;
        }

        /* Delete a guest profile */
        if (dwState & 0x80) // PROFILE_GUEST_USER
        {
            if (!DeleteProfileW(SidString.Buffer, NULL, NULL))
            {
                DPRINT1("DeleteProfile(%S, NULL, NULL) failed (Error %lu)\n",
                        SidString.Buffer, GetLastError());
                goto cleanup;
            }
        }
    }

    bRet = TRUE;

cleanup:
    if (hProfileKey != NULL)
        RegCloseKey(hProfileKey);

    if (hProfilesKey != NULL)
        RegCloseKey(hProfilesKey);

    if (hProfileMutex != NULL)
    {
        ReleaseMutex(hProfileMutex);
        CloseHandle(hProfileMutex);
    }

    RtlFreeUnicodeString(&SidString);

    DPRINT("UnloadUserProfile() done\n");

    return bRet;
}

/* EOF */
