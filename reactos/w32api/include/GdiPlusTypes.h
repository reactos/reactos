/*
 * GdiPlusTypes.h
 *
 * Windows GDI+
 *
 * This file is part of the w32api package.
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _GDIPLUSTYPES_H
#define _GDIPLUSTYPES_H

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

typedef float REAL;

extern "C" {
  typedef BOOL (CALLBACK * ImageAbort)(VOID *);
  typedef ImageAbort DrawImageAbort;
  typedef ImageAbort GetThumbnailImageAbort;
  typedef BOOL (CALLBACK * EnumerateMetafileProc)(EmfPlusRecordType, UINT, UINT, const BYTE*, VOID*);
}

typedef enum {
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
} Status;


class CharacterRange
{
public:
  CharacterRange(VOID)
  {
  }

  CharacterRange(INT first, INT length)
  {
  }

  CharacterRange &operator=(const CharacterRange &rhs)
  {
    First  = rhs.First;
    Length = rhs.Length;
    return *this;
  }

public:
  INT First;
  INT Length;
};


class SizeF
{
public:
  SizeF(VOID)
  {
  }

  SizeF(FLOAT width, FLOAT height)
  {
  }

  SizeF(const SizeF &size)
  {
  }

  BOOL Empty(VOID)
  {
    return FALSE;
  }

  BOOL Equals(const SizeF &sz)
  {
    return FALSE;
  }

  SizeF operator+(const SizeF &sz)
  {
    return SizeF(Width + sz.Width,
      Height + sz.Height);
  }

  SizeF operator-(const SizeF &sz)
  {
    return SizeF(Width - sz.Width,
      Height - sz.Height);
  }

public:
  FLOAT Height;
  FLOAT Width;
};


class PointF
{
public:
  PointF(REAL x, REAL y)
  {
  }

  PointF(const SizeF &size)
  {
  }

  PointF(VOID)
  {
  }

  PointF(const PointF &point)
  {
  }

  BOOL Equals(const PointF& point)
  {
    return FALSE;
  }

  PointF operator+(const PointF &point)
  {
    return PointF(X + point.X,
      Y + point.Y);
  }

  PointF operator-(const PointF &point)
  {
    return PointF(X - point.X,
      Y - point.Y);
  }

public:
  REAL X;
  REAL Y;
};


class PathData
{
public:
  PathData(VOID)
  {
  }

public:
  INT Count;
  PointF *Points;
  BYTE *Types;
};


class Size
{
public:
  Size(VOID)
  {
  }

  Size(INT width, INT height)
  {
  }

  Size(const Size &size)
  {
  }

  BOOL Empty(VOID)
  {
    return FALSE;
  }

  BOOL Equals(const Size &sz)
  {
    return FALSE;
  }

  Size operator+(const Size &sz)
  {
    return Size(Width + sz.Width,
      Height + sz.Height);
  }

  Size operator-(const Size &sz)
  {
    return Size(Width - sz.Width,
      Height - sz.Height);
  }

public:
  INT Height;
  INT Width;
};


class Point
{
public:
  Point(VOID)
  {
  }

  Point(INT x, INT y)
  {
  }

  Point(const Point &point)
  {
  }

  Point(const Size &size)
  {
  }

  BOOL Equals(const Point& point)
  {
    return FALSE;
  }

  Point operator+(const Point &point)
  {
    return Point(X + point.X,
      Y + point.Y);
  }

  Point operator-(const Point &point)
  {
    return Point(X - point.X,
      Y - point.Y);
  }

public:
  INT X;
  INT Y;
};


class Rect
{
public:
  Rect(VOID)
  {
  }

  Rect(const Point &location, const Size &size)
  {
  }

  Rect(INT x, INT y, INT width, INT height)
  {
  }

  Rect *Clone(VOID) const
  {
    return NULL;
  }

  BOOL Contains(const Point& pt)
  {
    return FALSE;
  }

  BOOL Contains(Rect& rect)
  {
    return FALSE;
  }

  BOOL Contains(INT x, INT y)
  {
    return FALSE;
  }

  BOOL Equals(const Rect& rect) const
  {
    return FALSE;
  }

  INT GetBottom(VOID) const
  {
    return 0;
  }

  VOID GetBounds(Rect* rect) const
  {
  }

  INT GetLeft(VOID) const
  {
    return 0;
  }

  VOID GetLocation(Point* point) const
  {
  }

  INT GetRight(VOID) const
  {
    return 0;
  }

  VOID GetSize(Size* size) const
  {
  }

  INT GetTop(VOID) const
  {
    return 0;
  }

  VOID Inflate(INT dx, INT dy)
  {
  }

  VOID Inflate(const Point& point)
  {
  }

  BOOL Intersect(Rect& c, const Rect& a, const Rect& b)
  {
    return FALSE;
  }

  BOOL Intersect(const Rect& rect)
  {
    return FALSE;
  }

  BOOL IntersectsWith(const Rect& rect) const
  {
    return FALSE;
  }

  BOOL IsEmptyArea(VOID) const
  {
    return FALSE;
  }

  VOID Offset(INT dx, INT dy)
  {
  }

  VOID Offset(const Point& point)
  {
  }

  BOOL Union(Rect& c, const Rect& a, const Rect& b)
  {
    return FALSE;
  }
public:
  INT X;
  INT Y;
  INT Width;
  INT Height;
};


class RectF
{
public:
  RectF(const PointF &location, const SizeF &size)
  {
  }

  RectF(VOID)
  {
  }

  RectF(REAL x, REAL y, REAL width, REAL height)
  {
  }

  RectF *Clone(VOID) const
  {
    return NULL;
  }

  BOOL Contains(const RectF& rect)
  {
    return FALSE;
  }

  BOOL Contains(const PointF& pt) const
  {
    return FALSE;
  }

  BOOL Contains(REAL x, REAL y)
  {
    return FALSE;
  }

  BOOL Equals(const RectF& rect) const
  {
    return FALSE;
  }

  REAL GetBottom(VOID) const
  {
    return 0;
  }

  VOID GetBounds(RectF* rect) const
  {
  }

  REAL GetLeft(VOID) const
  {
    return 0;
  }

  VOID GetLocation(PointF* point) const
  {
  }

  REAL GetRight(VOID) const
  {
    return 0;
  }

  VOID GetSize(SizeF* size) const
  {
  }

  REAL GetTop(VOID) const
  {
    return 0;
  }

  VOID Inflate(const PointF& point)
  {
  }

  VOID Inflate(REAL dx, REAL dy)
  {
  }

  BOOL Intersect(Rect& c, const Rect& a, const Rect& b)
  {
    return FALSE;
  }

  BOOL Intersect(const Rect& rect)
  {
    return FALSE;
  }

  BOOL IntersectsWith(const RectF& rect) const
  {
    return FALSE;
  }

  BOOL IsEmptyArea(VOID) const
  {
    return FALSE;
  }

  VOID Offset(INT dx, INT dy)
  {
  }

  VOID Offset(const Point& point)
  {
  }

  BOOL Union(RectF& c, const RectF& a, const RectF& b)
  {
    return FALSE;
  }

public:
  REAL X;
  REAL Y;
  REAL Width;
  REAL Height;
};

#endif /* _GDIPLUSTYPES_H */
