/*  LOOKPREV.C
**
**  Copyright (C) Microsoft, 1993, All Rights Reserved.
**
**
**  History:
**
*/
#include <windows.h>
#include "desk.h"
#include "deskid.h"
#include "look.h"

#define RCZ(element)         g_elements[element].rc

TCHAR g_szActive[40];
TCHAR g_szInactive[40];
TCHAR g_szMinimized[40];
TCHAR g_szIconTitle[40];
TCHAR g_szNormal[40];
TCHAR g_szDisabled[40];
TCHAR g_szSelected[40];
TCHAR g_szMsgBox[40];
TCHAR g_szButton[40];
//TCHAR g_szSmallCaption[40];
TCHAR g_szWindowText[40];
TCHAR g_szMsgBoxText[40];
TCHAR g_szABC[] = TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

int cxSize;

HMENU g_hmenuSample;
HBITMAP g_hbmLook = NULL;       // bitmap for the appearance preview

/*
** Note that the rectangles are defined back to front.  So we walk through
** the list backwards, checking PtInRect() until we find a match.
*/
int NEAR PASCAL LookPrev_HitTest(POINT pt)
{
    int i;

    // start with last, don't bother with 0 (desktop)... it's the fallback
    for (i = NUM_ELEMENTS - 1; i > 0; i--)
        if (PtInRect(&RCZ(i), pt))
            break;

    // if the chosen one is really a dupe of another, use the base one
    if (g_elements[i].iBaseElement != -1)
        i = g_elements[i].iBaseElement;

    return i;
}

// ----------------------------------------------------------------------------
// LookPrev_Init
// 
// create the preview bitmap and collect all of the global, non-changing data
// ----------------------------------------------------------------------------
void NEAR PASCAL LookPrev_Init(HWND hwnd)
{
    RECT rc;
    HDC hdc;

    GetClientRect(hwnd, &rc);
    hdc = GetDC(NULL);
    g_hbmLook = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
    ReleaseDC(NULL, hdc);

    //
    // Load our display strings.
    //
    LoadString(hInstance, IDS_ACTIVE, g_szActive, ARRAYSIZE(g_szActive));
    LoadString(hInstance, IDS_INACTIVE, g_szInactive, ARRAYSIZE(g_szInactive));
    LoadString(hInstance, IDS_MINIMIZED, g_szMinimized, ARRAYSIZE(g_szMinimized));
    LoadString(hInstance, IDS_ICONTITLE, g_szIconTitle, ARRAYSIZE(g_szIconTitle));
    LoadString(hInstance, IDS_NORMAL, g_szNormal, ARRAYSIZE(g_szNormal));
    LoadString(hInstance, IDS_DISABLED, g_szDisabled, ARRAYSIZE(g_szDisabled));
    LoadString(hInstance, IDS_SELECTED, g_szSelected, ARRAYSIZE(g_szSelected));
    LoadString(hInstance, IDS_MSGBOX, g_szMsgBox, ARRAYSIZE(g_szMsgBox));
    LoadString(hInstance, IDS_BUTTONTEXT, g_szButton, ARRAYSIZE(g_szButton));
//    LoadString(hInstance, IDS_SMCAPTION, g_szSmallCaption, ARRAYSIZE(g_szSmallCaption));
    LoadString(hInstance, IDS_WINDOWTEXT, g_szWindowText, ARRAYSIZE(g_szWindowText));
    LoadString(hInstance, IDS_MSGBOXTEXT, g_szMsgBoxText, ARRAYSIZE(g_szMsgBoxText));

    // load up and
    g_hmenuSample = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU));
    EnableMenuItem(g_hmenuSample, IDM_DISABLED, MF_GRAYED | MF_BYCOMMAND);

    // can do this here because hwnd is not yet visible
    HiliteMenuItem(hwnd, g_hmenuSample, IDM_SELECTED, MF_HILITE | MF_BYCOMMAND);
}

// ----------------------------------------------------------------------------
// LookPrev_Recalc
// 
// calculate all of the rectangles based on the given window rect
// ----------------------------------------------------------------------------
void FAR PASCAL LookPrev_Recalc(HWND hwnd)
{
    DWORD xyActive, xyInactive;
    DWORD cxNormal;
    DWORD xyButton;
    int cxDisabled, cxSelected;
    int cxAvgCharx2;
    RECT rc;
    HFONT hfontT;
    int cxFrame, cyFrame;
    int cyCaption;
    int i;
    SIZE sizButton;

    GetClientRect(hwnd, &rc);

    //
    // Get our drawing data
    //
    cxSize = GetSystemMetrics(SM_CXSIZE);
    cxFrame = (g_sizes[SIZE_FRAME].CurSize + 1) * cxBorder + cxEdge;
    cyFrame = (g_sizes[SIZE_FRAME].CurSize + 1) * cyBorder + cyEdge;
    cyCaption = g_sizes[SIZE_CAPTION].CurSize;

    //
    // Get text dimensions, with proper font.
    //

    hfontT = SelectObject(g_hdcMem, g_fonts[FONT_MENU].hfont);

    GetTextExtentPoint32(g_hdcMem, g_szNormal, lstrlen(g_szNormal), &sizButton);
    cxNormal = sizButton.cx;

    GetTextExtentPoint32(g_hdcMem, g_szDisabled, lstrlen(g_szDisabled), &sizButton);
    cxDisabled = sizButton.cx;

    GetTextExtentPoint32(g_hdcMem, g_szSelected, lstrlen(g_szSelected), &sizButton);
    cxSelected = sizButton.cx;

    // get the average width (USER style) of menu font
    GetTextExtentPoint32(g_hdcMem, g_szABC, 52, &sizButton);
    cxAvgCharx2 = 2 * (sizButton.cx / 52);

    // actual menu-handling widths of strings is bigger
    cxDisabled += cxAvgCharx2;
    cxSelected += cxAvgCharx2;
    cxNormal += cxAvgCharx2;

    SelectObject(g_hdcMem, hfontT);

    GetTextExtentPoint32(g_hdcMem, g_szButton, lstrlen(g_szButton), &sizButton);

    //
    // Desktop
    //
    RCZ(ELEMENT_DESKTOP) = rc;

    InflateRect(&rc, -8*cxBorder, -8*cyBorder);

    //
    // Windows
    //
    rc.bottom -= cyFrame + cyCaption;
    RCZ(ELEMENT_ACTIVEBORDER) = rc;
    OffsetRect(&RCZ(ELEMENT_ACTIVEBORDER), cxFrame,
                        cyFrame + cyCaption + cyBorder);
    RCZ(ELEMENT_ACTIVEBORDER).bottom -= cyCaption;

    //
    // Inactive window
    //

    rc.right -= cyCaption;
    RCZ(ELEMENT_INACTIVEBORDER) = rc;

    // Caption
    InflateRect(&rc, -cxFrame, -cyFrame);
    rc.bottom = rc.top + cyCaption + cyBorder;
    RCZ(ELEMENT_INACTIVECAPTION) = rc;

    // close button
    InflateRect(&rc, -cxEdge, -cyEdge);
    rc.bottom -= cyBorder;      // compensate for magic line under caption
    RCZ(ELEMENT_INACTIVESYSBUT1) = rc;
    RCZ(ELEMENT_INACTIVESYSBUT1).left = rc.right - (cyCaption - cxEdge);

    // min/max buttons
    RCZ(ELEMENT_INACTIVESYSBUT2) = rc;
    RCZ(ELEMENT_INACTIVESYSBUT2).right = RCZ(ELEMENT_INACTIVESYSBUT1).left - cxEdge;
    RCZ(ELEMENT_INACTIVESYSBUT2).left = RCZ(ELEMENT_INACTIVESYSBUT2).right - 
                                                2 * (cyCaption - cxEdge);

#if 0
    //
    // small caption window
    //
    RCZ(ELEMENT_SMCAPTION) = RCZ(ELEMENT_ACTIVEBORDER);
    RCZ(ELEMENT_SMCAPTION).bottom = RCZ(ELEMENT_SMCAPTION).top;
    RCZ(ELEMENT_SMCAPTION).top -= g_sizes[SIZE_SMCAPTION].CurSize + cyEdge + 2 * cyBorder;
    RCZ(ELEMENT_SMCAPTION).right -= cxFrame;
    RCZ(ELEMENT_SMCAPTION).left = RCZ(ELEMENT_INACTIVECAPTION).right + 2 * cxFrame;

    RCZ(ELEMENT_SMCAPSYSBUT) = RCZ(ELEMENT_SMCAPTION);
    // deflate inside frame/border to caption and then another edge's worth
    RCZ(ELEMENT_SMCAPSYSBUT).right -= 2 * cxEdge + cxBorder;
    RCZ(ELEMENT_SMCAPSYSBUT).top += 2 * cxEdge + cxBorder;
    RCZ(ELEMENT_SMCAPSYSBUT).bottom -= cxEdge + cxBorder;
    RCZ(ELEMENT_SMCAPSYSBUT).left = RCZ(ELEMENT_SMCAPSYSBUT).right - 
                                        (g_sizes[SIZE_SMCAPTION].CurSize - cxEdge);
#endif

    //
    // Active window
    //

    // Caption
    rc = RCZ(ELEMENT_ACTIVEBORDER);
    InflateRect(&rc, -cxFrame, -cyFrame);
    RCZ(ELEMENT_ACTIVECAPTION) = rc;
    RCZ(ELEMENT_ACTIVECAPTION).bottom = 
        RCZ(ELEMENT_ACTIVECAPTION).top + cyCaption + cyBorder;

    // close button
    RCZ(ELEMENT_ACTIVESYSBUT1) = RCZ(ELEMENT_ACTIVECAPTION);
    InflateRect(&RCZ(ELEMENT_ACTIVESYSBUT1), -cxEdge, -cyEdge);
    RCZ(ELEMENT_ACTIVESYSBUT1).bottom -= cyBorder;      // compensate for magic line under caption
    RCZ(ELEMENT_ACTIVESYSBUT1).left = RCZ(ELEMENT_ACTIVESYSBUT1).right - 
                                        (cyCaption - cxEdge);

    // min/max buttons
    RCZ(ELEMENT_ACTIVESYSBUT2) = RCZ(ELEMENT_ACTIVESYSBUT1);
    RCZ(ELEMENT_ACTIVESYSBUT2).right = RCZ(ELEMENT_ACTIVESYSBUT1).left - cxEdge;
    RCZ(ELEMENT_ACTIVESYSBUT2).left = RCZ(ELEMENT_ACTIVESYSBUT2).right - 
                                                2 * (cyCaption - cxEdge);

    // Menu
    rc.top = RCZ(ELEMENT_ACTIVECAPTION).bottom;
    RCZ(ELEMENT_MENUNORMAL) = rc;
    rc.top = RCZ(ELEMENT_MENUNORMAL).bottom = RCZ(ELEMENT_MENUNORMAL).top + g_sizes[SIZE_MENU].CurSize;
    RCZ(ELEMENT_MENUDISABLED) = RCZ(ELEMENT_MENUSELECTED) = RCZ(ELEMENT_MENUNORMAL);

    RCZ(ELEMENT_MENUDISABLED).left = RCZ(ELEMENT_MENUNORMAL).left + cxNormal;
    RCZ(ELEMENT_MENUDISABLED).right = RCZ(ELEMENT_MENUSELECTED).left = 
                        RCZ(ELEMENT_MENUDISABLED).left + cxDisabled;
    RCZ(ELEMENT_MENUSELECTED).right = RCZ(ELEMENT_MENUSELECTED).left + cxSelected;
    
    //
    // Client
    //
    RCZ(ELEMENT_WINDOW) = rc;

    //
    // Scrollbar
    //
    InflateRect(&rc, -cxEdge, -cyEdge); // take off client edge
    RCZ(ELEMENT_SCROLLBAR) = rc;
    rc.right = RCZ(ELEMENT_SCROLLBAR).left = rc.right - g_sizes[SIZE_SCROLL].CurSize;
    RCZ(ELEMENT_SCROLLUP) = RCZ(ELEMENT_SCROLLBAR);
    RCZ(ELEMENT_SCROLLUP).bottom = RCZ(ELEMENT_SCROLLBAR).top + g_sizes[SIZE_SCROLL].CurSize; 

    RCZ(ELEMENT_SCROLLDOWN) = RCZ(ELEMENT_SCROLLBAR);
    RCZ(ELEMENT_SCROLLDOWN).top = RCZ(ELEMENT_SCROLLBAR).bottom - g_sizes[SIZE_SCROLL].CurSize; 

    //
    // Message Box
    //
    rc.top = RCZ(ELEMENT_WINDOW).top + (RCZ(ELEMENT_WINDOW).bottom - RCZ(ELEMENT_WINDOW).top) / 2;
    rc.bottom = RCZ(ELEMENT_DESKTOP).bottom - 2*cyEdge;
    rc.left = RCZ(ELEMENT_WINDOW).left + 2*cyEdge;
    rc.right = RCZ(ELEMENT_WINDOW).left + (RCZ(ELEMENT_WINDOW).right - RCZ(ELEMENT_WINDOW).left) / 2 + 3*cyCaption;
    RCZ(ELEMENT_MSGBOX) = rc;

    // Caption
    RCZ(ELEMENT_MSGBOXCAPTION) = rc;
    RCZ(ELEMENT_MSGBOXCAPTION).top += cyEdge + cyBorder;
    RCZ(ELEMENT_MSGBOXCAPTION).bottom = RCZ(ELEMENT_MSGBOXCAPTION).top + cyCaption + cyBorder;
    RCZ(ELEMENT_MSGBOXCAPTION).left += cxEdge + cxBorder;
    RCZ(ELEMENT_MSGBOXCAPTION).right -= cxEdge + cxBorder;

    RCZ(ELEMENT_MSGBOXSYSBUT) = RCZ(ELEMENT_MSGBOXCAPTION);
    InflateRect(&RCZ(ELEMENT_MSGBOXSYSBUT), -cxEdge, -cyEdge);
    RCZ(ELEMENT_MSGBOXSYSBUT).left = RCZ(ELEMENT_MSGBOXSYSBUT).right - 
                                        (cyCaption - cxEdge);
    RCZ(ELEMENT_MSGBOXSYSBUT).bottom -= cyBorder;       // line under caption

    // Button
    RCZ(ELEMENT_BUTTON).bottom = RCZ(ELEMENT_MSGBOX).bottom - (4*cyBorder + cyEdge);
    RCZ(ELEMENT_BUTTON).top = RCZ(ELEMENT_BUTTON).bottom - (sizButton.cy + 8 * cyBorder);

    i = (RCZ(ELEMENT_BUTTON).bottom - RCZ(ELEMENT_BUTTON).top) * 3;
    RCZ(ELEMENT_BUTTON).left = (rc.left + (rc.right - rc.left)/2) - i/2;
    RCZ(ELEMENT_BUTTON).right = RCZ(ELEMENT_BUTTON).left + i;
}

// ----------------------------------------------------------------------------
//
//  MyDrawFrame() -
//
//  Draws bordered frame, border size cl, and adjusts passed in rect.
//
// ----------------------------------------------------------------------------
void NEAR PASCAL MyDrawFrame(HDC hdc, LPRECT prc, HBRUSH hbrColor, int cl)
{
    HBRUSH hbr;
    int cx, cy;
    RECT rcT;

    rcT = *prc;
    cx = cl * cxBorder;
    cy = cl * cyBorder;

    hbr = SelectObject(hdc, hbrColor);

    PatBlt(hdc, rcT.left, rcT.top, cx, rcT.bottom - rcT.top, PATCOPY);
    rcT.left += cx;

    PatBlt(hdc, rcT.left, rcT.top, rcT.right - rcT.left, cy, PATCOPY);
    rcT.top += cy;

    rcT.right -= cx;
    PatBlt(hdc, rcT.right, rcT.top, cx, rcT.bottom - rcT.top, PATCOPY);

    rcT.bottom -= cy;
    PatBlt(hdc, rcT.left, rcT.bottom, rcT.right - rcT.left, cy, PATCOPY);

    hbr = SelectObject(hdc, hbr);

    *prc = rcT;
}

/*
** draw a cyBorder band of 3DFACE at the bottom of the given rectangle.
** also, adjust the rectangle accordingly.
*/
void NEAR PASCAL MyDrawBorderBelow(HDC hdc, LPRECT prc)
{
    int i;

    i = prc->top;
    prc->top = prc->bottom - cyBorder;
    FillRect(hdc, prc, g_brushes[COLOR_3DFACE]);
    prc->top = i;
    prc->bottom -= cyBorder;
}

/*-------------------------------------------------------------------
** draw a full window caption with system menu, minimize button,
** maximize button, and text.
**-------------------------------------------------------------------*/
void NEAR PASCAL DrawFullCaption(HDC hdc, LPRECT prc, LPTSTR lpszTitle, UINT flags)
{
    int iRight;
    int iFont;

    SaveDC(hdc);

    // special case gross for small caption that already drew on bottom
    if (!(flags & DC_SMALLCAP))
        MyDrawBorderBelow(hdc, prc);

    iRight = prc->right;
    prc->right = prc->left + cxSize;
    DrawFrameControl(hdc, prc, DFC_CAPTION, DFCS_CAPTIONCLOSE);

    prc->left = prc->right;
    prc->right = iRight - 2*cxSize;
    iFont = flags & DC_SMALLCAP ? FONT_SMCAPTION : FONT_CAPTION;
    DrawCaptionTemp(NULL, hdc, prc, g_fonts[iFont].hfont, NULL, lpszTitle, flags | DC_ICON | DC_TEXT);

    prc->left = prc->right;
    prc->right = prc->left + cxSize;
    DrawFrameControl(hdc, prc, DFC_CAPTION, DFCS_CAPTIONMIN);
    prc->left = prc->right;
    prc->right = prc->left + cxSize;
    DrawFrameControl(hdc, prc, DFC_CAPTION, DFCS_CAPTIONMAX);

    RestoreDC(hdc, -1);
}

void NEAR PASCAL LookPrev_ShowBitmap(HWND hWnd, HDC hdc)
{
    RECT rc;
    HBITMAP hbmOld;
    HPALETTE hpalOld = NULL;

    if (g_hpal3D)
    {
        hpalOld = SelectPalette(hdc, g_hpal3D, FALSE);
        RealizePalette(hdc);
    }

    GetClientRect(hWnd, &rc);
    hbmOld = SelectObject(g_hdcMem, g_hbmLook);
    BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, g_hdcMem, 0, 0, SRCCOPY);
    SelectObject(g_hdcMem, hbmOld);

    if (hpalOld)
    {
        SelectPalette(hdc, hpalOld, FALSE);
        RealizePalette(hdc);
    }
}

// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
void NEAR PASCAL LookPrev_Draw(HWND hWnd, HDC hdc)
{
    RECT rcT;
    int nMode;
    DWORD rgbBk;
    int cxSize, cySize;
    HANDLE hOldColors;
    HPALETTE hpalOld = NULL;
    HICON hiconLogo;
    HFONT hfontOld;

    SaveDC(hdc);

    if (g_hpal3D)
    {
        hpalOld = SelectPalette(hdc, g_hpal3D, TRUE);
        RealizePalette(hdc);
    }

    hOldColors = SetSysColorsTemp(g_rgb, g_brushes, COLOR_MAX);

    hiconLogo = LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON,
                        g_sizes[SIZE_CAPTION].CurSize - 2*cxBorder,
                        g_sizes[SIZE_CAPTION].CurSize - 2*cyBorder, 0);

    //
    // Setup drawing stuff
    //
    nMode = SetBkMode(hdc, TRANSPARENT);
    rgbBk = GetTextColor(hdc);

    cxSize  = GetSystemMetrics(SM_CXSIZE);
    cySize  = GetSystemMetrics(SM_CYSIZE);

    //
    // Desktop
    //
    FillRect(hdc, &RCZ(ELEMENT_DESKTOP), g_brushes[COLOR_BACKGROUND]);

    //
    // Inactive window
    //

    // Border
    rcT = RCZ(ELEMENT_INACTIVEBORDER);
    DrawEdge(hdc, &rcT, EDGE_RAISED, BF_RECT | BF_ADJUST);
    MyDrawFrame(hdc, &rcT, g_brushes[COLOR_INACTIVEBORDER], g_sizes[SIZE_FRAME].CurSize);
    MyDrawFrame(hdc, &rcT, g_brushes[COLOR_3DFACE], 1);

    // Caption
    rcT = RCZ(ELEMENT_INACTIVECAPTION);
    MyDrawBorderBelow(hdc, &rcT);

    // NOTE: because USER draws icon stuff using its own DC and subsequently
    // its own palette, we need to make sure to use the inactivecaption
    // brush before USER does so that it will be realized against our palette.
    // this might get fixed in USER by better be safe.  

    // "clip" the caption title under the buttons
    rcT.left = RCZ(ELEMENT_INACTIVESYSBUT2).left - cyEdge;
    FillRect(hdc, &rcT, g_brushes[COLOR_INACTIVECAPTION]);
    rcT.right = rcT.left;
    rcT.left = RCZ(ELEMENT_INACTIVECAPTION).left;
    DrawCaptionTemp(NULL, hdc, &rcT, g_fonts[FONT_CAPTION].hfont, hiconLogo, g_szInactive, DC_ICON | DC_TEXT);

    DrawFrameControl(hdc, &RCZ(ELEMENT_INACTIVESYSBUT1), DFC_CAPTION, DFCS_CAPTIONCLOSE);
    rcT = RCZ(ELEMENT_INACTIVESYSBUT2);
    rcT.right -= (rcT.right - rcT.left)/2;
    DrawFrameControl(hdc, &rcT, DFC_CAPTION, DFCS_CAPTIONMIN);
    rcT.left = rcT.right;
    rcT.right = RCZ(ELEMENT_INACTIVESYSBUT2).right;
    DrawFrameControl(hdc, &rcT, DFC_CAPTION, DFCS_CAPTIONMAX);


#if 0
    //
    // small caption window
    // 

    {
    HICON hicon;
    int temp;


    rcT = RCZ(ELEMENT_SMCAPTION);
    hicon = LoadImage(NULL, IDI_APPLICATION,
            IMAGE_ICON,
                        g_sizes[SIZE_SMCAPTION].CurSize - 2*cxBorder,
                        g_sizes[SIZE_SMCAPTION].CurSize - 2*cyBorder,
                    0);

    DrawEdge(hdc, &rcT, EDGE_RAISED, BF_TOP | BF_LEFT | BF_RIGHT | BF_ADJUST);
    MyDrawFrame(hdc, &rcT, g_brushes[COLOR_3DFACE], 1);
    // "clip" the caption title under the buttons
    temp = rcT.left;  // remember start of actual caption
    rcT.left = RCZ(ELEMENT_SMCAPSYSBUT).left - cxEdge;
    FillRect(hdc, &rcT, g_brushes[COLOR_INACTIVECAPTION]);
    rcT.right = rcT.left;
    rcT.left = temp;  // start of actual caption
    DrawCaptionTemp(NULL, hdc, &rcT, g_fonts[FONT_SMCAPTION].hfont, hicon, g_szSmallCaption, DC_SMALLCAP | DC_ICON | DC_TEXT);
    DestroyIcon(hicon);

    DrawFrameControl(hdc, &RCZ(ELEMENT_SMCAPSYSBUT), DFC_CAPTION, DFCS_CAPTIONCLOSE);
    }
#endif

    //
    // Active window
    //

    // Border
    rcT = RCZ(ELEMENT_ACTIVEBORDER);
    DrawEdge(hdc, &rcT, EDGE_RAISED, BF_RECT | BF_ADJUST);
    MyDrawFrame(hdc, &rcT, g_brushes[COLOR_ACTIVEBORDER], g_sizes[SIZE_FRAME].CurSize);
    MyDrawFrame(hdc, &rcT, g_brushes[COLOR_3DFACE], 1);

    // Caption
    rcT = RCZ(ELEMENT_ACTIVECAPTION);
    MyDrawBorderBelow(hdc, &rcT);
    // "clip" the caption title under the buttons
    rcT.left = RCZ(ELEMENT_ACTIVESYSBUT2).left - cxEdge;
    FillRect(hdc, &rcT, g_brushes[COLOR_ACTIVECAPTION]);
    rcT.right = rcT.left;
    rcT.left = RCZ(ELEMENT_ACTIVECAPTION).left;
    DrawCaptionTemp(NULL, hdc, &rcT, g_fonts[FONT_CAPTION].hfont, hiconLogo, g_szActive, DC_ACTIVE | DC_ICON | DC_TEXT);

    DrawFrameControl(hdc, &RCZ(ELEMENT_ACTIVESYSBUT1), DFC_CAPTION, DFCS_CAPTIONCLOSE);
    rcT = RCZ(ELEMENT_ACTIVESYSBUT2);
    rcT.right -= (rcT.right - rcT.left)/2;
    DrawFrameControl(hdc, &rcT, DFC_CAPTION, DFCS_CAPTIONMIN);
    rcT.left = rcT.right;
    rcT.right = RCZ(ELEMENT_ACTIVESYSBUT2).right;
    DrawFrameControl(hdc, &rcT, DFC_CAPTION, DFCS_CAPTIONMAX);

    // Menu
    rcT = RCZ(ELEMENT_MENUNORMAL);
    DrawMenuBarTemp(hWnd, hdc, &rcT, g_hmenuSample, g_fonts[FONT_MENU].hfont);
    MyDrawBorderBelow(hdc, &rcT);

    //
    // Client area
    //

    rcT = RCZ(ELEMENT_WINDOW);
    DrawEdge(hdc, &rcT, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
    FillRect(hdc, &rcT, g_brushes[COLOR_WINDOW]);

    // window text
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, g_rgb[COLOR_WINDOWTEXT]);
    TextOut(hdc, RCZ(ELEMENT_WINDOW).left + 2*cxEdge, RCZ(ELEMENT_WINDOW).top + 2*cyEdge, g_szWindowText, lstrlen(g_szWindowText));

    //
    // scroll bar
    //
    rcT = RCZ(ELEMENT_SCROLLBAR);
    //MyDrawFrame(hdc, &rcT, g_brushes[COLOR_3DSHADOW], 1);
    //g_brushes[COLOR_SCROLLBAR]);
    //FillRect(hdc, &rcT, (HBRUSH)DefWindowProc(hWnd, WM_CTLCOLORSCROLLBAR, (WPARAM)hdc, (LPARAM)hWnd));
    FillRect(hdc, &rcT, g_brushes[COLOR_SCROLLBAR]);

    DrawFrameControl(hdc, &RCZ(ELEMENT_SCROLLUP), DFC_SCROLL, DFCS_SCROLLUP);
    DrawFrameControl(hdc, &RCZ(ELEMENT_SCROLLDOWN), DFC_SCROLL, DFCS_SCROLLDOWN);

    //
    // MessageBox
    //
    rcT = RCZ(ELEMENT_MSGBOX);
    DrawEdge(hdc, &rcT, EDGE_RAISED, BF_RECT | BF_ADJUST);
    FillRect(hdc, &rcT, g_brushes[COLOR_3DFACE]);

    rcT = RCZ(ELEMENT_MSGBOXCAPTION);
    MyDrawBorderBelow(hdc, &rcT);
    // "clip" the caption title under the buttons
    rcT.left = RCZ(ELEMENT_MSGBOXSYSBUT).left - cxEdge;
    FillRect(hdc, &rcT, g_brushes[COLOR_ACTIVECAPTION]);
    rcT.right = rcT.left;
    rcT.left = RCZ(ELEMENT_MSGBOXCAPTION).left;
    DrawCaptionTemp(NULL, hdc, &rcT, g_fonts[FONT_CAPTION].hfont, hiconLogo, g_szMsgBox, DC_ACTIVE | DC_ICON | DC_TEXT);

    DrawFrameControl(hdc, &RCZ(ELEMENT_MSGBOXSYSBUT), DFC_CAPTION, DFCS_CAPTIONCLOSE);

    // message box text
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, g_rgb[COLOR_WINDOWTEXT]);
    hfontOld = SelectObject(hdc, g_fonts[FONT_MSGBOX].hfont);
    TextOut(hdc, RCZ(ELEMENT_MSGBOX).left + 3*cxEdge, RCZ(ELEMENT_MSGBOXCAPTION).bottom + cyEdge,
                        g_szMsgBoxText, lstrlen(g_szMsgBoxText));
    if (hfontOld)
        SelectObject(hdc, hfontOld);

    //
    // Button
    //
    rcT = RCZ(ELEMENT_BUTTON);
    DrawFrameControl(hdc, &rcT, DFC_BUTTON, DFCS_BUTTONPUSH);

// ?????? what font should this use ??????
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, g_rgb[COLOR_BTNTEXT]);
    DrawText(hdc, g_szButton, -1, &rcT, DT_CENTER | DT_NOPREFIX |
        DT_SINGLELINE | DT_VCENTER);

    SetBkColor(hdc, rgbBk);
    SetBkMode(hdc, nMode);

    if (hiconLogo)
        DestroyIcon(hiconLogo);

    SetSysColorsTemp(NULL, NULL, (UINT)hOldColors);

    if (hpalOld)
    {
        hpalOld = SelectPalette(hdc, hpalOld, FALSE);
        RealizePalette(hdc);
    }

    RestoreDC(hdc, -1);
}

void FAR PASCAL LookPrev_Repaint(HWND hwnd)
{
    HBITMAP hbmOld;

    if (g_hbmLook)
    {
        hbmOld = SelectObject(g_hdcMem, g_hbmLook);
        LookPrev_Draw(hwnd, g_hdcMem);
        SelectObject(g_hdcMem, hbmOld);
    }
    InvalidateRect(hwnd, NULL, FALSE);
}

LONG CALLBACK  LookPreviewWndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    DWORD dw;
    int iEl;

    switch(message)
    {
        case WM_NCCREATE:
            dw = GetWindowLong (hWnd,GWL_STYLE);
            SetWindowLong (hWnd, GWL_STYLE, dw | WS_BORDER);
            dw = GetWindowLong (hWnd,GWL_EXSTYLE);
            SetWindowLong (hWnd, GWL_EXSTYLE, dw | WS_EX_CLIENTEDGE);
            return TRUE;

        case WM_CREATE:
            LookPrev_Init(hWnd);
            break;

        case WM_DESTROY:
            if (g_hbmLook)
                DeleteObject(g_hbmLook);
            if (g_hmenuSample)
                DestroyMenu(g_hmenuSample);
            break;

        case WM_PALETTECHANGED:
            if ((HWND)wParam == hWnd)
                break;
            //fallthru
        case WM_QUERYNEWPALETTE:
            if (g_hpal3D)
                InvalidateRect(hWnd, NULL, FALSE);
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            { POINT pt;
                pt.x = LOWORD(lParam);  // horizontal position of cursor
                pt.y = HIWORD(lParam);  // vertical position of cursor

                iEl = LookPrev_HitTest(pt);
                Look_SelectElement(GetParent(hWnd), iEl, TRUE);
            }
            break;

        case WM_SIZE:
            break;

        case WM_PAINT:
            BeginPaint(hWnd, &ps);
            if (g_hbmLook)
                LookPrev_ShowBitmap(hWnd, ps.hdc);
            else
                LookPrev_Draw(hWnd, ps.hdc);
            EndPaint(hWnd, &ps);
            return 0;
    }
    return DefWindowProc(hWnd,message,wParam,lParam);
}

BOOL FAR PASCAL RegisterLookPreviewClass(HINSTANCE hInst)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInst, LOOKPREV_CLASS, &wc)) {
        wc.style = 0;
        wc.lpfnWndProc = LookPreviewWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInst;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = LOOKPREV_CLASS;

        if (!RegisterClass(&wc))
            return FALSE;
    }

    return TRUE;
}
