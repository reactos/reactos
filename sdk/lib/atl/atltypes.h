/*
 * ReactOS ATL
 *
 * Copyright 2016 Mark Jansen
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

#pragma once


class CSize;
class CRect;


class CPoint : public tagPOINT
{
public:

    CPoint() noexcept
    {
        x = y = 0;
    }

    CPoint(int initX, int initY) noexcept
    {
        x = initX;
        y = initY;
    }

    CPoint(POINT initPt) noexcept
    {
        *((POINT*)this) = initPt;
    }

    CPoint(SIZE initSize) noexcept
    {
        *((SIZE*)this) = initSize;
    }

    CPoint(LPARAM dwPoint) noexcept
    {
        x = LOWORD(dwPoint);
        y = HIWORD(dwPoint);
    }

    void Offset(int xOffset, int yOffset) noexcept
    {
        x += xOffset;
        y += yOffset;
    }

    void Offset(POINT point) noexcept
    {
        Offset(point.x, point.y);
    }

    void Offset(SIZE size) noexcept
    {
        Offset(size.cx, size.cy);
    }

    BOOL operator==(POINT point) const noexcept
    {
        return (x == point.x && y == point.y);
    }

    BOOL operator!=(POINT point) const noexcept
    {
        return !(*this == point);
    }

    void operator+=(SIZE size) noexcept
    {
        Offset(size);
    }

    void operator+=(POINT point) noexcept
    {
        Offset(point);
    }

    void operator-=(SIZE size) noexcept
    {
        Offset(-size.cx, -size.cy);
    }

    void operator-=(POINT point) noexcept
    {
        Offset(-point.x, -point.y);
    }

    CPoint operator+(SIZE size) const noexcept
    {
        return CPoint(x + size.cx, y + size.cy);
    }

    CPoint operator+(POINT point) const noexcept
    {
        return CPoint(x + point.x, y + point.y);
    }

    CRect operator+(const RECT* lpRect) const noexcept;

    CSize operator-(POINT point) const noexcept;

    CPoint operator-(SIZE size) const noexcept
    {
        return CPoint(x - size.cx, y - size.cy);
    }

    CRect operator-(const RECT* lpRect) const noexcept;

    CPoint operator-() const noexcept
    {
        return CPoint(-x, -y);
    }
};

class CSize : public tagSIZE
{
public:
    CSize() noexcept
    {
        cx = cy = 0;
    }

    CSize(int initCX, int initCY) noexcept
    {
        cx = initCX;
        cy = initCY;
    }

    CSize(SIZE initSize) noexcept
    {
        *((SIZE*)this) = initSize;
    }

    CSize(POINT initPt) noexcept
    {
        *((POINT*)this) = initPt;
    }

    CSize(DWORD dwSize) noexcept
    {
        cx = LOWORD(dwSize);
        cy = HIWORD(dwSize);
    }

    BOOL operator==(SIZE size) const noexcept
    {
        return (size.cx == cx && size.cy == cy);
    }

    BOOL operator!=(SIZE size) const noexcept
    {
        return !(*this == size);
    }

    void operator+=(SIZE size) noexcept
    {
        cx += size.cx;
        cy += size.cy;
    }

    void operator-=(SIZE size) noexcept
    {
        cx -= size.cx;
        cy -= size.cy;
    }

    CSize operator+(SIZE size) const noexcept
    {
        return CSize(cx + size.cx, cy + size.cy);
    }

    CPoint operator+(POINT point) const noexcept
    {
        return CPoint(cx + point.x, cy + point.y);
    }

    CRect operator+(const RECT* lpRect) const noexcept;

    CSize operator-(SIZE size) const noexcept
    {
        return CSize(cx - size.cx, cy - size.cy);
    }

    CPoint operator-(POINT point) const noexcept
    {
        return CPoint(cx - point.x, cy - point.y);
    }

    CRect operator-(const RECT* lpRect) const noexcept;

    CSize operator-() const noexcept
    {
        return CSize(-cx, -cy);
    }
};


inline CSize CPoint::operator-(POINT point) const noexcept
{
    return CSize(x - point.x, y - point.y);
}


class CRect : public tagRECT
{
public:
    CRect() noexcept
    {
        left = top = right = bottom = 0;
    }

    CRect(int l, int t, int r, int b) noexcept
    {
        left = l;
        top = t;
        right = r;
        bottom = b;
    }

    CRect(const RECT& srcRect) noexcept
    {
        left = srcRect.left;
        top = srcRect.top;
        right = srcRect.right;
        bottom = srcRect.bottom;
    }

    CRect(LPCRECT lpSrcRect) noexcept
    {
        left = lpSrcRect->left;
        top = lpSrcRect->top;
        right = lpSrcRect->right;
        bottom = lpSrcRect->bottom;
    }

    CRect(POINT point, SIZE size) noexcept
    {
        left = point.x;
        top = point.y;
        right = point.x + size.cx;
        bottom = point.y + size.cy;
    }

    CRect(POINT topLeft, POINT bottomRight) noexcept
    {
        left = topLeft.x;
        top = topLeft.y;
        right = bottomRight.x;
        bottom = bottomRight.y;
    }

    CPoint& BottomRight() noexcept
    {
        return ((CPoint*)this)[1];
    }

    const CPoint& BottomRight() const noexcept
    {
        return ((const CPoint*)this)[1];
    }

    CPoint CenterPoint() const noexcept
    {
        return CPoint(left + (Width() >> 1), top + (Height() >> 1));
    }

    void CopyRect(LPCRECT lpSrcRect) noexcept
    {
        ::CopyRect(this, lpSrcRect);
    }

    void DeflateRect(int x, int y) noexcept
    {
        ::InflateRect(this, -x, -y);
    }

    void DeflateRect(SIZE size) noexcept
    {
        ::InflateRect(this, -size.cx, -size.cy);
    }

    void DeflateRect(LPCRECT lpRect) noexcept
    {
        DeflateRect(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
    }

    void DeflateRect(int l, int t, int r, int b) noexcept
    {
        left += l;
        top += t;
        right -= r;
        bottom -= b;
    }

    BOOL EqualRect(LPCRECT lpRect) const noexcept
    {
        return ::EqualRect(this, lpRect);
    }


    int Height() const noexcept
    {
        return bottom - top;
    }

    void InflateRect(int x, int y) noexcept
    {
        ::InflateRect(this, x, y);
    }

    void InflateRect(SIZE size) noexcept
    {
        ::InflateRect(this, size.cx, size.cy);
    }

    void InflateRect(LPCRECT lpRect) noexcept
    {
        InflateRect(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
    }

    void InflateRect(int l, int t, int r, int b) noexcept
    {
        left -= l;
        top -= t;
        right += r;
        bottom += b;
    }

    BOOL IntersectRect(LPCRECT lpRect1, LPCRECT lpRect2) noexcept
    {
        return ::IntersectRect(this, lpRect1, lpRect2);
    }

    BOOL IsRectEmpty() const noexcept
    {
        return ::IsRectEmpty(this);
    }

    BOOL IsRectNull() const noexcept
    {
        return (left == 0 && right == 0 &&
            top == 0 && bottom == 0);
    }

    void MoveToX(int x) noexcept
    {
        int dx = x - left;
        left = x;
        right += dx;
    }

    void MoveToY(int y) noexcept
    {
        int dy = y - top;
        top = y;
        bottom += dy;
    }

    void MoveToXY(int x, int y) noexcept
    {
        MoveToX(x);
        MoveToY(y);
    }

    void MoveToXY(POINT point) noexcept
    {
        MoveToXY(point.x, point.y);
    }

    void NormalizeRect() noexcept
    {
        if (left > right)
        {
            LONG tmp = left;
            left = right;
            right = tmp;
        }
        if (top > bottom)
        {
            LONG tmp = top;
            top = bottom;
            bottom = tmp;
        }
    }

    void OffsetRect(int x, int y) noexcept
    {
        ::OffsetRect(this, x, y);
    }

    void OffsetRect(POINT point) noexcept
    {
        ::OffsetRect(this, point.x, point.y);
    }

    void OffsetRect(SIZE size) noexcept
    {
        ::OffsetRect(this, size.cx, size.cy);
    }

    BOOL PtInRect(POINT point) const noexcept
    {
        return ::PtInRect(this, point);
    }

    void SetRect(int x1, int y1, int x2, int y2) noexcept
    {
        left = x1;
        top = y1;
        right = x2;
        bottom = y2;
    }

    void SetRectEmpty() noexcept
    {
        ZeroMemory(this, sizeof(*this));
    }

    CSize Size() const noexcept
    {
        return CSize(Width(), Height());
    }

    BOOL SubtractRect(LPCRECT lpRectSrc1, LPCRECT lpRectSrc2) noexcept
    {
        return ::SubtractRect(this, lpRectSrc1, lpRectSrc2);
    }

    CPoint& TopLeft() noexcept
    {
        return ((CPoint*)this)[0];
    }

    const CPoint& TopLeft() const noexcept
    {
        return ((const CPoint*)this)[0];
    }

    BOOL UnionRect(LPCRECT lpRect1, LPCRECT lpRect2) noexcept
    {
        return ::UnionRect(this, lpRect1, lpRect2);
    }

    int Width() const noexcept
    {
        return right - left;
    }


    BOOL operator==(const RECT& rect) const noexcept
    {
        return (left == rect.left &&
            top == rect.top &&
            right == rect.right &&
            bottom == rect.bottom);
    }

    BOOL operator!=(const RECT& rect) const noexcept
    {
        return !(*this == rect);
    }

    void operator=(const RECT& srcRect) noexcept
    {
        left = srcRect.left;
        top = srcRect.top;
        right = srcRect.right;
        bottom = srcRect.bottom;
    }

    void operator+=(POINT point) noexcept
    {
        OffsetRect(point);
    }

    void operator+=(SIZE size) noexcept
    {
        OffsetRect(size);
    }

    void operator+=(LPCRECT lpRect) noexcept
    {
        InflateRect(lpRect);
    }

    void operator-=(POINT point) noexcept
    {
        OffsetRect(-point.x, -point.y);
    }

    void operator-=(SIZE size) noexcept
    {
        OffsetRect(-size.cx, -size.cy);
    }

    void operator-=(LPCRECT lpRect) noexcept
    {
        DeflateRect(lpRect);
    }


    CRect operator+(POINT point) const noexcept
    {
        CRect r(this);
        r.OffsetRect(point);
        return r;
    }

    CRect operator+(LPCRECT lpRect) const noexcept
    {
        CRect r(this);
        r.InflateRect(lpRect);
        return r;
    }

    CRect operator+(SIZE size) const noexcept
    {
        CRect r(this);
        r.OffsetRect(size);
        return r;
    }

    CRect operator-(POINT point) const noexcept
    {
        CRect r(this);
        r.OffsetRect(-point.x, -point.y);
        return r;
    }

    CRect operator-(SIZE size) const noexcept
    {
        CRect r(this);
        r.OffsetRect(-size.cx, -size.cy);
        return r;
    }

    CRect operator-(LPCRECT lpRect) const noexcept
    {
        CRect r(this);
        r.DeflateRect(lpRect);
        return r;
    }

    void operator&=(const RECT& rect) noexcept
    {
        IntersectRect(this, &rect);
    }

    CRect operator&(const RECT& rect2) const noexcept
    {
        CRect r;
        r.IntersectRect(this, &rect2);
        return r;
    }

    void operator|=(const RECT& rect) noexcept
    {
        UnionRect(this, &rect);
    }

    CRect operator|(const RECT& rect2) const noexcept
    {
        CRect r;
        r.UnionRect(this, &rect2);
        return r;
    }

    operator LPRECT() noexcept
    {
        return this;
    }

    operator LPCRECT() const noexcept
    {
        return this;
    }
};

inline CRect CPoint::operator+(const RECT* lpRect) const noexcept
{
    CRect r(lpRect);
    r += *this;
    return r;
}

inline CRect CPoint::operator-(const RECT* lpRect) const noexcept
{
    CRect r(lpRect);
    r -= *this;
    return r;
}

inline CRect CSize::operator+(const RECT* lpRect) const noexcept
{
    CRect r(lpRect);
    r += *this;
    return r;
}

inline CRect CSize::operator-(const RECT* lpRect) const noexcept
{
    CRect r(lpRect);
    r -= *this;
    return r;
}

