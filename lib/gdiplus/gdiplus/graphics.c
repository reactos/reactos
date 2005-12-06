#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipFlush(GpGraphics *graphics,
  GpFlushIntention intention)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateFromHDC(HDC hdc,
  GpGraphics **graphics)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateFromHDC2(HDC hdc,
  HANDLE hDevice,
  GpGraphics **graphics)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateFromHWND(HWND hwnd,
  GpGraphics **graphics)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateFromHWNDICM(HWND hwnd,
  GpGraphics **graphics)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDeleteGraphics(GpGraphics *graphics)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetDC(GpGraphics* graphics,
  HDC * hdc)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipReleaseDC(GpGraphics* graphics,
  HDC hdc)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetCompositingMode(GpGraphics *graphics,
  CompositingMode compositingMode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetCompositingMode(GpGraphics *graphics,
  CompositingMode *compositingMode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetRenderingOrigin(GpGraphics *graphics,
  INT x,
  INT y)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetRenderingOrigin(GpGraphics *graphics,
  INT *x,
  INT *y)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetCompositingQuality(GpGraphics *graphics,
  CompositingQuality compositingQuality)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetCompositingQuality(GpGraphics *graphics,
  CompositingQuality *compositingQuality)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetSmoothingMode(GpGraphics *graphics,
  SmoothingMode smoothingMode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetSmoothingMode(GpGraphics *graphics,
  SmoothingMode *smoothingMode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetPixelOffsetMode(GpGraphics* graphics,
  PixelOffsetMode pixelOffsetMode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPixelOffsetMode(GpGraphics *graphics,
  PixelOffsetMode *pixelOffsetMode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetTextRenderingHint(GpGraphics *graphics,
  TextRenderingHint mode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetTextRenderingHint(GpGraphics *graphics,
  TextRenderingHint *mode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetTextContrast(GpGraphics *graphics,
  UINT contrast)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetTextContrast(GpGraphics *graphics,
  UINT * contrast)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetInterpolationMode(GpGraphics *graphics,
  InterpolationMode interpolationMode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetInterpolationMode(GpGraphics *graphics,
  InterpolationMode *interpolationMode)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPageUnit(GpGraphics *graphics,
  GpUnit *unit)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPageScale(GpGraphics *graphics,
  REAL *scale)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetPageUnit(GpGraphics *graphics,
  GpUnit unit)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetPageScale(GpGraphics *graphics,
  REAL scale)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetDpiX(GpGraphics *graphics,
  REAL* dpi)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetDpiY(GpGraphics *graphics,
  REAL* dpi)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGraphicsClear(GpGraphics *graphics,
  ARGB color)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSaveGraphics(GpGraphics *graphics,
  GraphicsState *state)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipRestoreGraphics(GpGraphics *graphics,
  GraphicsState state)
{
  return NotImplemented;
}


/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipTestControl(enum GpTestControlEnum control,
  void * param)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdiplusNotificationHook(OUT ULONG_PTR *token)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
VOID WINGDIPAPI
GdiplusNotificationUnhook(ULONG_PTR token)
{	 
}
