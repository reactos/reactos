/* $Id: setup.c,v 1.1 2004/01/09 19:52:01 ekohl Exp $ 
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/setup.c
 * PURPOSE:         Profile setup functions
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <string.h>

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


BOOL WINAPI
InitializeProfiles (VOID)
{
  WCHAR SystemRoot[MAX_PATH];
  WCHAR Path[MAX_PATH];
  LPWSTR Postfix;
  LPWSTR Ptr;
  PDIRDATA DirData;

  /* Build profile name postfix */
  if (!ExpandEnvironmentStringsW (L"%SystemRoot%",
				  SystemRoot,
				  MAX_PATH))
    return FALSE;

  SystemRoot[2] = L'.';
  Postfix = &SystemRoot[2];
  Ptr = Postfix;
  while (*Ptr != (WCHAR)0)
    {
      if (*Ptr == L'\\')
	*Ptr = '_';
      Ptr++;
    }
  _wcsupr (Postfix);

  /* Create 'Documents and Settings' directory */
  if (!ExpandEnvironmentStringsW (L"%SystemDrive%\\Documents and Settings",
				  Path,
				  MAX_PATH))
    return FALSE;

  if (!CreateDirectoryW (Path, NULL))
    return FALSE;

  /* Create 'Default User' directory */
  if (!ExpandEnvironmentStringsW (L"%SystemDrive%\\Documents and Settings\\Default User",
				  Path,
				  MAX_PATH))
    return FALSE;

  wcscat (Path, Postfix);

  if (!CreateDirectoryW (Path, NULL))
    return FALSE;

  /* Set current user profile */
  SetEnvironmentVariableW (L"USERPROFILE", Path);

  /* Create default user subdirectories */
  Ptr = AppendBackslash (Path);
  DirData = &DefaultUserDirectories[0];
  while (DirData->DirName != NULL)
    {
      wcscpy (Ptr, DirData->DirName);

      if (!CreateDirectoryW (Path, NULL))
	return FALSE;

      if (DirData->Hidden == TRUE)
	{
	  SetFileAttributesW (Path, FILE_ATTRIBUTE_HIDDEN);
	}

      DirData++;
    }

  /* Create 'All Users' directory */
  if (!ExpandEnvironmentStringsW (L"%SystemDrive%\\Documents and Settings\\All Users",
				  Path,
				  MAX_PATH))
    return FALSE;

  wcscat (Path, Postfix);

  if (!CreateDirectoryW (Path, NULL))
    return FALSE;

  /* Create all users subdirectories */
  Ptr = AppendBackslash (Path);
  DirData = &AllUsersDirectories[0];
  while (DirData->DirName != NULL)
    {
      wcscpy (Ptr, DirData->DirName);

      if (!CreateDirectoryW (Path, NULL))
	return FALSE;

      if (DirData->Hidden == TRUE)
	{
	  SetFileAttributesW (Path, FILE_ATTRIBUTE_HIDDEN);
	}

      DirData++;
    }

  return TRUE;
}
