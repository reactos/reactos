/*
 * PROJECT:     ReactOS Clipboard Viewer
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Provides a view of the contents of the ReactOS clipboard.
 * COPYRIGHT:   Copyright 2015-2018 Ricardo Hanke
 *              Copyright 2015-2018 Hermes Belusca-Maito
 *              Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

#include <shlobj.h> /* For CFSTR_... */

static const WCHAR szClassName[] = L"ClipBookWClass";

CLIPBOARD_GLOBALS Globals;
SCROLLSTATE Scrollstate;

static void InitGlobals(HINSTANCE hInstance)
{
    ZeroMemory(&Globals, sizeof(Globals));
    Globals.hInstance = hInstance;

    /* Registered clipboard formats */
    Globals.uCFSTR_FILENAMEA = RegisterClipboardFormatA(CFSTR_FILENAMEA);
    Globals.uCFSTR_FILENAMEW = RegisterClipboardFormatW(CFSTR_FILENAMEW);
}

static void SaveClipboardToFile(void)
{
    OPENFILENAMEW sfn;
    LPWSTR c;
    WCHAR szFileName[MAX_PATH];
    WCHAR szFilterMask[MAX_STRING_LEN + 10];

    ZeroMemory(&szFilterMask, sizeof(szFilterMask));
    c = szFilterMask + LoadStringW(Globals.hInstance, STRING_FORMAT_NT, szFilterMask, MAX_STRING_LEN) + 1;
    wcscpy(c, L"*.clp");

    ZeroMemory(&szFileName, sizeof(szFileName));
    ZeroMemory(&sfn, sizeof(sfn));
    sfn.lStructSize = sizeof(sfn);
    sfn.hwndOwner = Globals.hMainWnd;
    sfn.hInstance = Globals.hInstance;
    sfn.lpstrFilter = szFilterMask;
    sfn.lpstrFile = szFileName;
    sfn.nMaxFile = ARRAYSIZE(szFileName);
    sfn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    sfn.lpstrDefExt = L"clp";

    if (!GetSaveFileNameW(&sfn))
        return;

    if (!OpenClipboard(Globals.hMainWnd))
    {
        ShowLastWin32Error(Globals.hMainWnd);
        return;
    }

    WriteClipboardFile(szFileName, CLIP_FMT_NT /* CLIP_FMT_31 */);

    CloseClipboard();
}

static void LoadClipboardDataFromFile(LPWSTR lpszFileName)
{
    if (MessageBoxRes(Globals.hMainWnd, Globals.hInstance,
                      STRING_DELETE_MSG, STRING_DELETE_TITLE,
                      MB_ICONWARNING | MB_YESNO) != IDYES)
    {
        return;
    }

    if (!OpenClipboard(Globals.hMainWnd))
    {
        ShowLastWin32Error(Globals.hMainWnd);
        return;
    }

    EmptyClipboard();
    ReadClipboardFile(lpszFileName);

    CloseClipboard();
}

static void LoadClipboardFromFile(void)
{
    OPENFILENAMEW ofn;
    LPWSTR c;
    WCHAR szFileName[MAX_PATH];
    WCHAR szFilterMask[MAX_STRING_LEN + 10];

    ZeroMemory(&szFilterMask, sizeof(szFilterMask));
    c = szFilterMask + LoadStringW(Globals.hInstance, STRING_FORMAT_GEN, szFilterMask, MAX_STRING_LEN) + 1;
    wcscpy(c, L"*.clp");

    ZeroMemory(&szFileName, sizeof(szFileName));
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = Globals.hMainWnd;
    ofn.hInstance = Globals.hInstance;
    ofn.lpstrFilter = szFilterMask;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = ARRAYSIZE(szFileName);
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

    if (!GetOpenFileNameW(&ofn))
        return;

    LoadClipboardDataFromFile(szFileName);
}

static void LoadClipboardFromDrop(HDROP hDrop)
{
    WCHAR szFileName[MAX_PATH];

    DragQueryFileW(hDrop, 0, szFileName, ARRAYSIZE(szFileName));
    DragFinish(hDrop);

    LoadClipboardDataFromFile(szFileName);
}

BOOL CALLBACK DoSetTextMode(LPCVOID pvText, SIZE_T cbText, BOOL bUnicode)
{
    RECT rc;

    Globals.bTextMode = TRUE;

    if (bUnicode)
        SetWindowTextW(Globals.hwndText, (LPCWSTR)pvText);
    else
        SetWindowTextA(Globals.hwndText, (LPCSTR)pvText);

    ShowScrollBar(Globals.hMainWnd, SB_BOTH, FALSE);

    GetClientRect(Globals.hMainWnd, &rc);
    MoveWindow(Globals.hwndText, 0, 0, rc.right, rc.bottom, TRUE);
    ShowWindowAsync(Globals.hwndText, SW_SHOWNORMAL);
    InvalidateRect(Globals.hMainWnd, NULL, TRUE);
    InvalidateRect(Globals.hwndText, NULL, TRUE);

    return TRUE;
}

static void SetDisplayFormat(UINT uFormat)
{
    RECT rc;

    CheckMenuItem(Globals.hMenu, Globals.uCheckedItem, MF_BYCOMMAND | MF_UNCHECKED);
    Globals.uCheckedItem = uFormat + CMD_AUTOMATIC;
    CheckMenuItem(Globals.hMenu, Globals.uCheckedItem, MF_BYCOMMAND | MF_CHECKED);

    if (uFormat == 0)
        uFormat = GetAutomaticClipboardFormat();

    Globals.uDisplayFormat = uFormat;

    Globals.bTextMode = FALSE;
    if (DoTextFromFormat(Globals.uDisplayFormat, DoSetTextMode))
        return;

    ShowWindowAsync(Globals.hwndText, SW_HIDE);
    ShowScrollBar(Globals.hMainWnd, SB_BOTH, TRUE);

    GetClipboardDataDimensions(Globals.uDisplayFormat, &rc);
    Scrollstate.CurrentX = Scrollstate.CurrentY = 0;
    Scrollstate.iWheelCarryoverX = Scrollstate.iWheelCarryoverY = 0;
    UpdateWindowScrollState(Globals.hMainWnd, rc.right, rc.bottom, &Scrollstate);

    InvalidateRect(Globals.hMainWnd, NULL, TRUE);
}

static void InitMenuPopup(HMENU hMenu, LPARAM index)
{
    if ((GetMenuItemID(hMenu, 0) == CMD_DELETE) || (GetMenuItemID(hMenu, 1) == CMD_SAVE_AS))
    {
        if (CountClipboardFormats() == 0)
        {
            EnableMenuItem(hMenu, CMD_DELETE, MF_GRAYED);
            EnableMenuItem(hMenu, CMD_SAVE_AS, MF_GRAYED);
        }
        else
        {
            EnableMenuItem(hMenu, CMD_DELETE, MF_ENABLED);
            EnableMenuItem(hMenu, CMD_SAVE_AS, MF_ENABLED);
        }
    }

    DrawMenuBar(Globals.hMainWnd);
}

static void UpdateDisplayMenu(void)
{
    UINT uFormat;
    HMENU hMenu;
    WCHAR szFormatName[MAX_FMT_NAME_LEN + 1];

    hMenu = GetSubMenu(Globals.hMenu, DISPLAY_MENU_POS);

    while (GetMenuItemCount(hMenu) > 1)
    {
        DeleteMenu(hMenu, 1, MF_BYPOSITION);
    }

    if (CountClipboardFormats() == 0)
        return;

    if (!OpenClipboard(Globals.hMainWnd))
        return;

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    /* Display the supported clipboard formats first */
    for (uFormat = EnumClipboardFormats(0); uFormat;
         uFormat = EnumClipboardFormats(uFormat))
    {
        if (IsClipboardFormatSupported(uFormat))
        {
            RetrieveClipboardFormatName(Globals.hInstance, uFormat, TRUE,
                                        szFormatName, ARRAYSIZE(szFormatName));
            AppendMenuW(hMenu, MF_STRING, CMD_AUTOMATIC + uFormat, szFormatName);
        }
    }

    /* Now display the unsupported clipboard formats */
    for (uFormat = EnumClipboardFormats(0); uFormat;
         uFormat = EnumClipboardFormats(uFormat))
    {
        if (!IsClipboardFormatSupported(uFormat))
        {
            RetrieveClipboardFormatName(Globals.hInstance, uFormat, TRUE,
                                        szFormatName, ARRAYSIZE(szFormatName));
            AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 0, szFormatName);
        }
    }

    CloseClipboard();
}

static int OnCommand(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
        case CMD_OPEN:
        {
            LoadClipboardFromFile();
            break;
        }

        case CMD_SAVE_AS:
        {
            SaveClipboardToFile();
            break;
        }

        case CMD_EXIT:
        {
            PostMessageW(Globals.hMainWnd, WM_CLOSE, 0, 0);
            break;
        }

        case CMD_DELETE:
        {
            if (MessageBoxRes(Globals.hMainWnd, Globals.hInstance,
                              STRING_DELETE_MSG, STRING_DELETE_TITLE,
                              MB_ICONWARNING | MB_YESNO) != IDYES)
            {
                break;
            }

            DeleteClipboardContent();
            break;
        }

        case CMD_AUTOMATIC:
        {
            SetDisplayFormat(0);
            break;
        }

        case CMD_HELP:
        {
            HtmlHelpW(Globals.hMainWnd, L"clipbrd.chm", 0, 0);
            break;
        }

        case CMD_ABOUT:
        {
            WCHAR szTitle[MAX_STRING_LEN];

            LoadStringW(Globals.hInstance, STRING_CLIPBOARD, szTitle, ARRAYSIZE(szTitle));
            ShellAboutW(Globals.hMainWnd, szTitle, NULL,
                        LoadIconW(Globals.hInstance, MAKEINTRESOURCEW(CLIPBRD_ICON)));
            break;
        }

        default:
        {
            break;
        }
    }
    return 0;
}

static void OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    COLORREF crOldBkColor, crOldTextColor;
    RECT rc;

    if (Globals.bTextMode)
    {
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        return;
    }

    if (!OpenClipboard(Globals.hMainWnd))
        return;

    hdc = BeginPaint(hWnd, &ps);

    /* Erase the background if needed */
    if (ps.fErase)
        FillRect(ps.hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

    /* Set the correct background and text colors */
    crOldBkColor   = SetBkColor(ps.hdc, GetSysColor(COLOR_WINDOW));
    crOldTextColor = SetTextColor(ps.hdc, GetSysColor(COLOR_WINDOWTEXT));

    /* Realize the clipboard palette if there is one */
    RealizeClipboardPalette(ps.hdc);

    switch (Globals.uDisplayFormat)
    {
        case CF_NONE:
        {
            /* The clipboard is empty */
            break;
        }

        case CF_DSPTEXT:
        case CF_TEXT:
        case CF_OEMTEXT:
        case CF_UNICODETEXT:
            /* Done in Globals.hwndText */
            break;

        case CF_DSPBITMAP:
        case CF_BITMAP:
        {
            BitBltFromClipboard(ps, Scrollstate, SRCCOPY);
            break;
        }

        case CF_DIB:
        case CF_DIBV5:
        {
            SetDIBitsToDeviceFromClipboard(Globals.uDisplayFormat, ps, Scrollstate, DIB_RGB_COLORS);
            break;
        }

        case CF_DSPMETAFILEPICT:
        case CF_METAFILEPICT:
        {
            GetClientRect(hWnd, &rc);
            PlayMetaFileFromClipboard(hdc, &rc);
            break;
        }

        case CF_DSPENHMETAFILE:
        case CF_ENHMETAFILE:
        {
            GetClientRect(hWnd, &rc);
            PlayEnhMetaFileFromClipboard(hdc, &rc);
            break;
        }

        // case CF_PALETTE:
            // TODO: Draw a palette with squares filled with colors.
            // break;

        case CF_OWNERDISPLAY:
        {
            HGLOBAL hglb;
            PPAINTSTRUCT pps;

            hglb = GlobalAlloc(GMEM_MOVEABLE, sizeof(ps));
            if (hglb)
            {
                pps = GlobalLock(hglb);
                CopyMemory(pps, &ps, sizeof(ps));
                GlobalUnlock(hglb);

                SendClipboardOwnerMessage(TRUE, WM_PAINTCLIPBOARD,
                                          (WPARAM)hWnd, (LPARAM)hglb);

                GlobalFree(hglb);
            }
            break;
        }

        case CF_HDROP:
            /* Done in Globals.hwndText */
            break;

        default:
        {
            WCHAR szText[256];
            GetClientRect(hWnd, &rc);
            LoadStringW(Globals.hInstance, ERROR_UNSUPPORTED_FORMAT, szText, _countof(szText));
            DoSetTextMode(szText, wcslen(szText) * sizeof(WCHAR), TRUE);
            break;
        }
    }

    /* Restore the original colors */
    SetTextColor(ps.hdc, crOldTextColor);
    SetBkColor(ps.hdc, crOldBkColor);

    EndPaint(hWnd, &ps);

    CloseClipboard();
}

static LRESULT WINAPI MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CREATE:
        {
            /* Create a text box */
            const DWORD style = WS_CHILD | ES_MULTILINE | ES_READONLY | WS_HSCROLL | WS_VSCROLL;
            Globals.hwndText = CreateWindowExW(WS_EX_CLIENTEDGE,
                                               L"EDIT",
                                               NULL,
                                               style,
                                               0, 0, 0, 0,
                                               hWnd,
                                               NULL,
                                               Globals.hInstance,
                                               NULL);
            if (!Globals.hwndText)
                return -1;

            Globals.hMenu = GetMenu(hWnd);
            Globals.hWndNext = SetClipboardViewer(hWnd);

            // For now, the Help dialog item is disabled because of lacking of HTML support
            EnableMenuItem(Globals.hMenu, CMD_HELP, MF_BYCOMMAND | MF_GRAYED);

            UpdateLinesToScroll(&Scrollstate);

            UpdateDisplayMenu();
            SetDisplayFormat(0);

            DragAcceptFiles(hWnd, TRUE);
            break;
        }

        case WM_CLOSE:
        {
            DestroyWindow(hWnd);
            break;
        }

        case WM_DESTROY:
        {
            ChangeClipboardChain(hWnd, Globals.hWndNext);

            if (Globals.uDisplayFormat == CF_OWNERDISPLAY)
            {
                HGLOBAL hglb;
                PRECT prc;

                hglb = GlobalAlloc(GMEM_MOVEABLE, sizeof(*prc));
                if (hglb)
                {
                    prc = GlobalLock(hglb);
                    SetRectEmpty(prc);
                    GlobalUnlock(hglb);

                    SendClipboardOwnerMessage(TRUE, WM_SIZECLIPBOARD,
                                              (WPARAM)hWnd, (LPARAM)hglb);

                    GlobalFree(hglb);
                }
            }

            PostQuitMessage(0);
            break;
        }

        case WM_PAINT:
        {
            OnPaint(hWnd, wParam, lParam);
            break;
        }

        case WM_KEYDOWN:
        {
            OnKeyScroll(hWnd, wParam, lParam, &Scrollstate);
            break;
        }

        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        {
            OnMouseScroll(hWnd, uMsg, wParam, lParam, &Scrollstate);
            break;
        }

        case WM_HSCROLL:
        {
            // NOTE: Windows uses an offset of 16 pixels
            OnScroll(hWnd, SB_HORZ, wParam, 5, &Scrollstate);
            break;
        }

        case WM_VSCROLL:
        {
            // NOTE: Windows uses an offset of 16 pixels
            OnScroll(hWnd, SB_VERT, wParam, 5, &Scrollstate);
            break;
        }

        case WM_SIZE:
        {
            RECT rc;

            if (Globals.bTextMode)
            {
                GetClientRect(hWnd, &rc);
                MoveWindow(Globals.hwndText, 0, 0, rc.right, rc.bottom, TRUE);
                ShowWindowAsync(Globals.hwndText, SW_SHOWNORMAL);
                break;
            }

            if (Globals.uDisplayFormat == CF_OWNERDISPLAY)
            {
                HGLOBAL hglb;
                PRECT prc;

                hglb = GlobalAlloc(GMEM_MOVEABLE, sizeof(*prc));
                if (hglb)
                {
                    prc = GlobalLock(hglb);
                    if (wParam == SIZE_MINIMIZED)
                        SetRectEmpty(prc);
                    else
                        GetClientRect(hWnd, prc);
                    GlobalUnlock(hglb);

                    SendClipboardOwnerMessage(TRUE, WM_SIZECLIPBOARD,
                                              (WPARAM)hWnd, (LPARAM)hglb);

                    GlobalFree(hglb);
                }
                break;
            }

            GetClipboardDataDimensions(Globals.uDisplayFormat, &rc);
            UpdateWindowScrollState(hWnd, rc.right, rc.bottom, &Scrollstate);
            break;
        }

        case WM_CHANGECBCHAIN:
        {
            /* Transmit through the clipboard viewer chain */
            if ((HWND)wParam == Globals.hWndNext)
            {
                Globals.hWndNext = (HWND)lParam;
            }
            else if (Globals.hWndNext != NULL)
            {
                SendMessageW(Globals.hWndNext, uMsg, wParam, lParam);
            }

            break;
        }

        case WM_DESTROYCLIPBOARD:
            break;

        case WM_RENDERALLFORMATS:
        {
            /*
             * When the user has cleared the clipboard via the DELETE command,
             * we (clipboard viewer) become the clipboard owner. When we are
             * subsequently closed, this message is then sent to us so that
             * we get a chance to render everything we can. Since we don't have
             * anything to render, just empty the clipboard.
             */
            DeleteClipboardContent();
            break;
        }

        case WM_RENDERFORMAT:
            // TODO!
            break;

        case WM_DRAWCLIPBOARD:
        {
            UpdateDisplayMenu();
            SetDisplayFormat(0);

            /* Pass the message to the next window in clipboard viewer chain */
            SendMessageW(Globals.hWndNext, uMsg, wParam, lParam);
            break;
        }

        case WM_COMMAND:
        {
            if ((LOWORD(wParam) > CMD_AUTOMATIC))
            {
                SetDisplayFormat(LOWORD(wParam) - CMD_AUTOMATIC);
            }
            else
            {
                OnCommand(hWnd, uMsg, wParam, lParam);
            }
            break;
        }

        case WM_INITMENUPOPUP:
        {
            InitMenuPopup((HMENU)wParam, lParam);
            break;
        }

        case WM_DROPFILES:
        {
            LoadClipboardFromDrop((HDROP)wParam);
            break;
        }

        case WM_PALETTECHANGED:
        {
            /* Ignore if this comes from ourselves */
            if ((HWND)wParam == hWnd)
                break;

            /* Fall back to WM_QUERYNEWPALETTE */
        }

        case WM_QUERYNEWPALETTE:
        {
            BOOL Success;
            HDC hDC;

            if (!OpenClipboard(Globals.hMainWnd))
                return FALSE;

            hDC = GetDC(hWnd);
            if (!hDC)
            {
                CloseClipboard();
                return FALSE;
            }

            Success = RealizeClipboardPalette(hDC);

            ReleaseDC(hWnd, hDC);
            CloseClipboard();

            if (Success)
            {
                InvalidateRect(hWnd, NULL, TRUE);
                UpdateWindow(hWnd);
                return TRUE;
            }
            return FALSE;
        }

        case WM_SYSCOLORCHANGE:
        {
            SetDisplayFormat(Globals.uDisplayFormat);
            break;
        }

        case WM_SETTINGCHANGE:
        {
            if (wParam == SPI_SETWHEELSCROLLLINES)
            {
                UpdateLinesToScroll(&Scrollstate);
            }
            break;
        }

        default:
        {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }

    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HACCEL hAccel;
    HWND hPrevWindow;
    WNDCLASSEXW wndclass;
    WCHAR szBuffer[MAX_STRING_LEN];

    hPrevWindow = FindWindowW(szClassName, NULL);
    if (hPrevWindow)
    {
        BringWindowToFront(hPrevWindow);
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

    InitGlobals(hInstance);

    ZeroMemory(&wndclass, sizeof(wndclass));
    wndclass.cbSize = sizeof(wndclass);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = MainWndProc;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(CLIPBRD_ICON));
    wndclass.hCursor = LoadCursorW(0, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wndclass.lpszMenuName = MAKEINTRESOURCEW(MAIN_MENU);
    wndclass.lpszClassName = szClassName;

    if (!RegisterClassExW(&wndclass))
    {
        ShowLastWin32Error(NULL);
        return 0;
    }

    ZeroMemory(&Scrollstate, sizeof(Scrollstate));

    LoadStringW(hInstance, STRING_CLIPBOARD, szBuffer, ARRAYSIZE(szBuffer));
    Globals.hMainWnd = CreateWindowExW(WS_EX_CLIENTEDGE | WS_EX_ACCEPTFILES,
                                       szClassName,
                                       szBuffer,
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
        ShowLastWin32Error(NULL);
        return 0;
    }

    ShowWindow(Globals.hMainWnd, nCmdShow);
    UpdateWindow(Globals.hMainWnd);

    hAccel = LoadAcceleratorsW(Globals.hInstance, MAKEINTRESOURCEW(ID_ACCEL));
    if (!hAccel)
    {
        ShowLastWin32Error(Globals.hMainWnd);
    }

    /* If the user provided a path to a clipboard data file, try to open it */
    if (__argc >= 2)
        LoadClipboardDataFromFile(__wargv[1]);

    while (GetMessageW(&msg, 0, 0, 0))
    {
        if (!TranslateAcceleratorW(Globals.hMainWnd, hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return (int)msg.wParam;
}
