/*
 * GdiPlusHeaders.h
 *
 * Windows GDI+
 *
 * This file is part of the w32api package.
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _GDIPLUSHEADERS_H
#define _GDIPLUSHEADERS_H

class Image : public GdiplusBase
{
public:
  friend class Graphics;

  Image(IStream *stream, BOOL useEmbeddedColorManagement = FALSE)
  {
    if (useEmbeddedColorManagement)
      status = DllExports::GdipLoadImageFromStreamICM(stream, &image);
    else
      status = DllExports::GdipLoadImageFromStream(stream, &image);
  }

  Image(const WCHAR *filename, BOOL useEmbeddedColorManagement = FALSE)
  {
    if (useEmbeddedColorManagement)
      status = DllExports::GdipLoadImageFromFileICM(filename, &image);
    else
      status = DllExports::GdipLoadImageFromFile(filename, &image);
  }

  Image *Clone(VOID)
  {
    Image *newImage = new Image();
    SetStatus(DllExports::GdipCloneImage(image, &(newImage->image)));
    return newImage;
  }

  static Image *FromFile(const WCHAR *filename, BOOL useEmbeddedColorManagement = FALSE)
  {
    return new Image(filename, useEmbeddedColorManagement);
  }

  static Image *FromStream(IStream *stream, BOOL useEmbeddedColorManagement = FALSE)
  {
    return new Image(stream, useEmbeddedColorManagement);
  }

  Status GetAllPropertyItems(UINT totalBufferSize, UINT numProperties, PropertyItem *allItems)
  {
    return SetStatus(DllExports::GdipGetAllPropertyItems(image, totalBufferSize, numProperties, allItems));
  }

  Status GetBounds(RectF *srcRect, Unit *srcUnit)
  {
    return SetStatus(DllExports::GdipGetImageBounds(image, srcRect, srcUnit));
  }

  Status GetEncoderParameterList(const CLSID *clsidEncoder, UINT size, EncoderParameters *buffer)
  {
    return SetStatus(DllExports::GdipGetEncoderParameterList(image, clsidEncoder, size, buffer));
  }

  UINT GetEncoderParameterListSize(const CLSID *clsidEncoder)
  {
    UINT size;
    SetStatus(DllExports::GdipGetEncoderParameterListSize(image, clsidEncoder, &size));
    return size;
  }

  UINT GetFlags(VOID)
  {
    UINT flags;
    SetStatus(DllExports::GdipGetImageFlags(image, &flags));
    return flags;
  }

  UINT GetFrameCount(const GUID *dimensionID)
  {
    UINT count;
    SetStatus(DllExports::GdipImageGetFrameCount(image, dimensionID, &count));
    return count;
  }

  UINT GetFrameDimensionsCount(VOID)
  {
    UINT count;
    SetStatus(DllExports::GdipImageGetFrameDimensionsCount(image, &count));
    return count;
  }

  Status GetFrameDimensionsList(GUID *dimensionIDs, UINT count)
  {
    return SetStatus(DllExports::GdipImageGetFrameDimensionsList(image, dimensionIDs, count));
  }

  UINT GetHeight(VOID)
  {
    UINT height;
    SetStatus(DllExports::GdipGetImageHeight(image, &height));
    return height;
  }

  REAL GetHorizontalResolution(VOID)
  {
    REAL resolution;
    SetStatus(DllExports::GdipGetImageHorizontalResolution(image, &resolution));
    return resolution;
  }

  Status GetLastStatus(VOID)
  {
    return status;
  }

  Status GetPalette(ColorPalette *palette, INT size)
  {
    return SetStatus(DllExports::GdipGetImagePalette(image, palette, size));
  }

  INT GetPaletteSize(VOID)
  {
    INT size;
    SetStatus(DllExports::GdipGetImagePaletteSize(image, &size));
    return size;
  }

  Status GetPhysicalDimension(SizeF *size)
  {
    return SetStatus(DllExports::GdipGetImagePhysicalDimension(image, &(size->Width), &(size->Height)));
  }

  PixelFormat GetPixelFormat(VOID)
  {
    PixelFormat format;
    SetStatus(DllExports::GdipGetImagePixelFormat(image, &format));
    return format;
  }

  UINT GetPropertyCount(VOID)
  {
    UINT numOfProperty;
    SetStatus(DllExports::GdipGetPropertyCount(image, &numOfProperty));
    return numOfProperty;
  }

  Status GetPropertyIdList(UINT numOfProperty, PROPID *list)
  {
    return SetStatus(DllExports::GdipGetPropertyIdList(image, numOfProperty, list));
  }

  Status GetPropertyItem(PROPID propId, UINT propSize, PropertyItem *buffer)
  {
    return SetStatus(DllExports::GdipGetPropertyItem(image, propId, propSize, buffer));
  }

  UINT GetPropertyItemSize(PROPID propId)
  {
    UINT size;
    SetStatus(DllExports::GdipGetPropertyItemSize(image, propId, &size));
    return size;
  }

  Status GetPropertySize(UINT *totalBufferSize, UINT *numProperties)
  {
    return SetStatus(DllExports::GdipGetPropertySize(image, totalBufferSize, numProperties));
  }

  Status GetRawFormat(GUID *format)
  {
    return SetStatus(DllExports::GdipGetRawFormat(image, format));
  }

  Image *GetThumbnailImage(UINT thumbWidth, UINT thumbHeight, GetThumbnailImageAbort callback, VOID *callbackData)
  {
    Image *thumbImage = new Image();
    SetStatus(DllExports::GdipGetImageThumbnail(image, thumbWidth, thumbHeight, &(thumbImage->image), callback, callbackData));
    return thumbImage;
  }

  ImageType GetType(VOID)
  {
    ImageType type;
    SetStatus(DllExports::GdipGetImageType(image, &type));
    return type;
  }

  REAL GetVerticalResolution(VOID)
  {
    REAL resolution;
    SetStatus(DllExports::GdipGetImageVerticalResolution(image, &resolution));
    return resolution;
  }

  UINT GetWidth(VOID)
  {
    UINT width;
    SetStatus(DllExports::GdipGetImageWidth(image, &width));
    return width;
  }

  Status RemovePropertyItem(PROPID propId)
  {
    return SetStatus(DllExports::GdipRemovePropertyItem(image, propId));
  }

  Status RotateFlip(RotateFlipType rotateFlipType)
  {
    return SetStatus(DllExports::GdipImageRotateFlip(image, rotateFlipType));
  }

  Status Save(IStream *stream, const CLSID *clsidEncoder, const EncoderParameters *encoderParams)
  {
    return SetStatus(DllExports::GdipSaveImageToStream(image, stream, clsidEncoder, encoderParams));
  }

  Status Save(const WCHAR *filename, const CLSID *clsidEncoder, const EncoderParameters *encoderParams)
  {
    return SetStatus(DllExports::GdipSaveImageToFile(image, filename, clsidEncoder, encoderParams));
  }

  Status SaveAdd(const EncoderParameters* encoderParams)
  {
    return SetStatus(DllExports::GdipSaveAdd(image, encoderParams));
  }

  Status SaveAdd(Image *newImage, const EncoderParameters *encoderParams)
  {
    return SetStatus(DllExports::GdipSaveAddImage(image, newImage->image, encoderParams));
  }

  Status SelectActiveFrame(const GUID *dimensionID, UINT frameIndex)
  {
    return SetStatus(DllExports::GdipImageSelectActiveFrame(image, dimensionID, frameIndex));
  }

  Status SetPalette(const ColorPalette* palette)
  {
    return SetStatus(DllExports::GdipSetImagePalette(image, palette));
  }

  Status SetPropertyItem(const PropertyItem* item)
  {
    return SetStatus(DllExports::GdipSetPropertyItem(image, item));
  }

protected:
  Image()
  {
  }

private:
  mutable Status status;
  GpImage *image;

  Status SetStatus(Status status) const
  {
    if (status == Ok)
      return status;
    this->status = status;
    return status;
  }
};


class Bitmap : public Image
{
public:
  Bitmap(IDirectDrawSurface7 *surface)
  {
    status = DllExports::GdipCreateBitmapFromDirectDrawSurface(surface, &bitmap);
  }

  Bitmap(INT width, INT height, Graphics *target)
  {
    status = DllExports::GdipCreateBitmapFromGraphics(width, height, target->graphics, &bitmap);
  }

  Bitmap(const BITMAPINFO *gdiBitmapInfo, VOID *gdiBitmapData)
  {
    status = DllExports::GdipCreateBitmapFromGdiDib(gdiBitmapInfo, gdiBitmapData, &bitmap);
  }

  Bitmap(INT width, INT height, PixelFormat format)
  {
  }

  Bitmap(HBITMAP hbm, HPALETTE hpal)
  {
    status = DllExports::GdipCreateBitmapFromHBITMAP(hbm, hpal, &bitmap);
  }

  Bitmap(INT width, INT height, INT stride, PixelFormat format, BYTE *scan0)
  {
    status = DllExports::GdipCreateBitmapFromScan0(width, height, stride, format, scan0, &bitmap);
  }

  Bitmap(const WCHAR *filename, BOOL useIcm)
  {
    if (useIcm)
      status = DllExports::GdipCreateBitmapFromFileICM(filename, &bitmap);
    else
      status = DllExports::GdipCreateBitmapFromFile(filename, &bitmap);
  }

  Bitmap(HINSTANCE hInstance, const WCHAR *bitmapName)
  {
    status = DllExports::GdipCreateBitmapFromResource(hInstance, bitmapName, &bitmap);
  }

  Bitmap(HICON hicon)
  {
    status = DllExports::GdipCreateBitmapFromHICON(hicon, &bitmap);
  }

  Bitmap(IStream *stream, BOOL useIcm)
  {
    if (useIcm)
      status = DllExports::GdipCreateBitmapFromStreamICM(stream, &bitmap);
    else
      status = DllExports::GdipCreateBitmapFromStream(stream, &bitmap);
  }

  Bitmap *Clone(const Rect &rect, PixelFormat format)
  {
    Bitmap *dstBitmap = new Bitmap();
    SetStatus(DllExports::GdipCloneBitmapAreaI(rect.X, rect.Y, rect.Width, rect.Height, format, bitmap, &(dstBitmap->bitmap)));
    return dstBitmap;
  }

  Bitmap *Clone(const RectF &rect, PixelFormat format)
  {
    Bitmap *dstBitmap = new Bitmap();
    SetStatus(DllExports::GdipCloneBitmapArea(rect.X, rect.Y, rect.Width, rect.Height, format, bitmap, &(dstBitmap->bitmap)));
    return dstBitmap;
  }

  Bitmap *Clone(REAL x, REAL y, REAL width, REAL height, PixelFormat format)
  {
    Bitmap *dstBitmap = new Bitmap();
    SetStatus(DllExports::GdipCloneBitmapArea(x, y, width, height, format, bitmap, &(dstBitmap->bitmap)));
    return dstBitmap;
  }

  Bitmap *Clone(INT x, INT y, INT width, INT height, PixelFormat format)
  {
    Bitmap *dstBitmap = new Bitmap();
    SetStatus(DllExports::GdipCloneBitmapAreaI(x, y, width, height, format, bitmap, &(dstBitmap->bitmap)));
    return dstBitmap;
  }

  static Bitmap *FromBITMAPINFO(const BITMAPINFO *gdiBitmapInfo, VOID *gdiBitmapData)
  {
    return new Bitmap(gdiBitmapInfo, gdiBitmapData);
  }

  static Bitmap *FromDirectDrawSurface7(IDirectDrawSurface7 *surface)
  {
    return new Bitmap(surface);
  }

  static Bitmap *FromFile(const WCHAR *filename, BOOL useEmbeddedColorManagement)
  {
    return new Bitmap(filename, useEmbeddedColorManagement);
  }

  static Bitmap *FromHBITMAP(HBITMAP hbm, HPALETTE hpal)
  {
    return new Bitmap(hbm, hpal);
  }

  static Bitmap *FromHICON(HICON hicon)
  {
    return new Bitmap(hicon);
  }

  static Bitmap *FromResource(HINSTANCE hInstance, const WCHAR *bitmapName)
  {
    return new Bitmap(hInstance, bitmapName);
  }

  static Bitmap *FromStream(IStream *stream, BOOL useEmbeddedColorManagement)
  {
    return new Bitmap(stream, useEmbeddedColorManagement);
  }

  Status GetHBITMAP(const Color &colorBackground, HBITMAP *hbmReturn)
  {
    return SetStatus(DllExports::GdipCreateHBITMAPFromBitmap(bitmap, hbmReturn, colorBackground.GetValue()));
  }

  Status GetHICON(HICON *hicon)
  {
    return SetStatus(DllExports::GdipCreateHICONFromBitmap(bitmap, hbmReturn));
  }

  Status GetPixel(INT x, INT y, Color *color)
  {
    ARGB argb;
    Status s = SetStatus(DllExports::GdipBitmapGetPixel(bitmap, x, y, &argb));
    if (color != NULL)
      color->SetValue(argb);
    return s;
  }

  Status LockBits(const Rect *rect, UINT flags, PixelFormat format, BitmapData *lockedBitmapData)
  {
    return SetStatus(DllExports::GdipBitmapLockBits(bitmap, rect, flags, format, lockedBitmapData));
  }

  Status SetPixel(INT x, INT y, const Color &color)
  {
    return SetStatus(DllExports::GdipBitmapSetPixel(bitmap, x, y, color.GetValue()));
  }

  Status SetResolution(REAL xdpi, REAL ydpi)
  {
    return SetStatus(DllExports::GdipBitmapSetResolution(bitmap, xdpi, ydpi));
  }

  Status UnlockBits(BitmapData *lockedBitmapData)
  {
    return SetStatus(DllExports::GdipBitmapUnlockBits(bitmap, lockedBitmapData));
  }
  
protected:
  Bitmap()
  {
  }

private:
  mutable Status status;
  GpBitmap *bitmap;

  Status SetStatus(Status status) const
  {
    if (status == Ok)
      return status;
    this->status = status;
    return status;
  }
};


class CachedBitmap : public GdiplusBase
{
public:
  CachedBitmap(Bitmap *bitmap, Graphics *graphics)
  {
    status = DllExports::GdipCreateCachedBitmap(bitmap, graphics, &cachedBitmap);
  }

  Status GetLastStatus(VOID)
  {
    return status;
  }

private:
  mutable Status status;
  GpCachedBitmap *cachedBitmap;
};


class Font : public GdiplusBase
{
public:
  friend class FontFamily;
  friend class FontCollection;
  friend class Graphics;

  Font(const FontFamily *family, REAL emSize, INT style, Unit unit)
  {
    status = DllExports::GdipCreateFont(family->fontFamily, emSize, style. unit, &font);
  }

  Font(HDC hdc, const HFONT hfont)
  {
  }

  Font(HDC hdc, const LOGFONTA *logfont)
  {
    status = DllExports::GdipCreateFontFromLogfontA(hdc, logfont, &font);
  }

  Font(HDC hdc, const LOGFONTW *logfont)
  {
    status = DllExports::GdipCreateFontFromLogfontW(hdc, logfont, &font);
  }

  Font(const WCHAR *familyName, REAL emSize, INT style, Unit unit, const FontCollection *fontCollection)
  {
  }

  Font(HDC hdc)
  {
    status = DllExports::GdipCreateFontFromDC(hdc, &font);
  }

  Font *Clone(VOID) const
  {
    Font *cloneFont = new Font();
    cloneFont->status = DllExports::GdipCloneFont(font, &(cloneFont->font));
    return cloneFont;
  }

  Status GetFamily(FontFamily* family) const
  {
    return SetStatus(DllExports::GdipGetFamily(font, &(family->fontFamily)));
  }

  REAL GetHeight(const Graphics* graphics) const
  {
    REAL height;
    SetStatus(DllExports::GdipGetFontHeight(font, graphics->graphics, &height));
    return height;
  }

  REAL GetHeight(REAL dpi) const
  {
    REAL height;
    SetStatus(DllExports::GdipGetFontHeightGivenDPI(font, dpi, &height));
    return height;
  }

  Status GetLastStatus(VOID) const
  {
    return status;
  }

  Status GetLogFontA(const Graphics *g, LOGFONTA *logfontA) const
  {
    return SetStatus(DllExports::GdipGetLogFontA(font, g->graphics, logfontA));
  }

  Status GetLogFontW(const Graphics *g, LOGFONTW *logfontW) const
  {
    return SetStatus(DllExports::GdipGetLogFontW(font, g->graphics, logfontW));
  }

  REAL GetSize(VOID) const
  {
    REAL size;
    SetStatus(DllExports::GdipGetFontSize(font, &size));
    return size;
  }

  INT GetStyle(VOID) const
  {
    INT style;
    SetStatus(DllExports::GdipGetFontStyle(font, &style));
    return style;
  }

  Unit GetUnit(VOID) const
  {
    Unit unit;
    SetStatus(DllExports::GdipGetFontUnit(font, &unit);
    return unit;
  }

  BOOL IsAvailable(VOID) const
  {
    return FALSE;
  }

private:
  mutable Status status;
  GpFont *font;

  Status SetStatus(Status status) const
  {
    if (status == Ok)
      return status;
    this->status = status;
    return status;
  }
};


class FontCollection : public GdiplusBase
{
public:
  FontCollection(VOID)
  {
  }

  Status GetFamilies(INT numSought, FontFamily *gpfamilies, INT *numFound) const
  {
    return NotImplemented;
  }

  INT GetFamilyCount(VOID) const
  {
    return 0;
  }

  Status GetLastStatus(VOID)
  {
    return NotImplemented;
  }
};


class FontFamily : public GdiplusBase
{
public:
  FontFamily(VOID)
  {
  }

  FontFamily(const WCHAR *name, const FontCollection *fontCollection)
  {
    status = DllExports::GdipCreateFontFamilyFromName(name, fontCollection, &fontFamily);
  }

  FontFamily *Clone(VOID)
  {
    return NULL;
  }

  static const FontFamily *GenericMonospace(VOID)
  {
    FontFamily *genericMonospace = new FontFamily();
    genericMonospace->status = DllExports::GdipGetGenericFontFamilyMonospace(&(genericMonospace->fontFamily));
    return genericMonospace;
  }

  static const FontFamily *GenericSansSerif(VOID)
  {
    FontFamily *genericSansSerif = new FontFamily();
    genericSansSerif->status = DllExports::GdipGetGenericFontFamilySansSerif(&(genericSansSerif->fontFamily));
    return genericSansSerif;
  }

  static const FontFamily *GenericSerif(VOID)
  {
    FontFamily *genericSerif = new FontFamily();
    genericSerif->status = DllExports::GdipGetGenericFontFamilyMonospace(&(genericSerif->fontFamily));
    return genericSerif;
  }

  UINT16 GetCellAscent(INT style) const
  {
    UINT16 CellAscent;
    SetStatus(DllExports::GdipGetCellAscent(fontFamily, style, &CellAscent));
    return CellAscent;
  }

  UINT16 GetCellDescent(INT style) const
  {
    UINT16 CellDescent;
    SetStatus(DllExports::GdipGetCellDescent(fontFamily, style, &CellDescent));
    return CellDescent;
  }

  UINT16 GetEmHeight(INT style)
  {
    UINT16 EmHeight;
    SetStatus(DllExports::GdipGetEmHeight(fontFamily, style, &EmHeight));
    return EmHeight;
  }

  Status GetFamilyName(WCHAR name[LF_FACESIZE], WCHAR language) const
  {
    return SetStatus(DllExports::GdipGetFamilyName(fontFamily, name, language));
  }

  Status GetLastStatus(VOID) const
  {
    return status;
  }

  UINT16 GetLineSpacing(INT style) const
  {
    UINT16 LineSpacing;
    SetStatus(DllExports::GdipGetLineSpacing(fontFamily, style, &LineSpacing));
    return LineSpacing;
  }

  BOOL IsAvailable(VOID) const
  {
    return FALSE;
  }

  BOOL IsStyleAvailable(INT style) const
  {
    BOOL StyleAvailable;
    SetStatus(DllExports::GdipIsStyleAvailable(fontFamily, style, &StyleAvailable));
    return StyleAvailable;
  }

private:
  mutable Status status;
  GpFontFamily *fontFamily;

  Status SetStatus(Status status) const
  {
    if (status == Ok)
      return status;
    this->status = status;
    return status;
  }
};


class InstalledFontFamily : public FontFamily
{
public:
  InstalledFontFamily(VOID)
  {
  }
};


class PrivateFontCollection : public FontCollection
{
public:
  PrivateFontCollection(VOID)
  {
  }

  Status AddFontFile(const WCHAR* filename)
  {
    return NotImplemented;
  }

  Status AddMemoryFont(const VOID *memory, INT length)
  {
    return NotImplemented;
  }
};


class Region : public GdiplusBase
{
public:
  friend class Graphics;
  friend class GraphicsPath;
  friend class Matrix;

  Region(const Rect &rect)
  {
    status = DllExports::GdipCreateRegionRectI(&rect, &region);
  }

  Region(VOID)
  {
    status = DllExports::GdipCreateRegion(&region);
  }

  Region(const BYTE *regionData, INT size)
  {
    status = DllExports::GdipCreateRegionRgnData(regionData, size, &region);
  }

  Region(const GraphicsPath *path)
  {
    status = DllExports::GdipCreateRegionPath(path->path, &region);
  }

  Region(HRGN hRgn)
  {
    status = DllExports::GdipCreateRegionHrgn(hRgn, &region);
  }

  Region(const RectF &rect)
  {
    status = DllExports::GdipCreateRegionRectF(&rect, &region);
  }

  Region *Clone(VOID)
  {
    region *cloneRegion = new Region();
    cloneRegion->status = DllExports::GdipCloneRegion(region, &cloneRegion);
    return cloneRegion;
  }

  Status Complement(const GraphicsPath *path)
  {
    return SetStatus(DllExports::GdipCombineRegionPath(region, path->path, CombineModeComplement));
  }

  Status Complement(const Region *region)
  {
    return SetStatus(DllExports::GdipCombineRegionRegion(this->region, region->region, CombineModeComplement));
  }

  Status Complement(const Rect &rect)
  {
    return SetStatus(DllExports::GdipCombineRegionRectI(region, &rect, CombineModeComplement));
  }

  Status Complement(const RectF &rect)
  {
    return SetStatus(DllExports::GdipCombineRegionRect(region, &rect, CombineModeComplement));
  }

  BOOL Equals(const Region *region, const Graphics *g) const
  {
    BOOL result;
    SetStatus(DllExports::GdipIsEqualRegion(this->region, region->region, g->graphics, &result));
    return result;
  }

  Status Exclude(const GraphicsPath *path)
  {
    return SetStatus(DllExports::GdipCombineRegionPath(region, path->path, CombineModeExclude));
  }

  Status Exclude(const RectF &rect)
  {
    return SetStatus(DllExports::GdipCombineRegionRect(region, &rect, CombineModeExclude));
  }

  Status Exclude(const Rect &rect)
  {
    return SetStatus(DllExports::GdipCombineRegionRectI(region, &rect, CombineModeExclude));
  }

  Status Exclude(const Region *region)
  {
    return SetStatus(DllExports::GdipCombineRegionRegion(this->region, region->region, CombineModeExclude));
  }

  static Region *FromHRGN(HRGN hRgn)
  {
    return new Region(hRgn);
  }

  Status GetBounds(Rect *rect, const Graphics *g) const
  {
    return SetStatus(DllExports::GdipGetRegionBoundsI(region, g->graphics, rect));
  }

  Status GetBounds(RectF *rect, const Graphics *g) const
  {
    return SetStatus(DllExports::GdipGetRegionBounds(region, g->graphics, rect));
  }

  Status GetData(BYTE *buffer, UINT bufferSize, UINT *sizeFilled) const
  {
    return SetStatus(DllExports::GdipGetRegionData(region, budder, bufferSize, sizeFilled));
  }

  UINT GetDataSize(VOID) const
  {
    UINT bufferSize;
    SetStatus(DllExports::GdipGetRegionDataSize(region, &bufferSize));
    return bufferSize;
  }

  HRGN GetHRGN(const Graphics *g) const
  {
    HRGN hRgn;
    SetStatus(DllExports::GdipGetRegionHRgn(region, g->graphics, &hRgn));
    return hRgn;
  }

  Status GetLastStatus(VOID)
  {
    return status;
  }

  Status GetRegionScans(const Matrix *matrix, Rect *rects, INT *count) const
  {
    return SetStatus(DllExports::GdipGetRegionScansI(region, rects, count, matrix->matrix));
  }

  Status GetRegionScans(const Matrix *matrix, RectF *rects, INT *count) const
  {
    return SetStatus(DllExports::GdipGetRegionScans(region, rects, count, matrix->matrix));
  }

  UINT GetRegionScansCount(const Matrix *matrix) const
  {
    UINT count;
    SetStatus(DllExports::GdipGetRegionScansCount(region, &count, matrix->matrix));
    return count;
  }

  Status Intersect(const Rect &rect)
  {
    return SetStatus(DllExports::GdipCombineRegionRectI(region, &rect, CombineModeIntersect));
  }

  Status Intersect(const GraphicsPath *path)
  {
    return SetStatus(DllExports::GdipCombineRegionPath(region, path->path, CombineModeIntersect));
  }

  Status Intersect(const RectF &rect)
  {
    return SetStatus(DllExports::GdipCombineRegionRect(region, &rect, CombineModeIntersect));
  }

  Status Intersect(const Region *region)
  {
    return SetStatus(DllExports::GdipCombineRegionRegion(this->region, region->region, CombineModeIntersect));
  }

  BOOL IsEmpty(const Graphics *g) const
  {
    BOOL result;
    SetStatus(DllExports::GdipIsEmptyRegion(region, g->graphics, &result));
    return result;
  }

  BOOL IsInfinite(const Graphics *g) const
  {
    BOOL result;
    SetStatus(DllExports::GdipIsInfiniteRegion(region, g->graphics, &result));
    return result;
  }

  BOOL IsVisible(const PointF &point, const Graphics *g) const
  {
    BOOL result;
    SetStatus(DllExports::GdipIsVisibleRegionPoint(region, point.x, point.y, g->graphics, &result));
    return result;
  }

  BOOL IsVisible(const RectF &rect, const Graphics *g) const
  {
    BOOL result;
    SetStatus(DllExports::GdipIsVisibleRegionRect(region, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, g->graphics, &result));
    return result;
  }

  BOOL IsVisible(const Rect &rect, const Graphics *g) const
  {
    BOOL result;
    SetStatus(DllExports::GdipIsVisibleRegionRectI(region, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, g->graphics, &result));
    return result;
  }

  BOOL IsVisible(INT x, INT y, const Graphics *g) const
  {
    BOOL result;
    SetStatus(DllExports::GdipIsVisibleRegionPointI(region, x, y, g->graphics, &result));
    return result;
  }

  BOOL IsVisible(REAL x, REAL y, const Graphics *g) const
  {
    BOOL result;
    SetStatus(DllExports::GdipIsVisibleRegionPoint(region, x, y, g->graphics, &result));
    return result;
  }

  BOOL IsVisible(INT x, INT y, INT width, INT height, const Graphics *g) const
  {
    BOOL result;
    SetStatus(DllExports::GdipIsVisibleRegionRectI(region, x, y, width, height, g->graphics, &result));
    return result;
  }

  BOOL IsVisible(const Point &point, const Graphics *g) const
  {
    BOOL result;
    SetStatus(DllExports::GdipIsVisibleRegionPointI(region, point.x, point.y, g->graphics, &result));
    return result;
  }

  BOOL IsVisible(REAL x, REAL y, REAL width, REAL height, const Graphics *g) const
  {
    BOOL result;
    SetStatus(DllExports::GdipIsVisibleRegionRect(region, x, y, width, height, g->graphics, &result));
    return result;
  }

  Status MakeEmpty(VOID)
  {
    return SetStatus(DllExports::GdipSetEmpty(region));
  }

  Status MakeInfinite(VOID)
  {
    return SetStatus(DllExports::GdipSetInfinite(region));
  }

  Status Transform(const Matrix *matrix)
  {
    return SetStatus(DllExports::GdipTransformRegion(region, matrix->matrix));
  }

  Status Translate(REAL dx, REAL dy)
  {
    return SetStatus(DllExports::GdipTranslateRegion(region, dx, dy));
  }

  Status Translate(INT dx, INT dy)
  {
    return SetStatus(DllExports::GdipTranslateRegionI(region, dx, dy));
  }

  Status Union(const Rect &rect)
  {
    return SetStatus(DllExports::GdipCombineRegionRectI(region, &rect, CombineModeUnion));
  }

  Status Union(const Region *region)
  {
    return SetStatus(DllExports::GdipCombineRegionRegion(this->region, region->region, CombineModeUnion));
  }

  Status Union(const RectF &rect)
  {
    return SetStatus(DllExports::GdipCombineRegionRect(region, &rect, CombineModeUnion));
  }

  Status Union(const GraphicsPath *path)
  {
    return SetStatus(DllExports::GdipCombineRegionPath(region, path->path, CombineModeUnion));
  }

  Status Xor(const GraphicsPath *path)
  {
    return SetStatus(DllExports::GdipCombineRegionPath(region, path->path, CombineModeXor));
  }

  Status Xor(const RectF &rect)
  {
    return SetStatus(DllExports::GdipCombineRegionRect(region, &rect, CombineModeXor));
  }

  Status Xor(const Rect &rect)
  {
    return SetStatus(DllExports::GdipCombineRegionRectI(region, &rect, CombineModeXor));
  }

  Status Xor(const Region *region)
  {
    return SetStatus(DllExports::GdipCombineRegionRegion(this->region, region->region, CombineModeXor));
  }

private:
  mutable Status status;
  GpRegion *region;

  Status SetStatus(Status status) const
  {
    if (status == Ok)
      return status;
    this->status = status;
    return status;
  }
};


class CustomLineCap : public GdiplusBase
{
public:
  CustomLineCap(const GraphicsPath *fillPath, const GraphicsPath *strokePath, LineCap baseCap, REAL baseInset);
  CustomLineCap *Clone(VOID);
  LineCap GetBaseCap(VOID);
  REAL GetBaseInset(VOID);
  Status GetLastStatus(VOID);
  Status GetStrokeCaps(LineCap *startCap, LineCap *endCap);
  LineJoin GetStrokeJoin(VOID);
  REAL GetWidthScale(VOID);
  Status SetBaseCap(LineCap baseCap);
  Status SetBaseInset(REAL inset);
  Status SetStrokeCap(LineCap strokeCap);
  Status SetStrokeCaps(LineCap startCap, LineCap endCap);
  Status SetStrokeJoin(LineJoin lineJoin);
  Status SetWidthScale(IN REAL widthScale);
protected:
  CustomLineCap();
};

#endif /* _GDIPLUSHEADERS_H */
