/* $Id: desktop.c,v 1.1 2004/04/29 14:41:26 ekohl Exp $
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
AddDesktopItemW (BOOL bCommonItem,
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
  LPWSTR Ptr;
  DWORD dwLength;
  IShellLinkW* psl;
  IPersistFile* ppf;
  HRESULT hr;
  BOOL bResult;

  DPRINT ("AddDesktopItemW() called\n");

  bResult = FALSE;

  if (!GetDesktopPath (bCommonItem, szLinkPath))
    {
      DPRINT1 ("GetDesktopPath() failed\n");
      return FALSE;
    }

  DPRINT ("Desktop path: '%S'\n", szLinkPath);

  /* FIXME: Make sure the path exists */

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

/* EOF */
