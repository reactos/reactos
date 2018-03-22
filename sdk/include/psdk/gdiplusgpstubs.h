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

#ifndef _GDIPLUSGPSTUBS_H
#define _GDIPLUSGPSTUBS_H

#ifdef __cplusplus

class GpGraphics {};
class GpPen {};
class GpBrush {};
class GpHatch : public GpBrush {};
class GpSolidFill : public GpBrush {};
class GpPath {};
class GpMatrix {};
class GpPathIterator {};
class GpCustomLineCap {};
class GpAdjustableArrowCap : public GpCustomLineCap {};
class GpImage {};
class GpMetafile : public GpImage {};
class GpImageAttributes {};
class GpCachedBitmap {};
class GpBitmap : public GpImage {};
class GpPathGradient : public GpBrush {};
class GpLineGradient : public GpBrush {};
class GpTexture : public GpBrush {};
class GpFont {};
class GpFontCollection {};
class GpFontFamily {};
class GpStringFormat {};
class GpRegion {};
class CGpEffect {};

#else /* end of c++ declarations */

typedef struct GpGraphics GpGraphics;
typedef struct GpPen GpPen;
typedef struct GpBrush GpBrush;
typedef struct GpHatch GpHatch;
typedef struct GpSolidFill GpSolidFill;
typedef struct GpPath GpPath;
typedef struct GpMatrix GpMatrix;
typedef struct GpPathIterator GpPathIterator;
typedef struct GpCustomLineCap GpCustomLineCap;
typedef struct GpAdjustableArrowCap GpAdjustableArrowCap;
typedef struct GpImage GpImage;
typedef struct GpMetafile GpMetafile;
typedef struct GpImageAttributes GpImageAttributes;
typedef struct GpCachedBitmap GpCachedBitmap;
typedef struct GpBitmap GpBitmap;
typedef struct GpPathGradient GpPathGradient;
typedef struct GpLineGradient GpLineGradient;
typedef struct GpTexture GpTexture;
typedef struct GpFont GpFont;
typedef struct GpFontCollection GpFontCollection;
typedef struct GpFontFamily GpFontFamily;
typedef struct GpStringFormat GpStringFormat;
typedef struct GpRegion GpRegion;
typedef struct CGpEffect CGpEffect;

#endif /* end of c declarations */

typedef Status GpStatus;
typedef Unit GpUnit;
typedef BrushType GpBrushType;
typedef PointF GpPointF;
typedef FillMode GpFillMode;
typedef PathData GpPathData;
typedef LineCap GpLineCap;
typedef RectF GpRectF;
typedef Rect GpRect;
typedef LineJoin GpLineJoin;
typedef DashCap GpDashCap;
typedef DashStyle GpDashStyle;
typedef MatrixOrder GpMatrixOrder;
typedef Point GpPoint;
typedef WrapMode GpWrapMode;
typedef Color GpColor;
typedef FlushIntention GpFlushIntention;
typedef CoordinateSpace GpCoordinateSpace;
typedef PenAlignment GpPenAlignment;
typedef PenType GpPenType;
typedef HatchStyle GpHatchStyle;

#endif
