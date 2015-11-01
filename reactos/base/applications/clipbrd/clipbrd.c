/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Clipboard Viewer
 * FILE:            base/applications/clipbrd/clipbrd.c
 * PURPOSE:         Provides a view of the contents of the ReactOS clipboard.
 * PROGRAMMERS:     Ricardo Hanke
 */

#include "precomp.h"

static const WCHAR szClassName[] = L"ClipBookWClass";

CLIPBOARD_GLOBALS Globals;

static void SetDisplayFormat(UINT uFormat)
{
    CheckMenuItem(Globals.hMenu, Globals.uCheckedItem, MF_BYCOMMAND | MF_UNCHECKED);
    Globals.uCheckedItem = uFormat + CMD_AUTOMATIC;
    CheckMenuItem(Globals.hMenu, Globals.uCheckedItem, MF_BYCOMMAND | MF_CHECKED);

    if (uFormat == 0)
    {
        Globals.uDisplayFormat = GetAutomaticClipboardFormat();
    }
    else
    {
        Globals.uDisplayFormat =  uFormat;
    }

    InvalidateRect(Globals.hMainWnd, NULL, TRUE);
    UpdateWindow(Globals.hMainWnd);
}

static void InitMenuPopup(HMENU hMenu, LPARAM index)
{
    if (GetMenuItemID(hMenu, 0) == CMD_DELETE)
    {
        if (CountClipboardFormats() == 0)
        {
            EnableMenuItem(hMenu, CMD_DELETE, MF_GRAYED);
        }
        else
        {
            EnableMenuItem(hMenu, CMD_DELETE, MF_ENABLED);
        }
    }

    DrawMenuBar(Globals.hMainWnd);
}

void UpdateDisplayMenu(void)
{
    UINT uFormat;
    WCHAR szFormatName[MAX_STRING_LEN];
    HMENU hMenu;

    hMenu = GetSubMenu(Globals.hMenu, DISPLAY_MENU_POS);

    while (GetMenuItemCount(hMenu) > 1)
    {
        DeleteMenu(hMenu, 1, MF_BYPOSITION);
    }

    if (CountClipboardFormats() == 0)
        return;

    if (!OpenClipboard(NULL))
        return;

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    uFormat = EnumClipboardFormats(0);
    while (uFormat)
    {
        RetrieveClipboardFormatName(Globals.hInstance, uFormat, szFormatName, ARRAYSIZE(szFormatName));

        if (!IsClipboardFormatSupported(uFormat))
        {
            AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 0, szFormatName);
        }
        else
        {
            AppendMenuW(hMenu, MF_STRING, CMD_AUTOMATIC + uFormat, szFormatName);
        }

        uFormat = EnumClipboardFormats(uFormat);
    }

    CloseClipboard();
}

static int ClipboardCommandHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
        case CMD_EXIT:
        {
            PostMessageW(Globals.hMainWnd, WM_CLOSE, 0, 0);
            break;
        }

        case CMD_DELETE:
        {
            if (MessageBoxRes(Globals.hMainWnd, Globals.hInstance, STRING_DELETE_MSG, STRING_DELETE_TITLE, MB_ICONWARNING | MB_YESNO) == IDYES)
            {
                DeleteClipboardContent();
            }
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
            ShellAboutW(Globals.hMainWnd, szTitle, 0, NULL);
            break;
        }

        default:
        {
            break;
        }
    }
    return 0;
}

static void ClipboardPaintHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rc;

    hdc = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rc);

    switch (Globals.uDisplayFormat)
    {
        case CF_NONE:
        {
            break;
        }

        case CF_UNICODETEXT:
        {
            DrawTextFromClipboard(hdc, &rc, DT_LEFT);
            break;
        }

        case CF_BITMAP:
        {
            BitBltFromClipboard(hdc, rc.left, rc.top, rc.right, rc.bottom, 0, 0, SRCCOPY);
            break;
        }

        case CF_DIB:
        {
            SetDIBitsToDeviceFromClipboard(CF_DIB, hdc, rc.left, rc.top, 0, 0, 0, DIB_RGB_COLORS);
            break;
        }

        case CF_DIBV5:
        {
            SetDIBitsToDeviceFromClipboard(CF_DIBV5, hdc, rc.left, rc.top, 0, 0, 0, DIB_RGB_COLORS);
            break;
        }

        case CF_ENHMETAFILE:
        {
            PlayEnhMetaFileFromClipboard(hdc, &rc);
            break;
        }

        case CF_METAFILEPICT:
        {
            PlayMetaFileFromClipboard(hdc, &rc);
            break;
        }

        default:
        {

            DrawTextFromResource(Globals.hInstance, ERROR_UNSUPPORTED_FORMAT, hdc, &rc, DT_CENTER | DT_WORDBREAK);
            break;
        }
    }

    EndPaint(hWnd, &ps);
}

static LRESULT WINAPI MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_PAINT:
        {
            ClipboardPaintHandler(hWnd, uMsg, wParam, lParam);
            break;
        }

        case WM_SIZE:
        {
            InvalidateRect(hWnd, NULL, TRUE);
            UpdateWindow(hWnd);
            break;
        }

        case WM_CREATE:
        {
            Globals.hMenu = GetMenu(hWnd);
            Globals.hWndNext = SetClipboardViewer(hWnd);
            UpdateDisplayMenu();
            SetDisplayFormat(0);
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
            PostQuitMessage(0);
            break;
        }

        case WM_CHANGECBCHAIN:
        {
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

        case WM_DRAWCLIPBOARD:
        {
            UpdateDisplayMenu();
            SetDisplayFormat(0);

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
                ClipboardCommandHandler(hWnd, uMsg, wParam, lParam);
            }
            break;
        }

        case WM_INITMENUPOPUP:
        {
            InitMenuPopup((HMENU)wParam, lParam);
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

    ZeroMemory(&Globals, sizeof(Globals));
    Globals.hInstance = hInstance;

    ZeroMemory(&wndclass, sizeof(wndclass));
    wndclass.cbSize = sizeof(wndclass);
    wndclass.lpfnWndProc = MainWndProc;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(CLIP_ICON));
    wndclass.hCursor = LoadCursorW(0, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndclass.lpszMenuName = MAKEINTRESOURCEW(MAIN_MENU);
    wndclass.lpszClassName = szClassName;

    if (!RegisterClassExW(&wndclass))
    {
        ShowLastWin32Error(NULL);
        return 0;
    }

    LoadStringW(hInstance, STRING_CLIPBOARD, szBuffer, ARRAYSIZE(szBuffer));
    Globals.hMainWnd = CreateWindowExW(WS_EX_CLIENTEDGE,
                                       szClassName,
                                       szBuffer,
                                       WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
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
