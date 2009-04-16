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

#ifndef __WINE_GP_PRIVATE_H_
#define __WINE_GP_PRIVATE_H_

#include <math.h>
#include <stdarg.h>

#include "windef.h"
#include "wingdi.h"
#include "winbase.h"
#include "winuser.h"

#include "objbase.h"
#include "ocidl.h"

#include "gdiplus.h"

#define GP_DEFAULT_PENSTYLE (PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT | PS_JOIN_MITER)
#define MAX_ARC_PTS (13)
#define MAX_DASHLEN (16) /* this is a limitation of gdi */
#define INCH_HIMETRIC (2540)

#define VERSION_MAGIC 0xdbc01001
#define TENSION_CONST (0.3)

COLORREF ARGB2COLORREF(ARGB color);
extern INT arc2polybezier(GpPointF * points, REAL x1, REAL y1, REAL x2, REAL y2,
    REAL startAngle, REAL sweepAngle);
extern REAL gdiplus_atan2(REAL dy, REAL dx);
extern GpStatus hresult_to_status(HRESULT res);
extern REAL convert_unit(HDC hdc, GpUnit unit);

extern void calc_curve_bezier(CONST GpPointF *pts, REAL tension, REAL *x1,
    REAL *y1, REAL *x2, REAL *y2);
extern void calc_curve_bezier_endp(REAL xend, REAL yend, REAL xadj, REAL yadj,
    REAL tension, REAL *x, REAL *y);

extern BOOL lengthen_path(GpPath *path, INT len);

extern GpStatus trace_path(GpGraphics *graphics, GpPath *path);

typedef struct region_element region_element;
extern inline void delete_element(region_element *element);

static inline INT roundr(REAL x)
{
    return (INT) floorf(x + 0.5);
}

static inline REAL deg2rad(REAL degrees)
{
    return M_PI * degrees / 180.0;
}

struct GpPen{
    UINT style;
    GpUnit unit;
    REAL width;
    GpLineCap endcap;
    GpLineCap startcap;
    GpDashCap dashcap;
    GpCustomLineCap *customstart;
    GpCustomLineCap *customend;
    GpLineJoin join;
    REAL miterlimit;
    GpDashStyle dash;
    REAL *dashes;
    INT numdashes;
    REAL offset;    /* dash offset */
    GpBrush *brush;
    GpPenAlignment align;
};

struct GpGraphics{
    HDC hdc;
    HWND hwnd;
    SmoothingMode smoothing;
    CompositingQuality compqual;
    InterpolationMode interpolation;
    PixelOffsetMode pixeloffset;
    CompositingMode compmode;
    TextRenderingHint texthint;
    GpUnit unit;    /* page unit */
    REAL scale;     /* page scale */
    GpMatrix * worldtrans; /* world transform */
    BOOL busy;      /* hdc handle obtained by GdipGetDC */
    GpRegion *clip;
    UINT textcontrast; /* not used yet. get/set only */
};

struct GpBrush{
    HBRUSH gdibrush;
    GpBrushType bt;
    LOGBRUSH lb;
};

struct GpHatch{
    GpBrush brush;
    HatchStyle hatchstyle;
    ARGB forecol;
    ARGB backcol;
};

struct GpSolidFill{
    GpBrush brush;
    ARGB color;
};

struct GpPathGradient{
    GpBrush brush;
    PathData pathdata;
    ARGB centercolor;
    GpWrapMode wrap;
    BOOL gamma;
    GpPointF center;
    GpPointF focus;
    REAL* blendfac;  /* blend factors */
    REAL* blendpos;  /* blend positions */
    INT blendcount;
};

struct GpLineGradient{
    GpBrush brush;
    GpPointF startpoint;
    GpPointF endpoint;
    ARGB startcolor;
    ARGB endcolor;
    GpWrapMode wrap;
    BOOL gamma;
};

struct GpTexture{
    GpBrush brush;
    GpMatrix *transform;
    WrapMode wrap;  /* not used yet */
};

struct GpPath{
    GpFillMode fill;
    GpPathData pathdata;
    BOOL newfigure; /* whether the next drawing action starts a new figure */
    INT datalen; /* size of the arrays in pathdata */
};

struct GpMatrix{
    REAL matrix[6];
};

struct GpPathIterator{
    GpPathData pathdata;
    INT subpath_pos;    /* for NextSubpath methods */
    INT marker_pos;     /* for NextMarker methods */
    INT pathtype_pos;   /* for NextPathType methods */
};

struct GpCustomLineCap{
    GpPathData pathdata;
    BOOL fill;      /* TRUE for fill, FALSE for stroke */
    GpLineCap cap;  /* as far as I can tell, this value is ignored */
    REAL inset;     /* how much to adjust the end of the line */
    GpLineJoin join;
    REAL scale;
};

struct GpAdustableArrowCap{
    GpCustomLineCap cap;
};

struct GpImage{
    IPicture* picture;
    ImageType type;
    UINT flags;
};

struct GpMetafile{
    GpImage image;
    GpRectF bounds;
    GpUnit unit;
};

struct GpBitmap{
    GpImage image;
    INT width;
    INT height;
    PixelFormat format;
    ImageLockMode lockmode;
    INT numlocks;
    BYTE *bitmapbits;   /* pointer to the buffer we passed in BitmapLockBits */
};

struct GpCachedBitmap{
    GpImage *image;
};

struct GpImageAttributes{
    WrapMode wrap;
};

struct GpFont{
    LOGFONTW lfw;
    REAL emSize;
    UINT height;
    LONG line_spacing;
    Unit unit;
};

struct GpStringFormat{
    INT attr;
    LANGID lang;
    LANGID digitlang;
    StringAlignment align;
    StringTrimming trimming;
    HotkeyPrefix hkprefix;
    StringAlignment vertalign;
    StringDigitSubstitute digitsub;
    INT tabcount;
    REAL firsttab;
    REAL *tabs;
};

struct GpFontCollection{
    GpFontFamily **FontFamilies;
    INT count;
};

struct GpFontFamily{
    NEWTEXTMETRICW tmw;
    WCHAR FamilyName[LF_FACESIZE];
};

/* internal use */
typedef enum RegionType
{
    RegionDataRect          = 0x10000000,
    RegionDataPath          = 0x10000001,
    RegionDataEmptyRect     = 0x10000002,
    RegionDataInfiniteRect  = 0x10000003,
} RegionType;

struct region_element
{
    DWORD type; /* Rectangle, Path, SpecialRectangle, or CombineMode */
    union
    {
        GpRectF rect;
        struct
        {
            GpPath* path;
            struct
            {
                DWORD size;
                DWORD magic;
                DWORD count;
                DWORD flags;
            } pathheader;
        } pathdata;
        struct
        {
            struct region_element *left;  /* the original region */
            struct region_element *right; /* what *left was combined with */
        } combine;
    } elementdata;
};

struct GpRegion{
    struct
    {
        DWORD size;
        DWORD checksum;
        DWORD magic;
        DWORD num_children;
    } header;
    region_element node;
};

#endif
