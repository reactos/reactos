/* $Id: profile.c,v 1.1 2004/01/13 12:34:09 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/profile.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <string.h>

#include <userenv.h>

#include "internal.h"


/* FUNCTIONS ***************************************************************/

BOOL WINAPI
CreateUserProfileW (PSID Sid,
		    LPCWSTR lpUserName)
{
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
