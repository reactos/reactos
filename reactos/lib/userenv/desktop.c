/* $Id: desktop.c,v 1.5 2004/05/05 15:29:15 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/desktop.c
 * PURPOSE:         Desktop and start menu support functions.
 * PROGRAMMER:      Eric Kohl
 */

#include <ntos.h>
#include <windows.h>
#include <userenv.h>
#include <tchar.h>
#include <shlobj.h>

#include "internal.h"


/* FUNCTIONS ***************************************************************/

static BOOL
GetDesktopPath (BOOL bCommonPath,
		LPWSTR lpDesktopPath)
{
  WCHAR szPath[MAX_PATH];
  DWORD dwLength;
  DWORD dwType;
  HKEY hKey;

  DPRINT ("GetDesktopPath() called\n");

  if (RegOpenKeyExW (HKEY_CURRENT_USER,
		     L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
		     0,
		     KEY_ALL_ACCESS,
		     &hKey))
    {
      DPRINT1 ("RegOpenKeyExW() failed\n");
      return FALSE;
    }

  dwLength = MAX_PATH * sizeof(WCHAR);
  if (RegQueryValueExW (hKey,
			bCommonPath ? L"Common Desktop" : L"Desktop",
			0,
			&dwType,
			(LPBYTE)szPath,
			&dwLength))
    {
      DPRINT1 ("RegQueryValueExW() failed\n");
      RegCloseKey (hKey);
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

  DPRINT ("GetProgramsPath() called\n");

  if (RegOpenKeyExW (HKEY_CURRENT_USER,
		     L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
		     0,
		     KEY_ALL_ACCESS,
		     &hKey))
    {
      DPRINT1 ("RegOpenKeyExW() failed\n");
      return FALSE;
    }

  dwLength = MAX_PATH * sizeof(WCHAR);
  if (RegQueryValueExW (hKey,
			bCommonPath ? L"Common Programs" : L"Programs",
			0,
			&dwType,
			(LPBYTE)szPath,
			&dwLength))
    {
      DPRINT1 ("RegQueryValueExW() failed\n");
      RegCloseKey (hKey);
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
		 LPCSTR lpWorkingDirectory,
		 WORD wHotKey,
		 INT iShowCmd)
{
  DPRINT1 ("AddDesktopItemA() not implemented!\n");
  return FALSE;
}


BOOL WINAPI
AddDesktopItemW (BOOL bCommonDesktop,
		 LPCWSTR lpItemName,
		 LPCWSTR lpArguments,
		 LPCWSTR lpIconLocation,
		 INT iIcon,
		 LPCWSTR lpWorkingDirectory,
		 WORD wHotKey,
		 INT iShowCmd)
{
  WCHAR szLinkPath[MAX_PATH];
  WCHAR szArguments[MAX_PATH];
  WCHAR szCommand[MAX_PATH];
  WIN32_FIND_DATA FindData;
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
      DPRINT1 ("'%S' does not exist\n", szLinkPath);

      /* FIXME: create directory path */
      if (!CreateDirectoryW (szLinkPath, NULL))
        return FALSE;
    }
  else
    {
      DPRINT1 ("'%S' exists\n", szLinkPath);
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

  CoInitialize(NULL);

  hr = CoCreateInstance(&CLSID_ShellLink,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        &IID_IShellLinkW,
                       (LPVOID*)&psl);
  if (!SUCCEEDED(hr))
    {
      CoUninitialize();
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

  CoUninitialize();

  DPRINT ("AddDesktopItemW() done\n");

  return bResult;
}


BOOL WINAPI
DeleteDesktopItemA (BOOL bCommonItem,
		    LPCSTR lpItemName)
{
  DPRINT1 ("DeleteDesktopItemA() not implemented!\n");
  return FALSE;
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

  return DeleteFile (szLinkPath);
}


BOOL WINAPI
CreateGroupA (LPCSTR lpGroupName,
	      BOOL bCommonGroup)
{
  DPRINT1 ("CreateGroupA() not implemented!\n");
  return FALSE;
}


BOOL WINAPI
CreateGroupW (LPCWSTR lpGroupName,
	      BOOL bCommonGroup)
{
  WCHAR szGroupPath[MAX_PATH];

  DPRINT ("CreateGroupW() called\n");

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

  /* FIXME: Create directory path */
  if (!CreateDirectoryW (szGroupPath, NULL))
    return FALSE;

  /* FIXME: Notify the shell */

  DPRINT ("CreateGroupW() done\n");

  return TRUE;
}


BOOL WINAPI
DeleteGroupA (LPCSTR lpGroupName,
	      BOOL bCommonGroup)
{
  DPRINT1 ("DeleteGroupA() not implemented!\n");
  return FALSE;
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

  /* FIXME: Remove directory path */
  if (!RemoveDirectoryW (szGroupPath))
    return FALSE;

  /* FIXME: Notify the shell */

  DPRINT ("DeleteGroupW() done\n");

  return TRUE;
}


BOOL WINAPI
AddItemA (LPCSTR lpGroupName,
	  BOOL bCommonGroup,
	  LPCSTR lpItemName,
	  LPCSTR lpArguments,
	  LPCSTR lpIconLocation,
	  INT iIcon,
	  LPCSTR lpWorkingDirectory,
	  WORD wHotKey,
	  INT iShowCmd)
{
  DPRINT1 ("AddItemA() not implemented!\n");
  return FALSE;
}


BOOL WINAPI
AddItemW (LPCWSTR lpGroupName,
	  BOOL bCommonGroup,
	  LPCWSTR lpItemName,
	  LPCWSTR lpArguments,
	  LPCWSTR lpIconLocation,
	  INT iIcon,
	  LPCWSTR lpWorkingDirectory,
	  WORD wHotKey,
	  INT iShowCmd)
{
  WCHAR szLinkPath[MAX_PATH];
  WCHAR szArguments[MAX_PATH];
  WCHAR szCommand[MAX_PATH];
  WIN32_FIND_DATA FindData;
  HANDLE hFind;
  LPWSTR Ptr;
  DWORD dwLength;
  IShellLinkW* psl;
  IPersistFile* ppf;
  HRESULT hr;
  BOOL bResult;

  DPRINT ("AddDesktopItemW() called\n");

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

  CoInitialize(NULL);

  hr = CoCreateInstance(&CLSID_ShellLink,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        &IID_IShellLinkW,
                       (LPVOID*)&psl);
  if (!SUCCEEDED(hr))
    {
      CoUninitialize();
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

  CoUninitialize();

  DPRINT ("AddDesktopItemW() done\n");

  return bResult;
}


BOOL WINAPI
DeleteItemA (LPCSTR lpGroupName,
	     BOOL bCommonGroup,
	     LPCSTR lpItemName,
	     BOOL bDeleteGroup)
{
  DPRINT1 ("DeleteItemA() not implemented!\n");
  return FALSE;
}


BOOL WINAPI
DeleteItemW (LPCWSTR lpGroupName,
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
