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
/* $Id: setup.c,v 1.7 2004/09/30 20:23:00 ekohl Exp $ 
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/setup.c
 * PURPOSE:         Profile setup functions
 * PROGRAMMER:      Eric Kohl
 */

#include "precomp.h"


typedef struct _DIRDATA
{
  BOOL Hidden;
  LPWSTR DirName;
} DIRDATA, *PDIRDATA;

typedef struct _REGDATA
{
  LPWSTR ValueName;
  LPWSTR ValueData;
} REGDATA, *PREGDATA;



static DIRDATA
DefaultUserDirectories[] =
{
  {TRUE,  L"Application Data"},
  {FALSE, L"Desktop"},
  {FALSE, L"Favorites"},
  {FALSE, L"My Documents"},
  {TRUE,  L"PrintHood"},
  {TRUE,  L"Recent"},
  {FALSE, L"Start Menu"},
  {FALSE, L"Start Menu\\Programs"},
  {FALSE, L"Start Menu\\Programs\\Startup"},
  {TRUE,  L"Local Settings"},
  {TRUE,  L"Local Settings\\Application Data"},
  {FALSE, L"Local Settings\\Temp"},
  {FALSE, NULL}
};


static DIRDATA
AllUsersDirectories[] =
{
  {TRUE,  L"Application Data"},
  {FALSE, L"Desktop"},
  {FALSE, L"Favorites"},
  {FALSE, L"My Documents"},
  {FALSE, L"Start Menu"},
  {FALSE, L"Start Menu\\Programs"},
  {FALSE, L"Start Menu\\Programs\\Startup"},
  {FALSE, L"Start Menu\\Programs\\Administrative Tools"},
  {TRUE,  L"Templates"},
  {FALSE, NULL}
};


static REGDATA
CommonShellFolders[] =
{
  {L"Desktop", L"%ALLUSERSPROFILE%\\Desktop"},
  {L"Common AppData", L"%ALLUSERSPROFILE%\\Application Data"},
  {L"Common Programs", L"%ALLUSERSPROFILE%\\Start Menu\\Programs"},
  {L"Common Documents", L"%ALLUSERSPROFILE%\\Documents"},
  {L"Common Desktop", L"%ALLUSERSPROFILE%\\Desktop"},
  {L"Common Start Menu", L"%ALLUSERSPROFILE%\\Start Menu"},
  {L"CommonPictures", L"%ALLUSERSPROFILE%\\Documents\\My Pictures"},
  {L"CommonMusic", L"%ALLUSERSPROFILE%\\Documents\\My Music"},
  {L"CommonVideo", L"%ALLUSERSPROFILE%\\Documents\\My Videos"},
  {L"Common Favorites", L"%ALLUSERSPROFILE%\\Favorites"},
  {L"Common Startup", L"%ALLUSERSPROFILE%\\Start Menu\\Programs\\Startup"},
  {L"Common Administrative Tools", L"%ALLUSERSPROFILE%\\Start Menu\\Programs\\Administrative Tools"},
  {L"Common Templates", L"%ALLUSERSPROFILE%\\Templates"},
  {L"Personal", L"%ALLUSERSPROFILE%\\My Documents"},
  {NULL, NULL}
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
  LPWSTR lpszPtr;
  DWORD dwLength;
  PDIRDATA lpDirData;
  PREGDATA lpRegData;

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

  /* Create 'Default User' profile directory */
  wcscpy (szProfilePath, szProfilesPath);
  wcscat (szProfilePath, L"\\");
  wcscat (szProfilePath, szBuffer);
  if (!CreateDirectoryW (szProfilePath, NULL))
    {
      if (GetLastError () != ERROR_ALREADY_EXISTS)
	{
	  DPRINT1("Error: %lu\n", GetLastError());
	  RegCloseKey (hKey);
	  return FALSE;
	}
    }

  /* Set current user profile */
  SetEnvironmentVariableW (L"USERPROFILE", szProfilePath);

  /* Create 'Default User' subdirectories */
  /* FIXME: Get these paths from the registry */
  lpszPtr = AppendBackslash (szProfilePath);
  lpDirData = &DefaultUserDirectories[0];
  while (lpDirData->DirName != NULL)
    {
      wcscpy (lpszPtr, lpDirData->DirName);

      if (!CreateDirectoryW (szProfilePath, NULL))
	{
	  if (GetLastError () != ERROR_ALREADY_EXISTS)
	    {
	      DPRINT1("Error: %lu\n", GetLastError());
	      RegCloseKey (hKey);
	      return FALSE;
	    }
	}

      if (lpDirData->Hidden == TRUE)
	{
	  SetFileAttributesW (szProfilePath,
			      FILE_ATTRIBUTE_HIDDEN);
	}

      lpDirData++;
    }

  /* Set 'AllUsersProfile' value */
  wcscpy (szBuffer, L"All Users");
  if (!AppendSystemPostfix (szBuffer, MAX_PATH))
    {
      DPRINT1("AppendSystemPostfix() failed\n", GetLastError());
      RegCloseKey (hKey);
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
	  RegCloseKey (hKey);
	  return FALSE;
	}
    }

  /* Set 'All Users' profile */
  SetEnvironmentVariableW (L"ALLUSERSPROFILE", szProfilePath);

  /* Create 'All Users' subdirectories */
  /* FIXME: Take these paths from the registry */
  lpszPtr = AppendBackslash (szProfilePath);
  lpDirData = &AllUsersDirectories[0];
  while (lpDirData->DirName != NULL)
    {
      wcscpy (lpszPtr, lpDirData->DirName);

      if (!CreateDirectoryW (szProfilePath, NULL))
	{
	  if (GetLastError () != ERROR_ALREADY_EXISTS)
	    {
	      DPRINT1("Error: %lu\n", GetLastError());
	      RegCloseKey (hKey);
	      return FALSE;
	    }
	}

      if (lpDirData->Hidden == TRUE)
	{
	  SetFileAttributesW (szProfilePath,
			      FILE_ATTRIBUTE_HIDDEN);
	}

      lpDirData++;
    }

  /* Create common shell folder registry values */
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
		    0,
		    KEY_ALL_ACCESS,
		    &hKey))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      return FALSE;
    }

  lpRegData = &CommonShellFolders[0];
  while (lpRegData->ValueName != NULL)
    {
      if (!ExpandEnvironmentStringsW(lpRegData->ValueData,
				     szBuffer,
				     MAX_PATH))
	{
	  DPRINT1("Error: %lu\n", GetLastError());
	  RegCloseKey(hKey);
	  return FALSE;
	}

      dwLength = (wcslen (szBuffer) + 1) * sizeof(WCHAR);
      if (RegSetValueExW(hKey,
			 lpRegData->ValueName,
			 0,
			 REG_SZ,
			 (LPBYTE)szBuffer,
			 dwLength))
	{
	  DPRINT1("Error: %lu\n", GetLastError());
	  RegCloseKey(hKey);
	  return FALSE;
	}

      lpRegData++;
    }

  RegCloseKey(hKey);

  DPRINT1("Success\n");

  return TRUE;
}
