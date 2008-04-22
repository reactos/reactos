/*
 * PROJECT:     ReactOS Desktop Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/cpl/desk/preview.c
 * PURPOSE:     Draws the preview control
 * COPYRIGHT:   Copyright 2006, 2007 Eric Kohl
 */

#include "desk.h"
#include "preview.h"

static const TCHAR szPreviewWndClass[] = TEXT("PreviewWndClass");

typedef struct _PREVIEW_DATA
{
    HWND hwndParent;

    DWORD clrSysColor[COLOR_MENUBAR];

    HBRUSH hbrScrollbar;
    HBRUSH hbrDesktop;
    HBRUSH hbrWindow;

    INT cxEdge;
    INT cyEdge;

    INT cySizeFrame;

    INT cyCaption;
    INT cyBorder;
    INT cyMenu;
    INT cxScrollbar;

    RECT rcDesktop;
    RECT rcInactiveFrame;
    RECT rcInactiveCaption;
    RECT rcInactiveCaptionButtons;

    RECT rcActiveFrame;
    RECT rcActiveCaption;
    RECT rcActiveCaptionButtons;
    RECT rcActiveMenuBar;
    RECT rcSelectedMenuItem;
    RECT rcActiveClient;
    RECT rcActiveScroll;

    RECT rcDialogFrame;
    RECT rcDialogCaption;
    RECT rcDialogCaptionButtons;
    RECT rcDialogClient;

    RECT rcDialogButton;

    LPTSTR lpInAct;
    LPTSTR lpAct;
    LPTSTR lpWinTxt;
    LPTSTR lpMessBox;
    LPTSTR lpMessText;
    LPTSTR lpButText;

    LOGFONT lfCaptionFont;
    LOGFONT lfMenuFont;
    LOGFONT lfMessageFont;

    HFONT hCaptionFont;
    HFONT hMenuFont;
    HFONT hMessageFont;

    HMENU hMenu;

} PREVIEW_DATA, *PPREVIEW_DATA;



static VOID
DrawCaptionButtons(HDC hdc, LPRECT lpRect, BOOL bMinMax, int x)
{
    RECT rc3;
    RECT rc4;
    RECT rc5;

    rc3.left = lpRect->right - 2 - x;
    rc3.top = lpRect->top + 2;
    rc3.right = lpRect->right - 2;
    rc3.bottom = lpRect->bottom - 2;

    DrawFrameControl(hdc, &rc3, DFC_CAPTION, DFCS_CAPTIONCLOSE);

    if (bMinMax)
    {
        rc4.left = rc3.left - x - 2;
        rc4.top = rc3.top;
        rc4.right = rc3.right - x - 2;
        rc4.bottom = rc3.bottom;

        DrawFrameControl(hdc, &rc4, DFC_CAPTION, DFCS_CAPTIONMAX);

        rc5.left = rc4.left - x;
        rc5.top = rc4.top;
        rc5.right = rc4.right - x;
        rc5.bottom = rc4.bottom;

        DrawFrameControl(hdc, &rc5, DFC_CAPTION, DFCS_CAPTIONMIN);
    }
}


static VOID
DrawScrollbar(HDC hdc, LPRECT rc, HBRUSH hbrScrollbar)
{
    RECT rcTop;
    RECT rcBottom;
    RECT rcMiddle;
    int width;

    width = rc->right - rc->left;

    rcTop.left = rc->left;
    rcTop.right = rc->right;
    rcTop.top = rc->top;
    rcTop.bottom = rc->top + width;

    rcMiddle.left = rc->left;
    rcMiddle.right = rc->right;
    rcMiddle.top = rc->top + width;
    rcMiddle.bottom = rc->bottom - width;

    rcBottom.left = rc->left;
    rcBottom.right = rc->right;
    rcBottom.top = rc->bottom - width;
    rcBottom.bottom = rc->bottom;

    DrawFrameControl(hdc, &rcTop, DFC_SCROLL, DFCS_SCROLLUP);
    DrawFrameControl(hdc, &rcBottom, DFC_SCROLL, DFCS_SCROLLDOWN);

    FillRect(hdc, &rcMiddle, hbrScrollbar);
}


static VOID
OnCreate(HWND hwnd, PPREVIEW_DATA pPreviewData)
{
    NONCLIENTMETRICS NonClientMetrics;
    INT i;

    for (i = 0; i < COLOR_MENUBAR + 1; i++)
    {
        pPreviewData->clrSysColor[i] = GetSysColor(i);
    }

    pPreviewData->hbrScrollbar = CreateSolidBrush(pPreviewData->clrSysColor[COLOR_SCROLLBAR]);
    pPreviewData->hbrDesktop = CreateSolidBrush(pPreviewData->clrSysColor[COLOR_DESKTOP]);
    pPreviewData->hbrWindow = CreateSolidBrush(pPreviewData->clrSysColor[COLOR_WINDOW]);

    pPreviewData->cxEdge = GetSystemMetrics(SM_CXEDGE) - 2;
    pPreviewData->cyEdge = GetSystemMetrics(SM_CXEDGE) - 2;

    pPreviewData->cySizeFrame = GetSystemMetrics(SM_CYSIZEFRAME) - 1;

    pPreviewData->cyCaption = GetSystemMetrics(SM_CYCAPTION);
    pPreviewData->cyMenu = GetSystemMetrics(SM_CYMENU);
    pPreviewData->cxScrollbar = GetSystemMetrics(SM_CXVSCROLL);
    pPreviewData->cyBorder = GetSystemMetrics(SM_CYBORDER);

    /* load font info */
    NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);

    pPreviewData->lfCaptionFont = NonClientMetrics.lfCaptionFont;
    pPreviewData->hCaptionFont = CreateFontIndirect(&pPreviewData->lfCaptionFont);

    pPreviewData->lfMenuFont = NonClientMetrics.lfMenuFont;
    pPreviewData->hMenuFont = CreateFontIndirect(&pPreviewData->lfMenuFont);

    pPreviewData->lfMessageFont = NonClientMetrics.lfMessageFont;
    pPreviewData->hMessageFont = CreateFontIndirect(&pPreviewData->lfMessageFont);

    /* Load and modify the menu */
    pPreviewData->hMenu = LoadMenu(hApplet, MAKEINTRESOURCE(IDR_PREVIEW_MENU));
    EnableMenuItem(pPreviewData->hMenu,
                   1, MF_BYPOSITION | MF_GRAYED);
    HiliteMenuItem(hwnd, pPreviewData->hMenu,
                   2, MF_BYPOSITION | MF_HILITE);

//    GetMenuItemRect(hwnd, pPreviewData->hMenu,
//                    2, &pPreviewData->rcSelectedMenuItem);

    AllocAndLoadString(&pPreviewData->lpInAct, hApplet, IDS_INACTWIN);
    AllocAndLoadString(&pPreviewData->lpAct, hApplet, IDS_ACTWIN);
    AllocAndLoadString(&pPreviewData->lpWinTxt, hApplet, IDS_WINTEXT);
    AllocAndLoadString(&pPreviewData->lpMessBox, hApplet, IDS_MESSBOX);
    AllocAndLoadString(&pPreviewData->lpMessText, hApplet, IDS_MESSTEXT);
    AllocAndLoadString(&pPreviewData->lpButText, hApplet, IDS_BUTTEXT);
}


static VOID
CalculateItemSize(PPREVIEW_DATA pPreviewData)
{
    int width, height;

    /* Calculate the inactive window rectangle */
    pPreviewData->rcInactiveFrame.left = pPreviewData->rcDesktop.left + 8;
    pPreviewData->rcInactiveFrame.top = pPreviewData->rcDesktop.top + 8;
    pPreviewData->rcInactiveFrame.right = pPreviewData->rcDesktop.right - 25;
    pPreviewData->rcInactiveFrame.bottom = pPreviewData->rcDesktop.bottom - 30;

    /* Calculate the inactive caption rectangle */
    pPreviewData->rcInactiveCaption.left = pPreviewData->rcInactiveFrame.left + pPreviewData->cxEdge + pPreviewData->cySizeFrame + 1;
    pPreviewData->rcInactiveCaption.top = pPreviewData->rcInactiveFrame.top + pPreviewData->cyEdge + pPreviewData->cySizeFrame + 1;
    pPreviewData->rcInactiveCaption.right = pPreviewData->rcInactiveFrame.right - pPreviewData->cxEdge - pPreviewData->cySizeFrame - 1;
    pPreviewData->rcInactiveCaption.bottom = pPreviewData->rcInactiveCaption.top + pPreviewData->cyCaption - pPreviewData->cyBorder;

    /* Calculate the inactive caption buttons rectangle */
    pPreviewData->rcInactiveCaptionButtons.left = pPreviewData->rcInactiveCaption.right - 2 - 2 - 3 * 16;
    pPreviewData->rcInactiveCaptionButtons.top = pPreviewData->rcInactiveCaption.top + 2;
    pPreviewData->rcInactiveCaptionButtons.right = pPreviewData->rcInactiveCaption.right - 2;
    pPreviewData->rcInactiveCaptionButtons.bottom = pPreviewData->rcInactiveCaption.bottom - 2;

    /* Calculate the active window rectangle */
    pPreviewData->rcActiveFrame.left = pPreviewData->rcInactiveFrame.left + 3 + pPreviewData->cySizeFrame;
    pPreviewData->rcActiveFrame.top = pPreviewData->rcInactiveCaption.bottom + 1;
    pPreviewData->rcActiveFrame.right = pPreviewData->rcDesktop.right - 10;
    pPreviewData->rcActiveFrame.bottom = pPreviewData->rcDesktop.bottom - 25;

    /* Calculate the active caption rectangle */
    pPreviewData->rcActiveCaption.left = pPreviewData->rcActiveFrame.left + pPreviewData->cxEdge + pPreviewData->cySizeFrame + 1;
    pPreviewData->rcActiveCaption.top = pPreviewData->rcActiveFrame.top + pPreviewData->cxEdge + pPreviewData->cySizeFrame + 1;
    pPreviewData->rcActiveCaption.right = pPreviewData->rcActiveFrame.right - pPreviewData->cxEdge - pPreviewData->cySizeFrame - 1;
    pPreviewData->rcActiveCaption.bottom = pPreviewData->rcActiveCaption.top + pPreviewData->cyCaption - pPreviewData->cyBorder;

    /* Calculate the active caption buttons rectangle */
    pPreviewData->rcActiveCaptionButtons.left = pPreviewData->rcActiveCaption.right - 2 - 2 - 3 * 16;
    pPreviewData->rcActiveCaptionButtons.top = pPreviewData->rcActiveCaption.top + 2;
    pPreviewData->rcActiveCaptionButtons.right = pPreviewData->rcActiveCaption.right - 2;
    pPreviewData->rcActiveCaptionButtons.bottom = pPreviewData->rcActiveCaption.bottom - 2;

    /* Calculate the active menu bar rectangle */
    pPreviewData->rcActiveMenuBar.left = pPreviewData->rcActiveFrame.left + pPreviewData->cxEdge + pPreviewData->cySizeFrame + 1;
    pPreviewData->rcActiveMenuBar.top = pPreviewData->rcActiveCaption.bottom + 1;
    pPreviewData->rcActiveMenuBar.right = pPreviewData->rcActiveFrame.right - pPreviewData->cxEdge - pPreviewData->cySizeFrame - 1;
    pPreviewData->rcActiveMenuBar.bottom = pPreviewData->rcActiveMenuBar.top + pPreviewData->cyMenu + 1;

    /* Calculate the active client rectangle */
    pPreviewData->rcActiveClient.left = pPreviewData->rcActiveFrame.left + pPreviewData->cxEdge + pPreviewData->cySizeFrame + 1;
    pPreviewData->rcActiveClient.top = pPreviewData->rcActiveMenuBar.bottom;
    pPreviewData->rcActiveClient.right = pPreviewData->rcActiveFrame.right - pPreviewData->cxEdge - pPreviewData->cySizeFrame - 1;
    pPreviewData->rcActiveClient.bottom = pPreviewData->rcActiveFrame.bottom - pPreviewData->cyEdge - pPreviewData->cySizeFrame - 1;

    /* Calculate the active scroll rectangle */
    pPreviewData->rcActiveScroll.left = pPreviewData->rcActiveClient.right - 2 - pPreviewData->cxScrollbar;
    pPreviewData->rcActiveScroll.top = pPreviewData->rcActiveClient.top + 2;
    pPreviewData->rcActiveScroll.right = pPreviewData->rcActiveClient.right - 2;
    pPreviewData->rcActiveScroll.bottom = pPreviewData->rcActiveClient.bottom - 2;


    /* Dialog window */
    pPreviewData->rcDialogFrame.left = pPreviewData->rcActiveClient.left + 4;
    pPreviewData->rcDialogFrame.top = (pPreviewData->rcDesktop.bottom * 60) / 100;
    pPreviewData->rcDialogFrame.right = (pPreviewData->rcDesktop.right * 65) / 100;
    pPreviewData->rcDialogFrame.bottom = pPreviewData->rcDesktop.bottom - 5;

    /* Calculate the dialog caption rectangle */
    pPreviewData->rcDialogCaption.left = pPreviewData->rcDialogFrame.left + 3;
    pPreviewData->rcDialogCaption.top = pPreviewData->rcDialogFrame.top + 3;
    pPreviewData->rcDialogCaption.right = pPreviewData->rcDialogFrame.right - 3;
    pPreviewData->rcDialogCaption.bottom = pPreviewData->rcDialogFrame.top + pPreviewData->cyCaption + 1 + 1;

    /* Calculate the inactive caption buttons rectangle */
    pPreviewData->rcDialogCaptionButtons.left = pPreviewData->rcDialogCaption.right - 2 - 16;
    pPreviewData->rcDialogCaptionButtons.top = pPreviewData->rcDialogCaption.top + 2;
    pPreviewData->rcDialogCaptionButtons.right = pPreviewData->rcDialogCaption.right - 2;
    pPreviewData->rcDialogCaptionButtons.bottom = pPreviewData->rcDialogCaption.bottom - 2;

    /* Calculate the dialog client rectangle */
    pPreviewData->rcDialogClient.left = pPreviewData->rcDialogFrame.left + 3;
    pPreviewData->rcDialogClient.top = pPreviewData->rcDialogCaption.bottom + 1;
    pPreviewData->rcDialogClient.right = pPreviewData->rcDialogFrame.right - 3;
    pPreviewData->rcDialogClient.bottom = pPreviewData->rcDialogFrame.bottom - 3;

    /* Calculate the dialog button rectangle */
    width = 80;
    height = 28;

    pPreviewData->rcDialogButton.left =
        (pPreviewData->rcDialogClient.right + pPreviewData->rcDialogClient.left - width) / 2;
    pPreviewData->rcDialogButton.right = pPreviewData->rcDialogButton.left + width;
    pPreviewData->rcDialogButton.bottom = pPreviewData->rcDialogClient.bottom - 2;
    pPreviewData->rcDialogButton.top = pPreviewData->rcDialogButton.bottom - height;
}


static VOID
OnSize(INT cx, INT cy, PPREVIEW_DATA pPreviewData)
{
    /* Get Desktop rectangle */
    pPreviewData->rcDesktop.left = 0;
    pPreviewData->rcDesktop.top = 0;
    pPreviewData->rcDesktop.right = cx;
    pPreviewData->rcDesktop.bottom = cy;

    CalculateItemSize(pPreviewData);
}

#ifdef _MSC_VER

#if _UNICODE
typedef BOOL (WINAPI * DCT_PROC)(HWND hwnd, HDC hdc, const RECT *rect, HFONT hFont, HICON hIcon, LPCWSTR str, UINT uFlags);
#define DCT_ORD 178
#else
typedef BOOL (WINAPI * DCT_PROC)(HWND hwnd, HDC hdc, const RECT *rect, HFONT hFont, HICON hIcon, LPCSTR str, UINT uFlags);
#define DCT_ORD 177
#endif

typedef DWORD (WINAPI * DMBT_PROC)(HWND hwnd, HDC hDC, LPRECT lprect, HMENU hMenu, HFONT hFont);
#define DMBT_ORD 186

#endif

static VOID
OnPaint(HWND hwnd, PPREVIEW_DATA pPreviewData)
{
    PAINTSTRUCT ps;
    HFONT hOldFont;
    HDC hdc;
    RECT rc;

#ifdef _MSC_VER
    DCT_PROC DrawCaptionTemp;
    DMBT_PROC DrawMenuBarTemp;
    HMODULE hUser32;

    hUser32 = LoadLibrary(_T("user32.dll"));
    if(hUser32 == NULL)
        return FALSE;

    DrawCaptionTemp = (DCT_PROC)GetProcAddress(hUser32, (LPCSTR)DCT_ORD);
    DrawMenuBarTemp = (DMBT_PROC)GetProcAddress(hUser32, (LPCSTR)DMBT_ORD);
#endif

    hdc = BeginPaint(hwnd, &ps);

    /* Desktop */
    FillRect(hdc, &pPreviewData->rcDesktop, pPreviewData->hbrDesktop);

    /* Inactive Window */
    DrawEdge(hdc, &pPreviewData->rcInactiveFrame, EDGE_RAISED, BF_RECT | BF_MIDDLE);
    SetTextColor(hdc, pPreviewData->clrSysColor[COLOR_INACTIVECAPTIONTEXT]);
    DrawCaptionTemp(NULL, hdc, &pPreviewData->rcInactiveCaption,  pPreviewData->hCaptionFont,
                    NULL, pPreviewData->lpInAct, DC_GRADIENT | DC_ICON | DC_TEXT);
    DrawCaptionButtons(hdc, &pPreviewData->rcInactiveCaption, TRUE, pPreviewData->cyCaption - 2);

    /* Active Window */
    DrawEdge(hdc, &pPreviewData->rcActiveFrame, EDGE_RAISED, BF_RECT | BF_MIDDLE);
    SetTextColor(hdc, pPreviewData->clrSysColor[COLOR_CAPTIONTEXT]);
    DrawCaptionTemp(NULL, hdc, &pPreviewData->rcActiveCaption, pPreviewData->hCaptionFont,
                    NULL, pPreviewData->lpAct, DC_ACTIVE | DC_GRADIENT | DC_ICON | DC_TEXT);
    DrawCaptionButtons(hdc, &pPreviewData->rcActiveCaption, TRUE, pPreviewData->cyCaption - 2);

    /* Draw the menu bar */
    DrawMenuBarTemp(hwnd, hdc, &pPreviewData->rcActiveMenuBar,
                    pPreviewData->hMenu,
                    pPreviewData->hMenuFont);

    /* Draw the client area */
    CopyRect(&rc, &pPreviewData->rcActiveClient);
    DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
    FillRect(hdc, &rc, pPreviewData->hbrWindow);

    /* Draw the client text */
    CopyRect(&rc, &pPreviewData->rcActiveClient);
    rc.left += 4;
    rc.top += 2;
    SetTextColor(hdc, pPreviewData->clrSysColor[COLOR_WINDOWTEXT]);
    hOldFont = SelectObject(hdc, pPreviewData->hCaptionFont);
    DrawText(hdc, pPreviewData->lpWinTxt, lstrlen(pPreviewData->lpWinTxt), &rc, DT_LEFT);
    SelectObject(hdc, hOldFont);

    /* Draw the scroll bar */
    DrawScrollbar(hdc, &pPreviewData->rcActiveScroll, pPreviewData->hbrScrollbar);

    /* Dialog Window */
    DrawEdge(hdc, &pPreviewData->rcDialogFrame, EDGE_RAISED, BF_RECT | BF_MIDDLE);
    SetTextColor(hdc, pPreviewData->clrSysColor[COLOR_WINDOW]);
    DrawCaptionTemp(NULL, hdc, &pPreviewData->rcDialogCaption, pPreviewData->hCaptionFont,
                    NULL, pPreviewData->lpMessBox, DC_ACTIVE | DC_GRADIENT | DC_ICON | DC_TEXT);
    DrawCaptionButtons(hdc, &pPreviewData->rcDialogCaption, FALSE, pPreviewData->cyCaption - 2);

    /* Draw the dialog text */
    CopyRect(&rc, &pPreviewData->rcDialogClient);
    rc.left += 4;
    rc.top += 2;
    SetTextColor(hdc, RGB(0,0,0));
    hOldFont = SelectObject(hdc, pPreviewData->hMessageFont);
    DrawText(hdc, pPreviewData->lpMessText, lstrlen(pPreviewData->lpMessText), &rc, DT_LEFT);
    SelectObject(hdc, hOldFont);

    /* Draw Button */
    DrawFrameControl(hdc, &pPreviewData->rcDialogButton, DFC_BUTTON, DFCS_BUTTONPUSH);
    CopyRect(&rc, &pPreviewData->rcDialogButton);
    SetTextColor(hdc, pPreviewData->clrSysColor[COLOR_BTNTEXT]);
    hOldFont = SelectObject(hdc, pPreviewData->hMessageFont);
    DrawText(hdc, pPreviewData->lpButText, lstrlen(pPreviewData->lpButText), &rc, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
    SelectObject(hdc, hOldFont);

    EndPaint(hwnd, &ps);
}


static VOID
OnLButtonDown(HWND hwnd, int xPos, int yPos, PPREVIEW_DATA pPreviewData)
{
    UINT type = IDX_DESKTOP;
    POINT pt;

    pt.x = xPos;
    pt.y = yPos;

    if (PtInRect(&pPreviewData->rcInactiveFrame, pt))
        type = IDX_INACTIVE_BORDER;

    if (PtInRect(&pPreviewData->rcInactiveCaption, pt))
        type = IDX_INACTIVE_CAPTION;

    if (PtInRect(&pPreviewData->rcInactiveCaptionButtons, pt))
        type = IDX_CAPTION_BUTTON;

    if (PtInRect(&pPreviewData->rcActiveFrame, pt))
        type = IDX_ACTIVE_BORDER;

    if (PtInRect(&pPreviewData->rcActiveCaption, pt))
        type = IDX_ACTIVE_CAPTION;

    if (PtInRect(&pPreviewData->rcActiveCaptionButtons, pt))
        type = IDX_CAPTION_BUTTON;

//    if (PtInRect(&pPreviewData->rcSelectedMenuItem, pt))
//        type = IDX_SELECTION;

    if (PtInRect(&pPreviewData->rcActiveMenuBar, pt))
        type = IDX_MENU;

    if (PtInRect(&pPreviewData->rcActiveClient, pt))
        type = IDX_WINDOW;

    if (PtInRect(&pPreviewData->rcActiveScroll, pt))
        type = IDX_SCROLLBAR;

    if (PtInRect(&pPreviewData->rcDialogFrame, pt))
        type = IDX_DIALOG;

    if (PtInRect(&pPreviewData->rcDialogCaption, pt))
        type = IDX_ACTIVE_CAPTION;

    if (PtInRect(&pPreviewData->rcDialogCaptionButtons, pt))
        type = IDX_CAPTION_BUTTON;

    if (PtInRect(&pPreviewData->rcDialogButton, pt))
        type = IDX_3D_OBJECTS;

    SendMessage(GetParent(hwnd),
                WM_COMMAND,
                MAKEWPARAM(GetWindowLongPtrW(hwnd, GWLP_ID), 0),
                (LPARAM)type);
}


static VOID
OnDestroy(PPREVIEW_DATA pPreviewData)
{
    DeleteObject(pPreviewData->hbrScrollbar);
    DeleteObject(pPreviewData->hbrDesktop);
    DeleteObject(pPreviewData->hbrWindow);

    DeleteObject(pPreviewData->hCaptionFont);
    DeleteObject(pPreviewData->hMenuFont);
    DeleteObject(pPreviewData->hMessageFont);

    DestroyMenu(pPreviewData->hMenu);

    LocalFree((HLOCAL)pPreviewData->lpInAct);
    LocalFree((HLOCAL)pPreviewData->lpAct);
    LocalFree((HLOCAL)pPreviewData->lpWinTxt);
    LocalFree((HLOCAL)pPreviewData->lpMessBox);
    LocalFree((HLOCAL)pPreviewData->lpMessText);
    LocalFree((HLOCAL)pPreviewData->lpButText);
}


static LRESULT CALLBACK
PreviewWndProc(HWND hwnd,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PPREVIEW_DATA pPreviewData;

    pPreviewData = (PPREVIEW_DATA)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_CREATE:
            pPreviewData = (PPREVIEW_DATA)HeapAlloc(GetProcessHeap(),
                                                    HEAP_ZERO_MEMORY,
                                                    sizeof(PREVIEW_DATA));
            if (!pPreviewData)
                return -1;

            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pPreviewData);
            OnCreate(hwnd, pPreviewData);
            break;

        case WM_SIZE:
            OnSize(LOWORD(lParam), HIWORD(lParam), pPreviewData);
            break;

        case WM_PAINT:
            OnPaint(hwnd, pPreviewData);
            break;

        case WM_LBUTTONDOWN:
            OnLButtonDown(hwnd, LOWORD(lParam), HIWORD(lParam), pPreviewData);
            break;

        case WM_DESTROY:
            OnDestroy(pPreviewData);
            HeapFree(GetProcessHeap(), 0, pPreviewData);
            break;

        case PVM_GETCYCAPTION:
            return pPreviewData->cyCaption;

        case PVM_SETCYCAPTION:
            if ((INT)lParam > 0)
            {
                pPreviewData->cyCaption = (INT)lParam;
                CalculateItemSize(pPreviewData);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;

        case PVM_GETCYMENU:
            return pPreviewData->cyMenu;

        case PVM_SETCYMENU:
            if ((INT)lParam > 0)
            {
                pPreviewData->cyMenu = (INT)lParam;
                CalculateItemSize(pPreviewData);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;

        case PVM_GETCXSCROLLBAR:
            return pPreviewData->cxScrollbar;

        case PVM_SETCXSCROLLBAR:
            if ((INT)lParam > 0)
            {
                pPreviewData->cxScrollbar = (INT)lParam;
                CalculateItemSize(pPreviewData);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;

        case PVM_GETCYSIZEFRAME:
            return pPreviewData->cySizeFrame;

        case PVM_SETCYSIZEFRAME:
            if ((INT)lParam > 0)
            {
                pPreviewData->cySizeFrame = (INT)lParam;
                CalculateItemSize(pPreviewData);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;

        case PVM_SETCAPTIONFONT:
            CopyMemory(&pPreviewData->lfCaptionFont, (LOGFONT*)lParam, sizeof(LOGFONT));
            DeleteObject(pPreviewData->hCaptionFont);
            pPreviewData->hCaptionFont = CreateFontIndirect(&pPreviewData->lfCaptionFont);
            CalculateItemSize(pPreviewData);
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case PVM_SETMENUFONT:
            CopyMemory(&pPreviewData->lfMenuFont, (LOGFONT*)lParam, sizeof(LOGFONT));
            DeleteObject(pPreviewData->hMenuFont);
            pPreviewData->hMenuFont = CreateFontIndirect(&pPreviewData->lfMenuFont);
            CalculateItemSize(pPreviewData);
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case PVM_SETDIALOGFONT:
            CopyMemory(&pPreviewData->lfMessageFont, (LOGFONT*)lParam, sizeof(LOGFONT));
            DeleteObject(pPreviewData->hMessageFont);
            pPreviewData->hMessageFont = CreateFontIndirect(&pPreviewData->lfMessageFont);
            CalculateItemSize(pPreviewData);
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case PVM_SETCOLOR:
            pPreviewData->clrSysColor[(INT)wParam] = (DWORD)lParam;
            switch((INT)wParam)
            {
                case COLOR_SCROLLBAR:
                    DeleteObject(pPreviewData->hbrScrollbar);
                    pPreviewData->hbrScrollbar = CreateSolidBrush(pPreviewData->clrSysColor[COLOR_SCROLLBAR]);
                    break;

                case COLOR_DESKTOP:
                    DeleteObject(pPreviewData->hbrDesktop);
                    pPreviewData->hbrDesktop = CreateSolidBrush(pPreviewData->clrSysColor[COLOR_DESKTOP]);
                    break;

                case COLOR_WINDOW:
                    DeleteObject(pPreviewData->hbrWindow);
                    pPreviewData->hbrWindow = CreateSolidBrush(pPreviewData->clrSysColor[COLOR_WINDOW]);
                    break;
            }
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        default:
            DefWindowProc(hwnd,
                          uMsg,
                          wParam,
                          lParam);
    }

    return TRUE;
}


BOOL
RegisterPreviewControl(IN HINSTANCE hInstance)
{
    WNDCLASSEX wc = {0};

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = PreviewWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)NULL;
    wc.lpszClassName = szPreviewWndClass;

    return RegisterClassEx(&wc) != (ATOM)0;
}


VOID
UnregisterPreviewControl(IN HINSTANCE hInstance)
{
    UnregisterClass(szPreviewWndClass,
                    hInstance);
}
