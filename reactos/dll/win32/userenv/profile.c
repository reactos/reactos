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
 * FILE:            lib/userenv/profile.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ***************************************************************/

BOOL
AppendSystemPostfix (LPWSTR lpName,
		     DWORD dwMaxLength)
{
  WCHAR szSystemRoot[MAX_PATH];
  LPWSTR lpszPostfix;
  LPWSTR lpszPtr;

  /* Build profile name postfix */
  if (!ExpandEnvironmentStringsW (L"%SystemRoot%",
				  szSystemRoot,
				  MAX_PATH))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      return FALSE;
    }

  _wcsupr (szSystemRoot);

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


BOOL WINAPI
CreateUserProfileA (PSID Sid,
		    LPCSTR lpUserName)
{
  UNICODE_STRING UserName;
  BOOL bResult;
  NTSTATUS Status;

  Status = RtlCreateUnicodeStringFromAsciiz (&UserName,
					     (LPSTR)lpUserName);
  if (!NT_SUCCESS(Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  bResult = CreateUserProfileW (Sid,
				UserName.Buffer);

  RtlFreeUnicodeString (&UserName);

  return bResult;
}


BOOL WINAPI
CreateUserProfileW (PSID Sid,
		    LPCWSTR lpUserName)
{
  WCHAR szRawProfilesPath[MAX_PATH];
  WCHAR szProfilesPath[MAX_PATH];
  WCHAR szUserProfilePath[MAX_PATH];
  WCHAR szDefaultUserPath[MAX_PATH];
  WCHAR szBuffer[MAX_PATH];
  LPWSTR SidString;
  DWORD dwLength;
  DWORD dwDisposition;
  HKEY hKey;
  LONG Error;

  DPRINT("CreateUserProfileW() called\n");

  Error = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
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
  Error = RegQueryValueExW (hKey,
			    L"ProfilesDirectory",
			    NULL,
			    NULL,
			    (LPBYTE)szRawProfilesPath,
			    &dwLength);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1("Error: %lu\n", Error);
      RegCloseKey (hKey);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  /* Expand it */
  if (!ExpandEnvironmentStringsW (szRawProfilesPath,
				  szProfilesPath,
				  MAX_PATH))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
      return FALSE;
    }

  /* Get default user path */
  dwLength = MAX_PATH * sizeof(WCHAR);
  Error = RegQueryValueExW (hKey,
			    L"DefaultUserProfile",
			    NULL,
			    NULL,
			    (LPBYTE)szBuffer,
			    &dwLength);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1("Error: %lu\n", Error);
      RegCloseKey (hKey);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  RegCloseKey (hKey);

  wcscpy (szUserProfilePath, szProfilesPath);
  wcscat (szUserProfilePath, L"\\");
  wcscat (szUserProfilePath, lpUserName);
  if (!AppendSystemPostfix (szUserProfilePath, MAX_PATH))
    {
      DPRINT1("AppendSystemPostfix() failed\n", GetLastError());
      LocalFree ((HLOCAL)SidString);
      return FALSE;
    }

  wcscpy (szDefaultUserPath, szProfilesPath);
  wcscat (szDefaultUserPath, L"\\");
  wcscat (szDefaultUserPath, szBuffer);

  /* Create user profile directory */
  if (!CreateDirectoryW (szUserProfilePath, NULL))
    {
      if (GetLastError () != ERROR_ALREADY_EXISTS)
	{
	  DPRINT1("Error: %lu\n", GetLastError());
	  return FALSE;
	}
    }

  /* Copy default user directory */
  if (!CopyDirectory (szUserProfilePath, szDefaultUserPath))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      return FALSE;
    }

  /* Add profile to profile list */
  if (!ConvertSidToStringSidW (Sid,
                               &SidString))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      return FALSE;
    }

  wcscpy (szBuffer,
	  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
  wcscat (szBuffer, SidString);

  /* Create user profile key */
  Error = RegCreateKeyExW (HKEY_LOCAL_MACHINE,
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
      LocalFree ((HLOCAL)SidString);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  /* Create non-expanded user profile path */
  wcscpy (szBuffer, szRawProfilesPath);
  wcscat (szBuffer, L"\\");
  wcscat (szBuffer, lpUserName);
  if (!AppendSystemPostfix (szBuffer, MAX_PATH))
    {
      DPRINT1("AppendSystemPostfix() failed\n", GetLastError());
      LocalFree ((HLOCAL)SidString);
      RegCloseKey (hKey);
      return FALSE;
    }

  /* Set 'ProfileImagePath' value (non-expanded) */
  Error = RegSetValueExW (hKey,
		          L"ProfileImagePath",
		          0,
		          REG_EXPAND_SZ,
		          (LPBYTE)szBuffer,
		          (wcslen (szBuffer) + 1) * sizeof(WCHAR));
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1("Error: %lu\n", Error);
      LocalFree ((HLOCAL)SidString);
      RegCloseKey (hKey);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  /* Set 'Sid' value */
  Error = RegSetValueExW (hKey,
		          L"Sid",
		          0,
		          REG_BINARY,
		          Sid,
		          GetLengthSid (Sid));
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1("Error: %lu\n", Error);
      LocalFree ((HLOCAL)SidString);
      RegCloseKey (hKey);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  RegCloseKey (hKey);

  /* Create user hive name */
  wcscpy (szBuffer, szUserProfilePath);
  wcscat (szBuffer, L"\\ntuser.dat");

  /* Create new user hive */
  Error = RegLoadKeyW (HKEY_USERS,
		       SidString,
		       szBuffer);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1("Error: %lu\n", Error);
      LocalFree ((HLOCAL)SidString);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  /* Initialize user hive */
  if (!CreateUserHive (SidString, szUserProfilePath))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      LocalFree ((HLOCAL)SidString);
      return FALSE;
    }

  RegUnLoadKeyW (HKEY_USERS,
		 SidString);

  LocalFree ((HLOCAL)SidString);

  DPRINT("CreateUserProfileW() done\n");

  return TRUE;
}


BOOL WINAPI
GetAllUsersProfileDirectoryA (LPSTR lpProfileDir,
			      LPDWORD lpcchSize)
{
  LPWSTR lpBuffer;
  BOOL bResult;

  lpBuffer = GlobalAlloc (GMEM_FIXED,
			  *lpcchSize * sizeof(WCHAR));
  if (lpBuffer == NULL)
    return FALSE;

  bResult = GetAllUsersProfileDirectoryW (lpBuffer,
					  lpcchSize);
  if (bResult)
    {
      WideCharToMultiByte (CP_ACP,
			   0,
			   lpBuffer,
			   -1,
			   lpProfileDir,
			   *lpcchSize,
			   NULL,
			   NULL);
    }

  GlobalFree (lpBuffer);

  return bResult;
}


BOOL WINAPI
GetAllUsersProfileDirectoryW (LPWSTR lpProfileDir,
			      LPDWORD lpcchSize)
{
  WCHAR szProfilePath[MAX_PATH];
  WCHAR szBuffer[MAX_PATH];
  DWORD dwLength;
  HKEY hKey;
  LONG Error;

  Error = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
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
  Error = RegQueryValueExW (hKey,
			    L"ProfilesDirectory",
			    NULL,
			    NULL,
			    (LPBYTE)szBuffer,
			    &dwLength);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1("Error: %lu\n", Error);
      RegCloseKey (hKey);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  /* Expand it */
  if (!ExpandEnvironmentStringsW (szBuffer,
				  szProfilePath,
				  MAX_PATH))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
      return FALSE;
    }

  /* Get 'AllUsersProfile' name */
  dwLength = sizeof(szBuffer);
  Error = RegQueryValueExW (hKey,
			    L"AllUsersProfile",
			    NULL,
			    NULL,
			    (LPBYTE)szBuffer,
			    &dwLength);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1("Error: %lu\n", Error);
      RegCloseKey (hKey);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  RegCloseKey (hKey);

  wcscat (szProfilePath, L"\\");
  wcscat (szProfilePath, szBuffer);

  dwLength = wcslen (szProfilePath) + 1;
  if (lpProfileDir != NULL)
    {
      if (*lpcchSize < dwLength)
	{
	  *lpcchSize = dwLength;
	  SetLastError (ERROR_INSUFFICIENT_BUFFER);
	  return FALSE;
	}

      wcscpy (lpProfileDir, szProfilePath);
    }

  *lpcchSize = dwLength;

  return TRUE;
}


BOOL WINAPI
GetDefaultUserProfileDirectoryA (LPSTR lpProfileDir,
				 LPDWORD lpcchSize)
{
  LPWSTR lpBuffer;
  BOOL bResult;

  lpBuffer = GlobalAlloc (GMEM_FIXED,
			  *lpcchSize * sizeof(WCHAR));
  if (lpBuffer == NULL)
    return FALSE;

  bResult = GetDefaultUserProfileDirectoryW (lpBuffer,
					     lpcchSize);
  if (bResult)
    {
      WideCharToMultiByte (CP_ACP,
			   0,
			   lpBuffer,
			   -1,
			   lpProfileDir,
			   *lpcchSize,
			   NULL,
			   NULL);
    }

  GlobalFree (lpBuffer);

  return bResult;
}


BOOL WINAPI
GetDefaultUserProfileDirectoryW (LPWSTR lpProfileDir,
				 LPDWORD lpcchSize)
{
  WCHAR szProfilePath[MAX_PATH];
  WCHAR szBuffer[MAX_PATH];
  DWORD dwLength;
  HKEY hKey;
  LONG Error;

  Error = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
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
  Error = RegQueryValueExW (hKey,
			    L"ProfilesDirectory",
			    NULL,
			    NULL,
			    (LPBYTE)szBuffer,
			    &dwLength);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1("Error: %lu\n", Error);
      RegCloseKey (hKey);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  /* Expand it */
  if (!ExpandEnvironmentStringsW (szBuffer,
				  szProfilePath,
				  MAX_PATH))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
      return FALSE;
    }

  /* Get 'DefaultUserProfile' name */
  dwLength = sizeof(szBuffer);
  Error = RegQueryValueExW (hKey,
			    L"DefaultUserProfile",
			    NULL,
			    NULL,
			   (LPBYTE)szBuffer,
			   &dwLength);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1("Error: %lu\n", Error);
      RegCloseKey (hKey);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  RegCloseKey (hKey);

  wcscat (szProfilePath, L"\\");
  wcscat (szProfilePath, szBuffer);

  dwLength = wcslen (szProfilePath) + 1;
  if (lpProfileDir != NULL)
    {
      if (*lpcchSize < dwLength)
	{
	  *lpcchSize = dwLength;
	  SetLastError (ERROR_INSUFFICIENT_BUFFER);
	  return FALSE;
	}

      wcscpy (lpProfileDir, szProfilePath);
    }

  *lpcchSize = dwLength;

  return TRUE;
}


BOOL WINAPI
GetProfilesDirectoryA (LPSTR lpProfileDir,
		       LPDWORD lpcchSize)
{
  LPWSTR lpBuffer;
  BOOL bResult;

  lpBuffer = GlobalAlloc (GMEM_FIXED,
			  *lpcchSize * sizeof(WCHAR));
  if (lpBuffer == NULL)
    return FALSE;

  bResult = GetProfilesDirectoryW (lpBuffer,
				   lpcchSize);
  if (bResult)
    {
      WideCharToMultiByte (CP_ACP,
			   0,
			   lpBuffer,
			   -1,
			   lpProfileDir,
			   *lpcchSize,
			   NULL,
			   NULL);
    }

  GlobalFree (lpBuffer);

  return bResult;
}


BOOL WINAPI
GetProfilesDirectoryW (LPWSTR lpProfilesDir,
		       LPDWORD lpcchSize)
{
  WCHAR szProfilesPath[MAX_PATH];
  WCHAR szBuffer[MAX_PATH];
  DWORD dwLength;
  HKEY hKey;
  LONG Error;

  Error = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
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
  Error = RegQueryValueExW (hKey,
			    L"ProfilesDirectory",
			    NULL,
			    NULL,
			    (LPBYTE)szBuffer,
			    &dwLength);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1("Error: %lu\n", Error);
      RegCloseKey (hKey);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  RegCloseKey (hKey);

  /* Expand it */
  if (!ExpandEnvironmentStringsW (szBuffer,
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
	  *lpcchSize = dwLength;
	  SetLastError (ERROR_INSUFFICIENT_BUFFER);
	  return FALSE;
	}

      wcscpy (lpProfilesDir, szProfilesPath);
    }

  *lpcchSize = dwLength;

  return TRUE;
}


BOOL WINAPI
GetUserProfileDirectoryA (HANDLE hToken,
			  LPSTR lpProfileDir,
			  LPDWORD lpcchSize)
{
  LPWSTR lpBuffer;
  BOOL bResult;

  lpBuffer = GlobalAlloc (GMEM_FIXED,
			  *lpcchSize * sizeof(WCHAR));
  if (lpBuffer == NULL)
    return FALSE;

  bResult = GetUserProfileDirectoryW (hToken,
				      lpBuffer,
				      lpcchSize);
  if (bResult)
    {
      WideCharToMultiByte (CP_ACP,
			   0,
			   lpBuffer,
			   -1,
			   lpProfileDir,
			   *lpcchSize,
			   NULL,
			   NULL);
    }

  GlobalFree (lpBuffer);

  return bResult;
}


BOOL WINAPI
GetUserProfileDirectoryW (HANDLE hToken,
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

  if (!GetUserSidFromToken (hToken,
			    &SidString))
    {
      DPRINT1 ("GetUserSidFromToken() failed\n");
      return FALSE;
    }

  DPRINT ("SidString: '%wZ'\n", &SidString);

  wcscpy (szKeyName,
	  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
  wcscat (szKeyName,
	  SidString.Buffer);

  RtlFreeUnicodeString (&SidString);

  DPRINT ("KeyName: '%S'\n", szKeyName);

  Error = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		         szKeyName,
		         0,
		         KEY_QUERY_VALUE,
		         &hKey);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1 ("Error: %lu\n", Error);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  dwLength = sizeof(szRawImagePath);
  Error = RegQueryValueExW (hKey,
			    L"ProfileImagePath",
			    NULL,
			    NULL,
			    (LPBYTE)szRawImagePath,
			    &dwLength);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1 ("Error: %lu\n", Error);
      RegCloseKey (hKey);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  RegCloseKey (hKey);

  DPRINT ("RawImagePath: '%S'\n", szRawImagePath);

  /* Expand it */
  if (!ExpandEnvironmentStringsW (szRawImagePath,
				  szImagePath,
				  MAX_PATH))
    {
      DPRINT1 ("Error: %lu\n", GetLastError());
      return FALSE;
    }

  DPRINT ("ImagePath: '%S'\n", szImagePath);

  dwLength = wcslen (szImagePath) + 1;
  if (*lpcchSize < dwLength)
    {
      *lpcchSize = dwLength;
      SetLastError (ERROR_INSUFFICIENT_BUFFER);
      return FALSE;
    }

  *lpcchSize = dwLength;
  wcscpy (lpProfileDir,
	  szImagePath);

  return TRUE;
}


static BOOL
CheckForLoadedProfile (HANDLE hToken)
{
  UNICODE_STRING SidString;
  HKEY hKey;

  DPRINT ("CheckForLoadedProfile() called \n");

  if (!GetUserSidFromToken (hToken,
			    &SidString))
    {
      DPRINT1 ("GetUserSidFromToken() failed\n");
      return FALSE;
    }

  if (RegOpenKeyExW (HKEY_USERS,
		     SidString.Buffer,
		     0,
		     MAXIMUM_ALLOWED,
		     &hKey))
    {
      DPRINT ("Profile not loaded\n");
      RtlFreeUnicodeString (&SidString);
      return FALSE;
    }

  RegCloseKey (hKey);

  RtlFreeUnicodeString (&SidString);

  DPRINT ("Profile already loaded\n");

  return TRUE;
}


BOOL WINAPI
LoadUserProfileA (HANDLE hToken,
		  LPPROFILEINFOA lpProfileInfo)
{
  DPRINT ("LoadUserProfileA() not implemented\n");
  return FALSE;
}


BOOL WINAPI
LoadUserProfileW(
    IN HANDLE hToken,
    IN OUT LPPROFILEINFOW lpProfileInfo)
{
    WCHAR szUserHivePath[MAX_PATH];
    LPWSTR UserName = NULL, Domain = NULL;
    DWORD UserNameLength = 0, DomainLength = 0;
    PTOKEN_USER UserSid = NULL;
    SID_NAME_USE AccountType;
    UNICODE_STRING SidString = { 0, };
    LONG Error;
    BOOL ret = FALSE;
    DWORD dwLength = sizeof(szUserHivePath) / sizeof(szUserHivePath[0]);

    DPRINT("LoadUserProfileW() called\n");

    /* Check profile info */
    if (!lpProfileInfo
     || lpProfileInfo->dwSize != sizeof(PROFILEINFOW)
     || lpProfileInfo->lpUserName == NULL
     || lpProfileInfo->lpUserName[0] == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return TRUE;
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

    wcscat(szUserHivePath, L"\\");
    wcscat(szUserHivePath, lpProfileInfo->lpUserName);
    dwLength = sizeof(szUserHivePath) / sizeof(szUserHivePath[0]);
    if (!AppendSystemPostfix(szUserHivePath, dwLength))
    {
        DPRINT1("AppendSystemPostfix() failed\n", GetLastError());
        return FALSE;
    }

    /* Create user hive name */
    wcscat(szUserHivePath, L"\\ntuser.dat");
    DPRINT("szUserHivePath: %S\n", szUserHivePath);

    /* Create user profile directory if needed */
    if (GetFileAttributesW(szUserHivePath) == INVALID_FILE_ATTRIBUTES)
    {
        /* Get user sid */
        if (GetTokenInformation(hToken, TokenUser, NULL, 0, &dwLength)
         || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            DPRINT1 ("GetTokenInformation() failed\n");
            return FALSE;
        }
        UserSid = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), 0, dwLength);
        if (!UserSid)
        {
            DPRINT1 ("HeapAlloc() failed\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        if (!GetTokenInformation(hToken, TokenUser, UserSid, dwLength, &dwLength))
        {
            DPRINT1 ("GetTokenInformation() failed\n");
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
            ret = LookupAccountSidW(
                NULL, UserSid->User.Sid,
                UserName, &UserNameLength,
                Domain, &DomainLength,
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
    ret = GetUserSidFromToken(hToken, &SidString);
    if (!ret)
    {
        DPRINT1("GetUserSidFromToken() failed\n");
        goto cleanup;
    }
    ret = FALSE;

    /* Load user registry hive */
    Error = RegLoadKeyW(HKEY_USERS,
                        SidString.Buffer,
                        szUserHivePath);
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


BOOL WINAPI
UnloadUserProfile (HANDLE hToken,
		   HANDLE hProfile)
{
  UNICODE_STRING SidString;
  LONG Error;

  DPRINT ("UnloadUserProfile() called\n");

  if (hProfile == NULL)
    {
      DPRINT1 ("Invalide profile handle\n");
      SetLastError (ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  RegCloseKey (hProfile);

  if (!GetUserSidFromToken (hToken,
			    &SidString))
    {
      DPRINT1 ("GetUserSidFromToken() failed\n");
      return FALSE;
    }

  DPRINT ("SidString: '%wZ'\n", &SidString);

  Error = RegUnLoadKeyW (HKEY_USERS,
		         SidString.Buffer);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1 ("RegUnLoadKeyW() failed (Error %ld)\n", Error);
      RtlFreeUnicodeString (&SidString);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  RtlFreeUnicodeString (&SidString);

  DPRINT ("UnloadUserProfile() done\n");

  return TRUE;
}

/* EOF */
