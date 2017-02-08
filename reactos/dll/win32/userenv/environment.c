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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/userenv/environment.c
 * PURPOSE:         User environment functions
 * PROGRAMMER:      Eric Kohl
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

static
BOOL
SetUserEnvironmentVariable(PWSTR* Environment,
                           LPWSTR lpName,
                           LPWSTR lpValue,
                           BOOL bExpand)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    UNICODE_STRING SrcValue, DstValue;
    ULONG Length;
    PVOID Buffer = NULL;
    WCHAR ShortName[MAX_PATH];

    if (bExpand)
    {
        RtlInitUnicodeString(&SrcValue, lpValue);

        Length = 2 * MAX_PATH * sizeof(WCHAR);

        DstValue.Length = 0;
        DstValue.MaximumLength = Length;
        DstValue.Buffer = Buffer = LocalAlloc(LPTR, Length);
        if (DstValue.Buffer == NULL)
        {
            DPRINT1("LocalAlloc() failed\n");
            return FALSE;
        }

        Status = RtlExpandEnvironmentStrings_U(*Environment,
                                               &SrcValue,
                                               &DstValue,
                                               &Length);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("RtlExpandEnvironmentStrings_U() failed (Status %lx)\n", Status);
            DPRINT1("Length %lu\n", Length);

            if (Buffer)
                LocalFree(Buffer);

            return FALSE;
        }
    }
    else
    {
        RtlInitUnicodeString(&DstValue, lpValue);
    }

    if (!_wcsicmp(lpName, L"TEMP") || !_wcsicmp(lpName, L"TMP"))
    {
        if (GetShortPathNameW(DstValue.Buffer, ShortName, ARRAYSIZE(ShortName)))
        {
            RtlInitUnicodeString(&DstValue, ShortName);
        }
        else
        {
            DPRINT("GetShortPathNameW() failed for %S (Error %lu)\n", DstValue.Buffer, GetLastError());
        }

        DPRINT("Buffer: %S\n", ShortName);
    }

    RtlInitUnicodeString(&Name, lpName);

    DPRINT("Value: %wZ\n", &DstValue);

    Status = RtlSetEnvironmentVariable(Environment,
                                       &Name,
                                       &DstValue);

    if (Buffer)
        LocalFree(Buffer);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlSetEnvironmentVariable() failed (Status %lx)\n", Status);
        return FALSE;
    }

    return TRUE;
}


static
BOOL
AppendUserEnvironmentVariable(PWSTR* Environment,
                              LPWSTR lpName,
                              LPWSTR lpValue)
{
    NTSTATUS Status;
    UNICODE_STRING Name, Value;

    RtlInitUnicodeString(&Name, lpName);

    Value.Length = 0;
    Value.MaximumLength = 1024 * sizeof(WCHAR);
    Value.Buffer = LocalAlloc(LPTR, Value.MaximumLength);
    if (Value.Buffer == NULL)
        return FALSE;

    Value.Buffer[0] = UNICODE_NULL;

    Status = RtlQueryEnvironmentVariable_U(*Environment,
                                           &Name,
                                           &Value);
    if (NT_SUCCESS(Status))
        RtlAppendUnicodeToString(&Value, L";");

    RtlAppendUnicodeToString(&Value, lpValue);

    Status = RtlSetEnvironmentVariable(Environment,
                                       &Name,
                                       &Value);
    LocalFree(Value.Buffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlSetEnvironmentVariable() failed (Status %lx)\n", Status);
        return FALSE;
    }

    return TRUE;
}


static
HKEY
GetCurrentUserKey(HANDLE hToken)
{
    LONG Error;
    UNICODE_STRING SidString;
    HKEY hKey;

    if (!GetUserSidStringFromToken(hToken, &SidString))
    {
        DPRINT1("GetUserSidFromToken() failed\n");
        return NULL;
    }

    Error = RegOpenKeyExW(HKEY_USERS,
                          SidString.Buffer,
                          0,
                          MAXIMUM_ALLOWED,
                          &hKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("RegOpenKeyExW() failed (Error %ld)\n", Error);
        RtlFreeUnicodeString(&SidString);
        SetLastError((DWORD)Error);
        return NULL;
    }

    RtlFreeUnicodeString(&SidString);

    return hKey;
}


static
BOOL
GetUserAndDomainName(IN HANDLE hToken,
                     OUT LPWSTR *UserName,
                     OUT LPWSTR *DomainName)
{
    BOOL bRet = TRUE;
    PSID Sid = NULL;
    LPWSTR lpUserName = NULL;
    LPWSTR lpDomainName = NULL;
    DWORD cbUserName = 0;
    DWORD cbDomainName = 0;
    SID_NAME_USE SidNameUse;

    Sid = GetUserSid(hToken);
    if (Sid == NULL)
    {
        DPRINT1("GetUserSid() failed\n");
        return FALSE;
    }

    if (!LookupAccountSidW(NULL,
                           Sid,
                           NULL,
                           &cbUserName,
                           NULL,
                           &cbDomainName,
                           &SidNameUse))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            bRet = FALSE;
            goto done;
        }
    }

    lpUserName = LocalAlloc(LPTR, cbUserName * sizeof(WCHAR));
    if (lpUserName == NULL)
    {
        bRet = FALSE;
        goto done;
    }

    lpDomainName = LocalAlloc(LPTR, cbDomainName * sizeof(WCHAR));
    if (lpDomainName == NULL)
    {
        bRet = FALSE;
        goto done;
    }

    if (!LookupAccountSidW(NULL,
                           Sid,
                           lpUserName,
                           &cbUserName,
                           lpDomainName,
                           &cbDomainName,
                           &SidNameUse))
    {
        bRet = FALSE;
        goto done;
    }

    *UserName = lpUserName;
    *DomainName = lpDomainName;

done:
    if (bRet == FALSE)
    {
        if (lpUserName != NULL)
            LocalFree(lpUserName);

        if (lpDomainName != NULL)
            LocalFree(lpDomainName);
    }

    LocalFree(Sid);

    return bRet;
}


static
BOOL
SetUserEnvironment(PWSTR* Environment,
                   HKEY hKey,
                   LPWSTR lpSubKeyName)
{
    LONG Error;
    HKEY hEnvKey;
    DWORD dwValues;
    DWORD dwMaxValueNameLength;
    DWORD dwMaxValueDataLength;
    DWORD dwValueNameLength;
    DWORD dwValueDataLength;
    DWORD dwType;
    DWORD i;
    LPWSTR lpValueName;
    LPWSTR lpValueData;

    Error = RegOpenKeyExW(hKey,
                          lpSubKeyName,
                          0,
                          KEY_QUERY_VALUE,
                          &hEnvKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("RegOpenKeyExW() failed (Error %ld)\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    Error = RegQueryInfoKey(hEnvKey,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            &dwValues,
                            &dwMaxValueNameLength,
                            &dwMaxValueDataLength,
                            NULL,
                            NULL);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("RegQueryInforKey() failed (Error %ld)\n", Error);
        RegCloseKey(hEnvKey);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    if (dwValues == 0)
    {
        RegCloseKey(hEnvKey);
        return TRUE;
    }

    /* Allocate buffers */
    dwMaxValueNameLength++;
    lpValueName = LocalAlloc(LPTR, dwMaxValueNameLength * sizeof(WCHAR));
    if (lpValueName == NULL)
    {
        RegCloseKey(hEnvKey);
        return FALSE;
    }

    lpValueData = LocalAlloc(LPTR, dwMaxValueDataLength);
    if (lpValueData == NULL)
    {
        LocalFree(lpValueName);
        RegCloseKey(hEnvKey);
        return FALSE;
    }

    /* Enumerate values */
    for (i = 0; i < dwValues; i++)
    {
        dwValueNameLength = dwMaxValueNameLength;
        dwValueDataLength = dwMaxValueDataLength;

        Error = RegEnumValueW(hEnvKey,
                              i,
                              lpValueName,
                              &dwValueNameLength,
                              NULL,
                              &dwType,
                              (LPBYTE)lpValueData,
                              &dwValueDataLength);
        if (Error == ERROR_SUCCESS)
        {
            if (!_wcsicmp(lpValueName, L"PATH"))
            {
                /* Append 'Path' environment variable */
                AppendUserEnvironmentVariable(Environment,
                                              lpValueName,
                                              lpValueData);
            }
            else
            {
                /* Set environment variable */
                SetUserEnvironmentVariable(Environment,
                                           lpValueName,
                                           lpValueData,
                                           (dwType == REG_EXPAND_SZ));
            }
        }
        else
        {
            LocalFree(lpValueData);
            LocalFree(lpValueName);
            RegCloseKey(hEnvKey);

            return FALSE;
        }
    }

    LocalFree(lpValueData);
    LocalFree(lpValueName);
    RegCloseKey(hEnvKey);

    return TRUE;
}


static
BOOL
SetSystemEnvironment(PWSTR* Environment)
{
    LONG Error;
    HKEY hEnvKey;
    DWORD dwValues;
    DWORD dwMaxValueNameLength;
    DWORD dwMaxValueDataLength;
    DWORD dwValueNameLength;
    DWORD dwValueDataLength;
    DWORD dwType;
    DWORD i;
    LPWSTR lpValueName;
    LPWSTR lpValueData;

    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
                          0,
                          KEY_QUERY_VALUE,
                          &hEnvKey);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("RegOpenKeyExW() failed (Error %ld)\n", Error);
        return FALSE;
    }

    Error = RegQueryInfoKey(hEnvKey,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            &dwValues,
                            &dwMaxValueNameLength,
                            &dwMaxValueDataLength,
                            NULL,
                            NULL);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("RegQueryInforKey() failed (Error %ld)\n", Error);
        RegCloseKey(hEnvKey);
        return FALSE;
    }

    if (dwValues == 0)
    {
        RegCloseKey(hEnvKey);
        return TRUE;
    }

    /* Allocate buffers */
    dwMaxValueNameLength++;
    lpValueName = LocalAlloc(LPTR, dwMaxValueNameLength * sizeof(WCHAR));
    if (lpValueName == NULL)
    {
        RegCloseKey(hEnvKey);
        return FALSE;
    }

    lpValueData = LocalAlloc(LPTR, dwMaxValueDataLength);
    if (lpValueData == NULL)
    {
        LocalFree(lpValueName);
        RegCloseKey(hEnvKey);
        return FALSE;
    }

    /* Enumerate values */
    for (i = 0; i < dwValues; i++)
    {
        dwValueNameLength = dwMaxValueNameLength;
        dwValueDataLength = dwMaxValueDataLength;

        Error = RegEnumValueW(hEnvKey,
                              i,
                              lpValueName,
                              &dwValueNameLength,
                              NULL,
                              &dwType,
                              (LPBYTE)lpValueData,
                              &dwValueDataLength);
        if (Error == ERROR_SUCCESS)
        {
            /* Set environment variable */
            SetUserEnvironmentVariable(Environment,
                                       lpValueName,
                                       lpValueData,
                                       (dwType == REG_EXPAND_SZ));
        }
        else
        {
            LocalFree(lpValueData);
            LocalFree(lpValueName);
            RegCloseKey(hEnvKey);

            return FALSE;
        }
    }

    LocalFree(lpValueData);
    LocalFree(lpValueName);
    RegCloseKey(hEnvKey);

    return TRUE;
}


BOOL
WINAPI
CreateEnvironmentBlock(OUT LPVOID *lpEnvironment,
                       IN HANDLE hToken,
                       IN BOOL bInherit)
{
    NTSTATUS Status;
    LONG lError;
    PWSTR* Environment = (PWSTR*)lpEnvironment;
    DWORD Length;
    DWORD dwType;
    HKEY hKey;
    HKEY hKeyUser;
    LPWSTR lpUserName = NULL;
    LPWSTR lpDomainName = NULL;
    WCHAR Buffer[MAX_PATH];
    WCHAR szValue[1024];

    DPRINT("CreateEnvironmentBlock() called\n");

    if (lpEnvironment == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Status = RtlCreateEnvironment((BOOLEAN)bInherit, Environment);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlCreateEnvironment() failed (Status %lx)\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* Set 'SystemRoot' variable */
    Length = ARRAYSIZE(Buffer);
    if (GetEnvironmentVariableW(L"SystemRoot", Buffer, Length))
    {
        SetUserEnvironmentVariable(Environment,
                                   L"SystemRoot",
                                   Buffer,
                                   FALSE);
    }

    /* Set 'SystemDrive' variable */
    if (GetEnvironmentVariableW(L"SystemDrive", Buffer, Length))
    {
        SetUserEnvironmentVariable(Environment,
                                   L"SystemDrive",
                                   Buffer,
                                   FALSE);
    }

    /* Set variables from Session Manager */
    if (!SetSystemEnvironment(Environment))
    {
        RtlDestroyEnvironment(*Environment);
        return FALSE;
    }

    /* Set 'COMPUTERNAME' variable */
    Length = ARRAYSIZE(Buffer);
    if (GetComputerNameW(Buffer, &Length))
    {
        SetUserEnvironmentVariable(Environment,
                                   L"COMPUTERNAME",
                                   Buffer,
                                   FALSE);
    }

    /* Set 'ALLUSERSPROFILE' variable */
    Length = ARRAYSIZE(Buffer);
    if (GetAllUsersProfileDirectoryW(Buffer, &Length))
    {
        SetUserEnvironmentVariable(Environment,
                                   L"ALLUSERSPROFILE",
                                   Buffer,
                                   FALSE);
    }

    /* Set 'USERPROFILE' variable to the default users profile */
    Length = ARRAYSIZE(Buffer);
    if (GetDefaultUserProfileDirectoryW(Buffer, &Length))
    {
        SetUserEnvironmentVariable(Environment,
                                   L"USERPROFILE",
                                   Buffer,
                                   TRUE);
    }

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion",
                           0,
                           KEY_READ,
                           &hKey);
    if (lError == ERROR_SUCCESS)
    {
        Length = sizeof(szValue);
        lError = RegQueryValueExW(hKey,
                                  L"ProgramFilesDir",
                                  NULL,
                                  &dwType,
                                  (LPBYTE)szValue,
                                  &Length);
        if (lError == ERROR_SUCCESS)
        {
            SetUserEnvironmentVariable(Environment,
                                       L"ProgramFiles",
                                       szValue,
                                       FALSE);
        }

        Length = sizeof(szValue);
        lError = RegQueryValueExW(hKey,
                                  L"CommonFilesDir",
                                  NULL,
                                  &dwType,
                                  (LPBYTE)szValue,
                                  &Length);
        if (lError == ERROR_SUCCESS)
        {
            SetUserEnvironmentVariable(Environment,
                                       L"CommonProgramFiles",
                                       szValue,
                                       FALSE);
        }

        RegCloseKey(hKey);
    }

    /*
     * If no user token is specified, the system environment variables are set
     * and we stop here, otherwise continue setting the user-specific variables.
     */
    if (hToken == NULL)
        return TRUE;

    hKeyUser = GetCurrentUserKey(hToken);
    if (hKeyUser == NULL)
    {
        DPRINT1("GetCurrentUserKey() failed\n");
        RtlDestroyEnvironment(*Environment);
        return FALSE;
    }

    /* Set 'USERPROFILE' variable */
    Length = ARRAYSIZE(Buffer);
    if (GetUserProfileDirectoryW(hToken, Buffer, &Length))
    {
        DWORD MinLen = 2;

        SetUserEnvironmentVariable(Environment,
                                   L"USERPROFILE",
                                   Buffer,
                                   FALSE);

        // FIXME: Strangely enough the following two environment variables
        // are not set by userenv.dll in Windows... See r68284 / CORE-9875
        // FIXME2: This is done by msgina.dll !!

        /* At least <drive letter>:<path> */
        if (Length > MinLen)
        {
            /* Set 'HOMEDRIVE' variable */
            StringCchCopyNW(szValue, ARRAYSIZE(Buffer), Buffer, MinLen);
            SetUserEnvironmentVariable(Environment,
                                       L"HOMEDRIVE",
                                       szValue,
                                       FALSE);

            /* Set 'HOMEPATH' variable */
            StringCchCopyNW(szValue, ARRAYSIZE(Buffer), Buffer + MinLen, Length - MinLen);
            SetUserEnvironmentVariable(Environment,
                                       L"HOMEPATH",
                                       szValue,
                                       FALSE);
        }
    }
    else
    {
        DPRINT1("GetUserProfileDirectoryW failed with error %lu\n", GetLastError());
    }

    if (GetUserAndDomainName(hToken,
                             &lpUserName,
                             &lpDomainName))
    {
        /* Set 'USERNAME' variable */
        SetUserEnvironmentVariable(Environment,
                                   L"USERNAME",
                                   lpUserName,
                                   FALSE);

        /* Set 'USERDOMAIN' variable */
        SetUserEnvironmentVariable(Environment,
                                   L"USERDOMAIN",
                                   lpDomainName,
                                   FALSE);

        if (lpUserName != NULL)
            LocalFree(lpUserName);

        if (lpDomainName != NULL)
            LocalFree(lpDomainName);
    }

    /* Set user environment variables */
    SetUserEnvironment(Environment,
                       hKeyUser,
                       L"Environment");

    /* Set user volatile environment variables */
    SetUserEnvironment(Environment,
                       hKeyUser,
                       L"Volatile Environment");

    RegCloseKey(hKeyUser);

    return TRUE;
}


BOOL
WINAPI
DestroyEnvironmentBlock(IN LPVOID lpEnvironment)
{
    DPRINT("DestroyEnvironmentBlock() called\n");

    if (lpEnvironment == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlDestroyEnvironment(lpEnvironment);

    return TRUE;
}


BOOL
WINAPI
ExpandEnvironmentStringsForUserW(IN HANDLE hToken,
                                 IN LPCWSTR lpSrc,
                                 OUT LPWSTR lpDest,
                                 IN DWORD dwSize)
{
    BOOL Ret = FALSE;
    PVOID lpEnvironment;

    if (lpSrc == NULL || lpDest == NULL || dwSize == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (CreateEnvironmentBlock(&lpEnvironment,
                               hToken,
                               FALSE))
    {
        UNICODE_STRING SrcU, DestU;
        NTSTATUS Status;

        /* Initialize the strings */
        RtlInitUnicodeString(&SrcU, lpSrc);
        DestU.Length = 0;
        DestU.MaximumLength = dwSize * sizeof(WCHAR);
        DestU.Buffer = lpDest;

        /* Expand the strings */
        Status = RtlExpandEnvironmentStrings_U((PWSTR)lpEnvironment,
                                               &SrcU,
                                               &DestU,
                                               NULL);

        DestroyEnvironmentBlock(lpEnvironment);

        if (NT_SUCCESS(Status))
        {
            Ret = TRUE;
        }
        else
        {
            SetLastError(RtlNtStatusToDosError(Status));
        }
    }

    return Ret;
}


BOOL
WINAPI
ExpandEnvironmentStringsForUserA(IN HANDLE hToken,
                                 IN LPCSTR lpSrc,
                                 OUT LPSTR lpDest,
                                 IN DWORD dwSize)
{
    BOOL Ret = FALSE;
    DWORD dwSrcLen;
    LPWSTR lpSrcW = NULL, lpDestW = NULL;

    if (lpSrc == NULL || lpDest == NULL || dwSize == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    dwSrcLen = strlen(lpSrc);
    lpSrcW = (LPWSTR)GlobalAlloc(GMEM_FIXED,
                                 (dwSrcLen + 1) * sizeof(WCHAR));
    if (lpSrcW == NULL ||
        MultiByteToWideChar(CP_ACP,
                            0,
                            lpSrc,
                            -1,
                            lpSrcW,
                            dwSrcLen + 1) == 0)
    {
        goto Cleanup;
    }

    lpDestW = (LPWSTR)GlobalAlloc(GMEM_FIXED,
                                  dwSize * sizeof(WCHAR));
    if (lpDestW == NULL)
        goto Cleanup;

    Ret = ExpandEnvironmentStringsForUserW(hToken,
                                           lpSrcW,
                                           lpDestW,
                                           dwSize);
    if (Ret)
    {
        if (WideCharToMultiByte(CP_ACP,
                                0,
                                lpDestW,
                                -1,
                                lpDest,
                                dwSize,
                                NULL,
                                NULL) == 0)
        {
            Ret = FALSE;
        }
    }

Cleanup:
    if (lpSrcW != NULL)
        GlobalFree((HGLOBAL)lpSrcW);

    if (lpDestW != NULL)
        GlobalFree((HGLOBAL)lpDestW);

    return Ret;
}

/* EOF */
