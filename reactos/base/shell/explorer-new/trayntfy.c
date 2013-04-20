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
//#include <docobj.h>

/*
 * SysPagerWnd
 */
static const TCHAR szSysPagerWndClass[] = TEXT("SysPager");

typedef struct _NOTIFY_ITEM
{
    struct _NOTIFY_ITEM *next;
    INT Index;
    INT IconIndex;
    NOTIFYICONDATA iconData;
} NOTIFY_ITEM, *PNOTIFY_ITEM, **PPNOTIFY_ITEM;

typedef struct _SYS_PAGER_DATA
{
    HWND hWnd;
    HWND hWndToolbar;
    HIMAGELIST SysIcons;
    PNOTIFY_ITEM NotifyItems;
    INT ButtonCount;
    INT VisibleButtonCount;
} SYS_PAGER_WND_DATA, *PSYS_PAGER_WND_DATA;


static PNOTIFY_ITEM
SysPagerWnd_CreateNotifyItemData(IN OUT PSYS_PAGER_WND_DATA This)
{
    PNOTIFY_ITEM *findNotifyPointer = &This->NotifyItems;
    PNOTIFY_ITEM notifyItem;

    notifyItem = HeapAlloc(hProcessHeap,
                           HEAP_ZERO_MEMORY,
                           sizeof(*notifyItem));
    if (notifyItem == NULL)
        return NULL;

    notifyItem->next = NULL;

    while (*findNotifyPointer != NULL)
    {
        findNotifyPointer = &(*findNotifyPointer)->next;
    }

    *findNotifyPointer = notifyItem;

    return notifyItem;
}

static PPNOTIFY_ITEM
SysPagerWnd_FindPPNotifyItemByIconData(IN OUT PSYS_PAGER_WND_DATA This,
                                       IN CONST NOTIFYICONDATA *iconData)
{
    PPNOTIFY_ITEM findNotifyPointer = &This->NotifyItems;

    while (*findNotifyPointer != NULL)
    {
        if ((*findNotifyPointer)->iconData.hWnd == iconData->hWnd &&
            (*findNotifyPointer)->iconData.uID == iconData->uID)
        {
            return findNotifyPointer;
        }
        findNotifyPointer = &(*findNotifyPointer)->next;
    }

    return NULL;
}

static PPNOTIFY_ITEM
SysPagerWnd_FindPPNotifyItemByIndex(IN OUT PSYS_PAGER_WND_DATA This,
                                    IN WORD wIndex)
{
    PPNOTIFY_ITEM findNotifyPointer = &This->NotifyItems;

    while (*findNotifyPointer != NULL)
    {
        if ((*findNotifyPointer)->Index == wIndex)
        {
            return findNotifyPointer;
        }
        findNotifyPointer = &(*findNotifyPointer)->next;
    }

    return NULL;
}

static VOID
SysPagerWnd_UpdateButton(IN OUT PSYS_PAGER_WND_DATA This,
                         IN CONST NOTIFYICONDATA *iconData)
{
    TBBUTTONINFO tbbi;
    PNOTIFY_ITEM notifyItem;
    PPNOTIFY_ITEM NotifyPointer;

    NotifyPointer = SysPagerWnd_FindPPNotifyItemByIconData(This, iconData);
    notifyItem = *NotifyPointer;

    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_BYINDEX | TBIF_COMMAND;
    tbbi.idCommand = notifyItem->Index;

    if (iconData->uFlags & NIF_MESSAGE)
    {
        notifyItem->iconData.uCallbackMessage = iconData->uCallbackMessage;
    }

    if (iconData->uFlags & NIF_ICON)
    {
        tbbi.dwMask |= TBIF_IMAGE;
        notifyItem->IconIndex = tbbi.iImage = ImageList_AddIcon(This->SysIcons, iconData->hIcon);
    }

    /* TODO: support NIF_TIP */

    if (iconData->uFlags & NIF_STATE)
    {
        if (iconData->dwStateMask & NIS_HIDDEN &&
            (notifyItem->iconData.dwState & NIS_HIDDEN) != (iconData->dwState & NIS_HIDDEN))
        {
            tbbi.dwMask |= TBIF_STATE;
            if (iconData->dwState & NIS_HIDDEN)
            {
                tbbi.fsState |= TBSTATE_HIDDEN;
                This->VisibleButtonCount--;
            }
            else
            {
                tbbi.fsState &= ~TBSTATE_HIDDEN;
                This->VisibleButtonCount++;
            }
        }

        notifyItem->iconData.dwState &= ~iconData->dwStateMask;
        notifyItem->iconData.dwState |= (iconData->dwState & iconData->dwStateMask);
    }

    /* TODO: support NIF_INFO, NIF_GUID, NIF_REALTIME, NIF_SHOWTIP */

    SendMessage(This->hWndToolbar,
                TB_SETBUTTONINFO,
                (WPARAM)notifyItem->Index,
                (LPARAM)&tbbi);
}


static VOID
SysPagerWnd_AddButton(IN OUT PSYS_PAGER_WND_DATA This,
                      IN CONST NOTIFYICONDATA *iconData)
{
    TBBUTTON tbBtn;
    PNOTIFY_ITEM notifyItem;
    TCHAR text[] = TEXT("");

    notifyItem = SysPagerWnd_CreateNotifyItemData(This);

    notifyItem->next = NULL;
    notifyItem->Index = This->ButtonCount;
    This->ButtonCount++;
    This->VisibleButtonCount++;

    notifyItem->iconData.hWnd = iconData->hWnd;
    notifyItem->iconData.uID = iconData->uID;

    tbBtn.fsState = TBSTATE_ENABLED;
    tbBtn.fsStyle = BTNS_NOPREFIX;
    tbBtn.dwData = notifyItem->Index;

    tbBtn.iString = (INT_PTR)text;
    tbBtn.idCommand = notifyItem->Index;

    if (iconData->uFlags & NIF_MESSAGE)
    {
        notifyItem->iconData.uCallbackMessage = iconData->uCallbackMessage;
    }

    if (iconData->uFlags & NIF_ICON)
    {
        notifyItem->IconIndex = tbBtn.iBitmap = ImageList_AddIcon(This->SysIcons, iconData->hIcon);
    }

    /* TODO: support NIF_TIP */

    if (iconData->uFlags & NIF_STATE)
    {
        notifyItem->iconData.dwState &= ~iconData->dwStateMask;
        notifyItem->iconData.dwState |= (iconData->dwState & iconData->dwStateMask);
        if (notifyItem->iconData.dwState & NIS_HIDDEN)
        {
            tbBtn.fsState |= TBSTATE_HIDDEN;
            This->VisibleButtonCount--;
        }

    }

    /* TODO: support NIF_INFO, NIF_GUID, NIF_REALTIME, NIF_SHOWTIP */

    SendMessage(This->hWndToolbar,
                TB_INSERTBUTTON,
                notifyItem->Index,
                (LPARAM)&tbBtn);

    SendMessage(This->hWndToolbar,
                TB_SETBUTTONSIZE,
                0,
                MAKELONG(16, 16));
}

static VOID
SysPagerWnd_RemoveButton(IN OUT PSYS_PAGER_WND_DATA This,
                         IN CONST NOTIFYICONDATA *iconData)
{
    PPNOTIFY_ITEM NotifyPointer;

    NotifyPointer = SysPagerWnd_FindPPNotifyItemByIconData(This, iconData);
    if (NotifyPointer)
    {
        PNOTIFY_ITEM deleteItem;
        PNOTIFY_ITEM updateItem;
        deleteItem = *NotifyPointer;

        SendMessage(This->hWndToolbar,
                    TB_DELETEBUTTON,
                    deleteItem->Index,
                    0);

        *NotifyPointer = updateItem = deleteItem->next;

        if (!(deleteItem->iconData.dwState & NIS_HIDDEN))
            This->VisibleButtonCount--;
        HeapFree(hProcessHeap,
                 0,
                 deleteItem);
        This->ButtonCount--;

        while (updateItem != NULL)
        {
            TBBUTTONINFO tbbi;
            updateItem->Index--;
            tbbi.cbSize = sizeof(tbbi);
            tbbi.dwMask = TBIF_BYINDEX | TBIF_COMMAND;
            tbbi.idCommand = updateItem->Index;

            SendMessage(This->hWndToolbar,
                        TB_SETBUTTONINFO,
                        updateItem->Index,
                        (LPARAM)&tbbi);

            updateItem = updateItem->next;
        }
    }
}

static VOID
SysPagerWnd_HandleButtonClick(IN OUT PSYS_PAGER_WND_DATA This,
                              IN WORD wIndex,
                              IN UINT uMsg,
                              IN WPARAM wParam)
{
    PPNOTIFY_ITEM NotifyPointer;

    NotifyPointer = SysPagerWnd_FindPPNotifyItemByIndex(This, wIndex);
    if (NotifyPointer)
    {
        PNOTIFY_ITEM notifyItem;
        notifyItem = *NotifyPointer;

        if (IsWindow(notifyItem->iconData.hWnd))
        {
            if (uMsg == WM_MOUSEMOVE ||
                uMsg == WM_LBUTTONDOWN ||
                uMsg == WM_MBUTTONDOWN ||
                uMsg == WM_RBUTTONDOWN)
            {
                PostMessage(notifyItem->iconData.hWnd,
                            notifyItem->iconData.uCallbackMessage,
                            notifyItem->iconData.uID,
                            uMsg);
            }
            else
            {
                DWORD pid;
                GetWindowThreadProcessId(notifyItem->iconData.hWnd, &pid);
                if (pid == GetCurrentProcessId())
                {
                    PostMessage(notifyItem->iconData.hWnd,
                                notifyItem->iconData.uCallbackMessage,
                                notifyItem->iconData.uID,
                                uMsg);
                }
                else
                {
                    SendMessage(notifyItem->iconData.hWnd,
                                notifyItem->iconData.uCallbackMessage,
                                notifyItem->iconData.uID,
                                uMsg);
                }
            }
        }
    }
}

static VOID
SysPagerWnd_DrawBackground(IN HWND hwnd,
                           IN HDC hdc)
{
    RECT rect;

    GetClientRect(hwnd, &rect);
    DrawThemeParentBackground(hwnd, hdc, &rect);
}

static LRESULT CALLBACK
SysPagerWnd_ToolbarSubclassedProc(IN HWND hWnd,
                                  IN UINT msg,
                                  IN WPARAM wParam,
                                  IN LPARAM lParam,
                                  IN UINT_PTR uIdSubclass,
                                  IN DWORD_PTR dwRefData)
{
    if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)
    {
        HWND parent = GetParent(hWnd);

        if (!parent)
            return 0;

        return SendMessage(parent, msg, wParam, lParam);
    }

    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

static VOID
SysPagerWnd_Create(IN OUT PSYS_PAGER_WND_DATA This)
{
    This->hWndToolbar = CreateWindowEx(0,
                                       TOOLBARCLASSNAME,
                                       NULL,
                                       WS_CHILD | WS_VISIBLE  | WS_CLIPCHILDREN |
                                           TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_WRAPABLE |
                                           TBSTYLE_TRANSPARENT |
                                           CCS_TOP | CCS_NORESIZE | CCS_NODIVIDER,
                                       0,
                                       0,
                                       0,
                                       0,
                                       This->hWnd,
                                       NULL,
                                       hExplorerInstance,
                                       NULL);
    if (This->hWndToolbar != NULL)
    {
        SIZE BtnSize;
        SetWindowTheme(This->hWndToolbar, L"TrayNotify", NULL);
        /* Identify the version we're using */
        SendMessage(This->hWndToolbar,
                    TB_BUTTONSTRUCTSIZE,
                    sizeof(TBBUTTON),
                    0);

        This->SysIcons = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 1000);
        SendMessage(This->hWndToolbar, TB_SETIMAGELIST, 0, (LPARAM)This->SysIcons);

        BtnSize.cx = BtnSize.cy = 18;
        SendMessage(This->hWndToolbar,
                    TB_SETBUTTONSIZE,
                    0,
                    MAKELONG(BtnSize.cx, BtnSize.cy));

        SetWindowSubclass(This->hWndToolbar,
                          SysPagerWnd_ToolbarSubclassedProc,
                          2,
                          (DWORD_PTR)This);
    }
}

static VOID
SysPagerWnd_NCDestroy(IN OUT PSYS_PAGER_WND_DATA This)
{
    /* Free allocated resources */
    SetWindowLongPtr(This->hWnd,
                     0,
                     0);
    HeapFree(hProcessHeap,
             0,
             This);
}

static VOID
SysPagerWnd_NotifyMsg(IN HWND hwnd,
                      IN WPARAM wParam,
                      IN LPARAM lParam)
{
    PSYS_PAGER_WND_DATA This = (PSYS_PAGER_WND_DATA)GetWindowLongPtr(hwnd, 0);

    PCOPYDATASTRUCT cpData = (PCOPYDATASTRUCT)lParam;
    if (cpData->dwData == 1)
    {
        DWORD trayCommand;
        NOTIFYICONDATA *iconData;
        HWND parentHWND;
        RECT windowRect;
        parentHWND = GetParent(This->hWnd);
        parentHWND = GetParent(parentHWND);
        GetClientRect(parentHWND, &windowRect);

        /* FIXME: ever heard of "struct"? */
        trayCommand = *(DWORD *) (((BYTE *)cpData->lpData) + 4);
        iconData = (NOTIFYICONDATA *) (((BYTE *)cpData->lpData) + 8);

        switch (trayCommand)
        {
            case NIM_ADD:
            {
                PPNOTIFY_ITEM NotifyPointer;
                NotifyPointer = SysPagerWnd_FindPPNotifyItemByIconData(This,
                                                                       iconData);
                if (!NotifyPointer)
                {
                    SysPagerWnd_AddButton(This, iconData);
                }
                break;
            }
            case NIM_MODIFY:
            {
                PPNOTIFY_ITEM NotifyPointer;
                NotifyPointer = SysPagerWnd_FindPPNotifyItemByIconData(This,
                                                                       iconData);
                if (!NotifyPointer)
                {
                    SysPagerWnd_AddButton(This, iconData);
                }
                else
                {
                    SysPagerWnd_UpdateButton(This, iconData);
                }
                break;
            }
            case NIM_DELETE:
            {
                SysPagerWnd_RemoveButton(This, iconData);
                break;
            }
        }
        SendMessage(parentHWND,
                    WM_SIZE,
                    0,
                    MAKELONG(windowRect.right - windowRect.left,
                             windowRect.bottom - windowRect.top));
    }
}

static void
SysPagerWnd_GetSize(IN HWND hwnd,
                    IN WPARAM wParam,
                    IN PSIZE size)
{
    PSYS_PAGER_WND_DATA This = (PSYS_PAGER_WND_DATA)GetWindowLongPtr(hwnd, 0);
    INT rows = 0;
    TBMETRICS tbm;

    if (wParam) /* horizontal */
    {
        rows = size->cy / 24;
        if (rows == 0)
            rows++;
        size->cx = (This->VisibleButtonCount+rows - 1) / rows * 24;
    }
    else
    {
        rows = size->cx / 24;
        if (rows == 0)
            rows++;
        size->cy = (This->VisibleButtonCount+rows - 1) / rows * 24;
    }

    tbm.cbSize = sizeof(tbm);
    tbm.dwMask = TBMF_BARPAD | TBMF_BUTTONSPACING;
    tbm.cxBarPad = tbm.cyBarPad = 0;
    tbm.cxButtonSpacing = 0;
    tbm.cyButtonSpacing = 0;

    SendMessage(This->hWndToolbar,
                TB_SETMETRICS,
                0,
                (LPARAM)&tbm);
}

static LRESULT CALLBACK
SysPagerWndProc(IN HWND hwnd,
                IN UINT uMsg,
                IN WPARAM wParam,
                IN LPARAM lParam)
{
    PSYS_PAGER_WND_DATA This = NULL;
    LRESULT Ret = FALSE;

    if (uMsg != WM_NCCREATE)
    {
        This = (PSYS_PAGER_WND_DATA)GetWindowLongPtr(hwnd, 0);
    }

    if (This != NULL || uMsg == WM_NCCREATE)
    {
        switch (uMsg)
        {
            case WM_ERASEBKGND:
                SysPagerWnd_DrawBackground(hwnd,(HDC)wParam);
                return 0;

            case WM_NCCREATE:
            {
                LPCREATESTRUCT CreateStruct = (LPCREATESTRUCT)lParam;
                This = CreateStruct->lpCreateParams;
                This->hWnd = hwnd;
                This->NotifyItems = NULL;
                This->ButtonCount = 0;
                This->VisibleButtonCount = 0;

                SetWindowLongPtr(hwnd,
                                 0,
                                 (LONG_PTR)This);

                return TRUE;
            }
            case WM_CREATE:
                SysPagerWnd_Create(This);
                break;
            case WM_NCDESTROY:
                SysPagerWnd_NCDestroy(This);
                break;

            case WM_SIZE:
            {
                SIZE szClient;
                szClient.cx = LOWORD(lParam);
                szClient.cy = HIWORD(lParam);

                Ret = DefWindowProc(hwnd,
                                    uMsg,
                                    wParam,
                                    lParam);


                if (This->hWndToolbar != NULL && This->hWndToolbar != hwnd)
                {
                    SetWindowPos(This->hWndToolbar,
                                 NULL,
                                 0,
                                 0,
                                 szClient.cx,
                                 szClient.cy,
                                 SWP_NOZORDER);
                }
            }

            default:
                if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
                {
                    POINT pt;
                    INT iBtn;

                    pt.x = LOWORD(lParam);
                    pt.y = HIWORD(lParam);

                    iBtn = (INT)SendMessage(This->hWndToolbar,
                                            TB_HITTEST,
                                            0,
                                            (LPARAM)&pt);

                    if (iBtn >= 0)
                    {
                        SysPagerWnd_HandleButtonClick(This,iBtn,uMsg,wParam);
                    }

                    return 0;
                }

                Ret = DefWindowProc(hwnd,
                                    uMsg,
                                    wParam,
                                    lParam);
                break;
        }
    }

    return Ret;
}

static HWND
CreateSysPagerWnd(IN HWND hWndParent,
                  IN BOOL bVisible)
{
    PSYS_PAGER_WND_DATA SpData;
    DWORD dwStyle;
    HWND hWnd = NULL;

    SpData = HeapAlloc(hProcessHeap,
                       HEAP_ZERO_MEMORY,
                       sizeof(*SpData));
    if (SpData != NULL)
    {
        /* Create the window. The tray window is going to move it to the correct
           position and resize it as needed. */
        dwStyle = WS_CHILD | WS_CLIPSIBLINGS;
        if (bVisible)
            dwStyle |= WS_VISIBLE;

        hWnd = CreateWindowEx(0,
                              szSysPagerWndClass,
                              NULL,
                              dwStyle,
                              0,
                              0,
                              0,
                              0,
                              hWndParent,
                              NULL,
                              hExplorerInstance,
                              SpData);

        if (hWnd != NULL)
        {
            SetWindowTheme(hWnd, L"TrayNotify", NULL);
        }
        else
        {
            HeapFree(hProcessHeap,
                     0,
                     SpData);
        }
    }

    return hWnd;

}

static BOOL
RegisterSysPagerWndClass(VOID)
{
    WNDCLASS wcTrayClock;

    wcTrayClock.style = CS_DBLCLKS;
    wcTrayClock.lpfnWndProc = SysPagerWndProc;
    wcTrayClock.cbClsExtra = 0;
    wcTrayClock.cbWndExtra = sizeof(PSYS_PAGER_WND_DATA);
    wcTrayClock.hInstance = hExplorerInstance;
    wcTrayClock.hIcon = NULL;
    wcTrayClock.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcTrayClock.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wcTrayClock.lpszMenuName = NULL;
    wcTrayClock.lpszClassName = szSysPagerWndClass;

    return RegisterClass(&wcTrayClock) != 0;
}

static VOID
UnregisterSysPagerWndClass(VOID)
{
    UnregisterClass(szSysPagerWndClass,
                    hExplorerInstance);
}

/*
 * TrayClockWnd
 */

static const TCHAR szTrayClockWndClass[] = TEXT("TrayClockWClass");

#define ID_TRAYCLOCK_TIMER  0
#define ID_TRAYCLOCK_TIMER_INIT 1

static const struct
{
    BOOL IsTime;
    DWORD dwFormatFlags;
    LPCTSTR lpFormat;
} ClockWndFormats[] = {
    { TRUE, 0, NULL },
    { FALSE, 0, TEXT("dddd") },
    { FALSE, DATE_SHORTDATE, NULL }
};

#define CLOCKWND_FORMAT_COUNT (sizeof(ClockWndFormats) / sizeof(ClockWndFormats[0]))

#define TRAY_CLOCK_WND_SPACING_X    0
#define TRAY_CLOCK_WND_SPACING_Y    0

typedef struct _TRAY_CLOCK_WND_DATA
{
    HWND hWnd;
    HWND hWndNotify;
    HFONT hFont;
    COLORREF textColor;
    RECT rcText;
    SYSTEMTIME LocalTime;

    union
    {
        DWORD dwFlags;
        struct
        {
            DWORD IsTimerEnabled : 1;
            DWORD IsInitTimerEnabled : 1;
            DWORD LinesMeasured : 1;
            DWORD IsHorizontal : 1;
        };
    };
    DWORD LineSpacing;
    SIZE CurrentSize;
    WORD VisibleLines;
    SIZE LineSizes[CLOCKWND_FORMAT_COUNT];
    TCHAR szLines[CLOCKWND_FORMAT_COUNT][48];
} TRAY_CLOCK_WND_DATA, *PTRAY_CLOCK_WND_DATA;

static VOID
TrayClockWnd_SetFont(IN OUT PTRAY_CLOCK_WND_DATA This,
                     IN HFONT hNewFont,
                     IN BOOL bRedraw);

static VOID
TrayClockWnd_UpdateTheme(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    LOGFONTW clockFont;
    HTHEME clockTheme;
    HFONT hFont;

    clockTheme = OpenThemeData(This->hWnd, L"Clock");

    if (clockTheme)
    {
        GetThemeFont(clockTheme,
                     NULL,
                     CLP_TIME,
                     0,
                     TMT_FONT,
                     &clockFont);

        hFont = CreateFontIndirectW(&clockFont);

        TrayClockWnd_SetFont(This,
                             hFont,
                             FALSE);

        GetThemeColor(clockTheme,
                      CLP_TIME,
                      0,
                      TMT_TEXTCOLOR,
                      &This->textColor);
    }
    else
    {
        This->textColor = RGB(0,0,0);
    }

    CloseThemeData(clockTheme);
}

static BOOL
TrayClockWnd_MeasureLines(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    HDC hDC;
    HFONT hPrevFont;
    INT c, i;
    BOOL bRet = TRUE;

    hDC = GetDC(This->hWnd);
    if (hDC != NULL)
    {
        hPrevFont = SelectObject(hDC,
                                 This->hFont);

        for (i = 0;
             i != CLOCKWND_FORMAT_COUNT && bRet;
             i++)
        {
            if (This->szLines[i][0] != TEXT('\0') &&
                !GetTextExtentPoint(hDC,
                                    This->szLines[i],
                                    _tcslen(This->szLines[i]),
                                    &This->LineSizes[i]))
            {
                bRet = FALSE;
                break;
            }
        }

        SelectObject(hDC,
                     hPrevFont);

        ReleaseDC(This->hWnd,
                  hDC);

        if (bRet)
        {
            This->LineSpacing = 0;

            /* calculate the line spacing */
            for (i = 0, c = 0;
                 i != CLOCKWND_FORMAT_COUNT;
                 i++)
            {
                if (This->LineSizes[i].cx > 0)
                {
                    This->LineSpacing += This->LineSizes[i].cy;
                    c++;
                }
            }

            if (c > 0)
            {
                /* We want a spaceing of 1/2 line */
                This->LineSpacing = (This->LineSpacing / c) / 2;
            }

            return TRUE;
        }
    }

    return FALSE;
}

static WORD
TrayClockWnd_GetMinimumSize(IN OUT PTRAY_CLOCK_WND_DATA This,
                            IN BOOL Horizontal,
                            IN OUT PSIZE pSize)
{
    WORD iLinesVisible = 0;
    INT i;
    SIZE szMax = { 0, 0 };

    if (!This->LinesMeasured)
        This->LinesMeasured = TrayClockWnd_MeasureLines(This);

    if (!This->LinesMeasured)
        return 0;

    for (i = 0;
         i != CLOCKWND_FORMAT_COUNT;
         i++)
    {
        if (This->LineSizes[i].cx != 0)
        {
            if (iLinesVisible > 0)
            {
                if (Horizontal)
                {
                    if (szMax.cy + This->LineSizes[i].cy + (LONG)This->LineSpacing >
                        pSize->cy - (2 * TRAY_CLOCK_WND_SPACING_Y))
                    {
                        break;
                    }
                }
                else
                {
                    if (This->LineSizes[i].cx > pSize->cx - (2 * TRAY_CLOCK_WND_SPACING_X))
                        break;
                }

                /* Add line spacing */
                szMax.cy += This->LineSpacing;
            }

            iLinesVisible++;

            /* Increase maximum rectangle */
            szMax.cy += This->LineSizes[i].cy;
            if (This->LineSizes[i].cx > szMax.cx - (2 * TRAY_CLOCK_WND_SPACING_X))
                szMax.cx = This->LineSizes[i].cx + (2 * TRAY_CLOCK_WND_SPACING_X);
        }
    }

    szMax.cx += 2 * TRAY_CLOCK_WND_SPACING_X;
    szMax.cy += 2 * TRAY_CLOCK_WND_SPACING_Y;

    *pSize = szMax;

    return iLinesVisible;
}


static VOID
TrayClockWnd_UpdateWnd(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    SIZE szPrevCurrent;
    INT BufSize, iRet, i;
    RECT rcClient;

    ZeroMemory(This->LineSizes,
               sizeof(This->LineSizes));

    szPrevCurrent = This->CurrentSize;

    for (i = 0;
         i != CLOCKWND_FORMAT_COUNT;
         i++)
    {
        This->szLines[i][0] = TEXT('\0');
        BufSize = sizeof(This->szLines[0]) / sizeof(This->szLines[0][0]);

        if (ClockWndFormats[i].IsTime)
        {
            iRet = GetTimeFormat(LOCALE_USER_DEFAULT,
                                 AdvancedSettings.bShowSeconds ? ClockWndFormats[i].dwFormatFlags : TIME_NOSECONDS,
                                 &This->LocalTime,
                                 ClockWndFormats[i].lpFormat,
                                 This->szLines[i],
                                 BufSize);
        }
        else
        {
            iRet = GetDateFormat(LOCALE_USER_DEFAULT,
                                 ClockWndFormats[i].dwFormatFlags,
                                 &This->LocalTime,
                                 ClockWndFormats[i].lpFormat,
                                 This->szLines[i],
                                 BufSize);
        }

        if (iRet != 0 && i == 0)
        {
            /* Set the window text to the time only */
            SetWindowText(This->hWnd,
                          This->szLines[i]);
        }
    }

    This->LinesMeasured = TrayClockWnd_MeasureLines(This);

    if (This->LinesMeasured &&
        GetClientRect(This->hWnd,
                      &rcClient))
    {
        SIZE szWnd;

        szWnd.cx = rcClient.right;
        szWnd.cy = rcClient.bottom;

        This->VisibleLines = TrayClockWnd_GetMinimumSize(This,
                                                         This->IsHorizontal,
                                                         &szWnd);
        This->CurrentSize = szWnd;
    }

    if (IsWindowVisible(This->hWnd))
    {
        InvalidateRect(This->hWnd,
                       NULL,
                       TRUE);

        if (This->hWndNotify != NULL &&
            (szPrevCurrent.cx != This->CurrentSize.cx ||
             szPrevCurrent.cy != This->CurrentSize.cy))
        {
            NMHDR nmh;

            nmh.hwndFrom = This->hWnd;
            nmh.idFrom = GetWindowLongPtr(This->hWnd,
                                          GWLP_ID);
            nmh.code = NTNWM_REALIGN;

            SendMessage(This->hWndNotify,
                        WM_NOTIFY,
                        (WPARAM)nmh.idFrom,
                        (LPARAM)&nmh);
        }
    }
}

static VOID
TrayClockWnd_Update(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    GetLocalTime(&This->LocalTime);
    TrayClockWnd_UpdateWnd(This);
}

static UINT
TrayClockWnd_CalculateDueTime(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    UINT uiDueTime;

    /* Calculate the due time */
    GetLocalTime(&This->LocalTime);
    uiDueTime = 1000 - (UINT)This->LocalTime.wMilliseconds;
    if (AdvancedSettings.bShowSeconds)
        uiDueTime += (UINT)This->LocalTime.wSecond * 100;
    else
        uiDueTime += (59 - (UINT)This->LocalTime.wSecond) * 1000;

    if (uiDueTime < USER_TIMER_MINIMUM || uiDueTime > USER_TIMER_MAXIMUM)
        uiDueTime = 1000;
    else
    {
        /* Add an artificial delay of 0.05 seconds to make sure the timer
           doesn't fire too early*/
        uiDueTime += 50;
    }

    return uiDueTime;
}

static BOOL
TrayClockWnd_ResetTime(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    UINT uiDueTime;
    BOOL Ret;

    /* Disable all timers */
    if (This->IsTimerEnabled)
    {
        KillTimer(This->hWnd,
                  ID_TRAYCLOCK_TIMER);
        This->IsTimerEnabled = FALSE;
    }

    if (This->IsInitTimerEnabled)
    {
        KillTimer(This->hWnd,
                  ID_TRAYCLOCK_TIMER_INIT);
    }

    uiDueTime = TrayClockWnd_CalculateDueTime(This);

    /* Set the new timer */
    Ret = SetTimer(This->hWnd,
                   ID_TRAYCLOCK_TIMER_INIT,
                   uiDueTime,
                   NULL) != 0;
    This->IsInitTimerEnabled = Ret;

    /* Update the time */
    TrayClockWnd_Update(This);

    return Ret;
}

static VOID
TrayClockWnd_CalibrateTimer(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    UINT uiDueTime;
    BOOL Ret;
    UINT uiWait1, uiWait2;

    /* Kill the initialization timer */
    KillTimer(This->hWnd,
              ID_TRAYCLOCK_TIMER_INIT);
    This->IsInitTimerEnabled = FALSE;

    uiDueTime = TrayClockWnd_CalculateDueTime(This);

    if (AdvancedSettings.bShowSeconds)
    {
        uiWait1 = 1000 - 200;
        uiWait2 = 1000;
    }
    else
    {
        uiWait1 = 60 * 1000 - 200;
        uiWait2 = 60 * 1000;
    }

    if (uiDueTime > uiWait1)
    {
        /* The update of the clock will be up to 200 ms late, but that's
           acceptable. We're going to setup a timer that fires depending
           uiWait2. */
        Ret = SetTimer(This->hWnd,
                       ID_TRAYCLOCK_TIMER,
                       uiWait2,
                       NULL) != 0;
        This->IsTimerEnabled = Ret;

        /* Update the time */
        TrayClockWnd_Update(This);
    }
    else
    {
        /* Recalibrate the timer and recalculate again when the current
           minute/second ends. */
        TrayClockWnd_ResetTime(This);
    }
}

static VOID
TrayClockWnd_NCDestroy(IN OUT PTRAY_CLOCK_WND_DATA This)
{
    /* Disable all timers */
    if (This->IsTimerEnabled)
    {
        KillTimer(This->hWnd,
                  ID_TRAYCLOCK_TIMER);
    }

    if (This->IsInitTimerEnabled)
    {
        KillTimer(This->hWnd,
                  ID_TRAYCLOCK_TIMER_INIT);
    }

    /* Free allocated resources */
    SetWindowLongPtr(This->hWnd,
                     0,
                     0);
    HeapFree(hProcessHeap,
             0,
             This);
}

static VOID
TrayClockWnd_Paint(IN OUT PTRAY_CLOCK_WND_DATA This,
                   IN HDC hDC)
{
    RECT rcClient;
    HFONT hPrevFont;
    int iPrevBkMode, i, line;

    if (This->LinesMeasured &&
        GetClientRect(This->hWnd,
                      &rcClient))
    {
        iPrevBkMode = SetBkMode(hDC,
                                TRANSPARENT);

        SetTextColor(hDC, This->textColor);

        hPrevFont = SelectObject(hDC,
                                 This->hFont);

        rcClient.left = (rcClient.right / 2) - (This->CurrentSize.cx / 2);
        rcClient.top = (rcClient.bottom / 2) - (This->CurrentSize.cy / 2);
        rcClient.right = rcClient.left + This->CurrentSize.cx;
        rcClient.bottom = rcClient.top + This->CurrentSize.cy;

        for (i = 0, line = 0;
             i != CLOCKWND_FORMAT_COUNT && line < This->VisibleLines;
             i++)
        {
            if (This->LineSizes[i].cx != 0)
            {
                TextOut(hDC,
                        rcClient.left + (This->CurrentSize.cx / 2) - (This->LineSizes[i].cx / 2) +
                            TRAY_CLOCK_WND_SPACING_X,
                        rcClient.top + TRAY_CLOCK_WND_SPACING_Y,
                        This->szLines[i],
                        _tcslen(This->szLines[i]));

                rcClient.top += This->LineSizes[i].cy + This->LineSpacing;
                line++;
            }
        }

        SelectObject(hDC,
                     hPrevFont);

        SetBkMode(hDC,
                  iPrevBkMode);
    }
}

static VOID
TrayClockWnd_SetFont(IN OUT PTRAY_CLOCK_WND_DATA This,
                     IN HFONT hNewFont,
                     IN BOOL bRedraw)
{
    This->hFont = hNewFont;
    This->LinesMeasured = TrayClockWnd_MeasureLines(This);
    if (bRedraw)
    {
        InvalidateRect(This->hWnd,
                       NULL,
                       TRUE);
    }
}

static VOID
TrayClockWnd_DrawBackground(IN HWND hwnd,
                            IN HDC hdc)
{
    RECT rect;

    GetClientRect(hwnd, &rect);
    DrawThemeParentBackground(hwnd, hdc, &rect);
}

static LRESULT CALLBACK
TrayClockWndProc(IN HWND hwnd,
                 IN UINT uMsg,
                 IN WPARAM wParam,
                 IN LPARAM lParam)
{
    PTRAY_CLOCK_WND_DATA This = NULL;
    LRESULT Ret = FALSE;

    if (uMsg != WM_NCCREATE)
    {
        This = (PTRAY_CLOCK_WND_DATA)GetWindowLongPtr(hwnd,
                                                      0);
    }

    if (This != NULL || uMsg == WM_NCCREATE)
    {
        switch (uMsg)
        {
            case WM_THEMECHANGED:
                TrayClockWnd_UpdateTheme(This);
                break;
            case WM_ERASEBKGND:
                TrayClockWnd_DrawBackground(hwnd, (HDC)wParam);
                break;
            case WM_PAINT:
            case WM_PRINTCLIENT:
            {
                PAINTSTRUCT ps;
                HDC hDC = (HDC)wParam;

                if (wParam == 0)
                {
                    hDC = BeginPaint(This->hWnd,
                                     &ps);
                }

                if (hDC != NULL)
                {
                    TrayClockWnd_Paint(This,
                                       hDC);

                    if (wParam == 0)
                    {
                        EndPaint(This->hWnd,
                                 &ps);
                    }
                }
                break;
            }

            case WM_TIMER:
                switch (wParam)
                {
                    case ID_TRAYCLOCK_TIMER:
                        TrayClockWnd_Update(This);
                        break;

                    case ID_TRAYCLOCK_TIMER_INIT:
                        TrayClockWnd_CalibrateTimer(This);
                        break;
                }
                break;

            case WM_NCHITTEST:
                /* We want the user to be able to drag the task bar when clicking the clock */
                Ret = HTTRANSPARENT;
                break;

            case TCWM_GETMINIMUMSIZE:
            {
                This->IsHorizontal = (BOOL)wParam;

                Ret = (LRESULT)TrayClockWnd_GetMinimumSize(This,
                                                           (BOOL)wParam,
                                                           (PSIZE)lParam) != 0;
                break;
            }

            case TCWM_UPDATETIME:
            {
                Ret = (LRESULT)TrayClockWnd_ResetTime(This);
                break;
            }

            case WM_NCCREATE:
            {
                LPCREATESTRUCT CreateStruct = (LPCREATESTRUCT)lParam;
                This = (PTRAY_CLOCK_WND_DATA)CreateStruct->lpCreateParams;
                This->hWnd = hwnd;
                This->hWndNotify = CreateStruct->hwndParent;

                SetWindowLongPtr(hwnd,
                                 0,
                                 (LONG_PTR)This);
                TrayClockWnd_UpdateTheme(This);

                return TRUE;
            }

            case WM_SETFONT:
            {
                TrayClockWnd_SetFont(This,
                                     (HFONT)wParam,
                                     (BOOL)LOWORD(lParam));
                break;
            }

            case WM_CREATE:
                TrayClockWnd_ResetTime(This);
                break;

            case WM_NCDESTROY:
                TrayClockWnd_NCDestroy(This);
                break;

            case WM_SIZE:
            {
                SIZE szClient;

                szClient.cx = LOWORD(lParam);
                szClient.cy = HIWORD(lParam);
                This->VisibleLines = TrayClockWnd_GetMinimumSize(This,
                                                                 This->IsHorizontal,
                                                                 &szClient);
                This->CurrentSize = szClient;

                InvalidateRect(This->hWnd,
                               NULL,
                               TRUE);
                break;
            }

            default:
                Ret = DefWindowProc(hwnd,
                                    uMsg,
                                    wParam,
                                    lParam);
                break;
        }
    }

    return Ret;
}

static HWND
CreateTrayClockWnd(IN HWND hWndParent,
                   IN BOOL bVisible)
{
    PTRAY_CLOCK_WND_DATA TcData;
    DWORD dwStyle;
    HWND hWnd = NULL;

    TcData = HeapAlloc(hProcessHeap,
                       HEAP_ZERO_MEMORY,
                       sizeof(*TcData));
    if (TcData != NULL)
    {
        TcData->IsHorizontal = TRUE;
        /* Create the window. The tray window is going to move it to the correct
           position and resize it as needed. */
        dwStyle = WS_CHILD | WS_CLIPSIBLINGS;
        if (bVisible)
            dwStyle |= WS_VISIBLE;

        hWnd = CreateWindowEx(0,
                              szTrayClockWndClass,
                              NULL,
                              dwStyle,
                              0,
                              0,
                              0,
                              0,
                              hWndParent,
                              NULL,
                              hExplorerInstance,
                              TcData);

        if (hWnd != NULL)
        {
            SetWindowTheme(hWnd, L"TrayNotify", NULL);
        }
        else
        {
            HeapFree(hProcessHeap,
                     0,
                     TcData);
        }
    }

    return hWnd;

}

static BOOL
RegisterTrayClockWndClass(VOID)
{
    WNDCLASS wcTrayClock;

    wcTrayClock.style = CS_DBLCLKS;
    wcTrayClock.lpfnWndProc = TrayClockWndProc;
    wcTrayClock.cbClsExtra = 0;
    wcTrayClock.cbWndExtra = sizeof(PTRAY_CLOCK_WND_DATA);
    wcTrayClock.hInstance = hExplorerInstance;
    wcTrayClock.hIcon = NULL;
    wcTrayClock.hCursor = LoadCursor(NULL,
                                     IDC_ARROW);
    wcTrayClock.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wcTrayClock.lpszMenuName = NULL;
    wcTrayClock.lpszClassName = szTrayClockWndClass;

    return RegisterClass(&wcTrayClock) != 0;
}

static VOID
UnregisterTrayClockWndClass(VOID)
{
    UnregisterClass(szTrayClockWndClass,
                    hExplorerInstance);
}

/*
 * TrayNotifyWnd
 */

static const TCHAR szTrayNotifyWndClass[] = TEXT("TrayNotifyWnd");

#define TRAY_NOTIFY_WND_SPACING_X   2
#define TRAY_NOTIFY_WND_SPACING_Y   2

typedef struct _TRAY_NOTIFY_WND_DATA
{
    HWND hWnd;
    HWND hWndTrayClock;
    HWND hWndNotify;
    HWND hWndSysPager;
    HTHEME TrayTheme;
    SIZE szTrayClockMin;
    SIZE szTrayNotify;
    MARGINS ContentMargin;
    ITrayWindow *TrayWindow;
    HFONT hFontClock;
    union
    {
        DWORD dwFlags;
        struct
        {
            DWORD HideClock : 1;
            DWORD IsHorizontal : 1;
        };
    };
} TRAY_NOTIFY_WND_DATA, *PTRAY_NOTIFY_WND_DATA;

static VOID
TrayNotifyWnd_UpdateTheme(IN OUT PTRAY_NOTIFY_WND_DATA This)
{
    LONG_PTR style;

    if (This->TrayTheme)
        CloseThemeData(This->TrayTheme);

    if (IsThemeActive())
        This->TrayTheme = OpenThemeData(This->hWnd, L"TrayNotify");
    else
        This->TrayTheme = 0;

    if (This->TrayTheme)
    {
        style = GetWindowLong(This->hWnd, GWL_EXSTYLE);
        style = style & ~WS_EX_STATICEDGE;
        SetWindowLong(This->hWnd, GWL_EXSTYLE, style);

        GetThemeMargins(This->TrayTheme,
                        NULL,
                        TNP_BACKGROUND,
                        0,
                        TMT_CONTENTMARGINS,
                        NULL,
                        &This->ContentMargin);
    }
    else
    {
        style = GetWindowLong(This->hWnd, GWL_EXSTYLE);
        style = style | WS_EX_STATICEDGE;
        SetWindowLong(This->hWnd, GWL_EXSTYLE, style);

        This->ContentMargin.cxLeftWidth = 0;
        This->ContentMargin.cxRightWidth = 0;
        This->ContentMargin.cyTopHeight = 0;
        This->ContentMargin.cyBottomHeight = 0;
    }
}

static VOID
TrayNotifyWnd_Create(IN OUT PTRAY_NOTIFY_WND_DATA This)
{
    This->hWndTrayClock = CreateTrayClockWnd(This->hWnd,
                                             !This->HideClock);

    This->hWndSysPager = CreateSysPagerWnd(This->hWnd,
                                           !This->HideClock);

    TrayNotifyWnd_UpdateTheme(This);
}

static VOID
TrayNotifyWnd_NCDestroy(IN OUT PTRAY_NOTIFY_WND_DATA This)
{
    SetWindowLongPtr(This->hWnd,
                     0,
                     0);
    HeapFree(hProcessHeap,
             0,
             This);
}

static BOOL
TrayNotifyWnd_GetMinimumSize(IN OUT PTRAY_NOTIFY_WND_DATA This,
                             IN BOOL Horizontal,
                             IN OUT PSIZE pSize)
{
    SIZE szClock = { 0, 0 };
    SIZE szTray = { 0, 0 };

    This->IsHorizontal = Horizontal;
    if (This->IsHorizontal)
        SetWindowTheme(This->hWnd, L"TrayNotifyHoriz", NULL);
    else
        SetWindowTheme(This->hWnd, L"TrayNotifyVert", NULL);

    if (!This->HideClock)
    {
        if (Horizontal)
        {
            szClock.cy = pSize->cy - 2 * TRAY_NOTIFY_WND_SPACING_Y;
            if (szClock.cy <= 0)
                goto NoClock;
        }
        else
        {
            szClock.cx = pSize->cx - 2 * TRAY_NOTIFY_WND_SPACING_X;
            if (szClock.cx <= 0)
                goto NoClock;
        }

        SendMessage(This->hWndTrayClock,
                    TCWM_GETMINIMUMSIZE,
                    (WPARAM)Horizontal,
                    (LPARAM)&szClock);

        This->szTrayClockMin = szClock;
    }
    else
NoClock:
        This->szTrayClockMin = szClock;

    if (Horizontal)
    {
        szTray.cy = pSize->cy - 2 * TRAY_NOTIFY_WND_SPACING_Y;
    }
    else
    {
        szTray.cx = pSize->cx - 2 * TRAY_NOTIFY_WND_SPACING_X;
    }

    SysPagerWnd_GetSize(This->hWndSysPager,
                        Horizontal,
                        &szTray);

    This->szTrayNotify = szTray;

    if (Horizontal)
    {
        pSize->cx = 2 * TRAY_NOTIFY_WND_SPACING_X;

        if (!This->HideClock)
            pSize->cx += TRAY_NOTIFY_WND_SPACING_X + This->szTrayClockMin.cx;

        pSize->cx += szTray.cx;
    }
    else
    {
        pSize->cy = 2 * TRAY_NOTIFY_WND_SPACING_Y;

        if (!This->HideClock)
            pSize->cy += TRAY_NOTIFY_WND_SPACING_Y + This->szTrayClockMin.cy;

        pSize->cy += szTray.cy;
    }

    pSize->cy += This->ContentMargin.cyTopHeight + This->ContentMargin.cyBottomHeight;
    pSize->cx += This->ContentMargin.cxLeftWidth + This->ContentMargin.cxRightWidth;

    return TRUE;
}

static VOID
TrayNotifyWnd_Size(IN OUT PTRAY_NOTIFY_WND_DATA This,
                   IN const SIZE *pszClient)
{
    if (!This->HideClock)
    {
        POINT ptClock;
        SIZE szClock;

        if (This->IsHorizontal)
        {
            ptClock.x = pszClient->cx - TRAY_NOTIFY_WND_SPACING_X - This->szTrayClockMin.cx;
            ptClock.y = TRAY_NOTIFY_WND_SPACING_Y;
            szClock.cx = This->szTrayClockMin.cx;
            szClock.cy = pszClient->cy - (2 * TRAY_NOTIFY_WND_SPACING_Y);
        }
        else
        {
            ptClock.x = TRAY_NOTIFY_WND_SPACING_X;
            ptClock.y = pszClient->cy - TRAY_NOTIFY_WND_SPACING_Y - This->szTrayClockMin.cy;
            szClock.cx = pszClient->cx - (2 * TRAY_NOTIFY_WND_SPACING_X);
            szClock.cy = This->szTrayClockMin.cy;
        }

        SetWindowPos(This->hWndTrayClock,
                     NULL,
                     ptClock.x,
                     ptClock.y,
                     szClock.cx,
                     szClock.cy,
                     SWP_NOZORDER);

        if (This->IsHorizontal)
        {
            ptClock.x -= This->szTrayNotify.cx;
        }
        else
        {
            ptClock.y -= This->szTrayNotify.cy;
        }

        SetWindowPos(This->hWndSysPager,
                     NULL,
                     ptClock.x,
                     ptClock.y,
                     This->szTrayNotify.cx,
                     This->szTrayNotify.cy,
                     SWP_NOZORDER);
    }
}

static LRESULT
TrayNotifyWnd_DrawBackground(IN HWND hwnd,
                             IN UINT uMsg,
                             IN WPARAM wParam,
                             IN LPARAM lParam)
{
    PTRAY_NOTIFY_WND_DATA This = (PTRAY_NOTIFY_WND_DATA)GetWindowLongPtr(hwnd, 0);
    RECT rect;
    HDC hdc = (HDC)wParam;

    GetClientRect(hwnd, &rect);

    DrawThemeParentBackground(hwnd, hdc, &rect);
    DrawThemeBackground(This->TrayTheme, hdc, TNP_BACKGROUND, 0, &rect, 0);

    return 0;
}

VOID
TrayNotify_NotifyMsg(IN HWND hwnd,
                     IN WPARAM wParam,
                     IN LPARAM lParam)
{
    PTRAY_NOTIFY_WND_DATA This = (PTRAY_NOTIFY_WND_DATA)GetWindowLongPtr(hwnd, 0);
    if (This->hWndSysPager)
    {
        SysPagerWnd_NotifyMsg(This->hWndSysPager,
                              wParam,
                              lParam);
    }
}

BOOL
TrayNotify_GetClockRect(IN HWND hwnd,
                        OUT PRECT rcClock)
{
    PTRAY_NOTIFY_WND_DATA This = (PTRAY_NOTIFY_WND_DATA)GetWindowLongPtr(hwnd, 0);
    if (!IsWindowVisible(This->hWndTrayClock))
        return FALSE;

    return GetWindowRect(This->hWndTrayClock, rcClock);
}

static LRESULT CALLBACK
TrayNotifyWndProc(IN HWND hwnd,
                  IN UINT uMsg,
                  IN WPARAM wParam,
                  IN LPARAM lParam)
{
    PTRAY_NOTIFY_WND_DATA This = NULL;
    LRESULT Ret = FALSE;

    if (uMsg != WM_NCCREATE)
    {
        This = (PTRAY_NOTIFY_WND_DATA)GetWindowLongPtr(hwnd,
                                                       0);
    }

    if (This != NULL || uMsg == WM_NCCREATE)
    {
        switch (uMsg)
        {
            case WM_THEMECHANGED:
                TrayNotifyWnd_UpdateTheme(This);
                return 0;
            case WM_ERASEBKGND:
                return TrayNotifyWnd_DrawBackground(hwnd,
                                                    uMsg,
                                                    wParam,
                                                    lParam);
            case TNWM_GETMINIMUMSIZE:
            {
                Ret = (LRESULT)TrayNotifyWnd_GetMinimumSize(This,
                                                            (BOOL)wParam,
                                                            (PSIZE)lParam);
                break;
            }

            case TNWM_UPDATETIME:
            {
                if (This->hWndTrayClock != NULL)
                {
                    /* Forward the message to the tray clock window procedure */
                    Ret = TrayClockWndProc(This->hWndTrayClock,
                                           TCWM_UPDATETIME,
                                           wParam,
                                           lParam);
                }
                break;
            }

            case WM_SIZE:
            {
                SIZE szClient;

                szClient.cx = LOWORD(lParam);
                szClient.cy = HIWORD(lParam);

                TrayNotifyWnd_Size(This,
                                   &szClient);
                break;
            }

            case WM_NCHITTEST:
                /* We want the user to be able to drag the task bar when clicking the
                   tray notification window */
                Ret = HTTRANSPARENT;
                break;

            case TNWM_SHOWCLOCK:
            {
                BOOL PrevHidden = This->HideClock;
                This->HideClock = (wParam == 0);

                if (This->hWndTrayClock != NULL && PrevHidden != This->HideClock)
                {
                    ShowWindow(This->hWndTrayClock,
                               This->HideClock ? SW_HIDE : SW_SHOW);
                }

                Ret = (LRESULT)(!PrevHidden);
                break;
            }

            case WM_NOTIFY:
            {
                const NMHDR *nmh = (const NMHDR *)lParam;

                if (nmh->hwndFrom == This->hWndTrayClock)
                {
                    /* Pass down notifications */
                    Ret = SendMessage(This->hWndNotify,
                                      WM_NOTIFY,
                                      wParam,
                                      lParam);
                }
                break;
            }

            case WM_SETFONT:
            {
                if (This->hWndTrayClock != NULL)
                {
                    SendMessage(This->hWndTrayClock,
                                WM_SETFONT,
                                wParam,
                                lParam);
                }
                goto HandleDefaultMessage;
            }

            case WM_NCCREATE:
            {
                LPCREATESTRUCT CreateStruct = (LPCREATESTRUCT)lParam;
                This = (PTRAY_NOTIFY_WND_DATA)CreateStruct->lpCreateParams;
                This->hWnd = hwnd;
                This->hWndNotify = CreateStruct->hwndParent;

                SetWindowLongPtr(hwnd,
                                 0,
                                 (LONG_PTR)This);

                return TRUE;
            }

            case WM_CREATE:
                TrayNotifyWnd_Create(This);
                break;

            case WM_NCDESTROY:
                TrayNotifyWnd_NCDestroy(This);
                break;

            default:
HandleDefaultMessage:
                Ret = DefWindowProc(hwnd,
                                    uMsg,
                                    wParam,
                                    lParam);
                break;
        }
    }

    return Ret;
}

HWND
CreateTrayNotifyWnd(IN OUT ITrayWindow *TrayWindow,
                    IN BOOL bHideClock)
{
    PTRAY_NOTIFY_WND_DATA TnData;
    HWND hWndTrayWindow;
    HWND hWnd = NULL;

    hWndTrayWindow = ITrayWindow_GetHWND(TrayWindow);
    if (hWndTrayWindow == NULL)
        return NULL;

    TnData = HeapAlloc(hProcessHeap,
                       HEAP_ZERO_MEMORY,
                       sizeof(*TnData));
    if (TnData != NULL)
    {
        TnData->TrayWindow = TrayWindow;
        TnData->HideClock = bHideClock;

        /* Create the window. The tray window is going to move it to the correct
           position and resize it as needed. */
        hWnd = CreateWindowEx(WS_EX_STATICEDGE,
                              szTrayNotifyWndClass,
                              NULL,
                              WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                              0,
                              0,
                              0,
                              0,
                              hWndTrayWindow,
                              NULL,
                              hExplorerInstance,
                              TnData);

        if (hWnd == NULL)
        {
            HeapFree(hProcessHeap,
                     0,
                     TnData);
        }
    }

    return hWnd;
}

BOOL
RegisterTrayNotifyWndClass(VOID)
{
    WNDCLASS wcTrayWnd;
    BOOL Ret;

    wcTrayWnd.style = CS_DBLCLKS;
    wcTrayWnd.lpfnWndProc = TrayNotifyWndProc;
    wcTrayWnd.cbClsExtra = 0;
    wcTrayWnd.cbWndExtra = sizeof(PTRAY_NOTIFY_WND_DATA);
    wcTrayWnd.hInstance = hExplorerInstance;
    wcTrayWnd.hIcon = NULL;
    wcTrayWnd.hCursor = LoadCursor(NULL,
                                   IDC_ARROW);
    wcTrayWnd.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wcTrayWnd.lpszMenuName = NULL;
    wcTrayWnd.lpszClassName = szTrayNotifyWndClass;

    Ret = RegisterClass(&wcTrayWnd) != 0;

    if (Ret)
    {
        Ret = RegisterTrayClockWndClass();
        if (!Ret)
        {
            UnregisterClass(szTrayNotifyWndClass,
                            hExplorerInstance);
        }
        RegisterSysPagerWndClass();
    }

    return Ret;
}

VOID
UnregisterTrayNotifyWndClass(VOID)
{
    UnregisterTrayClockWndClass();

    UnregisterSysPagerWndClass();

    UnregisterClass(szTrayNotifyWndClass,
                    hExplorerInstance);
}
