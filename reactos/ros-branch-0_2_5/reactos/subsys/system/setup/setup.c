/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS GUI/console setup
 * FILE:            subsys/system/setup/setup.c
 * PURPOSE:         Second stage setup
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <tchar.h>

#include <syssetup.h>

#include <shlobj.h>
#include <objidl.h>
#include <shlwapi.h>

#define DEBUG

typedef DWORD STDCALL (*PINSTALL_REACTOS)(HINSTANCE hInstance);


/* FUNCTIONS ****************************************************************/


LPTSTR lstrchr(LPCTSTR s, TCHAR c)
{
  while (*s)
    {
      if (*s == c)
	return (LPTSTR)s;
      s++;
    }

  if (c == (TCHAR)0)
    return (LPTSTR)s;

  return (LPTSTR)NULL;
}


HRESULT CreateShellLink(LPCSTR linkPath, LPCSTR cmd, LPCSTR arg, LPCSTR dir, LPCSTR iconPath, int icon_nr, LPCSTR comment)
{
  IShellLinkA* psl;
  IPersistFile* ppf;
  WCHAR buffer[MAX_PATH];

  HRESULT hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, (LPVOID*)&psl);

  if (SUCCEEDED(hr))
    {
      hr = psl->lpVtbl->SetPath(psl, cmd);

      if (arg)
        {
          hr = psl->lpVtbl->SetArguments(psl, arg);
        }

      if (dir)
        {
          hr = psl->lpVtbl->SetWorkingDirectory(psl, dir);
        }

      if (iconPath)
        {
          hr = psl->lpVtbl->SetIconLocation(psl, iconPath, icon_nr);
        }

      if (comment)
        {
          hr = psl->lpVtbl->SetDescription(psl, comment);
        }

      hr = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf);

      if (SUCCEEDED(hr))
        {
          MultiByteToWideChar(CP_ACP, 0, linkPath, -1, buffer, MAX_PATH);

          hr = ppf->lpVtbl->Save(ppf, buffer, TRUE);

          ppf->lpVtbl->Release(ppf);
        }

      psl->lpVtbl->Release(psl);
    }

  return hr;
}


static VOID
CreateCmdLink()
{
  char path[MAX_PATH];
  LPSTR p;

  CoInitialize(NULL);

  SHGetSpecialFolderPathA(0, path, CSIDL_DESKTOP, TRUE);
  p = PathAddBackslashA(path);

  strcpy(p, "Command Prompt.lnk");
  CreateShellLink(path, "cmd.exe", "", NULL, NULL, 0, "Open command prompt");

  CoUninitialize();
}


static VOID
RunNewSetup (HINSTANCE hInstance)
{
  HMODULE hDll;
  PINSTALL_REACTOS InstallReactOS;

  hDll = LoadLibrary (TEXT("syssetup"));
  if (hDll == NULL)
    {
#ifdef DEBUG
      OutputDebugString (TEXT("Failed to load 'syssetup'!\n"));
#endif
      return;
    }

#ifdef DEBUG
  OutputDebugString (TEXT("Loaded 'syssetup'!\n"));
#endif

  InstallReactOS = (PINSTALL_REACTOS)GetProcAddress (hDll, "InstallReactOS");
  if (InstallReactOS == NULL)
    {
#ifdef DEBUG
      OutputDebugString (TEXT("Failed to get address for 'InstallReactOS()'!\n"));
#endif
      FreeLibrary (hDll);
      return;
    }

  InstallReactOS (hInstance);

  FreeLibrary (hDll);

  CreateCmdLink();
}


int STDCALL
WinMain (HINSTANCE hInstance,
	 HINSTANCE hPrevInstance,
	 LPSTR lpCmdLine,
	 int nShowCmd)
{
  LPTSTR CmdLine;
  LPTSTR p;

  CmdLine = GetCommandLine ();

#ifdef DEBUG
  OutputDebugString (TEXT("CmdLine: <"));
  OutputDebugString (CmdLine);
  OutputDebugString (TEXT(">\n"));
#endif

  p = lstrchr (CmdLine, TEXT('-'));
  if (p == NULL)
    return 0;

  if (!lstrcmpi (p, TEXT("-newsetup")))
    {
      RunNewSetup (hInstance);
    }

#if 0
  /* Add new setup types here */
  else if (...)
    {

    }
#endif

  return 0;
}

/* EOF */
