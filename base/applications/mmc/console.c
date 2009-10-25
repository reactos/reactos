/*
 * ReactOS Management Console
 * Copyright (C) 2006 - 2007 Thomas Weidenmueller
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

#include "precomp.h"

static const TCHAR szMMCMainFrame[] = TEXT("MMCMainFrame");
static const TCHAR szMMCChildFrm[] = TEXT("MMCChildFrm");

static LONG MainFrameWndCount = 0;
static ULONG NewConsoleCount = 0;

static LPTSTR
CreateNewConsoleTitle(VOID)
{
    LPTSTR lpTitle;

    if (LoadAndFormatString(hAppInstance,
                            IDS_CONSOLETITLE,
                            &lpTitle,
                            ++NewConsoleCount) == 0)
    {
        lpTitle = NULL;
    }

    return lpTitle;
}

typedef struct _CONSOLE_MAINFRAME_WND
{
    HWND hwnd;
    LPCTSTR lpConsoleTitle;
    HMENU hMenuConsoleRoot;
    union
    {
        DWORD Flags;
        struct
        {
            DWORD AppAuthorMode : 1;
        };
    };
} CONSOLE_MAINFRAME_WND, *PCONSOLE_MAINFRAME_WND;

static LRESULT CALLBACK
ConsoleMainFrameWndProc(IN HWND hwnd,
                        IN UINT uMsg,
                        IN WPARAM wParam,
                        IN LPARAM lParam)
{
    PCONSOLE_MAINFRAME_WND Info;
    LRESULT Ret = FALSE;

    Info = (PCONSOLE_MAINFRAME_WND)GetWindowLongPtr(hwnd,
                                                    0);

    if (Info != NULL || uMsg == WM_NCCREATE)
    {
        switch (uMsg)
        {
            case WM_COMMAND:
            {
                switch (LOWORD(wParam))
                {
                    case ID_FILE_EXIT:
                        PostMessage(hwnd,
                                    WM_CLOSE,
                                    0,
                                    0);
                        break;
                }
                break;
            }

            case WM_NCCREATE:
            {
                MainFrameWndCount++;

                Info = HeapAlloc(hAppHeap,
                                 0,
                                 sizeof(*Info));
                if (Info != NULL)
                {
                    ZeroMemory(Info,
                               sizeof(*Info));

                    Info->hwnd = hwnd;

                    SetWindowLongPtr(hwnd,
                                     0,
                                     (LONG_PTR)Info);

                    Info->hMenuConsoleRoot = LoadMenu(hAppInstance,
                                                      MAKEINTRESOURCE(IDM_CONSOLEROOT));
                    Ret = TRUE;
                }
                break;
            }

            case WM_CREATE:
            {
                LPCTSTR lpFileName = (LPCTSTR)(((LPCREATESTRUCT)lParam)->lpCreateParams);

                if (lpFileName != NULL)
                {
                    /* FIXME */
                }
                else
                {
                    Info->AppAuthorMode = TRUE;
                    Info->lpConsoleTitle = CreateNewConsoleTitle();
                }

                SetWindowText(Info->hwnd,
                              Info->lpConsoleTitle);
                break;
            }

            case WM_NCDESTROY:
                SetMenu(Info->hwnd,
                        NULL);

                if (Info->hMenuConsoleRoot != NULL)
                {
                    DestroyMenu(Info->hMenuConsoleRoot);
                    Info->hMenuConsoleRoot = NULL;
                }

                HeapFree(hAppHeap,
                         0,
                         Info);

                if (--MainFrameWndCount == 0)
                    PostQuitMessage(0);
                break;


            case WM_CLOSE:
                DestroyWindow(hwnd);
                break;

            default:
                goto HandleDefaultMsg;
        }
    }
    else
    {
HandleDefaultMsg:
        Ret = DefWindowProc(hwnd,
                            uMsg,
                            wParam,
                            lParam);
    }

    return Ret;
}

typedef struct _CONSOLE_CHILDFRM_WND
{
    HWND hwnd;
    PCONSOLE_MAINFRAME_WND MainFrame;
} CONSOLE_CHILDFRM_WND, *PCONSOLE_CHILDFRM_WND;

static LRESULT CALLBACK
ConsoleChildFrmProc(IN HWND hwnd,
                    IN UINT uMsg,
                    IN WPARAM wParam,
                    IN LPARAM lParam)
{
    PCONSOLE_CHILDFRM_WND Info;
    LRESULT Ret = FALSE;

    Info = (PCONSOLE_CHILDFRM_WND)GetWindowLongPtr(hwnd,
                                                   0);

    if (Info != NULL || uMsg == WM_NCCREATE)
    {
        switch (uMsg)
        {
            case WM_NCCREATE:
                Info = HeapAlloc(hAppHeap,
                                 0,
                                 sizeof(*Info));
                if (Info != NULL)
                {
                    ZeroMemory(Info,
                               sizeof(*Info));

                    Info->hwnd = hwnd;

                    SetWindowLongPtr(hwnd,
                                     0,
                                     (LONG_PTR)Info);

                    Ret = TRUE;
                }
                break;


            case WM_NCDESTROY:
                HeapFree(hAppHeap,
                         0,
                         Info);
                break;

            default:
                goto HandleDefaultMsg;
        }
    }
    else
    {
HandleDefaultMsg:
        Ret = DefWindowProc(hwnd,
                            uMsg,
                            wParam,
                            lParam);
    }

    return Ret;

}

BOOL
RegisterMMCWndClasses(VOID)
{
    WNDCLASS wc;
    BOOL Ret;

    /* Register the MMCMainFrame window class */
    wc.style = 0;
    wc.lpfnWndProc = ConsoleMainFrameWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(PCONSOLE_MAINFRAME_WND);
    wc.hInstance = hAppInstance;
    wc.hIcon = LoadIcon(hAppInstance,
                        MAKEINTRESOURCE(IDI_MAINAPP));
    wc.hCursor = LoadCursor(NULL,
                            MAKEINTRESOURCE(IDC_ARROW));
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szMMCMainFrame;

    Ret = (RegisterClass(&wc) != (ATOM)0);
    if (Ret)
    {
        /* Register the MMCChildFrm window class */
        wc.lpfnWndProc = ConsoleChildFrmProc;
        wc.cbWndExtra = sizeof(PCONSOLE_CHILDFRM_WND);
        wc.lpszClassName = szMMCChildFrm;

        Ret = (RegisterClass(&wc) != (ATOM)0);
        if (!Ret)
        {
            UnregisterClass(szMMCMainFrame,
                            hAppInstance);
        }
    }

    return Ret;
}

VOID
UnregisterMMCWndClasses(VOID)
{
    UnregisterClass(szMMCChildFrm,
                    hAppInstance);
    UnregisterClass(szMMCMainFrame,
                    hAppInstance);
}

HWND
CreateConsoleWindow(IN LPCTSTR lpFileName  OPTIONAL)
{
    HWND hWndConsole;
    LONG_PTR FileName = (LONG_PTR)lpFileName;

    hWndConsole = CreateWindowEx(WS_EX_WINDOWEDGE,
                                 szMMCMainFrame,
                                 NULL,
                                 WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 NULL,
                                 NULL,
                                 hAppInstance,
                                 (PVOID)FileName);

    if (hWndConsole != NULL)
    {
        ShowWindow(hWndConsole,
                   SW_SHOWDEFAULT);
    }

    return hWndConsole;
}
