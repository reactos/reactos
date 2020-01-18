/*
 * Copyright (C) 2007 Google (Evan Stade)
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _GDIPLUSTYPES_H
#define _GDIPLUSTYPES_H

typedef float REAL;

enum Status
{
    Ok = 0,
    GenericError = 1,
    InvalidParameter = 2,
    OutOfMemory = 3,
    ObjectBusy = 4,
    InsufficientBuffer = 5,
    NotImplemented = 6,
    Win32Error = 7,
    WrongState = 8,
    Aborted = 9,
    FileNotFound = 10,
    ValueOverflow = 11,
    AccessDenied = 12,
    UnknownImageFormat = 13,
    FontFamilyNotFound = 14,
    FontStyleNotFound = 15,
    NotTrueTypeFont = 16,
    UnsupportedGdiplusVersion = 17,
    GdiplusNotInitialized = 18,
    PropertyNotFound = 19,
    PropertyNotSupported = 20,
    ProfileNotFound = 21
};

#ifdef __cplusplus
extern "C"
{
#endif

    typedef BOOL(CALLBACK *ImageAbort)(VOID *);
    typedef ImageAbort DrawImageAbort;
    typedef ImageAbort GetThumbnailImageAbort;
    typedef struct GdiplusAbort GdiplusAbort;

    typedef BOOL(CALLBACK *EnumerateMetafileProc)(EmfPlusRecordType, UINT, UINT, const BYTE *, VOID *);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class Size;

class Point
{
  public:
    Point()
    {
        X = Y = 0;
    }

    Point(IN const Point &pt)
    {
        X = pt.X;
        Y = pt.Y;
    }

    Point(const Size &size);

    Point(IN INT x, IN INT y)
    {
        X = x;
        Y = y;
    }

    Point
    operator+(IN const Point &pt) const
    {
        return Point(X + pt.X, Y + pt.Y);
    }

    Point
    operator-(IN const Point &pt) const
    {
        return Point(X - pt.X, Y - pt.Y);
    }

    BOOL
    Equals(IN const Point &pt) const
    {
        return (X == pt.X) && (Y == pt.Y);
    }

  public:
    INT X;
    INT Y;
};

class SizeF;

class PointF
{
  public:
    PointF()
    {
        X = Y = 0.0f;
    }

    PointF(IN const PointF &pt)
    {
        X = pt.X;
        Y = pt.Y;
    }

    PointF(const SizeF &size);

    PointF(IN REAL x, IN REAL y)
    {
        X = x;
        Y = y;
    }

    PointF
    operator+(IN const PointF &pt) const
    {
        return PointF(X + pt.X, Y + pt.Y);
    }

    PointF
    operator-(IN const PointF &pt) const
    {
        return PointF(X - pt.X, Y - pt.Y);
    }

    BOOL
    Equals(IN const PointF &pt) const
    {
        return (X == pt.X) && (Y == pt.Y);
    }

  public:
    REAL X;
    REAL Y;
};

class PathData
{
  public:
    PathData()
    {
        Count = 0;
        Points = NULL;
        Types = NULL;
    }

    ~PathData()
    {
        if (Points != NULL)
        {
            delete Points;
        }

        if (Types != NULL)
        {
            delete Types;
        }
    }

  private:
    PathData(const PathData &);
    PathData &
    operator=(const PathData &);

  public:
    INT Count;
    PointF *Points;
    BYTE *Types;
};

class SizeF
{
  public:
    REAL Width;
    REAL Height;

    SizeF() : Width(0), Height(0)
    {
    }

    SizeF(const SizeF &size) : Width(size.Width), Height(size.Height)
    {
    }

    SizeF(REAL width, REAL height) : Width(width), Height(height)
    {
    }

    BOOL
    Empty() const
    {
        return Width == 0 && Height == 0;
    }

    BOOL
    Equals(const SizeF &sz) const
    {
        return Width == sz.Width && Height == sz.Height;
    }

    SizeF
    operator+(const SizeF &sz) const
    {
        return SizeF(Width + sz.Width, Height + sz.Height);
    }

    SizeF
    operator-(const SizeF &sz) const
    {
        return SizeF(Width - sz.Width, Height - sz.Height);
    }
};

#define REAL_EPSILON 1.192092896e-07F /* FLT_EPSILON */

class RectF
{
  public:
    REAL X;
    REAL Y;
    REAL Width;
    REAL Height;

    RectF() : X(0), Y(0), Width(0), Height(0)
    {
    }

    RectF(const PointF &location, const SizeF &size)
        : X(location.X), Y(location.Y), Width(size.Width), Height(size.Height)
    {
    }

    RectF(REAL x, REAL y, REAL width, REAL height) : X(x), Y(y), Width(width), Height(height)
    {
    }

    RectF *
    Clone() const
    {
        return new RectF(X, Y, Width, Height);
    }

    BOOL
    Contains(const PointF &pt) const
    {
        return Contains(pt.X, pt.Y);
    }

    BOOL
    Contains(const RectF &rect) const
    {
        return X <= rect.X && rect.GetRight() <= GetRight() && Y <= rect.Y && rect.GetBottom() <= GetBottom();
    }

    BOOL
    Contains(REAL x, REAL y) const
    {
        return X <= x && x < X + Width && Y <= y && y < Y + Height;
    }

    BOOL
    Equals(const RectF &rect) const
    {
        return X == rect.X && Y == rect.Y && Width == rect.Width && Height == rect.Height;
    }

    REAL
    GetBottom() const
    {
        return Y + Height;
    }

    VOID
    GetBounds(RectF *rect) const
    {
        rect->X = X;
        rect->Y = Y;
        rect->Width = Width;
        rect->Height = Height;
    }

    REAL
    GetLeft() const
    {
        return X;
    }

    VOID
    GetLocation(PointF *point) const
    {
        point->X = X;
        point->Y = Y;
    }

    REAL
    GetRight() const
    {
        return X + Width;
    }

    VOID
    GetSize(SizeF *size) const
    {
        size->Width = Width;
        size->Height = Height;
    }

    REAL
    GetTop() const
    {
        return Y;
    }

    VOID
    Inflate(REAL dx, REAL dy)
    {
        X -= dx;
        Y -= dy;
        Width += 2 * dx;
        Height += 2 * dy;
    }

    VOID
    Inflate(const PointF &point)
    {
        Inflate(point.X, point.Y);
    }

    static BOOL
    Intersect(RectF &c, const RectF &a, const RectF &b)
    {
        // FIXME
        return FALSE;
    }

    BOOL
    Intersect(const RectF &rect)
    {
        return Intersect(*this, *this, rect);
    }

    BOOL
    IntersectsWith(const RectF &rect) const
    {
        return GetLeft() < rect.GetRight() && GetTop() < rect.GetTop() && GetRight() > rect.GetLeft() &&
               GetBottom() > rect.GetTop();
    }

    BOOL
    IsEmptyArea() const
    {
        return (Width <= REAL_EPSILON) || (Height <= REAL_EPSILON);
    }

    VOID
    Offset(REAL dx, REAL dy)
    {
        X += dx;
        Y += dy;
    }

    VOID
    Offset(const PointF &point)
    {
        Offset(point.X, point.Y);
    }

    static BOOL
    Union(RectF &c, const RectF &a, const RectF &b)
    {
        // FIXME
        return FALSE;
    }
};

class Size
{
  public:
    INT Width;
    INT Height;

    Size() : Width(0), Height(0)
    {
    }

    Size(const Size &size) : Width(size.Width), Height(size.Height)
    {
    }

    Size(INT width, INT height) : Width(width), Height(height)
    {
    }

    BOOL
    Empty() const
    {
        return Width == 0 && Height == 0;
    }

    BOOL
    Equals(const Size &sz) const
    {
        return Width == sz.Width && Height == sz.Height;
    }

    Size
    operator+(const Size &sz) const
    {
        return Size(Width + sz.Width, Height + sz.Height);
    }

    Size
    operator-(const Size &sz) const
    {
        return Size(Width - sz.Width, Height - sz.Height);
    }
};

class Rect
{
  public:
    INT X;
    INT Y;
    INT Width;
    INT Height;

    Rect() : X(0), Y(0), Width(0), Height(0)
    {
    }

    Rect(const Point &location, const Size &size) : X(location.X), Y(location.Y), Width(size.Width), Height(size.Height)
    {
    }

    Rect(INT x, INT y, INT width, INT height) : X(x), Y(y), Width(width), Height(height)
    {
    }

    Rect *
    Clone() const
    {
        return new Rect(X, Y, Width, Height);
    }

    BOOL
    Contains(const Point &pt) const
    {
        return Contains(pt.X, pt.Y);
    }

    BOOL
    Contains(const Rect &rect) const
    {
        return X <= rect.X && rect.GetRight() <= GetRight() && Y <= rect.Y && rect.GetBottom() <= GetBottom();
    }

    BOOL
    Contains(INT x, INT y) const
    {
        return X <= x && x < X + Width && Y <= y && y < Y + Height;
    }

    BOOL
    Equals(const Rect &rect) const
    {
        return X == rect.X && Y == rect.Y && Width == rect.Width && Height == rect.Height;
    }

    INT
    GetBottom() const
    {
        return Y + Height;
    }

    VOID
    GetBounds(Rect *rect) const
    {
        rect->X = X;
        rect->Y = Y;
        rect->Width = Width;
        rect->Height = Height;
    }

    INT
    GetLeft() const
    {
        return X;
    }

    VOID
    GetLocation(Point *point) const
    {
        point->X = X;
        point->Y = Y;
    }

    INT
    GetRight() const
    {
        return X + Width;
    }

    VOID
    GetSize(Size *size) const
    {
        size->Width = Width;
        size->Height = Height;
    }

    INT
    GetTop() const
    {
        return Y;
    }

    VOID
    Inflate(INT dx, INT dy)
    {
        X -= dx;
        Y -= dy;
        Width += 2 * dx;
        Height += 2 * dy;
    }

    VOID
    Inflate(const Point &point)
    {
        Inflate(point.X, point.Y);
    }

    static BOOL
    Intersect(Rect &c, const Rect &a, const Rect &b)
    {
        // FIXME
        return FALSE;
    }

    BOOL
    Intersect(const Rect &rect)
    {
        return Intersect(*this, *this, rect);
    }

    BOOL
    IntersectsWith(const Rect &rect) const
    {
        return GetLeft() < rect.GetRight() && GetTop() < rect.GetTop() && GetRight() > rect.GetLeft() &&
               GetBottom() > rect.GetTop();
    }

    BOOL
    IsEmptyArea() const
    {
        return Width <= 0 || Height <= 0;
    }

    VOID
    Offset(INT dx, INT dy)
    {
        X += dx;
        Y += dy;
    }

    VOID
    Offset(const Point &point)
    {
        Offset(point.X, point.Y);
    }

    static BOOL
    Union(Rect &c, const Rect &a, const Rect &b)
    {
        // FIXME
        return FALSE;
    }
};

class CharacterRange
{
  public:
    CharacterRange()
    {
        First = Length = 0;
    }

    CharacterRange(INT first, INT length)
    {
        First = first;
        Length = length;
    }

    CharacterRange &
    operator=(const CharacterRange &rhs)
    {
        First = rhs.First;
        Length = rhs.Length;
        return *this;
    }

  public:
    INT First;
    INT Length;
};

inline Point::Point(const Size &size) : X(size.Width), Y(size.Height)
{
}

inline PointF::PointF(const SizeF &size) : X(size.Width), Y(size.Height)
{
}

#else /* end of c++ typedefs */

typedef struct Point
{
    INT X;
    INT Y;
} Point;

typedef struct PointF
{
    REAL X;
    REAL Y;
} PointF;

typedef struct PathData
{
    INT Count;
    PointF *Points;
    BYTE *Types;
} PathData;

typedef struct RectF
{
    REAL X;
    REAL Y;
    REAL Width;
    REAL Height;
} RectF;

typedef struct Rect
{
    INT X;
    INT Y;
    INT Width;
    INT Height;
} Rect;

typedef struct CharacterRange
{
    INT First;
    INT Length;
} CharacterRange;

typedef enum Status Status;

#endif /* end of c typedefs */

#endif
