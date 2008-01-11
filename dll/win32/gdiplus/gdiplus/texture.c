#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateTexture(GpImage *image,
  GpWrapMode wrapmode,
  GpTexture **texture)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateTexture2(GpImage *image,
  GpWrapMode wrapmode,
  REAL x,
  REAL y,
  REAL width,
  REAL height,
  GpTexture **texture)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateTextureIA(GpImage *image,
  GDIPCONST GpImageAttributes *imageAttributes,
  REAL x,
  REAL y,
  REAL width,
  REAL height,
  GpTexture **texture)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateTexture2I(GpImage *image,
  GpWrapMode wrapmode,
  INT x,
  INT y,
  INT width,
  INT height,
  GpTexture **texture)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateTextureIAI(GpImage *image,
  GDIPCONST GpImageAttributes *imageAttributes,
  INT x,
  INT y,
  INT width,
  INT height,
  GpTexture **texture)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetTextureTransform(GpTexture *brush,
  GpMatrix *matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetTextureTransform(GpTexture *brush,
  GDIPCONST GpMatrix *matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipResetTextureTransform(GpTexture* brush)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipMultiplyTextureTransform(GpTexture* brush,
  GDIPCONST GpMatrix *matrix,
  GpMatrixOrder order)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipTranslateTextureTransform(GpTexture* brush,
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
GdipScaleTextureTransform(GpTexture* brush,
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
GdipRotateTextureTransform(GpTexture* brush,
  REAL angle,
  GpMatrixOrder order)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetTextureWrapMode(GpTexture *brush,
  GpWrapMode wrapmode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetTextureWrapMode(GpTexture *brush,
  GpWrapMode *wrapmode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetTextureImage(GpTexture *brush,
  GpImage **image)
{
  return NotImplemented;
}
