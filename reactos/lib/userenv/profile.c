/* $Id: profile.c,v 1.11 2004/05/03 12:05:44 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/profile.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include <ntos.h>
#include <windows.h>
#include <string.h>

#include <userenv.h>

#include "internal.h"


/* FUNCTIONS ***************************************************************/

BOOL
AppendSystemPostfix (LPWSTR lpName,
		     DWORD dwMaxLength)
{
  WCHAR szSystemRoot[MAX_PATH];
  WCHAR szDrivePostfix[3];
  LPWSTR lpszPostfix;
  LPWSTR lpszPtr;
  DWORD dwPostfixLength;

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

  dwPostfixLength = wcslen (lpszPostfix);
  if (szSystemRoot[0] != L'C')
    {
      dwPostfixLength += 2;
      szDrivePostfix[0] = L'_';
      szDrivePostfix[1] = szSystemRoot[0];
      szDrivePostfix[2] = (WCHAR)0;
    }

  if (wcslen (lpName) + dwPostfixLength >= dwMaxLength)
    {
      DPRINT1("Error: buffer overflow\n");
      return FALSE;
    }

  wcscat (lpName, lpszPostfix);
  if (szSystemRoot[0] != L'C')
    {
      wcscat (lpName, szDrivePostfix);
    }

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
  UNICODE_STRING SidString;
  DWORD dwLength;
  DWORD dwDisposition;
  HKEY hKey;
  NTSTATUS Status;

  DPRINT ("CreateUserProfileW() called\n");

  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
		     0,
		     KEY_ALL_ACCESS,
		     &hKey))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      return FALSE;
    }

  /* Get profiles path */
  dwLength = MAX_PATH * sizeof(WCHAR);
  if (RegQueryValueExW (hKey,
			L"ProfilesDirectory",
			NULL,
			NULL,
			(LPBYTE)szRawProfilesPath,
			&dwLength))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
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
  if (RegQueryValueExW (hKey,
			L"DefaultUserProfile",
			NULL,
			NULL,
			(LPBYTE)szBuffer,
			&dwLength))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
      return FALSE;
    }

  RegCloseKey (hKey);

  wcscpy (szUserProfilePath, szProfilesPath);
  wcscat (szUserProfilePath, L"\\");
  wcscat (szUserProfilePath, lpUserName);
  if (!AppendSystemPostfix (szUserProfilePath, MAX_PATH))
    {
      DPRINT1("AppendSystemPostfix() failed\n", GetLastError());
      RtlFreeUnicodeString (&SidString);
      RegCloseKey (hKey);
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
  Status = RtlConvertSidToUnicodeString (&SidString, Sid, TRUE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Status: %lx\n", Status);
      return FALSE;
    }

  wcscpy (szBuffer,
	  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
  wcscat (szBuffer, SidString.Buffer);

  /* Create user profile key */
  if (RegCreateKeyExW (HKEY_LOCAL_MACHINE,
		       szBuffer,
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       KEY_ALL_ACCESS,
		       NULL,
		       &hKey,
		       &dwDisposition))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RtlFreeUnicodeString (&SidString);
      return FALSE;
    }

  /* Create non-expanded user profile path */
  wcscpy (szBuffer, szRawProfilesPath);
  wcscat (szBuffer, L"\\");
  wcscat (szBuffer, lpUserName);
  if (!AppendSystemPostfix (szBuffer, MAX_PATH))
    {
      DPRINT1("AppendSystemPostfix() failed\n", GetLastError());
      RtlFreeUnicodeString (&SidString);
      RegCloseKey (hKey);
      return FALSE;
    }

  /* Set 'ProfileImagePath' value (non-expanded) */
  if (RegSetValueExW (hKey,
		      L"ProfileImagePath",
		      0,
		      REG_EXPAND_SZ,
		      (LPBYTE)szBuffer,
		      (wcslen (szBuffer) + 1) * sizeof(WCHAR)))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RtlFreeUnicodeString (&SidString);
      RegCloseKey (hKey);
      return FALSE;
    }

  /* Set 'Sid' value */
  if (RegSetValueExW (hKey,
		      L"Sid",
		      0,
		      REG_BINARY,
		      Sid,
		      RtlLengthSid (Sid)))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RtlFreeUnicodeString (&SidString);
      RegCloseKey (hKey);
      return FALSE;
    }

  RegCloseKey (hKey);

  /* Create user hive name */
  wcscat (szUserProfilePath, L"\\ntuser.dat");

  /* Create new user hive */
  if (RegLoadKeyW (HKEY_USERS,
		   SidString.Buffer,
		   szUserProfilePath))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RtlFreeUnicodeString (&SidString);
      return FALSE;
    }

  /* Initialize user hive */
  if (!CreateUserHive (SidString.Buffer))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RtlFreeUnicodeString (&SidString);
      return FALSE;
    }

  RegUnLoadKeyW (HKEY_USERS,
		 SidString.Buffer);

  RtlFreeUnicodeString (&SidString);

  DPRINT ("CreateUserProfileW() done\n");

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

  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
		     0,
		     KEY_READ,
		     &hKey))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      return FALSE;
    }

  /* Get profiles path */
  dwLength = MAX_PATH * sizeof(WCHAR);
  if (RegQueryValueExW (hKey,
			L"ProfilesDirectory",
			NULL,
			NULL,
			(LPBYTE)szBuffer,
			&dwLength))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
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
  dwLength = MAX_PATH * sizeof(WCHAR);
  if (RegQueryValueExW (hKey,
			L"AllUsersProfile",
			NULL,
			NULL,
			(LPBYTE)szBuffer,
			&dwLength))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
      return FALSE;
    }

  RegCloseKey (hKey);

  wcscat (szProfilePath, L"\\");
  wcscat (szProfilePath, szBuffer);

  dwLength = wcslen (szProfilePath);
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

  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
		     0,
		     KEY_READ,
		     &hKey))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      return FALSE;
    }

  /* Get profiles path */
  dwLength = MAX_PATH * sizeof(WCHAR);
  if (RegQueryValueExW (hKey,
			L"ProfilesDirectory",
			NULL,
			NULL,
			(LPBYTE)szBuffer,
			&dwLength))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
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
  dwLength = MAX_PATH * sizeof(WCHAR);
  if (RegQueryValueExW (hKey,
			L"DefaultUserProfile",
			NULL,
			NULL,
			(LPBYTE)szBuffer,
			&dwLength))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
      return FALSE;
    }

  RegCloseKey (hKey);

  wcscat (szProfilePath, L"\\");
  wcscat (szProfilePath, szBuffer);

  dwLength = wcslen (szProfilePath);
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

  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
		     0,
		     KEY_READ,
		     &hKey))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      return FALSE;
    }

  /* Get profiles path */
  dwLength = MAX_PATH * sizeof(WCHAR);
  if (RegQueryValueExW (hKey,
			L"ProfilesDirectory",
			NULL,
			NULL,
			(LPBYTE)szBuffer,
			&dwLength))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
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

  dwLength = wcslen (szProfilesPath);
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

  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     szKeyName,
		     0,
		     KEY_ALL_ACCESS,
		     &hKey))
    {
      DPRINT1 ("Error: %lu\n", GetLastError());
      return FALSE;
    }

  dwLength = MAX_PATH * sizeof(WCHAR);
  if (RegQueryValueExW (hKey,
			L"ProfileImagePath",
			NULL,
			NULL,
			(LPBYTE)szRawImagePath,
			&dwLength))
    {
      DPRINT1 ("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
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

  dwLength = wcslen (szImagePath);
  if (dwLength > *lpcchSize)
    {
      DPRINT1 ("Buffer too small\n");
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
		     KEY_ALL_ACCESS,
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
LoadUserProfileW (HANDLE hToken,
		  LPPROFILEINFOW lpProfileInfo)
{
  WCHAR szUserHivePath[MAX_PATH];
  UNICODE_STRING SidString;
  DWORD dwLength;

  DPRINT ("LoadUserProfileW() called\n");

  /* Check profile info */
  if (lpProfileInfo->dwSize != sizeof(PROFILEINFOW) ||
      lpProfileInfo->lpUserName == NULL ||
      lpProfileInfo->lpUserName[0] == 0)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  /* Don't load a profile twice */
  if (CheckForLoadedProfile (hToken))
    {
      DPRINT ("Profile already loaded\n");
      lpProfileInfo->hProfile = NULL;
      return TRUE;
    }

  if (!GetProfilesDirectoryW (szUserHivePath,
			      &dwLength))
    {
      DPRINT1("GetProfilesDirectoryW() failed\n", GetLastError());
      return FALSE;
    }

  wcscat (szUserHivePath, L"\\");
  wcscat (szUserHivePath, lpProfileInfo->lpUserName);
  if (!AppendSystemPostfix (szUserHivePath, MAX_PATH))
    {
      DPRINT1("AppendSystemPostfix() failed\n", GetLastError());
      return FALSE;
    }

  /* Create user hive name */
  wcscat (szUserHivePath, L"\\ntuser.dat");

  DPRINT ("szUserHivePath: %S\n", szUserHivePath);

  if (!GetUserSidFromToken (hToken,
			    &SidString))
    {
      DPRINT1 ("GetUserSidFromToken() failed\n");
      return FALSE;
    }

  DPRINT ("SidString: '%wZ'\n", &SidString);

  if (RegLoadKeyW (HKEY_USERS,
		   SidString.Buffer,
		   szUserHivePath))
    {
      DPRINT1 ("RegLoadKeyW() failed (Error %ld)\n", GetLastError());
      RtlFreeUnicodeString (&SidString);
      return FALSE;
    }

  if (RegOpenKeyExW (HKEY_USERS,
		     SidString.Buffer,
		     0,
		     KEY_ALL_ACCESS,
		     (PHKEY)&lpProfileInfo->hProfile))
    {
      DPRINT1 ("RegOpenKeyExW() failed (Error %ld)\n", GetLastError());
      RtlFreeUnicodeString (&SidString);
      return FALSE;
    }

  RtlFreeUnicodeString (&SidString);

  DPRINT ("LoadUserProfileW() done\n");

  return TRUE;
}


BOOL WINAPI
UnloadUserProfile (HANDLE hToken,
		   HANDLE hProfile)
{
  UNICODE_STRING SidString;

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

  if (RegUnLoadKeyW (HKEY_USERS,
		     SidString.Buffer))
    {
      DPRINT1 ("RegUnLoadKeyW() failed (Error %ld)\n", GetLastError());
      RtlFreeUnicodeString (&SidString);
      return FALSE;
    }

  RtlFreeUnicodeString (&SidString);

  DPRINT ("UnloadUserProfile() done\n");

  return TRUE;
}

/* EOF */
