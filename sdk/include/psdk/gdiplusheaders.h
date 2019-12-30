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
            lastStatus = DllExports::GdipLoadImageFromStreamICM(stream, &nativeImage);
        else
            lastStatus = DllExports::GdipLoadImageFromStream(stream, &nativeImage);
    }

    Image(const WCHAR *filename, BOOL useEmbeddedColorManagement = FALSE)
    {
        if (useEmbeddedColorManagement)
            lastStatus = DllExports::GdipLoadImageFromFileICM(filename, &nativeImage);
        else
            lastStatus = DllExports::GdipLoadImageFromFile(filename, &nativeImage);
    }

    Image *Clone(VOID)
    {
        Image *newImage = new Image();
        SetStatus(DllExports::GdipCloneImage(nativeImage, newImage ? &newImage->nativeImage : NULL));
        return newImage;
    }

    virtual ~Image()
    {
        DllExports::GdipDisposeImage(nativeImage);
    }

    static Image *
    FromFile(const WCHAR *filename, BOOL useEmbeddedColorManagement = FALSE)
    {
        return new Image(filename, useEmbeddedColorManagement);
    }

    static Image *
    FromStream(IStream *stream, BOOL useEmbeddedColorManagement = FALSE)
    {
        return new Image(stream, useEmbeddedColorManagement);
    }

    Status
    GetAllPropertyItems(UINT totalBufferSize, UINT numProperties, PropertyItem *allItems)
    {
        return SetStatus(DllExports::GdipGetAllPropertyItems(nativeImage, totalBufferSize, numProperties, allItems));
    }

    Status
    GetBounds(RectF *srcRect, Unit *srcUnit)
    {
        return SetStatus(DllExports::GdipGetImageBounds(nativeImage, srcRect, srcUnit));
    }

    Status
    GetEncoderParameterList(const CLSID *clsidEncoder, UINT size, EncoderParameters *buffer)
    {
        return NotImplemented; // FIXME: not available: SetStatus(DllExports::GdipGetEncoderParameterList(nativeImage,
                               // clsidEncoder, size, buffer));
    }

    UINT
    GetEncoderParameterListSize(const CLSID *clsidEncoder)
    {
        return 0; // FIXME: not available:
                  //     UINT size;
                  //     SetStatus(DllExports::GdipGetEncoderParameterListSize(nativeImage, clsidEncoder, &size));
                  //     return size;
    }

    UINT GetFlags(VOID)
    {
        UINT flags;
        SetStatus(DllExports::GdipGetImageFlags(nativeImage, &flags));
        return flags;
    }

    UINT
    GetFrameCount(const GUID *dimensionID)
    {
        UINT count;
        SetStatus(DllExports::GdipImageGetFrameCount(nativeImage, dimensionID, &count));
        return count;
    }

    UINT GetFrameDimensionsCount(VOID)
    {
        UINT count;
        SetStatus(DllExports::GdipImageGetFrameDimensionsCount(nativeImage, &count));
        return count;
    }

    Status
    GetFrameDimensionsList(GUID *dimensionIDs, UINT count)
    {
        return SetStatus(DllExports::GdipImageGetFrameDimensionsList(nativeImage, dimensionIDs, count));
    }

    UINT GetHeight(VOID)
    {
        UINT height;
        SetStatus(DllExports::GdipGetImageHeight(nativeImage, &height));
        return height;
    }

    REAL GetHorizontalResolution(VOID)
    {
        REAL resolution;
        SetStatus(DllExports::GdipGetImageHorizontalResolution(nativeImage, &resolution));
        return resolution;
    }

    Status GetLastStatus(VOID)
    {
        return lastStatus;
    }

    Status
    GetPalette(ColorPalette *palette, INT size)
    {
        return SetStatus(DllExports::GdipGetImagePalette(nativeImage, palette, size));
    }

    INT GetPaletteSize(VOID)
    {
        INT size;
        SetStatus(DllExports::GdipGetImagePaletteSize(nativeImage, &size));
        return size;
    }

    Status
    GetPhysicalDimension(SizeF *size)
    {
        if (size)
            return SetStatus(DllExports::GdipGetImageDimension(nativeImage, &size->Width, &size->Height));
        else
            return SetStatus(DllExports::GdipGetImageDimension(nativeImage, NULL, NULL));
    }

    PixelFormat GetPixelFormat(VOID)
    {
        PixelFormat format;
        SetStatus(DllExports::GdipGetImagePixelFormat(nativeImage, &format));
        return format;
    }

    UINT GetPropertyCount(VOID)
    {
        UINT numOfProperty;
        SetStatus(DllExports::GdipGetPropertyCount(nativeImage, &numOfProperty));
        return numOfProperty;
    }

    Status
    GetPropertyIdList(UINT numOfProperty, PROPID *list)
    {
        return SetStatus(DllExports::GdipGetPropertyIdList(nativeImage, numOfProperty, list));
    }

    Status
    GetPropertyItem(PROPID propId, UINT propSize, PropertyItem *buffer)
    {
        return SetStatus(DllExports::GdipGetPropertyItem(nativeImage, propId, propSize, buffer));
    }

    UINT
    GetPropertyItemSize(PROPID propId)
    {
        UINT size;
        SetStatus(DllExports::GdipGetPropertyItemSize(nativeImage, propId, &size));
        return size;
    }

    Status
    GetPropertySize(UINT *totalBufferSize, UINT *numProperties)
    {
        return SetStatus(DllExports::GdipGetPropertySize(nativeImage, totalBufferSize, numProperties));
    }

    Status
    GetRawFormat(GUID *format)
    {
        return SetStatus(DllExports::GdipGetImageRawFormat(nativeImage, format));
    }

    Image *
    GetThumbnailImage(UINT thumbWidth, UINT thumbHeight, GetThumbnailImageAbort callback, VOID *callbackData)
    {
        Image *thumbImage = new Image();
        SetStatus(DllExports::GdipGetImageThumbnail(
            nativeImage, thumbWidth, thumbHeight, thumbImage ? &thumbImage->nativeImage : NULL, callback,
            callbackData));
        return thumbImage;
    }

    ImageType GetType(VOID)
    {
        ImageType type;
        SetStatus(DllExports::GdipGetImageType(nativeImage, &type));
        return type;
    }

    REAL GetVerticalResolution(VOID)
    {
        REAL resolution;
        SetStatus(DllExports::GdipGetImageVerticalResolution(nativeImage, &resolution));
        return resolution;
    }

    UINT GetWidth(VOID)
    {
        UINT width;
        SetStatus(DllExports::GdipGetImageWidth(nativeImage, &width));
        return width;
    }

    Status
    RemovePropertyItem(PROPID propId)
    {
        return SetStatus(DllExports::GdipRemovePropertyItem(nativeImage, propId));
    }

    Status
    RotateFlip(RotateFlipType rotateFlipType)
    {
        return SetStatus(DllExports::GdipImageRotateFlip(nativeImage, rotateFlipType));
    }

    Status
    Save(IStream *stream, const CLSID *clsidEncoder, const EncoderParameters *encoderParams)
    {
        return SetStatus(DllExports::GdipSaveImageToStream(nativeImage, stream, clsidEncoder, encoderParams));
    }

    Status
    Save(const WCHAR *filename, const CLSID *clsidEncoder, const EncoderParameters *encoderParams)
    {
        return SetStatus(DllExports::GdipSaveImageToFile(nativeImage, filename, clsidEncoder, encoderParams));
    }

    Status
    SaveAdd(const EncoderParameters *encoderParams)
    {
        return NotImplemented; // FIXME: not available: SetStatus(DllExports::GdipSaveAdd(nativeImage, encoderParams));
    }

    Status
    SaveAdd(Image *newImage, const EncoderParameters *encoderParams)
    {
        return NotImplemented; // FIXME: not available: SetStatus(DllExports::GdipSaveAddImage(nativeImage, newImage ?
                               // newImage->nativeImage : NULL, encoderParams));
    }

    Status
    SelectActiveFrame(const GUID *dimensionID, UINT frameIndex)
    {
        return SetStatus(DllExports::GdipImageSelectActiveFrame(nativeImage, dimensionID, frameIndex));
    }

    Status
    SetPalette(const ColorPalette *palette)
    {
        return SetStatus(DllExports::GdipSetImagePalette(nativeImage, palette));
    }

    Status
    SetPropertyItem(const PropertyItem *item)
    {
        return SetStatus(DllExports::GdipSetPropertyItem(nativeImage, item));
    }

  protected:
    Image()
    {
    }

  private:
    mutable Status lastStatus;
    GpImage *nativeImage;

    Status
    SetStatus(Status status) const
    {
        if (status == Ok)
            return status;
        lastStatus = status;
        return status;
    }
};

class Bitmap : public Image
{
    friend class CachedBitmap;

  public:
    //   Bitmap(IDirectDrawSurface7 *surface)  // <-- FIXME: compiler does not like this
    //   {
    //     lastStatus = DllExports::GdipCreateBitmapFromDirectDrawSurface(surface, &bitmap);
    //   }

    Bitmap(INT width, INT height, Graphics *target)
    {
        lastStatus = DllExports::GdipCreateBitmapFromGraphics(width, height, target ? target->graphics : NULL, &bitmap);
    }

    Bitmap(const BITMAPINFO *gdiBitmapInfo, VOID *gdiBitmapData)
    {
        lastStatus = DllExports::GdipCreateBitmapFromGdiDib(gdiBitmapInfo, gdiBitmapData, &bitmap);
    }

    Bitmap(INT width, INT height, PixelFormat format)
    {
    }

    Bitmap(HBITMAP hbm, HPALETTE hpal)
    {
        lastStatus = DllExports::GdipCreateBitmapFromHBITMAP(hbm, hpal, &bitmap);
    }

    Bitmap(INT width, INT height, INT stride, PixelFormat format, BYTE *scan0)
    {
        lastStatus = DllExports::GdipCreateBitmapFromScan0(width, height, stride, format, scan0, &bitmap);
    }

    Bitmap(const WCHAR *filename, BOOL useIcm)
    {
        if (useIcm)
            lastStatus = DllExports::GdipCreateBitmapFromFileICM(filename, &bitmap);
        else
            lastStatus = DllExports::GdipCreateBitmapFromFile(filename, &bitmap);
    }

    Bitmap(HINSTANCE hInstance, const WCHAR *bitmapName)
    {
        lastStatus = DllExports::GdipCreateBitmapFromResource(hInstance, bitmapName, &bitmap);
    }

    Bitmap(HICON hicon)
    {
        lastStatus = DllExports::GdipCreateBitmapFromHICON(hicon, &bitmap);
    }

    Bitmap(IStream *stream, BOOL useIcm)
    {
        if (useIcm)
            lastStatus = DllExports::GdipCreateBitmapFromStreamICM(stream, &bitmap);
        else
            lastStatus = DllExports::GdipCreateBitmapFromStream(stream, &bitmap);
    }

    Bitmap *
    Clone(const Rect &rect, PixelFormat format)
    {
        Bitmap *dstBitmap = new Bitmap();
        SetStatus(DllExports::GdipCloneBitmapAreaI(
            rect.X, rect.Y, rect.Width, rect.Height, format, bitmap, dstBitmap ? &dstBitmap->bitmap : NULL));
        return dstBitmap;
    }

    Bitmap *
    Clone(const RectF &rect, PixelFormat format)
    {
        Bitmap *dstBitmap = new Bitmap();
        SetStatus(DllExports::GdipCloneBitmapArea(
            rect.X, rect.Y, rect.Width, rect.Height, format, bitmap, dstBitmap ? &dstBitmap->bitmap : NULL));
        return dstBitmap;
    }

    Bitmap *
    Clone(REAL x, REAL y, REAL width, REAL height, PixelFormat format)
    {
        Bitmap *dstBitmap = new Bitmap();
        SetStatus(DllExports::GdipCloneBitmapArea(
            x, y, width, height, format, bitmap, dstBitmap ? &dstBitmap->bitmap : NULL));
        return dstBitmap;
    }

    Bitmap *
    Clone(INT x, INT y, INT width, INT height, PixelFormat format)
    {
        Bitmap *dstBitmap = new Bitmap();
        SetStatus(DllExports::GdipCloneBitmapAreaI(
            x, y, width, height, format, bitmap, dstBitmap ? &dstBitmap->bitmap : NULL));
        return dstBitmap;
    }

    static Bitmap *
    FromBITMAPINFO(const BITMAPINFO *gdiBitmapInfo, VOID *gdiBitmapData)
    {
        return new Bitmap(gdiBitmapInfo, gdiBitmapData);
    }

    //   static Bitmap *FromDirectDrawSurface7(IDirectDrawSurface7 *surface)  // <-- FIXME: compiler does not like this
    //   {
    //     return new Bitmap(surface);
    //   }

    static Bitmap *
    FromFile(const WCHAR *filename, BOOL useEmbeddedColorManagement)
    {
        return new Bitmap(filename, useEmbeddedColorManagement);
    }

    static Bitmap *
    FromHBITMAP(HBITMAP hbm, HPALETTE hpal)
    {
        return new Bitmap(hbm, hpal);
    }

    static Bitmap *
    FromHICON(HICON hicon)
    {
        return new Bitmap(hicon);
    }

    static Bitmap *
    FromResource(HINSTANCE hInstance, const WCHAR *bitmapName)
    {
        return new Bitmap(hInstance, bitmapName);
    }

    static Bitmap *
    FromStream(IStream *stream, BOOL useEmbeddedColorManagement)
    {
        return new Bitmap(stream, useEmbeddedColorManagement);
    }

    Status
    GetHBITMAP(const Color &colorBackground, HBITMAP *hbmReturn)
    {
        return SetStatus(DllExports::GdipCreateHBITMAPFromBitmap(bitmap, hbmReturn, colorBackground.GetValue()));
    }

    Status
    GetHICON(HICON *hicon)
    {
        return SetStatus(DllExports::GdipCreateHICONFromBitmap(bitmap, hicon));
    }

    Status
    GetPixel(INT x, INT y, Color *color)
    {
        ARGB argb;
        Status s = SetStatus(DllExports::GdipBitmapGetPixel(bitmap, x, y, &argb));
        if (color != NULL)
            color->SetValue(argb);
        return s;
    }

    Status
    LockBits(const Rect *rect, UINT flags, PixelFormat format, BitmapData *lockedBitmapData)
    {
        return SetStatus(DllExports::GdipBitmapLockBits(bitmap, rect, flags, format, lockedBitmapData));
    }

    Status
    SetPixel(INT x, INT y, const Color &color)
    {
        return SetStatus(DllExports::GdipBitmapSetPixel(bitmap, x, y, color.GetValue()));
    }

    Status
    SetResolution(REAL xdpi, REAL ydpi)
    {
        return SetStatus(DllExports::GdipBitmapSetResolution(bitmap, xdpi, ydpi));
    }

    Status
    UnlockBits(BitmapData *lockedBitmapData)
    {
        return SetStatus(DllExports::GdipBitmapUnlockBits(bitmap, lockedBitmapData));
    }

  protected:
    Bitmap()
    {
    }

  private:
    mutable Status lastStatus;
    GpBitmap *bitmap;

    Status
    SetStatus(Status status) const
    {
        if (status == Ok)
            return status;
        lastStatus = status;
        return status;
    }
};

class CachedBitmap : public GdiplusBase
{
  public:
    CachedBitmap(Bitmap *bitmap, Graphics *graphics)
    {
        lastStatus =
            DllExports::GdipCreateCachedBitmap(bitmap->bitmap, graphics ? graphics->graphics : NULL, &cachedBitmap);
    }

    Status GetLastStatus(VOID)
    {
        return lastStatus;
    }

  private:
    mutable Status lastStatus;
    GpCachedBitmap *cachedBitmap;
};

class FontCollection : public GdiplusBase
{
    friend class FontFamily;

  public:
    FontCollection(VOID)
    {
    }

    Status
    GetFamilies(INT numSought, FontFamily *gpfamilies, INT *numFound) const
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

  private:
    GpFontCollection *fontCollection;
};

class FontFamily : public GdiplusBase
{
    friend class Font;

  public:
    FontFamily(VOID)
    {
    }

    FontFamily(const WCHAR *name, const FontCollection *fontCollection)
    {
        status = DllExports::GdipCreateFontFamilyFromName(
            name, fontCollection ? fontCollection->fontCollection : NULL, &fontFamily);
    }

    FontFamily *Clone(VOID)
    {
        return NULL;
    }

    static const FontFamily *GenericMonospace(VOID)
    {
        FontFamily *genericMonospace = new FontFamily();
        genericMonospace->status =
            DllExports::GdipGetGenericFontFamilyMonospace(genericMonospace ? &genericMonospace->fontFamily : NULL);
        return genericMonospace;
    }

    static const FontFamily *GenericSansSerif(VOID)
    {
        FontFamily *genericSansSerif = new FontFamily();
        genericSansSerif->status =
            DllExports::GdipGetGenericFontFamilySansSerif(genericSansSerif ? &genericSansSerif->fontFamily : NULL);
        return genericSansSerif;
    }

    static const FontFamily *GenericSerif(VOID)
    {
        FontFamily *genericSerif = new FontFamily();
        genericSerif->status =
            DllExports::GdipGetGenericFontFamilyMonospace(genericSerif ? &genericSerif->fontFamily : NULL);
        return genericSerif;
    }

    UINT16
    GetCellAscent(INT style) const
    {
        UINT16 CellAscent;
        SetStatus(DllExports::GdipGetCellAscent(fontFamily, style, &CellAscent));
        return CellAscent;
    }

    UINT16
    GetCellDescent(INT style) const
    {
        UINT16 CellDescent;
        SetStatus(DllExports::GdipGetCellDescent(fontFamily, style, &CellDescent));
        return CellDescent;
    }

    UINT16
    GetEmHeight(INT style)
    {
        UINT16 EmHeight;
        SetStatus(DllExports::GdipGetEmHeight(fontFamily, style, &EmHeight));
        return EmHeight;
    }

    Status
    GetFamilyName(WCHAR name[LF_FACESIZE], WCHAR language) const
    {
        return SetStatus(DllExports::GdipGetFamilyName(fontFamily, name, language));
    }

    Status GetLastStatus(VOID) const
    {
        return status;
    }

    UINT16
    GetLineSpacing(INT style) const
    {
        UINT16 LineSpacing;
        SetStatus(DllExports::GdipGetLineSpacing(fontFamily, style, &LineSpacing));
        return LineSpacing;
    }

    BOOL IsAvailable(VOID) const
    {
        return FALSE;
    }

    BOOL
    IsStyleAvailable(INT style) const
    {
        BOOL StyleAvailable;
        SetStatus(DllExports::GdipIsStyleAvailable(fontFamily, style, &StyleAvailable));
        return StyleAvailable;
    }

  private:
    mutable Status status;
    GpFontFamily *fontFamily;

    Status
    SetStatus(Status status) const
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

    Status
    AddFontFile(const WCHAR *filename)
    {
        return NotImplemented;
    }

    Status
    AddMemoryFont(const VOID *memory, INT length)
    {
        return NotImplemented;
    }
};

class Font : public GdiplusBase
{
  public:
    friend class FontFamily;
    friend class FontCollection;
    friend class Graphics;

    Font(const FontFamily *family, REAL emSize, INT style, Unit unit)
    {
        status = DllExports::GdipCreateFont(family->fontFamily, emSize, style, unit, &font);
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
        cloneFont->status = DllExports::GdipCloneFont(font, cloneFont ? &cloneFont->font : NULL);
        return cloneFont;
    }

    Status
    GetFamily(FontFamily *family) const
    {
        return SetStatus(DllExports::GdipGetFamily(font, family ? &family->fontFamily : NULL));
    }

    REAL
    GetHeight(const Graphics *graphics) const
    {
        REAL height;
        SetStatus(DllExports::GdipGetFontHeight(font, graphics ? graphics->graphics : NULL, &height));
        return height;
    }

    REAL
    GetHeight(REAL dpi) const
    {
        REAL height;
        SetStatus(DllExports::GdipGetFontHeightGivenDPI(font, dpi, &height));
        return height;
    }

    Status GetLastStatus(VOID) const
    {
        return status;
    }

    Status
    GetLogFontA(const Graphics *g, LOGFONTA *logfontA) const
    {
        return SetStatus(DllExports::GdipGetLogFontA(font, g ? g->graphics : NULL, logfontA));
    }

    Status
    GetLogFontW(const Graphics *g, LOGFONTW *logfontW) const
    {
        return SetStatus(DllExports::GdipGetLogFontW(font, g ? g->graphics : NULL, logfontW));
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
        SetStatus(DllExports::GdipGetFontUnit(font, &unit));
        return unit;
    }

    BOOL IsAvailable(VOID) const
    {
        return FALSE;
    }

  protected:
    Font()
    {
    }

  private:
    mutable Status status;
    GpFont *font;

    Status
    SetStatus(Status status) const
    {
        if (status == Ok)
            return status;
        this->status = status;
        return status;
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
        status = DllExports::GdipCreateRegionRect(&rect, &region);
    }

    Region *Clone(VOID)
    {
        Region *cloneRegion = new Region();
        cloneRegion->status = DllExports::GdipCloneRegion(region, cloneRegion ? &cloneRegion->region : NULL);
        return cloneRegion;
    }

    Status
    Complement(const GraphicsPath *path)
    {
        return SetStatus(DllExports::GdipCombineRegionPath(region, path ? path->path : NULL, CombineModeComplement));
    }

    Status
    Complement(const Region *region)
    {
        return SetStatus(
            DllExports::GdipCombineRegionRegion(this->region, region ? region->region : NULL, CombineModeComplement));
    }

    Status
    Complement(const Rect &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRectI(region, &rect, CombineModeComplement));
    }

    Status
    Complement(const RectF &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRect(region, &rect, CombineModeComplement));
    }

    BOOL
    Equals(const Region *region, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsEqualRegion(
            this->region, region ? region->region : NULL, g ? g->graphics : NULL, &result));
        return result;
    }

    Status
    Exclude(const GraphicsPath *path)
    {
        return SetStatus(DllExports::GdipCombineRegionPath(region, path ? path->path : NULL, CombineModeExclude));
    }

    Status
    Exclude(const RectF &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRect(region, &rect, CombineModeExclude));
    }

    Status
    Exclude(const Rect &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRectI(region, &rect, CombineModeExclude));
    }

    Status
    Exclude(const Region *region)
    {
        return SetStatus(
            DllExports::GdipCombineRegionRegion(this->region, region ? region->region : NULL, CombineModeExclude));
    }

    static Region *
    FromHRGN(HRGN hRgn)
    {
        return new Region(hRgn);
    }

    Status
    GetBounds(Rect *rect, const Graphics *g) const
    {
        return SetStatus(DllExports::GdipGetRegionBoundsI(region, g ? g->graphics : NULL, rect));
    }

    Status
    GetBounds(RectF *rect, const Graphics *g) const
    {
        return SetStatus(DllExports::GdipGetRegionBounds(region, g ? g->graphics : NULL, rect));
    }

    Status
    GetData(BYTE *buffer, UINT bufferSize, UINT *sizeFilled) const
    {
        return SetStatus(DllExports::GdipGetRegionData(region, buffer, bufferSize, sizeFilled));
    }

    UINT GetDataSize(VOID) const
    {
        UINT bufferSize;
        SetStatus(DllExports::GdipGetRegionDataSize(region, &bufferSize));
        return bufferSize;
    }

    HRGN
    GetHRGN(const Graphics *g) const
    {
        HRGN hRgn;
        SetStatus(DllExports::GdipGetRegionHRgn(region, g ? g->graphics : NULL, &hRgn));
        return hRgn;
    }

    Status GetLastStatus(VOID)
    {
        return status;
    }

    Status
    GetRegionScans(const Matrix *matrix, Rect *rects, INT *count) const
    {
        return SetStatus(DllExports::GdipGetRegionScansI(region, rects, count, matrix ? matrix->matrix : NULL));
    }

    Status
    GetRegionScans(const Matrix *matrix, RectF *rects, INT *count) const
    {
        return SetStatus(DllExports::GdipGetRegionScans(region, rects, count, matrix ? matrix->matrix : NULL));
    }

    UINT
    GetRegionScansCount(const Matrix *matrix) const
    {
        UINT count;
        SetStatus(DllExports::GdipGetRegionScansCount(region, &count, matrix ? matrix->matrix : NULL));
        return count;
    }

    Status
    Intersect(const Rect &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRectI(region, &rect, CombineModeIntersect));
    }

    Status
    Intersect(const GraphicsPath *path)
    {
        return SetStatus(DllExports::GdipCombineRegionPath(region, path ? path->path : NULL, CombineModeIntersect));
    }

    Status
    Intersect(const RectF &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRect(region, &rect, CombineModeIntersect));
    }

    Status
    Intersect(const Region *region)
    {
        return SetStatus(
            DllExports::GdipCombineRegionRegion(this->region, region ? region->region : NULL, CombineModeIntersect));
    }

    BOOL
    IsEmpty(const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsEmptyRegion(region, g ? g->graphics : NULL, &result));
        return result;
    }

    BOOL
    IsInfinite(const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsInfiniteRegion(region, g ? g->graphics : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(const PointF &point, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRegionPoint(region, point.X, point.Y, g ? g->graphics : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(const RectF &rect, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRegionRect(
            region, rect.X, rect.Y, rect.Width, rect.Height, g ? g->graphics : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(const Rect &rect, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRegionRectI(
            region, rect.X, rect.Y, rect.Width, rect.Height, g ? g->graphics : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(INT x, INT y, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRegionPointI(region, x, y, g ? g->graphics : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(REAL x, REAL y, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRegionPoint(region, x, y, g ? g->graphics : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(INT x, INT y, INT width, INT height, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRegionRectI(region, x, y, width, height, g ? g->graphics : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(const Point &point, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRegionPointI(region, point.X, point.Y, g ? g->graphics : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(REAL x, REAL y, REAL width, REAL height, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRegionRect(region, x, y, width, height, g ? g->graphics : NULL, &result));
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

    Status
    Transform(const Matrix *matrix)
    {
        return SetStatus(DllExports::GdipTransformRegion(region, matrix ? matrix->matrix : NULL));
    }

    Status
    Translate(REAL dx, REAL dy)
    {
        return SetStatus(DllExports::GdipTranslateRegion(region, dx, dy));
    }

    Status
    Translate(INT dx, INT dy)
    {
        return SetStatus(DllExports::GdipTranslateRegionI(region, dx, dy));
    }

    Status
    Union(const Rect &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRectI(region, &rect, CombineModeUnion));
    }

    Status
    Union(const Region *region)
    {
        return SetStatus(
            DllExports::GdipCombineRegionRegion(this->region, region ? region->region : NULL, CombineModeUnion));
    }

    Status
    Union(const RectF &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRect(region, &rect, CombineModeUnion));
    }

    Status
    Union(const GraphicsPath *path)
    {
        return SetStatus(DllExports::GdipCombineRegionPath(region, path ? path->path : NULL, CombineModeUnion));
    }

    Status
    Xor(const GraphicsPath *path)
    {
        return SetStatus(DllExports::GdipCombineRegionPath(region, path ? path->path : NULL, CombineModeXor));
    }

    Status
    Xor(const RectF &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRect(region, &rect, CombineModeXor));
    }

    Status
    Xor(const Rect &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRectI(region, &rect, CombineModeXor));
    }

    Status
    Xor(const Region *region)
    {
        return SetStatus(
            DllExports::GdipCombineRegionRegion(this->region, region ? region->region : NULL, CombineModeXor));
    }

  private:
    mutable Status status;
    GpRegion *region;

    Status
    SetStatus(Status status) const
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

    Status
    GetStrokeCaps(LineCap *startCap, LineCap *endCap);

    LineJoin GetStrokeJoin(VOID);
    REAL GetWidthScale(VOID);

    Status
    SetBaseCap(LineCap baseCap);

    Status
    SetBaseInset(REAL inset);

    Status
    SetStrokeCap(LineCap strokeCap);

    Status
    SetStrokeCaps(LineCap startCap, LineCap endCap);

    Status
    SetStrokeJoin(LineJoin lineJoin);

    Status
    SetWidthScale(IN REAL widthScale);

  protected:
    CustomLineCap();
};

#endif /* _GDIPLUSHEADERS_H */
