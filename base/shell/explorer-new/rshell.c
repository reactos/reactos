/*
 * ReactOS Explorer
 *
 * Copyright 2014 Giannis Adamopoulos
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

typedef HRESULT(*PSTARTMENU_CONSTRUCTOR)(REFIID riid, void **ppv);

HRESULT
CStartMenu_Constructor(
    REFIID riid,
    void **ppv)
{
    HINSTANCE hRShell = LoadLibraryW(L"rshell.dll");

    if (hRShell)
    {
        PSTARTMENU_CONSTRUCTOR func = (PSTARTMENU_CONSTRUCTOR)GetProcAddress(hRShell, "CStartMenu_Constructor");
        if(func)
        {
            return func(riid, ppv);
        }
    }

    return CoCreateInstance(&CLSID_StartMenu,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            riid,
                            ppv);   
}

typedef HANDLE(WINAPI *PSHCREATEDESKTOP)(IShellDesktopTray *ShellDesk);

HANDLE WINAPI SHCreateDesktop(IShellDesktopTray *ShellDesk)
{
    HINSTANCE hRShell = LoadLibraryW(L"rshell.dll");

    if (hRShell)
    {
        PSHCREATEDESKTOP func = (PSHCREATEDESKTOP)GetProcAddress(hRShell, "SHCreateDesktop");
        if (func)
        {
            return func(ShellDesk);
        }
    }

    hRShell = LoadLibraryW(L"shell32.dll");

    if (hRShell)
    {
        PSHCREATEDESKTOP func = (PSHCREATEDESKTOP)GetProcAddress(hRShell, "SHCreateDesktop");
        if (func)
        {
            return func(ShellDesk);
        }
    }

    return 0;
}

typedef BOOL(WINAPI *PSHDESKTOPMESSAGELOOP)(HANDLE hDesktop);

BOOL WINAPI SHDesktopMessageLoop(HANDLE hDesktop)
{
    HINSTANCE hRShell = LoadLibraryW(L"rshell.dll");

    if (hRShell)
    {
        PSHDESKTOPMESSAGELOOP func = (PSHDESKTOPMESSAGELOOP)GetProcAddress(hRShell, "SHDesktopMessageLoop");
        if (func)
        {
            return func(hDesktop);
        }
    }

    hRShell = LoadLibraryW(L"shell32.dll");

    if (hRShell)
    {
        PSHDESKTOPMESSAGELOOP func = (PSHDESKTOPMESSAGELOOP)GetProcAddress(hRShell, "SHDesktopMessageLoop");
        if (func)
        {
            return func(hDesktop);
        }
    }

    return FALSE;
}

typedef DWORD(WINAPI* PWINLIST_INIT)(void);

DWORD WINAPI WinList_Init(void)
{
    HINSTANCE hRShell = LoadLibraryW(L"rshell.dll");

    if (hRShell)
    {
        PWINLIST_INIT func = (PWINLIST_INIT)GetProcAddress(hRShell, "WinList_Init");
        if (func)
        {
            return func();
        }
    }

    hRShell = LoadLibraryW(L"shdocvw.dll");

    if (hRShell)
    {
        PWINLIST_INIT func = (PWINLIST_INIT)GetProcAddress(hRShell, "WinList_Init");
        if (func)
        {
            return func();
        }
    }

    return 0;
}

typedef void (WINAPI *PSHELLDDEINIT)(BOOL bInit);

void WINAPI ShellDDEInit(BOOL bInit)
{
    HINSTANCE hRShell = LoadLibraryW(L"rshell.dll");

    if (hRShell)
    {
        PSHELLDDEINIT func = (PSHELLDDEINIT)GetProcAddress(hRShell, "ShellDDEInit");
        if (func)
        {
            func(bInit);
            return;
        }
    }

    hRShell = LoadLibraryW(L"shell32.dll");

    if (hRShell)
    {
        PSHELLDDEINIT func = (PSHELLDDEINIT)GetProcAddress(hRShell, "ShellDDEInit");
        if (func)
        {
            func(bInit);
            return;
        }
    }
}