/* $Id: profile.c,v 1.2 2004/01/15 14:59:06 ekohl Exp $
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

//  DebugPrint ("User SID: %wZ\n", &SidString);
//  RtlFreeUnicodeString (&SidString);

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

  /* FIXME: Copy default user hive */

  RegUnLoadKeyW (HKEY_USERS,
		 SidString.Buffer);

  RtlFreeUnicodeString (&SidString);

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

/* EOF */
