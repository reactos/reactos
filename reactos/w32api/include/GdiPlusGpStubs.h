/*
 * GdiPlusGpStubs.h
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

#ifndef _GDIPLUSGPSTUBS_H
#define _GDIPLUSGPSTUBS_H

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

class Matrix;

class GpCustomLineCap {};
class GpAdjustableArrowCap : public GpCustomLineCap {};
class GpImage {};
class GpBitmap : public GpImage {};
class GpGraphics {};
class CGpEffect {};
class GpBrush {};
class GpPath {};
class GpCachedBitmap;
class GpFont {};
class GpFontFamily {};
class GpFontCollection {};
class GpPen {};
class GpRegion {};
class GpImageAttributes {};
class GpMetafile : public GpImage {};
class GpStringFormat {};
class GpHatch : public GpBrush {};
class GpLineGradient : public GpBrush {};
class GpPathGradient : public GpBrush {};
class GpPathIterator {};
class GpSolidFill : public GpBrush {};
class GpTexture : public GpBrush {};

typedef Status GpStatus;
typedef Rect GpRect;
typedef PathData GpPathData;
typedef BrushType GpBrushType;
typedef LineCap GpLineCap;
typedef LineJoin GpLineJoin;
typedef FlushIntention GpFlushIntention;
typedef Matrix GpMatrix;
typedef MatrixOrder GpMatrixOrder;
typedef Unit GpUnit;
typedef CoordinateSpace GpCoordinateSpace;
typedef PointF GpPointF;
typedef Point GpPoint;
typedef RectF GpRectF;
typedef FillMode GpFillMode;
typedef HatchStyle GpHatchStyle;
typedef WrapMode GpWrapMode;
typedef DashCap GpDashCap;
typedef PenAlignment GpPenAlignment;
typedef PenType GpPenType;
typedef DashStyle GpDashStyle;

#endif /* _GDIPLUSGPSTUBS_H */
