/*
 * GdiPlusBase.h
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

#ifndef _GDIPLUSBASE_H
#define _GDIPLUSBASE_H

class GdiplusBase
{
  public:
    void
    operator delete(void *in_pVoid)
    {
        DllExports::GdipFree(in_pVoid);
    }

    void
    operator delete[](void *in_pVoid)
    {
        DllExports::GdipFree(in_pVoid);
    }

    void *
    operator new(size_t in_size)
    {
        return DllExports::GdipAlloc(in_size);
    }

    void *
    operator new[](size_t in_size)
    {
        return DllExports::GdipAlloc(in_size);
    }
};

class Brush;
class CachedBitmap;
class CustomLineCap;
class Font;
class FontCollection;
class FontFamily;
class Graphics;
class GraphicsPath;
class Image;
class ImageAttributes;
class Matrix;
class Metafile;
class Pen;
class Region;
class StringFormat;

// get native
GpBrush *&
getNat(const Brush *brush);

GpCachedBitmap *&
getNat(const CachedBitmap *cb);

GpCustomLineCap *&
getNat(const CustomLineCap *cap);

GpFont *&
getNat(const Font *font);

GpFontCollection *&
getNat(const FontCollection *fc);

GpFontFamily *&
getNat(const FontFamily *ff);

GpGraphics *&
getNat(const Graphics *graphics);

GpPath *&
getNat(const GraphicsPath *path);

GpImage *&
getNat(const Image *image);

GpImageAttributes *&
getNat(const ImageAttributes *ia);

GpMatrix *&
getNat(const Matrix *matrix);

GpMetafile *&
getNat(const Metafile *metafile);

GpPen *&
getNat(const Pen *pen);

GpRegion *&
getNat(const Region *region);

GpStringFormat *&
getNat(const StringFormat *sf);

#endif /* _GDIPLUSBASE_H */
