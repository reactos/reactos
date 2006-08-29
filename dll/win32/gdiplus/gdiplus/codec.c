#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageDecodersSize(UINT *numDecoders,
  UINT *size)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageDecoders(UINT numDecoders,
  UINT size,
  ImageCodecInfo *decoders)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageEncodersSize(UINT *numEncoders,
  UINT *size)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageEncoders(UINT numEncoders,
  UINT size,
  ImageCodecInfo *encoders)
{
  return NotImplemented;
}
