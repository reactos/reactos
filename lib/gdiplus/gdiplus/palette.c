#include <windows.h>
#include <GdiPlusPrivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetNearestColor(GpGraphics *graphics,
  ARGB* argb)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
HPALETTE WINGDIPAPI
GdipCreateHalftonePalette()
{
  return NULL;
}
