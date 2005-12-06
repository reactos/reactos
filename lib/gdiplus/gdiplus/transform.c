#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetWorldTransform(GpGraphics *graphics,
  GpMatrix *matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipResetWorldTransform(GpGraphics *graphics)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipMultiplyWorldTransform(GpGraphics *graphics,
  GDIPCONST GpMatrix *matrix,
  GpMatrixOrder order)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipTranslateWorldTransform(GpGraphics *graphics,
  REAL dx,
  REAL dy,
  GpMatrixOrder order)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipScaleWorldTransform(GpGraphics *graphics,
  REAL sx,
  REAL sy,
  GpMatrixOrder order)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipRotateWorldTransform(GpGraphics *graphics,
  REAL angle,
  GpMatrixOrder order)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetWorldTransform(GpGraphics *graphics,
  GpMatrix *matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipResetPageTransform(GpGraphics *graphics)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipTransformPoints(GpGraphics *graphics,
  GpCoordinateSpace destSpace,
  GpCoordinateSpace srcSpace,
  GpPointF *points,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipTransformPointsI(GpGraphics *graphics,
  GpCoordinateSpace destSpace,
  GpCoordinateSpace srcSpace,
  GpPoint *points,
  INT count)
{
  return NotImplemented;
}
