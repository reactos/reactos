#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreatePathIter(GpPathIterator **iterator,
  GpPath* path)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDeletePathIter(GpPathIterator *iterator)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPathIterNextSubpath(GpPathIterator* iterator,
  INT *resultCount,
  INT* startIndex,
  INT* endIndex,
  BOOL* isClosed)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPathIterNextSubpathPath(GpPathIterator* iterator,
  INT* resultCount,
  GpPath* path,
  BOOL* isClosed)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPathIterNextPathType(GpPathIterator* iterator,
  INT* resultCount,
  BYTE* pathType,
  INT* startIndex,
  INT* endIndex)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPathIterNextMarker(GpPathIterator* iterator,
  INT *resultCount,
  INT* startIndex,
  INT* endIndex)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPathIterNextMarkerPath(GpPathIterator* iterator,
  INT* resultCount,
  GpPath* path)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPathIterGetCount(GpPathIterator* iterator,
  INT* count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPathIterGetSubpathCount(GpPathIterator* iterator,
  INT* count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPathIterIsValid(GpPathIterator* iterator,
  BOOL* valid)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPathIterHasCurve(GpPathIterator* iterator,
  BOOL* hasCurve)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPathIterRewind(GpPathIterator* iterator)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPathIterEnumerate(GpPathIterator* iterator,
  INT* resultCount,
  GpPointF *points,
  BYTE *types,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPathIterCopyData(GpPathIterator* iterator,
  INT* resultCount,
  GpPointF* points,
  BYTE* types,
  INT startIndex,
  INT endIndex)
{
  return NotImplemented;
}
