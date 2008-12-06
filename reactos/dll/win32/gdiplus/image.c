/*
 * Copyright (C) 2007 Google (Evan Stade)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"

#define COBJMACROS
#include "objbase.h"
#include "olectl.h"
#include "ole2.h"

#include "initguid.h"
#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

#define PIXELFORMATBPP(x) ((x) ? ((x) >> 8) & 255 : 24)

static INT ipicture_pixel_height(IPicture *pic)
{
    HDC hdcref;
    OLE_YSIZE_HIMETRIC y;

    IPicture_get_Height(pic, &y);

    hdcref = GetDC(0);

    y = MulDiv(y, GetDeviceCaps(hdcref, LOGPIXELSY), INCH_HIMETRIC);
    ReleaseDC(0, hdcref);

    return y;
}

static INT ipicture_pixel_width(IPicture *pic)
{
    HDC hdcref;
    OLE_XSIZE_HIMETRIC x;

    IPicture_get_Width(pic, &x);

    hdcref = GetDC(0);

    x = MulDiv(x, GetDeviceCaps(hdcref, LOGPIXELSX), INCH_HIMETRIC);

    ReleaseDC(0, hdcref);

    return x;
}

GpStatus WINGDIPAPI GdipBitmapGetPixel(GpBitmap* bitmap, INT x, INT y,
    ARGB *color)
{
    static int calls;
    TRACE("%p %d %d %p\n", bitmap, x, y, color);

    if(!bitmap || !color)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    *color = 0xdeadbeef;

    return NotImplemented;
}

/* This function returns a pointer to an array of pixels that represents the
 * bitmap. The *entire* bitmap is locked according to the lock mode specified by
 * flags.  It is correct behavior that a user who calls this function with write
 * privileges can write to the whole bitmap (not just the area in rect).
 *
 * FIXME: only used portion of format is bits per pixel. */
GpStatus WINGDIPAPI GdipBitmapLockBits(GpBitmap* bitmap, GDIPCONST GpRect* rect,
    UINT flags, PixelFormat format, BitmapData* lockeddata)
{
    BOOL bm_is_selected;
    INT stride, bitspp = PIXELFORMATBPP(format);
    HDC hdc;
    HBITMAP hbm, old = NULL;
    BITMAPINFO *pbmi;
    BYTE *buff = NULL;
    UINT abs_height;
    GpRect act_rect; /* actual rect to be used */

    TRACE("%p %p %d %d %p\n", bitmap, rect, flags, format, lockeddata);

    if(!lockeddata || !bitmap)
        return InvalidParameter;

    if(rect){
        if(rect->X < 0 || rect->Y < 0 || (rect->X + rect->Width > bitmap->width) ||
          (rect->Y + rect->Height > bitmap->height) || !flags)
            return InvalidParameter;

        act_rect = *rect;
    }
    else{
        act_rect.X = act_rect.Y = 0;
        act_rect.Width  = bitmap->width;
        act_rect.Height = bitmap->height;
    }

    if(flags & ImageLockModeUserInputBuf)
        return NotImplemented;

    if(bitmap->lockmode)
        return WrongState;

    IPicture_get_Handle(bitmap->image.picture, (OLE_HANDLE*)&hbm);
    IPicture_get_CurDC(bitmap->image.picture, &hdc);
    bm_is_selected = (hdc != 0);

    pbmi = GdipAlloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    if (!pbmi)
        return OutOfMemory;
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biBitCount = 0;

    if(!bm_is_selected){
        hdc = CreateCompatibleDC(0);
        old = SelectObject(hdc, hbm);
    }

    /* fill out bmi */
    GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);

    abs_height = abs(pbmi->bmiHeader.biHeight);
    stride = pbmi->bmiHeader.biWidth * bitspp / 8;
    stride = (stride + 3) & ~3;

    buff = GdipAlloc(stride * abs_height);

    pbmi->bmiHeader.biBitCount = bitspp;

    if(buff)
        GetDIBits(hdc, hbm, 0, abs_height, buff, pbmi, DIB_RGB_COLORS);

    if(!bm_is_selected){
        SelectObject(hdc, old);
        DeleteDC(hdc);
    }

    if(!buff){
        GdipFree(pbmi);
        return OutOfMemory;
    }

    lockeddata->Width  = act_rect.Width;
    lockeddata->Height = act_rect.Height;
    lockeddata->PixelFormat = format;
    lockeddata->Reserved = flags;

    if(pbmi->bmiHeader.biHeight > 0){
        lockeddata->Stride = -stride;
        lockeddata->Scan0  = buff + (bitspp / 8) * act_rect.X +
                             stride * (abs_height - 1 - act_rect.Y);
    }
    else{
        lockeddata->Stride = stride;
        lockeddata->Scan0  = buff + (bitspp / 8) * act_rect.X + stride * act_rect.Y;
    }

    bitmap->lockmode = flags;
    bitmap->numlocks++;

    bitmap->bitmapbits = buff;

    GdipFree(pbmi);
    return Ok;
}

GpStatus WINGDIPAPI GdipBitmapUnlockBits(GpBitmap* bitmap,
    BitmapData* lockeddata)
{
    HDC hdc;
    HBITMAP hbm, old = NULL;
    BOOL bm_is_selected;
    BITMAPINFO *pbmi;

    if(!bitmap || !lockeddata)
        return InvalidParameter;

    if(!bitmap->lockmode)
        return WrongState;

    if(lockeddata->Reserved & ImageLockModeUserInputBuf)
        return NotImplemented;

    if(lockeddata->Reserved & ImageLockModeRead){
        if(!(--bitmap->numlocks))
            bitmap->lockmode = 0;

        GdipFree(bitmap->bitmapbits);
        bitmap->bitmapbits = NULL;
        return Ok;
    }

    IPicture_get_Handle(bitmap->image.picture, (OLE_HANDLE*)&hbm);
    IPicture_get_CurDC(bitmap->image.picture, &hdc);
    bm_is_selected = (hdc != 0);

    pbmi = GdipAlloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biBitCount = 0;

    if(!bm_is_selected){
        hdc = CreateCompatibleDC(0);
        old = SelectObject(hdc, hbm);
    }

    GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);
    pbmi->bmiHeader.biBitCount = PIXELFORMATBPP(lockeddata->PixelFormat);
    SetDIBits(hdc, hbm, 0, abs(pbmi->bmiHeader.biHeight),
              bitmap->bitmapbits, pbmi, DIB_RGB_COLORS);

    if(!bm_is_selected){
        SelectObject(hdc, old);
        DeleteDC(hdc);
    }

    GdipFree(pbmi);
    GdipFree(bitmap->bitmapbits);
    bitmap->bitmapbits = NULL;
    bitmap->lockmode = 0;

    return Ok;
}

GpStatus WINGDIPAPI GdipCloneImage(GpImage *image, GpImage **cloneImage)
{
    IStream* stream;
    HRESULT hr;
    INT size;
    LARGE_INTEGER move;
    GpStatus stat = GenericError;

    TRACE("%p, %p\n", image, cloneImage);

    if (!image || !cloneImage)
        return InvalidParameter;

    hr = CreateStreamOnHGlobal(0, TRUE, &stream);
    if (FAILED(hr))
        return GenericError;

    hr = IPicture_SaveAsFile(image->picture, stream, FALSE, &size);
    if(FAILED(hr))
    {
        WARN("Failed to save image on stream\n");
        goto out;
    }

    /* Set seek pointer back to the beginning of the picture */
    move.QuadPart = 0;
    hr = IStream_Seek(stream, move, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        goto out;

    stat = GdipLoadImageFromStream(stream, cloneImage);
    if (stat != Ok) WARN("Failed to load image from stream\n");

out:
    IStream_Release(stream);
    return stat;
}

GpStatus WINGDIPAPI GdipCreateBitmapFromFile(GDIPCONST WCHAR* filename,
    GpBitmap **bitmap)
{
    GpStatus stat;
    IStream *stream;

    TRACE("(%s) %p\n", debugstr_w(filename), bitmap);

    if(!filename || !bitmap)
        return InvalidParameter;

    stat = GdipCreateStreamOnFile(filename, GENERIC_READ, &stream);

    if(stat != Ok)
        return stat;

    stat = GdipCreateBitmapFromStream(stream, bitmap);

    IStream_Release(stream);

    return stat;
}

GpStatus WINGDIPAPI GdipCreateBitmapFromGdiDib(GDIPCONST BITMAPINFO* info,
                                               VOID *bits, GpBitmap **bitmap)
{
    DWORD height, stride;
    PixelFormat format;

    FIXME("(%p, %p, %p) - partially implemented\n", info, bits, bitmap);

    height = abs(info->bmiHeader.biHeight);
    stride = ((info->bmiHeader.biWidth * info->bmiHeader.biBitCount + 31) >> 3) & ~3;

    if(info->bmiHeader.biHeight > 0) /* bottom-up */
    {
        bits = (BYTE*)bits + (height - 1) * stride;
        stride = -stride;
    }

    switch(info->bmiHeader.biBitCount) {
    case 1:
        format = PixelFormat1bppIndexed;
        break;
    case 4:
        format = PixelFormat4bppIndexed;
        break;
    case 8:
        format = PixelFormat8bppIndexed;
        break;
    case 24:
        format = PixelFormat24bppRGB;
        break;
    default:
        FIXME("don't know how to handle %d bpp\n", info->bmiHeader.biBitCount);
        *bitmap = NULL;
        return InvalidParameter;
    }

    return GdipCreateBitmapFromScan0(info->bmiHeader.biWidth, height, stride, format,
                                     bits, bitmap);

}

/* FIXME: no icm */
GpStatus WINGDIPAPI GdipCreateBitmapFromFileICM(GDIPCONST WCHAR* filename,
    GpBitmap **bitmap)
{
    TRACE("(%s) %p\n", debugstr_w(filename), bitmap);

    return GdipCreateBitmapFromFile(filename, bitmap);
}

GpStatus WINGDIPAPI GdipCreateBitmapFromResource(HINSTANCE hInstance,
    GDIPCONST WCHAR* lpBitmapName, GpBitmap** bitmap)
{
    HBITMAP hbm;
    GpStatus stat = InvalidParameter;

    TRACE("%p (%s) %p\n", hInstance, debugstr_w(lpBitmapName), bitmap);

    if(!lpBitmapName || !bitmap)
        return InvalidParameter;

    /* load DIB */
    hbm = LoadImageW(hInstance, lpBitmapName, IMAGE_BITMAP, 0, 0,
                     LR_CREATEDIBSECTION);

    if(hbm){
        stat = GdipCreateBitmapFromHBITMAP(hbm, NULL, bitmap);
        DeleteObject(hbm);
    }

    return stat;
}

GpStatus WINGDIPAPI GdipCreateHBITMAPFromBitmap(GpBitmap* bitmap,
    HBITMAP* hbmReturn, ARGB background)
{
    FIXME("stub\n");

    hbmReturn = NULL;

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipConvertToEmfPlus(const GpGraphics* ref,
    GpMetafile* metafile, BOOL* succ, EmfType emfType,
    const WCHAR* description, GpMetafile** out_metafile)
{
    static int calls;

    if(!ref || !metafile || !out_metafile)
        return InvalidParameter;

    *succ = FALSE;
    *out_metafile = NULL;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

/* FIXME: this should create a bitmap in the given size with the attributes
 * (resolution etc.) of the graphics object */
GpStatus WINGDIPAPI GdipCreateBitmapFromGraphics(INT width, INT height,
    GpGraphics* target, GpBitmap** bitmap)
{
    static int calls;
    GpStatus ret;

    if(!target || !bitmap)
        return InvalidParameter;

    if(!(calls++))
        FIXME("hacked stub\n");

    ret = GdipCreateBitmapFromScan0(width, height, 0, PixelFormat24bppRGB,
                                    NULL, bitmap);

    return ret;
}

GpStatus WINGDIPAPI GdipCreateBitmapFromScan0(INT width, INT height, INT stride,
    PixelFormat format, BYTE* scan0, GpBitmap** bitmap)
{
    BITMAPFILEHEADER *bmfh;
    BITMAPINFOHEADER *bmih;
    BYTE *buff;
    INT datalen, size;
    IStream *stream;

    TRACE("%d %d %d %d %p %p\n", width, height, stride, format, scan0, bitmap);

    if (!bitmap) return InvalidParameter;

    if(width <= 0 || height <= 0 || (scan0 && (stride % 4))){
        *bitmap = NULL;
        return InvalidParameter;
    }

    if(scan0 && !stride)
        return InvalidParameter;

    /* FIXME: windows allows negative stride (reads backwards from scan0) */
    if(stride < 0){
        FIXME("negative stride\n");
        return InvalidParameter;
    }

    *bitmap = GdipAlloc(sizeof(GpBitmap));
    if(!*bitmap)    return OutOfMemory;

    if(stride == 0){
        stride = width * (PIXELFORMATBPP(format) / 8);
        stride = (stride + 3) & ~3;
    }

    datalen = abs(stride * height);
    size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + datalen;
    buff = GdipAlloc(size);
    if(!buff){
        GdipFree(*bitmap);
        return OutOfMemory;
    }

    bmfh = (BITMAPFILEHEADER*) buff;
    bmih = (BITMAPINFOHEADER*) (bmfh + 1);

    bmfh->bfType    = (((WORD)'M') << 8) + (WORD)'B';
    bmfh->bfSize    = size;
    bmfh->bfOffBits = size - datalen;

    bmih->biSize            = sizeof(BITMAPINFOHEADER);
    bmih->biWidth           = width;
    bmih->biHeight          = -height;
    /* FIXME: use the rest of the data from format */
    bmih->biBitCount        = PIXELFORMATBPP(format);
    bmih->biCompression     = BI_RGB;
    bmih->biSizeImage       = datalen;

    if(scan0)
        memcpy(bmih + 1, scan0, datalen);
    else
        memset(bmih + 1, 0, datalen);

    if(CreateStreamOnHGlobal(buff, TRUE, &stream) != S_OK){
        ERR("could not make stream\n");
        GdipFree(*bitmap);
        GdipFree(buff);
        return GenericError;
    }

    if(OleLoadPicture(stream, 0, FALSE, &IID_IPicture,
        (LPVOID*) &((*bitmap)->image.picture)) != S_OK){
        TRACE("Could not load picture\n");
        IStream_Release(stream);
        GdipFree(*bitmap);
        GdipFree(buff);
        return GenericError;
    }

    (*bitmap)->image.type = ImageTypeBitmap;
    (*bitmap)->image.flags = ImageFlagsNone;
    (*bitmap)->width = width;
    (*bitmap)->height = height;
    (*bitmap)->format = format;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateBitmapFromStream(IStream* stream,
    GpBitmap **bitmap)
{
    GpStatus stat;

    TRACE("%p %p\n", stream, bitmap);

    stat = GdipLoadImageFromStream(stream, (GpImage**) bitmap);

    if(stat != Ok)
        return stat;

    if((*bitmap)->image.type != ImageTypeBitmap){
        IPicture_Release((*bitmap)->image.picture);
        GdipFree(bitmap);
        return GenericError; /* FIXME: what error to return? */
    }

    return Ok;
}

/* FIXME: no icm */
GpStatus WINGDIPAPI GdipCreateBitmapFromStreamICM(IStream* stream,
    GpBitmap **bitmap)
{
    TRACE("%p %p\n", stream, bitmap);

    return GdipCreateBitmapFromStream(stream, bitmap);
}

GpStatus WINGDIPAPI GdipCreateCachedBitmap(GpBitmap *bitmap, GpGraphics *graphics,
    GpCachedBitmap **cachedbmp)
{
    GpStatus stat;

    TRACE("%p %p %p\n", bitmap, graphics, cachedbmp);

    if(!bitmap || !graphics || !cachedbmp)
        return InvalidParameter;

    *cachedbmp = GdipAlloc(sizeof(GpCachedBitmap));
    if(!*cachedbmp)
        return OutOfMemory;

    stat = GdipCloneImage(&(bitmap->image), &(*cachedbmp)->image);
    if(stat != Ok){
        GdipFree(*cachedbmp);
        return stat;
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteCachedBitmap(GpCachedBitmap *cachedbmp)
{
    TRACE("%p\n", cachedbmp);

    if(!cachedbmp)
        return InvalidParameter;

    GdipDisposeImage(cachedbmp->image);
    GdipFree(cachedbmp);

    return Ok;
}

GpStatus WINGDIPAPI GdipDrawCachedBitmap(GpGraphics *graphics,
    GpCachedBitmap *cachedbmp, INT x, INT y)
{
    TRACE("%p %p %d %d\n", graphics, cachedbmp, x, y);

    if(!graphics || !cachedbmp)
        return InvalidParameter;

    return GdipDrawImage(graphics, cachedbmp->image, (REAL)x, (REAL)y);
}

GpStatus WINGDIPAPI GdipDisposeImage(GpImage *image)
{
    HDC hdc;

    TRACE("%p\n", image);

    if(!image)
        return InvalidParameter;

    IPicture_get_CurDC(image->picture, &hdc);
    DeleteDC(hdc);
    IPicture_Release(image->picture);
    if (image->type == ImageTypeBitmap)
        GdipFree(((GpBitmap*)image)->bitmapbits);
    GdipFree(image);

    return Ok;
}

GpStatus WINGDIPAPI GdipFindFirstImageItem(GpImage *image, ImageItemData* item)
{
    if(!image || !item)
        return InvalidParameter;

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetImageBounds(GpImage *image, GpRectF *srcRect,
    GpUnit *srcUnit)
{
    TRACE("%p %p %p\n", image, srcRect, srcUnit);

    if(!image || !srcRect || !srcUnit)
        return InvalidParameter;
    if(image->type == ImageTypeMetafile){
        *srcRect = ((GpMetafile*)image)->bounds;
        *srcUnit = ((GpMetafile*)image)->unit;
    }
    else if(image->type == ImageTypeBitmap){
        srcRect->X = srcRect->Y = 0.0;
        srcRect->Width = (REAL) ((GpBitmap*)image)->width;
        srcRect->Height = (REAL) ((GpBitmap*)image)->height;
        *srcUnit = UnitPixel;
    }
    else{
        srcRect->X = srcRect->Y = 0.0;
        srcRect->Width = ipicture_pixel_width(image->picture);
        srcRect->Height = ipicture_pixel_height(image->picture);
        *srcUnit = UnitPixel;
    }

    TRACE("returning (%f, %f) (%f, %f) unit type %d\n", srcRect->X, srcRect->Y,
          srcRect->Width, srcRect->Height, *srcUnit);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageDimension(GpImage *image, REAL *width,
    REAL *height)
{
    TRACE("%p %p %p\n", image, width, height);

    if(!image || !height || !width)
        return InvalidParameter;

    if(image->type == ImageTypeMetafile){
        HDC hdc = GetDC(0);

        *height = convert_unit(hdc, ((GpMetafile*)image)->unit) *
                        ((GpMetafile*)image)->bounds.Height;

        *width = convert_unit(hdc, ((GpMetafile*)image)->unit) *
                        ((GpMetafile*)image)->bounds.Width;

        ReleaseDC(0, hdc);
    }

    else if(image->type == ImageTypeBitmap){
        *height = ((GpBitmap*)image)->height;
        *width = ((GpBitmap*)image)->width;
    }
    else{
        *height = ipicture_pixel_height(image->picture);
        *width = ipicture_pixel_width(image->picture);
    }

    TRACE("returning (%f, %f)\n", *height, *width);
    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageGraphicsContext(GpImage *image,
    GpGraphics **graphics)
{
    HDC hdc;

    TRACE("%p %p\n", image, graphics);

    if(!image || !graphics)
        return InvalidParameter;

    if(image->type != ImageTypeBitmap){
        FIXME("not implemented for image type %d\n", image->type);
        return NotImplemented;
    }

    IPicture_get_CurDC(image->picture, &hdc);

    if(!hdc){
        hdc = CreateCompatibleDC(0);
        IPicture_SelectPicture(image->picture, hdc, NULL, NULL);
    }

    return GdipCreateFromHDC(hdc, graphics);
}

GpStatus WINGDIPAPI GdipGetImageHeight(GpImage *image, UINT *height)
{
    TRACE("%p %p\n", image, height);

    if(!image || !height)
        return InvalidParameter;

    if(image->type == ImageTypeMetafile){
        HDC hdc = GetDC(0);

        *height = roundr(convert_unit(hdc, ((GpMetafile*)image)->unit) *
                        ((GpMetafile*)image)->bounds.Height);

        ReleaseDC(0, hdc);
    }
    else if(image->type == ImageTypeBitmap)
        *height = ((GpBitmap*)image)->height;
    else
        *height = ipicture_pixel_height(image->picture);

    TRACE("returning %d\n", *height);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageHorizontalResolution(GpImage *image, REAL *res)
{
    static int calls;

    if(!image || !res)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetImagePaletteSize(GpImage *image, INT *size)
{
    FIXME("%p %p\n", image, size);

    if(!image || !size)
        return InvalidParameter;

    return NotImplemented;
}

/* FIXME: test this function for non-bitmap types */
GpStatus WINGDIPAPI GdipGetImagePixelFormat(GpImage *image, PixelFormat *format)
{
    TRACE("%p %p\n", image, format);

    if(!image || !format)
        return InvalidParameter;

    if(image->type != ImageTypeBitmap)
        *format = PixelFormat24bppRGB;
    else
        *format = ((GpBitmap*) image)->format;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageRawFormat(GpImage *image, GUID *format)
{
    static int calls;

    if(!image || !format)
        return InvalidParameter;

    if(!(calls++))
        FIXME("stub\n");

    /* FIXME: should be detected from embedded picture or stored separately */
    switch (image->type)
    {
    case ImageTypeBitmap:   *format = ImageFormatBMP; break;
    case ImageTypeMetafile: *format = ImageFormatEMF; break;
    default:
        WARN("unknown type %u\n", image->type);
        *format = ImageFormatUndefined;
    }
    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageType(GpImage *image, ImageType *type)
{
    TRACE("%p %p\n", image, type);

    if(!image || !type)
        return InvalidParameter;

    *type = image->type;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageVerticalResolution(GpImage *image, REAL *res)
{
    static int calls;

    if(!image || !res)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetImageWidth(GpImage *image, UINT *width)
{
    TRACE("%p %p\n", image, width);

    if(!image || !width)
        return InvalidParameter;

    if(image->type == ImageTypeMetafile){
        HDC hdc = GetDC(0);

        *width = roundr(convert_unit(hdc, ((GpMetafile*)image)->unit) *
                        ((GpMetafile*)image)->bounds.Width);

        ReleaseDC(0, hdc);
    }
    else if(image->type == ImageTypeBitmap)
        *width = ((GpBitmap*)image)->width;
    else
        *width = ipicture_pixel_width(image->picture);

    TRACE("returning %d\n", *width);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetMetafileHeaderFromMetafile(GpMetafile * metafile,
    MetafileHeader * header)
{
    static int calls;

    if(!metafile || !header)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return Ok;
}

GpStatus WINGDIPAPI GdipGetAllPropertyItems(GpImage *image, UINT size,
    UINT num, PropertyItem* items)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return InvalidParameter;
}

GpStatus WINGDIPAPI GdipGetPropertyCount(GpImage *image, UINT* num)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return InvalidParameter;
}

GpStatus WINGDIPAPI GdipGetPropertyIdList(GpImage *image, UINT num, PROPID* list)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return InvalidParameter;
}

GpStatus WINGDIPAPI GdipGetPropertyItem(GpImage *image, PROPID id, UINT size,
    PropertyItem* buffer)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return InvalidParameter;
}

GpStatus WINGDIPAPI GdipGetPropertyItemSize(GpImage *image, PROPID pid,
    UINT* size)
{
    static int calls;

    TRACE("%p %x %p\n", image, pid, size);

    if(!size || !image)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetPropertySize(GpImage *image, UINT* size, UINT* num)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return InvalidParameter;
}

GpStatus WINGDIPAPI GdipImageGetFrameCount(GpImage *image,
    GDIPCONST GUID* dimensionID, UINT* count)
{
    static int calls;

    if(!image || !dimensionID || !count)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipImageGetFrameDimensionsCount(GpImage *image,
    UINT* count)
{
    if(!image || !count)
        return InvalidParameter;

    *count = 1;

    FIXME("stub\n");

    return Ok;
}

GpStatus WINGDIPAPI GdipImageGetFrameDimensionsList(GpImage* image,
    GUID* dimensionIDs, UINT count)
{
    static int calls;

    if(!image || !dimensionIDs)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return Ok;
}

GpStatus WINGDIPAPI GdipImageSelectActiveFrame(GpImage *image,
    GDIPCONST GUID* dimensionID, UINT frameidx)
{
    static int calls;

    if(!image || !dimensionID)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return Ok;
}

GpStatus WINGDIPAPI GdipLoadImageFromFile(GDIPCONST WCHAR* filename,
                                          GpImage **image)
{
    GpStatus stat;
    IStream *stream;

    TRACE("(%s) %p\n", debugstr_w(filename), image);

    if (!filename || !image)
        return InvalidParameter;

    stat = GdipCreateStreamOnFile(filename, GENERIC_READ, &stream);

    if (stat != Ok)
        return stat;

    stat = GdipLoadImageFromStream(stream, image);

    IStream_Release(stream);

    return stat;
}

/* FIXME: no icm handling */
GpStatus WINGDIPAPI GdipLoadImageFromFileICM(GDIPCONST WCHAR* filename,GpImage **image)
{
    TRACE("(%s) %p\n", debugstr_w(filename), image);

    return GdipLoadImageFromFile(filename, image);
}

GpStatus WINGDIPAPI GdipLoadImageFromStream(IStream* stream, GpImage **image)
{
    IPicture *pic;
    short type;

    TRACE("%p %p\n", stream, image);

    if(!stream || !image)
        return InvalidParameter;

    if(OleLoadPicture(stream, 0, FALSE, &IID_IPicture,
        (LPVOID*) &pic) != S_OK){
        TRACE("Could not load picture\n");
        return GenericError;
    }

    IPicture_get_Type(pic, &type);

    if(type == PICTYPE_BITMAP){
        BITMAPINFO *pbmi;
        BITMAPCOREHEADER* bmch;
        HBITMAP hbm;
        HDC hdc;

        pbmi = GdipAlloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
        if (!pbmi)
            return OutOfMemory;
        *image = GdipAlloc(sizeof(GpBitmap));
        if(!*image){
            GdipFree(pbmi);
            return OutOfMemory;
        }
        (*image)->type = ImageTypeBitmap;

        (*((GpBitmap**) image))->width = ipicture_pixel_width(pic);
        (*((GpBitmap**) image))->height = ipicture_pixel_height(pic);

        /* get the pixel format */
        IPicture_get_Handle(pic, (OLE_HANDLE*)&hbm);
        IPicture_get_CurDC(pic, &hdc);

        bmch = (BITMAPCOREHEADER*) (&pbmi->bmiHeader);
        bmch->bcSize = sizeof(BITMAPCOREHEADER);

        if(!hdc){
            HBITMAP old;
            hdc = CreateCompatibleDC(0);
            old = SelectObject(hdc, hbm);
            GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);
            SelectObject(hdc, old);
            DeleteDC(hdc);
        }
        else
            GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);

        (*((GpBitmap**) image))->format = (bmch->bcBitCount << 8) | PixelFormatGDI;
        GdipFree(pbmi);
    }
    else if(type == PICTYPE_METAFILE || type == PICTYPE_ENHMETAFILE){
        /* FIXME: missing initialization code */
        *image = GdipAlloc(sizeof(GpMetafile));
        if(!*image) return OutOfMemory;
        (*image)->type = ImageTypeMetafile;
    }
    else{
        *image = GdipAlloc(sizeof(GpImage));
        if(!*image) return OutOfMemory;
        (*image)->type = ImageTypeUnknown;
    }

    (*image)->picture = pic;
    (*image)->flags   = ImageFlagsNone;

    return Ok;
}

/* FIXME: no ICM */
GpStatus WINGDIPAPI GdipLoadImageFromStreamICM(IStream* stream, GpImage **image)
{
    TRACE("%p %p\n", stream, image);

    return GdipLoadImageFromStream(stream, image);
}

GpStatus WINGDIPAPI GdipRemovePropertyItem(GpImage *image, PROPID propId)
{
    static int calls;

    if(!image)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetPropertyItem(GpImage *image, GDIPCONST PropertyItem* item)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSaveImageToFile(GpImage *image, GDIPCONST WCHAR* filename,
                                        GDIPCONST CLSID *clsidEncoder,
                                        GDIPCONST EncoderParameters *encoderParams)
{
    GpStatus stat;
    IStream *stream;

    TRACE("%p (%s) %p %p\n", image, debugstr_w(filename), clsidEncoder, encoderParams);

    if (!image || !filename|| !clsidEncoder)
        return InvalidParameter;

    if (!(image->picture))
        return InvalidParameter;

    stat = GdipCreateStreamOnFile(filename, GENERIC_WRITE, &stream);
    if (stat != Ok)
        return GenericError;

    stat = GdipSaveImageToStream(image, stream, clsidEncoder, encoderParams);

    IStream_Release(stream);
    return stat;
}

/*************************************************************************
 * Encoding functions -
 *   These functions encode an image in different image file formats.
 */
#define BITMAP_FORMAT_BMP   0x4d42 /* "BM" */
#define BITMAP_FORMAT_JPEG  0xd8ff
#define BITMAP_FORMAT_GIF   0x4947
#define BITMAP_FORMAT_PNG   0x5089
#define BITMAP_FORMAT_APM   0xcdd7

static GpStatus encode_image_BMP(LPVOID bitmap_bits, LPBITMAPINFO bitmap_info,
                                 void **output, unsigned int *output_size)
{
    int num_palette_entries;
    BITMAPFILEHEADER *bmp_file_hdr;
    BITMAPINFO *bmp_info_hdr;

    if (bitmap_info->bmiHeader.biClrUsed) {
        num_palette_entries = bitmap_info->bmiHeader.biClrUsed;
        if (num_palette_entries > 256) num_palette_entries = 256;
    } else {
        if (bitmap_info->bmiHeader.biBitCount <= 8)
            num_palette_entries = 1 << bitmap_info->bmiHeader.biBitCount;
        else
            num_palette_entries = 0;
    }

    *output_size =
        sizeof(BITMAPFILEHEADER) +
        sizeof(BITMAPINFOHEADER) +
        num_palette_entries * sizeof(RGBQUAD) +
        bitmap_info->bmiHeader.biSizeImage;

    *output = GdipAlloc(*output_size);

    bmp_file_hdr = (BITMAPFILEHEADER*) *output;
    bmp_file_hdr->bfType = BITMAP_FORMAT_BMP;
    bmp_file_hdr->bfSize = *output_size;
    bmp_file_hdr->bfOffBits =
        sizeof(BITMAPFILEHEADER) +
        sizeof(BITMAPINFOHEADER) +
        num_palette_entries * sizeof (RGBQUAD);

    bmp_info_hdr = (BITMAPINFO*) ((unsigned char*)(*output) + sizeof(BITMAPFILEHEADER));
    memcpy(bmp_info_hdr, bitmap_info, sizeof(BITMAPINFOHEADER) + num_palette_entries * sizeof(RGBQUAD));
    memcpy((unsigned char *)(*output) +
           sizeof(BITMAPFILEHEADER) +
           sizeof(BITMAPINFOHEADER) +
           num_palette_entries * sizeof(RGBQUAD),
           bitmap_bits, bitmap_info->bmiHeader.biSizeImage);

    return Ok;
}

typedef GpStatus encode_image_func(LPVOID bitmap_bits, LPBITMAPINFO bitmap_info,
                                   void **output, unsigned int *output_size);

typedef enum {
    BMP,
    NUM_ENCODERS_SUPPORTED
} ImageFormat;

static const ImageCodecInfo codecs[NUM_ENCODERS_SUPPORTED];
static encode_image_func *const encode_image_funcs[NUM_ENCODERS_SUPPORTED] = {
    encode_image_BMP,
};

/*****************************************************************************
 * GdipSaveImageToStream [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipSaveImageToStream(GpImage *image, IStream* stream,
    GDIPCONST CLSID* clsid, GDIPCONST EncoderParameters* params)
{
    GpStatus stat;
    HRESULT hr;
    short type;
    HBITMAP hbmp;
    HBITMAP old_hbmp;
    HDC hdc;
    int bm_is_selected;
    BITMAPINFO bmp_info;
    LPVOID bmp_bits;
    encode_image_func* encode_image;
    LPVOID output;
    unsigned int output_size;
    unsigned int dummy;
    int i;

    old_hbmp = 0;
    output = NULL;
    output_size = 0;

    TRACE("%p %p %p %p\n", image, stream, clsid, params);

    if(!image || !stream)
        return InvalidParameter;

    if (!image->picture)
        return GenericError;

    hr = IPicture_get_Type(image->picture, &type);
    if (FAILED(hr) || type != PICTYPE_BITMAP)
        return GenericError;

    /* select correct encoder */
    encode_image = NULL;
    for (i = 0; i < NUM_ENCODERS_SUPPORTED; i++) {
        if (IsEqualCLSID(clsid, &codecs[i].Clsid))
            encode_image = encode_image_funcs[i];
    }
    if (encode_image == NULL)
        return UnknownImageFormat;

    /* extract underlying hbitmap representation from the IPicture */
    hr = IPicture_get_Handle(image->picture, (OLE_HANDLE*)&hbmp);
    if (FAILED(hr) || !hbmp)
        return GenericError;
    hr = IPicture_get_CurDC(image->picture, &hdc);
    if (FAILED(hr))
        return GenericError;
    bm_is_selected = (hdc != 0);
    if (!bm_is_selected) {
        hdc = CreateCompatibleDC(0);
        old_hbmp = SelectObject(hdc, hbmp);
    }

    /* get bits from HBITMAP */
    bmp_info.bmiHeader.biSize = sizeof(bmp_info.bmiHeader);
    bmp_info.bmiHeader.biBitCount = 0;
    GetDIBits(hdc, hbmp, 0, 0, NULL, &bmp_info, DIB_RGB_COLORS);

    bmp_bits = GdipAlloc(bmp_info.bmiHeader.biSizeImage);

    if (bmp_bits)
        GetDIBits(hdc, hbmp, 0, abs(bmp_info.bmiHeader.biHeight), bmp_bits, &bmp_info, DIB_RGB_COLORS);

    if (!bm_is_selected) {
        SelectObject(hdc, old_hbmp);
        DeleteDC(hdc);
    }

    if (!bmp_bits)
        return OutOfMemory;

    stat = encode_image(bmp_bits, &bmp_info, &output, &output_size);
    if (stat == Ok)
        IStream_Write(stream, output, output_size, &dummy);

    GdipFree(output);
    GdipFree(bmp_bits);

    return stat;
}

/*****************************************************************************
 * GdipSetImagePalette [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipSetImagePalette(GpImage *image,
    GDIPCONST ColorPalette *palette)
{
    static int calls;

    if(!image || !palette)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

/*************************************************************************
 * Encoders -
 *   Structures that represent which formats we support for encoding.
 */

/* ImageCodecInfo creation routines taken from libgdiplus */
static const WCHAR bmp_codecname[] = {'B', 'u', 'i','l', 't', '-','i', 'n', ' ', 'B', 'M', 'P', 0}; /* Built-in BMP */
static const WCHAR bmp_extension[] = {'*','.','B', 'M', 'P',';', '*','.', 'D','I', 'B',';', '*','.', 'R', 'L', 'E',0}; /* *.BMP;*.DIB;*.RLE */
static const WCHAR bmp_mimetype[] = {'i', 'm', 'a','g', 'e', '/', 'b', 'm', 'p', 0}; /* image/bmp */
static const WCHAR bmp_format[] = {'B', 'M', 'P', 0}; /* BMP */
static const BYTE bmp_sig_pattern[] = { 0x42, 0x4D };
static const BYTE bmp_sig_mask[] = { 0xFF, 0xFF };

static const ImageCodecInfo codecs[NUM_ENCODERS_SUPPORTED] =
    {
        { /* BMP */
            /* Clsid */              { 0x557cf400, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x0, 0x0, 0xf8, 0x1e, 0xf3, 0x2e } },
            /* FormatID */           { 0xb96b3cabU, 0x0728U, 0x11d3U, {0x9d, 0x7b, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e} },
            /* CodecName */          bmp_codecname,
            /* DllName */            NULL,
            /* FormatDescription */  bmp_format,
            /* FilenameExtension */  bmp_extension,
            /* MimeType */           bmp_mimetype,
            /* Flags */              ImageCodecFlagsEncoder | ImageCodecFlagsDecoder | ImageCodecFlagsSupportBitmap | ImageCodecFlagsBuiltin,
            /* Version */            1,
            /* SigCount */           1,
            /* SigSize */            2,
            /* SigPattern */         bmp_sig_pattern,
            /* SigMask */            bmp_sig_mask,
        },
    };

/*****************************************************************************
 * GdipGetImageDecodersSize [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetImageDecodersSize(UINT *numDecoders, UINT *size)
{
    FIXME("%p %p stub!\n", numDecoders, size);

    if (!numDecoders || !size)
        return InvalidParameter;

    *numDecoders = 0;
    *size = 0;

    return Ok;
}

/*****************************************************************************
 * GdipGetImageDecoders [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetImageDecoders(UINT numDecoders, UINT size, ImageCodecInfo *decoders)
{
    FIXME("%u %u %p stub!\n", numDecoders, size, decoders);

    if (!decoders)
        return GenericError;

    return NotImplemented;
}

/*****************************************************************************
 * GdipGetImageEncodersSize [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetImageEncodersSize(UINT *numEncoders, UINT *size)
{
    TRACE("%p %p\n", numEncoders, size);

    if (!numEncoders || !size)
        return InvalidParameter;

    *numEncoders = NUM_ENCODERS_SUPPORTED;
    *size = sizeof (codecs);

    return Ok;
}

/*****************************************************************************
 * GdipGetImageEncoders [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetImageEncoders(UINT numEncoders, UINT size, ImageCodecInfo *encoders)
{
    TRACE("%u %u %p\n", numEncoders, size, encoders);

    if (!encoders ||
        (numEncoders != NUM_ENCODERS_SUPPORTED) ||
        (size != sizeof (codecs)))
        return GenericError;

    memcpy(encoders, codecs, sizeof (codecs));

    return Ok;
}

/*****************************************************************************
 * GdipCreateBitmapFromHBITMAP [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateBitmapFromHBITMAP(HBITMAP hbm, HPALETTE hpal, GpBitmap** bitmap)
{
    BITMAP bm;
    GpStatus retval;
    PixelFormat format;

    TRACE("%p %p %p\n", hbm, hpal, bitmap);

    if(!hbm || !bitmap)
        return InvalidParameter;

    /* TODO: Support for device-dependent bitmaps */
    if(hpal){
        FIXME("no support for device-dependent bitmaps\n");
        return NotImplemented;
    }

    if (GetObjectA(hbm, sizeof(bm), &bm) != sizeof(bm))
            return InvalidParameter;

    /* TODO: Figure out the correct format for 16, 32, 64 bpp */
    switch(bm.bmBitsPixel) {
        case 1:
            format = PixelFormat1bppIndexed;
            break;
        case 4:
            format = PixelFormat4bppIndexed;
            break;
        case 8:
            format = PixelFormat8bppIndexed;
            break;
        case 24:
            format = PixelFormat24bppRGB;
            break;
        case 32:
            format = PixelFormat32bppRGB;
            break;
        case 48:
            format = PixelFormat48bppRGB;
            break;
        default:
            FIXME("don't know how to handle %d bpp\n", bm.bmBitsPixel);
            return InvalidParameter;
    }

    retval = GdipCreateBitmapFromScan0(bm.bmWidth, bm.bmHeight, bm.bmWidthBytes,
        format, bm.bmBits, bitmap);

    return retval;
}

/*****************************************************************************
 * GdipSetEffectParameters [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipSetEffectParameters(CGpEffect *effect,
    const VOID *params, const UINT size)
{
    static int calls;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

/*****************************************************************************
 * GdipGetImageFlags [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetImageFlags(GpImage *image, UINT *flags)
{
    TRACE("%p %p\n", image, flags);

    if(!image || !flags)
        return InvalidParameter;

    *flags = image->flags;

    return Ok;
}

GpStatus WINGDIPAPI GdipTestControl(GpTestControlEnum control, void *param)
{
    TRACE("(%d, %p)\n", control, param);

    switch(control){
        case TestControlForceBilinear:
            if(param)
                FIXME("TestControlForceBilinear not handled\n");
            break;
        case TestControlNoICM:
            if(param)
                FIXME("TestControlNoICM not handled\n");
            break;
        case TestControlGetBuildNumber:
            *((DWORD*)param) = 3102;
            break;
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipRecordMetafileFileName(GDIPCONST WCHAR* fileName,
                            HDC hdc, EmfType type, GDIPCONST GpRectF *pFrameRect,
                            MetafileFrameUnit frameUnit, GDIPCONST WCHAR *desc,
                            GpMetafile **metafile)
{
    FIXME("%s %p %d %p %d %s %p stub!\n", debugstr_w(fileName), hdc, type, pFrameRect,
                                 frameUnit, debugstr_w(desc), metafile);

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipRecordMetafileFileNameI(GDIPCONST WCHAR* fileName, HDC hdc, EmfType type,
                            GDIPCONST GpRect *pFrameRect, MetafileFrameUnit frameUnit,
                            GDIPCONST WCHAR *desc, GpMetafile **metafile)
{
    FIXME("%s %p %d %p %d %s %p stub!\n", debugstr_w(fileName), hdc, type, pFrameRect,
                                 frameUnit, debugstr_w(desc), metafile);

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipImageForceValidation(GpImage *image)
{
    FIXME("%p\n", image);

    return Ok;
}
