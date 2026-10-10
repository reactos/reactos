/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "../controls.h"

#define IDC_SCROLL_HORZ_STD     9001
#define IDC_SCROLL_HORZ_BOTTOM  9002
#define IDC_SCROLL_VERT_STD     9003
#define IDC_SCROLL_SIZEGRIP     9004
#define IDC_SCROLL_LOG_EDIT     9005

typedef struct _SCROLLBAR_PAGE_DATA
{
    HWND hLabelHorzStd, hHorzStdScroll;
    HWND hLabelHorzBottom, hHorzBottomScroll;

    HWND hLabelVertStd, hVertStdScroll;
    HWND hLabelSizeGrip, hSizeGripParent, hSizeGrip;

    HWND hLabelLog, hLogEdit;
} SCROLLBAR_PAGE_DATA, *PSCROLLBAR_PAGE_DATA;

static VOID
LogScrollEvent(HWND hLogEdit, LPCWSTR szText)
{
    int len;

    if (!hLogEdit)
        return;

    len = GetWindowTextLengthW(hLogEdit);
    SendMessageW(hLogEdit, EM_SETSEL, len, len);
    SendMessageW(hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)szText);
    SendMessageW(hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
}

static LRESULT
InitPage(HWND Parent, PSCROLLBAR_PAGE_DATA PageData)
{
    SCROLLINFO si = {0};

    PageData->hLabelHorzStd =
        CreateChild(IDC_STATIC, L"STATIC", L"Standard Horizontal:", SS_LEFT, 0, 0, 0, 0, Parent);
    PageData->hHorzStdScroll =
        CreateChild(IDC_SCROLL_HORZ_STD,
                    L"SCROLLBAR", NULL,
                    SBS_HORZ | WS_TABSTOP,
                    0, 0, 0, 0, Parent);

    PageData->hLabelHorzBottom =
        CreateChild(IDC_STATIC, L"STATIC", L"Bottom-Aligned Horz:", SS_LEFT, 0, 0, 0, 0, Parent);
    PageData->hHorzBottomScroll =
        CreateChild(IDC_SCROLL_HORZ_BOTTOM, L"SCROLLBAR", NULL,
                    SBS_HORZ | SBS_BOTTOMALIGN | WS_TABSTOP,
                    0, 0, 0, 0, Parent);

    PageData->hLabelVertStd =
        CreateChild(IDC_STATIC, L"STATIC", L"Standard Vertical:", SS_LEFT, 0, 0, 0, 0, Parent);
    PageData->hVertStdScroll =
        CreateChild(IDC_SCROLL_VERT_STD, L"SCROLLBAR", NULL,
                    SBS_VERT | WS_TABSTOP,
                    0, 0, 0, 0, Parent);

    PageData->hLabelSizeGrip =
        CreateChild(IDC_STATIC, L"STATIC", L"Size Grip Box:", SS_LEFT, 0, 0, 0, 0, Parent);
    PageData->hSizeGripParent =
        CreateChild(IDC_STATIC, L"STATIC", NULL,
                    WS_BORDER | WS_CLIPSIBLINGS,
                    0, 0, 0, 0, Parent);
    PageData->hSizeGrip =
        CreateChild(IDC_SCROLL_SIZEGRIP, L"SCROLLBAR", NULL,
                    SBS_SIZEGRIP,
                    0, 0, 50, 30, PageData->hSizeGripParent);

    /* Event log */
    PageData->hLabelLog =
        CreateChild(IDC_STATIC, L"STATIC", L"Scroll Event Log:", SS_LEFT, 0, 0, 0, 0, Parent);
    PageData->hLogEdit =
        CreateChild(IDC_SCROLL_LOG_EDIT, L"EDIT", L"",
                    WS_BORDER | WS_TABSTOP | WS_VSCROLL |
                    ES_LEFT | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                    0, 0, 0, 0, Parent);

    /* Configure ranges and thumb page sizes using native SCROLLINFO */
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = 100;
    si.nPage = 15;
    si.nPos = 25;
    SetScrollInfo(PageData->hHorzStdScroll, SB_CTL, &si, TRUE);
    si.nPos = 60;
    SetScrollInfo(PageData->hHorzBottomScroll, SB_CTL, &si, TRUE);
    si.nPos = 40;
    SetScrollInfo(PageData->hVertStdScroll, SB_CTL, &si, TRUE);

    return TRUE;
}


static VOID
HandleScrollNotification(HWND hScroll, WORD nScrollCode, PSCROLLBAR_PAGE_DATA PageData, LPCWSTR szBarName)
{
    SCROLLINFO si = {0};
    int nOldPos, nNewPos;
    WCHAR szLog[128];
    LPCWSTR szCodeName = L"UNKNOWN";

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(hScroll, SB_CTL, &si);

    nOldPos = si.nPos;
    nNewPos = si.nPos;

    switch (nScrollCode)
    {
        case SB_LINELEFT:  /* also SB_LINEUP */
            nNewPos -= 1;
            szCodeName = L"LINELEFT / LINEUP";
            break;

        case SB_LINERIGHT: /* also SB_LINEDOWN */
            nNewPos += 1;
            szCodeName = L"LINERIGHT / LINEDOWN";
            break;

        case SB_PAGELEFT:  /* also SB_PAGEUP */
            nNewPos -= (int)si.nPage;
            szCodeName = L"PAGELEFT / PAGEUP";
            break;

        case SB_PAGERIGHT: /* also SB_PAGEDOWN */
            nNewPos += (int)si.nPage;
            szCodeName = L"PAGERIGHT / PAGEDOWN";
            break;

        case SB_THUMBTRACK:
            nNewPos = si.nTrackPos;
            szCodeName = L"THUMBTRACK";
            break;

        case SB_THUMBPOSITION:
            nNewPos = si.nTrackPos;
            szCodeName = L"THUMBPOSITION";
            break;

        case SB_TOP: /* also SB_LEFT */
            nNewPos = si.nMin;
            szCodeName = L"TOP / LEFT";
            break;

        case SB_BOTTOM: /* also SB_RIGHT */
            nNewPos = si.nMax;
            szCodeName = L"BOTTOM / RIGHT";
            break;

        case SB_ENDSCROLL:
            return; /* Ignore end-scroll events to avoid log flooding */
    }

    /* Clamp position to valid range [Min, Max - Page + 1] */
    if (nNewPos < si.nMin)
        nNewPos = si.nMin;
    if (nNewPos > (int)(si.nMax - (si.nPage ? si.nPage - 1 : 0)))
        nNewPos = si.nMax - (si.nPage ? si.nPage - 1 : 0);

    /* Update scrollbar position if changed */
    if (nNewPos != nOldPos)
    {
        si.fMask = SIF_POS;
        si.nPos = nNewPos;
        SetScrollInfo(hScroll, SB_CTL, &si, TRUE);
    }

    wsprintfW(szLog, L"[%s] %s -> Pos: %d", szBarName, szCodeName, nNewPos);
    LogScrollEvent(PageData->hLogEdit, szLog);
}

static VOID
OnSize(PSCROLLBAR_PAGE_DATA PageData, int cx, int cy)
{
    HDWP hdwp;
    int margin_x = 20;
    int col2_fixed_w = 120; /* Fixed width for the second column */
    int dyn_col_width;
    int col1_x, col2_x, col3_x;
    int col1_w, col2_w, col3_w;
    int vert_h, log_h;

    /* Prevent sizing calculations before controls are created */
    if (!PageData || !PageData->hLogEdit)
        return;

    /* Calculate dynamic widths for remaining columns */
    dyn_col_width = (cx - col2_fixed_w - (margin_x * 4)) / 2;
    if (dyn_col_width < 80) dyn_col_width = 80;

    col1_w = dyn_col_width;
    col2_w = col2_fixed_w;

    col1_x = margin_x;
    col2_x = col1_x + col1_w + margin_x;
    col3_x = col2_x + col2_w + margin_x;

    /* Have column 3 precisely fill the remaining available width */
    col3_w = cx - col3_x - margin_x;
    if (col3_w < 80) col3_w = 80;

    /* Calculate dynamic heights based on container bounds */
    vert_h = cy - 35 - 80;
    if (vert_h < 100) vert_h = 100;

    log_h = cy - 35 - 20;
    if (log_h < 100) log_h = 100;

    /* Batch window movements for flicker-free resizing */
    hdwp = BeginDeferWindowPos(10);

#define MOVE_CTRL(hwnd, x, y, w, h) \
    if (hwnd) hdwp = DeferWindowPos(hdwp, hwnd, NULL, x, y, w, h, SWP_NOZORDER)

    /* --- COLUMN 1: Horizontal Scrollbars --- */
    MOVE_CTRL(PageData->hLabelHorzStd,     col1_x,  15, col1_w, 15);
    MOVE_CTRL(PageData->hHorzStdScroll,    col1_x,  35, col1_w, 20);
    MOVE_CTRL(PageData->hLabelHorzBottom,  col1_x,  75, col1_w, 15);
    MOVE_CTRL(PageData->hHorzBottomScroll, col1_x,  95, col1_w, 20);

    /* --- COLUMN 2: Vertical Scrollbars & Size Grip --- */
    MOVE_CTRL(PageData->hLabelVertStd,     col2_x,  15, col2_w, 15);
    /* Center the vertical scrollbar inside the fixed column */
    MOVE_CTRL(PageData->hVertStdScroll,    col2_x + (col2_w / 2) - 10, 35, 20, vert_h);
    MOVE_CTRL(PageData->hLabelSizeGrip,    col2_x,  35 + vert_h + 10, col2_w, 15);
    MOVE_CTRL(PageData->hSizeGripParent,   col2_x,  35 + vert_h + 30, col2_w, 40);

    /* --- COLUMN 3: Event Log Display --- */
    MOVE_CTRL(PageData->hLabelLog,         col3_x,  15, col3_w, 15);
    MOVE_CTRL(PageData->hLogEdit,          col3_x,  35, col3_w, log_h);

#undef MOVE_CTRL

    EndDeferWindowPos(hdwp);
}

LRESULT
ScrollBarPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PSCROLLBAR_PAGE_DATA PageData;

    if (msg == WM_CREATE)
    {
        PageData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SCROLLBAR_PAGE_DATA));
        PageHost->UserData = PageData;

        InitPage(PageHost->Wnd, PageData);
        return TRUE;
    }
    else
    {
        PageData = (PSCROLLBAR_PAGE_DATA)PageHost->UserData;
    }

    switch (msg)
    {
        case WM_SIZE:
            OnSize(PageData, LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_HSCROLL:
        case WM_VSCROLL:
            if (lParam)
            {
                HWND hScroll = (HWND)lParam;
                LPCWSTR szName = L"Scrollbar";

                if (hScroll == PageData->hHorzStdScroll)
                    szName = L"Horz Standard";
                else if (hScroll == PageData->hHorzBottomScroll)
                    szName = L"Horz Bottom";
                else if (hScroll == PageData->hVertStdScroll)
                    szName = L"Vert Standard";

                HandleScrollNotification(hScroll, LOWORD(wParam), PageData, szName);
                return TRUE;
            }
            return FALSE;

        default:
            return FALSE;
    }
}
