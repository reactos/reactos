/* $Id: profile.c,v 1.5 2004/02/28 11:30:59 ekohl Exp $
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
GetUserProfileDirectoryW (HANDLE hToken,
			  LPWSTR lpProfileDir,
			  LPDWORD lpcchSize)
{
  /* FIXME */
  return GetDefaultUserProfileDirectoryW (lpProfileDir, lpcchSize);
}


BOOL WINAPI
LoadUserProfileW (HANDLE hToken,
		  LPPROFILEINFOW lpProfileInfo)
{

  /* Check profile info */
  if (lpProfileInfo->dwSize != sizeof(PROFILEINFOW) ||
      lpProfileInfo->lpUserName == NULL ||
      lpProfileInfo->lpUserName[0] == 0)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  /* FIXME: load the profile */

  return TRUE;
}

/* EOF */
