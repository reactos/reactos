/*
 * GdiPlusBrush.h
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

#ifndef _GDIPLUSBRUSH_H
#define _GDIPLUSBRUSH_H

class Brush : public GdiplusBase
{
public:
  Brush *Clone(VOID) const
  {
    return NULL;
  }

  Status GetLastStatus(VOID)
  {
    return NotImplemented;
  }

  BrushType GetType(VOID)
  {
    return BrushTypeSolidColor;
  }
};


class HatchBrush : public Brush
{
public:
  HatchBrush(HatchStyle hatchStyle, const Color &foreColor, const Color &backColor)
  {
  }

  Status GetBackgroundColor(Color *color) const
  {
    return NotImplemented;
  }

  Status GetForegroundColor(Color *color) const
  {
    return NotImplemented;
  }

  HatchStyle GetHatchStyle(VOID) const
  {
    return HatchStyleHorizontal;
  }
};


class LinearGradientBrush : public Brush
{
public:
  LinearGradientBrush(const PointF &point1, const PointF &point2, const Color &color1, const Color &color2)
  {
  }

  LinearGradientBrush(const Rect &rect, const Color &color1, const Color &color2, REAL angle, BOOL isAngleScalable)
  {
  }

  LinearGradientBrush(const Rect &rect, const Color &color1, const Color &color2, LinearGradientMode mode)
  {
  }

  LinearGradientBrush(const Point &point1, const Point &point2, const Color &color1, const Color &color2)
  {
  }

  LinearGradientBrush(const RectF &rect, const Color &color1, const Color &color2, REAL angle, BOOL isAngleScalable)
  {
  }

  LinearGradientBrush(const RectF &rect, const Color &color1, const Color &color2, LinearGradientMode mode)
  {
  }

  Status GetBlend(REAL *blendFactors, REAL *blendPositions, INT count)
  {
    return NotImplemented;
  }

  INT GetBlendCount(VOID) const
  {
    return 0;
  }

  BOOL GetGammaCorrection(VOID) const
  {
    return FALSE;
  }

  INT GetInterpolationColorCount(VOID) const
  {
    return 0;
  }

  Status GetInterpolationColors(Color *presetColors, REAL *blendPositions, INT count) const
  {
    return NotImplemented;
  }

  Status GetLinearColors(Color* colors) const
  {
    return NotImplemented;
  }

  Status GetRectangle(Rect *rect) const
  {
    return NotImplemented;
  }

  Status GetRectangle(RectF* rect) const
  {
    return NotImplemented;
  }

  Status GetTransform(Matrix* matrix) const
  {
    return NotImplemented;
  }

  WrapMode GetWrapMode(VOID) const
  {
    return WrapModeTile;
  }

  Status MultiplyTransform(const Matrix *matrix, MatrixOrder order)
  {
    return NotImplemented;
  }

  Status ResetTransform(VOID)
  {
    return NotImplemented;
  }

  Status RotateTransform(REAL angle, MatrixOrder order)
  {
    return NotImplemented;
  }

  Status ScaleTransform(REAL sx, REAL sy, MatrixOrder order)
  {
    return NotImplemented;
  }

  Status SetBlend(const REAL *blendFactors, const REAL *blendPositions, INT count)
  {
    return NotImplemented;
  }

  Status SetBlendBellShape(REAL focus, REAL scale)
  {
    return NotImplemented;
  }

  Status SetBlendTriangularShape(REAL focus, REAL scale)
  {
    return NotImplemented;
  }

  Status SetGammaCorrection(BOOL useGammaCorrection)
  {
    return NotImplemented;
  }

  Status SetInterpolationColors(const Color *presetColors, const REAL *blendPositions, INT count)
  {
    return NotImplemented;
  }

  Status SetLinearColors(const Color& color1, const Color& color2)
  {
    return NotImplemented;
  }

  Status SetTransform(const Matrix* matrix)
  {
    return NotImplemented;
  }

  Status SetWrapMode(WrapMode wrapMode)
  {
    return NotImplemented;
  }

  Status TranslateTransform(REAL dx, REAL dy, MatrixOrder order)
  {
    return NotImplemented;
  }
};


class SolidBrush : Brush
{
public:
  SolidBrush(const Color &color)
  {
  }

  Status GetColor(Color *color) const
  {
    return NotImplemented;
  }

  Status SetColor(const Color &color)
  {
    return NotImplemented;
  }
};


class TextureBrush : Brush
{
public:
  TextureBrush(Image *image, WrapMode wrapMode, const RectF &dstRect)
  {
  }

  TextureBrush(Image *image, Rect &dstRect, ImageAttributes *imageAttributes)
  {
  }

  TextureBrush(Image *image, WrapMode wrapMode, INT dstX, INT dstY, INT dstWidth, INT dstHeight)
  {
  }

  TextureBrush(Image *image, WrapMode wrapMode, REAL dstX, REAL dstY, REAL dstWidth, REAL dstHeight)
  {
  }

  TextureBrush(Image *image, RectF &dstRect, ImageAttributes *imageAttributes)
  {
  }

  TextureBrush(Image *image, WrapMode wrapMode)
  {
  }

  TextureBrush(Image *image, WrapMode wrapMode, const Rect &dstRect)
  {
  }

  Image *GetImage(VOID) const
  {
    return NULL;
  }

  Status GetTransform(Matrix *matrix) const
  {
    return NotImplemented;
  }

  WrapMode GetWrapMode(VOID) const
  {
    return WrapModeTile;
  }

  Status MultiplyTransform(Matrix *matrix, MatrixOrder order)
  {
    return NotImplemented;
  }

  Status ResetTransform(VOID)
  {
    return NotImplemented;
  }

  Status RotateTransform(REAL angle, MatrixOrder order)
  {
    return NotImplemented;
  }

  Status ScaleTransform(REAL sx, REAL sy, MatrixOrder order)
  {
    return NotImplemented;
  }

  Status SetTransform(const Matrix *matrix)
  {
    return NotImplemented;
  }

  Status SetWrapMode(WrapMode wrapMode)
  {
    return NotImplemented;
  }

  Status TranslateTransform(REAL dx, REAL dy, MatrixOrder order)
  {
    return NotImplemented;
  }
};

#endif /* _GDIPLUSBRUSH_H */
