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
 * PROJECT:     ReactOS "Welcome"/AutoRun application
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

#include <reactos/buildno.h>

#include "resource.h"

#define LIGHT_BLUE RGB(214, 239, 247)
#define DARK_BLUE  RGB(107, 123, 140)

#define TITLE_WIDTH  480
#define TITLE_HEIGHT  93

/* GLOBALS ******************************************************************/

TCHAR szWindowClass[] = TEXT("WelcomeWindowClass");
TCHAR szAppTitle[80];

HINSTANCE hInstance;

HWND hWndMain = NULL;

HWND hWndCheckButton = NULL;
HWND hWndCloseButton = NULL;

BOOL bDisplayCheckBox = FALSE;
BOOL bDisplayExitBtn  = TRUE;

#define BUFFER_SIZE 1024

#define TOPIC_TITLE_LENGTH  80
#define TOPIC_DESC_LENGTH   1024

typedef struct _TOPIC
{
    HBITMAP hBitmap;
    HWND hWndButton;

    /*
     * TRUE : szCommand contains a command (e.g. executable to run);
     * FALSE: szCommand contains a custom "Welcome"/AutoRun action.
     */
    BOOL bIsCommand;

    TCHAR szText[80];
    TCHAR szTitle[TOPIC_TITLE_LENGTH];
    TCHAR szDesc[TOPIC_DESC_LENGTH];
    TCHAR szCommand[512];
    TCHAR szArgs[512];
} TOPIC, *PTOPIC;

DWORD dwNumberTopics = 0;
PTOPIC* pTopics = NULL;

WNDPROC fnOldBtn;

TCHAR szDefaultTitle[TOPIC_TITLE_LENGTH];
TCHAR szDefaultDesc[TOPIC_DESC_LENGTH];

#define TOPIC_BTN_ID_BASE   100

INT nTopic = -1;        // Active (focused) topic
INT nDefaultTopic = -1; // Default selected topic

HDC hdcMem = NULL;
HBITMAP hTitleBitmap = NULL;
HBITMAP hDefaultTopicBitmap = NULL;

HFONT hFontTopicButton;
HFONT hFontTopicTitle;
HFONT hFontTopicDescription;
HFONT hFontCheckButton;

HBRUSH hbrLightBlue;
HBRUSH hbrDarkBlue;

RECT rcTitlePanel;
RECT rcLeftPanel;
RECT rcRightPanel;


INT_PTR CALLBACK
MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


/* FUNCTIONS ****************************************************************/

INT GetLocaleName(IN LCID Locale, OUT LPTSTR lpLCData, IN SIZE_T cchData)
{
    INT cchRet, cchRet2;

    /* Try to retrieve the locale language name (LOCALE_SNAME is supported on Vista+) */
    cchRet = GetLocaleInfo(Locale, LOCALE_SNAME, lpLCData, cchData);
    if (cchRet || (GetLastError() != ERROR_INVALID_FLAGS))
        return cchRet;

    /*
     * We failed because LOCALE_SNAME was unrecognized, so try to manually build
     * a language name in the form xx-YY (WARNING: this method has its limitations).
     */
    cchRet = GetLocaleInfo(Locale, LOCALE_SISO639LANGNAME, lpLCData, cchData);
    if (cchRet <= 1)
        return cchRet;

    lpLCData += (cchRet - 1);
    cchData -= (cchRet - 1);
    if (cchData <= 1)
        return cchRet;

    /* Try to get the second part; we add the '-' separator only if we succeed */
    cchRet2 = GetLocaleInfo(Locale, LOCALE_SISO3166CTRYNAME, lpLCData + 1, cchData - 1);
    if (cchRet2 <= 1)
        return cchRet;
    cchRet += cchRet2; // 'cchRet' already counts '-'.

    *lpLCData = _T('-');

    return cchRet;
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

            /* New-line and carriage return */
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

            /* \xhhhh hexadecimal character specification */
            case _T('x'):
            {
                LPTSTR lpStringNew;
                *pEscape = (WCHAR)_tcstoul(lpString + 1, &lpStringNew, 16);
                memmove(lpString, lpStringNew, (_tcslen(lpStringNew) + 1) * sizeof(TCHAR));
                break;
            }

            /* Unknown escape sequence, ignore it */
            default:
                lpString++;
                break;
        }
    }
}

/*
 * Expands the path for the ReactOS Installer "reactos.exe".
 * See also base/system/userinit/userinit.c!StartInstaller()
 */
VOID ExpandInstallerPath(IN OUT LPTSTR lpInstallerPath, IN SIZE_T PathSize)
{
    SYSTEM_INFO SystemInfo;
    PTSTR ptr;

#if 0
    if (_tcsicmp(lpInstallerPath, TEXT("reactos.exe")) != 0)
        return;
#endif

    /*
     * Using the default drive, under the directory whose name
     * corresponds to the currently-runnning CPU architecture.
     */
    GetSystemInfo(&SystemInfo);

    *lpInstallerPath = UNICODE_NULL;
    GetModuleFileName(NULL, lpInstallerPath, PathSize);
    ptr = _tcschr(lpInstallerPath, _T('\\'));
    if (ptr)
        *++ptr = UNICODE_NULL;
    else
        *lpInstallerPath = UNICODE_NULL;

    /* Append the corresponding CPU architecture */
    switch (SystemInfo.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_INTEL:
            StringCchCat(lpInstallerPath, PathSize, TEXT("I386"));
            break;

        case PROCESSOR_ARCHITECTURE_MIPS:
            StringCchCat(lpInstallerPath, PathSize, TEXT("MIPS"));
            break;

        case PROCESSOR_ARCHITECTURE_ALPHA:
            StringCchCat(lpInstallerPath, PathSize, TEXT("ALPHA"));
            break;

        case PROCESSOR_ARCHITECTURE_PPC:
            StringCchCat(lpInstallerPath, PathSize, TEXT("PPC"));
            break;

        case PROCESSOR_ARCHITECTURE_SHX:
            StringCchCat(lpInstallerPath, PathSize, TEXT("SHX"));
            break;

        case PROCESSOR_ARCHITECTURE_ARM:
            StringCchCat(lpInstallerPath, PathSize, TEXT("ARM"));
            break;

        case PROCESSOR_ARCHITECTURE_IA64:
            StringCchCat(lpInstallerPath, PathSize, TEXT("IA64"));
            break;

        case PROCESSOR_ARCHITECTURE_ALPHA64:
            StringCchCat(lpInstallerPath, PathSize, TEXT("ALPHA64"));
            break;

#if 0 // .NET CPU-independent code
        case PROCESSOR_ARCHITECTURE_MSIL:
            StringCchCat(lpInstallerPath, PathSize, TEXT("MSIL"));
            break;
#endif

        case PROCESSOR_ARCHITECTURE_AMD64:
            StringCchCat(lpInstallerPath, PathSize, TEXT("AMD64"));
            break;

        case PROCESSOR_ARCHITECTURE_UNKNOWN:
        default:
            break;
    }

    StringCchCat(lpInstallerPath, PathSize, TEXT("\\reactos.exe"));
}

VOID InitializeTopicList(VOID)
{
    dwNumberTopics = 0;
    pTopics = NULL;
}

PTOPIC AddNewTopic(VOID)
{
    PTOPIC pTopic, *pTopicsTmp;

    /* Allocate (or reallocate) the list of topics */
    if (!pTopics)
        pTopicsTmp = HeapAlloc(GetProcessHeap(), 0, (dwNumberTopics + 1) * sizeof(*pTopics));
    else
        pTopicsTmp = HeapReAlloc(GetProcessHeap(), 0, pTopics, (dwNumberTopics + 1) * sizeof(*pTopics));
    if (!pTopicsTmp)
        return NULL; // Cannot reallocate more
    pTopics = pTopicsTmp;

    /* Allocate a new topic entry */
    pTopic = HeapAlloc(GetProcessHeap(), 0, sizeof(*pTopic));
    if (!pTopic)
        return NULL; // Cannot reallocate more
    pTopics[dwNumberTopics++] = pTopic;

    /* Return the allocated topic entry */
    return pTopic;
}

PTOPIC AddNewTopicEx(
    IN LPTSTR szText  OPTIONAL,
    IN LPTSTR szTitle OPTIONAL,
    IN LPTSTR szDesc  OPTIONAL,
    IN LPTSTR szCommand OPTIONAL,
    IN LPTSTR szArgs    OPTIONAL,
    IN LPTSTR szAction  OPTIONAL)
{
    PTOPIC pTopic = AddNewTopic();
    if (!pTopic)
        return NULL;

    if (szText && *szText)
        StringCchCopy(pTopic->szText, ARRAYSIZE(pTopic->szText), szText);
    else
        *pTopic->szText = 0;

    if (szTitle && *szTitle)
        StringCchCopy(pTopic->szTitle, ARRAYSIZE(pTopic->szTitle), szTitle);
    else
        *pTopic->szTitle = 0;

    if (szDesc && *szDesc)
    {
        StringCchCopy(pTopic->szDesc, ARRAYSIZE(pTopic->szDesc), szDesc);
        TranslateEscapes(pTopic->szDesc);
    }
    else
    {
        *pTopic->szDesc = 0;
    }

    if (szCommand && *szCommand)
    {
        pTopic->bIsCommand = TRUE;
        StringCchCopy(pTopic->szCommand, ARRAYSIZE(pTopic->szCommand), szCommand);

        /* Check for special applications: ReactOS Installer */
        if (_tcsicmp(pTopic->szCommand, TEXT("reactos.exe")) == 0)
            ExpandInstallerPath(pTopic->szCommand, ARRAYSIZE(pTopic->szCommand));
    }
    else
    {
        pTopic->bIsCommand = FALSE;
        *pTopic->szCommand = 0;
    }

    /* Only care about command arguments if we actually have a command */
    if (*pTopic->szCommand)
    {
        if (szArgs && *szArgs)
        {
            StringCchCopy(pTopic->szArgs, ARRAYSIZE(pTopic->szArgs), szArgs);
        }
        else
        {
            /* Check for special applications: ReactOS Shell */
            if (/* pTopic->szCommand && */ *pTopic->szCommand &&
                _tcsicmp(pTopic->szCommand, TEXT("explorer.exe")) == 0)
            {
#if 0
                TCHAR CurrentDir[MAX_PATH];
                GetCurrentDirectory(ARRAYSIZE(CurrentDir), CurrentDir);
#endif
                StringCchCopy(pTopic->szArgs, ARRAYSIZE(pTopic->szArgs), TEXT("\\"));
            }
            else
            {
                *pTopic->szArgs = 0;
            }
        }
    }
    else
    {
        *pTopic->szArgs = 0;
    }

    /* Only care about custom actions if we actually don't have a command */
    if (!*pTopic->szCommand && szAction && *szAction)
    {
        /*
         * Re-use the pTopic->szCommand member. We distinguish with respect to
         * a regular command by using the pTopic->bIsCommand flag.
         */
        pTopic->bIsCommand = FALSE;
        StringCchCopy(pTopic->szCommand, ARRAYSIZE(pTopic->szCommand), szAction);
        TranslateEscapes(pTopic->szCommand);
    }

    return pTopic;
}

static VOID
LoadLocalizedResourcesInternal(VOID)
{
#define MAX_NUMBER_INTERNAL_TOPICS  3

    UINT i;
    LPTSTR lpszCommand, lpszAction;
    TOPIC newTopic, *pTopic;

    for (i = 0; i < MAX_NUMBER_INTERNAL_TOPICS; ++i)
    {
        lpszCommand = NULL, lpszAction = NULL;

        /* Retrieve the information */
        if (!LoadString(hInstance, IDS_TOPIC_BUTTON0 + i, newTopic.szText, ARRAYSIZE(newTopic.szText)))
            *newTopic.szText = 0;
        if (!LoadString(hInstance, IDS_TOPIC_TITLE0 + i, newTopic.szTitle, ARRAYSIZE(newTopic.szTitle)))
            *newTopic.szTitle = 0;
        if (!LoadString(hInstance, IDS_TOPIC_DESC0 + i, newTopic.szDesc, ARRAYSIZE(newTopic.szDesc)))
            *newTopic.szDesc = 0;

        if (!LoadString(hInstance, IDS_TOPIC_COMMAND0 + i, newTopic.szCommand, ARRAYSIZE(newTopic.szCommand)))
            *newTopic.szCommand = 0;

        /* Only care about command arguments if we actually have a command */
        if (*newTopic.szCommand)
        {
            lpszCommand = newTopic.szCommand;
            if (!LoadString(hInstance, IDS_TOPIC_CMD_ARGS0 + i, newTopic.szArgs, ARRAYSIZE(newTopic.szArgs)))
                *newTopic.szArgs = 0;
        }
        /* Only care about custom actions if we actually don't have a command */
        else // if (!*newTopic.szCommand)
        {
            lpszAction = newTopic.szCommand;
            if (!LoadString(hInstance, IDS_TOPIC_ACTION0 + i, newTopic.szCommand, ARRAYSIZE(newTopic.szCommand)))
                *newTopic.szCommand = 0;
        }

        /* Allocate a new topic */
        pTopic = AddNewTopicEx(newTopic.szText,
                               newTopic.szTitle,
                               newTopic.szDesc,
                               lpszCommand,
                               newTopic.szArgs,
                               lpszAction);
        if (!pTopic)
            break; // Cannot reallocate more
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
    LPTSTR lpszCommand, lpszAction;
    TOPIC newTopic, *pTopic;

    /* Retrieve the locale name (on which the INI file name is based) */
    dwRet = (DWORD)GetLocaleName(Locale, szBuffer, ARRAYSIZE(szBuffer));
    if (!dwRet)
    {
        /* Fall back to english (US) */
        StringCchCopy(szBuffer, ARRAYSIZE(szBuffer), TEXT("en-US"));
    }

    /* Build the INI file name */
    StringCchPrintf(szIniPath, ARRAYSIZE(szIniPath),
                    TEXT("%s\\%s.ini"), lpResPath, szBuffer);

    /* Verify that the file exists, otherwise fall back to english (US) */
    if (GetFileAttributes(szIniPath) == INVALID_FILE_ATTRIBUTES)
    {
        StringCchCopy(szBuffer, ARRAYSIZE(szBuffer), TEXT("en-US"));

        StringCchPrintf(szIniPath, ARRAYSIZE(szIniPath),
                        TEXT("%s\\%s.ini"), lpResPath, szBuffer);
    }

    /* Verify that the file exists, otherwise fall back to internal (localized) resource */
    if (GetFileAttributes(szIniPath) == INVALID_FILE_ATTRIBUTES)
        return FALSE; // For localized resources, see the general function.

    /* Try to load the default localized strings */
    GetPrivateProfileString(TEXT("Defaults"), TEXT("AppTitle"), TEXT("ReactOS - Welcome") /* default */,
                            szAppTitle, ARRAYSIZE(szAppTitle), szIniPath);
    GetPrivateProfileString(TEXT("Defaults"), TEXT("DefaultTopicTitle"), TEXT("") /* default */,
                            szDefaultTitle, ARRAYSIZE(szDefaultTitle), szIniPath);
    if (!GetPrivateProfileString(TEXT("Defaults"), TEXT("DefaultTopicDescription"), TEXT("") /* default */,
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

    /* Loop over the sections and load the topics */
    lpszSection = lpszSections;
    for (; lpszSection && *lpszSection; lpszSection += (_tcslen(lpszSection) + 1))
    {
        /* Ignore everything that is not a topic */
        if (_tcsnicmp(lpszSection, TEXT("Topic"), 5) != 0)
            continue;

        lpszCommand = NULL, lpszAction = NULL;

        /* Retrieve the information */
        GetPrivateProfileString(lpszSection, TEXT("MenuText"), TEXT("") /* default */,
                                newTopic.szText, ARRAYSIZE(newTopic.szText), szIniPath);
        GetPrivateProfileString(lpszSection, TEXT("Title"), TEXT("") /* default */,
                                newTopic.szTitle, ARRAYSIZE(newTopic.szTitle), szIniPath);
        GetPrivateProfileString(lpszSection, TEXT("Description"), TEXT("") /* default */,
                                newTopic.szDesc, ARRAYSIZE(newTopic.szDesc), szIniPath);

        GetPrivateProfileString(lpszSection, TEXT("ConfigCommand"), TEXT("") /* default */,
                                newTopic.szCommand, ARRAYSIZE(newTopic.szCommand), szIniPath);

        /* Only care about command arguments if we actually have a command */
        if (*newTopic.szCommand)
        {
            lpszCommand = newTopic.szCommand;
            GetPrivateProfileString(lpszSection, TEXT("ConfigArgs"), TEXT("") /* default */,
                                    newTopic.szArgs, ARRAYSIZE(newTopic.szArgs), szIniPath);
        }
        /* Only care about custom actions if we actually don't have a command */
        else // if (!*newTopic.szCommand)
        {
            lpszAction = newTopic.szCommand;
            GetPrivateProfileString(lpszSection, TEXT("Action"), TEXT("") /* default */,
                                    newTopic.szCommand, ARRAYSIZE(newTopic.szCommand), szIniPath);
        }

        /* Allocate a new topic */
        pTopic = AddNewTopicEx(newTopic.szText,
                               newTopic.szTitle,
                               newTopic.szDesc,
                               lpszCommand,
                               newTopic.szArgs,
                               lpszAction);
        if (!pTopic)
            break; // Cannot reallocate more
    }

    HeapFree(GetProcessHeap(), 0, lpszSections);

    return TRUE;
}

static VOID
LoadConfiguration(VOID)
{
    BOOL  bLoadDefaultResources;
    TCHAR szAppPath[MAX_PATH];
    TCHAR szIniPath[MAX_PATH];
    TCHAR szResPath[MAX_PATH];

    /* Initialize the topic list */
    InitializeTopicList();

    /*
     * First, try to load the default internal (localized) strings.
     * They can be redefined by the localized INI files.
     */
    if (!LoadString(hInstance, IDS_APPTITLE, szAppTitle, ARRAYSIZE(szAppTitle)))
        StringCchCopy(szAppTitle, ARRAYSIZE(szAppTitle), TEXT("ReactOS - Welcome"));
    if (!LoadString(hInstance, IDS_DEFAULT_TOPIC_TITLE, szDefaultTitle, ARRAYSIZE(szDefaultTitle)))
        *szDefaultTitle = 0;
    if (!LoadString(hInstance, IDS_DEFAULT_TOPIC_DESC, szDefaultDesc, ARRAYSIZE(szDefaultDesc)))
        *szDefaultDesc = 0;

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
    StringCchPrintf(szIniPath, ARRAYSIZE(szIniPath), TEXT("%s\\welcome.ini"), szAppPath);

    /* Verify that the file exists, otherwise use the default configuration */
    if (GetFileAttributes(szIniPath) == INVALID_FILE_ATTRIBUTES)
    {
        /* Use the default internal (localized) resources */
        LoadLocalizedResourcesInternal();
        return;
    }

    /* Load the settings from the INI configuration file */
    bDisplayCheckBox = !!GetPrivateProfileInt(TEXT("Welcome"), TEXT("DisplayCheckBox"),  FALSE /* default */, szIniPath);
    bDisplayExitBtn  = !!GetPrivateProfileInt(TEXT("Welcome"), TEXT("DisplayExitButton"), TRUE /* default */, szIniPath);

    /* Load the default internal (localized) resources if needed */
    bLoadDefaultResources = !!GetPrivateProfileInt(TEXT("Welcome"), TEXT("LoadDefaultResources"), FALSE /* default */, szIniPath);
    if (bLoadDefaultResources)
        LoadLocalizedResourcesInternal();

    GetPrivateProfileString(TEXT("Welcome"), TEXT("ResourceDir"), TEXT("") /* default */,
                            szResPath, ARRAYSIZE(szResPath), szIniPath);

    /* Set the current directory to the one of this application, and retrieve the resources */
    SetCurrentDirectory(szAppPath);
    if (!LoadLocalizedResourcesFromINI(LOCALE_USER_DEFAULT, szResPath))
    {
        /*
         * Loading localized resources from INI file failed, try to load the
         * internal resources only if they were not already loaded earlier.
         */
        if (!bLoadDefaultResources)
            LoadLocalizedResourcesInternal();
    }
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
    HANDLE hMutex = NULL;
    WNDCLASSEX wndclass;
    MSG msg;
    HWND hWndFocus;
    INT xPos, yPos;
    INT xWidth, yHeight;
    RECT rcWindow;
    HICON hMainIcon;
    HMENU hSystemMenu;
    DWORD dwStyle = WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                    WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    BITMAP BitmapInfo;
    ULONG ulInnerWidth  = TITLE_WIDTH;
    ULONG ulInnerHeight = (TITLE_WIDTH * 3) / 4;
    ULONG ulTitleHeight = TITLE_HEIGHT + 3;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpszCmdLine);

    /* Ensure only one instance is running */
    hMutex = CreateMutex(NULL, FALSE, szWindowClass);
    if (hMutex && (GetLastError() == ERROR_ALREADY_EXISTS))
    {
        /* If already started, find its window */
        hWndMain = FindWindow(szWindowClass, NULL);

        /* Activate window */
        ShowWindow(hWndMain, SW_SHOWNORMAL);
        SetForegroundWindow(hWndMain);

        /* Close the mutex handle and quit */
        CloseHandle(hMutex);
        return 0;
    }

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
    wndclass.cbSize = sizeof(wndclass);
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
    wndclass.lpszClassName = szWindowClass;

    RegisterClassEx(&wndclass);

    /* Load the banner bitmap, and compute the window dimensions */
    hTitleBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TITLE_BITMAP));
    if (hTitleBitmap)
    {
        GetObject(hTitleBitmap, sizeof(BitmapInfo), &BitmapInfo);
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
    xWidth  = rcWindow.right - rcWindow.left;
    yHeight = rcWindow.bottom - rcWindow.top;

    /* Compute the window position */
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
    hWndMain = CreateWindow(szWindowClass,
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
        /* Check for ENTER key presses */
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN)
        {
            /*
             * The user pressed the ENTER key. Retrieve the handle to the
             * child window that has the keyboard focus, and send it a
             * WM_COMMAND message.
             */
            hWndFocus = GetFocus();
            if (hWndFocus)
            {
                SendMessage(hWndMain, WM_COMMAND,
                            (WPARAM)GetDlgCtrlID(hWndFocus), (LPARAM)hWndFocus);
            }
        }
        /* Allow using keyboard navigation */
        else if (!IsDialogMessage(hWndMain, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    /* Cleanup */
    FreeResources();

    /* Close the mutex handle and quit */
    CloseHandle(hMutex);
    return msg.wParam;
}


INT_PTR CALLBACK
ButtonSubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static WPARAM wParamOld = 0;
    static LPARAM lParamOld = 0;

    LONG i;

    if (uMsg == WM_MOUSEMOVE)
    {
        /* Ignore mouse-move messages on the same point */
        if ((wParam == wParamOld) && (lParam == lParamOld))
            return 0;

        /* Retrieve the topic index of this button */
        i = GetWindowLongPtr(hWnd, GWLP_ID) - TOPIC_BTN_ID_BASE;

        /*
         * Change the focus to this button if the current topic index differs
         * (we will receive WM_SETFOCUS afterwards).
         */
        if (nTopic != i)
            SetFocus(hWnd);

        wParamOld = wParam;
        lParamOld = lParam;
    }
    else if (uMsg == WM_SETFOCUS)
    {
        /* Retrieve the topic index of this button */
        i = GetWindowLongPtr(hWnd, GWLP_ID) - TOPIC_BTN_ID_BASE;

        /* Change the current topic index and repaint the description panel */
        if (nTopic != i)
        {
            nTopic = i;
            InvalidateRect(hWndMain, &rcRightPanel, TRUE);
        }
    }
    else if (uMsg == WM_KILLFOCUS)
    {
        /*
         * We lost focus, either because the user changed button focus,
         * or because the main window to which we belong went inactivated.
         * If we are in the latter case, we ignore the focus change.
         * If we are in the former case, we reset to the default topic.
         */
        if (GetParent(hWnd) == GetForegroundWindow())
        {
            nTopic = -1;
            InvalidateRect(hWndMain, &rcRightPanel, TRUE);
        }
    }

    return CallWindowProc(fnOldBtn, hWnd, uMsg, wParam, lParam);
}


static BOOL
RunAction(INT nTopic)
{
    PCWSTR Command = NULL, Args = NULL;

    if (nTopic < 0)
        return TRUE;

    Command = pTopics[nTopic]->szCommand;
    if (/* !Command && */ !*Command)
        return TRUE;

    /* Check for known actions */
    if (!pTopics[nTopic]->bIsCommand)
    {
        if (!_tcsicmp(Command, TEXT("<exit>")))
            return FALSE;

        if (!_tcsnicmp(Command, TEXT("<msg>"), 5))
        {
            MessageBox(hWndMain, Command + 5, TEXT("ReactOS"), MB_OK | MB_TASKMODAL);
            return TRUE;
        }
    }
    else
    /* Run the command */
    {
        Args = pTopics[nTopic]->szArgs;
        if (!*Args) Args = NULL;
        ShellExecute(NULL, NULL,
                     Command, Args,
                     NULL, SW_SHOWDEFAULT);
    }

    return TRUE;
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
    UINT i;
    INT nLength;
    HDC ScreenDC;
    LOGFONT lf;
    DWORD dwTop;
    DWORD dwHeight = 0;
    TCHAR szText[80];

    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    hbrLightBlue = CreateSolidBrush(LIGHT_BLUE);
    hbrDarkBlue  = CreateSolidBrush(DARK_BLUE);

    ZeroMemory(&lf, sizeof(lf));

    lf.lfEscapement  = 0;
    lf.lfOrientation = 0; // TA_BASELINE;
    // lf.lfItalic = lf.lfUnderline = lf.lfStrikeOut = FALSE;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision  = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = FF_DONTCARE;
    StringCchCopy(lf.lfFaceName, ARRAYSIZE(lf.lfFaceName), TEXT("Tahoma"));

    /* Topic title font */
    lf.lfHeight = -18;
    lf.lfWidth  = 0;
    lf.lfWeight = FW_NORMAL;
    hFontTopicTitle = CreateFontIndirect(&lf);

    /* Topic description font */
    lf.lfHeight = -11;
    lf.lfWidth  = 0;
    lf.lfWeight = FW_THIN;
    hFontTopicDescription = CreateFontIndirect(&lf);

    /* Topic button font */
    lf.lfHeight = -11;
    lf.lfWidth  = 0;
    lf.lfWeight = FW_BOLD;
    hFontTopicButton = CreateFontIndirect(&lf);

    /* Load title bitmap */
    if (hTitleBitmap)
        hTitleBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TITLE_BITMAP));

    /* Load topic bitmaps */
    hDefaultTopicBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_DEFAULT_TOPIC_BITMAP));
    for (i = 0; i < dwNumberTopics; i++)
    {
        // FIXME: Not implemented yet!
        // pTopics[i]->hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TOPIC_BITMAP0 + i));
        pTopics[i]->hBitmap = NULL;
    }

    ScreenDC = GetWindowDC(hWnd);
    hdcMem = CreateCompatibleDC(ScreenDC);
    ReleaseDC(hWnd, ScreenDC);

    /* Load and create the menu buttons */
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
                                                  (HMENU)IntToPtr(TOPIC_BTN_ID_BASE + i), // Similar to SetWindowLongPtr(GWLP_ID)
                                                  hInstance,
                                                  NULL);
            nDefaultTopic = i;
            SendMessage(pTopics[i]->hWndButton, WM_SETFONT, (WPARAM)hFontTopicButton, MAKELPARAM(TRUE, 0));
            fnOldBtn = (WNDPROC)SetWindowLongPtr(pTopics[i]->hWndButton, GWLP_WNDPROC, (DWORD_PTR)ButtonSubclassWndProc);
        }
        else
        {
            pTopics[i]->hWndButton = NULL;
        }

        dwTop += dwHeight;
    }

    /* Create the checkbox */
    if (bDisplayCheckBox)
    {
        nLength = LoadString(hInstance, IDS_CHECKTEXT, szText, ARRAYSIZE(szText));
        if (nLength > 0)
        {
            lf.lfHeight = -10;
            lf.lfWidth  = 0;
            lf.lfWeight = FW_THIN;
            hFontCheckButton = CreateFontIndirect(&lf);

            hWndCheckButton = CreateWindow(TEXT("BUTTON"),
                                           szText,
                                           WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | BS_MULTILINE /**/| BS_FLAT/**/,
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

    /* Create the "Exit" button */
    if (bDisplayExitBtn)
    {
        nLength = LoadString(hInstance, IDS_CLOSETEXT, szText, ARRAYSIZE(szText));
        if (nLength > 0)
        {
            hWndCloseButton = CreateWindow(TEXT("BUTTON"),
                                           szText,
                                           WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | BS_FLAT,
                                           rcRightPanel.right - 8 - 57,
                                           rcRightPanel.bottom - 8 - 21,
                                           57,
                                           21,
                                           hWnd,
                                           (HMENU)IDC_CLOSEBUTTON,
                                           hInstance,
                                           NULL);
            nDefaultTopic = -1;
            SendMessage(hWndCloseButton, WM_SETFONT, (WPARAM)hFontTopicButton, MAKELPARAM(TRUE, 0));
        }
        else
        {
            hWndCloseButton = NULL;
        }
    }

    return 0;
}


static LRESULT
OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    /* Retrieve the low-word from wParam */
    wParam = LOWORD(wParam);

    /* Execute action */
    if (wParam == IDC_CLOSEBUTTON)
    {
        DestroyWindow(hWnd);
    }
    else if (wParam - TOPIC_BTN_ID_BASE < dwNumberTopics)
    {
        if (RunAction(wParam - TOPIC_BTN_ID_BASE) == FALSE)
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
    TCHAR szVersion[50];
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
    hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(WHITE_BRUSH));
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
    if ((nTopic == -1) && (hDefaultTopicBitmap))
    {
        GetObject(hDefaultTopicBitmap, sizeof(bmpInfo), &bmpInfo);
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
    else if ((nTopic != -1) && (pTopics[nTopic]->hBitmap))
    {
        GetObject(pTopics[nTopic]->hBitmap, sizeof(bmpInfo), &bmpInfo);
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
    StringCchCopy(szVersion, ARRAYSIZE(szVersion),
                  TEXT("ReactOS ") TEXT(KERNEL_VERSION_STR));

    /*
     * Compute the original rect (position & size) of the version info,
     * depending whether the checkbox is displayed (version info in the
     * right panel) or not (version info in the left panel).
     */
    if (bDisplayCheckBox)
        rcTitle = rcRightPanel;
    else
        rcTitle = rcLeftPanel;

    rcTitle.left   = rcTitle.left + 8;
    rcTitle.right  = rcTitle.right - 5;
    rcTitle.top    = rcTitle.bottom - 43;
    rcTitle.bottom = rcTitle.bottom - 8;

    hOldFont = (HFONT)SelectObject(hdc, hFontTopicDescription);
    DrawText(hdc, szVersion, -1, &rcTitle, DT_BOTTOM | DT_CALCRECT | DT_SINGLELINE);
    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    DrawText(hdc, szVersion, -1, &rcTitle, DT_BOTTOM | DT_SINGLELINE);
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
    SelectObject(hdc, hOldFont);

    /* Draw topic description */
    rcDescription.left = rcRightPanel.left + 12;
    rcDescription.right = rcRightPanel.right - 8;
    rcDescription.top = rcTitle.bottom + 8;
    rcDescription.bottom = rcRightPanel.bottom - 20;
    hOldFont = (HFONT)SelectObject(hdc, hFontTopicDescription);
    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    DrawText(hdc, lpDesc, -1, &rcDescription, DT_TOP | DT_WORDBREAK);
    SelectObject(hdc, hOldFont);

    SetBkMode(hdc, OPAQUE);

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

#if 0
    /* Neither the checkbox button nor the close button implement owner-drawing */
    if (lpDis->hwndItem == hWndCheckButton)
        return 0;
    if (lpDis->hwndItem == hWndCloseButton)
    {
        DrawFrameControl(lpDis->hDC,
                         &lpDis->rcItem,
                         DFC_BUTTON,
                         DFCS_BUTTONPUSH | DFCS_FLAT);
        return TRUE;
    }
#endif

    if (lpDis->CtlID == (ULONG)(TOPIC_BTN_ID_BASE + nTopic))
        hOldBrush = (HBRUSH)SelectObject(lpDis->hDC, GetStockObject(WHITE_BRUSH));
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
    SetTextColor(lpDis->hDC, GetSysColor(COLOR_WINDOWTEXT));
    iBkMode = SetBkMode(lpDis->hDC, TRANSPARENT);
    DrawText(lpDis->hDC, szText, -1, &lpDis->rcItem, DT_TOP | DT_LEFT | DT_WORDBREAK);
    SetBkMode(lpDis->hDC, iBkMode);

    return TRUE;
}


static LRESULT
OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    static WPARAM wParamOld = 0;
    static LPARAM lParamOld = 0;

    /* Ignore mouse-move messages on the same point */
    if ((wParam == wParamOld) && (lParam == lParamOld))
        return 0;

    /*
     * If the user moves the mouse over the main window, outside of the
     * topic buttons, reset the current topic to the default one and
     * change the focus to some other default button (to keep keyboard
     * navigation possible).
     */
    if (nTopic != -1)
    {
        INT nOldTopic = nTopic;
        nTopic = -1;
        /* Also repaint the buttons, otherwise nothing repaints... */
        InvalidateRect(pTopics[nOldTopic]->hWndButton, NULL, TRUE);

        /* Set the focus to some other default button */
        if (hWndCheckButton)
            SetFocus(hWndCheckButton);
        else if (hWndCloseButton)
            SetFocus(hWndCloseButton);
        // SetFocus(hWnd);

        /* Repaint the description panel */
        InvalidateRect(hWndMain, &rcRightPanel, TRUE);
    }

    wParamOld = wParam;
    lParamOld = lParam;

    return 0;
}


static LRESULT
OnCtlColorStatic(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);

    if ((HWND)lParam == hWndCheckButton)
    {
        SetBkMode((HDC)wParam, TRANSPARENT);
        return (LRESULT)hbrLightBlue;
    }

    return 0;
}


static LRESULT
OnActivate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(lParam);

    if (wParam != WA_INACTIVE)
    {
        /*
         * The main window is re-activated, set the focus back to
         * either the current topic or a default button.
         */
        if (nTopic != -1)
            SetFocus(pTopics[nTopic]->hWndButton);
        else if (hWndCheckButton)
            SetFocus(hWndCheckButton);
        else if (hWndCloseButton)
            SetFocus(hWndCloseButton);

        // InvalidateRect(hWndMain, &rcRightPanel, TRUE);
    }

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
        if (pTopics[i]->hWndButton)
            DestroyWindow(pTopics[i]->hWndButton);
    }

    if (hWndCloseButton)
        DestroyWindow(hWndCloseButton);

    if (hWndCheckButton)
        DestroyWindow(hWndCheckButton);

    DeleteDC(hdcMem);

    /* Delete bitmaps */
    DeleteObject(hDefaultTopicBitmap);
    DeleteObject(hTitleBitmap);
    for (i = 0; i < dwNumberTopics; i++)
    {
        if (pTopics[i]->hBitmap)
            DeleteObject(pTopics[i]->hBitmap);
    }

    DeleteObject(hFontTopicTitle);
    DeleteObject(hFontTopicDescription);
    DeleteObject(hFontTopicButton);

    if (hFontCheckButton)
        DeleteObject(hFontCheckButton);

    DeleteObject(hbrLightBlue);
    DeleteObject(hbrDarkBlue);

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
