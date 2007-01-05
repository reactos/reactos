/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/cpl/desk/preview.c
 * PURPOSE:     Draws the preview control
 * COPYRIGHT:   Copyright 2006 Eric Kohl
 */

#include "desk.h"
#include "preview.h"

static const TCHAR szPreviewWndClass[] = TEXT("PreviewWndClass");

typedef struct _PREVIEW_DATA
{
    DWORD clrDesktop;
    HBRUSH hbrDesktop;

    DWORD clrWindow;
    HBRUSH hbrWindow;

    INT cxEdge;
    INT cyEdge;

    INT cyCaption;

    RECT rcDesktop;
    RECT rcInactiveFrame;
    RECT rcInactiveCaption;

    RECT rcActiveFrame;
    RECT rcActiveCaption;
    RECT rcActiveClient;
    RECT rcActiveScroll;

    RECT rcDialogFrame;
    RECT rcDialogCaption;
    RECT rcDialogClient;

    RECT rcDialogButton;

    LPTSTR lpInAct;
    LPTSTR lpAct;
    LPTSTR lpWinTxt;
    LPTSTR lpMessBox;
    LPTSTR lpMessText;
    LPTSTR lpButText;
    LPTSTR lpMenNorm;
    LPTSTR lpMenDis;
    LPTSTR lpMenSel;

    LOGFONT CaptionFont;
    LOGFONT DialogFont;
    LOGFONT MenuFont;

} PREVIEW_DATA, *PPREVIEW_DATA;


/* HACK: fill the caption bar squares due to NULL text */
static VOID
FillSquare(HDC hdc, PRECT rect)
{
    INT x, y, i;

    x = rect->left + 2;
    y = rect->top + 2;

    for (i = 3; i < 12; i++)
    {
        BitBlt(hdc, x, y + i, 10, 1, hdc, x, y, SRCCOPY);
    }
}


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

    /* Calculate the active window rectangle */
    pPreviewData->rcActiveFrame.left = pPreviewData->rcInactiveFrame.left + 3 + 1;
    pPreviewData->rcActiveFrame.top = pPreviewData->rcInactiveCaption.bottom + 2;
    pPreviewData->rcActiveFrame.right = pPreviewData->rcDesktop.right - 10;
    pPreviewData->rcActiveFrame.bottom = pPreviewData->rcDesktop.bottom - 25;

    /* Calculate the active caption rectangle */
    pPreviewData->rcActiveCaption.left = pPreviewData->rcActiveFrame.left + 3 + 1;
    pPreviewData->rcActiveCaption.top = pPreviewData->rcActiveFrame.top + 3 + 1;
    pPreviewData->rcActiveCaption.right = pPreviewData->rcActiveFrame.right - 3 - 1;
    pPreviewData->rcActiveCaption.bottom = pPreviewData->rcActiveFrame.top + pPreviewData->cyCaption/*20*/ + 2;

    /* FIXME: Calculate the active menu bar rectangle */

    /* Calculate the active client rectangle */
    pPreviewData->rcActiveClient.left = pPreviewData->rcActiveFrame.left + 3 + 1;
    pPreviewData->rcActiveClient.top = pPreviewData->rcActiveCaption.bottom + 20 + 2;
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
    pPreviewData->rcDialogCaption.bottom = pPreviewData->rcDialogFrame.top + 20 + 2;

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
DrawScrollbar(HDC hdc, LPRECT rc)
{

}


static LRESULT CALLBACK
PreviewWndProc(HWND hwnd,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PPREVIEW_DATA pPreviewData;
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rc;

    pPreviewData = (PPREVIEW_DATA)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_CREATE:
        {
            NONCLIENTMETRICS NonClientMetrics;

            pPreviewData = (PPREVIEW_DATA)HeapAlloc(GetProcessHeap(),
                                                    HEAP_ZERO_MEMORY,
                                                    sizeof(PREVIEW_DATA));
            if (!pPreviewData)
                return -1;

            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pPreviewData);
            pPreviewData->clrDesktop = GetSysColor(COLOR_DESKTOP);
            pPreviewData->hbrDesktop = CreateSolidBrush(pPreviewData->clrDesktop);
            pPreviewData->clrWindow = GetSysColor(COLOR_WINDOW);
            pPreviewData->hbrWindow = CreateSolidBrush(pPreviewData->clrWindow);

            pPreviewData->cxEdge = GetSystemMetrics(SM_CXEDGE);
            pPreviewData->cyEdge = GetSystemMetrics(SM_CXEDGE);

            pPreviewData->cyCaption = 20; //GetSystemMetrics(SM_CYCAPTION);

            /* load font info */
            NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
            pPreviewData->CaptionFont = NonClientMetrics.lfCaptionFont;
            pPreviewData->MenuFont = NonClientMetrics.lfMenuFont;
            pPreviewData->DialogFont = NonClientMetrics.lfMessageFont;

            AllocAndLoadString(&pPreviewData->lpInAct, hApplet, IDS_INACTWIN);
            AllocAndLoadString(&pPreviewData->lpAct, hApplet, IDS_ACTWIN);
            AllocAndLoadString(&pPreviewData->lpWinTxt, hApplet, IDS_WINTEXT);
            AllocAndLoadString(&pPreviewData->lpMessBox, hApplet, IDS_MESSBOX);
            AllocAndLoadString(&pPreviewData->lpMessText, hApplet, IDS_MESSTEXT);
            AllocAndLoadString(&pPreviewData->lpButText, hApplet, IDS_BUTTEXT);
            AllocAndLoadString(&pPreviewData->lpMenNorm, hApplet, IDS_NORMAL);
            AllocAndLoadString(&pPreviewData->lpMenDis, hApplet, IDS_DISABLED);
            AllocAndLoadString(&pPreviewData->lpMenSel, hApplet, IDS_SELECTED);

            break;
        }

        case WM_SIZE:
            OnSize(LOWORD(lParam), HIWORD(lParam), pPreviewData);
            break;

        case WM_PAINT:
        {
            RECT tmpRc;

            hdc = BeginPaint(hwnd, &ps);

            /* Desktop */
            FillRect(hdc, &pPreviewData->rcDesktop, pPreviewData->hbrDesktop);

            /* Inactive Window */
            DrawEdge(hdc, &pPreviewData->rcInactiveFrame, EDGE_RAISED, BF_RECT | BF_MIDDLE);
            DrawCaption(hwnd, hdc, &pPreviewData->rcInactiveCaption, DC_GRADIENT | DC_TEXT);
            DrawCaptionButtons(hdc, &pPreviewData->rcInactiveCaption, TRUE);
            FillSquare(hdc, &pPreviewData->rcInactiveCaption);
            CopyRect(&tmpRc, &pPreviewData->rcInactiveCaption);
            tmpRc.left += 4;
            tmpRc.top += 2;
            SelectObject(hdc, CreateFontIndirect(&pPreviewData->CaptionFont));
            SetTextColor(hdc, RGB(212,208,200));
            DrawText(hdc, pPreviewData->lpInAct, lstrlen(pPreviewData->lpInAct), &tmpRc, DT_LEFT);
            DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));

            /* Active Window */
            DrawEdge(hdc, &pPreviewData->rcActiveFrame, EDGE_RAISED, BF_RECT | BF_MIDDLE);
            DrawCaption(hwnd, hdc, &pPreviewData->rcActiveCaption, DC_ACTIVE | DC_GRADIENT | DC_TEXT);
            DrawCaptionButtons(hdc, &pPreviewData->rcActiveCaption, TRUE);
            FillSquare(hdc, &pPreviewData->rcActiveCaption);
            CopyRect(&tmpRc, &pPreviewData->rcActiveCaption);
            tmpRc.left += 4;
            tmpRc.top += 2;
            SetTextColor(hdc, RGB(255,255,255)); // FIXME: don't hardcode colors
            SelectObject(hdc, CreateFontIndirect(&pPreviewData->CaptionFont));
            DrawText(hdc, pPreviewData->lpAct, lstrlen(pPreviewData->lpAct), &tmpRc, DT_LEFT);
            DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));

            /* FIXME: Draw the menu bar */
            CopyRect(&rc, &pPreviewData->rcActiveClient);
            DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
            FillRect(hdc, &rc, pPreviewData->hbrWindow);

            /* Draw the client text */
            CopyRect(&tmpRc, &pPreviewData->rcActiveClient);
            tmpRc.left += 4;
            tmpRc.top += 2;
            SetTextColor(hdc, RGB(0,0,0));
            SelectObject(hdc, CreateFontIndirect(&pPreviewData->DialogFont));
            DrawText(hdc, pPreviewData->lpWinTxt, lstrlen(pPreviewData->lpWinTxt), &tmpRc, DT_LEFT);
            DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));

            /* Draw the scroll bar */
            DrawScrollbar(hdc, &pPreviewData->rcActiveScroll);

            /* Dialog Window */
            DrawEdge(hdc, &pPreviewData->rcDialogFrame, EDGE_RAISED, BF_RECT | BF_MIDDLE);
            DrawCaption(hwnd, hdc, &pPreviewData->rcDialogCaption, DC_ACTIVE | DC_GRADIENT | DC_TEXT);
            DrawCaptionButtons(hdc, &pPreviewData->rcDialogCaption, FALSE);
            FillSquare(hdc, &pPreviewData->rcDialogCaption);
            CopyRect(&tmpRc, &pPreviewData->rcDialogCaption);
            tmpRc.left += 4;
            tmpRc.top += 2;
            SetTextColor(hdc, RGB(255,255,255));
            SelectObject(hdc, CreateFontIndirect(&pPreviewData->CaptionFont));
            DrawText(hdc, pPreviewData->lpMessBox, lstrlen(pPreviewData->lpMessBox), &tmpRc, DT_LEFT);
            DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));

            /* draw the dialog text */
            CopyRect(&tmpRc, &pPreviewData->rcDialogClient);
            tmpRc.left += 4;
            tmpRc.top += 2;
            SetTextColor(hdc, RGB(0,0,0));
            SelectObject(hdc, CreateFontIndirect(&pPreviewData->DialogFont));
            DrawText(hdc, pPreviewData->lpMessText, lstrlen(pPreviewData->lpMessText), &tmpRc, DT_LEFT);
            DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));

            /* Draw Button */
            DrawFrameControl(hdc, &pPreviewData->rcDialogButton, DFC_BUTTON, DFCS_BUTTONPUSH);
            CopyRect(&tmpRc, &pPreviewData->rcDialogButton);
            tmpRc.top += 6;
            SelectObject(hdc, CreateFontIndirect(&pPreviewData->DialogFont));
            DrawText(hdc, pPreviewData->lpButText, lstrlen(pPreviewData->lpButText), &tmpRc, DT_CENTER);
            DeleteObject(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));

            EndPaint(hwnd, &ps);
            break;
        }

        case WM_DESTROY:
            DeleteObject(pPreviewData->hbrDesktop);
            DeleteObject(pPreviewData->hbrWindow);

            LocalFree((HLOCAL)pPreviewData->lpInAct);
            LocalFree((HLOCAL)pPreviewData->lpAct);
            LocalFree((HLOCAL)pPreviewData->lpWinTxt);
            LocalFree((HLOCAL)pPreviewData->lpMessBox);
            LocalFree((HLOCAL)pPreviewData->lpMessText);
            LocalFree((HLOCAL)pPreviewData->lpButText);
            LocalFree((HLOCAL)pPreviewData->lpMenNorm);
            LocalFree((HLOCAL)pPreviewData->lpMenDis);
            LocalFree((HLOCAL)pPreviewData->lpMenSel);

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
