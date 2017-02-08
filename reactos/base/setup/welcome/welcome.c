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
 * FILE:        base/setup/welcome/welcome.c
 * PROGRAMMERS: Eric Kohl
 *              Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Hermes Belusca-Maito
 *
 * NOTE:
 *   This utility can be customized by using localized INI configuration files.
 *   The default strings are stored in the utility's resources.
 */

#include <stdarg.h>
#include <tchar.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winnls.h>
#include <winuser.h>
#include <shellapi.h>
#include <strsafe.h>

#include <reactos/version.h>

#include "resource.h"

#define LIGHT_BLUE RGB(214, 239, 247)
#define DARK_BLUE  RGB(107, 123, 140)

#define TITLE_WIDTH  480
#define TITLE_HEIGHT  93

/* GLOBALS ******************************************************************/

TCHAR szFrameClass[] = TEXT("WelcomeWindowClass");
TCHAR szAppTitle[80];

HINSTANCE hInstance;

HWND hWndMain = NULL;
HWND hWndDefaultTopic = NULL;

HDC hdcMem = NULL;

ULONG ulInnerWidth = TITLE_WIDTH;
ULONG ulInnerHeight = (TITLE_WIDTH * 3) / 4;
ULONG ulTitleHeight = TITLE_HEIGHT + 3;

HBITMAP hTitleBitmap = NULL;
HBITMAP hDefaultTopicBitmap = NULL;
HWND hWndCloseButton = NULL;
HWND hWndCheckButton = NULL;

BOOL bDisplayCheckBox = FALSE; // FIXME: We should also repaint the OS version correctly!
BOOL bDisplayExitBtn  = TRUE;

#define BUFFER_SIZE 1024

#define TOPIC_TITLE_LENGTH  80
#define TOPIC_DESC_LENGTH   1024

typedef struct _TOPIC
{
    HBITMAP hBitmap;
    HWND hWndButton;
    TCHAR szText[80];
    TCHAR szTitle[TOPIC_TITLE_LENGTH];
    TCHAR szDesc[TOPIC_DESC_LENGTH];
    TCHAR szAction[512];
} TOPIC, *PTOPIC;

DWORD dwNumberTopics = 0;
PTOPIC* pTopics = NULL;

TCHAR szDefaultTitle[TOPIC_TITLE_LENGTH];
TCHAR szDefaultDesc[TOPIC_DESC_LENGTH];

INT nTopic = -1;        // Active (focused) topic
INT nDefaultTopic = -1; // Default selected topic

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

INT GetLocaleName(IN LCID Locale, OUT LPTSTR lpLCData, IN SIZE_T cchData)
{
    INT ret, ret2;

    /* Try to retrieve the locale language name (LOCALE_SNAME is supported on Vista+) */
    ret = GetLocaleInfo(Locale, LOCALE_SNAME, lpLCData, cchData);
    if (ret || (GetLastError() != ERROR_INVALID_FLAGS))
        return ret;

    /*
     * We failed because LOCALE_SNAME was unrecognized, so try to manually build
     * a language name in the form xx-YY (WARNING: this method has its limitations).
     */
    ret = GetLocaleInfo(Locale, LOCALE_SISO639LANGNAME, lpLCData, cchData);
    if (ret <= 1)
        return ret;

    lpLCData += (ret - 1);
    cchData -= (ret - 1);
    if (cchData <= 1)
        return ret;

    /* Try to get the second part; we add the '-' separator only if we succeed */
    ret2 = GetLocaleInfo(Locale, LOCALE_SISO3166CTRYNAME, lpLCData + 1, cchData - 1);
    if (ret2 <= 1)
        return ret;
    ret += ret2; // 'ret' already counts '-'.

    *lpLCData = _T('-');

    return ret;
}

VOID TranslateEscapes(IN OUT LPTSTR lpString)
{
    LPTSTR pEscape = NULL; // Next backslash escape sequence.

    while (lpString && *lpString)
    {
        /* Find the next backslash escape sequence */
        pEscape = _tcschr(lpString, _T('\\'));
        if (!pEscape)
            break;

        /* Go past the escape backslash */
        lpString = pEscape + 1;

        /* Find which sequence it is */
        switch (*lpString)
        {
            case _T('\0'):
                // *pEscape = _T('\0'); // Enable if one wants to convert \<NULL> into <NULL>.
                // lpString = pEscape + 1; // Loop will stop at the next iteration.
                break;

            case _T('n'): case _T('r'):
            // case _T('\\'): // others?
            // So far we only need to deal with the newlines.
            {
                if (*lpString == _T('n'))
                    *pEscape = _T('\n');
                else if (*lpString == _T('r'))
                    *pEscape = _T('\r');

                memmove(lpString, lpString + 1, (_tcslen(lpString + 1) + 1) * sizeof(TCHAR));
                break;
            }

            /* Unknown escape sequence, ignore it */
            default:
                lpString++;
                break;
        }
    }
}

static BOOL
LoadLocalizedResourcesFromINI(LCID Locale, LPTSTR lpResPath)
{
    DWORD dwRet;
    DWORD dwSize;
    TCHAR szBuffer[LOCALE_NAME_MAX_LENGTH];
    TCHAR szIniPath[MAX_PATH];
    LPTSTR lpszSections = NULL, lpszSection = NULL;
    PTOPIC pTopic, *pTopicsTmp;

    /* Retrieve the locale name (on which the INI file name is based) */
    dwRet = (DWORD)GetLocaleName(Locale, szBuffer, ARRAYSIZE(szBuffer));
    if (!dwRet)
    {
        /* Fall back to english (US) */
        StringCchCopy(szBuffer, ARRAYSIZE(szBuffer), TEXT("en-US"));
    }

    /* Build the INI file name */
    StringCchCopy(szIniPath, ARRAYSIZE(szIniPath), lpResPath);
    StringCchCat(szIniPath, ARRAYSIZE(szIniPath), TEXT("\\"));
    StringCchCat(szIniPath, ARRAYSIZE(szIniPath), szBuffer);
    StringCchCat(szIniPath, ARRAYSIZE(szIniPath), TEXT(".ini"));

    /* Verify that the file exists, otherwise fall back to english (US) */
    if (GetFileAttributes(szIniPath) == INVALID_FILE_ATTRIBUTES)
    {
        StringCchCopy(szBuffer, ARRAYSIZE(szBuffer), TEXT("en-US"));

        StringCchCopy(szIniPath, ARRAYSIZE(szIniPath), lpResPath);
        StringCchCat(szIniPath, ARRAYSIZE(szIniPath), TEXT("\\"));
        StringCchCat(szIniPath, ARRAYSIZE(szIniPath), szBuffer);
        StringCchCat(szIniPath, ARRAYSIZE(szIniPath), TEXT(".ini"));
    }

    /* Verify that the file exists, otherwise fall back to internal (localized) resource */
    if (GetFileAttributes(szIniPath) == INVALID_FILE_ATTRIBUTES)
        return FALSE; // TODO: For localized resource, see the general function.

    /* Try to load the default localized strings */
    GetPrivateProfileString(TEXT("Defaults"), TEXT("AppTitle"), TEXT("ReactOS Welcome") /* default */,
                            szAppTitle, ARRAYSIZE(szAppTitle), szIniPath);
    if (!GetPrivateProfileString(TEXT("Defaults"), TEXT("DefaultTopicTitle"), NULL /* default */,
                                 szDefaultTitle, ARRAYSIZE(szDefaultTitle), szIniPath))
    {
        *szDefaultTitle = 0;
    }
    if (!GetPrivateProfileString(TEXT("Defaults"), TEXT("DefaultTopicDescription"), NULL /* default */,
                                 szDefaultDesc, ARRAYSIZE(szDefaultDesc), szIniPath))
    {
        *szDefaultDesc = 0;
    }
    else
    {
        TranslateEscapes(szDefaultDesc);
    }

    /* Allocate a buffer big enough to hold all the section names */
    for (dwSize = BUFFER_SIZE; ; dwSize += BUFFER_SIZE)
    {
        lpszSections = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(TCHAR));
        if (!lpszSections)
            return TRUE; // FIXME!
        dwRet = GetPrivateProfileSectionNames(lpszSections, dwSize, szIniPath);
        if (dwRet < dwSize - 2)
            break;
        HeapFree(GetProcessHeap(), 0, lpszSections);
    }

    dwNumberTopics = 0;
    pTopics = NULL;

    /* Loop over the sections and load the topics */
    lpszSection = lpszSections;
    for (; lpszSection && *lpszSection; lpszSection += (_tcslen(lpszSection) + 1))
    {
        if (_tcsnicmp(lpszSection, TEXT("Topic"), 5) == 0)
        {
            /* Allocate (or reallocate) the list of topics */
            if (!pTopics)
                pTopicsTmp = HeapAlloc(GetProcessHeap(), 0, (dwNumberTopics + 1) * sizeof(*pTopics));
            else
                pTopicsTmp = HeapReAlloc(GetProcessHeap(), 0, pTopics, (dwNumberTopics + 1) * sizeof(*pTopics));
            if (!pTopicsTmp)
                break; // Cannot reallocate more
            pTopics = pTopicsTmp;

            /* Allocate a new topic */
            pTopic = HeapAlloc(GetProcessHeap(), 0, sizeof(*pTopic));
            if (!pTopic)
                break; // Cannot reallocate more
            pTopics[dwNumberTopics++] = pTopic;

            /* Retrieve the information */
            if (!GetPrivateProfileString(lpszSection, TEXT("Button"), NULL /* default */,
                                         pTopic->szText, ARRAYSIZE(pTopic->szText), szIniPath))
            {
                *pTopic->szText = 0;
            }
            if (!GetPrivateProfileString(lpszSection, TEXT("Title"), NULL /* default */,
                                         pTopic->szTitle, ARRAYSIZE(pTopic->szTitle), szIniPath))
            {
                *pTopic->szTitle = 0;
            }
            if (!GetPrivateProfileString(lpszSection, TEXT("Description"), NULL /* default */,
                                         pTopic->szDesc, ARRAYSIZE(pTopic->szDesc), szIniPath))
            {
                *pTopic->szDesc = 0;
            }
            else
            {
                TranslateEscapes(pTopic->szDesc);
            }
            if (!GetPrivateProfileString(lpszSection, TEXT("Action"), NULL /* default */,
                                         pTopic->szAction, ARRAYSIZE(pTopic->szAction), szIniPath))
            {
                *pTopic->szAction = 0;
            }
            else
            {
                TranslateEscapes(pTopic->szAction);
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, lpszSections);

    return TRUE;
}

static BOOL
LoadLocalizedResources(LPTSTR lpResPath)
{
#define MAX_NUMBER_INTERNAL_TOPICS  3

    UINT i;
    PTOPIC pTopic, *pTopicsTmp;

    dwNumberTopics = 0;
    pTopics = NULL;

    /*
     * First, try to load the default internal (localized) strings.
     * They can be redefined by the localized INI files.
     */
    if (!LoadString(hInstance, IDS_APPTITLE, szAppTitle, ARRAYSIZE(szAppTitle)))
        StringCchCopy(szAppTitle, ARRAYSIZE(szAppTitle), TEXT("ReactOS Welcome"));
    if (!LoadString(hInstance, IDS_DEFAULTTOPICTITLE, szDefaultTitle, ARRAYSIZE(szDefaultTitle)))
        *szDefaultTitle = 0;
    if (!LoadString(hInstance, IDS_DEFAULTTOPICDESC, szDefaultDesc, ARRAYSIZE(szDefaultDesc)))
        *szDefaultDesc = 0;

    /* Try to load the resources from INI file */
    if (*lpResPath && LoadLocalizedResourcesFromINI(LOCALE_USER_DEFAULT, lpResPath))
        return TRUE;

    /* We failed, fall back to internal (localized) resource */
    for (i = 0; i < MAX_NUMBER_INTERNAL_TOPICS; ++i)
    {
        /* Allocate (or reallocate) the list of topics */
        if (!pTopics)
            pTopicsTmp = HeapAlloc(GetProcessHeap(), 0, (dwNumberTopics + 1) * sizeof(*pTopics));
        else
            pTopicsTmp = HeapReAlloc(GetProcessHeap(), 0, pTopics, (dwNumberTopics + 1) * sizeof(*pTopics));
        if (!pTopicsTmp)
            break; // Cannot reallocate more
        pTopics = pTopicsTmp;

        /* Allocate a new topic */
        pTopic = HeapAlloc(GetProcessHeap(), 0, sizeof(*pTopic));
        if (!pTopic)
            break; // Cannot reallocate more
        pTopics[dwNumberTopics++] = pTopic;

        /* Retrieve the information */
        if (!LoadString(hInstance, IDS_TOPICBUTTON0 + i, pTopic->szText, ARRAYSIZE(pTopic->szText)))
            *pTopic->szText = 0;
        if (!LoadString(hInstance, IDS_TOPICTITLE0 + i, pTopic->szTitle, ARRAYSIZE(pTopic->szTitle)))
            *pTopic->szTitle = 0;
        if (!LoadString(hInstance, IDS_TOPICDESC0 + i, pTopic->szDesc, ARRAYSIZE(pTopic->szDesc)))
            *pTopic->szDesc = 0;
        if (!LoadString(hInstance, IDS_TOPICACTION0 + i, pTopic->szAction, ARRAYSIZE(pTopic->szAction)))
            *pTopic->szAction = 0;
    }

    return TRUE;
}

static VOID
FreeResources(VOID)
{
    if (!pTopics)
        return;

    while (dwNumberTopics--)
    {
        if (pTopics[dwNumberTopics])
            HeapFree(GetProcessHeap(), 0, pTopics[dwNumberTopics]);
    }
    HeapFree(GetProcessHeap(), 0, pTopics);
    pTopics = NULL;
    dwNumberTopics = 0;
}

static BOOL
LoadConfiguration(VOID)
{
    TCHAR szAppPath[MAX_PATH];
    TCHAR szIniPath[MAX_PATH];
    TCHAR szResPath[MAX_PATH];

    /* Retrieve the full path to this application */
    GetModuleFileName(NULL, szAppPath, ARRAYSIZE(szAppPath));
    if (*szAppPath)
    {
        LPTSTR lpFileName = _tcsrchr(szAppPath, _T('\\'));
        if (lpFileName)
            *lpFileName = 0;
        else
            *szAppPath = 0;
    }

    /* Build the full INI file path name */
    StringCchCopy(szIniPath, ARRAYSIZE(szIniPath), szAppPath);
    StringCchCat(szIniPath, ARRAYSIZE(szIniPath), TEXT("\\welcome.ini"));

    /* Verify that the file exists, otherwise use the default configuration */
    if (GetFileAttributes(szIniPath) == INVALID_FILE_ATTRIBUTES)
    {
        /* Use the default configuration and retrieve the default resources */
        return LoadLocalizedResources(TEXT(""));
    }

    /* Load the settings from the INI configuration file */
    bDisplayCheckBox = !!GetPrivateProfileInt(TEXT("Welcome"), TEXT("DisplayCheckBox"),  FALSE /* default */, szIniPath);
    bDisplayExitBtn  = !!GetPrivateProfileInt(TEXT("Welcome"), TEXT("DisplayExitButton"), TRUE /* default */, szIniPath);

    if (!GetPrivateProfileString(TEXT("Welcome"), TEXT("ResourceDir"), NULL /* default */,
                                 szResPath, ARRAYSIZE(szResPath), szIniPath))
    {
        *szResPath = 0;
    }

    /* Set the current directory to the one of this application, and retrieve the resources */
    SetCurrentDirectory(szAppPath);
    return LoadLocalizedResources(szResPath);
}

#if 0
static VOID
ShowLastWin32Error(HWND hWnd)
{
    LPTSTR lpMessageBuffer = NULL;
    DWORD dwError = GetLastError();

    if (dwError == ERROR_SUCCESS)
        return;

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dwError,
                       LANG_USER_DEFAULT,
                       (LPTSTR)&lpMessageBuffer,
                       0, NULL))
    {
        return;
    }

    MessageBox(hWnd, lpMessageBuffer, szAppTitle, MB_OK | MB_ICONERROR);
    LocalFree(lpMessageBuffer);
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
    INT xPos, yPos;
    INT xWidth, yHeight;
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

    /* Load the configuration and the resources */
    LoadConfiguration();

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
    if (hSystemMenu)
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

    /* Cleanup */
    FreeResources();

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
RunAction(INT nTopic)
{
    // TCHAR CurrentDir[MAX_PATH];
    TCHAR Parameters[2];
    TCHAR AppName[MAX_PATH];

    InvalidateRect(hWndMain, NULL, TRUE);

    if (nTopic < 0)
        return TRUE;

    // GetCurrentDirectory(ARRAYSIZE(CurrentDir), CurrentDir);

    StringCchCopy(AppName, ARRAYSIZE(AppName), pTopics[nTopic]->szAction);
    if (!*AppName)
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
        // StringCchCat(AppName, ARRAYSIZE(AppName), TEXT(" "));
        // StringCchCat(AppName, ARRAYSIZE(AppName), CurrentDir);
        _tcscpy(Parameters, TEXT("\\"));
    }
    else
    {
        *Parameters = 0;
    }

    ShellExecute(NULL, NULL, AppName, Parameters, NULL, SW_SHOWDEFAULT);

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
    UINT i;
    INT nLength;
    HDC ScreenDC;
    DWORD dwTop;
    DWORD dwHeight = 0;

    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    hbrLightBlue = CreateSolidBrush(LIGHT_BLUE);
    hbrDarkBlue = CreateSolidBrush(DARK_BLUE);
    hbrRightPanel = CreateSolidBrush(RGB(255, 255, 255));

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
    for (i = 0; i < dwNumberTopics; i++)
    {
        // FIXME: Not implemented yet!
        // pTopics[i]->hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TOPICBITMAP0 + i));
        pTopics[i]->hBitmap = NULL;
    }

    ScreenDC = GetWindowDC(hWnd);
    hdcMem = CreateCompatibleDC(ScreenDC);
    ReleaseDC(hWnd, ScreenDC);

    /* Load and create buttons */
    dwTop = rcLeftPanel.top;
    for (i = 0; i < dwNumberTopics; i++)
    {
        if (*pTopics[i]->szText)
        {
            dwHeight = GetButtonHeight(hdcMem,
                                       hFontTopicButton,
                                       pTopics[i]->szText,
                                       rcLeftPanel.right - rcLeftPanel.left);

            pTopics[i]->hWndButton = CreateWindow(TEXT("BUTTON"),
                                                  pTopics[i]->szText,
                                                  WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | BS_MULTILINE | BS_OWNERDRAW,
                                                  rcLeftPanel.left,
                                                  dwTop,
                                                  rcLeftPanel.right - rcLeftPanel.left,
                                                  dwHeight,
                                                  hWnd,
                                                  (HMENU)IntToPtr(i),
                                                  hInstance,
                                                  NULL);
            hWndDefaultTopic = pTopics[i]->hWndButton;
            nDefaultTopic = i;
            SubclassButton(pTopics[i]->hWndButton);
            SendMessage(pTopics[i]->hWndButton, WM_SETFONT, (WPARAM)hFontTopicButton, MAKELPARAM(TRUE, 0));
        }
        else
        {
            pTopics[i]->hWndButton = NULL;
        }

        dwTop += dwHeight;
    }

    /* Create "Exit" button */
    if (bDisplayExitBtn)
    {
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
    }

    /* Create checkbox */
    if (bDisplayCheckBox)
    {
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
                                           WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX /**/| BS_FLAT/**/,
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
            hFontCheckButton = NULL;
            hWndCheckButton = NULL;
        }
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
    else if ((LOWORD(wParam) < dwNumberTopics))
    {
        if (RunAction(LOWORD(wParam)) == FALSE)
            DestroyWindow(hWnd); // Corresponds to a <exit> action.
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
    BITMAP bmpInfo;
    TCHAR version[50];
    LPTSTR lpTitle = NULL, lpDesc = NULL;

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

    /* Draw dark vertical line */
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
    else if ((nTopic != -1) && (pTopics[nTopic]->hBitmap != NULL))
    {
        GetObject(pTopics[nTopic]->hBitmap, sizeof(BITMAP), &bmpInfo);
        hOldBitmap = (HBITMAP)SelectObject(hdcMem, pTopics[nTopic]->hBitmap);
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
        lpTitle = szDefaultTitle;
        lpDesc  = szDefaultDesc;
    }
    else
    {
        lpTitle = pTopics[nTopic]->szTitle;
        lpDesc  = pTopics[nTopic]->szDesc;
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
    DrawText(hdc, lpTitle, -1, &rcTitle, DT_TOP | DT_CALCRECT);

    SetTextColor(hdc, DARK_BLUE);
    DrawText(hdc, lpTitle, -1, &rcTitle, DT_TOP);

    /* Draw topic description */
    rcDescription.left = rcRightPanel.left + 12;
    rcDescription.right = rcRightPanel.right - 8;
    rcDescription.top = rcTitle.bottom + 8;
    rcDescription.bottom = rcRightPanel.bottom - 20;

    SelectObject(hdc, hFontTopicDescription);
    SetTextColor(hdc, RGB(0, 0, 0));
    DrawText(hdc, lpDesc, -1, &rcDescription, DT_TOP | DT_WORDBREAK);

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
    INT iBkMode;
    TCHAR szText[80];

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
        SetTextColor(lpDis->hDC, RGB(0, 0, 0));
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
    UINT i;

    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    for (i = 0; i < dwNumberTopics; i++)
    {
        if (pTopics[i]->hWndButton != NULL)
            DestroyWindow(pTopics[i]->hWndButton);
    }

    if (hWndCloseButton != NULL)
        DestroyWindow(hWndCloseButton);

    if (hWndCheckButton != NULL)
        DestroyWindow(hWndCheckButton);

    DeleteDC(hdcMem);

    /* Delete bitmaps */
    DeleteObject(hDefaultTopicBitmap);
    DeleteObject(hTitleBitmap);
    for (i = 0; i < dwNumberTopics; i++)
    {
        if (pTopics[i]->hBitmap != NULL)
            DeleteObject(pTopics[i]->hBitmap);
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
