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

enum Status{
    Ok                          = 0,
    GenericError                = 1,
    InvalidParameter            = 2,
    OutOfMemory                 = 3,
    ObjectBusy                  = 4,
    InsufficientBuffer          = 5,
    NotImplemented              = 6,
    Win32Error                  = 7,
    WrongState                  = 8,
    Aborted                     = 9,
    FileNotFound                = 10,
    ValueOverflow               = 11,
    AccessDenied                = 12,
    UnknownImageFormat          = 13,
    FontFamilyNotFound          = 14,
    FontStyleNotFound           = 15,
    NotTrueTypeFont             = 16,
    UnsupportedGdiplusVersion   = 17,
    GdiplusNotInitialized       = 18,
    PropertyNotFound            = 19,
    PropertyNotSupported        = 20,
    ProfileNotFound             = 21
};


#ifdef __cplusplus
extern "C" {
#endif

typedef BOOL (CALLBACK * ImageAbort)(VOID *);
typedef ImageAbort DrawImageAbort;
typedef ImageAbort GetThumbnailImageAbort;
typedef struct GdiplusAbort GdiplusAbort;

typedef BOOL (CALLBACK * EnumerateMetafileProc)(EmfPlusRecordType,UINT,UINT,const BYTE*,VOID*);

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

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

   /* FIXME: missing constructor that takes a Size */

   Point(IN INT x, IN INT y)
   {
       X = x;
       Y = y;
   }

   Point operator+(IN const Point& pt) const
   {
       return Point(X + pt.X, Y + pt.Y);
   }

   Point operator-(IN const Point& pt) const
   {
       return Point(X - pt.X, Y - pt.Y);
   }

   BOOL Equals(IN const Point& pt)
   {
       return (X == pt.X) && (Y == pt.Y);
   }

public:
    INT X;
    INT Y;
};

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

   /* FIXME: missing constructor that takes a SizeF */

   PointF(IN REAL x, IN REAL y)
   {
       X = x;
       Y = y;
   }

   PointF operator+(IN const PointF& pt) const
   {
       return PointF(X + pt.X, Y + pt.Y);
   }

   PointF operator-(IN const PointF& pt) const
   {
       return PointF(X - pt.X, Y - pt.Y);
   }

   BOOL Equals(IN const PointF& pt)
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
    PathData& operator=(const PathData &);

public:
    INT Count;
    PointF* Points;
    BYTE* Types;
};

/* FIXME: missing the methods. */
class RectF
{
public:
    REAL X;
    REAL Y;
    REAL Width;
    REAL Height;
};

/* FIXME: missing the methods. */
class Rect
{
public:
    INT X;
    INT Y;
    INT Width;
    INT Height;
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

    CharacterRange& operator=(const CharacterRange& rhs)
    {
        First = rhs.First;
        Length = rhs.Length;
        return *this;
    }
public:
    INT First;
    INT Length;
};

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
    PointF* Points;
    BYTE* Types;
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
