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
    friend class TextureBrush;

    Image(IStream *stream, BOOL useEmbeddedColorManagement = FALSE) : nativeImage(NULL)
    {
        if (useEmbeddedColorManagement)
            lastStatus = DllExports::GdipLoadImageFromStreamICM(stream, &nativeImage);
        else
            lastStatus = DllExports::GdipLoadImageFromStream(stream, &nativeImage);
    }

    Image(const WCHAR *filename, BOOL useEmbeddedColorManagement = FALSE) : nativeImage(NULL)
    {
        if (useEmbeddedColorManagement)
            lastStatus = DllExports::GdipLoadImageFromFileICM(filename, &nativeImage);
        else
            lastStatus = DllExports::GdipLoadImageFromFile(filename, &nativeImage);
    }

    Image *
    Clone()
    {
        GpImage *cloneimage = NULL;
        SetStatus(DllExports::GdipCloneImage(nativeImage, &cloneimage));
        return new Image(cloneimage, lastStatus);
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
        if (allItems == NULL)
            return SetStatus(InvalidParameter);
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
#if 1
        return SetStatus(NotImplemented);
#else
        return SetStatus(DllExports::GdipGetEncoderParameterList(nativeImage, clsidEncoder, size, buffer));
#endif
    }

    UINT
    GetEncoderParameterListSize(const CLSID *clsidEncoder)
    {
        UINT size = 0;
        SetStatus(DllExports::GdipGetEncoderParameterListSize(nativeImage, clsidEncoder, &size));
        return size;
    }

    UINT
    GetFlags()
    {
        UINT flags = 0;
        SetStatus(DllExports::GdipGetImageFlags(nativeImage, &flags));
        return flags;
    }

    UINT
    GetFrameCount(const GUID *dimensionID)
    {
        UINT count = 0;
        SetStatus(DllExports::GdipImageGetFrameCount(nativeImage, dimensionID, &count));
        return count;
    }

    UINT
    GetFrameDimensionsCount()
    {
        UINT count = 0;
        SetStatus(DllExports::GdipImageGetFrameDimensionsCount(nativeImage, &count));
        return count;
    }

    Status
    GetFrameDimensionsList(GUID *dimensionIDs, UINT count)
    {
        return SetStatus(DllExports::GdipImageGetFrameDimensionsList(nativeImage, dimensionIDs, count));
    }

    UINT
    GetHeight()
    {
        UINT height = 0;
        SetStatus(DllExports::GdipGetImageHeight(nativeImage, &height));
        return height;
    }

    REAL
    GetHorizontalResolution()
    {
        REAL resolution = 0.0f;
        SetStatus(DllExports::GdipGetImageHorizontalResolution(nativeImage, &resolution));
        return resolution;
    }

    Status
    GetLastStatus()
    {
        return lastStatus;
    }

    Status
    GetPalette(ColorPalette *palette, INT size)
    {
        return SetStatus(DllExports::GdipGetImagePalette(nativeImage, palette, size));
    }

    INT
    GetPaletteSize()
    {
        INT size = 0;
        SetStatus(DllExports::GdipGetImagePaletteSize(nativeImage, &size));
        return size;
    }

    Status
    GetPhysicalDimension(SizeF *size)
    {
        if (size == NULL)
            return SetStatus(InvalidParameter);

        return SetStatus(DllExports::GdipGetImageDimension(nativeImage, &size->Width, &size->Height));
    }

    PixelFormat
    GetPixelFormat()
    {
        PixelFormat format;
        SetStatus(DllExports::GdipGetImagePixelFormat(nativeImage, &format));
        return format;
    }

    UINT
    GetPropertyCount()
    {
        UINT numOfProperty = 0;
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
        UINT size = 0;
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
        GpImage *thumbImage = NULL;
        SetStatus(DllExports::GdipGetImageThumbnail(
            nativeImage, thumbWidth, thumbHeight, &thumbImage, callback, callbackData));
        Image *newImage = new Image(thumbImage, lastStatus);
        if (newImage == NULL)
        {
            DllExports::GdipDisposeImage(thumbImage);
        }
        return newImage;
    }

    ImageType
    GetType()
    {
        ImageType type;
        SetStatus(DllExports::GdipGetImageType(nativeImage, &type));
        return type;
    }

    REAL
    GetVerticalResolution()
    {
        REAL resolution = 0.0f;
        SetStatus(DllExports::GdipGetImageVerticalResolution(nativeImage, &resolution));
        return resolution;
    }

    UINT
    GetWidth()
    {
        UINT width = 0;
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
        return SetStatus(DllExports::GdipSaveAdd(nativeImage, encoderParams));
    }

    Status
    SaveAdd(Image *newImage, const EncoderParameters *encoderParams)
    {
#if 1
        // FIXME: Not available yet
        return SetStatus(NotImplemented);
#else
        if (!newImage)
            return SetStatus(InvalidParameter);

        return SetStatus(DllExports::GdipSaveAddImage(nativeImage, getNat(newImage), encoderParams));
#endif
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

#if 0
    ImageLayout
    GetLayout() const
    {
        return SetStatus(NotImplemented);
    }

    Status
    SetLayout(const ImageLayout layout)
    {
        return SetStatus(NotImplemented);
    }
#endif

  protected:
    GpImage *nativeImage;
    mutable Status lastStatus;

    Image()
    {
    }

    Image(GpImage *image, Status status) : nativeImage(image), lastStatus(status)
    {
    }

    Status
    SetStatus(Status status) const
    {
        if (status != Ok)
            lastStatus = status;
        return status;
    }

    void
    SetNativeImage(GpImage *image)
    {
        nativeImage = image;
    }

  private:
    // Image is not copyable
    Image(const Image &);
    Image &
    operator=(const Image &);

    // get native
    friend inline GpImage *&
    getNat(const Image *image)
    {
        return const_cast<Image *>(image)->nativeImage;
    }
};

class Bitmap : public Image
{
    friend class CachedBitmap;

  public:
    // Bitmap(IDirectDrawSurface7 *surface)  // <-- FIXME: compiler does not like this
    // {
    //   lastStatus = DllExports::GdipCreateBitmapFromDirectDrawSurface(surface, &bitmap);
    // }

    Bitmap(INT width, INT height, Graphics *target)
    {
        GpBitmap *bitmap = NULL;
        lastStatus = DllExports::GdipCreateBitmapFromGraphics(width, height, target ? getNat(target) : NULL, &bitmap);
        SetNativeImage(bitmap);
    }

    Bitmap(const BITMAPINFO *gdiBitmapInfo, VOID *gdiBitmapData)
    {
        GpBitmap *bitmap = NULL;
        lastStatus = DllExports::GdipCreateBitmapFromGdiDib(gdiBitmapInfo, gdiBitmapData, &bitmap);
        SetNativeImage(bitmap);
    }

    Bitmap(INT width, INT height, PixelFormat format)
    {
        GpBitmap *bitmap = NULL;
        lastStatus = DllExports::GdipCreateBitmapFromScan0(width, height, 0, format, NULL, &bitmap);
        SetNativeImage(bitmap);
    }

    Bitmap(HBITMAP hbm, HPALETTE hpal)
    {
        GpBitmap *bitmap = NULL;
        lastStatus = DllExports::GdipCreateBitmapFromHBITMAP(hbm, hpal, &bitmap);
        SetNativeImage(bitmap);
    }

    Bitmap(INT width, INT height, INT stride, PixelFormat format, BYTE *scan0)
    {
        GpBitmap *bitmap = NULL;
        lastStatus = DllExports::GdipCreateBitmapFromScan0(width, height, stride, format, scan0, &bitmap);
        SetNativeImage(bitmap);
    }

    Bitmap(const WCHAR *filename, BOOL useIcm)
    {
        GpBitmap *bitmap = NULL;

        if (useIcm)
            lastStatus = DllExports::GdipCreateBitmapFromFileICM(filename, &bitmap);
        else
            lastStatus = DllExports::GdipCreateBitmapFromFile(filename, &bitmap);

        SetNativeImage(bitmap);
    }

    Bitmap(HINSTANCE hInstance, const WCHAR *bitmapName)
    {
        GpBitmap *bitmap = NULL;
        lastStatus = DllExports::GdipCreateBitmapFromResource(hInstance, bitmapName, &bitmap);
        SetNativeImage(bitmap);
    }

    Bitmap(HICON hicon)
    {
        GpBitmap *bitmap = NULL;
        lastStatus = DllExports::GdipCreateBitmapFromHICON(hicon, &bitmap);
        SetNativeImage(bitmap);
    }

    Bitmap(IStream *stream, BOOL useIcm)
    {
        GpBitmap *bitmap = NULL;
        if (useIcm)
            lastStatus = DllExports::GdipCreateBitmapFromStreamICM(stream, &bitmap);
        else
            lastStatus = DllExports::GdipCreateBitmapFromStream(stream, &bitmap);
        SetNativeImage(bitmap);
    }

    Bitmap *
    Clone(const Rect &rect, PixelFormat format)
    {
        return Clone(rect.X, rect.Y, rect.Width, rect.Height, format);
    }

    Bitmap *
    Clone(const RectF &rect, PixelFormat format)
    {
        return Clone(rect.X, rect.Y, rect.Width, rect.Height, format);
    }

    Bitmap *
    Clone(REAL x, REAL y, REAL width, REAL height, PixelFormat format)
    {
        GpBitmap *bitmap = NULL;
        lastStatus = DllExports::GdipCloneBitmapArea(x, y, width, height, format, GetNativeBitmap(), &bitmap);

        if (lastStatus != Ok)
            return NULL;

        Bitmap *newBitmap = new Bitmap(bitmap);
        if (newBitmap == NULL)
        {
            DllExports::GdipDisposeImage(bitmap);
        }

        return newBitmap;
    }

    Bitmap *
    Clone(INT x, INT y, INT width, INT height, PixelFormat format)
    {
        GpBitmap *bitmap = NULL;
        lastStatus = DllExports::GdipCloneBitmapAreaI(x, y, width, height, format, GetNativeBitmap(), &bitmap);

        if (lastStatus != Ok)
            return NULL;

        Bitmap *newBitmap = new Bitmap(bitmap);
        if (newBitmap == NULL)
        {
            DllExports::GdipDisposeImage(bitmap);
        }

        return newBitmap;
    }

    static Bitmap *
    FromBITMAPINFO(const BITMAPINFO *gdiBitmapInfo, VOID *gdiBitmapData)
    {
        return new Bitmap(gdiBitmapInfo, gdiBitmapData);
    }

    // static Bitmap *FromDirectDrawSurface7(IDirectDrawSurface7 *surface)  // <-- FIXME: compiler does not like this
    // {
    //   return new Bitmap(surface);
    // }

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
        return SetStatus(
            DllExports::GdipCreateHBITMAPFromBitmap(GetNativeBitmap(), hbmReturn, colorBackground.GetValue()));
    }

    Status
    GetHICON(HICON *hicon)
    {
        return SetStatus(DllExports::GdipCreateHICONFromBitmap(GetNativeBitmap(), hicon));
    }

    Status
    GetPixel(INT x, INT y, Color *color)
    {
        ARGB argb;
        Status s = SetStatus(DllExports::GdipBitmapGetPixel(GetNativeBitmap(), x, y, &argb));
        if (color)
            color->SetValue(argb);
        return s;
    }

    Status
    LockBits(const Rect *rect, UINT flags, PixelFormat format, BitmapData *lockedBitmapData)
    {
        return SetStatus(DllExports::GdipBitmapLockBits(GetNativeBitmap(), rect, flags, format, lockedBitmapData));
    }

    Status
    SetPixel(INT x, INT y, const Color &color)
    {
        return SetStatus(DllExports::GdipBitmapSetPixel(GetNativeBitmap(), x, y, color.GetValue()));
    }

    Status
    SetResolution(REAL xdpi, REAL ydpi)
    {
        return SetStatus(DllExports::GdipBitmapSetResolution(GetNativeBitmap(), xdpi, ydpi));
    }

    Status
    UnlockBits(BitmapData *lockedBitmapData)
    {
        return SetStatus(DllExports::GdipBitmapUnlockBits(GetNativeBitmap(), lockedBitmapData));
    }

  protected:
    Bitmap()
    {
    }

    Bitmap(GpBitmap *nativeBitmap)
    {
        lastStatus = Ok;
        SetNativeImage(nativeBitmap);
    }

    GpBitmap *
    GetNativeBitmap() const
    {
        return static_cast<GpBitmap *>(nativeImage);
    }
};

class CachedBitmap : public GdiplusBase
{
  public:
    CachedBitmap(Bitmap *bitmap, Graphics *graphics)
    {
        nativeCachedBitmap = NULL;
        lastStatus = DllExports::GdipCreateCachedBitmap(
            bitmap->GetNativeBitmap(), graphics ? getNat(graphics) : NULL, &nativeCachedBitmap);
    }

    ~CachedBitmap()
    {
        DllExports::GdipDeleteCachedBitmap(nativeCachedBitmap);
    }

    Status
    GetLastStatus()
    {
        return lastStatus;
    }

  protected:
    mutable Status lastStatus;
    GpCachedBitmap *nativeCachedBitmap;

  private:
    // CachedBitmap is not copyable
    CachedBitmap(const CachedBitmap &);
    CachedBitmap &
    operator=(const CachedBitmap &);

    // get native
    friend inline GpCachedBitmap *&
    getNat(const CachedBitmap *cb)
    {
        return const_cast<CachedBitmap *>(cb)->nativeCachedBitmap;
    }
};

class FontCollection : public GdiplusBase
{
    friend class FontFamily;

  public:
    FontCollection() : nativeFontCollection(NULL), lastStatus(Ok)
    {
    }

    virtual ~FontCollection()
    {
    }

    Status
    GetFamilies(INT numSought, FontFamily *gpfamilies, INT *numFound) const
    {
        return SetStatus(NotImplemented);
    }

    INT
    GetFamilyCount() const
    {
        INT numFound = 0;
        lastStatus = DllExports::GdipGetFontCollectionFamilyCount(nativeFontCollection, &numFound);
        return numFound;
    }

    Status
    GetLastStatus() const
    {
        return lastStatus;
    }

  protected:
    GpFontCollection *nativeFontCollection;
    mutable Status lastStatus;

    Status
    SetStatus(Status status) const
    {
        if (status != Ok)
            lastStatus = status;
        return status;
    }

  private:
    // FontCollection is not copyable
    FontCollection(const FontCollection &);
    FontCollection &
    operator=(const FontCollection &);

    // get native
    friend inline GpFontCollection *&
    getNat(const FontCollection *fc)
    {
        return const_cast<FontCollection *>(fc)->nativeFontCollection;
    }
};

class FontFamily : public GdiplusBase
{
    friend class Font;

  public:
    FontFamily()
    {
    }

    FontFamily(const WCHAR *name, const FontCollection *fontCollection)
    {
        GpFontCollection *theCollection = fontCollection ? getNat(fontCollection) : NULL;
        status = DllExports::GdipCreateFontFamilyFromName(name, theCollection, &fontFamily);
    }

    FontFamily *
    Clone()
    {
        return NULL;
    }

    static const FontFamily *
    GenericMonospace()
    {
        FontFamily *genericMonospace = new FontFamily();
        genericMonospace->status =
            DllExports::GdipGetGenericFontFamilyMonospace(genericMonospace ? &genericMonospace->fontFamily : NULL);
        return genericMonospace;
    }

    static const FontFamily *
    GenericSansSerif()
    {
        FontFamily *genericSansSerif = new FontFamily();
        genericSansSerif->status =
            DllExports::GdipGetGenericFontFamilySansSerif(genericSansSerif ? &genericSansSerif->fontFamily : NULL);
        return genericSansSerif;
    }

    static const FontFamily *
    GenericSerif()
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

    Status
    GetLastStatus() const
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

    BOOL
    IsAvailable() const
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

    // get native
    friend inline GpFontFamily *&
    getNat(const FontFamily *ff)
    {
        return const_cast<FontFamily *>(ff)->fontFamily;
    }
};

class InstalledFontFamily : public FontFamily
{
  public:
    InstalledFontFamily()
    {
    }
};

class PrivateFontCollection : public FontCollection
{
  public:
    PrivateFontCollection()
    {
        nativeFontCollection = NULL;
        lastStatus = DllExports::GdipNewPrivateFontCollection(&nativeFontCollection);
    }

    virtual ~PrivateFontCollection()
    {
        DllExports::GdipDeletePrivateFontCollection(&nativeFontCollection);
    }

    Status
    AddFontFile(const WCHAR *filename)
    {
        return SetStatus(DllExports::GdipPrivateAddFontFile(nativeFontCollection, filename));
    }

    Status
    AddMemoryFont(const VOID *memory, INT length)
    {
        return SetStatus(DllExports::GdipPrivateAddMemoryFont(nativeFontCollection, memory, length));
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

    Font *
    Clone() const
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
        SetStatus(DllExports::GdipGetFontHeight(font, graphics ? getNat(graphics) : NULL, &height));
        return height;
    }

    REAL
    GetHeight(REAL dpi) const
    {
        REAL height;
        SetStatus(DllExports::GdipGetFontHeightGivenDPI(font, dpi, &height));
        return height;
    }

    Status
    GetLastStatus() const
    {
        return status;
    }

    Status
    GetLogFontA(const Graphics *g, LOGFONTA *logfontA) const
    {
        return SetStatus(DllExports::GdipGetLogFontA(font, g ? getNat(g) : NULL, logfontA));
    }

    Status
    GetLogFontW(const Graphics *g, LOGFONTW *logfontW) const
    {
        return SetStatus(DllExports::GdipGetLogFontW(font, g ? getNat(g) : NULL, logfontW));
    }

    REAL
    GetSize() const
    {
        REAL size;
        SetStatus(DllExports::GdipGetFontSize(font, &size));
        return size;
    }

    INT
    GetStyle() const
    {
        INT style;
        SetStatus(DllExports::GdipGetFontStyle(font, &style));
        return style;
    }

    Unit
    GetUnit() const
    {
        Unit unit;
        SetStatus(DllExports::GdipGetFontUnit(font, &unit));
        return unit;
    }

    BOOL
    IsAvailable() const
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

    // get native
    friend inline GpFont *&
    getNat(const Font *font)
    {
        return const_cast<Font *>(font)->font;
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
        lastStatus = DllExports::GdipCreateRegionRectI(&rect, &nativeRegion);
    }

    Region()
    {
        lastStatus = DllExports::GdipCreateRegion(&nativeRegion);
    }

    Region(const BYTE *regionData, INT size)
    {
        lastStatus = DllExports::GdipCreateRegionRgnData(regionData, size, &nativeRegion);
    }

    Region(const GraphicsPath *path)
    {
        lastStatus = DllExports::GdipCreateRegionPath(getNat(path), &nativeRegion);
    }

    Region(HRGN hRgn)
    {
        lastStatus = DllExports::GdipCreateRegionHrgn(hRgn, &nativeRegion);
    }

    Region(const RectF &rect)
    {
        lastStatus = DllExports::GdipCreateRegionRect(&rect, &nativeRegion);
    }

    Region *
    Clone()
    {
        Region *cloneRegion = new Region();
        cloneRegion->lastStatus =
            DllExports::GdipCloneRegion(nativeRegion, cloneRegion ? &cloneRegion->nativeRegion : NULL);
        return cloneRegion;
    }

    Status
    Complement(const GraphicsPath *path)
    {
        GpPath *thePath = path ? getNat(path) : NULL;
        return SetStatus(DllExports::GdipCombineRegionPath(nativeRegion, thePath, CombineModeComplement));
    }

    Status
    Complement(const Region *region)
    {
        GpRegion *theRegion = region ? getNat(region) : NULL;
        return SetStatus(DllExports::GdipCombineRegionRegion(nativeRegion, theRegion, CombineModeComplement));
    }

    Status
    Complement(const Rect &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRectI(nativeRegion, &rect, CombineModeComplement));
    }

    Status
    Complement(const RectF &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRect(nativeRegion, &rect, CombineModeComplement));
    }

    BOOL
    Equals(const Region *region, const Graphics *g) const
    {
        BOOL result;
        SetStatus(
            DllExports::GdipIsEqualRegion(nativeRegion, region ? getNat(region) : NULL, g ? getNat(g) : NULL, &result));
        return result;
    }

    Status
    Exclude(const GraphicsPath *path)
    {
        return SetStatus(
            DllExports::GdipCombineRegionPath(nativeRegion, path ? getNat(path) : NULL, CombineModeExclude));
    }

    Status
    Exclude(const RectF &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRect(nativeRegion, &rect, CombineModeExclude));
    }

    Status
    Exclude(const Rect &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRectI(nativeRegion, &rect, CombineModeExclude));
    }

    Status
    Exclude(const Region *region)
    {
        return SetStatus(
            DllExports::GdipCombineRegionRegion(nativeRegion, region ? getNat(region) : NULL, CombineModeExclude));
    }

    static Region *
    FromHRGN(HRGN hRgn)
    {
        return new Region(hRgn);
    }

    Status
    GetBounds(Rect *rect, const Graphics *g) const
    {
        return SetStatus(DllExports::GdipGetRegionBoundsI(nativeRegion, g ? getNat(g) : NULL, rect));
    }

    Status
    GetBounds(RectF *rect, const Graphics *g) const
    {
        return SetStatus(DllExports::GdipGetRegionBounds(nativeRegion, g ? getNat(g) : NULL, rect));
    }

    Status
    GetData(BYTE *buffer, UINT bufferSize, UINT *sizeFilled) const
    {
        return SetStatus(DllExports::GdipGetRegionData(nativeRegion, buffer, bufferSize, sizeFilled));
    }

    UINT
    GetDataSize() const
    {
        UINT bufferSize;
        SetStatus(DllExports::GdipGetRegionDataSize(nativeRegion, &bufferSize));
        return bufferSize;
    }

    HRGN
    GetHRGN(const Graphics *g) const
    {
        HRGN hRgn;
        SetStatus(DllExports::GdipGetRegionHRgn(nativeRegion, g ? getNat(g) : NULL, &hRgn));
        return hRgn;
    }

    Status
    GetLastStatus()
    {
        return lastStatus;
    }

    Status
    GetRegionScans(const Matrix *matrix, Rect *rects, INT *count) const
    {
        return SetStatus(DllExports::GdipGetRegionScansI(nativeRegion, rects, count, matrix ? getNat(matrix) : NULL));
    }

    Status
    GetRegionScans(const Matrix *matrix, RectF *rects, INT *count) const
    {
        return SetStatus(DllExports::GdipGetRegionScans(nativeRegion, rects, count, matrix ? getNat(matrix) : NULL));
    }

    UINT
    GetRegionScansCount(const Matrix *matrix) const
    {
        UINT count;
        SetStatus(DllExports::GdipGetRegionScansCount(nativeRegion, &count, matrix ? getNat(matrix) : NULL));
        return count;
    }

    Status
    Intersect(const Rect &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRectI(nativeRegion, &rect, CombineModeIntersect));
    }

    Status
    Intersect(const GraphicsPath *path)
    {
        GpPath *thePath = path ? getNat(path) : NULL;
        return SetStatus(DllExports::GdipCombineRegionPath(nativeRegion, thePath, CombineModeIntersect));
    }

    Status
    Intersect(const RectF &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRect(nativeRegion, &rect, CombineModeIntersect));
    }

    Status
    Intersect(const Region *region)
    {
        return SetStatus(
            DllExports::GdipCombineRegionRegion(nativeRegion, region ? getNat(region) : NULL, CombineModeIntersect));
    }

    BOOL
    IsEmpty(const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsEmptyRegion(nativeRegion, g ? getNat(g) : NULL, &result));
        return result;
    }

    BOOL
    IsInfinite(const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsInfiniteRegion(nativeRegion, g ? getNat(g) : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(const PointF &point, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRegionPoint(nativeRegion, point.X, point.Y, g ? getNat(g) : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(const RectF &rect, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRegionRect(
            nativeRegion, rect.X, rect.Y, rect.Width, rect.Height, g ? getNat(g) : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(const Rect &rect, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRegionRectI(
            nativeRegion, rect.X, rect.Y, rect.Width, rect.Height, g ? getNat(g) : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(INT x, INT y, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRegionPointI(nativeRegion, x, y, g ? getNat(g) : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(REAL x, REAL y, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRegionPoint(nativeRegion, x, y, g ? getNat(g) : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(INT x, INT y, INT width, INT height, const Graphics *g) const
    {
        BOOL result;
        SetStatus(
            DllExports::GdipIsVisibleRegionRectI(nativeRegion, x, y, width, height, g ? getNat(g) : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(const Point &point, const Graphics *g) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRegionPointI(nativeRegion, point.X, point.Y, g ? getNat(g) : NULL, &result));
        return result;
    }

    BOOL
    IsVisible(REAL x, REAL y, REAL width, REAL height, const Graphics *g) const
    {
        BOOL result;
        SetStatus(
            DllExports::GdipIsVisibleRegionRect(nativeRegion, x, y, width, height, g ? getNat(g) : NULL, &result));
        return result;
    }

    Status
    MakeEmpty()
    {
        return SetStatus(DllExports::GdipSetEmpty(nativeRegion));
    }

    Status
    MakeInfinite()
    {
        return SetStatus(DllExports::GdipSetInfinite(nativeRegion));
    }

    Status
    Transform(const Matrix *matrix)
    {
        return SetStatus(DllExports::GdipTransformRegion(nativeRegion, matrix ? getNat(matrix) : NULL));
    }

    Status
    Translate(REAL dx, REAL dy)
    {
        return SetStatus(DllExports::GdipTranslateRegion(nativeRegion, dx, dy));
    }

    Status
    Translate(INT dx, INT dy)
    {
        return SetStatus(DllExports::GdipTranslateRegionI(nativeRegion, dx, dy));
    }

    Status
    Union(const Rect &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRectI(nativeRegion, &rect, CombineModeUnion));
    }

    Status
    Union(const Region *region)
    {
        return SetStatus(
            DllExports::GdipCombineRegionRegion(nativeRegion, region ? getNat(region) : NULL, CombineModeUnion));
    }

    Status
    Union(const RectF &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRect(nativeRegion, &rect, CombineModeUnion));
    }

    Status
    Union(const GraphicsPath *path)
    {
        return SetStatus(DllExports::GdipCombineRegionPath(nativeRegion, path ? getNat(path) : NULL, CombineModeUnion));
    }

    Status
    Xor(const GraphicsPath *path)
    {
        return SetStatus(DllExports::GdipCombineRegionPath(nativeRegion, path ? getNat(path) : NULL, CombineModeXor));
    }

    Status
    Xor(const RectF &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRect(nativeRegion, &rect, CombineModeXor));
    }

    Status
    Xor(const Rect &rect)
    {
        return SetStatus(DllExports::GdipCombineRegionRectI(nativeRegion, &rect, CombineModeXor));
    }

    Status
    Xor(const Region *region)
    {
        return SetStatus(
            DllExports::GdipCombineRegionRegion(nativeRegion, region ? getNat(region) : NULL, CombineModeXor));
    }

  private:
    GpRegion *nativeRegion;
    mutable Status lastStatus;

    Status
    SetStatus(Status status) const
    {
        if (status != Ok)
            lastStatus = status;
        return status;
    }

    // get native
    friend inline GpRegion *&
    getNat(const Region *region)
    {
        return const_cast<Region *>(region)->nativeRegion;
    }
};

class CustomLineCap : public GdiplusBase
{
  public:
    CustomLineCap(const GraphicsPath *fillPath, const GraphicsPath *strokePath, LineCap baseCap, REAL baseInset = 0);

    ~CustomLineCap();

    CustomLineCap *
    Clone();

    LineCap
    GetBaseCap();

    REAL
    GetBaseInset();

    Status
    GetLastStatus();

    Status
    GetStrokeCaps(LineCap *startCap, LineCap *endCap);

    LineJoin
    GetStrokeJoin();

    REAL
    GetWidthScale();

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
    GpCustomLineCap *nativeCap;
    mutable Status lastStatus;

    CustomLineCap() : nativeCap(NULL), lastStatus(Ok)
    {
    }

    CustomLineCap(GpCustomLineCap *cap, Status status) : nativeCap(cap), lastStatus(status)
    {
    }

    void
    SetNativeCap(GpCustomLineCap *cap)
    {
        nativeCap = cap;
    }

    Status
    SetStatus(Status status) const
    {
        if (status == Ok)
            lastStatus = status;
        return status;
    }

  private:
    // CustomLineCap is not copyable
    CustomLineCap(const CustomLineCap &);
    CustomLineCap &
    operator=(const CustomLineCap &);

    // get native
    friend inline GpCustomLineCap *&
    getNat(const CustomLineCap *cap)
    {
        return const_cast<CustomLineCap *>(cap)->nativeCap;
    }
};

inline Image *
TextureBrush::GetImage() const
{
    GpImage *image = NULL;
    GpTexture *texture = GetNativeTexture();
    SetStatus(DllExports::GdipGetTextureImage(texture, &image));
    if (lastStatus != Ok)
        return NULL;

    Image *newImage = new Image(image, lastStatus);
    if (!newImage)
        DllExports::GdipDisposeImage(image);
    return newImage;
}

#endif /* _GDIPLUSHEADERS_H */
