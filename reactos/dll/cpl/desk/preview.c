/*
 * PROJECT:     ReactOS Timedate Control Panel
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

    DWORD clrDesktop;
    HBRUSH hbrDesktop;

    DWORD clrWindow;
    HBRUSH hbrWindow;

    DWORD clrScrollbar;
    HBRUSH hbrScrollbar;

    DWORD clrActiveCaptionText;
    DWORD clrInactiveCaptionText;
    DWORD clrWindowText;
    DWORD clrButtonText;

    INT cxEdge;
    INT cyEdge;

    INT cyCaption;

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
DrawCaptionButtons(HDC hdc, LPRECT lpRect, BOOL bMinMax)
{
    RECT rc3;
    RECT rc4;
    RECT rc5;

    rc3.left = lpRect->right - 2 - 16;
    rc3.top = lpRect->top + 2;
    rc3.right = lpRect->right - 2;
    rc3.bottom = lpRect->bottom - 2;

    DrawFrameControl(hdc, &rc3, DFC_CAPTION, DFCS_CAPTIONCLOSE);

    if (bMinMax)
    {
        rc4.left = rc3.left - 16 - 2;
        rc4.top = rc3.top;
        rc4.right = rc3.right - 16 - 2;
        rc4.bottom = rc3.bottom;

        DrawFrameControl(hdc, &rc4, DFC_CAPTION, DFCS_CAPTIONMAX);

        rc5.left = rc4.left - 16;
        rc5.top = rc4.top;
        rc5.right = rc4.right - 16;
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

    pPreviewData->clrScrollbar = GetSysColor(COLOR_SCROLLBAR);
    pPreviewData->hbrScrollbar = CreateSolidBrush(pPreviewData->clrScrollbar);

    pPreviewData->clrDesktop = GetSysColor(COLOR_DESKTOP);
    pPreviewData->hbrDesktop = CreateSolidBrush(pPreviewData->clrDesktop);
    pPreviewData->clrWindow = GetSysColor(COLOR_WINDOW);
    pPreviewData->hbrWindow = CreateSolidBrush(pPreviewData->clrWindow);

    pPreviewData->clrActiveCaptionText = GetSysColor(COLOR_CAPTIONTEXT);
    pPreviewData->clrInactiveCaptionText = GetSysColor(COLOR_INACTIVECAPTIONTEXT);
    pPreviewData->clrWindowText = GetSysColor(COLOR_WINDOWTEXT);
    pPreviewData->clrButtonText = GetSysColor(COLOR_BTNTEXT);

    pPreviewData->cxEdge = GetSystemMetrics(SM_CXEDGE);
    pPreviewData->cyEdge = GetSystemMetrics(SM_CXEDGE);

    pPreviewData->cyCaption = 20; //GetSystemMetrics(SM_CYCAPTION);

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
    EnableMenuItem(pPreviewData->hMenu, ID_MENU_DISABLED,
                   MF_BYCOMMAND | MF_DISABLED);
    HiliteMenuItem(hwnd, pPreviewData->hMenu,
                   ID_MENU_SELECTED, MF_BYCOMMAND | MF_HILITE);

//    GetMenuItemRect(hwnd, pPreviewData->hMenu,
//                    ID_MENU_SELECTED, &pPreviewData->rcSelectedMenuItem);


    AllocAndLoadString(&pPreviewData->lpInAct, hApplet, IDS_INACTWIN);
    AllocAndLoadString(&pPreviewData->lpAct, hApplet, IDS_ACTWIN);
    AllocAndLoadString(&pPreviewData->lpWinTxt, hApplet, IDS_WINTEXT);
    AllocAndLoadString(&pPreviewData->lpMessBox, hApplet, IDS_MESSBOX);
    AllocAndLoadString(&pPreviewData->lpMessText, hApplet, IDS_MESSTEXT);
    AllocAndLoadString(&pPreviewData->lpButText, hApplet, IDS_BUTTEXT);
}


static VOID
OnSize(INT cx, INT cy, PPREVIEW_DATA pPreviewData)
{
    int width, height;

    /* Get Desktop rectangle */
    pPreviewData->rcDesktop.left = 0;
    pPreviewData->rcDesktop.top = 0;
    pPreviewData->rcDesktop.right = cx;
    pPreviewData->rcDesktop.bottom = cy;

    /* Calculate the inactive window rectangle */
    pPreviewData->rcInactiveFrame.left = pPreviewData->rcDesktop.left + 8;
    pPreviewData->rcInactiveFrame.top = pPreviewData->rcDesktop.top + 8;
    pPreviewData->rcInactiveFrame.right = pPreviewData->rcDesktop.right - 25;
    pPreviewData->rcInactiveFrame.bottom = pPreviewData->rcDesktop.bottom - 30;

    /* Calculate the inactive caption rectangle */
    pPreviewData->rcInactiveCaption.left = pPreviewData->rcInactiveFrame.left + pPreviewData->cxEdge + 1/*3*/ + 1;
    pPreviewData->rcInactiveCaption.top = pPreviewData->rcInactiveFrame.top + pPreviewData->cyEdge + 1/*3*/ + 1;
    pPreviewData->rcInactiveCaption.right = pPreviewData->rcInactiveFrame.right - pPreviewData->cxEdge - 1/*3*/ - 1;
    pPreviewData->rcInactiveCaption.bottom = pPreviewData->rcInactiveFrame.top + pPreviewData->cyCaption /*20*/ + 2;

    /* Calculate the inactive caption buttons rectangle */
    pPreviewData->rcInactiveCaptionButtons.left = pPreviewData->rcInactiveCaption.right - 2 - 2 - 3 * 16;
    pPreviewData->rcInactiveCaptionButtons.top = pPreviewData->rcInactiveCaption.top + 2;
    pPreviewData->rcInactiveCaptionButtons.right = pPreviewData->rcInactiveCaption.right - 2;
    pPreviewData->rcInactiveCaptionButtons.bottom = pPreviewData->rcInactiveCaption.bottom - 2;

    /* Calculate the active window rectangle */
    pPreviewData->rcActiveFrame.left = pPreviewData->rcInactiveFrame.left + 3 + 1;
    pPreviewData->rcActiveFrame.top = pPreviewData->rcInactiveCaption.bottom + 1;
    pPreviewData->rcActiveFrame.right = pPreviewData->rcDesktop.right - 10;
    pPreviewData->rcActiveFrame.bottom = pPreviewData->rcDesktop.bottom - 25;

    /* Calculate the active caption rectangle */
    pPreviewData->rcActiveCaption.left = pPreviewData->rcActiveFrame.left + 3 + 1;
    pPreviewData->rcActiveCaption.top = pPreviewData->rcActiveFrame.top + 3 + 1;
    pPreviewData->rcActiveCaption.right = pPreviewData->rcActiveFrame.right - 3 - 1;
    pPreviewData->rcActiveCaption.bottom = pPreviewData->rcActiveFrame.top + pPreviewData->cyCaption/*20*/ + 2;

    /* Calculate the active caption buttons rectangle */
    pPreviewData->rcActiveCaptionButtons.left = pPreviewData->rcActiveCaption.right - 2 - 2 - 3 * 16;
    pPreviewData->rcActiveCaptionButtons.top = pPreviewData->rcActiveCaption.top + 2;
    pPreviewData->rcActiveCaptionButtons.right = pPreviewData->rcActiveCaption.right - 2;
    pPreviewData->rcActiveCaptionButtons.bottom = pPreviewData->rcActiveCaption.bottom - 2;

    /* Calculate the active menu bar rectangle */
    pPreviewData->rcActiveMenuBar.left = pPreviewData->rcActiveFrame.left + 3 + 1;
    pPreviewData->rcActiveMenuBar.top = pPreviewData->rcActiveCaption.bottom + 1;
    pPreviewData->rcActiveMenuBar.right = pPreviewData->rcActiveFrame.right - 3 - 1;
    pPreviewData->rcActiveMenuBar.bottom = pPreviewData->rcActiveMenuBar.top + 20;

    /* Calculate the active client rectangle */
    pPreviewData->rcActiveClient.left = pPreviewData->rcActiveFrame.left + 3 + 1;
    pPreviewData->rcActiveClient.top = pPreviewData->rcActiveMenuBar.bottom; // + 1;
    pPreviewData->rcActiveClient.right = pPreviewData->rcActiveFrame.right - 3 - 1;
    pPreviewData->rcActiveClient.bottom = pPreviewData->rcActiveFrame.bottom - 3 - 1;

    /* Calculate the active scroll rectangle */
    pPreviewData->rcActiveScroll.left = pPreviewData->rcActiveClient.right - 2 - 16;
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
    pPreviewData->rcDialogCaption.bottom = pPreviewData->rcDialogFrame.top + 20 + 1;

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
OnPaint(HWND hwnd, PPREVIEW_DATA pPreviewData)
{
    PAINTSTRUCT ps;
    HFONT hOldFont;
    HDC hdc;
    RECT rc;

    hdc = BeginPaint(hwnd, &ps);

    /* Desktop */
    FillRect(hdc, &pPreviewData->rcDesktop, pPreviewData->hbrDesktop);

    /* Inactive Window */
    DrawEdge(hdc, &pPreviewData->rcInactiveFrame, EDGE_RAISED, BF_RECT | BF_MIDDLE);
    SetTextColor(hdc, pPreviewData->clrInactiveCaptionText);
    DrawCaptionTemp(NULL, hdc, &pPreviewData->rcInactiveCaption,  pPreviewData->hCaptionFont,
                    NULL, pPreviewData->lpInAct, DC_GRADIENT | DC_ICON | DC_TEXT);
    DrawCaptionButtons(hdc, &pPreviewData->rcInactiveCaption, TRUE);

    /* Active Window */
    DrawEdge(hdc, &pPreviewData->rcActiveFrame, EDGE_RAISED, BF_RECT | BF_MIDDLE);
    SetTextColor(hdc, pPreviewData->clrActiveCaptionText);
    DrawCaptionTemp(NULL, hdc, &pPreviewData->rcActiveCaption, pPreviewData->hCaptionFont,
                    NULL, pPreviewData->lpAct, DC_ACTIVE | DC_GRADIENT | DC_ICON | DC_TEXT);
    DrawCaptionButtons(hdc, &pPreviewData->rcActiveCaption, TRUE);

    /* FIXME: Draw the menu bar */
    DrawMenuBarTemp(hwnd, hdc, &pPreviewData->rcActiveMenuBar,
                    pPreviewData->hMenu /*HMENU hMenu*/,
                    pPreviewData->hMessageFont /*HFONT hFont*/);

    /* Draw the client area */
    CopyRect(&rc, &pPreviewData->rcActiveClient);
    DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
    FillRect(hdc, &rc, pPreviewData->hbrWindow);

    /* Draw the client text */
    CopyRect(&rc, &pPreviewData->rcActiveClient);
    rc.left += 4;
    rc.top += 2;
    SetTextColor(hdc, pPreviewData->clrWindowText);
    hOldFont = SelectObject(hdc, pPreviewData->hMessageFont);
    DrawText(hdc, pPreviewData->lpWinTxt, lstrlen(pPreviewData->lpWinTxt), &rc, DT_LEFT);
    SelectObject(hdc, hOldFont);

    /* Draw the scroll bar */
    DrawScrollbar(hdc, &pPreviewData->rcActiveScroll, pPreviewData->hbrScrollbar);

    /* Dialog Window */
    DrawEdge(hdc, &pPreviewData->rcDialogFrame, EDGE_RAISED, BF_RECT | BF_MIDDLE);
    SetTextColor(hdc, pPreviewData->clrActiveCaptionText);
    DrawCaptionTemp(NULL, hdc, &pPreviewData->rcDialogCaption, pPreviewData->hCaptionFont,
                    NULL, pPreviewData->lpMessBox, DC_ACTIVE | DC_GRADIENT | DC_ICON | DC_TEXT);
    DrawCaptionButtons(hdc, &pPreviewData->rcDialogCaption, FALSE);

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
    SetTextColor(hdc, pPreviewData->clrButtonText);
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

    if (PtInRect(&pPreviewData->rcActiveMenuBar, pt))
        type = IDX_MENU;

//    if (PtInRect(&pPreviewData->rcSelectedMenuItem, pt))
//        type = IDX_SELECTION;

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

    SendMessage(GetParent(hwnd), WM_USER, 0, type);
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
    wc.hbrBackground = (HBRUSH)NULL; //(COLOR_BTNFACE + 1);
    wc.lpszClassName = szPreviewWndClass;

    return RegisterClassEx(&wc) != (ATOM)0;
}


VOID
UnregisterPreviewControl(IN HINSTANCE hInstance)
{
    UnregisterClass(szPreviewWndClass,
                    hInstance);
}
