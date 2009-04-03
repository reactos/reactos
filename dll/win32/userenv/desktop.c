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
 * FILE:            lib/userenv/desktop.c
 * PURPOSE:         Desktop and start menu support functions.
 * PROGRAMMER:      Eric Kohl
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ***************************************************************/

static BOOL
GetDesktopPath (BOOL bCommonPath,
		LPWSTR lpDesktopPath)
{
  WCHAR szPath[MAX_PATH];
  DWORD dwLength;
  DWORD dwType;
  HKEY hKey;
  LONG Error;

  DPRINT ("GetDesktopPath() called\n");

  Error = RegOpenKeyExW (HKEY_CURRENT_USER,
		         L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
		         0,
		         KEY_QUERY_VALUE,
		         &hKey);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1 ("RegOpenKeyExW() failed\n");
      SetLastError((DWORD)Error);
      return FALSE;
    }

  dwLength = MAX_PATH * sizeof(WCHAR);
  Error = RegQueryValueExW (hKey,
			    bCommonPath ? L"Common Desktop" : L"Desktop",
			    0,
			    &dwType,
			    (LPBYTE)szPath,
			   &dwLength);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1 ("RegQueryValueExW() failed\n");
      RegCloseKey (hKey);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  RegCloseKey (hKey);

  if (dwType == REG_EXPAND_SZ)
    {
      ExpandEnvironmentStringsW (szPath,
				 lpDesktopPath,
				 MAX_PATH);
    }
  else
    {
      wcscpy (lpDesktopPath, szPath);
    }

  DPRINT ("GetDesktopPath() done\n");

  return TRUE;
}


static BOOL
GetProgramsPath (BOOL bCommonPath,
		 LPWSTR lpProgramsPath)
{
  WCHAR szPath[MAX_PATH];
  DWORD dwLength;
  DWORD dwType;
  HKEY hKey;
  LONG Error;

  DPRINT ("GetProgramsPath() called\n");

  Error = RegOpenKeyExW (HKEY_CURRENT_USER,
		         L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
		         0,
		         KEY_QUERY_VALUE,
		         &hKey);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1 ("RegOpenKeyExW() failed\n");
      SetLastError((DWORD)Error);
      return FALSE;
    }

  dwLength = MAX_PATH * sizeof(WCHAR);
  Error = RegQueryValueExW (hKey,
			    bCommonPath ? L"Common Programs" : L"Programs",
			    0,
			    &dwType,
			    (LPBYTE)szPath,
			    &dwLength);
  if (Error != ERROR_SUCCESS)
    {
      DPRINT1 ("RegQueryValueExW() failed\n");
      RegCloseKey (hKey);
      SetLastError((DWORD)Error);
      return FALSE;
    }

  RegCloseKey (hKey);

  if (dwType == REG_EXPAND_SZ)
    {
      ExpandEnvironmentStringsW (szPath,
				 lpProgramsPath,
				 MAX_PATH);
    }
  else
    {
      wcscpy (lpProgramsPath,
	      szPath);
    }

  DPRINT ("GetProgramsPath() done\n");

  return TRUE;
}


BOOL WINAPI
AddDesktopItemA (BOOL bCommonItem,
		 LPCSTR lpItemName,
		 LPCSTR lpArguments,
		 LPCSTR lpIconLocation,
		 INT iIcon,
		 LPCSTR lpWorkingDirectory, /* Optional */
		 WORD wHotKey,
		 INT iShowCmd)
{
  UNICODE_STRING ItemName;
  UNICODE_STRING Arguments;
  UNICODE_STRING IconLocation;
  UNICODE_STRING WorkingDirectory;
  BOOL bResult;
  NTSTATUS Status;

  Status = RtlCreateUnicodeStringFromAsciiz(&ItemName,
					    (LPSTR)lpItemName);
  if (!NT_SUCCESS(Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  Status = RtlCreateUnicodeStringFromAsciiz(&Arguments,
					    (LPSTR)lpArguments);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&ItemName);
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  Status = RtlCreateUnicodeStringFromAsciiz(&IconLocation,
					    (LPSTR)lpIconLocation);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&Arguments);
      RtlFreeUnicodeString(&ItemName);
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  if (lpWorkingDirectory != NULL)
    {
      Status = RtlCreateUnicodeStringFromAsciiz(&WorkingDirectory,
						(LPSTR)lpWorkingDirectory);
      if (!NT_SUCCESS(Status))
	{
	  RtlFreeUnicodeString(&IconLocation);
	  RtlFreeUnicodeString(&Arguments);
	  RtlFreeUnicodeString(&ItemName);
	  SetLastError (RtlNtStatusToDosError (Status));
	  return FALSE;
	}
    }

  bResult = AddDesktopItemW(bCommonItem,
			    ItemName.Buffer,
			    Arguments.Buffer,
			    IconLocation.Buffer,
			    iIcon,
			    (lpWorkingDirectory != NULL) ? WorkingDirectory.Buffer : NULL,
			    wHotKey,
			    iShowCmd);

  if (lpWorkingDirectory != NULL)
    {
      RtlFreeUnicodeString(&WorkingDirectory);
    }

  RtlFreeUnicodeString(&IconLocation);
  RtlFreeUnicodeString(&Arguments);
  RtlFreeUnicodeString(&ItemName);

  return bResult;
}


BOOL WINAPI
AddDesktopItemW (BOOL bCommonDesktop,
		 LPCWSTR lpItemName,
		 LPCWSTR lpArguments,
		 LPCWSTR lpIconLocation,
		 INT iIcon,
		 LPCWSTR lpWorkingDirectory,  /* Optional */
		 WORD wHotKey,
		 INT iShowCmd)
{
  DYN_FUNCS Ole32;
  WCHAR szLinkPath[MAX_PATH];
  WCHAR szArguments[MAX_PATH];
  WCHAR szCommand[MAX_PATH];
  WIN32_FIND_DATAW FindData;
  HANDLE hFind;
  LPWSTR Ptr;
  DWORD dwLength;
  IShellLinkW* psl;
  IPersistFile* ppf;
  HRESULT hr;
  BOOL bResult;

  DPRINT ("AddDesktopItemW() called\n");

  bResult = FALSE;

  if (!GetDesktopPath (bCommonDesktop, szLinkPath))
    {
      DPRINT1 ("GetDesktopPath() failed\n");
      return FALSE;
    }
  DPRINT ("Desktop path: '%S'\n", szLinkPath);

  /* Make sure the path exists */
  hFind = FindFirstFileW (szLinkPath,
			  &FindData);
  if (hFind == INVALID_HANDLE_VALUE)
    {
      DPRINT ("'%S' does not exist\n", szLinkPath);

      /* Create directory path */
      if (!CreateDirectoryPath (szLinkPath, NULL))
        return FALSE;
    }
  else
    {
      DPRINT ("'%S' exists\n", szLinkPath);
      FindClose (hFind);
    }

  /* Append backslash, item name and ".lnk" extension */
  wcscat (szLinkPath, L"\\");
  wcscat (szLinkPath, lpItemName);
  wcscat (szLinkPath, L".lnk");
  DPRINT ("Link path: '%S'\n", szLinkPath);

  /* Split 'lpArguments' string into command and arguments */
  Ptr = wcschr (lpArguments, L' ');
  DPRINT ("Ptr %p  lpArguments %p\n", Ptr, lpArguments);
  if (Ptr != NULL)
    {
      dwLength = (DWORD)(Ptr - lpArguments);
      DPRINT ("dwLength %lu\n", dwLength);
      memcpy (szCommand, lpArguments, dwLength * sizeof(WCHAR));
      szCommand[dwLength] = 0;
      Ptr++;
      wcscpy (szArguments, Ptr);
    }
  else
    {
      wcscpy (szCommand, lpArguments);
      szArguments[0] = 0;
    }
  DPRINT ("szCommand: '%S'\n", szCommand);
  DPRINT ("szArguments: '%S'\n", szArguments);

  /* Dynamically load ole32.dll */
  if (!LoadDynamicImports(&DynOle32, &Ole32))
    {
      DPRINT1("USERENV: Unable to load OLE32.DLL\n");
      return FALSE;
    }

  Ole32.fn.CoInitialize(NULL);

  hr = Ole32.fn.CoCreateInstance(&CLSID_ShellLink,
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 &IID_IShellLinkW,
                                 (LPVOID*)&psl);
  if (!SUCCEEDED(hr))
    {
      Ole32.fn.CoUninitialize();
      UnloadDynamicImports(&Ole32);
      return FALSE;
    }

  hr = psl->lpVtbl->QueryInterface(psl,
                                   &IID_IPersistFile,
                                   (LPVOID*)&ppf);
  if (SUCCEEDED(hr))
    {
      psl->lpVtbl->SetDescription(psl,
                                  lpItemName);

      psl->lpVtbl->SetPath(psl,
                           szCommand);

      psl->lpVtbl->SetArguments(psl,
                                szArguments);

      psl->lpVtbl->SetIconLocation(psl,
                                   lpIconLocation,
                                   iIcon);

      if (lpWorkingDirectory != NULL)
        {
          psl->lpVtbl->SetWorkingDirectory(psl,
                                           lpWorkingDirectory);
        }
      else
        {
          psl->lpVtbl->SetWorkingDirectory(psl,
                                           L"%HOMEDRIVE%%HOMEPATH%");
        }

      psl->lpVtbl->SetHotkey(psl,
                             wHotKey);

      psl->lpVtbl->SetShowCmd(psl,
                              iShowCmd);

      hr = ppf->lpVtbl->Save(ppf,
                             szLinkPath,
                             TRUE);
      if (SUCCEEDED(hr))
        bResult = TRUE;

      ppf->lpVtbl->Release(ppf);
    }

  psl->lpVtbl->Release(psl);

  Ole32.fn.CoUninitialize();

  UnloadDynamicImports(&Ole32);

  DPRINT ("AddDesktopItemW() done\n");

  return bResult;
}


BOOL WINAPI
DeleteDesktopItemA (BOOL bCommonItem,
		    LPCSTR lpItemName)
{
  UNICODE_STRING ItemName;
  BOOL bResult;
  NTSTATUS Status;

  Status = RtlCreateUnicodeStringFromAsciiz(&ItemName,
					    (LPSTR)lpItemName);
  if (!NT_SUCCESS(Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  bResult = DeleteDesktopItemW(bCommonItem,
			       ItemName.Buffer);

  RtlFreeUnicodeString(&ItemName);

  return bResult;
}


BOOL WINAPI
DeleteDesktopItemW (BOOL bCommonItem,
		    LPCWSTR lpItemName)
{
  WCHAR szLinkPath[MAX_PATH];

  DPRINT ("DeleteDesktopItemW() called\n");

  if (!GetDesktopPath (bCommonItem, szLinkPath))
    {
      DPRINT1 ("GetDesktopPath() failed\n");
      return FALSE;
    }

  wcscat (szLinkPath, L"\\");
  wcscat (szLinkPath, lpItemName);
  wcscat (szLinkPath, L".lnk");
  DPRINT ("Link path: '%S'\n", szLinkPath);

  return DeleteFileW (szLinkPath);
}


BOOL WINAPI
CreateGroupA (LPCSTR lpGroupName,
	      BOOL bCommonGroup)
{
  UNICODE_STRING GroupName;
  BOOL bResult;
  NTSTATUS Status;

  Status = RtlCreateUnicodeStringFromAsciiz(&GroupName,
					    (LPSTR)lpGroupName);
  if (!NT_SUCCESS(Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  bResult = CreateGroupW(GroupName.Buffer, bCommonGroup);

  RtlFreeUnicodeString(&GroupName);

  return bResult;
}


BOOL WINAPI
CreateGroupW (LPCWSTR lpGroupName,
	      BOOL bCommonGroup)
{
  WCHAR szGroupPath[MAX_PATH];

  DPRINT1 ("CreateGroupW() called\n");

  if (lpGroupName == NULL || *lpGroupName == 0)
    return TRUE;

  if (!GetProgramsPath (bCommonGroup, szGroupPath))
    {
      DPRINT1 ("GetProgramsPath() failed\n");
      return FALSE;
    }
  DPRINT1 ("Programs path: '%S'\n", szGroupPath);

  wcscat (szGroupPath, L"\\");
  wcscat (szGroupPath, lpGroupName);
  DPRINT1 ("Group path: '%S'\n", szGroupPath);

  /* Create directory path */
  if (!CreateDirectoryPath (szGroupPath, NULL))
    return FALSE;

  /* FIXME: Notify the shell */

  DPRINT1 ("CreateGroupW() done\n");

  return TRUE;
}


BOOL WINAPI
DeleteGroupA (LPCSTR lpGroupName,
	      BOOL bCommonGroup)
{
  UNICODE_STRING GroupName;
  BOOL bResult;
  NTSTATUS Status;

  Status = RtlCreateUnicodeStringFromAsciiz(&GroupName,
					    (LPSTR)lpGroupName);
  if (!NT_SUCCESS(Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  bResult = DeleteGroupW(GroupName.Buffer, bCommonGroup);

  RtlFreeUnicodeString(&GroupName);

  return bResult;
}


BOOL WINAPI
DeleteGroupW (LPCWSTR lpGroupName,
	      BOOL bCommonGroup)
{
  WCHAR szGroupPath[MAX_PATH];

  DPRINT ("DeleteGroupW() called\n");

  if (lpGroupName == NULL || *lpGroupName == 0)
    return TRUE;

  if (!GetProgramsPath (bCommonGroup, szGroupPath))
    {
      DPRINT1 ("GetProgramsPath() failed\n");
      return FALSE;
    }
  DPRINT ("Programs path: '%S'\n", szGroupPath);

  wcscat (szGroupPath, L"\\");
  wcscat (szGroupPath, lpGroupName);
  DPRINT ("Group path: '%S'\n", szGroupPath);

  /* Remove directory path */
  if (!RemoveDirectoryPath (szGroupPath))
    return FALSE;

  /* FIXME: Notify the shell */

  DPRINT ("DeleteGroupW() done\n");

  return TRUE;
}


BOOL WINAPI
AddItemA (LPCSTR lpGroupName,  /* Optional */
	  BOOL bCommonGroup,
	  LPCSTR lpItemName,
	  LPCSTR lpArguments,
	  LPCSTR lpIconLocation,
	  INT iIcon,
	  LPCSTR lpWorkingDirectory,  /* Optional */
	  WORD wHotKey,
	  INT iShowCmd)
{
  UNICODE_STRING GroupName;
  UNICODE_STRING ItemName;
  UNICODE_STRING Arguments;
  UNICODE_STRING IconLocation;
  UNICODE_STRING WorkingDirectory;
  BOOL bResult;
  NTSTATUS Status;

  Status = RtlCreateUnicodeStringFromAsciiz(&ItemName,
					    (LPSTR)lpItemName);
  if (!NT_SUCCESS(Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  Status = RtlCreateUnicodeStringFromAsciiz(&Arguments,
					    (LPSTR)lpArguments);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&ItemName);
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  Status = RtlCreateUnicodeStringFromAsciiz(&IconLocation,
					    (LPSTR)lpIconLocation);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&Arguments);
      RtlFreeUnicodeString(&ItemName);
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  if (lpGroupName != NULL)
    {
      Status = RtlCreateUnicodeStringFromAsciiz(&GroupName,
						(LPSTR)lpGroupName);
      if (!NT_SUCCESS(Status))
	{
	  RtlFreeUnicodeString(&IconLocation);
	  RtlFreeUnicodeString(&Arguments);
	  RtlFreeUnicodeString(&ItemName);
	  SetLastError (RtlNtStatusToDosError (Status));
	  return FALSE;
	}
    }

  if (lpWorkingDirectory != NULL)
    {
      Status = RtlCreateUnicodeStringFromAsciiz(&WorkingDirectory,
						(LPSTR)lpWorkingDirectory);
      if (!NT_SUCCESS(Status))
	{
	  if (lpGroupName != NULL)
	    {
	      RtlFreeUnicodeString(&GroupName);
	    }
	  RtlFreeUnicodeString(&IconLocation);
	  RtlFreeUnicodeString(&Arguments);
	  RtlFreeUnicodeString(&ItemName);
	  SetLastError (RtlNtStatusToDosError (Status));
	  return FALSE;
	}
    }

  bResult = AddItemW((lpGroupName != NULL) ? GroupName.Buffer : NULL,
		     bCommonGroup,
		     ItemName.Buffer,
		     Arguments.Buffer,
		     IconLocation.Buffer,
		     iIcon,
		     (lpWorkingDirectory != NULL) ? WorkingDirectory.Buffer : NULL,
		     wHotKey,
		     iShowCmd);

  if (lpGroupName != NULL)
    {
      RtlFreeUnicodeString(&GroupName);
    }

  if (lpWorkingDirectory != NULL)
    {
      RtlFreeUnicodeString(&WorkingDirectory);
    }

  RtlFreeUnicodeString(&IconLocation);
  RtlFreeUnicodeString(&Arguments);
  RtlFreeUnicodeString(&ItemName);

  return bResult;
}


BOOL WINAPI
AddItemW (LPCWSTR lpGroupName,  /* Optional */
	  BOOL bCommonGroup,
	  LPCWSTR lpItemName,
	  LPCWSTR lpArguments,
	  LPCWSTR lpIconLocation,
	  INT iIcon,
	  LPCWSTR lpWorkingDirectory,  /* Optional */
	  WORD wHotKey,
	  INT iShowCmd)
{
  DYN_FUNCS Ole32;
  WCHAR szLinkPath[MAX_PATH];
  WCHAR szArguments[MAX_PATH];
  WCHAR szCommand[MAX_PATH];
  WIN32_FIND_DATAW FindData;
  HANDLE hFind;
  LPWSTR Ptr;
  DWORD dwLength;
  IShellLinkW* psl;
  IPersistFile* ppf;
  HRESULT hr;
  BOOL bResult;

  DPRINT ("AddItemW() called\n");

  bResult = FALSE;

  if (!GetProgramsPath (bCommonGroup, szLinkPath))
    {
      DPRINT1 ("GetProgramsPath() failed\n");
      return FALSE;
    }

  DPRINT ("Programs path: '%S'\n", szLinkPath);

  if (lpGroupName != NULL && *lpGroupName != 0)
    {
      wcscat (szLinkPath, L"\\");
      wcscat (szLinkPath, lpGroupName);

      /* Make sure the path exists */
      hFind = FindFirstFileW (szLinkPath,
			      &FindData);
      if (hFind == INVALID_HANDLE_VALUE)
	{
	  DPRINT ("'%S' does not exist\n", szLinkPath);
	  if (!CreateGroupW (lpGroupName,
			     bCommonGroup))
	    return FALSE;
	}
      else
	{
	  DPRINT ("'%S' exists\n", szLinkPath);
	  FindClose (hFind);
	}
    }

  wcscat (szLinkPath, L"\\");
  wcscat (szLinkPath, lpItemName);
  wcscat (szLinkPath, L".lnk");
  DPRINT ("Link path: '%S'\n", szLinkPath);

  /* Split 'lpArguments' string into command and arguments */
  Ptr = wcschr (lpArguments, L' ');
  DPRINT ("Ptr %p  lpArguments %p\n", Ptr, lpArguments);
  if (Ptr != NULL)
    {
      dwLength = (DWORD)(Ptr - lpArguments);
      DPRINT ("dwLength %lu\n", dwLength);
      memcpy (szCommand, lpArguments, dwLength * sizeof(WCHAR));
      szCommand[dwLength] = 0;
      Ptr++;
      wcscpy (szArguments, Ptr);
    }
  else
    {
      wcscpy (szCommand, lpArguments);
      szArguments[0] = 0;
    }
  DPRINT ("szCommand: '%S'\n", szCommand);
  DPRINT ("szArguments: '%S'\n", szArguments);

  /* Dynamically load ole32.dll */
  if (!LoadDynamicImports(&DynOle32, &Ole32))
    {
      DPRINT1("USERENV: Unable to load OLE32.DLL\n");
      return FALSE;
    }

  Ole32.fn.CoInitialize(NULL);

  hr = Ole32.fn.CoCreateInstance(&CLSID_ShellLink,
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 &IID_IShellLinkW,
                                 (LPVOID*)&psl);
  if (!SUCCEEDED(hr))
    {
      Ole32.fn.CoUninitialize();
      UnloadDynamicImports(&Ole32);
      return FALSE;
    }

  hr = psl->lpVtbl->QueryInterface(psl,
                                   &IID_IPersistFile,
                                   (LPVOID*)&ppf);
  if (SUCCEEDED(hr))
    {
      psl->lpVtbl->SetDescription(psl,
                                  lpItemName);

      psl->lpVtbl->SetPath(psl,
                           szCommand);

      psl->lpVtbl->SetArguments(psl,
                                szArguments);

      psl->lpVtbl->SetIconLocation(psl,
                                   lpIconLocation,
                                   iIcon);

      if (lpWorkingDirectory != NULL)
        {
          psl->lpVtbl->SetWorkingDirectory(psl,
                                           lpWorkingDirectory);
        }
      else
        {
          psl->lpVtbl->SetWorkingDirectory(psl,
                                           L"%HOMEDRIVE%%HOMEPATH%");
        }

      psl->lpVtbl->SetHotkey(psl,
                             wHotKey);

      psl->lpVtbl->SetShowCmd(psl,
                              iShowCmd);

      hr = ppf->lpVtbl->Save(ppf,
                             szLinkPath,
                             TRUE);
      if (SUCCEEDED(hr))
        bResult = TRUE;

      ppf->lpVtbl->Release(ppf);
    }

  psl->lpVtbl->Release(psl);

  Ole32.fn.CoUninitialize();
  UnloadDynamicImports(&Ole32);

  DPRINT ("AddItemW() done\n");

  return bResult;
}


BOOL WINAPI
DeleteItemA (LPCSTR lpGroupName, /* Optional */
	     BOOL bCommonGroup,
	     LPCSTR lpItemName,
	     BOOL bDeleteGroup)
{
  UNICODE_STRING GroupName;
  UNICODE_STRING ItemName;
  BOOL bResult;
  NTSTATUS Status;

  if (lpGroupName != NULL)
    {
      Status = RtlCreateUnicodeStringFromAsciiz(&GroupName,
						(LPSTR)lpGroupName);
      if (!NT_SUCCESS(Status))
	{
	  SetLastError (RtlNtStatusToDosError (Status));
	  return FALSE;
	}
    }

  Status = RtlCreateUnicodeStringFromAsciiz(&ItemName,
					    (LPSTR)lpItemName);
  if (!NT_SUCCESS(Status))
    {
      if (lpGroupName != NULL)
	{
	  RtlFreeUnicodeString(&GroupName);
	}

      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  bResult = DeleteItemW((lpGroupName != NULL) ? GroupName.Buffer : NULL,
			bCommonGroup,
			ItemName.Buffer,
			bDeleteGroup);

  RtlFreeUnicodeString(&ItemName);
  if (lpGroupName != NULL)
    {
      RtlFreeUnicodeString(&GroupName);
    }

  return bResult;
}


BOOL WINAPI
DeleteItemW (LPCWSTR lpGroupName, /* Optional */
	     BOOL bCommonGroup,
	     LPCWSTR lpItemName,
	     BOOL bDeleteGroup)
{
  WCHAR szItemPath[MAX_PATH];
  LPWSTR Ptr;

  DPRINT ("DeleteItemW() called\n");

  if (!GetProgramsPath (bCommonGroup, szItemPath))
    {
      DPRINT1 ("GetProgramsPath() failed\n");
      return FALSE;
    }
  DPRINT ("Programs path: '%S'\n", szItemPath);

  if (lpGroupName != NULL && *lpGroupName != 0)
    {
      wcscat (szItemPath, L"\\");
      wcscat (szItemPath, lpGroupName);
    }

  wcscat (szItemPath, L"\\");
  wcscat (szItemPath, lpItemName);
  wcscat (szItemPath, L".lnk");
  DPRINT ("Item path: '%S'\n", szItemPath);

  if (!DeleteFileW (szItemPath))
    return FALSE;

  /* FIXME: Notify the shell */

  if (bDeleteGroup)
    {
      Ptr = wcsrchr (szItemPath, L'\\');
      if (Ptr == NULL)
	return TRUE;

      *Ptr = 0;
      DPRINT ("Item path: '%S'\n", szItemPath);
      if (RemoveDirectoryW (szItemPath))
	{
	  /* FIXME: Notify the shell */
	}
    }

  DPRINT ("DeleteItemW() done\n");

  return TRUE;
}

/* EOF */
