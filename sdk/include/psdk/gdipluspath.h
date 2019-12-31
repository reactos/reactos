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

class FontFamily;
class Graphics;

class GraphicsPath : public GdiplusBase
{
    friend class Region;

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
        GpPath *nativePath2 = NULL;
        if (addingPath)
            nativePath2 = addingPath->nativePath;

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
        return SetStatus(NotImplemented);
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
        return SetStatus(NotImplemented);
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
        return SetStatus(NotImplemented);
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
        return SetStatus(NotImplemented);
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
        GpMatrix *nativeMatrix = NULL;
        if (matrix)
            nativeMatrix = matrix->nativeMatrix;

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
    GetPathPoints(Point *points, INT count)
    {
        return NotImplemented;
    }

    Status
    GetPathPoints(PointF *points, INT count)
    {
        return NotImplemented;
    }

    Status
    GetPathTypes(BYTE *types, INT count)
    {
        return NotImplemented;
    }

    INT
    GetPointCount()
    {
        return 0;
    }

    BOOL
    IsOutlineVisible(const Point &point, const Pen *pen, const Graphics *g)
    {
        return FALSE;
    }

    BOOL
    IsOutlineVisible(REAL x, REAL y, const Pen *pen, const Graphics *g)
    {
        return FALSE;
    }

    BOOL
    IsOutlineVisible(INT x, INT y, const Pen *pen, const Graphics *g)
    {
        return FALSE;
    }

    BOOL
    IsOutlineVisible(const PointF &point, const Pen *pen, const Graphics *g)
    {
        return FALSE;
    }

    BOOL
    IsVisible(REAL x, REAL y, const Graphics *g)
    {
        return FALSE;
    }

    BOOL
    IsVisible(const PointF &point, const Graphics *g)
    {
        return IsVisible(point.X, point.Y, g);
    }

    BOOL
    IsVisible(INT x, INT y, const Graphics *g)
    {
        return FALSE;
    }

    BOOL
    IsVisible(const Point &point, const Graphics *g)
    {
        return IsVisible(point.X, point.Y, g);
    }

    Status
    Outline(const Matrix *matrix, REAL flatness)
    {
        return NotImplemented;
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
        GpMatrix *nativeMatrix = NULL;
        if (matrix)
            nativeMatrix = matrix->nativeMatrix;

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
};

class GraphicsPathIterator : public GdiplusBase
{
  public:
    GraphicsPathIterator(GraphicsPath *path)
    {
    }

    INT
    CopyData(PointF *points, BYTE *types, INT startIndex, INT endIndex)
    {
        return 0;
    }

    INT
    Enumerate(PointF *points, BYTE *types, INT count)
    {
        return 0;
    }

    INT
    GetCount()
    {
        return 0;
    }

    Status
    GetLastStatus() const
    {
        return NotImplemented;
    }

    INT
    GetSubpathCount() const
    {
        return 0;
    }

    BOOL
    HasCurve() const
    {
        return FALSE;
    }

    INT
    NextMarker(GraphicsPath *path)
    {
        return 0;
    }

    INT
    NextMarker(INT *startIndex, INT *endIndex)
    {
        return 0;
    }

    INT
    NextPathType(BYTE *pathType, INT *startIndex, INT *endIndex)
    {
        return 0;
    }

    INT
    NextSubpath(GraphicsPath *path, BOOL *isClosed)
    {
        return 0;
    }

    INT
    NextSubpath(INT *startIndex, INT *endIndex, BOOL *isClosed)
    {
        return 0;
    }

    VOID
    Rewind()
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

    INT
    GetBlendCount()
    {
        return 0;
    }

    Status
    GetBlend(REAL *blendFactors, REAL *blendPositions, INT count)
    {
        return NotImplemented;
    }

    Status
    GetCenterColor(Color *color)
    {
        return NotImplemented;
    }

    Status
    GetCenterPoint(Point *point)
    {
        return NotImplemented;
    }

    Status
    GetCenterPoint(PointF *point)
    {
        return NotImplemented;
    }

    Status
    GetFocusScales(REAL *xScale, REAL *yScale)
    {
        return NotImplemented;
    }

    BOOL
    GetGammaCorrection()
    {
        return FALSE;
    }

    Status
    GetGraphicsPath(GraphicsPath *path)
    {
        return NotImplemented;
    }

    INT
    GetInterpolationColorCount()
    {
        return 0;
    }

    Status
    GetInterpolationColors(Color *presetColors, REAL *blendPositions, INT count)
    {
        return NotImplemented;
    }

    INT
    GetPointCount()
    {
        return 0;
    }

    Status
    GetRectangle(RectF *rect)
    {
        return NotImplemented;
    }

    Status
    GetRectangle(Rect *rect)
    {
        return NotImplemented;
    }

    INT
    GetSurroundColorCount()
    {
        return 0;
    }

    Status
    GetSurroundColors(Color *colors, INT *count)
    {
        return NotImplemented;
    }

    Status
    GetTransform(Matrix *matrix)
    {
        return NotImplemented;
    }

    WrapMode
    GetWrapMode()
    {
        return WrapModeTile;
    }

    Status
    MultiplyTransform(Matrix *matrix, MatrixOrder order)
    {
        return NotImplemented;
    }

    Status
    ResetTransform()
    {
        return NotImplemented;
    }

    Status
    RotateTransform(REAL angle, MatrixOrder order)
    {
        return NotImplemented;
    }

    Status
    ScaleTransform(REAL sx, REAL sy, MatrixOrder order)
    {
        return NotImplemented;
    }

    Status
    SetBlend(REAL *blendFactors, REAL *blendPositions, INT count)
    {
        return NotImplemented;
    }

    Status
    SetBlendBellShape(REAL focus, REAL scale)
    {
        return NotImplemented;
    }

    Status
    SetBlendTriangularShape(REAL focus, REAL scale)
    {
        return NotImplemented;
    }

    Status
    SetCenterColor(const Color &color)
    {
        return NotImplemented;
    }

    Status
    SetCenterPoint(const Point &point)
    {
        return NotImplemented;
    }

    Status
    SetCenterPoint(const PointF &point)
    {
        return NotImplemented;
    }

    Status
    SetFocusScales(REAL xScale, REAL yScale)
    {
        return NotImplemented;
    }

    Status
    SetGammaCorrection(BOOL useGammaCorrection)
    {
        return NotImplemented;
    }

    Status
    SetGraphicsPath(const GraphicsPath *path)
    {
        return NotImplemented;
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
        return NotImplemented;
    }

    Status
    SetWrapMode(WrapMode wrapMode)
    {
        return NotImplemented;
    }

    Status
    TranslateTransform(REAL dx, REAL dy, MatrixOrder order)
    {
        return NotImplemented;
    }
};

#endif /* _GDIPLUSPATH_H */
