#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateRegion(GpRegion **region)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateRegionRect(GDIPCONST GpRectF *rect,
  GpRegion **region)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateRegionRectI(GDIPCONST GpRect *rect,
  GpRegion **region)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateRegionPath(GpPath *path,
  GpRegion **region)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateRegionRgnData(GDIPCONST BYTE *regionData,
  INT size,
  GpRegion **region)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateRegionHrgn(HRGN hRgn,
  GpRegion **region)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCloneRegion(GpRegion *region,
  GpRegion **cloneRegion)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDeleteRegion(GpRegion *region)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetInfinite(GpRegion *region)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetEmpty(GpRegion *region)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCombineRegionRect(GpRegion *region,
  GDIPCONST GpRectF *rect,
  CombineMode combineMode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCombineRegionRectI(GpRegion *region,
  GDIPCONST GpRect *rect,
  CombineMode combineMode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCombineRegionPath(GpRegion *region,
  GpPath *path,
  CombineMode combineMode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCombineRegionRegion(GpRegion *region,
  GpRegion *region2,
  CombineMode combineMode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipTranslateRegion(GpRegion *region,
  REAL dx,
  REAL dy)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipTranslateRegionI(GpRegion *region,
  INT dx,
  INT dy)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipTransformRegion(GpRegion *region,
  GpMatrix *matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetRegionBounds(GpRegion *region,
  GpGraphics *graphics,
  GpRectF *rect)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetRegionBoundsI(GpRegion *region,
  GpGraphics *graphics,
  GpRect *rect)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetRegionHRgn(GpRegion *region,
  GpGraphics *graphics,
  HRGN *hRgn)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipIsEmptyRegion(GpRegion *region,
  GpGraphics *graphics,
  BOOL *result)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipIsInfiniteRegion(GpRegion *region,
  GpGraphics *graphics,
  BOOL *result)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipIsEqualRegion(GpRegion *region,
  GpRegion *region2,
  GpGraphics *graphics,
  BOOL *result)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetRegionDataSize(GpRegion *region,
  UINT * bufferSize)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetRegionData(GpRegion *region,
  BYTE * buffer,
  UINT bufferSize,
  UINT * sizeFilled)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipIsVisibleRegionPoint(GpRegion *region,
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
GdipIsVisibleRegionPointI(GpRegion *region,
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
GdipIsVisibleRegionRect(GpRegion *region,
  REAL x,
  REAL y,
  REAL width,
  REAL height,
  GpGraphics *graphics,
  BOOL *result)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipIsVisibleRegionRectI(GpRegion *region,
  INT x,
  INT y,
  INT width,
  INT height,
  GpGraphics *graphics,
  BOOL *result)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetRegionScansCount(GpRegion *region,
  UINT* count,
  GpMatrix* matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetRegionScans(GpRegion *region,
  GpRectF* rects,
  INT* count,
  GpMatrix* matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetRegionScansI(GpRegion *region,
  GpRect* rects,
  INT* count,
  GpMatrix* matrix)
{
  return NotImplemented;
}
