/*
 * GdiPlusPen.h
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

#ifndef _GDIPLUSPEN_H
#define _GDIPLUSPEN_H

class CustomLineCap;

class Pen : public GdiplusBase
{
  friend class Graphics;

public:
  Pen(const Brush *brush, REAL width = 1.0f)
  {
  }

  Pen(const Color &color, REAL width = 1.0f)
  {
    Unit unit = UnitWorld;
    pen = NULL;
    status = DllExports::GdipCreatePen1(color.GetValue(),
      width,
      unit,
      &pen);
  }

  Pen *Clone(VOID)
  {
    return NULL;
  }

  PenAlignment GetAlignment(VOID)
  {
    PenAlignment penAlignment;
    SetStatus(DllExports::GdipGetPenMode(pen, &penAlignment));
    return penAlignment;
  }

  Brush *GetBrush(VOID)
  {
    return NULL;
  }

  Status GetColor(Color *color)
  {
    ARGB argb;
    Status status = SetStatus(DllExports::GdipGetPenColor(pen, &argb));
    if (color)
      color->SetValue(argb);
    return status;
  }

  Status GetCompoundArray(REAL *compoundArray, INT count)
  {
    return NotImplemented;  // FIXME: not available: SetStatus(DllExports::GdipGetPenCompoundArray(pen, count));
  }

  INT GetCompoundArrayCount(VOID)
  {
    INT count;
    SetStatus(DllExports::GdipGetPenCompoundCount(pen, &count));
    return count;
  }

  Status GetCustomEndCap(CustomLineCap *customCap)
  {
    return NotImplemented;
  }

  Status GetCustomStartCap(CustomLineCap *customCap)
  {
    return NotImplemented;
  }

  DashCap GetDashCap(VOID)
  {
    DashCap dashCap;
    SetStatus(DllExports::GdipGetPenDashCap197819(pen, &dashCap));
    return dashCap;
  }

  REAL GetDashOffset(VOID)
  {
    REAL offset;
    SetStatus(DllExports::GdipGetPenDashOffset(pen, &offset));
    return offset;
  }

  Status GetDashPattern(REAL *dashArray, INT count)
  {
    return SetStatus(DllExports::GdipGetPenDashArray(pen, dashArray, count));
  }

  INT GetDashPatternCount(VOID)
  {
    INT count;
    SetStatus(DllExports::GdipGetPenDashCount(pen, &count));
    return count;
  }

  DashStyle GetDashStyle(VOID)
  {
    DashStyle dashStyle;
    SetStatus(DllExports::GdipGetPenDashStyle(pen, &dashStyle));
    return dashStyle;
  }

  LineCap GetEndCap(VOID)
  {
    LineCap endCap;
    SetStatus(DllExports::GdipGetPenEndCap(pen, &endCap));
    return endCap;
  }

  Status GetLastStatus(VOID)
  {
    return status;
  }

  LineJoin GetLineJoin(VOID)
  {
    LineJoin lineJoin;
    SetStatus(DllExports::GdipGetPenLineJoin(pen, &lineJoin));
    return lineJoin;
  }

  REAL GetMiterLimit(VOID)
  {
    REAL miterLimit;
    SetStatus(DllExports::GdipGetPenMiterLimit(pen, &miterLimit));
    return miterLimit;
  }

  PenType GetPenType(VOID)
  {
    PenType type;
    SetStatus(DllExports::GdipGetPenFillType(pen, &type));
    return type;
  }

  LineCap GetStartCap(VOID)
  {
    LineCap startCap;
    SetStatus(DllExports::GdipGetPenStartCap(pen, &startCap));
    return startCap;
  }

  Status GetTransform(Matrix *matrix)
  {
    return NotImplemented;
  }

  REAL GetWidth(VOID)
  {
    REAL width;
    SetStatus(DllExports::GdipGetPenWidth(pen, &width));
    return width;
  }

  Status MultiplyTransform(Matrix *matrix, MatrixOrder order)
  {
    return NotImplemented;  // FIXME: not available: SetStatus(DllExports::GdipMultiplyPenTransform(pen, matrix ? matrix->matrix : NULL, order));
  }

  Status ResetTransform(VOID)
  {
    return SetStatus(DllExports::GdipResetPenTransform(pen));
  }

  Status RotateTransform(REAL angle, MatrixOrder order)
  {
    return NotImplemented;  // FIXME: not available: SetStatus(DllExports::GdipRotatePenTransform(pen, angle, order));
  }

  Status ScaleTransform(REAL sx, REAL sy, MatrixOrder order)
  {
    return SetStatus(DllExports::GdipScalePenTransform(pen, sx, sy, order));
  }

  Status SetAlignment(PenAlignment penAlignment)
  {
    return SetStatus(DllExports::GdipSetPenMode(pen, penAlignment));
  }

  Status SetBrush(const Brush *brush)
  {
    return SetStatus(DllExports::GdipSetPenBrushFill(pen, brush ? brush->brush : NULL));
  }

  Status SetColor(const Color &color)
  {
    return SetStatus(DllExports::GdipSetPenColor(pen, color.GetValue()));
  }

  Status SetCompoundArray(const REAL *compoundArray, INT count)
  {
    return SetStatus(DllExports::GdipSetPenCompoundArray(pen, compoundArray, count));
  }

  Status SetCustomEndCap(const CustomLineCap *customCap)
  {
    return NotImplemented;
  }

  Status SetCustomStartCap(const CustomLineCap *customCap)
  {
    return NotImplemented;
  }

  Status SetDashCap(DashCap dashCap)
  {
    return SetStatus(DllExports::GdipSetPenDashCap197819(pen, dashCap));
  }

  Status SetDashOffset(REAL dashOffset)
  {
    return SetStatus(DllExports::GdipSetPenDashOffset(pen, dashOffset));
  }

  Status SetDashPattern(const REAL *dashArray, INT count)
  {
    return SetStatus(DllExports::GdipSetPenDashArray(pen, dashArray, count));
  }

  Status SetDashStyle(DashStyle dashStyle)
  {
    return SetStatus(DllExports::GdipSetPenDashStyle(pen, dashStyle));
  }

  Status SetEndCap(LineCap endCap)
  {
    return SetStatus(DllExports::GdipSetPenEndCap(pen, endCap));
  }

  Status SetLineCap(LineCap startCap, LineCap endCap, DashCap dashCap)
  {
    return SetStatus(DllExports::GdipSetPenLineCap197819(pen, startCap, endCap, dashCap));
  }

  Status SetLineJoin(LineJoin lineJoin)
  {
    return SetStatus(DllExports::GdipSetPenLineJoin(pen, lineJoin));
  }

  Status SetMiterLimit(REAL miterLimit)
  {
    return SetStatus(DllExports::GdipSetPenMiterLimit(pen, miterLimit));
  }

  Status SetStartCap(LineCap startCap)
  {
    return SetStatus(DllExports::GdipSetPenStartCap(pen, startCap));
  }

  Status SetTransform(const Matrix *matrix)
  {
    return SetStatus(DllExports::GdipSetPenTransform(pen, matrix ? matrix->matrix : NULL));
  }

  Status SetWidth(REAL width)
  {
    return SetStatus(DllExports::GdipSetPenWidth(pen, width));
  }

private:
  GpPen *pen;

private:
  mutable Status status;

  Status SetStatus(Status status) const
  {
    if (status == Ok)
      return status;
    this->status = status;
    return status;
  }
};

#endif /* _GDIPLUSPEN_H */
