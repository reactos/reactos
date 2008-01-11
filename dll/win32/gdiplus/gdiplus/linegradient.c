#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateLineBrush(GDIPCONST GpPointF* point1,
  GDIPCONST GpPointF* point2,
  ARGB color1,
  ARGB color2,
  GpWrapMode wrapMode,
  GpLineGradient **lineGradient)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateLineBrushI(GDIPCONST GpPoint* point1,
  GDIPCONST GpPoint* point2,
  ARGB color1,
  ARGB color2,
  GpWrapMode wrapMode,
  GpLineGradient **lineGradient)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateLineBrushFromRect(GDIPCONST GpRectF* rect,
  ARGB color1,
  ARGB color2,
  LinearGradientMode mode,
  GpWrapMode wrapMode,
  GpLineGradient **lineGradient)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateLineBrushFromRectI(GDIPCONST GpRect* rect,
  ARGB color1,
  ARGB color2,
  LinearGradientMode mode,
  GpWrapMode wrapMode,
  GpLineGradient **lineGradient)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateLineBrushFromRectWithAngle(GDIPCONST GpRectF* rect,
  ARGB color1,
  ARGB color2,
  REAL angle,
  BOOL isAngleScalable,
  GpWrapMode wrapMode,
  GpLineGradient **lineGradient)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateLineBrushFromRectWithAngleI(GDIPCONST GpRect* rect,
  ARGB color1,
  ARGB color2,
  REAL angle,
  BOOL isAngleScalable,
  GpWrapMode wrapMode,
  GpLineGradient **lineGradient)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetLineColors(GpLineGradient *brush,
  ARGB color1,
  ARGB color2)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetLineColors(GpLineGradient *brush,
  ARGB* colors)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetLineRect(GpLineGradient *brush,
  GpRectF *rect)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetLineRectI(GpLineGradient *brush,
  GpRect *rect)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetLineGammaCorrection(GpLineGradient *brush,
  BOOL useGammaCorrection)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetLineGammaCorrection(GpLineGradient *brush,
  BOOL *useGammaCorrection)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetLineBlendCount(GpLineGradient *brush,
  INT *count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetLineBlend(GpLineGradient *brush,
  REAL *blend,
  REAL* positions,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetLineBlend(GpLineGradient *brush,
  GDIPCONST REAL *blend,
  GDIPCONST REAL* positions,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetLinePresetBlendCount(GpLineGradient *brush,
  INT *count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetLinePresetBlend(GpLineGradient *brush,
  ARGB *blend,
  REAL* positions,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetLinePresetBlend(GpLineGradient *brush,
  GDIPCONST ARGB *blend,
  GDIPCONST REAL* positions,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetLineSigmaBlend(GpLineGradient *brush,
  REAL focus,
  REAL scale)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetLineLinearBlend(GpLineGradient *brush,
  REAL focus,
  REAL scale)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetLineWrapMode(GpLineGradient *brush,
  GpWrapMode wrapmode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetLineWrapMode(GpLineGradient *brush,
  GpWrapMode *wrapmode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetLineTransform(GpLineGradient *brush,
  GpMatrix *matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetLineTransform(GpLineGradient *brush,
  GDIPCONST GpMatrix *matrix)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipResetLineTransform(GpLineGradient* brush)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipMultiplyLineTransform(GpLineGradient* brush,
  GDIPCONST GpMatrix *matrix,
  GpMatrixOrder order)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipTranslateLineTransform(GpLineGradient* brush,
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
GdipScaleLineTransform(GpLineGradient* brush,
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
GdipRotateLineTransform(GpLineGradient* brush,
  REAL angle,
  GpMatrixOrder order)
{
  return NotImplemented;
}
