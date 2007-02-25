#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateMatrix(GpMatrix **matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateMatrix2(REAL m11,
  REAL m12,
  REAL m21,
  REAL m22,
  REAL dx,
  REAL dy,
  GpMatrix **matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateMatrix3(GDIPCONST GpRectF *rect,
  GDIPCONST GpPointF *dstplg,
  GpMatrix **matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateMatrix3I(GDIPCONST GpRect *rect,
  GDIPCONST GpPoint *dstplg,
  GpMatrix **matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCloneMatrix(GpMatrix *matrix,
  GpMatrix **cloneMatrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDeleteMatrix(GpMatrix *matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetMatrixElements(GpMatrix *matrix,
  REAL m11,
  REAL m12,
  REAL m21,
  REAL m22,
  REAL dx,
  REAL dy)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipMultiplyMatrix(GpMatrix *matrix,
  GpMatrix* matrix2,
  GpMatrixOrder order)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipTranslateMatrix(GpMatrix *matrix,
  REAL offsetX,
  REAL offsetY,
  GpMatrixOrder order)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipScaleMatrix(GpMatrix *matrix,
  REAL scaleX,
  REAL scaleY,
  GpMatrixOrder order)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipRotateMatrix(GpMatrix *matrix,
  REAL angle,
  GpMatrixOrder order)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipShearMatrix(GpMatrix *matrix,
  REAL shearX,
  REAL shearY,
  GpMatrixOrder order)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipInvertMatrix(GpMatrix *matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipTransformMatrixPoints(GpMatrix *matrix,
  GpPointF *pts,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipTransformMatrixPointsI(GpMatrix *matrix,
  GpPoint *pts,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipVectorTransformMatrixPoints(GpMatrix *matrix,
  GpPointF *pts,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipVectorTransformMatrixPointsI(GpMatrix *matrix,
  GpPoint *pts,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetMatrixElements(GDIPCONST GpMatrix *matrix,
  REAL *matrixOut)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipIsMatrixInvertible(GDIPCONST GpMatrix *matrix,
  BOOL *result)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipIsMatrixIdentity(GDIPCONST GpMatrix *matrix,
  BOOL *result)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipIsMatrixEqual(GDIPCONST GpMatrix *matrix,
  GDIPCONST GpMatrix *matrix2,
  BOOL *result)
{
  return NotImplemented;
}
