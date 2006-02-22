#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreatePath2(GDIPCONST GpPointF* points,
  GDIPCONST BYTE* types,
  INT count,
  GpFillMode fillMode,
  GpPath **path)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreatePath2I(GDIPCONST GpPoint* points,
  GDIPCONST BYTE* types,
  INT count,
  GpFillMode fillMode,
  GpPath **path)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipClonePath(GpPath* path,
  GpPath **clonePath)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDeletePath(GpPath* path)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipResetPath(GpPath* path)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPointCount(GpPath* path,
  INT* count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPathTypes(GpPath* path,
  BYTE* types,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPathPoints(GpPath* path,
  GpPointF* points,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPathPointsI(GpPath* path,
  GpPoint* points,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPathFillMode(GpPath *path,
  GpFillMode *fillmode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetPathFillMode(GpPath *path,
  GpFillMode fillmode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPathData(GpPath *path,
  GpPathData* pathData)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipStartPathFigure(GpPath *path)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipClosePathFigure(GpPath *path)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipClosePathFigures(GpPath *path)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetPathMarker(GpPath* path)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipClearPathMarkers(GpPath* path)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipReversePath(GpPath* path)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPathLastPoint(GpPath* path,
  GpPointF* lastPoint)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathLine(GpPath *path,
  REAL x1,
  REAL y1,
  REAL x2,
  REAL y2)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathLine2(GpPath *path,
  GDIPCONST GpPointF *points,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathArc(GpPath *path,
  REAL x,
  REAL y,
  REAL width,
  REAL height,
  REAL startAngle,
  REAL sweepAngle)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathBezier(GpPath *path,
  REAL x1,
  REAL y1,
  REAL x2,
  REAL y2,
  REAL x3,
  REAL y3,
  REAL x4,
  REAL y4)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathBeziers(GpPath *path,
  GDIPCONST GpPointF *points,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathCurve(GpPath *path,
  GDIPCONST GpPointF *points,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathCurve2(GpPath *path,
  GDIPCONST GpPointF *points,
  INT count,
  REAL tension)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathCurve3(GpPath *path,
  GDIPCONST GpPointF *points,
  INT count,
  INT offset,
  INT numberOfSegments,
  REAL tension)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathClosedCurve(GpPath *path,
  GDIPCONST GpPointF *points,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathClosedCurve2(GpPath *path,
  GDIPCONST GpPointF *points,
  INT count,
  REAL tension)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathRectangle(GpPath *path,
  REAL x,
  REAL y,
  REAL width,
  REAL height)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathRectangles(GpPath *path,
  GDIPCONST GpRectF *rects,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathEllipse(GpPath *path,
  REAL x,
  REAL y,
  REAL width,
  REAL height)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathPie(GpPath *path,
  REAL x,
  REAL y,
  REAL width,
  REAL height,
  REAL startAngle,
  REAL sweepAngle)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathPolygon(GpPath *path,
  GDIPCONST GpPointF *points,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathPath(GpPath *path,
  GDIPCONST GpPath* addingPath,
  BOOL connect)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathString(GpPath *path,
  GDIPCONST WCHAR *string,
  INT length,
  GDIPCONST GpFontFamily *family,
  INT style,
  REAL emSize,
  GDIPCONST RectF *layoutRect,
  GDIPCONST GpStringFormat *format)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathStringI(GpPath *path,
  GDIPCONST WCHAR *string,
  INT length,
  GDIPCONST GpFontFamily *family,
  INT style,
  REAL emSize,
  GDIPCONST Rect *layoutRect,
  GDIPCONST GpStringFormat *format)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathLineI(GpPath *path,
  INT x1,
  INT y1,
  INT x2,
  INT y2)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathLine2I(GpPath *path,
  GDIPCONST GpPoint *points,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathArcI(GpPath *path,
  INT x,
  INT y,
  INT width,
  INT height,
  REAL startAngle,
  REAL sweepAngle)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathBezierI(GpPath *path,
  INT x1,
  INT y1,
  INT x2,
  INT y2,
  INT x3,
  INT y3,
  INT x4,
  INT y4)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathBeziersI(GpPath *path,
  GDIPCONST GpPoint *points,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathCurveI(GpPath *path,
  GDIPCONST GpPoint *points,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathCurve2I(GpPath *path,
  GDIPCONST GpPoint *points,
  INT count,
  REAL tension)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathCurve3I(GpPath *path,
  GDIPCONST GpPoint *points,
  INT count,
  INT offset,
  INT numberOfSegments,
  REAL tension)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathClosedCurveI(GpPath *path,
  GDIPCONST GpPoint *points,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathClosedCurve2I(GpPath *path,
  GDIPCONST GpPoint *points,
  INT count,
  REAL tension)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathRectangleI(GpPath *path,
  INT x,
  INT y,
  INT width,
  INT height)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathRectanglesI(GpPath *path,
  GDIPCONST GpRect *rects,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathEllipseI(GpPath *path,
  INT x,
  INT y,
  INT width,
  INT height)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathPieI(GpPath *path,
  INT x,
  INT y,
  INT width,
  INT height,
  REAL startAngle,
  REAL sweepAngle)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipAddPathPolygonI(GpPath *path,
  GDIPCONST GpPoint *points,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipFlattenPath(GpPath *path,
  GpMatrix* matrix,
  REAL flatness)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipWindingModeOutline(GpPath *path,
  GpMatrix *matrix,
  REAL flatness)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipWidenPath(GpPath *nativePath,
  GpPen *pen,
  GpMatrix *matrix,
  REAL flatness)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipWarpPath(GpPath *path,
  GpMatrix* matrix,
  GDIPCONST GpPointF *points,
  INT count,
  REAL srcx,
  REAL srcy,
  REAL srcwidth,
  REAL srcheight,
  WarpMode warpMode,
  REAL flatness)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipTransformPath(GpPath* path,
  GpMatrix* matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPathWorldBounds(GpPath* path,
  GpRectF* bounds,
  GDIPCONST GpMatrix *matrix,
  GDIPCONST GpPen *pen)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPathWorldBoundsI(GpPath* path,
  GpRect* bounds,
  GDIPCONST GpMatrix *matrix,
  GDIPCONST GpPen *pen)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipIsVisiblePathPoint(GpPath* path,
  REAL x,
  REAL y,
  GpGraphics *graphics,
  BOOL *result)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipIsVisiblePathPointI(GpPath* path,
  INT x,
  INT y,
  GpGraphics *graphics,
  BOOL *result)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipIsOutlineVisiblePathPoint(GpPath* path,
  REAL x,
  REAL y,
  GpPen *pen,
  GpGraphics *graphics,
  BOOL *result)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipIsOutlineVisiblePathPointI(GpPath* path,
  INT x,
  INT y,
  GpPen *pen,
  GpGraphics *graphics,
  BOOL *result)
{
  return NotImplemented;
}
