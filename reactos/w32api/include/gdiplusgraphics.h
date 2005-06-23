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

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

class Graphics : public GdiplusBase
{
public:
  Graphics(Image *image)
  {
  }

  Graphics(HDC hdc)
  {
    GpGraphics *graphics = NULL;
    status = DllExports::GdipCreateFromHDC(hdc, &graphics);
    SetGraphics(graphics);
  }

  Graphics(HDC hdc, HANDLE hdevice)
  {
  }

  Graphics(HWND hwnd, BOOL icm)
  {
  }

  Status AddMetafileComment(const BYTE *data, UINT sizeData)
  {
    return NotImplemented;
  }

  GraphicsContainer BeginContainer(VOID)
  {
    return GraphicsContainer();
  }

  GraphicsContainer BeginContainer(const RectF &dstrect, const RectF &srcrect, Unit unit)
  {
    return GraphicsContainer();
  }

  GraphicsContainer BeginContainer(const Rect &dstrect, const Rect &srcrect, Unit unit)
  {
    return GraphicsContainer();
  }

  Status Clear(const Color &color)
  {
    return NotImplemented;
  }

  Status DrawArc(const Pen *pen, const Rect &rect, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status DrawArc(const Pen *pen, const RectF &rect, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status DrawArc(const Pen *pen, REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status DrawArc(const Pen *pen, INT x, INT y, INT width, INT height, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status DrawBezier(const Pen *pen, const Point &pt1, const Point &pt2, const Point &pt3, const Point &pt4)
  {
    return NotImplemented;
  }

  Status DrawBezier(const Pen *pen, const PointF &pt1, const PointF &pt2, const PointF &pt3, const PointF &pt4)
  {
    return NotImplemented;
  }

  Status DrawBezier(const Pen *pen, REAL x1, REAL y1, REAL x2, REAL y2, REAL x3, REAL y3, REAL x4, REAL y4)
  {
    return NotImplemented;
  }

  Status DrawBezier(const Pen *pen, INT x1, INT y1, INT x2, INT y2, INT x3, INT y3, INT x4, INT y4)
  {
    return NotImplemented;
  }

  Status DrawBeziers(const Pen *pen, const Point *points, INT count)
  {
    return NotImplemented;
  }

  Status DrawBeziers(const Pen *pen, const PointF *points, INT count)
  {
    return NotImplemented;
  }

  Status DrawCachedBitmap(CachedBitmap *cb, INT x, INT y)
  {
    return NotImplemented;
  }

  Status DrawClosedCurve(const Pen *pen, const Point *points, INT count)
  {
    return NotImplemented;
  }

  Status DrawClosedCurve(const Pen *pen, const PointF *points, INT count)
  {
    return NotImplemented;
  }

  Status DrawClosedCurve(const Pen *pen, const PointF *points, INT count, REAL tension)
  {
    return NotImplemented;
  }

  Status DrawClosedCurve(const Pen *pen, const Point *points, INT count, REAL tension)
  {
    return NotImplemented;
  }

  Status DrawCurve(const Pen *pen, const Point *points, INT count)
  {
    return NotImplemented;
  }

  Status DrawCurve(const Pen *pen, const PointF *points, INT count)
  {
    return NotImplemented;
  }

  Status DrawCurve(const Pen *pen, const PointF *points, INT count, REAL tension)
  {
    return NotImplemented;
  }

  Status DrawCurve(const Pen *pen, const Point *points, INT count, INT offset, INT numberOfSegments, REAL tension)
  {
    return NotImplemented;
  }

  Status DrawCurve(const Pen *pen, const PointF *points, INT count, INT offset, INT numberOfSegments, REAL tension)
  {
    return NotImplemented;
  }

  Status DrawCurve(const Pen *pen, const Point *points, INT count, REAL tension)
  {
    return NotImplemented;
  }

  Status DrawDriverString(const UINT16 *text, INT length, const Font *font, const Brush *brush, const PointF *positions, INT flags, const Matrix *matrix)
  {
    return NotImplemented;
  }

  Status DrawEllipse(const Pen *pen, const Rect &rect)
  {
    return NotImplemented;
  }

  Status DrawEllipse(const Pen *pen, REAL x, REAL y, REAL width, REAL height)
  {
    return NotImplemented;
  }

  Status DrawEllipse(const Pen *pen, const RectF &rect)
  {
    return NotImplemented;
  }

  Status DrawEllipse(const Pen *pen, INT x, INT y, INT width, INT height)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, const Point *destPoints, INT count)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, INT x, INT y)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, const Point &point)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, REAL x, REAL y)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, const PointF &point)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, const PointF *destPoints, INT count)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, REAL x, REAL y, REAL srcx, REAL srcy, REAL srcwidth, REAL srcheight, Unit srcUnit)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, const RectF &rect)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, INT x, INT y, INT width, INT height)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, const PointF *destPoints, INT count, REAL srcx, REAL srcy, REAL srcwidth, REAL srcheight, Unit srcUnit, ImageAttributes *imageAttributes, DrawImageAbort callback, VOID *callbackData)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, const Rect &destRect, INT srcx, INT srcy, INT srcwidth, INT srcheight, Unit srcUnit, ImageAttributes *imageAttributes, DrawImageAbort callback, VOID *callbackData)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, const Point *destPoints, INT count, INT srcx, INT srcy, INT srcwidth, INT srcheight, Unit srcUnit, ImageAttributes *imageAttributes, DrawImageAbort callback, VOID *callbackData)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, REAL x, REAL y, REAL width, REAL height)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, const Rect &rect)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, INT x, INT y, INT srcx, INT srcy, INT srcwidth, INT srcheight, Unit srcUnit)
  {
    return NotImplemented;
  }

  Status DrawImage(Image *image, const RectF &destRect, REAL srcx, REAL srcy, REAL srcwidth, REAL srcheight, Unit srcUnit, ImageAttributes *imageAttributes, DrawImageAbort callback, VOID *callbackData)
  {
    return NotImplemented;
  }

  Status DrawLine(const Pen *pen, const Point &pt1, const Point &pt2)
  {
    return NotImplemented;
  }

  Status DrawLine(const Pen *pen, const PointF &pt1, const Point &pt2)
  {
    return NotImplemented;
  }

  Status DrawLine(const Pen *pen, REAL x1, REAL y1, REAL x2, REAL y2)
  {
    return NotImplemented;
  }

  Status DrawLine(const Pen *pen, INT x1, INT y1, INT x2, INT y2)
  {
    return SetStatus(DllExports::GdipDrawLine(graphics,
      pen->pen,
      x1,
      y1,
      x2,
      y2));
  }

  Status DrawLines(const Pen *pen, const Point *points, INT count)
  {
    return NotImplemented;
  }

  Status DrawLines(const Pen *pen, const PointF *points, INT count)
  {
    return NotImplemented;
  }

  Status DrawPath(const Pen *pen, const GraphicsPath *path)
  {
    return NotImplemented;
  }

  Status DrawPie(const Pen *pen, const Rect &rect, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status DrawPie(const Pen *pen, INT x, INT y, INT width, INT height, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status DrawPie(const Pen *pen, REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status DrawPie(const Pen *pen, const RectF &rect, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status DrawPolygon(const Pen *pen, const Point *points, INT *count)
  {
    return NotImplemented;
  }

  Status DrawPolygon(const Pen *pen, const PointF *points, INT *count)
  {
    return NotImplemented;
  }

  Status DrawRectangle(const Pen *pen, const Rect &rect)
  {
    return NotImplemented;
  }

  Status DrawRectangle(const Pen *pen, INT x, INT y, INT width, INT height)
  {
    return NotImplemented;
  }

  Status DrawRectangle(const Pen *pen, REAL x, REAL y, REAL width, REAL height)
  {
    return NotImplemented;
  }

  Status DrawRectangle(const Pen *pen, const RectF &rect)
  {
    return NotImplemented;
  }

  Status DrawRectangles(const Pen *pen, const Rect *rects, INT count)
  {
    return NotImplemented;
  }

  Status DrawRectangles(const Pen *pen, const RectF *rects, INT count)
  {
    return NotImplemented;
  }

  Status DrawString(const WCHAR *string, INT length, const Font *font, const RectF &layoutRect, const StringFormat *stringFormat, const Brush *brush)
  {
    return NotImplemented;
  }

  Status DrawString(const WCHAR *string, INT length, const Font *font, const PointF &origin, const Brush *brush)
  {
    return NotImplemented;
  }

  Status DrawString(const WCHAR *string, INT length, const Font *font, const PointF &origin, const StringFormat *stringFormat, const Brush *brush)
  {
    return NotImplemented;
  }

  Status EndContainer(GraphicsContainer state)
  {
    return NotImplemented;
  }

  Status EnumerateMetafile(const Metafile *metafile, const Metafile &destPoint, EnumerateMetafileProc callback, VOID *callbackData, ImageAttributes *imageAttributes)
  {
    return NotImplemented;
  }

  Status EnumerateMetafile(const Metafile *metafile, const Point *destPoints, INT count, EnumerateMetafileProc callback, VOID *callbackData, ImageAttributes *imageAttributes)
  {
    return NotImplemented;
  }

  Status EnumerateMetafile(const Metafile *metafile, const Point &destPoint, const Rect &srcRect, Unit srcUnit, EnumerateMetafileProc callback, VOID *callbackData, ImageAttributes *imageAttributes)
  {
    return NotImplemented;
  }

  Status EnumerateMetafile(const Metafile *metafile, const PointF *destPoints, INT count, EnumerateMetafileProc callback, VOID *callbackData, ImageAttributes *imageAttributes)
  {
    return NotImplemented;
  }

  Status EnumerateMetafile(const Metafile *metafile, const Rect &destRect, EnumerateMetafileProc callback, VOID *callbackData, ImageAttributes *imageAttributes)
  {
    return NotImplemented;
  }

  Status EnumerateMetafile(const Metafile *metafile, const RectF &destRect, const RectF &srcRect, Unit srcUnit, EnumerateMetafileProc callback, VOID *callbackData, ImageAttributes *imageAttributes)
  {
    return NotImplemented;
  }

  Status EnumerateMetafile(const Metafile *metafile, const RectF &destRect, EnumerateMetafileProc callback, VOID *callbackData, ImageAttributes *imageAttributes)
  {
    return NotImplemented;
  }

  Status EnumerateMetafile(const Metafile *metafile, const PointF &destPoint, const Rect &srcRect, Unit srcUnit, EnumerateMetafileProc callback, VOID *callbackData, ImageAttributes *imageAttributes)
  {
    return NotImplemented;
  }

  Status EnumerateMetafile(const Metafile *metafile, const Point *destPoints, INT count, const Rect &srcRect, Unit srcUnit, EnumerateMetafileProc callback, VOID *callbackData, ImageAttributes *imageAttributes)
  {
    return NotImplemented;
  }

  Status EnumerateMetafile(const Metafile *metafile, const Rect &destRect, const Rect &srcRect, Unit srcUnit, EnumerateMetafileProc callback, VOID *callbackData, ImageAttributes *imageAttributes)
  {
    return NotImplemented;
  }

  Status EnumerateMetafile(const Metafile *metafile, const Point *destPoints, INT count, const RectF &srcRect, Unit srcUnit, EnumerateMetafileProc callback, VOID *callbackData, ImageAttributes *imageAttributes)
  {
    return NotImplemented;
  }

  Status EnumerateMetafile(const Metafile *metafile, const PointF &destPoint, EnumerateMetafileProc callback, VOID *callbackData, ImageAttributes *imageAttributes)
  {
    return NotImplemented;
  }

  Status ExcludeClip(const Rect& rect)
  {
    return NotImplemented;
  }

  Status ExcludeClip(const RectF &rect)
  {
    return NotImplemented;
  }

  Status ExcludeClip(const Region *region)
  {
    return NotImplemented;
  }

  Status FillClosedCurve(const Brush *brush, const Point *points, INT count)
  {
    return NotImplemented;
  }

  Status FillClosedCurve(const Brush *brush, const Point *points, INT count, FillMode fillMode, REAL tension)
  {
    return NotImplemented;
  }

  Status FillClosedCurve(const Brush *brush, const PointF *points, INT count)
  {
    return NotImplemented;
  }

  Status FillClosedCurve(const Brush *brush, const PointF *points, INT count, FillMode fillMode, REAL tension)
  {
    return NotImplemented;
  }

  Status FillEllipse(const Brush *brush, const Rect &rect)
  {
    return NotImplemented;
  }

  Status FillEllipse(const Brush *brush, REAL x, REAL y, REAL width, REAL height)
  {
    return NotImplemented;
  }

  Status FillEllipse(const Brush *brush, const RectF &rect)
  {
    return NotImplemented;
  }

  Status FillEllipse(const Brush *brush, INT x, INT y, INT width, INT height)
  {
    return NotImplemented;
  }

  Status FillPath(const Brush *brush, const GraphicsPath *path)
  {
    return NotImplemented;
  }

  Status FillPie(const Brush *brush, const Rect &rect, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status FillPie(const Brush *brush, INT x, INT y, INT width, INT height, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status FillPie(const Brush *brush, REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status FillPie(const Brush *brush, RectF &rect, REAL startAngle, REAL sweepAngle)
  {
    return NotImplemented;
  }

  Status FillPolygon(const Brush *brush, const Point *points, INT count)
  {
    return NotImplemented;
  }

  Status FillPolygon(const Brush *brush, const PointF *points, INT count)
  {
    return NotImplemented;
  }

  Status FillPolygon(const Brush *brush, const Point *points, INT count, FillMode fillMode)
  {
    return NotImplemented;
  }

  Status FillPolygon(const Brush *brush, const PointF *points, INT count, FillMode fillMode)
  {
    return NotImplemented;
  }

  Status FillRectangle(const Brush *brush, const Rect &rect)
  {
    return NotImplemented;
  }

  Status FillRectangle(const Brush *brush, const RectF &rect)
  {
    return NotImplemented;
  }

  Status FillRectangle(const Brush *brush, REAL x, REAL y, REAL width, REAL height)
  {
    return NotImplemented;
  }

  Status FillRectangle(const Brush *brush, INT x, INT y, INT width, INT height)
  {
    return NotImplemented;
  }

  Status FillRectangles(const Brush *brush, const Rect *rects, INT count)
  {
    return NotImplemented;
  }

  Status FillRectangles(const Brush *brush, const RectF *rects, INT count)
  {
    return NotImplemented;
  }

  Status FillRegion(const Brush *brush, const Region *region)
  {
    return NotImplemented;
  }

  VOID Flush(FlushIntention intention)
  {
  }

  static Graphics *FromHDC(HDC hdc)
  {
    return NULL;
  }

  static Graphics *FromHDC(HDC hdc, HANDLE hDevice)
  {
    return NULL;
  }

  static Graphics *FromHWND(HWND hWnd, BOOL icm)
  {
    return NULL;
  }

  static Graphics *FromImage(Image *image)
  {
    return NULL;
  }

  Status GetClip(Region *region) const
  {
    return NotImplemented;
  }

  Status GetClipBounds(Rect* rect) const
  {
    return NotImplemented;
  }

  Status GetClipBounds(RectF* rect) const
  {
    return NotImplemented;
  }

  CompositingMode GetCompositingMode(VOID)
  {
    return CompositingModeSourceOver;
  }

  CompositingQuality GetCompositingQuality(VOID)
  {
    return CompositingQualityDefault;
  }

  REAL GetDpiX(VOID)
  {
    return 0;
  }

  REAL GetDpiY(VOID)
  {
    return 0;
  }

  static HPALETTE GetHalftonePalette(VOID)
  {
    return NULL;
  }

  HDC GetHDC(VOID)
  {
    return NULL;
  }

  InterpolationMode GetInterpolationMode(VOID)
  {
    return InterpolationModeInvalid;
  }

  Status GetLastStatus(VOID)
  {
    return NotImplemented;
  }

  Status GetNearestColor(Color* color) const
  {
    return NotImplemented;
  }

  REAL GetPageScale(VOID)
  {
    return 0;
  }

  Unit GetPageUnit(VOID)
  {
    return UnitWorld;
  }

  PixelOffsetMode GetPixelOffsetMode(VOID)
  {
    return PixelOffsetModeInvalid;
  }

  Status GetRenderingOrigin(INT *x, INT *y)
  {
    return NotImplemented;
  }

  SmoothingMode GetSmoothingMode(VOID) const
  {
    return SmoothingModeInvalid;
  }

  UINT GetTextContrast(VOID) const
  {
    return 0;
  }

  TextRenderingHint GetTextRenderingHint(VOID) const
  {
    return TextRenderingHintSystemDefault;
  }

  Status GetTransform(Matrix* matrix)
  {
    return NotImplemented;
  }

  Status GetVisibleClipBounds(Rect* rect) const
  {
    return NotImplemented;
  }

  Status GetVisibleClipBounds(RectF* rect) const
  {
    return NotImplemented;
  }

  Status IntersectClip(const Rect& rect)
  {
    return NotImplemented;
  }

  Status IntersectClip(const Region* region)
  {
    return NotImplemented;
  }

  Status IntersectClip(const RectF& rect)
  {
    return NotImplemented;
  }

  BOOL IsClipEmpty(VOID) const
  {
    return FALSE;
  }

  BOOL IsVisible(const Point& point) const
  {
    return FALSE;
  }

  BOOL IsVisible(const Rect& rect) const
  {
    return FALSE;
  }

  BOOL IsVisible(REAL x, REAL y) const
  {
    return FALSE;
  }

  BOOL IsVisible(const RectF& rect) const
  {
    return FALSE;
  }

  BOOL IsVisible(INT x, INT y, INT width, INT height) const
  {
    return FALSE;
  }

  BOOL IsVisible(INT x, INT y) const
  {
    return FALSE;
  }

  BOOL IsVisible(const PointF& point) const
  {
    return FALSE;
  }

  BOOL IsVisible(REAL x, REAL y, REAL width, REAL height) const
  {
    return FALSE;
  }

  BOOL IsVisibleClipEmpty(VOID) const
  {
    return FALSE;
  }

  Status MeasureCharacterRanges(const WCHAR *string, INT length, const Font *font, const RectF &layoutRect, const StringFormat *stringFormat, INT regionCount, Region *regions) const
  {
    return NotImplemented;
  }

  Status MeasureDriverString(const UINT16 *text, INT length, const Font *font, const PointF *positions, INT flags, const Matrix *matrix, RectF *boundingBox) const
  {
    return NotImplemented;
  }

  Status MeasureString(const WCHAR *string, INT length, const Font *font, const RectF &layoutRect, RectF *boundingBox) const
  {
    return NotImplemented;
  }

  Status MeasureString(const WCHAR *string, INT length, const Font *font, const PointF &origin, const StringFormat *stringFormat, RectF *boundingBox) const
  {
    return NotImplemented;
  }

  Status MeasureString(const WCHAR *string, INT length, const Font *font, const RectF &layoutRect, const StringFormat *stringFormat, RectF *boundingBox, INT *codepointsFitted, INT *linesFilled) const
  {
    return NotImplemented;
  }

  Status MeasureString(const WCHAR *string, INT length, const Font *font, const SizeF &layoutRectSize, const StringFormat *stringFormat, SizeF *size, INT *codepointsFitted, INT *linesFilled) const
  {
    return NotImplemented;
  }

  Status MeasureString(const WCHAR *string, INT length, const Font *font, const PointF &origin, RectF *boundingBox) const
  {
    return NotImplemented;
  }

  Status MultiplyTransform(Matrix *matrix, MatrixOrder order)
  {
    return NotImplemented;
  }

  VOID ReleaseHDC(HDC hdc)
  {
  }

  Status ResetClip(VOID)
  {
    return NotImplemented;
  }

  Status ResetTransform(VOID)
  {
    return NotImplemented;
  }

  Status Restore(GraphicsState gstate)
  {
    return NotImplemented;
  }

  Status RotateTransform(REAL angle, MatrixOrder order)
  {
    return NotImplemented;
  }

  GraphicsState Save(VOID)
  {
    return 0;
  }

  Status ScaleTransform(REAL sx, REAL sy, MatrixOrder order)
  {
    return NotImplemented;
  }

  Status SetClip(const Graphics *g, CombineMode combineMode)
  {
    return NotImplemented;
  }

  Status SetClip(const GraphicsPath *path, CombineMode combineMode)
  {
    return NotImplemented;
  }

  Status SetClip(const Region *region, CombineMode combineMode)
  {
    return NotImplemented;
  }

  Status SetClip(const Rect &rect, CombineMode combineMode)
  {
    return NotImplemented;
  }

  Status SetClip(HRGN hRgn, CombineMode combineMode)
  {
    return NotImplemented;
  }

  Status SetClip(const RectF& rect, CombineMode combineMode)
  {
    return NotImplemented;
  }

  Status SetCompositingMode(CompositingMode compositingMode)
  {
    return NotImplemented;
  }

  Status SetCompositingQuality(CompositingQuality compositingQuality)
  {
    return NotImplemented;
  }

  Status SetInterpolationMode(InterpolationMode interpolationMode)
  {
    return NotImplemented;
  }

  Status SetPageScale(REAL scale)
  {
    return NotImplemented;
  }

  Status SetPageUnit(Unit unit)
  {
    return NotImplemented;
  }

  Status SetPixelOffsetMode(PixelOffsetMode pixelOffsetMode)
  {
    return NotImplemented;
  }

  Status SetRenderingOrigin(INT x, INT y)
  {
    return NotImplemented;
  }

  Status SetSmoothingMode(SmoothingMode smoothingMode)
  {
    return NotImplemented;
  }

  Status SetTextContrast(UINT contrast)
  {
    return NotImplemented;
  }

  Status SetTextRenderingHint(TextRenderingHint newMode)
  {
    return NotImplemented;
  }

  Status SetTransform(const Matrix *matrix)
  {
    return NotImplemented;
  }

  Status TransformPoints(CoordinateSpace destSpace, CoordinateSpace srcSpace, Point *pts, INT count)
  {
    return NotImplemented;
  }

  Status TranslateClip(INT dx, INT dy)
  {
    return NotImplemented;
  }

  Status TranslateClip(REAL dx, REAL dy)
  {
    return NotImplemented;
  }

  Status TranslateTransform(REAL dx, REAL dy, MatrixOrder order)
  {
    return NotImplemented;
  }

private:
  Status SetStatus(Status status) const
  {
    if (status == Ok)
      return status;
    this->status = status;
    return status;
  }

  VOID SetGraphics(GpGraphics *graphics)
  {
    this->graphics = graphics;
  }

private:
  mutable Status status;
  GpGraphics *graphics;
};

#endif /* _GDIPLUSGRAPHICS_H */
