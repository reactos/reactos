/* $Id: setup.c,v 1.2 2004/01/13 12:34:09 ekohl Exp $ 
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/setup.c
 * PURPOSE:         Profile setup functions
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <userenv.h>

#include "internal.h"

typedef struct _DIRDATA
{
  BOOL Hidden;
  LPWSTR DirName;
} DIRDATA, *PDIRDATA;


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


static LPWSTR
AppendBackslash(LPWSTR String)
{
  ULONG Length;

  Length = lstrlenW (String);
  if (String[Length - 1] != L'\\')
    {
      String[Length] = L'\\';
      Length++;
      String[Length] = (WCHAR)0;
    }

  return &String[Length];
}


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
  WCHAR szSystemRoot[MAX_PATH];
  WCHAR szBuffer[MAX_PATH];
  LPWSTR lpszPostfix;
  LPWSTR lpszPtr;
  DWORD dwLength;
  PDIRDATA lpDirData;

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

  /* Build profile name postfix */
  if (!ExpandEnvironmentStringsW (L"%SystemRoot%",
				  szSystemRoot,
				  MAX_PATH))
    {
      DPRINT1("Error: %lu\n", GetLastError());
      RegCloseKey (hKey);
      return FALSE;
    }

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
  _wcsupr (lpszPostfix);

  /* Set 'DefaultUserProfile' value */
  wcscpy (szBuffer, L"Default User");
  wcscat (szBuffer, lpszPostfix);

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
  wcscat (szBuffer, lpszPostfix);

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

  RegCloseKey (hKey);

  DPRINT1("Success\n");

  return TRUE;
}
