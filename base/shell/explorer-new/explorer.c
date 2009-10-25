/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <precomp.h>

HINSTANCE hExplorerInstance;
HMODULE hUser32;
HANDLE hProcessHeap;
HKEY hkExplorer = NULL;
DRAWCAPTEMP DrawCapTemp = NULL;

/* undoc GUID */
DEFINE_GUID(CLSID_RebarBandSite, 0xECD4FC4D, 0x521C, 0x11D0, 0xB7, 0x92, 0x00, 0xA0, 0xC9, 0x03, 0x12, 0xE1);

LONG
SetWindowStyle(IN HWND hWnd,
               IN LONG dwStyleMask,
               IN LONG dwStyle)
{
    LONG PrevStyle, Style;

    ASSERT((~dwStyleMask & dwStyle) == 0);

    PrevStyle = GetWindowLongPtr(hWnd,
                                 GWL_STYLE);
    if (PrevStyle != 0 &&
        (PrevStyle & dwStyleMask) != dwStyle)
    {
        Style = PrevStyle & ~dwStyleMask;
        Style |= dwStyle;

        PrevStyle = SetWindowLongPtr(hWnd,
                                     GWL_STYLE,
                                     Style);
    }

    return PrevStyle;
}

LONG
SetWindowExStyle(IN HWND hWnd,
                 IN LONG dwStyleMask,
                 IN LONG dwStyle)
{
    LONG PrevStyle, Style;

    ASSERT((~dwStyleMask & dwStyle) == 0);

    PrevStyle = GetWindowLongPtr(hWnd,
                                 GWL_EXSTYLE);
    if (PrevStyle != 0 &&
        (PrevStyle & dwStyleMask) != dwStyle)
    {
        Style = PrevStyle & ~dwStyleMask;
        Style |= dwStyle;

        PrevStyle = SetWindowLongPtr(hWnd,
                                     GWL_EXSTYLE,
                                     Style);
    }

    return PrevStyle;
}

HMENU
LoadPopupMenu(IN HINSTANCE hInstance,
              IN LPCTSTR lpMenuName)
{
    HMENU hMenu, hSubMenu = NULL;

    hMenu = LoadMenu(hInstance,
                     lpMenuName);

    if (hMenu != NULL)
    {
        hSubMenu = GetSubMenu(hMenu,
                              0);
        if (hSubMenu != NULL &&
            !RemoveMenu(hMenu,
                        0,
                        MF_BYPOSITION))
        {
            hSubMenu = NULL;
        }

        DestroyMenu(hMenu);
    }

    return hSubMenu;
}

HMENU
FindSubMenu(IN HMENU hMenu,
            IN UINT uItem,
            IN BOOL fByPosition)
{
    MENUITEMINFO mii;

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_SUBMENU;

    if (GetMenuItemInfo(hMenu,
                        uItem,
                        fByPosition,
                        &mii))
    {
        return mii.hSubMenu;
    }

    return NULL;
}

BOOL
GetCurrentLoggedOnUserName(OUT LPTSTR szBuffer,
                           IN DWORD dwBufferSize)
{
    DWORD dwType;
    DWORD dwSize;

    /* Query the user name from the registry */
    dwSize = (dwBufferSize * sizeof(TCHAR)) - 1;
    if (RegQueryValueEx(hkExplorer,
                        TEXT("Logon User Name"),
                        0,
                        &dwType,
                        (LPBYTE)szBuffer,
                        &dwSize) == ERROR_SUCCESS &&
        (dwSize / sizeof(TCHAR)) > 1 &&
        szBuffer[0] != _T('\0'))
    {
        szBuffer[dwSize / sizeof(TCHAR)] = _T('\0');
        return TRUE;
    }

    /* Fall back to GetUserName() */
    dwSize = dwBufferSize;
    if (!GetUserName(szBuffer,
                     &dwSize))
    {
        szBuffer[0] = _T('\0');
        return FALSE;
    }

    return TRUE;
}

BOOL
FormatMenuString(IN HMENU hMenu,
                 IN UINT uPosition,
                 IN UINT uFlags,
                 ...)
{
    va_list vl;
    MENUITEMINFO mii;
    TCHAR szBuf[128];
    TCHAR szBufFmt[128];

    /* Find the menu item and read the formatting string */
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING;
    mii.dwTypeData = (LPTSTR)szBufFmt;
    mii.cch = sizeof(szBufFmt) / sizeof(szBufFmt[0]);
    if (GetMenuItemInfo(hMenu,
                        uPosition,
                        uFlags,
                        &mii))
    {
        /* Format the string */
        va_start(vl, uFlags);
        _vsntprintf(szBuf,
                    (sizeof(szBuf) / sizeof(szBuf[0])) - 1,
                    szBufFmt,
                    vl);
        va_end(vl);
        szBuf[(sizeof(szBuf) / sizeof(szBuf[0])) - 1] = _T('\0');

        /* Update the menu item */
        mii.dwTypeData = (LPTSTR)szBuf;
        if (SetMenuItemInfo(hMenu,
                            uPosition,
                            uFlags,
                            &mii))
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
GetExplorerRegValueSet(IN HKEY hKey,
                       IN LPCTSTR lpSubKey,
                       IN LPCTSTR lpValue)
{
    TCHAR szBuffer[MAX_PATH];
    HKEY hkSubKey;
    DWORD dwType, dwSize;
    BOOL Ret = FALSE;

    _tcscpy(szBuffer,
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"));
    _tcscat(szBuffer,
            _T("\\"));
    _tcscat(szBuffer,
            lpSubKey);

    dwSize = sizeof(szBuffer);
    if (RegOpenKeyEx(hKey,
                     szBuffer,
                     0,
                     KEY_QUERY_VALUE,
                     &hkSubKey) == ERROR_SUCCESS)
    {
        ZeroMemory(szBuffer,
                   sizeof(szBuffer));

        if (RegQueryValueEx(hkSubKey,
                            lpValue,
                            0,
                            &dwType,
                            (LPBYTE)szBuffer,
                            &dwSize) == ERROR_SUCCESS)
        {
            if (dwType == REG_DWORD && dwSize == sizeof(DWORD))
                Ret = *((PDWORD)szBuffer) != 0;
            else if (dwSize > 0)
                Ret = *((PUCHAR)szBuffer) != 0;
        }

        RegCloseKey(hkSubKey);
    }
    return Ret;
}


static BOOL
SetShellReadyEvent(IN LPCTSTR lpEventName)
{
    HANDLE hEvent;

    hEvent = OpenEvent(EVENT_MODIFY_STATE,
                       FALSE,
                       lpEventName);
    if (hEvent != NULL)
    {
        SetEvent(hEvent);

        CloseHandle(hEvent);
        return TRUE;
    }

    return FALSE;
}

INT WINAPI
_tWinMain(IN HINSTANCE hInstance,
          IN HINSTANCE hPrevInstance,
          IN LPTSTR lpCmdLine,
          IN INT nCmdShow)
{
    ITrayWindow *Tray = NULL;
    HANDLE hShellDesktop = NULL;
    BOOL CreateShellDesktop = FALSE;

    if (RegOpenKey(HKEY_CURRENT_USER,
                   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                   &hkExplorer) != ERROR_SUCCESS)
    {
        /* FIXME - display error */
        return 1;
    }

    hExplorerInstance = hInstance;
    hProcessHeap = GetProcessHeap();

    hUser32 = GetModuleHandle(TEXT("USER32.DLL"));
    if (hUser32 != NULL)
    {
        DrawCapTemp = (DRAWCAPTEMP)GetProcAddress(hUser32,
                                                  PROC_NAME_DRAWCAPTIONTEMP);
    }

    InitCommonControls();
    OleInitialize(NULL);

    if (GetShellWindow() == NULL)
        CreateShellDesktop = TRUE;

    /* FIXME - initialize SSO Thread */

    if (CreateShellDesktop)
    {
        if (RegisterTrayWindowClass() && RegisterTaskSwitchWndClass())
        {
            Tray = CreateTrayWindow();

            if (Tray != NULL)
                hShellDesktop = DesktopCreateWindow(Tray);
        }

        /* WinXP: Notify msgina to hide the welcome screen */
        if (!SetShellReadyEvent(TEXT("msgina: ShellReadyEvent")))
            SetShellReadyEvent(TEXT("Global\\msgina: ShellReadyEvent"));
    }
    else
    {
        /* A shell is already loaded. Parse the command line arguments
           and unless we need to do something specific simply display
           the desktop in a separate explorer window */
        /* FIXME */
    }

    if (Tray != NULL)
        TrayMessageLoop(Tray);

    if (hShellDesktop != NULL)
        DesktopDestroyShellWindow(hShellDesktop);

    /* FIXME - shutdown SSO Thread */

    OleUninitialize();

    RegCloseKey(hkExplorer);
    hkExplorer = NULL;

    return 0;
}
