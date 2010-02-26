//
//    CardLib - CardButton class
//
//    Freeware
//    Copyright J Brown 2001
//
#include <windows.h>
#include <tchar.h>

#include "cardlib.h"
#include "cardwindow.h"
#include "cardbutton.h"
#include "cardcolor.h"

HPALETTE UseNicePalette(HDC, HPALETTE);
void     RestorePalette(HDC, HPALETTE);

void PaintRect(HDC hdc, RECT *rect, COLORREF colour);

CardButton::CardButton(CardWindow &parent, int Id, TCHAR *szText, UINT Style, bool visible,
                        int x, int y, int width, int height)

 : parentWnd(parent), id(Id), uStyle(Style), fVisible(visible), ButtonCallback(0)
{
    crText = RGB(255,255,255);
    crBack = RGB(0, 128, 0);

    xadjust = 0;
    yadjust = 0;
    xjustify = 0;
    yjustify = 0;

    fMouseDown = false;
    fButtonDown = false;

    hIcon = 0;

    SetText(szText);
    Move(x, y, width, height);

    mxlock = CreateMutex(0, FALSE, 0);

    hFont = 0;
}

CardButton::~CardButton()
{
    CloseHandle(mxlock);
}

void CardButton::DrawRect(HDC hdc, RECT *rect, bool fNormal)
{
    RECT fill;

    HANDLE hOld;

    HPEN hhi = CreatePen(0, 0, MAKE_PALETTERGB(crHighlight));
    HPEN hsh = CreatePen(0, 0, MAKE_PALETTERGB(crShadow));
    HPEN hbl = (HPEN)GetStockObject(BLACK_PEN);

    int x        = rect->left;
    int y        = rect->top;
    int width    = rect->right-rect->left - 1;
    int height    = rect->bottom-rect->top - 1;

    SetRect(&fill, x+1, y+1, x+width-1, y+height-1);

    int one = 1;

    if(!fNormal)
    {
        x += width;
        y += height;
        width = -width;
        height = -height;
        one = -1;
        OffsetRect(&fill, 1, 1);
    }

    if(fNormal)
        hOld = SelectObject(hdc, hhi);
    else
        hOld = SelectObject(hdc, hhi);

    MoveToEx(hdc, x, y+height, 0);
    LineTo(hdc, x, y);
    LineTo(hdc, x+width, y);
    SelectObject(hdc, hOld);

    hOld = SelectObject(hdc, hbl);
    LineTo(hdc, x+width, y+height);
    LineTo(hdc, x-one, y+height);
    SelectObject(hdc, hOld);

    hOld = SelectObject(hdc, hsh);
    MoveToEx(hdc, x+one, y+height-one, 0);
    LineTo(hdc, x+width-one, y+height-one);
    LineTo(hdc, x+width-one, y);
    SelectObject(hdc, hOld);

    PaintRect(hdc, &fill, MAKE_PALETTERGB(crBack));

    DeleteObject(hhi);
    DeleteObject(hsh);
}

void CardButton::Clip(HDC hdc)
{
    if(fVisible == false) return;

    ExcludeClipRect(hdc, rect.left,  rect.top, rect.right, rect.bottom);
}

void CardButton::Draw(HDC hdc, bool fNormal)
{
    SIZE textsize;
    int x, y;        //text x, y
    int ix, iy;        //icon x, y
    int iconwidth = 0;

    RECT cliprect;

    if(fVisible == 0) return;

    if(hFont == 0)
        SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
    else
        SelectObject(hdc, hFont);

    GetTextExtentPoint32(hdc, szText, lstrlen(szText), &textsize);

    if(hIcon)
    {
        x = rect.left + 32 + 8;
    }
    else
    {
        if(uStyle & CB_ALIGN_LEFT)
        {
            x = rect.left + iconwidth;
        }
        else if(uStyle & CB_ALIGN_RIGHT)
        {
            x = rect.left + (rect.right-rect.left-iconwidth-textsize.cx);
        }
        else    //centered
        {
            x = rect.right - rect.left - iconwidth;
            x = (x - textsize.cx) / 2;
            x += rect.left + iconwidth;
        }
    }

    y = rect.bottom - rect.top;
    y = (y - textsize.cy) / 2;
    y += rect.top;

    //calc icon position..
    ix = rect.left + 4;
    iy = rect.top + (rect.bottom-rect.top-32) / 2;

    //if button is pressed, then shift text
    if(fNormal == false && (uStyle & CB_PUSHBUTTON))
    {
        x += 1;
        y += 1;
        ix += 1;
        iy += 1;
    }

    SetRect(&cliprect, x, y, x+textsize.cx, y+textsize.cy);
    ExcludeClipRect(hdc, x, y, x+textsize.cx, y+textsize.cy);

    //
    //    Calc icon pos
    //

    if(hIcon)
    {
        ExcludeClipRect(hdc, ix, iy, ix + 32, iy + 32);
    }

    if(uStyle & CB_PUSHBUTTON)
    {
        DrawRect(hdc, &rect, fNormal);

        SetBkColor(hdc,   MAKE_PALETTERGB(crBack));
        SetTextColor(hdc, crText);//MAKE_PALETTERGB(crText));

        SelectClipRgn(hdc, 0);

        ExtTextOut(hdc, x, y, ETO_OPAQUE, &cliprect, szText, lstrlen(szText), 0);
    }
    else
    {
        SetBkColor(hdc,      MAKE_PALETTERGB(crBack));
        SetTextColor(hdc, crText);//MAKE_PALETTERGB(crText));

        SelectClipRgn(hdc, 0);

        ExtTextOut(hdc, x, y, ETO_OPAQUE, &rect, szText, lstrlen(szText), 0);
    }

    if(hIcon)
    {
        HBRUSH hbr = CreateSolidBrush(MAKE_PALETTERGB(crBack));
        DrawIconEx(hdc, ix, iy, hIcon, 32, 32, 0, hbr, 0);
        DeleteObject(hbr);
    }

}

void CardButton::AdjustPosition(int winwidth, int winheight)
{
    int width = rect.right-rect.left;
    int height = rect.bottom-rect.top;

    width = width & ~0x1;

    switch(xjustify)
    {
    case CS_XJUST_NONE:
        break;

    case CS_XJUST_CENTER:        //centered
        rect.left = (winwidth - (width)) / 2;
        rect.left += xadjust;
        rect.right = rect.left+width;
        break;

    case CS_XJUST_RIGHT:        //right-aligned
        rect.left = winwidth - width;
        rect.left += xadjust;
        rect.right = rect.left+width;
        break;
    }

    switch(yjustify)
    {
    case CS_YJUST_NONE:
        break;

    case CS_YJUST_CENTER:        //centered
        rect.top = (winheight - (height)) / 2;
        rect.top += yadjust;
        rect.bottom = rect.top+height;
        break;

    case CS_YJUST_BOTTOM:        //right-aligned
        rect.top = winheight - height;
        rect.top += yadjust;
        rect.bottom = rect.top+height;
        break;
    }

}

int CardButton::OnLButtonDown(HWND hwnd, int x, int y)
{
    if((uStyle & CB_PUSHBUTTON) == 0)
        return 0;

    //make sure that the user is allowed to do something
    if(WaitForSingleObject(mxlock, 0) != WAIT_OBJECT_0)
    {
        return 0;
    }
    else
    {
        ReleaseMutex(mxlock);
    }

    fMouseDown = true;
    fButtonDown = true;

    Redraw();

    SetCapture(hwnd);

    return 1;
}

int CardButton::OnMouseMove(HWND hwnd, int x, int y)
{
    if(fMouseDown)
    {
        bool fOldButtonDown = fButtonDown;

        POINT pt;

        pt.x = x;
        pt.y = y;

        if(PtInRect(&rect, pt))
            fButtonDown = true;
        else
            fButtonDown = false;

        if(fButtonDown != fOldButtonDown)
            Redraw();
    }

    return 0;
}

int CardButton::OnLButtonUp(HWND hwnd, int x, int y)
{
    if(fMouseDown)
    {
        fMouseDown = false;
        fButtonDown = false;

        if(uStyle & CB_PUSHBUTTON)
        {
            Redraw();
            ReleaseCapture();
        }

        //if have clicked the button
        if(parentWnd.CardButtonFromPoint(x, y) == this)
        {
            if(ButtonCallback)
            {
                ButtonCallback(*this);
            }
            else
            {
                HWND hwnd = (HWND)parentWnd;
                SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), (LONG_PTR)hwnd);
            }
        }
    }

    return 0;
}

//#define _countof(array) (sizeof(array)/sizeof(array[0]))

CardButton *CardWindow::CreateButton(int id, TCHAR *szText, UINT uStyle, bool fVisible, int x, int y, int width, int height)
{
    CardButton *cb;

    if(nNumButtons == MAXBUTTONS)
        return 0;

    cb = new CardButton(*this, id, szText, uStyle, fVisible, x, y, width, height);
    Buttons[nNumButtons++] = cb;

    if(uStyle & CB_PUSHBUTTON)
    {
        cb->SetBackColor(CardButton::GetFace(crBackgnd));
        //cb->SetBackColor(ScaleLumRGB(crBackgnd, 0.1));
        cb->SetForeColor(RGB(255,255,255));
    }
    else
    {
        cb->SetBackColor(crBackgnd);
        cb->SetForeColor(RGB(255,255,255));
    }

    return cb;
}

void CardButton::SetText(TCHAR *lpszFormat, ...)
{
    int count;

    va_list args;
    va_start(args, lpszFormat);

    count = wvsprintf(szText, lpszFormat, args);
    va_end(args);
}

int CardButton::Id()
{
    return id;
}

void CardButton::Show(bool fShow)
{
    fVisible = fShow;
}

void CardButton::Move(int x, int y, int width, int height)
{
    SetRect(&rect, x, y, x+width, y+height);
}

void CardButton::Redraw()
{
    HDC hdc = GetDC((HWND)parentWnd);

    HPALETTE hOldPal = UseNicePalette(hdc, __hPalette);

    Draw(hdc, !fButtonDown);

    RestorePalette(hdc, hOldPal);

    ReleaseDC((HWND)parentWnd, hdc);
}

void CardButton::SetForeColor(COLORREF cr)
{
    crText = cr;
}

void CardButton::SetBackColor(COLORREF cr)
{
    crBack = cr;

    crHighlight = GetHighlight(cr);
    crShadow    = GetShadow(cr);

    //crHighlight = ScaleLumRGB(cr, +0.25);
    //crShadow    = ScaleLumRGB(cr, -0.25);
}

//    Static member
COLORREF CardButton::GetHighlight(COLORREF crBase)
{
    return ColorScaleRGB(crBase, RGB(255,255,255), 0.25);
}

//    Static member
COLORREF CardButton::GetShadow(COLORREF crBase)
{
    return ColorScaleRGB(crBase, RGB(0,  0,  0),   0.25);
}

COLORREF CardButton::GetFace(COLORREF crBase)
{
    return ColorScaleRGB(crBase, RGB(255,255,255), 0.1);
}

void CardButton::SetPlacement(UINT xJustify, UINT yJustify, int xAdjust, int yAdjust)
{
    xadjust = xAdjust;
    yadjust = yAdjust;
    xjustify = xJustify;
    yjustify = yJustify;
}

void CardButton::SetIcon(HICON hicon, bool fRedraw)
{
    hIcon = hicon;

    if(fRedraw)
        Redraw();
}

void CardButton::SetFont(HFONT font)
{
    //don't delete the existing font..
    hFont = font;
}

void CardButton::SetButtonProc(pButtonProc proc)
{
    ButtonCallback    = proc;
}

bool CardButton::Lock()
{
    DWORD dw = WaitForSingleObject(mxlock, 0);

    if(dw == WAIT_OBJECT_0)
        return true;
    else
        return false;
}

bool CardButton::UnLock()
{
    if(ReleaseMutex(mxlock))
        return true;
    else
        return false;
}

void CardButton::SetStyle(UINT style)
{
    uStyle = style;
}

UINT CardButton::GetStyle()
{
    return uStyle;
}
