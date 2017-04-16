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
#include <browseui_undoc.h>

HINSTANCE hExplorerInstance;
HANDLE hProcessHeap;
HKEY hkExplorer = NULL;

class CExplorerModule : public CComModule
{
public:
};

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

CExplorerModule                             gModule;
CAtlWinModule                               gWinModule;

void *operator new (size_t, void *buf)
{
    return buf;
}

static VOID InitializeAtlModule(HINSTANCE hInstance, BOOL bInitialize)
{
    if (bInitialize)
    {
        gModule.Init(ObjectMap, hInstance, NULL);
    }
    else
    {
        gModule.Term();
    }
}

#if !WIN7_DEBUG_MODE
static BOOL
SetShellReadyEvent(IN LPCWSTR lpEventName)
{
    HANDLE hEvent;

    hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, lpEventName);
    if (hEvent != NULL)
    {
        SetEvent(hEvent);

        CloseHandle(hEvent);
        return TRUE;
    }

    return FALSE;
}

static VOID
HideMinimizedWindows(IN BOOL bHide)
{
    MINIMIZEDMETRICS mm;

    mm.cbSize = sizeof(mm);
    if (!SystemParametersInfoW(SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, 0))
    {
        ERR("SystemParametersInfoW failed with %lu\n", GetLastError());
        return;
    }
    if (bHide)
        mm.iArrange |= ARW_HIDE;
    else
        mm.iArrange &= ~ARW_HIDE;
    if (!SystemParametersInfoW(SPI_SETMINIMIZEDMETRICS, sizeof(mm), &mm, 0))
        ERR("SystemParametersInfoW failed with %lu\n", GetLastError());
}
#endif

#if !WIN7_COMPAT_MODE
static INT 
StartWithCommandLine(IN HINSTANCE hInstance)
{
    BOOL b = FALSE;
    EXPLORER_CMDLINE_PARSE_RESULTS parseResults = { 0 };

    if (SHExplorerParseCmdLine(&parseResults))
        b = SHCreateFromDesktop(&parseResults);

    if (parseResults.strPath)
        SHFree(parseResults.strPath);

    if (parseResults.pidlPath)
        ILFree(parseResults.pidlPath);

    if (parseResults.pidlRoot)
        ILFree(parseResults.pidlRoot);

    return b;
}
#endif

static INT 
StartWithDesktop(IN HINSTANCE hInstance)
{
    InitializeAtlModule(hInstance, TRUE);

    if (RegOpenKeyW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
        &hkExplorer) != ERROR_SUCCESS)
    {
        WCHAR Message[256];
        LoadStringW(hInstance, IDS_STARTUP_ERROR, Message, _countof(Message));
        MessageBox(NULL, Message, NULL, MB_ICONERROR);
        return 1;
    }

    hExplorerInstance = hInstance;
    hProcessHeap = GetProcessHeap();

    LoadTaskBarSettings();
    InitCommonControls();
    OleInitialize(NULL);

#if !WIN7_DEBUG_MODE
    ProcessStartupItems();
#endif

#if !WIN7_COMPAT_MODE
    /* Initialize shell dde support */
    _ShellDDEInit(TRUE);
#endif

    /* Initialize shell icons */
    FileIconInit(TRUE);

    /* Initialize CLSID_ShellWindows class */
    _WinList_Init();

    CComPtr<ITrayWindow> Tray;
    CreateTrayWindow(&Tray);

#if !WIN7_DEBUG_MODE
    /* This not only hides the minimized window captions in the bottom
    left screen corner, but is also needed in order to receive
    HSHELL_* notification messages (which are required for taskbar
    buttons to work right) */
    HideMinimizedWindows(TRUE);

    HANDLE hShellDesktop = NULL;
    if (Tray != NULL)
        hShellDesktop = DesktopCreateWindow(Tray);

    /* WinXP: Notify msgina to hide the welcome screen */
    if (!SetShellReadyEvent(L"msgina: ShellReadyEvent"))
        SetShellReadyEvent(L"Global\\msgina: ShellReadyEvent");
#endif

    if (Tray != NULL)
    {
        TrayMessageLoop(Tray);
#if !WIN7_DEBUG_MODE
        HideMinimizedWindows(FALSE);
#endif
    }

#if !WIN7_DEBUG_MODE
    if (hShellDesktop != NULL)
        DesktopDestroyShellWindow(hShellDesktop);
#endif

    OleUninitialize();

    RegCloseKey(hkExplorer);
    hkExplorer = NULL;

    InitializeAtlModule(hInstance, FALSE);

    return 0;
}

INT WINAPI
_tWinMain(IN HINSTANCE hInstance,
          IN HINSTANCE hPrevInstance,
          IN LPTSTR lpCmdLine,
          IN INT nCmdShow)
{
    /*
    * Set our shutdown parameters: we want to shutdown the very last,
    * but before any TaskMgr instance (which has a shutdown level of 1).
    */
    SetProcessShutdownParameters(2, 0);

    InitRSHELL();

#if !WIN7_COMPAT_MODE
    BOOL CreateShellDesktop = FALSE;

    TRACE("Explorer starting... Commandline: %S\n", lpCmdLine);

    if (GetShellWindow() == NULL)
        CreateShellDesktop = TRUE;

    if (!CreateShellDesktop)
    {
        return StartWithCommandLine(hInstance);
    }
#endif

    return StartWithDesktop(hInstance);
}
