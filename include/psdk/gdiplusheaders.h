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

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

class Image : public GdiplusBase
{
public:
  friend class Graphics;

  Image(IStream *stream, BOOL useEmbeddedColorManagement)
  {
  }

  Image(const WCHAR *filename, BOOL useEmbeddedColorManagement)
  {
  }

  Image *Clone(VOID)
  {
    return NULL;
  }

  static Image *FromFile(const WCHAR *filename, BOOL useEmbeddedColorManagement)
  {
    return NULL;
  }

  static Image *FromStream(IStream *stream, BOOL useEmbeddedColorManagement)
  {
    return NULL;
  }

  Status GetAllPropertyItems(UINT totalBufferSize, UINT numProperties, PropertyItem *allItems)
  {
    return NotImplemented;
  }

  Status GetBounds(RectF *srcRect, Unit *srcUnit)
  {
    return NotImplemented;
  }

  Status GetEncoderParameterList(const CLSID *clsidEncoder, UINT size, EncoderParameters *buffer)
  {
    return NotImplemented;
  }

  UINT GetEncoderParameterListSize(const CLSID *clsidEncoder)
  {
    return 0;
  }

  UINT GetFlags(VOID)
  {
    return 0;
  }

  UINT GetFrameCount(const GUID *dimensionID)
  {
    return 0;
  }

  UINT GetFrameDimensionsCount(VOID)
  {
    return 0;
  }

  Status GetFrameDimensionsList(GUID *dimensionIDs, UINT count)
  {
    return NotImplemented;
  }

  UINT GetHeight(VOID)
  {
    return 0;
  }

  REAL GetHorizontalResolution(VOID)
  {
    return 0;
  }

  Status GetLastStatus(VOID)
  {
    return NotImplemented;
  }

  Status GetPalette(ColorPalette *palette, INT size)
  {
    return NotImplemented;
  }

  INT GetPaletteSize(VOID)
  {
    return 0;
  }

  Status GetPhysicalDimension(SizeF *size)
  {
    return NotImplemented;
  }

  PixelFormat GetPixelFormat(VOID)
  {
    return PixelFormatUndefined;
  }

  UINT GetPropertyCount(VOID)
  {
    return 0;
  }

  Status GetPropertyIdList(UINT numOfProperty, PROPID *list)
  {
    return NotImplemented;
  }

  Status GetPropertyItem(PROPID propId, UINT propSize, PropertyItem *buffer)
  {
    return NotImplemented;
  }

  UINT GetPropertyItemSize(PROPID propId)
  {
    return 0;
  }

  Status GetPropertySize(UINT *totalBufferSize, UINT *numProperties)
  {
    return NotImplemented;
  }

  Status GetRawFormat(GUID *format)
  {
    return NotImplemented;
  }

  Image *GetThumbnailImage(UINT thumbWidth, UINT thumbHeight, GetThumbnailImageAbort callback, VOID *callbackData)
  {
    return NULL;
  }

  ImageType GetType(VOID)
  {
    return ImageTypeUnknown;
  }

  REAL GetVerticalResolution(VOID)
  {
    return 0;
  }

  UINT GetWidth(VOID)
  {
    return 0;
  }

  Status RemovePropertyItem(PROPID propId)
  {
    return NotImplemented;
  }

  Status RotateFlip(RotateFlipType rotateFlipType)
  {
    return NotImplemented;
  }

  Status Save(IStream *stream, const CLSID *clsidEncoder, const EncoderParameters *encoderParams)
  {
    return NotImplemented;
  }

  Status Save(const WCHAR *filename, const CLSID *clsidEncoder, const EncoderParameters *encoderParams)
  {
    return NotImplemented;
  }

  Status SaveAdd(const EncoderParameters* encoderParams)
  {
    return NotImplemented;
  }

  Status SaveAdd(Image *newImage, const EncoderParameters *encoderParams)
  {
    return NotImplemented;
  }

  Status SelectActiveFrame(const GUID *dimensionID, UINT frameIndex)
  {
    return NotImplemented;
  }

  Status SetPalette(const ColorPalette* palette)
  {
    return NotImplemented;
  }

  Status SetPropertyItem(const PropertyItem* item)
  {
    return NotImplemented;
  }

protected:
  Image()
  {
  }
};


class Bitmap : public Image
{
public:
  Bitmap(IDirectDrawSurface7 *surface)
  {
  }

  Bitmap(INT width, INT height, Graphics *target)
  {
  }

  Bitmap(const BITMAPINFO *gdiBitmapInfo, VOID *gdiBitmapData)
  {
  }

  Bitmap(INT width, INT height, PixelFormat format)
  {
  }

  Bitmap(HBITMAP hbm, HPALETTE hpal)
  {
  }

  Bitmap(INT width, INT height, INT stride, PixelFormat format, BYTE *scan0)
  {
  }

  Bitmap(const WCHAR *filename, BOOL useIcm)
  {
  }

  Bitmap(HINSTANCE hInstance, const WCHAR *bitmapName)
  {
  }

  Bitmap(HICON hicon)
  {
  }

  Bitmap(IStream *stream, BOOL useIcm)
  {
  }

  Bitmap *Clone(const Rect &rect, PixelFormat format)
  {
    return NULL;
  }

  Bitmap *Clone(const RectF &rect, PixelFormat format)
  {
    return NULL;
  }

  Bitmap *Clone(REAL x, REAL y, REAL width, REAL height, PixelFormat format)
  {
    return NULL;
  }

  Bitmap *Clone(INT x, INT y, INT width, INT height, PixelFormat format)
  {
    return NULL;
  }

  static Bitmap *FromBITMAPINFO(const BITMAPINFO *gdiBitmapInfo, VOID *gdiBitmapData)
  {
    return NULL;
  }

  static Bitmap *FromDirectDrawSurface7(IDirectDrawSurface7 *surface)
  {
    return NULL;
  }

  static Bitmap *FromFile(const WCHAR *filename, BOOL useEmbeddedColorManagement)
  {
    return NULL;
  }

  static Bitmap *FromHBITMAP(HBITMAP hbm, HPALETTE hpal)
  {
    return NULL;
  }

  static Bitmap *FromHICON(HICON hicon)
  {
    return NULL;
  }

  static Bitmap *FromResource(HINSTANCE hInstance, const WCHAR *bitmapName)
  {
    return NULL;
  }

  static Bitmap *FromStream(IStream *stream, BOOL useEmbeddedColorManagement)
  {
    return NULL;
  }

  Status GetHBITMAP(const Color &colorBackground, HBITMAP *hbmReturn)
  {
    return NotImplemented;
  }

  Status GetHICON(HICON *hicon)
  {
    return NotImplemented;
  }

  Status GetPixel(INT x, INT y, Color *color)
  {
    return NotImplemented;
  }

  Status LockBits(const Rect *rect, UINT flags, PixelFormat format, BitmapData *lockedBitmapData)
  {
    return NotImplemented;
  }

  Status SetPixel(INT x, INT y, const Color &color)
  {
    return NotImplemented;
  }

  Status SetResolution(REAL xdpi, REAL ydpi)
  {
    return NotImplemented;
  }

  Status UnlockBits(BitmapData *lockedBitmapData)
  {
    return NotImplemented;
  }
};


class CachedBitmap : public GdiplusBase
{
public:
  CachedBitmap(Bitmap *bitmap, Graphics *graphics)
  {
  }

  Status GetLastStatus(VOID)
  {
    return NotImplemented;
  }
};


class Font : public GdiplusBase
{
public:
  friend class FontFamily;
  friend class FontCollection;

  Font(const FontFamily *family, REAL emSize, INT style, Unit unit)
  {
  }

  Font(HDC hdc, const HFONT hfont)
  {
  }

  Font(HDC hdc, const LOGFONTA *logfont)
  {
  }

  Font(HDC hdc, const LOGFONTW *logfont)
  {
  }

  Font(const WCHAR *familyName, REAL emSize, INT style, Unit unit, const FontCollection *fontCollection)
  {
  }

  Font(HDC hdc)
  {
  }

  Font *Clone(VOID) const
  {
    return NULL;
  }

  Status GetFamily(FontFamily* family) const
  {
    return NotImplemented;
  }

  REAL GetHeight(const Graphics* graphics) const
  {
    return 0;
  }

  REAL GetHeight(REAL dpi) const
  {
    return 0;
  }

  Status GetLastStatus(VOID) const
  {
    return NotImplemented;
  }

  Status GetLogFontA(const Graphics *g, LOGFONTA *logfontA) const
  {
    return NotImplemented;
  }

  Status GetLogFontW(const Graphics *g, LOGFONTW *logfontW) const
  {
    return NotImplemented;
  }

  REAL GetSize(VOID) const
  {
    return 0;
  }

  INT GetStyle(VOID) const
  {
    return 0;
  }

  Unit GetUnit(VOID) const
  {
    return UnitWorld;
  }

  BOOL IsAvailable(VOID) const
  {
    return FALSE;
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
  }

  FontFamily *Clone(VOID)
  {
    return NULL;
  }

  static const FontFamily *GenericMonospace(VOID)
  {
    return NULL;
  }

  static const FontFamily *GenericSansSerif(VOID)
  {
    return NULL;
  }

  static const FontFamily *GenericSerif(VOID)
  {
    return NULL;
  }

  UINT16 GetCellAscent(INT style) const
  {
    return 0;
  }

  UINT16 GetCellDescent(INT style) const
  {
    return 0;
  }

  UINT16 GetEmHeight(INT style)
  {
    return 0;
  }

  Status GetFamilyName(WCHAR name[LF_FACESIZE], WCHAR language) const
  {
    return NotImplemented;
  }

  Status GetLastStatus(VOID) const
  {
    return NotImplemented;
  }

  UINT16 GetLineSpacing(INT style) const
  {
    return 0;
  }

  BOOL IsAvailable(VOID) const
  {
    return FALSE;
  }

  BOOL IsStyleAvailable(INT style) const
  {
    return FALSE;
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
  friend class GraphicsPath;
  friend class Matrix;

  Region(const Rect &rect)
  {
  }

  Region(VOID)
  {
  }

  Region(const BYTE *regionData, INT size)
  {
  }

  Region(const GraphicsPath *path)
  {
  }

  Region(HRGN hRgn)
  {
  }

  Region(const RectF &rect)
  {
  }

  Region *Clone(VOID)
  {
    return NULL;
  }

  Status Complement(const GraphicsPath *path)
  {
    return NotImplemented;
  }

  Status Complement(const Region *region)
  {
    return NotImplemented;
  }

  Status Complement(const Rect &rect)
  {
    return NotImplemented;
  }

  Status Complement(const RectF &rect)
  {
    return NotImplemented;
  }

  BOOL Equals(const Region *region, const Graphics *g) const
  {
    return FALSE;
  }

  Status Exclude(const GraphicsPath *path)
  {
    return NotImplemented;
  }

  Status Exclude(const RectF &rect)
  {
    return NotImplemented;
  }

  Status Exclude(const Rect &rect)
  {
    return NotImplemented;
  }

  Status Exclude(const Region *region)
  {
    return NotImplemented;
  }

  static Region *FromHRGN(HRGN hRgn)
  {
    return NULL;
  }

  Status GetBounds(Rect *rect, const Graphics *g) const
  {
    return NotImplemented;
  }

  Status GetBounds(RectF *rect, const Graphics *g) const
  {
    return NotImplemented;
  }

  Status GetData(BYTE *buffer, UINT bufferSize, UINT *sizeFilled) const
  {
    return NotImplemented;
  }

  UINT GetDataSize(VOID) const
  {
    return 0;
  }

  HRGN GetHRGN(const Graphics *g) const
  {
    return NULL;
  }

  Status GetLastStatus(VOID)
  {
    return NotImplemented;
  }

  Status GetRegionScans(const Matrix *matrix, Rect *rects, INT *count) const
  {
    return NotImplemented;
  }

  Status GetRegionScans(const Matrix *matrix, RectF *rects, INT *count) const
  {
    return NotImplemented;
  }

  UINT GetRegionScansCount(const Matrix *matrix) const
  {
    return 0;
  }

  Status Intersect(const Rect &rect)
  {
    return NotImplemented;
  }

  Status Intersect(const GraphicsPath *path)
  {
    return NotImplemented;
  }

  Status Intersect(const RectF &rect)
  {
    return NotImplemented;
  }

  Status Intersect(const Region *region)
  {
    return NotImplemented;
  }

  BOOL IsEmpty(const Graphics *g) const
  {
    return NotImplemented;
  }

  BOOL IsInfinite(const Graphics *g) const
  {
    return FALSE;
  }

  BOOL IsVisible(const PointF &point, const Graphics *g) const
  {
    return FALSE;
  }

  BOOL IsVisible(const RectF &rect, const Graphics *g) const
  {
    return FALSE;
  }

  BOOL IsVisible(const Rect &rect, const Graphics *g) const
  {
    return FALSE;
  }

  BOOL IsVisible(INT x, INT y, const Graphics *g) const
  {
    return FALSE;
  }

  BOOL IsVisible(REAL x, REAL y, const Graphics *g) const
  {
    return FALSE;
  }

  BOOL IsVisible(INT x, INT y, INT width, INT height, const Graphics *g) const
  {
    return FALSE;
  }

  BOOL IsVisible(const Point &point, const Graphics *g) const
  {
    return FALSE;
  }

  BOOL IsVisible(REAL x, REAL y, REAL width, REAL height, const Graphics *g) const
  {
    return FALSE;
  }

  Status MakeEmpty(VOID)
  {
    return NotImplemented;
  }

  Status MakeInfinite(VOID)
  {
    return NotImplemented;
  }

  Status Transform(const Matrix *matrix)
  {
    return NotImplemented;
  }

  Status Translate(REAL dx, REAL dy)
  {
    return NotImplemented;
  }

  Status Translate(INT dx, INT dy)
  {
    return NotImplemented;
  }

  Status Union(const Rect &rect)
  {
    return NotImplemented;
  }

  Status Union(const Region *region)
  {
    return NotImplemented;
  }

  Status Union(const RectF &rect)
  {
    return NotImplemented;
  }

  Status Union(const GraphicsPath *path)
  {
    return NotImplemented;
  }

  Status Xor(const GraphicsPath *path)
  {
    return NotImplemented;
  }

  Status Xor(const RectF &rect)
  {
    return NotImplemented;
  }

  Status Xor(const Rect &rect)
  {
    return NotImplemented;
  }

  Status Xor(const Region *region)
  {
    return NotImplemented;
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
