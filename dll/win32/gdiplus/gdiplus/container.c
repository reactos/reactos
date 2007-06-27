#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipBeginContainer(GpGraphics *graphics,
  GDIPCONST GpRectF* dstrect,
  GDIPCONST GpRectF *srcrect,
  GpUnit unit,
  GraphicsContainer *state)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipBeginContainerI(GpGraphics *graphics,
  GDIPCONST GpRect* dstrect,
  GDIPCONST GpRect *srcrect,
  GpUnit unit,
  GraphicsContainer *state)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipBeginContainer2(GpGraphics *graphics,
  GraphicsContainer* state)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipEndContainer(GpGraphics *graphics,
  GraphicsContainer state)
{
  return NotImplemented;
}
