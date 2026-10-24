/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "../controls.h"

#define IDC_STATIC_NOTIFY 100001

typedef struct _STATIC_PAGE_DATA
{
    HWND hLeft;
    HWND hCenter;
    HWND hRight;

    HWND hNotify;

    HWND hPrefix;
    HWND hNoPrefix;

    HWND hBlackRect;
    HWND hGrayRect;
    HWND hWhiteRect;

    HWND hBlackFrame;
    HWND hGrayFrame;
    HWND hWhiteFrame;

    HWND hEtchedFrame;
    HWND hEtchedHorz;
    HWND hEtchedVert;

    HWND hBitmapButton;
    HWND hIconButton;
} STATIC_PAGE_DATA, *PSTATIC_PAGE_DATA;

static
LRESULT
InitPage(HWND Parent, PSTATIC_PAGE_DATA PageData)
{
    PageData->hLeft =
        CreateChild(0,
                    L"STATIC",
                    L"Left aligned",
                    SS_LEFT,
                    20, 20, 400, 20,
                    Parent);
    PageData->hCenter =
        CreateChild(0,
                    L"STATIC",
                    L"Center aligned",
                    SS_CENTER,
                    20, 40, 400, 20,
                    Parent);
    PageData->hRight =
        CreateChild(0,
                    L"STATIC",
                    L"Right aligned",
                    SS_RIGHT,
                    20, 60, 400, 20,
                    Parent);

    PageData->hNotify =
        CreateChild(IDC_STATIC_NOTIFY,
                    L"STATIC",
                    L"Click me (SS_NOTIFY)",
                    SS_NOTIFY,
                    20, 80, 180, 20,
                    Parent);

    PageData->hPrefix =
        CreateChild(0,
                    L"STATIC",
                    L"Prefix: E&xit",
                    SS_SIMPLE,
                    20, 100, 180, 20,
                    Parent);
    PageData->hNoPrefix =
        CreateChild(0,
                    L"STATIC",
                    L"NoPrefix: E&xit",
                    SS_NOPREFIX,
                    220, 100, 180, 20,
                    Parent);

    CreateChild(0,
                L"Static",
                L"A very long text here that should be ellipsed just because it's too long!",
                SS_WORDELLIPSIS,
                20, 120, 100, 20,
                Parent);

    PageData->hBlackRect =
        CreateChild(0,
                    L"STATIC",
                    NULL,
                    SS_BLACKRECT,
                    20, 150, 40, 40,
                    Parent);
    PageData->hGrayRect =
        CreateChild(0,
                    L"STATIC",
                    NULL,
                    SS_GRAYRECT,
                    80, 150, 40, 40,
                    Parent);
    PageData->hWhiteRect =
        CreateChild(0,
                    L"STATIC",
                    NULL,
                    SS_WHITERECT,
                    140, 150, 40, 40,
                    Parent);

    PageData->hBlackFrame =
        CreateChild(0,
                    L"STATIC",
                    NULL,
                    SS_BLACKFRAME,
                    200, 150, 40, 40,
                    Parent);
    PageData->hGrayFrame =
        CreateChild(0,
                    L"STATIC",
                    NULL,
                    SS_GRAYFRAME,
                    260, 150, 40, 40,
                    Parent);
    PageData->hWhiteFrame =
        CreateChild(0,
                    L"STATIC",
                    NULL,
                    SS_WHITEFRAME,
                    320, 150, 40, 40,
                    Parent);

    PageData->hEtchedFrame =
        CreateChild(0,
                    L"STATIC",
                    NULL,
                    SS_ETCHEDFRAME,
                    20, 200, 100, 100,
                    Parent);

    PageData->hEtchedHorz =
        CreateChild(0,
                    L"STATIC",
                    NULL,
                    SS_ETCHEDHORZ,
                    140, 200, 200, 2,
                    Parent);

    PageData->hEtchedVert =
        CreateChild(0,
                    L"STATIC",
                    NULL,
                    SS_ETCHEDVERT,
                    140, 220, 2, 80,
                    Parent);

    PageData->hIconButton =
        CreateChild(0,
                    L"STATIC",
                    NULL,
                    SS_ICON,
                    20, 320, 32, 32,
                    Parent);
    SendMessageW(PageData->hIconButton,
                 STM_SETICON,
                 (WPARAM)LoadIconW(NULL, IDI_INFORMATION),
                 0);

    PageData->hBitmapButton =
        CreateChild(0,
                    L"STATIC",
                    NULL,
                    SS_BITMAP,
                    80, 320, 32, 32,
                    Parent);
    SendMessageW(PageData->hBitmapButton,
                 STM_SETIMAGE,
                 IMAGE_BITMAP,
                 (LPARAM)LoadBitmapW(NULL,
                                     MAKEINTRESOURCEW(OBM_CLOSE)));

    return TRUE;
}

static VOID
OnSize(PSTATIC_PAGE_DATA PageData, int cx, int cy)
{
    HDWP hdwp;
    int margin_x = 20;
    int full_w;
    int half_w;

    if (!PageData || !PageData->hLeft)
        return;

    /* Rows 1, 2, 3: Full width calculation */
    full_w = cx - (margin_x * 2);
    if (full_w < 100) full_w = 100;

    /* Row 4 (Prefix/NoPrefix): Two equal columns */
    half_w = (cx - (margin_x * 3)) / 2;
    if (half_w < 50) half_w = 50;

    hdwp = BeginDeferWindowPos(5);

#define MOVE_CTRL(hwnd, x, y, w, h) \
    if (hwnd) hdwp = DeferWindowPos(hdwp, hwnd, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOCOPYBITS)

    /* Top three rows (Full Width) */
    MOVE_CTRL(PageData->hLeft,   margin_x, 20, full_w, 20);
    MOVE_CTRL(PageData->hCenter, margin_x, 40, full_w, 20);
    MOVE_CTRL(PageData->hRight,  margin_x, 60, full_w, 20);

    /* "Fourth" Row pair divided in 2 equal columns */
    MOVE_CTRL(PageData->hPrefix,   margin_x,                      100, half_w, 20);
    MOVE_CTRL(PageData->hNoPrefix, margin_x + half_w + margin_x,  100, half_w, 20);

#undef MOVE_CTRL

    EndDeferWindowPos(hdwp);
}

LRESULT
StaticPageProc(PPAGE_HOST PageHost,
               UINT msg,
               WPARAM wParam,
               LPARAM lParam)
{
    PSTATIC_PAGE_DATA PageData;

    if (msg == WM_CREATE)
    {
        PageData = HeapAlloc(GetProcessHeap(),
                             HEAP_ZERO_MEMORY,
                             sizeof(*PageData));

        if (!PageData)
            return -1;

        PageHost->UserData = PageData;
        return InitPage(PageHost->Wnd, PageData);
    }

    PageData = (PSTATIC_PAGE_DATA)PageHost->UserData;

    switch (msg)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_STATIC_NOTIFY)
            {
                SetWindowTextW(PageData->hNotify,
                               L"Clicked!");
                return TRUE;
            }
            break;

        case WM_SIZE:
            OnSize(PageData, LOWORD(lParam), HIWORD(lParam));
            return TRUE;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, PageData);
            PageHost->UserData = NULL;
            break;
    }

    return FALSE;
}
