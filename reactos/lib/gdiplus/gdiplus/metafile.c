#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
UINT WINGDIPAPI
GdipEmfToWmfBits(HENHMETAFILE hemf,
  UINT cbData16,
  LPBYTE pData16,
  INT iMapMode,
  INT eFlags)
{
  return 0;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipEnumerateMetafileDestPoint(GpGraphics * graphics,
  GDIPCONST GpMetafile * metafile,
  GDIPCONST PointF * destPoint,
  EnumerateMetafileProc callback,
  VOID * callbackData,
  GDIPCONST GpImageAttributes * imageAttributes)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipEnumerateMetafileDestPointI(GpGraphics * graphics,
  GDIPCONST GpMetafile * metafile,
  GDIPCONST Point * destPoint,
  EnumerateMetafileProc callback,
  VOID * callbackData,
  GDIPCONST GpImageAttributes * imageAttributes)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipEnumerateMetafileDestRect(GpGraphics * graphics,
  GDIPCONST GpMetafile * metafile,
  GDIPCONST RectF * destRect,
  EnumerateMetafileProc callback,
  VOID * callbackData,
  GDIPCONST GpImageAttributes * imageAttributes)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipEnumerateMetafileDestRectI(GpGraphics * graphics,
  GDIPCONST GpMetafile * metafile,
  GDIPCONST Rect * destRect,
  EnumerateMetafileProc callback,
  VOID * callbackData,
  GDIPCONST GpImageAttributes * imageAttributes)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipEnumerateMetafileDestPoints(GpGraphics * graphics,
  GDIPCONST GpMetafile * metafile,
  GDIPCONST PointF * destPoints,
  INT count,
  EnumerateMetafileProc callback,
  VOID * callbackData,
  GDIPCONST GpImageAttributes * imageAttributes)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipEnumerateMetafileDestPointsI(GpGraphics * graphics,
  GDIPCONST GpMetafile * metafile,
  GDIPCONST Point * destPoints,
  INT count,
  EnumerateMetafileProc callback,
  VOID * callbackData,
  GDIPCONST GpImageAttributes * imageAttributes)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipEnumerateMetafileSrcRectDestPoint(GpGraphics * graphics,
  GDIPCONST GpMetafile * metafile,
  GDIPCONST PointF * destPoint,
  GDIPCONST RectF * srcRect,
  Unit srcUnit,
  EnumerateMetafileProc callback,
  VOID * callbackData,
  GDIPCONST GpImageAttributes * imageAttributes)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipEnumerateMetafileSrcRectDestPointI(GpGraphics * graphics,
  GDIPCONST GpMetafile * metafile,
  GDIPCONST Point * destPoint,
  GDIPCONST Rect * srcRect,
  Unit srcUnit,
  EnumerateMetafileProc callback,
  VOID * callbackData,
  GDIPCONST GpImageAttributes * imageAttributes)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipEnumerateMetafileSrcRectDestRect(GpGraphics * graphics,
  GDIPCONST GpMetafile * metafile,
  GDIPCONST RectF * destRect,
  GDIPCONST RectF * srcRect,
  Unit srcUnit,
  EnumerateMetafileProc callback,
  VOID * callbackData,
  GDIPCONST GpImageAttributes * imageAttributes)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipEnumerateMetafileSrcRectDestRectI(GpGraphics * graphics,
  GDIPCONST GpMetafile * metafile,
  GDIPCONST Rect * destRect,
  GDIPCONST Rect * srcRect,
  Unit srcUnit,
  EnumerateMetafileProc callback,
  VOID * callbackData,
  GDIPCONST GpImageAttributes * imageAttributes)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipEnumerateMetafileSrcRectDestPoints(GpGraphics * graphics,
  GDIPCONST GpMetafile * metafile,
  GDIPCONST PointF * destPoints,
  INT count,
  GDIPCONST RectF * srcRect,
  Unit srcUnit,
  EnumerateMetafileProc callback,
  VOID * callbackData,
  GDIPCONST GpImageAttributes * imageAttributes)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipEnumerateMetafileSrcRectDestPointsI(GpGraphics * graphics,
  GDIPCONST GpMetafile * metafile,
  GDIPCONST Point * destPoints,
  INT count,
  GDIPCONST Rect * srcRect,
  Unit srcUnit,
  EnumerateMetafileProc callback,
  VOID * callbackData,
  GDIPCONST GpImageAttributes * imageAttributes)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipPlayMetafileRecord(GDIPCONST GpMetafile * metafile,
  EmfPlusRecordType recordType,
  UINT flags,
  UINT dataSize,
  GDIPCONST BYTE * data)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetMetafileHeaderFromEmf(HENHMETAFILE hEmf,
  MetafileHeader * header)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetMetafileHeaderFromFile(GDIPCONST WCHAR* filename,
  MetafileHeader * header)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetMetafileHeaderFromStream(IStream * stream,
  MetafileHeader * header)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetMetafileHeaderFromMetafile(GpMetafile * metafile,
  MetafileHeader * header)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetHemfFromMetafile(GpMetafile * metafile,
  HENHMETAFILE * hEmf)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateStreamOnFile(GDIPCONST WCHAR * filename,
  UINT access,
  IStream **stream)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateMetafileFromWmf(HMETAFILE hWmf,
  BOOL deleteWmf,
  GDIPCONST WmfPlaceableFileHeader * wmfPlaceableFileHeader,
  GpMetafile **metafile)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateMetafileFromEmf(HENHMETAFILE hEmf,
  BOOL deleteEmf,
  GpMetafile **metafile)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateMetafileFromFile(GDIPCONST WCHAR* file,
  GpMetafile **metafile)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateMetafileFromWmfFile(GDIPCONST WCHAR* file,
  GDIPCONST WmfPlaceableFileHeader * wmfPlaceableFileHeader,
  GpMetafile **metafile)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateMetafileFromStream(IStream * stream,
  GpMetafile **metafile)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipRecordMetafile(HDC referenceHdc,
  EmfType type,
  GDIPCONST GpRectF * frameRect,
  MetafileFrameUnit frameUnit,
  GDIPCONST WCHAR * description,
  GpMetafile ** metafile)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipRecordMetafileI(HDC referenceHdc,
  EmfType type,
  GDIPCONST GpRect * frameRect,
  MetafileFrameUnit frameUnit,
  GDIPCONST WCHAR * description,
  GpMetafile ** metafile)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipRecordMetafileFileName(GDIPCONST WCHAR* fileName,
  HDC referenceHdc,
  EmfType type,
  GDIPCONST GpRectF * frameRect,
  MetafileFrameUnit frameUnit,
  GDIPCONST WCHAR * description,
  GpMetafile ** metafile)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipRecordMetafileFileNameI(GDIPCONST WCHAR* fileName,
  HDC referenceHdc,
  EmfType type,
  GDIPCONST GpRect * frameRect,
  MetafileFrameUnit frameUnit,
  GDIPCONST WCHAR * description,
  GpMetafile ** metafile)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipRecordMetafileStream(IStream * stream,
  HDC referenceHdc,
  EmfType type,
  GDIPCONST GpRectF * frameRect,
  MetafileFrameUnit frameUnit,
  GDIPCONST WCHAR * description,
  GpMetafile ** metafile)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipRecordMetafileStreamI(IStream * stream,
  HDC referenceHdc,
  EmfType type,
  GDIPCONST GpRect * frameRect,
  MetafileFrameUnit frameUnit,
  GDIPCONST WCHAR * description,
  GpMetafile ** metafile)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetMetafileDownLevelRasterizationLimit(GpMetafile * metafile,
  UINT metafileRasterizationLimitDpi)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetMetafileDownLevelRasterizationLimit(GDIPCONST GpMetafile * metafile,
  UINT * metafileRasterizationLimitDpi)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipComment(GpGraphics* graphics,
  UINT sizeData,
  GDIPCONST BYTE * data)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipConvertToEmfPlus(const GpGraphics* refGraphics,
  GpMetafile* metafile,
  BOOL* conversionSuccess,
  EmfType emfType,
  const WCHAR* description,
  GpMetafile** out_metafile)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipConvertToEmfPlusToFile(const GpGraphics* refGraphics,
  GpMetafile* metafile,
  BOOL* conversionSuccess,
  const WCHAR* filename,
  EmfType emfType,
  const WCHAR* description,
  GpMetafile** out_metafile)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipConvertToEmfPlusToStream(const GpGraphics* refGraphics,
  GpMetafile* metafile,
  BOOL* conversionSuccess,
  IStream* stream,
  EmfType emfType,
  const WCHAR* description,
  GpMetafile** out_metafile)
{
  return NotImplemented;
}
