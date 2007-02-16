/*
 * GdiPlusPath.h
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

#ifndef _GDIPLUSPATH_H
#define _GDIPLUSPATH_H

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

class GraphicsPath : public GdiplusBase
{
public:
  GraphicsPath(const Point *points, const BYTE *types, INT count, FillMode fillMode)
  {
  }

  GraphicsPath(FillMode fillMode)
  {
  }

  GraphicsPath(const PointF *points, const BYTE *types, INT count, FillMode fillMode)
  {
  }

  Status AddArc(const Rect &rect, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status AddArc(const RectF &rect, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status AddArc(INT x, INT y, INT width, INT height, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status AddArc(REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status AddBezier(const Point &pt1, const Point &pt2, const Point &pt3, const Point &pt4)
  {
    return NotImplemented;
  }

  Status AddBezier(REAL x1, REAL y1, REAL x2, REAL y2, REAL x3, REAL y3, REAL x4, REAL y4)
  {
    return NotImplemented;
  }

  Status AddBezier(const PointF &pt1, const PointF &pt2, const PointF &pt3, const PointF &pt4)
  {
    return NotImplemented;
  }

  Status AddBezier(INT x1, INT y1, INT x2, INT y2, INT x3, INT y3, INT x4, INT y4)
  {
    return NotImplemented;
  }

  Status AddBeziers(const Point *points, INT count)
  {
    return NotImplemented;
  }

  Status AddBeziers(const PointF *points, INT count)
  {
    return NotImplemented;
  }

  Status AddClosedCurve(const Point *points, INT count)
  {
    return NotImplemented;
  }

  Status AddClosedCurve(const Point *points, INT count, REAL tension)
  {
    return NotImplemented;
  }

  Status AddClosedCurve(const PointF *points, INT count, REAL tension)
  {
    return NotImplemented;
  }

  Status AddClosedCurve(const PointF *points, INT count)
  {
    return NotImplemented;
  }

  Status AddCurve(const Point *points, INT count)
  {
    return NotImplemented;
  }

  Status AddCurve(const PointF *points, INT count, REAL tension)
  {
    return NotImplemented;
  }

  Status AddCurve(const PointF *points, INT count)
  {
    return NotImplemented;
  }

  Status AddCurve(const Point *points, INT count, INT offset, INT numberOfSegments, REAL tension)
  {
    return NotImplemented;
  }

  Status AddCurve(const Point *points, INT count, REAL tension)
  {
    return NotImplemented;
  }

  Status AddCurve(const PointF *points, INT count, INT offset, INT numberOfSegments, REAL tension)
  {
    return NotImplemented;
  }

  Status AddEllipse(const Rect &rect)
  {
    return NotImplemented;
  }

  Status AddEllipse(const RectF &rect)
  {
    return NotImplemented;
  }

  Status AddEllipse(INT x, INT y, INT width, INT height)
  {
    return NotImplemented;
  }

  Status AddEllipse(REAL x, REAL y, REAL width, REAL height)
  {
    return NotImplemented;
  }

  Status AddLine(const Point &pt1, const Point &pt2)
  {
    return NotImplemented;
  }

  Status AddLine(const PointF &pt1, const PointF &pt2)
  {
    return NotImplemented;
  }

  Status AddLine(REAL x1, REAL y1, REAL x2, REAL y2)
  {
    return NotImplemented;
  }

  Status AddLine(INT x1, INT y1, INT x2, INT y2)
  {
    return NotImplemented;
  }

  Status AddLines(const Point *points, INT count)
  {
    return NotImplemented;
  }

  Status AddLines(const PointF *points, INT count)
  {
    return NotImplemented;
  }

  Status AddPath(const GraphicsPath *addingPath, BOOL connect)
  {
    return NotImplemented;
  }

  Status AddPie(const Rect &rect, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status AddPie(INT x, INT y, INT width, INT height, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status AddPie(REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status AddPie(const RectF &rect, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status AddPolygon(const Point *points, INT count)
  {
    return NotImplemented;
  }

  Status AddPolygon(const PointF *points, INT count)
  {
    return NotImplemented;
  }

  Status AddRectangle(const Rect &rect)
  {
    return NotImplemented;
  }

  Status AddRectangle(const RectF &rect)
  {
    return NotImplemented;
  }

  Status AddRectangles(const Rect *rects, INT count)
  {
    return NotImplemented;
  }

  Status AddRectangles(const RectF *rects, INT count)
  {
    return NotImplemented;
  }

  Status AddString(const WCHAR *string, INT length, const FontFamily *family, INT style, REAL emSize, const Rect &layoutRect, const StringFormat *format)
  {
    return NotImplemented;
  }

  Status AddString(const WCHAR *string, INT length, const FontFamily *family, INT style, REAL emSize, const PointF &origin, const StringFormat *format)
  {
    return NotImplemented;
  }

  Status AddString(const WCHAR *string, INT length, const FontFamily *family, INT style, REAL emSize, const Point &origin, const StringFormat *format)
  {
    return NotImplemented;
  }

  Status AddString(const WCHAR *string, INT length, const FontFamily *family, INT style, REAL emSize, const RectF &layoutRect, const StringFormat *format)
  {
    return NotImplemented;
  }

  Status ClearMarkers(VOID)
  {
    return NotImplemented;
  }

  GraphicsPath *Clone(VOID)
  {
    return NULL;
  }

  Status CloseAllFigures(VOID)
  {
    return NotImplemented;
  }

  Status CloseFigure(VOID)
  {
    return NotImplemented;
  }

  Status Flatten(const Matrix *matrix, REAL flatness)
  {
    return NotImplemented;
  }

  Status GetBounds(Rect *bounds, const Matrix *matrix, const Pen *pen)
  {
    return NotImplemented;
  }

  Status GetBounds(RectF *bounds, const Matrix *matrix, const Pen *pen)
  {
    return NotImplemented;
  }

  FillMode GetFillMode(VOID)
  {
    return FillModeAlternate;
  }

  Status GetLastPoint(PointF *lastPoint)
  {
    return NotImplemented;
  }

  Status GetLastStatus(VOID)
  {
    return NotImplemented;
  }

  Status GetPathData(PathData *pathData)
  {
    return NotImplemented;
  }

  Status GetPathPoints(Point *points, INT count)
  {
    return NotImplemented;
  }

  Status GetPathPoints(PointF *points, INT count)
  {
    return NotImplemented;
  }

  Status GetPathTypes(BYTE *types, INT count)
  {
    return NotImplemented;
  }

  INT GetPointCount(VOID)
  {
    return 0;
  }

  BOOL IsOutlineVisible(const Point &point, const Pen *pen, const Graphics *g)
  {
    return FALSE;
  }

  BOOL IsOutlineVisible(REAL x, REAL y, const Pen *pen, const Graphics *g)
  {
    return FALSE;
  }

  BOOL IsOutlineVisible(INT x, INT y, const Pen *pen, const Graphics *g)
  {
    return FALSE;
  }

  BOOL IsOutlineVisible(const PointF &point, const Pen *pen, const Graphics *g)
  {
    return FALSE;
  }

  BOOL IsVisible(REAL x, REAL y, const Graphics *g)
  {
    return FALSE;
  }

  BOOL IsVisible(const PointF &point, const Graphics *g)
  {
    return FALSE;
  }

  BOOL IsVisible(INT x, INT y, const Graphics *g)
  {
    return FALSE;
  }

  BOOL IsVisible(const Point &point, const Graphics *g)
  {
    return NotImplemented;
  }

  Status Outline(const Matrix *matrix, REAL flatness)
  {
    return NotImplemented;
  }

  Status Reset(VOID)
  {
    return NotImplemented;
  }

  Status Reverse(VOID)
  {
    return NotImplemented;
  }

  Status SetFillMode(FillMode fillmode)
  {
    return NotImplemented;
  }

  Status SetMarker(VOID)
  {
    return NotImplemented;
  }

  Status StartFigure(VOID)
  {
    return NotImplemented;
  }

  Status Transform(const Matrix *matrix)
  {
    return NotImplemented;
  }

  Status Warp(const PointF *destPoints, INT count, const RectF &srcRect, const Matrix *matrix, WarpMode warpMode, REAL flatness)
  {
    return NotImplemented;
  }

  Status Widen(const Pen *pen, const Matrix *matrix, REAL flatness)
  {
    return NotImplemented;
  }
};


class GraphicsPathIterator : public GdiplusBase
{
public:
  GraphicsPathIterator(GraphicsPath *path)
  {
  }

  INT CopyData(PointF *points, BYTE *types, INT startIndex, INT endIndex)
  {
    return 0;
  }

  INT Enumerate(PointF *points, BYTE *types, INT count)
  {
    return 0;
  }

  INT GetCount(VOID)
  {
    return 0;
  }

  Status GetLastStatus(VOID)
  {
    return NotImplemented;
  }

  INT GetSubpathCount(VOID)
  {
    return 0;
  }

  BOOL HasCurve(VOID)
  {
    return FALSE;
  }

  INT NextMarker(GraphicsPath *path)
  {
    return 0;
  }

  INT NextMarker(INT *startIndex, INT *endIndex)
  {
    return 0;
  }

  INT NextPathType(BYTE *pathType, INT *startIndex, INT *endIndex)
  {
    return 0;
  }

  INT NextSubpath(GraphicsPath *path, BOOL *isClosed)
  {
    return 0;
  }

  INT NextSubpath(INT *startIndex, INT *endIndex, BOOL *isClosed)
  {
    return 0;
  }

  VOID Rewind(VOID)
  {
  }
};


class PathGradientBrush : public Brush
{
public:
  PathGradientBrush(const Point *points, INT count, WrapMode wrapMode)
  {
  }

  PathGradientBrush(const PointF *points, INT count, WrapMode wrapMode)
  {
  }

  PathGradientBrush(const GraphicsPath *path)
  {
  }

  INT GetBlendCount(VOID)
  {
    return 0;
  }

  Status GetBlend(REAL *blendFactors, REAL *blendPositions, INT count)
  {
    return NotImplemented;
  }

  Status GetCenterColor(Color *color)
  {
    return NotImplemented;
  }

  Status GetCenterPoint(Point *point)
  {
    return NotImplemented;
  }

  Status GetCenterPoint(PointF *point)
  {
    return NotImplemented;
  }

  Status GetFocusScales(REAL *xScale, REAL *yScale)
  {
    return NotImplemented;
  }

  BOOL GetGammaCorrection(VOID)
  {
    return FALSE;
  }

  Status GetGraphicsPath(GraphicsPath *path)
  {
    return NotImplemented;
  }

  INT GetInterpolationColorCount(VOID)
  {
    return 0;
  }

  Status GetInterpolationColors(Color *presetColors, REAL *blendPositions, INT count)
  {
    return NotImplemented;
  }

  INT GetPointCount(VOID)
  {
    return 0;
  }

  Status GetRectangle(RectF *rect)
  {
    return NotImplemented;
  }

  Status GetRectangle(Rect *rect)
  {
    return NotImplemented;
  }

  INT GetSurroundColorCount(VOID)
  {
    return 0;
  }

  Status GetSurroundColors(Color *colors, INT *count)
  {
    return NotImplemented;
  }

  Status GetTransform(Matrix *matrix)
  {
    return NotImplemented;
  }

  WrapMode GetWrapMode(VOID)
  {
    return WrapModeTile;
  }

  Status MultiplyTransform(Matrix *matrix, MatrixOrder order)
  {
    return NotImplemented;
  }

  Status ResetTransform(VOID)
  {
    return NotImplemented;
  }

  Status RotateTransform(REAL angle, MatrixOrder order)
  {
    return NotImplemented;
  }

  Status ScaleTransform(REAL sx, REAL sy, MatrixOrder order)
  {
    return NotImplemented;
  }

  Status SetBlend(REAL *blendFactors, REAL *blendPositions, INT count)
  {
    return NotImplemented;
  }

  Status SetBlendBellShape(REAL focus, REAL scale)
  {
    return NotImplemented;
  }

  Status SetBlendTriangularShape(REAL focus, REAL scale)
  {
    return NotImplemented;
  }

  Status SetCenterColor(const Color &color)
  {
    return NotImplemented;
  }

  Status SetCenterPoint(const Point &point)
  {
    return NotImplemented;
  }

  Status SetCenterPoint(const PointF &point)
  {
    return NotImplemented;
  }

  Status SetFocusScales(REAL xScale, REAL yScale)
  {
    return NotImplemented;
  }

  Status SetGammaCorrection(BOOL useGammaCorrection)
  {
    return NotImplemented;
  }

  Status SetGraphicsPath(const GraphicsPath* path)
  {
    return NotImplemented;
  }

  Status SetInterpolationColors(const Color *presetColors, REAL *blendPositions, INT count)
  {
    return NotImplemented;
  }

  Status SetSurroundColors(const Color *colors, INT *count)
  {
    return NotImplemented;
  }

  Status SetTransform(const Matrix *matrix)
  {
    return NotImplemented;
  }

  Status SetWrapMode(WrapMode wrapMode)
  {
    return NotImplemented;
  }

  Status TranslateTransform(REAL dx, REAL dy, MatrixOrder order)
  {
    return NotImplemented;
  }
};

#endif /* _GDIPLUSPATH_H */
