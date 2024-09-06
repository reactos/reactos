/*
 * PROJECT:    ReactOS Notepad
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Providing a Windows-compatible simple text editor for ReactOS
 * COPYRIGHT:  Copyright 2000 Mike McCormack <Mike_McCormack@looksmart.com.au>
 *             Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
 *             Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *             Copyright 2002 Andriy Palamarchuk
 *             Copyright 2020-2023 Katayama Hirofumi MZ
 */

#include "notepad.h"

#include <shlobj.h>
#include <strsafe.h>

NOTEPAD_GLOBALS Globals;
static ATOM aFINDMSGSTRING;

VOID NOTEPAD_EnableSearchMenu()
{
    BOOL bEmpty = (GetWindowTextLengthW(Globals.hEdit) == 0);
    UINT uEnable = MF_BYCOMMAND | (bEmpty ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(Globals.hMenu, CMD_SEARCH, uEnable);
    EnableMenuItem(Globals.hMenu, CMD_SEARCH_NEXT, uEnable);
    EnableMenuItem(Globals.hMenu, CMD_SEARCH_PREV, uEnable);
}

/***********************************************************************
 *           SetFileName
 *
 *  Sets Global File Name.
 */
VOID SetFileName(LPCTSTR szFileName)
{
    StringCchCopy(Globals.szFileName, _countof(Globals.szFileName), szFileName);
    Globals.szFileTitle[0] = 0;
    GetFileTitle(szFileName, Globals.szFileTitle, _countof(Globals.szFileTitle));

    if (szFileName && szFileName[0])
        SHAddToRecentDocs(SHARD_PATHW, szFileName);
}

/***********************************************************************
 *           NOTEPAD_MenuCommand
 *
 *  All handling of main menu events
 */
static int NOTEPAD_MenuCommand(WPARAM wParam)
{
    switch (wParam)
    {
    case CMD_NEW:        DIALOG_FileNew(); break;
    case CMD_NEW_WINDOW: DIALOG_FileNewWindow(); break;
    case CMD_OPEN:       DIALOG_FileOpen(); break;
    case CMD_SAVE:       DIALOG_FileSave(); break;
    case CMD_SAVE_AS:    DIALOG_FileSaveAs(); break;
    case CMD_PRINT:      DIALOG_FilePrint(); break;
    case CMD_PAGE_SETUP: DIALOG_FilePageSetup(); break;
    case CMD_EXIT:       DIALOG_FileExit(); break;

    case CMD_UNDO:       DIALOG_EditUndo(); break;
    case CMD_CUT:        DIALOG_EditCut(); break;
    case CMD_COPY:       DIALOG_EditCopy(); break;
    case CMD_PASTE:      DIALOG_EditPaste(); break;
    case CMD_DELETE:     DIALOG_EditDelete(); break;
    case CMD_SELECT_ALL: DIALOG_EditSelectAll(); break;
    case CMD_TIME_DATE:  DIALOG_EditTimeDate(); break;

    case CMD_SEARCH:      DIALOG_Search(); break;
    case CMD_SEARCH_NEXT: DIALOG_SearchNext(TRUE); break;
    case CMD_REPLACE:     DIALOG_Replace(); break;
    case CMD_GOTO:        DIALOG_GoTo(); break;
    case CMD_SEARCH_PREV: DIALOG_SearchNext(FALSE); break;

    case CMD_WRAP: DIALOG_EditWrap(); break;
    case CMD_FONT: DIALOG_SelectFont(); break;

    case CMD_STATUSBAR: DIALOG_ViewStatusBar(); break;

    case CMD_HELP_CONTENTS: DIALOG_HelpContents(); break;
    case CMD_HELP_ABOUT_NOTEPAD: DIALOG_HelpAboutNotepad(); break;

    default:
        break;
    }
    return 0;
}

/***********************************************************************
 *           NOTEPAD_FindTextAt
 */
static BOOL
NOTEPAD_FindTextAt(FINDREPLACE *pFindReplace, LPCTSTR pszText, INT iTextLength, DWORD dwPosition)
{
    BOOL bMatches;
    size_t iTargetLength;
    LPCTSTR pchPosition;

    if (!pFindReplace || !pszText)
        return FALSE;

    iTargetLength = _tcslen(pFindReplace->lpstrFindWhat);
    pchPosition = &pszText[dwPosition];

    /* Make proper comparison */
    if (pFindReplace->Flags & FR_MATCHCASE)
        bMatches = !_tcsncmp(pchPosition, pFindReplace->lpstrFindWhat, iTargetLength);
    else
        bMatches = !_tcsnicmp(pchPosition, pFindReplace->lpstrFindWhat, iTargetLength);

    if (bMatches && (pFindReplace->Flags & FR_WHOLEWORD))
    {
        if (dwPosition > 0)
        {
            if (_istalnum(*(pchPosition - 1)) || *(pchPosition - 1) == _T('_'))
                bMatches = FALSE;
        }
        if ((INT)dwPosition + iTargetLength < iTextLength)
        {
            if (_istalnum(pchPosition[iTargetLength]) || pchPosition[iTargetLength] == _T('_'))
                bMatches = FALSE;
        }
    }

    return bMatches;
}

/***********************************************************************
 *           NOTEPAD_FindNext
 */
BOOL NOTEPAD_FindNext(FINDREPLACE *pFindReplace, BOOL bReplace, BOOL bShowAlert)
{
    int iTextLength, iTargetLength;
    size_t iAdjustment = 0;
    LPTSTR pszText = NULL;
    DWORD dwPosition, dwBegin, dwEnd;
    BOOL bMatches = FALSE;
    TCHAR szResource[128], szText[128];
    BOOL bSuccess;

    iTargetLength = (int) _tcslen(pFindReplace->lpstrFindWhat);

    /* Retrieve the window text */
    iTextLength = GetWindowTextLength(Globals.hEdit);
    if (iTextLength > 0)
    {
        pszText = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, (iTextLength + 1) * sizeof(TCHAR));
        if (!pszText)
            return FALSE;

        GetWindowText(Globals.hEdit, pszText, iTextLength + 1);
    }

    SendMessage(Globals.hEdit, EM_GETSEL, (WPARAM) &dwBegin, (LPARAM) &dwEnd);
    if (bReplace && ((dwEnd - dwBegin) == (DWORD) iTargetLength))
    {
        if (NOTEPAD_FindTextAt(pFindReplace, pszText, iTextLength, dwBegin))
        {
            SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM) pFindReplace->lpstrReplaceWith);
            iAdjustment = _tcslen(pFindReplace->lpstrReplaceWith) - (dwEnd - dwBegin);
        }
    }

    if (pFindReplace->Flags & FR_DOWN)
    {
        /* Find Down */
        dwPosition = dwEnd;
        while(dwPosition < (DWORD) iTextLength)
        {
            bMatches = NOTEPAD_FindTextAt(pFindReplace, pszText, iTextLength, dwPosition);
            if (bMatches)
                break;
            dwPosition++;
        }
    }
    else
    {
        /* Find Up */
        dwPosition = dwBegin;
        while(dwPosition > 0)
        {
            dwPosition--;
            bMatches = NOTEPAD_FindTextAt(pFindReplace, pszText, iTextLength, dwPosition);
            if (bMatches)
                break;
        }
    }

    if (bMatches)
    {
        /* Found target */
        if (dwPosition > dwBegin)
            dwPosition += (DWORD) iAdjustment;
        SendMessage(Globals.hEdit, EM_SETSEL, dwPosition, dwPosition + iTargetLength);
        SendMessage(Globals.hEdit, EM_SCROLLCARET, 0, 0);
        bSuccess = TRUE;
    }
    else
    {
        /* Can't find target */
        if (bShowAlert)
        {
            LoadString(Globals.hInstance, STRING_CANNOTFIND, szResource, _countof(szResource));
            StringCchPrintf(szText, _countof(szText), szResource, pFindReplace->lpstrFindWhat);
            LoadString(Globals.hInstance, STRING_NOTEPAD, szResource, _countof(szResource));
            MessageBox(Globals.hFindReplaceDlg, szText, szResource, MB_OK);
        }
        bSuccess = FALSE;
    }

    if (pszText)
        HeapFree(GetProcessHeap(), 0, pszText);
    return bSuccess;
}

/***********************************************************************
 *           NOTEPAD_ReplaceAll
 */
static VOID NOTEPAD_ReplaceAll(FINDREPLACE *pFindReplace)
{
    BOOL bShowAlert = TRUE;

    SendMessage(Globals.hEdit, EM_SETSEL, 0, 0);

    while (NOTEPAD_FindNext(pFindReplace, TRUE, bShowAlert))
    {
        bShowAlert = FALSE;
    }
}

/***********************************************************************
 *           NOTEPAD_FindTerm
 */
static VOID NOTEPAD_FindTerm(VOID)
{
    Globals.hFindReplaceDlg = NULL;
}

/***********************************************************************
 * Data Initialization
 */
static VOID NOTEPAD_InitData(HINSTANCE hInstance)
{
    LPTSTR p;
    static const TCHAR txt_files[] = _T("*.txt");
    static const TCHAR all_files[] = _T("*.*");

    ZeroMemory(&Globals, sizeof(Globals));
    Globals.hInstance = hInstance;
    Globals.encFile = ENCODING_DEFAULT;

    p = Globals.szFilter;
    p += LoadString(Globals.hInstance, STRING_TEXT_FILES_TXT, p, MAX_STRING_LEN) + 1;
    _tcscpy(p, txt_files);
    p += _countof(txt_files);

    p += LoadString(Globals.hInstance, STRING_ALL_FILES, p, MAX_STRING_LEN) + 1;
    _tcscpy(p, all_files);
    p += _countof(all_files);
    *p = '\0';
    Globals.find.lpstrFindWhat = NULL;

    Globals.hDevMode = NULL;
    Globals.hDevNames = NULL;
}

/***********************************************************************
 * Enable/disable items on the menu based on control state
 */
static VOID NOTEPAD_InitMenuPopup(HMENU menu, LPARAM index)
{
    DWORD dwStart, dwEnd;
    int enable;

    UNREFERENCED_PARAMETER(index);

    CheckMenuItem(menu, CMD_WRAP, (Globals.bWrapLongLines ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, CMD_STATUSBAR, (Globals.bShowStatusBar ? MF_CHECKED : MF_UNCHECKED));
    EnableMenuItem(menu, CMD_UNDO,
        SendMessage(Globals.hEdit, EM_CANUNDO, 0, 0) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(menu, CMD_PASTE,
        IsClipboardFormatAvailable(CF_TEXT) ? MF_ENABLED : MF_GRAYED);
    SendMessage(Globals.hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    enable = ((dwStart == dwEnd) ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(menu, CMD_CUT, enable);
    EnableMenuItem(menu, CMD_COPY, enable);
    EnableMenuItem(menu, CMD_DELETE, enable);

    EnableMenuItem(menu, CMD_SELECT_ALL,
        GetWindowTextLength(Globals.hEdit) ? MF_ENABLED : MF_GRAYED);
}

LRESULT CALLBACK EDIT_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            switch (wParam)
            {
                case VK_UP:
                case VK_DOWN:
                case VK_LEFT:
                case VK_RIGHT:
                    DIALOG_StatusBarUpdateCaretPos();
                    break;
                default:
                {
                    UpdateWindowCaption(FALSE);
                    break;
                }
            }
        }
        case WM_LBUTTONUP:
        {
            DIALOG_StatusBarUpdateCaretPos();
            break;
        }
    }
    return CallWindowProc( Globals.EditProc, hWnd, msg, wParam, lParam);
}

/***********************************************************************
 *           NOTEPAD_WndProc
 */
static LRESULT
WINAPI
NOTEPAD_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {

    case WM_CREATE:
        Globals.hMainWnd = hWnd;
        Globals.hMenu = GetMenu(hWnd);

        DragAcceptFiles(hWnd, TRUE); /* Accept Drag & Drop */

        /* Create controls */
        DoCreateEditWindow();
        DoShowHideStatusBar();

        DIALOG_FileNew(); /* Initialize file info */

        // For now, the "Help" dialog is disabled due to the lack of HTML Help support
        EnableMenuItem(Globals.hMenu, CMD_HELP_CONTENTS, MF_BYCOMMAND | MF_GRAYED);
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == EN_HSCROLL || HIWORD(wParam) == EN_VSCROLL)
            DIALOG_StatusBarUpdateCaretPos();
        if ((HIWORD(wParam) == EN_CHANGE))
            NOTEPAD_EnableSearchMenu();
        NOTEPAD_MenuCommand(LOWORD(wParam));
        break;

    case WM_CLOSE:
        if (DoCloseFile())
            DestroyWindow(hWnd);
        break;

    case WM_QUERYENDSESSION:
        if (DoCloseFile()) {
            return 1;
        }
        break;

    case WM_DESTROY:
        if (Globals.hFont)
            DeleteObject(Globals.hFont);
        if (Globals.hDevMode)
            GlobalFree(Globals.hDevMode);
        if (Globals.hDevNames)
            GlobalFree(Globals.hDevNames);
        SetWindowLongPtr(Globals.hEdit, GWLP_WNDPROC, (LONG_PTR)Globals.EditProc);
        NOTEPAD_SaveSettingsToRegistry();
        PostQuitMessage(0);
        break;

    case WM_SIZE:
    {
        RECT rc;
        GetClientRect(hWnd, &rc);

        if (Globals.bShowStatusBar)
        {
            RECT rcStatus;
            SendMessageW(Globals.hStatusBar, WM_SIZE, 0, 0);
            GetWindowRect(Globals.hStatusBar, &rcStatus);
            rc.bottom -= rcStatus.bottom - rcStatus.top;
        }

        MoveWindow(Globals.hEdit, 0, 0, rc.right, rc.bottom, TRUE);

        if (Globals.bShowStatusBar)
        {
            /* Align status bar parts, only if the status bar resize operation succeeds */
            DIALOG_StatusBarAlignParts();
        }
        break;
    }

    /* The entire client area is covered by edit control and by
     * the status bar. So there is no need to erase main background.
     * This resolves the horrible flicker effect during windows resizes. */
    case WM_ERASEBKGND:
        return 1;

    case WM_SETFOCUS:
        SetFocus(Globals.hEdit);
        break;

    case WM_DROPFILES:
    {
        TCHAR szFileName[MAX_PATH];
        HDROP hDrop = (HDROP) wParam;

        DragQueryFile(hDrop, 0, szFileName, _countof(szFileName));
        DragFinish(hDrop);
        DoOpenFile(szFileName);
        break;
    }

    case WM_INITMENUPOPUP:
        NOTEPAD_InitMenuPopup((HMENU)wParam, lParam);
        break;

    default:
        if (msg == aFINDMSGSTRING)
        {
            FINDREPLACE *pFindReplace = (FINDREPLACE *) lParam;
            Globals.find = *(FINDREPLACE *) lParam;

            WaitCursor(TRUE);

            if (pFindReplace->Flags & FR_FINDNEXT)
                NOTEPAD_FindNext(pFindReplace, FALSE, TRUE);
            else if (pFindReplace->Flags & FR_REPLACE)
                NOTEPAD_FindNext(pFindReplace, TRUE, TRUE);
            else if (pFindReplace->Flags & FR_REPLACEALL)
                NOTEPAD_ReplaceAll(pFindReplace);
            else if (pFindReplace->Flags & FR_DIALOGTERM)
                NOTEPAD_FindTerm();

            WaitCursor(FALSE);
            break;
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

static int AlertFileDoesNotExist(LPCTSTR szFileName)
{
    return DIALOG_StringMsgBox(Globals.hMainWnd, STRING_DOESNOTEXIST,
                               szFileName,
                               MB_ICONEXCLAMATION | MB_YESNO);
}

static BOOL HandleCommandLine(LPTSTR cmdline)
{
    BOOL opt_print = FALSE;
    TCHAR szPath[MAX_PATH];

    while (*cmdline == _T(' ') || *cmdline == _T('-') || *cmdline == _T('/'))
    {
        TCHAR option;

        if (*cmdline++ == _T(' ')) continue;

        option = *cmdline;
        if (option) cmdline++;
        while (*cmdline == _T(' ')) cmdline++;

        switch(option)
        {
            case 'p':
            case 'P':
                opt_print = TRUE;
                break;
        }
    }

    if (*cmdline)
    {
        /* file name is passed in the command line */
        LPCTSTR file_name = NULL;
        BOOL file_exists = FALSE;
        TCHAR buf[MAX_PATH];

        if (cmdline[0] == _T('"'))
        {
            cmdline++;
            cmdline[lstrlen(cmdline) - 1] = 0;
        }

        file_name = cmdline;
        if (FileExists(file_name))
        {
            file_exists = TRUE;
        }
        else if (!HasFileExtension(cmdline))
        {
            static const TCHAR txt[] = _T(".txt");

            /* try to find file with ".txt" extension */
            if (!_tcscmp(txt, cmdline + _tcslen(cmdline) - _tcslen(txt)))
            {
                file_exists = FALSE;
            }
            else
            {
                _tcsncpy(buf, cmdline, MAX_PATH - _tcslen(txt) - 1);
                _tcscat(buf, txt);
                file_name = buf;
                file_exists = FileExists(file_name);
            }
        }

        GetFullPathName(file_name, _countof(szPath), szPath, NULL);

        if (file_exists)
        {
            DoOpenFile(szPath);
            InvalidateRect(Globals.hMainWnd, NULL, FALSE);
            if (opt_print)
            {
                DIALOG_FilePrint();
                return FALSE;
            }
        }
        else
        {
            switch (AlertFileDoesNotExist(file_name))
            {
            case IDYES:
                DoOpenFile(szPath);
                break;

            case IDNO:
                break;
            }
        }
    }

    return TRUE;
}

/***********************************************************************
 *           WinMain
 */
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE prev, LPTSTR cmdline, int show)
{
    MSG msg;
    HACCEL hAccel;
    WNDCLASSEX wndclass;
    WINDOWPLACEMENT wp;
    static const TCHAR className[] = _T("Notepad");
    static const TCHAR winName[] = _T("Notepad");

    switch (GetUserDefaultUILanguage())
    {
    case MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT):
        SetProcessDefaultLayout(LAYOUT_RTL);
        break;

    default:
        break;
    }

    UNREFERENCED_PARAMETER(prev);

    aFINDMSGSTRING = (ATOM)RegisterWindowMessage(FINDMSGSTRING);

    NOTEPAD_InitData(hInstance);
    NOTEPAD_LoadSettingsFromRegistry(&wp);

    ZeroMemory(&wndclass, sizeof(wndclass));
    wndclass.cbSize = sizeof(wndclass);
    wndclass.lpfnWndProc = NOTEPAD_WndProc;
    wndclass.hInstance = Globals.hInstance;
    wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NPICON));
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndclass.lpszMenuName = MAKEINTRESOURCE(MAIN_MENU);
    wndclass.lpszClassName = className;
    wndclass.hIconSm = (HICON)LoadImage(hInstance,
                                        MAKEINTRESOURCE(IDI_NPICON),
                                        IMAGE_ICON,
                                        GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON),
                                        0);
    if (!RegisterClassEx(&wndclass))
    {
        ShowLastError();
        return 1;
    }

    /* Globals.hMainWnd will be set in WM_CREATE handling */
    CreateWindow(className,
                 winName,
                 WS_OVERLAPPEDWINDOW,
                 CW_USEDEFAULT,
                 CW_USEDEFAULT,
                 CW_USEDEFAULT,
                 CW_USEDEFAULT,
                 NULL,
                 NULL,
                 Globals.hInstance,
                 NULL);
    if (!Globals.hMainWnd)
    {
        ShowLastError();
        return 1;
    }

    /* Use the result of CW_USEDEFAULT if the data in the registry is not valid */
    if (wp.rcNormalPosition.right == wp.rcNormalPosition.left)
    {
        GetWindowPlacement(Globals.hMainWnd, &wp);
    }
    /* Does the parent process want to force a show action? */
    if (show != SW_SHOWDEFAULT)
    {
        wp.showCmd = show;
    }
    SetWindowPlacement(Globals.hMainWnd, &wp);
    UpdateWindow(Globals.hMainWnd);

    if (!HandleCommandLine(cmdline))
        return 0;

    hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(ID_ACCEL));

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if ((!Globals.hFindReplaceDlg || !IsDialogMessage(Globals.hFindReplaceDlg, &msg)) &&
            !TranslateAccelerator(Globals.hMainWnd, hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DestroyAcceleratorTable(hAccel);

    return (int) msg.wParam;
}
