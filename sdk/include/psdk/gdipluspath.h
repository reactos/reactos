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

class GraphicsPath : public GdiplusBase
{
    friend class Region;
    friend class CustomLineCap;
    friend class Graphics;

  public:
    GraphicsPath(const Point *points, const BYTE *types, INT count, FillMode fillMode) : nativePath(NULL)
    {
        lastStatus = DllExports::GdipCreatePath2I(points, types, count, fillMode, &nativePath);
    }

    GraphicsPath(FillMode fillMode = FillModeAlternate) : nativePath(NULL)
    {
        lastStatus = DllExports::GdipCreatePath(fillMode, &nativePath);
    }

    GraphicsPath(const PointF *points, const BYTE *types, INT count, FillMode fillMode = FillModeAlternate)
        : nativePath(NULL)
    {
        lastStatus = DllExports::GdipCreatePath2(points, types, count, fillMode, &nativePath);
    }

    ~GraphicsPath()
    {
        DllExports::GdipDeletePath(nativePath);
    }

    Status
    AddArc(const Rect &rect, REAL startAngle, REAL sweepAngle)
    {
        return AddArc(rect.X, rect.Y, rect.Width, rect.Height, startAngle, sweepAngle);
    }

    Status
    AddArc(const RectF &rect, REAL startAngle, REAL sweepAngle)
    {
        return AddArc(rect.X, rect.Y, rect.Width, rect.Height, startAngle, sweepAngle);
    }

    Status
    AddArc(INT x, INT y, INT width, INT height, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipAddPathArcI(nativePath, x, y, width, height, startAngle, sweepAngle));
    }

    Status
    AddArc(REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipAddPathArc(nativePath, x, y, width, height, startAngle, sweepAngle));
    }

    Status
    AddBezier(const Point &pt1, const Point &pt2, const Point &pt3, const Point &pt4)
    {
        return AddBezier(pt1.X, pt1.Y, pt2.X, pt2.Y, pt3.X, pt3.Y, pt4.X, pt4.Y);
    }

    Status
    AddBezier(REAL x1, REAL y1, REAL x2, REAL y2, REAL x3, REAL y3, REAL x4, REAL y4)
    {
        return SetStatus(DllExports::GdipAddPathBezier(nativePath, x1, y1, x2, y2, x3, y3, x4, y4));
    }

    Status
    AddBezier(const PointF &pt1, const PointF &pt2, const PointF &pt3, const PointF &pt4)
    {
        return AddBezier(pt1.X, pt1.Y, pt2.X, pt2.Y, pt3.X, pt3.Y, pt4.X, pt4.Y);
    }

    Status
    AddBezier(INT x1, INT y1, INT x2, INT y2, INT x3, INT y3, INT x4, INT y4)
    {
        return SetStatus(DllExports::GdipAddPathBezierI(nativePath, x1, y1, x2, y2, x3, y3, x4, y4));
    }

    Status
    AddBeziers(const Point *points, INT count)
    {
        return SetStatus(DllExports::GdipAddPathBeziersI(nativePath, points, count));
    }

    Status
    AddBeziers(const PointF *points, INT count)
    {
        return SetStatus(DllExports::GdipAddPathBeziers(nativePath, points, count));
    }

    Status
    AddClosedCurve(const Point *points, INT count)
    {
        return SetStatus(DllExports::GdipAddPathClosedCurveI(nativePath, points, count));
    }

    Status
    AddClosedCurve(const Point *points, INT count, REAL tension)
    {
        return SetStatus(DllExports::GdipAddPathClosedCurve2I(nativePath, points, count, tension));
    }

    Status
    AddClosedCurve(const PointF *points, INT count, REAL tension)
    {
        return SetStatus(DllExports::GdipAddPathClosedCurve2(nativePath, points, count, tension));
    }

    Status
    AddClosedCurve(const PointF *points, INT count)
    {
        return SetStatus(DllExports::GdipAddPathClosedCurve(nativePath, points, count));
    }

    Status
    AddCurve(const Point *points, INT count)
    {
        return SetStatus(DllExports::GdipAddPathCurveI(nativePath, points, count));
    }

    Status
    AddCurve(const PointF *points, INT count, REAL tension)
    {
        return SetStatus(DllExports::GdipAddPathCurve2(nativePath, points, count, tension));
    }

    Status
    AddCurve(const PointF *points, INT count)
    {
        return SetStatus(DllExports::GdipAddPathCurve(nativePath, points, count));
    }

    Status
    AddCurve(const Point *points, INT count, INT offset, INT numberOfSegments, REAL tension)
    {
        return SetStatus(DllExports::GdipAddPathCurve3I(nativePath, points, count, offset, numberOfSegments, tension));
    }

    Status
    AddCurve(const Point *points, INT count, REAL tension)
    {
        return SetStatus(DllExports::GdipAddPathCurve2I(nativePath, points, count, tension));
    }

    Status
    AddCurve(const PointF *points, INT count, INT offset, INT numberOfSegments, REAL tension)
    {
        return SetStatus(DllExports::GdipAddPathCurve3(nativePath, points, count, offset, numberOfSegments, tension));
    }

    Status
    AddEllipse(const Rect &rect)
    {
        return AddEllipse(rect.X, rect.Y, rect.Width, rect.Height);
    }

    Status
    AddEllipse(const RectF &rect)
    {
        return AddEllipse(rect.X, rect.Y, rect.Width, rect.Height);
    }

    Status
    AddEllipse(INT x, INT y, INT width, INT height)
    {
        return SetStatus(DllExports::GdipAddPathEllipseI(nativePath, x, y, width, height));
    }

    Status
    AddEllipse(REAL x, REAL y, REAL width, REAL height)
    {
        return SetStatus(DllExports::GdipAddPathEllipse(nativePath, x, y, width, height));
    }

    Status
    AddLine(const Point &pt1, const Point &pt2)
    {
        return AddLine(pt1.X, pt1.Y, pt2.X, pt2.Y);
    }

    Status
    AddLine(const PointF &pt1, const PointF &pt2)
    {
        return AddLine(pt1.X, pt1.Y, pt2.X, pt2.Y);
    }

    Status
    AddLine(REAL x1, REAL y1, REAL x2, REAL y2)
    {
        return SetStatus(DllExports::GdipAddPathLine(nativePath, x1, y1, x2, y2));
    }

    Status
    AddLine(INT x1, INT y1, INT x2, INT y2)
    {
        return SetStatus(DllExports::GdipAddPathLineI(nativePath, x1, y1, x2, y2));
    }

    Status
    AddLines(const Point *points, INT count)
    {
        return SetStatus(DllExports::GdipAddPathLine2I(nativePath, points, count));
    }

    Status
    AddLines(const PointF *points, INT count)
    {
        return SetStatus(DllExports::GdipAddPathLine2(nativePath, points, count));
    }

    Status
    AddPath(const GraphicsPath *addingPath, BOOL connect)
    {
        GpPath *nativePath2 = addingPath ? getNat(addingPath) : NULL;
        return SetStatus(DllExports::GdipAddPathPath(nativePath, nativePath2, connect));
    }

    Status
    AddPie(const Rect &rect, REAL startAngle, REAL sweepAngle)
    {
        return AddPie(rect.X, rect.Y, rect.Width, rect.Height, startAngle, sweepAngle);
    }

    Status
    AddPie(INT x, INT y, INT width, INT height, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipAddPathPieI(nativePath, x, y, width, height, startAngle, sweepAngle));
    }

    Status
    AddPie(REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipAddPathPie(nativePath, x, y, width, height, startAngle, sweepAngle));
    }

    Status
    AddPie(const RectF &rect, REAL startAngle, REAL sweepAngle)
    {
        return AddPie(rect.X, rect.Y, rect.Width, rect.Height, startAngle, sweepAngle);
    }

    Status
    AddPolygon(const Point *points, INT count)
    {
        return SetStatus(DllExports::GdipAddPathPolygonI(nativePath, points, count));
    }

    Status
    AddPolygon(const PointF *points, INT count)
    {
        return SetStatus(DllExports::GdipAddPathPolygon(nativePath, points, count));
    }

    Status
    AddRectangle(const Rect &rect)
    {
        return SetStatus(DllExports::GdipAddPathRectangleI(nativePath, rect.X, rect.Y, rect.Width, rect.Height));
    }

    Status
    AddRectangle(const RectF &rect)
    {
        return SetStatus(DllExports::GdipAddPathRectangle(nativePath, rect.X, rect.Y, rect.Width, rect.Height));
    }

    Status
    AddRectangles(const Rect *rects, INT count)
    {
        return SetStatus(DllExports::GdipAddPathRectanglesI(nativePath, rects, count));
    }

    Status
    AddRectangles(const RectF *rects, INT count)
    {
        return SetStatus(DllExports::GdipAddPathRectangles(nativePath, rects, count));
    }

    Status
    AddString(
        const WCHAR *string,
        INT length,
        const FontFamily *family,
        INT style,
        REAL emSize,
        const Rect &layoutRect,
        const StringFormat *format)
    {
        return SetStatus(DllExports::GdipAddPathStringI(
            nativePath, string, length, family ? getNat(family) : NULL, style, emSize, &layoutRect,
            format ? getNat(format) : NULL));
    }

    Status
    AddString(
        const WCHAR *string,
        INT length,
        const FontFamily *family,
        INT style,
        REAL emSize,
        const PointF &origin,
        const StringFormat *format)
    {
        RectF rect(origin.X, origin.Y, 0.0f, 0.0f);
        return SetStatus(DllExports::GdipAddPathString(
            nativePath, string, length, family ? getNat(family) : NULL, style, emSize, &rect,
            format ? getNat(format) : NULL));
    }

    Status
    AddString(
        const WCHAR *string,
        INT length,
        const FontFamily *family,
        INT style,
        REAL emSize,
        const Point &origin,
        const StringFormat *format)
    {
        Rect rect(origin.X, origin.Y, 0, 0);
        return SetStatus(DllExports::GdipAddPathStringI(
            nativePath, string, length, family ? getNat(family) : NULL, style, emSize, &rect,
            format ? getNat(format) : NULL));
    }

    Status
    AddString(
        const WCHAR *string,
        INT length,
        const FontFamily *family,
        INT style,
        REAL emSize,
        const RectF &layoutRect,
        const StringFormat *format)
    {
        return SetStatus(DllExports::GdipAddPathString(
            nativePath, string, length, family ? getNat(family) : NULL, style, emSize, &layoutRect,
            format ? getNat(format) : NULL));
    }

    Status
    ClearMarkers()
    {
        return SetStatus(DllExports::GdipClearPathMarkers(nativePath));
    }

    GraphicsPath *
    Clone()
    {
        GpPath *clonepath = NULL;
        SetStatus(DllExports::GdipClonePath(nativePath, &clonepath));
        if (lastStatus != Ok)
            return NULL;
        return new GraphicsPath(clonepath);
    }

    Status
    CloseAllFigures()
    {
        return SetStatus(DllExports::GdipClosePathFigures(nativePath));
    }

    Status
    CloseFigure()
    {
        return SetStatus(DllExports::GdipClosePathFigure(nativePath));
    }

    Status
    Flatten(const Matrix *matrix, REAL flatness)
    {
        GpMatrix *nativeMatrix = matrix ? getNat(matrix) : NULL;
        return SetStatus(DllExports::GdipFlattenPath(nativePath, nativeMatrix, flatness));
    }

    Status
    GetBounds(Rect *bounds, const Matrix *matrix, const Pen *pen)
    {
        return SetStatus(NotImplemented);
    }

    Status
    GetBounds(RectF *bounds, const Matrix *matrix, const Pen *pen)
    {
        return SetStatus(NotImplemented);
    }

    FillMode
    GetFillMode()
    {
        FillMode fillmode = FillModeAlternate;
        SetStatus(DllExports::GdipGetPathFillMode(nativePath, &fillmode));
        return fillmode;
    }

    Status
    GetLastPoint(PointF *lastPoint) const
    {
        return SetStatus(DllExports::GdipGetPathLastPoint(nativePath, lastPoint));
    }

    Status
    GetLastStatus() const
    {
        return lastStatus;
    }

    Status
    GetPathData(PathData *pathData)
    {
        return NotImplemented;
    }

    Status
    GetPathPoints(Point *points, INT count) const
    {
        return SetStatus(DllExports::GdipGetPathPointsI(nativePath, points, count));
    }

    Status
    GetPathPoints(PointF *points, INT count) const
    {
        return SetStatus(DllExports::GdipGetPathPoints(nativePath, points, count));
    }

    Status
    GetPathTypes(BYTE *types, INT count) const
    {
        return SetStatus(DllExports::GdipGetPathTypes(nativePath, types, count));
    }

    INT
    GetPointCount() const
    {
        INT count = 0;
        SetStatus(DllExports::GdipGetPointCount(nativePath, &count));
        return count;
    }

    BOOL
    IsOutlineVisible(const Point &point, const Pen *pen, const Graphics *g) const
    {
        return IsOutlineVisible(point.X, point.Y, pen, g);
    }

    BOOL
    IsOutlineVisible(REAL x, REAL y, const Pen *pen, const Graphics *g) const
    {
        GpGraphics *nativeGraphics = g ? getNat(g) : NULL;
        GpPen *nativePen = pen ? getNat(pen) : NULL;
        BOOL flag = FALSE;
        SetStatus(DllExports::GdipIsOutlineVisiblePathPoint(nativePath, x, y, nativePen, nativeGraphics, &flag));
        return flag;
    }

    BOOL
    IsOutlineVisible(INT x, INT y, const Pen *pen, const Graphics *g) const
    {
        GpGraphics *nativeGraphics = g ? getNat(g) : NULL;
        GpPen *nativePen = pen ? getNat(pen) : NULL;
        BOOL flag = FALSE;
        SetStatus(DllExports::GdipIsOutlineVisiblePathPointI(nativePath, x, y, nativePen, nativeGraphics, &flag));
        return flag;
    }

    BOOL
    IsOutlineVisible(const PointF &point, const Pen *pen, const Graphics *g) const
    {
        return IsOutlineVisible(point.X, point.Y, pen, g);
    }

    BOOL
    IsVisible(REAL x, REAL y, const Graphics *g) const
    {
        GpGraphics *nativeGraphics = g ? getNat(g) : NULL;
        BOOL flag = FALSE;
        SetStatus(DllExports::GdipIsVisiblePathPoint(nativePath, x, y, nativeGraphics, &flag));
        return flag;
    }

    BOOL
    IsVisible(const PointF &point, const Graphics *g) const
    {
        return IsVisible(point.X, point.Y, g);
    }

    BOOL
    IsVisible(INT x, INT y, const Graphics *g) const
    {
        GpGraphics *nativeGraphics = g ? getNat(g) : NULL;
        BOOL flag = FALSE;
        SetStatus(DllExports::GdipIsVisiblePathPointI(nativePath, x, y, nativeGraphics, &flag));
        return flag;
    }

    BOOL
    IsVisible(const Point &point, const Graphics *g) const
    {
        return IsVisible(point.X, point.Y, g);
    }

    Status
    Outline(const Matrix *matrix, REAL flatness)
    {
        GpMatrix *nativeMatrix = matrix ? getNat(matrix) : NULL;
        return SetStatus(DllExports::GdipWindingModeOutline(nativePath, nativeMatrix, flatness));
    }

    Status
    Reset()
    {
        return SetStatus(DllExports::GdipResetPath(nativePath));
    }

    Status
    Reverse()
    {
        return SetStatus(DllExports::GdipReversePath(nativePath));
    }

    Status
    SetFillMode(FillMode fillmode)
    {
        return SetStatus(DllExports::GdipSetPathFillMode(nativePath, fillmode));
    }

    Status
    SetMarker()
    {
        return SetStatus(DllExports::GdipSetPathMarker(nativePath));
    }

    Status
    StartFigure()
    {
        return SetStatus(DllExports::GdipStartPathFigure(nativePath));
    }

    Status
    Transform(const Matrix *matrix)
    {
        if (!matrix)
            return Ok;
        return SetStatus(DllExports::GdipTransformPath(nativePath, matrix->nativeMatrix));
    }

    Status
    Warp(
        const PointF *destPoints,
        INT count,
        const RectF &srcRect,
        const Matrix *matrix,
        WarpMode warpMode,
        REAL flatness)
    {
        GpMatrix *nativeMatrix = matrix ? getNat(matrix) : NULL;
        return SetStatus(DllExports::GdipWarpPath(
            nativePath, nativeMatrix, destPoints, count, srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, warpMode,
            flatness));
    }

    Status
    Widen(const Pen *pen, const Matrix *matrix, REAL flatness)
    {
        return SetStatus(NotImplemented);
    }

  protected:
    GpPath *nativePath;
    mutable Status lastStatus;

    GraphicsPath()
    {
    }

    GraphicsPath(GpPath *path) : nativePath(path), lastStatus(Ok)
    {
    }

    Status
    SetStatus(Status status) const
    {
        if (status != Ok)
            lastStatus = status;
        return status;
    }

    void
    SetNativePath(GpPath *path)
    {
        nativePath = path;
    }

  private:
    // GraphicsPath is not copyable
    GraphicsPath(const GraphicsPath &);
    GraphicsPath &
    operator=(const GraphicsPath &);

    // get native
    friend inline GpPath *&
    getNat(const GraphicsPath *path)
    {
        return const_cast<GraphicsPath *>(path)->nativePath;
    }
};

class GraphicsPathIterator : public GdiplusBase
{
  public:
    GraphicsPathIterator(GraphicsPath *path)
    {
        GpPathIterator *it = NULL;
        GpPath *nativePath = path ? getNat(path) : NULL;
        lastStatus = DllExports::GdipCreatePathIter(&it, nativePath);
        nativeIterator = it;
    }

    ~GraphicsPathIterator()
    {
        DllExports::GdipDeletePathIter(nativeIterator);
    }

    INT
    CopyData(PointF *points, BYTE *types, INT startIndex, INT endIndex)
    {
        INT resultCount;
        SetStatus(DllExports::GdipPathIterCopyData(nativeIterator, &resultCount, points, types, startIndex, endIndex));
        return resultCount;
    }

    INT
    Enumerate(PointF *points, BYTE *types, INT count)
    {
        INT resultCount;
        SetStatus(DllExports::GdipPathIterEnumerate(nativeIterator, &resultCount, points, types, count));
        return resultCount;
    }

    INT
    GetCount() const
    {
        INT resultCount;
        SetStatus(DllExports::GdipPathIterGetCount(nativeIterator, &resultCount));
        return resultCount;
    }

    Status
    GetLastStatus() const
    {
        return lastStatus;
    }

    INT
    GetSubpathCount() const
    {
        INT resultCount;
        SetStatus(DllExports::GdipPathIterGetSubpathCount(nativeIterator, &resultCount));
        return resultCount;
    }

    BOOL
    HasCurve() const
    {
        BOOL hasCurve;
        SetStatus(DllExports::GdipPathIterHasCurve(nativeIterator, &hasCurve));
        return hasCurve;
    }

    INT
    NextMarker(GraphicsPath *path)
    {
        INT resultCount;
        GpPath *nativePath = path ? getNat(path) : NULL;
        SetStatus(DllExports::GdipPathIterNextMarkerPath(nativeIterator, &resultCount, nativePath));
        return resultCount;
    }

    INT
    NextMarker(INT *startIndex, INT *endIndex)
    {
        INT resultCount;
        SetStatus(DllExports::GdipPathIterNextMarker(nativeIterator, &resultCount, startIndex, endIndex));
        return resultCount;
    }

    INT
    NextPathType(BYTE *pathType, INT *startIndex, INT *endIndex)
    {
        INT resultCount;
        SetStatus(DllExports::GdipPathIterNextPathType(nativeIterator, &resultCount, pathType, startIndex, endIndex));
        return resultCount;
    }

    INT
    NextSubpath(GraphicsPath *path, BOOL *isClosed)
    {
        GpPath *nativePath = path ? getNat(path) : NULL;
        INT resultCount;
        SetStatus(DllExports::GdipPathIterNextSubpathPath(nativeIterator, &resultCount, nativePath, isClosed));
        return resultCount;
    }

    INT
    NextSubpath(INT *startIndex, INT *endIndex, BOOL *isClosed)
    {
        INT resultCount;
        SetStatus(DllExports::GdipPathIterNextSubpath(nativeIterator, &resultCount, startIndex, endIndex, isClosed));
        return resultCount;
    }

    VOID
    Rewind()
    {
        SetStatus(DllExports::GdipPathIterRewind(nativeIterator));
    }

  protected:
    GpPathIterator *nativeIterator;
    mutable Status lastStatus;

    Status
    SetStatus(Status status) const
    {
        if (status != Ok)
            lastStatus = status;
        return status;
    }
};

class PathGradientBrush : public Brush
{
  public:
    friend class Pen;

    PathGradientBrush(const Point *points, INT count, WrapMode wrapMode = WrapModeClamp)
    {
        GpPathGradient *brush = NULL;
        lastStatus = DllExports::GdipCreatePathGradientI(points, count, wrapMode, &brush);
        SetNativeBrush(brush);
    }

    PathGradientBrush(const PointF *points, INT count, WrapMode wrapMode = WrapModeClamp)
    {
        GpPathGradient *brush = NULL;
        lastStatus = DllExports::GdipCreatePathGradient(points, count, wrapMode, &brush);
        SetNativeBrush(brush);
    }

    PathGradientBrush(const GraphicsPath *path)
    {
        GpPathGradient *brush = NULL;
        lastStatus = DllExports::GdipCreatePathGradientFromPath(getNat(path), &brush);
        SetNativeBrush(brush);
    }

    INT
    GetBlendCount() const
    {
        INT count = 0;
        SetStatus(DllExports::GdipGetPathGradientBlendCount(GetNativeGradient(), &count));
        return count;
    }

    Status
    GetBlend(REAL *blendFactors, REAL *blendPositions, INT count) const
    {
        return SetStatus(
            DllExports::GdipGetPathGradientBlend(GetNativeGradient(), blendFactors, blendPositions, count));
    }

    Status
    GetCenterColor(Color *color) const
    {
        if (color != NULL)
            return SetStatus(InvalidParameter);

        ARGB argb;
        SetStatus(DllExports::GdipGetPathGradientCenterColor(GetNativeGradient(), &argb));
        color->SetValue(argb);
        return GetLastStatus();
    }

    Status
    GetCenterPoint(Point *point) const
    {
        return SetStatus(DllExports::GdipGetPathGradientCenterPointI(GetNativeGradient(), point));
    }

    Status
    GetCenterPoint(PointF *point) const
    {
        return SetStatus(DllExports::GdipGetPathGradientCenterPoint(GetNativeGradient(), point));
    }

    Status
    GetFocusScales(REAL *xScale, REAL *yScale) const
    {
        return SetStatus(DllExports::GdipGetPathGradientFocusScales(GetNativeGradient(), xScale, yScale));
    }

    BOOL
    GetGammaCorrection() const
    {
        BOOL useGammaCorrection;
        SetStatus(DllExports::GdipGetPathGradientGammaCorrection(GetNativeGradient(), &useGammaCorrection));
        return useGammaCorrection;
    }

    Status
    GetGraphicsPath(GraphicsPath *path) const
    {
        if (!path)
            return SetStatus(InvalidParameter);

        return SetStatus(DllExports::GdipGetPathGradientPath(GetNativeGradient(), getNat(path)));
    }

    INT
    GetInterpolationColorCount() const
    {
        INT count = 0;
        SetStatus(DllExports::GdipGetPathGradientPresetBlendCount(GetNativeGradient(), &count));
        return count;
    }

    Status
    GetInterpolationColors(Color *presetColors, REAL *blendPositions, INT count) const
    {
        return NotImplemented;
    }

    INT
    GetPointCount() const
    {
        INT count;
        SetStatus(DllExports::GdipGetPathGradientPointCount(GetNativeGradient(), &count));
        return count;
    }

    Status
    GetRectangle(RectF *rect) const
    {
        return SetStatus(DllExports::GdipGetPathGradientRect(GetNativeGradient(), rect));
    }

    Status
    GetRectangle(Rect *rect) const
    {
        return SetStatus(DllExports::GdipGetPathGradientRectI(GetNativeGradient(), rect));
    }

    INT
    GetSurroundColorCount() const
    {
        INT count;
        SetStatus(DllExports::GdipGetPathGradientSurroundColorCount(GetNativeGradient(), &count));
        return count;
    }

    Status
    GetSurroundColors(Color *colors, INT *count) const
    {
        return NotImplemented;
    }

    Status
    GetTransform(Matrix *matrix) const
    {
        return SetStatus(DllExports::GdipGetPathGradientTransform(GetNativeGradient(), getNat(matrix)));
    }

    WrapMode
    GetWrapMode() const
    {
        WrapMode wrapMode;
        SetStatus(DllExports::GdipGetPathGradientWrapMode(GetNativeGradient(), &wrapMode));
        return wrapMode;
    }

    Status
    MultiplyTransform(Matrix *matrix, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipMultiplyPathGradientTransform(GetNativeGradient(), getNat(matrix), order));
    }

    Status
    ResetTransform()
    {
        return SetStatus(DllExports::GdipResetPathGradientTransform(GetNativeGradient()));
    }

    Status
    RotateTransform(REAL angle, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipRotatePathGradientTransform(GetNativeGradient(), angle, order));
    }

    Status
    ScaleTransform(REAL sx, REAL sy, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipScalePathGradientTransform(GetNativeGradient(), sx, sy, order));
    }

    Status
    SetBlend(REAL *blendFactors, REAL *blendPositions, INT count)
    {
        return SetStatus(
            DllExports::GdipSetPathGradientBlend(GetNativeGradient(), blendFactors, blendPositions, count));
    }

    Status
    SetBlendBellShape(REAL focus, REAL scale)
    {
        return SetStatus(DllExports::GdipSetPathGradientSigmaBlend(GetNativeGradient(), focus, scale));
    }

    Status
    SetBlendTriangularShape(REAL focus, REAL scale = 1.0f)
    {
        return SetStatus(DllExports::GdipSetPathGradientLinearBlend(GetNativeGradient(), focus, scale));
    }

    Status
    SetCenterColor(const Color &color)
    {
        return SetStatus(DllExports::GdipSetPathGradientCenterColor(GetNativeGradient(), color.GetValue()));
    }

    Status
    SetCenterPoint(const Point &point)
    {
        return SetStatus(DllExports::GdipSetPathGradientCenterPointI(GetNativeGradient(), const_cast<Point *>(&point)));
    }

    Status
    SetCenterPoint(const PointF &point)
    {
        return SetStatus(DllExports::GdipSetPathGradientCenterPoint(GetNativeGradient(), const_cast<PointF *>(&point)));
    }

    Status
    SetFocusScales(REAL xScale, REAL yScale)
    {
        return SetStatus(DllExports::GdipSetPathGradientFocusScales(GetNativeGradient(), xScale, yScale));
    }

    Status
    SetGammaCorrection(BOOL useGammaCorrection)
    {
        return SetStatus(DllExports::GdipSetPathGradientGammaCorrection(GetNativeGradient(), useGammaCorrection));
    }

    Status
    SetGraphicsPath(const GraphicsPath *path)
    {
        if (!path)
            return SetStatus(InvalidParameter);
        return SetStatus(DllExports::GdipSetPathGradientPath(GetNativeGradient(), getNat(path)));
    }

    Status
    SetInterpolationColors(const Color *presetColors, REAL *blendPositions, INT count)
    {
        return NotImplemented;
    }

    Status
    SetSurroundColors(const Color *colors, INT *count)
    {
        return NotImplemented;
    }

    Status
    SetTransform(const Matrix *matrix)
    {
        return SetStatus(DllExports::GdipSetPathGradientTransform(GetNativeGradient(), getNat(matrix)));
    }

    Status
    SetWrapMode(WrapMode wrapMode)
    {
        return SetStatus(DllExports::GdipSetPathGradientWrapMode(GetNativeGradient(), wrapMode));
    }

    Status
    TranslateTransform(REAL dx, REAL dy, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipTranslatePathGradientTransform(GetNativeGradient(), dx, dy, order));
    }

  protected:
    GpPathGradient *
    GetNativeGradient() const
    {
        return static_cast<GpPathGradient *>(nativeBrush);
    }

    PathGradientBrush()
    {
    }
};

#endif /* _GDIPLUSPATH_H */
