/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            dll/cpl/desk/draw.c
 * PURPOSE:         Providing drawing functions
 *
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "desk.h"

#define MENU_BAR_ITEMS_SPACE (12)

/******************************************************************************/

static const signed char LTInnerNormal[] = {
    -1,        -1,                  -1,                 -1,
    -1,        COLOR_BTNHIGHLIGHT,  COLOR_BTNHIGHLIGHT, -1,
    -1,        COLOR_3DDKSHADOW,    COLOR_3DDKSHADOW,   -1,
    -1,        -1,                  -1,                 -1
};

static const signed char LTOuterNormal[] = {
    -1,                 COLOR_3DLIGHT,   COLOR_BTNSHADOW, -1,
    COLOR_BTNHIGHLIGHT, COLOR_3DLIGHT,   COLOR_BTNSHADOW, -1,
    COLOR_3DDKSHADOW,   COLOR_3DLIGHT,   COLOR_BTNSHADOW, -1,
    -1,                 COLOR_3DLIGHT,   COLOR_BTNSHADOW, -1
};

static const signed char RBInnerNormal[] = {
    -1,        -1,              -1,                 -1,
    -1,        COLOR_BTNSHADOW, COLOR_BTNSHADOW,    -1,
    -1,        COLOR_3DLIGHT,   COLOR_3DLIGHT,      -1,
    -1,        -1,              -1,                 -1
};

static const signed char RBOuterNormal[] = {
    -1,                 COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1,
    COLOR_BTNSHADOW,    COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1,
    COLOR_3DLIGHT,      COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1,
    -1,                 COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1
};

static const signed char LTRBOuterMono[] = {
    -1,           COLOR_WINDOWFRAME, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME,
    COLOR_WINDOW, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME,
    COLOR_WINDOW, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME,
    COLOR_WINDOW, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME,
};

static const signed char LTRBInnerMono[] = {
    -1, -1,           -1,           -1,
    -1, COLOR_WINDOW, COLOR_WINDOW, COLOR_WINDOW,
    -1, COLOR_WINDOW, COLOR_WINDOW, COLOR_WINDOW,
    -1, COLOR_WINDOW, COLOR_WINDOW, COLOR_WINDOW,
};

static BOOL
MyIntDrawRectEdge(HDC hdc, LPRECT rc, UINT uType, UINT uFlags, COLOR_SCHEME *scheme)
{
    signed char LTInnerI, LTOuterI;
    signed char RBInnerI, RBOuterI;
    HPEN LTInnerPen, LTOuterPen;
    HPEN RBInnerPen, RBOuterPen;
    RECT InnerRect = *rc;
    POINT SavePoint;
    HPEN SavePen;
    int LBpenplus = 0;
    int LTpenplus = 0;
    int RTpenplus = 0;
    int RBpenplus = 0;
    HBRUSH hbr;

    /* Init some vars */
    LTInnerPen = LTOuterPen = RBInnerPen = RBOuterPen = (HPEN)GetStockObject(NULL_PEN);
    SavePen = (HPEN)SelectObject(hdc, LTInnerPen);

    /* Determine the colors of the edges */
    LTInnerI = LTInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
    LTOuterI = LTOuterNormal[uType & (BDR_INNER|BDR_OUTER)];
    RBInnerI = RBInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
    RBOuterI = RBOuterNormal[uType & (BDR_INNER|BDR_OUTER)];

    if ((uFlags & BF_BOTTOMLEFT) == BF_BOTTOMLEFT)
        LBpenplus = 1;
    if ((uFlags & BF_TOPRIGHT) == BF_TOPRIGHT)
        RTpenplus = 1;
    if ((uFlags & BF_BOTTOMRIGHT) == BF_BOTTOMRIGHT)
        RBpenplus = 1;
    if ((uFlags & BF_TOPLEFT) == BF_TOPLEFT)
        LTpenplus = 1;

    if ((uFlags & MY_BF_ACTIVEBORDER))
        hbr = CreateSolidBrush(scheme->crColor[COLOR_ACTIVEBORDER]);
    else
        hbr = CreateSolidBrush(scheme->crColor[COLOR_BTNFACE]);

    FillRect(hdc, &InnerRect, hbr);
    DeleteObject(hbr);

    MoveToEx(hdc, 0, 0, &SavePoint);

    /* Draw the outer edge */
    if (LTOuterI != -1)
    {
        LTOuterPen = GetStockObject(DC_PEN);
        SelectObject(hdc, LTOuterPen);
        SetDCPenColor(hdc, scheme->crColor[LTOuterI]);
        if (uFlags & BF_TOP)
        {
            MoveToEx(hdc, InnerRect.left, InnerRect.top, NULL);
            LineTo(hdc, InnerRect.right, InnerRect.top);
        }
        if (uFlags & BF_LEFT)
        {
            MoveToEx(hdc, InnerRect.left, InnerRect.top, NULL);
            LineTo(hdc, InnerRect.left, InnerRect.bottom);
        }
    }

    if (RBOuterI != -1)
    {
        RBOuterPen = GetStockObject(DC_PEN);
        SelectObject(hdc, RBOuterPen);
        SetDCPenColor(hdc, scheme->crColor[RBOuterI]);
        if (uFlags & BF_BOTTOM)
        {
            MoveToEx(hdc, InnerRect.left, InnerRect.bottom-1, NULL);
            LineTo(hdc, InnerRect.right, InnerRect.bottom-1);
        }
        if (uFlags & BF_RIGHT)
        {
            MoveToEx(hdc, InnerRect.right-1, InnerRect.top, NULL);
            LineTo(hdc, InnerRect.right-1, InnerRect.bottom);
        }
    }

    /* Draw the inner edge */
    if (LTInnerI != -1)
    {
        LTInnerPen = GetStockObject(DC_PEN);
        SelectObject(hdc, LTInnerPen);
        SetDCPenColor(hdc, scheme->crColor[LTInnerI]);
        if (uFlags & BF_TOP)
        {
            MoveToEx(hdc, InnerRect.left+LTpenplus, InnerRect.top+1, NULL);
            LineTo(hdc, InnerRect.right-RTpenplus, InnerRect.top+1);
        }
        if (uFlags & BF_LEFT)
        {
            MoveToEx(hdc, InnerRect.left+1, InnerRect.top+LTpenplus, NULL);
            LineTo(hdc, InnerRect.left+1, InnerRect.bottom-LBpenplus);
        }
    }

    if (RBInnerI != -1)
    {
        RBInnerPen = GetStockObject(DC_PEN);
        SelectObject(hdc, RBInnerPen);
        SetDCPenColor(hdc, scheme->crColor[RBInnerI]);
        if (uFlags & BF_BOTTOM)
        {
            MoveToEx(hdc, InnerRect.left+LBpenplus, InnerRect.bottom-2, NULL);
            LineTo(hdc, InnerRect.right-RBpenplus, InnerRect.bottom-2);
        }
        if (uFlags & BF_RIGHT)
        {
            MoveToEx(hdc, InnerRect.right-2, InnerRect.top+RTpenplus, NULL);
            LineTo(hdc, InnerRect.right-2, InnerRect.bottom-RBpenplus);
        }
    }

    if (uFlags & BF_ADJUST)
    {
        int add = (LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0)
                      + (LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0);

        if (uFlags & BF_LEFT)
            InnerRect.left += add;
        if (uFlags & BF_RIGHT)
            InnerRect.right -= add;
        if (uFlags & BF_TOP)
            InnerRect.top += add;
        if (uFlags & BF_BOTTOM)
            InnerRect.bottom -= add;

        if (uFlags & BF_ADJUST)
            *rc = InnerRect;
    }

    /* Cleanup */
    SelectObject(hdc, SavePen);
    MoveToEx(hdc, SavePoint.x, SavePoint.y, NULL);
    return TRUE;
}

static BOOL
MyDrawFrameButton(HDC hdc, LPRECT rc, UINT uState, COLOR_SCHEME *scheme)
{
    UINT edge;
    if (uState & (DFCS_PUSHED | DFCS_CHECKED | DFCS_FLAT))
        edge = EDGE_SUNKEN;
    else
        edge = EDGE_RAISED;
    return MyIntDrawRectEdge(hdc, rc, edge, (uState & DFCS_FLAT) | BF_RECT | BF_SOFT, scheme);
}

static int
MyMakeSquareRect(LPRECT src, LPRECT dst)
{
    int Width  = src->right - src->left;
    int Height = src->bottom - src->top;
    int SmallDiam = Width > Height ? Height : Width;

    *dst = *src;

    /* Make it a square box */
    if (Width < Height)      /* SmallDiam == Width */
    {
        dst->top += (Height-Width)/2;
        dst->bottom = dst->top + SmallDiam;
    }
    else if (Width > Height) /* SmallDiam == Height */
    {
        dst->left += (Width-Height)/2;
        dst->right = dst->left + SmallDiam;
    }

    return SmallDiam;
}

static BOOL
MyDrawFrameCaption(HDC dc, LPRECT r, UINT uFlags, COLOR_SCHEME *scheme)
{
    LOGFONT lf;
    HFONT hFont, hOldFont;
    COLORREF clrsave;
    RECT myr;
    INT bkmode;
    TCHAR Symbol;
    switch(uFlags & 0xff)
    {
    case DFCS_CAPTIONCLOSE:
        Symbol = 'r';
        break;
    case DFCS_CAPTIONHELP:
        Symbol = 's';
        break;
    case DFCS_CAPTIONMIN:
        Symbol = '0';
        break;
    case DFCS_CAPTIONMAX:
        Symbol = '1';
        break;
    case DFCS_CAPTIONRESTORE:
        Symbol = '2';
        break;
    default:
        return FALSE;
    }
    MyIntDrawRectEdge(dc, r, (uFlags & DFCS_PUSHED) ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT | BF_MIDDLE | BF_SOFT, scheme);
    ZeroMemory(&lf, sizeof(lf));
    MyMakeSquareRect(r, &myr);
    myr.left += 1;
    myr.top += 1;
    myr.right -= 1;
    myr.bottom -= 1;
    if (uFlags & DFCS_PUSHED)
       OffsetRect(&myr,1,1);
    lf.lfHeight = myr.bottom - myr.top;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpy(lf.lfFaceName, TEXT("Marlett"));
    hFont = CreateFontIndirect(&lf);
    /* Save font and text color */
    hOldFont = SelectObject(dc, hFont);
    clrsave = GetTextColor(dc);
    bkmode = GetBkMode(dc);
    /* Set color and drawing mode */
    SetBkMode(dc, TRANSPARENT);
    if (uFlags & DFCS_INACTIVE)
    {
        /* Draw shadow */
        SetTextColor(dc, scheme->crColor[COLOR_BTNHIGHLIGHT]);
        TextOut(dc, myr.left + 1, myr.top + 1, &Symbol, 1);
    }
    SetTextColor(dc, scheme->crColor[(uFlags & DFCS_INACTIVE) ? COLOR_BTNSHADOW : COLOR_BTNTEXT]);
    /* Draw selected symbol */
    TextOut(dc, myr.left, myr.top, &Symbol, 1);
    /* Restore previous settings */
    SetTextColor(dc, clrsave);
    SelectObject(dc, hOldFont);
    SetBkMode(dc, bkmode);
    DeleteObject(hFont);
    return TRUE;
}

/******************************************************************************/

static BOOL
MyDrawFrameScroll(HDC dc, LPRECT r, UINT uFlags, COLOR_SCHEME *scheme)
{
    LOGFONT lf;
    HFONT hFont, hOldFont;
    COLORREF clrsave;
    RECT myr;
    INT bkmode;
    TCHAR Symbol;
    switch(uFlags & 0xff)
    {
    case DFCS_SCROLLCOMBOBOX:
    case DFCS_SCROLLDOWN:
        Symbol = '6';
        break;

    case DFCS_SCROLLUP:
        Symbol = '5';
        break;

    case DFCS_SCROLLLEFT:
        Symbol = '3';
        break;

    case DFCS_SCROLLRIGHT:
        Symbol = '4';
        break;

    default:
        return FALSE;
    }
    MyIntDrawRectEdge(dc, r, (uFlags & DFCS_PUSHED) ? EDGE_SUNKEN : EDGE_RAISED, (uFlags&DFCS_FLAT) | BF_MIDDLE | BF_RECT, scheme);
    ZeroMemory(&lf, sizeof(lf));
    MyMakeSquareRect(r, &myr);
    myr.left += 1;
    myr.top += 1;
    myr.right -= 1;
    myr.bottom -= 1;
    if (uFlags & DFCS_PUSHED)
       OffsetRect(&myr,1,1);
    lf.lfHeight = myr.bottom - myr.top;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpy(lf.lfFaceName, TEXT("Marlett"));
    hFont = CreateFontIndirect(&lf);
    /* Save font and text color */
    hOldFont = SelectObject(dc, hFont);
    clrsave = GetTextColor(dc);
    bkmode = GetBkMode(dc);
    /* Set color and drawing mode */
    SetBkMode(dc, TRANSPARENT);
    if (uFlags & DFCS_INACTIVE)
    {
        /* Draw shadow */
        SetTextColor(dc, scheme->crColor[COLOR_BTNHIGHLIGHT]);
        TextOut(dc, myr.left + 1, myr.top + 1, &Symbol, 1);
    }
    SetTextColor(dc, scheme->crColor[(uFlags & DFCS_INACTIVE) ? COLOR_BTNSHADOW : COLOR_BTNTEXT]);
    /* Draw selected symbol */
    TextOut(dc, myr.left, myr.top, &Symbol, 1);
    /* restore previous settings */
    SetTextColor(dc, clrsave);
    SelectObject(dc, hOldFont);
    SetBkMode(dc, bkmode);
    DeleteObject(hFont);
    return TRUE;
}

BOOL
MyDrawFrameControl(HDC hDC, LPRECT rc, UINT uType, UINT uState, COLOR_SCHEME *scheme)
{
    switch(uType)
    {
    case DFC_BUTTON:
        return MyDrawFrameButton(hDC, rc, uState, scheme);
    case DFC_CAPTION:
        return MyDrawFrameCaption(hDC, rc, uState, scheme);
    case DFC_SCROLL:
        return MyDrawFrameScroll(hDC, rc, uState, scheme);
    }
    return FALSE;
}

BOOL
MyDrawEdge(HDC hDC, LPRECT rc, UINT edge, UINT flags, COLOR_SCHEME *scheme)
{
    return MyIntDrawRectEdge(hDC, rc, edge, flags, scheme);
}

VOID
MyDrawCaptionButtons(HDC hdc, LPRECT lpRect, BOOL bMinMax, int x, COLOR_SCHEME *scheme)
{
    RECT rc3;
    RECT rc4;
    RECT rc5;

    rc3.left = lpRect->right - 2 - x;
    rc3.top = lpRect->top + 2;
    rc3.right = lpRect->right - 2;
    rc3.bottom = lpRect->bottom - 2;

    MyDrawFrameControl(hdc, &rc3, DFC_CAPTION, DFCS_CAPTIONCLOSE, scheme);

    if (bMinMax)
    {
        rc4.left = rc3.left - x - 2;
        rc4.top = rc3.top;
        rc4.right = rc3.right - x - 2;
        rc4.bottom = rc3.bottom;

        MyDrawFrameControl(hdc, &rc4, DFC_CAPTION, DFCS_CAPTIONMAX, scheme);

        rc5.left = rc4.left - x;
        rc5.top = rc4.top;
        rc5.right = rc4.right - x;
        rc5.bottom = rc4.bottom;

        MyDrawFrameControl(hdc, &rc5, DFC_CAPTION, DFCS_CAPTIONMIN, scheme);
    }
}

VOID
MyDrawScrollbar(HDC hdc, LPRECT rc, HBRUSH hbrScrollbar, COLOR_SCHEME *scheme)
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

    MyDrawFrameControl(hdc, &rcTop, DFC_SCROLL, DFCS_SCROLLUP, scheme);
    MyDrawFrameControl(hdc, &rcBottom, DFC_SCROLL, DFCS_SCROLLDOWN, scheme);

    FillRect(hdc, &rcMiddle, hbrScrollbar);
}

/******************************************************************************/

BOOL
MyDrawCaptionTemp(HWND hwnd, HDC hdc, const RECT *rect, HFONT hFont, HICON hIcon, LPCWSTR str, UINT uFlags, COLOR_SCHEME *scheme)
{
    //ULONG Height;
    //UINT VCenter, Padding;
    //LONG ButtonWidth;
    HBRUSH hbr;
    HGDIOBJ hFontOld;
    RECT rc;

    //Height = scheme->Size[SIZE_CAPTION_Y] - 1;
    //VCenter = (rect->bottom - rect->top) / 2;
    //Padding = VCenter - (Height / 2);

    //ButtonWidth = scheme->Size[SIZE_SIZE_X] - 2;

    if (uFlags & DC_GRADIENT)
    {
        GRADIENT_RECT gcap = {0, 1};
        TRIVERTEX vert[2];
        COLORREF Colors[2];

        Colors[0] = scheme->crColor[((uFlags & DC_ACTIVE) ?
            COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION)];
        Colors[1] = scheme->crColor[((uFlags & DC_ACTIVE) ?
            COLOR_GRADIENTACTIVECAPTION : COLOR_GRADIENTINACTIVECAPTION)];

        vert[0].x = rect->left;
        vert[0].y = rect->top;
        vert[0].Red = (WORD)Colors[0]<<8;
        vert[0].Green = (WORD)Colors[0] & 0xFF00;
        vert[0].Blue = (WORD)(Colors[0]>>8) & 0xFF00;
        vert[0].Alpha = 0;

        vert[1].x = rect->right;
        vert[1].y = rect->bottom;
        vert[1].Red = (WORD)Colors[1]<<8;
        vert[1].Green = (WORD)Colors[1] & 0xFF00;
        vert[1].Blue = (WORD)(Colors[1]>>8) & 0xFF00;
        vert[1].Alpha = 0;

        GdiGradientFill(hdc, vert, 2, &gcap, 1, GRADIENT_FILL_RECT_H);
    }
    else
    {
        if (uFlags & DC_ACTIVE)
            hbr = CreateSolidBrush(scheme->crColor[COLOR_ACTIVECAPTION]);
        else
            hbr = CreateSolidBrush(scheme->crColor[COLOR_INACTIVECAPTION]);
        FillRect(hdc, rect, hbr);
        DeleteObject(hbr);
    }

    hFontOld = SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    if (uFlags & DC_ACTIVE)
        SetTextColor(hdc, scheme->crColor[COLOR_CAPTIONTEXT]);
    else
        SetTextColor(hdc, scheme->crColor[COLOR_INACTIVECAPTIONTEXT]);
    rc.left = rect->left + 2;
    rc.top = rect->top;
    rc.right = rect->right;
    rc.bottom = rect->bottom;
    DrawTextW(hdc, str, -1, &rc, DT_SINGLELINE | DT_VCENTER);
    SelectObject(hdc, hFontOld);
    return TRUE;
}

/******************************************************************************/

DWORD
MyDrawMenuBarTemp(HWND Wnd, HDC DC, LPRECT Rect, HMENU Menu, HFONT Font, COLOR_SCHEME *scheme)
{
    HBRUSH hbr;
    HPEN hPen;
    HGDIOBJ hPenOld, hFontOld;
    BOOL flat_menu;
    INT i, bkgnd, x;
    RECT rect;
    WCHAR Text[128];
    UINT uFormat = DT_CENTER | DT_VCENTER | DT_SINGLELINE;

    flat_menu = scheme->bFlatMenus;

    if (flat_menu)
        hbr = CreateSolidBrush(scheme->crColor[COLOR_MENUBAR]);
    else
        hbr = CreateSolidBrush(scheme->crColor[COLOR_MENU]);
    FillRect(DC, Rect, hbr);
    DeleteObject(hbr);

    hPen = CreatePen(PS_SOLID, 0, scheme->crColor[COLOR_3DFACE]);
    hPenOld = SelectObject(DC, hPen);
    MoveToEx(DC, Rect->left, Rect->bottom - 1, NULL);
    LineTo(DC, Rect->right, Rect->bottom - 1);
    SelectObject(DC, hPenOld);
    DeleteObject(hPen);

    bkgnd = (flat_menu ? COLOR_MENUBAR : COLOR_MENU);
    x = Rect->left;
    hFontOld = SelectObject(DC, Font);
    for (i = 0; i < 3; i++)
    {
        GetMenuStringW(Menu, i, Text, 128, MF_BYPOSITION);

        rect.left = rect.right = x;
        rect.top = Rect->top;
        rect.bottom = Rect->bottom;
        DrawTextW(DC, Text, -1, &rect, DT_SINGLELINE | DT_CALCRECT);
        rect.bottom = Rect->bottom;
        rect.right += MENU_BAR_ITEMS_SPACE;
        x += rect.right - rect.left;

        if (i == 2)
        {
            if (flat_menu)
            {
                SetTextColor(DC, scheme->crColor[COLOR_HIGHLIGHTTEXT]);
                SetBkColor(DC, scheme->crColor[COLOR_HIGHLIGHT]);

                InflateRect (&rect, -1, -1);
                hbr = CreateSolidBrush(scheme->crColor[COLOR_MENUHILIGHT]);
                FillRect(DC, &rect, hbr);
                DeleteObject(hbr);

                InflateRect (&rect, 1, 1);
                hbr = CreateSolidBrush(scheme->crColor[COLOR_HIGHLIGHT]);
                FrameRect(DC, &rect, hbr);
                DeleteObject(hbr);
            }
            else
            {
                SetTextColor(DC, scheme->crColor[COLOR_MENUTEXT]);
                SetBkColor(DC, scheme->crColor[COLOR_MENU]);
                DrawEdge(DC, &rect, BDR_SUNKENOUTER, BF_RECT);
            }
        }
        else
        {
            if (i == 1)
                SetTextColor(DC, scheme->crColor[COLOR_GRAYTEXT]);
            else
                SetTextColor(DC, scheme->crColor[COLOR_MENUTEXT]);

            SetBkColor(DC, scheme->crColor[bkgnd]);
            hbr = CreateSolidBrush(scheme->crColor[bkgnd]);
            FillRect(DC, &rect, hbr);
            DeleteObject(hbr);
        }

        SetBkMode(DC, TRANSPARENT);

        rect.left += MENU_BAR_ITEMS_SPACE / 2;
        rect.right -= MENU_BAR_ITEMS_SPACE / 2;

        if (i == 1)
        {
            ++rect.left; ++rect.top; ++rect.right; ++rect.bottom;
            SetTextColor(DC, scheme->crColor[COLOR_BTNHIGHLIGHT]);
            DrawTextW(DC, Text, -1, &rect, uFormat);
            --rect.left; --rect.top; --rect.right; --rect.bottom;
            SetTextColor(DC, scheme->crColor[COLOR_BTNSHADOW]);
        }
        DrawTextW(DC, Text, -1, &rect, uFormat);
    }
    SelectObject(DC, hFontOld);

    return TRUE;
}
