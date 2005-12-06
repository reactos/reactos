#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCloneBrush(GpBrush *brush,
  GpBrush **cloneBrush)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDeleteBrush(GpBrush *brush)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetBrushType(GpBrush *brush,
  GpBrushType *type)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateHatchBrush(GpHatchStyle hatchstyle,
  ARGB forecol,
  ARGB backcol,
  GpHatch **brush)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetHatchStyle(GpHatch *brush,
  GpHatchStyle *hatchstyle)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetHatchForegroundColor(GpHatch *brush,
  ARGB* forecol)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetHatchBackgroundColor(GpHatch *brush,
  ARGB* backcol)
{
  return NotImplemented;
}
