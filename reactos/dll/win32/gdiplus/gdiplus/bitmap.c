#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateBitmapFromStream(IStream* stream,
  GpBitmap **bitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateBitmapFromFile(GDIPCONST WCHAR* filename,
  GpBitmap **bitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateBitmapFromStreamICM(IStream* stream,
  GpBitmap **bitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateBitmapFromFileICM(GDIPCONST WCHAR* filename,
  GpBitmap **bitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateBitmapFromScan0(INT width,
  INT height,
  INT stride,
  PixelFormat format,
  BYTE* scan0,
  GpBitmap** bitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateBitmapFromGraphics(INT width,
  INT height,
  GpGraphics* target,
  GpBitmap** bitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateBitmapFromDirectDrawSurface(IDirectDrawSurface7* surface,
GpBitmap** bitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateBitmapFromGdiDib(GDIPCONST BITMAPINFO* gdiBitmapInfo,
  VOID* gdiBitmapData,
  GpBitmap** bitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateBitmapFromHBITMAP(HBITMAP hbm,
  HPALETTE hpal,
  GpBitmap** bitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateHBITMAPFromBitmap(GpBitmap* bitmap,
  HBITMAP* hbmReturn,
  ARGB background)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateBitmapFromHICON(HICON hicon,
  GpBitmap** bitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateHICONFromBitmap(GpBitmap* bitmap,
  HICON* hbmReturn)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateBitmapFromResource(HINSTANCE hInstance,
  GDIPCONST WCHAR* lpBitmapName,
  GpBitmap** bitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCloneBitmapArea(REAL x,
  REAL y,
  REAL width,
  REAL height,
  PixelFormat format,
  GpBitmap *srcBitmap,
  GpBitmap **dstBitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCloneBitmapAreaI(INT x,
  INT y,
  INT width,
  INT height,
  PixelFormat format,
  GpBitmap *srcBitmap,
  GpBitmap **dstBitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipBitmapLockBits(GpBitmap* bitmap,
  GDIPCONST GpRect* rect,
  UINT flags,
  PixelFormat format,
  BitmapData* lockedBitmapData)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipBitmapUnlockBits(GpBitmap* bitmap,
  BitmapData* lockedBitmapData)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipBitmapGetPixel(GpBitmap* bitmap,
  INT x,
  INT y,
  ARGB *color)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipBitmapSetPixel(GpBitmap* bitmap,
  INT x,
  INT y,
  ARGB color)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipBitmapSetResolution(GpBitmap* bitmap,
  REAL xdpi,
  REAL ydpi)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipBitmapConvertFormat(IN GpBitmap *pInputBitmap,
  PixelFormat format,
  DitherType dithertype,
  PaletteType palettetype,
  ColorPalette *palette,
  REAL alphaThresholdPercent)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipInitializePalette(OUT ColorPalette *palette,
  PaletteType palettetype,
  INT optimalColors,
  BOOL useTransparentColor,
  GpBitmap *bitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipBitmapApplyEffect(GpBitmap* bitmap,
  CGpEffect *effect,
  RECT *roi,
  BOOL useAuxData,
  VOID **auxData,
  INT *auxDataSize)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipBitmapCreateApplyEffect(GpBitmap **inputBitmaps,
  INT numInputs,
  CGpEffect *effect,
  RECT *roi,
  RECT *outputRect,
  GpBitmap **outputBitmap,
  BOOL useAuxData,
  VOID **auxData,
  INT *auxDataSize)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipBitmapGetHistogram(GpBitmap* bitmap,
  IN HistogramFormat format,
  IN UINT NumberOfEntries,
  OUT UINT *channel0,
  OUT UINT *channel1,
  OUT UINT *channel2,
  OUT UINT *channel3)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipBitmapGetHistogramSize(IN HistogramFormat format,
  OUT UINT *NumberOfEntries)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateCachedBitmap(GpBitmap *bitmap,
  GpGraphics *graphics,
  GpCachedBitmap **cachedBitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDeleteCachedBitmap(GpCachedBitmap *cachedBitmap)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawCachedBitmap(GpGraphics *graphics,
  GpCachedBitmap *cachedBitmap,
  INT x,
  INT y)
{
  return NotImplemented;
}
