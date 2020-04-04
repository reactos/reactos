/*
 * GdiPlusGraphics.h
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

#ifndef _GDIPLUSGRAPHICS_H
#define _GDIPLUSGRAPHICS_H

class Graphics : public GdiplusBase
{
    friend class Region;
    friend class Font;
    friend class Bitmap;
    friend class CachedBitmap;
    friend class ImageAttributes;

  public:
    Graphics(Image *image)
    {
        GpGraphics *graphics = NULL;
        if (image)
        {
            lastStatus = DllExports::GdipGetImageGraphicsContext(getNat(image), &graphics);
        }
        SetNativeGraphics(graphics);
    }

    Graphics(HDC hdc)
    {
        GpGraphics *graphics = NULL;
        lastStatus = DllExports::GdipCreateFromHDC(hdc, &graphics);
        SetNativeGraphics(graphics);
    }

    Graphics(HDC hdc, HANDLE hdevice)
    {
        GpGraphics *graphics = NULL;
        lastStatus = DllExports::GdipCreateFromHDC2(hdc, hdevice, &graphics);
        SetNativeGraphics(graphics);
    }

    Graphics(HWND hwnd, BOOL icm = FALSE)
    {
        GpGraphics *graphics = NULL;
        if (icm)
        {
            lastStatus = DllExports::GdipCreateFromHWNDICM(hwnd, &graphics);
        }
        else
        {
            lastStatus = DllExports::GdipCreateFromHWND(hwnd, &graphics);
        }
        SetNativeGraphics(graphics);
    }

    ~Graphics()
    {
        DllExports::GdipDeleteGraphics(nativeGraphics);
    }

    Status
    AddMetafileComment(const BYTE *data, UINT sizeData)
    {
        return SetStatus(DllExports::GdipComment(nativeGraphics, sizeData, data));
    }

    GraphicsContainer
    BeginContainer()
    {
        return GraphicsContainer();
    }

    GraphicsContainer
    BeginContainer(const RectF &dstrect, const RectF &srcrect, Unit unit)
    {
        return GraphicsContainer();
    }

    GraphicsContainer
    BeginContainer(const Rect &dstrect, const Rect &srcrect, Unit unit)
    {
        return GraphicsContainer();
    }

    Status
    Clear(const Color &color)
    {
        return SetStatus(DllExports::GdipGraphicsClear(nativeGraphics, color.GetValue()));
    }

    Status
    DrawArc(const Pen *pen, const Rect &rect, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipDrawArcI(
            nativeGraphics, pen ? getNat(pen) : NULL, rect.X, rect.Y, rect.Width, rect.Height, startAngle, sweepAngle));
    }

    Status
    DrawArc(const Pen *pen, const RectF &rect, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipDrawArcI(
            nativeGraphics, pen ? getNat(pen) : NULL, rect.X, rect.Y, rect.Width, rect.Height, startAngle, sweepAngle));
    }

    Status
    DrawArc(const Pen *pen, REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipDrawArc(
            nativeGraphics, pen ? getNat(pen) : NULL, x, y, width, height, startAngle, sweepAngle));
    }

    Status
    DrawArc(const Pen *pen, INT x, INT y, INT width, INT height, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipDrawArcI(
            nativeGraphics, pen ? getNat(pen) : NULL, x, y, width, height, startAngle, sweepAngle));
    }

    Status
    DrawBezier(const Pen *pen, const Point &pt1, const Point &pt2, const Point &pt3, const Point &pt4)
    {
        return SetStatus(DllExports::GdipDrawBezierI(
            nativeGraphics, pen ? getNat(pen) : NULL, pt1.X, pt1.Y, pt2.X, pt2.Y, pt3.X, pt3.Y, pt4.X, pt4.Y));
    }

    Status
    DrawBezier(const Pen *pen, const PointF &pt1, const PointF &pt2, const PointF &pt3, const PointF &pt4)
    {
        return SetStatus(DllExports::GdipDrawBezier(
            nativeGraphics, pen ? getNat(pen) : NULL, pt1.X, pt1.Y, pt2.X, pt2.Y, pt3.X, pt3.Y, pt4.X, pt4.Y));
    }

    Status
    DrawBezier(const Pen *pen, REAL x1, REAL y1, REAL x2, REAL y2, REAL x3, REAL y3, REAL x4, REAL y4)
    {
        return SetStatus(
            DllExports::GdipDrawBezier(nativeGraphics, pen ? getNat(pen) : NULL, x1, y1, x2, y2, x3, y3, x4, y4));
    }

    Status
    DrawBezier(const Pen *pen, INT x1, INT y1, INT x2, INT y2, INT x3, INT y3, INT x4, INT y4)
    {
        return SetStatus(
            DllExports::GdipDrawBezierI(nativeGraphics, pen ? getNat(pen) : NULL, x1, y1, x2, y2, x3, y3, x4, y4));
    }

    Status
    DrawBeziers(const Pen *pen, const Point *points, INT count)
    {
        return SetStatus(DllExports::GdipDrawBeziersI(nativeGraphics, pen ? getNat(pen) : NULL, points, count));
    }

    Status
    DrawBeziers(const Pen *pen, const PointF *points, INT count)
    {
        return SetStatus(DllExports::GdipDrawBeziers(nativeGraphics, pen ? getNat(pen) : NULL, points, count));
    }

    Status
    DrawCachedBitmap(CachedBitmap *cb, INT x, INT y)
    {
        return SetStatus(DllExports::GdipDrawCachedBitmap(nativeGraphics, getNat(cb), x, y));
    }

    Status
    DrawClosedCurve(const Pen *pen, const Point *points, INT count)
    {
        return SetStatus(DllExports::GdipDrawClosedCurveI(nativeGraphics, pen ? getNat(pen) : NULL, points, count));
    }

    Status
    DrawClosedCurve(const Pen *pen, const PointF *points, INT count)
    {
        return SetStatus(DllExports::GdipDrawClosedCurve(nativeGraphics, pen ? getNat(pen) : NULL, points, count));
    }

    Status
    DrawClosedCurve(const Pen *pen, const PointF *points, INT count, REAL tension)
    {
        return SetStatus(
            DllExports::GdipDrawClosedCurve2(nativeGraphics, pen ? getNat(pen) : NULL, points, count, tension));
    }

    Status
    DrawClosedCurve(const Pen *pen, const Point *points, INT count, REAL tension)
    {
        return SetStatus(
            DllExports::GdipDrawClosedCurve2I(nativeGraphics, pen ? getNat(pen) : NULL, points, count, tension));
    }

    Status
    DrawCurve(const Pen *pen, const Point *points, INT count)
    {
        return SetStatus(DllExports::GdipDrawCurveI(nativeGraphics, pen ? getNat(pen) : NULL, points, count));
    }

    Status
    DrawCurve(const Pen *pen, const PointF *points, INT count)
    {
        return SetStatus(DllExports::GdipDrawCurve(nativeGraphics, pen ? getNat(pen) : NULL, points, count));
    }

    Status
    DrawCurve(const Pen *pen, const PointF *points, INT count, REAL tension)
    {
        return SetStatus(DllExports::GdipDrawCurve2(nativeGraphics, pen ? getNat(pen) : NULL, points, count, tension));
    }

    Status
    DrawCurve(const Pen *pen, const Point *points, INT count, INT offset, INT numberOfSegments, REAL tension)
    {
        return SetStatus(DllExports::GdipDrawCurve3I(
            nativeGraphics, pen ? getNat(pen) : NULL, points, count, offset, numberOfSegments, tension));
    }

    Status
    DrawCurve(const Pen *pen, const PointF *points, INT count, INT offset, INT numberOfSegments, REAL tension)
    {
        return SetStatus(DllExports::GdipDrawCurve3(
            nativeGraphics, pen ? getNat(pen) : NULL, points, count, offset, numberOfSegments, tension));
    }

    Status
    DrawCurve(const Pen *pen, const Point *points, INT count, REAL tension)
    {
        return SetStatus(DllExports::GdipDrawCurve2I(nativeGraphics, pen ? getNat(pen) : NULL, points, count, tension));
    }

    Status
    DrawDriverString(
        const UINT16 *text,
        INT length,
        const Font *font,
        const Brush *brush,
        const PointF *positions,
        INT flags,
        const Matrix *matrix)
    {
        return SetStatus(DllExports::GdipDrawDriverString(
            nativeGraphics, text, length, font ? getNat(font) : NULL, brush ? getNat(brush) : NULL, positions, flags,
            matrix ? getNat(matrix) : NULL));
    }

    Status
    DrawEllipse(const Pen *pen, const Rect &rect)
    {
        return SetStatus(DllExports::GdipDrawEllipseI(
            nativeGraphics, pen ? getNat(pen) : NULL, rect.X, rect.Y, rect.Width, rect.Height));
    }

    Status
    DrawEllipse(const Pen *pen, REAL x, REAL y, REAL width, REAL height)
    {
        return SetStatus(DllExports::GdipDrawEllipse(nativeGraphics, pen ? getNat(pen) : NULL, x, y, width, height));
    }

    Status
    DrawEllipse(const Pen *pen, const RectF &rect)
    {
        return SetStatus(DllExports::GdipDrawEllipse(
            nativeGraphics, pen ? getNat(pen) : NULL, rect.X, rect.Y, rect.Width, rect.Height));
    }

    Status
    DrawEllipse(const Pen *pen, INT x, INT y, INT width, INT height)
    {
        return SetStatus(DllExports::GdipDrawEllipseI(nativeGraphics, pen ? getNat(pen) : NULL, x, y, width, height));
    }

    Status
    DrawImage(Image *image, const Point *destPoints, INT count)
    {
        if (count != 3 && count != 4)
            return SetStatus(InvalidParameter);

        return SetStatus(
            DllExports::GdipDrawImagePointsI(nativeGraphics, image ? getNat(image) : NULL, destPoints, count));
    }

    Status
    DrawImage(Image *image, INT x, INT y)
    {
        return SetStatus(DllExports::GdipDrawImageI(nativeGraphics, image ? getNat(image) : NULL, x, y));
    }

    Status
    DrawImage(Image *image, const Point &point)
    {
        return DrawImage(image, point.X, point.Y);
    }

    Status
    DrawImage(Image *image, REAL x, REAL y)
    {
        return SetStatus(DllExports::GdipDrawImage(nativeGraphics, image ? getNat(image) : NULL, x, y));
    }

    Status
    DrawImage(Image *image, const PointF &point)
    {
        return DrawImage(image, point.X, point.Y);
    }

    Status
    DrawImage(Image *image, const PointF *destPoints, INT count)
    {
        if (count != 3 && count != 4)
            return SetStatus(InvalidParameter);

        return SetStatus(
            DllExports::GdipDrawImagePoints(nativeGraphics, image ? getNat(image) : NULL, destPoints, count));
    }

    Status
    DrawImage(Image *image, REAL x, REAL y, REAL srcx, REAL srcy, REAL srcwidth, REAL srcheight, Unit srcUnit)
    {
        return SetStatus(DllExports::GdipDrawImagePointRect(
            nativeGraphics, image ? getNat(image) : NULL, x, y, srcx, srcy, srcwidth, srcheight, srcUnit));
    }

    Status
    DrawImage(Image *image, const RectF &rect)
    {
        return DrawImage(image, rect.X, rect.Y, rect.Width, rect.Height);
    }

    Status
    DrawImage(Image *image, INT x, INT y, INT width, INT height)
    {
        return SetStatus(
            DllExports::GdipDrawImageRectI(nativeGraphics, image ? getNat(image) : NULL, x, y, width, height));
    }

    Status
    DrawImage(
        Image *image,
        const PointF *destPoints,
        INT count,
        REAL srcx,
        REAL srcy,
        REAL srcwidth,
        REAL srcheight,
        Unit srcUnit,
        ImageAttributes *imageAttributes,
        DrawImageAbort callback,
        VOID *callbackData)
    {
        return SetStatus(DllExports::GdipDrawImagePointsRect(
            nativeGraphics, image ? getNat(image) : NULL, destPoints, count, srcx, srcy, srcwidth, srcheight, srcUnit,
            imageAttributes ? getNat(imageAttributes) : NULL, callback, callbackData));
    }

    Status
    DrawImage(
        Image *image,
        const Rect &destRect,
        INT srcx,
        INT srcy,
        INT srcwidth,
        INT srcheight,
        Unit srcUnit,
        const ImageAttributes *imageAttributes = NULL,
        DrawImageAbort callback = NULL,
        VOID *callbackData = NULL)
    {
        return SetStatus(DllExports::GdipDrawImageRectRectI(
            nativeGraphics, image ? getNat(image) : NULL, destRect.X, destRect.Y, destRect.Width, destRect.Height, srcx,
            srcy, srcwidth, srcheight, srcUnit, imageAttributes ? getNat(imageAttributes) : NULL, callback,
            callbackData));
    }

    Status
    DrawImage(
        Image *image,
        const Point *destPoints,
        INT count,
        INT srcx,
        INT srcy,
        INT srcwidth,
        INT srcheight,
        Unit srcUnit,
        ImageAttributes *imageAttributes = NULL,
        DrawImageAbort callback = NULL,
        VOID *callbackData = NULL)
    {
        return SetStatus(DllExports::GdipDrawImagePointsRectI(
            nativeGraphics, image ? getNat(image) : NULL, destPoints, count, srcx, srcy, srcwidth, srcheight, srcUnit,
            imageAttributes ? getNat(imageAttributes) : NULL, callback, callbackData));
    }

    Status
    DrawImage(Image *image, REAL x, REAL y, REAL width, REAL height)
    {
        return SetStatus(
            DllExports::GdipDrawImageRect(nativeGraphics, image ? getNat(image) : NULL, x, y, width, height));
    }

    Status
    DrawImage(Image *image, const Rect &rect)
    {
        return DrawImage(image, rect.X, rect.Y, rect.Width, rect.Height);
    }

    Status
    DrawImage(Image *image, INT x, INT y, INT srcx, INT srcy, INT srcwidth, INT srcheight, Unit srcUnit)
    {
        return SetStatus(DllExports::GdipDrawImagePointRectI(
            nativeGraphics, image ? getNat(image) : NULL, x, y, srcx, srcy, srcwidth, srcheight, srcUnit));
    }

    Status
    DrawImage(
        Image *image,
        const RectF &destRect,
        REAL srcx,
        REAL srcy,
        REAL srcwidth,
        REAL srcheight,
        Unit srcUnit,
        ImageAttributes *imageAttributes = NULL,
        DrawImageAbort callback = NULL,
        VOID *callbackData = NULL)
    {
        return SetStatus(DllExports::GdipDrawImageRectRect(
            nativeGraphics, image ? getNat(image) : NULL, destRect.X, destRect.Y, destRect.Width, destRect.Height, srcx,
            srcy, srcwidth, srcheight, srcUnit, imageAttributes ? getNat(imageAttributes) : NULL, callback,
            callbackData));
    }

    Status
    DrawLine(const Pen *pen, const Point &pt1, const Point &pt2)
    {
        return SetStatus(
            DllExports::GdipDrawLineI(nativeGraphics, pen ? getNat(pen) : NULL, pt1.X, pt1.Y, pt2.X, pt2.Y));
    }

    Status
    DrawLine(const Pen *pen, const PointF &pt1, const Point &pt2)
    {
        return SetStatus(
            DllExports::GdipDrawLine(nativeGraphics, pen ? getNat(pen) : NULL, pt1.X, pt1.Y, pt2.X, pt2.Y));
    }

    Status
    DrawLine(const Pen *pen, REAL x1, REAL y1, REAL x2, REAL y2)
    {
        return SetStatus(DllExports::GdipDrawLine(nativeGraphics, pen ? getNat(pen) : NULL, x1, y1, x2, y2));
    }

    Status
    DrawLine(const Pen *pen, INT x1, INT y1, INT x2, INT y2)
    {
        return SetStatus(DllExports::GdipDrawLine(nativeGraphics, pen ? getNat(pen) : NULL, x1, y1, x2, y2));
    }

    Status
    DrawLines(const Pen *pen, const Point *points, INT count)
    {
        return SetStatus(DllExports::GdipDrawLinesI(nativeGraphics, pen ? getNat(pen) : NULL, points, count));
    }

    Status
    DrawLines(const Pen *pen, const PointF *points, INT count)
    {
        return SetStatus(DllExports::GdipDrawLines(nativeGraphics, pen ? getNat(pen) : NULL, points, count));
    }

    Status
    DrawPath(const Pen *pen, const GraphicsPath *path)
    {
        return SetStatus(
            DllExports::GdipDrawPath(nativeGraphics, pen ? getNat(pen) : NULL, path ? getNat(path) : NULL));
    }

    Status
    DrawPie(const Pen *pen, const Rect &rect, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipDrawPieI(
            nativeGraphics, pen ? getNat(pen) : NULL, rect.X, rect.Y, rect.Width, rect.Height, startAngle, sweepAngle));
    }

    Status
    DrawPie(const Pen *pen, INT x, INT y, INT width, INT height, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipDrawPieI(
            nativeGraphics, pen ? getNat(pen) : NULL, x, y, width, height, startAngle, sweepAngle));
    }

    Status
    DrawPie(const Pen *pen, REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipDrawPie(
            nativeGraphics, pen ? getNat(pen) : NULL, x, y, width, height, startAngle, sweepAngle));
    }

    Status
    DrawPie(const Pen *pen, const RectF &rect, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipDrawPie(
            nativeGraphics, pen ? getNat(pen) : NULL, rect.X, rect.Y, rect.Width, rect.Height, startAngle, sweepAngle));
    }

    Status
    DrawPolygon(const Pen *pen, const Point *points, INT count)
    {
        return SetStatus(DllExports::GdipDrawPolygonI(nativeGraphics, pen ? getNat(pen) : NULL, points, count));
    }

    Status
    DrawPolygon(const Pen *pen, const PointF *points, INT count)
    {
        return SetStatus(DllExports::GdipDrawPolygon(nativeGraphics, pen ? getNat(pen) : NULL, points, count));
    }

    Status
    DrawRectangle(const Pen *pen, const Rect &rect)
    {
        return SetStatus(DllExports::GdipDrawRectangleI(
            nativeGraphics, pen ? getNat(pen) : NULL, rect.X, rect.Y, rect.Width, rect.Height));
    }

    Status
    DrawRectangle(const Pen *pen, INT x, INT y, INT width, INT height)
    {
        return SetStatus(DllExports::GdipDrawRectangleI(nativeGraphics, pen ? getNat(pen) : NULL, x, y, width, height));
    }

    Status
    DrawRectangle(const Pen *pen, REAL x, REAL y, REAL width, REAL height)
    {
        return SetStatus(DllExports::GdipDrawRectangle(nativeGraphics, pen ? getNat(pen) : NULL, x, y, width, height));
    }

    Status
    DrawRectangle(const Pen *pen, const RectF &rect)
    {
        return SetStatus(DllExports::GdipDrawRectangleI(
            nativeGraphics, pen ? getNat(pen) : NULL, rect.X, rect.Y, rect.Width, rect.Height));
    }

    Status
    DrawRectangles(const Pen *pen, const Rect *rects, INT count)
    {
        return SetStatus(DllExports::GdipDrawRectanglesI(nativeGraphics, pen ? getNat(pen) : NULL, rects, count));
    }

    Status
    DrawRectangles(const Pen *pen, const RectF *rects, INT count)
    {
        return SetStatus(DllExports::GdipDrawRectangles(nativeGraphics, pen ? getNat(pen) : NULL, rects, count));
    }

    Status
    DrawString(
        const WCHAR *string,
        INT length,
        const Font *font,
        const RectF &layoutRect,
        const StringFormat *stringFormat,
        const Brush *brush)
    {
        return SetStatus(DllExports::GdipDrawString(
            nativeGraphics, string, length, font ? getNat(font) : NULL, &layoutRect,
            stringFormat ? getNat(stringFormat) : NULL, brush ? brush->nativeBrush : NULL));
    }

    Status
    DrawString(const WCHAR *string, INT length, const Font *font, const PointF &origin, const Brush *brush)
    {
        RectF rect(origin.X, origin.Y, 0.0f, 0.0f);
        return SetStatus(DllExports::GdipDrawString(
            nativeGraphics, string, length, font ? getNat(font) : NULL, &rect, NULL, brush ? getNat(brush) : NULL));
    }

    Status
    DrawString(
        const WCHAR *string,
        INT length,
        const Font *font,
        const PointF &origin,
        const StringFormat *stringFormat,
        const Brush *brush)
    {
        RectF rect(origin.X, origin.Y, 0.0f, 0.0f);
        return SetStatus(DllExports::GdipDrawString(
            nativeGraphics, string, length, font ? getNat(font) : NULL, &rect,
            stringFormat ? getNat(stringFormat) : NULL, brush ? getNat(brush) : NULL));
    }

    Status
    EndContainer(GraphicsContainer state)
    {
        return SetStatus(DllExports::GdipEndContainer(nativeGraphics, state));
    }

    Status
    EnumerateMetafile(
        const Metafile *metafile,
        const PointF &destPoint,
        EnumerateMetafileProc callback,
        VOID *callbackData = NULL,
        const ImageAttributes *imageAttributes = NULL)
    {
        return SetStatus(NotImplemented);
    }

    Status
    EnumerateMetafile(
        const Metafile *metafile,
        const Point *destPoints,
        INT count,
        EnumerateMetafileProc callback,
        VOID *callbackData = NULL,
        const ImageAttributes *imageAttributes = NULL)
    {
        return SetStatus(NotImplemented);
    }

    Status
    EnumerateMetafile(
        const Metafile *metafile,
        const Point &destPoint,
        const Rect &srcRect,
        Unit srcUnit,
        EnumerateMetafileProc callback,
        VOID *callbackData = NULL,
        const ImageAttributes *imageAttributes = NULL)
    {
        return SetStatus(NotImplemented);
    }

    Status
    EnumerateMetafile(
        const Metafile *metafile,
        const PointF *destPoints,
        INT count,
        EnumerateMetafileProc callback,
        VOID *callbackData = NULL,
        const ImageAttributes *imageAttributes = NULL)
    {
        return SetStatus(NotImplemented);
    }

    Status
    EnumerateMetafile(
        const Metafile *metafile,
        const Rect &destRect,
        EnumerateMetafileProc callback,
        VOID *callbackData = NULL,
        const ImageAttributes *imageAttributes = NULL)
    {
        return SetStatus(NotImplemented);
    }

    Status
    EnumerateMetafile(
        const Metafile *metafile,
        const RectF &destRect,
        const RectF &srcRect,
        Unit srcUnit,
        EnumerateMetafileProc callback,
        VOID *callbackData = NULL,
        const ImageAttributes *imageAttributes = NULL)
    {
        return SetStatus(NotImplemented);
    }

    Status
    EnumerateMetafile(
        const Metafile *metafile,
        const RectF &destRect,
        EnumerateMetafileProc callback,
        VOID *callbackData = NULL,
        const ImageAttributes *imageAttributes = NULL)
    {
        return SetStatus(NotImplemented);
    }

    Status
    EnumerateMetafile(
        const Metafile *metafile,
        const PointF &destPoint,
        const RectF &srcRect,
        Unit srcUnit,
        EnumerateMetafileProc callback,
        VOID *callbackData = NULL,
        const ImageAttributes *imageAttributes = NULL)
    {
        return SetStatus(NotImplemented);
    }

    Status
    EnumerateMetafile(
        const Metafile *metafile,
        const Point *destPoints,
        INT count,
        const Rect &srcRect,
        Unit srcUnit,
        EnumerateMetafileProc callback,
        VOID *callbackData = NULL,
        const ImageAttributes *imageAttributes = NULL)
    {
        return SetStatus(NotImplemented);
    }

    Status
    EnumerateMetafile(
        const Metafile *metafile,
        const Rect &destRect,
        const Rect &srcRect,
        Unit srcUnit,
        EnumerateMetafileProc callback,
        VOID *callbackData = NULL,
        const ImageAttributes *imageAttributes = NULL)
    {
        return SetStatus(NotImplemented);
    }

    Status
    EnumerateMetafile(
        const Metafile *metafile,
        const PointF *destPoints,
        INT count,
        const RectF &srcRect,
        Unit srcUnit,
        EnumerateMetafileProc callback,
        VOID *callbackData = NULL,
        const ImageAttributes *imageAttributes = NULL)
    {
        return SetStatus(NotImplemented);
    }

    Status
    EnumerateMetafile(
        const Metafile *metafile,
        const Point &destPoint,
        EnumerateMetafileProc callback,
        VOID *callbackData = NULL,
        const ImageAttributes *imageAttributes = NULL)
    {
        return SetStatus(NotImplemented);
    }

    Status
    ExcludeClip(const Rect &rect)
    {
        return SetStatus(
            DllExports::GdipSetClipRectI(nativeGraphics, rect.X, rect.Y, rect.Width, rect.Height, CombineModeExclude));
    }

    Status
    ExcludeClip(const RectF &rect)
    {
        return SetStatus(
            DllExports::GdipSetClipRect(nativeGraphics, rect.X, rect.Y, rect.Width, rect.Height, CombineModeExclude));
    }

    Status
    ExcludeClip(const Region *region)
    {
        return SetStatus(DllExports::GdipSetClipRegion(nativeGraphics, getNat(region), CombineModeExclude));
    }

    Status
    FillClosedCurve(const Brush *brush, const Point *points, INT count)
    {
        return SetStatus(DllExports::GdipFillClosedCurveI(nativeGraphics, brush ? getNat(brush) : NULL, points, count));
    }

    Status
    FillClosedCurve(const Brush *brush, const Point *points, INT count, FillMode fillMode, REAL tension)
    {
        return SetStatus(DllExports::GdipFillClosedCurve2I(
            nativeGraphics, brush ? getNat(brush) : NULL, points, count, tension, fillMode));
    }

    Status
    FillClosedCurve(const Brush *brush, const PointF *points, INT count)
    {
        return SetStatus(DllExports::GdipFillClosedCurve(nativeGraphics, brush ? getNat(brush) : NULL, points, count));
    }

    Status
    FillClosedCurve(const Brush *brush, const PointF *points, INT count, FillMode fillMode, REAL tension)
    {
        return SetStatus(DllExports::GdipFillClosedCurve2(
            nativeGraphics, brush ? getNat(brush) : NULL, points, count, tension, fillMode));
    }

    Status
    FillEllipse(const Brush *brush, const Rect &rect)
    {
        return SetStatus(DllExports::GdipFillEllipseI(
            nativeGraphics, brush ? getNat(brush) : NULL, rect.X, rect.Y, rect.Width, rect.Height));
    }

    Status
    FillEllipse(const Brush *brush, REAL x, REAL y, REAL width, REAL height)
    {
        return SetStatus(
            DllExports::GdipFillEllipse(nativeGraphics, brush ? getNat(brush) : NULL, x, y, width, height));
    }

    Status
    FillEllipse(const Brush *brush, const RectF &rect)
    {
        return SetStatus(DllExports::GdipFillEllipse(
            nativeGraphics, brush ? getNat(brush) : NULL, rect.X, rect.Y, rect.Width, rect.Height));
    }

    Status
    FillEllipse(const Brush *brush, INT x, INT y, INT width, INT height)
    {
        return SetStatus(
            DllExports::GdipFillEllipseI(nativeGraphics, brush ? getNat(brush) : NULL, x, y, width, height));
    }

    Status
    FillPath(const Brush *brush, const GraphicsPath *path)
    {
        return SetStatus(DllExports::GdipFillPath(nativeGraphics, getNat(brush), getNat(path)));
    }

    Status
    FillPie(const Brush *brush, const Rect &rect, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipFillPieI(
            nativeGraphics, brush ? getNat(brush) : NULL, rect.X, rect.Y, rect.Width, rect.Height, startAngle,
            sweepAngle));
    }

    Status
    FillPie(const Brush *brush, INT x, INT y, INT width, INT height, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipFillPieI(
            nativeGraphics, brush ? getNat(brush) : NULL, x, y, width, height, startAngle, sweepAngle));
    }

    Status
    FillPie(const Brush *brush, REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipFillPie(
            nativeGraphics, brush ? getNat(brush) : NULL, x, y, width, height, startAngle, sweepAngle));
    }

    Status
    FillPie(const Brush *brush, RectF &rect, REAL startAngle, REAL sweepAngle)
    {
        return SetStatus(DllExports::GdipFillPie(
            nativeGraphics, brush ? getNat(brush) : NULL, rect.X, rect.Y, rect.Width, rect.Height, startAngle,
            sweepAngle));
    }

    Status
    FillPolygon(const Brush *brush, const Point *points, INT count)
    {
        return SetStatus(DllExports::GdipFillPolygon2I(nativeGraphics, brush ? getNat(brush) : NULL, points, count));
    }

    Status
    FillPolygon(const Brush *brush, const PointF *points, INT count)
    {
        return SetStatus(DllExports::GdipFillPolygon2(nativeGraphics, brush ? getNat(brush) : NULL, points, count));
    }

    Status
    FillPolygon(const Brush *brush, const Point *points, INT count, FillMode fillMode)
    {
        return SetStatus(
            DllExports::GdipFillPolygonI(nativeGraphics, brush ? getNat(brush) : NULL, points, count, fillMode));
    }

    Status
    FillPolygon(const Brush *brush, const PointF *points, INT count, FillMode fillMode)
    {
        return SetStatus(
            DllExports::GdipFillPolygon(nativeGraphics, brush ? getNat(brush) : NULL, points, count, fillMode));
    }

    Status
    FillRectangle(const Brush *brush, const Rect &rect)
    {
        return SetStatus(DllExports::GdipFillRectangleI(
            nativeGraphics, brush ? getNat(brush) : NULL, rect.X, rect.Y, rect.Width, rect.Height));
    }

    Status
    FillRectangle(const Brush *brush, const RectF &rect)
    {
        return SetStatus(DllExports::GdipFillRectangle(
            nativeGraphics, brush ? getNat(brush) : NULL, rect.X, rect.Y, rect.Width, rect.Height));
    }

    Status
    FillRectangle(const Brush *brush, REAL x, REAL y, REAL width, REAL height)
    {
        return SetStatus(
            DllExports::GdipFillRectangle(nativeGraphics, brush ? getNat(brush) : NULL, x, y, width, height));
    }

    Status
    FillRectangle(const Brush *brush, INT x, INT y, INT width, INT height)
    {
        return SetStatus(
            DllExports::GdipFillRectangleI(nativeGraphics, brush ? getNat(brush) : NULL, x, y, width, height));
    }

    Status
    FillRectangles(const Brush *brush, const Rect *rects, INT count)
    {
        return SetStatus(DllExports::GdipFillRectanglesI(nativeGraphics, brush ? getNat(brush) : NULL, rects, count));
    }

    Status
    FillRectangles(const Brush *brush, const RectF *rects, INT count)
    {
        return SetStatus(DllExports::GdipFillRectangles(nativeGraphics, brush ? getNat(brush) : NULL, rects, count));
    }

    Status
    FillRegion(const Brush *brush, const Region *region)
    {
        return SetStatus(DllExports::GdipFillRegion(nativeGraphics, getNat(brush), getNat(region)));
    }

    VOID
    Flush(FlushIntention intention)
    {
        DllExports::GdipFlush(nativeGraphics, intention);
    }

    static Graphics *
    FromHDC(HDC hdc)
    {
        return new Graphics(hdc);
    }

    static Graphics *
    FromHDC(HDC hdc, HANDLE hDevice)
    {
        return new Graphics(hdc, hDevice);
    }

    static Graphics *
    FromHWND(HWND hWnd, BOOL icm)
    {
        return new Graphics(hWnd, icm);
    }

    static Graphics *
    FromImage(Image *image)
    {
        return new Graphics(image);
    }

    Status
    GetClip(Region *region) const
    {
        return SetStatus(DllExports::GdipGetClip(nativeGraphics, getNat(region)));
    }

    Status
    GetClipBounds(Rect *rect) const
    {
        return SetStatus(DllExports::GdipGetClipBoundsI(nativeGraphics, rect));
    }

    Status
    GetClipBounds(RectF *rect) const
    {
        return SetStatus(DllExports::GdipGetClipBounds(nativeGraphics, rect));
    }

    CompositingMode
    GetCompositingMode() const
    {
        CompositingMode compositingMode;
        SetStatus(DllExports::GdipGetCompositingMode(nativeGraphics, &compositingMode));
        return compositingMode;
    }

    CompositingQuality
    GetCompositingQuality() const
    {
        CompositingQuality compositingQuality;
        SetStatus(DllExports::GdipGetCompositingQuality(nativeGraphics, &compositingQuality));
        return compositingQuality;
    }

    REAL
    GetDpiX() const
    {
        REAL dpi;
        SetStatus(DllExports::GdipGetDpiX(nativeGraphics, &dpi));
        return dpi;
    }

    REAL
    GetDpiY() const
    {
        REAL dpi;
        SetStatus(DllExports::GdipGetDpiY(nativeGraphics, &dpi));
        return dpi;
    }

    static HPALETTE
    GetHalftonePalette()
    {
        return NULL;
    }

    HDC
    GetHDC()
    {
        HDC hdc = NULL;
        SetStatus(DllExports::GdipGetDC(nativeGraphics, &hdc));
        return hdc;
    }

    InterpolationMode
    GetInterpolationMode() const
    {
        InterpolationMode interpolationMode;
        SetStatus(DllExports::GdipGetInterpolationMode(nativeGraphics, &interpolationMode));
        return interpolationMode;
    }

    Status
    GetLastStatus() const
    {
        return lastStatus;
    }

    Status
    GetNearestColor(Color *color) const
    {
        if (color == NULL)
            return SetStatus(InvalidParameter);

        ARGB argb = color->GetValue();
        SetStatus(DllExports::GdipGetNearestColor(nativeGraphics, &argb));

        color->SetValue(argb);
        return lastStatus;
    }

    REAL
    GetPageScale() const
    {
        REAL scale;
        SetStatus(DllExports::GdipGetPageScale(nativeGraphics, &scale));
        return scale;
    }

    Unit
    GetPageUnit() const
    {
        Unit unit;
        SetStatus(DllExports::GdipGetPageUnit(nativeGraphics, &unit));
        return unit;
    }

    PixelOffsetMode
    GetPixelOffsetMode() const
    {
        PixelOffsetMode pixelOffsetMode;
        SetStatus(DllExports::GdipGetPixelOffsetMode(nativeGraphics, &pixelOffsetMode));
        return pixelOffsetMode;
    }

    Status
    GetRenderingOrigin(INT *x, INT *y) const
    {
        return SetStatus(DllExports::GdipGetRenderingOrigin(nativeGraphics, x, y));
    }

    SmoothingMode
    GetSmoothingMode() const
    {
        SmoothingMode smoothingMode;
        SetStatus(DllExports::GdipGetSmoothingMode(nativeGraphics, &smoothingMode));
        return smoothingMode;
    }

    UINT
    GetTextContrast() const
    {
        UINT contrast;
        SetStatus(DllExports::GdipGetTextContrast(nativeGraphics, &contrast));
        return contrast;
    }

    TextRenderingHint
    GetTextRenderingHint() const
    {
        TextRenderingHint mode;
        SetStatus(DllExports::GdipGetTextRenderingHint(nativeGraphics, &mode));
        return mode;
    }

    UINT
    GetTextGammaValue() const
    {
#if 1
        return SetStatus(NotImplemented); // FIXME
#else
        UINT gammaValue;
        SetStatus(DllExports::GdipGetTextGammaValue(nativeGraphics, &gammaValue));
        return gammaValue;
#endif
    }

    Status
    GetTransform(Matrix *matrix) const
    {
        return SetStatus(DllExports::GdipGetWorldTransform(nativeGraphics, getNat(matrix)));
    }

    Status
    GetVisibleClipBounds(Rect *rect) const
    {
        return SetStatus(DllExports::GdipGetVisibleClipBoundsI(nativeGraphics, rect));
    }

    Status
    GetVisibleClipBounds(RectF *rect) const
    {
        return SetStatus(DllExports::GdipGetVisibleClipBounds(nativeGraphics, rect));
    }

    Status
    IntersectClip(const Rect &rect)
    {
        return SetStatus(DllExports::GdipSetClipRectI(
            nativeGraphics, rect.X, rect.Y, rect.Width, rect.Height, CombineModeIntersect));
    }

    Status
    IntersectClip(const Region *region)
    {
        return SetStatus(DllExports::GdipSetClipRegion(nativeGraphics, getNat(region), CombineModeIntersect));
    }

    Status
    IntersectClip(const RectF &rect)
    {
        return SetStatus(
            DllExports::GdipSetClipRect(nativeGraphics, rect.X, rect.Y, rect.Width, rect.Height, CombineModeIntersect));
    }

    BOOL
    IsClipEmpty() const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsClipEmpty(nativeGraphics, &result));
        return result;
    }

    BOOL
    IsVisible(const Point &point) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisiblePointI(nativeGraphics, point.X, point.Y, &result));
        return result;
    }

    BOOL
    IsVisible(const Rect &rect) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRectI(nativeGraphics, rect.X, rect.Y, rect.Width, rect.Height, &result));
        return result;
    }

    BOOL
    IsVisible(REAL x, REAL y) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisiblePoint(nativeGraphics, x, y, &result));
        return result;
    }

    BOOL
    IsVisible(const RectF &rect) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRect(nativeGraphics, rect.X, rect.Y, rect.Width, rect.Height, &result));
        return result;
    }

    BOOL
    IsVisible(INT x, INT y, INT width, INT height) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRectI(nativeGraphics, x, y, width, height, &result));
        return result;
    }

    BOOL
    IsVisible(INT x, INT y) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisiblePointI(nativeGraphics, x, y, &result));
        return result;
    }

    BOOL
    IsVisible(const PointF &point) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisiblePoint(nativeGraphics, point.X, point.Y, &result));
        return result;
    }

    BOOL
    IsVisible(REAL x, REAL y, REAL width, REAL height) const
    {
        BOOL result;
        SetStatus(DllExports::GdipIsVisibleRect(nativeGraphics, x, y, width, height, &result));
        return result;
    }

    BOOL
    IsVisibleClipEmpty() const
    {
        BOOL flag = FALSE;
        SetStatus(DllExports::GdipIsVisibleClipEmpty(nativeGraphics, &flag));
        return flag;
    }

    Status
    MeasureCharacterRanges(
        const WCHAR *string,
        INT length,
        const Font *font,
        const RectF &layoutRect,
        const StringFormat *stringFormat,
        INT regionCount,
        Region *regions) const
    {
        return NotImplemented;
    }

    Status
    MeasureDriverString(
        const UINT16 *text,
        INT length,
        const Font *font,
        const PointF *positions,
        INT flags,
        const Matrix *matrix,
        RectF *boundingBox) const
    {
        return NotImplemented;
    }

    Status
    MeasureString(const WCHAR *string, INT length, const Font *font, const RectF &layoutRect, RectF *boundingBox) const
    {
        return NotImplemented;
    }

    Status
    MeasureString(
        const WCHAR *string,
        INT length,
        const Font *font,
        const PointF &origin,
        const StringFormat *stringFormat,
        RectF *boundingBox) const
    {
        return NotImplemented;
    }

    Status
    MeasureString(
        const WCHAR *string,
        INT length,
        const Font *font,
        const RectF &layoutRect,
        const StringFormat *stringFormat,
        RectF *boundingBox,
        INT *codepointsFitted,
        INT *linesFilled) const
    {
        return NotImplemented;
    }

    Status
    MeasureString(
        const WCHAR *string,
        INT length,
        const Font *font,
        const SizeF &layoutRectSize,
        const StringFormat *stringFormat,
        SizeF *size,
        INT *codepointsFitted,
        INT *linesFilled) const
    {
        return NotImplemented;
    }

    Status
    MeasureString(const WCHAR *string, INT length, const Font *font, const PointF &origin, RectF *boundingBox) const
    {
        return NotImplemented;
    }

    Status
    MultiplyTransform(Matrix *matrix, MatrixOrder order)
    {
        return SetStatus(DllExports::GdipMultiplyWorldTransform(nativeGraphics, getNat(matrix), order));
    }

    VOID
    ReleaseHDC(HDC hdc)
    {
        SetStatus(DllExports::GdipReleaseDC(nativeGraphics, hdc));
    }

    Status
    ResetClip()
    {
        return SetStatus(DllExports::GdipResetClip(nativeGraphics));
    }

    Status
    ResetTransform()
    {
        return SetStatus(DllExports::GdipResetWorldTransform(nativeGraphics));
    }

    Status
    Restore(GraphicsState gstate)
    {
        return SetStatus(DllExports::GdipRestoreGraphics(nativeGraphics, gstate));
    }

    Status
    RotateTransform(REAL angle, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipRotateWorldTransform(nativeGraphics, angle, order));
    }

    GraphicsState
    Save()
    {
        GraphicsState gstate;
        SetStatus(DllExports::GdipSaveGraphics(nativeGraphics, &gstate));
        return gstate;
    }

    Status
    ScaleTransform(REAL sx, REAL sy, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipScaleWorldTransform(nativeGraphics, sx, sy, order));
    }

    Status
    SetClip(const Graphics *g, CombineMode combineMode = CombineModeReplace)
    {
        return SetStatus(DllExports::GdipSetClipGraphics(nativeGraphics, g ? getNat(g) : NULL, combineMode));
    }

    Status
    SetClip(const GraphicsPath *path, CombineMode combineMode = CombineModeReplace)
    {
        return SetStatus(DllExports::GdipSetClipPath(nativeGraphics, getNat(path), combineMode));
    }

    Status
    SetClip(const Region *region, CombineMode combineMode = CombineModeReplace)
    {
        return SetStatus(DllExports::GdipSetClipRegion(nativeGraphics, getNat(region), combineMode));
    }

    Status
    SetClip(const Rect &rect, CombineMode combineMode = CombineModeReplace)
    {
        return SetStatus(
            DllExports::GdipSetClipRectI(nativeGraphics, rect.X, rect.Y, rect.Width, rect.Height, combineMode));
    }

    Status
    SetClip(HRGN hRgn, CombineMode combineMode = CombineModeReplace)
    {
        return SetStatus(DllExports::GdipSetClipHrgn(nativeGraphics, hRgn, combineMode));
    }

    Status
    SetClip(const RectF &rect, CombineMode combineMode = CombineModeReplace)
    {
        return SetStatus(
            DllExports::GdipSetClipRect(nativeGraphics, rect.X, rect.Y, rect.Width, rect.Height, combineMode));
    }

    Status
    SetCompositingMode(CompositingMode compositingMode)
    {
        return SetStatus(DllExports::GdipSetCompositingMode(nativeGraphics, compositingMode));
    }

    Status
    SetCompositingQuality(CompositingQuality compositingQuality)
    {
        return SetStatus(DllExports::GdipSetCompositingQuality(nativeGraphics, compositingQuality));
    }

    Status
    SetInterpolationMode(InterpolationMode interpolationMode)
    {
        return SetStatus(DllExports::GdipSetInterpolationMode(nativeGraphics, interpolationMode));
    }

    Status
    SetPageScale(REAL scale)
    {
        return SetStatus(DllExports::GdipSetPageScale(nativeGraphics, scale));
    }

    Status
    SetPageUnit(Unit unit)
    {
        return SetStatus(DllExports::GdipSetPageUnit(nativeGraphics, unit));
    }

    Status
    SetPixelOffsetMode(PixelOffsetMode pixelOffsetMode)
    {
        return SetStatus(DllExports::GdipSetPixelOffsetMode(nativeGraphics, pixelOffsetMode));
    }

    Status
    SetRenderingOrigin(INT x, INT y)
    {
        return SetStatus(DllExports::GdipSetRenderingOrigin(nativeGraphics, x, y));
    }

    Status
    SetSmoothingMode(SmoothingMode smoothingMode)
    {
        return SetStatus(DllExports::GdipSetSmoothingMode(nativeGraphics, smoothingMode));
    }

    Status
    SetTextContrast(UINT contrast)
    {
        return SetStatus(DllExports::GdipSetTextContrast(nativeGraphics, contrast));
    }

    Status
    SetTextRenderingHint(TextRenderingHint newMode)
    {
        return SetStatus(DllExports::GdipSetTextRenderingHint(nativeGraphics, newMode));
    }

    Status
    SetTransform(const Matrix *matrix)
    {
        return SetStatus(DllExports::GdipSetWorldTransform(nativeGraphics, getNat(matrix)));
    }

    Status
    TransformPoints(CoordinateSpace destSpace, CoordinateSpace srcSpace, Point *pts, INT count)
    {
        return SetStatus(DllExports::GdipTransformPointsI(nativeGraphics, destSpace, srcSpace, pts, count));
    }

    Status
    TransformPoints(CoordinateSpace destSpace, CoordinateSpace srcSpace, PointF *pts, INT count)
    {
        return SetStatus(DllExports::GdipTransformPoints(nativeGraphics, destSpace, srcSpace, pts, count));
    }

    Status
    TranslateClip(INT dx, INT dy)
    {
        return SetStatus(DllExports::GdipTranslateClipI(nativeGraphics, dx, dy));
    }

    Status
    TranslateClip(REAL dx, REAL dy)
    {
        return SetStatus(DllExports::GdipTranslateClip(nativeGraphics, dx, dy));
    }

    Status
    TranslateTransform(REAL dx, REAL dy, MatrixOrder order = MatrixOrderPrepend)
    {
        return SetStatus(DllExports::GdipTranslateWorldTransform(nativeGraphics, dx, dy, order));
    }

  private:
    Status
    SetStatus(Status status) const
    {
        if (status != Ok)
            lastStatus = status;
        return status;
    }

    VOID
    SetNativeGraphics(GpGraphics *graphics)
    {
        nativeGraphics = graphics;
    }

  protected:
    GpGraphics *nativeGraphics;
    mutable Status lastStatus;

    // get native
    friend inline GpGraphics *&
    getNat(const Graphics *graphics)
    {
        return const_cast<Graphics *>(graphics)->nativeGraphics;
    }
};

#endif /* _GDIPLUSGRAPHICS_H */
