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
/* $Id: setup.c,v 1.8 2004/10/02 12:31:28 ekohl Exp $ 
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/setup.c
 * PURPOSE:         Profile setup functions
 * PROGRAMMER:      Eric Kohl
 */

#include "precomp.h"

typedef struct _FOLDERDATA
{
  LPWSTR ValueName;
  LPWSTR Path;
  BOOL Hidden;
  BOOL bShellFolder;
  BOOL bUserShellFolder;
} FOLDERDATA, *PFOLDERDATA;


static FOLDERDATA
UserShellFolders[] =
{
  {L"AppData", L"Application Data", TRUE, TRUE, TRUE},
  {L"Desktop", L"Desktop", FALSE, TRUE, TRUE},
  {L"Favorites", L"Favorites", FALSE, TRUE, TRUE},
  {L"Personal", L"My Documents", FALSE, TRUE, TRUE},
  {L"PrintHood", L"PrintHood", TRUE, TRUE, TRUE},
  {L"Recent", L"Recent", TRUE, TRUE, TRUE},
  {L"SendTo", L"SendTo", FALSE, TRUE, TRUE},
  {L"Templates", L"Templates", FALSE, TRUE, TRUE},
  {L"Start Menu", L"Start Menu", FALSE, TRUE, TRUE},
  {L"Programs", L"Start Menu\\Programs", FALSE, TRUE, TRUE},
  {L"Startup", L"Start Menu\\Programs\\Startup", FALSE, TRUE, TRUE},
  {L"Local Settings", L"Local Settings", TRUE, TRUE, TRUE},
  {L"Local AppData", L"Local Settings\\Application Data", TRUE, TRUE, TRUE},
  {L"Temp", L"Local Settings\\Temp", FALSE, FALSE, FALSE},
  {NULL, NULL, FALSE, FALSE, FALSE}
};


static FOLDERDATA
CommonShellFolders[] =
{
  {L"Common AppData", L"Application Data", TRUE, TRUE, TRUE},
  {L"Common Desktop", L"Desktop", FALSE, TRUE, TRUE},
  {L"Common Favorites", L"Favorites", FALSE, TRUE, TRUE},
  {L"Common Start Menu", L"Start Menu", FALSE, TRUE, TRUE},
  {L"Common Programs", L"Start Menu\\Programs", FALSE, TRUE, TRUE},
  {L"Common Administrative Tools", L"Start Menu\\Programs\\Administrative Tools", FALSE, TRUE, FALSE},
  {L"Common Startup", L"Start Menu\\Programs\\Startup", FALSE, TRUE, TRUE},
  {L"Common Templates", L"Templates", TRUE, TRUE, TRUE},
  {L"Common Documents", L"My Documents", FALSE, TRUE, TRUE},
  {L"CommonPictures", L"My Documents\\My Pictures", FALSE, TRUE, TRUE},
  {L"CommonMusic", L"My Documents\\My Music", FALSE, TRUE, TRUE},
  {L"CommonVideo", L"My Documents\\My Videos", FALSE, TRUE, TRUE},
  {NULL, NULL, FALSE, FALSE, FALSE}
};


void
DebugPrint(char* fmt,...)
{
  char buffer[512];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

  OutputDebugStringA(buffer);
}


BOOL WINAPI
InitializeProfiles (VOID)
{
  WCHAR szProfilesPath[MAX_PATH];
  WCHAR szProfilePath[MAX_PATH];
  WCHAR szBuffer[MAX_PATH];
  DWORD dwLength;
  PFOLDERDATA lpFolderData;
  HKEY hKey;

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
			(LPBYTE)szBuffer,
			&dwLength))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
      return FALSE;
    }

  /* Expand it */
  if (!ExpandEnvironmentStringsW (szBuffer,
				  szProfilesPath,
				  MAX_PATH))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
      return FALSE;
    }

  /* Create profiles directory */
  if (!CreateDirectoryW (szProfilesPath, NULL))
    {
      if (GetLastError () != ERROR_ALREADY_EXISTS)
	{
	  DPRINT1("Error: %lu\n", GetLastError());
	  RegCloseKey (hKey);
	  return FALSE;
	}
    }

  /* Set 'DefaultUserProfile' value */
  wcscpy (szBuffer, L"Default User");
  if (!AppendSystemPostfix (szBuffer, MAX_PATH))
    {
      DPRINT1("AppendSystemPostfix() failed\n", GetLastError());
      RegCloseKey (hKey);
      return FALSE;
    }

  dwLength = (wcslen (szBuffer) + 1) * sizeof(WCHAR);
  if (RegSetValueExW (hKey,
		      L"DefaultUserProfile",
		      0,
		      REG_SZ,
		      (LPBYTE)szBuffer,
		      dwLength))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
      return FALSE;
    }

  RegCloseKey (hKey);

  /* Create 'Default User' profile directory */
  wcscpy (szProfilePath, szProfilesPath);
  wcscat (szProfilePath, L"\\");
  wcscat (szProfilePath, szBuffer);
  if (!CreateDirectoryW (szProfilePath, NULL))
    {
      if (GetLastError () != ERROR_ALREADY_EXISTS)
	{
	  DPRINT1("Error: %lu\n", GetLastError());
	  return FALSE;
	}
    }

  /* Set current user profile */
  SetEnvironmentVariableW (L"USERPROFILE", szProfilePath);

  /* Create 'Default User' subdirectories */
  /* FIXME: Get these paths from the registry */
  lpFolderData = &UserShellFolders[0];
  while (lpFolderData->ValueName != NULL)
    {
      wcscpy(szBuffer, szProfilePath);
      wcscat(szBuffer, L"\\");
      wcscat(szBuffer, lpFolderData->Path);

      if (!CreateDirectoryW(szBuffer, NULL))
	{
	  if (GetLastError () != ERROR_ALREADY_EXISTS)
	    {
	      DPRINT1("Error: %lu\n", GetLastError());
	      return FALSE;
	    }
	}

      if (lpFolderData->Hidden == TRUE)
	{
	  SetFileAttributesW (szBuffer,
			      FILE_ATTRIBUTE_HIDDEN);
	}

      lpFolderData++;
    }

  /* Set default 'Shell Folders' values */
  if (RegOpenKeyExW(HKEY_USERS,
		    L".Default\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
		    0,
		    KEY_ALL_ACCESS,
		    &hKey))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      return FALSE;
    }

  lpFolderData = &UserShellFolders[0];
  while (lpFolderData->ValueName != NULL)
    {
      if (lpFolderData->bShellFolder)
	{
	  wcscpy(szBuffer, szProfilePath);
	  wcscat(szBuffer, L"\\");
	  wcscat(szBuffer, lpFolderData->Path);

	  dwLength = (wcslen (szBuffer) + 1) * sizeof(WCHAR);
	  if (RegSetValueExW(hKey,
			     lpFolderData->ValueName,
			     0,
			     REG_SZ,
			     (LPBYTE)szBuffer,
			     dwLength))
	    {
	      DPRINT1("Error: %lu\n", GetLastError());
	      RegCloseKey(hKey);
	      return FALSE;
	    }
	}

      lpFolderData++;
    }

  /* Set 'Fonts' folder path */
  GetWindowsDirectory(szBuffer, MAX_PATH);
  wcscat(szBuffer, L"\\media\\fonts");

  dwLength = (wcslen (szBuffer) + 1) * sizeof(WCHAR);
  if (RegSetValueExW(hKey,
		     L"Fonts",
		     0,
		     REG_SZ,
		     (LPBYTE)szBuffer,
		     dwLength))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey(hKey);
      return FALSE;
    }

  RegCloseKey(hKey);

  /* Set default 'User Shell Folders' values */
  if (RegOpenKeyExW(HKEY_USERS,
		    L".Default\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
		    0,
		    KEY_ALL_ACCESS,
		    &hKey))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      return FALSE;
    }

  lpFolderData = &UserShellFolders[0];
  while (lpFolderData->ValueName != NULL)
    {
      if (lpFolderData->bUserShellFolder)
	{
	  wcscpy(szBuffer, L"%USERPROFILE%\\");
	  wcscat(szBuffer, lpFolderData->Path);

	  dwLength = (wcslen (szBuffer) + 1) * sizeof(WCHAR);
	  if (RegSetValueExW(hKey,
			     lpFolderData->ValueName,
			     0,
			     REG_EXPAND_SZ,
			     (LPBYTE)szBuffer,
			     dwLength))
	    {
	      DPRINT1("Error: %lu\n", GetLastError());
	      RegCloseKey(hKey);
	      return FALSE;
	    }
	}

      lpFolderData++;
    }

  RegCloseKey(hKey);


  /* Set 'AllUsersProfile' value */
  wcscpy (szBuffer, L"All Users");
  if (!AppendSystemPostfix (szBuffer, MAX_PATH))
    {
      DPRINT1("AppendSystemPostfix() failed\n", GetLastError());
      return FALSE;
    }

  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
		     0,
		     KEY_ALL_ACCESS,
		     &hKey))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      return FALSE;
    }

  dwLength = (wcslen (szBuffer) + 1) * sizeof(WCHAR);
  if (RegSetValueExW (hKey,
		      L"AllUsersProfile",
		      0,
		      REG_SZ,
		      (LPBYTE)szBuffer,
		      dwLength))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
      return FALSE;
    }

  RegCloseKey (hKey);


  /* Create 'All Users' profile directory */
  wcscpy (szProfilePath, szProfilesPath);
  wcscat (szProfilePath, L"\\");
  wcscat (szProfilePath, szBuffer);
  if (!CreateDirectoryW (szProfilePath, NULL))
    {
      if (GetLastError () != ERROR_ALREADY_EXISTS)
	{
	  DPRINT1("Error: %lu\n", GetLastError());
	  return FALSE;
	}
    }

  /* Set 'All Users' profile */
  SetEnvironmentVariableW (L"ALLUSERSPROFILE", szProfilePath);

  /* Create 'All Users' subdirectories */
  /* FIXME: Take these paths from the registry */
  lpFolderData = &CommonShellFolders[0];
  while (lpFolderData->ValueName != NULL)
    {
      wcscpy(szBuffer, szProfilePath);
      wcscat(szBuffer, L"\\");
      wcscat(szBuffer, lpFolderData->Path);

      if (!CreateDirectoryW(szBuffer, NULL))
	{
	  if (GetLastError () != ERROR_ALREADY_EXISTS)
	    {
	      DPRINT1("Error: %lu\n", GetLastError());
	      return FALSE;
	    }
	}

      if (lpFolderData->Hidden)
	{
	  SetFileAttributesW(szBuffer,
			     FILE_ATTRIBUTE_HIDDEN);
	}

      lpFolderData++;
    }

  /* Set common 'Shell Folders' values */
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
		    0,
		    KEY_ALL_ACCESS,
		    &hKey))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      return FALSE;
    }

  lpFolderData = &CommonShellFolders[0];
  while (lpFolderData->ValueName != NULL)
    {
      if (lpFolderData->bShellFolder)
	{
	  wcscpy(szBuffer, szProfilePath);
	  wcscat(szBuffer, L"\\");
	  wcscat(szBuffer, lpFolderData->Path);

	  dwLength = (wcslen (szBuffer) + 1) * sizeof(WCHAR);
	  if (RegSetValueExW(hKey,
			     lpFolderData->ValueName,
			     0,
			     REG_SZ,
			     (LPBYTE)szBuffer,
			     dwLength))
	    {
	      DPRINT1("Error: %lu\n", GetLastError());
	      RegCloseKey(hKey);
	      return FALSE;
	    }
	}

      lpFolderData++;
    }

  RegCloseKey(hKey);

  /* Set common 'User Shell Folders' values */
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
		    0,
		    KEY_ALL_ACCESS,
		    &hKey))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      return FALSE;
    }

  lpFolderData = &CommonShellFolders[0];
  while (lpFolderData->ValueName != NULL)
    {
      if (lpFolderData->bUserShellFolder)
	{
	  wcscpy(szBuffer, L"%ALLUSERSPROFILE%\\");
	  wcscat(szBuffer, lpFolderData->Path);

	  dwLength = (wcslen (szBuffer) + 1) * sizeof(WCHAR);
	  if (RegSetValueExW(hKey,
			     lpFolderData->ValueName,
			     0,
			     REG_EXPAND_SZ,
			     (LPBYTE)szBuffer,
			     dwLength))
	    {
	      DPRINT1("Error: %lu\n", GetLastError());
	      RegCloseKey(hKey);
	      return FALSE;
	    }
	}

      lpFolderData++;
    }

  RegCloseKey(hKey);


  DPRINT1("Success\n");

  return TRUE;
}

/* EOF */
