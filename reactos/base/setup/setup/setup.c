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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS GUI/console setup
 * FILE:            base/setup/setup/setup.c
 * PURPOSE:         Second stage setup
 * PROGRAMMER:      Eric Kohl
 */

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <tchar.h>

#define NDEBUG
#include <debug.h>

typedef DWORD (WINAPI *PINSTALL_REACTOS)(HINSTANCE hInstance);

/* FUNCTIONS ****************************************************************/

LPTSTR
lstrchr(
    LPCTSTR s,
    TCHAR c)
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


static
VOID
RunNewSetup(
    HINSTANCE hInstance)
{
    HMODULE hDll;
    PINSTALL_REACTOS InstallReactOS;

    hDll = LoadLibrary(TEXT("syssetup"));
    if (hDll == NULL)
    {
        DPRINT("Failed to load 'syssetup'!\n");
        return;
    }

    DPRINT("Loaded 'syssetup'!\n");
    InstallReactOS = (PINSTALL_REACTOS)GetProcAddress(hDll, "InstallReactOS");
    if (InstallReactOS == NULL)
    {
        DPRINT("Failed to get address for 'InstallReactOS()'!\n");
        FreeLibrary(hDll);
        return;
    }

    InstallReactOS(hInstance);

    FreeLibrary(hDll);
}


static
VOID
RunLiveCD(
    HINSTANCE hInstance)
{
    HMODULE hDll;
    PINSTALL_REACTOS InstallLiveCD;

    hDll = LoadLibrary(TEXT("syssetup"));
    if (hDll == NULL)
    {
        DPRINT("Failed to load 'syssetup'!\n");
        return;
    }

    DPRINT("Loaded 'syssetup'!\n");
    InstallLiveCD = (PINSTALL_REACTOS)GetProcAddress(hDll, "InstallLiveCD");
    if (InstallLiveCD == NULL)
    {
        DPRINT("Failed to get address for 'InstallReactOS()'!\n");
        FreeLibrary(hDll);
        return;
    }

    InstallLiveCD(hInstance);

    FreeLibrary(hDll);
}


int
WINAPI
_tWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine,
    int nShowCmd)
{
    LPTSTR CmdLine;
    LPTSTR p;

    CmdLine = GetCommandLine();

    DPRINT("CmdLine: <%s>\n",CmdLine);

    p = lstrchr(CmdLine, TEXT('-'));
    if (p == NULL)
        return 0;

    if (!lstrcmpi(p, TEXT("-newsetup")))
    {
        RunNewSetup(hInstance);
    }
    else if (!lstrcmpi(p, TEXT("-mini")))
    {
        RunLiveCD(hInstance);
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
