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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

#include <winver.h>

HINSTANCE hExplorerInstance;
HMODULE hUser32;
HANDLE hProcessHeap;
HKEY hkExplorer = NULL;
DRAWCAPTEMP DrawCapTemp = NULL;

typedef struct _LANGCODEPAGE
{
    WORD wLanguage;
    WORD wCodePage;
} LANGCODEPAGE, *PLANGCODEPAGE;

LONG
SetWindowStyle(IN HWND hWnd,
               IN LONG dwStyleMask,
               IN LONG dwStyle)
{
    LONG PrevStyle, Style;

    ASSERT((~dwStyleMask & dwStyle) == 0);

    PrevStyle = GetWindowLong(hWnd,
                              GWL_STYLE);
    if (PrevStyle != 0 &&
        (PrevStyle & dwStyleMask) != dwStyle)
    {
        Style = PrevStyle & ~dwStyleMask;
        Style |= dwStyle;

        PrevStyle = SetWindowLong(hWnd,
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

    PrevStyle = GetWindowLong(hWnd,
                              GWL_EXSTYLE);
    if (PrevStyle != 0 &&
        (PrevStyle & dwStyleMask) != dwStyle)
    {
        Style = PrevStyle & ~dwStyleMask;
        Style |= dwStyle;

        PrevStyle = SetWindowLong(hWnd,
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

    StringCbCopy(szBuffer, sizeof(szBuffer),
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"));
    if (FAILED(StringCbCat(szBuffer, sizeof(szBuffer),
            _T("\\"))))
        return FALSE;
    if (FAILED(StringCbCat(szBuffer, sizeof(szBuffer),
            lpSubKey)))
    return FALSE;

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

BOOL
GetVersionInfoString(IN TCHAR *szFileName,
                     IN TCHAR *szVersionInfo,
                     OUT TCHAR *szBuffer,
                     IN UINT cbBufLen)
{
    LPVOID lpData = NULL;
    TCHAR szSubBlock[128];
    TCHAR *lpszLocalBuf = NULL;
    LANGID UserLangId;
    PLANGCODEPAGE lpTranslate = NULL;
    DWORD dwLen;
    DWORD dwHandle;
    UINT cbTranslate;
    UINT cbLen;
    BOOL bRet = FALSE;
    unsigned int i;

    dwLen = GetFileVersionInfoSize(szFileName, &dwHandle);

    if (dwLen > 0)
    {
        lpData = HeapAlloc(hProcessHeap, 0, dwLen);

        if (lpData != NULL)
        {
            if (GetFileVersionInfo(szFileName,
                                  0,
                                  dwLen,
                                  lpData) != 0)
            {
                UserLangId = GetUserDefaultLangID();

                VerQueryValue(lpData,
                    TEXT("\\VarFileInfo\\Translation"),
                    (LPVOID *)&lpTranslate,
                    &cbTranslate);

                for (i = 0; i < cbTranslate / sizeof(LANGCODEPAGE); i++)
                {
                    /* If the bottom eight bits of the language id's
                    match, use this version information (since this
                    means that the version information and the users
                    default language are the same). */
                    if ((lpTranslate[i].wLanguage & 0xFF) ==
                        (UserLangId & 0xFF))
                    {
                        wnsprintf(szSubBlock,
                            sizeof(szSubBlock) / sizeof(szSubBlock[0]),
                            TEXT("\\StringFileInfo\\%04X%04X\\%s"),
                            lpTranslate[i].wLanguage,
                            lpTranslate[i].wCodePage,
                            szVersionInfo);

                        if (VerQueryValue(lpData,
                            szSubBlock,
                            (LPVOID *)&lpszLocalBuf,
                            &cbLen) != 0)
                        {
                            _tcsncpy(szBuffer, lpszLocalBuf, cbBufLen / sizeof(*szBuffer));

                            bRet = TRUE;
                            break;
                        }
                    }
                }
            }
            HeapFree(hProcessHeap, 0, lpData);
            lpData = NULL;
        }
    }

    return bRet;
}

static VOID
HideMinimizedWindows(IN BOOL bHide)
{
    MINIMIZEDMETRICS mm;

    mm.cbSize = sizeof(mm);
    if (!SystemParametersInfo(SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, 0))
    {
        ERR("SystemParametersInfo failed with %lu\n", GetLastError());
        return;
    }
    if (bHide)
        mm.iArrange |= ARW_HIDE;
    else
        mm.iArrange &= ~ARW_HIDE;
    if (!SystemParametersInfo(SPI_SETMINIMIZEDMETRICS, sizeof(mm), &mm, 0))
        ERR("SystemParametersInfo failed with %lu\n", GetLastError());
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

    DbgPrint("Explorer starting... Commandline: %S\n", lpCmdLine);

    if (RegOpenKey(HKEY_CURRENT_USER,
                   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                   &hkExplorer) != ERROR_SUCCESS)
    {
        TCHAR Message[256];
        LoadString(hInstance, IDS_STARTUP_ERROR, Message, 256);
        MessageBox(NULL, Message, NULL, MB_ICONERROR);
        return 1;
    }

    hExplorerInstance = hInstance;
    hProcessHeap = GetProcessHeap();
    LoadAdvancedSettings();

    hUser32 = GetModuleHandle(TEXT("USER32.DLL"));
    if (hUser32 != NULL)
    {
        DrawCapTemp = (DRAWCAPTEMP)GetProcAddress(hUser32,
                                                  PROC_NAME_DRAWCAPTIONTEMP);
    }

    InitCommonControls();
    OleInitialize(NULL);

    /*
     * Set our shutdown parameters: we want to shutdown the very last,
     * but before any TaskMgr instance (which has a shutdown level of 1).
     */
    SetProcessShutdownParameters(2, 0);

    ProcessStartupItems();

    if (GetShellWindow() == NULL)
        CreateShellDesktop = TRUE;

    /* FIXME - initialize SSO Thread */

    if (CreateShellDesktop)
    {
        /* Initialize shell dde support */
        ShellDDEInit(TRUE);

        /* Initialize shell icons */
        FileIconInit(TRUE);

        /* Initialize CLSID_ShellWindows class */
        WinList_Init();

        if (RegisterTrayWindowClass() && RegisterTaskSwitchWndClass())
        {
            Tray = CreateTrayWindow();
            /* This not only hides the minimized window captions in the bottom
               left screen corner, but is also needed in order to receive
               HSHELL_* notification messages (which are required for taskbar
               buttons to work right) */
            HideMinimizedWindows(TRUE);

            if (Tray != NULL)
                hShellDesktop = DesktopCreateWindow(Tray);
        }

        /* WinXP: Notify msgina to hide the welcome screen */
        if (!SetShellReadyEvent(TEXT("msgina: ShellReadyEvent")))
            SetShellReadyEvent(TEXT("Global\\msgina: ShellReadyEvent"));
    }
    else
    {
        WCHAR root[MAX_PATH];
        HMODULE hBrowseui;
        HRESULT hr;
        LPSHELLFOLDER pDesktopFolder = NULL;
        LPITEMIDLIST pidlRoot = NULL;
        typedef HRESULT(WINAPI *SH_OPEN_NEW_FRAME)(LPITEMIDLIST pidl, IUnknown *paramC, long param10, long param14);
        SH_OPEN_NEW_FRAME SHOpenNewFrame;

        /* A shell is already loaded. Parse the command line arguments
           and unless we need to do something specific simply display
           the desktop in a separate explorer window */
        /* FIXME */

        /* Commandline switches:
         *
         * /n               Open a new window, even if an existing one still exists.
         * /e               Start with the explorer sidebar shown.
         * /root,<object>   Open a window for the given object path.
         * /select,<object> Open a window with the given object selected.
         */

        /* FIXME: Do it right */
        WCHAR* tmp = wcsstr(lpCmdLine,L"/root,");
        if (tmp)
        {
            WCHAR* tmp2;
            
            tmp += 6; // skip to beginning of path
            tmp2 = wcschr(tmp, L',');

            if (tmp2)
            {
                wcsncpy(root, tmp, tmp2 - tmp);
            }
            else
            {
                wcscpy(root, tmp);
            }
        }
        else
        {
            wcscpy(root, lpCmdLine);
        }

        if (root[0] == L'"')
        {
            int len = wcslen(root) - 2;
            wcsncpy(root, root + 1, len);
            root[len] = 0;
        }

        if (wcslen(root) > 0)
        {
            LPITEMIDLIST  pidl;
            ULONG         chEaten;
            ULONG         dwAttributes;

            if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder)))
            {
                hr = pDesktopFolder->lpVtbl->ParseDisplayName(pDesktopFolder,
                    NULL,
                    NULL,
                    root,
                    &chEaten,
                    &pidl,
                    &dwAttributes);
                if (SUCCEEDED(hr))
                {
                    pidlRoot = pidl;
                    DbgPrint("Got PIDL for folder '%S'\n", root);
                }
            }
        }

        if (!pidlRoot)
        {
            DbgPrint("No folder, getting PIDL for My Computer.\n", root);
            hr = SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlRoot);
            if (FAILED(hr))
                return 0;
        }

        DbgPrint("Trying to open browser window... \n");

        hBrowseui = LoadLibraryW(L"browseui.dll");
        if (!hBrowseui)
        {
            DbgPrint("Browseui not found.. \n");
            return 0;
        }

        SHOpenNewFrame = (SH_OPEN_NEW_FRAME) GetProcAddress(hBrowseui, (LPCSTR) 103);

        hr = SHOpenNewFrame(pidlRoot, (IUnknown*)pDesktopFolder, 0, 0);
        if (FAILED(hr))
            return 0;

        /* FIXME: we should wait a bit here and see if a window was created. If not we should exit this process. */
        Sleep(1000);
        ExitThread(0);

        return 0;
    }

    if (Tray != NULL)
    {
        RegisterHotKey(NULL, IDHK_RUN, MOD_WIN, 'R');
        TrayMessageLoop(Tray);
        HideMinimizedWindows(FALSE);
        ITrayWindow_Release(Tray);
        UnregisterTrayWindowClass();
    }

    if (hShellDesktop != NULL)
        DesktopDestroyShellWindow(hShellDesktop);

    /* FIXME - shutdown SSO Thread */

    OleUninitialize();

    RegCloseKey(hkExplorer);
    hkExplorer = NULL;

    return 0;
}
