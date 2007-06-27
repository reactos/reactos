#include <windows.h>
#include <gdiplusprivate.h>
#include <debug.h>

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawImage(GpGraphics *graphics,
  GpImage *image,
  REAL x,
  REAL y)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawImageI(GpGraphics *graphics,
  GpImage *image,
  INT x,
  INT y)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawImageRect(GpGraphics *graphics,
  GpImage *image,
  REAL x,
  REAL y,
  REAL width,
  REAL height)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawImageRectI(GpGraphics *graphics,
  GpImage *image,
  INT x,
  INT y,
  INT width,
  INT height)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawImagePoints(GpGraphics *graphics,
  GpImage *image,
  GDIPCONST GpPointF *dstpoints,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawImagePointsI(GpGraphics *graphics,
  GpImage *image,
  GDIPCONST GpPoint *dstpoints,
  INT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawImagePointRect(GpGraphics *graphics,
  GpImage *image,
  REAL x,
  REAL y,
  REAL srcx,
  REAL srcy,
  REAL srcwidth,
  REAL srcheight,
  GpUnit srcUnit)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawImagePointRectI(GpGraphics *graphics,
  GpImage *image,
  INT x,
  INT y,
  INT srcx,
  INT srcy,
  INT srcwidth,
  INT srcheight,
  GpUnit srcUnit)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawImageRectRect(GpGraphics *graphics,
  GpImage *image,
  REAL dstx,
  REAL dsty,
  REAL dstwidth,
  REAL dstheight,
  REAL srcx,
  REAL srcy,
  REAL srcwidth,
  REAL srcheight,
  GpUnit srcUnit,
  GDIPCONST GpImageAttributes* imageAttributes,
  DrawImageAbort callback,
  VOID * callbackData)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawImageRectRectI(GpGraphics *graphics,
  GpImage *image,
  INT dstx,
  INT dsty,
  INT dstwidth,
  INT dstheight,
  INT srcx,
  INT srcy,
  INT srcwidth,
  INT srcheight,
  GpUnit srcUnit,
  GDIPCONST GpImageAttributes* imageAttributes,
  DrawImageAbort callback,
  VOID * callbackData)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawImagePointsRect(GpGraphics *graphics,
  GpImage *image,
  GDIPCONST GpPointF *points,
  INT count,
  REAL srcx,
  REAL srcy,
  REAL srcwidth,
  REAL srcheight,
  GpUnit srcUnit,
  GDIPCONST GpImageAttributes* imageAttributes,
  DrawImageAbort callback,
  VOID * callbackData)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawImagePointsRectI(GpGraphics *graphics,
  GpImage *image,
  GDIPCONST GpPoint *points,
  INT count,
  INT srcx,
  INT srcy,
  INT srcwidth,
  INT srcheight,
  GpUnit srcUnit,
  GDIPCONST GpImageAttributes* imageAttributes,
  DrawImageAbort callback,
  VOID * callbackData)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDrawImageFX(GpGraphics *graphics,
  GpImage *image,
  GpRectF *source,
  GpMatrix *xForm,
  CGpEffect *effect,
  GpImageAttributes *imageAttributes,
  GpUnit srcUnit)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipLoadImageFromStream(IStream* stream,
  GpImage **image)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipLoadImageFromFile(GDIPCONST WCHAR* filename,
  GpImage **image)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipLoadImageFromStreamICM(IStream* stream,
  GpImage **image)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipLoadImageFromFileICM(GDIPCONST WCHAR* filename,
  GpImage **image)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCloneImage(GpImage *image,
  GpImage **cloneImage)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDisposeImage(GpImage *image)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSaveImageToFile(GpImage *image,
  GDIPCONST WCHAR* filename,
  GDIPCONST CLSID* clsidEncoder,
  GDIPCONST EncoderParameters* encoderParams)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSaveImageToStream(GpImage *image,
  IStream* stream,
  GDIPCONST CLSID* clsidEncoder,
  GDIPCONST EncoderParameters* encoderParams)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSaveAdd(GpImage *image,
  GDIPCONST EncoderParameters* encoderParams)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSaveAddImage(GpImage *image,
  GpImage* newImage,
  GDIPCONST EncoderParameters* encoderParams)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageGraphicsContext(GpImage *image,
  GpGraphics **graphics)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageBounds(GpImage *image,
  GpRectF *srcRect,
  GpUnit *srcUnit)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageDimension(GpImage *image,
  REAL *width,
  REAL *height)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageType(GpImage *image,
  ImageType *type)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageWidth(GpImage *image,
  UINT *width)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageHeight(GpImage *image,
  UINT *height)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageHorizontalResolution(GpImage *image,
  REAL *resolution)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageVerticalResolution(GpImage *image,
  REAL *resolution)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageFlags(GpImage *image,
  UINT *flags)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageRawFormat(GpImage *image,
  GUID *format)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImagePixelFormat(GpImage *image,
  PixelFormat *format)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageThumbnail(GpImage *image,
  UINT thumbWidth,
  UINT thumbHeight,
  GpImage **thumbImage,
  GetThumbnailImageAbort callback,
  VOID * callbackData)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetEncoderParameterListSize(GpImage *image,
  GDIPCONST CLSID* clsidEncoder,
  UINT* size)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetEncoderParameterList(GpImage *image,
  GDIPCONST CLSID* clsidEncoder,
  UINT size,
  EncoderParameters* buffer)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipImageGetFrameDimensionsCount(GpImage* image,
  UINT* count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipImageGetFrameDimensionsList(GpImage* image,
  GUID* dimensionIDs,
  UINT count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipImageGetFrameCount(GpImage *image,
  GDIPCONST GUID* dimensionID,
  UINT* count)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipImageSelectActiveFrame(GpImage *image,
  GDIPCONST GUID* dimensionID,
  UINT frameIndex)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipImageRotateFlip(GpImage *image,
  RotateFlipType rfType)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImagePalette(GpImage *image,
  ColorPalette *palette,
  INT size)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetImagePalette(GpImage *image,
  GDIPCONST ColorPalette *palette)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImagePaletteSize(GpImage *image,
  INT *size)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPropertyCount(GpImage *image,
  UINT* numOfProperty)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPropertyIdList(GpImage *image,
  UINT numOfProperty,
  PROPID* list)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPropertyItemSize(GpImage *image,
  PROPID propId,
  UINT* size)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPropertyItem(GpImage *image,
  PROPID propId,
  UINT propSize,
  PropertyItem* buffer)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetPropertySize(GpImage *image,
  UINT* totalBufferSize,
  UINT* numProperties)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetAllPropertyItems(GpImage *image,
  UINT totalBufferSize,
  UINT numProperties,
  PropertyItem* allItems)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipRemovePropertyItem(GpImage *image,
  PROPID propId)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetPropertyItem(GpImage *image,
  GDIPCONST PropertyItem* item)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipFindFirstImageItem(GpImage *image,
  ImageItemData* item)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipFindNextImageItem(GpImage *image,
  ImageItemData* item)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageItemData(GpImage *image,
  ImageItemData* item)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipImageForceValidation(GpImage *image)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCreateImageAttributes(GpImageAttributes **imageattr)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipCloneImageAttributes(GDIPCONST GpImageAttributes *imageattr,
  GpImageAttributes **cloneImageattr)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipDisposeImageAttributes(GpImageAttributes *imageattr)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetImageAttributesToIdentity(GpImageAttributes *imageattr,
  ColorAdjustType type)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipResetImageAttributes(GpImageAttributes *imageattr,
  ColorAdjustType type)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetImageAttributesColorMatrix(GpImageAttributes *imageattr,
  ColorAdjustType type,
  BOOL enableFlag,
  GDIPCONST struct ColorMatrix* colorMatrix,
  GDIPCONST struct ColorMatrix* grayMatrix,
  ColorMatrixFlags flags)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetImageAttributesThreshold(GpImageAttributes *imageattr,
  ColorAdjustType type,
  BOOL enableFlag,
  REAL threshold)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetImageAttributesGamma(GpImageAttributes *imageattr,
  ColorAdjustType type,
  BOOL enableFlag,
  REAL gamma)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetImageAttributesNoOp(GpImageAttributes *imageattr,
  ColorAdjustType type,
  BOOL enableFlag)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetImageAttributesColorKeys(GpImageAttributes *imageattr,
  ColorAdjustType type,
  BOOL enableFlag,
  ARGB colorLow,
  ARGB colorHigh)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetImageAttributesOutputChannel(GpImageAttributes *imageattr,
  ColorAdjustType type,
  BOOL enableFlag,
  ColorChannelFlags channelFlags)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetImageAttributesOutputChannelColorProfile(GpImageAttributes *imageattr,
  ColorAdjustType type,
  BOOL enableFlag,
  GDIPCONST WCHAR *colorProfileFilename)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetImageAttributesRemapTable(GpImageAttributes *imageattr,
  ColorAdjustType type,
  BOOL enableFlag,
  UINT mapSize,
  GDIPCONST ColorMap *map)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetImageAttributesWrapMode(GpImageAttributes *imageAttr,
  WrapMode wrap,
  ARGB argb,
  BOOL clamp)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetImageAttributesICMMode(GpImageAttributes *imageAttr,
  BOOL on)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipGetImageAttributesAdjustedPalette(GpImageAttributes *imageAttr,
  ColorPalette * colorPalette,
  ColorAdjustType colorAdjustType)
{
  return NotImplemented;
}

/*
 * @unimplemented
 */
GpStatus WINGDIPAPI
GdipSetImageAttributesCachedBackground(GpImageAttributes *imageattr,
  BOOL enableFlag)
{
  return NotImplemented;
}
