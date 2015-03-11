/*
 *  ReactOS applications
 *  Copyright (C) 2001, 2002, 2003 ReactOS Team
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
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS welcome/autorun application
 * FILE:        subsys/system/welcome/welcome.c
 * PROGRAMMERS: Eric Kohl
 *              Casper S. Hornstrup (chorns@users.sourceforge.net)
 *
 * NOTE:
 *   This utility can be customized by modifying the resources.
 *   Please do NOT change the source code in order to customize this
 *   utility but change the resources!
 *
 * TODO: Use instead a XML file!
 */

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <shellapi.h>
#include <reactos/version.h>
#include <tchar.h>
#include <winnls.h>

#include "resource.h"

#define LIGHT_BLUE 0x00F7EFD6
#define DARK_BLUE  0x008C7B6B

#define TITLE_WIDTH  480
#define TITLE_HEIGHT  93

#define MAX_NUMBER_TOPICS   10
#define TOPIC_DESC_LENGTH   1024

/*
 * Disable this define if you want to revert back to the old behaviour, i.e.
 * opening files with CreateProcess. This defines uses ShellExecute instead.
 */
#define USE_SHELL_EXECUTE

/* GLOBALS ******************************************************************/

TCHAR szFrameClass[] = TEXT("WelcomeWindowClass");
TCHAR szAppTitle[80];

HINSTANCE hInstance;

HWND hWndMain = NULL;
HWND hWndDefaultTopic = NULL;

HDC hdcMem = NULL;

int nTopic = -1;
int nDefaultTopic = -1;

ULONG ulInnerWidth = TITLE_WIDTH;
ULONG ulInnerHeight = (TITLE_WIDTH * 3) / 4;
ULONG ulTitleHeight = TITLE_HEIGHT + 3;

HBITMAP hTitleBitmap = NULL;
HBITMAP hDefaultTopicBitmap = NULL;
HBITMAP hTopicBitmap[MAX_NUMBER_TOPICS];
HWND hWndTopicButton[MAX_NUMBER_TOPICS];
HWND hWndCloseButton = NULL;
HWND hWndCheckButton = NULL;

HFONT hFontTopicButton;
HFONT hFontTopicTitle;
HFONT hFontTopicDescription;
HFONT hFontCheckButton;

HBRUSH hbrLightBlue;
HBRUSH hbrDarkBlue;
HBRUSH hbrRightPanel;

RECT rcTitlePanel;
RECT rcLeftPanel;
RECT rcRightPanel;

WNDPROC fnOldBtn;


INT_PTR CALLBACK
MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


/* FUNCTIONS ****************************************************************/

#ifndef USE_SHELL_EXECUTE
static VOID
ShowLastWin32Error(HWND hWnd)
{
    LPTSTR lpMessageBuffer = NULL;
    DWORD dwError = GetLastError();

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      dwError,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)&lpMessageBuffer,
                      0, NULL))
    {
        MessageBox(hWnd, lpMessageBuffer, szAppTitle, MB_OK | MB_ICONERROR);
    }

    if (lpMessageBuffer)
    {
        LocalFree(lpMessageBuffer);
    }
}
#endif

int WINAPI
_tWinMain(HINSTANCE hInst,
          HINSTANCE hPrevInstance,
          LPTSTR lpszCmdLine,
          int nCmdShow)
{
    WNDCLASSEX wndclass;
    MSG msg;
    int xPos;
    int yPos;
    int xWidth;
    int yHeight;
    RECT rcWindow;
    HICON hMainIcon;
    HMENU hSystemMenu;
    DWORD dwStyle = WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                    WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    BITMAP BitmapInfo;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpszCmdLine);

    switch (GetUserDefaultUILanguage())
    {
        case MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT):
            SetProcessDefaultLayout(LAYOUT_RTL);
            break;

        default:
            break;
    }

    hInstance = hInst;

    /* Load icons */
    hMainIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));

    /* Register the window class */
    wndclass.cbSize = sizeof(WNDCLASSEX);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = (WNDPROC)MainWndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = hMainIcon;
    wndclass.hIconSm = NULL;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szFrameClass;

    RegisterClassEx(&wndclass);

    hTitleBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TITLEBITMAP));
    if (hTitleBitmap != NULL)
    {
        GetObject(hTitleBitmap, sizeof(BITMAP), &BitmapInfo);
        ulInnerWidth = BitmapInfo.bmWidth;
        ulInnerHeight = (ulInnerWidth * 3) / 4;
        ulTitleHeight = BitmapInfo.bmHeight + 3;
        DeleteObject(hTitleBitmap);
    }
    ulInnerHeight -= GetSystemMetrics(SM_CYCAPTION);

    rcWindow.top = 0;
    rcWindow.bottom = ulInnerHeight - 1;
    rcWindow.left = 0;
    rcWindow.right = ulInnerWidth - 1;

    AdjustWindowRect(&rcWindow, dwStyle, FALSE);
    xWidth = rcWindow.right - rcWindow.left;
    yHeight = rcWindow.bottom - rcWindow.top;

    xPos = (GetSystemMetrics(SM_CXSCREEN) - xWidth) / 2;
    yPos = (GetSystemMetrics(SM_CYSCREEN) - yHeight) / 2;

    rcTitlePanel.top = 0;
    rcTitlePanel.bottom = ulTitleHeight;
    rcTitlePanel.left = 0;
    rcTitlePanel.right = ulInnerWidth - 1;

    rcLeftPanel.top = rcTitlePanel.bottom;
    rcLeftPanel.bottom = ulInnerHeight - 1;
    rcLeftPanel.left = 0;
    rcLeftPanel.right = ulInnerWidth / 3;

    rcRightPanel.top = rcLeftPanel.top;
    rcRightPanel.bottom = rcLeftPanel.bottom;
    rcRightPanel.left = rcLeftPanel.right;
    rcRightPanel.right = ulInnerWidth - 1;

    if (!LoadString(hInstance, (UINT_PTR)MAKEINTRESOURCE(IDS_APPTITLE), szAppTitle, ARRAYSIZE(szAppTitle)))
        _tcscpy(szAppTitle, TEXT("ReactOS Welcome"));

    /* Create main window */
    hWndMain = CreateWindow(szFrameClass,
                            szAppTitle,
                            dwStyle,
                            xPos,
                            yPos,
                            xWidth,
                            yHeight,
                            0,
                            0,
                            hInstance,
                            NULL);

    hSystemMenu = GetSystemMenu(hWndMain, FALSE);
    if(hSystemMenu)
    {
        RemoveMenu(hSystemMenu, SC_SIZE, MF_BYCOMMAND);
        RemoveMenu(hSystemMenu, SC_MAXIMIZE, MF_BYCOMMAND);
    }

    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);

    while (GetMessage(&msg, NULL, 0, 0) != FALSE)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}


INT_PTR CALLBACK
ButtonSubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LONG i;

    if (uMsg == WM_MOUSEMOVE)
    {
        i = GetWindowLongPtr(hWnd, GWL_ID);
        if (nTopic != i)
        {
            nTopic = i;
            SetFocus(hWnd);
            InvalidateRect(hWndMain, &rcRightPanel, TRUE);
        }
    }

    return CallWindowProc(fnOldBtn, hWnd, uMsg, wParam, lParam);
}


static BOOL
RunApplication(int nTopic)
{
#ifndef USE_SHELL_EXECUTE
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;
    TCHAR CurrentDir[256];
#else
    TCHAR Parameters[2];
#endif
    TCHAR AppName[512];
    int nLength;

    InvalidateRect(hWndMain, NULL, TRUE);

#ifndef USE_SHELL_EXECUTE
    GetCurrentDirectory(ARRAYSIZE(CurrentDir), CurrentDir);
#endif

    nLength = LoadString(hInstance, IDS_TOPICACTION0 + nTopic, AppName, ARRAYSIZE(AppName));
    if (nLength == 0)
        return TRUE;

    if (!_tcsicmp(AppName, TEXT("<exit>")))
        return FALSE;

    if (!_tcsnicmp(AppName, TEXT("<msg>"), 5))
    {
        MessageBox(hWndMain, AppName + 5, TEXT("ReactOS"), MB_OK | MB_TASKMODAL);
        return TRUE;
    }

    if (_tcsicmp(AppName, TEXT("explorer.exe")) == 0)
    {
#ifndef USE_SHELL_EXECUTE
        _tcscat(AppName, TEXT(" "));
        _tcscat(AppName, CurrentDir);
#else
        _tcscpy(Parameters, TEXT("\\"));
#endif
    }
#ifdef USE_SHELL_EXECUTE
    else
    {
        *Parameters = 0;
    }
#endif

#ifndef USE_SHELL_EXECUTE
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.lpTitle = TEXT("Test");
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_SHOWNORMAL;

    if (!CreateProcess(NULL, AppName, NULL, NULL, FALSE, CREATE_NEW_CONSOLE,
                       NULL, CurrentDir, &StartupInfo, &ProcessInfo))
    {
        ShowLastWin32Error(hWndMain);
        return TRUE;
    }

    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);
#else
    ShellExecute(NULL, NULL, AppName, Parameters, NULL, SW_SHOWDEFAULT);
#endif

    return TRUE;
}


static VOID
SubclassButton(HWND hWnd)
{
    fnOldBtn = (WNDPROC)SetWindowLongPtr(hWnd, GWL_WNDPROC, (DWORD_PTR)ButtonSubclassWndProc);
}


static DWORD
GetButtonHeight(HDC hDC,
                HFONT hFont,
                LPCTSTR szText,
                DWORD dwWidth)
{
    HFONT hOldFont;
    RECT rect;

    rect.left = 0;
    rect.right = dwWidth - 20;
    rect.top = 0;
    rect.bottom = 25;

    hOldFont = (HFONT)SelectObject(hDC, hFont);
    DrawText(hDC, szText, -1, &rect, DT_TOP | DT_CALCRECT | DT_WORDBREAK);
    SelectObject(hDC, hOldFont);

    return (rect.bottom-rect.top + 14);
}


static LRESULT
OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    TCHAR szText[80];
    int i,nLength;
    HDC ScreenDC;
    DWORD dwTop;
    DWORD dwHeight = 0;

    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    hbrLightBlue = CreateSolidBrush(LIGHT_BLUE);
    hbrDarkBlue = CreateSolidBrush(DARK_BLUE);
    hbrRightPanel = CreateSolidBrush(0x00FFFFFF);

    /* Topic title font */
    hFontTopicTitle = CreateFont(-18, 0, 0, 0, FW_NORMAL,
                                 FALSE, FALSE, FALSE,
                                 ANSI_CHARSET,
                                 OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS,
                                 DEFAULT_QUALITY,
                                 FF_DONTCARE,
                                 TEXT("Arial"));

    /* Topic description font */
    hFontTopicDescription = CreateFont(-11, 0, 0, 0, FW_THIN,
                                       FALSE, FALSE, FALSE,
                                       ANSI_CHARSET,
                                       OUT_DEFAULT_PRECIS,
                                       CLIP_DEFAULT_PRECIS,
                                       DEFAULT_QUALITY,
                                       FF_DONTCARE,
                                       TEXT("Arial"));

    /* Topic button font */
    hFontTopicButton = CreateFont(-11, 0, 0, 0, FW_BOLD,
                                  FALSE, FALSE, FALSE,
                                  ANSI_CHARSET,
                                  OUT_DEFAULT_PRECIS,
                                  CLIP_DEFAULT_PRECIS,
                                  DEFAULT_QUALITY,
                                  FF_DONTCARE,
                                  TEXT("Arial"));

    /* Load title bitmap */
    if (hTitleBitmap != NULL)
        hTitleBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TITLEBITMAP));

    /* Load topic bitmaps */
    hDefaultTopicBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_DEFAULTTOPICBITMAP));
    for (i = 0; i < ARRAYSIZE(hTopicBitmap); i++)
    {
        hTopicBitmap[i] = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TOPICBITMAP0 + i));
    }

    ScreenDC = GetWindowDC(hWnd);
    hdcMem = CreateCompatibleDC(ScreenDC);
    ReleaseDC(hWnd, ScreenDC);

    /* load and create buttons */
    dwTop = rcLeftPanel.top;
    for (i = 0; i < ARRAYSIZE(hWndTopicButton); i++)
    {
        nLength = LoadString(hInstance, IDS_TOPICBUTTON0 + i, szText, ARRAYSIZE(szText));
        if (nLength > 0)
        {
            dwHeight = GetButtonHeight(hdcMem,
                                       hFontTopicButton,
                                       szText,
                                       rcLeftPanel.right - rcLeftPanel.left);

            hWndTopicButton[i] = CreateWindow(TEXT("BUTTON"),
                                              szText,
                                              WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | BS_MULTILINE | BS_OWNERDRAW,
                                              rcLeftPanel.left,
                                              dwTop,
                                              rcLeftPanel.right - rcLeftPanel.left,
                                              dwHeight,
                                              hWnd,
                                              (HMENU)IntToPtr(i),
                                              hInstance,
                                              NULL);
            hWndDefaultTopic = hWndTopicButton[i];
            nDefaultTopic = i;
            SubclassButton(hWndTopicButton[i]);
            SendMessage(hWndTopicButton[i], WM_SETFONT, (WPARAM)hFontTopicButton, MAKELPARAM(TRUE, 0));
        }
        else
        {
            hWndTopicButton[i] = NULL;
        }

        dwTop += dwHeight;
    }

    /* Create exit button */
    nLength = LoadString(hInstance, IDS_CLOSETEXT, szText, ARRAYSIZE(szText));
    if (nLength > 0)
    {
        hWndCloseButton = CreateWindow(TEXT("BUTTON"),
                                       szText,
                                       WS_VISIBLE | WS_CHILD | BS_FLAT,
                                       rcRightPanel.right - 10 - 57,
                                       rcRightPanel.bottom - 10 - 21,
                                       57,
                                       21,
                                       hWnd,
                                       (HMENU)IDC_CLOSEBUTTON,
                                       hInstance,
                                       NULL);
        hWndDefaultTopic = NULL;
        nDefaultTopic = -1;
        SendMessage(hWndCloseButton, WM_SETFONT, (WPARAM)hFontTopicButton, MAKELPARAM(TRUE, 0));
    }
    else
    {
        hWndCloseButton = NULL;
    }

    /* Create checkbox */
    nLength = LoadString(hInstance, IDS_CHECKTEXT, szText, ARRAYSIZE(szText));
    if (nLength > 0)
    {
        hFontCheckButton = CreateFont(-10, 0, 0, 0, FW_THIN,
                                      FALSE, FALSE, FALSE,
                                      ANSI_CHARSET,
                                      OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS,
                                      DEFAULT_QUALITY,
                                      FF_DONTCARE,
                                      TEXT("Tahoma"));

        hWndCheckButton = CreateWindow(TEXT("BUTTON"),
                                       szText,
                                       WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                                       rcLeftPanel.left + 8,
                                       rcLeftPanel.bottom - 8 - 13,
                                       rcLeftPanel.right - rcLeftPanel.left - 16,
                                       13,
                                       hWnd,
                                       (HMENU)IDC_CHECKBUTTON,
                                       hInstance,
                                       NULL);
        SendMessage(hWndCheckButton, WM_SETFONT, (WPARAM)hFontCheckButton, MAKELPARAM(TRUE, 0));
    }
    else
    {
        hWndCheckButton = NULL;
        hFontCheckButton = NULL;
    }

    return 0;
}


static LRESULT
OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    if (LOWORD(wParam) == IDC_CLOSEBUTTON)
    {
        DestroyWindow(hWnd);
    }
    else if ((LOWORD(wParam) < MAX_NUMBER_TOPICS))
    {
        if (RunApplication(LOWORD(wParam)) == FALSE)
            DestroyWindow(hWnd);
    }
    return 0;
}


static VOID
PaintBanner(HDC hdc, LPRECT rcPanel)
{
    HBITMAP hOldBitmap;
    HBRUSH hOldBrush;

    /* Title bitmap */
    hOldBitmap = (HBITMAP)SelectObject(hdcMem, hTitleBitmap);
    BitBlt(hdc,
           rcPanel->left,
           rcPanel->top,
           rcPanel->right - rcPanel->left,
           rcPanel->bottom - 3,
           hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOldBitmap);

    /* Dark blue line */
    hOldBrush = (HBRUSH)SelectObject(hdc, hbrDarkBlue);
    PatBlt(hdc,
           rcPanel->left,
           rcPanel->bottom - 3,
           rcPanel->right - rcPanel->left,
           3,
           PATCOPY);

    SelectObject(hdc, hOldBrush);
}


static LRESULT
OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HPEN hPen;
    HPEN hOldPen;
    HDC hdc;
    PAINTSTRUCT ps;
    HBITMAP hOldBitmap = NULL;
    HBRUSH hOldBrush;
    HFONT hOldFont;
    RECT rcTitle, rcDescription;
    TCHAR szTopicTitle[80];
    TCHAR szTopicDesc[TOPIC_DESC_LENGTH];
    int nLength;
    BITMAP bmpInfo;
    TCHAR version[50];

    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    hdc = BeginPaint(hWnd, &ps);

    /* Banner panel */
    PaintBanner(hdc, &rcTitlePanel);

    /* Left panel */
    hOldBrush = (HBRUSH)SelectObject(hdc, hbrLightBlue);
    PatBlt(hdc,
           rcLeftPanel.left,
           rcLeftPanel.top,
           rcLeftPanel.right - rcLeftPanel.left,
           rcLeftPanel.bottom - rcLeftPanel.top,
           PATCOPY);
    SelectObject(hdc, hOldBrush);

    /* Right panel */
    hOldBrush = (HBRUSH)SelectObject(hdc, WHITE_BRUSH);
    PatBlt(hdc,
           rcRightPanel.left,
           rcRightPanel.top,
           rcRightPanel.right - rcRightPanel.left,
           rcRightPanel.bottom - rcRightPanel.top,
           PATCOPY);
    SelectObject(hdc, hOldBrush);

    /* Draw dark verical line */
    hPen = CreatePen(PS_SOLID, 0, DARK_BLUE);
    hOldPen = (HPEN)SelectObject(hdc, hPen);
    MoveToEx(hdc, rcRightPanel.left, rcRightPanel.top, NULL);
    LineTo(hdc, rcRightPanel.left, rcRightPanel.bottom);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    /* Draw topic bitmap */
    if ((nTopic == -1) && (hDefaultTopicBitmap != NULL))
    {
        GetObject(hDefaultTopicBitmap, sizeof(BITMAP), &bmpInfo);
        hOldBitmap = (HBITMAP)SelectObject(hdcMem, hDefaultTopicBitmap);
        BitBlt(hdc,
               rcRightPanel.right - bmpInfo.bmWidth,
               rcRightPanel.bottom - bmpInfo.bmHeight,
               bmpInfo.bmWidth,
               bmpInfo.bmHeight,
               hdcMem,
               0,
               0,
               SRCCOPY);
    }
    else if ((nTopic != -1) && (hTopicBitmap[nTopic] != NULL))
    {
        GetObject(hTopicBitmap[nTopic], sizeof(BITMAP), &bmpInfo);
        hOldBitmap = (HBITMAP)SelectObject(hdcMem, hTopicBitmap[nTopic]);
        BitBlt(hdc,
               rcRightPanel.right - bmpInfo.bmWidth,
               rcRightPanel.bottom - bmpInfo.bmHeight,
               bmpInfo.bmWidth,
               bmpInfo.bmHeight,
               hdcMem,
               0,
               0,
               SRCCOPY);
    }

    if (nTopic == -1)
    {
        nLength = LoadString(hInstance, IDS_DEFAULTTOPICTITLE, szTopicTitle, ARRAYSIZE(szTopicTitle));
    }
    else
    {
        nLength = LoadString(hInstance, IDS_TOPICTITLE0 + nTopic, szTopicTitle, ARRAYSIZE(szTopicTitle));
        if (nLength == 0)
            nLength = LoadString(hInstance, IDS_DEFAULTTOPICTITLE, szTopicTitle, ARRAYSIZE(szTopicTitle));
    }

    if (nTopic == -1)
    {
        nLength = LoadString(hInstance, IDS_DEFAULTTOPICDESC, szTopicDesc, ARRAYSIZE(szTopicDesc));
    }
    else
    {
        nLength = LoadString(hInstance, IDS_TOPICDESC0 + nTopic, szTopicDesc, ARRAYSIZE(szTopicDesc));
        if (nLength == 0)
            nLength = LoadString(hInstance, IDS_DEFAULTTOPICDESC, szTopicDesc, ARRAYSIZE(szTopicDesc));
    }

    SetBkMode(hdc, TRANSPARENT);

    /* Draw version information */
    _stprintf(version, TEXT("ReactOS %d.%d.%d"),
              KERNEL_VERSION_MAJOR,
              KERNEL_VERSION_MINOR,
              KERNEL_VERSION_PATCH_LEVEL);

    rcTitle.left = rcLeftPanel.left + 8;
    rcTitle.right = rcLeftPanel.right - 5;
    rcTitle.top = rcLeftPanel.bottom - 40;
    rcTitle.bottom = rcLeftPanel.bottom - 5;
    hOldFont = (HFONT)SelectObject(hdc, hFontTopicDescription);
    DrawText(hdc, version, -1, &rcTitle, DT_BOTTOM | DT_CALCRECT | DT_SINGLELINE);
    DrawText(hdc, version, -1, &rcTitle, DT_BOTTOM | DT_SINGLELINE);
    SelectObject(hdc, hOldFont);

    /* Draw topic title */
    rcTitle.left = rcRightPanel.left + 12;
    rcTitle.right = rcRightPanel.right - 8;
    rcTitle.top = rcRightPanel.top + 8;
    rcTitle.bottom = rcTitle.top + 57;
    hOldFont = (HFONT)SelectObject(hdc, hFontTopicTitle);
    DrawText(hdc, szTopicTitle, -1, &rcTitle, DT_TOP | DT_CALCRECT);

    SetTextColor(hdc, DARK_BLUE);
    DrawText(hdc, szTopicTitle, -1, &rcTitle, DT_TOP);

    /* Draw topic description */
    rcDescription.left = rcRightPanel.left + 12;
    rcDescription.right = rcRightPanel.right - 8;
    rcDescription.top = rcTitle.bottom + 8;
    rcDescription.bottom = rcRightPanel.bottom - 20;

    SelectObject(hdc, hFontTopicDescription);
    SetTextColor(hdc, 0x00000000);
    DrawText(hdc, szTopicDesc, -1, &rcDescription, DT_TOP | DT_WORDBREAK);

    SetBkMode(hdc, OPAQUE);
    SelectObject(hdc, hOldFont);

    SelectObject(hdcMem, hOldBrush);
    SelectObject(hdcMem, hOldBitmap);

    EndPaint(hWnd, &ps);

    return 0;
}


static LRESULT
OnDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT)lParam;
    HPEN hPen, hOldPen;
    HBRUSH hOldBrush;
    TCHAR szText[80];
    int iBkMode;

    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(wParam);

    if (lpDis->hwndItem == hWndCloseButton)
    {
        DrawFrameControl(lpDis->hDC,
                         &lpDis->rcItem,
                         DFC_BUTTON,
                         DFCS_BUTTONPUSH | DFCS_FLAT);
    }
    else
    {
        if (lpDis->CtlID == (ULONG)nTopic)
            hOldBrush = (HBRUSH)SelectObject(lpDis->hDC, hbrRightPanel);
        else
            hOldBrush = (HBRUSH)SelectObject(lpDis->hDC, hbrLightBlue);

        PatBlt(lpDis->hDC,
               lpDis->rcItem.left,
               lpDis->rcItem.top,
               lpDis->rcItem.right,
               lpDis->rcItem.bottom,
               PATCOPY);
        SelectObject(lpDis->hDC, hOldBrush);

        hPen = CreatePen(PS_SOLID, 0, DARK_BLUE);
        hOldPen = (HPEN)SelectObject(lpDis->hDC, hPen);
        MoveToEx(lpDis->hDC, lpDis->rcItem.left, lpDis->rcItem.bottom - 1, NULL);
        LineTo(lpDis->hDC, lpDis->rcItem.right, lpDis->rcItem.bottom - 1);
        SelectObject(lpDis->hDC, hOldPen);
        DeleteObject(hPen);

        InflateRect(&lpDis->rcItem, -10, -4);
        OffsetRect(&lpDis->rcItem, 0, 1);
        GetWindowText(lpDis->hwndItem, szText, ARRAYSIZE(szText));
        SetTextColor(lpDis->hDC, 0x00000000);
        iBkMode = SetBkMode(lpDis->hDC, TRANSPARENT);
        DrawText(lpDis->hDC, szText, -1, &lpDis->rcItem, DT_TOP | DT_LEFT | DT_WORDBREAK);
        SetBkMode(lpDis->hDC, iBkMode);
    }

    return 0;
}


static LRESULT
OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    if (nTopic != -1)
    {
        nTopic = -1;
        SetFocus(hWnd);
        InvalidateRect(hWndMain, &rcRightPanel, TRUE);
    }

    return 0;
}


static LRESULT
OnCtlColorStatic(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);

    if ((HWND)lParam == hWndCheckButton)
    {
        SetBkColor((HDC)wParam, LIGHT_BLUE);
        return (LRESULT)hbrLightBlue;
    }

    return 0;
}


static LRESULT
OnActivate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    nTopic = -1;
    InvalidateRect(hWndMain, &rcRightPanel, TRUE);

    return 0;
}


static LRESULT
OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    int i;

    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    for (i = 0; i < ARRAYSIZE(hWndTopicButton); i++)
    {
        if (hWndTopicButton[i] != NULL)
            DestroyWindow(hWndTopicButton[i]);
    }

    if (hWndCloseButton != NULL)
        DestroyWindow(hWndCloseButton);

    if (hWndCheckButton != NULL)
        DestroyWindow(hWndCheckButton);

    DeleteDC(hdcMem);

    /* Delete bitmaps */
    DeleteObject(hDefaultTopicBitmap);
    DeleteObject(hTitleBitmap);
    for (i = 0; i < ARRAYSIZE(hTopicBitmap); i++)
    {
        if (hTopicBitmap[i] != NULL)
            DeleteObject(hTopicBitmap[i]);
    }

    DeleteObject(hFontTopicTitle);
    DeleteObject(hFontTopicDescription);
    DeleteObject(hFontTopicButton);

    if (hFontCheckButton != NULL)
        DeleteObject(hFontCheckButton);

    DeleteObject(hbrLightBlue);
    DeleteObject(hbrDarkBlue);
    DeleteObject(hbrRightPanel);

    return 0;
}


INT_PTR CALLBACK
MainWndProc(HWND hWnd,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            return OnCreate(hWnd, wParam, lParam);

        case WM_COMMAND:
            return OnCommand(hWnd, wParam, lParam);

        case WM_ACTIVATE:
            return OnActivate(hWnd, wParam, lParam);

        case WM_PAINT:
            return OnPaint(hWnd, wParam, lParam);

        case WM_DRAWITEM:
            return OnDrawItem(hWnd, wParam, lParam);

        case WM_CTLCOLORSTATIC:
            return OnCtlColorStatic(hWnd, wParam, lParam);

        case WM_MOUSEMOVE:
            return OnMouseMove(hWnd, wParam, lParam);

        case WM_DESTROY:
            OnDestroy(hWnd, wParam, lParam);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/* EOF */
