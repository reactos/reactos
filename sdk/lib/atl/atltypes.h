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

    CPoint() throw()
    {
        x = y = 0;
    }

    CPoint(int initX, int initY) throw()
    {
        x = initX;
        y = initY;
    }

    CPoint(POINT initPt) throw()
    {
        *((POINT*)this) = initPt;
    }

    CPoint(SIZE initSize) throw()
    {
        *((SIZE*)this) = initSize;
    }

    CPoint(LPARAM dwPoint) throw()
    {
        x = LOWORD(dwPoint);
        y = HIWORD(dwPoint);
    }

    void Offset(int xOffset, int yOffset) throw()
    {
        x += xOffset;
        y += yOffset;
    }

    void Offset(POINT point) throw()
    {
        Offset(point.x, point.y);
    }

    void Offset(SIZE size) throw()
    {
        Offset(size.cx, size.cy);
    }

    BOOL operator==(POINT point) const throw()
    {
        return (x == point.x && y == point.y);
    }

    BOOL operator!=(POINT point) const throw()
    {
        return !(*this == point);
    }

    void operator+=(SIZE size) throw()
    {
        Offset(size);
    }

    void operator+=(POINT point) throw()
    {
        Offset(point);
    }

    void operator-=(SIZE size) throw()
    {
        Offset(-size.cx, -size.cy);
    }

    void operator-=(POINT point) throw()
    {
        Offset(-point.x, -point.y);
    }

    CPoint operator+(SIZE size) const throw()
    {
        return CPoint(x + size.cx, y + size.cy);
    }

    CPoint operator+(POINT point) const throw()
    {
        return CPoint(x + point.x, y + point.y);
    }

    CRect operator+(const RECT* lpRect) const throw();

    CSize operator-(POINT point) const throw();

    CPoint operator-(SIZE size) const throw()
    {
        return CPoint(x - size.cx, y - size.cy);
    }

    CRect operator-(const RECT* lpRect) const throw();

    CPoint operator-() const throw()
    {
        return CPoint(-x, -y);
    }
};

class CSize : public tagSIZE
{
public:
    CSize() throw()
    {
        cx = cy = 0;
    }

    CSize(int initCX, int initCY) throw()
    {
        cx = initCX;
        cy = initCY;
    }

    CSize(SIZE initSize) throw()
    {
        *((SIZE*)this) = initSize;
    }

    CSize(POINT initPt) throw()
    {
        *((POINT*)this) = initPt;
    }

    CSize(DWORD dwSize) throw()
    {
        cx = LOWORD(dwSize);
        cy = HIWORD(dwSize);
    }

    BOOL operator==(SIZE size) const throw()
    {
        return (size.cx == cx && size.cy == cy);
    }

    BOOL operator!=(SIZE size) const throw()
    {
        return !(*this == size);
    }

    void operator+=(SIZE size) throw()
    {
        cx += size.cx;
        cy += size.cy;
    }

    void operator-=(SIZE size) throw()
    {
        cx -= size.cx;
        cy -= size.cy;
    }

    CSize operator+(SIZE size) const throw()
    {
        return CSize(cx + size.cx, cy + size.cy);
    }

    CPoint operator+(POINT point) const throw()
    {
        return CPoint(cx + point.x, cy + point.y);
    }

    CRect operator+(const RECT* lpRect) const throw();

    CSize operator-(SIZE size) const throw()
    {
        return CSize(cx - size.cx, cy - size.cy);
    }

    CPoint operator-(POINT point) const throw()
    {
        return CPoint(cx - point.x, cy - point.y);
    }

    CRect operator-(const RECT* lpRect) const throw();

    CSize operator-() const throw()
    {
        return CSize(-cx, -cy);
    }
};


CSize CPoint::operator-(POINT point) const throw()
{
    return CSize(x - point.x, y - point.y);
}


class CRect : public tagRECT
{
public:
    CRect() throw()
    {
        left = top = right = bottom = 0;
    }

    CRect(int l, int t, int r, int b) throw()
    {
        left = l;
        top = t;
        right = r;
        bottom = b;
    }

    CRect(const RECT& srcRect) throw()
    {
        left = srcRect.left;
        top = srcRect.top;
        right = srcRect.right;
        bottom = srcRect.bottom;
    }

    CRect(LPCRECT lpSrcRect) throw()
    {
        left = lpSrcRect->left;
        top = lpSrcRect->top;
        right = lpSrcRect->right;
        bottom = lpSrcRect->bottom;
    }

    CRect(POINT point, SIZE size) throw()
    {
        left = point.x;
        top = point.y;
        right = point.x + size.cx;
        bottom = point.y + size.cy;
    }

    CRect(POINT topLeft, POINT bottomRight) throw()
    {
        left = topLeft.x;
        top = topLeft.y;
        right = bottomRight.x;
        bottom = bottomRight.y;
    }

    CPoint& BottomRight() throw()
    {
        return ((CPoint*)this)[1];
    }

    const CPoint& BottomRight() const throw()
    {
        return ((const CPoint*)this)[1];
    }

    CPoint CenterPoint() const throw()
    {
        return CPoint(left + (Width() >> 1), top + (Height() >> 1));
    }

    void CopyRect(LPCRECT lpSrcRect) throw()
    {
        ::CopyRect(this, lpSrcRect);
    }

    void DeflateRect(int x, int y) throw()
    {
        ::InflateRect(this, -x, -y);
    }

    void DeflateRect(SIZE size) throw()
    {
        ::InflateRect(this, -size.cx, -size.cy);
    }

    void DeflateRect(LPCRECT lpRect) throw()
    {
        DeflateRect(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
    }

    void DeflateRect(int l, int t, int r, int b) throw()
    {
        left += l;
        top += t;
        right -= r;
        bottom -= b;
    }

    BOOL EqualRect(LPCRECT lpRect) const throw()
    {
        return ::EqualRect(this, lpRect);
    }


    int Height() const throw()
    {
        return bottom - top;
    }

    void InflateRect(int x, int y) throw()
    {
        ::InflateRect(this, x, y);
    }

    void InflateRect(SIZE size) throw()
    {
        ::InflateRect(this, size.cx, size.cy);
    }

    void InflateRect(LPCRECT lpRect) throw()
    {
        InflateRect(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
    }

    void InflateRect(int l, int t, int r, int b) throw()
    {
        left -= l;
        top -= t;
        right += r;
        bottom += b;
    }

    BOOL IntersectRect(LPCRECT lpRect1, LPCRECT lpRect2) throw()
    {
        return ::IntersectRect(this, lpRect1, lpRect2);
    }

    BOOL IsRectEmpty() const throw()
    {
        return ::IsRectEmpty(this);
    }

    BOOL IsRectNull() const throw()
    {
        return (left == 0 && right == 0 &&
            top == 0 && bottom == 0);
    }

    //void MoveToX(int x) throw()
    //void MoveToXY(int x, int y) throw()
    //void MoveToXY(POINT point) throw()
    //void MoveToY(int y) throw()
    //void NormalizeRect() throw()

    void OffsetRect(int x, int y) throw()
    {
        ::OffsetRect(this, x, y);
    }

    void OffsetRect(POINT point) throw()
    {
        ::OffsetRect(this, point.x, point.y);
    }

    void OffsetRect(SIZE size) throw()
    {
        ::OffsetRect(this, size.cx, size.cy);
    }

    BOOL PtInRect(POINT point) const throw()
    {
        return ::PtInRect(this, point);
    }
    //void SetRect(int x1, int y1, int x2, int y2) throw()
    //void SetRectEmpty() throw()
    //CSize Size() const throw()
    //BOOL SubtractRect(LPCRECT lpRectSrc1, LPCRECT lpRectSrc2) throw()

    CPoint& TopLeft() throw()
    {
        return ((CPoint*)this)[0];
    }

    const CPoint& TopLeft() const throw()
    {
        return ((const CPoint*)this)[0];
    }

    BOOL UnionRect(LPCRECT lpRect1, LPCRECT lpRect2) throw()
    {
        return ::UnionRect(this, lpRect1, lpRect2);
    }

    int Width() const throw()
    {
        return right - left;
    }


    BOOL operator==(const RECT& rect) const throw()
    {
        return (left == rect.left &&
            top == rect.top &&
            right == rect.right &&
            bottom == rect.bottom);
    }

    BOOL operator!=(const RECT& rect) const throw()
    {
        return !(*this == rect);
    }

    void operator=(const RECT& srcRect) throw()
    {
        left = srcRect.left;
        top = srcRect.top;
        right = srcRect.right;
        bottom = srcRect.bottom;
    }

    void operator+=(POINT point) throw()
    {
        OffsetRect(point);
    }

    void operator+=(SIZE size) throw()
    {
        OffsetRect(size);
    }

    void operator+=(LPCRECT lpRect) throw()
    {
        InflateRect(lpRect);
    }

    void operator-=(POINT point) throw()
    {
        OffsetRect(-point.x, -point.y);
    }

    void operator-=(SIZE size) throw()
    {
        OffsetRect(-size.cx, -size.cy);
    }

    void operator-=(LPCRECT lpRect) throw()
    {
        DeflateRect(lpRect);
    }


    CRect operator+(POINT point) const throw()
    {
        CRect r(this);
        r.OffsetRect(point);
        return r;
    }

    CRect operator+(LPCRECT lpRect) const throw()
    {
        CRect r(this);
        r.InflateRect(lpRect);
        return r;
    }

    CRect operator+(SIZE size) const throw()
    {
        CRect r(this);
        r.OffsetRect(size);
        return r;
    }

    CRect operator-(POINT point) const throw()
    {
        CRect r(this);
        r.OffsetRect(-point.x, -point.y);
        return r;
    }

    CRect operator-(SIZE size) const throw()
    {
        CRect r(this);
        r.OffsetRect(-size.cx, -size.cy);
        return r;
    }

    CRect operator-(LPCRECT lpRect) const throw()
    {
        CRect r(this);
        r.DeflateRect(lpRect);
        return r;
    }

    void operator&=(const RECT& rect) throw()
    {
        IntersectRect(this, &rect);
    }

    CRect operator&(const RECT& rect2) const throw()
    {
        CRect r;
        r.IntersectRect(this, &rect2);
        return r;
    }

    void operator|=(const RECT& rect) throw()
    {
        UnionRect(this, &rect);
    }

    CRect operator|(const RECT& rect2) const throw()
    {
        CRect r;
        r.UnionRect(this, &rect2);
        return r;
    }

    operator LPRECT() throw()
    {
        return this;
    }

    operator LPCRECT() const throw()
    {
        return this;
    }
};

CRect CPoint::operator+(const RECT* lpRect) const throw()
{
    CRect r(lpRect);
    r += *this;
    return r;
}

CRect CPoint::operator-(const RECT* lpRect) const throw()
{
    CRect r(lpRect);
    r -= *this;
    return r;
}

CRect CSize::operator+(const RECT* lpRect) const throw()
{
    CRect r(lpRect);
    r += *this;
    return r;
}

CRect CSize::operator-(const RECT* lpRect) const throw()
{
    CRect r(lpRect);
    r -= *this;
    return r;
}

