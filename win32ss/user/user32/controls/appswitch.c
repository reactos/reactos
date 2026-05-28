/*
 * PROJECT:     ReactOS user32.dll
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     App switching functionality
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald <johannes.anderwald@reactos.org>
 *              Copyright 2011 David Quintana <gigaherz@gmail.com>
 *              Copyright 2011-2016 James Tabor <james.tabor@reactos.org>
 *              Copyright 2016-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

// TODO: Move to Win32k

#include <user32.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

#define DIALOG_MARGIN   8       // Margin of dialog contents

#define CX_ICON         32      // Width of icon
#define CY_ICON         32      // Height of icon
#define ICON_MARGIN     4       // Margin width around an icon

#define CX_ITEM         (CX_ICON + 2 * ICON_MARGIN)
#define CY_ITEM         (CY_ICON + 2 * ICON_MARGIN)
#define ITEM_MARGIN     4       // Margin width around an item

#define CX_ITEM_SPACE   (CX_ITEM + 2 * ITEM_MARGIN)
#define CY_ITEM_SPACE   (CY_ITEM + 2 * ITEM_MARGIN)

#define CY_TEXT_MARGIN  4       // Margin height around text

/* Limit the number of windows shown in the Alt-Tab dialog.
 * 120 windows result in (12*40) by (10*40) pixels worth of icons. */
#define MAX_WINDOWS 120

/* Global variables */
HWND g_hSwitchDlg = NULL;
HFONT g_hDlgFont;
int fontHeight = 0;

int selectedWindow = 0;
int nShift = 0;
BOOL bIsOpen = FALSE;
BOOL bEsc = FALSE;

WCHAR windowText[1024];

HWND windowList[MAX_WINDOWS];
HICON iconList[MAX_WINDOWS];
int windowCount = 0;

int cxBorder, cyBorder;
int nItems, nCols, nRows;
int itemsW, itemsH;
int totalW, totalH;
int xOffset, yOffset;
POINT ptStart;

enum { DefSwitchRows = 3, DefSwitchColumns = 7 };
BOOL CoolSwitch = TRUE;
int CoolSwitchRows = DefSwitchRows;
int CoolSwitchColumns = DefSwitchColumns;

/* Switch Window styles */
const DWORD g_Style = WS_POPUP | WS_BORDER | WS_DISABLED;
const DWORD g_ExStyle = WS_EX_TOPMOST | WS_EX_DLGMODALFRAME | WS_EX_TOOLWINDOW;

static int GetRegInt(HKEY hKey, PCWSTR Name, int DefVal)
{
    WCHAR buf[sizeof("-2147483648")];
    DWORD cb = sizeof(buf), type;
    DWORD err = RegQueryValueExW(hKey, Name, NULL, &type, (BYTE*)buf, &cb);
    if (err == ERROR_SUCCESS && cb <= sizeof(buf) - sizeof(*buf))
    {
        buf[cb / sizeof(*buf)] = UNICODE_NULL;
        if (type == REG_SZ || type == REG_EXPAND_SZ)
        {
            WCHAR *pszEnd;
            long Value = wcstol(buf, &pszEnd, 10);
            return pszEnd > buf ? Value : DefVal;
        }
        if ((type == REG_DWORD || type == REG_BINARY) && cb == sizeof(DWORD))
        {
            return *(DWORD*)buf;
        }
    }
    return DefVal;
}

static void LoadCoolSwitchSettings(void)
{
    static BOOL SettingsLoaded = FALSE;
    HKEY hKey;

    if (SettingsLoaded
#if DBG
        && !(GetKeyState(VK_SCROLL) & 1) // If Scroll-Lock is on, always read the settings
#endif
        )
    {
        return;
    }

    SettingsLoaded = TRUE;
    // TODO: Should read from win.ini instead when IniFileMapping is implemented
    if (!RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hKey))
    {
        CoolSwitch = GetRegInt(hKey, L"CoolSwitch", TRUE);
        CoolSwitchRows = GetRegInt(hKey, L"CoolSwitchRows", DefSwitchRows);
        CoolSwitchColumns = GetRegInt(hKey, L"CoolSwitchColumns", DefSwitchColumns);
        RegCloseKey(hKey);
    }

    if (CoolSwitchRows * CoolSwitchColumns < 3)
    {
        CoolSwitchRows = DefSwitchRows;
        CoolSwitchColumns = DefSwitchColumns;
    }

    TRACE("CoolSwitch: %d\n", CoolSwitch);
    TRACE("CoolSwitchRows: %d\n", CoolSwitchRows);
    TRACE("CoolSwitchColumns: %d\n", CoolSwitchColumns);
}

void ResizeAndCenter(HWND hwnd, int width, int height)
{
    const int screenwidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenheight = GetSystemMetrics(SM_CYSCREEN);

    int x, y;
    RECT Rect;

    x = (screenwidth - width) / 2;
    y = (screenheight - height) / 2;

    SetRect(&Rect, x, y, x + width, y + height);
    AdjustWindowRectEx(&Rect, g_Style, FALSE, g_ExStyle);

    x = Rect.left;
    y = Rect.top;
    width = Rect.right - Rect.left;
    height = Rect.bottom - Rect.top;
    MoveWindow(hwnd, x, y, width, height, FALSE);

    ptStart.x = x;
    ptStart.y = y;
}

void CompleteSwitch(BOOL doSwitch)
{
    if (!bIsOpen)
        return;

    bIsOpen = FALSE;

    TRACE("[ATbot] CompleteSwitch Hiding window\n");
    ShowWindowAsync(g_hSwitchDlg, SW_HIDE);

    if (doSwitch)
    {
        if (selectedWindow >= windowCount)
            return;

        // FIXME: workaround because ReactOS fails to activate the previous window correctly.
        //if (selectedWindow != 0)
        {
            HWND hwnd = windowList[selectedWindow];
            GetWindowTextW(hwnd, windowText, _countof(windowText));
            TRACE("[ATbot] CompleteSwitch Switching to 0x%08x (%ls)\n", hwnd, windowText);
            SwitchToThisWindow(hwnd, TRUE);
        }
    }

    windowCount = 0;
}

BOOL CALLBACK EnumerateCallback(HWND window, LPARAM lParam)
{
    HICON hIcon = NULL;
    LRESULT bAlive;

    UNREFERENCED_PARAMETER(lParam);

    /* First try to get the big icon assigned to the window */
#define ICON_TIMEOUT 100 // in milliseconds
    bAlive = SendMessageTimeoutW(window, WM_GETICON, ICON_BIG, 0,
                                 SMTO_ABORTIFHUNG | SMTO_BLOCK, ICON_TIMEOUT,
                                 (PDWORD_PTR)&hIcon);
    if (!hIcon)
    {
        /* If no icon is assigned, try to get the window's class icon */
        hIcon = (HICON)GetClassLongPtrW(window, GCL_HICON);
        if (!hIcon)
        {
            /* If we still don't have an icon, see if we can do with
             * the small icon, or a default application icon */
            if (bAlive)
            {
                SendMessageTimeoutW(window, WM_GETICON, ICON_SMALL2, 0,
                                    SMTO_ABORTIFHUNG | SMTO_BLOCK, ICON_TIMEOUT,
                                    (PDWORD_PTR)&hIcon);
            }
#undef ICON_TIMEOUT
            if (!hIcon)
            {
                /* Use the ReactOS logo icon as default */
                hIcon = gpsi->hIconWindows;
                if (!hIcon)
                {
                    /* If all attempts getting an icon fails, go to the next window */
                    return TRUE;
                }
            }
        }
    }

    windowList[windowCount] = window;
    iconList[windowCount] = CopyIcon(hIcon);
    windowCount++;

    /* If we got to the max number of windows, we won't be able to add more */
    return (windowCount < MAX_WINDOWS);
}

static HWND GetNiceRootOwner(HWND hwnd)
{
    HWND hwndOwner;
    DWORD ExStyle, OwnerExStyle;

    for (;;)
    {
        /* A window with WS_EX_APPWINDOW is treated as if it has no owner */
        ExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        if (ExStyle & WS_EX_APPWINDOW)
            break;

        /* Is the owner visible?
         * A window with WS_EX_TOOLWINDOW is treated as if it isn't visible */
        hwndOwner = GetWindow(hwnd, GW_OWNER);
        OwnerExStyle = GetWindowLong(hwndOwner, GWL_EXSTYLE);
        if (!IsWindowVisible(hwndOwner) || (OwnerExStyle & WS_EX_TOOLWINDOW))
            break;

        hwnd = hwndOwner;
    }

    return hwnd;
}

// See: https://devblogs.microsoft.com/oldnewthing/20071008-00/?p=24863
BOOL IsAltTabWindow(HWND hwnd)
{
    LONG_PTR ExStyle, ClassStyle;
    RECT rc;
    HWND hwndTry, hwndWalk;
    WCHAR szClass[64];

    /* Must be visible */
    if (!IsWindowVisible(hwnd))
        return FALSE;

    /* Must not be WS_EX_TOOLWINDOW nor WS_EX_NOACTIVATE */
    ExStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (ExStyle & (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE))
        return FALSE;

    /* Must be not empty rect */
    GetWindowRect(hwnd, &rc);
    if (IsRectEmpty(&rc))
        return FALSE;

    /* Check special windows */
    if (!GetClassNameW(hwnd, szClass, _countof(szClass)) ||
        wcscmp(szClass, L"Shell_TrayWnd") == 0 ||
        wcscmp(szClass, L"Progman") == 0)
    {
        return TRUE;
    }

    /* Must not be an IME-related window */
    ClassStyle = GetClassLongPtrW(hwnd, GCL_STYLE);
    if (ClassStyle & CS_IME)
        return FALSE;

    /* Get a 'nice' root owner */
    hwndWalk = GetNiceRootOwner(hwnd);

    /* Walk back from hwndWalk toward hwnd */
    for (;;)
    {
        hwndTry = GetLastActivePopup(hwndWalk);
        if (hwndTry == hwndWalk)
            break;

        ExStyle = GetWindowLong(hwndTry, GWL_EXSTYLE);
        if (IsWindowVisible(hwndTry) && !(ExStyle & WS_EX_TOOLWINDOW))
            break;

        hwndWalk = hwndTry;
    }

    return (hwnd == hwndTry); // Success if reached.
}

static BOOL CALLBACK
EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (IsAltTabWindow(hwnd))
    {
        if (!EnumerateCallback(hwnd, lParam))
            return FALSE;
    }
    return TRUE;
}

void ProcessMouseMessage(UINT message, LPARAM lParam)
{
    int xPos = LOWORD(lParam);
    int yPos = HIWORD(lParam);

    int xIndex = (xPos - DIALOG_MARGIN) / CX_ITEM_SPACE;
    int yIndex = (yPos - DIALOG_MARGIN) / CY_ITEM_SPACE;

    if (xIndex < 0 || nCols <= xIndex ||
        yIndex < 0 || nRows <= yIndex)
    {
        return;
    }

    selectedWindow = (yIndex*nCols) + xIndex;
    if (message == WM_MOUSEMOVE)
    {
        InvalidateRect(g_hSwitchDlg, NULL, TRUE);
        //RedrawWindow(g_hSwitchDlg, NULL, NULL, 0);
    }
    else
    {
        CompleteSwitch(TRUE);
    }
}

void OnPaint(HWND hWnd)
{
    HDC dialogDC;
    PAINTSTRUCT paint;
    RECT cRC, textRC;
    int i, xPos, yPos, CharCount;
    HFONT dcFont;

    /* Don't show anything if there are no items to show */
    if (nCols == 0 || nItems == 0)
        return;

    dialogDC = BeginPaint(hWnd, &paint);
    if (dialogDC == NULL)
        return;

    /* Fill the client area */
    GetClientRect(hWnd, &cRC);
    FillRect(dialogDC, &cRC, (HBRUSH)(COLOR_3DFACE + 1));

    /* If the selection index exceeds the displayed items, shift them */
    if (selectedWindow >= nItems)
        nShift = selectedWindow - nItems + 1;
    else
        nShift = 0;

    for (i = 0; i < nItems; ++i)
    {
        /* Get the icon to display */
        HICON hIcon = iconList[i + nShift];

        /* Calculate the position where we start drawing */
        xPos = DIALOG_MARGIN + CX_ITEM_SPACE * (i % nCols) + ITEM_MARGIN;
        yPos = DIALOG_MARGIN + CY_ITEM_SPACE * (i / nCols) + ITEM_MARGIN;

        /* Centering */
        if (nItems < CoolSwitchColumns)
        {
            xPos += (itemsW - nItems * CX_ITEM_SPACE) / 2;
        }

        /* If this position is selected... */
        if (selectedWindow == i + nShift)
        {
            /* ... draw a rectangle using the highlighting pen */
            HPEN hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_HIGHLIGHT));

            SelectObject(dialogDC, hPen);
            SelectObject(dialogDC, GetStockObject(NULL_BRUSH));
            Rectangle(dialogDC, xPos, yPos, xPos + CX_ITEM, yPos + CY_ITEM);
            Rectangle(dialogDC, xPos + 1, yPos + 1,
                                xPos + CX_ITEM - 1, yPos + CY_ITEM - 1);

            DeleteObject(hPen);
        }

        /* Draw the icon */
        DrawIconEx(dialogDC, xPos + ICON_MARGIN, yPos + ICON_MARGIN,
                   hIcon, CX_ICON, CY_ICON, 0, NULL, DI_NORMAL);
    }

    /* Set the text rectangle */
    SetRect(&textRC, DIALOG_MARGIN, DIALOG_MARGIN + itemsH,
            totalW - DIALOG_MARGIN, totalH - DIALOG_MARGIN);

    /* Draw the sunken button around the text */
    DrawFrameControl(dialogDC, &textRC, DFC_BUTTON,
                     DFCS_BUTTONPUSH | DFCS_PUSHED);

    /* Retrieve the window text and draw it */
    CharCount = GetWindowTextW(windowList[selectedWindow],
                               windowText, _countof(windowText));

    dcFont = SelectObject(dialogDC, g_hDlgFont);
    SetTextColor(dialogDC, GetSysColor(COLOR_BTNTEXT));
    SetBkMode(dialogDC, TRANSPARENT);
    DrawTextW(dialogDC, windowText, CharCount, &textRC,
              DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE);
    SelectObject(dialogDC, dcFont);

    EndPaint(hWnd, &paint);
}

BOOL CreateSwitcherWindow(HINSTANCE hInstance)
{
    g_hSwitchDlg = CreateWindowExW(WS_EX_TOPMOST|WS_EX_DLGMODALFRAME|WS_EX_TOOLWINDOW,
                                   WC_SWITCH,
                                   L"",
                                   WS_POPUP|WS_BORDER|WS_DISABLED,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   400, 150,
                                   NULL, NULL,
                                   hInstance, NULL);
    if (!g_hSwitchDlg)
    {
        TRACE("[ATbot] Task Switcher Window failed to create\n");
        return FALSE;
    }

    bIsOpen = FALSE;
    return TRUE;
}

BOOL GetDialogFont(void)
{
    HDC tDC;
    TEXTMETRICW tm;

    g_hDlgFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!g_hDlgFont)
        return FALSE;

    tDC = GetDC(NULL);
    GetTextMetricsW(tDC, &tm);
    fontHeight = tm.tmHeight;
    ReleaseDC(NULL, tDC);

    return TRUE;
}

void PrepareWindow(void)
{
    nItems = windowCount;

    nCols = CoolSwitchColumns;
    nRows = (nItems + CoolSwitchColumns - 1) / CoolSwitchColumns;
    if (nRows > CoolSwitchRows)
    {
        nRows = CoolSwitchRows;
        nItems = nRows * nCols;
    }

    itemsW = nCols * CX_ITEM_SPACE;
    itemsH = nRows * CY_ITEM_SPACE;

    totalW = itemsW + 2 * DIALOG_MARGIN;
    totalH = itemsH + 2 * DIALOG_MARGIN;
    totalH += fontHeight + 2 * CY_TEXT_MARGIN;

    ResizeAndCenter(g_hSwitchDlg, totalW, totalH);
}

BOOL ProcessHotKey(void)
{
    if (!bIsOpen)
    {
        windowCount = 0;
        EnumWindows(EnumWindowsProc, 0);

        if (windowCount == 0)
            return FALSE;

        if (windowCount == 1)
        {
            SwitchToThisWindow(windowList[0], TRUE);
            return FALSE;
        }

        if (!CreateSwitcherWindow(User32Instance))
            return FALSE;

        selectedWindow = 1;

        TRACE("[ATbot] HotKey received - Opening window\n");
        ShowWindowAsync(g_hSwitchDlg, SW_SHOWNORMAL);
        SwitchToThisWindow(g_hSwitchDlg, TRUE);
        bIsOpen = TRUE;
    }
    else
    {
        TRACE("[ATbot] HotKey received - Rotating\n");
        selectedWindow = (selectedWindow + 1) % windowCount;
        InvalidateRect(g_hSwitchDlg, NULL, TRUE);
    }
    return TRUE;
}

void RotateTasks(BOOL bShift)
{
    HWND hwndFirst, hwndLast;
    DWORD Size;

    if (windowCount < 2 || !bEsc)
        return;

    hwndFirst = windowList[0];
    hwndLast = windowList[windowCount - 1];

    if (bShift)
    {
        SetWindowPos(hwndLast, HWND_TOP, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                     SWP_NOOWNERZORDER | SWP_NOREPOSITION | SWP_ASYNCWINDOWPOS);

        SwitchToThisWindow(hwndLast, TRUE);

        Size = (windowCount - 1) * sizeof(HWND);
        MoveMemory(&windowList[1], &windowList[0], Size);
        windowList[0] = hwndLast;
    }
    else
    {
        SetWindowPos(hwndFirst, hwndLast, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                     SWP_NOOWNERZORDER | SWP_NOREPOSITION | SWP_ASYNCWINDOWPOS);

        SwitchToThisWindow(windowList[1], TRUE);

        Size = (windowCount - 1) * sizeof(HWND);
        MoveMemory(&windowList[0], &windowList[1], Size);
        windowList[windowCount - 1] = hwndFirst;
    }
}

static void MoveLeft(void)
{
    selectedWindow = selectedWindow - 1;
    if (selectedWindow < 0)
        selectedWindow = windowCount - 1;
    InvalidateRect(g_hSwitchDlg, NULL, TRUE);
}

static void MoveRight(void)
{
    selectedWindow = (selectedWindow + 1) % windowCount;
    InvalidateRect(g_hSwitchDlg, NULL, TRUE);
}

static void MoveUp(void)
{
    INT iRow = selectedWindow / nCols;
    INT iCol = selectedWindow % nCols;

    --iRow;
    if (iRow < 0)
        iRow = nRows - 1;

    selectedWindow = iRow * nCols + iCol;
    if (selectedWindow >= windowCount)
        selectedWindow = windowCount - 1;
    InvalidateRect(g_hSwitchDlg, NULL, TRUE);
}

static void MoveDown(void)
{
    INT iRow = selectedWindow / nCols;
    INT iCol = selectedWindow % nCols;

    ++iRow;
    if (iRow >= nRows)
        iRow = 0;

    selectedWindow = iRow * nCols + iCol;
    if (selectedWindow >= windowCount)
        selectedWindow = windowCount - 1;
    InvalidateRect(g_hSwitchDlg, NULL, TRUE);
}

void DestroyAppWindows(void)
{
    /* Loop over every icon list item, and destroy the icon */
    INT i;
    for (i = 0; i < windowCount; ++i)
    {
        DestroyIcon(iconList[i]);
        iconList[i] = NULL;
    }
}

LRESULT WINAPI DoAppSwitch(WPARAM wParam, LPARAM lParam)
{
    HWND hwndActive;
    MSG msg;

    // FIXME: Is loading timing OK?
    LoadCoolSwitchSettings();

    if (!CoolSwitch)
        return 0;

    /* Do nothing if we are already in the loop */
    if (g_hSwitchDlg || bEsc)
        return 0;

    if (lParam == VK_ESCAPE)
    {
        bEsc = TRUE;

        windowCount = 0;
        EnumWindows(EnumWindowsProc, 0);

        if (windowCount < 2)
            return 0;

        RotateTasks(GetAsyncKeyState(VK_SHIFT) < 0);

        hwndActive = GetActiveWindow();
        if (!hwndActive)
        {
            bEsc = FALSE;
            return 0;
        }
    }

    // NOTE: Introduced in commit 958cc23088 (r63531), and massaged in commit c17a8770a3 (PR #1718)
    /* Capture current active window */
    hwndActive = GetActiveWindow();
    if (hwndActive)
        SetCapture(hwndActive);

    switch (lParam)
    {
    case VK_TAB:
        if (!GetDialogFont() || !ProcessHotKey())
            goto Exit;
        break;

    case VK_ESCAPE:
        break;

    default:
        goto Exit;
    }

// Disabled: If we have visible, possibly minimized, windows, but none active (for some reason...),
// still allow the Alt+Tab window to show up so as to switch to a window and make it active.
// Otherwise ReactOS could stay "blocked" without any window being able to become active.
#if 0 // Well, it _seems_ windows still does something a bit similar...
    // NOTE: Introduced in commit 958cc23088 (r63531), and massaged in commit c17a8770a3 (PR #1718)
    if (!hwndActive)
        goto Exit;
#endif

    /* Main message loop */
    for (;;)
    {
        for (;;)
        {
            if (PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE))
            {
                if (!CallMsgFilterW(&msg, MSGF_NEXTWINDOW))
                    break;
                /* Remove the message from the queue */
                PeekMessageW(&msg, NULL, msg.message, msg.message, PM_REMOVE);
            }
            else
            {
                WaitMessage();
            }
        }

        switch (msg.message)
        {
        case WM_KEYUP:
        {
            PeekMessageW(&msg, NULL, msg.message, msg.message, PM_REMOVE);
            if (msg.wParam == VK_MENU)
            {
                CompleteSwitch(TRUE);
            }
            else if (msg.wParam == VK_RETURN)
            {
                CompleteSwitch(TRUE);
            }
            else if (msg.wParam == VK_ESCAPE)
            {
                TRACE("DoAppSwitch VK_ESCAPE 2\n");
                CompleteSwitch(FALSE);
            }
            goto Exit;
        }

        case WM_SYSKEYDOWN:
        {
            PeekMessageW(&msg, NULL, msg.message, msg.message, PM_REMOVE);
            if (HIWORD(msg.lParam) & KF_ALTDOWN)
            {
                if (msg.wParam == VK_TAB)
                {
                    if (bEsc) break;
                    if (GetKeyState(VK_SHIFT) < 0)
                        MoveLeft();
                    else
                        MoveRight();
                }
                else if (msg.wParam == VK_ESCAPE)
                {
                    if (!bEsc) break;
                    RotateTasks(GetKeyState(VK_SHIFT) < 0);
                }
                else if (msg.wParam == VK_LEFT)
                {
                    MoveLeft();
                }
                else if (msg.wParam == VK_RIGHT)
                {
                    MoveRight();
                }
                else if (msg.wParam == VK_UP)
                {
                    MoveUp();
                }
                else if (msg.wParam == VK_DOWN)
                {
                    MoveDown();
                }
            }
            break;
        }

        case WM_LBUTTONUP:
            PeekMessageW(&msg, NULL, msg.message, msg.message, PM_REMOVE);
            ProcessMouseMessage(msg.message, msg.lParam);
            goto Exit;

        default:
            if (PeekMessageW(&msg, NULL, msg.message, msg.message, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            break;
        }
    }

Exit:
    ReleaseCapture();
    if (g_hSwitchDlg)
        DestroyWindow(g_hSwitchDlg);
    g_hSwitchDlg = NULL;
    if (bEsc)
        DestroyAppWindows();
    bEsc = FALSE;
    windowCount = 0;
    selectedWindow = 0;
    return 0;
}

/**
 * @brief   Switch System Class window procedure.
 **/
LRESULT WINAPI SwitchWndProc_common(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL unicode)
{
    PWND pWnd;
    PALTTABINFO ati;

    pWnd = ValidateHwnd(hWnd);
    if (pWnd && !pWnd->fnid)
        NtUserSetWindowFNID(hWnd, FNID_SWITCH);

    switch (uMsg)
    {
    case WM_NCCREATE:
        ati = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ati));
        if (!ati)
            return 0;
        SetWindowLongPtrW(hWnd, 0, (LONG_PTR)ati);
        return TRUE;

    case WM_SHOWWINDOW:
        if (wParam)
        {
            PrepareWindow();
            ati = (PALTTABINFO)GetWindowLongPtrW(hWnd, 0);
            ati->cbSize = sizeof(*ati);
            ati->cItems = nItems;
            ati->cColumns = nCols;
            ati->cRows = nRows;
            if (nCols)
            {
                ati->iColFocus = (selectedWindow - nShift) % nCols;
                ati->iRowFocus = (selectedWindow - nShift) / nCols;
            }
            else
            {
                ati->iColFocus = 0;
                ati->iRowFocus = 0;
            }
            ati->cxItem = CX_ITEM_SPACE;
            ati->cyItem = CY_ITEM_SPACE;
            ati->ptStart = ptStart;
        }
        return 0;

    case WM_MOUSEMOVE:
        ProcessMouseMessage(uMsg, lParam);
        return 0;

    case WM_ACTIVATE:
        if (wParam == WA_INACTIVE)
            CompleteSwitch(FALSE);
        return 0;

    case WM_PAINT:
        OnPaint(hWnd);
        return 0;

    case WM_DESTROY:
        bIsOpen = FALSE;
        ati = (PALTTABINFO)GetWindowLongPtrW(hWnd, 0);
        HeapFree(GetProcessHeap(), 0, ati);
        SetWindowLongPtrW(hWnd, 0, 0);
        DestroyAppWindows();
        NtUserSetWindowFNID(hWnd, FNID_DESTROY);
        return 0;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT WINAPI SwitchWndProcA(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return SwitchWndProc_common(hWnd, uMsg, wParam, lParam, FALSE);
}

LRESULT WINAPI SwitchWndProcW(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return SwitchWndProc_common(hWnd, uMsg, wParam, lParam, TRUE);
}
